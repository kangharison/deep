# NVMe 드라이버 큐 관리(Queue Management) 기능 설계

> 대상: 사내 custom Linux NVMe PCIe 드라이버 (런타임 SQ/CQ create/delete 를 이미 `ioctl` 로 지원 중)
> 레퍼런스 베이스라인: `linux-study/drivers/nvme/host/` (vanilla, kernel stable tree)
> 목표 기능: **(1) 가변 큐 개수 생성 / (2) 임의 SQ-CQ 매핑 / (3) 큐별 차등 queue depth / (4) namespace–SQ 바인딩 (§9)**
> 산출물 범위: 설계 문서 + vanilla `pci.c` 기준 패치 스케치(의사 diff)

---

## 0. 한눈에 보기 (TL;DR)

| 기능　　　　| vanilla 한계　　　　　　　　　　| 근본 원인(코드)　　　　　　　　　　　　　　　　　　　　　　　　　　　　　| 본 설계의 해법　　　　　　　　　　　　　　　　 |
| -------------| ---------------------------------| --------------------------------------------------------------------------| ------------------------------------------------|
| 큐 개수　　 | CPU 수에 묶임, 부팅 시 1회 협상 | `nvme_max_io_queues()` 상한 + `nvme_set_queue_count()` (pci.c:2941~2943) | ioctl 로 협상량 내에서 임의 개수/임의 qid 생성 |
| SQ-CQ 매핑　| SQ qid == CQ qid (1:1 고정)　　 | `adapter_alloc_sq()` 의 `create_sq.cqid = qid` (pci.c:1766)　　　　　　　| `cqid` 를 파라미터화 → N:1 매핑 허용　　　　　 |
| Queue depth | 전역 단일값, 모든 IO 큐 동일　　| `nvme_alloc_queue(dev, i, dev->q_depth)` (pci.c:2384)　　　　　　　　　　| 큐별 depth 인자를 ioctl 로 전달　　　　　　　　|
| NS–SQ 바인딩| 개념 자체가 없음 (전 NS 큐 공유)| `blk_mq_alloc_disk(ctrl->tagset, ...)` (core.c:4153) + CPU기반 hctx 선택 (pci.c:1408) | nsid→SQ집합 바인딩 테이블 + Tier B 제출 경로에서 강제 (§9) |

세 기능 모두 **NVMe 스펙(Create I/O Submission/Completion Queue, Set Features-Number of Queues)이 본래 허용**하는 동작이다. vanilla 드라이버가 단순화를 위해 "CPU당 1큐 + 1:1 매핑 + 균일 depth"로 고정했을 뿐이다. 따라서 컨트롤러 펌웨어 수정 없이 호스트 드라이버만으로 구현 가능하다.

> **단, "큐 생성"과 "큐 사용"은 별개 문제다.** ioctl 로 NVMe 컨트롤러 쪽 SQ/CQ 를 만드는 것만으로는, 상위 블록 계층(blk-mq)이 그 큐를 모른다. 일반 블록 IO 가 새 큐를 쓰게 하려면 **blk-mq 로 토폴로지 변경을 전파**해야 한다(`blk_mq_update_nr_hw_queues`). 이 전파의 비용·제약 때문에 큐를 **Tier A(blk-mq 통합)** 와 **Tier B(raw/passthrough)** 로 나눠 설계한다 — **§5 가 본 설계의 핵심**이다.

---

## 1. Vanilla 베이스라인 분석 (코드 근거)

세 기능을 어디에 손대야 하는지 정확히 짚기 위해, 현재 큐 생성 경로를 코드 라인과 함께 정리한다.

### 1.1 핵심 자료구조: `struct nvme_queue` (pci.c:365~394)

```c
struct nvme_queue {
    struct nvme_dev *dev;
    spinlock_t       sq_lock;
    void            *sq_cmds;        // SQ 엔트리 버퍼 (DMA)
    struct nvme_completion *cqes;    // CQ 엔트리 버퍼 (DMA)
    dma_addr_t       sq_dma_addr;
    dma_addr_t       cq_dma_addr;
    u32 __iomem     *q_db;           // 이 큐의 doorbell 레지스터 위치
    u32              q_depth;        // ★ 큐 깊이 (현재는 전부 dev->q_depth 복사)
    u16              cq_vector;      // ★ 이 큐가 묶인 MSI-X 인터럽트 벡터
    u16              sq_tail;
    u16              cq_head;
    u16              qid;            // ★ 큐 식별자 (SQ/CQ 공용 = 1:1 전제)
    u8               cq_phase;
    u8               sqes;           // SQ 엔트리 크기 (2^sqes 바이트)
    ...
};
```

주목할 점: 구조체 자체에는 **"이 SQ가 어느 CQ에 붙는가(cqid)"를 저장하는 필드가 없다.** `qid` 하나로 SQ/CQ를 동시에 가리킨다 — 이것이 1:1 매핑이 코드 전반에 박혀 있는 근본 이유다. (→ §3.2에서 `cqid` 필드 추가)

큐 메모리 크기 매크로 (pci.c:34~35):

```c
#define SQ_SIZE(q)  ((q)->q_depth << (q)->sqes)              // depth * 엔트리크기
#define CQ_SIZE(q)  ((q)->q_depth * sizeof(struct nvme_completion))
```

→ `q_depth` 가 큐별로 다르면 큐별 DMA 버퍼 크기도 자동으로 달라진다. **depth 차등화는 매크로 변경 없이 `q_depth` 필드만 큐별로 다르게 채우면 된다.**

### 1.2 큐 개수 결정 경로

```
nvme_setup_io_queues()                              // pci.c:2909
  ├─ nr_io_queues = min(nvme_max_io_queues(dev),    // pci.c:2941  ← CPU 수 기반 상한
  │                     dev->nr_allocated_queues-1)
  ├─ nvme_set_queue_count(&dev->ctrl, &nr_io_queues)// pci.c:2943  ← 컨트롤러와 개수 협상 (Set Features FID 0x07)
  ├─ nvme_setup_irqs(dev, nr_io_queues)             // pci.c:2997  ← MSI-X 벡터 할당
  └─ nvme_create_io_queues(dev)                     // pci.c:3019
        └─ for (i ...) nvme_create_queue(&dev->queues[i], i, polled)  // pci.c:2401
```

- `nvme_set_queue_count()` 이 컨트롤러에게 "IO 큐 N개 쓰겠다"를 통지하고, 펌웨어가 허용 가능한 개수를 회신한다. 이 **협상된 상한 안에서는 임의 개수의 SQ/CQ를 만들 수 있다.**
- vanilla 는 부팅/리셋 시 이 경로를 1회 실행하고 끝. 런타임 추가 생성 인터페이스가 없다.
- custom 드라이버는 여기에 ioctl 진입점을 추가해 둔 상태 (= "create io queue ioctl"). 본 설계는 **그 ioctl 을 확장**한다.

### 1.3 SQ-CQ 1:1 매핑이 박힌 지점

CQ 생성 (pci.c:1719~1740):

```c
static int adapter_alloc_cq(struct nvme_dev *dev, u16 qid,
                            struct nvme_queue *nvmeq, s16 vector)
{
    ...
    c.create_cq.cqid      = cpu_to_le16(qid);                 // pci.c:1734  CQ id = qid
    c.create_cq.qsize     = cpu_to_le16(nvmeq->q_depth - 1);  // pci.c:1735  0-based depth
    c.create_cq.irq_vector= cpu_to_le16(vector);              // pci.c:1737  이 CQ의 인터럽트 벡터
    ...
}
```

SQ 생성 (pci.c:1742~1769) — **여기가 핵심**:

```c
static int adapter_alloc_sq(struct nvme_dev *dev, u16 qid,
                            struct nvme_queue *nvmeq)
{
    ...
    c.create_sq.sqid  = cpu_to_le16(qid);                 // pci.c:1763  SQ id = qid
    c.create_sq.qsize = cpu_to_le16(nvmeq->q_depth - 1);  // pci.c:1764
    c.create_sq.cqid  = cpu_to_le16(qid);                 // pci.c:1766  ★★ SQ를 같은 qid의 CQ에 강제 바인딩
}
```

`create_sq.cqid = qid` — 이 한 줄이 "SQ는 반드시 같은 번호의 CQ에 붙는다"는 1:1 제약의 실체다. NVMe 스펙상 이 `cqid` 필드는 **임의의 (이미 생성된) CQ id** 를 가리킬 수 있다. 즉 SQ 3,4,5 를 모두 CQ 1 에 붙이는 것이 스펙상 합법이다. (→ §3.3에서 파라미터화)

벡터 할당도 1:1 전제 (pci.c:2201~2202):

```c
if (!polled)
    vector = dev->num_vecs == 1 ? 0 : qid;   // 벡터 == qid
```

### 1.4 Queue depth 가 전역 단일값인 지점

depth 결정 (pci.c:3188 부근, `nvme_pci_enable()`):

```c
dev->q_depth = min_t(u32, NVME_CAP_MQES(dev->ctrl.cap) + 1,   // 컨트롤러 최대 큐크기(MQES)
                     io_queue_depth);                          // 모듈 파라미터, 기본 1024
```

모듈 파라미터 (pci.c:108~110):

```c
static unsigned int io_queue_depth = 1024;
module_param_cb(io_queue_depth, &io_queue_depth_ops, &io_queue_depth, 0644);
MODULE_PARM_DESC(io_queue_depth, "set io queue depth, should >= 2 and < 4096");
```

모든 IO 큐 생성 시 동일 depth 주입 (pci.c:2384):

```c
for (i = dev->ctrl.queue_count; i <= dev->max_qid; i++)
    nvme_alloc_queue(dev, i, dev->q_depth);   // ★ 전부 dev->q_depth
```

`nvme_alloc_queue(dev, qid, depth)` (pci.c:2103) 는 이미 **depth 를 인자로 받도록 설계**되어 있다. 단지 호출부가 전역값만 넘길 뿐이다. → **depth 차등화는 호출부에서 큐별 값을 넘기기만 하면 된다.** (admin 큐는 `NVME_AQ_DEPTH`=32 로 이미 다른 값을 쓰고 있어, 차등 depth 자체는 검증된 동작이다.)

### 1.5 ioctl 진입점 (custom 확장 지점)

`/dev/nvmeX` 컨트롤러 캐릭터 디바이스 핸들러 (ioctl.c:844~875):

```c
long nvme_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct nvme_ctrl *ctrl = file->private_data;
    void __user *argp = (void __user *)arg;
    switch (cmd) {
    case NVME_IOCTL_ADMIN_CMD: ...
    case NVME_IOCTL_RESET:     ...
    /* ★ custom: 여기 아래에 큐 관리 ioctl 케이스를 추가 */
    default:
        return -ENOTTY;
    }
}
```

custom 드라이버의 "create io queue ioctl" 도 이 switch 에 케이스를 추가한 형태일 것이다. 본 설계는 동일 위치에 **큐 관리 명령군**을 추가한다.

---

## 2. 설계 목표 상세

### 목표 1 — 가변 큐 개수 생성
- 부팅 시 CPU 수만큼 자동 생성하는 정책과 **무관하게**, 사용자가 원하는 개수/원하는 qid 의 IO 큐를 런타임에 생성/삭제.
- 상한: `nvme_set_queue_count()` 로 협상된 개수 및 `dev->nr_allocated_queues`. 이를 초과하려면 리셋 후 재협상 필요(§6 제약 참고).

### 목표 2 — 임의 SQ-CQ 매핑
- 하나의 CQ 에 여러 SQ 를 붙이는 N:1 매핑 허용 (예: SQ 2·3·4 → CQ 1).
- 활용: 인터럽트/폴링 컨텍스트를 한 CQ 로 모아 인터럽트 수 절감, 또는 우선순위 큐를 별도 CQ 로 분리.

### 목표 3 — 큐별 차등 queue depth
- 큐마다 다른 depth (예: 지연민감 큐는 32, 대역폭 큐는 2048).
- 상한: `NVME_CAP_MQES(cap)+1`. 하한: 2.

---

## 3. 인터페이스 설계 (ioctl)

### 3.1 UAPI 구조체 및 명령 정의

새 헤더 `include/uapi/linux/nvme_queue_mgmt.h` (또는 custom 드라이버의 기존 uapi 헤더에 추가):

