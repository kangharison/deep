# BaM Lock-Free 병렬 큐 메커니즘 분석

## 개요

`nvm_parallel_queue.h`는 수천 개의 GPU 스레드가 동시에 NVMe SQ(Submission Queue)에 명령을 삽입하고, CQ(Completion Queue)에서 완료를 수거할 수 있게 하는 lock-free 병렬 큐 메커니즘을 구현한다. 표준 NVMe에서 SQ/CQ는 단일 프로듀서-단일 컨슈머 모델이지만, BaM은 이를 다중 프로듀서-다중 컨슈머로 확장한다.

**핵심 파일**: `include/nvm_parallel_queue.h`

## 기존 NVMe 큐 vs BaM 병렬 큐 비교

```
┌────────────────────────────────┬────────────────────────────────┐
│      기존 NVMe 큐              │        BaM 병렬 큐             │
├────────────────────────────────┼────────────────────────────────┤
│ 단일 프로듀서 (CPU)            │ 다중 프로듀서 (GPU 스레드)     │
│ 단일 컨슈머 (NVMe 컨트롤러)   │ 다중 컨슈머 (GPU 스레드)       │
│ tail 직접 이동 + 도어벨        │ 티켓 기반 위치 할당 + 집합적   │
│                                │ tail 이동 + 도어벨             │
│ head는 컨트롤러가 관리         │ head를 GPU가 소프트웨어로 추적 │
│ CID = tail 기반 순차 할당      │ CID = lock-free 티켓 풀        │
│ 순차적 제출                    │ 비순차적 제출 (슬롯 단위)      │
│ CQ 순차 dequeue                │ CQ CID 검색 기반 dequeue       │
│ 도어벨 = CPU MMIO 쓰기        │ 도어벨 = GPU MMIO (st.mmio)    │
│ 메모리 배리어: CPU mfence      │ 메모리 배리어: __threadfence   │
│                                │  + simt::memory_order_*        │
└────────────────────────────────┴────────────────────────────────┘
```

## nvm_queue_t 구조체 상세

**파일**: `include/nvm_types.h:123-165`

```cpp
typedef struct {
    // 동기화를 위한 padded atomic 변수들 (32바이트 정렬)
    simt::atomic<uint32_t, simt::thread_scope_device> head_lock;   // head 이동 락
    uint8_t pad0[28];
    simt::atomic<uint32_t, simt::thread_scope_device> tail_lock;   // tail 이동 락
    uint8_t pad1[28];
    simt::atomic<uint32_t, simt::thread_scope_device> head;        // 논리적 head
    uint8_t pad2[28];
    simt::atomic<uint32_t, simt::thread_scope_device> tail;        // 논리적 tail
    uint8_t pad3[28];
    simt::atomic<uint32_t, simt::thread_scope_system> tail_copy;   // tail 복사본
    uint8_t pad4[28];
    simt::atomic<uint32_t, simt::thread_scope_system> head_copy;   // head 복사본
    uint8_t pad5[28];

    simt::atomic<uint32_t, simt::thread_scope_device> in_ticket;   // SQ 삽입 티켓
    uint8_t pad6[28];
    simt::atomic<uint32_t, simt::thread_scope_device> cid_ticket;  // CID 할당 티켓

    padded_struct* tickets;       // 슬롯별 티켓 번호 (라운드 추적)
    padded_struct* head_mark;     // 슬롯별 head 이동 완료 마크
    padded_struct* tail_mark;     // 슬롯별 tail 이동 완료 마크
    padded_struct* cid;           // CID 사용 상태 (LOCKED/UNLOCKED)
    padded_struct* pos_locks;     // CQ 위치 락

    uint32_t qs_minus_1;          // qs - 1 (비트 마스크용)
    uint32_t qs_log2;             // log2(qs) (라운드 계산용)
    uint16_t no;                  // 큐 번호
    uint16_t es;                  // 엔트리 크기
    uint32_t qs;                  // 큐 크기
    int8_t   phase;               // 현재 phase tag
    int8_t   local;               // 로컬 메모리 여부
    volatile uint32_t* db;        // 도어벨 레지스터 포인터 (GPU 매핑)
    volatile void* vaddr;         // 큐 메모리 가상 주소
    uint64_t ioaddr;              // 큐 메모리 물리 주소
} nvm_queue_t;
```

