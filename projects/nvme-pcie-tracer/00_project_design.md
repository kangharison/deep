# nvme-pcie-tracer: NVMe PCIe-Level Bidirectional Command Tracer

## 1. 프로젝트 개요

### 프로젝트명
nvme-pcie-tracer

### 목표
NVMe 디바이스에 대한 **양방향 PCIe 레벨 커맨드 트레이싱**을 순수 소프트웨어로 구현한다. 물리 x86_64 머신에서 커널 모듈과 BPF를 활용하여, NVMe 디바이스와 호스트 간의 모든 통신을 캡처하고 디코딩한다.

### 추적 대상

| 방향 | 이벤트 | 설명 |
|------|--------|------|
| Host → Device | BAR0 레지스터 R/W | CAP, VS, CC, CSTS 등 컨트롤러 레지스터 접근 |
| Host → Device | Doorbell R/W | SQ Tail Doorbell 쓰기, CQ Head Doorbell 쓰기 |
| Host → Device | Config Space R/W | PCIe Configuration Space 접근 |
| Device → Host | SQ Fetch (DMA Read) | 디바이스가 SQ에서 커맨드를 가져감 |
| Device → Host | CQ Write (DMA Write) | 디바이스가 CQ에 완료 엔트리 기록 |
| Device → Host | Data DMA | 데이터 읽기/쓰기 DMA 전송 |
| Device → Host | MSI-X 인터럽트 | 완료 알림 인터럽트 |

### 아키텍처 다이어그램

```
+============================================================================+
|                        nvme-pcie-tracer 전체 구조                           |
+============================================================================+

  [ 유저스페이스 ]
  +--------------------------+     +-----------------------+
  | nvme_trace_reader        |     | analyze.py            |
  |  - debugfs/relay 읽기    |     |  - 레이턴시 분석      |
  |  - 실시간 출력           |     |  - 큐별 통계          |
  |  - 파일 저장             |     |  - matplotlib 시각화  |
  +------------|-------------+     +------------|----------+
               |                                |
  =============|================================|=========================
               | debugfs / relay                | JSON/CSV
  =============|================================|=========================
  [ 커널스페이스 ]
               |
  +------------|--------------------------------------------------------------+
  |            v                                                              |
  |  +---------------------+                                                  |
  |  | trace_buffer.c      |  <-- per-CPU 링 버퍼 (lock-free)                |
  |  | (relay subsystem)   |                                                  |
  |  +--+---+---+---+---+--+                                                  |
  |     ^   ^   ^   ^   ^                                                     |
  |     |   |   |   |   |                                                     |
  |  +--+   |   |   |   +------------------------------------------+         |
  |  |      |   |   +----------------------------+                  |         |
  |  |      |   +----------------+               |                  |         |
  |  v      v                    v               v                  v         |
  | +------------+ +------------+ +------------+ +-------------+ +--------+  |
  | |nvme_mmio   | |nvme_sq     | |nvme_cq     | |nvme_irq     | |nvme    |  |
  | |_tracer.c   | |_monitor.c  | |_monitor.c  | |_hooker.c    | |_iommu  |  |
  | |            | |            | |            | |             | |_tracer  |  |
  | |PTE fault / | |kprobe:     | |kprobe:     | |kprobe:      | |.c      |  |
  | |kprobe      | |nvme_sq_    | |nvme_handle | |nvme_irq()   | |        |  |
  | |(mmiotrace) | |copy_cmd()  | |_cqe()      | |             | |kprobe: |  |
  | +-----+------+ +-----+------+ +-----+------+ +------+------+ |dma_map |  |
  |       |              |              |               |         +---+----+  |
  +-------|--------------|--------------|---------------|-------------|--------+
          |              |              |               |             |
  ========|==============|==============|===============|=============|========
          v              v              v               v             v
  +-------+------+ +----+-------+ +----+-------+ +----+--------+ +--+------+
  | BAR0 MMIO    | | SQ Memory  | | CQ Memory  | | MSI-X       | | IOMMU   |
  | Registers    | | (Host RAM) | | (Host RAM) | | Controller  | | DMA Map |
  +-------+------+ +-----+------+ +-----+------+ +------+------+ +----+----+
          |              |              |               |             |
          v              v              v               v             v
  +-----------------------------------------------------------------------+
  |                     PCIe Bus (TLP Transactions)                        |
  +-----------------------------------------------------------------------+
          |              |              |               |
          v              v              v               v
  +-----------------------------------------------------------------------+
  |                       NVMe SSD Controller                              |
  +-----------------------------------------------------------------------+
```

### 추적 흐름 (I/O 한 건의 전체 라이프사이클)

```
시간순서:
  t0  [H→D] SQ에 커맨드 복사     nvme_sq_copy_cmd()      SQE 64B 캡처
  t1  [H→D] SQ Tail Doorbell     nvme_write_sq_db()      qid, new_tail
  t2  [D→H] SQ Fetch             (간접: CQE의 sq_head)   SQ Head 변화
  t3  [D→H] 데이터 DMA           dma_map_sg_attrs()      IOVA, 크기, 방향
  t4  [D→H] CQ에 완료 기록       nvme_handle_cqe()       CQE 16B 캡처
  t5  [D→H] MSI-X 인터럽트       nvme_irq()              vector, cpu
  t6  [H→D] CQ Head Doorbell     nvme_poll_cq() 끝       qid, new_head
```


## 2. 추적 가능한 이벤트 목록

### 2.1 Host → Device (MMIO 방향)

