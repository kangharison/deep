# BaM 소프트웨어 페이지 캐시 메커니즘 상세 분석

## 개요

BaM의 페이지 캐시는 GPU 메모리에 위치하는 소프트웨어 관리 캐시로, NVMe SSD의 데이터를 GPU에서 빠르게 접근할 수 있도록 한다. 수천 개의 GPU 스레드가 동시에 접근하는 환경에서 동작해야 하므로, `simt::atomic`을 기반으로 한 lock-free 상태 머신을 사용한다.

**핵심 파일**: `include/page_cache.h`

## 캐시 아키텍처

```
┌──────────────────────────────────────────────────────────┐
│                    page_cache_d_t (GPU)                    │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  cache_pages[0..n_pages-1]  (메타데이터 배열)        │ │
│  │  ┌──────────┬───────────────────┐                   │ │
│  │  │page_take │ page_translation  │ × n_pages         │ │
│  │  │_lock     │ (global_address)  │                   │ │
│  │  └──────────┴───────────────────┘                   │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  base_addr[0..page_size*n_pages]  (데이터 영역)     │ │
│  │  ┌──────┬──────┬──────┬──────┬─────┐               │ │
│  │  │Page 0│Page 1│Page 2│ ...  │Page N│  (DMA 가능)  │ │
│  │  └──────┴──────┴──────┴──────┴─────┘               │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  prp1[0..n_pages-1]   (PRP1 물리 주소 배열)        │ │
│  │  prp2[0..n_pages-1]   (PRP2 물리 주소 배열)        │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  ranges[0..n_ranges-1]                               │ │
│  │  ┌────────────────────────────┐                     │ │
│  │  │ ranges[r] → pages_t       │ × n_ranges          │ │
│  │  │  └→ data_page_t[page_count]│                     │ │
│  │  │      ├─ state (atomic)     │                     │ │
│  │  │      └─ offset             │                     │ │
│  │  └────────────────────────────┘                     │ │
│  └─────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

## 캐시 상태 머신

### 데이터 페이지 상태 (data_page_t.state)

`state`는 32비트 atomic 변수로, 비트 필드가 다중 의미를 갖는다:

```
 31  30  29  28                              0
┌───┬───┬───┬──────────────────────────────────┐
│ V │ B │ D │        Reference Count            │
└───┴───┴───┴──────────────────────────────────┘
 V = VALID  (0x80000000)  - 데이터가 유효
 B = BUSY   (0x40000000)  - I/O 진행 중
 D = DIRTY  (0x20000000)  - 수정됨 (write-back 필요)
 CNT_MASK = 0x1fffffff    - 29비트 레퍼런스 카운트
```

**정의** (`page_cache.h:57-70`):
```cpp
#define INVALID 0x00000000U      // 유효하지 않음 (V=0, B=0)
#define VALID   0x80000000U      // 유효 (V=1)
#define BUSY    0x40000000U      // 사용 중 (B=1)
#define DIRTY   0x20000000U      // 수정됨 (D=1)
#define CNT_SHIFT (29ULL)
#define CNT_MASK 0x1fffffffU
```

### 4가지 주요 상태

```
 V  B  코드      이름      의미
─── ── ──────── ─────── ─────────────────────────────
 0  0  NV_NB    Invalid  데이터 없음, 새로 로딩 필요
 0  1  NV_B     Loading  다른 스레드가 데이터 로딩 중
 1  0  V_NB     Valid    데이터 유효, 접근 가능
 1  1  V_B      Evicting 교체 진행 중 (write-back 포함 가능)
```

### 상태 전이도

```
                    ┌──────────────┐
                    │   INVALID    │ (초기 상태)
                    │   NV_NB      │
                    │ state = 0x0  │
                    └──────┬───────┘
                           │
                     fetch_or(BUSY)
                     [캐시 미스]
                           │
                    ┌──────▼───────┐
                    │   LOADING    │
                    │   NV_B       │
                    │ BUSY 설정    │
                    └──────┬───────┘
                           │
                  read_data() 완료
                  fetch_xor(DISABLE_BUSY_ENABLE_VALID)
                  (BUSY 해제 + VALID 설정)
                           │
                    ┌──────▼───────┐
          ┌────────│    VALID     │◄──────────────────────┐
          │        │    V_NB      │                       │
          │        │ state = 0x80 │                       │
          │        └──────┬───────┘                       │
          │               │                               │
     fetch_add(cnt)  find_slot() 에서                     │
     [캐시 히트]     교체 대상 선택                        │
          │          fetch_or(BUSY)                        │
          │               │                               │
          │        ┌──────▼───────┐                       │
          │        │  EVICTING    │                       │
          │        │    V_B       │                       │
          │        │ BUSY 설정    │                       │
          │        └──────┬───────┘                       │
          │               │                               │
          │          [dirty?]──Yes──► write_data()         │
          │               │                               │
          │         state.fetch_and(CNT_MASK)              │
          │         (VALID, BUSY, DIRTY 모두 클리어)       │
          │               │                               │
          │        ┌──────▼───────┐                       │
          │        │   INVALID    │                       │
          │        │   NV_NB      │                       │
          │        └──────┬───────┘                       │
          │               │                               │
          │          새 데이터 로딩                        │
          │               │                               │
          │               └───────────────────────────────┘
          │
          └──► fetch_sub(cnt)
               [release_page]