```c
/* 큐 생성 플래그 */
#define NVME_QMGMT_F_POLLED        (1u << 0)  /* 인터럽트 대신 폴링 큐로 생성 */
#define NVME_QMGMT_F_CQ_ONLY       (1u << 1)  /* CQ만 생성 (SQ는 별도 명령으로) */
#define NVME_QMGMT_F_SQ_ONLY       (1u << 2)  /* SQ만 생성 (기존 CQ에 매핑) */
#define NVME_QMGMT_F_BLKMQ_VISIBLE (1u << 3)  /* Tier A: blk-mq 에 전파해 일반 블록 IO 노출 (§5) */

/* 단일 큐 생성 요청 */
struct nvme_qmgmt_create {
    __u16 sqid;        /* 만들 SQ id. F_CQ_ONLY 면 무시 */
    __u16 cqid;        /* 이 SQ를 붙일 CQ id. ★ 임의 매핑의 핵심 */
    __u16 cq_depth;    /* CQ 깊이(엔트리 수). F_SQ_ONLY 면 무시 */
    __u16 sq_depth;    /* SQ 깊이(엔트리 수). ★ 큐별 차등 depth */
    __u16 irq_vector;  /* CQ가 쓸 MSI-X 벡터 index */
    __u16 flags;       /* NVME_QMGMT_F_* */
    __u32 rsvd;
};

/* 단일 큐 삭제 요청 */
struct nvme_qmgmt_delete {
    __u16 qid;         /* 삭제할 큐 id */
    __u16 is_cq;       /* 1=CQ 삭제, 0=SQ 삭제. (스펙상 SQ를 먼저 지워야 CQ 삭제 가능) */
    __u32 rsvd;
};

/* 현재 큐 토폴로지 조회 */
struct nvme_qmgmt_info_entry {
    __u16 qid;
    __u16 cqid;        /* SQ가 매핑된 CQ. CQ 엔트리면 자기 자신 */
    __u16 depth;
    __u16 cq_vector;
    __u8  type;        /* 0=SQ, 1=CQ */
    __u8  online;
    __u16 rsvd;
};

struct nvme_qmgmt_info {
    __u32 nr_entries;          /* in: 버퍼 용량 / out: 실제 큐 수 */
    __u32 max_io_queues;       /* out: 협상된 상한 */
    __u16 max_qid_supported;   /* out */
    __u16 mqes;                /* out: 컨트롤러 최대 큐 크기(0-based+1) */
    __u64 entries_ptr;         /* nvme_qmgmt_info_entry[] 사용자 버퍼 포인터 */
};

/* ioctl 명령 번호 — 기존 'N' 매직과 충돌하지 않는 서브코드 사용 */
#define NVME_IOCTL_QMGMT_CREATE  _IOWR('N', 0x80, struct nvme_qmgmt_create)
#define NVME_IOCTL_QMGMT_DELETE  _IOW ('N', 0x81, struct nvme_qmgmt_delete)
#define NVME_IOCTL_QMGMT_INFO    _IOWR('N', 0x82, struct nvme_qmgmt_info)
```

> 매직 서브코드 `0x80~0x82` 는 vanilla 가 쓰는 `0x40~0x49` 대역과 겹치지 않게 선택. custom 드라이버의 기존 큐 생성 ioctl 번호를 확인해 충돌 없는 값으로 최종 확정할 것.

### 3.2 사용 시나리오 (유저스페이스)

```c
int fd = open("/dev/nvme0", O_RDWR);

/* (1) CQ 1을 depth 256, 벡터 1로 생성 */
struct nvme_qmgmt_create cq = {
    .cqid = 1, .cq_depth = 256, .irq_vector = 1,
    .flags = NVME_QMGMT_F_CQ_ONLY,
};
ioctl(fd, NVME_IOCTL_QMGMT_CREATE, &cq);

/* (2) SQ 2, 3, 4를 모두 CQ 1에 매핑(N:1), 각기 다른 depth */
for (int s = 2; s <= 4; s++) {
    struct nvme_qmgmt_create sq = {
        .sqid = s, .cqid = 1,
        .sq_depth = (s == 2) ? 32 : 1024,   /* ★ 큐별 차등 depth */
        .flags = NVME_QMGMT_F_SQ_ONLY,
    };
    ioctl(fd, NVME_IOCTL_QMGMT_CREATE, &sq);
}

/* (3) 토폴로지 확인 */
struct nvme_qmgmt_info_entry buf[64];
struct nvme_qmgmt_info info = { .nr_entries = 64, .entries_ptr = (__u64)buf };
ioctl(fd, NVME_IOCTL_QMGMT_INFO, &info);
```

---

## 4. 패치 스케치 (vanilla `pci.c` 기준 의사 diff)

> custom 드라이버 소스가 별도이므로, 아래는 **vanilla `pci.c` 를 기준으로 한 변경 지점 설명**이다. custom 쪽의 동등 함수에 같은 취지로 이식한다.

### 4.1 `struct nvme_queue` 에 `cqid` 필드 추가 (1:1 전제 해체)

```diff
 struct nvme_queue {
     ...
     u32  q_depth;
     u16  cq_vector;
     u16  sq_tail;
     u16  cq_head;
     u16  qid;
+    u16  cqid;     /* 이 SQ가 매핑된 CQ id. CQ 큐면 == qid. ★ N:1 매핑 지원 */
     u8   cq_phase;
     ...
 };
```

`qid` 만으로 SQ/CQ 를 동시에 가리키던 가정을 깨고, SQ 가 **어느 CQ 의 cqes/doorbell 을 쓰는지**를 명시적으로 보유한다. 완료 처리(`nvme_poll`/IRQ 핸들러)는 CQ 큐 객체 기준으로 동작하므로, SQ-전용 큐 객체는 `cqid` 로 해당 CQ 객체를 찾아 doorbell·cqes 를 공유하게 한다.

### 4.2 `adapter_alloc_sq()` 의 cqid 파라미터화 (목표 2의 핵심)

```diff
-static int adapter_alloc_sq(struct nvme_dev *dev, u16 qid,
-                            struct nvme_queue *nvmeq)
+static int adapter_alloc_sq(struct nvme_dev *dev, u16 qid,
+                            struct nvme_queue *nvmeq, u16 cqid)
 {
     ...
     c.create_sq.sqid  = cpu_to_le16(qid);
     c.create_sq.qsize = cpu_to_le16(nvmeq->q_depth - 1);
-    c.create_sq.cqid  = cpu_to_le16(qid);          /* 1:1 강제 */
+    c.create_sq.cqid  = cpu_to_le16(cqid);         /* ★ 임의 CQ에 매핑 */
     ...
 }
```

호출부(`nvme_create_queue`, pci.c:2210)는 자동 경로에서 `cqid = qid` 를 넘겨 기존 동작 보존. ioctl 경로에서는 사용자가 지정한 `cqid` 를 넘긴다.

> 제약: `create_sq.cqid` 가 가리키는 CQ 는 **반드시 먼저 생성**되어 있어야 한다(스펙 요구). ioctl 핸들러에서 대상 CQ 의 `NVMEQ_ENABLED` 비트를 검사한다.

### 4.3 큐별 차등 depth — 이미 인자화된 경로 활용 (목표 3)

`nvme_alloc_queue(dev, qid, depth)` (pci.c:2103) 는 그대로 사용한다. ioctl 핸들러가 사용자 지정 `sq_depth`/`cq_depth` 를 검증 후 넘긴다:

```c
/* depth 검증: 2 <= depth <= MQES+1 */
u16 mqes = NVME_CAP_MQES(dev->ctrl.cap) + 1;
if (req.sq_depth < 2 || req.sq_depth > mqes)
    return -EINVAL;
```

`SQ_SIZE`/`CQ_SIZE` 매크로(pci.c:34)가 `q_depth` 를 참조하므로 DMA 버퍼 크기는 자동으로 큐별 차등 적용된다. **매크로/할당 로직 변경 불필요.**

### 4.4 ioctl 핸들러 — 신규 함수 (custom 의 기존 create-queue ioctl 과 동형)

`pci.c` 에 컨트롤러 콜백으로 노출하거나, custom 의 기존 디스패치에 추가:

```c
static int nvme_ioctl_qmgmt_create(struct nvme_dev *dev,
                                   struct nvme_qmgmt_create __user *uarg)
{
    struct nvme_qmgmt_create req;
    struct nvme_queue *nvmeq, *cqq;
    int ret, vector;

    if (!capable(CAP_SYS_ADMIN))                 /* 권한 검사 (RESET ioctl 과 동일 정책) */
        return -EACCES;
    if (copy_from_user(&req, uarg, sizeof(req))) /* ioctl.c:213 패턴 */
        return -EFAULT;

    mutex_lock(&dev->shutdown_lock);             /* 큐 토폴로지 변경 직렬화 */

    /* ── CQ 생성 분기 ───────────────────────────────── */
    if (req.flags & NVME_QMGMT_F_CQ_ONLY) {
        if (req.cqid == 0 || req.cqid > dev->max_qid) { ret = -EINVAL; goto out; }
        if (test_bit(NVMEQ_ENABLED, &dev->queues[req.cqid].flags)) {
            ret = -EEXIST; goto out;             /* 이미 존재 */
        }
        ret = nvme_alloc_queue(dev, req.cqid, req.cq_depth);   /* pci.c:2103, ★차등 depth */
        if (ret) goto out;
        nvmeq = &dev->queues[req.cqid];
        nvmeq->cqid = req.cqid;                                /* CQ는 자기 자신 */
        ret = adapter_alloc_cq(dev, req.cqid, nvmeq, req.irq_vector); /* pci.c:1719 */
        if (ret) goto out;
        nvmeq->cq_vector = req.irq_vector;
        nvme_init_queue(nvmeq, req.cqid);                      /* pci.c:2152 */
        if (!(req.flags & NVME_QMGMT_F_POLLED))
            ret = queue_request_irq(nvmeq);                    /* pci.c:2138 */
        set_bit(NVMEQ_ENABLED, &nvmeq->flags);
        goto out;
    }

    /* ── SQ 생성 분기 (임의 CQ에 매핑) ──────────────── */
    cqq = &dev->queues[req.cqid];
    if (!test_bit(NVMEQ_ENABLED, &cqq->flags)) { /* 매핑 대상 CQ가 먼저 있어야 함 */
        ret = -ENOENT; goto out;
    }
    if (req.sqid == 0 || req.sqid > dev->max_qid) { ret = -EINVAL; goto out; }

    ret = nvme_alloc_queue(dev, req.sqid, req.sq_depth);  /* ★ 큐별 depth */
    if (ret) goto out;
    nvmeq = &dev->queues[req.sqid];
    nvmeq->cqid = req.cqid;                               /* ★ N:1 매핑 기록 */
    ret = adapter_alloc_sq(dev, req.sqid, nvmeq, req.cqid); /* §4.2 변경 함수 */
    if (ret) goto out;
    set_bit(NVMEQ_ENABLED, &nvmeq->flags);

out:
    mutex_unlock(&dev->shutdown_lock);
    return ret;
}
```

삭제 핸들러는 기존 `adapter_delete_sq()`/`adapter_delete_cq()` (pci.c:1776/1771) 를 그대로 호출하되, **SQ 를 먼저, CQ 를 나중에** 삭제하는 스펙 순서를 강제한다. 한 CQ 에 매핑된 SQ 가 남아 있으면 CQ 삭제를 거부(`-EBUSY`)한다.

### 4.5 디스패치 등록 (ioctl.c:851 switch 또는 custom 디스패치)

```diff
 long nvme_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
 {
     ...
     switch (cmd) {
     case NVME_IOCTL_RESET: ...
+    case NVME_IOCTL_QMGMT_CREATE:
+        return nvme_pci_qmgmt_create(ctrl, argp);   /* PCIe transport 전용 콜백 */
+    case NVME_IOCTL_QMGMT_DELETE:
+        return nvme_pci_qmgmt_delete(ctrl, argp);
+    case NVME_IOCTL_QMGMT_INFO:
+        return nvme_pci_qmgmt_info(ctrl, argp);
     default:
         return -ENOTTY;
     }
 }
```

> `nvme_dev_ioctl` 은 transport 공용(ioctl.c). PCIe 전용 동작이므로 `struct nvme_ctrl_ops` 에 콜백을 추가하거나, custom 드라이버가 이미 갖고 있는 PCIe 전용 ioctl 경로에 직접 케이스를 추가한다.

---

## 5. blk-mq 전파(propagation) 설계 — 런타임 토폴로지 변경의 정합성

> **이 섹션이 본 설계의 핵심 난점이다.** ioctl 로 NVMe 컨트롤러 쪽 SQ/CQ 만 만들고 끝내면, 상위 블록 계층(blk-mq)은 부팅 시점의 옛 토폴로지를 그대로 믿고 IO 를 라우팅한다 → 정합성 붕괴. "큐 생성"은 ioctl 로 충분하지만, **그 큐를 일반 블록 IO 가 쓰게 하려면 blk-mq 에 반드시 전파해야 한다.**

### 5.1 blk-mq 가 캐싱하고 있는 3가지 가정

blk-mq 는 tagset 할당(`nvme_alloc_io_tag_set`, core.c:4938) / `blk_mq_update_nr_hw_queues` 시점의 큐 토폴로지를 아래 형태로 굳혀 둔다. ioctl 로 NVMe 쪽만 바꾸면 셋 다 어긋난다.

**(1) hctx 개수 = `nr_hw_queues`** — core.c:4961

