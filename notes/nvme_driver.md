# NVMe Driver Queue 아키텍처: CPU에서 Device까지

## 목표
CPU에서 NVMe 디바이스까지 모든 큐(queue)의 연결 관계를 파악한다.
`request_queue`, `blk_mq_ctx`(sw queue), `blk_mq_hw_ctx`(hw queue),
`blk_mq_tag_set`, `nvme_queue`, NVMe SQ/CQ의 관계를 이해한다.

---

## 1. 전체 큐 연결 구조 (Big Picture)

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │                     Application (fio, dd, ...)                     │
 └──────────────────────────────┬──────────────────────────────────────┘
                                │  read()/write()/io_uring
                                ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │                      VFS / Page Cache                              │
 └──────────────────────────────┬──────────────────────────────────────┘
                                │  submit_bio()
                                ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │                     Block Layer (blk-mq)                           │
 │                                                                     │
 │  ┌──────────┐  ┌──────────┐  ┌──────────┐        ┌──────────┐     │
 │  │  CPU 0   │  │  CPU 1   │  │  CPU 2   │  ...   │  CPU N   │     │
 │  │ blk_mq_  │  │ blk_mq_  │  │ blk_mq_  │        │ blk_mq_  │     │
 │  │ ctx[0]   │  │ ctx[1]   │  │ ctx[2]   │        │ ctx[N]   │     │
 │  │(SW Queue)│  │(SW Queue)│  │(SW Queue)│        │(SW Queue)│     │
 │  └────┬─────┘  └────┬─────┘  └────┬─────┘        └────┬─────┘     │
 │       │              │              │                   │           │
 │       │    CPU→HW Queue 매핑 (blk_mq_map_queues)       │           │
 │       │              │              │                   │           │
 │       ▼              ▼              ▼                   ▼           │
 │  ┌─────────────────────────┐  ┌─────────────────────────┐          │
 │  │   blk_mq_hw_ctx[0]     │  │   blk_mq_hw_ctx[1]     │   ...    │
 │  │   (HW Queue Context)   │  │   (HW Queue Context)   │          │
 │  │                         │  │                         │          │
 │  │  tags ──► blk_mq_tags  │  │  tags ──► blk_mq_tags  │          │
 │  │  (request pool + tag   │  │  (request pool + tag   │          │
 │  │   bitmap)              │  │   bitmap)              │          │
 │  └────────────┬────────────┘  └────────────┬────────────┘          │
 │               │                             │                       │
 │               │  ◄── blk_mq_tag_set 가 전체를 관리 ──►             │
 │               │      (nr_hw_queues, queue_depth,                   │
 │               │       ops = &nvme_mq_ops)                          │
 └───────────────┼─────────────────────────────┼───────────────────────┘
                 │  .queue_rq = nvme_queue_rq() │
                 ▼                              ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │                     NVMe PCIe Driver                               │
 │                                                                     │
 │  ┌──────────────────────┐      ┌──────────────────────┐            │
 │  │   nvme_queue[0]      │      │   nvme_queue[1]      │    ...    │
 │  │                      │      │                      │            │
 │  │  sq_cmds[] (SQ ring) │      │  sq_cmds[] (SQ ring) │            │
 │  │  cqes[]   (CQ ring)  │      │  cqes[]   (CQ ring)  │            │
 │  │  sq_tail, cq_head    │      │  sq_tail, cq_head    │            │
 │  │  q_db (doorbell ptr) │      │  q_db (doorbell ptr) │            │
 │  └──────────┬───────────┘      └──────────┬───────────┘            │
 └─────────────┼──────────────────────────────┼────────────────────────┘
               │  MMIO Doorbell Write         │
               ▼                              ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │                     NVMe Controller (Hardware)                     │
 │                                                                     │
 │    BAR0 MMIO Registers                                             │
 │    ├── 0x0000-0x0FFF: Controller Registers (CAP, CC, CSTS...)     │
 │    └── 0x1000+: Doorbell Registers                                 │
 │        ├── SQ0 Tail DB │ CQ0 Head DB  (Admin Queue)               │
 │        ├── SQ1 Tail DB │ CQ1 Head DB  (I/O Queue 1)              │
 │        ├── SQ2 Tail DB │ CQ2 Head DB  (I/O Queue 2)              │
 │        └── ...                                                     │
 │                                                                     │
 │    SQ/CQ는 Host 메모리에 있고, Device가 DMA로 접근                 │
 └─────────────────────────────────────────────────────────────────────┘
