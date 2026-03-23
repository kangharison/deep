# BaM block/main.cu GPU 커널 & 병렬 NVMe 큐 심층 분석

## 1. 전체 아키텍처 개요

block/main.cu는 BaM 프레임워크의 블록 수준 I/O 벤치마크이다. GPU 스레드가 CPU 개입 없이 NVMe SSD에 직접 I/O 커맨드를 발행하고 완료를 폴링하는 전체 경로를 구현한다.

```
┌─────────────────────── GPU ───────────────────────┐
│                                                    │
│  ┌─────────────────────────────────────────────┐  │
│  │        CUDA Kernel (수천 개의 스레드)          │  │
│  │                                              │  │
│  │  Thread 0 ──┐                                │  │
│  │  Thread 1 ──┤  각 스레드가 독립적으로          │  │
│  │  Thread 2 ──┤  read_data() / write_data()    │  │
│  │    ...      │  를 호출하여 NVMe I/O 수행       │  │
│  │  Thread N ──┘                                │  │
│  └──────┬──────────────────────────┬────────────┘  │
│         │                          │               │
│  ┌──────▼──────┐           ┌───────▼──────┐       │
│  │  SQ (GPU    │           │  CQ (GPU     │       │
│  │  메모리)     │           │  메모리)      │       │
│  │  커맨드 작성  │           │  완료 폴링    │       │
│  └──────┬──────┘           └───────▲──────┘       │
│         │ 도어벨 (MMIO)             │ DMA           │
│         │ st.mmio PTX              │               │
└─────────┼──────────────────────────┼───────────────┘
          │       PCIe Bus           │
┌─────────▼──────────────────────────┼───────────────┐
│                NVMe SSD                             │
│  1) 도어벨을 읽어 새 SQ 엔트리 확인                    │
│  2) SQ에서 커맨드를 DMA로 가져옴 (GPU 메모리에서)       │
│  3) 데이터를 GPU 메모리의 PRP 주소로 DMA 전송           │
│  4) CQ에 완료 엔트리를 DMA로 기록 (GPU 메모리에)        │
└─────────────────────────────────────────────────────┘
```

핵심 포인트: SQ/CQ 모두 GPU 메모리에 있고, 도어벨은 NVMe BAR0의 MMIO 레지스터에 GPU가 PTX `st.mmio` 명령으로 직접 쓴다. CPU는 초기화 이후 데이터 경로에 전혀 관여하지 않는다.


## 2. GPU 커널 상세 분석

### 2.1 sequential_access_kernel

```c
__global__ __launch_bounds__(64,32)
void sequential_access_kernel(Controller** ctrls, page_cache_d_t* pc,
    uint32_t req_size, uint32_t n_reqs, unsigned long long* req_count,
    uint32_t num_ctrls, uint64_t reqs_per_thread,
    uint32_t access_type, uint8_t* access_type_assignment);
```

**launch_bounds(64,32)**: 블록당 최대 64스레드(= warp 2개), SM당 최대 32블록. 레지스터 압력을 줄이고 SM 점유율을 극대화한다.

#### 컨트롤러/큐 선택 전략

```
Warp (32 스레드)
├── Lane 0: ctrl = atomic_counter++ % n_ctrls     ← 라운드로빈
│            queue = smid % n_qps                  ← SM 기반 지역성
├── Lane 1~31: __shfl_sync로 Lane 0의 값을 받음
└── 결과: warp 전체가 같은 (ctrl, queue) 쌍을 사용
```

- **컨트롤러**: `ctrl_counter`를 atomic 증가 → `% n_ctrls`로 라운드로빈
- **큐**: `smid % n_qps`로 SM ID 기반 배정. 같은 SM의 warp들이 같은 큐를 공유하므로 L1 캐시 지역성이 향상됨
- **__shfl_sync(0xFFFFFFFF, ctrl, 0)**: warp 내 모든 레인이 lane 0의 값을 받음. 레지스터 간 전달이므로 메모리 접근 없이 O(1)

#### 주소 계산