| 이벤트 | 메커니즘 | 추적할 정보 | 오버헤드 |
|--------|----------|-------------|----------|
| BAR0 레지스터 읽기 (CAP, VS, CSTS) | mmiotrace PTE fault / kprobe | 주소, 오프셋, 반환값, 타임스탬프 | 높음(mmiotrace) / 낮음(kprobe) |
| BAR0 레지스터 쓰기 (CC, AQA, ASQ, ACQ) | mmiotrace PTE fault / kprobe | 주소, 오프셋, 쓰기 값 | 높음 / 낮음 |
| SQ Tail Doorbell 쓰기 | kprobe on `nvme_write_sq_db` | qid, new_tail, shadow DB 여부 | 낮음 |
| CQ Head Doorbell 쓰기 | kprobe on `nvme_update_cq_head` | qid, new_head | 낮음 |
| Config Space R/W | kprobe on `pci_read/write_config_*` | BDF, offset, size, value | 매우 낮음 |
| SQ에 커맨드 쓰기 | kprobe on `nvme_sq_copy_cmd` | SQE 64바이트 전체 디코딩 | 낮음 |

**커널 코드 참조 - SQ Tail Doorbell (`pci.c`)**:
```c
static inline void nvme_write_sq_db(struct nvme_queue *nvmeq, bool write_sq)
{
    if (nvme_dbbuf_update_and_check_event(nvmeq->sq_tail,
            nvmeq->dbbuf_sq_db, nvmeq->dbbuf_sq_ei))
        writel(nvmeq->sq_tail, nvmeq->q_db);  // ← BAR0 MMIO Write
    nvmeq->last_sq_tail = nvmeq->sq_tail;
}
```

**커널 코드 참조 - SQ 커맨드 복사 (`pci.c`)**:
```c
static inline void nvme_sq_copy_cmd(struct nvme_queue *nvmeq,
                                    struct nvme_command *cmd)
{
    memcpy(nvmeq->sq_cmds + (nvmeq->sq_tail << nvmeq->sqes),
        absolute_pointer(cmd), sizeof(*cmd));  // 64바이트 복사
    if (++nvmeq->sq_tail == nvmeq->q_depth)
        nvmeq->sq_tail = 0;
}
```

### 2.2 Device → Host (DMA 방향)

| 이벤트 | 메커니즘 | 추적할 정보 | 오버헤드 |
|--------|----------|-------------|----------|
| CQ에 완료 기록 | kprobe on `nvme_handle_cqe` | CQE 16바이트 (status, cid, sq_head, phase) | 낮음 |
| MSI-X 인터럽트 | kprobe on `nvme_irq` | IRQ 번호, CPU, 타임스탬프, CQE 수 | 매우 낮음 |
| SQ Fetch 확인 | CQE의 sq_head 필드 변화 감지 | 간접적 (디바이스가 SQ에서 얼마나 소비했는지) | 없음 |
| 데이터 DMA 매핑 | kprobe on `dma_map_sg_attrs` | IOVA 주소, 물리 주소, 크기, 방향 | 낮음 |

**커널 코드 참조 - CQE 처리 (`pci.c`)**:
```c
static inline void nvme_handle_cqe(struct nvme_queue *nvmeq,
                                   struct io_comp_batch *iob, u16 idx)
{
    struct nvme_completion *cqe = &nvmeq->cqes[idx];
    __u16 command_id = READ_ONCE(cqe->command_id);
    // command_id → request: blk-mq tag 시스템 사용
}
```

**커널 코드 참조 - IRQ 핸들러 (`pci.c`)**:
```c
static irqreturn_t nvme_irq(int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    DEFINE_IO_COMP_BATCH(iob);
    if (nvme_poll_cq(nvmeq, &iob)) {
        if (!rq_list_empty(&iob.req_list))
            nvme_pci_complete_batch(&iob);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}
```

### 2.3 이벤트 분류 체계

```
TRACE_EVENT_TYPE (16비트):

  Host → Device (0x01xx):
    0x0100  MMIO_READ           BAR0 레지스터 읽기
    0x0101  MMIO_WRITE          BAR0 레지스터 쓰기
    0x0102  DOORBELL_SQ_TAIL    SQ Tail Doorbell 쓰기
    0x0103  DOORBELL_CQ_HEAD    CQ Head Doorbell 쓰기
    0x0104  CONFIG_READ         Config Space 읽기
    0x0105  CONFIG_WRITE        Config Space 쓰기
    0x0110  SQ_SUBMIT           SQ에 커맨드 제출

  Device → Host (0x02xx):
    0x0200  CQ_COMPLETE         CQ에 완료 엔트리 도착
    0x0201  IRQ_FIRED           MSI-X 인터럽트 발생
    0x0202  SQ_HEAD_UPDATE      SQ Head 업데이트 (간접 감지)
    0x0210  DMA_MAP             DMA 매핑 생성
    0x0211  DMA_UNMAP           DMA 매핑 해제

  Internal (0x03xx):
    0x0300  QUEUE_CREATE        큐 생성
    0x0301  QUEUE_DELETE        큐 삭제
    0x0302  RESET               컨트롤러 리셋
```


## 3. 아키텍처 설계

### 3.1 모듈 구조

```
nvme-pcie-tracer/
├── 00_project_design.md          # 이 문서
├── kernel/                       # 커널 모듈
│   ├── nvme_mmio_tracer.c        # MMIO 추적 (mmiotrace 확장)
│   ├── nvme_sq_monitor.c         # SQ 커맨드 캡처
│   ├── nvme_cq_monitor.c         # CQ 완료 캡처
│   ├── nvme_irq_hooker.c         # IRQ 후킹
│   ├── nvme_iommu_tracer.c       # IOMMU DMA 추적
│   ├── nvme_pcie_tracer.c        # 통합 모듈 (메인 진입점)
│   ├── nvme_cmd_decode.c         # NVMe 커맨드 디코딩
│   ├── nvme_cmd_decode.h
│   ├── trace_buffer.c            # 공유 링 버퍼 (relay)
│   ├── trace_buffer.h
│   ├── trace_events.h            # 이벤트 타입 정의
│   └── Makefile
├── user/                         # 유저스페이스 도구
│   ├── nvme_trace_reader.c       # 트레이스 읽기/표시
│   ├── nvme_trace_decoder.c      # NVMe 커맨드 디코딩
│   ├── nvme_trace_filter.c       # 필터링
│   └── Makefile
├── scripts/                      # 편의 스크립트
│   ├── setup.sh                  # 환경 설정
│   ├── start_trace.sh            # 트레이싱 시작
│   ├── stop_trace.sh             # 트레이싱 중지
│   └── analyze.py                # 분석 스크립트
├── bpf/                          # BPF 기반 대안
│   ├── nvme_tracer.bpf.c         # BPF 커널 프로그램
│   ├── nvme_tracer.h             # BPF 공유 헤더
│   └── nvme_tracer.py            # BPF 유저스페이스 (bcc)
└── tests/
    ├── test_sq_monitor.sh
    ├── test_cq_monitor.sh
    └── test_decode.c
```