```

---

## 2. 핵심 구조체 상세 분석

### 2.1 blk_mq_tag_set — 모든 큐의 총괄 관리자

```c
// include/linux/blk-mq.h
struct blk_mq_tag_set {
    const struct blk_mq_ops *ops;    // 드라이버 콜백 (nvme_mq_ops)
    unsigned int     nr_hw_queues;   // HW 큐 개수 (= NVMe I/O 큐 수)
    unsigned int     queue_depth;    // 큐당 최대 outstanding 요청 수
    unsigned int     cmd_size;       // 드라이버별 추가 데이터 크기
    int              numa_node;
    unsigned int     nr_maps;        // CPU→큐 매핑 테이블 수 (보통 3)
    struct blk_mq_queue_map map[HCTX_MAX_TYPES]; // CPU→HW큐 매핑
    struct blk_mq_tags **tags;       // tags[i] = i번 HW큐의 tag 관리
    struct list_head  tag_list;      // 이 tag_set을 사용하는 request_queue 목록
};
```

**핵심 역할:**
- NVMe 드라이버가 `nvme_dev_add()`에서 한 번 초기화
- `nr_hw_queues`개의 HW 큐를 생성하는 설계도 역할
- 모든 `request_queue`(namespace)가 이 tag_set을 공유

**NVMe 드라이버의 초기화:**
```c
// drivers/nvme/host/pci.c — nvme_dev_add()
dev->tagset.ops       = &nvme_mq_ops;
dev->tagset.nr_hw_queues = dev->online_queues - 1; // admin 제외
dev->tagset.queue_depth  = min(dev->q_depth, BLK_MQ_MAX_DEPTH) - 1;
dev->tagset.cmd_size     = sizeof(struct nvme_iod);
dev->tagset.numa_node    = dev->ctrl.numa_node;
dev->tagset.flags        = BLK_MQ_F_SHOULD_MERGE;
blk_mq_alloc_tag_set(&dev->tagset);
```

---

### 2.2 blk_mq_ctx — Software Queue (per-CPU)

```c
// block/blk-mq.h (내부 헤더)
struct blk_mq_ctx {
    struct {
        spinlock_t      lock;
        struct list_head rq_lists[HCTX_MAX_TYPES]; // 대기 request 리스트
    } ____cacheline_aligned_in_smp;

    unsigned int        cpu;          // 이 sw큐가 담당하는 CPU 번호
    unsigned short      index_hw[HCTX_MAX_TYPES]; // 매핑된 HW큐 인덱스
    struct blk_mq_hw_ctx *hctxs[HCTX_MAX_TYPES];  // 매핑된 HW큐 포인터

    struct request_queue *queue;      // 소속 request_queue
    struct kobject       kobj;
};
```

**핵심 역할:**
- **CPU마다 1개** 존재 (per-cpu 구조)
- bio를 request로 변환 후 임시 보관
- lock contention 최소화 (각 CPU가 자기 큐에만 접근)
- `index_hw[]`로 자신이 어떤 HW 큐에 매핑되는지 알고 있음

**매핑 관계:**
```
CPU 개수 > HW Queue 개수 (일반적)

예: 8 CPU, 4 NVMe I/O Queue
  CPU 0 → blk_mq_ctx[0] → hctx[0] → nvme_queue[1]
  CPU 1 → blk_mq_ctx[1] → hctx[0] → nvme_queue[1]
  CPU 2 → blk_mq_ctx[2] → hctx[1] → nvme_queue[2]
  CPU 3 → blk_mq_ctx[3] → hctx[1] → nvme_queue[2]
  CPU 4 → blk_mq_ctx[4] → hctx[2] → nvme_queue[3]
  CPU 5 → blk_mq_ctx[5] → hctx[2] → nvme_queue[3]
  CPU 6 → blk_mq_ctx[6] → hctx[3] → nvme_queue[4]
  CPU 7 → blk_mq_ctx[7] → hctx[3] → nvme_queue[4]
```

---

### 2.3 blk_mq_hw_ctx — Hardware Queue Context

```c
// include/linux/blk-mq.h
struct blk_mq_hw_ctx {
    struct {
        spinlock_t       lock;
        struct list_head dispatch;   // 디스패치 대기 리스트
    } ____cacheline_aligned_in_smp;

