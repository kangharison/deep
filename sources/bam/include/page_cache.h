/* page_cache.h - BaM GPU 페이지 캐시 핵심 구현
 *
 * BaM(Big Accelerator Memory)의 가장 중요한 파일이다.
 * GPU 메모리에 NVMe SSD 데이터를 캐싱하는 소프트웨어 관리 페이지 캐시를 구현한다.
 *
 * 핵심 구조:
 * - page_cache_d_t: GPU 디바이스에 상주하는 캐시 메타데이터 (캐시 페이지 배열, PRP 리스트 등)
 * - page_cache_t: 호스트에서 캐시를 초기화하고 관리하는 래퍼
 * - range_d_t<T>: SSD의 특정 데이터 범위를 나타내며, 캐시 접근의 단위
 * - array_d_t<T>: 여러 range를 묶어 하나의 논리 배열로 보이게 하는 추상화
 * - data_page_t: 각 SSD 페이지의 상태를 추적하는 구조체 (INVALID/VALID/BUSY/DIRTY + 참조 카운트)
 * - cache_page_t: 캐시 슬롯 메타데이터 (FREE/LOCKED/UNLOCKED + 페이지 매핑 정보)
 *
 * 페이지 상태 머신 (data_page_t.state의 상위 3비트):
 *   비트31(VALID) | 비트30(BUSY) | 비트29(DIRTY) | 비트[28:0](참조 카운트)
 *
 *   INVALID(00): 캐시에 없음 → BUSY(01): 로드 중 → VALID(10): 사용 가능
 *   VALID+DIRTY: 수정됨, eviction 시 write-back 필요
 *   참조 카운트 > 0이면 eviction 불가 (사용 중)
 *
 * 캐시 교체 정책: Clock/FIFO 기반 (page_ticket으로 순환 탐색)
 *
 * Warp-level coalescing: 같은 페이지를 접근하는 warp 내 스레드들을 __match_any_sync로 그룹화하여
 *   한 번의 acquire/release로 여러 스레드를 처리한다.
 */
#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif
#ifndef __forceinline__
#define __forceinline__ inline
#endif


#include "util.h"
#include "host_util.h"
#include "nvm_types.h"
#include "nvm_util.h"
#include "buffer.h"
#include "ctrl.h"
#include <iostream>
#include "nvm_parallel_queue.h"
#include "nvm_cmd.h"

/* FREE(2): cache_page_t의 page_take_lock 초기값. 아직 어떤 SSD 페이지에도 매핑되지 않은 빈 캐시 슬롯을 나타낸다. */
#define FREE 2
// enum locality {HIGH_SPATIAL, LOW_SPATIAL, MEDIUM_SPATIAL};
// template <typname T>
// stryct array_t {
//   range_t<T>* ranges;
//   uint32_t n_ranges;

//   void add_range(start_idx, end_idx, locality l )
//   {
//   ranges.push_back(new range(star))
//   }
// }

// enum page_state {USE = 1U, USE_DIRTY = ((1U << 31) | 1), VALID_DIRTY = (1U << 31),
//     VALID = 0U, INVALID = (UINT_MAX & 0x7fffffff),
//     BUSY = ((UINT_MAX & 0x7fffffff)-1)};

/* data_dist_t: 다중 NVMe 컨트롤러 사용 시 데이터 분배 방식.
 * REPLICATE: 모든 컨트롤러에 동일 데이터 복제 (읽기 성능 향상, 쓰기 시 모든 컨트롤러에 기록)
 * STRIPE: 페이지 단위로 컨트롤러에 분산 (페이지 오프셋 % n_ctrls로 컨트롤러 선택)
 */
enum data_dist_t {REPLICATE = 0, STRIPE = 1};

// #define USE (1ULL)
// #define VALID_DIRTY (1ULL << 31)
// #define USE_DIRTY (VALID_DIRTY | USE)
// #define VALID (0ULL)
// #define INVALID (0xffffffffULL)
// #define BUSY ((0xffffffffULL)-1)

/* ALL_CTRLS: REPLICATE 분배 시 모든 컨트롤러에 기록함을 나타내는 특수값 */
#define ALL_CTRLS 0xffffffffffffffff

//broken
#define VALID_DIRTY (1ULL << 31)
#define USE_DIRTY (VALID_DIRTY | USE)

/* === data_page_t.state 비트 필드 정의 ===
 * 32비트 상태 워드의 구조: [VALID(1)][BUSY(1)][DIRTY(1)][참조카운트(29비트)]
 *
 * INVALID(0x00000000): 캐시에 없음, 참조 카운트 0
 * VALID(0x80000000):   비트31 설정 - 데이터가 캐시에 유효하게 로드됨
 * BUSY(0x40000000):    비트30 설정 - SSD에서 데이터 로드 중 (다른 스레드는 대기)
 * DIRTY(0x20000000):   비트29 설정 - 수정됨, eviction 시 write-back 필요
 * CNT_MASK(0x1fffffff): 하위 29비트 - 현재 이 페이지를 사용 중인 스레드 수 (참조 카운트)
 */
#define INVALID 0x00000000U
#define VALID   0x80000000U
#define BUSY    0x40000000U
#define DIRTY   0x20000000U
#define CNT_SHIFT (29ULL)       // 상태 비트 시작 위치 (상위 3비트가 상태)
#define CNT_MASK 0x1fffffffU    // 참조 카운트 마스크 (하위 29비트)
#define VALID_MASK 0x7
#define BUSY_MASK 0xb
/* DISABLE_BUSY_ENABLE_VALID: BUSY 비트를 끄고 VALID 비트를 켜는 XOR 마스크.
 * 데이터 로드가 완료되면 state ^= 0xc0000000으로 BUSY→VALID 전환한다. */
#define DISABLE_BUSY_ENABLE_VALID 0xc0000000U
/* DISABLE_BUSY_MASK: BUSY 비트만 끄는 AND 마스크 (state &= 0xbfffffff) */
#define DISABLE_BUSY_MASK 0xbfffffffU
/* 상태 머신의 4가지 상태 (상위 2비트: VALID, BUSY)
 * NV_NB(00): Not Valid, Not Busy = INVALID (캐시 미스, 로드 필요)
 * NV_B (01): Not Valid, Busy     = 다른 스레드가 로드 중 (대기)
 * V_NB (10): Valid, Not Busy     = 캐시 히트 (즉시 사용 가능)
 * V_B  (11): Valid, Busy         = 유효하지만 동시 로드 경합 (대기)
 */
#define NV_NB 0x00U
#define NV_B 0x01U
#define V_NB 0x02U
#define V_B 0x03U


struct page_cache_t;

struct page_cache_d_t;

//typedef padded_struct_pc* page_states_t;


template <typename T>
struct range_t;

template<typename T>
struct array_d_t;

template <typename T>
struct range_d_t;

/*struct data_page_t {
  simt::atomic<uint64_t, simt::thread_scope_device>  state; //state
  //
  uint32_t offset;

  };
*/
/* data_page_t: SSD 데이터 페이지의 캐시 상태를 추적하는 구조체.
 * 각 SSD 페이지(range 내)마다 하나씩 존재한다.
 * 32바이트 정렬: GPU 캐시라인(128B) 내에 4개가 들어가며, false sharing을 줄인다.
 */
typedef struct __align__(32) {
    simt::atomic<uint32_t, simt::thread_scope_device>  state; // 페이지 상태 비트필드: [VALID|BUSY|DIRTY|참조카운트(29비트)]
                                                              //
    uint32_t offset; // 이 SSD 페이지가 매핑된 캐시 슬롯 번호 (cache_pages 배열의 인덱스)
    //uint8_t pad[32-4-4];

} __attribute__((aligned (32))) data_page_t;

/* pages_t: data_page_t 배열 포인터 타입. 각 range가 자신의 pages 배열을 가진다. */
typedef data_page_t* pages_t;

/* returned_cache_page_t<T>: 캐시 페이지 접근 결과를 반환하는 구조체.
 * get_raw() 함수가 반환하며, 캐시된 데이터 페이지의 주소, 크기, 오프셋 정보를 담는다.
 */
template<typename T>
struct returned_cache_page_t {
    T* addr;          // 캐시 페이지의 GPU 메모리 주소
    uint32_t size;    // 페이지 내 T 타입 원소 개수
    uint32_t offset;  // 요청된 인덱스의 페이지 내 오프셋 (T 단위)

    /* 읽기 전용 인덱스 연산자: 범위를 벗어나면 0을 반환한다 */
    T operator[](size_t i) const {
        if (i < size)
            return addr[i];
        else
            return 0;
    }

    /* 읽기/쓰기 인덱스 연산자: 범위를 벗어나면 첫 원소 참조를 반환한다 (안전 장치) */
    T& operator[](size_t i) {
        if (i < size)
            return addr[i];
        else
            return addr[0];
    }
};
/* TLB 메모리 위치 상수: TLB 엔트리를 어디에 저장할지 결정한다 */
#define THREAD_ 0  // 스레드 로컬 (레지스터)
#define SHARED_ 1  // 공유 메모리 (블록 내 공유)
#define GLOBAL_ 2  // 전역 메모리

/* TID: 블록 내 3차원 스레드 인덱스를 1차원으로 변환한다 */
#define TID ( (threadIdx.x + blockDim.x * (threadIdx.y + blockDim.y * threadIdx.z)))

/* BLKSIZE: 블록 내 전체 스레드 수를 계산한다 */
#define BLKSIZE ( (blockDim.x * blockDim.y * blockDim.z) )

/* SYNC: 메모리 위치에 따라 블록 동기화를 수행한다. THREAD_ 스코프이면 동기화하지 않는다. */
#ifdef __CUDACC__
#define SYNC (loc != THREAD_ ? __syncthreads() : (void)0)
#else
#define SYNC (void)0
#endif




/* === TLB용 상태 비트 (tlb_entry.state) ===
 * TLB 엔트리 자체의 잠금/유효 상태를 관리한다.
 * VALID_ 비트: TLB 엔트리가 잠겨있음 (다른 스레드 접근 차단)
 * 하위 30비트: 참조 카운트 (이 TLB 엔트리를 사용 중인 스레드 수)
 */
#define INVALID_ 0x00000000
#define VALID_ 0x80000000
#define BUSY_ 0x40000000
#define CNT_MASK_ 0x3fffffff
#define INVALID_MASK_ 0x7fffffff
#define DISABLE_BUSY_MASK_ 0xbfffffff

/* tlb_entry: 소프트웨어 TLB의 한 엔트리. SSD 페이지 → 캐시 슬롯 매핑을 캐싱한다.
 * warp 내 스레드들이 반복 접근하는 페이지의 캐시 주소를 빠르게 찾기 위해 사용한다.
 * _scope: atomic 연산의 가시성 범위 (보통 thread_scope_device)
 */
template<simt::thread_scope _scope = simt::thread_scope_device>
struct tlb_entry {
    uint64_t global_id;                          // 이 엔트리가 매핑하는 SSD 페이지의 전역 ID
    simt::atomic<uint32_t, _scope> state;        // 잠금 + 참조 카운트: [VALID_(잠금)|참조카운트(30비트)]
    data_page_t* page = nullptr;                 // 대응하는 data_page_t 포인터 (캐시 상태 추적)

    __forceinline__
    __host__ __device__
    tlb_entry() { init(); }

    /* init: TLB 엔트리를 초기 상태로 리셋한다 */
    __forceinline__
    __host__ __device__
    void init() {
        global_id = 0;
        state.store(0, simt::memory_order_relaxed);
        page = nullptr;
    }

    /* release(count): 참조 카운트를 count만큼 감소시킨다. 여러 스레드가 동시에 해제할 때 사용. */
    __forceinline__
    __device__
    void release(const uint32_t count) {
//		    if (global_id == 515920192)
//			printf("--(2)st: %llx\tcount: %llu\n", (unsigned long long) state.load(simt::memory_order_relaxed), (unsigned long long) count);

        state.fetch_sub(count, simt::memory_order_release); }

    /* release(): 참조 카운트를 1 감소시킨다. page가 있으면 data_page_t의 state를 직접 감소시킨다. */
    __forceinline__
    __device__
    void release() { if (page != nullptr)  {
//		    if (global_id == 515920192)
//			printf("--(1)st: %llx\tcount: %llu\n", (unsigned long long) state.load(simt::memory_order_relaxed), (unsigned long long) 1);

            page->state.fetch_sub(1, simt::memory_order_release); }}



};




/* tlb: 소프트웨어 TLB (Translation Lookaside Buffer).
 * GPU 블록 또는 warp 단위로 최근 접근한 SSD 페이지의 캐시 주소를 캐싱한다.
 * n: TLB 엔트리 수 (기본 32, 보통 warp 크기와 동일)
 * _scope: atomic 가시성 범위
 * loc: TLB 저장 위치 (THREAD_, SHARED_, GLOBAL_)
 *
 * 동작 원리:
 * - 페이지 전역 ID를 n으로 모듈러하여 TLB 엔트리를 결정한다 (직접 매핑)
 * - 히트: 엔트리의 global_id가 일치하면 참조 카운트를 증가시키고 캐시 주소를 반환한다
 * - 미스: 기존 엔트리를 해제하고, page_cache에서 새 페이지를 acquire하여 TLB를 갱신한다
 * - warp 내 같은 페이지 접근은 __match_any_sync로 그룹화하여 마스터 스레드만 TLB를 조작한다
 */
template<typename T, size_t n = 32, simt::thread_scope _scope = simt::thread_scope_device, size_t loc = GLOBAL_>
struct tlb {
    tlb_entry<_scope> entries[n]; // TLB 엔트리 배열
    array_d_t<T>* array = nullptr; // 소속 배열 포인터 (page acquire 시 사용)

