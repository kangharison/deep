# BaM GPU Thread → NVMe SQ 매핑 관계 분석

**소스코드 위치:** `sources/bam/`
**핵심 파일:**
- `include/ctrl.h:48-245` — Controller 구조체, 큐 예약/생성 루프, queue_counter
- `include/queue.h:36-227` — QueuePair 구조체, SQ/CQ DMA 할당 + 도어벨 GPU 매핑 + lock-free 배열 초기화
- `src/admin.cpp:29-70` — Admin 커맨드 빌더 (Create CQ/SQ의 NVMe dword 구성)
- `src/admin.cpp:327-549` — `nvm_admin_cq_create()` / `nvm_admin_sq_create()` 실행
- `src/queue.cpp:22-54` — `nvm_queue_clear()` 큐 디스크립터 초기화 + 도어벨 주소 계산
- `benchmarks/iodepth-block/main.cu:173-220` — Warp→Queue 선택 로직 (런타임)
- `include/nvm_parallel_queue.h:231-342` — SQ slot 할당, 도어벨 배칭
- `include/page_cache.h:2159-2190` — access_data_async / poll_async

## 1. 문제: 스레드 수 >> 큐 수

GPU는 수만~수십만 스레드를 동시에 실행하지만 NVMe SQ는 수십~수백 개가 한계다.

```
GPU 스레드: ~수만~수십만 (예: 1024 블록 × 64 스레드 = 65,536)
NVMe SQ:   ~수십 (예: n_qps = 128)
비율:       ~500:1
```

BaM은 이 N:1 매핑을 **3단계 계층**으로 해결한다.

## 2. 3단계 매핑 계층

```
                        GPU
┌──────────────────────────────────────────────┐
│  SM 0          SM 1          SM 2     ...    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐     │
│  │Warp0(32t)│ │Warp0(32t)│ │Warp0(32t)│     │
│  │Warp1(32t)│ │Warp1(32t)│ │Warp1(32t)│     │
│  │Warp2(32t)│ │Warp2(32t)│ │Warp2(32t)│     │
│  │...       │ │...       │ │...       │     │
│  └─────┬────┘ └─────┬────┘ └─────┬────┘     │
│        │            │            │           │
│  ──────┼────────────┼────────────┼────────── │
│  단계①│ smid%n_ctrls│            │           │
│        ▼            ▼            ▼           │
│   Controller 0  Controller 1  Controller 0   │
│   (SSD 0)       (SSD 1)       (SSD 0)        │
│        │            │            │           │
│  ──────┼────────────┼────────────┼────────── │
│  단계②│ fetch_add(1)│% n_qps     │           │
│        ▼            ▼            ▼           │
│    QP[3]         QP[7]        QP[1]          │
│   (SQ+CQ)      (SQ+CQ)     (SQ+CQ)         │
│        │            │            │           │
│  ──────┼────────────┼────────────┼────────── │
│  단계③│ __shfl_sync │            │           │
│        ▼            ▼            ▼           │
│   32 threads    32 threads   32 threads      │
│   全員同じQP    全員同じQP   全員同じQP      │
└──────────────────────────────────────────────┘
```

| 단계 | 매핑 | コード | 粒度 | 目的 |
|:----:|------|--------|:----:|------|
| ① | SM → Controller(SSD) | `smid % n_ctrls` | SM 단위 | PCIe locality — 같은 SM의 warp들은 같은 SSD로 |
| ② | Warp → QueuePair | `queue_counter.fetch_add(1) % n_qps` | Warp 단위 | 큐 간 부하 분산 — warp마다 다른 QP로 |
| ③ | Thread → Warp | `__shfl_sync(0xFFFFFFFF, *, 0)` | Thread(32) | 동일 warp 내 divergence 제거 |

## 3. 단계별 코드 상세

### 단계①: SM → Controller 선택

**파일:** `benchmarks/iodepth-block/main.cu:180-187`

```cuda
uint32_t smid = get_smid();          // PTX: %smid 레지스터 읽기
// ...
ctrl = smid % (pc->n_ctrls);        // SM ID를 컨트롤러 수로 나눈 나머지
```

- `get_smid()`는 PTX 인라인 어셈블리로 현재 warp가 실행 중인 SM 번호를 반환
- 같은 SM에서 실행되는 모든 warp는 같은 컨트롤러(= 같은 NVMe SSD)를 선택
- **목적:** PCIe 토폴로지 지역성 확보 — 같은 SM의 I/O가 같은 SSD에 집중되어 PCIe 스위칭 최소화

**다중 SSD 예시 (n_ctrls=2):**
```
SM 0 → SSD 0    SM 1 → SSD 1    SM 2 → SSD 0    SM 3 → SSD 1 ...
```

### 단계②: Warp → QueuePair 선택

**파일:** `benchmarks/iodepth-block/main.cu:189-190`, `include/ctrl.h:63`