    struct blk_mq_tags   *tags;       // 이 HW큐의 tag 관리자
    struct blk_mq_tags   *sched_tags; // I/O 스케줄러용 tags
    unsigned long        *tags_bitmap; // tag 할당 비트맵
    unsigned int         queue_num;   // HW 큐 인덱스 (0-based)
    unsigned int         nr_ctx;      // 매핑된 SW큐(CPU) 개수
    struct blk_mq_ctx    **ctxs;      // 매핑된 SW큐 배열

    struct request_queue *queue;      // 소속 request_queue
    struct blk_mq_tag_set *tag_set;

    cpumask_var_t        cpumask;     // 이 HW큐를 사용하는 CPU들
    atomic_t             nr_active;   // 현재 활성 request 수

    unsigned int         type;        // HCTX_TYPE_DEFAULT/READ/POLL
    void                 *driver_data; // 드라이버 전용 데이터
                                       // NVMe: nvme_queue 포인터
};
```

**핵심 역할:**
- blk-mq에서 **실제 디스패치 단위**
- 여러 SW 큐(CPU)의 request를 모아서 드라이버에 전달
- `driver_data`가 NVMe의 `nvme_queue`를 가리킴 → **1:1 매핑**
- `tags`로 request 객체 풀 관리 + command ID(tag) 할당

---

### 2.4 request_queue — Block Device의 요청 큐

```c
// include/linux/blkdev.h
struct request_queue {
    struct blk_mq_tag_set  *tag_set;     // 연결된 tag_set
    struct blk_mq_hw_ctx   **queue_hw_ctx; // HW큐 배열 (인덱스로 접근)
    unsigned int           nr_hw_queues;   // HW큐 개수

    struct blk_mq_ctx __percpu *queue_ctx; // per-CPU SW큐

    struct gendisk         *disk;
    struct elevator_queue  *elevator;     // I/O 스케줄러

    unsigned long          queue_flags;
    unsigned int           nr_requests;   // 최대 request 수
    // ... 기타 필드
};
```

**핵심 역할:**
- **NVMe namespace당 1개** 생성 (예: /dev/nvme0n1)
- 하지만 같은 NVMe controller의 namespace들은 **tag_set을 공유**
- SW큐와 HW큐를 모두 보유하며 연결하는 허브 역할

**생성 과정:**
```c
// drivers/nvme/host/core.c — nvme_alloc_ns()
ns->disk = blk_mq_alloc_disk(ctrl->tagset, ...);
// 내부에서:
//   1. request_queue 생성
//   2. tag_set의 nr_hw_queues만큼 blk_mq_hw_ctx 생성
//   3. CPU마다 blk_mq_ctx 생성
//   4. CPU→HW큐 매핑 설정
```

---

### 2.5 nvme_queue — NVMe PCIe 드라이버의 큐

```c
// drivers/nvme/host/pci.c
struct nvme_queue {
    struct nvme_dev      *dev;          // NVMe 디바이스
    spinlock_t           sq_lock;       // SQ 보호용 락
    void                 *sq_cmds;      // Submission Queue 링 버퍼 (DMA)
    struct nvme_completion *cqes;       // Completion Queue 링 버퍼 (DMA)
    dma_addr_t           sq_dma_addr;   // SQ 물리 주소 (DMA용)
    dma_addr_t           cq_dma_addr;   // CQ 물리 주소 (DMA용)

    u32 __iomem          *q_db;         // Doorbell 레지스터 베이스
                                        // q_db[0] = SQ Tail DB
                                        // q_db[1] = CQ Head DB

    u16                  q_depth;       // 큐 깊이
    u16                  sq_tail;       // SQ tail 인덱스 (호스트가 관리)
    u16                  last_sq_tail;  // 마지막 DB write 시점의 tail
    u16                  cq_head;       // CQ head 인덱스 (호스트가 관리)
    u16                  qid;           // Queue ID (0=admin, 1+=I/O)
    u8                   cq_phase;      // CQ phase bit (wrap 시 토글)