    __forceinline__
    __host__ __device__
    tlb() {}
/*
  __forceinline__
  __device__
  tlb(array_d_t<T>* a) { init(a); }
*/
    /* init: TLB를 초기화한다. 블록 내 모든 스레드가 협력하여 엔트리를 리셋한다. */
    __forceinline__
    __device__
    void init(array_d_t<T>* a) {
        if (n) {
            size_t tid = TID;
            if (tid == 0)
                array = a; // 스레드 0이 배열 포인터를 설정한다
            // 블록 내 스레드들이 stride 패턴으로 엔트리를 초기화한다
            for (; tid < n; tid+=BLKSIZE)
                entries[tid].init();
        }

    }

    /* fini: TLB를 정리한다. 모든 엔트리의 참조 카운트를 해제하여 캐시 페이지가 eviction 가능하게 한다. */
    __forceinline__
    __device__
    void fini() {
        if (n) {
            size_t tid = TID;
            for (; tid < n; tid+=BLKSIZE)
                entries[tid].release(); // 각 엔트리의 data_page_t 참조 카운트를 감소시킨다
        }
    }

    __forceinline__
    __device__
    ~tlb() {  }

    /* acquire: TLB를 통해 SSD 페이지의 캐시 주소를 획득한다.
     * warp 내 같은 gid를 접근하는 스레드들을 __match_any_sync로 그룹화하여
     * 마스터 스레드만 TLB를 조작하고, 결과를 __shfl_sync로 브로드캐스트한다.
     *
     * i: 배열 내 원소 인덱스
     * gid: 페이지의 전역 고유 ID (range_id + 페이지 번호로 계산)
     * start/end: [출력] 이 캐시 페이지가 커버하는 원소 범위
     * range: 소속 range 포인터
     * page_: range 내 페이지 번호
     */
    __forceinline__
    __device__
    T* acquire(const size_t i, const size_t gid, size_t& start, size_t& end, range_d_t<T>* range, const size_t page_) {

        uint32_t lane = lane_id();              // warp 내 레인 ID
        size_t ent = gid % n;                   // 직접 매핑: gid를 TLB 크기로 모듈러
        tlb_entry<_scope>* entry = entries + ent; // 대상 TLB 엔트리
        uint32_t mask = __activemask();         // 현재 활성 스레드 마스크
        // 같은 gid를 접근하는 스레드들을 그룹화한다
        uint32_t eq_mask = __match_any_sync(mask, gid);
        // 같은 TLB 인스턴스를 사용하는 스레드만 필터링한다
        eq_mask &= __match_any_sync(mask, (uint64_t)this);
        uint32_t master = __ffs(eq_mask) - 1;   // 그룹의 마스터 스레드 (가장 낮은 레인)
        uint32_t count = __popc(eq_mask);        // 그룹 내 스레드 수 (참조 카운트 증가량)
        uint64_t base_master, base;
        if (lane == master) {
            // 마스터 스레드만 TLB 엔트리를 조작한다
            uint64_t c = 0;
            bool cont = false;
            uint32_t st;
            do {

                // TLB 엔트리를 잠근다 (VALID_ 비트를 설정하여 배타적 접근)
                do {
                    st = entry->state.fetch_or(VALID_, simt::memory_order_acquire);
                    if ((st & VALID_) == 0)
                        break; // 잠금 성공: 이전에 잠기지 않았음
                    __nanosleep(100); // 잠금 실패: 다른 스레드가 사용 중, 대기
                } while (true);

                // TLB 히트: 페이지가 있고 gid가 일치하면 참조 카운트만 증가시킨다
                if ((entry->page != nullptr) && (gid == entry->global_id)) {
//		    if (gid == 515920192)
//			printf("++(1)st: %llx\tst&Val: %llx\tcount: %llu\n", (unsigned long long) st, (unsigned long long) (st & VALID_), (unsigned long long) count);

                    st += count; // 참조 카운트를 그룹 스레드 수만큼 증가시킨다

                    // 캐시 페이지의 GPU 메모리 주소를 계산한다
                    base_master = (uint64_t) range->get_cache_page_addr(entry->page->offset);

                    entry->state.store(st, simt::memory_order_release); // 잠금 해제 + 새 참조 카운트 저장
                    break;
                }
                // TLB 미스: 페이지가 없거나 참조 카운트가 0 (이전 매핑이 해제 가능)
                else if(((entry->page == nullptr)) || (((st & 0x3fffffff) == 0))) {
//		    if (gid == 515920192)
//			printf("++(2)st: %llx\tst&Val: %llx\tVal: %llx\tcount: %llu\n", (unsigned long long) st, (unsigned long long) (st & VALID_), (unsigned long long) VALID_, (unsigned long long) count);
//
                    // 이전 매핑이 있으면 참조 카운트를 해제한다
                    if (entry->page != nullptr)
                        entry->page->state.fetch_sub(1, simt::memory_order_release);
                    // page_cache에서 새 페이지를 acquire한다 (캐시 미스 → SSD 읽기 발생 가능)
                    data_page_t* page = nullptr;
                    base_master = (uint64_t) array->acquire_page_(i, page, start, end, range, page_);
                    if (((uint64_t) page == 0xffffffffffffffff) || (page == nullptr))
                        printf("failure\n");
                    // TLB 엔트리를 새 페이지로 갱신한다
                    entry->page = page;
                    entry->global_id = gid;
                    st += count; // 참조 카운트 설정
                    entry->state.store(st, simt::memory_order_release); // 잠금 해제
                    break;

                }
                else {
                    // 충돌: 다른 gid가 매핑되어 있고 아직 참조 중이므로 대기한다
                    if (++c % 100000 == 0)
                        printf("c: %llu\ttid: %llu\twanted_gid: %llu\tgot_gid: %llu\tst: %llx\tst&0x7: %llx\n", (unsigned long long) c, (unsigned long long) (TID), (unsigned long long) gid, (unsigned long long) entry->global_id, (unsigned long long) st, (unsigned long long) (st & 0x7fffffff));
                    entry->state.store(st, simt::memory_order_relaxed); // 잠금 해제하고 재시도
                    __nanosleep(100);

                }

            } while(true);


        }

        // 마스터 스레드의 결과(캐시 주소)를 그룹 내 모든 스레드에 브로드캐스트한다
        base_master = __shfl_sync(eq_mask,  base_master, master);

        return (T*) base_master;

    }

    /* release: TLB 엔트리의 참조 카운트를 감소시킨다.
     * 같은 gid를 가진 스레드들을 그룹화하여 마스터만 한 번에 count만큼 감소시킨다.
     */
    __forceinline__
    __device__
    void release(const size_t gid) {
        //size_t gid = array->get_page_gid(i);
        uint32_t lane = lane_id();
        uint32_t mask = __activemask();
        uint32_t eq_mask = __match_any_sync(mask, gid);
        eq_mask &= __match_any_sync(mask, (uint64_t)this);
        uint32_t master = __ffs(eq_mask) - 1;
        uint32_t count = __popc(eq_mask);

        size_t ent = gid % n;
        tlb_entry<_scope>* entry = entries + ent;
        if (lane == master)
            entry->release(count);
        __syncwarp(eq_mask);

    }

};

/* bam_ptr_tlb: TLB를 사용하는 BaM 스마트 포인터.
 * 인덱스 접근 시 현재 페이지 범위를 벗어나면 TLB를 통해 새 페이지를 획득한다.
 * TLB 덕분에 같은 페이지를 반복 접근할 때 page_cache lookup을 건너뛸 수 있다.
 */
template<typename T, size_t n = 32, simt::thread_scope _scope = simt::thread_scope_device, size_t loc = GLOBAL_>
struct bam_ptr_tlb {
    tlb<T,n,_scope,loc>* tlb_ = nullptr; // 소속 TLB 인스턴스
    array_d_t<T>* array = nullptr;       // 소속 배열
    range_d_t<T>* range;                 // 현재 페이지의 range
    size_t page;                         // 현재 range 내 페이지 번호
    size_t start = 0;                    // 현재 캐시 페이지가 커버하는 원소 시작 인덱스
    size_t end = 0;                      // 현재 캐시 페이지가 커버하는 원소 끝 인덱스 (배타적)
    size_t gid = 0;                      // 현재 페이지의 전역 ID
    T* addr = nullptr;                   // 현재 캐시 페이지의 GPU 메모리 주소

    __forceinline__
    __host__ __device__
    bam_ptr_tlb(array_d_t<T>* a, tlb<T,n,_scope,loc>* t) { init(a, t); }

    __forceinline__
    __device__
    ~bam_ptr_tlb() { fini(); }

    __forceinline__
    __host__ __device__
    void init(array_d_t<T>* a, tlb<T,n,_scope,loc>* t) { array = a; tlb_ = t; }

    __forceinline__
    __device__
    void fini(void) {
        if (addr) {

            tlb_->release(gid);
            addr = nullptr;
        }

    }

    __forceinline__
    __device__
    void update_page(const size_t i) {
        ////printf("++++acquire: i: %llu\tpage: %llu\tstart: %llu\tend: %llu\trange: %llu\n",
//            (unsigned long long) i, (unsigned long long) page, (unsigned long long) start, (unsigned long long) end, (unsigned long long) range_id);
        fini(); //destructor
        array->get_page_gid(i, range, page, gid);
        addr = (T*) tlb_->acquire(i, gid, start, end, range, page);
//        //printf("----acquire: i: %llu\tpage: %llu\tstart: %llu\tend: %llu\trange: %llu\n",
//            (unsigned long long) i, (unsigned long long) page, (unsigned long long) start, (unsigned long long) end, (unsigned long long) range_id);
    }

    __forceinline__
    __device__
    T operator[](const size_t i) const {
        if ((i < start) || (i >= end)) {
            update_page(i);
        }
        return addr[i-start];
    }

    __forceinline__
    __device__
    T& operator[](const size_t i) {
        if ((i < start) || (i >= end)) {
            update_page(i);
            range->mark_page_dirty(page);
        }
        return addr[i-start];
    }
};


/* bam_ptr: TLB 없이 직접 page_cache에 접근하는 BaM 스마트 포인터.
 * 인덱스 접근 시 현재 페이지 범위를 벗어나면 page_cache에서 새 페이지를 acquire한다.
 * bam_ptr_tlb보다 단순하지만, 매번 page_cache를 조회하므로 반복 접근 시 오버헤드가 있다.
 * 쓰기 접근 시 DIRTY 비트를 자동으로 설정한다.
 */
template<typename T>
struct bam_ptr {
    data_page_t* page = nullptr;     // 현재 사용 중인 data_page_t (참조 카운트 관리용)
    array_d_t<T>* array = nullptr;   // 소속 배열
    size_t start = 0;                // 현재 페이지가 커버하는 시작 인덱스
    size_t end = 0;                  // 현재 페이지가 커버하는 끝 인덱스 (배타적)
    int64_t range_id = -1;           // 현재 페이지의 range ID
    T* addr = nullptr;               // 현재 캐시 페이지의 GPU 메모리 주소

    __forceinline__
    __host__ __device__
    bam_ptr(array_d_t<T>* a) { init(a); }

    __forceinline__
    __host__ __device__
    ~bam_ptr() { fini(); }

    __forceinline__
    __host__ __device__
    void init(array_d_t<T>* a) { array = a; }

    __forceinline__
    __host__ __device__
    void fini(void) {
        if (page) {
            array->release_page(page, range_id, start);
            page = nullptr;
        }

    }

    __forceinline__
    __host__ __device__
    T* update_page(const size_t i) {
        ////printf("++++acquire: i: %llu\tpage: %llu\tstart: %llu\tend: %llu\trange: %llu\n",
//            (unsigned long long) i, (unsigned long long) page, (unsigned long long) start, (unsigned long long) end, (unsigned long long) range_id);
        fini(); //destructor
        addr = (T*) array->acquire_page(i, page, start, end, range_id);
//        //printf("----acquire: i: %llu\tpage: %llu\tstart: %llu\tend: %llu\trange: %llu\n",
//            (unsigned long long) i, (unsigned long long) page, (unsigned long long) start, (unsigned long long) end, (unsigned long long) range_id);
        return addr;
    }

    __forceinline__
    __host__ __device__
    T operator[](const size_t i) const {
        if ((i < start) || (i >= end)) {
           T* tmpaddr =  update_page(i);
        }
        return addr[i-start];
    }
    
    __host__ __device__
    T* memref(size_t i) {
        T* ret_; 
        if ((i < start) || (i >= end)) {
           ret_ =  update_page(i);
        }
        return ret_;
    }


    __forceinline__
    __host__ __device__
    T& operator[](const size_t i) {
        if ((i < start) || (i >= end)) {
            update_page(i);
            page->state.fetch_or(DIRTY, simt::memory_order_relaxed);
        }
        return addr[i-start];
    }
};

/* cache_page_t: 캐시 슬롯 메타데이터. 캐시의 각 물리적 슬롯마다 하나씩 존재한다.
 * page_take_lock: 캐시 슬롯의 잠금 상태
 *   FREE(2): 아직 한 번도 사용되지 않은 빈 슬롯
 *   UNLOCKED(0): 매핑되어 있지만 잠기지 않음 (eviction 후보)
 *   LOCKED(1): 현재 eviction 또는 매핑 변경 중
 * page_translation: 이 캐시 슬롯에 매핑된 SSD 페이지의 전역 주소 (range_id + 페이지 오프셋 인코딩)
 * 32바이트 정렬: GPU 캐시라인 내 false sharing 방지
 */
typedef struct __align__(32) {
    simt::atomic<uint32_t, simt::thread_scope_device>  page_take_lock; // 슬롯 잠금 상태 (FREE/LOCKED/UNLOCKED)
    //
    uint64_t page_translation; // 매핑된 SSD 전역 주소: (page_offset << n_ranges_bits) | range_id
    uint8_t pad[32-8]; // 32바이트 정렬을 위한 패딩

} __attribute__((aligned (32))) cache_page_t;
/*
  struct cache_page_t {
  simt::atomic<uint32_t, simt::thread_scope_device>  page_take_lock;
  uint32_t  page_translation;
  uint8_t   range_id;
  };
*/
/* page_cache_d_t: GPU 디바이스에 상주하는 페이지 캐시 메타데이터.
 * GPU 커널에서 직접 접근하여 캐시 조회, 페이지 할당, eviction을 수행한다.
 * 이 구조체의 핵심 역할은 SSD 페이지 ↔ GPU 캐시 슬롯 매핑을 관리하는 것이다.
 */