```cuda
queue = ctrls[ctrl]->queue_counter.fetch_add(1, simt::memory_order_relaxed)
        % (ctrls[ctrl]->n_qps);
```

```cpp
// ctrl.h:63
simt::atomic<uint64_t, simt::thread_scope_device> queue_counter;
```

- `queue_counter`는 Controller 구조체의 **device-scope atomic 카운터**
- 각 warp가 호출할 때마다 1 증가 → 나머지 연산으로 큐 인덱스 결정
- **라운드로빈**: warp 0→QP[0], warp 1→QP[1], ..., warp N→QP[N%n_qps]
- **목적:** 여러 warp를 여러 QP에 분산하여 SQ slot 경합 최소화

**정적 매핑이 아닌 동적 매핑:**
- 주석 처리된 `smid % n_qps`는 정적 SM 기반 매핑
- 실제 사용하는 `fetch_add` 방식은 warp 스케줄링 순서에 따라 동적으로 분산
- warp가 커널 실행 시마다 다른 큐를 받을 수 있음 → 균등 분배

### 단계③: Warp 내 브로드캐스트

**파일:** `benchmarks/iodepth-block/main.cu:185-194`

```cuda
uint32_t laneid = lane_id();         // warp 내 0~31

uint32_t ctrl, queue;
if (laneid == 0) {                   // lane 0만 연산 수행
    ctrl = smid % (pc->n_ctrls);
    queue = ctrls[ctrl]->queue_counter.fetch_add(1, ...) % (ctrls[ctrl]->n_qps);
}
// lane 0의 결과를 warp 전체(32스레드)에 전파
ctrl  = __shfl_sync(0xFFFFFFFF, ctrl, 0);
queue = __shfl_sync(0xFFFFFFFF, queue, 0);
```

- `__shfl_sync(mask, var, srcLane)`: srcLane(=0)의 `var` 값을 mask에 해당하는 모든 lane에 복사
- `0xFFFFFFFF` = 32비트 마스크 = warp 전체 32 스레드
- **결과:** 하나의 warp 내 32개 스레드가 완전히 동일한 (ctrl, queue) 쌍을 가짐
- **이유:** SIMT 실행 모델에서 warp 내 divergence를 제거 — 32개 스레드가 같은 SQ에 접근해야 같은 코드 경로를 실행

## 4. SQ/CQ 생성 과정 상세

GPU 커널 실행 전, 호스트 측에서 NVMe Admin 커맨드를 통해 SQ/CQ를 생성한다. 이 과정은 **Controller 생성자 → QueuePair 생성자 → Admin 커맨드** 3단계로 진행된다.

### 4.1 전체 생성 흐름

```
호스트 프로세스 (CPU)
  │
  ├── ① Controller 생성자 (ctrl.h:176-245)
  │     │
  │     ├── open("/dev/libnvm0") → nvm_ctrl_init()
  │     │   → NVMe BAR0 MMIO 매핑 (도어벨 레지스터 포함)
  │     │
  │     ├── Admin Queue 생성 → nvm_aq_create()
  │     │   → 컨트롤러 리셋 + Identify Controller/Namespace
  │     │
  │     ├── cudaHostRegister(ctrl->mm_ptr, cudaHostRegisterIoMemory)
  │     │   → NVMe MMIO 레지스터를 GPU가 직접 접근 가능하도록 등록 ★
  │     │
  │     ├── reserveQueues(MAX_QUEUES) → Set/Get Number of Queues
  │     │   → n_sqs, n_cqs 확보 (HW가 지원하는 최대 큐 수)
  │     │
  │     ├── n_qps = min(n_sqs, n_cqs, numQueues) → 실제 사용할 QP 수 결정
  │     │
  │     ├── ② for i in 0..n_qps:  (각 QueuePair 개별 생성)
  │     │     │
  │     │     └── new QueuePair(ctrl, cudaDevice, ..., i+1, queueDepth)
  │     │           │
  │     │           ├── ③ SQ/CQ DMA 메모리를 GPU VRAM에 할당
  │     │           ├── ④ Admin Create CQ 커맨드 → NVMe 컨트롤러에 CQ 등록
  │     │           ├── ⑤ CQ 도어벨 → cudaHostGetDevicePointer로 GPU 포인터 획득
  │     │           ├── ⑥ Admin Create SQ 커맨드 → NVMe 컨트롤러에 SQ 등록 (CQ 연결)
  │     │           ├── ⑦ SQ 도어벨 → cudaHostGetDevicePointer로 GPU 포인터 획득
  │     │           └── ⑧ lock-free 병렬 배열(tickets, marks, cid) GPU 할당
  │     │
  │     ├── cudaMemcpy(d_qps+i, h_qps[i]) → 각 QP를 GPU 메모리에 복사
  │     └── cudaMemcpy(d_ctrl_ptr, this)   → Controller 자체도 GPU에 복사
  │
  └── GPU 커널 실행 시 d_qps[queue]로 접근 가능
```