```c
set->nr_hw_queues = ctrl->queue_count - 1;   /* IO 큐 개수만큼 hctx 생성 */
```
→ 새 SQ 를 만들어도 hctx 개수가 그대로면 **어떤 hctx 도 그 큐로 IO 를 보내지 않는다** (일반 블록 IO 로는 사용 불가).

**(2) hctx_idx → qid 선형 매핑 (가장 까다로움)** — pci.c:655~658, 633~637

```c
nvme_init_hctx(hctx, data, hctx_idx)
  → nvme_init_hctx_common(hctx, data, hctx_idx + 1)     // pci.c:658
        struct nvme_queue *nvmeq = &dev->queues[qid];    // pci.c:633  ★ qid = hctx_idx + 1 하드 가정
        tags = dev->tagset.tags[qid - 1];                // pci.c:637  tags 배열도 qid-1 인덱싱
        hctx->driver_data = nvmeq;                        // pci.c:644  이후 모든 IO가 이 nvmeq로
```
blk-mq 의 hctx N 번은 **무조건** `dev->queues[N+1]` 에 바인딩된다 (연속·무결손 전제). IO 제출 시 `nvmeq = req->mq_hctx->driver_data` (pci.c:769 등)로 라우팅되므로, ioctl 로 **구멍 있는 qid** 나 **N:1 매핑**을 만들면 이 선형식이 깨진다.

**(3) 균일 tag depth** — core.c:4946

```c
set->queue_depth = min_t(unsigned, ctrl->sqsize, BLK_MQ_MAX_DEPTH - 1);  /* tagset 전체 단일값 */
```
태그 수 = in-flight 상한. **tagset 전체가 하나의 depth** 를 공유한다. 큐별 depth 를 다르게 주면, 깊은 큐 기준으로 태그가 발급될 때 얕은 SQ 가 오버런(`sq_tail` 이 `cq_head` 추월)된다.

### 5.2 전파를 안 하면 생기는 구체적 고장

| 토폴로지 변경 | blk-mq 미전파 시 증상　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　 |
| ---------------| -------------------------------------------------------------------------------------------|
| SQ 삭제　　　 | hctx 가 여전히 그 큐로 라우팅 → 이미 `dma_free` 된 큐에 제출 → use-after-free, crash/hang |
| SQ 추가　　　 | 어떤 hctx 도 매핑 안 됨 → 일반 블록 IO 로 사용 불가 (passthrough 로만 접근 가능)　　　　　|
| depth 축소　　| blk-mq 가 태그 과다 발급 → SQ tail 이 head 추월, 완료 유실/덮어쓰기　　　　　　　　　　　 |
| N:1 매핑　　　| hctx↔nvmeq 1:1 전제 위반 → 완료 경로가 잘못된 CQ 객체 참조　　　　　　　　　　　　　　　　|

### 5.3 전파 메커니즘 — `blk_mq_update_nr_hw_queues()`

핵심 API. vanilla 도 리셋 경로(`nvme_pci_update_nr_queues`, pci.c:3133~3155)에서 사용한다.

```c
/* pci.c:3151 — shutdown_lock 보유 상태에서 호출 */
blk_mq_update_nr_hw_queues(&dev->tagset, dev->online_queues - 1);
nvme_free_queues(dev, dev->online_queues);   /* 더 이상 안 쓰는 큐 회수 */
```

이 함수 한 번이 내부적으로 (구현: `__blk_mq_update_nr_hw_queues`, block/blk-mq.c:5136~5235):
1. `update_nr_hwq_lock`(rwsem) + `tag_list_lock` 을 **함수가 직접 획득** (blk-mq.c:5239~5240)
2. 새 태그 배열 prealloc (freeze 前, blk-mq.c:5179)
3. tagset 의 **모든 request_queue 를 freeze** = 진행 중 IO drain 완료까지 대기 + 신규 IO 차단 (`blk_mq_freeze_queue_nomemsave`, blk-mq.c:5183~5185)
  1. (freeze 완료 後) hctx 재할당 + `->map_queues`(=`nvme_pci_map_queues`, pci.c:681) + `->init_hctx`(=`nvme_init_hctx`, pci.c:655) 재실행으로 CPU↔hctx↔nvmeq 재매핑 (blk-mq.c:5193~5209) 
4. **unfreeze** (`blk_mq_elv_switch_back`, blk-mq.c:5216)

#### ★ freeze 는 호출자가 하지 않는다 (중요)

"freeze 완료 후 진행해야 하나?" → **함수가 내부에서 freeze→drain→remap→unfreeze 를 원자적으로 처리하므로, 호출자가 미리 freeze 하면 안 된다.**

- 함수가 freeze ref 를 +1 하고 unfreeze 는 `switch_back` 에서 **정확히 1회** 수행한다(blk-mq.c:5183, 5216). 호출자가 사전 freeze 하면 ref 불균형 → **큐 영구 freeze → IO 영구 hang**.
- blk-mq 락도 함수가 직접 잡으므로(blk-mq.c:5239~5240), 호출자는 이 락들을 **잡지 않은 상태**여야 한다. 호출자가 들고 있어도 되는 것은 드라이버 사설 락(`dev->shutdown_lock`)뿐 — vanilla 도 pci.c:3142 에서 그렇게 한다.

#### ★ 단, NVMe 큐 alloc/free 와의 **선후 순서**는 호출자가 보장해야 한다

freeze 자체는 함수에 맡기되, 하드웨어 큐 작업과 전파의 순서는 직접 맞춰야 한다.

**삭제**: `전파 → 삭제` (vanilla pci.c:3151~3153 동일)
```c
blk_mq_update_nr_hw_queues(&dev->tagset, online-1); /* ① hctx 분리 + freeze 로 in-flight drain */
nvme_free_queues(dev, online);                       /* ② 그 후에 adapter_delete_sq/cq + DMA free */
```
순서를 뒤집으면 drain 전에 큐 메모리를 해제 → in-flight IO 가 free 된 큐 참조 → **use-after-free**.

**생성**: `생성 → 전파`
```c
nvme_alloc_queue + adapter_alloc_cq/sq + set NVMEQ_ENABLED; /* ① NVMe 큐를 먼저 만들고 online */
blk_mq_update_nr_hw_queues(&dev->tagset, new_count);        /* ② 그 후 전파 */
```
이유: 전파가 내부에서 `nvme_init_hctx`(pci.c:655)→`nvmeq = &dev->queues[qid]` 바인딩을 하므로, 그 큐가 **이미 alloc + enabled** 되어 있어야 한다.

→ 따라서 **ioctl create/delete 핸들러는 위 선후 순서를 지켜 NVMe 큐 작업과 `io_queues[]`·`online_queues`·`ctrl->queue_count` 갱신을 한 뒤, `shutdown_lock` 만 잡은 채(사전 freeze 없이) `blk_mq_update_nr_hw_queues()` 를 호출**한다.

```diff
 static int nvme_ioctl_qmgmt_create(struct nvme_dev *dev, ...)
 {
     ...
     /* NVMe Create CQ/SQ admin 명령 수행 (§4.4) */
     set_bit(NVMEQ_ENABLED, &nvmeq->flags);
+
+    /* ★ blk-mq 전파: 새 큐가 블록 IO 라우팅에 반영되도록 */
+    if (req.flags & NVME_QMGMT_F_BLKMQ_VISIBLE) {
+        dev->io_queues[HCTX_TYPE_DEFAULT] = /* 갱신된 default 큐 수 */;
+        blk_mq_update_nr_hw_queues(&dev->tagset, dev->online_queues - 1);
+    }
 out:
     mutex_unlock(&dev->shutdown_lock);
     return ret;
 }
```

> 주의: `blk_mq_update_nr_hw_queues` 는 **전체 IO freeze 를 수반하는 무거운 연산**이다. 큐 하나 추가할 때마다 호출하면 IO 가 매번 멈춘다. → 여러 큐를 만들 때는 batch 생성 후 1회만 전파하는 인터페이스(아래 §5.5)를 권장.

### 5.4 두 가지 운용 모드 (Tier A / Tier B)

전파의 비용·제약 때문에 단일 정책으로는 세 목표를 모두 만족시킬 수 없다. **큐를 두 부류로 명시 구분**한다 — 생성 ioctl 의 플래그(`NVME_QMGMT_F_BLKMQ_VISIBLE`)로 선택.

| 구분 | **Tier A: blk-mq 통합 큐** | **Tier B: raw / passthrough 큐** |
|------|---------------------------|----------------------------------|
| 용도 | 파일시스템·bio 등 일반 블록 IO | 직접 제출 (io_uring passthrough, 전용 벤치) |
| blk-mq 전파 | `blk_mq_update_nr_hw_queues` **필수** | blk-mq 에 비노출 → 전파 **불필요** |
| qid 제약 | **연속·무결손** 강제 (또는 §5.6 indirection) | 임의 qid 자유 |
| N:1 SQ-CQ 매핑 | 제약 큼 (hctx 1:1 전제) | **완전 자유** |
| 큐별 차등 depth | tagset depth ≤ **최소 SQ depth** 로 제약 | **완전 자유** |
| 비용 | 변경마다 전체 IO freeze | 없음 |
| 목표 2·3 자유도 | 낮음 | 높음 |

**권장 전략**:
- **목표 2(임의 매핑)·목표 3(큐별 depth)** 의 완전한 자유도는 **Tier B** 에서 제공 (spdk-스타일 직접 제출, io_uring `IORING_OP_URING_CMD` passthrough 가 특정 큐로 바인딩).
- **목표 1(가변 개수)** 중 "일반 블록 IO 로 쓰는 큐"는 **Tier A** 로, 전파를 거쳐 blk-mq 에 반영.
- custom 드라이버의 기존 create-queue ioctl 이 어느 쪽인지부터 확인해 정책을 일치시킨다.

### 5.5 depth 전파의 한계와 처리

blk-mq 의 하드웨어 태그 수(`set->queue_depth`)는 **tagset 할당 시 고정**되며, 런타임에 바꾸려면 tagset 재할당이 필요하다. (`/sys/block/X/queue/nr_requests` 로 만지는 `blk_mq_update_nr_requests` 는 스케줄러 depth 일 뿐 하드웨어 태그 수가 아니다.)

- **Tier A**: 모든 hctx 가 단일 `queue_depth` 를 공유하므로, 큐별 depth 를 다르게 하려면 **tagset depth 를 노출된 SQ 들의 최소 depth 로 설정**해야 안전하다(깊은 큐는 손해). 진짜 큐별 차등이 필요하면 tagset 을 depth 클래스별로 분리하는 고급 설계가 필요(권장하지 않음, 복잡도 과다).
- **Tier B**: blk-mq 태그와 무관 → 큐별 depth 완전 자유. **큐별 차등 depth 의 주 무대.**

### 5.6 (선택) 임의 qid·N:1 을 Tier A 에서도 쓰려면: indirection 테이블

연속 qid 전제를 깨려면 `hctx_idx → 실제 qid` 매핑 배열을 도입하고 `nvme_init_hctx` 를 수정해야 한다.

```diff
+/* dev 에 추가: blk-mq hctx 인덱스 → 실제 NVMe qid 매핑 */
+u16 hctx_to_qid[NVME_MAX_QUEUES];

 static int nvme_init_hctx(struct blk_mq_hw_ctx *hctx, void *data, unsigned hctx_idx)
 {
-    return nvme_init_hctx_common(hctx, data, hctx_idx + 1);
+    struct nvme_dev *dev = to_nvme_dev(data);
+    return nvme_init_hctx_common(hctx, data, dev->hctx_to_qid[hctx_idx]);
 }
```
단, `nvme_pci_map_queues`(pci.c:681)의 CPU↔hctx 매핑과 IRQ affinity 도 함께 손봐야 하며, N:1 매핑 시 완료 경로가 §4.1 의 `nvmeq->cqid` 로 CQ 소유 객체를 찾아 cqes/doorbell 을 공유해야 한다. **복잡도가 높아 1차 범위에서는 제외**하고, 임의 매핑은 Tier B 로 우회하는 것을 권장.

### 5.7 활성 IO 중 전파 동작 — freeze/drain 타임라인 (★ 운영상 가장 중요)

**핵심 질문: fio 등으로 IO 가 계속 내려오는 중에 `blk_mq_update_nr_hw_queues` 가 불리면?**
→ **"순간 정지(freeze) → in-flight drain → 재매핑 → 재개"** 로 동작한다. 데이터 유실은 없으나 IO 가 잠깐 멈춘다.

메커니즘은 `q->q_usage_counter`(percpu_ref). 평상시 fio 제출은 `submit_bio → __bio_queue_enter()` 가 이 카운터를 +1 (blk-core.c:333), 완료 시 `blk_queue_exit()` 가 −1 (blk-core.c:369).