struct page_cache_d_t {
    uint8_t* base_addr;          // 캐시 데이터 영역의 GPU 메모리 시작 주소 (DMA 매핑됨)
    uint64_t page_size;          // 캐시 페이지 크기 (바이트, 보통 4KB~64KB)
    uint64_t page_size_minus_1;  // page_size - 1: 비트 AND 마스크용 (인덱스 → 페이지 내 오프셋)
    uint64_t page_size_log;      // log2(page_size): 나눗셈 대신 비트 시프트용
    uint64_t n_pages;            // 캐시 슬롯 총 수 (GPU 메모리에 맞는 만큼)
    uint64_t n_pages_minus_1;    // n_pages - 1: 모듈러 연산 대신 비트 AND용
    cache_page_t* cache_pages;   // 캐시 슬롯 메타데이터 배열 (n_pages개, GPU 메모리)
    padded_struct_pc* page_ticket; // Clock 교체 정책의 순환 포인터 (atomic 증가로 다음 후보 선택)
    uint64_t* prp1;              // NVMe PRP1 주소 배열: 각 캐시 슬롯의 물리 주소 (n_pages개)
    uint64_t* prp2;              // NVMe PRP2 주소 배열: page_size > ctrl.page_size일 때 사용
    uint64_t    ctrl_page_size;  // NVMe 컨트롤러 페이지 크기 (보통 4KB)
    uint64_t  range_cap;         // 최대 range 수
    pages_t*   ranges;           // range별 data_page_t 배열 포인터 (GPU 메모리)
    pages_t*   h_ranges;         // 호스트 측 ranges 복사본
    uint64_t n_ranges;           // 현재 등록된 range 수
    uint64_t n_ranges_bits;      // log2(range_cap): 전역 주소 인코딩에 사용
    uint64_t n_ranges_mask;      // range_cap - 1: 전역 주소에서 range_id 추출용 마스크
    uint64_t n_cachelines_for_states; // 상태 배열의 캐시라인 수 (캐시라인 분산 접근용)

    uint64_t* ranges_page_starts; // range별 SSD 시작 페이지 번호 배열
    data_dist_t* ranges_dists;    // range별 데이터 분배 방식 (REPLICATE/STRIPE)
    simt::atomic<uint64_t, simt::thread_scope_device>* ctrl_counter; // 컨트롤러 라운드로빈 카운터

    simt::atomic<uint64_t, simt::thread_scope_device>* q_head; // 이중 읽기 최적화: 글로벌 큐 head
    simt::atomic<uint64_t, simt::thread_scope_device>* q_tail; // 이중 읽기 최적화: 글로벌 큐 tail
    simt::atomic<uint64_t, simt::thread_scope_device>* q_lock; // 이중 읽기 최적화: 글로벌 큐 잠금
    simt::atomic<uint64_t, simt::thread_scope_device>* extra_reads; // 이중 읽기 횟수 통계 카운터

    Controller** d_ctrls;        // NVMe 컨트롤러 배열 (GPU 디바이스 포인터)
    uint64_t n_ctrls;            // NVMe 컨트롤러 수
    bool prps;                   // PRP2가 필요한지 여부 (page_size > ctrl_page_size)

    uint64_t n_blocks_per_page;  // 캐시 페이지 당 NVMe LBA 블록 수 (page_size / lba_size)

    /* get_cache_page: 캐시 슬롯 번호로 cache_page_t 포인터를 반환한다 */
    __forceinline__
    __device__
    cache_page_t* get_cache_page(const uint32_t page) const;

    /* find_slot: 새 SSD 페이지를 위한 캐시 슬롯을 찾는다 (Clock 교체 정책).
     * 빈 슬롯(FREE)이 있으면 즉시 사용하고, 없으면 참조 카운트가 0인 슬롯을 eviction한다.
     * DIRTY 페이지는 eviction 전에 SSD에 write-back한다.
     */
    __forceinline__
    __device__
    uint32_t find_slot(uint64_t address, uint64_t range_id, const uint32_t queue_);


};


/* read_data: NVMe SSD에서 데이터를 캐시 슬롯으로 읽어온다 (NVM_IO_READ 커맨드 발행) */
__device__ void read_data(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry);
/* write_data: 캐시 슬롯의 데이터를 NVMe SSD에 기록한다 (NVM_IO_WRITE 커맨드 발행, write-back) */
__device__ void write_data(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry);

/* get_backing_page_: 데이터 분배 방식에 따라 SSD 상의 실제 페이지 번호를 계산한다.
 * STRIPE: 페이지가 여러 컨트롤러에 분산되므로 page_offset / n_ctrls로 컨트롤러 내 페이지 번호를 구한다
 * REPLICATE: 모든 컨트롤러에 동일하므로 page_offset을 그대로 사용한다
 */
__forceinline__
__device__
uint64_t get_backing_page_(const uint64_t page_start, const size_t page_offset, const uint64_t n_ctrls, const data_dist_t dist) {
    uint64_t page = page_start;
    if (dist == STRIPE) {
        page += page_offset / n_ctrls; // 스트라이프: n_ctrls개 컨트롤러에 분산
    }
    else if (dist == REPLICATE) {
        page += page_offset; // 복제: 각 컨트롤러에 전체 데이터 존재
    }

    return page;
}

/* get_backing_ctrl_: 데이터 분배 방식에 따라 접근할 NVMe 컨트롤러를 결정한다.
 * STRIPE: page_offset % n_ctrls로 해당 페이지가 있는 컨트롤러를 선택한다
 * REPLICATE: ALL_CTRLS를 반환하여 모든 컨트롤러에서 읽기 가능함을 나타낸다
 */
__forceinline__
__device__
uint64_t get_backing_ctrl_(const size_t page_offset, const uint64_t n_ctrls, const data_dist_t dist) {
    uint64_t ctrl;

    if (dist == STRIPE) {
        ctrl = page_offset % n_ctrls; // 해당 페이지가 있는 컨트롤러
    }
    else if (dist == REPLICATE) {
        ctrl = ALL_CTRLS; // 모든 컨트롤러에 복제됨
    }
    return ctrl;

}

/* __flush: DIRTY 캐시 페이지를 SSD에 write-back하는 CUDA 커널.
 * 모든 캐시 슬롯을 병렬로 검사하여 DIRTY 비트가 설정된 페이지를 SSD에 기록한다.
 * flush_cache() 호스트 함수에서 호출된다.
 */
__global__
void __flush(page_cache_d_t* pc) {
    uint64_t page = threadIdx.x + blockIdx.x * blockDim.x;

    if (page < pc->n_pages) {
        uint64_t previous_global_address = pc->cache_pages[page].page_translation;
        //uint8_t previous_range = this->cache_pages[page].range_id;
        uint64_t previous_range = previous_global_address & pc->n_ranges_mask;
        uint64_t previous_address = previous_global_address >> pc->n_ranges_bits;
        //uint32_t new_state = BUSY;

        uint32_t expected_state = pc->ranges[previous_range][previous_address].state.load(simt::memory_order_relaxed);

        uint32_t d = expected_state & DIRTY;
        uint32_t smid = get_smid();
        if (d) {

            uint64_t ctrl = get_backing_ctrl_(previous_address, pc->n_ctrls, pc->ranges_dists[previous_range]);
            //uint64_t get_backing_page(const uint64_t page_start, const size_t page_offset, const uint64_t n_ctrls, const data_dist_t dist) {
            uint64_t index = get_backing_page_(pc->ranges_page_starts[previous_range], previous_address, pc->n_ctrls, pc->ranges_dists[previous_range]);
            // //printf("Eviciting range_id: %llu\tpage_id: %llu\tctrl: %llx\tindex: %llu\n",
            //        (unsigned long long) previous_range, (unsigned long long)previous_address,
            //        (unsigned long long) ctrl, (unsigned long long) index);
            if (ctrl == ALL_CTRLS) {
                for (ctrl = 0; ctrl < pc->n_ctrls; ctrl++) {
                    Controller* c = pc->d_ctrls[ctrl];
                    uint32_t queue = smid % (c->n_qps);
                    write_data(pc, (c->d_qps)+queue, (index*pc->n_blocks_per_page), pc->n_blocks_per_page, page);
                }
            }
            else {

                Controller* c = pc->d_ctrls[ctrl];
                uint32_t queue = smid % (c->n_qps);

                //index = ranges_page_starts[previous_range] + previous_address;


                write_data(pc, (c->d_qps)+queue, (index*pc->n_blocks_per_page), pc->n_blocks_per_page, page);
            }

            pc->ranges[previous_range][previous_address].state.fetch_and(~DIRTY);

        }
    }
}

/* page_cache_t: 호스트 측 페이지 캐시 관리자.
 * GPU 페이지 캐시의 초기화, range 등록, flush, 통계 출력을 담당한다.
 * 내부에 page_cache_d_t (디바이스 메타데이터)를 가지고 있으며,
 * 생성 시 GPU 메모리를 DMA 매핑하여 NVMe 컨트롤러가 직접 읽고 쓸 수 있게 설정한다.
 */
struct page_cache_t {

    page_cache_d_t pdt;                // 디바이스 측 캐시 메타데이터 (GPU로 복사됨)
    pages_t*   h_ranges;               // 호스트 측 range별 pages 배열 포인터
    uint64_t* h_ranges_page_starts;    // 호스트 측 range별 SSD 시작 페이지 번호
    data_dist_t* h_ranges_dists;       // 호스트 측 range별 데이터 분배 방식
    page_cache_d_t* d_pc_ptr;          // GPU 메모리에 복사된 page_cache_d_t 포인터

    DmaPtr pages_dma;                  // 캐시 데이터 영역 DMA 메모리 (NVMe가 직접 접근)
    DmaPtr prp_list_dma;               // PRP 리스트 DMA 메모리 (큰 페이지용)
    BufferPtr prp1_buf;                // PRP1 주소 배열 GPU 버퍼
    BufferPtr prp2_buf;                // PRP2 주소 배열 GPU 버퍼
    BufferPtr cache_pages_buf;         // cache_page_t 배열 GPU 버퍼
    BufferPtr ranges_buf;              // ranges 포인터 배열 GPU 버퍼
    BufferPtr pc_buff;                 // page_cache_d_t GPU 복사본 버퍼
    BufferPtr d_ctrls_buff;            // 컨트롤러 포인터 배열 GPU 버퍼
    BufferPtr ranges_page_starts_buf;  // range 시작 페이지 배열 GPU 버퍼
    BufferPtr ranges_dists_buf;        // range 분배 방식 배열 GPU 버퍼

    BufferPtr page_ticket_buf;         // Clock 교체 포인터 GPU 버퍼
    BufferPtr ctrl_counter_buf;        // 컨트롤러 라운드로빈 카운터 GPU 버퍼
    BufferPtr q_head_buf;              // 이중 읽기 큐 head GPU 버퍼
    BufferPtr q_tail_buf;              // 이중 읽기 큐 tail GPU 버퍼
    BufferPtr q_lock_buf;              // 이중 읽기 큐 잠금 GPU 버퍼
    BufferPtr extra_reads_buf;         // 이중 읽기 통계 카운터 GPU 버퍼

    void print_reset_stats(void) {
        uint64_t v = 0;
        cuda_err_chk(cudaMemcpy(&v, pdt.extra_reads, sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaMemcpyDeviceToHost));

        cuda_err_chk(cudaMemset(pdt.extra_reads, 0, sizeof(simt::atomic<uint64_t, simt::thread_scope_device>)));

//        printf("Cache Extra Reads: %llu\n", v);
    }

    /* flush_cache: 모든 DIRTY 캐시 페이지를 SSD에 write-back하는 CUDA 커널을 실행한다 */
    void flush_cache() {
        size_t threads = 64;
        size_t n_blocks = (pdt.n_pages + threads - 1) / threads;

        __flush<<<n_blocks, threads>>>(d_pc_ptr);


    }

    /* add_range: 새로운 데이터 범위(range)를 캐시에 등록한다.
     * range에 ID를 부여하고 캐시 메타데이터를 GPU 메모리에 동기화한다.
     */
    template <typename T>
    void add_range(range_t<T>* range) {
        range->rdt.range_id  = pdt.n_ranges++;
        h_ranges[range->rdt.range_id] = range->rdt.pages;
        h_ranges_page_starts[range->rdt.range_id] = range->rdt.page_start;
        h_ranges_dists[range->rdt.range_id] = range->rdt.dist;
        cuda_err_chk(cudaMemcpy(pdt.ranges_page_starts, h_ranges_page_starts, pdt.n_ranges * sizeof(uint64_t), cudaMemcpyHostToDevice));
        cuda_err_chk(cudaMemcpy(pdt.ranges, h_ranges, pdt.n_ranges* sizeof(pages_t), cudaMemcpyHostToDevice));
        cuda_err_chk(cudaMemcpy(pdt.ranges_dists, h_ranges_dists, pdt.n_ranges* sizeof(data_dist_t), cudaMemcpyHostToDevice));
        cuda_err_chk(cudaMemcpy(d_pc_ptr, &pdt, sizeof(page_cache_d_t), cudaMemcpyHostToDevice));

    }