### 4.2 Controller 생성자: 큐 수 결정 및 루프

**파일:** `include/ctrl.h:176-245`

```cpp
Controller::Controller(const char* path, uint32_t ns_id,
                       uint32_t cudaDevice, uint64_t queueDepth,
                       uint64_t numQueues)
{
    // ① NVMe 디바이스 열기 + BAR0 매핑
    int fd = open(path, O_RDWR);
    nvm_ctrl_init(&ctrl, fd);

    // ② Admin Queue 생성 + 컨트롤러 초기화 (리셋/Identify)
    aq_mem = createDma(ctrl, ctrl->page_size * 3);  // ASQ + ACQ + Identify 버퍼
    initializeController(*this, ns_id);
    // → nvm_admin_ctrl_info()  : Identify Controller
    // → nvm_admin_ns_info()    : Identify Namespace (LBA 크기 등)
    // → nvm_admin_get_num_queues() : 지원되는 SQ/CQ 수 조회

    // ③ NVMe MMIO(도어벨 포함)를 GPU에서 직접 접근 가능하도록 등록 ★핵심★
    cudaHostRegister((void*)ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE,
                     cudaHostRegisterIoMemory);
    // → 이 호출 이후 GPU 커널이 도어벨 MMIO write 가능

    // ④ 컨트롤러에 최대 큐 수 요청 (Set Features: Number of Queues)
    reserveQueues(MAX_QUEUES, MAX_QUEUES);

    // ⑤ 실제 사용할 QP 수 결정
    n_qps = std::min(n_sqs, n_cqs);
    n_qps = std::min(n_qps, (uint16_t)numQueues);
    // 출력 예: "SQs: 128  CQs: 128  n_qps: 128"

    // ⑥ QueuePair 배열 할당 (호스트+GPU 양쪽)
    h_qps = (QueuePair**)malloc(sizeof(QueuePair*) * n_qps);
    cudaMalloc((void**)&d_qps, sizeof(QueuePair) * n_qps);

    // ⑦ 각 QueuePair 생성 + GPU에 복사
    for (size_t i = 0; i < n_qps; i++) {
        h_qps[i] = new QueuePair(ctrl, cudaDevice, ns, info, aq_ref,
                                  i + 1,         // QP ID (0은 Admin 전용)
                                  queueDepth);   // SQ 엔트리 수
        cudaMemcpy(d_qps + i, h_qps[i], sizeof(QueuePair),
                   cudaMemcpyHostToDevice);
    }

    // ⑧ Controller 구조체 자체를 GPU 메모리에 복사
    d_ctrl_buff = createBuffer(sizeof(Controller), cudaDevice);
    d_ctrl_ptr = d_ctrl_buff.get();
    cudaMemcpy(d_ctrl_ptr, this, sizeof(Controller), cudaMemcpyHostToDevice);
}
```

### 4.3 QueuePair 생성자: SQ/CQ 개별 생성 과정

**파일:** `include/queue.h:106-225`

```cpp
QueuePair::QueuePair(const nvm_ctrl_t* ctrl, uint32_t cudaDevice,
                     nvm_ns_info ns, nvm_ctrl_info info,
                     nvm_aq_ref& aq_ref, uint16_t qp_id, uint64_t queueDepth)
{
    // ──────── 큐 크기 결정 ────────
    // NVMe CAP 레지스터에서 CQR(Contiguous Queues Required) 비트 확인
    uint64_t cap = ((volatile uint64_t*)ctrl->mm_ptr)[0];
    bool cqr = (cap & 0x0000000000010000) != 0;

    // MQES (Maximum Queue Entries Supported) = CAP[15:0] + 1
    uint16_t mqes = ((volatile uint16_t*)ctrl->mm_ptr)[0] + 1;

    // CQR이면 64KB 제한 적용, 아니면 MQES까지 허용
    uint64_t sq_size = cqr ? std::min((uint64_t)MAX_SQ_ENTRIES_64K, (uint64_t)mqes)
                           : mqes;
    uint64_t cq_size = cqr ? std::min((uint64_t)MAX_CQ_ENTRIES_64K, (uint64_t)mqes)
                           : mqes;
    // 사용자 지정 queueDepth와 비교하여 최소값
    sq_size = std::min(queueDepth, sq_size);
    cq_size = std::min(queueDepth, cq_size);
```

**큐 크기 제한 정리:**

| 제약 조건 | SQ 최대 | CQ 최대 | 비고 |
|----------|:-------:|:-------:|------|
| NVMe MQES | HW 의존 | HW 의존 | CAP[15:0]+1 |
| CQR=1 (64KB 제한) | 1024 (64KB/64B) | 4096 (64KB/16B) | 연속 메모리 강제 |
| 사용자 queueDepth | queueDepth | queueDepth | 파라미터 |
| **실제 사용** | **min(위 3개)** | **min(위 3개)** | |