    u32                  *dbbuf_sq_db;  // Shadow Doorbell (최적화)
    u32                  *dbbuf_cq_db;
    u32                  *dbbuf_sq_ei;  // Event Index (최적화)
    u32                  *dbbuf_cq_ei;
};
```

**핵심 역할:**
- NVMe 하드웨어와 직접 통신하는 최하위 큐
- SQ/CQ 링 버퍼의 DMA 메모리 관리
- Doorbell 레지스터로 디바이스에 신호 전달
- `blk_mq_hw_ctx->driver_data`로 연결됨

---

## 3. 큐 연결 관계 (Reference Chain)

```
┌─────────────────────────────────────────────────────────────┐
│                    참조 관계도                               │
│                                                             │
│  blk_mq_tag_set (1개, NVMe Controller 당)                  │
│    │                                                        │
│    ├── .ops = &nvme_mq_ops                                 │
│    │     └── .queue_rq  = nvme_queue_rq   (요청 제출)      │
│    │     └── .complete  = nvme_pci_complete_rq              │
│    │     └── .map_queues = nvme_pci_map_queues              │
│    │                                                        │
│    ├── .tags[i] ──► blk_mq_tags                            │
│    │                  └── .static_rqs[] (pre-alloc 요청들)  │
│    │                  └── .bitmap_tags  (tag 할당 비트맵)   │
│    │                                                        │
│    └── .tag_list ──► request_queue ──► request_queue ──►   │
│         (namespace마다 하나의 request_queue)                │
│                                                             │
│  request_queue (1개, namespace 당, 예: /dev/nvme0n1)       │
│    │                                                        │
│    ├── .tag_set ──────────► blk_mq_tag_set (위)            │
│    │                                                        │
│    ├── .queue_ctx (per-CPU)                                 │
│    │    ├── ctx[0] ──► blk_mq_ctx (CPU 0용 SW큐)          │
│    │    ├── ctx[1] ──► blk_mq_ctx (CPU 1용 SW큐)          │
│    │    └── ...                                             │
│    │                                                        │
│    └── .queue_hw_ctx[]                                      │
│         ├── hctx[0] ──► blk_mq_hw_ctx                      │
│         │    ├── .driver_data ──► nvme_queue[1]             │
│         │    ├── .tags ──► blk_mq_tags                     │
│         │    └── .ctxs[] = {ctx[0], ctx[1]} (매핑된 CPU들) │
│         │                                                   │
│         ├── hctx[1] ──► blk_mq_hw_ctx                      │
│         │    ├── .driver_data ──► nvme_queue[2]             │
│         │    └── .ctxs[] = {ctx[2], ctx[3]}                │
│         └── ...                                             │
│                                                             │
│  nvme_queue[i] (NVMe I/O Queue)                            │
│    ├── .sq_cmds ──► SQ 링버퍼 (Host 메모리, DMA)          │
│    ├── .cqes    ──► CQ 링버퍼 (Host 메모리, DMA)          │
│    └── .q_db    ──► MMIO Doorbell 레지스터 (BAR0)          │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. I/O 요청의 전체 흐름

### 4.1 제출 경로 (Submission Path)

```
① Application: write(fd, buf, 4096)
        │
        ▼
② VFS: vfs_write() → ... → ext4_writepages()
        │
        ▼
③ submit_bio(bio)
   │
   │  bio에는 논리 블록 주소(LBA), 데이터 페이지, 길이 등 포함
        │
        ▼
④ blk_mq_submit_bio()
   │
   ├── 현재 CPU 번호로 blk_mq_ctx 결정
   │   ctx = rq->mq_ctx = __blk_mq_get_ctx(q, cpu)
   │
   ├── ctx가 매핑된 blk_mq_hw_ctx 결정
   │   hctx = ctx->hctxs[type]
   │
   ├── blk_mq_get_tag()로 tag 할당
   │   tag = blk_mq_get_tag(&hctx->tags)
   │   → tag는 0 ~ queue_depth-1 범위의 정수
   │   → NVMe command의 CID(Command ID)로 사용됨
   │
   ├── request 객체 획득 (tag로 인덱싱)
   │   rq = hctx->tags->static_rqs[tag]
   │
   └── blk_mq_run_hw_queue(hctx)
       │
       ▼
⑤ blk_mq_dispatch_rq_list()
   │
   └── hctx->tags->ops->queue_rq(hctx, &bd)
       │
       ▼
⑥ nvme_queue_rq(hctx, bd)           ← NVMe 드라이버 진입점
   │
   ├── nvmeq = hctx->driver_data     ← nvme_queue 획득
   │
   ├── nvme_setup_cmd(ns, rq, &cmnd)
   │   └── bio → NVMe Command 변환
   │       ├── Read  → nvme_setup_rw() → opcode = nvme_cmd_read
   │       └── Write → nvme_setup_rw() → opcode = nvme_cmd_write
   │
   ├── nvme_map_data(dev, rq, &cmnd)
   │   └── DMA 매핑: 데이터 페이지의 물리 주소 → PRP/SGL 리스트 구성
   │
   └── nvme_submit_cmd(nvmeq, &cmnd)
       │
       ▼
⑦ nvme_submit_cmd(nvmeq, &cmnd)
   │
   ├── spin_lock(&nvmeq->sq_lock)
   │
   ├── memcpy(&nvmeq->sq_cmds[sq_tail], &cmnd, sizeof(cmnd))
   │   └── NVMe 커맨드를 SQ 링버퍼에 복사
   │
   ├── sq_tail = (sq_tail + 1) % q_depth
   │   └── tail 포인터 전진 (circular)
   │
   ├── writel(sq_tail, nvmeq->q_db)
   │   └── ★ SQ Tail Doorbell에 MMIO Write
   │   └── 이 순간 NVMe 컨트롤러가 SQ에서 커맨드를 Fetch하기 시작
   │
   └── spin_unlock(&nvmeq->sq_lock)
```