    /* page_cache_t 생성자: GPU 페이지 캐시를 초기화한다.
     * ps: 캐시 페이지 크기 (바이트)
     * np: 캐시 페이지 수 (GPU 메모리 크기에 맞춰 설정)
     * cudaDevice: CUDA GPU 디바이스 ID
     * ctrl: 기본 NVMe 컨트롤러 (DMA 매핑용)
     * max_range: 최대 range 수
     * ctrls: 사용할 NVMe 컨트롤러 목록
     *
     * 주요 초기화 단계:
     * 1) 캐시 메타데이터 GPU 버퍼 할당 (cache_pages, ranges, tickets 등)
     * 2) 캐시 데이터 영역 DMA 매핑 (NVMe가 GPU 메모리에 직접 데이터 전송)
     * 3) PRP 테이블 구성 (NVMe 커맨드의 물리 주소 목록)
     * 4) page_cache_d_t를 GPU에 복사
     */
    page_cache_t(const uint64_t ps, const uint64_t np, const uint32_t cudaDevice, const Controller& ctrl, const uint64_t max_range, const std::vector<Controller*>& ctrls) {

        ctrl_counter_buf = createBuffer(sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaDevice);
        q_head_buf = createBuffer(sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaDevice);
        q_tail_buf = createBuffer(sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaDevice);
        q_lock_buf = createBuffer(sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaDevice);
        extra_reads_buf = createBuffer(sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaDevice);
        pdt.ctrl_counter = (simt::atomic<uint64_t, simt::thread_scope_device>*)ctrl_counter_buf.get();
        pdt.page_size = ps;
        pdt.q_head = (simt::atomic<uint64_t, simt::thread_scope_device>*)q_head_buf.get();
        pdt.q_tail = (simt::atomic<uint64_t, simt::thread_scope_device>*)q_tail_buf.get();
        pdt.q_lock = (simt::atomic<uint64_t, simt::thread_scope_device>*)q_lock_buf.get();
        pdt.extra_reads = (simt::atomic<uint64_t, simt::thread_scope_device>*)extra_reads_buf.get();
        pdt.page_size_minus_1 = ps - 1;
        pdt.n_pages = np;
        pdt.ctrl_page_size = ctrl.ctrl->page_size;
        pdt.n_pages_minus_1 = np - 1;
        pdt.n_ctrls = ctrls.size();
        d_ctrls_buff = createBuffer(pdt.n_ctrls * sizeof(Controller*), cudaDevice);
        pdt.d_ctrls = (Controller**) d_ctrls_buff.get();
        pdt.n_blocks_per_page = (ps/ctrl.blk_size);
        pdt.n_cachelines_for_states = np/STATES_PER_CACHELINE;
        for (size_t k = 0; k < pdt.n_ctrls; k++)
            cuda_err_chk(cudaMemcpy(pdt.d_ctrls+k, &(ctrls[k]->d_ctrl_ptr), sizeof(Controller*), cudaMemcpyHostToDevice));
        //n_ctrls = ctrls.size();
        //d_ctrls_buff = createBuffer(n_ctrls * sizeof(Controller*), cudaDevice);
        //d_ctrls = (Controller**) d_ctrls_buff.get();
        //for (size_t k = 0; k < n_ctrls; k++)
        //    cuda_err_chk(cudaMemcpy(d_ctrls+k, &(ctrls[k]->d_ctrl_ptr), sizeof(Controller*), cudaMemcpyHostToDevice));

        pdt.range_cap = max_range;
        pdt.n_ranges = 0;
        pdt.n_ranges_bits = (max_range == 1) ? 1 : std::log2(max_range);
        pdt.n_ranges_mask = max_range-1;
        std::cout << "n_ranges_bits: " << std::dec << pdt.n_ranges_bits << std::endl;
        std::cout << "n_ranges_mask: " << std::dec << pdt.n_ranges_mask << std::endl;

        pdt.page_size_log = std::log2(ps);
        ranges_buf = createBuffer(max_range * sizeof(pages_t), cudaDevice);
        pdt.ranges = (pages_t*)ranges_buf.get();
        h_ranges = new pages_t[max_range];

        h_ranges_page_starts = new uint64_t[max_range];
        std::memset(h_ranges_page_starts, 0, max_range * sizeof(uint64_t));

        //pages_translation_buf = createBuffer(np * sizeof(uint32_t), cudaDevice);
        //pdt.page_translation = (uint32_t*)page_translation_buf.get();
        //page_translation_buf = createBuffer(np * sizeof(padded_struct_pc), cudaDevice);
        //page_translation = (padded_struct_pc*)page_translation_buf.get();

        //page_take_lock_buf = createBuffer(np * sizeof(padded_struct_pc), cudaDevice);
        //pdt.page_take_lock =  (padded_struct_pc*)page_take_lock_buf.get();

        cache_pages_buf = createBuffer(np * sizeof(cache_page_t), cudaDevice);
        pdt.cache_pages = (cache_page_t*)cache_pages_buf.get();

        ranges_page_starts_buf = createBuffer(max_range * sizeof(uint64_t), cudaDevice);
        pdt.ranges_page_starts = (uint64_t*) ranges_page_starts_buf.get();

        page_ticket_buf = createBuffer(1 * sizeof(padded_struct_pc), cudaDevice);
        pdt.page_ticket =  (padded_struct_pc*)page_ticket_buf.get();
        //std::vector<padded_struct_pc> tps(np, FREE);
        cache_page_t* tps = new cache_page_t[np];
        for (size_t i = 0; i < np; i++)
            tps[i].page_take_lock = FREE;
        cuda_err_chk(cudaMemcpy(pdt.cache_pages, tps, np*sizeof(cache_page_t), cudaMemcpyHostToDevice));
        delete tps;

        ranges_dists_buf = createBuffer(max_range * sizeof(data_dist_t), cudaDevice);
        pdt.ranges_dists = (data_dist_t*)ranges_dists_buf.get();
        h_ranges_dists = new data_dist_t[max_range];

        uint64_t cache_size = ps*np;
        this->pages_dma = createDma(ctrl.ctrl, NVM_PAGE_ALIGN(cache_size, 1UL << 16), cudaDevice);
        pdt.base_addr = (uint8_t*) this->pages_dma.get()->vaddr;
        std::cout << "pages_dma: " << std::hex << this->pages_dma.get()->vaddr << "\t" << this->pages_dma.get()->ioaddrs[0] << std::endl;
        std::cout << "HEREN\n";
        const uint32_t uints_per_page = ctrl.ctrl->page_size / sizeof(uint64_t);
        if ((pdt.page_size > (ctrl.ctrl->page_size * uints_per_page)) || (np == 0) || (pdt.page_size < ctrl.ns.lba_data_size))
            throw error(string("page_cache_t: Can't have such page size or number of pages"));
        if (ps <= this->pages_dma.get()->page_size) {
            std::cout << "Cond1\n";
            uint64_t how_many_in_one = ctrl.ctrl->page_size/ps;
            this->prp1_buf = createBuffer(np * sizeof(uint64_t), cudaDevice);
            pdt.prp1 = (uint64_t*) this->prp1_buf.get();


            std::cout << np << " " << sizeof(uint64_t) << " " << how_many_in_one << " " << this->pages_dma.get()->n_ioaddrs <<std::endl;
            uint64_t* temp = new uint64_t[how_many_in_one *  this->pages_dma.get()->n_ioaddrs];
            std::memset(temp, 0, how_many_in_one *  this->pages_dma.get()->n_ioaddrs);
            if (temp == NULL)
                std::cout << "NULL\n";

            for (size_t i = 0; (i < this->pages_dma.get()->n_ioaddrs) ; i++) {
                for (size_t j = 0; (j < how_many_in_one); j++) {
                    temp[i*how_many_in_one + j] = ((uint64_t)this->pages_dma.get()->ioaddrs[i]) + j*ps;
                    //std::cout << std::dec << "\ti: " << i << "\tj: " << j << "\tindex: "<< (i*how_many_in_one + j) << "\t" << std::hex << (((uint64_t)this->pages_dma.get()->ioaddrs[i]) + j*ps) << std::dec << std::endl;
                }
            }
            cuda_err_chk(cudaMemcpy(pdt.prp1, temp, np * sizeof(uint64_t), cudaMemcpyHostToDevice));
            delete temp;
            //std::cout << "HERE1\n";
            //free(temp);
            //std::cout << "HERE2\n";
            pdt.prps = false;
        }

        else if ((ps > this->pages_dma.get()->page_size) && (ps <= (this->pages_dma.get()->page_size * 2))) {
            this->prp1_buf = createBuffer(np * sizeof(uint64_t), cudaDevice);
            pdt.prp1 = (uint64_t*) this->prp1_buf.get();
            this->prp2_buf = createBuffer(np * sizeof(uint64_t), cudaDevice);
            pdt.prp2 = (uint64_t*) this->prp2_buf.get();
            //uint64_t* temp1 = (uint64_t*) malloc(np * sizeof(uint64_t));
            uint64_t* temp1 = new uint64_t[np * sizeof(uint64_t)];
            std::memset(temp1, 0, np * sizeof(uint64_t));
            //uint64_t* temp2 = (uint64_t*) malloc(np * sizeof(uint64_t));
            uint64_t* temp2 = new uint64_t[np * sizeof(uint64_t)];
            std::memset(temp2, 0, np * sizeof(uint64_t));
            for (size_t i = 0; i < np; i++) {
                temp1[i] = ((uint64_t)this->pages_dma.get()->ioaddrs[i*2]);
                temp2[i] = ((uint64_t)this->pages_dma.get()->ioaddrs[i*2+1]);
            }
            cuda_err_chk(cudaMemcpy(pdt.prp1, temp1, np * sizeof(uint64_t), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(pdt.prp2, temp2, np * sizeof(uint64_t), cudaMemcpyHostToDevice));

            delete temp1;
            delete temp2;
            pdt.prps = true;
        }
        else {
            this->prp1_buf = createBuffer(np * sizeof(uint64_t), cudaDevice);
            pdt.prp1 = (uint64_t*) this->prp1_buf.get();
            uint32_t prp_list_size =  ctrl.ctrl->page_size  * np;
            this->prp_list_dma = createDma(ctrl.ctrl, NVM_PAGE_ALIGN(prp_list_size, 1UL << 16), cudaDevice);
            this->prp2_buf = createBuffer(np * sizeof(uint64_t), cudaDevice);
            pdt.prp2 = (uint64_t*) this->prp2_buf.get();
            uint64_t* temp1 = new uint64_t[np * sizeof(uint64_t)];
            uint64_t* temp2 = new uint64_t[np * sizeof(uint64_t)];
            uint64_t* temp3 = new uint64_t[prp_list_size];
            std::memset(temp1, 0, np * sizeof(uint64_t));
            std::memset(temp2, 0, np * sizeof(uint64_t));
            std::memset(temp3, 0, prp_list_size);
            uint32_t how_many_in_one = ps /  ctrl.ctrl->page_size ;
            for (size_t i = 0; i < np; i++) {
                temp1[i] = ((uint64_t) this->pages_dma.get()->ioaddrs[i*how_many_in_one]);
                temp2[i] = ((uint64_t) this->prp_list_dma.get()->ioaddrs[i]);
                for(size_t j = 0; j < (how_many_in_one-1); j++) {
                    temp3[i*uints_per_page + j] = ((uint64_t) this->pages_dma.get()->ioaddrs[i*how_many_in_one + j + 1]);
                }
            }
            /*
              for (size_t i = 0; i < this->pages_dma.get()->n_ioaddrs; i+=how_many_in_one) {
              temp1[i/how_many_in_one] = ((uint64_t)this->pages_dma.get()->ioaddrs[i]);
              temp2[i/how_many_in_one] = ((uint64_t)this->prp_list_dma.get()->ioaddrs[i]);
              for (size_t j = 0; j < (how_many_in_one-1); j++) {

              temp3[(i/how_many_in_one)*uints_per_page + j] = ((uint64_t)this->pages_dma.get()->ioaddrs[i+1+j]);
              }
              }
            */

            std::cout << "Done creating PRP\n";
            cuda_err_chk(cudaMemcpy(pdt.prp1, temp1, np * sizeof(uint64_t), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(pdt.prp2, temp2, np * sizeof(uint64_t), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(this->prp_list_dma.get()->vaddr, temp3, prp_list_size, cudaMemcpyHostToDevice));

            delete temp1;
            delete temp2;
            delete temp3;
            pdt.prps = true;
        }


        pc_buff = createBuffer(sizeof(page_cache_d_t), cudaDevice);
        d_pc_ptr = (page_cache_d_t*)pc_buff.get();
        cuda_err_chk(cudaMemcpy(d_pc_ptr, &pdt, sizeof(page_cache_d_t), cudaMemcpyHostToDevice));
        std::cout << "Finish Making Page Cache\n";

    }

    ~page_cache_t() {
        delete h_ranges;
        delete h_ranges_page_starts;
        delete h_ranges_dists;
    }





};



/* range_d_t<T>: SSD 상의 데이터 범위를 나타내는 GPU 디바이스 구조체.
 * 하나의 range는 연속된 SSD 페이지 영역에 대응하며, 자체 페이지 상태 배열(pages)을 가진다.
 * GPU 커널에서 배열 인덱스를 페이지 번호로 변환하고, 캐시에서 데이터를 acquire/release한다.
 *
 * 핵심 함수들:
 * - get_page(i): 원소 인덱스 → 페이지 번호 변환
 * - acquire_page(pg): 페이지 획득 (캐시 히트/미스 처리, SSD 읽기 포함)
 * - release_page(pg): 페이지 참조 카운트 감소
 */
template <typename T>
struct range_d_t {
    uint64_t index_start;        // 이 range가 커버하는 배열 시작 인덱스
    uint64_t count;              // 배열 끝 인덱스 (배타적)
    uint64_t range_id;           // 캐시 내 range 식별 번호
    uint64_t page_start_offset;  // 첫 페이지 내 바이트 오프셋 (정렬 보정용)
    uint64_t page_size;          // 캐시 페이지 크기 (바이트)
    uint64_t page_start;         // SSD 상의 시작 페이지 번호
    uint64_t page_count;         // 이 range의 총 페이지 수
    size_t n_elems_per_page;     // 페이지 당 T 타입 원소 수 (page_size / sizeof(T))
    data_dist_t dist;            // 데이터 분배 방식 (REPLICATE/STRIPE)
    uint8_t* src;                // 소스 데이터 포인터 (사용되지 않음)

    simt::atomic<uint64_t, simt::thread_scope_device> access_cnt;   // 총 접근 횟수 (통계)
    simt::atomic<uint64_t, simt::thread_scope_device> miss_cnt;     // 캐시 미스 횟수 (통계)
    simt::atomic<uint64_t, simt::thread_scope_device> hit_cnt;      // 캐시 히트 횟수 (통계)
    simt::atomic<uint64_t, simt::thread_scope_device> read_io_cnt;  // SSD 읽기 I/O 횟수 (통계)


    pages_t pages;  // data_page_t 배열: 각 SSD 페이지의 캐시 상태 추적 (page_count개)
    page_cache_d_t cache; // 소속 캐시의 메타데이터 복사본 (캐시 조회 시 사용)
    //range_d_t(range_t<T>* rt);
    /* get_backing_page: 페이지 오프셋 → SSD 실제 페이지 번호 변환 */
    __forceinline__ __device__
    uint64_t get_backing_page(const size_t i) const;
    /* get_backing_ctrl: 페이지 오프셋 → 담당 NVMe 컨트롤러 결정 */
    __forceinline__ __device__
    uint64_t get_backing_ctrl(const size_t i) const;
    /* get_sector_size: 캐시 페이지 크기 반환 */
    __forceinline__ __device__
    uint64_t get_sector_size() const;
    /* get_page: 원소 인덱스 → range 내 페이지 번호 변환 */
    __forceinline__ __device__
    uint64_t get_page(const size_t i) const;
    /* get_subindex: 원소 인덱스 → 페이지 내 바이트 오프셋 변환 */
    __forceinline__ __device__
    uint64_t get_subindex(const size_t i) const;
    /* get_global_address: (페이지번호, range_id)를 하나의 전역 주소로 인코딩 */
    __forceinline__ __device__
    uint64_t get_global_address(const size_t page) const;
    /* release_page: 참조 카운트를 1 감소 (단일 스레드) */
    __forceinline__ __device__
    void release_page(const size_t pg) const;
    /* release_page: 참조 카운트를 count만큼 감소 (warp coalescing용) */
    __forceinline__ __device__
    void release_page(const size_t pg, const uint32_t count) const;
    /* acquire_page: 페이지를 획득한다 (캐시 히트/미스 처리, SSD 읽기 포함).
     * BaM 페이지 캐시의 핵심 함수. 상태 머신을 사용하여:
     * - INVALID → BUSY 전환, SSD에서 읽기, VALID로 전환
     * - VALID → 즉시 반환 (캐시 히트)
     * - BUSY → spin-wait (다른 스레드가 로드 중)
     */
    __forceinline__ __device__
    uint64_t acquire_page(const size_t pg, const uint32_t count, const bool write, const uint32_t ctrl, const uint32_t queue) ;
    /* write_done: 쓰기 완료 후 호출 */
    __forceinline__ __device__
    void write_done(const size_t pg, const uint32_t count) const;
    /* operator[]: 원소 읽기 (acquire → 읽기 → release) */
    __forceinline__ __device__
    T operator[](const size_t i) ;
    /* operator(): 원소 쓰기 (acquire → 쓰기 → release) */
    __forceinline__ __device__
    void operator()(const size_t i, const T val);
    /* get_cache_page: 페이지 번호 → cache_page_t 포인터 */
    __forceinline__ __device__
    cache_page_t* get_cache_page(const size_t pg) const;
    /* get_cache_page_addr: 캐시 슬롯 번호 → GPU 메모리 주소 계산 */
    __forceinline__ __device__
    uint64_t get_cache_page_addr(const uint32_t page_trans) const;
    /* mark_page_dirty: 페이지의 DIRTY 비트를 설정한다 (쓰기 접근 시) */
    __forceinline__ __device__
    void mark_page_dirty(const size_t index);
};

/* range_t<T>: 호스트 측 데이터 범위 관리 구조체.
 * SSD의 특정 데이터 영역을 나타내며, 해당 영역의 페이지 상태 배열을 GPU에 할당한다.
 * 생성 시 캐시에 자동 등록된다.
 */
template <typename T>
struct range_t {
    range_d_t<T> rdt;            // 디바이스 측 range 메타데이터 (GPU로 복사됨)

    range_d_t<T>* d_range_ptr;   // GPU 메모리에 복사된 range_d_t 포인터
    page_cache_d_t* cache;       // 소속 캐시의 GPU 포인터

    BufferPtr pages_buff;        // data_page_t 배열 GPU 버퍼

    BufferPtr range_buff;        // range_d_t GPU 복사본 버퍼

    /* 생성자 파라미터:
     * is: 배열 시작 인덱스
     * count: 배열 끝 인덱스
     * ps: SSD 시작 페이지
     * pc: 총 페이지 수
     * pso: 페이지 내 시작 오프셋
     * p_size: 페이지 크기
     * c_h: 소속 캐시
     * dist: 데이터 분배 방식
     */
    range_t(uint64_t is, uint64_t count, uint64_t ps, uint64_t pc, uint64_t pso, uint64_t p_size, page_cache_t* c_h, uint32_t cudaDevice, data_dist_t dist = REPLICATE);



};

template <typename T>
range_t<T>::range_t(uint64_t is, uint64_t count, uint64_t ps, uint64_t pc, uint64_t pso, uint64_t p_size, page_cache_t* c_h, uint32_t cudaDevice, data_dist_t dist) {
    rdt.access_cnt = 0;
    rdt.miss_cnt = 0;
    rdt.hit_cnt = 0;
    rdt.read_io_cnt = 0;
    rdt.index_start = is;
    rdt.count = count;
    //range_id = (c_h->range_count)++;
    rdt.page_start = ps;
    rdt.page_count = pc;
    rdt.page_size = c_h->pdt.page_size;
    rdt.page_start_offset = pso;
    rdt.dist = dist;
    size_t s = pc;//(rdt.page_end-rdt.page_start);//*page_size / c_h->page_size;
    rdt.n_elems_per_page = rdt.page_size / sizeof(T);
    cache = (page_cache_d_t*) c_h->d_pc_ptr;
    pages_buff = createBuffer(s * sizeof(data_page_t), cudaDevice);
    rdt.pages = (pages_t) pages_buff.get();
    //std::vector<padded_struct_pc> ts(s, INVALID);
    data_page_t* ts = new data_page_t[s];
    for (size_t i = 0; i < s; i++) {
        ts[i].state = INVALID;
    }
    ////printf("S value: %llu\n", (unsigned long long)s);
    cuda_err_chk(cudaMemcpy(rdt.pages//_states
                            , ts, s * sizeof(data_page_t), cudaMemcpyHostToDevice));
    delete ts;

    //page_addresses_buff = createBuffer(s * sizeof(uint32_t), cudaDevice);
    //rdt.page_addresses = (uint32_t*) page_addresses_buff.get();
    //page_addresses_buff = createBuffer(s * sizeof(padded_struct_pc), cudaDevice);
    //page_addresses = (padded_struct_pc*) page_addresses_buff.get();

    range_buff = createBuffer(sizeof(range_d_t<T>), cudaDevice);
    d_range_ptr = (range_d_t<T>*)range_buff.get();
    //rdt.range_id  = c_h->pdt.n_ranges++;


    cuda_err_chk(cudaMemcpy(d_range_ptr, &rdt, sizeof(range_d_t<T>), cudaMemcpyHostToDevice));

    c_h->add_range(this);

    rdt.cache = c_h->pdt;
    cuda_err_chk(cudaMemcpy(d_range_ptr, &rdt, sizeof(range_d_t<T>), cudaMemcpyHostToDevice));

}




template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_backing_page(const size_t page_offset) const {
    return get_backing_page_(page_start, page_offset, cache.n_ctrls, dist);
}




template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_backing_ctrl(const size_t page_offset) const {
    return get_backing_ctrl_(page_offset, cache.n_ctrls, dist);
}

template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_sector_size() const {
    return page_size;
}


template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_page(const size_t i) const {
    uint64_t index = ((i - index_start) * sizeof(T) + page_start_offset) >> (cache.page_size_log);
    return index;
}
template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_subindex(const size_t i) const {
    uint64_t index = ((i - index_start) * sizeof(T) + page_start_offset) & (cache.page_size_minus_1);
    return index;
}
template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_global_address(const size_t page) const {
    return ((page << cache.n_ranges_bits) | range_id);
}
template <typename T>
__forceinline__
__device__
void range_d_t<T>::release_page(const size_t pg) const {
    uint64_t index = pg;
    pages[index].state.fetch_sub(1, simt::memory_order_release);
}

template <typename T>
__forceinline__
__device__
void range_d_t<T>::release_page(const size_t pg, const uint32_t count) const {
    uint64_t index = pg;
    pages[index].state.fetch_sub(count, simt::memory_order_release);
}

template <typename T>
__forceinline__
__device__
cache_page_t* range_d_t<T>::get_cache_page(const size_t pg) const {
    uint32_t page_trans = pages[pg].offset;
    return cache.get_cache_page(page_trans);
}

template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::get_cache_page_addr(const uint32_t page_trans) const {
    return ((uint64_t)((cache.base_addr+(page_trans * cache.page_size))));
}

template <typename T>
__forceinline__
__device__
void range_d_t<T>::mark_page_dirty(const size_t index) {
    pages[index].state.fetch_or(DIRTY, simt::memory_order_relaxed);
}


/* acquire_page: BaM 페이지 캐시의 핵심 - 페이지 상태 머신을 구동하여 캐시 접근을 처리한다.
 *
 * 상태 머신 (상위 2비트: VALID, BUSY):
 * ┌─────────┐  fetch_or(BUSY)   ┌─────────┐  read_data()    ┌─────────┐
 * │ INVALID │ ───────────────→ │  BUSY   │ ──xor(0xC...)──→│  VALID  │
 * │ (NV_NB) │                  │ (NV_B)  │                 │ (V_NB)  │
 * └─────────┘                  └─────────┘                 └─────────┘
 *      ↑ eviction (find_slot)       ↑ spin-wait                │ 직접 반환
 *      └────────────────────────────┘                          │ (캐시 히트)
 *                                                              ↓
 *
 * pg: range 내 페이지 번호
 * count: 참조 카운트 증가량 (warp coalescing으로 여러 스레드 합산)
 * write: 쓰기 접근 여부 (true이면 DIRTY 비트 설정)
 * ctrl_: 사용할 NVMe 컨트롤러 (힌트)
 * queue: 사용할 NVMe 큐 번호
 * 반환값: 캐시 슬롯 번호 (cache_pages 배열의 인덱스)
 */
template <typename T>
__forceinline__
__device__
uint64_t range_d_t<T>::acquire_page(const size_t pg, const uint32_t count, const bool write, const uint32_t ctrl_, const uint32_t queue) {
    uint64_t index = pg;
    // 접근 통계를 기록한다
    access_cnt.fetch_add(count, simt::memory_order_relaxed);
    bool fail = true;
    unsigned int ns = 8;
    uint64_t read_state,st,st_new;
    // 참조 카운트를 먼저 증가시키고 이전 상태를 읽는다 (acquire로 가시성 보장)
    read_state = pages[index].state.fetch_add(count, simt::memory_order_acquire);
    do {
        // 상위 2비트(VALID, BUSY)를 추출하여 현재 상태를 판단한다
        st = (read_state >> (CNT_SHIFT+1)) & 0x03;

        switch (st) {
            // === INVALID (NV_NB): 캐시에 없음 → SSD에서 로드 필요 ===
        case NV_NB:
            // BUSY 비트를 설정하여 로드 권한을 획득한다
            st_new = pages[index].state.fetch_or(BUSY, simt::memory_order_acquire);
            if ((st_new & BUSY) == 0) {
                // BUSY 설정 성공: 이 스레드가 데이터를 로드할 책임이 있다
                uint64_t st_new_st = (st_new >> (CNT_SHIFT+1)) & 0x03;
                if (st_new_st == NV_NB) {
                    // 캐시 슬롯을 찾는다 (Clock 교체: 빈 슬롯 또는 eviction 대상)
                    uint32_t page_trans = cache.find_slot(index, range_id, queue);
                    //uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
                    //uint32_t sm_id = get_smid();
                    //uint32_t ctrl = (tid/32) % (cache.n_ctrls);
                    //uint32_t ctrl = sm_id % (cache.n_ctrls);
                    //uint32_t ctrl = cache.ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (cache.n_ctrls);
                    // 담당 NVMe 컨트롤러를 결정한다
                    uint64_t ctrl = get_backing_ctrl(index);
                    if (ctrl == ALL_CTRLS)
                        // REPLICATE 모드: 라운드로빈으로 컨트롤러를 선택한다
                        ctrl = cache.ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (cache.n_ctrls);
                    // SSD 상의 실제 페이지 번호를 계산한다
                    uint64_t b_page = get_backing_page(index);
                    Controller* c = cache.d_ctrls[ctrl];
                    c->access_counter.fetch_add(1, simt::memory_order_relaxed); // SSD 접근 통계
                    //uint32_t queue = (tid/32) % (c->n_qps);
                    //uint32_t queue = c->queue_counter.fetch_add(1, simt::memory_order_relaxed) % (c->n_qps);
                    //uint32_t queue = ((sm_id * 64) + warp_id()) % (c->n_qps);
                    read_io_cnt.fetch_add(1, simt::memory_order_relaxed); // I/O 통계
                    // NVMe Read 커맨드를 발행하여 SSD에서 캐시 슬롯으로 데이터를 읽는다
                    read_data(&cache, (c->d_qps)+queue, ((b_page)*cache.n_blocks_per_page), cache.n_blocks_per_page, page_trans);
                    // 이 SSD 페이지가 매핑된 캐시 슬롯 번호를 저장한다
                    pages[index].offset = page_trans;
                    // while (cache.page_translation[global_page].load(simt::memory_order_acquire) != page_trans)
                    //     __nanosleep(100);
                    //miss_cnt.fetch_add(count, simt::memory_order_relaxed);
                    miss_cnt.fetch_add(count, simt::memory_order_relaxed); // 미스 통계
                    // 쓰기 접근이면 DIRTY 비트를 설정한다 (나중에 eviction 시 write-back 필요)
                    if (write)
                        pages[index].state.fetch_or(DIRTY, simt::memory_order_relaxed);
                    // 상태 전환: BUSY→VALID (XOR 0xC0000000 = BUSY 비트 끄기 + VALID 비트 켜기)
                    pages[index].state.fetch_xor(DISABLE_BUSY_ENABLE_VALID, simt::memory_order_release);
                    return page_trans; // 캐시 슬롯 번호를 반환한다

                    fail = false;
                } else {
                    // 다른 스레드가 먼저 로드를 시작했다: BUSY 비트를 끄고 재시도한다
                    pages[index].state.fetch_and(DISABLE_BUSY_MASK, simt::memory_order_release);
                    printf("Race loading page\n");
                }
            }

            break;
            // === VALID (V_NB): 캐시 히트! 데이터가 이미 캐시에 있다 ===
        case V_NB:
            // 쓰기 접근이고 아직 DIRTY가 아니면 DIRTY 비트를 설정한다
            if (write && ((read_state & DIRTY) == 0))
                pages[index].state.fetch_or(DIRTY, simt::memory_order_relaxed);
            // 캐시 슬롯 번호를 읽는다
            uint32_t page_trans = pages[index].offset;
            // while (cache.page_translation[global_page].load(simt::memory_order_acquire) != page_trans)
            //     __nanosleep(100);
            //hit_cnt.fetch_add(count, simt::memory_order_relaxed);
            hit_cnt.fetch_add(count, simt::memory_order_relaxed); // 히트 통계
            return page_trans; // 캐시 슬롯 번호를 즉시 반환한다

            fail = false;

            break;
        // === BUSY (NV_B, V_B): 다른 스레드가 로드/처리 중 → spin-wait하고 재시도 ===
        case NV_B:
        case V_B:
        default:
            break;



         
        }
        if (fail) {
            //if ((++j % 1000000) == 0)
            //    printf("failed to acquire_page: j: %llu\tcnt_shift+1: %llu\tpage: %llu\tread_state: %llx\tst: %llx\tst_new: %llx\n", (unsigned long long)j, (unsigned long long) (CNT_SHIFT+1), (unsigned long long) index, (unsigned long long)read_state, (unsigned long long)st, (unsigned long long)st_new);
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
            __nanosleep(ns);
            if (ns < 256) {
                ns *= 2;
            }
#endif
            read_state = pages[index].state.load(simt::memory_order_acquire);
        }

    } while (fail);
    return 0;
}




/* array_d_t<T>: GPU 디바이스 측 배열 추상화. 여러 range를 묶어 하나의 논리 배열로 보이게 한다.
 * GPU 커널에서 배열 인덱스로 접근하면 자동으로 해당 range를 찾고,
 * warp-level coalescing으로 같은 페이지를 접근하는 스레드들을 그룹화하여
 * 한 번의 acquire로 여러 스레드를 처리한다.
 *
 * 핵심 함수:
 * - seq_read(i): 인덱스 i의 값을 읽는다 (acquire → 읽기 → release)
 * - seq_write(i, val): 인덱스 i에 val을 쓴다 (acquire → 쓰기 → release)
 * - coalesce_page(): warp 내 같은 페이지 접근을 합치는 최적화
 * - AtomicAdd(i, val): 인덱스 i에 atomic 덧셈 수행
 */
template<typename T>
struct array_d_t {
    uint64_t n_elems;        // 전체 원소 수
    uint64_t start_offset;   // 디스크 시작 오프셋
    uint64_t n_ranges;       // range 수
    uint8_t *src;            // 소스 데이터 포인터 (사용되지 않음)

    range_d_t<T>* d_ranges;  // range 배열 (GPU 메모리)

    __forceinline__
    __device__
    void get_page_gid(const uint64_t i, range_d_t<T>*& r_, size_t& pg, size_t& gid) const {
        int64_t r = find_range(i);
        r_ = d_ranges+r;

        if (r != -1) {
            r_ = d_ranges+r;
            pg = r_->get_page(i);
            gid = r_->get_global_address(pg);
        }
        else {
            r_ = nullptr;
            printf("here\n");
        }
    }
    __forceinline__
    __device__
    void memcpy(const uint64_t i, const uint64_t count, T* dest) {
        uint32_t lane = lane_id();
        int64_t r = find_range(i);
        auto r_ = d_ranges+r;

        uint32_t ctrl;
        uint32_t queue;

        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = 0xffffffff;
#endif
            uint32_t leader = 0;
            if (lane == leader) {
                page_cache_d_t* pc = &(r_->cache);
                ctrl = pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
                queue = get_smid() % (pc->d_ctrls[ctrl]->n_qps);
            }
            ctrl = __shfl_sync(mask, ctrl, leader);
            queue = __shfl_sync(mask, queue, leader);

            uint64_t page = r_->get_page(i);
            //uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);
            //uint64_t p_s = r_->page_size;

            uint32_t active_cnt = 32;
            uint32_t eq_mask = mask;
            int master = 0;
            uint64_t base_master;
            uint64_t base;
            //bool memcpyflag_master;
            //bool memcpyflag;
            uint32_t count = 1;
            if (master == lane) {
                //std::pair<uint64_t, bool> base_memcpyflag;
                base = r_->acquire_page(page, count, false, ctrl, queue);
                base_master = base;
//                //printf("++tid: %llu\tbase: %p  page:%llu\n", (unsigned long long) threadIdx.x, base_master, (unsigned long long) page);
            }
            base_master = __shfl_sync(eq_mask,  base_master, master);

            //if (threadIdx.x == 63) {
            ////printf("--tid: %llu\tpage: %llu\tsubindex: %llu\tbase_master: %llu\teq_mask: %x\tmaster: %llu\n", (unsigned long long) threadIdx.x, (unsigned long long) page, (unsigned long long) subindex, (unsigned long long) base_master, (unsigned) eq_mask, (unsigned long long) master);
            //}
            //
            ulonglong4* src_ = (ulonglong4*) r_->get_cache_page_addr(base_master);
            ulonglong4* dst_ = (ulonglong4*) dest;
            warp_memcpy<ulonglong4>(dst_, src_, 512/32);

            __syncwarp(eq_mask);
            if (master == lane)
                r_->release_page(page, count);
            __syncwarp(mask);

        }

    }
    /* find_range: 원소 인덱스가 속하는 range를 선형 탐색으로 찾는다.
     * 찾으면 range 인덱스를 반환하고, 못 찾으면 -1을 반환한다.
     */
    __forceinline__
    __device__
    int64_t find_range(const size_t i) const {
        int64_t range = -1;
        int64_t k = 0;
        for (; k < n_ranges; k++) {
            // index_start <= i < count 이면 이 range에 속한다
            if ((d_ranges[k].index_start <= i) && (d_ranges[k].count > i)) {
                range = k;
                break;
            }

        }
        return range;
    }
    /* coalesce_page: warp 내에서 같은 페이지를 접근하는 스레드들을 합치는 최적화 함수.
     * __match_any_sync(gaddr)로 같은 전역 페이지 주소를 가진 스레드를 그룹화하고,
     * 마스터 스레드만 acquire_page를 호출하여 참조 카운트를 한 번에 count만큼 증가시킨다.
     * 결과 캐시 주소는 __shfl_sync로 그룹 전체에 브로드캐스트한다.
     * 이 최적화로 warp 내 최대 32배의 acquire 호출을 줄일 수 있다.
     */
    __forceinline__
    __device__
    void coalesce_page(const uint32_t lane, const uint32_t mask, const int64_t r, const uint64_t page, const uint64_t gaddr, const bool write,
                       uint32_t& eq_mask, int& master, uint32_t& count, uint64_t& base_master) const {
        uint32_t ctrl;
        uint32_t queue;
        uint32_t leader = __ffs(mask) - 1;
        auto r_ = d_ranges+r;
        if (lane == leader) {
            page_cache_d_t* pc = &(r_->cache);
            ctrl = 0;//pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
            queue = get_smid() % (pc->d_ctrls[0]->n_qps);
        }

        ctrl = 0; //__shfl_sync(mask, ctrl, leader);
        queue = __shfl_sync(mask, queue, leader);


        uint32_t active_cnt = __popc(mask);
        eq_mask = __match_any_sync(mask, gaddr);
        eq_mask &= __match_any_sync(mask, (uint64_t)this);
        master = __ffs(eq_mask) - 1;

        uint32_t dirty = __any_sync(eq_mask, write);

        uint64_t base;
        //bool memcpyflag_master;
        //bool memcpyflag;
        count = __popc(eq_mask);
        if (master == lane) {
            //std::pair<uint64_t, bool> base_memcpyflag;
            base = r_->acquire_page(page, count, dirty, ctrl, queue);
            base_master = base;
//                //printf("++tid: %llu\tbase: %p  page:%llu\n", (unsigned long long) threadIdx.x, base_master, (unsigned long long) page);
        }
        base_master = __shfl_sync(eq_mask,  base_master, master);
    }

    __forceinline__
    __device__
    returned_cache_page_t<T> get_raw(const size_t i) const {
        returned_cache_page_t<T> ret;
        uint32_t lane = lane_id();
        int64_t r = find_range(i);
        auto r_ = d_ranges+r;


        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t eq_mask;
            int master;
            uint64_t base_master;
            uint32_t count;
            uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);

            coalesce_page(lane, mask, r, page, gaddr, false, eq_mask, master, count, base_master);



            ret.addr = (T*) r_->get_cache_page_addr(base_master);
            ret.size = r_->get_sector_size()/sizeof(T);
            ret.offset = subindex/sizeof(T);
            //ret.page = page;
            __syncwarp(mask);


        }
        return ret;
    }
    __forceinline__
    __device__
    void release_raw(const size_t i) const {
        uint32_t lane = lane_id();
        int64_t r = find_range(i);
        auto r_ = d_ranges+r;


        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t eq_mask;
            int master;
            uint64_t base_master;
            uint32_t count;
            uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);