### 3.2 데이터 흐름

```
  kprobe handlers ──┐     mmiotrace hooks ──┐     BPF programs ──┐
  (각 모듈)         │     (PTE fault)       │     (대안 구현)     │
                    v                       v                     v
              trace_record()          trace_record()        bpf_ringbuf
              → 이벤트 구조체          → 이벤트 구조체       _submit()
                    │                       │                     │
                    └───────────┬────────────┘                     │
                                v                                 v
                    relay 서브시스템                      BPF 링 버퍼
                    per-CPU 버퍼 (lock-free)              (perf_event 기반)
                                │                                 │
                                v                                 v
                    debugfs                              유저스페이스
                    /sys/kernel/debug/                    BPF reader
                    nvme-pcie-tracer/
                                │                                 │
                                v                                 v
                    nvme_trace_reader (유저스페이스)      nvme_tracer.py
                                │                                 │
                                └───────────────┬─────────────────┘
                                                v
                                    출력 / 저장 / 분석
```

### 3.3 트레이스 이벤트 구조체

```c
/* trace_events.h */

enum trace_direction {
    DIR_HOST_TO_DEVICE = 0,
    DIR_DEVICE_TO_HOST = 1,
    DIR_INTERNAL       = 2,
};

enum trace_event_type {
    EVT_MMIO_READ = 0x0100, EVT_MMIO_WRITE = 0x0101,
    EVT_DOORBELL_SQ_TAIL = 0x0102, EVT_DOORBELL_CQ_HEAD = 0x0103,
    EVT_CONFIG_READ = 0x0104, EVT_CONFIG_WRITE = 0x0105,
    EVT_SQ_SUBMIT = 0x0110,
    EVT_CQ_COMPLETE = 0x0200, EVT_IRQ_FIRED = 0x0201,
    EVT_SQ_HEAD_UPDATE = 0x0202,
    EVT_DMA_MAP = 0x0210, EVT_DMA_UNMAP = 0x0211,
    EVT_QUEUE_CREATE = 0x0300, EVT_QUEUE_DELETE = 0x0301,
    EVT_RESET = 0x0302,
};

enum nvme_bar0_reg {
    REG_CAP  = 0x00, REG_VS   = 0x08, REG_INTMS = 0x0C, REG_INTMC = 0x10,
    REG_CC   = 0x14, REG_CSTS = 0x1C, REG_NSSR  = 0x20, REG_AQA   = 0x24,
    REG_ASQ  = 0x28, REG_ACQ  = 0x30, REG_DOORBELL = 0x1000,
};

/*
 * 핵심 트레이스 이벤트 구조체
 * 크기: 고정 헤더(40B) + union(최대 64B) = 104바이트 (패딩 포함 128B)
 */
struct nvme_trace_event {
    /* === 공통 헤더 (40바이트) === */
    u64 timestamp_ns;       /* ktime_get_ns() */
    u32 cpu;                /* smp_processor_id() */
    u16 event_type;         /* enum trace_event_type */
    u16 direction;          /* enum trace_direction */
    u32 queue_id;           /* NVMe 큐 번호 (0=Admin, 1~N=I/O) */
    u64 address;            /* MMIO 주소 또는 DMA 주소 */
    u32 size;               /* 접근 크기 (바이트) */
    u32 flags;

    /* === 이벤트별 상세 정보 (최대 64바이트) === */
    union {
        struct nvme_command cmd;              /* SQ: 64B NVMe 커맨드 전체 */

        struct {                              /* CQ: 16B 완료 + 메타 */
            struct nvme_completion cqe;
            u16 sq_head_before;
            u16 sq_head_after;
            u32 batch_count;
        } completion;

        struct {                              /* MMIO: 레지스터 접근 */
            u64 bar_offset;
            u64 value;
            u32 reg_id;                       /* enum nvme_bar0_reg */
            u32 width;                        /* 1, 2, 4, 8 바이트 */
        } mmio;

        struct {                              /* Doorbell */
            u32 new_value;
            u32 old_value;
            u32 shadow_db;
            u32 event_idx;
            u8  mmio_skipped;                 /* Shadow DB로 MMIO 생략 여부 */
        } doorbell;

        struct {                              /* DMA 매핑 */
            u64 iova;
            u64 phys_addr;
            u32 length;
            u32 dma_direction;
            u32 nents;
        } dma;

        struct {                              /* IRQ */
            u32 vector;
            u32 irq_num;
            u32 cqes_processed;
            u8  irq_result;
        } irq;

        struct {                              /* Config Space */
            u16 bus; u8 dev; u8 func;
            u16 offset; u32 value; u8 width;
        } config;

        u8 raw[64];
    };
} __attribute__((packed));
```


## 4. 각 모듈 구현 설계

### 4.1 nvme_mmio_tracer.c (MMIO 추적)

두 가지 방법을 제공한다.

#### 방법 1: mmiotrace 활용 (PTE Fault 기반)

커널의 `arch/x86/mm/kmmio.c` 인프라를 활용한다. PTE의 Present 비트를 제거하여 MMIO 접근 시 Page Fault를 유발하고, fault handler에서 정보를 기록한 후 single-step으로 실제 접근을 수행한다.