```cpp
    // ──────── DMA 메모리 할당 (GPU VRAM) ────────
    // SQ 메모리: NVMe 컨트롤러가 PCIe P2P로 직접 접근할 GPU 메모리
    size_t sq_mem_size = sq_size * sizeof(nvm_cmd_t);  // sq_size × 64B
    size_t cq_mem_size = cq_size * sizeof(nvm_cpl_t);  // cq_size × 16B

    this->sq_mem = createDma(ctrl,
        NVM_PAGE_ALIGN(sq_mem_size, 1UL << 16),  // 64KB 정렬
        cudaDevice);                               // GPU 디바이스에 할당
    this->cq_mem = createDma(ctrl,
        NVM_PAGE_ALIGN(cq_mem_size, 1UL << 16),
        cudaDevice);
```

**SQ/CQ 메모리가 GPU VRAM에 있는 이유:**
- NVMe 컨트롤러가 SQ에서 커맨드를 fetch할 때 → PCIe P2P로 GPU VRAM 직접 읽기
- NVMe 컨트롤러가 CQ에 완료를 쓸 때 → PCIe P2P로 GPU VRAM에 직접 쓰기
- GPU 스레드가 SQ/CQ에 접근할 때 → 로컬 GPU 메모리 접근 (PCIe 불필요)

```cpp
    // ──────── CQ 생성 (Admin 커맨드) ────────
    int status = nvm_admin_cq_create(aq_ref,
        &this->cq,          // 결과 CQ 디스크립터
        qp_id,              // CQ ID (=QP ID, 1부터)
        this->cq_mem.get(), // DMA 핸들 (GPU VRAM 주소 포함)
        0,                  // 오프셋
        cq_size,            // CQ 엔트리 수
        false);             // 연속 메모리 (PRP 불필요)

    // CQ 도어벨을 GPU 디바이스 포인터로 변환 ★
    void* devicePtr = nullptr;
    cudaHostGetDevicePointer(&devicePtr, (void*)this->cq.db, 0);
    this->cq.db = (volatile uint32_t*)devicePtr;
    // → 이제 GPU 커널이 cq.db에 직접 MMIO write 가능

    // ──────── SQ 생성 (Admin 커맨드, CQ와 연결) ────────
    status = nvm_admin_sq_create(aq_ref,
        &this->sq,          // 결과 SQ 디스크립터
        &this->cq,          // 연결할 CQ ★ (SQ→CQ 쌍 형성)
        qp_id,              // SQ ID (=QP ID, CQ와 동일)
        this->sq_mem.get(), // DMA 핸들 (GPU VRAM)
        0,                  // 오프셋
        sq_size,            // SQ 엔트리 수
        false);             // 연속 메모리

    // SQ 도어벨을 GPU 디바이스 포인터로 변환 ★
    cudaHostGetDevicePointer(&devicePtr, (void*)this->sq.db, 0);
    this->sq.db = (volatile uint32_t*)devicePtr;

    // ──────── GPU 병렬 접근용 구조체 초기화 ────────
    init_gpu_specific_struct(cudaDevice);
}
```

### 4.4 nvm_queue_clear: 큐 디스크립터 초기화 + 도어벨 주소 계산

`nvm_admin_cq_create()` / `nvm_admin_sq_create()` 내부에서 Admin 커맨드 발행 전에 호출된다.

**파일:** `src/queue.cpp:22-54`

```cpp
int nvm_queue_clear(nvm_queue_t* queue, const nvm_ctrl_t* ctrl,
                    bool cq, uint16_t no, uint32_t qs,
                    bool local, volatile void* vaddr, uint64_t ioaddr)
{
    queue->no = no;           // 큐 ID
    queue->qs = qs;           // 큐 크기 (엔트리 수)
    queue->es = cq ? sizeof(nvm_cpl_t)    // CQ: 16B/엔트리
                   : sizeof(nvm_cmd_t);   // SQ: 64B/엔트리
    queue->head = 0;
    queue->tail = 0;
    queue->phase = 1;         // CQ phase bit 초기값
    queue->local = !!local;
    queue->head_lock = 0;     // GPU 병렬 접근용 atomic lock
    queue->tail_lock = 0;
    queue->in_ticket = 0;     // Lamport ticket 카운터
    queue->cid_ticket = 0;    // Command ID 할당 카운터

    // ★ 도어벨 레지스터 주소 계산 ★
    // NVMe 스펙: BAR0 + 0x1000 + (2*QID + cq?1:0) * (4 << dstrd)
    queue->db = cq ? CQ_DBL(ctrl->mm_ptr, no, ctrl->dstrd)
                   : SQ_DBL(ctrl->mm_ptr, no, ctrl->dstrd);
    //  SQ doorbell = BAR0 + 0x1000 + (2*no)     * (4 << dstrd)
    //  CQ doorbell = BAR0 + 0x1000 + (2*no + 1) * (4 << dstrd)

    queue->vaddr  = vaddr;    // 큐 메모리 가상 주소 (GPU VRAM)
    queue->ioaddr = ioaddr;   // 큐 메모리 버스 주소 (NVMe 컨트롤러가 DMA로 접근)
    return 0;
}
```

