/* nvm_queue.h - NVMe Submission Queue(SQ)와 Completion Queue(CQ)의 기본 조작 함수
 * NVMe 컨트롤러와 통신하기 위한 큐 초기화, 커맨드 인큐/디큐, 도어벨 업데이트 등의
 * 저수준 큐 연산을 제공한다. GPU(__device__)와 CPU(__host__) 양쪽에서 사용 가능하다.
 */
#ifndef __NVM_QUEUE_H__
#define __NVM_QUEUE_H__

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif

#include <nvm_util.h>
#include <nvm_types.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>


/*
 * 큐 디스크립터를 초기화한다.
 *
 * 빈 큐 디스크립터를 초기화한다.
 * 사용자가 큐 메모리를 사용하기 전에 수동으로 클리어해야 한다.
 *
 * 주의: vaddr은 페이지 정렬되어야 하며 최소 한 페이지 이상이어야 한다.
 *
 * 큐 메모리는 물리적으로 연속적이어야 한다.
 */

__host__
int nvm_queue_clear(nvm_queue_t* q,            // NVM 큐 디스크립터
                    const nvm_ctrl_t* ctrl,    // NVM 컨트롤러 핸들
                    bool cq,                   // true이면 CQ, false이면 SQ인지 지정
                    uint16_t no,               // 큐 번호
                    uint32_t qs,               // 큐 크기 (엔트리 수)
                    bool local,                // 로컬 메모리인지 원격 메모리인지 지정
                    volatile void* vaddr,      // 큐 메모리의 가상 주소
                    uint64_t ioaddr);          // 컨트롤러가 보는 큐 메모리의 버스 주소



/*
 * 큐 디스크립터를 리셋하고 모든 멤버를 초기 상태로 설정한다.
 *
 * 주의: 큐가 생성되었지만 아직 삭제되지 않은 상태에서 이 함수를 사용하면
 *       컨트롤러 상태가 불일치하게 될 수 있다.
 */

__host__
void nvm_queue_reset(nvm_queue_t* q);




/*
 * nvm_sq_enqueue: SQ에 서브미션 커맨드를 인큐한다.
 *
 * 지정된 SQ에 커맨드를 인큐하고, 큐 슬롯 포인터를 반환하여
 * 큐 메모리에 직접 커맨드를 인라인으로 빌드할 수 있게 한다.
 *
 * 큐가 가득 차면 NULL을 반환한다.
 */
__host__ __device__ static inline
nvm_cmd_t* nvm_sq_enqueue(nvm_queue_t* sq)
{
    // 큐가 가득 찼는지 확인한다: (tail - head) % qs == qs - 1이면 full
    if (((uint16_t) (sq->tail - sq->head) % sq->qs) == sq->qs - 1)
    {
        return NULL;
    }

    // tail 위치의 큐 슬롯 포인터를 계산한다 (base_addr + entry_size * tail)
    nvm_cmd_t* cmd = (nvm_cmd_t*) (((unsigned char*) sq->vaddr) + sq->es * sq->tail);

    // tail 포인터를 증가시키고, 큐 끝에 도달하면 위상 태그를 반전시킨다
    if (++sq->tail == sq->qs)
    {
        sq->phase = !sq->phase; // 위상 비트를 토글한다 (CQ에서 새 완료 감지에 사용)
        sq->tail = 0;           // tail을 큐 시작으로 wrap-around한다
    }

    return cmd;
}



/*
 * nvm_sq_enqueue_n: n개 스레드 중 i번째 스레드가 커맨드를 인큐한다.
 *
 * 이 함수는 실제 큐 상태를 확인하지 않는다. 호출자는 마지막으로 받은 포인터를
 * 저장해두었다가 다음 호출 시 전달하여 위치 계산을 단순화해야 한다.
 *
 * 따라서 이 함수를 호출하기 전에 모든 완료가 소비되어야 한다.
 * 이렇게 하는 이유는 불필요한 스레드 동기화/배리어를 피하기 위해서이다.
 *
 * 주의: n은 큐 크기보다 작아야 한다.
 * 주의: 반환된 포인터를 저장하여 다음 호출의 마지막 매개변수로 사용해야 한다.
 */
#ifdef __CUDACC__
__device__ static inline
nvm_cmd_t* nvm_sq_enqueue_n(nvm_queue_t* sq, nvm_cmd_t* last, uint16_t n, uint16_t i)
{
    unsigned char* start = (unsigned char*) sq->vaddr;       // 큐 메모리 시작 주소
    unsigned char* end = start + (sq->qs * sq->es);          // 큐 메모리 끝 주소
    nvm_cmd_t* cmd = NULL;

    // n이 큐 크기 이상이면 인큐할 수 없다
    if (n >= sq->qs)
    {
        return NULL;
    }

    if (last == NULL)
    {
        // 첫 호출 시: i번째 스레드의 슬롯 위치를 직접 계산한다
        cmd = (nvm_cmd_t*) (start + sq->es * i);
    }
    else
    {
        // 이전 위치에서 n칸 앞으로 이동한다
        cmd = (nvm_cmd_t*) (((unsigned char*) last) + n * sq->es);

        // 큐 끝을 넘으면 wrap-around한다
        if (((nvm_cmd_t*) end) <= cmd)
        {
            cmd = (nvm_cmd_t*) (start + (((unsigned char*) cmd) - end));
        }
    }

    // 0번 스레드만 큐 상태(tail 포인터)를 업데이트한다
    if (i == 0)
    {
        sq->tail = (((uint32_t) sq->tail) + ((uint32_t) n)) % sq->qs;
    }

    // 모든 스레드가 상태 업데이트를 볼 수 있도록 동기화한다
    __syncthreads();

    return cmd;
}
#endif