### 4.2 완료 경로 (Completion Path)

```
① NVMe Controller: 커맨드 실행 완료
   │
   ├── CQ 엔트리를 Host 메모리에 DMA Write
   │   nvmeq->cqes[cq_head] = {
   │       .command_id = tag,     // 어떤 request인지 식별
   │       .sq_head    = ...,     // SQ에서 처리한 위치
   │       .status     = ...,     // 성공/실패
   │       .phase      = ...      // Phase bit
   │   }
   │
   └── MSI-X 인터럽트 발생
        │
        ▼
② nvme_irq(irq, data)              ← 인터럽트 핸들러
   │
   └── nvme_process_cq(nvmeq)
       │
       ▼
③ nvme_process_cq(nvmeq)
   │
   ├── while (cqes[cq_head].phase == cq_phase)  // 새 완료 확인
   │   │
   │   ├── command_id = cqes[cq_head].command_id
   │   │   └── = blk-mq tag = request 식별자
   │   │
   │   ├── rq = blk_mq_tag_to_rq(nvmeq->tags, command_id)
   │   │   └── tag로 struct request 복원
   │   │
   │   ├── cq_head = (cq_head + 1) % q_depth
   │   │   if (cq_head == 0) cq_phase ^= 1  // phase 토글
   │   │
   │   └── nvme_end_request(rq, status)
   │       └── blk_mq_complete_request(rq)
   │           └── 최종적으로 bio->bi_end_io() 호출
   │               → 애플리케이션에 완료 통지
   │
   └── writel(cq_head, nvmeq->q_db + 1)
       └── ★ CQ Head Doorbell에 MMIO Write
       └── "여기까지 처리했다"고 디바이스에 알림
```

---

## 5. Tag의 역할 — 요청 추적의 핵심

```
Tag = Command ID = Request 식별자

                    blk_mq_tags
                    ┌────────────────────────────┐
                    │  bitmap_tags               │
                    │  ┌──┬──┬──┬──┬──┬──┬──┐   │
                    │  │0 │1 │2 │3 │4 │..│N │   │  N = queue_depth
                    │  │■ │□ │■ │□ │■ │  │□ │   │  ■=사용중, □=비어있음
                    │  └──┴──┴──┴──┴──┴──┴──┘   │
                    │                            │
                    │  static_rqs[]              │
                    │  ┌────────────────────┐    │
                    │  │ [0] → request      │    │ ← pre-allocated
                    │  │ [1] → request      │    │
                    │  │ [2] → request      │    │
                    │  │ ...                │    │
                    │  │ [N] → request      │    │
                    │  └────────────────────┘    │
                    └────────────────────────────┘

제출 시: tag = blk_mq_get_tag()  → bitmap에서 빈 슬롯 할당
         rq = tags->static_rqs[tag]
         rq->tag = tag
         NVMe cmd.command_id = tag

완료 시: command_id = cqe->command_id  (디바이스가 돌려줌)
         rq = tags->static_rqs[command_id]
         blk_mq_put_tag()  → bitmap 해제
```

**왜 Tag가 중요한가?**
- NVMe는 큐에 **여러 커맨드를 동시에** 제출 (queue_depth만큼)
- 완료 순서는 제출 순서와 **다를 수 있음** (out-of-order completion)
- Command ID(=tag)가 있어야 어떤 요청이 완료되었는지 식별 가능

---

## 6. 큐 개수 결정 과정

### 6.1 NVMe I/O 큐 수 결정