```

## 캐시 라인 할당/해제 알고리즘

### 할당: find_slot()

**파일**: `page_cache.h:1786`

CLOCK-like 알고리즘으로 캐시 슬롯을 할당한다.

```
find_slot() 알고리즘:
┌─────────────────────────────────────────────────────┐
│ 1. page_ticket을 원자적으로 증가 → 후보 페이지 선택 │
│    page = page_ticket.fetch_add(1) % n_pages        │
│                                                      │
│ 2. cache_pages[page].page_take_lock 확인:           │
│                                                      │
│    ├── FREE (=2):                                    │
│    │   ├── CAS(FREE → LOCKED) 성공?                  │
│    │   │   ├── page_translation = global_address     │
│    │   │   ├── store(UNLOCKED)                       │
│    │   │   └── return page (성공!)                   │
│    │   └── 실패 → 재시도                             │
│    │                                                  │
│    ├── UNLOCKED (=0):                                │
│    │   ├── CAS(UNLOCKED → LOCKED) 성공?              │
│    │   │   ├── 이전 매핑의 state 확인                │
│    │   │   │   ├── ref_count == 0 && !BUSY?          │
│    │   │   │   │   ├── fetch_or(BUSY) 성공?          │
│    │   │   │   │   │   ├── dirty? → write_data()     │
│    │   │   │   │   │   ├── 이전 state 클리어         │
│    │   │   │   │   │   ├── page_translation 갱신     │
│    │   │   │   │   │   └── return page (성공!)       │
│    │   │   │   │   └── BUSY 해제 → 재시도            │
│    │   │   │   └── ref_count > 0 → 사용중, 재시도    │
│    │   │   └── store(UNLOCKED)                       │
│    │   └── CAS 실패 → 재시도                         │
│    │                                                  │
│    └── LOCKED (=1):                                  │
│        └── 다른 스레드가 사용 중 → 재시도             │
│                                                      │
│ 3. 실패 시 1번으로 (다음 후보 페이지)                │
└─────────────────────────────────────────────────────┘
```

**cache_page_t.page_take_lock 3가지 상태**:
| 값 | 이름 | 의미 |
|----|------|------|
| 0 | UNLOCKED | 할당된 슬롯, 접근 가능 |
| 1 | LOCKED | 다른 스레드가 메타데이터 수정 중 |
| 2 | FREE | 미할당 슬롯 (초기 상태) |

### 해제: release_page()

**파일**: `page_cache.h:1083`

```cpp
__device__
void release_page(const size_t pg) const {
    pages[pg].state.fetch_sub(1, simt::memory_order_release);
}

__device__
void release_page(const size_t pg, const uint32_t count) const {
    pages[pg].state.fetch_sub(count, simt::memory_order_release);
}
```

레퍼런스 카운트를 원자적으로 감소시킨다. 카운트가 0이 되면 `find_slot()`에서 교체 대상으로 선택될 수 있다.

## 레퍼런스 카운팅 메커니즘

### 카운트 증가

`acquire_page()` (`page_cache.h:1122`) 진입 시:
```cpp
read_state = pages[index].state.fetch_add(count, simt::memory_order_acquire);
```

`count`는 warp 내에서 동일 페이지를 접근하는 스레드 수(`__popc(eq_mask)`)이다. 이를 통해 여러 스레드의 레퍼런스를 한 번의 원자적 연산으로 증가시킨다.

### 카운트 감소

`release_page()` 호출 시:
```cpp
pages[index].state.fetch_sub(count, simt::memory_order_release);
```

마찬가지로 master 스레드가 합쳐진 카운트를 한 번에 감소시킨다.

### 카운트와 상태 비트의 공존

```
state 값 해석 예:
  0x80000003  →  VALID, ref_count = 3 (3개 스레드가 사용 중)
  0xA0000001  →  VALID + DIRTY, ref_count = 1
  0x40000000  →  BUSY, ref_count = 0 (로딩 중)
  0x00000000  →  INVALID, ref_count = 0 (교체 가능)
