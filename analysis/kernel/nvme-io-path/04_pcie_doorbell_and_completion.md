# NVMe PCIe Doorbell과 Completion 메커니즘 심층 분석

> 분석 대상: Linux Kernel `drivers/nvme/host/pci.c`, `include/linux/nvme.h`
> 이 문서는 NVMe 컨트롤러의 PCIe BAR0 레지스터 맵, Doorbell 메커니즘, 인터럽트 처리, 폴링 모드, DMA 주소 매핑, PCIe TLP 관점의 I/O 흐름을 상세히 분석한다.

---

## 1. NVMe PCIe 레지스터 맵

NVMe 컨트롤러는 PCIe BAR0 영역에 모든 제어 레지스터를 매핑한다. 호스트 드라이버는 이 레지스터들을 MMIO(Memory-Mapped I/O)로 접근한다.

### 1.1 BAR0 레지스터 레이아웃

```
 PCIe BAR0 MMIO 영역
 ┌─────────────────────────────────────────────────────────────┐
 │  오프셋    │  크기  │  레지스터명                           │
 ├────────────┼────────┼───────────────────────────────────────┤
 │  0x0000    │  8B    │  CAP   (Controller Capabilities)      │  ← 읽기 전용
 │  0x0008    │  4B    │  VS    (Version)                      │  ← 읽기 전용
 │  0x000C    │  4B    │  INTMS (Interrupt Mask Set)            │  ← 쓰기 전용
 │  0x0010    │  4B    │  INTMC (Interrupt Mask Clear)          │  ← 쓰기 전용
 │  0x0014    │  4B    │  CC    (Controller Configuration)      │  ← 읽기/쓰기
 │  0x0018    │  4B    │  Reserved                              │
 │  0x001C    │  4B    │  CSTS  (Controller Status)             │  ← 읽기 전용*
 │  0x0020    │  4B    │  NSSR  (NVM Subsystem Reset)           │  ← 읽기/쓰기
 │  0x0024    │  4B    │  AQA   (Admin Queue Attributes)        │  ← 읽기/쓰기
 │  0x0028    │  8B    │  ASQ   (Admin SQ Base Address)         │  ← 읽기/쓰기
 │  0x0030    │  8B    │  ACQ   (Admin CQ Base Address)         │  ← 읽기/쓰기
 │  0x0038    │  4B    │  CMBLOC (CMB Location)                 │  ← 읽기 전용
 │  0x003C    │  4B    │  CMBSZ  (CMB Size)                     │  ← 읽기 전용
 │  0x0040    │  4B    │  BPINFO (Boot Partition Info)           │
 │  0x0044    │  4B    │  BPRSEL (Boot Partition Read Select)    │
 │  0x0048    │  8B    │  BPMBL  (Boot Partition Mem Buffer Loc) │
 │  0x0050    │  8B    │  CMBMSC (CMB Memory Space Control)      │
 │  0x0068    │  4B    │  CRTO   (Controller Ready Timeouts)     │
 │  ...       │        │  (Reserved)                             │
 │  0x0E00    │  4B    │  PMRCAP (PMR Capabilities)              │
 │  0x0E04    │  4B    │  PMRCTL (PMR Control)                   │
 │  0x0E08    │  4B    │  PMRSTS (PMR Status)                    │
 │  0x0E0C    │  4B    │  PMREBS (PMR Elasticity Buffer Size)    │
 │  0x0E10    │  4B    │  PMRSWTP (PMR Sustained Write Through.) │
 │  ...       │        │  (Reserved)                             │
 │  0x1000    │  4B    │  SQ 0 Tail Doorbell                    │  ← Doorbell 시작
 │  0x1000    │        │  + (4 << DSTRD)                        │
 │            │  4B    │  CQ 0 Head Doorbell                    │
 │  ...       │        │  (I/O Queue Doorbells 반복)             │
 └─────────────────────────────────────────────────────────────┘
```

커널 코드에서 레지스터 오프셋은 `include/linux/nvme.h`에 enum으로 정의되어 있다:

```c
enum {
    NVME_REG_CAP    = 0x0000,   /* Controller Capabilities */
    NVME_REG_VS     = 0x0008,   /* Version */
    NVME_REG_INTMS  = 0x000c,   /* Interrupt Mask Set */
    NVME_REG_INTMC  = 0x0010,   /* Interrupt Mask Clear */
    NVME_REG_CC     = 0x0014,   /* Controller Configuration */
    NVME_REG_CSTS   = 0x001c,   /* Controller Status */
    NVME_REG_NSSR   = 0x0020,   /* NVM Subsystem Reset */
    NVME_REG_AQA    = 0x0024,   /* Admin Queue Attributes */
    NVME_REG_ASQ    = 0x0028,   /* Admin SQ Base Address */
    NVME_REG_ACQ    = 0x0030,   /* Admin CQ Base Address */
    NVME_REG_DBS    = 0x1000,   /* SQ 0 Tail Doorbell */
};
```

### 1.2 CAP 레지스터 (0x00) - Controller Capabilities

64비트 읽기 전용 레지스터로, 컨트롤러의 근본적인 능력을 기술한다.

```
 비트 레이아웃:
 ┌──────┬───────┬──────────────────────────────────────────┐
 │ 비트 │ 필드  │ 설명                                     │
 ├──────┼───────┼──────────────────────────────────────────┤
 │15:00 │ MQES  │ Max Queue Entries Supported (0-based)     │
 │      │       │ 실제 최대 = MQES + 1                     │
 │23:16 │ Rsvd  │ 예약                                     │
 │31:24 │ TO    │ Timeout (500ms 단위)                     │
 │      │       │ CC.EN=1 설정 후 CSTS.RDY=1까지 대기 시간 │
 │35:32 │ DSTRD │ Doorbell Stride                          │
 │      │       │ Doorbell 간격 = 4 << DSTRD 바이트        │
 │36    │ NSSRS │ NVM Subsystem Reset Supported             │
 │44:37 │ CSS   │ Command Sets Supported (비트마스크)       │
 │47:45 │ Rsvd  │ 예약                                     │
 │51:48 │MPSMIN │ Memory Page Size Minimum                  │
 │      │       │ 최소 페이지 = 2^(12+MPSMIN)              │
 │55:52 │MPSMAX │ Memory Page Size Maximum                  │
 │      │       │ 최대 페이지 = 2^(12+MPSMAX)              │
 │57    │ CMBS  │ CMB Supported                             │
 │63:58 │ Rsvd  │ 예약                                     │
 └──────┴───────┴──────────────────────────────────────────┘
```

커널에서 CAP 레지스터 필드를 추출하는 매크로:

```c
#define NVME_CAP_MQES(cap)    ((cap) & 0xffff)          /* 최대 큐 엔트리 수 */
#define NVME_CAP_TIMEOUT(cap) (((cap) >> 24) & 0xff)    /* 타임아웃 (500ms 단위) */
#define NVME_CAP_STRIDE(cap)  (((cap) >> 32) & 0xf)     /* Doorbell 스트라이드 */
#define NVME_CAP_NSSRC(cap)   (((cap) >> 36) & 0x1)     /* 서브시스템 리셋 지원 */
#define NVME_CAP_CSS(cap)     (((cap) >> 37) & 0xff)    /* 커맨드 셋 */
#define NVME_CAP_MPSMIN(cap)  (((cap) >> 48) & 0xf)     /* 최소 페이지 크기 */
#define NVME_CAP_MPSMAX(cap)  (((cap) >> 52) & 0xf)     /* 최대 페이지 크기 */
#define NVME_CAP_CMBS(cap)    (((cap) >> 57) & 0x1)     /* CMB 지원 */
```

`nvme_pci_enable()`에서 CAP을 읽어서 큐 깊이와 Doorbell 스트라이드를 설정한다:

```c
/* pci.c: nvme_pci_enable() */
dev->ctrl.cap = lo_hi_readq(dev->bar + NVME_REG_CAP);
dev->q_depth = min_t(u32, NVME_CAP_MQES(dev->ctrl.cap) + 1, io_queue_depth);
dev->db_stride = 1 << NVME_CAP_STRIDE(dev->ctrl.cap);
dev->dbs = dev->bar + 4096;  /* 0x1000 = NVME_REG_DBS */
```

DSTRD가 0이면 `db_stride = 1 << 0 = 1`이고, 실제 Doorbell 간격은 `4 * 1 = 4바이트`가 된다. 대부분의 NVMe 디바이스는 DSTRD=0을 사용한다.

### 1.3 CC 레지스터 (0x14) - Controller Configuration

호스트가 컨트롤러를 구성하기 위해 쓰는 32비트 레지스터이다.

```
 비트 레이아웃:
 ┌──────┬────────┬──────────────────────────────────────────┐
 │ 비트 │ 필드   │ 설명                                     │
 ├──────┼────────┼──────────────────────────────────────────┤
 │  0   │ EN     │ Enable: 1=컨트롤러 활성화, 0=비활성화    │
 │ 3:1  │ Rsvd   │ 예약                                     │
 │ 6:4  │ CSS    │ I/O Command Set Selected                  │
 │10:7  │ MPS    │ Memory Page Size: 2^(12+MPS)              │
 │13:11 │ AMS    │ Arbitration Mechanism Selected             │
 │15:14 │ SHN    │ Shutdown Notification                      │
 │      │        │ 00=없음, 01=정상종료, 10=급종료           │
 │19:16 │ IOSQES │ I/O SQ Entry Size: 2^IOSQES               │
 │      │        │ 통상 6 (= 64바이트)                      │
 │23:20 │ IOCQES │ I/O CQ Entry Size: 2^IOCQES               │
 │      │        │ 통상 4 (= 16바이트)                      │
 │31:24 │ Rsvd   │ 예약                                     │
 └──────┴────────┴──────────────────────────────────────────┘
```

### 1.4 CSTS 레지스터 (0x1C) - Controller Status