            uint32_t active_cnt = __popc(mask);
            eq_mask = __match_any_sync(mask, gaddr);
            eq_mask &= __match_any_sync(mask, (uint64_t)this);
            master = __ffs(eq_mask) - 1;
            count = __popc(eq_mask);
            if (master == lane)
                r_->release_page(page, count);
            __syncwarp(mask);



        }
    }

    __forceinline__
    __device__
    void* acquire_page_(const size_t i, data_page_t*& page_, size_t& start, size_t& end, range_d_t<T>* r_, const size_t page) const {
        //uint32_t lane = lane_id();



        void* ret = nullptr;
        page_ = nullptr;
        if (r_) {
            //uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);
            page_cache_d_t* pc = &(r_->cache);
            uint32_t ctrl = 0;//pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
            uint32_t queue = get_smid() % (pc->d_ctrls[0]->n_qps);
            uint64_t base_master = r_->acquire_page(page, 1, false, ctrl, queue);
            //coalesce_page(lane, mask, r, page, gaddr, false, eq_mask, master, count, base_master);

            page_ = &r_->pages[base_master];


            ret = (void*)r_->get_cache_page_addr(base_master);
            start = r_->n_elems_per_page * page;
            end = start +r_->n_elems_per_page;// * (page+1);
            //ret.page = page;

        }
        return ret;
    }
    __forceinline__
    __device__
    void* acquire_page(const size_t i, data_page_t*& page_, size_t& start, size_t& end, int64_t& r) const {
        uint32_t lane = lane_id();
        r = find_range(i);
        auto r_ = d_ranges+r;

        void* ret = nullptr;
        page_ = nullptr;
        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t eq_mask;
            int master;
            uint64_t base_master;
            uint32_t count;
            uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);

            coalesce_page(lane, mask, r, page, gaddr, false, eq_mask, master, count, base_master);
            page_ = &r_->pages[base_master];


            ret = (void*)r_->get_cache_page_addr(base_master);
            start = r_->n_elems_per_page * page;
            end = start +r_->n_elems_per_page;// * (page+1);
            //ret.page = page;
            __syncwarp(mask);
        }
        return ret;
    }

    __forceinline__
    __device__
    void release_page(data_page_t* page_, const int64_t r, const size_t i) const {
        uint32_t lane = lane_id();
        auto r_ = d_ranges+r;

        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t eq_mask;
            int master;
            uint32_t count;
            uint64_t page = r_->get_page(i);
            uint64_t gaddr = r_->get_global_address(page);

            uint32_t active_cnt = __popc(mask);
            eq_mask = __match_any_sync(mask, gaddr);
            eq_mask &= __match_any_sync(mask, (uint64_t)this);
            master = __ffs(eq_mask) - 1;
            count = __popc(eq_mask);
            if (master == lane)
                r_->release_page(page, count);
            __syncwarp(mask);



        }
    }

    __forceinline__
    __device__
    T seq_read(const size_t i) const {
        uint32_t lane = lane_id();
        int64_t r = find_range(i);
        auto r_ = d_ranges+r;
        T ret;

        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t eq_mask;
            int master;
            uint64_t base_master;
            uint32_t count;
            uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);

            coalesce_page(lane, mask, r, page, gaddr, false, eq_mask, master, count, base_master);

            //if (threadIdx.x == 63) {
            ////printf("--tid: %llu\tpage: %llu\tsubindex: %llu\tbase_master: %llu\teq_mask: %x\tmaster: %llu\n", (unsigned long long) threadIdx.x, (unsigned long long) page, (unsigned long long) subindex, (unsigned long long) base_master, (unsigned) eq_mask, (unsigned long long) master);
            //}
            ret = ((T*)(r_->get_cache_page_addr(base_master)+subindex))[0];
            __syncwarp(eq_mask);
            if (master == lane)
                r_->release_page(page, count);
            __syncwarp(mask);

        }
        return ret;
    }
    __forceinline__
    __device__
    void seq_write(const size_t i, const T val) const {
        uint32_t lane = lane_id();
        int64_t r = find_range(i);
        auto r_ = d_ranges+r;


        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t eq_mask;
            int master;
            uint64_t base_master;
            uint32_t count;
            uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);
            uint64_t gaddr = r_->get_global_address(page);

            coalesce_page(lane, mask, r, page, gaddr, true, eq_mask, master, count, base_master);

            //if (threadIdx.x == 63) {
            ////printf("--tid: %llu\tpage: %llu\tsubindex: %llu\tbase_master: %llu\teq_mask: %x\tmaster: %llu\n", (unsigned long long) threadIdx.x, (unsigned long long) page, (unsigned long long) subindex, (unsigned long long) base_master, (unsigned) eq_mask, (unsigned long long) master);
            //}
            ((T*)(r_->get_cache_page_addr(base_master)+subindex))[0] = val;
            __syncwarp(eq_mask);
            if (master == lane)
                r_->release_page(page, count);
            __syncwarp(mask);

        }
    }
    __forceinline__
    __device__
    T operator[](size_t i) const {
        return seq_read(i);
        // size_t k = 0;
        // bool found = false;
        // for (; k < n_ranges; k++) {
        //     if ((d_ranges[k].index_start <= i) && (d_ranges[k].index_end > i)) {
        //         found = true;
        //         break;
        //     }

        // }
        // if (found)
        //     return (((d_ranges[k]))[i-d_ranges[k].index_start]);
    }
    __forceinline__
    __device__
    void operator()(size_t i, T val) const {
        seq_write(i, val);
        // size_t k = 0;
        // bool found = false;
        // uint32_t mask = __activemask();
        // for (; k < n_ranges; k++) {
        //     if ((d_ranges[k].index_start <= i) && (d_ranges[k].index_end > i)) {
        //         found = true;
        //         break;
        //     }
        // }
        // __syncwarp(mask);
        // if (found)
        //     ((d_ranges[k]))(i-d_ranges[k].index_start, val);
    }


    __forceinline__
    __device__
    T AtomicAdd(const size_t i, const T val) const {
        //uint64_t tid = threadIdx.x + blockIdx.x * blockDim.x;
        uint32_t lane = lane_id();
        int64_t r = find_range(i);
        auto r_ = d_ranges+r;

        T old_val = 0;

        uint32_t ctrl;
        uint32_t queue;

        if (r != -1) {
#ifndef __CUDACC__
            uint32_t mask = 1;
#else
            uint32_t mask = __activemask();
#endif
            uint32_t leader = __ffs(mask) - 1;
            if (lane == leader) {
                page_cache_d_t* pc = &(r_->cache);
                ctrl = pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
                queue = get_smid() % (pc->d_ctrls[ctrl]->n_qps);
            }
            ctrl = __shfl_sync(mask, ctrl, leader);
            queue = __shfl_sync(mask, queue, leader);

            uint64_t page = r_->get_page(i);
            uint64_t subindex = r_->get_subindex(i);


            uint64_t gaddr = r_->get_global_address(page);
            //uint64_t p_s = r_->page_size;

            uint32_t active_cnt = __popc(mask);
            uint32_t eq_mask = __match_any_sync(mask, gaddr);
            eq_mask &= __match_any_sync(mask, (uint64_t)this);
            int master = __ffs(eq_mask) - 1;
            uint64_t base_master;
            uint64_t base;
            //bool memcpyflag_master;
            //bool memcpyflag;
            uint32_t count = __popc(eq_mask);
            if (master == lane) {
                base = r_->acquire_page(page, count, true, ctrl, queue);
                base_master = base;
                //    //printf("++tid: %llu\tbase: %llu  memcpyflag_master:%llu\n", (unsigned long long) threadIdx.x, (unsigned long long) base_master, (unsigned long long) memcpyflag_master);
            }
            base_master = __shfl_sync(eq_mask,  base_master, master);

            //if (threadIdx.x == 63) {
            ////printf("--tid: %llu\tpage: %llu\tsubindex: %llu\tbase_master: %llu\teq_mask: %x\tmaster: %llu\n", (unsigned long long) threadIdx.x, (unsigned long long) page, (unsigned long long) subindex, (unsigned long long) base_master, (unsigned) eq_mask, (unsigned long long) master);
            //}
            // ((T*)(base_master+subindex))[0] = val;
            old_val = atomicAdd((T*)(r_->get_cache_page_addr(base_master)+subindex), val);
            // //printf("AtomicAdd: tid: %llu\tpage: %llu\tsubindex: %llu\tval: %llu\told_val: %llu\tbase_master: %llx\n",
            //        (unsigned long long) tid, (unsigned long long) page, (unsigned long long) subindex, (unsigned long long) val,
            //     (unsigned long long) old_val, (unsigned long long) base_master);
            __syncwarp(eq_mask);
            if (master == lane)
                r_->release_page(page, count);
            __syncwarp(mask);
        }

        return old_val;
    }




};

