/* nvm_parallel_queue.h - GPU 스레드 다수가 동시에 NVMe 큐를 사용하기 위한 lock-free 병렬 큐 연산
 *
 * BaM의 핵심 파일 중 하나이다. 수천 개의 GPU 스레드가 하나의 NVMe SQ/CQ 쌍을 동시에 사용할 때
 * 정확한 순서 보장과 데이터 일관성을 유지하면서도 높은 처리량을 달성하기 위한 lock-free 알고리즘을 구현한다.
 *
 * 핵심 메커니즘:
 * - tickets 배열: Lamport's bakery 알고리즘 변형으로 SQ 슬롯 접근 순서를 보장한다.
 *   각 슬롯에 티켓 번호가 할당되고, 자기 차례가 올 때까지 spin-wait한다.
 * - tail_mark/head_mark: 커맨드 기록/완료 처리가 끝났음을 표시하는 마커.
 *   리더 스레드가 연속된 완료 마크를 수집하여 도어벨을 한 번에 업데이트한다.
 * - in_ticket: 전역 순서 번호를 atomic하게 발급하여 큐 슬롯을 할당한다.
 * - cid 배열: 65536개 Command ID를 lock-free로 할당/해제한다.
 * - PTX asm "st.mmio.relaxed.sys.global.u32": GPU에서 PCIe MMIO 도어벨에 직접 쓰기한다.
 *
 * 모든 함수는 __device__ (GPU 커널 전용)로 선언되어 있다.
 */
#ifndef __NVM_PARALLEL_QUEUE_H_
#define __NVM_PARALLEL_QUEUE_H_

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif
#ifndef __forceinline__
#define __forceinline__ inline
#endif


//#ifndef __CUDACC__
//#define __device__
//#define __host__
//#define __forceinline__ inline
//#endif

#include "host_util.h"
#include "nvm_types.h"
#include "nvm_util.h"
#include <simt/atomic>

/* LOCKED/UNLOCKED: 마커(mark) 배열에서 사용하는 상태 값.
 * LOCKED(1): 해당 슬롯이 처리 완료 표시됨 (도어벨 업데이트 대기 중)
 * UNLOCKED(0): 해당 슬롯이 아직 미처리이거나 이미 도어벨에 반영됨
 */
#define LOCKED   1
#define UNLOCKED 0

/* get_id: 티켓 번호에서 해당 슬롯의 "세대(generation)" 번호를 계산한다.
 * 큐 크기가 2^y일 때, ticket >> y 는 해당 슬롯이 몇 번째 회전에 사용되는지를 나타낸다.
 * *2를 하는 이유: 인큐 시 id를 한 번 사용하고, 디큐 시 +1로 한 번 더 사용하여
 * 같은 슬롯의 인큐/디큐를 순서대로 직렬화한다.
 */
__forceinline__ __device__ uint64_t get_id(uint64_t x, uint64_t y) {
    //return (x >> y);
    return (x >> y) * 2;  // (x/2^y) *2: 인큐와 디큐 각각에 하나의 세대 번호를 할당한다
}



/* get_cid: 사용 가능한 Command ID를 lock-free 방식으로 할당한다.
 * NVMe에서 각 커맨드는 고유한 CID가 필요하다 (CQ에서 완료를 매칭하기 위해).
 * 65536개의 CID 풀에서 UNLOCKED 상태인 것을 atomic fetch_or로 선점한다.
 * cid_ticket을 atomic 증가시켜 CID 풀을 순회하면서 빈 슬롯을 찾는다.
 */
inline __device__
uint16_t get_cid(nvm_queue_t* sq) {
    bool not_found = true;
    uint16_t id;

    do {
        // cid_ticket을 atomic 증가시켜 다음 후보 CID 인덱스를 얻는다 (16비트 wrap-around)
        id = sq->cid_ticket.fetch_add(1, simt::memory_order_relaxed) & (65535);
        // 해당 CID 슬롯을 LOCKED로 atomic하게 설정하여 선점을 시도한다
        // fetch_or의 반환값이 이전 상태: UNLOCKED(0)이었으면 선점 성공, LOCKED(1)이었으면 이미 사용 중
        uint64_t old = sq->cid[id].val.fetch_or(LOCKED, simt::memory_order_acquire);
        not_found = old == LOCKED; // 이미 LOCKED이면 다른 스레드가 사용 중이므로 재시도
    } while (not_found);


    return id;

}

