/* ctrl.cpp - NVMe 컨트롤러 핸들 관리 및 리셋 구현 파일
 *
 * NVMe 컨트롤러의 초기화, 참조 카운팅, 리셋(CC.EN 토글) 등 핵심 관리 기능을 제공한다.
 * 컨트롤러 핸들은 참조 카운트로 수명을 관리하며, BAR0 레지스터에서 컨트롤러 속성을 읽어온다.
 */
#include <nvm_types.h>    // NVMe 타입 정의
#include <nvm_ctrl.h>     // 컨트롤러 핸들 공개 API
#include <nvm_util.h>     // 유틸리티 매크로
#include <stdint.h>       // 정수형 타입
#include <stddef.h>       // 표준 정의
#include <stdbool.h>      // bool 타입
#include <stdlib.h>       // malloc, free
#include <errno.h>        // POSIX 에러 코드
#include <atomic>         // C++ 메모리 펜스 (std::atomic_thread_fence)
#include "mutex.h"        // 뮤텍스 래퍼
#include "lib_ctrl.h"     // 내부 컨트롤러 구조체 정의
#include "lib_util.h"     // 내부 유틸리티 함수
#include "regs.h"         // NVMe 레지스터 접근 매크로
#include "dprintf.h"      // 디버그 출력


/* Convenience defines */
// 페이지 크기(바이트)를 NVMe CC.MPS 레지스터 값으로 인코딩 (log2(ps) - 12)
#define encode_page_size(ps)    _nvm_b2log((ps) >> 12)
// 엔트리 크기(바이트)를 NVMe 레지스터 값으로 인코딩 (log2(es))
#define encode_entry_size(es)   _nvm_b2log(es)



/* create_handle - 컨트롤러 핸들 컨테이너를 할당하고 초기화하는 헬퍼 함수.
 * 참조 카운트를 1로 설정하고 디바이스 포인터와 콜백 함수를 저장한다.
 */
static struct controller* create_handle(struct device* dev, const struct device_ops* ops, enum device_type type)
{
    int err;
    struct controller* handle;

    // 디바이스가 있는데 릴리즈 콜백이 없으면 비정상 상태
    if (dev != NULL && (ops == NULL || ops->release_device == NULL))
    {
        dprintf("Inconsistent state, device operations is not set\n");
        return NULL;
    }

    // controller 구조체 동적 할당
    handle = (struct controller*) malloc(sizeof(struct controller));
    if (handle == NULL)
    {
        dprintf("Failed to allocate controller handle: %s\n", strerror(errno));
        return NULL;
    }

    // 사용자 핸들(nvm_ctrl_t) 부분을 0으로 초기화
    memset(&handle->handle, 0, sizeof(nvm_ctrl_t));

    // 뮤텍스 초기화 (참조 카운트 보호용)
    err = _nvm_mutex_init(&handle->lock);
    if (err != 0)
    {
        free(handle);
        return NULL;
    }

    // 초기 참조 카운트 = 1 (생성자가 소유)
    handle->count = 1;
    // 디바이스 핸들 저장
    handle->device = dev;
    // 디바이스 타입 저장 (UNKNOWN, IOCTL, SMARTIO 등)
    handle->type = type;
    // 디바이스 콜백 함수 복사
    if (ops != NULL)
    {
        handle->ops = *ops;
    }
    else
    {
        // 콜백이 없으면 0으로 초기화
        memset(&handle->ops, 0, sizeof(struct device_ops));
    }

    return handle;
}



/* remove_handle - 컨트롤러 핸들 컨테이너를 해제하는 함수.
 * 뮤텍스를 파괴하고 메모리를 반환한다. 뮤텍스가 사용 중이면 재시도한다.
 */
static void remove_handle(struct controller* handle)
{
    int status;

    // 뮤텍스 파괴 시도 - EBUSY이면 다른 스레드가 사용 중이므로 반복
    do
    {
        status = _nvm_mutex_free(&handle->lock);
    }
    while (status == EBUSY);

    // 컨트롤러 핸들 메모리 해제
    free(handle);
}



/* _nvm_ctrl_get - 컨트롤러 참조 카운트를 증가시킨다.
 * 외부에서 컨트롤러 핸들을 사용할 때 호출하여 핸들이 중간에 해제되지 않도록 보장한다.
 */
struct controller* _nvm_ctrl_get(const nvm_ctrl_t* ctrl)
{
    if (ctrl != NULL)
    {
        // This is technically undefined behaviour (casting away const),
        // but we are not modifying the handle itself, only the container.
        // const를 제거하는 것은 기술적으로 UB이지만, 핸들이 아닌 컨테이너만 수정하므로 실용적으로 문제없다
        struct controller* controller = _nvm_container_of(ctrl, struct controller, handle);

        // 뮤텍스를 잡고 참조 카운트를 안전하게 증가
        int err = _nvm_mutex_lock(&controller->lock);
        if (err != 0)
        {
            dprintf("Failed to take device reference lock: %s\n", strerror(err));
            return NULL;
        }

        // Increase reference count
        // 참조 카운트 증가
        ++controller->count;

        _nvm_mutex_unlock(&controller->lock);

        return controller;
    }