```

교체 가능 조건: `(state & CNT_MASK) == 0 && (state & BUSY) == 0`

## I/O 합병 (Coalescing) 로직

### Warp-level 합병

**파일**: `page_cache.h:1332`의 `coalesce_page()`

GPU의 warp(32 스레드)가 동시에 데이터에 접근할 때, 같은 캐시 페이지를 접근하는 스레드들의 요청을 합친다.

```
합병 과정:
┌──────────────────────────────────────────────────┐
│ Warp (32 스레드)                                  │
│                                                   │
│ Thread  0: page[100] ─┐                          │
│ Thread  1: page[200]  │                          │
│ Thread  2: page[100] ─┤                          │
│ Thread  3: page[300]  │                          │
│ Thread  4: page[100] ─┤ eq_mask = 0b10000010101  │
│ ...                    │ master = thread 0        │
│ Thread 31: page[100] ─┘ count = 4                │
│                                                   │
│ __match_any_sync(mask, gaddr):                    │
│   같은 gaddr를 가진 스레드들의 비트마스크 반환    │
│                                                   │
│ __ffs(eq_mask) - 1:                               │
│   가장 낮은 비트의 스레드를 master로 선출         │
│                                                   │
│ __popc(eq_mask):                                  │
│   합쳐진 스레드 수 = ref count 증가량            │
│                                                   │
│ master만 acquire_page(page, count=4) 실행         │
│                                                   │
│ __shfl_sync(eq_mask, base_master, master):         │
│   master의 결과를 모든 스레드에 브로드캐스트       │
└──────────────────────────────────────────────────┘
```

**효과**:
- 캐시 히트 시: 1번의 `fetch_add`로 4개 스레드의 ref count 처리
- 캐시 미스 시: 1번의 NVMe I/O로 4개 스레드의 데이터 요구 처리
- Dirty 합병: `__any_sync(eq_mask, write)` — 그룹 내 1개라도 쓰기면 dirty 마킹

## 다중 컨트롤러 스트라이핑/복제

### 데이터 분산 모드

**파일**: `page_cache.h:42`

```cpp
enum data_dist_t { REPLICATE = 0, STRIPE = 1 };
```

### 스트라이핑 (STRIPE)

**파일**: `page_cache.h:558-582`

```cpp
__device__
uint64_t get_backing_page_(uint64_t page_start, size_t page_offset,
                           uint64_t n_ctrls, data_dist_t dist) {
    if (dist == STRIPE) {
        return page_start + page_offset / n_ctrls;  // 페이지를 컨트롤러 수로 나눔
    }
}

__device__
uint64_t get_backing_ctrl_(size_t page_offset, uint64_t n_ctrls, data_dist_t dist) {
    if (dist == STRIPE) {
        return page_offset % n_ctrls;  // 페이지 번호로 컨트롤러 선택
    }
}
```

스트라이핑에서 페이지-컨트롤러 매핑:
```
페이지 0 → 컨트롤러 0
페이지 1 → 컨트롤러 1
페이지 2 → 컨트롤러 2 (n_ctrls=3일 때)
페이지 3 → 컨트롤러 0
페이지 4 → 컨트롤러 1
...
```

### 복제 (REPLICATE)

```cpp
if (dist == REPLICATE) {
    page = page_start + page_offset;    // 모든 컨트롤러에 같은 페이지
    ctrl = ALL_CTRLS;                    // 0xffffffffffffffff
}
```

복제 모드에서 읽기 시, `ctrl_counter`를 라운드 로빈으로 사용하여 컨트롤러를 선택:
```cpp
if (ctrl == ALL_CTRLS)
    ctrl = cache.ctrl_counter->fetch_add(1, ...) % cache.n_ctrls;