/* put_cid: 사용이 끝난 Command ID를 반환한다.
 * CID 슬롯을 UNLOCKED로 설정하여 다른 스레드가 재사용할 수 있게 한다.
 */
inline __device__
void put_cid(nvm_queue_t* sq, uint16_t id) {
    sq->cid[id].val.store(UNLOCKED, simt::memory_order_release);
}

/* move_tail: SQ tail을 가능한 만큼 전진시킨다.
 * 리더 스레드가 호출하여, cur_tail부터 연속으로 LOCKED된 tail_mark를 수집한다.
 * 각 LOCKED 마크를 UNLOCKED로 교환하면서 카운트를 세고,
 * 연속이 끊기거나 큐가 가득 차면 멈춘다.
 * 반환값: tail을 전진시킬 수 있는 엔트리 수
 *
 * 이 함수의 핵심: 여러 스레드가 각자 자기 슬롯에 커맨드를 기록하고 tail_mark를 LOCKED로 설정하면,
 * 리더 스레드가 연속된 마크를 모아서 도어벨을 한 번에 업데이트한다.
 * 이를 통해 도어벨 쓰기 횟수를 최소화하여 PCIe 대역폭을 절약한다.
 */
inline __device__
uint32_t move_tail(nvm_queue_t* q, uint32_t cur_tail) {
    uint32_t count = 0;




    bool pass = true;
    while (pass ) {
        // 큐가 가득 찼는지 확인: (cur_tail+count+1)이 head와 같으면 full
        pass = (((cur_tail+count+1) & q->qs_minus_1) != (q->head.load(simt::memory_order_relaxed) & q->qs_minus_1 ));
        if (pass) {
            // tail_mark[pos]를 UNLOCKED로 교환: 이전 값이 LOCKED이면 이 슬롯은 기록 완료됨
            pass = ((q->tail_mark[(cur_tail+count)&q->qs_minus_1].val.exchange(UNLOCKED, simt::memory_order_relaxed)) == LOCKED);
            if (pass)
                count++; // 연속으로 기록 완료된 슬롯 수를 증가시킨다
        }

    }

    // head_lock 카운터를 증가시켜 SQ 상태 업데이트 완료를 알린다
    q->head_lock.fetch_add(1, simt::memory_order_acq_rel);
    return (count);
}

/* move_head_cq: CQ head를 가능한 만큼 전진시키고, 대응하는 SQ head도 업데이트한다.
 * CQ에서 완료 처리된 엔트리들의 head_mark를 수집하여 연속 처리된 수를 반환한다.
 * 마지막 CQ 엔트리에서 SQ head 포인터(SQ Head Pointer 필드)를 읽어 SQ의 head를 갱신한다.
 * 이를 통해 SQ 슬롯이 재사용 가능하게 된다.
 */
inline __device__
uint32_t move_head_cq(nvm_queue_t* q, uint32_t cur_head, nvm_queue_t* sq) {
    uint32_t count = 0;
    (void) sq;

    bool pass = true;
    while (pass) {
        uint32_t loc = (cur_head+count++)&q->qs_minus_1;
        // head_mark[loc]를 UNLOCKED로 교환: LOCKED이면 이 위치의 완료가 처리됨
        pass = (q->head_mark[loc].val.exchange(UNLOCKED, simt::memory_order_relaxed)) == LOCKED;

    }
    // 마지막 교환이 실패한 것이므로 1을 빼서 실제 처리된 수를 구한다
    count -= 1;
    if (count) {
        // 마지막으로 처리된 CQ 엔트리에서 SQ Head Pointer를 읽는다
        // NVMe CQ 엔트리의 dword[2] 하위 16비트가 SQ Head Pointer이다
        uint32_t loc_ = (cur_head + (count -1)) & q->qs_minus_1;
        uint32_t cpl_entry = ((nvm_cpl_t*)q->vaddr)[loc_].dword[2];
        uint16_t new_sq_head =  (cpl_entry & 0x0000ffff); // SQ Head Pointer 추출
        uint32_t sq_move_count = 0;
        uint32_t cur_sq_head = sq->head.load(simt::memory_order_relaxed);
        uint32_t loc = cur_sq_head & sq->qs_minus_1;

        // SQ head를 new_sq_head까지 전진시키면서 각 슬롯의 티켓을 증가시킨다
        if (loc != new_sq_head) {
            for (; loc != new_sq_head; sq_move_count++, loc= ((loc+1)  & sq->qs_minus_1)) {
                // 해당 SQ 슬롯의 티켓을 +1하여 다음 세대에서 재사용 가능하게 한다
                sq->tickets[loc].val.fetch_add(1, simt::memory_order_relaxed);
            }

            // SQ head를 한꺼번에 전진시킨다
            sq->head.fetch_add(sq_move_count, simt::memory_order_acq_rel);
        }
    }
    return (count);

}