```
t0  fio 정상 가동 — q_usage_counter = 현재 in-flight 수

t1  blk_mq_update_nr_hw_queues 진입
      └─ percpu_ref_kill(&q->q_usage_counter)              // blk-mq.c:171  freeze 시작
         ★ 이 순간부터 fio의 "신규" bio:
            __bio_queue_enter() → blk_try_enter_queue() 실패
              → wait_event(q->mq_freeze_wq, !mq_freeze_depth)// blk-core.c:353  ★ 에러 아닌 sleep(park)
            예외) REQ_NOWAIT(io_uring nowait 등) → -EAGAIN 즉시 반환  // blk-core.c:338

t2  blk_mq_freeze_queue_wait()                              // blk-mq.c:191
      └─ wait_event(mq_freeze_wq, percpu_ref_is_zero(counter))
           ★ 이미 제출된 in-flight 가 "살아있는 큐에서" 전부 완료될 때까지 대기 (drain)

t3  drain 완료(counter==0) → hctx 재할당 + map_queues + init_hctx 재바인딩  // blk-mq.c:5193~5209

t4  blk_mq_elv_switch_back → unfreeze
      └─ percpu_ref_resurrect + wake_up_all(&q->mq_freeze_wq)// blk-mq.c:221~222
         ★ t1에서 park 됐던 fio bio 깨어나 재진입 → "새 토폴로지"의 큐로 제출
```

**fio 체감 결과**

| 항목 | 결과 |
|------|------|
| 처리량 | t1~t4 동안 **순간 0** (IO 일시 정지) |
| 지연 | 그 구간에 걸린 IO 는 **완료 지연 spike** (park 시간만큼, 보통 sub-ms~수 ms) |
| 에러/유실 | **없음** (blocking 제출: psync/libaio/io_uring 기본). 재매핑 후 정상 재개 |
| 예외 | `REQ_NOWAIT` 계열은 그 창에서 **-EAGAIN** → fio 가 재시도/에러 카운트 |
| 재개 후 | CPU→큐 매핑이 바뀌었으면 같은 fio 스레드가 **다른 SQ** 로 제출 |

**함정 2가지 (반드시 인지)**

1. **tagset 전체가 함께 freeze**: `list_for_each_entry(q, &set->tag_list, ...)` (blk-mq.c:5183) 로 **그 tagset 을 공유하는 모든 namespace** 가 같이 멈춘다. nvme0n1·nvme0n2 가 `dev->tagset` 공유 시, n2 재구성이 n1 의 fio 까지 정지시킨다.
2. **drain 무한 대기 → ioctl hang**: `blk_mq_freeze_queue_wait` 는 in-flight 0 까지 무한 대기(blk-mq.c:191). **SQ 를 먼저 삭제하고** 전파하면 그 SQ 의 in-flight 가 영영 완료 못 함 → drain 미완 → **ioctl 이 D-state 영구 hang**. → §5.3 의 `삭제: 전파(drain) → 삭제` 순서는 데드락 회피 **필수 조건**. fio 부하가 높을수록 drain 이 길어지므로 **batch 1회 전파** 또는 저부하 시점 수행을 권장.

---

## 6. 동기화·제약·엣지케이스

| 항목 | 설계 결정 |
|------|----------|
| 토폴로지 변경 직렬화 | `dev->shutdown_lock` 보유 후 create/delete 수행 (vanilla `nvme_create_queue` 가 `nvme_setup_io_queues_trylock` 으로 같은 락 사용, pci.c:2218) |
| 협상 상한 초과 | `dev->max_qid`/`nr_allocated_queues` 초과 요청은 `-EINVAL`. 더 늘리려면 컨트롤러 리셋 후 `nvme_set_queue_count` 재협상 필요 |
| CQ 선행 생성 | SQ 매핑 대상 CQ 의 `NVMEQ_ENABLED` 미설정 시 `-ENOENT` |
| 삭제 순서 | CQ 에 살아있는 SQ 가 있으면 CQ 삭제 `-EBUSY`. SQ→CQ 순서 강제 (NVMe 스펙 요구) |
| blk-mq tagset 정합성 | **§5 참조.** Tier A(블록 IO 노출) 큐는 `blk_mq_update_nr_hw_queues` 전파 필수, Tier B(raw/passthrough) 큐는 전파 불필요. 임의 매핑·큐별 depth 의 자유도는 Tier B 에서 제공 |
| 인터럽트 벡터 공유 | N:1 매핑 시 여러 SQ 완료가 같은 CQ→같은 벡터로 모임. `cq_vector` 중복 등록 방지(CQ 1회만 `queue_request_irq`) |
| depth 경계 | `2 <= depth <= NVME_CAP_MQES(cap)+1`. CMB 사용 시 `nvme_cmb_qdepth()`(pci.c:2055) 상한 추가 검사 |
| 폴링 큐 | `NVME_QMGMT_F_POLLED` 시 IRQ 미등록, `NVMEQ_POLLED` 비트 설정 (pci.c:2204) |

---

## 7. 검증 계획

1. **생성/매핑 확인**
   - `NVME_IOCTL_QMGMT_INFO` 로 토폴로지 덤프 → SQ 2·3·4 의 `cqid == 1` 확인.
   - `nvme get-feature -f 0x7` (Number of Queues) 로 협상 개수 교차 확인.
2. **차등 depth 확인**
   - 큐별 `SQ_SIZE`/`CQ_SIZE` DMA 할당량을 `dev_dbg` 로 로깅, 요청 depth 와 일치 확인.
   - 작은 depth 큐에 깊은 큐 이상으로 명령 주입 → SQ full(`sq_tail` wrap) 백프레셔 동작 확인 (pci.c:735 `++sq_tail == q_depth`).
3. **N:1 완료 정합성**
   - 서로 다른 SQ 로 제출한 명령의 완료가 공유 CQ 에서 올바른 `command_id`/`sqid` 로 구분 회수되는지 확인 (CQE 의 SQ Identifier 필드).
4. **에러 경로**
   - 없는 CQ 에 SQ 매핑 → `-ENOENT`.
   - 살아있는 SQ 보유 CQ 삭제 → `-EBUSY`.
   - depth=0/1, depth>MQES+1 → `-EINVAL`.
   - 비-CAP_SYS_ADMIN 호출 → `-EACCES`.
5. **blk-mq 전파 정합성 (Tier A)**
   - Tier A 큐 생성/삭제 후 `/sys/block/nvme0n1/mq/` 의 hctx 개수가 변경되는지 확인.
   - 생성한 큐로 실제 `fio --ioengine=io_uring` IO 가 분산되는지 (큐별 카운터 / `bpftrace` 로 `nvme_queue_rq` 의 `nvmeq->qid` 추적).
   - 삭제한 큐로 IO 가 더 이상 라우팅되지 않는지 (use-after-free 부재) — `KASAN` 빌드로 검증.
6. **스트레스/안정성**
   - 생성↔삭제 반복 루프 중 `fio` IO 부하 → leak(`dma_alloc_coherent` 누수)·해제 순서 오류 점검.
   - 컨트롤러 리셋(`NVME_IOCTL_RESET`)과 동시 호출 → `shutdown_lock` 경합 정상 처리 확인.
   - Tier A 전파 중(`blk_mq_update_nr_hw_queues` freeze 구간) IO hang/타임아웃 부재 확인.

---

## 8. 단계별 구현 로드맵

| 단계 | 내용 | 산출물 |
|------|------|--------|
| 0 | custom 드라이버의 기존 create-queue ioctl 번호·구조체·디스패치 경로 확인 + 그 큐가 Tier A/B 중 무엇인지 판별 | 충돌 없는 ioctl 매직, Tier 정책 확정 |
| 1 | `struct nvme_queue` 에 `cqid` 추가 + `adapter_alloc_sq()` cqid 파라미터화 | §4.1, §4.2 |
| 2 | `NVME_IOCTL_QMGMT_CREATE`(CQ/SQ) 핸들러 + 차등 depth (**Tier B 먼저** — 전파 없이 검증 단순) | §4.4 |
| 3 | `NVME_IOCTL_QMGMT_DELETE` + 삭제 순서/refcount | §6 |
| 4 | `NVME_IOCTL_QMGMT_INFO` 토폴로지 조회 | §3.1 |
| 5 | **Tier A blk-mq 전파** — `blk_mq_update_nr_hw_queues` 연동, `io_queues[]` 갱신, depth=min 정책 | §5.3, §5.5 |
| 6 | 유저스페이스 테스트 도구(open/ioctl 래퍼) + 검증 시나리오 | §7 |
| 7 | (선택) Tier A 에서 임의 qid·N:1 매핑 — `hctx_to_qid` indirection | §5.6 |
| 8 | **NS–SQ 바인딩 테이블** + `BIND`/`UNBIND` ioctl + 검증 규칙 | §9.2, §9.3 |
| 9 | Tier B 제출 경로에서 바인딩 강제 + raw 큐 CID 할당자 | §9.4 |
| 10 | 바인딩 수명주기 훅 — NS 제거 시 자동 해제, 리셋 시 테이블 무효화 | §9.5 |
| 11 | (후속) Tier A 바인딩 — per-NS tagset 분리 설계 | §9.6 |

---

## 9. Namespace–SQ 바인딩 설계

> 목표: **namespace 마다 전용 SQ 집합을 배정**하고(1 NS : N SQ, exclusive), 그 NS 의 IO 가 바인딩된 SQ 로만 제출되게 한다.
> 1차 범위: **Tier B(raw/passthrough) 큐에서 바인딩 강제**. Tier A(blk-mq 일반 블록 IO)는 per-NS tagset 분리가 필요한 별도 라운드(§9.6).
> 활용: NS별 QoS 격리(테넌트별 depth/큐 수 차등), noisy-neighbor 차단, NS별 인터럽트/폴링 정책 분리.

### 9.0 전제 — 스펙과 vanilla 의 위치

NVMe 스펙에서 SQ 는 **컨트롤러 소속**이다. 어떤 SQ 로든 어떤 NSID 의 명령이든 보낼 수 있고, 명령의 대상 NS 는 SQE 의 `nsid` 필드(예: `cmnd->rw.nsid`, core.c:1022)로만 식별된다. 즉 **"NS–SQ 바인딩"은 스펙이 제공하는 기능이 아니라, 호스트 드라이버가 새로 만들어야 하는 순수 정책**이다. 컨트롤러 펌웨어는 전혀 관여하지 않으므로 (목표 1~3과 마찬가지로) 호스트 수정만으로 구현 가능하다.

### 9.1 Vanilla 분석 — 왜 NS–SQ 연결성이 존재하지 않는가

**(1) 모든 NS 가 컨트롤러의 단일 tagset 을 공유한다** — core.c:4153

```c
disk = blk_mq_alloc_disk(ctrl->tagset, &lim, ns);   /* core.c:4153 — nvme0n1·n2·... 전부 같은 tagset */
ns->queue = disk->queue;                             /* core.c:4160 */
```

NS 마다 `request_queue` 는 따로 있지만, hctx 집합·CPU→큐 매핑·태그 풀은 **tagset 단위로 공유**된다. blk-mq 의 큐 선택은 tagset 의 queue map(CPU 기준)으로만 결정되므로, **request_queue 가 다르다는 사실(=NS 가 다르다는 사실)은 큐 선택에 아무 영향이 없다.**

**(2) SQ 선택은 100% 제출 CPU 기준이다** — pci.c:1408

```c
static blk_status_t nvme_queue_rq(struct blk_mq_hw_ctx *hctx, ...)
{
    struct nvme_queue *nvmeq = hctx->driver_data;   /* pci.c:1408 — hctx 가 곧 SQ. NS 무관 */
```

**(3) passthrough 도 큐 선택은 우회하지 못한다** — ioctl.c:107

```c
nvme_submit_user_cmd(ns->queue, ...)                 /* ioctl.c:162 — NVME_IOCTL_IO_CMD 등 */
  └─ nvme_alloc_user_request(q, ...)
       └─ req = blk_mq_alloc_request(q, ...);        /* ioctl.c:107 — 결국 blk-mq 태그/hctx 경유 */
```

io_uring passthrough(`nvme_ns_chr_uring_cmd`, ioctl.c:673)도 동일하게 `ns->queue` 위에서 request 를 할당한다. **vanilla 의 "passthrough"는 bio/FS 계층만 우회할 뿐, SQ 선택은 여전히 CPU→hctx 매핑이 결정한다.** 따라서 바인딩을 강제하려면 blk-mq 를 거치지 않는 **Tier B 전용 제출 경로**(§9.4)가 필요하다.

**(4) NS 식별과 수명** — NS 객체는 `struct nvme_ns`(nvme.h:583), NSID 는 `ns->head->ns_id`(nvme.h:595 → `nvme_ns_head`). 컨트롤러의 NS 목록은 `ctrl->namespaces` 리스트(srcu 보호, `nvme_find_get_ns` core.c:4097 이 nsid→ns 조회). NS 는 attach/detach·rescan 으로 **런타임에 생기고 사라진다**(`nvme_alloc_ns` core.c:4135 / `nvme_ns_remove` core.c:4250) — 바인딩 테이블의 수명주기 정합성(§9.5)이 필요한 이유.

### 9.2 매핑 모델과 자료구조