### 4.5 Admin 커맨드 빌더: NVMe Create CQ/SQ 명령 구성

**파일:** `src/admin.cpp:29-70`

#### Create I/O CQ 커맨드 (NVMe 스펙 5.4)

```cpp
static void admin_cq_create(nvm_cmd_t* cmd, const nvm_queue_t* cq,
                            uint64_t ioaddr, bool need_prp)
{
    nvm_cmd_header(cmd, 0, NVM_ADMIN_CREATE_CQ, 0);  // opcode=0x05
    nvm_cmd_data_ptr(cmd, ioaddr, 0);                 // PRP1 = CQ 메모리 물리주소

    // dword10: [31:16]=큐 크기(0-based), [15:0]=큐 식별자
    cmd->dword[10] = ((uint32_t)(cq->qs - 1) << 16) | cq->no;

    // dword11: [31:16]=인터럽트 벡터, [1]=IEN, [0]=PC(물리적 연속)
    cmd->dword[11] = (0x0000 << 16) | (0x00 << 1) | (!need_prp);
    // IEN=0: 인터럽트 비활성화 (GPU가 CQ를 polling하므로 불필요)
    // PC=1:  물리적 연속 메모리 (GPU VRAM은 연속 할당)
}
```

#### Create I/O SQ 커맨드 (NVMe 스펙 5.3)

```cpp
static void admin_sq_create(nvm_cmd_t* cmd, const nvm_queue_t* sq,
                            const nvm_queue_t* cq, uint64_t ioaddr,
                            bool need_prp)
{
    nvm_cmd_header(cmd, 0, NVM_ADMIN_CREATE_SQ, 0);  // opcode=0x01
    nvm_cmd_data_ptr(cmd, ioaddr, 0);                 // PRP1 = SQ 메모리 물리주소

    // dword10: [31:16]=큐 크기(0-based), [15:0]=큐 식별자
    cmd->dword[10] = ((uint32_t)(sq->qs - 1) << 16) | sq->no;

    // dword11: [31:16]=연결할 CQ 식별자 ★, [1]=큐 우선순위, [0]=PC
    cmd->dword[11] = ((uint32_t)cq->no << 16) | (0x00 << 1) | (!need_prp);
    // cq->no: 이 SQ의 완료가 기록될 CQ의 ID
    // → SQ:CQ = 1:1 매핑 (BaM에서는 QP ID가 동일)
}
```

**NVMe 스펙과 매핑:**
```
dword10 레이아웃:
  ┌─────────────────┬─────────────────┐
  │ [31:16] QSIZE   │ [15:0] QID      │
  │ 큐 크기(0-based)│ 큐 식별자       │
  └─────────────────┴─────────────────┘

dword11 레이아웃 (Create SQ):
  ┌─────────────────┬───┬───┬───┐
  │ [31:16] CQID    │   │QPR│PC │
  │ 연결할 CQ ID ★ │   │   │   │
  └─────────────────┴───┴───┴───┘
```

### 4.6 init_gpu_specific_struct: GPU 병렬 접근용 배열 초기화

**파일:** `include/queue.h:70-94`

Admin 커맨드로 SQ/CQ를 NVMe 컨트롤러에 등록한 후, GPU 스레드 간 동기화를 위한 추가 배열을 GPU 메모리에 할당한다.

```cpp
void init_gpu_specific_struct(const uint32_t cudaDevice) {
    // ── SQ 관련 ──
    // tickets[qs]: Lamport Bakery 티켓 (slot별 generation 번호)
    this->sq_tickets  = createBuffer(sq.qs * sizeof(padded_struct), cudaDevice);
    // tail_mark[qs]: 도어벨 배칭용 마커 (LOCKED=쓰기완료, UNLOCKED=미완료)
    this->sq_tail_mark = createBuffer(sq.qs * sizeof(padded_struct), cudaDevice);
    // cid[65536]: Command ID 풀 (NVMe 스펙 최대 65536개 CID)
    this->sq_cid      = createBuffer(65536 * sizeof(padded_struct), cudaDevice);

    // nvm_queue_t에 GPU 포인터 연결
    sq.tickets   = (padded_struct*)sq_tickets.get();
    sq.tail_mark = (padded_struct*)sq_tail_mark.get();
    sq.cid       = (padded_struct*)sq_cid.get();
    sq.qs_minus_1 = sq.qs - 1;                    // 모듈러 → 비트AND 최적화
    sq.qs_log2    = (uint32_t)std::log2(sq.qs);    // 나눗셈 → 시프트 최적화

    // ── CQ 관련 ──
    // head_mark[qs]: CQ head 진행 마커 (완료 처리 추적)
    this->cq_head_mark = createBuffer(cq.qs * sizeof(padded_struct), cudaDevice);
    // pos_locks[qs]: CQ 슬롯 재사용 방지 잠금
    this->cq_pos_locks = createBuffer(cq.qs * sizeof(padded_struct), cudaDevice);

    cq.head_mark = (padded_struct*)cq_head_mark.get();
    cq.pos_locks = (padded_struct*)cq_pos_locks.get();
    cq.qs_minus_1 = cq.qs - 1;
    cq.qs_log2    = (uint32_t)std::log2(cq.qs);
}
```