/* clean_cids: 처리된 CQ 엔트리들의 Command ID를 일괄 반환한다.
 * (현재는 cq_dequeue에서 직접 처리하므로 사용되지 않는 코드)
 */
inline __device__
void clean_cids(nvm_queue_t* cq, nvm_queue_t* sq, uint32_t count) {
    for (size_t i  = 0; i < count; i++) {
        put_cid(sq, cq->clean_cid[i]);
    }
}

/* move_head_sq: SQ head를 가능한 만큼 전진시킨다.
 * CQ 처리 후 SQ 슬롯을 해제할 때 사용한다.
 * head_mark가 LOCKED인 연속된 슬롯 수를 세고, 각 슬롯의 티켓을 증가시켜
 * 해당 슬롯이 다음 세대에서 재사용될 수 있게 한다.
 */
inline __device__
uint32_t move_head_sq(nvm_queue_t* q, uint32_t cur_head) {
    uint32_t count = 0;

    bool pass = true;
    while (pass) {

        uint64_t loc = (cur_head + count)&q->qs_minus_1;
        // head_mark[loc]를 UNLOCKED로 교환: LOCKED이면 이 슬롯의 디큐가 완료됨
        pass = (q->head_mark[loc].val.exchange(UNLOCKED, simt::memory_order_relaxed)) == LOCKED;
        if (pass) {
            // 해당 슬롯의 티켓을 +1하여 인큐 세대를 닫는다 (다음 인큐가 대기에서 풀림)
            q->tickets[loc].val.fetch_add(1, simt::memory_order_relaxed);

            count++;


        }


    }
    return (count);

}

/* ulonglong4 = 32바이트: NVMe 커맨드(64바이트)를 2회 복사하기 위한 타입 */
typedef ulonglong4 copy_type;

/* sq_enqueue: SQ에 NVMe 커맨드를 lock-free로 인큐한다.
 *
 * 이것이 BaM 병렬 큐의 핵심 함수이다. 수천 개의 GPU 스레드가 동시에 호출한다.
 *
 * 알고리즘 순서:
 * 1) in_ticket을 atomic 증가시켜 전역 순서 번호(ticket)를 받는다
 * 2) ticket으로 슬롯 위치(pos)와 세대 번호(id)를 계산한다
 * 3) tickets[pos]가 자신의 id가 될 때까지 spin-wait한다 (순서 보장)
 * 4) 큐 메모리에 NVMe 커맨드를 복사한다 (ulonglong4 단위로 2회)
 * 5) tail_mark[pos]를 LOCKED로 설정하여 기록 완료를 표시한다
 * 6) 리더 선출: tail_lock을 잡은 스레드가 연속 마크를 수집하고 도어벨을 업데이트한다
 * 7) 자신의 tail_mark가 UNLOCKED가 될 때까지 대기한다 (도어벨 반영 완료 대기)
 * 8) tickets[pos]를 +1하여 디큐 세대를 열고 반환한다
 *
 * pc_tail: page cache tail 포인터 (read_data에서 이중 읽기 최적화에 사용, NULL이면 무시)
 */