```
tid = blockIdx.x * blockDim.x + threadIdx.x

start_block = (tid * req_size) >> block_size_log
            = (tid * page_size) / lba_data_size

n_blocks = req_size >> block_size_log
         = page_size / lba_data_size

예: page_size=4096, lba_data_size=512 → n_blocks=8
    tid=5 → start_block = 5*4096/512 = 40
    즉, LBA 40부터 8개 블록을 읽음
```

각 스레드는 `reqs_per_thread`만큼 반복하여 **같은 주소**에 I/O를 수행한다. 이는 캐시 효과나 반복 성능을 측정하기 위함이다.


### 2.2 random_access_kernel

순차 커널과 거의 동일하지만 두 가지가 다르다:

| 항목 | sequential | random |
|------|-----------|--------|
| 주소 계산 | `tid * req_size` | `assignment[tid] * req_size` |
| 큐 선택 | `smid % n_qps` (SM 기반) | `queue_counter++ % n_qps` (라운드로빈) |
| launch_bounds | `(64,32)` 명시 | 명시하지 않음 (컴파일러 자동) |

- **assignment 배열**: 호스트에서 `rand() % n_blocks`로 생성하여 GPU로 cudaMemcpy한 랜덤 인덱스
- **큐 선택이 라운드로빈인 이유**: 랜덤 접근은 어차피 SSD 내부에서 지역성이 없으므로, SM 기반 지역성 최적화의 이점이 없다


### 2.3 access_type에 따른 분기

```
access_type == READ  → read_data()    : SSD → GPU 캐시 슬롯
access_type == WRITE → write_data()   : GPU 캐시 슬롯 → SSD
access_type == MIXED → access_data()  : opcode로 구분 (스레드별 다름)
```

MIXED 모드에서 각 스레드의 opcode는 `access_type_assignment[tid]`에 저장되어 있으며, 호스트에서 `rand() % 100 <= ratio`로 읽기/쓰기를 확률적으로 배정한다.


## 3. I/O 함수 호출 체인

### 3.1 read_data() 전체 흐름

```
read_data(pc, qp, starting_lba, n_blocks, pc_entry)
│
├── (1) get_cid(&qp->sq)                    ← CID 할당 (lock-free)
│
├── (2) nvm_cmd_header(&cmd, cid, NVM_IO_READ, ns)  ← 커맨드 헤더 빌드
│       nvm_cmd_data_ptr(&cmd, prp1, prp2)           ← PRP 주소 설정
│       nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks)← LBA 범위 설정
│
├── (3) sq_enqueue(&qp->sq, &cmd)            ← SQ에 인큐 + 도어벨
│       │
│       ├── ticket = in_ticket.fetch_add(1)  ← 전역 순서 번호 발급
│       ├── spin-wait: tickets[pos] == id    ← 자기 순서까지 대기
│       ├── memcpy(sq_slot, cmd, 64B)        ← 커맨드 복사
│       ├── tail_mark[pos] = LOCKED          ← 기록 완료 표시
│       ├── [리더] move_tail → st.mmio(db)   ← 도어벨 쓰기
│       └── wait: tail_mark[pos] == UNLOCKED ← 도어벨 반영 대기
│
├── (4) cq_poll(&qp->cq, cid)               ← CQ 폴링 (완료 대기)
│       │
│       └── while(true):
│           ├── CQ head부터 스캔
│           ├── dword[3]에서 CID + Phase 확인
│           └── 매칭 시 CQ 인덱스 반환
│
├── (5) cq_dequeue(&qp->cq, cq_pos, &qp->sq) ← CQ 디큐 + CQ 도어벨
│       │
│       ├── pos_locks[pos] 획득             ← 슬롯별 잠금
│       ├── head_mark[pos] = LOCKED         ← 처리 완료 표시
│       ├── [리더] move_head_cq             ← CQ head 전진
│       │   ├── 연속 LOCKED 수집
│       │   ├── SQ Head Pointer 읽기 (CQ dword[2])
│       │   ├── SQ head 전진 + tickets 증가
│       │   └── st.mmio(cq->db)             ← CQ 도어벨 쓰기
│       ├── wait: head가 loc_를 지나감       ← 반영 확인
│       └── pos_locks[pos] 해제
│
├── (6) enqueue_second(...)                  ← 이중 읽기 최적화
│
└── (7) put_cid(&qp->sq, cid)               ← CID 반환
```