컨트롤러가 자신의 상태를 보고하는 32비트 레지스터이다.

```
 비트 레이아웃:
 ┌──────┬───────┬──────────────────────────────────────────┐
 │ 비트 │ 필드  │ 설명                                     │
 ├──────┼───────┼──────────────────────────────────────────┤
 │  0   │ RDY   │ Ready: CC.EN=1 후 컨트롤러 준비 완료     │
 │  1   │ CFS   │ Controller Fatal Status: 치명적 오류      │
 │ 3:2  │ SHST  │ Shutdown Status                           │
 │      │       │ 00=정상동작, 01=진행중, 10=완료           │
 │  4   │ NSSRO │ NVM Subsystem Reset Occurred               │
 │  5   │ PP    │ Processing Paused                          │
 │  6   │ ST    │ Shutdown Type                               │
 │31:7  │ Rsvd  │ 예약                                     │
 └──────┴───────┴──────────────────────────────────────────┘
```

### 1.5 AQA, ASQ, ACQ 레지스터 - Admin Queue 설정

Admin Queue는 다른 큐와 달리 Create CQ/SQ 커맨드가 아니라 레지스터에 직접 설정한다. 이유는 Admin Queue가 있어야 다른 큐를 만들 수 있기 때문이다 (닭과 달걀 문제).

```c
/* pci.c: nvme_pci_configure_admin_queue() */
nvmeq = &dev->queues[0];
aqa = nvmeq->q_depth - 1;       /* ASQS: Admin SQ Size (0-based) */
aqa |= aqa << 16;               /* ACQS: Admin CQ Size (상위 16비트) */

writel(aqa, dev->bar + NVME_REG_AQA);                     /* AQA 설정 */
lo_hi_writeq(nvmeq->sq_dma_addr, dev->bar + NVME_REG_ASQ); /* ASQ 베이스 주소 */
lo_hi_writeq(nvmeq->cq_dma_addr, dev->bar + NVME_REG_ACQ); /* ACQ 베이스 주소 */

result = nvme_enable_ctrl(&dev->ctrl);  /* CC.EN = 1 → 컨트롤러 활성화 */
```

AQA 레지스터 형식:
```
 ┌────────────────────┬────────────────────┐
 │ [27:16] ACQS       │ [11:0] ASQS        │
 │ Admin CQ Size      │ Admin SQ Size      │
 │ (0-based)          │ (0-based)          │
 └────────────────────┴────────────────────┘
```

### 1.6 BAR0 매핑 과정

`nvme_remap_bar()`가 Doorbell 영역까지 포함하도록 BAR0을 매핑한다:

```c
/* pci.c: nvme_remap_bar() */
static int nvme_remap_bar(struct nvme_dev *dev, unsigned long size)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);

    if (size <= dev->bar_mapped_size)
        return 0;
    if (size > pci_resource_len(pdev, 0))
        return -ENOMEM;
    if (dev->bar)
        iounmap(dev->bar);
    dev->bar = ioremap(pci_resource_start(pdev, 0), size);
    dev->bar_mapped_size = size;
    dev->dbs = dev->bar + NVME_REG_DBS;  /* Doorbell 시작 = BAR0 + 0x1000 */
    return 0;
}
```

BAR 매핑 크기 계산:

```c
/* Doorbell 영역까지 포함하는 BAR 크기 */
static unsigned long db_bar_size(struct nvme_dev *dev, unsigned nr_io_queues)
{
    return NVME_REG_DBS + ((nr_io_queues + 1) * 8 * dev->db_stride);
}
```

I/O 큐가 N개이면 Admin 큐 포함 (N+1)개의 큐 쌍이 있고, 각 큐 쌍은 SQ Tail DB + CQ Head DB = 2개의 Doorbell을 가지므로 `(N+1) * 2 * (4 << DSTRD)` 바이트가 필요하다. `db_stride = 1 << DSTRD`이므로 `(N+1) * 8 * db_stride`와 같다.

---

## 2. Doorbell 레지스터 상세

### 2.1 Doorbell 주소 계산

NVMe 스펙에 따른 Doorbell 레지스터 위치 공식:

```
SQ y Tail Doorbell = 0x1000 + (2y * (4 << DSTRD))
CQ y Head Doorbell = 0x1000 + ((2y + 1) * (4 << DSTRD))
```

DSTRD=0 (대부분의 디바이스)일 때:

```
SQ 0 Tail DB = 0x1000 + (0 * 4) = 0x1000
CQ 0 Head DB = 0x1000 + (1 * 4) = 0x1004
SQ 1 Tail DB = 0x1000 + (2 * 4) = 0x1008
CQ 1 Head DB = 0x1000 + (3 * 4) = 0x100C
SQ 2 Tail DB = 0x1000 + (4 * 4) = 0x1010
CQ 2 Head DB = 0x1000 + (5 * 4) = 0x1014
...
```

```
 BAR0 Doorbell 영역 (DSTRD=0 기준)
 ┌──────────┬────────────────────────┐
 │ 0x1000   │ SQ 0 Tail Doorbell     │ ← Admin SQ
 │ 0x1004   │ CQ 0 Head Doorbell     │ ← Admin CQ
 │ 0x1008   │ SQ 1 Tail Doorbell     │ ← I/O Queue 1
 │ 0x100C   │ CQ 1 Head Doorbell     │
 │ 0x1010   │ SQ 2 Tail Doorbell     │ ← I/O Queue 2
 │ 0x1014   │ CQ 2 Head Doorbell     │
 │ 0x1018   │ SQ 3 Tail Doorbell     │ ← I/O Queue 3
 │ 0x101C   │ CQ 3 Head Doorbell     │
 │  ...     │  ...                   │
 └──────────┴────────────────────────┘
```

커널 코드에서 Doorbell 인덱스 계산:

```c
/* pci.c: Doorbell 인덱스 계산 헬퍼 */
static inline unsigned int sq_idx(unsigned int qid, u32 stride)
{
    return qid * 2 * stride;
}

static inline unsigned int cq_idx(unsigned int qid, u32 stride)
{
    return (qid * 2 + 1) * stride;
}
```

`dev->dbs`는 `u32 __iomem *` 타입이므로 인덱스 1은 4바이트 이동이다. `db_stride = 1`이면 SQ 0 Tail은 `dbs[0]`, CQ 0 Head는 `dbs[1]`, SQ 1 Tail은 `dbs[2]` 위치에 해당한다.

큐 할당 시 각 큐의 Doorbell 포인터를 설정한다:

```c
/* pci.c: nvme_alloc_queue() */
nvmeq->q_db = &dev->dbs[qid * 2 * dev->db_stride];

/* SQ Tail DB = q_db[0]
 * CQ Head DB = q_db[dev->db_stride]
 *
 * db_stride=1이면:
 *   q_db[0] = SQ Tail Doorbell
 *   q_db[1] = CQ Head Doorbell
 */
```

### 2.2 SQ Tail Doorbell 동작

호스트가 SQ에 새로운 커맨드를 쓴 후, SQ Tail Doorbell에 새 tail 값을 써서 컨트롤러에게 "새 커맨드가 있다"고 알린다.

동작 순서:

```
 호스트 (CPU)                              NVMe 컨트롤러
 ───────────                               ──────────────
 1. SQ[tail]에 64B 커맨드 복사
    (nvme_sq_copy_cmd)

 2. tail++ (wrap-around 처리)

 3. writel(tail, q_db)
    → CPU가 MMIO 쓰기 실행
    → PCIe Memory Write TLP 생성     ─────→ 4. Doorbell 값 수신
    → PCIe 버스를 통해 전송                    (SQ에 새 커맨드 있음 인지)

                                           5. SQ에서 커맨드 Fetch
                                              (PCIe Memory Read TLP
                                               → 호스트 메모리에서 읽기)

                                           6. 커맨드 실행 시작
```

커맨드 복사 코드:

```c
/* pci.c: nvme_sq_copy_cmd() */
static inline void nvme_sq_copy_cmd(struct nvme_queue *nvmeq,
                                    struct nvme_command *cmd)
{
    /* SQ의 현재 tail 위치에 64바이트 커맨드를 복사한다 */
    memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqes),
           absolute_pointer(cmd), sizeof(*cmd));
    /* tail을 전진시킨다 (링 버퍼이므로 wrap-around) */
    if (++nvmeq->sq_tail == nvmeq->q_depth)
        nvmeq->sq_tail = 0;
}
```

SQ Tail Doorbell 쓰기 코드 (핵심 함수):

```c
/* pci.c: nvme_write_sq_db() - I/O 제출의 마지막 단계 */
static inline void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    /* 배치 최적화: write_sq=false이면 Doorbell 쓰기를 지연시킨다 */
    if (!write_sq) {
        u16 next_tail = nvmeq->sq_tail + 1;
        if (next_tail == nvmeq->q_depth)
            next_tail = 0;
        /* 큐가 가득 차는 경우에만 강제로 Doorbell을 쓴다 */
        if (next_tail != nvmeq->last_sq_tail)
            return;
    }

    /* Shadow Doorbell 확인 후 필요하면 MMIO writel() */
    if (nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
            nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
        writel(nvmeq->sq_tail, nvmeq->q_db);   /* ← 실제 MMIO 쓰기 */

    nvmeq->last_sq_tail = nvmeq->sq_tail;
}
```

`write_sq` 파라미터의 의미:
- `true`: 즉시 Doorbell을 쓴다 (배치의 마지막 커맨드이거나, commit_rqs 호출 시)
- `false`: 다음 커맨드가 큐를 한 바퀴 돌아 wrap하는 경우에만 Doorbell을 쓴다

이 배치 최적화는 여러 커맨드를 한 번의 Doorbell로 알려서 MMIO 쓰기 횟수를 줄인다.

### 2.3 CQ Head Doorbell 동작