**28바이트 패딩 이유**: GPU의 캐시라인 크기(보통 128바이트)에 맞춰 false sharing을 방지한다. `simt::atomic<uint32_t>` (4바이트) + 패딩 28바이트 = 32바이트, 4개의 변수가 하나의 128바이트 캐시라인을 점유한다.

## 티켓 기반 CID 할당 알고리즘

### get_cid() - CID 할당

**파일**: `nvm_parallel_queue.h:36`

```cpp
inline __device__
uint16_t get_cid(nvm_queue_t* sq) {
    bool not_found = true;
    uint16_t id;

    do {
        // 1. 후보 CID를 티켓으로 선택
        id = sq->cid_ticket.fetch_add(1, simt::memory_order_relaxed) & 65535;

        // 2. 해당 CID가 사용 가능한지 확인 (UNLOCKED → LOCKED)
        uint64_t old = sq->cid[id].val.fetch_or(LOCKED, simt::memory_order_acquire);
        not_found = (old == LOCKED);  // 이미 사용 중이면 재시도
    } while (not_found);

    return id;
}
```

```
CID 풀 (65536개):
┌───┬───┬───┬───┬───┬───┬───┬─────┐
│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ ... │
│ U │ L │ U │ U │ L │ U │ L │     │  U=Unlocked, L=Locked
└───┴───┴───┴───┴───┴───┴───┴─────┘
      ↑
  cid_ticket 현재 위치

스레드 A: cid_ticket=1 → cid[1]=L (이미 사용) → 재시도
           cid_ticket=2 → cid[2]=U → fetch_or(L) → CID=2 획득
스레드 B: cid_ticket=3 → cid[3]=U → fetch_or(L) → CID=3 획득
```

### put_cid() - CID 반환

**파일**: `nvm_parallel_queue.h:55`

```cpp
inline __device__
void put_cid(nvm_queue_t* sq, uint16_t id) {
    sq->cid[id].val.store(UNLOCKED, simt::memory_order_release);
}
```

## SQ Enqueue: 티켓 기반 슬롯 할당

### sq_enqueue() 전체 흐름

**파일**: `nvm_parallel_queue.h:168`

```
sq_enqueue() 단계별 흐름:
┌──────────────────────────────────────────────────────────────┐
│ Phase 1: 슬롯 할당 (티켓 획득)                               │
│                                                               │
│   ticket = in_ticket.fetch_add(1)     // 원자적 티켓 획득    │
│   pos = ticket & qs_minus_1           // 큐 내 위치 (모듈로) │
│   id = get_id(ticket, qs_log2)        // 라운드 번호         │
│                                                               │
│ Phase 2: 슬롯 대기 (이전 라운드 완료 확인)                   │
│                                                               │
│   while (tickets[pos] != id)          // 내 차례 대기        │
│       __nanosleep(ns)                 // 지수 백오프         │
│                                                               │
│ Phase 3: 명령 복사                                            │
│                                                               │
│   queue_loc[i] = cmd_[i]             // ulonglong4 단위 복사 │
│                                                               │
│ Phase 4: 준비 완료 표시                                       │
│                                                               │
│   tail_mark[pos] = LOCKED             // 이 슬롯 준비됨      │
│                                                               │
│ Phase 5: 도어벨 링 시도                                       │
│                                                               │
│   while (tail_mark[pos] == LOCKED)    // 아직 제출 안 됨     │
│     if (tail_lock == UNLOCKED)        // 도어벨 락 시도      │
│       tail_lock = LOCKED                                      │
│       move_count = move_tail(sq, cur_tail)                    │
│       if (move_count > 0)                                     │
│         *(sq->db) = new_tail          // 도어벨 쓰기!        │
│         sq->tail = new_tail                                   │
│       tail_lock = UNLOCKED                                    │
│                                                               │
│ Phase 6: 티켓 갱신 (다음 라운드 허용)                        │
│                                                               │
│   tickets[pos].fetch_add(1)           // 다음 스레드 진입 허용│
└──────────────────────────────────────────────────────────────┘
```

### get_id() - 라운드 번호 계산

**파일**: `nvm_parallel_queue.h:28`

```cpp
__device__ uint64_t get_id(uint64_t x, uint64_t y) {
    return (x >> y) * 2;  // (ticket / queue_size) * 2
}
```

큐 크기가 1024(qs_log2=10)인 경우:
- ticket=0 → id=0, ticket=1023 → id=0 (같은 라운드)
- ticket=1024 → id=2, ticket=2047 → id=2 (다음 라운드)
- ticket=2048 → id=4 (그 다음 라운드)

