/* queue.cpp - NVMe 큐(SQ/CQ) 초기화 및 완료 대기 구현 파일
 *
 * NVMe Submission Queue(SQ)와 Completion Queue(CQ)의 초기화, 리셋, 블로킹 디큐 기능을 제공한다.
 * 큐 디스크립터의 각 필드(번호, 크기, 도어벨, 위상 비트 등)를 설정하고,
 * CQ에서 완료 엔트리를 타임아웃까지 대기하며 폴링하는 기능을 구현한다.
 */
#include <nvm_types.h>    // NVMe 타입 정의 (nvm_queue_t, nvm_cmd_t, nvm_cpl_t 등)
#include <nvm_queue.h>    // 큐 관련 공개 API (nvm_sq_enqueue, nvm_cq_dequeue 등)
#include <nvm_util.h>     // 유틸리티 매크로
#include <stddef.h>       // 표준 정의
#include <stdint.h>       // 정수형 타입
#include <time.h>         // 시간 관련 (nanosleep용)
#include "regs.h"         // NVMe 레지스터 매크로 (SQ_DBL, CQ_DBL 도어벨 주소 계산)
#include "lib_util.h"     // 내부 유틸리티 (_nvm_delay_remain 등)
#include <simt/atomic>    // GPU용 atomic 연산 (CUDA 환경에서 사용)


/* nvm_queue_clear - 큐 디스크립터를 초기화한다.
 * CQ 또는 SQ의 모든 필드를 설정하고, 도어벨 레지스터 주소를 계산한다.
 * 큐 생성 시(admin_cq_create, admin_sq_create) 호출된다.
 */
int nvm_queue_clear(nvm_queue_t* queue, const nvm_ctrl_t* ctrl, bool cq, uint16_t no, uint32_t qs,
        bool local, volatile void* vaddr, uint64_t ioaddr)
{
    // 큐 크기 범위 검증: 최소 2개, 최대 64K개, 컨트롤러 최대값 이하
    if (qs < 2 || qs > 0x10000 || qs > ctrl->max_qs)
    {
        return EINVAL;
    }

    queue->no = no;              // 큐 식별 번호 (Queue ID)
    queue->qs = qs;              // 큐 크기 (엔트리 수)
    // 엔트리 크기: CQ이면 nvm_cpl_t(16바이트), SQ이면 nvm_cmd_t(64바이트)
    queue->es = cq ? sizeof(nvm_cpl_t) : sizeof(nvm_cmd_t);
    queue->head = 0;             // 헤드 포인터 초기화 (CQ: 다음 읽을 위치, SQ: 다음 완료된 위치)
    queue->tail = 0;             // 테일 포인터 초기화 (SQ: 다음 쓸 위치)
    queue->last = 0;             // 마지막으로 처리된 위치
    queue->phase = 1;            // 위상 비트 초기값 (CQ의 P 비트와 비교에 사용)
    queue->local = !!local;      // 로컬 메모리 여부 (GPU 원격 메모리가 아닌 경우 true)
    queue->head_lock = 0;        // 헤드 동시 접근 보호용 락
    queue->tail_lock = 0;        // 테일 동시 접근 보호용 락
    // queue->head_copy = 0;
    // queue->tail_copy = 0;
    queue->in_ticket = 0;        // 티켓 기반 동기화용 입장 티켓
    queue->cid_ticket = 0;       // 커맨드 ID 할당용 티켓

    // 도어벨 레지스터 주소 계산: BAR0 오프셋 0x1000부터 시작, dstrd에 따라 간격 결정
    // CQ이면 홀수 인덱스(2*no+1), SQ이면 짝수 인덱스(2*no) 도어벨 사용
    queue->db = (cq ? CQ_DBL(ctrl->mm_ptr, queue->no, ctrl->dstrd) : SQ_DBL(ctrl->mm_ptr, queue->no, ctrl->dstrd));
    queue->vaddr = vaddr;        // 큐 메모리의 가상 주소 (DMA 매핑된 주소)
    queue->ioaddr = ioaddr;      // 큐 메모리의 버스/물리 주소 (컨트롤러가 DMA로 접근하는 주소)

    return 0;
}



/* nvm_queue_reset - 큐의 상태 포인터만 초기 상태로 리셋한다.
 * 큐의 설정(번호, 크기, 도어벨 등)은 유지하고 head/tail/phase만 초기화한다.
 * 큐를 재사용할 때 호출한다.
 */
void nvm_queue_reset(nvm_queue_t* queue)
{
    queue->head = 0;     // 헤드 포인터 초기화
    queue->tail = 0;     // 테일 포인터 초기화
    queue->last = 0;     // 마지막 처리 위치 초기화
    queue->phase = 1;    // 위상 비트를 초기값(1)으로 리셋
}



/* nvm_cq_dequeue_block - CQ에서 완료 엔트리를 블로킹 방식으로 디큐한다.
 * 완료가 올 때까지 또는 타임아웃이 만료될 때까지 폴링을 반복한다.
 * timeout은 밀리초 단위이며, 내부적으로 나노초로 변환하여 _nvm_delay_remain으로 대기한다.
 */
nvm_cpl_t* nvm_cq_dequeue_block(nvm_queue_t* cq, uint64_t timeout)
{
    // 타임아웃을 밀리초에서 나노초로 변환
    uint64_t nsecs = timeout * 1000000UL;
    // 먼저 논블로킹 디큐를 시도
    nvm_cpl_t* cpl = nvm_cq_dequeue(cq);

    // 완료가 없으면 타임아웃까지 폴링 반복
    while (cpl == NULL && nsecs > 0)
    {
        // 1ms 대기 후 남은 시간 감소
        nsecs = _nvm_delay_remain(nsecs);
        // 다시 디큐 시도
        cpl = nvm_cq_dequeue(cq);
    }

    // 완료 엔트리 포인터 반환 (타임아웃이면 NULL)
    return cpl;
}