/* array_t<T>: 호스트 측 배열 관리 구조체.
 * 여러 range를 합쳐 하나의 논리 배열을 구성하고, GPU 메모리에 array_d_t를 복사한다.
 * bafs_ptr가 이 구조체를 통해 SSD 데이터에 투명하게 접근한다.
 */
template<typename T>
struct array_t {
    array_d_t<T> adt;              // 디바이스 측 배열 메타데이터

    array_d_t<T>* d_array_ptr;     // GPU 메모리에 복사된 array_d_t 포인터



    BufferPtr d_array_buff;        // array_d_t GPU 복사본 버퍼
    BufferPtr d_ranges_buff;       // range 포인터 배열 GPU 버퍼
    BufferPtr d_d_ranges_buff;     // range_d_t 배열 GPU 버퍼

    void print_reset_stats(void) {
        std::vector<range_d_t<T>> rdt(adt.n_ranges);
        //range_d_t<T>* rdt = new range_d_t<T>[adt.n_ranges];
        cuda_err_chk(cudaMemcpy(rdt.data(), adt.d_ranges, adt.n_ranges*sizeof(range_d_t<T>), cudaMemcpyDeviceToHost));
        for (size_t i = 0; i < adt.n_ranges; i++) {

            std::cout << std::dec << "#READ IOs: "  << rdt[i].read_io_cnt 
                                  << "\t#Accesses:" << rdt[i].access_cnt
                                  << "\t#Misses:"   << rdt[i].miss_cnt 
                                  << "\tMiss Rate:" << ((float)rdt[i].miss_cnt/rdt[i].access_cnt)
                                  << "\t#Hits: "    << rdt[i].hit_cnt 
                                  << "\tHit Rate:"  << ((float)rdt[i].hit_cnt/rdt[i].access_cnt) 
                                  << "\tCLSize:"    << rdt[i].page_size 
                                  << std::endl;
            std::cout << "*********************************" << std::endl;
            rdt[i].read_io_cnt = 0;
            rdt[i].access_cnt = 0;
            rdt[i].miss_cnt = 0;
            rdt[i].hit_cnt = 0;
        }
        cuda_err_chk(cudaMemcpy(adt.d_ranges, rdt.data(), adt.n_ranges*sizeof(range_d_t<T>), cudaMemcpyHostToDevice));
    }