### 3.2 write_data()

read_data()와 동일한 구조이지만:
- opcode가 `NVM_IO_WRITE`
- `enqueue_second()` (이중 읽기 최적화)를 **수행하지 않음**
- 데이터 방향: GPU 캐시 슬롯 → SSD

### 3.3 access_data()

read_data/write_data의 간소화 버전:
- opcode를 파라미터로 받아 읽기/쓰기를 구분
- 이중 읽기 최적화 없음
- `cq_poll`과 `cq_dequeue`가 단순 버전 (loc_, head 추적 없음)


## 4. 병렬 NVMe 큐 (nvm_parallel_queue.h) 핵심 메커니즘

### 4.1 핵심 과제

수천 개의 GPU 스레드가 **동시에** 하나의 SQ/CQ 쌍에 접근한다. 커널 NVMe 드라이버는 단일 CPU에서 SQ를 독점하지만, BaM은 수천 스레드가 경합하므로 완전히 다른 동기화가 필요하다.

### 4.2 Ticket-based 순서 보장 (sq_enqueue)

```
           Thread A         Thread B         Thread C
              │                │                │
              ▼                ▼                ▼
    ticket=0 (pos=0,id=0)  ticket=1 (pos=1,id=0)  ticket=2 (pos=2,id=0)
              │                │                │
     tickets[0]==0? YES  tickets[1]==0? YES  tickets[2]==0? YES
              │                │                │
    cmd→SQ[0] 복사       cmd→SQ[1] 복사       cmd→SQ[2] 복사
              │                │                │
    tail_mark[0]=LOCKED  tail_mark[1]=LOCKED  tail_mark[2]=LOCKED
              │                │                │
              └────────┬───────┘                │
                 리더 선출 (tail_lock)            │
                       │                        │
              move_tail: 연속 LOCKED 수집         │
              count=2 (pos 0,1 연속)             │
                       │                        │
              st.mmio(db, 2)  ← 도어벨 1회!      │
              tail_mark[0,1]=UNLOCKED            │
                                                │
                                         다음 리더가 pos 2 처리
```

**핵심 최적화**: 도어벨 쓰기(PCIe MMIO)는 비싸므로 (`st.mmio`는 ~1μs), 여러 커맨드를 모아서 도어벨을 1회만 울린다. 리더 선출 메커니즘이 이를 자동으로 수행한다.

### 4.3 티켓과 세대(Generation) 번호

```
ticket = 5, queue_size = 4 (qs_log2 = 2)

pos = ticket & (4-1) = 5 & 3 = 1         ← 슬롯 위치
id  = (5 >> 2) * 2 = 1 * 2 = 2           ← 세대 번호 (짝수 = 인큐)

tickets[1]이 2가 될 때까지 spin-wait
인큐 완료 후 tickets[1]을 3으로 증가 (홀수 = 디큐 세대)
디큐 완료 후 tickets[1]을 4로 증가 (짝수 = 다음 인큐 세대)
```

- 짝수 세대: 인큐 대기 → 커맨드 기록 허용
- 홀수 세대: 디큐 대기 → 슬롯 해제 허용
- 이를 통해 같은 슬롯에 대한 인큐/디큐 순서가 자연스럽게 교대된다

### 4.4 CID (Command ID) 풀 관리

```
CID 풀: 65536개의 슬롯 (uint64_t 배열, 각각 LOCKED/UNLOCKED)

get_cid():
  1) cid_ticket을 atomic 증가 → 후보 ID 결정
  2) cid[id].fetch_or(LOCKED) → 이전 값이 UNLOCKED면 선점 성공
  3) 실패하면 다음 ticket으로 재시도

put_cid(id):
  cid[id].store(UNLOCKED) → 반환
```

NVMe 스펙에서 CID는 SQ 내에서 유일해야 한다. 여러 스레드가 동시에 커맨드를 발행하므로, CID 충돌을 방지하기 위해 lock-free 풀을 사용한다.

### 4.5 CQ 폴링 메커니즘