**모델: 1 NS : N SQ, exclusive.**
- 한 NS 에 SQ 여러 개 배정 가능 (멀티스레드 제출 분산).
- **한 SQ 는 최대 1개 NS 에만 바인딩** (배타) — 이미 바인딩된 SQ 재바인딩은 `-EBUSY`.
- 바인딩 안 된 NS 는 기존 공유 경로 그대로 사용 (점진 적용 가능).

조회는 양방향이 필요하다 — 제출 fast path 는 `nsid → SQ 집합`, 큐 삭제 검증은 `sqid → nsid`.

```c
/* ── 정방향: nsid → 바인딩 (dev 에 xarray 추가; NSID는 sparse 가능하므로 배열 대신 xarray) ── */
struct nvme_ns_sq_bind {
    u32            nsid;                              /* 대상 NSID */
    unsigned long  sq_map[BITS_TO_LONGS(NVME_MAX_QID)]; /* 바인딩된 sqid 비트맵 */
    u16            nr_sqs;                            /* 바인딩된 SQ 개수 (선택 정책용) */
    atomic_t       rr_next;                           /* round-robin 커서 (정책 b) */
    struct rcu_head rcu;                              /* 제출 fast path 가 RCU 로 읽으므로 */
};

struct nvme_dev {
    ...
+   struct xarray  ns_sq_bindings;    /* nsid → struct nvme_ns_sq_bind*. 변경은 shutdown_lock, 읽기는 RCU */
};

/* ── 역방향: sqid → nsid (nvme_queue 에 필드 추가) ── */
struct nvme_queue {
    ...
    u16  cqid;          /* §4.1 에서 추가한 SQ→CQ 매핑 */
+   u32  bound_nsid;    /* 이 SQ가 바인딩된 NSID. 0 = unbound. 설정자: BIND/UNBIND ioctl(shutdown_lock 하).
+                        * 읽는 자: 큐 삭제 핸들러(-EBUSY 판정), INFO ioctl. */
};
```

동기화 모델 (큐 토폴로지와 동일한 정책으로 단순화):

| 접근 | 컨텍스트 | 메커니즘 |
|------|---------|---------|
| 바인딩 생성/해제 | ioctl (프로세스) | `dev->shutdown_lock` — 큐 create/delete 와 같은 락으로 직렬화 → 바인딩 중 큐가 사라지는 레이스 원천 차단 |
| 제출 시 조회 | Tier B 제출 경로 (프로세스/io_uring) | `rcu_read_lock()` + `xa_load(&dev->ns_sq_bindings, nsid)` — 락 없는 fast path |
| 해제 시 회수 | ioctl | `xa_erase` 후 `kfree_rcu` — 진행 중 제출이 옛 바인딩을 다 읽고 나간 뒤 해제 |

### 9.3 ioctl 인터페이스 확장

§3.1 의 명령군에 추가한다:

```c
struct nvme_qmgmt_bind {
    __u32 nsid;        /* 대상 namespace ID */
    __u16 sqid;        /* 바인딩/해제할 SQ id (1회 1개 — 여러 개는 반복 호출) */
    __u16 flags;       /* 예약 (strict 모드 등 확장용) */
};

#define NVME_IOCTL_QMGMT_BIND    _IOW('N', 0x83, struct nvme_qmgmt_bind)
#define NVME_IOCTL_QMGMT_UNBIND  _IOW('N', 0x84, struct nvme_qmgmt_bind)
```

`NVME_IOCTL_QMGMT_INFO`(§3.1)의 엔트리에는 예약 필드를 활용해 바인딩을 노출한다:

```diff
 struct nvme_qmgmt_info_entry {
     __u16 qid;
     __u16 cqid;
     __u16 depth;
     __u16 cq_vector;
     __u8  type;
     __u8  online;
-    __u16 rsvd;
+    __u16 rsvd;
+    __u32 bound_nsid;   /* 이 SQ가 바인딩된 NSID. 0 = unbound */
 };
```

**BIND 검증 규칙** (모두 `shutdown_lock` 보유 후 검사):

| 검사 | 실패 시 | 근거 |
|------|--------|------|
| `capable(CAP_SYS_ADMIN)` | `-EACCES` | 큐 관리 ioctl 과 동일 정책 (§4.4) |
| SQ 존재 + `NVMEQ_ENABLED`(pci.c:385) | `-ENOENT` | 없는 큐에 바인딩 불가 |
| 해당 SQ 가 **Tier B**(`F_BLKMQ_VISIBLE` 아님) | `-EINVAL` | Tier A 큐는 blk-mq 가 CPU 기준으로 사용 중 — 바인딩 의미 없음(§9.6 전까지) |
| NSID 존재 (`nvme_find_get_ns`, core.c:4097) | `-ENOENT` | 죽은 NSID 바인딩 방지. 확인 후 `nvme_put_ns` 로 ref 반납 |
| `nvmeq->bound_nsid == 0` | `-EBUSY` | exclusive 규칙 — 다른 NS 가 선점한 SQ |
| 같은 (nsid, sqid) 중복 BIND | `-EEXIST` | 멱등 아님을 명시 |

UNBIND 는 역으로 `nvmeq->bound_nsid == nsid` 가 아니면 `-ENOENT`. 비트맵에서 마지막 SQ 가 빠지면 `nvme_ns_sq_bind` 객체 자체를 `xa_erase` + `kfree_rcu`.

### 9.4 제출 경로 — Tier B 에서 바인딩 강제

Tier B 큐는 blk-mq 밖에 있으므로(§5.4) 제출 경로도 드라이버가 직접 소유한다. custom 드라이버의 raw 제출 ioctl(또는 io_uring `IORING_OP_URING_CMD` 확장)이 진입점이라고 가정하면:

```
사용자 raw 제출 (NSID 포함)
  ├─ rcu_read_lock()
  ├─ bind = xa_load(&dev->ns_sq_bindings, nsid)     /* ① 바인딩 조회 (락 없음) */
  │    ├─ bind == NULL → 정책 분기:
  │    │     기본: 기존 경로(공유/지정 큐)로 제출        ← 미바인딩 NS 하위호환
  │    │     strict 플래그 시: -EPERM                  ← 바인딩 없으면 raw 제출 거부
  │    └─ bind 있음 → ② SQ 선택
  ├─ sqid = select_sq(bind)                          /* ② N개 중 1개 선택 (아래 정책) */
  ├─ cid  = alloc_cid(&dev->queues[sqid])            /* ③ raw 큐 전용 CID 할당 */
  ├─ spin_lock(&nvmeq->sq_lock)                      /* ④ SQ 기록 — 여러 제출자가 같은 SQ 공유 가능 */
  │    SQE 복사(nsid·cid 포함) → sq_tail 증가 → doorbell
  ├─ spin_unlock + rcu_read_unlock
  └─ 완료: 바인딩 SQ가 매핑된 CQ(§4 의 cqid)에서 CQE 수확
       → CQE.sq_id 로 SQ 식별, CID 로 요청 복원 (부록 E 의 phase/genctr 방어 동일 적용)
```

**② SQ 선택 정책** — 1 NS : N SQ 에서 N 개 중 무엇을 고를 것인가:

| 정책 | 방법 | 장단점 |
|------|------|--------|
| (a) 사용자 명시 | 제출 명령에 sqid 포함, 드라이버는 **바인딩 검증만** (`sqid ∈ bind->sq_map` 확인, 아니면 `-EPERM`) | Tier B 사용자는 어차피 큐를 직접 다루는 주체 — **권장 기본값** |
| (b) submitter CPU 해시 | `sq_map 의 (cpu % nr_sqs) 번째` | sqid 미지정 시 fallback. 같은 CPU 는 같은 SQ → `sq_lock` 경합 최소 |
| (c) round-robin | `atomic_inc(&bind->rr_next) % nr_sqs` | 균등 분산이 필요하나 CPU 수 < SQ 수일 때 |

**③ raw 큐 CID 할당자** — blk-mq 태그가 없으므로 자체 할당이 필요하다:

```c
/* nvme_queue 에 추가 (Tier B 큐만 사용) */
+ unsigned long *cid_bitmap;   /* q_depth 비트. find_first_zero_bit + test_and_set_bit 로 할당 */
+ void         **cid_ctx;      /* cid → 제출 컨텍스트(완료 통지 대상) 역참조 배열 */
```

- 할당 실패(비트맵 만석) = **SQ full** → `-EAGAIN` 반환 (사용자 재시도). 이것이 Tier B 의 자연스러운 backpressure 다 — blk-mq 태그 고갈 대기와 등가.
- `command_id` 의 상위 4비트 genctr 인코딩(부록 E.2, nvme.h:645)은 **그대로 유지**한다 — stale CQE 방어 계층을 Tier B 에서도 동일하게 갖추기 위함.

> **왜 vanilla passthrough 를 고치지 않는가**: `NVME_IOCTL_IO_CMD`/uring_cmd 는 `blk_mq_alloc_request`(ioctl.c:107)로 태그를 받는 순간 hctx(=SQ)가 결정된다. 여기서 SQ 만 바꿔치기하면 **태그 소유권과 SQ depth 회계가 분리**되어(여러 hctx 의 태그가 한 SQ 로 몰려 SQ 오버런) §5.1-(3) 과 같은 부류의 정합성 붕괴가 생긴다. 바인딩 강제는 자체 CID 회계를 갖는 Tier B 경로에서만 한다.

### 9.5 수명주기 정합성 — 바인딩은 NS·큐·컨트롤러 셋의 수명에 걸쳐 있다

바인딩 테이블은 (NS, SQ, 컨트롤러 상태) 세 객체를 참조하므로, 각각의 소멸 이벤트에 대한 정책이 필요하다.

| 이벤트 | 정책 | 근거 |
|--------|------|------|
| **바인딩된 SQ 삭제 요청** (`QMGMT_DELETE`) | `-EBUSY` — 먼저 UNBIND 강제 | 자동 해제보다 명시적 순서가 안전. CQ 삭제 시 SQ 잔존 검사(§6)와 동일 철학 |
| **NS 제거** (detach/rescan, `nvme_ns_remove` core.c:4250) | 제거 훅에서 해당 nsid 바인딩 **자동 해제** | NS 는 사용자 의지와 무관하게 사라질 수 있음(컨트롤러 주도 AEN→rescan). `-EBUSY` 로 막을 수 있는 대상이 아님 |
| **컨트롤러 리셋** (`nvme_reset_work` → `nvme_dev_disable` pci.c:282) | 테이블 **전체 무효화** + 로그 통지 | 리셋은 모든 IO 큐를 파괴·재생성한다. ioctl 로 만든 Tier B 큐 자체가 리셋 후 사라지므로, 바인딩만 보존해도 가리키는 대상이 없다. 사용자가 큐 재생성→재바인딩 |
| **UNBIND 시 in-flight** | 신규 제출만 차단, in-flight 는 자연 완료 | SQ 자체는 살아있으므로 위험 없음. CID 비트맵이 0 이 될 때까지 큐는 정상 동작 |
| **NS 제거 시 in-flight** | 자동 해제 후 CID 비트맵 drain 대기 (또는 timeout→`nvme_cancel_*` 류 강제완료) | 부록 B.4 원칙 — free 전 in-flight 0 보장은 별도 책임 |

NS 제거 훅의 위치: `nvme_ns_remove`(core.c:4250)는 transport 공용 core.c 에 있으므로, PCIe 전용 바인딩 해제는 `nvme_ctrl_ops` 에 선택적 콜백(예: `.ns_detach_notify`)을 추가하거나, custom 드라이버가 이미 갖고 있는 PCIe 전용 훅에 연결한다. 호출 시점은 **`blk_mq_destroy_queue`/cdev 제거보다 앞** — 새 raw 제출이 죽은 NS 를 보기 전에 테이블에서 지운다 (RCU grace period 가 진행 중 제출을 보호).

### 9.6 (후속 라운드) Tier A 바인딩 — per-NS tagset 분리 방향

일반 블록 IO(파일시스템)까지 바인딩을 지키게 하려면, **"모든 NS 가 ctrl->tagset 을 공유한다"는 전제 자체를 깨야 한다.** 공유 tagset 의 queue map 은 CPU 기준 단일 매핑이라 NS 별로 다른 hctx 집합을 줄 방법이 없기 때문이다.

방향: `nvme_alloc_ns`(core.c:4135)에서 `ctrl->tagset` 대신 **NS(또는 NS 그룹)별 tagset** 을 할당하고, 그 tagset 의 `nr_hw_queues` = 바인딩된 SQ 수, `.init_hctx` 는 §5.6 의 `hctx_to_qid` indirection 으로 바인딩된 qid 들에 매핑한다.

수술 범위 (단일 tagset 전제가 박힌 경로들 — 모두 NS별 tagset 순회로 바꿔야 함):