    return NULL;
}



/* _nvm_ctrl_put - 컨트롤러 참조 카운트를 감소시킨다.
 * 참조 카운트가 0이 되면 디바이스 리소스를 해제하고 핸들을 삭제한다.
 */
void _nvm_ctrl_put(struct controller* controller)
{
    if (controller != NULL)
    {
        uint32_t count = 0;

        _nvm_mutex_lock(&controller->lock);
        // 참조 카운트를 감소시키고 결과를 저장
        count = --controller->count;
        if (count == 0)
        {
            // 마지막 참조가 해제되면 디바이스 리소스를 정리한다
            if (controller->device != NULL)
            {
                // release_device 콜백으로 BAR 매핑 해제 등을 수행
                controller->ops.release_device(controller->device, controller->handle.mm_ptr, controller->handle.mm_size);
            }

            controller->device = NULL;
        }
        _nvm_mutex_unlock(&controller->lock);

        // 뮤텍스 해제 후에 핸들을 삭제 (뮤텍스 잠금 상태에서 free하면 안 됨)
        if (count == 0)
        {
            remove_handle(controller);
        }
    }
}



/* _nvm_ctrl_init - 컨트롤러 핸들을 초기화하는 내부 함수.
 * BAR0 메모리 매핑 포인터에서 컨트롤러 속성(페이지 크기, 타임아웃, 최대 큐 크기 등)을 읽어
 * nvm_ctrl_t 구조체에 설정한다.
 */
int _nvm_ctrl_init(nvm_ctrl_t** handle, struct device* dev, const struct device_ops* ops, enum device_type type,
        volatile void* mm_ptr, size_t mm_size)
{
    struct controller* container;
    nvm_ctrl_t* ctrl;

    *handle = NULL;

    // 컨트롤러 핸들 컨테이너 생성
    container = create_handle(dev, ops, type);
    if (container == NULL)
    {
        return ENOMEM;
    }

    ctrl = &container->handle;
    // BAR0 메모리 매핑 포인터와 크기 저장
    ctrl->mm_ptr = mm_ptr;
    ctrl->mm_size = mm_size;

    // 최소 BAR 매핑 크기 검증 (NVMe 레지스터에 접근하려면 최소 크기가 필요)
    if (ctrl->mm_size < NVM_CTRL_MEM_MINSIZE)
    {
        remove_handle(container);
        return EINVAL;
    }

    // Get the system page size
    // 호스트 시스템의 페이지 크기를 가져온다 (보통 4KB)
    size_t page_size = _nvm_host_page_size();
    if (page_size == 0)
    {
        remove_handle(container);
        return ENOMEM;
    }

    // Get the controller page size
    // CAP 레지스터에서 컨트롤러가 지원하는 페이지 크기 범위를 읽는다
    uint8_t host_page_size = encode_page_size(page_size);       // 호스트 페이지 크기를 log2 인코딩
    uint8_t max_page_size = CAP$MPSMAX(ctrl->mm_ptr);           // 컨트롤러 최대 페이지 크기
    uint8_t min_page_size = CAP$MPSMIN(ctrl->mm_ptr);           // 컨트롤러 최소 페이지 크기

    // 호스트 페이지 크기가 컨트롤러 지원 범위 내에 있는지 검증
    if ( ! (min_page_size <= host_page_size && host_page_size <= max_page_size) )
    {
        remove_handle(container);
        dprintf("System page size is incompatible with controller page size\n");
        return ERANGE;
    }

    // Set controller properties
    // CAP 레지스터에서 읽은 값으로 컨트롤러 속성을 설정한다
    ctrl->page_size = page_size;                                 // 페이지 크기 (바이트)
    ctrl->dstrd = CAP$DSTRD(ctrl->mm_ptr);                      // 도어벨 스트라이드 (레지스터 간격)
    ctrl->timeout = CAP$TO(ctrl->mm_ptr) * 500UL;               // 타임아웃 (밀리초, CAP.TO * 500ms)
    ctrl->max_qs = CAP$MQES(ctrl->mm_ptr) + 1; // CAP.MQES is 0's based  // 최대 큐 엔트리 수 (0-based이므로 +1)

    *handle = ctrl;

    return 0;
}



/* nvm_raw_ctrl_reset - 컨트롤러를 수동으로 리셋하고 Admin 큐를 설정한다.
 * CC.EN=0으로 비활성화 → Admin 큐 주소/속성 설정 → CC.EN=1로 활성화하는 NVMe 리셋 시퀀스를 수행한다.
 * 사용자 핸들의 포인터를 직접 사용하여, 사용자가 오버라이드한 경우에도 동작한다.
 */