`*2`를 하는 이유: `tickets[pos]`는 enqueue 시 +1, SQ head 이동 시 +1로 총 2씩 증가하므로, 다음 라운드의 id는 2 간격이다.

### move_tail() - 연속 슬롯 수집

**파일**: `nvm_parallel_queue.h:60`

```cpp
inline __device__
uint32_t move_tail(nvm_queue_t* q, uint32_t cur_tail) {
    uint32_t count = 0;
    bool pass = true;

    while (pass) {
        // SQ가 가득 차지 않았는지 확인
        pass = ((cur_tail+count+1) & q->qs_minus_1) !=
               (q->head.load(...) & q->qs_minus_1);

        if (pass) {
            // tail_mark가 LOCKED(준비됨)인 연속 슬롯 수집
            pass = (q->tail_mark[(cur_tail+count) & q->qs_minus_1]
                    .val.exchange(UNLOCKED, ...)) == LOCKED;
            if (pass)
                count++;
        }
    }
    q->head_lock.fetch_add(1, simt::memory_order_acq_rel);
    return count;
}
```

```
tail_mark 배열 상태 예시 (qs=8, cur_tail=3):
 pos:  0  1  2  3  4  5  6  7
       U  U  U  L  L  L  U  U
                ↑
             cur_tail

move_tail은 pos 3,4,5의 연속 LOCKED 슬롯을 수집 → count=3
도어벨에 (3+3) % 8 = 6을 기록
```

### 도어벨 쓰기 최적화

```cpp
// PTX 인라인 어셈블리로 MMIO 쓰기 최적화
asm volatile ("st.mmio.relaxed.sys.global.u32 [%0], %1;"
              :: "l"(sq->db), "r"(new_db) : "memory");
```

`st.mmio`는 CUDA PTX에서 MMIO(Memory-Mapped I/O) 전용 store 명령으로, PCIe BAR 영역의 도어벨 레지스터에 직접 쓴다. `relaxed`는 추가 메모리 배리어 없이 쓰기를 수행한다.

## CQ 처리: 폴링과 Dequeue

### cq_poll() - CID 기반 완료 검색

**파일**: `nvm_parallel_queue.h:379`

기존 NVMe CQ는 head부터 순차적으로 dequeue하지만, BaM에서는 각 스레드가 자신의 CID를 가지고 CQ를 직접 검색한다.

```cpp
inline __device__
uint32_t cq_poll(nvm_queue_t* cq, uint16_t search_cid,
                 uint32_t* loc_, uint32_t* cq_head) {
    while (true) {
        uint32_t head = cq->head.load(simt::memory_order_relaxed);

        for (size_t i = 0; i < cq->qs_minus_1; i++) {
            uint32_t cur_head = head + i;
            // phase tag 계산: wrap 횟수가 홀수면 0, 짝수면 1
            bool search_phase = (~(cur_head >> cq->qs_log2)) & 0x01;
            uint32_t loc = cur_head & cq->qs_minus_1;

            // CQ 엔트리의 DWORD3 (Status + CID) 읽기
            uint32_t cpl_entry = ((nvm_cpl_t*)cq->vaddr)[loc].dword[3];
            uint32_t cid = cpl_entry & 0x0000ffff;       // 하위 16비트: CID
            bool phase = (cpl_entry & 0x00010000) >> 16;  // 비트 16: Phase

            // CID와 phase가 모두 일치하면 완료 발견
            if ((cid == search_cid) && (phase == search_phase)) {
                *cq_head = head;
                *loc_ = cur_head;
                return loc;
            }
            // phase 불일치 → 아직 도착하지 않은 엔트리
            if (phase != search_phase)
                break;
        }
        __nanosleep(ns);  // 지수 백오프
    }
}
```

### cq_dequeue() - 완료 처리 및 CQ 도어벨

**파일**: `nvm_parallel_queue.h:425`