```
동작 원리 (kmmio.c 분석):

1. register_kmmio_probe()로 BAR0 영역에 probe 등록
   → BAR0 PTE에서 Present 비트 제거 → TLB flush

2. NVMe 드라이버가 readl()/writel() 수행
   → Page Fault → kmmio_handler()

3. kmmio_handler():
   - per-CPU kmmio_context에 fault 정보 저장
   - pre_handler 콜백 (접근 전 정보 기록)
   - PTE Present 비트 복원 + Single-step 활성화 (EFLAGS.TF=1)

4. 실제 MMIO 접근 수행 (single-step)

5. Debug Trap (#DB) → post_kmmio_handler()
   - post_handler 콜백 (접근 후 값 기록)
   - PTE Present 비트 다시 제거 + Single-step 비활성화
```

```c
/* 골격 코드 */
static struct kmmio_probe mmio_probe;

static void nvme_mmio_pre_handler(struct kmmio_probe *p,
                                  struct pt_regs *regs, unsigned long addr)
{
    struct nvme_trace_event *evt = trace_buffer_reserve(sizeof(*evt));
    if (!evt) return;
    evt->timestamp_ns = ktime_get_ns();
    evt->cpu = smp_processor_id();
    evt->mmio.bar_offset = addr - (unsigned long)nvme_bar_virt;
    evt->mmio.reg_id = bar_offset_to_reg_id(evt->mmio.bar_offset);
    per_cpu(pending_mmio_evt, smp_processor_id()) = evt;
}

static void nvme_mmio_post_handler(struct kmmio_probe *p,
                                   unsigned long condition, struct pt_regs *regs)
{
    struct nvme_trace_event *evt = per_cpu(pending_mmio_evt, smp_processor_id());
    if (!evt) return;
    /* instruction decode로 read/write 판별, 값 기록 */
    evt->event_type = is_write_instruction(regs) ? EVT_MMIO_WRITE : EVT_MMIO_READ;
    evt->mmio.value = is_write_instruction(regs) ? get_write_value(regs) : regs->ax;
    trace_buffer_commit(evt);
}

static int nvme_mmio_tracer_init(struct pci_dev *pdev)
{
    mmio_probe.addr = (unsigned long)pci_iomap(pdev, 0, 0);
    mmio_probe.len = pci_resource_len(pdev, 0);
    mmio_probe.pre_handler = nvme_mmio_pre_handler;
    mmio_probe.post_handler = nvme_mmio_post_handler;
    return register_kmmio_probe(&mmio_probe);
}
```

#### 방법 2: kprobe 기반 (가벼운 대안)

NVMe 드라이버 내부의 레지스터 접근 래퍼에 kprobe를 건다. mmiotrace보다 훨씬 가볍지만 드라이버 코드를 통한 접근만 캡처된다.

```c
static struct kprobe kp_reg_read32 = { .symbol_name = "nvme_pci_reg_read32" };
static struct kprobe kp_reg_write32 = { .symbol_name = "nvme_pci_reg_write32" };
```

**주의**: Shadow Doorbell Buffer 사용 시 실제 MMIO writel()이 생략될 수 있으므로 `nvme_dbbuf_update_and_check_event()`도 함께 추적해야 한다.


### 4.2 nvme_sq_monitor.c (SQ 커맨드 캡처)

#### 후킹 대상 함수 체인

```
nvme_queue_rq()                ← blk-mq 콜백 (I/O 진입점)
  → nvme_prep_rq()             ← 커맨드 빌드 + DMA 매핑
  → nvme_sq_copy_cmd()         ← ★ SQ에 64바이트 복사 (최적 후킹 지점, 인라인)
  → nvme_write_sq_db()         ← Doorbell 쓰기 (인라인)
```

`nvme_sq_copy_cmd()`는 `static inline`이므로 직접 kprobe 불가. 호출처에 kprobe를 건다.

```c
/* 골격 코드 */
static struct kprobe kp_submit = { .symbol_name = "__nvme_submit_cmd" };

/* x86_64: RDI=nvme_queue*, RSI=nvme_command* */
static int sq_submit_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct nvme_queue *nvmeq = (struct nvme_queue *)regs->di;
    struct nvme_command *cmd = (struct nvme_command *)regs->si;
    struct nvme_trace_event evt = {};

    if (!nvmeq || !cmd) return 0;

    evt.timestamp_ns = ktime_get_ns();
    evt.cpu = smp_processor_id();
    evt.event_type = EVT_SQ_SUBMIT;
    evt.direction = DIR_HOST_TO_DEVICE;
    evt.queue_id = nvmeq->qid;
    evt.size = sizeof(struct nvme_command);
    memcpy(&evt.cmd, cmd, sizeof(struct nvme_command));  /* SQE 64B 전체 */
    evt.address = nvmeq->sq_dma_addr + (nvmeq->sq_tail << nvmeq->sqes);

    trace_buffer_write(&evt);
    return 0;
}
```

#### SQE 디코딩 핵심 필드

```
공통:        cmd.common.opcode, cmd.common.command_id, cmd.common.nsid
Read/Write:  cmd.rw.slba, cmd.rw.length, cmd.rw.dptr.prp1, cmd.rw.dptr.prp2
Identify:    cmd.identify.cns, cmd.identify.ctrlid
Create CQ:   cmd.create_cq.cqid, cmd.create_cq.qsize, cmd.create_cq.irq_vector
```

#### Opcode 매핑 테이블

```c
/* I/O opcodes */
{ 0x00, "Flush" }, { 0x01, "Write" }, { 0x02, "Read" },
{ 0x04, "Write Uncorrectable" }, { 0x05, "Compare" },
{ 0x08, "Write Zeroes" }, { 0x09, "Dataset Management" },

/* Admin opcodes */
{ 0x00, "Delete I/O SQ" }, { 0x01, "Create I/O SQ" },
{ 0x02, "Get Log Page" }, { 0x04, "Delete I/O CQ" },
{ 0x05, "Create I/O CQ" }, { 0x06, "Identify" },
{ 0x09, "Set Features" }, { 0x0A, "Get Features" },
```