**각 배열의 역할 요약:**

| 배열 | 크기 | 용도 | 사용 위치 |
|------|------|------|----------|
| `sq.tickets[qs]` | SQ 슬롯 수 | slot별 generation 번호 (Lamport Bakery) | `sq_enqueue()` — 슬롯 순서 보장 |
| `sq.tail_mark[qs]` | SQ 슬롯 수 | 커맨드 쓰기 완료 표시 | `sq_enqueue()` → `move_tail()` — 도어벨 배칭 |
| `sq.cid[65536]` | 65536 | Command ID 사용/해제 상태 | `get_cid()` / `put_cid()` |
| `cq.head_mark[qs]` | CQ 슬롯 수 | 완료 처리 완료 표시 | `cq_dequeue()` → `move_head_cq()` — CQ 도어벨 배칭 |
| `cq.pos_locks[qs]` | CQ 슬롯 수 | CQ 슬롯 재사용 경합 방지 | `cq_dequeue()` — 같은 위치 동시 접근 차단 |

### 4.7 생성 완료 후 메모리 배치도

```
NVMe SSD Controller (BAR0 MMIO)
  ├── 0x0000: CAP, VS, CC, CSTS ...
  ├── 0x1000 + (2*1+0)*4 = 0x1008: SQ1 도어벨 ──── cudaHostGetDevicePointer ──→ sq.db (GPU ptr)
  ├── 0x1000 + (2*1+1)*4 = 0x100C: CQ1 도어벨 ──── cudaHostGetDevicePointer ──→ cq.db (GPU ptr)
  ├── 0x1000 + (2*2+0)*4 = 0x1010: SQ2 도어벨
  ├── 0x1000 + (2*2+1)*4 = 0x1014: CQ2 도어벨
  └── ...

GPU VRAM
  ├── d_qps[0].sq.vaddr ──→ SQ1 메모리 (1024 × 64B = 64KB) ← NVMe가 P2P DMA로 fetch
  ├── d_qps[0].cq.vaddr ──→ CQ1 메모리 (1024 × 16B = 16KB) ← NVMe가 P2P DMA로 write
  ├── d_qps[0].sq.tickets  ──→ [1024 × padded_struct]
  ├── d_qps[0].sq.tail_mark ──→ [1024 × padded_struct]
  ├── d_qps[0].sq.cid      ──→ [65536 × padded_struct]
  ├── d_qps[0].cq.head_mark ──→ [1024 × padded_struct]
  ├── d_qps[0].cq.pos_locks ──→ [1024 × padded_struct]
  ├── d_qps[1].sq.vaddr ──→ SQ2 메모리 ...
  ├── ...
  ├── d_ctrl_ptr ──→ Controller 구조체 복사본 (queue_counter 포함)
  └── page_cache ──→ PRP 버퍼 (NVMe가 데이터를 쓸 GPU VRAM 주소)

호스트 메모리
  ├── h_qps[0..n_qps-1] ──→ 호스트 측 QueuePair 원본
  └── aq_mem ──→ Admin Queue (ASQ + ACQ + Identify 버퍼)
```

### 4.8 SQ와 CQ의 연결 관계 (NVMe 스펙 준수)

BaM에서 SQ:CQ는 **1:1** 관계이며, 같은 QP ID를 공유한다.

```
QueuePair[1]     QueuePair[2]     QueuePair[3]     ...
┌──────────┐    ┌──────────┐    ┌──────────┐
│  SQ #1   │    │  SQ #2   │    │  SQ #3   │
│  qp_id=1 │    │  qp_id=2 │    │  qp_id=3 │
│          │    │          │    │          │
│ CQID=1 ──┤    │ CQID=2 ──┤    │ CQID=3 ──┤
└──────┬───┘    └──────┬───┘    └──────┬───┘
       │               │               │
       ▼               ▼               ▼
┌──────────┐    ┌──────────┐    ┌──────────┐
│  CQ #1   │    │  CQ #2   │    │  CQ #3   │
└──────────┘    └──────────┘    └──────────┘

Admin Create SQ 커맨드의 dword11[31:16] = CQID 로 연결
```