/*
 * nvm_cq_poll: CQ의 head를 폴링하여 새로운 완료 엔트리가 있는지 확인한다.
 *
 * 새 완료가 있으면 해당 포인터를 반환하고, 큐가 비어있으면 NULL을 반환한다.
 * 호출자가 수동으로 dequeue를 호출해야 한다.
 */
__host__ __device__ static inline
nvm_cpl_t* nvm_cq_poll(const nvm_queue_t* cq)
{
    // head 위치의 완료 엔트리 포인터를 계산한다
    nvm_cpl_t* cpl = (nvm_cpl_t*) (((unsigned char*) cq->vaddr) + cq->es * cq->head);

#ifndef __CUDA_ARCH__
    // CPU 경로: 로컬 메모리인 경우 CPU 캐시를 무효화하여 최신 데이터를 읽는다
    if (cq->local)
    {
        nvm_cache_invalidate((void*) cpl, sizeof(nvm_cpl_t));
    }
#endif

    // 위상 태그를 확인하여 새 완료가 준비되었는지 판단한다
    // NVMe CQ의 위상 비트가 현재 예상 위상과 다르면 아직 새 완료가 없다
    if (!_RB(*NVM_CPL_STATUS(cpl), 0, 0) != !cq->phase)
    {
        return NULL;
    }

    return cpl;
}



/*
 * nvm_cq_dequeue: CQ에서 완료 엔트리를 디큐한다.
 *
 * 준비된 완료가 없으면 NULL을 반환한다.
 * 호출자가 대응하는 SQ를 수동으로 업데이트해야 한다.
 */
__host__ __device__ static inline
nvm_cpl_t* nvm_cq_dequeue(nvm_queue_t* cq)
{
    // CQ를 폴링하여 새 완료가 있는지 확인한다
    nvm_cpl_t* cpl = nvm_cq_poll(cq);

    if (cpl != NULL)
    {
        // head 포인터를 증가시키고, 큐 끝이면 wrap-around 및 위상 반전한다
        if (++cq->head == cq->qs)
        {
            cq->head = 0;
            cq->phase = !cq->phase; // 위상 태그를 반전한다
        }
    }

    return cpl;
}



/*
 * nvm_cq_dequeue_block: CQ에서 완료 엔트리를 블로킹 방식으로 디큐한다.
 *
 * 준비된 완료가 없으면 컨트롤러 타임아웃 또는 완료 도착까지 블록한다.
 * 타임아웃이 지나면 NULL을 반환한다.
 */

__host__
nvm_cpl_t* nvm_cq_dequeue_block(nvm_queue_t* cq, uint64_t timeout);




/*
 * nvm_sq_submit: SQ tail 도어벨을 울려서 인큐된 모든 커맨드를 NVMe 컨트롤러에 제출한다.
 *
 * 호출 전에 모든 커맨드가 준비되어야 한다.
 * 도어벨 레지스터에 새 tail 값을 MMIO 쓰기하여 컨트롤러에게 새 커맨드가 있음을 알린다.
 */
__host__ __device__ static inline
void nvm_sq_submit(nvm_queue_t* sq)
{
    // 새로 인큐된 커맨드가 있고 도어벨이 유효한 경우에만 제출한다
    if (sq->last != sq->tail && sq->db != NULL)
    {
#ifndef __CUDA_ARCH__
        if (sq->local)
        {
            // 로컬 메모리: CPU 캐시의 내용을 메모리로 플러시하여 컨트롤러가 읽을 수 있게 한다
            // TODO: 실제 변경된 엔트리만 플러시하도록 최적화 필요
            nvm_cache_flush((void*) sq->vaddr, sq->es * sq->qs);
        }
        else
        {
            // 원격 메모리(PCIe BAR 등): write combining 버퍼를 플러시한다
            nvm_wcb_flush();
        }
#endif

        // 도어벨 레지스터에 tail 값을 MMIO 쓰기한다 (volatile로 컴파일러 최적화 방지)
        *((volatile uint32_t*) sq->db) = sq->tail;
        // last를 tail로 업데이트하여 중복 제출을 방지한다
        sq->last = sq->tail;
    }
}



/*
 * nvm_sq_update: SQ head 포인터를 업데이트한다.
 * CQ에서 완료를 처리한 후 대응하는 SQ의 head를 전진시켜 슬롯을 해제한다.
 */
__host__ __device__ static inline
void nvm_sq_update(nvm_queue_t* sq)
{
    // SQ head 포인터를 1 증가시키고, 큐 끝에 도달하면 0으로 wrap-around한다
    if (sq->db != NULL && ++sq->head == sq->qs)
    {
        sq->head = 0;
    }
}



/*
 * nvm_cq_update: 컨트롤러의 CQ head 도어벨을 업데이트한다.
 *
 * 도어벨을 울려서 모든 완료가 처리되었음을 컨트롤러에게 알린다.
 * 이 호출 이전에 획득한 모든 완료 포인터는 이 호출 이후 무효가 된다.
 */
__host__ __device__ static inline
void nvm_cq_update(nvm_queue_t* cq)
{
    // 새로 처리된 완료가 있고 도어벨이 유효한 경우에만 업데이트한다
    if (cq->last != cq->head && cq->db != NULL)
    {
        // CQ 도어벨 레지스터에 새 head 값을 MMIO 쓰기한다
        *((volatile uint32_t*) cq->db) = cq->head;
        // tail과 last를 head로 동기화한다
        cq->tail = cq->last = cq->head;
    }
}


//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#endif

#endif /* __NVM_QUEUE_H__ */