### 4.3 nvme_cq_monitor.c (CQ 완료 캡처)

#### 방법 1: nvme_irq() 진입 시 CQ 메모리 직접 읽기

`nvme_irq()`에 kprobe를 걸고, 드라이버가 CQE를 처리하기 전에 CQ 메모리를 스캔한다. Phase bit 검사로 유효한 CQE를 식별한다.

```c
/* 골격 코드 */
static struct kprobe kp_irq = { .symbol_name = "nvme_irq" };

/* x86_64: RDI=irq, RSI=data(nvme_queue*) */
static int cq_irq_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct nvme_queue *nvmeq = (struct nvme_queue *)regs->si;
    struct nvme_completion *cqe;
    u16 head = nvmeq->cq_head;
    u8 phase = nvmeq->cq_phase;
    int count = 0;

    while (1) {
        cqe = &nvmeq->cqes[head];
        /* Phase bit 검사: CQE status의 비트0이 예상 phase와 일치해야 유효 */
        if ((le16_to_cpu(READ_ONCE(cqe->status)) & 1) != phase)
            break;

        /* CQE 이벤트 기록 */
        struct nvme_trace_event evt = {};
        evt.timestamp_ns = ktime_get_ns();
        evt.cpu = smp_processor_id();
        evt.event_type = EVT_CQ_COMPLETE;
        evt.direction = DIR_DEVICE_TO_HOST;
        evt.queue_id = nvmeq->qid;
        memcpy(&evt.completion.cqe, cqe, sizeof(*cqe));
        evt.completion.sq_head_after = le16_to_cpu(cqe->sq_head);
        trace_buffer_write(&evt);

        if (++head == nvmeq->q_depth) { head = 0; phase ^= 1; }
        if (++count >= 64) break;  /* 오버헤드 제한 */
    }
    return 0;
}
```

#### 방법 2: CQ DMA 메모리 직접 폴링 (Phase 3)

커널 스레드가 CQ 메모리를 주기적으로 폴링하여, 인터럽트보다 먼저 CQE 도착을 감지한다. Phase bit 변화를 감시한다. CPU 사용률이 증가하는 단점이 있다.


### 4.4 nvme_irq_hooker.c (인터럽트 후킹)

```c
static struct kprobe kp_nvme_irq = { .symbol_name = "nvme_irq" };
static struct kretprobe krp_nvme_irq = {
    .kp.symbol_name = "nvme_irq", .maxactive = NR_CPUS,
};

/* 진입: IRQ 발생 시점 + 벡터/CPU 기록 */
static int irq_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct nvme_queue *nvmeq = (struct nvme_queue *)regs->si;
    struct nvme_trace_event evt = {};
    evt.timestamp_ns = ktime_get_ns();
    evt.cpu = smp_processor_id();
    evt.event_type = EVT_IRQ_FIRED;
    evt.direction = DIR_DEVICE_TO_HOST;
    evt.queue_id = nvmeq->qid;
    evt.irq.irq_num = (u32)regs->di;
    evt.irq.vector = nvmeq->cq_vector;
    trace_buffer_write(&evt);
    return 0;
}

/* 리턴: kretprobe로 IRQ_HANDLED/IRQ_NONE 구분 */
static int irq_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    /* IRQ_NONE이면 spurious interrupt → 통계용 카운터 증가 */
    return 0;
}
```


### 4.5 nvme_iommu_tracer.c (IOMMU DMA 추적)

```c
static struct kprobe kp_dma_map = { .symbol_name = "dma_map_sg_attrs" };
static struct kprobe kp_dma_unmap = { .symbol_name = "dma_unmap_sg_attrs" };

/* NVMe 디바이스만 필터링: dev->driver->name == "nvme" */
static int dma_map_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct device *dev = (struct device *)regs->di;
    if (!dev || !dev->driver || strcmp(dev->driver->name, "nvme") != 0)
        return 0;

    struct nvme_trace_event evt = {};
    evt.timestamp_ns = ktime_get_ns();
    evt.event_type = EVT_DMA_MAP;
    evt.dma.nents = (int)regs->dx;
    evt.dma.dma_direction = (int)regs->cx;
    /* 첫 SG 엔트리의 물리 주소/크기 기록 */
    trace_buffer_write(&evt);
    return 0;
}
```

IOMMU Fault 기반 추적은 `dmar.c`의 `dmar_fault()` 핸들러를 참고하지만, 정상 DMA에서는 fault가 발생하지 않으므로 Phase 3에서 read-only 모니터링으로 제한한다.


### 4.6 trace_buffer.c (공유 링 버퍼)

```c
/* trace_buffer.h */
#define TRACE_BUF_SIZE      (4 * 1024 * 1024)   /* per-CPU 4MB */
#define TRACE_SUBBUF_SIZE   (64 * 1024)
#define TRACE_N_SUBBUFS     (TRACE_BUF_SIZE / TRACE_SUBBUF_SIZE)

int  trace_buffer_init(void);
void trace_buffer_exit(void);
int  trace_buffer_write(struct nvme_trace_event *evt);
struct nvme_trace_event *trace_buffer_reserve(size_t len);
void trace_buffer_commit(struct nvme_trace_event *evt);
```

```c
/* trace_buffer.c - relay 서브시스템 기반 */
static struct rchan *trace_chan;
static struct dentry *trace_dir;
static bool tracing_enabled = false;

static int subbuf_start_handler(struct rchan_buf *buf, void *subbuf,
                                void *prev_subbuf, size_t prev_padding)
{
    if (relay_buf_full(buf)) return 0;  /* 오래된 데이터 덮어쓰기 */
    return 1;
}

int trace_buffer_init(void)
{
    trace_dir = debugfs_create_dir("nvme-pcie-tracer", NULL);
    trace_chan = relay_open("trace", trace_dir,
                           TRACE_SUBBUF_SIZE, TRACE_N_SUBBUFS,
                           &relay_callbacks, NULL);
    debugfs_create_bool("enabled", 0644, trace_dir, &tracing_enabled);
    return trace_chan ? 0 : -ENOMEM;
}

int trace_buffer_write(struct nvme_trace_event *evt)
{
    if (!tracing_enabled) return 0;
    relay_write(trace_chan, evt, sizeof(*evt));  /* per-CPU lock-free */
    return 0;
}
```