**NVMe 스펙은 N:1 (여러 SQ → 1 CQ)도 허용하지만, BaM은 단순성을 위해 1:1 사용.** 이유:
- GPU의 수천 warp가 동시에 CQ를 polling → CQ도 분산되어야 경합 감소
- SQ와 CQ의 ID/도어벨/lock-free 배열을 하나의 QueuePair 구조체로 묶어 관리 편의성 확보

## 5. 같은 SQ를 공유하는 Warp 간 동기화

여러 warp가 같은 QP를 받으면 SQ slot 경합이 발생한다. BaM은 **Lamport's Bakery Ticket 알고리즘**으로 lock-free 동기화를 구현한다.

**파일:** `include/nvm_parallel_queue.h:231-342`

```
Warp A          Warp B          Warp C          (같은 SQ 공유)
   │               │               │
   ▼               ▼               ▼
① in_ticket.fetch_add(1)  ← atomic, 각자 고유 ticket 번호 획득
   ticket=0          ticket=1          ticket=2
   pos=0             pos=1             pos=2
   id=0              id=0              id=0     (generation)
   │               │               │
   ▼               ▼               ▼
② tickets[pos] == id 대기  ← slot별 generation 확인
   (pos=0 즉시통과)  (pos=1 즉시통과)  (pos=2 즉시통과)
   │               │               │
   ▼               ▼               ▼
③ SQ slot에 64B NVMe 커맨드 복사 (ulonglong4 × 2)
   │               │               │
   ▼               ▼               ▼
④ tail_mark[pos] = LOCKED  ← "나는 쓰기 완료"
   │               │               │
   ▼               ▼               ▼
⑤ 도어벨 배칭: tail_lock 획득한 1개 warp가 대표로
   연속된 LOCKED slot을 모아서 doorbell 1회만 발행

   asm("st.mmio.relaxed.sys.global.u32 [db], new_tail")
   ──────── PCIe MMIO ────────→ NVMe SSD 컨트롤러
```

### 도어벨 배칭 상세 (`move_tail`)

**파일:** `nvm_parallel_queue.h:93-126`

```cuda
uint32_t move_tail(nvm_queue_t* q, uint32_t cur_tail) {
    uint32_t count = 0;
    bool pass = true;
    while (pass) {
        // 큐가 full이 아닌지 확인
        pass = (((cur_tail+count+1) & q->qs_minus_1)
                != (q->head.load(memory_order_relaxed) & q->qs_minus_1));
        if (pass) {
            // tail_mark가 LOCKED(=쓰기 완료)인 연속 slot을 수집
            pass = (q->tail_mark[(cur_tail+count) & q->qs_minus_1]
                    .val.exchange(UNLOCKED, memory_order_relaxed) == LOCKED);
            if (pass) count++;
        }
    }
    return count;  // 한 번의 doorbell로 count개 커맨드를 한꺼번에 통지
}
```

- Warp A,B,C가 slot 0,1,2에 동시에 쓰면 → 대표 warp가 `move_tail`로 3개를 모아서 doorbell 1회
- **PCIe MMIO 오버헤드 절감**: doorbell 1회 = ~수백ns의 PCIe write → 3회→1회로 3배 절감

## 6. 비동기 I/O 인터페이스

GPU 스레드가 실제로 I/O를 발행하고 완료를 확인하는 인터페이스:

**파일:** `include/page_cache.h:2172-2190`

```cuda
// Phase 1: SQ에 커맨드 인큐 (doorbell까지, 완료 대기 없음)
inline __device__
void access_data_async(page_cache_d_t* pc, QueuePair* qp,
                       uint64_t starting_lba, uint64_t n_blocks,
                       unsigned long long pc_entry, uint8_t opcode,
                       uint16_t* cid, uint16_t* sq_pos) {
    nvm_cmd_t cmd;
    *cid = get_cid(&(qp->sq));                      // CID 할당
    nvm_cmd_header(&cmd, *cid, opcode, qp->nvmNamespace);
    nvm_cmd_data_ptr(&cmd, pc->prp1[pc_entry], ...); // PRP = GPU VRAM 주소
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);
    *sq_pos = sq_enqueue(&qp->sq, &cmd);             // SQ 인큐 + 도어벨
}

// Phase 2: CQ 폴링으로 완료 확인 + 자원 해제
inline __device__
void poll_async(QueuePair* qp, uint16_t cid, uint16_t sq_pos) {
    uint32_t cq_pos = cq_poll(&qp->cq, cid);        // CQ에서 해당 CID 탐색
    cq_dequeue(&qp->cq, cq_pos, &qp->sq);           // CQ head 갱신 + SQ ticket 해제
    put_cid(&qp->sq, cid);                           // CID 반환
}
```

**벤치마크에서의 사용 패턴 (I/O depth=n):**