inline __device__
uint16_t sq_enqueue(nvm_queue_t* sq, nvm_cmd_t* cmd, simt::atomic<uint64_t, simt::thread_scope_device>* pc_tail =NULL, uint64_t * cur_pc_tail=NULL) {

    uint32_t ticket;
    // 단계 1: 전역 순서 번호를 atomic하게 발급받는다
    ticket = sq->in_ticket.fetch_add(1, simt::memory_order_relaxed);

    // 단계 2: 티켓에서 큐 슬롯 위치와 세대 번호를 계산한다
    uint32_t pos = ticket & (sq->qs_minus_1);    // 슬롯 위치 = ticket mod queue_size
    uint64_t id = get_id(ticket, sq->qs_log2);   // 세대 번호 = (ticket / queue_size) * 2

    // 단계 3a: relaxed 순서로 먼저 티켓 값이 맞는지 빠르게 확인한다 (spin-wait 최적화)
    unsigned int ns = 8;
    while ((sq->tickets[pos].val.load(simt::memory_order_relaxed) != id) ) {
        // 지수 백오프 nanosleep으로 바쁜 대기의 전력 소비를 줄인다
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
        __nanosleep(ns);
        if (ns < 256) {
            ns *= 2; // 대기 시간을 점진적으로 늘린다 (지수 백오프)
        }
#endif
    }

    // 단계 3b: acquire 순서로 다시 확인하여 이전 쓰기가 모두 보이도록 한다
    ns = 8;
    while ((sq->tickets[pos].val.load(simt::memory_order_acquire) != id) ) {
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
        __nanosleep(ns);
        if (ns < 256) {
            ns *= 2;
        }
#endif
    }

    // 단계 4: NVMe 커맨드(64바이트)를 큐 메모리 슬롯에 복사한다
    // ulonglong4(32바이트) 단위로 2회 반복하여 64바이트를 복사한다
    copy_type* queue_loc = ((copy_type*)(((nvm_cmd_t*)(sq->vaddr)) + pos));
    copy_type* cmd_ = ((copy_type*)(cmd->dword));

#pragma unroll
    for (uint32_t i = 0; i < 64/sizeof(copy_type); i++) {
        queue_loc[i] = cmd_[i]; // 32바이트씩 2회 = 64바이트 복사
    }




    // page cache tail 스냅샷을 저장한다 (이중 읽기 최적화용)
    if (pc_tail) {
        *cur_pc_tail = pc_tail->load(simt::memory_order_relaxed);
    }
    // 단계 5: tail_mark[pos]를 LOCKED로 설정하여 이 슬롯에 커맨드 기록이 완료되었음을 표시한다
    sq->tail_mark[pos].val.store(LOCKED, simt::memory_order_release);

    // 단계 6: 리더 선출 및 도어벨 업데이트
    // 자신의 tail_mark가 아직 LOCKED(도어벨에 반영되지 않음)이면 리더 역할을 시도한다
    bool cont = true;
    ns = 8;
    cont = sq->tail_mark[pos].val.load(simt::memory_order_relaxed) == LOCKED;
    while(cont) {
        // tail_lock을 잡을 수 있는지 확인한다 (lock-free 리더 선출)
        bool new_cont = sq->tail_lock.load(simt::memory_order_relaxed) == LOCKED;
        if (!new_cont) {
            // tail_lock을 atomic하게 잡는다 (fetch_or로 CAS 대체)
            new_cont = sq->tail_lock.fetch_or(LOCKED, simt::memory_order_acquire) == LOCKED;
            if(!new_cont) {
                // 리더가 되었다: 현재 tail 위치에서 연속 완료된 엔트리를 수집한다
                uint32_t cur_tail = sq->tail.load(simt::memory_order_relaxed);

                // move_tail: 연속으로 LOCKED된 tail_mark를 수집하여 전진 가능한 수를 반환
                uint32_t tail_move_count = move_tail(sq, cur_tail);

                if (tail_move_count) {
                    uint32_t new_tail = cur_tail + tail_move_count;
                    uint32_t new_db = (new_tail) & (sq->qs_minus_1); // 도어벨 값 계산
                    // page cache tail을 다시 읽는다 (도어벨 쓰기 직전의 최신 값)
                    if (pc_tail) {
                        *cur_pc_tail = pc_tail->load(simt::memory_order_acquire);
                    }
                    // PTX 인라인 어셈블리로 PCIe MMIO 도어벨에 직접 쓴다
                    // st.mmio.relaxed.sys.global.u32: GPU에서 MMIO 레지스터에 store하는 특수 명령어
		    asm volatile ("st.mmio.relaxed.sys.global.u32 [%0], %1;" :: "l"(sq->db),"r"(new_db) : "memory");

                    // 전역 tail 포인터를 업데이트한다
                    sq->tail.store(new_tail, simt::memory_order_release);
                }
                // tail_lock을 해제한다
                sq->tail_lock.store(UNLOCKED, simt::memory_order_release);
            }
        }
        // 단계 7: 자신의 tail_mark가 UNLOCKED가 되었는지 확인한다 (도어벨 반영 완료)
        cont = sq->tail_mark[pos].val.load(simt::memory_order_relaxed) == LOCKED;
        if (cont) {
            // 아직 반영되지 않았으면 지수 백오프로 대기한다
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
            __nanosleep(ns);
            if (ns < 256) {
                ns *= 2;
            }
#endif
        }

    }



    // 단계 8: tickets[pos]를 +1하여 이 슬롯의 디큐(head 이동) 세대를 연다
    // 이 슬롯을 기다리고 있던 sq_dequeue 스레드가 진행할 수 있게 된다
    sq->tickets[pos].val.fetch_add(1, simt::memory_order_acq_rel);
    return pos;

}