#### debugfs 인터페이스

```
/sys/kernel/debug/nvme-pcie-tracer/
├── enabled            # 트레이싱 on/off (echo 1/0)
├── trace0..traceN     # per-CPU 버퍼 (relay)
├── stats              # 통계 (total, dropped)
├── filter_qid         # 큐 ID 필터 (-1 = 모두)
└── filter_events      # 이벤트 타입 비트마스크
```


## 5. 유저스페이스 도구 설계

### 5.1 nvme_trace_reader

relay 채널에서 이벤트를 읽어 텍스트로 출력한다. 각 CPU의 relay 파일을 `poll()`로 감시하고, 이벤트 도착 시 `read()`로 구조체를 읽는다.

### 5.2 nvme_trace_decoder - 출력 형식

```
[타임스탬프]       CPU  방향  큐  이벤트      상세정보
──────────────────────────────────────────────────────────────────────────
[12345.678901234] C03  H→D  SQ[1]  SUBMIT    opcode=READ(0x02) nsid=1 cid=42
                                              slba=0x00001000 nlb=7 (4KB)
                                              prp1=0x00000000FE000000
[12345.678902100] C03  H→D  DB[1]  SQ_TAIL   tail=15 (old=14)
[12345.679100500] C03  D→H  CQ[1]  COMPLETE  cid=42 status=SUCCESS(0x0000)
                                              sqhd=15 sqid=1 phase=1
[12345.679101200] C03  D→H  IRQ    MSI-X     vector=5 irq=87
[12345.679105000] C03  H→D  DB[1]  CQ_HEAD   head=22 (old=21)
──────────────────────────────────────────────────────────────────────────
Latency: SUBMIT→COMPLETE = 198.266 us (cid=42, qid=1)
```

### 5.3 nvme_trace_filter - 필터 옵션

```
--qid <N>              특정 큐 ID만
--direction <H2D|D2H>  방향 필터
--event <type>         이벤트 타입 (예: SQ_SUBMIT,CQ_COMPLETE)
--opcode <op>          NVMe opcode (예: READ,WRITE)
--nsid <N>             네임스페이스 ID
--slba <range>         LBA 범위 (예: 0x1000-0x2000)
--latency-min <us>     최소 레이턴시 이상만 표시
```

### 5.4 analyze.py

```python
"""
기능:
  1. Submit→Complete 레이턴시 분석 (avg, min, max, p50, p99)
  2. 큐별 I/O 통계 (IOPS, 대역폭)
  3. Doorbell 쓰기 빈도/배치 분석
  4. IRQ coalescing 효율 (CQE/interrupt 비율)
  5. 시각화: 레이턴시 히스토그램, 이벤트 타임라인, 큐별 히트맵
"""

class TraceAnalyzer:
    def calculate_latencies(self):
        """cid 기반 Submit→Complete 매칭으로 per-I/O 레이턴시 계산"""
        for evt in self.events:
            if evt['type'] == EVT_SQ_SUBMIT:
                self.submit_map[(evt['qid'], evt['cid'])] = evt
            elif evt['type'] == EVT_CQ_COMPLETE:
                key = (evt['qid'], evt['cid'])
                if key in self.submit_map:
                    latency = evt['timestamp'] - self.submit_map.pop(key)['timestamp']
                    self.latencies[evt['qid']].append(latency)

    def print_latency_stats(self):
        """큐별: count, avg, min, max, p50, p99 (마이크로초)"""

    def plot_latency_histogram(self, output='latency_hist.png'):
        """matplotlib 히스토그램"""

    def plot_timeline(self, output='timeline.png'):
        """이벤트 타임라인: SQ_SUBMIT(파랑), CQ_COMPLETE(녹), IRQ(빨)"""
```


## 6. BPF 기반 대안 구현

### 장단점 비교

| 항목 | 커널 모듈 | BPF |
|------|-----------|-----|
| 커널 빌드 | 필요 (kernel-devel) | 불필요 |
| 안전성 | 버그 시 커널 패닉 가능 | BPF 검증기가 보장 |
| MMIO PTE 추적 | 가능 (mmiotrace) | 불가 |
| 배포 | .ko 빌드 배포 | CO-RE로 커널 버전 무관 |

### BPF로 가능/불가능한 이벤트

```
가능: SQ 캡처, CQ 캡처, Doorbell, IRQ, DMA, Config Space (모두 kprobe)
불가: BAR0 MMIO 직접 감시 (PTE 조작 불가), CQ 메모리 직접 폴링
```

### BPF 구현 골격

```c
/* bpf/nvme_tracer.bpf.c - libbpf CO-RE */
struct { __uint(type, BPF_MAP_TYPE_RINGBUF); __uint(max_entries, 4*1024*1024); } events SEC(".maps");

SEC("kprobe/nvme_irq")
int BPF_KPROBE(trace_nvme_irq, int irq, void *data)
{
    struct nvme_queue *nvmeq = data;
    struct nvme_trace_event *evt = bpf_ringbuf_reserve(&events, sizeof(*evt), 0);
    if (!evt) return 0;
    evt->timestamp_ns = bpf_ktime_get_ns();
    evt->cpu = bpf_get_smp_processor_id();
    evt->event_type = EVT_IRQ_FIRED;
    evt->queue_id = BPF_CORE_READ(nvmeq, qid);
    evt->irq.vector = BPF_CORE_READ(nvmeq, cq_vector);
    bpf_ringbuf_submit(evt, 0);
    return 0;
}

SEC("kprobe/dma_map_sg_attrs")
int BPF_KPROBE(trace_dma_map, struct device *dev, struct scatterlist *sg,
               int nents, enum dma_data_direction dir)
{
    /* NVMe 디바이스만 필터링: dev->driver->name 검사 */
    /* ... */
}
```