```
cq_dequeue() 흐름:
┌──────────────────────────────────────────────────────┐
│ 1. cq->tail.fetch_add(1)   // 처리할 완료 수 추적   │
│                                                       │
│ 2. pos_locks[pos] 대기 + 획득                        │
│    (이전 같은 위치의 dequeue가 완료될 때까지)        │
│                                                       │
│ 3. head_mark[pos] = LOCKED  // 이 슬롯 처리됨 표시  │
│                                                       │
│ 4. head 이동 시도:                                    │
│    head_lock 획득                                     │
│    head_move_count = move_head_cq()                   │
│    if (head_move_count > 0)                           │
│      SQ head도 업데이트                               │
│      *(cq->db) = new_head   // CQ 도어벨 쓰기       │
│      cq->head = new_head                              │
│    head_lock 해제                                     │
│                                                       │
│ 5. 자신의 위치가 head를 지나갈 때까지 대기            │
│                                                       │
│ 6. pos_locks[pos] = 0       // 위치 락 해제          │
└──────────────────────────────────────────────────────┘
```

### move_head_cq() - CQ Head 이동 + SQ Head 갱신

**파일**: `nvm_parallel_queue.h:83`

```cpp
inline __device__
uint32_t move_head_cq(nvm_queue_t* q, uint32_t cur_head, nvm_queue_t* sq) {
    uint32_t count = 0;
    bool pass = true;

    while (pass) {
        uint32_t loc = (cur_head + count++) & q->qs_minus_1;
        // 연속된 LOCKED(처리완료) 슬롯 수집
        pass = (q->head_mark[loc].val.exchange(UNLOCKED, ...)) == LOCKED;
    }
    count -= 1;

    if (count) {
        // 마지막 CQ 엔트리에서 SQ head 정보 읽기
        uint32_t cpl_entry = ((nvm_cpl_t*)q->vaddr)[loc_].dword[2];
        uint16_t new_sq_head = cpl_entry & 0x0000ffff;  // SQHD 필드

        // SQ의 head를 갱신하고 tickets 업데이트
        uint32_t cur_sq_head = sq->head.load(...);
        for (; loc != new_sq_head; sq_move_count++, loc = (loc+1) & sq->qs_minus_1) {
            sq->tickets[loc].val.fetch_add(1, ...);  // 해당 슬롯 재사용 가능
        }
        sq->head.fetch_add(sq_move_count, simt::memory_order_acq_rel);
    }
    return count;
}
```

## 메모리 순서 보장 분석

### simt::memory_order 사용 패턴

BaM에서는 `simt::atomic`의 다양한 메모리 순서를 전략적으로 사용한다:

| 연산 | memory_order | 이유 |
|------|-------------|------|
| `in_ticket.fetch_add(1)` | `relaxed` | 순서 불필요, 유일한 티켓 할당만 보장 |
| `tickets[pos].load()` | `relaxed` → `acquire` | 먼저 relaxed로 빠르게 확인, 이후 acquire로 가시성 확보 |
| `tail_mark[pos].store(LOCKED)` | `release` | 명령 복사가 완료된 후에야 LOCKED가 보여야 함 |
| `*(sq->db) = new_tail` | `st.mmio.relaxed` | MMIO는 자체적으로 순서 보장 |
| `sq->tail.store(new_tail)` | `release` | 도어벨 후 tail 갱신이 보여야 함 |
| `cid[id].fetch_or(LOCKED)` | `acquire` | CID 획득 시 이전 상태를 올바르게 읽어야 함 |
| `cid[id].store(UNLOCKED)` | `release` | CID 해제 시 관련 데이터 쓰기가 완료되어야 함 |

### thread_scope 사용 패턴

```cpp
simt::thread_scope_device   // GPU 내 모든 스레드에서 가시적 (대부분)
simt::thread_scope_system   // GPU + CPU + NVMe 컨트롤러에서 가시적 (tail_copy, head_copy)
```

`thread_scope_system`은 PCIe를 통해 NVMe 컨트롤러와 공유되는 데이터에 사용된다.

### 티켓 대기 시 이중 루프 패턴

```cpp
// Phase 1: relaxed로 빠르게 폴링
unsigned int ns = 8;
while (tickets[pos].load(simt::memory_order_relaxed) != id) {
    __nanosleep(ns);
    if (ns < 256) ns *= 2;
}

// Phase 2: acquire로 최종 확인 (가시성 보장)
ns = 8;
while (tickets[pos].load(simt::memory_order_acquire) != id) {
    __nanosleep(ns);
    if (ns < 256) ns *= 2;
}
```

이 패턴은 relaxed로 빠르게 대기한 후, acquire로 다른 스레드의 쓰기가 확실히 보이도록 한다.

## 전체 SQ/CQ 동작 타이밍 다이어그램