| 경로 | 위치 | 현재 전제 |
|------|------|----------|
| `nvme_quiesce_io_queues` / `unquiesce` | core.c:5296~5310 | `blk_mq_quiesce_tagset(ctrl->tagset)` 1회로 전 NS 정지 |
| `nvme_cancel_tagset` | core.c:539~542 | `ctrl->tagset` 순회로 전 in-flight 강제완료 |
| `nvme_pci_update_nr_queues` | pci.c:3133 | `dev->tagset` 단일 대상 전파 |
| tagset 수명 (`nvme_alloc_io_tag_set`/`remove`) | core.c:4938/4990 | 컨트롤러당 1회 할당/해제 |
| multipath (`nvme_ns_head` 경유 IO) | multipath.c | head 의 request_queue 가 단일 tagset 전제 |

부수 효과: tagset 이 NS 별로 분리되면 **NS별 queue_depth 차등이 자연히 가능**해진다 (§5.5 의 "tagset depth = min" 제약이 NS 단위로 좁혀짐). 비용은 NS 수 × 태그 풀 메모리와 위 경로 전반의 복잡도 증가. **별도 설계 라운드로 분리**하고, 1차에서는 Tier B 강제 + Tier A 는 "바인딩 무시(공유 경로)" 로 명확히 정의한다.

### 9.7 검증 계획

1. **바인딩 CRUD**: BIND→INFO 로 `bound_nsid` 확인, 중복 BIND `-EEXIST`, 타 NS 선점 SQ `-EBUSY`, 없는 NSID/SQ `-ENOENT`, Tier A 큐 바인딩 `-EINVAL`.
2. **제출 강제**: 바인딩 후 raw 제출이 바인딩 SQ 로만 나가는지 — 큐별 제출 카운터 또는 `bpftrace` 로 SQ doorbell 기록 시 qid 추적. sqid 명시 제출이 `sq_map` 밖이면 `-EPERM`.
3. **선택 정책**: N개 SQ 바인딩 후 멀티스레드 제출 → CPU 해시/RR 분산 확인.
4. **backpressure**: depth 작은 바인딩 SQ 에 폭주 제출 → CID 비트맵 만석 시 `-EAGAIN`, 완료 후 재시도 성공.
5. **수명주기**: (a) 바인딩 SQ 삭제 시도 `-EBUSY` → UNBIND 후 삭제 성공. (b) `nvme detach-ns` 중 IO 부하 → 자동 해제 + UAF 부재(KASAN). (c) 리셋(`NVME_IOCTL_RESET`) 후 INFO 로 테이블 클리어 확인.
6. **동시성**: BIND/UNBIND 루프 ‖ raw 제출 루프 ‖ 큐 create/delete 루프 동시 수행 — RCU/shutdown_lock 경합에서 crash·leak 부재.

---

## 부록 A. vanilla 코드 참조 인덱스

| 심볼 | 위치 | 역할 |
|------|------|------|
| `SQ_SIZE`/`CQ_SIZE` | pci.c:34~35 | depth 기반 큐 버퍼 크기 |
| `struct nvme_queue` | pci.c:365~394 | 큐 객체 (cqid 필드 추가 대상) |
| `adapter_alloc_cq` | pci.c:1719 | Create I/O CQ admin 명령 |
| `adapter_alloc_sq` | pci.c:1742 | Create I/O SQ admin 명령 (cqid 하드코딩 1766) |
| `adapter_delete_cq/sq` | pci.c:1771/1776 | Delete I/O CQ/SQ |
| `nvme_alloc_queue` | pci.c:2103 | 큐 DMA 버퍼 할당 (depth 인자) |
| `nvme_init_queue` | pci.c:2152 | doorbell/포인터 초기화 |
| `queue_request_irq` | pci.c:2138 | MSI-X 인터럽트 등록 |
| `nvme_create_queue` | pci.c:2189 | CQ+SQ 묶음 생성 (자동 경로) |
| `nvme_create_io_queues` | pci.c:2378 | 부팅 시 전체 IO 큐 생성 루프 |
| `nvme_setup_io_queues` | pci.c:2909 | 개수 협상+IRQ+생성 총괄 |
| `nvme_dev_ioctl` | ioctl.c:844 | /dev/nvmeX ioctl 디스패치 (확장 지점) |
| `io_queue_depth` param | pci.c:108 | 전역 depth 모듈 파라미터 |
| `blk_mq_alloc_disk(ctrl->tagset,...)` | core.c:4153 | 전 NS 가 단일 tagset 공유하는 지점 (§9.1) |
| `nvme_queue_rq` | pci.c:1405 | CPU기반 hctx→SQ 선택 (NS 무관, §9.1) |
| `struct nvme_ns` / `nvme_ns_head` | nvme.h:583/524 | NS 객체, NSID 는 `head->ns_id` |
| `nvme_find_get_ns` | core.c:4097 | nsid→ns 조회 (srcu, BIND 검증에 사용) |
| `nvme_alloc_ns` / `nvme_ns_remove` | core.c:4135/4250 | NS 수명 (바인딩 자동해제 훅 지점, §9.5) |
| `nvme_submit_user_cmd` | ioctl.c:162 | vanilla passthrough — blk-mq 경유 (§9.1) |
| `nvme_ns_chr_uring_cmd` | ioctl.c:673 | io_uring passthrough 진입점 |

---

## 부록 B. blk-mq 큐 제어 연산 총정리 (freeze / quiesce / stop / cancel …)

런타임 큐 관리는 "큐를 잠시 멈추는" 연산을 정확히 골라 써야 한다. 이름이 비슷한 freeze·quiesce·stop 은 **막는 지점과 대기 의미가 전부 다르다.**

### B.1 3대 정지 메커니즘 비교

| 축 | **FREEZE** | **QUIESCE** | **STOP/START** |
|----|-----------|-------------|----------------|
| 함수 | `blk_mq_freeze_queue` / `blk_mq_unfreeze_queue` | `blk_mq_quiesce_queue` / `blk_mq_unquiesce_queue` | `blk_mq_stop_hw_queue` / `blk_mq_start_hw_queue` |
| 기반 상태 | `q_usage_counter` (percpu_ref) | `QUEUE_FLAG_QUIESCED` (blk-mq.c:267) | `BLK_MQ_S_STOPPED` 비트 (blk-mq.c:2489) |
| **막는 지점** | 큐 **진입**(`bio_queue_enter`, tag 할당 前) | **dispatch**(`.queue_rq` 호출 경로) | 특정 hctx 의 dispatch |
| 신규 요청 처리 | **sleep**(park). REQ_NOWAIT → -EAGAIN (blk-core.c:338) | 내부 SW큐/스케줄러에 **적재** (디바이스로만 안 감) | 내부 적재 |
| **in-flight drain 대기** | **✅ 완료+free 까지 대기** (counter→0, blk-mq.c:191) | ❌ 진행 중 `.queue_rq` 반환만 대기 (RCU/SRCU sync, blk-mq.c:281) | ❌ 보장 안 함 |
| end_io 콜백 | (drain 으로 모두 실행됨) | **막지 않음** (blk-mq.c:294) | 막지 않음 |
| 범위 | request_queue(namespace) 단위 | queue 또는 tagset (`blk_mq_quiesce_tagset`) | hctx 1개 |
| 대표 용도 | 토폴로지 변경(무 in-flight 필요) | IO 전송만 일시 차단(요청은 유지) | 자원부족 백프레셔 |

### B.2 각 메커니즘의 역할과 NVMe 사용처

**FREEZE — "아무 IO도 안 떠 있는 상태를 만든다"**
- 신규 진입 차단 + in-flight 전부 drain (§5.7). "진행 중 IO 가 0 이어야 안전한" 변경에 사용.
- 세분: `blk_freeze_queue_start`(시작만, 비대기) / `blk_mq_freeze_queue_wait`(drain 대기) / `blk_mq_unfreeze_queue`(재개).
- 대상 연산: `blk_mq_update_nr_hw_queues`(§5.3), nr_requests 변경, elevator 교체, namespace revalidate.
- NVMe: `nvme_start_freeze`+`nvme_wait_freeze`+`nvme_unfreeze` — 리셋 전후 IO 0 보장 (core.c:1248~1259, 5275). timeout work 에서 start, reset work 에서 unfreeze 하는 non_owner 패턴 (core.c:5289).

**QUIESCE — "디바이스로 명령만 잠깐 안 보낸다 (요청은 계속 받음)"**
- dispatch 만 멈춤. 요청은 정상 할당되어 내부에 쌓이고, 하드웨어로만 안 나감.
- drain 안 함 → RCU/SRCU sync 로 진행 중 `.queue_rq` 종료만 대기. **end_io 는 계속 실행됨**(이미 떠 있는 완료는 처리됨).
- 큐는 살려두고 명령 전송만 멈출 때(에러 복구/리셋/펌웨어 활성화).
- NVMe: `nvme_quiesce_io_queues`/`unquiesce`(core.c:5294, `blk_mq_quiesce_tagset` 사용), `nvme_quiesce_admin_queue`.

**STOP/START — "특정 hctx 백프레셔" (동기화 barrier 아님)**
- `.queue_rq` 가 `BLK_STS_RESOURCE` 반환 시 그 hctx 를 멈췄다 재개하는 흐름 제어.
- ⚠️ blk-mq.c:2498 주석: *"stop 후 dispatch 가 drain/차단된다고 보장하지 않는다. 그 목적이면 quiesce 를 써라."* → **안전 barrier 로 쓰면 안 됨.**

### B.3 보조 연산

| 연산 | 함수 | 역할 |
|------|------|------|
| RUN | `blk_mq_run_hw_queue(s)` (blk-mq.c:2352) | 쌓인 요청을 하드웨어로 미는 dispatch 트리거. unquiesce/start 후 흐름 재개 |
| REQUEUE | `blk_mq_requeue_request` + `blk_mq_kick_requeue_list` (blk-mq.c:1549, 1605) | 이미 디스패치된 요청을 재시도 큐로 되돌림 (자원부족, 경로 failover). 완료와 다름 |
| **CANCEL** | `blk_mq_tagset_busy_iter` + `blk_mq_tagset_wait_completed_request` | in-flight 태그를 순회하며 **강제 에러 완료**. NVMe `nvme_cancel_tagset`(core.c:537) — 죽은 컨트롤러의 응답 없는 IO 강제 종료 |
| SYNC | `blk_sync_queue` | run/timeout/requeue work 등 비동기 워크 완료 대기. NVMe `nvme_sync_queues`(core.c:5338) |

### B.4 본 설계(큐 관리 ioctl)에서의 선택 지침

- **하드웨어 큐를 실제로 free 하기 직전**: 반드시 **FREEZE**(또는 CANCEL)로 in-flight 0 을 보장한 뒤 `adapter_delete_sq/cq` → DMA free. QUIESCE·STOP 만으로는 in-flight 가 남아 use-after-free 위험.
- **CANCEL = drain hang 탈출구**: 큐를 잘못된 순서로 삭제해 CQE 가 영영 안 와도(§5.7 함정 2), `nvme_cancel_tagset` 이 in-flight 를 강제 에러 완료시켜 ref 를 떨궈 freeze 를 푼다.
- **단일 SQ 만 덜 침습적으로 멈추고 싶을 때**: 전체 namespace freeze 대신 **QUIESCE** 고려(요청은 적재만). 단 free 전 in-flight 0 보장은 별도로 필요.
- **Tier A 전파(`blk_mq_update_nr_hw_queues`)**: 내부에서 FREEZE 를 자동 수행하므로 호출자가 추가로 freeze/quiesce 하지 않는다 (§5.3).

---

## 부록 C. NVMe ↔ blk-mq 함수 제어 표면 총정리 (study용)

NVMe 드라이버와 blk-mq 사이에는 **두 방향**의 인터페이스가 있다. 이 부록은 NVMe host 드라이버(`drivers/nvme/host/`)가 호출하거나 제공하는 blk-mq/블록 함수를 역할별로 모두 모은 것이다. (라인은 본 트리 기준)

```
   ┌─────────────────────────── NVMe host driver ───────────────────────────┐
   │  C.1 제공(콜백): blk_mq_ops { queue_rq, complete, init_hctx, ... }       │  ← blk-mq가 위→아래로 호출
   │  C.2~C.7 호출:   blk_mq_*/blk_* (tagset·freeze·quiesce·request·...)      │  → NVMe가 아래→위로 호출
   └─────────────────────────────────────────────────────────────────────────┘
                                blk-mq (block/blk-mq.c, blk-core.c, blk-mq-tag.c, blk-mq-cpumap.c)
```

### C.1 NVMe가 blk-mq에 **제공**하는 콜백 — `blk_mq_ops` (pci.c:2241, 2249)

blk-mq 가 IO 처리 중 호출하는 진입점. NVMe 가 구현해 등록한다.