```c
// drivers/nvme/host/pci.c — nvme_setup_io_queues()

// 1단계: 원하는 큐 수 계산
nr_io_queues = min(num_possible_cpus(), max_queues);
//            = min(CPU 수, 디바이스 지원 최대 큐 수)

// 2단계: Set Feature 커맨드로 디바이스에 요청
nvme_set_queue_count(ctrl, &nr_io_queues);
// 디바이스가 실제 할당 가능한 수로 조정하여 반환

// 3단계: 큐 할당
for (i = dev->online_queues; i <= nr_io_queues; i++)
    nvme_alloc_queue(dev, i, dev->q_depth);
    // SQ/CQ DMA 메모리 할당 + doorbell 주소 설정

// 4단계: Create I/O CQ & SQ 커맨드
for (i = dev->online_queues; i <= nr_io_queues; i++) {
    nvme_create_queue(&dev->queues[i], i, ...);
    // Admin Command: Create I/O Completion Queue
    // Admin Command: Create I/O Submission Queue
}
```

### 6.2 CPU → Queue 매핑 (3가지 유형)

```c
// NVMe 드라이버의 map_queues 콜백
static void nvme_pci_map_queues(struct blk_mq_tag_set *set) {
    // Type 0: HCTX_TYPE_DEFAULT (일반 I/O)
    //   → write 큐로 매핑
    map->nr_queues = dev->io_queues[HCTX_TYPE_DEFAULT];
    blk_mq_map_queues(&set->map[HCTX_TYPE_DEFAULT]);

    // Type 1: HCTX_TYPE_READ (읽기 전용)
    //   → read 큐로 매핑 (지원 시)
    map->nr_queues = dev->io_queues[HCTX_TYPE_READ];

    // Type 2: HCTX_TYPE_POLL (폴링 모드)
    //   → poll 큐로 매핑 (인터럽트 없이 busy-wait)
    map->nr_queues = dev->io_queues[HCTX_TYPE_POLL];
}
```

```
예시: 16 CPU, NVMe가 8개 큐 지원

  Default 큐: 4개 (write + 일반 I/O)
  Read 큐:    2개 (read 전용)
  Poll 큐:    2개 (io_uring poll 모드)

  CPU  0,1,2,3 → Default hctx[0] → nvme_queue[1]
  CPU  4,5,6,7 → Default hctx[1] → nvme_queue[2]
  CPU  0,1,2,3 → Read    hctx[2] → nvme_queue[5]  (read일 때)
  CPU  4,5,6,7 → Read    hctx[3] → nvme_queue[6]  (read일 때)
  CPU  8~11    → Poll    hctx[4] → nvme_queue[7]
  CPU 12~15    → Poll    hctx[5] → nvme_queue[8]
```

---

## 7. nvme_alloc_queue() — NVMe 큐 메모리 할당

```c
// drivers/nvme/host/pci.c
static int nvme_alloc_queue(struct nvme_dev *dev, int qid, int depth)
{
    struct nvme_queue *nvmeq = &dev->queues[qid];

    // CQ 할당 (DMA coherent memory)
    nvmeq->cqes = dma_alloc_coherent(dev->dev,
                      depth * sizeof(struct nvme_completion),
                      &nvmeq->cq_dma_addr, GFP_KERNEL);

    // SQ 할당 (DMA coherent memory, 또는 CMB)
    if (use_cmb) {
        // Controller Memory Buffer 사용 (디바이스 메모리에 SQ 배치)
        nvmeq->sq_cmds = nvme_cmb_alloc(dev, ...);
    } else {
        nvmeq->sq_cmds = dma_alloc_coherent(dev->dev,
                              depth * sizeof(struct nvme_command),
                              &nvmeq->sq_dma_addr, GFP_KERNEL);
    }

    // Doorbell 주소 계산
    nvmeq->q_db = dev->dbs + (qid * 2 * dev->db_stride);
    //                        │         │
    //                        │         └── stride: 보통 1 (4바이트 단위)
    //                        └── 큐마다 SQ+CQ 2개의 doorbell

    nvmeq->q_depth = depth;
    nvmeq->qid = qid;
    nvmeq->cq_phase = 1;  // 초기 phase bit
    spin_lock_init(&nvmeq->sq_lock);

    return 0;
}
```

---

## 8. Doorbell과 DMA 메커니즘

### 8.1 SQ/CQ 동작 원리