```
Thread A          Thread B          NVMe Controller
   │                 │                    │
   │ ticket=0        │ ticket=1           │
   │ pos=0           │ pos=1              │
   │                 │                    │
   │ tickets[0]==0 ✓ │ tickets[1]==0 ✓    │
   │ cmd 복사        │ cmd 복사           │
   │ tail_mark[0]=L  │ tail_mark[1]=L     │
   │                 │                    │
   │ tail_lock 획득  │                    │
   │ move_tail:      │                    │
   │  [0]=L → count=1│                   │
   │  [1]=L → count=2│                   │
   │ db_write(2)     │                    │
   │ ─────────────────────────────────>   │
   │ tail=2          │                    │
   │ tail_lock 해제  │                    │  SQ에서 cmd 읽기
   │                 │                    │  플래시에서 데이터 읽기
   │ tickets[0] += 1 │ tickets[1] += 1   │  GPU 메모리에 DMA 쓰기
   │                 │                    │
   │                 │                    │  CQ에 완료 기록
   │                 │                    │  ◄─────────────
   │ cq_poll(cid_A)  │ cq_poll(cid_B)    │
   │  CQ 검색...     │  CQ 검색...       │
   │  cid 매칭!      │  cid 매칭!        │
   │                 │                    │
   │ cq_dequeue      │ cq_dequeue        │
   │  head_mark[x]=L │  head_mark[y]=L   │
   │  head_lock 획득 │                    │
   │  move_head_cq   │                    │
   │  cq_db_write    │                    │
   │  ─────────────────────────────────>  │
   │  SQ head 갱신   │                    │
   │  head_lock 해제 │                    │
   │                 │                    │
   │ put_cid(cid_A)  │ put_cid(cid_B)    │
```

## SQ dequeue (SQ Head 소프트웨어 관리)

### sq_dequeue()

**파일**: `nvm_parallel_queue.h:333`

```cpp
inline __device__
void sq_dequeue(nvm_queue_t* sq, uint16_t pos) {
    // 이 위치의 명령이 컨트롤러에 의해 소비되었음을 표시
    sq->head_mark[pos].val.store(LOCKED, simt::memory_order_relaxed);

    // head 이동 시도
    while (sq->head_mark[pos].val.load(...) == LOCKED) {
        if (sq->head_lock.exchange(LOCKED, ...) != LOCKED) {
            uint32_t cur_head = sq->head.load(...);
            uint32_t head_move_count = move_head_sq(sq, cur_head);

            if (head_move_count) {
                sq->head.store(cur_head + head_move_count, ...);
            }
            sq->head_lock.store(UNLOCKED, ...);
        }
        __nanosleep(ns);
    }
}
```

## 지수 백오프 전략

모든 spin-wait에서 사용되는 공통 패턴:

```cpp
unsigned int ns = 8;        // 시작: 8ns
while (condition) {
    __nanosleep(ns);
    if (ns < 256) {
        ns *= 2;            // 8 → 16 → 32 → 64 → 128 → 256 (최대)
    }
}
```

8ns에서 시작하여 최대 256ns까지 2배씩 증가한다. `__nanosleep()`은 CUDA CC 7.0 이상에서 지원되며, GPU SM(Streaming Multiprocessor)의 다른 warp에 실행 기회를 양보한다.

## 요약

BaM 병렬 큐의 핵심 설계 원칙:

1. **티켓 기반 슬롯 할당**: `in_ticket.fetch_add(1)`로 각 스레드에 고유 슬롯을 비충돌적으로 할당
2. **라운드 추적**: `tickets[]` 배열로 같은 슬롯의 이전 사용이 완료되었는지 확인, 큐 wrap-around 안전성 보장
3. **mark-and-sweep tail/head 이동**: 각 스레드가 자신의 슬롯에 mark를 남기고, 하나의 스레드(락 획득자)가 연속 마크를 수집하여 도어벨에 반영
4. **CID 기반 비순차 완료 처리**: 각 스레드가 자신의 CID로 CQ를 직접 검색, 순서에 무관하게 완료 처리
5. **이중 레벨 메모리 순서**: relaxed로 빠르게 폴링 → acquire로 최종 확인하는 패턴으로 성능과 정확성 모두 확보
6. **MMIO 어셈블리 최적화**: PTX `st.mmio`로 도어벨 쓰기의 오버헤드 최소화