호스트가 CQ에서 완료 엔트리(CQE)를 확인한 후, CQ Head Doorbell에 새 head 값을 써서 컨트롤러에게 "여기까지 읽었다"고 알린다. 컨트롤러는 이 정보를 통해 CQ 공간이 얼마나 남았는지 알 수 있다.

```
 호스트 (CPU)                              NVMe 컨트롤러
 ───────────                               ──────────────
 1. CQ[head]의 Phase Tag 확인
    (nvme_cqe_pending)

 2. Phase 일치 → 유효한 CQE

 3. CQE 처리 (command_id → request 찾기)
    (nvme_handle_cqe)

 4. head++ (wrap-around 시 phase 토글)
    (nvme_update_cq_head)

 5. 모든 CQE 처리 후:
    writel(head, q_db + db_stride)
    → PCIe Memory Write TLP        ─────→ 6. CQ Head 업데이트 수신
                                              (CQ 슬롯 재사용 가능)
```

CQ Head Doorbell 쓰기 코드:

```c
/* pci.c: nvme_ring_cq_doorbell() */
static inline void nvme_ring_cq_doorbell(struct nvme_queue *nvmeq)
{
    u16 head = nvmeq->cq_head;

    /* Shadow Doorbell 확인 후 필요하면 MMIO writel() */
    if (nvme_dbbuf_update_and_check_event(head, nvmeq->dbbuf_cq_db,
                                          nvmeq->dbbuf_cq_ei))
        writel(head, nvmeq->q_db + nvmeq->dev->db_stride);
        /*                         ^^^^^^^^^^^^^^^^^^^^^^^^^
         * CQ Head DB는 SQ Tail DB 바로 다음에 위치한다.
         * q_db는 SQ Tail DB를 가리키므로, + db_stride로 CQ Head DB에 접근.
         */
}
```

### 2.4 Doorbell Buffer (Shadow Doorbell) 최적화

NVMe 1.3에서 도입된 Shadow Doorbell Buffer는 불필요한 MMIO writel()을 줄여 성능을 향상시키는 최적화 기능이다.

#### 기본 원리

MMIO writel()은 PCIe Memory Write TLP를 발생시키는 비용이 큰 연산이다. Shadow Doorbell은 호스트와 컨트롤러가 공유하는 메모리 영역을 사용하여, 컨트롤러가 필요할 때만 실제 MMIO를 요구하도록 한다.

```
 ┌─────────────────────────────────────────────────────────┐
 │                  Shadow Doorbell 구조                    │
 │                                                         │
 │  호스트 메모리 (DMA coherent)                           │
 │  ┌─────────────────────────────────────┐                │
 │  │ dbbuf_dbs (Shadow Doorbell Buffer)  │                │
 │  │  ┌──────────────────────────┐       │                │
 │  │  │ SQ 0 Tail Shadow DB     │       │  호스트가 쓴다  │
 │  │  │ CQ 0 Head Shadow DB     │       │                │
 │  │  │ SQ 1 Tail Shadow DB     │       │                │
 │  │  │ CQ 1 Head Shadow DB     │       │                │
 │  │  │ ...                     │       │                │
 │  │  └──────────────────────────┘       │                │
 │  └─────────────────────────────────────┘                │
 │                                                         │
 │  ┌─────────────────────────────────────┐                │
 │  │ dbbuf_eis (Event Index Buffer)      │                │
 │  │  ┌──────────────────────────┐       │                │
 │  │  │ SQ 0 Event Index        │       │ 컨트롤러가 쓴다│
 │  │  │ CQ 0 Event Index        │       │                │
 │  │  │ SQ 1 Event Index        │       │                │
 │  │  │ CQ 1 Event Index        │       │                │
 │  │  │ ...                     │       │                │
 │  │  └──────────────────────────┘       │                │
 │  └─────────────────────────────────────┘                │
 └─────────────────────────────────────────────────────────┘
```

#### 초기화 과정

**1단계: DMA 메모리 할당**

```c
/* pci.c: nvme_dbbuf_dma_alloc() */
static void nvme_dbbuf_dma_alloc(struct nvme_dev *dev)
{
    unsigned int mem_size = nvme_dbbuf_size(dev);
    /* mem_size = nr_allocated_queues * 8 * db_stride
     * 각 큐마다 SQ DB(4B) + CQ DB(4B) = 8바이트 필요 */

    /* OACS 비트로 Shadow Doorbell 지원 여부 확인 */
    if (!(dev->ctrl.oacs & NVME_CTRL_OACS_DBBUF_SUPP))
        return;

    /* 이미 할당된 경우 내용만 클리어 (리셋 시) */
    if (dev->dbbuf_dbs) {
        memset(dev->dbbuf_dbs, 0, mem_size);
        memset(dev->dbbuf_eis, 0, mem_size);
        return;
    }

    /* Shadow DB 버퍼: 호스트 → 컨트롤러 방향 */
    dev->dbbuf_dbs = dma_alloc_coherent(dev->dev, mem_size,
                                        &dev->dbbuf_dbs_dma_addr,
                                        GFP_KERNEL);
    /* Event Index 버퍼: 컨트롤러 → 호스트 방향 */
    dev->dbbuf_eis = dma_alloc_coherent(dev->dev, mem_size,
                                        &dev->dbbuf_eis_dma_addr,
                                        GFP_KERNEL);
}
```

`dma_alloc_coherent()`는 CPU와 디바이스가 동시에 접근할 수 있는 DMA coherent 메모리를 할당한다. 이 메모리는 캐시 일관성이 보장되므로 별도의 캐시 flush/invalidate가 필요 없다.

**2단계: 개별 큐에 포인터 설정**

```c
/* pci.c: nvme_dbbuf_init() */
static void nvme_dbbuf_init(struct nvme_dev *dev,
                            struct nvme_queue *nvmeq, int qid)
{
    /* Admin 큐(qid=0)는 Shadow Doorbell을 사용하지 않는다 */
    if (!dev->dbbuf_dbs || !qid)
        return;

    /* 각 큐의 Shadow DB/EI 포인터를 전체 버퍼 내 해당 위치로 설정 */
    nvmeq->dbbuf_sq_db = &dev->dbbuf_dbs[sq_idx(qid, dev->db_stride)];
    nvmeq->dbbuf_cq_db = &dev->dbbuf_dbs[cq_idx(qid, dev->db_stride)];
    nvmeq->dbbuf_sq_ei = &dev->dbbuf_eis[sq_idx(qid, dev->db_stride)];
    nvmeq->dbbuf_cq_ei = &dev->dbbuf_eis[cq_idx(qid, dev->db_stride)];
}
```

**3단계: 컨트롤러에 DMA 주소 전달**

```c
/* pci.c: nvme_dbbuf_set() */
static void nvme_dbbuf_set(struct nvme_dev *dev)
{
    struct nvme_command c = { };

    if (!dev->dbbuf_dbs)
        return;

    /* Doorbell Buffer Config Admin 커맨드 (opcode 0x7C) */
    c.dbbuf.opcode = nvme_admin_dbbuf;
    c.dbbuf.prp1 = cpu_to_le64(dev->dbbuf_dbs_dma_addr);  /* Shadow DB 물리 주소 */
    c.dbbuf.prp2 = cpu_to_le64(dev->dbbuf_eis_dma_addr);  /* Event Index 물리 주소 */

    if (nvme_submit_sync_cmd(dev->ctrl.admin_q, &c, NULL, 0)) {
        dev_warn(dev->ctrl.device, "unable to set dbbuf\n");
        nvme_dbbuf_dma_free(dev);
        /* 실패 시 각 큐의 Shadow DB 포인터도 해제 */
        for (i = 1; i <= dev->online_queues; i++)
            nvme_dbbuf_free(&dev->queues[i]);
    }
}
```

#### Event Index 메커니즘 (라인바이라인 분석)

Event Index는 컨트롤러가 "이 값 이상이 되면 MMIO로 알려줘"라고 호스트에게 설정하는 값이다. virtio의 `vring_need_event()`와 동일한 알고리즘을 사용한다.

```c
/* pci.c: nvme_dbbuf_need_event() */
static inline int nvme_dbbuf_need_event(u16 event_idx, u16 new_idx, u16 old)
{
    /* new_idx가 event_idx를 "넘어섰는지" 판단한다.
     *
     * 수식: (new_idx - event_idx - 1) < (new_idx - old)
     *
     * 의미: old → new_idx 범위 안에 event_idx가 포함되어 있으면 true.
     * u16 연산이므로 wrap-around가 자동 처리된다.
     *
     * 예시 (큐 깊이 8):
     *   old=2, new_idx=5, event_idx=4
     *   (5-4-1) = 0 < (5-2) = 3 → true → MMIO 필요
     *
     *   old=2, new_idx=5, event_idx=6
     *   (5-6-1) = 65534 (u16 overflow) > (5-2) = 3 → false → MMIO 불필요
     */
    return (u16)(new_idx - event_idx - 1) < (u16)(new_idx - old);
}
```

Shadow Doorbell 업데이트 및 MMIO 필요 여부 판단:

```c
/* pci.c: nvme_dbbuf_update_and_check_event() */
static bool nvme_dbbuf_update_and_check_event(u16 value, __le32 *dbbuf_db,
                                              volatile __le32 *dbbuf_ei)
{
    if (dbbuf_db) {
        u16 old_value, event_idx;

        /*
         * [1단계] wmb() - Write Memory Barrier
         * SQ에 커맨드를 쓴 것이 Shadow DB를 업데이트하기 전에
         * 메모리에 확실히 반영되도록 보장한다.
         * 이 배리어가 없으면 컨트롤러가 Shadow DB의 새 값을 보고
         * SQ를 fetch했을 때 아직 커맨드가 쓰이지 않은 상태를 볼 수 있다.
         */
        wmb();

        /* [2단계] 이전 값을 저장하고, 새 값을 Shadow DB에 쓴다 */
        old_value = le32_to_cpu(*dbbuf_db);
        *dbbuf_db = cpu_to_le32(value);
        /* 이것은 일반 메모리 쓰기이므로 MMIO보다 훨씬 빠르다!
         * PCIe TLP가 발생하지 않는다. */

        /*
         * [3단계] mb() - Full Memory Barrier
         * Shadow DB 업데이트가 Event Index 읽기보다 먼저 수행되도록 보장한다.
         * 컨트롤러도 이에 대응하는 순서를 보장해야 한다:
         *   컨트롤러: Event Index 업데이트 → Shadow DB 읽기
         *   호스트:   Shadow DB 업데이트 → Event Index 읽기
         */
        mb();

        /* [4단계] 컨트롤러가 설정한 Event Index를 읽는다 */
        event_idx = le32_to_cpu(*dbbuf_ei);

        /* [5단계] MMIO가 필요한지 판단 */
        if (!nvme_dbbuf_need_event(event_idx, value, old_value))
            return false;  /* MMIO 불필요! Shadow DB 업데이트만으로 충분 */
    }

    return true;  /* MMIO writel() 필요 */
}
```

동작 시나리오 예시:

```
 시나리오: SQ에 커맨드를 4개 연속 제출

 컨트롤러가 event_idx=3으로 설정 (= "tail이 3 이상이 되면 알려줘")

 커맨드 1: tail=1, old=0
   nvme_dbbuf_need_event(3, 1, 0):
   (1-3-1)=65533 > (1-0)=1 → false → MMIO 건너뜀 ✓

 커맨드 2: tail=2, old=1
   nvme_dbbuf_need_event(3, 2, 1):
   (2-3-1)=65534 > (2-1)=1 → false → MMIO 건너뜀 ✓

 커맨드 3: tail=3, old=2
   nvme_dbbuf_need_event(3, 3, 2):
   (3-3-1)=65535 > (3-2)=1 → false → MMIO 건너뜀 ✓

 커맨드 4: tail=4, old=3
   nvme_dbbuf_need_event(3, 4, 3):
   (4-3-1)=0 < (4-3)=1 → true → MMIO 수행! ★

 결과: 4개 커맨드에 MMIO는 1번만 수행 (75% 절감)
```

이 최적화는 컨트롤러가 아직 이전 커맨드를 처리 중이어서 새 커맨드를 즉시 알 필요가 없을 때 특히 효과적이다.

---

## 3. SQ/CQ Ring Buffer 동작 시각화

큐 깊이(q_depth) = 4인 경우를 예로 든다 (실제로는 1024 이상).

### 3.1 초기 상태 (empty)

```
 Submission Queue (SQ)                  Completion Queue (CQ)
 ┌───────┐                             ┌───────┐
 │ [0]   │ ← head = tail = 0           │ [0]   │ ← head = 0
 │ [1]   │    (비어 있음)               │ [1]   │    phase = 1
 │ [2]   │                             │ [2]   │    (모든 CQE의 phase=0
 │ [3]   │                             │ [3]   │     → host phase=1과 불일치
 └───────┘                             └───────┘     → "유효한 CQE 없음")

 SQ Tail Doorbell: 0 (아직 쓰지 않음)
 CQ Head Doorbell: 0 (아직 쓰지 않음)
```

### 3.2 커맨드 3개 제출 후

호스트가 `nvme_queue_rq()`를 3번 호출하여 커맨드 A, B, C를 제출한다.

```
 Submission Queue (SQ)
 ┌───────┐
 │ [0] A │   cmd_id=0, Read LBA 100
 │ [1] B │   cmd_id=1, Write LBA 200
 │ [2] C │   cmd_id=2, Read LBA 300
 │ [3]   │ ← tail = 3
 └───────┘

 1. nvme_sq_copy_cmd()로 SQ[0], SQ[1], SQ[2]에 64B 커맨드 복사
 2. tail이 0→1→2→3으로 전진
 3. nvme_write_sq_db(): writel(3, q_db)
    → PCIe Memory Write TLP로 tail=3을 컨트롤러에 전달
    → 컨트롤러: "SQ[0]~SQ[2]에 새 커맨드 3개 있구나"

 Completion Queue (CQ)
 ┌───────────────────┐
 │ [0] phase=0       │ ← head = 0, host phase = 1
 │ [1] phase=0       │    (phase 불일치 → CQE 없음)
 │ [2] phase=0       │
 │ [3] phase=0       │
 └───────────────────┘
```

### 3.3 컨트롤러가 2개 처리 후

컨트롤러가 커맨드 A, B를 처리하고 CQ에 완료 엔트리를 기록한다.

```
 Submission Queue (SQ)
 ┌───────┐
 │ [0] A │   (컨트롤러가 fetch 완료)
 │ [1] B │   (컨트롤러가 fetch 완료)
 │ [2] C │   (아직 처리 중 또는 대기)
 │ [3]   │ ← tail = 3
 └───────┘
 컨트롤러 SQ head = 2 (A, B를 가져감)

 Completion Queue (CQ)
 ┌──────────────────────────────────────────┐
 │ [0] cmd_id=0, status=OK, phase=1  ★NEW  │ ← head = 0, host phase = 1
 │ [1] cmd_id=1, status=OK, phase=1  ★NEW  │    phase 일치 → 유효!
 │ [2] phase=0                              │    phase 불일치 → 여기까지
 │ [3] phase=0                              │
 └──────────────────────────────────────────┘

 컨트롤러가 CQ에 CQE를 DMA로 기록할 때:
   - CQE.result: 커맨드별 결과
   - CQE.sq_head: 컨트롤러가 처리한 SQ 위치 (=2)
   - CQE.sq_id: 이 CQE에 대응하는 SQ ID
   - CQE.command_id: 원래 커맨드의 ID (blk-mq tag)
   - CQE.status: 완료 상태 + Phase 비트 (LSB=1)

 → 인터럽트 발생 (MSI-X Memory Write TLP)
```

### 3.4 호스트가 CQ 확인 후

호스트가 인터럽트를 받고 `nvme_irq()` → `nvme_poll_cq()`로 CQ를 처리한다.

```
 nvme_poll_cq() 실행:

 반복 1: CQ[0]의 phase=1 == host phase=1 → 유효!
   dma_rmb()  ← DMA 읽기 배리어
   nvme_handle_cqe(nvmeq, iob, 0)
     → cmd_id=0으로 request 찾기
     → 요청 완료 처리
   nvme_update_cq_head() → head: 0→1

 반복 2: CQ[1]의 phase=1 == host phase=1 → 유효!
   dma_rmb()
   nvme_handle_cqe(nvmeq, iob, 1)
     → cmd_id=1로 request 찾기
     → 요청 완료 처리
   nvme_update_cq_head() → head: 1→2

 반복 3: CQ[2]의 phase=0 != host phase=1 → 유효하지 않음!
   → 루프 종료

 nvme_ring_cq_doorbell(): writel(2, q_db + db_stride)
   → PCIe Memory Write TLP로 head=2를 컨트롤러에 전달
   → 컨트롤러: "CQ[0], CQ[1]은 재사용 가능"

 최종 상태:

 Completion Queue (CQ)
 ┌──────────────────────────────────────────┐
 │ [0] (처리됨, 재사용 가능)                │
 │ [1] (처리됨, 재사용 가능)                │
 │ [2] phase=0                    ← head=2 │  host phase = 1
 │ [3] phase=0                              │
 └──────────────────────────────────────────┘
```

### Phase Bit 토글 시나리오

CQ가 한 바퀴 돌면 Phase가 반전된다:

```
 CQ 깊이 = 4인 경우의 Phase 전이

 첫 번째 라운드 (host phase = 1):
   CQ[0].phase=1 ✓  CQ[1].phase=1 ✓  CQ[2].phase=1 ✓  CQ[3].phase=1 ✓
   → head가 q_depth에 도달 → head=0으로 리셋, host phase=0으로 토글

 두 번째 라운드 (host phase = 0):
   CQ[0].phase=0 ✓  CQ[1].phase=0 ✓  CQ[2].phase=0 ✓  CQ[3].phase=0 ✓
   → head가 q_depth에 도달 → head=0으로 리셋, host phase=1로 토글

 코드:
   static inline void nvme_update_cq_head(struct nvme_queue *nvmeq)
   {
       u32 tmp = nvmeq->cq_head + 1;
       if (tmp == nvmeq->q_depth) {
           nvmeq->cq_head = 0;
           nvmeq->cq_phase ^= 1;   /* Phase 토글: 0↔1 */
       } else {
           nvmeq->cq_head = tmp;
       }
   }
```

---

## 4. MSI-X 인터럽트 메커니즘

### 4.1 MSI-X 초기화

NVMe 컨트롤러는 MSI-X 인터럽트를 사용하여 각 I/O 큐에 독립적인 인터럽트 벡터를 할당한다.

#### 인터럽트 벡터 할당

```c
/* pci.c: nvme_setup_irqs() */
static int nvme_setup_irqs(struct nvme_dev *dev, unsigned int nr_io_queues)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);
    struct irq_affinity affd = {
        .pre_vectors  = 1,              /* Admin 큐용 벡터 1개 예약 */
        .calc_sets    = nvme_calc_irq_sets, /* 큐 타입별 벡터 수 계산 */
        .priv         = dev,
    };
    unsigned int irq_queues, poll_queues;
    unsigned int flags = PCI_IRQ_ALL_TYPES | PCI_IRQ_AFFINITY;

    /* 폴링 큐는 인터럽트가 필요 없으므로 제외 */
    poll_queues = min(dev->nr_poll_queues, nr_io_queues - 1);
    dev->io_queues[HCTX_TYPE_POLL] = poll_queues;

    /* 필요한 인터럽트 수: Admin 1개 + 비폴링 I/O 큐 수 */
    irq_queues = 1;
    if (!(dev->ctrl.quirks & NVME_QUIRK_SINGLE_VECTOR))
        irq_queues += (nr_io_queues - poll_queues);

    return pci_alloc_irq_vectors_affinity(pdev, 1, irq_queues, flags, &affd);
}
```