/* sq_dequeue: SQ 슬롯을 해제한다 (CQ에서 완료를 받은 후 호출).
 *
 * 완료된 SQ 슬롯의 head_mark를 LOCKED로 설정하고,
 * 리더 스레드가 연속 처리된 슬롯을 수집하여 SQ head를 전진시킨다.
 * SQ head가 전진하면 해당 슬롯들의 티켓이 증가하여 다음 인큐에서 재사용 가능해진다.
 */
inline __device__
void sq_dequeue(nvm_queue_t* sq, uint16_t pos) {

    // 이 슬롯의 head_mark를 LOCKED로 설정하여 디큐 완료를 표시한다
    sq->head_mark[pos].val.store(LOCKED, simt::memory_order_relaxed);
    bool cont = true;
    unsigned int ns = 8;
    cont = sq->head_mark[pos].val.load(simt::memory_order_relaxed) == LOCKED;
    while (cont) {
            // head_lock을 잡아서 리더 역할을 시도한다
            bool new_cont = sq->head_lock.exchange(LOCKED, simt::memory_order_acquire) == LOCKED;
            if (!new_cont){
                // 리더: 현재 head에서 연속 LOCKED된 head_mark를 수집한다
                uint32_t cur_head = sq->head.load(simt::memory_order_relaxed);;

                uint32_t head_move_count = move_head_sq(sq, cur_head);
                if (head_move_count) {
                    // SQ head를 전진시킨다 (해당 슬롯들이 재사용 가능해진다)
                    sq->head.store(cur_head + head_move_count, simt::memory_order_relaxed);

                }

                // head_lock을 해제한다
                sq->head_lock.store(UNLOCKED, simt::memory_order_release);
            }
            // 자신의 head_mark가 UNLOCKED가 되었는지 확인한다
            cont = sq->head_mark[pos].val.load(simt::memory_order_relaxed) == LOCKED;
            if (cont) {
                // 지수 백오프로 대기한다
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
                __nanosleep(ns);
                if (ns < 256) {
                    ns *= 2;
                }
#endif

            }
    }



}

/* cq_poll: CQ에서 특정 Command ID(search_cid)의 완료를 폴링한다.
 *
 * NVMe CQ를 스캔하여 search_cid와 일치하고 올바른 위상(phase)을 가진 엔트리를 찾는다.
 * 찾을 때까지 무한 반복하며, 지수 백오프로 PCIe 트래픽을 줄인다.
 *
 * NVMe CQ 위상 비트 메커니즘:
 * - CQ가 한 바퀴 돌 때마다 위상이 반전된다
 * - 짝수 바퀴면 phase=1, 홀수 바퀴면 phase=0 (또는 반대)
 * - 이를 통해 새 완료와 오래된 완료를 구분한다
 *
 * 반환값: 매칭된 CQ 엔트리의 큐 내 위치(인덱스)
 * loc_: 논리적 CQ 위치 (wrap-around 포함)
 * cq_head: 검색 시작 시점의 CQ head 값
 */