| 콜백 | NVMe 구현 | 역할 (blk-mq가 언제 부르나) |
|------|----------|------------------------------|
| `.queue_rq` | `nvme_queue_rq` (pci.c) | 요청 1건을 하드웨어(SQ)에 제출. dispatch 핵심 경로 |
| `.queue_rqs` | `nvme_queue_rqs` | 요청 배치를 한 번에 제출(플러그 머지). 락/doorbell 비용 절감 |
| `.commit_rqs` | `nvme_commit_rqs` | 배치 제출 후 SQ doorbell 한 번에 기록(write-combine) |
| `.complete` | `nvme_pci_complete_rq` | 완료 후처리(언맵 등). softirq/완료 컨텍스트에서 호출 |
| `.init_hctx` | `nvme_init_hctx` / `nvme_admin_init_hctx` | hctx↔`nvme_queue` 바인딩 (`hctx->driver_data`). §5.1 핵심 |
| `.init_request` | `nvme_pci_init_request` | request당 `nvme_iod`/`nvme_command` 초기화 |
| `.map_queues` | `nvme_pci_map_queues` | CPU↔hctx 매핑 구성(IRQ affinity 기반). §5.1 |
| `.timeout` | `nvme_timeout` | 명령 타임아웃 처리(abort/reset 트리거). drain hang 복구의 시작점 |
| `.poll` | `nvme_poll` | 폴링 큐에서 CQ 직접 수확(IRQ 없이) |

### C.2 tagset / queue / disk **생성·소멸**

| 함수 | 정의 | 역할 | NVMe 래퍼/호출처 |
|------|------|------|------------------|
| `blk_mq_alloc_tag_set` | blk-mq.c:4864 | tagset(태그 풀·hctx 템플릿) 할당. `queue_depth`·`nr_hw_queues` 확정 | `nvme_alloc_admin_tag_set`(core.c:4865), `nvme_alloc_io_tag_set`(core.c:4938) |
| `blk_mq_free_tag_set` | blk-mq.c:4985 | tagset 해제 | `nvme_remove_*_tag_set`(core.c:4922/4990) |
| `__blk_mq_alloc_disk` / `blk_mq_alloc_disk` | blk-mq.c:4490 | tagset 기반 gendisk+request_queue 생성 | namespace 디스크 생성 |
| `blk_mq_alloc_queue` | blk-mq.c | request_queue만 생성 | fabrics connect_q 등 |
| `blk_mq_destroy_queue` | blk-mq.c:4473 | request_queue 파괴(드레인 포함) | 컨트롤러/네임스페이스 제거 |
| `blk_put_queue` | blk-core.c | request_queue refcount 해제 | 위와 짝 |
| `blk_mq_update_nr_hw_queues` | blk-mq.c:5237 | **hctx 개수 런타임 변경**(내부 freeze→remap→unfreeze). §5.3 | `nvme_pci_update_nr_queues`(pci.c:3133) |
| `blk_mq_num_possible_queues` | blk-mq-cpumap.c:38 | 가능한 큐 수 상한 산출 | 큐 개수 계획 |
| `blk_mq_map_queues` | blk-mq-cpumap.c:59 | CPU→큐 기본 매핑(폴링 큐용) | `.map_queues` 안에서 |
| `blk_mq_map_hw_queues` | blk-mq-cpumap.c:108 | IRQ affinity 기반 CPU→큐 매핑 | `.map_queues` 안에서 |

### C.3 큐 상태 제어 — **FREEZE** 계열 (진입 차단 + drain)

| 함수 | 정의 | 역할 |
|------|------|------|
| `blk_freeze_queue_start` | blk-mq.c:182 | freeze 시작(percpu_ref kill, 신규 진입 차단). **비대기** |
| `blk_freeze_queue_start_non_owner` | blk-mq.c:244 | 위와 동일하나 owner 추적 없이(타 컨텍스트에서 unfreeze) | 
| `blk_mq_freeze_queue_wait` | blk-mq.c:189 | in-flight **drain 대기**(counter→0) |
| `blk_mq_freeze_queue_wait_timeout` | blk-mq.c:195 | drain을 타임아웃 한도까지만 대기 |
| `blk_mq_freeze_queue` | blk-mq.c | start + wait (합본). **drain까지 보장** |
| `blk_mq_unfreeze_queue` (`_nomemrestore`) | blk-mq.c:230 | freeze 해제(percpu_ref resurrect + 깨우기) |
| `blk_mq_unfreeze_queue_non_owner` | blk-mq.c:251 | non_owner 짝 해제 |

**NVMe 래퍼**: `nvme_start_freeze`(core.c:5275, =start_non_owner) · `nvme_wait_freeze`(core.c:5262) · `nvme_wait_freeze_timeout`(core.c:5245) · `nvme_unfreeze`(core.c:5231). → start/wait 분리 이유는 §본문 참조.

### C.4 큐 상태 제어 — **QUIESCE** 계열 (dispatch 차단, drain 아님)

| 함수 | 정의 | 역할 |
|------|------|------|
| `blk_mq_quiesce_queue_nowait` | blk-mq.c:261 | QUIESCED 플래그만 set. **비대기** (배치용) |
| `blk_mq_wait_quiesce_done` | blk-mq.c:281 | 진행 중 `.queue_rq` 종료 대기(`synchronize_rcu/srcu`) |
| `blk_mq_quiesce_queue` | blk-mq.c:299 | nowait + wait_done (단일 큐 quiesce) |
| `blk_mq_unquiesce_queue` | blk-mq.c:315 | quiesce 해제 + run_hw_queues로 적재분 재개 |
| `blk_mq_quiesce_tagset` | blk-mq.c:335 | tagset 전체: 큐마다 nowait + **wait 1회** |
| `blk_mq_unquiesce_tagset` | blk-mq.c:350 | tagset 전체 해제 |

**NVMe 래퍼**: `nvme_quiesce_io_queues`(core.c:5294, =quiesce_tagset) · `nvme_unquiesce_io_queues`(core.c:5305) · `nvme_quiesce_admin_queue`(core.c:5314) · `nvme_unquiesce_admin_queue`(core.c:5323).

### C.5 dispatch 흐름 제어 — stop/start/run/requeue

| 함수 | 정의 | 역할 |
|------|------|------|
| `blk_mq_stop_hw_queue(s)` | blk-mq.c:2485/2502 | 특정 hctx dispatch 정지(백프레셔). **barrier 아님**(주석 2498) |
| `blk_mq_start_hw_queue(s)` | blk-mq.c:2512/2520 | 정지 해제 + run |
| `blk_mq_start_stopped_hw_queue(s)` | blk-mq.c:2530/2546 | 정지된 것만 재개 |
| `blk_mq_run_hw_queue(s)` | blk-mq.c:2352/2417 | 적재 요청을 하드웨어로 미는 dispatch 트리거 |
| `blk_mq_delay_run_hw_queue(s)` | blk-mq.c:2316/2445 | 지연 후 run |
| `blk_mq_requeue_request` | blk-mq.c:1549 | 디스패치된 요청을 재시도 큐로 되돌림 |
| `blk_mq_kick_requeue_list` / `_delay_` | blk-mq.c:1605 | 재시도 큐 처리 트리거 | 
| `blk_steal_bios` | blk-mq.c:3425 | 요청에서 bio 회수(멀티패스 failover 재전송) |

**NVMe 사용**: 재시도/requeue는 `nvme_retry_req`/멀티패스 경로(core.c:333 등). stop/start는 PCIe IO 경로에서는 거의 안 쓰고 주로 fabrics/자원 제어용.

### C.6 request **할당·제출·완료·취소**

| 함수 | 정의 | 역할 | NVMe 호출처 |
|------|------|------|-------------|
| `blk_mq_alloc_request` | blk-mq.c:661 | 일반 경로 request 할당(태그 획득) | passthrough/admin 명령 |
| `blk_mq_alloc_request_hctx` | blk-mq.c | 특정 hctx 지정 할당 | connect 등 큐 지정 명령 |
| `blk_execute_rq` | blk-mq.c:1508 | 동기 실행(완료까지 블록) | `nvme_submit_sync_cmd`(core.c:1198) |
| `blk_execute_rq_nowait` | blk-mq.c:1446 | 비동기 실행 | keep-alive/AEN 등 |
| `blk_mq_free_request` | blk-mq.c:820 | request 해제 → 태그 반납 → `blk_queue_exit`(ref−1) | 완료 후처리 |
| `blk_mq_complete_request` | blk-mq.c:1353 | 완료 처리 트리거(softirq로 위임 가능) | `nvme_try_complete_req` |
| `blk_mq_complete_request_remote` | blk-mq.c:1319 | 완료를 제출 CPU로 IPI 위임 | 위 내부 |
| `blk_mq_end_request` | blk-mq.c:1176 | 완료 최종 처리(bio 종료) | `nvme_complete_rq` 종단 |
| `blk_mq_add_to_batch` | blk-mq.h | 완료를 `io_comp_batch`에 모아 일괄 종료 | `nvme_handle_cqe`(IRQ 배치 완료) |
| `blk_mq_tagset_busy_iter` | blk-mq-tag.c:434 | **in-flight 태그 순회**(콜백 적용) | `nvme_cancel_tagset`(core.c:537) |
| `blk_mq_tagset_wait_completed_request` | blk-mq.h:948 | 위 강제완료가 모두 처리될 때까지 대기 | `nvme_cancel_tagset` 짝 |

**핵심 콜백** `nvme_cancel_request`(core.c:521): in-flight를 `NVME_SC_HOST_ABORTED_CMD`로 강제완료 → §exception 경로.

### C.7 sync / teardown / 상태 조회

| 함수 | 정의 | 역할 |
|------|------|------|
| `blk_sync_queue` | blk-core.c:220 | timeout/requeue/run work 등 비동기 워크 flush | (NVMe: `nvme_sync_queues` core.c:5343, `nvme_sync_io_queues` core.c:5330) |
| `blk_queue_enter` / `__bio_queue_enter` | blk-core.c:304/333 | 큐 진입 게이트(freeze 시 여기서 sleep) | (제출 경로, §5.7) |
| `blk_queue_exit` | blk-core.c:369 | 진입 ref 해제(`percpu_ref_put`) | 완료 종단 |
| `blk_mark_disk_dead` | blk-mq.c | 디스크 dead 표시(이후 IO 즉시 실패) | 컨트롤러 소실 |
| `blk_queue_dying` | blk-core.c | 큐 dying 상태 조회 | 종료 판정 |
| `blk_mq_rq_state` / `_request_completed` / `_request_started` | blk-mq.h | request 상태 조회 | 타임아웃/취소 판정 |
| `blk_mq_rq_to_pdu` / `_from_pdu` | blk-mq.h | request ↔ 드라이버 PDU(`nvme_iod`) 변환 | 전 경로 |

### C.8 한눈 매핑 — NVMe 래퍼 → 내부 blk-mq 함수

| NVMe 래퍼 (core.c) | 내부 호출 | 의미 |
|--------------------|-----------|------|
| `nvme_start_freeze` | `blk_freeze_queue_start_non_owner` | 진입 차단 시작(비대기) |
| `nvme_wait_freeze[_timeout]` | `blk_mq_freeze_queue_wait[_timeout]` | drain 대기 |
| `nvme_unfreeze` | `blk_mq_unfreeze_queue_non_owner` | freeze 해제 |
| `nvme_quiesce_io_queues` | `blk_mq_quiesce_tagset` | IO dispatch 정지(전 큐, wait 1회) |
| `nvme_unquiesce_io_queues` | `blk_mq_unquiesce_tagset` | IO dispatch 재개 |
| `nvme_quiesce_admin_queue` | `blk_mq_quiesce_queue` | admin dispatch 정지 |
| `nvme_unquiesce_admin_queue` | `blk_mq_unquiesce_queue` | admin dispatch 재개 |
| `nvme_sync_queues`/`nvme_sync_io_queues` | `blk_sync_queue` | 비동기 워크 flush |
| `nvme_cancel_tagset`/`nvme_cancel_admin_tagset` | `blk_mq_tagset_busy_iter`+`_wait_completed_request` | in-flight 강제완료 |
| `nvme_pci_update_nr_queues` | `blk_mq_update_nr_hw_queues` | hctx 개수 전파(§5) |
| `nvme_alloc_*_tag_set` / `nvme_remove_*_tag_set` | `blk_mq_alloc_tag_set` / `blk_mq_free_tag_set` | tagset 수명 |

---

## 부록 D. blk-mq 제어 함수 — 디테일 + 예시 간략표

### D.1 큐 상태 제어 (가장 중요)