`pci_alloc_irq_vectors_affinity()`는 CPU affinity를 고려하여 MSI-X 벡터를 할당한다. 각 벡터는 특정 CPU에 바인딩되어, 인터럽트 처리가 해당 CPU에서만 수행된다.

#### 읽기/쓰기 큐 벡터 분배

```c
/* pci.c: nvme_calc_irq_sets() */
static void nvme_calc_irq_sets(struct irq_affinity *affd, unsigned int nrirqs)
{
    struct nvme_dev *dev = affd->priv;
    unsigned int nr_read_queues, nr_write_queues = dev->nr_write_queues;

    if (!nrirqs) {
        nrirqs = 1;
        nr_read_queues = 0;
    } else if (nrirqs == 1 || !nr_write_queues) {
        /* 인터럽트 1개이거나 write_queues=0이면 읽기/쓰기 공유 */
        nr_read_queues = 0;
    } else if (nr_write_queues >= nrirqs) {
        /* 쓰기 큐가 너무 많으면 읽기 큐 최소 1개 보장 */
        nr_read_queues = 1;
    } else {
        nr_read_queues = nrirqs - nr_write_queues;
    }

    dev->io_queues[HCTX_TYPE_DEFAULT] = nrirqs - nr_read_queues;
    dev->io_queues[HCTX_TYPE_READ] = nr_read_queues;
}
```

인터럽트 벡터 할당 예시 (CPU 4개, poll_queues=0, write_queues=0):

```
 벡터 0 → Admin Queue (CPU 0)
 벡터 1 → I/O Queue 1 (CPU 0)
 벡터 2 → I/O Queue 2 (CPU 1)
 벡터 3 → I/O Queue 3 (CPU 2)
 벡터 4 → I/O Queue 4 (CPU 3)
```

#### 인터럽트 핸들러 등록

```c
/* pci.c: queue_request_irq() */
static int queue_request_irq(struct nvme_queue *nvmeq)
{
    struct pci_dev *pdev = to_pci_dev(nvmeq->dev->dev);
    int nr = nvmeq->dev->ctrl.instance;

    if (use_threaded_interrupts) {
        /* 스레드 인터럽트: hardirq는 확인만, 실제 처리는 스레드에서 */
        return pci_request_irq(pdev, nvmeq->cq_vector,
                nvme_irq_check,  /* hardirq handler */
                nvme_irq,        /* thread handler */
                nvmeq, "nvme%dq%d", nr, nvmeq->qid);
    } else {
        /* 일반 모드: hardirq에서 모든 처리 수행 (가장 빠름) */
        return pci_request_irq(pdev, nvmeq->cq_vector,
                nvme_irq,        /* hardirq handler */
                NULL,            /* no thread */
                nvmeq, "nvme%dq%d", nr, nvmeq->qid);
    }
}
```

스레드 인터럽트 vs 일반 인터럽트:

```
 일반 모드 (기본):
   HW 인터럽트 → nvme_irq() [hardirq context]
                  └→ nvme_poll_cq()
                  └→ nvme_handle_cqe()
                  └→ nvme_pci_complete_rq()
   장점: 가장 빠른 응답 시간
   단점: hardirq에서 오래 실행되면 다른 인터럽트를 지연시킬 수 있음

 스레드 모드 (use_threaded_interrupts=1):
   HW 인터럽트 → nvme_irq_check() [hardirq context]
                  └→ CQE 있으면 IRQ_WAKE_THREAD 반환
                → nvme_irq() [thread context]
                  └→ nvme_poll_cq()
                  └→ 실제 처리
   장점: hardirq 시간 최소화, preemption 가능
   단점: 컨텍스트 스위칭 오버헤드로 지연 시간 증가
```

### 4.2 인터럽트 처리 경로 (라인바이라인 분석)

#### nvme_irq() - 인터럽트 핸들러

```c
/* pci.c: nvme_irq() */
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    DEFINE_IO_COMP_BATCH(iob);
    /* iob: 배치 완료를 위한 스택 변수.
     * 여러 CQE를 한 번에 모아서 블록 레이어에 알리면
     * lock 획득/해제 횟수를 줄일 수 있다. */

    if (nvme_poll_cq(nvmeq, &iob)) {
        /* CQ에서 처리한 CQE가 하나 이상 있었다 */
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);
            /* 배치로 모은 완료들을 한 번에 블록 레이어에 전달 */
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
    /* 이 큐에 처리할 CQE가 없었다.
     * 인터럽트 공유(legacy INT# 등) 시 다른 핸들러에 양보. */
}
```

#### nvme_cqe_pending() - CQE 유효성 확인

```c
/* pci.c: nvme_cqe_pending() */
static inline bool nvme_cqe_pending(struct nvme_queue *nvmeq)
{
    struct nvme_completion *hcqe = &nvmeq->cqes[nvmeq->cq_head];

    return (le16_to_cpu(READ_ONCE(hcqe->status)) & 1) == nvmeq->cq_phase;
    /*
     * CQE의 status 필드의 최하위 비트(bit 0)가 Phase Tag이다.
     *
     * READ_ONCE(): 컴파일러가 값을 캐시하지 않도록 강제한다.
     *   디바이스가 DMA로 CQE를 쓰므로, 값이 언제든 변할 수 있다.
     *
     * Phase 비교:
     *   CQE.phase == host cq_phase → 새로운 유효한 CQE
     *   CQE.phase != host cq_phase → 이전 라운드의 오래된 CQE (또는 비어있음)
     */
}
```

#### nvme_poll_cq() - CQ 순회 처리 (핵심 루프)

```c
/* pci.c: nvme_poll_cq() */
static inline bool nvme_poll_cq(struct nvme_queue *nvmeq,
                                struct io_comp_batch *iob)
{
    bool found = false;

    while (nvme_cqe_pending(nvmeq)) {
        found = true;

        /*
         * dma_rmb() - DMA Read Memory Barrier
         *
         * Phase Tag 확인과 CQE의 나머지 필드 읽기 사이에 배리어가 필요하다.
         *
         * 문제 시나리오 (배리어 없을 때):
         * 1. CPU가 CQE의 status를 읽어 phase=1 확인 (유효!)
         * 2. CPU가 CQE의 command_id를 읽으려 함
         * 3. 하지만 디바이스가 아직 command_id를 DMA로 쓰는 중...
         * → 불완전한 CQE를 읽을 수 있다!
         *
         * NVMe 스펙은 디바이스가 Phase Tag를 마지막에 쓰도록 요구한다.
         * 따라서 Phase가 유효하면 나머지 필드도 완전히 쓰인 것이다.
         * 다만 CPU의 out-of-order 실행이 이 순서를 깨뜨릴 수 있으므로
         * dma_rmb()로 순서를 강제한다.
         */
        dma_rmb();

        nvme_handle_cqe(nvmeq, iob, nvmeq->cq_head);
        nvme_update_cq_head(nvmeq);
    }

    if (found)
        nvme_ring_cq_doorbell(nvmeq);
        /* 처리한 CQE가 있으면 디바이스에 CQ Head를 알린다.
         * 모든 CQE를 처리한 후 한 번만 Doorbell을 울린다 (배치 최적화). */

    return found;
}
```

#### nvme_handle_cqe() - 개별 CQE 처리

```c
/* pci.c: nvme_handle_cqe() */
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq,
                                   struct io_comp_batch *iob, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];
    __u16 command_id = READ_ONCE(cqe->command_id);
    struct request *req;

    /* AEN(Asynchronous Event Notification) 특별 처리 */
    if (unlikely(nvme_is_aen_req(nvmeq->qid, command_id))) {
        nvme_complete_async_event(&nvmeq->dev->ctrl,
                cqe->status, &cqe->result);
        return;
    }

    /* command_id(= blk-mq tag)로 원래 request를 찾는다 */
    req = nvme_find_rq(nvme_queue_tagset(nvmeq), command_id);
    if (unlikely(!req)) {
        dev_warn(nvmeq->dev->ctrl.device,
            "invalid id %d completed on queue %d\n",
            command_id, le16_to_cpu(cqe->sq_id));
        return;
    }

    trace_nvme_sq(req, cqe->sq_head, nvmeq->sq_tail);

    /* 완료 처리 시도: 먼저 특수 경로(passthrough 등) 확인 */
    if (!nvme_try_complete_req(req, cqe->status, cqe->result) &&
        /* 배치에 추가 시도 */
        !blk_mq_add_to_batch(req, iob,
                 nvme_req(req)->status != NVME_SC_SUCCESS,
                 nvme_pci_complete_batch))
        /* 배치에 추가 실패 시 개별 완료 */
        nvme_pci_complete_rq(req);
}
```

CQE 구조체 (16바이트):

```c
/* include/linux/nvme.h */
struct nvme_completion {
    union nvme_result {
        __le16  u16;
        __le32  u32;
        __le64  u64;
    } result;           /* [7:0]  커맨드별 결과 데이터 */
    __le16  sq_head;    /* [9:8]  SQ Head Pointer */
    __le16  sq_id;      /* [11:10] SQ ID */
    __u16   command_id; /* [13:12] 완료된 커맨드의 ID */
    __le16  status;     /* [15:14] 상태 + Phase Tag */
};
```

```
 CQE 비트 레이아웃 (16바이트 = 128비트)
 ┌────────────────────────────────────────────────────────┐
 │ 바이트  │ 필드         │ 설명                          │
 ├─────────┼──────────────┼───────────────────────────────┤
 │  7:0    │ result       │ 커맨드별 결과 (64비트)        │
 │  9:8    │ sq_head      │ SQ에서 컨트롤러가 처리한 위치 │
 │ 11:10   │ sq_id        │ 대응하는 SQ의 ID              │
 │ 13:12   │ command_id   │ 원래 커맨드의 CID (=blk-mq tag)│
 │ 15:14   │ status       │ bit[0]: Phase Tag             │
 │         │              │ bit[8:1]: Status Code         │
 │         │              │ bit[11:9]: Status Code Type   │
 │         │              │ bit[14]: More                 │
 │         │              │ bit[15]: Do Not Retry          │
 └─────────┴──────────────┴───────────────────────────────┘
```

