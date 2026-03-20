# BaM GPU Thread → NVMe SQ 매핑 관계 분석

**소스코드 위치:** `sources/bam/`
**핵심 파일:**
- `benchmarks/iodepth-block/main.cu:173-220` — Warp→Queue 선택 로직
- `include/ctrl.h:48-245` — Controller 구조체, 큐 생성, queue_counter
- `include/queue.h:106-220` — QueuePair 생성 (SQ/CQ DMA + 도어벨 GPU 매핑)
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

## 4. QueuePair 생성 과정 (호스트 측)

GPU 커널이 사용할 QP는 호스트 측 Controller 생성자에서 만들어진다.

**파일:** `include/ctrl.h:176-245`

```
Controller 생성자 흐름:

  ① open("/dev/libnvmX") → nvm_ctrl_init() → BAR0 MMIO 매핑
  ② Admin Queue 생성 → 컨트롤러 리셋/Identify
  ③ reserveQueues(MAX_QUEUES) → Set/Get Number of Queues
  ④ n_qps = min(n_sqs, n_cqs, numQueues)
  ⑤ for i in 0..n_qps:
       h_qps[i] = new QueuePair(ctrl, cudaDevice, ..., i+1, queueDepth)
       cudaMemcpy(d_qps+i, h_qps[i], ..., HostToDevice)
  ⑥ cudaMemcpy(d_ctrl_ptr, this, ..., HostToDevice)  // Controller 자체도 GPU로
```

**핵심 코드:** `ctrl.h:217-236`

```cpp
n_qps = std::min(n_sqs, n_cqs);          // HW가 지원하는 SQ/CQ 수
n_qps = std::min(n_qps, (uint16_t)numQueues);  // 사용자 요청 수
printf("SQs: %d\tCQs: %d\tn_qps: %d\n", n_sqs, n_cqs, n_qps);

h_qps = (QueuePair**) malloc(sizeof(QueuePair)*n_qps);
cuda_err_chk(cudaMalloc((void**)&d_qps, sizeof(QueuePair)*n_qps));

for (size_t i = 0; i < n_qps; i++) {
    h_qps[i] = new QueuePair(ctrl, cudaDevice, ns, info, aq_ref,
                              i+1,           // QP ID (1부터 시작, 0은 Admin)
                              queueDepth);   // SQ 엔트리 수
    cuda_err_chk(cudaMemcpy(d_qps+i, h_qps[i], sizeof(QueuePair),
                            cudaMemcpyHostToDevice));
}
```

QueuePair 생성자 (`queue.h:106-220`) 내부에서:
1. SQ/CQ DMA 메모리를 GPU 디바이스에 할당 (`createDma(ctrl, size, cudaDevice)`)
2. Admin 커맨드로 CQ 생성 → `nvm_admin_cq_create()`
3. CQ 도어벨을 GPU 포인터로 변환 → `cudaHostGetDevicePointer(&devicePtr, cq.db)`
4. Admin 커맨드로 SQ 생성 → `nvm_admin_sq_create()`
5. SQ 도어벨을 GPU 포인터로 변환 → `cudaHostGetDevicePointer(&devicePtr, sq.db)`
6. lock-free 병렬 접근용 배열 할당:
   - `sq_tickets[qs]` — Lamport's Bakery 티켓 (slot별 generation)
   - `sq_tail_mark[qs]` — 도어벨 배칭 마커 (LOCKED/UNLOCKED)
   - `sq_cid[65536]` — Command ID 풀
   - `cq_head_mark[qs]` — CQ head 진행 마커
   - `cq_pos_locks[qs]` — CQ position 레벨 잠금

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