    array_t(const uint64_t num_elems, const uint64_t disk_start_offset, const std::vector<range_t<T>*>& ranges, uint32_t cudaDevice) {
        adt.n_elems = num_elems;
        adt.start_offset = disk_start_offset;

        adt.n_ranges = ranges.size();
        d_array_buff = createBuffer(sizeof(array_d_t<T>), cudaDevice);
        d_array_ptr = (array_d_t<T>*) d_array_buff.get();

        //d_ranges_buff = createBuffer(n_ranges * sizeof(range_t<T>*), cudaDevice);
        d_d_ranges_buff = createBuffer(adt.n_ranges * sizeof(range_d_t<T>), cudaDevice);
        adt.d_ranges = (range_d_t<T>*)d_d_ranges_buff.get();
        //d_ranges = (range_t<T>**) d_ranges_buff.get();
        for (size_t k = 0; k < adt.n_ranges; k++) {
            //cuda_err_chk(cudaMemcpy(d_ranges+k, &(ranges[k]->d_range_ptr), sizeof(range_t<T>*), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(adt.d_ranges+k, (ranges[k]->d_range_ptr), sizeof(range_d_t<T>), cudaMemcpyDeviceToDevice));
        }

        cuda_err_chk(cudaMemcpy(d_array_ptr, &adt, sizeof(array_d_t<T>), cudaMemcpyHostToDevice));
    }

};

__forceinline__
__device__
cache_page_t* page_cache_d_t::get_cache_page(const uint32_t page) const {
    return &this->cache_pages[page];
}



/* find_slot: 새 SSD 페이지를 위한 캐시 슬롯을 찾는 핵심 함수.
 * Clock/FIFO 교체 정책으로 캐시 슬롯을 순환 탐색한다.
 *
 * 동작:
 * 1) page_ticket을 atomic 증가시켜 다음 후보 슬롯을 선택한다
 * 2) 슬롯이 FREE이면 즉시 할당한다
 * 3) 슬롯이 UNLOCKED이면 eviction을 시도한다:
 *    a) 이전 매핑의 data_page_t 참조 카운트가 0이고 BUSY가 아니면 eviction 가능
 *    b) DIRTY 페이지는 SSD에 write-back한 후 eviction
 *    c) 이전 data_page_t의 VALID/DIRTY 비트를 클리어하여 INVALID로 전환
 * 4) 실패하면 다른 슬롯으로 재시도
 *
 * address: range 내 페이지 번호
 * range_id: 소속 range ID
 * queue_: 사용할 NVMe 큐 번호
 * 반환값: 할당된 캐시 슬롯 번호
 */
__forceinline__
__device__
uint32_t page_cache_d_t::find_slot(uint64_t address, uint64_t range_id, const uint32_t queue_) {
    bool fail = true;
    uint64_t count = 0;
    // 전역 주소 인코딩: 페이지 번호와 range_id를 하나의 값으로 합친다
    uint64_t global_address =(uint64_t) ((address << n_ranges_bits) | range_id);
    uint32_t page = 0;
    unsigned int ns = 8;
	uint64_t j = 0;
    uint64_t expected_state = VALID;
    uint64_t new_expected_state = 0;

    do {

//	if (++count %100000 == 0)
//		printf("here\tc: %llu\n", (unsigned long long) count);

        //if (count < this->n_pages)
        // Clock 포인터를 atomic 증가시켜 다음 후보 슬롯을 선택한다 (순환)
        page = page_ticket->fetch_add(1, simt::memory_order_relaxed)  % (this->n_pages);
        //page = page_ticket->fetch_add(1, simt::memory_order_relaxed);
        //if (page < (n_cachelines_for_states*STATES_PER_CACHELINE)) {
        //    page = (page/n_cachelines_for_states) + ((page%n_cachelines_for_states)*STATES_PER_CACHELINE);
        //}
        //uint64_t unlocked = UNLOCKED;

        // uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
        ////printf("tid: %llu page: %llu\n", tid, page);

        bool lock = false;
        // 슬롯의 잠금 상태를 읽는다
        uint32_t v = this->cache_pages[page].page_take_lock.load(simt::memory_order_relaxed);
        // === FREE 슬롯: 아직 사용되지 않은 빈 슬롯 ===
        if ( v == FREE ) {
            // CAS로 FREE→LOCKED 전환을 시도한다
            lock = this->cache_pages[page].page_take_lock.compare_exchange_weak(v, LOCKED, simt::memory_order_acquire, simt::memory_order_relaxed);
            if ( lock ) {
                // 성공: 전역 주소를 기록하고 UNLOCKED로 전환한다
                this->cache_pages[page].page_translation = global_address;
                this->cache_pages[page].page_take_lock.store(UNLOCKED, simt::memory_order_release);
                fail = false;
            }
        }
        // === UNLOCKED 슬롯: 이미 매핑되어 있지만 잠기지 않음 (eviction 대상) ===
        else if ( v == UNLOCKED ) {

            lock = this->cache_pages[page].page_take_lock.compare_exchange_weak(v, LOCKED, simt::memory_order_acquire, simt::memory_order_relaxed);
            if (lock) {
                //uint32_t previous_address = this->cache_pages[page].page_translation;
                uint64_t previous_global_address = this->cache_pages[page].page_translation;
                //uint8_t previous_range = this->cache_pages[page].range_id;
                uint64_t previous_range = previous_global_address & n_ranges_mask;
                uint64_t previous_address = previous_global_address >> n_ranges_bits;
                //uint32_t new_state = BUSY;
                //if ((previous_range >= range_cap) || (previous_address >= n_pages))
                //    //printf("prev_ga: %llu\tprev_range: %llu\tprev_add: %llu\trange_cap: %llu\tn_pages: %llu\n", (unsigned long long) previous_global_address, (unsigned long long) previous_range, (unsigned long long) previous_address,
                //           (unsigned long long) range_cap, (unsigned long long) n_pages);
                expected_state = this->ranges[previous_range][previous_address].state.load(simt::memory_order_relaxed);

                uint32_t cnt = expected_state & CNT_MASK;
                uint32_t b = expected_state & BUSY;
                if ((cnt == 0) && (b == 0) ) {
                    new_expected_state = this->ranges[previous_range][previous_address].state.fetch_or(BUSY, simt::memory_order_acquire);
                    if (((new_expected_state & BUSY ) == 0) ) {
                        //while ((new_expected_state & CNT_MASK ) != 0) new_expected_state = this->ranges[previous_range][previous_address].state.load(simt::memory_order_acquire);
                        if (((new_expected_state & CNT_MASK ) == 0) ) {
                            if ((new_expected_state & DIRTY)) {
                                uint64_t ctrl = get_backing_ctrl_(previous_address, n_ctrls, ranges_dists[previous_range]);
                                //uint64_t get_backing_page(const uint64_t page_start, const size_t page_offset, const uint64_t n_ctrls, const data_dist_t dist) {
                                uint64_t index = get_backing_page_(ranges_page_starts[previous_range], previous_address, n_ctrls, ranges_dists[previous_range]);
                                // //printf("Eviciting range_id: %llu\tpage_id: %llu\tctrl: %llx\tindex: %llu\n",
                                //        (unsigned long long) previous_range, (unsigned long long)previous_address,
                                //        (unsigned long long) ctrl, (unsigned long long) index);
                                if (ctrl == ALL_CTRLS) {
                                    for (ctrl = 0; ctrl < n_ctrls; ctrl++) {
                                        Controller* c = this->d_ctrls[ctrl];
                                        uint32_t queue = queue_ % (c->n_qps);
                                        write_data(this, (c->d_qps)+queue, (index*this->n_blocks_per_page), this->n_blocks_per_page, page);
                                    }
                                }
                                else {

                                    Controller* c = this->d_ctrls[ctrl];
                                    uint32_t queue = queue_ % (c->n_qps);

                                    //index = ranges_page_starts[previous_range] + previous_address;


                                    write_data(this, (c->d_qps)+queue, (index*this->n_blocks_per_page), this->n_blocks_per_page, page);
                                }
                            }

                            fail = false;
                            this->ranges[previous_range][previous_address].state.fetch_and(CNT_MASK, simt::memory_order_release);
                        }
                        else { 
                            this->ranges[previous_range][previous_address].state.fetch_and(DISABLE_BUSY_MASK, simt::memory_order_release);
//if ((j % 1000000) == 0) {
//                printf("failed to find slot j: %llu\taddr: %llx\tpage: %llx\texpected_state: %llx\tnew_expected_date: %llx\n", (unsigned long long) j, (unsigned long long) address, (unsigned long long)page, (unsigned long long) expected_state, (unsigned long long) new_expected_state);
//}
                        }
                    }
                }

                //this->ranges[previous_range][previous_address].compare_exchange_strong(expected_state, new_state, simt::memory_order_acquire, simt::memory_order_relaxed);

                if (!fail) {
                    //this->cache_pages[page].page_translation = address;
                    //this->cache_pages[page].range_id = range_id;
//                    this->page_translation[page] = global_address;
                    this->cache_pages[page].page_translation = global_address;
                }
                //this->page_translation[page].store(global_address, simt::memory_order_release);
                this->cache_pages[page].page_take_lock.store(UNLOCKED, simt::memory_order_release);
            }


        }

        count++;
/*if (fail) {
  if ((++j % 1000000) == 0) {
  printf("failed to find slot j: %llu\n", (unsigned long long) j);
  }
  }*/
        if (fail) {
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
//             __nanosleep(ns);
//             if (ns < 256) {
//                 ns *= 2;
//             }
#endif
            //   if ((j % 10000000) == 0) {
            //     printf("failed to find slot j: %llu\taddr: %llx\tpage: %llx\texpected_state: %llx\tnew_expected_date: %llx\n", (unsigned long long) j, (unsigned long long) address, (unsigned long long)page, (unsigned long long) expected_state, (unsigned long long) new_expected_state);
//            }
//	   expected_state = 0;
//	   new_expected_state = 0;


        }

    } while(fail);
    return page;

}