### 4.3 Completion 이후 흐름

CQE 처리 후의 호출 체인:

```
 nvme_handle_cqe()
   │
   ├→ nvme_pci_complete_rq()         (개별 완료)
   │    ├→ nvme_pci_unmap_rq()       DMA 매핑 해제
   │    │    ├→ nvme_unmap_metadata() 메타데이터 DMA unmap
   │    │    └→ nvme_unmap_data()     데이터 DMA unmap
   │    └→ nvme_complete_rq()        nvme-core에 완료 알림
   │         └→ blk_mq_end_request() blk-mq에 요청 완료 알림
   │              └→ 요청자에게 I/O 완료 통지
   │                  (bio_endio → 파일시스템/VFS → 사용자 공간)
   │
   └→ nvme_pci_complete_batch()      (배치 완료)
        ├→ 각 요청에 대해 nvme_pci_unmap_rq()
        └→ nvme_complete_batch()
             └→ blk_mq_end_request_batch() 일괄 완료
```

---

## 5. Polling 모드

### 5.1 폴링의 개념

폴링 모드에서는 인터럽트를 사용하지 않고 호스트 CPU가 직접 CQ를 반복적으로 확인한다. 인터럽트 지연(일반적으로 수 마이크로초)을 완전히 제거할 수 있어 초저지연 workload에 적합하다.

```
 인터럽트 모드:
   커맨드 제출 → (디바이스 처리) → CQ에 CQE 기록
   → MSI-X 인터럽트 → CPU 인터럽트 핸들러 호출 → nvme_irq()
   │                                              │
   │← 인터럽트 지연 (수 us) →│                    │

 폴링 모드:
   커맨드 제출 → (디바이스 처리) → CQ에 CQE 기록
   → CPU가 즉시 확인 → nvme_poll()
   │                    │
   │← 0 us 지연 →│

 SPDK는 전체 I/O를 폴링으로 처리한다.
 커널은 poll_queues 모듈 파라미터로 일부 큐만 폴링으로 설정할 수 있다.
```

### 5.2 폴링 큐 설정

```c
/* 모듈 파라미터 */
static unsigned int poll_queues;
module_param_cb(poll_queues, &io_queue_count_ops, &poll_queues, 0644);

/* nvme_setup_irqs()에서 폴링 큐는 인터럽트에서 제외 */
poll_queues = min(dev->nr_poll_queues, nr_io_queues - 1);
dev->io_queues[HCTX_TYPE_POLL] = poll_queues;
irq_queues = 1 + (nr_io_queues - poll_queues);  /* 폴링 큐 제외 */
```

폴링 큐는 `NVMEQ_POLLED` 플래그가 설정되며, 인터럽트 핸들러가 등록되지 않는다.

### 5.3 nvme_poll() - 폴링 콜백

```c
/* pci.c: nvme_poll() - blk_mq_ops.poll 콜백 */
static int nvme_poll(struct blk_mq_hw_ctx *hctx, struct io_comp_batch *iob)
{
    struct nvme_queue *nvmeq = hctx->driver_data;
    bool found;

    /* 빠른 확인: CQE가 없으면 즉시 반환 */
    if (!nvme_cqe_pending(nvmeq))
        return 0;

    spin_lock(&nvmeq->cq_poll_lock);
    found = nvme_poll_cq(nvmeq, iob);
    spin_unlock(&nvmeq->cq_poll_lock);

    return found;
}
```

이 함수는 `io_uring`의 `IORING_SETUP_IOPOLL` 모드에서 호출된다:

```
 사용자 공간 (io_uring):
   io_uring_enter(IORING_ENTER_GETEVENTS)
   → io_iopoll_check()
   → io_do_iopoll()
   → blk_mq_poll()
   → nvme_poll()        ← 여기
   → nvme_poll_cq()
   → nvme_handle_cqe()
```

### 5.4 폴링 vs 인터럽트 비교

```
 ┌──────────────┬──────────────────┬──────────────────┐
 │              │ 인터럽트 모드    │ 폴링 모드        │
 ├──────────────┼──────────────────┼──────────────────┤
 │ 지연 시간    │ 보통 (us 수준)   │ 최소 (sub-us)    │
 │ CPU 사용     │ I/O 없을 때 idle │ 항상 busy-wait   │
 │ 처리량       │ 높음             │ 최고             │
 │ 에너지 효율  │ 좋음             │ 나쁨             │
 │ 적합한 상황  │ 범용             │ 초저지연 필수    │
 │ SPDK 유사성  │ 해당 없음        │ SPDK 전체 모델   │
 └──────────────┴──────────────────┴──────────────────┘
```

---

## 6. DMA 주소 매핑

NVMe 컨트롤러가 호스트 메모리에 접근하려면 물리(DMA) 주소가 필요하다. NVMe는 PRP과 SGL 두 가지 방식으로 데이터 전송 주소를 지정한다.

### 6.1 PRP (Physical Region Page) List

PRP은 NVMe 1.0부터 지원하는 기본 DMA 주소 지정 방식이다. 4KB 페이지 정렬 제약이 있지만 구조가 간단하고 빠르다.

#### PRP 기본 구조

NVMe 커맨드의 DPTR(Data Pointer) 필드에 PRP1과 PRP2가 있다:

```
 NVMe 커맨드 (64바이트) DPTR 영역:
 ┌─────────────────────────────────────────┐
 │ 바이트 24-31: PRP1 (첫 번째 데이터 주소)│
 │ 바이트 32-39: PRP2 (두 번째 또는 리스트)│
 └─────────────────────────────────────────┘
```

전송 크기에 따른 PRP 사용:

```
 ┌──────────────────┬──────────────────────────────────────┐
 │ 전송 크기        │ PRP 구조                             │
 ├──────────────────┼──────────────────────────────────────┤
 │ ≤ 1 페이지(4KB)  │ PRP1 → 데이터                       │
 │                  │ PRP2 = 0 (사용 안 함)                │
 ├──────────────────┼──────────────────────────────────────┤
 │ ≤ 2 페이지(8KB)  │ PRP1 → 첫 번째 페이지               │
 │                  │ PRP2 → 두 번째 페이지                │
 ├──────────────────┼──────────────────────────────────────┤
 │ > 2 페이지       │ PRP1 → 첫 번째 페이지               │
 │                  │ PRP2 → PRP 리스트 시작 주소          │
 └──────────────────┴──────────────────────────────────────┘
```

#### PRP List 체인 구조

```
 PRP 리스트 체인 (16KB 읽기 = 4페이지 전송):

 SQE 커맨드:
 ┌──────────────────────────┐
 │ PRP1: 0x1000_0000        │ → 첫 번째 데이터 페이지 (4KB)
 │ PRP2: 0x2000_0000        │ → PRP 리스트 페이지 시작 주소
 └──────────────────────────┘

 PRP 리스트 페이지 (0x2000_0000):
 ┌──────────────────────────┐
 │ [0] 0x1000_1000          │ → 두 번째 데이터 페이지 (4KB)
 │ [1] 0x1000_2000          │ → 세 번째 데이터 페이지 (4KB)
 │ [2] 0x1000_3000          │ → 네 번째 데이터 페이지 (4KB)
 │ [3] 0x0000_0000          │    (사용 안 함)
 │ ...                      │
 │ [511] (다음 PRP 리스트)  │ → 페이지가 꽉 차면 다음 리스트 링크
 └──────────────────────────┘

 대용량 전송 시 PRP 리스트 체인:

 SQE.PRP2 ─→ PRP 리스트 페이지 0 ─→ PRP 리스트 페이지 1 ─→ ...
              │                        │
              │ [0]~[510]: 데이터 주소  │ [0]~[510]: 데이터 주소
              │ [511]: 다음 리스트 주소  │ [511]: 다음 리스트 주소
              │                        │
              ▼                        ▼
          데이터 페이지들            데이터 페이지들

 한 PRP 리스트 페이지(4KB)에 들어가는 PRP 엔트리:
   4096 / 8 = 512개
   마지막 1개는 다음 리스트 링크로 사용 → 실제 데이터 엔트리 511개
   = 511 × 4KB = 2044KB ≈ 2MB per PRP list page
```

커널에서의 정의:

```c
/* pci.c */
#define PRPS_PER_PAGE  ((NVME_CTRL_PAGE_SIZE / sizeof(__le64)) - 1)
/* = (4096 / 8) - 1 = 511 */

#define NVME_MAX_NR_DESCRIPTORS  5  /* 최대 PRP 리스트 체인 수 */
/* 5 × 511 × 4KB + PRP1 = 약 10MB → NVME_MAX_BYTES(8MB)를 커버 */
```

#### nvme_pci_setup_data_prp() 핵심 로직