```cuda
// main.cu:209-219
uint16_t cids[n];       // 레지스터에 n개 CID
uint16_t sq_poss[n];    // 레지스터에 n개 SQ 위치

// 비동기 발행: n개 커맨드를 연속 인큐 (대기 없음)
#pragma unroll
for (size_t i = 0; i < n; i++)
    access_data_async(pc, qp, start_block, n_blocks, tid, NVM_IO_READ, cids+i, sq_poss+i);

// 완료 회수: n개 커맨드의 CQ 엔트리를 폴링
#pragma unroll
for (size_t i = 0; i < n; i++)
    poll_async(qp, cids[i], sq_poss[i]);
```

## 7. 전체 연결 관계 요약도

```
┌─────────────── 호스트 초기화 (1회) ────────────────┐
│                                                     │
│  Controller("/dev/libnvm0", ns=1, gpu=0, qd=1024,  │
│             numQueues=128)                          │
│       │                                             │
│       ├── reserveQueues(MAX) → n_sqs, n_cqs 확보   │
│       ├── n_qps = min(n_sqs, n_cqs, 128)          │
│       │                                             │
│       └── for i in 0..n_qps:                       │
│              QueuePair(i+1, qd=1024)               │
│              ├── SQ: GPU VRAM에 DMA 할당            │
│              ├── CQ: GPU VRAM에 DMA 할당            │
│              ├── SQ doorbell → cudaHostGetDevicePtr │
│              ├── CQ doorbell → cudaHostGetDevicePtr │
│              └── tickets/marks/cids → GPU 할당      │
│                                                     │
│  → cudaMemcpy(d_qps, d_ctrl_ptr) → GPU에 복사      │
└─────────────────────────────────────────────────────┘

┌─────────────── GPU 커널 실행 (매 I/O) ─────────────┐
│                                                     │
│  Thread (수만 개)                                   │
│    │                                                │
│    ├── ① smid = get_smid()                         │
│    ├── ② ctrl = smid % n_ctrls           [SM 단위] │
│    ├── ③ queue = fetch_add(1) % n_qps    [Warp 단위]│
│    ├── ④ __shfl_sync → warp 32t 전파     [Thread]  │
│    │                                                │
│    ├── ⑤ get_cid() → atomic CID 할당               │
│    ├── ⑥ NVMe Read Cmd 작성 (PRP=GPU VRAM)         │
│    ├── ⑦ sq_enqueue() → ticket slot 할당 + 복사    │
│    ├── ⑧ move_tail() → 연속 slot 모아 doorbell 1회 │
│    │      asm("st.mmio ... [db], new_tail")         │
│    │      ─────── PCIe P2P ───────→ NVMe SSD       │
│    │                                                │
│    ├── ⑨ cq_poll() → CQ phase bit 폴링            │
│    │      ◄────── SSD가 CQ에 완료 기록 ────────    │
│    ├── ⑩ cq_dequeue() → CQ head doorbell + ticket해제│
│    └── ⑪ put_cid() → CID 반환                     │
│                                                     │
│  → 데이터가 GPU VRAM PRP 버퍼에 도착                │
└─────────────────────────────────────────────────────┘
```

## 8. SPDK와의 비교

| 항목 | SPDK (CPU) | BaM (GPU) |
|------|-----------|-----------|
| **I/O 발행자** | CPU 스레드 | GPU warp (32 threads) |
| **큐 할당 단위** | CPU 코어당 1 SQ (lock-free) | Warp → 라운드로빈 QP |
| **경합 해결** | 코어당 전용 큐 → 경합 없음 | Lamport Ticket → lock-free 경합 해결 |
| **도어벨** | CPU MMIO write | GPU PTX `st.mmio` |
| **배칭** | 명시적 배치 제출 | tail_mark 기반 자동 배칭 |
| **완료 확인** | CQ 폴링 (CPU spin) | CQ 폴링 (GPU spin + nanosleep) |
| **Latency hiding** | 없음 (CPU가 블록) | warp 스위칭으로 자연스러운 hiding |

**핵심 차이:** SPDK는 코어당 전용 큐로 경합을 원천 차단하지만, GPU는 SM당 수십 warp가 동시 실행되므로 큐 공유가 불가피 → BaM이 Lamport Ticket + 도어벨 배칭으로 이를 해결.

## 9. 수치 예시

A100 GPU(108 SM) + NVMe SSD 1개(n_qps=128, qd=1024) 기준:

```
SM 수:          108
Warp/SM:        ~32 (동시 실행 가능)
총 동시 Warp:   ~3,456
총 동시 Thread: ~110,592
QP 수:          128
Warp/QP (평균): ~27 warp가 1개 QP 공유
SQ 엔트리:      1024개/QP
→ 27 warp × 32 thread = 864 thread가 1024 slot 공유
→ slot 경합은 있으나 ticket 알고리즘으로 lock-free 처리
→ doorbell 배칭으로 최대 수십 개 커맨드를 1회 doorbell로 통합
```