```
CQ 메모리 레이아웃 (각 엔트리 16바이트):
┌──────────┬──────────┬───────────────────┬──────────────────┐
│ dword[0] │ dword[1] │     dword[2]      │     dword[3]     │
│ cmd-spec │ cmd-spec │ SQ Head | SQ ID   │ Status | P | CID │
└──────────┴──────────┴───────────────────┴──────────────────┘
                                                   ▲
                                            Phase bit (비트 16)

cq_poll 스캔 로직:
  head부터 순회하면서:
    1) dword[3]에서 CID 추출 (하위 16비트)
    2) dword[3]에서 Phase bit 추출 (비트 16)
    3) 예상 phase = ~(cur_head >> qs_log2) & 1
    4) CID 매칭 + Phase 매칭 → 완료 확인
    5) Phase 불일치 → 아직 새 완료가 없으므로 스캔 중단
```

**Phase bit**: NVMe 컨트롤러가 CQ 엔트리를 기록할 때 Phase를 설정한다. CQ를 한 바퀴 돌 때마다 phase가 반전된다. 이를 통해 새 완료와 이전 바퀴의 완료를 구분한다.


## 5. enqueue_second: 이중 읽기 최적화

```
read_data()의 마지막 단계에서 호출됨.

목적: 첫 번째 읽기 완료 후, 같은 LBA를 한 번 더 읽어서
      NVMe 컨트롤러의 내부 캐시(DRAM 캐시)를 활용한다.

동작:
  1) q_head/q_tail로 글로벌 I/O 순서를 추적
  2) q_lock을 잡아서 단일 스레드만 이중 읽기를 수행
  3) 같은 LBA로 SQ 인큐 → CQ 폴링 → 완료
  4) q_head를 업데이트하여 다른 스레드에게 진행 상황을 알림
  5) extra_reads 카운터를 증가시켜 통계 추적

q_lock을 잡지 못한 스레드:
  - __nanosleep(8~256ns)으로 지수 백오프
  - q_head가 자신의 pc_pos를 지나가면 다른 스레드가 대신 처리한 것이므로 탈출

sec 조건 (순환 버퍼 wrap-around 비교):
  (cur_head < prev_head && prev_head <= pos) ||
  (prev_head <= pos && pos < cur_head) ||
  (pos < cur_head && cur_head < prev_head)
  → cur_head가 pos를 지나갔으면 true
```

이 최적화는 NVMe SSD의 DRAM 캐시를 활용한다. 첫 번째 읽기에서 SSD 내부 DRAM에 데이터가 올라오므로, 두 번째 읽기는 NAND 접근 없이 DRAM에서 바로 응답한다. 이는 특히 핫 데이터에 대한 반복 접근에서 레이턴시를 크게 줄인다.


## 6. 지수 백오프 (Exponential Backoff)

모든 spin-wait 루프에서 동일한 패턴이 사용된다:

```c
unsigned int ns = 8;       // 초기 8 나노초
while (condition) {
    __nanosleep(ns);       // CUDA 7.0+ 지원, SM에서 스레드를 일시 중단
    if (ns < 256) {
        ns *= 2;           // 8 → 16 → 32 → 64 → 128 → 256 (최대)
    }
}
```

- **__nanosleep**: 스레드를 지정된 나노초 동안 중단. 다른 warp에 실행 기회를 줌
- **목적**: PCIe 대역폭을 불필요한 폴링 트래픽으로 낭비하지 않음
- **최대 256ns**: 너무 길면 레이턴시가 증가하므로 상한을 둠


## 7. main() 함수 벤치마크 파이프라인

```
(1) Settings 파싱
     │
(2) NVMe 컨트롤러 초기화 (n_ctrls개)
     │  Controller(path, ns, gpu, queueDepth, numQueues)
     │  → BAR0 mmap → Admin Queue → Identify → I/O QueuePair 생성
     │
(3) Page Cache 생성
     │  page_cache_t(page_size, n_pages, gpu, ctrl, 64, ctrls)
     │  → GPU cudaMalloc(page_size * n_pages)
     │  → 각 페이지의 PRP 물리 주소 준비
     │
(4) [random] 랜덤 인덱스 배열 생성 → cudaMemcpy
     │
(5) [MIXED] 읽기/쓰기 비율 배정 → cudaMemcpy
     │
(6) Event(before) ← cudaEventRecord
     │
     ├── [random]     random_access_kernel<<<g,b>>>(...)
     └── [sequential] sequential_access_kernel<<<g,b>>>(...)
     │
     Event(after) ← cudaEventRecord
     │
     cudaDeviceSynchronize()  ← 커널 완료 대기
     │
(7) 성능 계산:
     elapsed = after - before (마이크로초)
     ios     = n_threads * numReqs
     iops    = ios / (elapsed / 1,000,000)
     bw      = (ios * page_size) / elapsed / 1GiB
     │
     h_pc.print_reset_stats()  ← 캐시 적중률, 미스율 출력
```