```c
/* pci.c: nvme_pci_setup_data_prp() - 핵심 PRP 설정 함수 */
static blk_status_t nvme_pci_setup_data_prp(struct request *req,
        struct blk_dma_iter *iter)
{
    struct nvme_iod *iod = blk_mq_rq_to_pdu(req);
    unsigned int length = blk_rq_payload_bytes(req);

    /* PRP1: 항상 첫 번째 DMA 주소 (비정렬 가능) */
    prp1_dma = iter->addr;
    prp_len = min(length, NVME_CTRL_PAGE_SIZE -
                  (iter->addr & (NVME_CTRL_PAGE_SIZE - 1)));
    /*   ^^ 첫 페이지에서 전송할 수 있는 바이트 수
     *   예: addr=0x1000_0800이면 페이지 내 오프셋=0x800
     *       한 페이지에서 0x1000-0x800=0x800=2048바이트만 가능 */

    length -= prp_len;
    if (!length)
        goto done;  /* 1페이지 이내 → PRP1만 사용 */

    /* PRP2: 2페이지 이내면 직접 주소, 초과하면 PRP 리스트 */
    if (length <= NVME_CTRL_PAGE_SIZE) {
        prp2_dma = iter->addr;  /* 두 번째 페이지 직접 지정 */
        goto done;
    }

    /* PRP 리스트 필요: DMA 풀에서 리스트 페이지 할당 */
    prp_list = dma_pool_alloc(nvme_dma_pool(nvmeq, iod), GFP_ATOMIC,
                              &prp2_dma);
    iod->descriptors[iod->nr_descriptors++] = prp_list;

    /* PRP 리스트에 나머지 페이지 주소를 채운다 */
    i = 0;
    for (;;) {
        prp_list[i++] = cpu_to_le64(iter->addr);
        length -= prp_len;
        if (!length)
            break;

        /* 리스트 페이지가 꽉 찼으면 새 리스트 페이지 할당 + 체인 링크 */
        if (i == NVME_CTRL_PAGE_SIZE >> 3) {  /* i == 512 */
            __le64 *old_prp_list = prp_list;
            prp_list = dma_pool_alloc(..., &prp_list_dma);
            iod->descriptors[iod->nr_descriptors++] = prp_list;

            /* 마지막 엔트리를 새 리스트로 이동하고 링크 설정 */
            prp_list[0] = old_prp_list[i - 1];       /* 데이터 주소 이동 */
            old_prp_list[i - 1] = cpu_to_le64(prp_list_dma); /* 링크 설정 */
            i = 1;
        }
    }

done:
    iod->cmd.common.dptr.prp1 = cpu_to_le64(prp1_dma);
    iod->cmd.common.dptr.prp2 = cpu_to_le64(prp2_dma);
}
```

### 6.2 SGL (Scatter Gather List)

SGL은 NVMe 1.1+에서 선택적으로 지원하는 데이터 주소 지정 방식이다. PRP과 달리 임의 주소와 임의 길이를 지원하여 더 유연하다.

#### SGL Descriptor 구조

```c
/* include/linux/nvme.h */
struct nvme_sgl_desc {
    __le64  addr;       /* 데이터 버퍼 시작 주소 */
    __le32  length;     /* 데이터 길이 (바이트) */
    __u8    rsvd[3];
    __u8    type;       /* 상위 4비트: 디스크립터 타입 */
};
/* sizeof(nvme_sgl_desc) = 16바이트 */
```

SGL 디스크립터 타입:

```
 ┌──────────────────────┬───────────────────────────────────┐
 │ 타입                 │ 설명                              │
 ├──────────────────────┼───────────────────────────────────┤
 │ Data Block (0x00)    │ 실제 데이터 버퍼를 가리킴         │
 │ Segment (0x02)       │ SGL 디스크립터 리스트를 가리킴    │
 │ Last Segment (0x03)  │ 마지막 SGL 세그먼트               │
 └──────────────────────┴───────────────────────────────────┘
```

SGL 체인 구조:

```
 SQE 커맨드:
 ┌──────────────────────────────┐
 │ SGL Descriptor:              │
 │   type = Last Segment (0x03) │
 │   addr = SGL 리스트 주소     │
 │   length = N * 16            │
 └──────────────────────────────┘
         │
         ▼
 SGL 리스트 (DMA 풀에서 할당):
 ┌────────────────────────────┐
 │ [0] Data Block             │
 │   addr=0x1000, len=8192   │ → 데이터 버퍼 1 (8KB)
 │ [1] Data Block             │
 │   addr=0x5000, len=4096   │ → 데이터 버퍼 2 (4KB)
 │ [2] Data Block             │
 │   addr=0xA000, len=16384  │ → 데이터 버퍼 3 (16KB)
 └────────────────────────────┘

 PRP과의 차이:
 - PRP: 각 엔트리가 4KB 페이지 정렬 필수, 길이는 암묵적(항상 페이지 크기)
 - SGL: 각 엔트리가 임의 주소+임의 길이, 비정렬 가능
```

#### PRP vs SGL 선택 로직

```c
/* pci.c: nvme_pci_use_sgls() */
static inline enum nvme_use_sgl nvme_pci_use_sgls(struct nvme_dev *dev,
        struct request *req)
{
    struct nvme_queue *nvmeq = req->mq_hctx->driver_data;

    /* Admin 큐(qid=0)에서는 항상 PRP 사용 */
    if (nvmeq->qid && nvme_ctrl_sgl_supported(&dev->ctrl)) {
        /* SGL을 반드시 사용해야 하는 경우: */
        if (req_phys_gap_mask(req) & (NVME_CTRL_PAGE_SIZE - 1) ||
            /* 1. 페이지 갭이 있어 PRP으로 표현 불가 */
            nvme_req(req)->flags & NVME_REQ_USERCMD ||
            /* 2. 사용자 커맨드 (길이 검증 필요) */
            req->nr_integrity_segments > 1)
            /* 3. 다중 무결성 세그먼트 */
            return SGL_FORCED;
        return SGL_SUPPORTED;
    }
    return SGL_UNSUPPORTED;
}
```

SGL 사용 임계값 (sgl_threshold):

```c
/* 모듈 파라미터: 기본 32KB */
static unsigned int sgl_threshold = SZ_32K;

/* 평균 세그먼트 크기가 임계값 이상이면 SGL 대신 PRP 사용
 * (PRP이 더 효율적인 경우가 많으므로) */
```

SPDK와의 비교:
- SPDK는 컨트롤러가 SGL을 지원하면 항상 SGL을 사용한다
- 커널 드라이버는 `sgl_threshold`로 조건부 사용하며, 기본적으로 PRP을 선호한다

### 6.3 nvme_map_data() 흐름 요약

```
 nvme_prep_rq()
   │
   ├→ nvme_setup_cmd()          NVMe 커맨드 구조체 빌드
   │
   └→ nvme_map_data()           DMA 매핑
        │
        ├→ [단일 세그먼트]
        │    dma_map_bvec()     직접 DMA 매핑
        │    PRP1에 주소 설정
        │
        └→ [다중 세그먼트]
             ├→ blk_rq_dma_map()     scatter-gather DMA 매핑
             │
             ├→ [PRP 경로]
             │    nvme_pci_setup_data_prp()
             │    PRP1, PRP2 설정
             │    필요시 PRP 리스트 체인 구성
             │
             └→ [SGL 경로]
                  nvme_pci_setup_data_sgl()
                  SGL 디스크립터 구성
```

---

## 7. PCIe TLP 관점에서의 NVMe I/O

NVMe의 모든 통신은 PCIe TLP(Transaction Layer Packet)으로 수행된다. 하나의 NVMe Read I/O 커맨드가 PCIe 버스에서 어떤 TLP들로 변환되는지 살펴본다.

### 7.1 TLP 타입별 역할

```
 ┌──────────────────────────────┬────────────────────────────────────┐
 │ NVMe 동작                   │ PCIe TLP                           │
 ├──────────────────────────────┼────────────────────────────────────┤
 │ Doorbell 쓰기               │ Memory Write (Host → Device)       │
 │ 레지스터 읽기               │ Memory Read (Host → Device)        │
 │ SQ Entry Fetch              │ Memory Read (Device → Host)        │
 │ 데이터 전송 (Read 응답)     │ Memory Write (Device → Host)       │
 │ 데이터 전송 (Write 명령)    │ Memory Read (Device → Host)        │
 │ CQE 기록                    │ Memory Write (Device → Host CQ)    │
 │ MSI-X 인터럽트              │ Memory Write (Device → Host LAPIC) │
 └──────────────────────────────┴────────────────────────────────────┘
```

### 7.2 NVMe Read 커맨드의 PCIe 타임라인

```
 시간 →
 ─────────────────────────────────────────────────────────────────────────

 호스트 (CPU)                    PCIe 버스                    NVMe 컨트롤러
 ────────────                    ─────────                    ──────────────

 1. SQ[tail]에 64B 커맨드 복사
    (memcpy to DMA coherent mem)
    │
    │  ※ SQ가 호스트 메모리에 있는 경우
    │    (CMB 미사용 시)
    │
 2. writel(tail, Doorbell)
    │
    ├──→ [MWr TLP: 4B]          ──────────────────────→  3. Doorbell 수신
    │    Host → Device                                       "SQ에 새 커맨드!"
    │    Addr: BAR0+0x1008                               │
    │    Data: tail 값                                   │
    │                                                    4. SQ Fetch 시작
    │                            ←─────────────────────  │
    │              [MRd TLP: 64B Request]                ├──→ [MRd TLP]
    │              Device → Host                         │    Device → Host
    │              Addr: SQ DMA 주소                     │    "SQ 커맨드 읽기"
    │                                                    │
    │    호스트 메모리 컨트롤러가                         │
    │    SQ 데이터를 읽어서 응답                          │
    │              [CplD TLP: 64B+헤더]                   │
    │              Host → Device             ──────────→ 5. 커맨드 디코딩
    │                                                    │
    │                                                    6. NAND에서 데이터 읽기
    │                                                    │   (내부 처리)
    │                                                    │
    │  ※ PRP 리스트가 있으면:                            │
    │              [MRd TLP: PRP List Fetch]              │
    │              Device → Host             ←───────── 6a. PRP 리스트도 fetch
    │              [CplD TLP: PRP List Data]              │
    │              Host → Device             ──────────→ │
    │                                                    │
    │                                                    7. 데이터를 호스트 메모리에 전송
    │              [MWr TLP: Data]                        │
    │              Device → Host             ←───────── ├──→ [MWr TLP]
    │              Addr: PRP1/PRP2 주소                   │    4KB DMA 쓰기
    │              Data: 읽은 데이터                      │    (큰 데이터는 여러 TLP)
    │                                                    │
    │                                                    8. CQE를 호스트 CQ에 기록
    │              [MWr TLP: 16B CQE]                     │
    │              Device → Host             ←───────── ├──→ [MWr TLP]
    │              Addr: CQ DMA 주소                      │    16B CQE 쓰기
    │              Data: status, cmd_id 등                │
    │                                                    │
    │                                                    9. MSI-X 인터럽트 발생
    │              [MWr TLP: MSI-X]                       │
    │              Device → Host             ←───────── └──→ [MWr TLP]
    │              Addr: MSI-X Table 주소                      인터럽트!
    │              Data: 메시지 데이터
    │
 10. CPU 인터럽트 수신
    │
 11. nvme_irq() → nvme_poll_cq()
    │  CQ[head] 읽기 (로컬 메모리)
    │  Phase 확인 → 유효!
    │  command_id로 request 찾기
    │
 12. nvme_ring_cq_doorbell()
    │
    ├──→ [MWr TLP: 4B]          ──────────────────────→  13. CQ Head 수신
    │    Host → Device                                       "CQ 슬롯 재사용 가능"
    │    Addr: BAR0+0x100C
    │    Data: new CQ head

 ─────────────────────────────────────────────────────────────────────────
```