int nvm_raw_ctrl_reset(const nvm_ctrl_t* ctrl, uint64_t acq_addr, uint64_t asq_addr)
{
    // CC(Controller Configuration) 레지스터 포인터 획득
    volatile uint32_t* cc = CC(ctrl->mm_ptr);

    // Set CC.EN to 0
    // CC.EN 비트를 0으로 클리어하여 컨트롤러를 비활성화한다
    *cc = *cc & ~1;

    // Wait for CSTS.RDY to transition from 1 to 0
    // CSTS.RDY가 1→0으로 전환될 때까지 대기 (컨트롤러 비활성화 완료)
    uint64_t timeout = ctrl->timeout * 1000000UL;    // 밀리초 → 나노초 변환
    uint64_t remaining = _nvm_delay_remain(timeout);  // 남은 대기 시간 초기화
    // 메모리 펜스: MMIO 쓰기가 완료된 후 읽기를 수행하도록 보장
    std::atomic_thread_fence(std::memory_order_seq_cst);
    while (CSTS$RDY(ctrl->mm_ptr) != 0)
    {
        if (remaining == 0)
        {
            dprintf("Timeout exceeded while waiting for controller reset\n");
            return ETIME;
        }

        // 1ms씩 대기하면서 남은 시간을 감소
        remaining = _nvm_delay_remain(remaining);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    // Set admin queue attributes
    // AQA(Admin Queue Attributes) 레지스터에 Admin CQ/SQ의 최대 엔트리 수 설정
    volatile uint32_t* aqa = AQA(ctrl->mm_ptr);

    // 한 페이지에 들어갈 수 있는 CQ/SQ 엔트리 수 계산 (0-based이므로 -1)
    uint32_t cq_max_entries = ctrl->page_size / sizeof(nvm_cpl_t) - 1;
    uint32_t sq_max_entries = ctrl->page_size / sizeof(nvm_cmd_t) - 1;
    // AQA 레지스터: 상위 비트에 ACQ 크기, 하위 비트에 ASQ 크기 설정
    *aqa = AQA$ACQS(cq_max_entries) | AQA$ASQS(sq_max_entries);

    // Set admin completion queue
    // ACQ 레지스터에 Admin Completion Queue의 물리 주소 설정
    volatile uint64_t* acq = ACQ(ctrl->mm_ptr);
    *acq = acq_addr;

    // Set admin submission queue
    // ASQ 레지스터에 Admin Submission Queue의 물리 주소 설정
    volatile uint64_t* asq = ASQ(ctrl->mm_ptr);
    *asq = asq_addr;
    // 메모리 펜스: 큐 주소 쓰기가 CC.EN 설정 전에 완료되도록 보장
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // Set CC.MPS to pagesize and CC.EN to 1
    // CC 레지스터 설정: CQ/SQ 엔트리 크기, 페이지 크기, 커맨드 셋(NVM), Enable 비트
    uint32_t cqes = encode_entry_size(sizeof(nvm_cpl_t));  // CQ 엔트리 크기 인코딩 (log2)
    uint32_t sqes = encode_entry_size(sizeof(nvm_cmd_t));  // SQ 엔트리 크기 인코딩 (log2)
    *cc = CC$IOCQES(cqes) | CC$IOSQES(sqes) | CC$MPS(encode_page_size(ctrl->page_size)) | CC$CSS(0) | CC$EN(1);
    // 메모리 펜스: CC 레지스터 쓰기 후 CSTS 읽기 전에 완료 보장
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // Wait for CSTS.RDY to transition from 0 to 1
    // CSTS.RDY가 0→1로 전환될 때까지 대기 (컨트롤러 활성화 완료)
    remaining = _nvm_delay_remain(timeout);

    while (CSTS$RDY(ctrl->mm_ptr) != 1)
    {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if (remaining == 0)
        {
            dprintf("Timeout exceeded while waiting for controller enable\n");
            return ETIME;
        }

        remaining = _nvm_delay_remain(remaining);
    }

    //uint64_t asqaddr = *asq;
    //uint64_t acqaddr = *acq;
    //printf("reset: asqaddr: %llx\tacqaddr: %llx\n", asqaddr, acqaddr);
    return 0;
}



/* nvm_ctrl_free - 컨트롤러 핸들을 해제한다.
 * 참조 카운트를 감소시키고, 0이 되면 리소스를 정리한다.
 */
void nvm_ctrl_free(nvm_ctrl_t* ctrl)
{
    if (ctrl != NULL)
    {
        // container_of 매크로로 핸들에서 컨테이너 구조체를 역추적
        struct controller* container = _nvm_container_of(ctrl, struct controller, handle);
        // 참조 카운트 감소 (0이면 리소스 해제)
        _nvm_ctrl_put(container);
    }
}



/* nvm_raw_ctrl_init - 사용자가 직접 메모리 매핑한 BAR 포인터로 컨트롤러를 초기화한다.
 * 디바이스 핸들 없이 raw 포인터만으로 컨트롤러 핸들을 생성하는 간단한 래퍼이다.
 */
int nvm_raw_ctrl_init(nvm_ctrl_t** ctrl, volatile void* mm_ptr, size_t mm_size)
{
    // 디바이스 핸들/콜백 없이 UNKNOWN 타입으로 초기화
    return _nvm_ctrl_init(ctrl, NULL, NULL, DEVICE_TYPE_UNKNOWN, mm_ptr, mm_size);
}