## 8. 커널 NVMe 드라이버와의 비교

| 항목 | Linux 커널 NVMe 드라이버 | BaM 병렬 큐 |
|------|----------------------|-------------|
| SQ 접근 주체 | CPU 1개 (per-CPU queue) | GPU 스레드 수천 개 |
| SQ 동기화 | spin_lock 또는 lock-free (단일 생산자) | Ticket + tail_mark + 리더 선출 |
| 도어벨 쓰기 | writel() — CPU MMIO | `st.mmio.relaxed.sys.global.u32` — GPU PTX |
| 도어벨 배칭 | 매 커맨드마다 또는 nvme_write_sq_db 조건부 | move_tail로 연속 인큐를 모아서 1회 |
| CQ 폴링 | irq_poll / nvme_irqs_disabled | GPU 스레드가 CQ 메모리를 직접 폴링 |
| CQ 도어벨 | 인터럽트 + completion doorbell | GPU가 `st.mmio`로 직접 업데이트 |
| SQ/CQ 위치 | 호스트 DRAM (DMA coherent) | GPU VRAM (P2P DMA) |
| CID 관리 | blk-mq tag 시스템 | 65536 슬롯 lock-free 풀 |
| 인터럽트 | MSI-X → softirq | 없음 (100% 폴링) |


## 9. 성능 관련 핵심 설계 결정

### 9.1 Warp 단위 컨트롤러/큐 공유
- 같은 warp의 32스레드가 동일 (ctrl, queue)를 사용
- `__shfl_sync`로 레지스터 간 전파하므로 메모리 접근 제로
- SQ 인큐 시 warp 내 스레드들이 인접 슬롯을 받아 move_tail의 연속성 향상

### 9.2 SM 기반 큐 배정 (순차)
- `queue = smid % n_qps`: 같은 SM에서 실행되는 warp들이 같은 큐를 공유
- GPU L1 캐시에 SQ/CQ 메타데이터가 유지되어 폴링 성능 향상
- 랜덤 접근에서는 이 이점이 없으므로 라운드로빈으로 전환

### 9.3 도어벨 배칭
- MMIO 쓰기(st.mmio)는 ~1μs로 비쌈 (PCIe TLP 왕복)
- move_tail이 연속 LOCKED 마크를 모아서 도어벨을 1회만 울림
- N개 커맨드에 대해 도어벨 1회 → PCIe 대역폭 절약

### 9.4 이중 읽기 (enqueue_second)
- 첫 읽기: NAND → SSD DRAM 캐시 → GPU (느림)
- 둘째 읽기: SSD DRAM 캐시 → GPU (빠름)
- NVMe SSD의 내부 DRAM 캐시를 워밍업하는 효과


## 10. 실행 예시

```bash
# 랜덤 읽기, 1024 스레드, 페이지 64개, 4KB 페이지, 큐 2개, 컨트롤러 1개
./block -r true -t 1024 -p 1024 -P 4096 -q 2 -k 1 -n 10 -o 0

# 순차 쓰기, 2048 스레드, 8KB 페이지
./block -r false -t 2048 -p 2048 -P 8192 -o 1

# 혼합 (70% 읽기, 30% 쓰기)
./block -r true -t 1024 -p 1024 -o 2 -s 70
```

출력 예시:
```
Elapsed Time: 5234.56   Number of Ops: 10240   Data Size (bytes): 41943040
Ops/sec: 1956520.3      Effective Bandwidth(GB/S): 7.47
Cache Hits: 8192  Misses: 2048  Hit Rate: 80.0%
```