### 7.3 TLP 횟수 분석 (4KB Read, PRP 방식)

```
 ┌────────────────────────────────────────┬────────┬──────────┐
 │ 동작                                  │ 방향   │ TLP 수   │
 ├────────────────────────────────────────┼────────┼──────────┤
 │ SQ Tail Doorbell 쓰기                 │ H→D    │ 1 MWr    │
 │ SQ Entry Fetch (64B)                  │ D→H    │ 1 MRd    │
 │ SQ Entry 응답                         │ H→D    │ 1 CplD   │
 │ 데이터 전송 (4KB)                     │ D→H    │ 1 MWr    │
 │ CQE 기록 (16B)                        │ D→H    │ 1 MWr    │
 │ MSI-X 인터럽트                        │ D→H    │ 1 MWr    │
 │ CQ Head Doorbell 쓰기                 │ H→D    │ 1 MWr    │
 ├────────────────────────────────────────┼────────┼──────────┤
 │ 합계                                  │        │ 7 TLP    │
 └────────────────────────────────────────┴────────┴──────────┘

 Shadow Doorbell 사용 시:
 - SQ Tail Doorbell MWr이 건너뛸 수 있음 → 6 TLP
 - CQ Head Doorbell MWr이 건너뛸 수 있음 → 5 TLP

 CMB(Controller Memory Buffer)에 SQ를 배치한 경우:
 - SQ Fetch MRd/CplD가 불필요 → 2 TLP 절감
 - 디바이스가 로컬 메모리에서 SQ를 직접 읽으므로 PCIe 트래픽 감소

 폴링 모드 사용 시:
 - MSI-X 인터럽트 MWr 불필요 → 1 TLP 절감
```

### 7.4 NVMe Write 커맨드의 차이

Read와 Write의 데이터 전송 방향이 다르다:

```
 NVMe Read:
   데이터 전송 = Device가 호스트 메모리에 쓰기 (MWr, Device → Host)
   컨트롤러: NAND 읽기 → DMA로 호스트 메모리에 쓰기

 NVMe Write:
   데이터 전송 = Device가 호스트 메모리에서 읽기 (MRd, Device → Host)
   컨트롤러: DMA로 호스트 메모리에서 데이터 읽기 → NAND에 쓰기

 Write의 TLP 시퀀스:
   Doorbell MWr → SQ Fetch MRd/CplD → Data Fetch MRd/CplD → CQE MWr → MSI-X MWr
                                        ^^^^^^^^^^^^^^^^^
                                        Device가 호스트 메모리를 읽어야 하므로
                                        MRd + CplD 쌍이 필요 (Read보다 TLP 많음)
```

### 7.5 PCIe 대역폭 효율

```
 PCIe Gen3 x4 대역폭: ~3.94 GB/s (양방향 합계 ~7.88 GB/s)
 PCIe Gen4 x4 대역폭: ~7.88 GB/s (양방향 합계 ~15.75 GB/s)

 4KB Random Read IOPS 계산:

 하나의 4KB Read에 필요한 PCIe 트래픽:
   H→D: Doorbell(4B) + SQ CplD(64B+헤더) + CQ Doorbell(4B) ≈ ~128B
   D→H: SQ Fetch(64B+헤더) + Data(4096B+헤더) + CQE(16B) + MSI-X(4B) ≈ ~4.3KB

 PCIe Gen3 x4에서 이론적 최대 IOPS:
   (3.94 GB/s) / (4.3 KB/IO) ≈ ~940K IOPS (D→H 방향 제한)

 실제로는 PCIe TLP 헤더 오버헤드, flow control 등으로 약 800K~1M IOPS 수준.
 이것이 고성능 NVMe SSD가 PCIe Gen3 x4에서 ~1M IOPS에 도달하는 이유이다.
```

---

## 요약: 전체 I/O 경로에서의 Doorbell과 Completion 위치

```
 애플리케이션: write(fd, buf, 4096)
      │
      ▼
 VFS → 파일시스템 → Block Layer (blk-mq)
      │
      ▼
 nvme_queue_rq()
      │
      ├→ nvme_prep_rq()
      │    ├→ nvme_setup_cmd()         NVMe 커맨드 빌드
      │    └→ nvme_map_data()          DMA 매핑 (PRP/SGL)
      │
      ├→ nvme_sq_copy_cmd()            SQ[tail]에 64B 복사
      │
      └→ nvme_write_sq_db()            ★ SQ Tail Doorbell 쓰기
           ├→ nvme_dbbuf_update_...()  Shadow DB 확인
           └→ writel(tail, q_db)       MMIO → PCIe MWr TLP
                                        │
 ═══════════════════════════════════════ │ ══════════════════
                PCIe 버스                │
 ═══════════════════════════════════════ │ ══════════════════
                                        ▼
                              NVMe 컨트롤러
                              ├→ SQ Fetch (MRd TLP)
                              ├→ NAND Read/Write
                              ├→ Data Transfer (MWr/MRd TLP)
                              ├→ CQE 기록 (MWr TLP → Host CQ)
                              └→ MSI-X 인터럽트 (MWr TLP)
                                        │
 ═══════════════════════════════════════ │ ══════════════════
                                        ▼
 CPU 인터럽트 → nvme_irq()
      │
      └→ nvme_poll_cq()
           │
           ├→ nvme_cqe_pending()       Phase Tag 확인
           ├→ dma_rmb()                메모리 배리어
           ├→ nvme_handle_cqe()        CQE 처리 → request 찾기
           ├→ nvme_update_cq_head()    CQ head 전진 + Phase 토글
           │
           └→ nvme_ring_cq_doorbell()  ★ CQ Head Doorbell 쓰기
                ├→ nvme_dbbuf_update_...() Shadow DB 확인
                └→ writel(head, q_db + stride) MMIO → PCIe MWr TLP
      │
      └→ nvme_pci_complete_rq()
           ├→ nvme_pci_unmap_rq()      DMA unmap
           └→ nvme_complete_rq()
                └→ blk_mq_end_request()
                     └→ 요청자에게 I/O 완료 통지
```

---

## 핵심 데이터 구조 요약

```c
/* nvme_dev: PCIe NVMe 디바이스 */
struct nvme_dev {
    struct nvme_queue *queues;       /* 큐 배열: [0]=Admin, [1..N]=I/O */
    u32 __iomem *dbs;               /* Doorbell 시작 (BAR0 + 0x1000) */
    u32 db_stride;                  /* 1 << CAP.DSTRD */
    void __iomem *bar;              /* BAR0 MMIO 시작 */
    __le32 *dbbuf_dbs;              /* Shadow Doorbell Buffer */
    __le32 *dbbuf_eis;              /* Event Index Buffer */
    struct nvme_ctrl ctrl;          /* 공통 컨트롤러 추상화 */
};

/* nvme_queue: NVMe 큐 쌍 (SQ + CQ) */
struct nvme_queue {
    void *sq_cmds;                  /* SQ 커맨드 버퍼 (DMA) */
    struct nvme_completion *cqes;   /* CQ 엔트리 배열 (DMA) */
    u32 __iomem *q_db;              /* SQ Tail Doorbell MMIO 주소 */
    u16 sq_tail;                    /* SQ Tail 포인터 */
    u16 cq_head;                    /* CQ Head 포인터 */
    u8 cq_phase;                    /* CQ Phase 비트 */
    __le32 *dbbuf_sq_db;            /* SQ Shadow Doorbell */
    __le32 *dbbuf_cq_db;            /* CQ Shadow Doorbell */
    __le32 *dbbuf_sq_ei;            /* SQ Event Index */
    __le32 *dbbuf_cq_ei;            /* CQ Event Index */
};

/* nvme_completion: CQ 엔트리 (16바이트) */
struct nvme_completion {
    union nvme_result result;       /* 커맨드별 결과 (8B) */
    __le16 sq_head;                 /* SQ Head Pointer */
    __le16 sq_id;                   /* SQ ID */
    __u16  command_id;              /* 커맨드 ID (= blk-mq tag) */
    __le16 status;                  /* 상태 + Phase(bit 0) */
};
```

---

## 소스 파일 위치

| 파일 | 역할 |
|------|------|
| `drivers/nvme/host/pci.c` | PCIe NVMe 드라이버 (Doorbell, 인터럽트, DMA, probe) |
| `include/linux/nvme.h` | NVMe 스펙 레지스터, 커맨드, CQE 구조체 정의 |
| `drivers/nvme/host/nvme.h` | 드라이버 내부 구조체 (nvme_ctrl, nvme_req 등) |
| `drivers/nvme/host/core.c` | NVMe 공통 코드 (nvme_complete_rq 등) |