| 함수 | 디테일 설명 | 예시 |
|------|------------|------|
| `blk_freeze_queue_start` | 진입 게이트 차단 시작(percpu_ref kill). **drain 안 기다리고 즉시 리턴** | `nvme_start_freeze()` — 타임아웃 워크에서 신규 IO만 즉시 막을 때 |
| `blk_mq_freeze_queue_wait` | in-flight가 완료+free되어 counter==0 될 때까지 **drain 대기** | `nvme_wait_freeze()` — safe shutdown에서 IO 다 끝나길 기다릴 때 |
| `blk_mq_freeze_queue` | 위 둘 합본(start+wait). **drain까지 보장** | namespace 크기 변경 전 `blk_mq_freeze_queue(ns->queue)` |
| `blk_mq_unfreeze_queue` | freeze 해제(ref resurrect + 대기자 깨움) | `nvme_unfreeze()` — 리셋 복구 후 IO 재개 |
| `blk_mq_quiesce_queue` | dispatch만 차단(요청은 적재). 진행 중 `.queue_rq` 종료까지 RCU 대기. **drain 아님** | `nvme_quiesce_admin_queue()` — 리셋 중 admin 명령 전송만 중단 |
| `blk_mq_quiesce_queue_nowait` | QUIESCED 플래그만 set, **대기 없음** (여러 큐 배치용) | `blk_mq_quiesce_tagset()` 내부: 큐마다 nowait + 끝에 wait 1회 |
| `blk_mq_unquiesce_queue` | quiesce 해제 + 적재분 run | `nvme_unquiesce_io_queues()` — dispatch 재개 |
| `blk_mq_stop_hw_queue` | 특정 hctx dispatch 정지. **barrier 아님**(drain 보장 X) | `.queue_rq`가 `BLK_STS_RESOURCE` 반환 시 자원 회복까지 정지 |
| `blk_mq_start_hw_queue` | stop 해제 + run | 자원 회복 후 그 hctx 재개 |

### D.2 토폴로지 / tagset

| 함수 | 디테일 설명 | 예시 |
|------|------------|------|
| `blk_mq_update_nr_hw_queues` | hctx 개수 변경. 내부에서 **freeze→remap(init_hctx/map_queues)→unfreeze** 자동 수행 | `nvme_pci_update_nr_queues()` — 리셋 후 큐 수 바뀌면 호출 |
| `blk_mq_alloc_tag_set` | 태그 풀+hctx 템플릿 할당. `queue_depth`·`nr_hw_queues` 확정 | `nvme_alloc_io_tag_set()` — 컨트롤러 init 시 1회 |
| `blk_mq_free_tag_set` | tagset 해제 | `nvme_remove_io_tag_set()` |
| `blk_mq_alloc_disk` | tagset 기반 gendisk+request_queue 생성 | namespace 등록 시 디스크 생성 |
| `blk_mq_destroy_queue` | request_queue 파괴(드레인 포함) | namespace/컨트롤러 제거 |
| `blk_mq_map_hw_queues` | IRQ affinity 기반 CPU→큐 매핑 | `.map_queues`(nvme_pci_map_queues) 내부 |

### D.3 request 생명주기

| 함수 | 디테일 설명 | 예시 |
|------|------------|------|
| `blk_mq_alloc_request` | 태그 획득하며 request 할당 | passthrough/admin 명령 발행 |
| `blk_execute_rq` | 동기 실행, **완료까지 블록** | `nvme_submit_sync_cmd()` — Identify, Create SQ/CQ 등 |
| `blk_execute_rq_nowait` | 비동기 실행 | keep-alive, AEN 발행 |
| `blk_mq_free_request` | 태그 반납 + `blk_queue_exit`(ref−1) | 완료 후처리 끝 |
| `blk_mq_complete_request` | 완료 처리 트리거(softirq/IPI 위임 가능) | `nvme_try_complete_req()` (CQE 처리 시) |
| `blk_mq_end_request` | bio 종료까지 완료 마무리 | `nvme_complete_rq()` 종단 |
| `blk_mq_add_to_batch` | 완료를 `io_comp_batch`에 모아 일괄 종료 | `nvme_handle_cqe()` — IRQ에서 배치 완료 |

### D.4 취소 / 동기화 (exception 경로)

| 함수 | 디테일 설명 | 예시 |
|------|------------|------|
| `blk_mq_tagset_busy_iter` | in-flight 태그를 순회하며 콜백 적용 | `nvme_cancel_tagset()` — 각 IO를 `nvme_cancel_request`로 강제완료 |
| `blk_mq_tagset_wait_completed_request` | 위 강제완료가 모두 처리될 때까지 대기 | `nvme_cancel_tagset()` 짝 |
| `nvme_cancel_request`(콜백) | in-flight를 `NVME_SC_HOST_ABORTED_CMD`로 강제 에러완료 → 버림 | device-stop 후 남은 IO 폐기 |
| `blk_sync_queue` | timeout/requeue/run work 등 비동기 워크 flush | `nvme_sync_queues()` — 핸들러가 죽은 req 못 건드리게 |
| `blk_mq_requeue_request` | 디스패치된 요청을 재시도 큐로 되돌림 | 자원부족/멀티패스 failover 재시도 |
| `blk_steal_bios` | request에서 bio 회수(다른 경로 재전송) | NVMe 멀티패스 failover |

### D.5 ops 콜백 (NVMe가 제공 → blk-mq가 호출)

| 콜백 | 디테일 설명 | 예시(언제 불리나) |
|------|------------|------------------|
| `.queue_rq` (`nvme_queue_rq`) | 요청 1건을 SQ에 기록+doorbell | blk-mq dispatch가 IO 1건 내릴 때 |
| `.queue_rqs` (`nvme_queue_rqs`) | 배치 제출(락/doorbell 절감) | 플러그에 모인 다건을 한 번에 |
| `.complete` (`nvme_pci_complete_rq`) | 언맵 등 완료 후처리 | CQE 처리 후 완료 컨텍스트 |
| `.init_hctx` (`nvme_init_hctx`) | hctx↔nvme_queue 바인딩 | tagset alloc/`update_nr_hw_queues` 시 |
| `.map_queues` (`nvme_pci_map_queues`) | CPU↔hctx 매핑 구성 | 위와 동일 시점 |
| `.timeout` (`nvme_timeout`) | 타임아웃→abort/reset | 명령이 제한시간 내 미완료 |
| `.poll` (`nvme_poll`) | 폴링 큐 CQ 직접 수확 | io_uring IOPOLL 등 |

### D.6 우리 ioctl 설계에서 쓰는 조합 (요약)

| 시나리오 | 함수 시퀀스 | 디테일 |
|---------|------------|--------|
| Tier A 큐 추가 | 큐 생성 → `blk_mq_update_nr_hw_queues` | 전파가 내부 freeze 수행 → 호출자 freeze 불필요 |
| Tier A 큐 삭제 | `blk_mq_update_nr_hw_queues`(전파/drain) → `adapter_delete_sq/cq` | **전파 먼저, 삭제 나중** (UAF 방지) |
| pause(되돌릴 일시정지) | `nvme_start_freeze` → `nvme_quiesce_io_queues` | drain 미대기, 유입+dispatch만 차단 |
| resume | `nvme_unquiesce_io_queues` → `nvme_unfreeze` | **역순 필수**(dispatch 먼저 재개해야 drain 데드락 회피) |
| exception(버림) | `start_freeze`→`quiesce`→**device-stop**→`reap_pending_cqes`→`cancel_tagset`→`sync` | cancel은 반드시 device-stop 이후(DMA 오염 방지) |

---

## 부록 E. 완료(Completion) 정합성 — stale/중복 CQE 방어 4겹

런타임 큐 삭제/재구성·exception 처리 시 가장 위험한 것은 **"이미 끝났거나 free된 명령에 대한 완료가 다시 올라와 엉뚱한 request를 완료시키는 것"**(이중완료·use-after-free)이다. NVMe 드라이버는 이를 **4개 계층**으로 막는다 — detection 2겹 + prevention + recovery.

### E.1 한눈 정리표

| 계층 | 막는 위협 | 성격 | 한계 |
|------|----------|------|------|
| **구조적 순서**(freeze/quiesce + Delete SQ→CQ 핸드셰이크 후 free) | stale 완료·DMA-into-freed 원천 차단 | **prevention (하드 보장)** | 컨트롤러가 응답해야 함 |
| **CQ phase tag** (P 비트) | **같은 CQ 슬롯 재읽기**(옛 lap 찌꺼기) | detection | "새 phase로 올라온 잘못된 완료"는 못 막음 |
| **genctr** (command-id 세대) | 늦은/중복 **명령 식별** 오류(tag 재사용) | detection | 4비트 → 16세대 wrap 시 우연 일치 가능 |
| **timeout → reset** | 위 전부 뚫렸을 때 전체 복구 | recovery | 무거움(컨트롤러 reset) |

핵심: **신뢰의 근거는 detection이 아니라 "구조적 순서"**. phase·genctr는 잔여 윈도우 보조용이다.

### E.2 genctr (generation counter)

**목적**: tag는 명령 완료 후 재사용되므로, 늦게 도착한 옛 CQE가 같은 tag의 **새 명령을 오완료**시키는 것을 막는다.

**인코딩** — command_id 16비트 분할 (nvme.h:645):
```
| xxxx | xxxxxxxxxxxx |
  gen     tag          gen=[15:12](4비트), tag=[11:0](→ depth 최대 4096)
```
```c
nvme_cid(rq)            = (genctr & 0xf) << 12 | rq->tag   // 제출 시 조립 (nvme.h:655)
nvme_genctr_from_cid(c) = (c & 0xf000) >> 12               // CQE에서 추출
```

**생명주기**:
1. 제출: `cmd->common.command_id = nvme_cid(req)` (core.c:1130)
2. 완료: `nvme_try_complete_req()` 에서 `rq->genctr++` (nvme.h, `NVME_QUIRK_SKIP_CID_GEN` 시 생략)
3. CQE 도착: `nvme_find_rq()` (nvme.h:660) 가 CQE의 gen ≠ 현재 gen 이면 **"genctr mismatch" 로 폐기** → `nvme_handle_cqe`(pci.c:1550)가 그 CQE 무시

**한계**: 4비트 → 같은 tag가 그 사이 16의 배수로 재사용되면 세대가 우연 일치 → 못 잡음. 근시간 stale 검출용 best-effort.

### E.3 CQ phase tag (P 비트)

**목적**: 순환 CQ 버퍼에서 **이미 소비한(옛 lap) 엔트리를 새 완료로 오독**하는 것을 막는다. genctr가 못 막는 "같은 물리 슬롯 재읽기"를 담당.

```c
// 이 슬롯이 처리할 새 CQE인지: status bit0(P) == 기대 phase 일 때만 (pci.c)
static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq) {
    return (le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == nvmeq->cq_phase;
}
// CQ 한 바퀴마다 기대 phase 토글
if (++cq_head == q_depth) { cq_head = 0; cq_phase ^= 1; }
```
- device는 새 완료를 현재 phase로 기록, wrap 시 phase 뒤집음. 호스트도 wrap마다 기대 phase `^= 1`.
- 옛 엔트리는 **반대 phase** → `nvme_cqe_pending` false → 읽지 않음.
- 큐 (재)초기화 시 `memset(cqes,0)` + `cq_phase=1` + `cq_head=0` (`nvme_init_queue`, pci.c) → 찌꺼기 원천 제거.

### E.4 phase vs genctr — 상보 관계 (어느 쪽이 무엇을 막나)

| 위협 | phase | genctr |
|------|:---:|:---:|
| 같은 물리 CQ 슬롯 재읽기(찌꺼기 재출현, CID 동일) | ✅ | ❌ (CID 같아 구분 불가) |
| device가 새 phase로 진짜 새 CQE 게시(끝난/취소된 명령) | ❌ (진짜 새 쓰기라 통과) | ✅ (세대 불일치 폐기) |
| 위 둘 다 통과(16세대 wrap 우연 일치) | ❌ | ❌ → **구조적 순서로만 차단** |

### E.5 구조적 순서 = 하드 보장 (Delete SQ→CQ 핸드셰이크)

```c
nvme_delete_io_queues():                          // pci.c
    __nvme_delete_io_queues(dev, delete_sq)   →   // ① SQ 먼저: fetch 중단 + 미처리 폐기
    __nvme_delete_io_queues(dev, delete_cq)       // ② CQ 나중: 완료 시 "더 이상 게시 안 함" 보장
```
- **Delete I/O CQ 완료 = 동기점**: 이후 컨트롤러는 그 CQ에 완료를 게시하지 않음(스펙 보장). → **그제서야 CQ 버퍼 free** 하면 DMA-into-freed 불가.
- detection이 아니라 컨트롤러의 positive handshake라 **가장 강한 보장**.

### E.6 그래서 큐 관리/Exception 설계의 절대 원칙

1. **버퍼 free는 Delete SQ→CQ 완료 확인 후에만** (E.5). 가능하면 **free 자체를 미루고 큐 메모리 재사용**(부록 D.6 / vanilla `nvme_alloc_queue` early-return).
2. **cancel 은 device-stop 이후에만** — host 강제완료가 device의 실제 진행과 겹치지 않게(§exception 경로).
3. **detection(phase·genctr)에만 의존 금지** — 잔여 윈도우가 있으므로, 신뢰는 항상 구조적 순서에 둔다.
4. **최종 안전망은 timeout→reset** — 컨트롤러가 핸드셰이크에 응답하지 않으면 reset으로 전체 정리.