inline __device__
uint32_t cq_poll(nvm_queue_t* cq, uint16_t search_cid, uint32_t* loc_ = NULL, uint32_t* cq_head = NULL) {
    uint64_t j = 0;
    unsigned int ns = 8;
    while (true) {
        // 현재 CQ head를 읽는다
        uint32_t head = cq->head.load(simt::memory_order_relaxed);

        // CQ 전체를 head부터 스캔한다
        for (size_t i = 0; i < cq->qs_minus_1; i++) {
            uint32_t cur_head = head + i;
            // 현재 위치의 예상 위상을 계산한다: (cur_head / qs)가 짝수이면 1, 홀수이면 0
            bool search_phase = ((~(cur_head >> cq->qs_log2)) & 0x01);
            // 실제 CQ 메모리의 위치를 계산한다
            uint32_t loc = cur_head & (cq->qs_minus_1);
            // CQ 엔트리의 dword[3]을 읽는다 (Status Field + CID)
            uint32_t cpl_entry = ((nvm_cpl_t*)cq->vaddr)[loc].dword[3];
            // 하위 16비트가 Command ID이다
            uint32_t cid = (cpl_entry & 0x0000ffff);
            // 비트 16이 위상(Phase Tag) 비트이다
            bool phase = (cpl_entry & 0x00010000) >> 16;

            // CID가 일치하고 위상도 맞으면 찾은 것이다
            if ((cid == search_cid) && (phase == search_phase)){
                *cq_head = head;
                *loc_ = cur_head;
                return loc; // CQ 배열 내 실제 인덱스를 반환한다
            }
            // 위상이 다르면 아직 새 완료가 없으므로 스캔을 중단한다
            if (phase != search_phase)
                break;
        }
        j++;
        // 지수 백오프: PCIe를 통한 CQ 메모리 폴링 빈도를 줄인다
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
         __nanosleep(ns);
         if (ns < 256) {
             ns *= 2;
         }
#endif
    }
}

/* cq_dequeue: CQ에서 완료 엔트리를 디큐하고 SQ head를 업데이트한다.
 *
 * 가장 복잡한 함수이다. 3단계로 동작한다:
 *
 * 1단계 - 위치 잠금: pos_locks[pos]를 잡아서 같은 CQ 슬롯의 동시 접근을 방지한다.
 *   이전 사용자가 해제할 때까지 spin-wait한 후 fetch_or로 잠금을 획득한다.
 *
 * 2단계 - head 전진: head_mark[pos]를 LOCKED로 설정하고 리더를 선출한다.
 *   리더는 연속 처리된 CQ 엔트리를 수집하여 CQ 도어벨을 업데이트하고,
 *   동시에 CQ 엔트리에 포함된 SQ Head Pointer로 SQ head도 갱신한다.
 *   도어벨 업데이트는 PTX st.mmio 명령어로 PCIe MMIO에 직접 쓴다.
 *
 * 3단계 - 자기 위치 반영 대기: CQ head가 자신의 논리적 위치를 넘어갈 때까지 대기한다.
 *   이는 도어벨이 컨트롤러에 반영되어 해당 CQ 슬롯이 재사용 가능해졌음을 보장한다.
 *   wrap-around를 고려한 비교 로직이 사용된다.
 *
 * 마지막으로 pos_locks[pos]를 해제하여 다음 사용자가 같은 슬롯을 쓸 수 있게 한다.
 */