## 7. 구현 우선순위 (Phase별)

### Phase 1: 핵심 기능 (MVP) - 예상 2주

| 순서 | 구현 항목 | 파일 |
|------|-----------|------|
| 1-1 | 이벤트 구조체/타입 정의 | trace_events.h |
| 1-2 | 공유 링 버퍼 (relay) | trace_buffer.c/h |
| 1-3 | SQ 커맨드 캡처 | nvme_sq_monitor.c |
| 1-4 | CQ 완료 캡처 | nvme_cq_monitor.c |
| 1-5 | Doorbell 추적 | nvme_sq_monitor.c에 포함 |
| 1-6 | 통합 모듈 (init/exit) | nvme_pcie_tracer.c |
| 1-7 | 유저스페이스 reader | nvme_trace_reader.c |
| 1-8 | 빌드 시스템 | Makefile (kernel + user) |

**Phase 1 완료 시 동작 예시**:
```bash
sudo insmod nvme_pcie_tracer.ko target_pci="0000:03:00.0"
echo 1 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled
sudo ./nvme_trace_reader
# [12345.678] C03 H→D SQ[1] SUBMIT opcode=READ cid=42 slba=0x1000 nlb=7
# [12345.679] C03 D→H CQ[1] COMPLETE cid=42 status=SUCCESS sqhd=15
sudo rmmod nvme_pcie_tracer
```

### Phase 2: 확장 - 예상 3주

| 순서 | 구현 항목 |
|------|-----------|
| 2-1 | MMIO 레지스터 R/W 추적 (kprobe 방식) |
| 2-2 | DMA 매핑 추적 |
| 2-3 | IRQ 상세 후킹 (kretprobe 포함) |
| 2-4 | NVMe 전체 opcode 디코더 |
| 2-5 | 필터링 (qid, opcode, direction) |
| 2-6 | Config Space R/W 추적 |
| 2-7 | Shadow Doorbell 추적 (MMIO skip 감지) |
| 2-8 | 레이턴시 분석 (analyze.py) |

### Phase 3: 고급 - 예상 4주

| 순서 | 구현 항목 |
|------|-----------|
| 3-1 | mmiotrace 기반 MMIO 추적 (PTE fault 방식) |
| 3-2 | CQ 메모리 직접 폴링 (커널 스레드) |
| 3-3 | BPF 기반 트레이서 (libbpf CO-RE) |
| 3-4 | 시각화 (히스토그램, 타임라인, 히트맵) |
| 3-5 | 성능 최적화 + 테스트 |


## 8. 성능 영향 분석

### 8.1 모듈별 예상 오버헤드

| 모듈 | 메커니즘 | per-event | IOPS 감소 | 비고 |
|------|----------|-----------|-----------|------|
| nvme_sq_monitor | kprobe | ~200ns | 1-3% | SQ 제출마다 |
| nvme_cq_monitor | kprobe | ~200ns | 1-3% | IRQ context |
| nvme_irq_hooker | kprobe | ~150ns | <1% | IRQ당 1회 |
| nvme_mmio_tracer (kprobe) | kprobe | ~200ns | <1% | 초기화 시에만 빈번 |
| nvme_mmio_tracer (mmiotrace) | PTE fault+SS | ~5-10us | **50-90%** | 모든 MMIO에 fault |
| nvme_iommu_tracer | kprobe | ~200ns | 1-2% | DMA map/unmap |
| trace_buffer | relay_write | ~50ns | <0.5% | per-CPU lock-free |

**종합**: kprobe 기반만 사용 시 총 3-8% IOPS 감소. mmiotrace는 초기화 디버깅 전용.

### 8.2 오버헤드 최소화 전략

```c
/* static key로 비활성화 시 제로 오버헤드 */
DEFINE_STATIC_KEY_FALSE(nvme_tracing_active);

static int sq_submit_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    if (!static_branch_unlikely(&nvme_tracing_active))
        return 0;  /* 비활성화 시 NOP으로 패치됨 */
    /* 실제 로직 */
}
```

추가 전략: 모듈별 개별 on/off, 큐/opcode 필터링 (커널 내), 샘플링 (N번째 이벤트만 기록).


## 9. 빌드 시스템

### 9.1 CentOS/Rocky Linux 9 의존성 설치

```bash
# 필수
sudo dnf install -y kernel-devel-$(uname -r) gcc make elfutils-libelf-devel

# BPF (선택)
sudo dnf install -y clang llvm libbpf-devel bpftool bcc-devel

# 분석 (선택)
sudo dnf install -y python3-matplotlib python3-numpy
```

### 9.2 커널 모듈 Makefile

```makefile
# kernel/Makefile
obj-m += nvme_pcie_tracer.o
nvme_pcie_tracer-objs := \
    nvme_pcie_tracer_main.o nvme_sq_monitor.o nvme_cq_monitor.o \
    nvme_irq_hooker.o nvme_mmio_tracer.o nvme_iommu_tracer.o \
    nvme_cmd_decode.o trace_buffer.o
ccflags-y := -DDEBUG -g -Wall -Werror
KDIR ?= /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

### 9.3 편의 스크립트

```bash
#!/bin/bash
# scripts/start_trace.sh
PCI_BDF="${1:-auto}"
if [ "$PCI_BDF" = "auto" ]; then
    PCI_BDF="0000:$(lspci -d ::0108 | head -1 | awk '{print $1}')"