/* poll_async: 비동기 I/O의 완료를 폴링하고 CQ/SQ를 정리한다 */
inline __device__ void poll_async(QueuePair* qp, uint16_t cid, uint16_t sq_pos) {
    uint32_t cq_pos = cq_poll(&qp->cq, cid);
    //sq_dequeue(&qp->sq, sq_pos);

    cq_dequeue(&qp->cq, cq_pos, &qp->sq);



    put_cid(&qp->sq, cid);
}

/* access_data_async: NVMe I/O 커맨드를 SQ에 인큐만 하고 완료를 기다리지 않는다 (비동기) */
inline __device__ void access_data_async(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry, const uint8_t opcode, uint16_t * cid, uint16_t* sq_pos) {
    nvm_cmd_t cmd;
    *cid = get_cid(&(qp->sq));
    ////printf("cid: %u\n", (unsigned int) cid);


    nvm_cmd_header(&cmd, *cid, opcode, qp->nvmNamespace);
    uint64_t prp1 = pc->prp1[pc_entry];
    uint64_t prp2 = 0;
    if (pc->prps)
        prp2 = pc->prp2[pc_entry];
    ////printf("tid: %llu\tstart_lba: %llu\tn_blocks: %llu\tprp1: %p\n", (unsigned long long) (threadIdx.x+blockIdx.x*blockDim.x), (unsigned long long) starting_lba, (unsigned long long) n_blocks, (void*) prp1);
    nvm_cmd_data_ptr(&cmd, prp1, prp2);
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);
    *sq_pos = sq_enqueue(&qp->sq, &cmd);



}

/* enqueue_second: 이중 읽기 최적화 - 첫 번째 읽기 완료 후 같은 데이터를 한 번 더 읽는다.
 * 목적: NVMe 컨트롤러의 prefetch/readahead 효과를 극대화하기 위해
 * 같은 LBA를 즉시 다시 읽어서 컨트롤러 내부 캐시 히트를 유도한다.
 * q_head/q_tail/q_lock으로 이중 읽기의 순서를 관리한다.
 */
inline __device__ void enqueue_second(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, nvm_cmd_t* cmd, const uint16_t cid, const uint64_t pc_pos, const uint64_t pc_prev_head) {
    nvm_cmd_rw_blks(cmd, starting_lba, 1);
    unsigned int ns = 8;
    do {
        //check if new head past pc_pos
        //cur_pc_head == new head
        //prev_pc_head == old head
        //pc_pos == position i wanna move the head past
        uint64_t cur_pc_head = pc->q_head->load(simt::memory_order_relaxed);
        //sec == true when cur_pc_head past pc_pos
        bool sec = ((cur_pc_head < pc_prev_head) && (pc_prev_head <= pc_pos)) ||
            ((pc_prev_head <= pc_pos) && (pc_pos < cur_pc_head)) ||
            ((pc_pos < cur_pc_head) && (cur_pc_head < pc_prev_head));

        if (sec) break;

        //if not
        uint64_t qlv = pc->q_lock->load(simt::memory_order_relaxed);
        //got lock
        if (qlv == 0) {
            qlv = pc->q_lock->fetch_or(1, simt::memory_order_acquire);
            if (qlv == 0) {
                uint64_t cur_pc_tail;// = pc->q_tail.load(simt::memory_order_acquire);

                uint16_t sq_pos = sq_enqueue(&qp->sq, cmd, pc->q_tail, &cur_pc_tail);
                uint32_t head, head_;
                uint32_t cq_pos = cq_poll(&qp->cq, cid, &head, &head_);

                pc->q_head->store(cur_pc_tail, simt::memory_order_release);
                pc->q_lock->store(0, simt::memory_order_release);
                pc->extra_reads->fetch_add(1, simt::memory_order_relaxed);
                cq_dequeue(&qp->cq, cq_pos, &qp->sq, head, head_);



                break;
            }
        }
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
         __nanosleep(ns);
         if (ns < 256) {
             ns *= 2;
         }
#endif
    } while(true);

}

/* read_data: SSD에서 캐시 슬롯으로 데이터를 읽는 동기 함수.
 * 1) CID를 할당한다
 * 2) NVM_IO_READ 커맨드를 빌드한다 (PRP1/PRP2 = 캐시 슬롯의 물리 주소)
 * 3) SQ에 인큐하고 도어벨을 울린다
 * 4) CQ에서 완료를 폴링한다
 * 5) 이중 읽기 최적화를 수행한다 (enqueue_second)
 * 6) CID를 반환한다
 */
inline __device__ void read_data(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry) {



    nvm_cmd_t cmd;
    // 사용 가능한 Command ID를 할당한다
    uint16_t cid = get_cid(&(qp->sq));
    ////printf("cid: %u\n", (unsigned int) cid);


    // NVMe Read 커맨드 헤더를 빌드한다 (CID, opcode, namespace)
    nvm_cmd_header(&cmd, cid, NVM_IO_READ, qp->nvmNamespace);
    // PRP1: 캐시 슬롯의 물리 주소 (NVMe가 DMA로 데이터를 기록할 목적지)
    uint64_t prp1 = pc->prp1[pc_entry];
    uint64_t prp2 = 0;
    if (pc->prps)
        prp2 = pc->prp2[pc_entry]; // 페이지가 크면 PRP2도 설정한다
    // 커맨드에 데이터 포인터(PRP1/PRP2)를 설정한다
    nvm_cmd_data_ptr(&cmd, prp1, prp2);
    // 읽을 LBA 범위를 설정한다
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);
    // SQ에 커맨드를 인큐하고 도어벨을 울린다
    uint16_t sq_pos = sq_enqueue(&qp->sq, &cmd);
    uint32_t head, head_;
    uint64_t pc_pos;
    uint64_t pc_prev_head;

    // CQ에서 이 CID의 완료를 폴링한다 (데이터 전송 완료까지 대기)
    uint32_t cq_pos = cq_poll(&qp->cq, cid, &head, &head_);

    // 이중 읽기 순서 추적: CQ tail 증가, 글로벌 큐 위치 기록
    qp->cq.tail.fetch_add(1, simt::memory_order_acq_rel);
    pc_prev_head = pc->q_head->load(simt::memory_order_relaxed);
    pc_pos = pc->q_tail->fetch_add(1, simt::memory_order_acq_rel);

    // CQ 엔트리를 디큐하고 SQ head를 업데이트한다
    cq_dequeue(&qp->cq, cq_pos, &qp->sq, head, head_);

    // 이중 읽기 최적화: 같은 데이터를 한 번 더 읽어 컨트롤러 캐시를 활용한다
    enqueue_second(pc, qp, starting_lba, &cmd, cid, pc_pos, pc_prev_head);

    // 사용 완료된 CID를 반환한다
    put_cid(&qp->sq, cid);


}


/* write_data: 캐시 슬롯의 데이터를 SSD에 기록하는 동기 함수 (write-back).
 * eviction 시 DIRTY 페이지를 SSD에 쓸 때 호출된다.
 * read_data와 동일한 흐름이지만 opcode가 NVM_IO_WRITE이고 이중 읽기는 수행하지 않는다.
 */
inline __device__ void write_data(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry) {
    //uint64_t starting_lba = starting_byte >> qp->block_size_log;
    //uint64_t rem_bytes = starting_byte & qp->block_size_minus_1;
    //uint64_t end_lba = CEIL((starting_byte+num_bytes), qp->block_size);

    //uint16_t n_blocks = CEIL(num_bytes, qp->block_size, qp->block_size_log);



    nvm_cmd_t cmd;
    uint16_t cid = get_cid(&(qp->sq));
    ////printf("cid: %u\n", (unsigned int) cid);


    nvm_cmd_header(&cmd, cid, NVM_IO_WRITE, qp->nvmNamespace);
    uint64_t prp1 = pc->prp1[pc_entry];
    uint64_t prp2 = 0;
    if (pc->prps)
        prp2 = pc->prp2[pc_entry];
    ////printf("tid: %llu\tstart_lba: %llu\tn_blocks: %llu\tprp1: %p\n", (unsigned long long) (threadIdx.x+blockIdx.x*blockDim.x), (unsigned long long) starting_lba, (unsigned long long) n_blocks, (void*) prp1);
    nvm_cmd_data_ptr(&cmd, prp1, prp2);
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);
    uint16_t sq_pos = sq_enqueue(&qp->sq, &cmd);
    uint32_t head, head_;
    uint64_t pc_pos;
    uint64_t pc_prev_head;

    uint32_t cq_pos = cq_poll(&qp->cq, cid, &head, &head_);
    qp->cq.tail.fetch_add(1, simt::memory_order_acq_rel);
    pc_prev_head = pc->q_head->load(simt::memory_order_relaxed);
    pc_pos = pc->q_tail->fetch_add(1, simt::memory_order_acq_rel);
    cq_dequeue(&qp->cq, cq_pos, &qp->sq, head, head_);
    //sq_dequeue(&qp->sq, sq_pos);




    put_cid(&qp->sq, cid);

}

/* access_data: 범용 NVMe I/O 함수. opcode로 읽기/쓰기를 구분한다.
 * read_data/write_data의 간단한 버전으로, 이중 읽기 최적화 없이 동기적으로 I/O를 수행한다.
 */
inline __device__ void access_data(page_cache_d_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry, const uint8_t opcode) {
    //uint64_t starting_lba = starting_byte >> qp->block_size_log;
    //uint64_t rem_bytes = starting_byte & qp->block_size_minus_1;
    //uint64_t end_lba = CEIL((starting_byte+num_bytes), qp->block_size);

    //uint16_t n_blocks = CEIL(num_bytes, qp->block_size, qp->block_size_log);



    nvm_cmd_t cmd;
    uint16_t cid = get_cid(&(qp->sq));
    ////printf("cid: %u\n", (unsigned int) cid);


    nvm_cmd_header(&cmd, cid, opcode, qp->nvmNamespace);
    uint64_t prp1 = pc->prp1[pc_entry];
    uint64_t prp2 = 0;
    if (pc->prps)
        prp2 = pc->prp2[pc_entry];
    ////printf("tid: %llu\tstart_lba: %llu\tn_blocks: %llu\tprp1: %p\n", (unsigned long long) (threadIdx.x+blockIdx.x*blockDim.x), (unsigned long long) starting_lba, (unsigned long long) n_blocks, (void*) prp1);
    nvm_cmd_data_ptr(&cmd, prp1, prp2);
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);
    uint16_t sq_pos = sq_enqueue(&qp->sq, &cmd);

    uint32_t cq_pos = cq_poll(&qp->cq, cid);
    cq_dequeue(&qp->cq, cq_pos, &qp->sq);
    //sq_dequeue(&qp->sq, sq_pos);




    put_cid(&qp->sq, cid);


}



//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#undef __forceinline__
//#endif


#endif // __PAGE_CACHE_H__