inline __device__
void cq_dequeue(nvm_queue_t* cq, uint16_t pos, nvm_queue_t* sq, uint32_t loc_ = 0, uint32_t cur_head_ = 0) {
    // CQ tail을 atomic 증가: 현재 처리 중인 완료 수를 추적한다
    cq->tail.fetch_add(1, simt::memory_order_acq_rel);

    // === 1단계: 위치 잠금 획득 ===
    // pos_locks[pos]가 해제(0)될 때까지 relaxed로 먼저 확인한다 (불필요한 atomic RMW 회피)
    unsigned int ns = 8;
    while ((cq->pos_locks[pos].val.load(simt::memory_order_relaxed) != 0) ) {
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
        __nanosleep(ns);
        if (ns < 256) {
            ns *= 2;
        }
#endif
    }

    // fetch_or로 잠금을 획득한다 (이전 값이 0이면 성공)
    ns = 8;
    while ((cq->pos_locks[pos].val.fetch_or(1, simt::memory_order_acquire) != 0) ) {
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
        __nanosleep(ns);
        if (ns < 256) {
            ns *= 2;
        }
#endif
    }

    // === 2단계: head_mark 설정 및 리더 선출 ===
    // 이 CQ 슬롯의 head_mark를 LOCKED로 설정하여 완료 처리 완료를 표시한다
    cq->head_mark[pos].val.store(LOCKED, simt::memory_order_release);


    bool cont = true;
    ns = 8;
    cont = cq->head_mark[pos].val.load(simt::memory_order_relaxed) == LOCKED;
    while (cont) {
            // head_lock을 잡아서 리더 역할을 시도한다
            bool new_cont = cq->head_lock.fetch_or(LOCKED, simt::memory_order_acquire) == LOCKED;
            if (!new_cont) {
                // 리더: CQ head에서 연속 처리된 완료를 수집하고 SQ head도 업데이트한다
                uint32_t cur_head = cq->head.load(simt::memory_order_relaxed);;

                // move_head_cq: 연속 head_mark 수집 + SQ head 갱신
                uint32_t head_move_count = move_head_cq(cq, cur_head, sq);

                if (head_move_count) {
                    uint32_t new_head = cur_head + head_move_count;

                    uint32_t new_db = (new_head) & (cq->qs_minus_1);

                    // PTX 인라인 어셈블리로 CQ 도어벨에 새 head 값을 MMIO 쓰기한다
                    asm volatile ("st.mmio.relaxed.sys.global.u32 [%0], %1;" :: "l"(cq->db),"r"(new_db) : "memory");

                    // CQ head를 전역적으로 업데이트한다
                    cq->head.store(new_head, simt::memory_order_release);

                }
                // head_lock을 해제한다
                cq->head_lock.store(UNLOCKED, simt::memory_order_release);
            }
            // 자신의 head_mark가 UNLOCKED가 되었는지 확인한다
            cont = cq->head_mark[pos].val.load(simt::memory_order_relaxed) == LOCKED;
            if (cont) {
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
                __nanosleep(ns);
                if (ns < 256) {
                    ns *= 2;
                }
#endif
            }
    }


    // === 3단계: 자기 위치가 도어벨에 반영될 때까지 대기 ===
    // CQ head가 자신의 논리적 위치(loc_)를 넘어가야 해당 슬롯이 안전하게 재사용 가능하다
	uint64_t j = 0;
    uint32_t new_head = cq->head.load(simt::memory_order_relaxed);
    ns = 8;
    do {
        // wrap-around를 고려한 비교: new_head가 loc_를 지나갔는지 확인
        if (new_head > cur_head_) {
            // 일반 경우: new_head와 cur_head_가 같은 방향
            if ((loc_ >= cur_head_) && (loc_ < new_head))
                break; // loc_가 [cur_head_, new_head) 구간에 있으면 반영됨

        }
        else if (new_head < cur_head_) {
            // wrap-around 경우: head가 큐 끝을 넘어 다시 시작됨
            if ((loc_ >= cur_head_))
                break; // loc_가 cur_head_ 이후이면 반영됨
            if (loc_ < new_head)
                break; // loc_가 새 head 이전이면 반영됨
        }

        j++;
        new_head = cq->head.load(simt::memory_order_relaxed);
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 700 || !defined(__CUDA_ARCH__))
        __nanosleep(ns);
        if (ns < 256) {
            ns *= 2;
        }
#endif
    } while(true);

    // 위치 잠금을 해제하여 다음 사용자가 같은 CQ 슬롯을 사용할 수 있게 한다
    cq->pos_locks[pos].val.store(0, simt::memory_order_release);
}

//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#undef __forceinline__
//#endif

#endif // __NVM_PARALLEL_QUEUE_H_