```
                     Host Memory                    NVMe Controller
                  ┌──────────────┐                 ┌──────────────┐
                  │              │                 │              │
  ① Host가       │  SQ[0]  cmd  │ ──── DMA ────►  │  Fetch &     │
  SQ에 커맨드    │  SQ[1]  cmd  │    Read          │  Execute     │
  기록           │  SQ[2]  ...  │                  │              │
                  │    ↑ sq_tail │                 │              │
                  │              │                 │              │
  ② Host가       │              │                 │              │
  Doorbell에     │              │  ◄── MMIO ────  │  q_db[0]     │
  sq_tail 기록   │              │    Write         │  (SQ Tail DB)│
                  │              │                 │              │
  ③ Device가     │              │                 │              │
  CQ에 결과      │  CQ[0]  cpl  │ ◄─── DMA ─────  │  Complete &  │
  DMA Write     │  CQ[1]  cpl  │    Write         │  Write CQE   │
                  │    ↑ cq_head │                 │              │
                  │              │                 │              │
  ④ Device가     │              │                 │  MSI-X IRQ   │
  인터럽트 발생  │              │ ◄── Interrupt ── │  ──────────► │
                  │              │                 │              │
  ⑤ Host가       │              │                 │              │
  Doorbell에     │              │  ──── MMIO ───► │  q_db[1]     │
  cq_head 기록   │              │    Write         │  (CQ Head DB)│
                  └──────────────┘                 └──────────────┘
```

### 8.2 Shadow Doorbell (성능 최적화)

```
일반 Doorbell:
  매번 MMIO Write → PCIe 버스 왕복 → 느림 (~수백 ns)

Shadow Doorbell:
  ┌──────────────────────────────────────────────────┐
  │  Host Memory                                     │
  │  ┌──────────────┐  ┌──────────────┐              │
  │  │ dbbuf_sq_db  │  │ dbbuf_sq_ei  │              │
  │  │ (shadow DB)  │  │ (event idx)  │              │
  │  └──────────────┘  └──────────────┘              │
  │                                                   │
  │  제출 시:                                         │
  │  1. shadow DB에 sq_tail 기록 (메모리 Write, 빠름)│
  │  2. event_idx와 비교                              │
  │  3. 실제 MMIO Write 필요한 경우에만 doorbell 기록 │
  │     → MMIO Write 횟수 대폭 감소!                 │
  └──────────────────────────────────────────────────┘
```

---

## 9. Admin Queue vs I/O Queue

```
┌─────────────────────────────────────────────┐
│          NVMe Controller                     │
│                                              │
│  Admin Queue (qid = 0)                      │
│  ├── 컨트롤러 관리 명령 전용                │
│  ├── Create I/O CQ/SQ                       │
│  ├── Identify Controller/Namespace           │
│  ├── Set/Get Features                        │
│  ├── Firmware 관리                           │
│  └── blk-mq와 연결 안 됨                    │
│      (별도의 request_queue 사용)             │
│                                              │
│  I/O Queue (qid = 1, 2, ..., N)             │
│  ├── 실제 데이터 Read/Write                 │
│  ├── blk_mq_hw_ctx와 1:1 매핑              │
│  ├── CPU affinity로 큐 배정                 │
│  └── 큐 개수 = min(CPU수, 디바이스 지원 수) │
│                                              │
└─────────────────────────────────────────────┘
```

---

## 10. 전체 초기화 순서

```
nvme_probe()                              ← PCIe 드라이버 probe
  │
  ├── nvme_dev_alloc()                    ← nvme_dev 구조체 할당
  │
  ├── nvme_setup_prp_pools()              ← PRP DMA pool 생성
  │
  ├── nvme_pci_alloc_ctrl()
  │   └── nvme_alloc_queue(dev, 0, ...)   ← Admin Queue 할당 (qid=0)
  │
  ├── nvme_pci_enable()
  │   ├── pci_enable_device()
  │   ├── pci_request_regions()
  │   └── ioremap(BAR0)                  ← MMIO 레지스터 매핑
  │
  ├── nvme_configure_admin_queue()
  │   ├── AQA 레지스터 설정 (Admin Queue Attributes)
  │   ├── ASQ 레지스터 = admin SQ DMA 주소
  │   ├── ACQ 레지스터 = admin CQ DMA 주소
  │   └── CC.EN = 1 (컨트롤러 활성화)
  │
  ├── nvme_setup_io_queues()
  │   ├── nr_io_queues 결정 (CPU 수 기반)
  │   ├── nvme_set_queue_count()          ← Set Feature로 큐 수 협상
  │   ├── pci_alloc_irq_vectors()         ← MSI-X 인터럽트 벡터 할당
  │   ├── for each I/O queue:
  │   │   └── nvme_alloc_queue(dev, i, depth)
  │   └── for each I/O queue:
  │       └── nvme_create_queue()
  │           ├── Create I/O CQ (Admin Cmd)
  │           └── Create I/O SQ (Admin Cmd)
  │
  ├── nvme_dev_add()
  │   ├── blk_mq_alloc_tag_set()          ← tag_set 초기화
  │   │   └── ops = &nvme_mq_ops
  │   │       nr_hw_queues = online_queues - 1
  │   │       queue_depth = q_depth - 1
  │   └── nvme_scan_work() → nvme_alloc_ns()
  │       └── blk_mq_alloc_disk(tagset)
  │           ├── request_queue 생성
  │           ├── blk_mq_hw_ctx[] 생성
  │           ├── blk_mq_ctx[] 생성 (per-CPU)
  │           └── CPU→HW큐 매핑 설정
  │
  └── /dev/nvme0n1 등록 완료!
```