fi
sudo insmod kernel/nvme_pcie_tracer.ko target_pci="$PCI_BDF"
echo 1 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled > /dev/null
echo "트레이싱 활성화: $PCI_BDF"
```

```bash
#!/bin/bash
# scripts/stop_trace.sh
echo 0 | sudo tee /sys/kernel/debug/nvme-pcie-tracer/enabled > /dev/null
cat /sys/kernel/debug/nvme-pcie-tracer/stats 2>/dev/null
sudo rmmod nvme_pcie_tracer
echo "완료"
```


## 10. 핵심 커널 구조체 참조

프로젝트에서 kprobe handler가 접근하는 커널 내부 구조체 요약이다.

### nvme_queue (pci.c)

```c
struct nvme_queue {
    struct nvme_dev *dev;
    void *sq_cmds;                  /* SQ 커맨드 버퍼 (64B * q_depth) */
    struct nvme_completion *cqes;   /* CQ 엔트리 배열 (16B * q_depth) */
    dma_addr_t sq_dma_addr;         /* SQ DMA 주소 (디바이스가 fetch하는 주소) */
    dma_addr_t cq_dma_addr;         /* CQ DMA 주소 (디바이스가 write하는 주소) */
    u32 __iomem *q_db;              /* SQ Tail Doorbell; q_db+db_stride=CQ Head DB */
    u32 q_depth;
    u16 cq_vector;                  /* MSI-X 벡터 번호 */
    u16 sq_tail, last_sq_tail, cq_head, qid;
    u8  cq_phase, sqes;
    __le32 *dbbuf_sq_db, *dbbuf_cq_db;   /* Shadow Doorbell */
    __le32 *dbbuf_sq_ei, *dbbuf_cq_ei;   /* Event Index */
};
```

### nvme_dev (pci.c)

```c
struct nvme_dev {
    struct nvme_queue *queues;      /* queues[0]=Admin, queues[1..N]=I/O */
    u32 __iomem *dbs;              /* Doorbell 시작 (BAR0+0x1000) */
    void __iomem *bar;             /* BAR0 MMIO 시작 주소 */
    u32 db_stride;                 /* Doorbell 간격 (4<<dstrd) */
    struct nvme_ctrl ctrl;
    __le32 *dbbuf_dbs, *dbbuf_eis; /* Shadow DB / Event Index 버퍼 */
};
```

### nvme_command / nvme_completion (include/linux/nvme.h)

```c
struct nvme_command {               /* 64 bytes - SQE */
    union {
        struct nvme_common_command common;
        struct nvme_rw_command rw;
        struct nvme_identify identify;
        struct nvme_create_cq create_cq;
        struct nvme_create_sq create_sq;
        /* ... 기타 Admin/Fabrics/IO 커맨드 ... */
    };
};

struct nvme_completion {            /* 16 bytes - CQE */
    union nvme_result result;       /* 커맨드별 결과 (최대 8B) */
    __le16 sq_head;                 /* 컨트롤러가 소비한 SQ 위치 */
    __le16 sq_id;                   /* 출처 SQ ID */
    __u16  command_id;              /* 완료된 커맨드의 CID */
    __le16 status;                  /* 비트0=Phase, 비트1~15=상태 코드 */
};
```

### kmmio_probe (kmmio.c)

```c
struct kmmio_probe {
    unsigned long addr;             /* 모니터링 시작 주소 */
    unsigned long len;              /* 영역 크기 */
    void (*pre_handler)(...);       /* MMIO 접근 전 콜백 */
    void (*post_handler)(...);      /* MMIO 접근 후 콜백 (값 확인) */
};
/* register_kmmio_probe()로 등록 → PTE Present 비트 제거 → page fault 유발 */
```


## 11. 주요 제약사항과 해결 방안

### 인라인 함수 kprobe 제약

`nvme_sq_copy_cmd()`, `nvme_write_sq_db()`, `nvme_handle_cqe()` 등은 `static inline`이므로 독립 심볼이 없어 직접 kprobe 불가하다.

**해결**: (1) 상위 함수(`nvme_queue_rq()`, `nvme_irq()`)에 kprobe, (2) BPF `fentry`/`fexit`로 BTF 기반 인라인 사이트 후킹 (커널 5.5+), (3) `CONFIG_OPTIMIZE_INLINING=n` 빌드 (비권장).

### SQ Fetch 직접 감지 불가

디바이스 내부 동작이므로 소프트웨어로 불가하다. **간접 감지**: CQE의 `sq_head` 필드 변화량으로 batch fetch 패턴 추론.

### CQ DMA Write 타이밍

kprobe는 `nvme_irq()` 진입 시점에서만 동작하므로, 실제 CQ DMA Write 시각은 측정 불가하다. **완화**: Phase 3의 CQ 폴링으로 인터럽트 전 CQE 도착 감지 (폴링 주기 정밀도에 제한).

### 동시성

kprobe handler는 process context(SQ), interrupt context(CQ, IRQ) 등 다양한 컨텍스트에서 실행된다. `relay_write()`는 per-CPU 버퍼를 사용하므로 기본적으로 안전하며, kprobe pre_handler 내에서는 preemption이 비활성화되어 `smp_processor_id()`도 안전하다.


## 12. 핵심 설계 결정 요약

| 결정 사항 | 선택 | 이유 |
|-----------|------|------|
| 메인 구현 | 커널 모듈 (kprobe) | MMIO 추적 가능, 최대 유연성 |
| 대안 구현 | BPF (Phase 3) | 안전성, 배포 편의성 |
| 이벤트 전달 | relay 서브시스템 | per-CPU lock-free, 검증된 인프라 |
| MMIO 추적 | kprobe 우선, mmiotrace 선택 | kprobe가 I/O 경로에서 충분히 가벼움 |
| SQ 캡처 지점 | `nvme_queue_rq()` 또는 상위 | 인라인 제약 회피 |
| CQ 캡처 지점 | `nvme_irq()` 진입 시 CQ 메모리 읽기 | 드라이버 처리 전 원본 캡처 |
| 이벤트 구조체 크기 | 128바이트 고정 | 64B 커맨드 포함, 캐시라인 정렬 |
| 타겟 플랫폼 | x86_64 물리 머신 | mmiotrace가 x86 전용 |
| 최소 커널 버전 | 5.4+ | kprobe, relay, BPF 모두 안정 |