```

복제 모드에서 쓰기(flush/eviction) 시, 모든 컨트롤러에 write-back:
```cpp
if (ctrl == ALL_CTRLS) {
    for (ctrl = 0; ctrl < pc->n_ctrls; ctrl++) {
        write_data(pc, (c->d_qps)+queue, index*n_blocks, n_blocks, page);
    }
}
```

## 핵심 함수별 상세 분석

### page_cache_d_t::get_cache_page()

```cpp
__device__
cache_page_t* get_cache_page(const uint32_t page) const {
    return &cache_pages[page];
}
```

### range_d_t<T>::get_cache_page_addr()

```cpp
__device__
uint64_t get_cache_page_addr(const uint32_t page_trans) const {
    return (uint64_t)(cache.base_addr + (page_trans * cache.page_size));
}
```

`page_trans`(캐시 페이지 인덱스)를 GPU 메모리 주소로 변환한다.

### range_d_t<T>::mark_page_dirty()

```cpp
__device__
void mark_page_dirty(const size_t index) {
    pages[index].state.fetch_or(DIRTY, simt::memory_order_relaxed);
}
```

### __flush 커널

**파일**: `page_cache.h:584`

전체 캐시를 flush하는 GPU 커널:
```cpp
__global__
void __flush(page_cache_d_t* pc) {
    uint64_t page = threadIdx.x + blockIdx.x * blockDim.x;
    if (page < pc->n_pages) {
        // 이전 매핑 정보 조회
        previous_global_address = pc->cache_pages[page].page_translation;
        previous_range = previous_global_address & pc->n_ranges_mask;
        previous_address = previous_global_address >> pc->n_ranges_bits;

        expected_state = pc->ranges[previous_range][previous_address].state.load(...);
        if (expected_state & DIRTY) {
            // dirty 페이지 write-back
            write_data(pc, qp, index*n_blocks_per_page, n_blocks_per_page, page);
            // dirty 비트 클리어
            pc->ranges[previous_range][previous_address].state.fetch_and(~DIRTY);
        }
    }
}
```

## 글로벌 주소 인코딩

캐시는 여러 range의 페이지를 관리하므로, range_id와 페이지 번호를 하나의 64비트 값으로 인코딩한다:

```cpp
uint64_t get_global_address(const size_t page) const {
    return (page << cache.n_ranges_bits) | range_id;
}
```

```
 63                n_ranges_bits    0
┌────────────────────┬──────────────┐
│     page number    │  range_id    │
└────────────────────┴──────────────┘
```

디코딩:
```cpp
uint64_t previous_range = previous_global_address & n_ranges_mask;
uint64_t previous_address = previous_global_address >> n_ranges_bits;
```

## TLB (Translation Lookaside Buffer) 최적화

**파일**: `page_cache.h:158-343`

`tlb` 구조체는 최근 접근한 페이지를 추가로 캐싱하는 소프트웨어 TLB이다.

```cpp
template<typename T, size_t n = 32, simt::thread_scope _scope = simt::thread_scope_device>
struct tlb {
    tlb_entry<_scope> entries[n];    // n개 엔트리
    array_d_t<T>* array;
};
```

`tlb_entry`:
```cpp
struct tlb_entry {
    uint64_t global_id;                                    // 캐시된 글로벌 주소
    simt::atomic<uint32_t, _scope> state;                 // VALID 비트 + ref count
    data_page_t* page;                                     // 데이터 페이지 포인터
};
```

TLB 조회/삽입 (`tlb::acquire`, line 252):
1. `gid % n`으로 TLB 엔트리 인덱스 계산
2. `state.fetch_or(VALID_)`로 lock 획득
3. 엔트리의 `global_id`가 일치하면 TLB 히트 → ref count 증가 + 바로 반환
4. 불일치 또는 빈 엔트리면 TLB 미스 → `array->acquire_page_()`로 풀 캐시에서 가져옴 → TLB에 등록

## bam_ptr와 bam_ptr_tlb

### bam_ptr (TLB 없는 단순 포인터)

**파일**: `page_cache.h:415-485`

```cpp
template<typename T>
struct bam_ptr {
    data_page_t* page;
    array_d_t<T>* array;
    size_t start, end;
    T* addr;

    T operator[](const size_t i) {
        if ((i < start) || (i >= end))
            update_page(i);              // 페이지 경계 넘으면 새 페이지 획득
        return addr[i - start];
    }
};
```

### bam_ptr_tlb (TLB 사용 포인터)

**파일**: `page_cache.h:347-411`

TLB를 통해 최근 접근 페이지를 빠르게 재접근할 수 있다.

## 요약

BaM 페이지 캐시의 핵심 특성:
1. **GPU-native 설계**: `simt::atomic` 기반으로 수천 GPU 스레드의 동시 접근 지원
2. **결합된 상태 비트 + ref count**: 32비트 원자 변수 하나에 VALID/BUSY/DIRTY 플래그와 29비트 참조 카운트를 결합
3. **Warp-level I/O coalescing**: `__match_any_sync()`으로 같은 페이지 접근을 합쳐 I/O 횟수 최소화
4. **CLOCK-like 교체 알고리즘**: `page_ticket` 기반 라운드 로빈으로 교체 대상 선택, ref count 0이고 busy가 아닌 페이지만 교체
5. **다중 컨트롤러 지원**: STRIPE/REPLICATE 모드로 여러 NVMe SSD에 데이터 분산 또는 복제
6. **Dirty write-back**: 교체 시 dirty 페이지를 NVMe에 write-back 후 교체