---

## 11. SPDK와의 비교

```
┌──────────────────┬───────────────────────┬──────────────────────┐
│     항목         │    Linux Kernel       │    SPDK              │
├──────────────────┼───────────────────────┼──────────────────────┤
│ 큐 계층 수       │ 4단계                 │ 2단계                │
│                  │ sw_q→hw_q→nvme_q→SQ  │ qpair→SQ             │
│                  │                       │ (blk-mq 없음)        │
├──────────────────┼───────────────────────┼──────────────────────┤
│ CPU→큐 매핑      │ blk_mq_map_queues()  │ 애플리케이션이 직접  │
│                  │ (자동)                │ qpair을 thread에 배정│
├──────────────────┼───────────────────────┼──────────────────────┤
│ Tag/CID 관리     │ blk_mq_tags bitmap   │ tracker pool         │
│                  │ (커널이 관리)          │ (free/outstanding)   │
├──────────────────┼───────────────────────┼──────────────────────┤
│ 동기화           │ spinlock (sq_lock)    │ Lock-free            │
│                  │                       │ (단일 thread per 큐) │
├──────────────────┼───────────────────────┼──────────────────────┤
│ 완료 처리        │ MSI-X 인터럽트        │ Polling              │
│                  │ + softirq             │ (busy-wait)          │
├──────────────────┼───────────────────────┼──────────────────────┤
│ Doorbell         │ 매 submission마다     │ Batching 가능        │
│                  │ MMIO Write            │ (여러 cmd 후 1회)    │
├──────────────────┼───────────────────────┼──────────────────────┤
│ Context Switch   │ 있음 (커널 진입)      │ 없음 (user-space)    │
├──────────────────┼───────────────────────┼──────────────────────┤
│ 레이턴시         │ ~10-20 μs             │ ~1-3 μs              │
└──────────────────┴───────────────────────┴──────────────────────┘
```

---

## 12. 핵심 요약

```
★ 한 줄 요약:
bio → blk_mq_ctx(SW큐) → blk_mq_hw_ctx(HW큐) → nvme_queue → SQ → Doorbell → Device

★ 매핑 관계:
- CPU : blk_mq_ctx     = N : N  (CPU마다 1개)
- blk_mq_ctx : hctx    = M : 1  (여러 CPU가 하나의 HW큐 공유 가능)
- hctx : nvme_queue     = 1 : 1  (HW큐와 NVMe큐는 1:1)
- nvme_queue : SQ/CQ    = 1 : 1  (큐마다 SQ+CQ 한 쌍)
- tag_set : hctx        = 1 : N  (tag_set이 모든 HW큐 관리)
- tag_set : request_queue = 1 : M (여러 namespace가 tag_set 공유)

★ Tag(Command ID)의 흐름:
  할당: blk_mq_get_tag() → rq->tag = tag → NVMe cmd.cid = tag
  반환: cqe.command_id → rq 복원 → blk_mq_put_tag()
```

---

## 참고 소스 파일

| 파일 | 핵심 내용 |
|------|-----------|
| `drivers/nvme/host/pci.c` | nvme_queue, nvme_queue_rq, nvme_setup_io_queues |
| `drivers/nvme/host/core.c` | nvme_alloc_ns, namespace/request_queue 생성 |
| `include/linux/blk-mq.h` | blk_mq_tag_set, blk_mq_hw_ctx 정의 |
| `block/blk-mq.h` | blk_mq_ctx (내부 구조체) 정의 |
| `block/blk-mq.c` | blk_mq_submit_bio, blk_mq_dispatch_rq_list |
| `include/linux/blkdev.h` | request_queue 정의 |
