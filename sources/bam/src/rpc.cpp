/* rpc.cpp - RPC(Remote Procedure Call) 기반 Admin 명령 실행 구현 파일
 *
 * NVMe Admin 큐 쌍(ACQ/ASQ)을 관리하고, RPC 바인딩을 통해 Admin 명령을 실행한다.
 * 로컬 모드에서는 직접 Admin 큐에 명령을 제출하고, 원격 모드에서는 RPC 스텁을 통해 전달한다.
 * Admin 참조(nvm_aq_ref)는 서버측 바인딩 핸들 리스트와 클라이언트측 스텁을 모두 관리한다.
 */
#ifdef _SISCI
#include <sisci_api.h>     // SISCI(SmartIO) API
#include <sisci_types.h>   // SISCI 타입
#include <sisci_error.h>   // SISCI 에러 코드
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif
#endif

#include <nvm_types.h>    // NVMe 타입 정의
#include <nvm_ctrl.h>     // 컨트롤러 핸들
#include <nvm_aq.h>       // Admin Queue 공개 API
#include <nvm_rpc.h>      // RPC 공개 API
#include <nvm_queue.h>    // 큐 조작 함수
#include <nvm_ctrl.h>     // 컨트롤러 핸들 (중복 include)
#include <nvm_cmd.h>      // 커맨드 헬퍼 함수
#include <nvm_util.h>     // 유틸리티 매크로
#include <nvm_error.h>    // 에러 처리
#include <nvm_dma.h>      // DMA 매핑
#include <stddef.h>       // 표준 정의
#include <stdint.h>       // 정수형 타입
#include <stdbool.h>      // bool 타입
#include <stdlib.h>       // malloc, free
#include <errno.h>        // POSIX 에러 코드
#include <string.h>       // strerror
#include "mutex.h"        // 뮤텍스 래퍼
#include "rpc.h"          // 내부 RPC 타입 및 함수 선언
#include "lib_ctrl.h"     // 내부 컨트롤러 구조체
#include "lib_util.h"     // 내부 유틸리티
#include "dprintf.h"      // 디버그 출력
#include <atomic>         // C++ 메모리 펜스


/* 로컬 Admin 큐 쌍 디스크립터.
 * 로컬 모드에서 Admin CQ/SQ를 직접 관리할 때 사용한다.
 */
struct local_admin
{
    nvm_dma_t*          qmem;       // 큐 메모리 DMA 핸들 (CQ + SQ용 DMA 영역)
    nvm_queue_t         acq;        // Admin Completion Queue 디스크립터
    nvm_queue_t         asq;        // Admin Submission Queue 디스크립터
    uint64_t            timeout;    // 컨트롤러 타임아웃 (밀리초)
};



/* RPC 서버측 바인딩 핸들 연결 리스트.
 * 서버가 관리하는 클라이언트별 핸들을 키로 구분하여 저장한다.
 */
struct rpc_handle
{
    struct rpc_handle*  next;       // 다음 핸들을 가리키는 포인터 (연결 리스트)
    uint32_t            key;        // 핸들 식별 키
    void*               data;       // 커스텀 인스턴스 데이터
    rpc_free_handle_t   release;    // 인스턴스 데이터 해제 콜백
};



/* Admin 큐 쌍 참조 구조체.
 * RPC 클라이언트(원격) 또는 서버(로컬) 모두를 표현한다.
 * 로컬 모드: stub이 execute_command를 가리키고, data가 local_admin을 가리킨다.
 * 원격 모드: stub이 네트워크 RPC 스텁을 가리킨다.
 */
struct nvm_admin_reference
{
    struct controller*      ctrl;       // 컨트롤러 참조 (참조 카운트 관리)
    struct mutex            lock;       // 참조에 대한 배타적 접근 보장용 뮤텍스
    struct rpc_handle*      handles;    // 서버측 바인딩 핸들 연결 리스트
    void*                   data;       // 커스텀 인스턴스 데이터 (로컬: local_admin*, 원격: RPC 데이터)
    rpc_free_binding_t      release;    // 인스턴스 데이터 해제 콜백
    rpc_stub_t              stub;       // 클라이언트측 RPC 스텁 함수 포인터
};



/* _nvm_rpc_handle_insert - RPC 서버의 바인딩 핸들 리스트에 새 핸들을 삽입한다.
 * 동일 키가 이미 존재하면 삽입에 실패한다 (중복 방지).
 */
int _nvm_rpc_handle_insert(nvm_aq_ref ref, uint32_t key, void* data, rpc_free_handle_t release)
{
    // 필수 인자 검증
    if (data == NULL || release == NULL)
    {
        return EINVAL;
    }

    // 새 핸들 노드 할당
    struct rpc_handle* handle = (struct rpc_handle*) malloc(sizeof(struct rpc_handle));
    if (handle == NULL)
    {
        dprintf("Failed to allocate binding handle: %s\n", strerror(errno));
        return ENOMEM;
    }

    // 핸들 노드 초기화
    handle->next = NULL;
    handle->key = key;
    handle->data = data;
    handle->release = release;

    // 참조 락을 잡고 리스트에 삽입
    int err = _nvm_mutex_lock(&ref->lock);
    if (err != 0)
    {
        free(handle);
        dprintf("Failed to take reference lock: %s\n", strerror(err));
        return err;
    }

    // 연결 리스트 끝까지 순회하면서 중복 키 검사
    struct rpc_handle* prev = NULL;
    struct rpc_handle* curr = ref->handles;

    while (curr != NULL)
    {
        // 동일 키가 이미 존재하면 삽입 실패
        if (curr->key == handle->key)
        {
            _nvm_mutex_unlock(&ref->lock);
            free(handle);
            dprintf("Handle already inserted\n");
            return EINVAL;
        }

        prev = curr;
        curr = curr->next;
    }

    // 리스트 끝에 새 핸들 추가
    if (prev != NULL)
    {
        prev->next = handle;
    }
    else
    {
        // 리스트가 비어있으면 첫 번째 노드로 설정
        ref->handles = handle;
    }

    _nvm_mutex_unlock(&ref->lock);
    return 0;
}



/* _nvm_rpc_handle_remove - 지정된 키의 바인딩 핸들을 리스트에서 제거한다.
 * 핸들의 release 콜백을 호출하여 인스턴스 데이터를 정리한다.
 */
void _nvm_rpc_handle_remove(nvm_aq_ref ref, uint32_t key)
{
    _nvm_mutex_lock(&ref->lock);

    // 연결 리스트에서 해당 키를 가진 노드를 검색
    struct rpc_handle* prev = NULL;
    struct rpc_handle* curr = ref->handles;

    while (curr != NULL && curr->key != key)
    {
        prev = curr;
        curr = curr->next;
    }

    // 리스트에서 노드 제거 (연결 리스트 재연결)
    if (prev != NULL)
    {
        prev->next = curr->next;
    }
    else
    {
        // 첫 번째 노드를 제거하는 경우
        ref->handles = curr->next;
    }

    // 제거된 핸들의 리소스 해제
    if (curr != NULL)
    {
        curr->release(curr->key, curr->data);   // 인스턴스 데이터 해제 콜백 호출
        free(curr);                              // 핸들 노드 메모리 해제
    }

    _nvm_mutex_unlock(&ref->lock);
}



/* release_handles - 서버의 모든 바인딩 핸들을 제거하는 헬퍼 함수.
 * Admin 참조 해제 시 호출된다. 반드시 락을 잡은 상태에서 호출해야 한다.
 */
static void release_handles(nvm_aq_ref ref)
{
    struct rpc_handle* curr;
    struct rpc_handle* next;

    if (ref != NULL)
    {
        curr = NULL;
        next = ref->handles;
        // 연결 리스트를 순회하면서 모든 핸들 해제
        while (next != NULL)
        {
            curr = next;
            next = curr->next;
            curr->release(curr->key, curr->data);   // 인스턴스 데이터 해제
            free(curr);                              // 핸들 노드 해제
        }

        ref->handles = NULL;
    }
}



/* _nvm_ref_get - Admin 참조를 할당하고 컨트롤러 참조 카운트를 증가시키는 헬퍼 함수.
 * nvm_admin_reference 구조체를 동적 할당하고, 뮤텍스와 컨트롤러 참조를 초기화한다.
 */
int _nvm_ref_get(nvm_aq_ref* handle, const nvm_ctrl_t* ctrl)
{
    *handle = NULL;
    struct controller* cref;

    // 컨트롤러 참조 카운트 증가
    cref = _nvm_ctrl_get(ctrl);
    if (cref == NULL)
    {
        return ENOTTY;
    }

    // Admin 참조 구조체 할당
    nvm_aq_ref ref = (nvm_aq_ref) malloc(sizeof(struct nvm_admin_reference));
    if (ref == NULL)
    {
        _nvm_ctrl_put(cref);
        dprintf("Failed to allocate reference: %s\n", strerror(errno));
        return ENOMEM;
    }

    // 참조 보호용 뮤텍스 초기화
    int err = _nvm_mutex_init(&ref->lock);
    if (err != 0)
    {
        _nvm_ctrl_put(cref);
        free(ref);
        dprintf("Failed to initialize reference lock: %s\n", strerror(err));
        return err;
    }

    // Admin 참조 멤버 초기화
    ref->ctrl = cref;          // 컨트롤러 참조
    ref->handles = NULL;       // 서버측 핸들 리스트 (비어있음)
    ref->data = NULL;          // 인스턴스 데이터 (아직 바인딩 안 됨)
    ref->release = NULL;       // 해제 콜백 (아직 바인딩 안 됨)
    ref->stub = NULL;          // RPC 스텁 (아직 바인딩 안 됨)

    *handle = ref;
    return 0;
}



/* _nvm_ref_put - Admin 참조를 해제하는 헬퍼 함수.
 * 모든 바인딩 핸들을 제거하고, 인스턴스 데이터를 해제하고, 컨트롤러 참조를 감소시킨다.
 */
void _nvm_ref_put(nvm_aq_ref ref)
{
    if (ref != NULL)
    {
        _nvm_mutex_lock(&ref->lock);

        // 서버측 모든 바인딩 핸들 해제
        release_handles(ref);

        // 인스턴스 데이터 해제 콜백 호출 (있는 경우)
        if (ref->release != NULL)
        {
            ref->release(ref->data);
        }

        // 컨트롤러 참조 감소
        _nvm_ctrl_put(ref->ctrl);
        _nvm_mutex_unlock(&ref->lock);

        // 뮤텍스 파괴 및 메모리 해제
        _nvm_mutex_free(&ref->lock);
        free(ref);
    }
}



/* execute_command - 로컬 Admin 큐를 통해 NVM Admin 명령을 실행한다.
 * ASQ에 명령을 enqueue하고, ACQ에서 완료를 블로킹 대기한다.
 * 반드시 참조 락을 잡은 상태에서 호출해야 한다.
 */
static int execute_command(struct local_admin* admin, const nvm_cmd_t* cmd, nvm_cpl_t* cpl)
{
    nvm_cmd_t local_copy;      // 명령의 로컬 복사본
    nvm_cmd_t* in_queue_cmd;   // SQ 내의 명령 슬롯 포인터
    nvm_cpl_t* in_queue_cpl;   // CQ 내의 완료 엔트리 포인터

    // Try to enqueue a message
    // SQ에 빈 슬롯을 할당받는다
    if ((in_queue_cmd = nvm_sq_enqueue(&admin->asq)) == NULL)
    {
        // Queue was full, but we're holding the lock so no blocking
        // 큐가 가득 찬 경우 (락을 잡고 있으므로 블로킹하지 않고 재시도 필요)
        return EAGAIN;
    }

    // Copy command into queue slot (but keep original id)
    // 큐 슬롯에 넣을 명령 ID를 할당받는다 (큐 내부에서 고유해야 함)
    uint16_t in_queue_id = NVM_DEFAULT_CID(&admin->asq);

    // 원본 명령을 로컬에 복사
    memcpy(&local_copy, cmd, sizeof(nvm_cmd_t));

    // 명령 ID를 큐 할당 ID로 교체
    *NVM_CMD_CID(&local_copy) = in_queue_id;
    // 큐 슬롯에 명령 복사
    *in_queue_cmd = local_copy;

    //for (int i = 0; i < 16; i++) {
    //    printf("cmd: %p\tdword[%d] = %x\n", in_queue_cmd, i, local_copy.dword[i]);
    //}
    // 메모리 펜스: 명령 데이터 쓰기가 도어벨 쓰기 전에 완료되도록 보장
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // Submit command and wait for completion
    // SQ 도어벨을 울려서 컨트롤러에 새 명령이 있음을 알린다
    nvm_sq_submit(&admin->asq);
    // 메모리 펜스: 도어벨 쓰기 후 CQ 폴링 전에 보장
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // 타임아웃까지 CQ에서 완료 엔트리를 블로킹 대기
    in_queue_cpl = nvm_cq_dequeue_block(&admin->acq, admin->timeout);
    if (in_queue_cpl == NULL)
    {
        dprintf("Waiting for admin queue completion timed out\n");
        return ETIME;
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    //for (int i = 0; i < 4; i++) {
    //    printf("cpl: %p\tdword[%d] = %x\n", in_queue_cpl, i, in_queue_cpl->dword[i]);

    //}
    //printf("cpl cmd_id: %u\tstatus and phase: %x\n", in_queue_cpl->dword[3] & 0x0000ffff, in_queue_cpl->dword[3] >> 16);

    // SQ 헤드 포인터를 업데이트 (CQ 엔트리에서 SQ 헤드 값을 가져옴)
    nvm_sq_update(&admin->asq);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // Copy completion and return
    // 완료 엔트리를 호출자의 cpl로 복사
    *cpl = *in_queue_cpl;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // CQ 도어벨을 울려서 컨트롤러에 완료를 처리했음을 알린다
    nvm_cq_update(&admin->acq);

    //*NVM_CPL_CID(cpl) = *NVM_CMD_CID(cmd);

    return 0;
}



/* create_admin - 로컬 Admin 큐 쌍 디스크립터를 생성하는 헬퍼 함수.
 * DMA 윈도우에서 ACQ(페이지 0)와 ASQ(페이지 1)를 초기화한다.
 */
static int create_admin(struct local_admin** handle, const struct controller* ctrl, const nvm_dma_t* window)
{
    int status;
    nvm_dma_t* copy;
    struct local_admin* admin;

    *handle = NULL;

    // 컨트롤러 페이지 크기와 DMA 윈도우 페이지 크기가 일치하는지 검증
    if (ctrl->handle.page_size != window->page_size)
    {
        dprintf("Controller page size differs from DMA window page size\n");
        return EINVAL;
    }
    // DMA 윈도우가 최소 2페이지(CQ + SQ) 이상이어야 한다
    else if (window->n_ioaddrs < 2)
    {
        dprintf("DMA window is not large enough\n");
        return ERANGE;
    }
    // DMA 윈도우가 가상 주소 공간에 매핑되어 있어야 한다
    else if (window->vaddr == NULL)
    {
        dprintf("DMA window is not mapped into virtual address space\n");
        return EINVAL;
    }

    // DMA 핸들을 복제한다 (원본 핸들과 독립적인 수명 관리를 위해)
    status = nvm_dma_remap(&copy, window);
    if (status != 0)
    {
        return status;
    }

    // local_admin 구조체 할당
    admin = (struct local_admin*) malloc(sizeof(struct local_admin));
    if (admin == NULL)
    {
        nvm_dma_unmap(copy);
        dprintf("Failed to create admin queue-pair descriptors: %s\n", strerror(errno));
        return errno;
    }

    // 복제된 DMA 핸들 저장
    admin->qmem = copy;
    // 큐 메모리 영역을 0으로 초기화 (CQ 페이지 + SQ 페이지 = 2페이지)
    memset((void*) admin->qmem->vaddr, 0, 2 * admin->qmem->page_size);

    // ACQ 초기화: 페이지 0 사용, CQ 모드, 큐 번호 0 (Admin)
    nvm_queue_clear(&admin->acq, &ctrl->handle, true, 0, ctrl->handle.page_size / sizeof(nvm_cpl_t),
            admin->qmem->local, admin->qmem->vaddr, admin->qmem->ioaddrs[0]);

    // ASQ 초기화: 페이지 1 사용, SQ 모드, 큐 번호 0 (Admin)
    nvm_queue_clear(&admin->asq, &ctrl->handle, false, 0, ctrl->handle.page_size / sizeof(nvm_cmd_t),
            admin->qmem->local,  NVM_DMA_OFFSET(admin->qmem, 1), admin->qmem->ioaddrs[1]);

    // 컨트롤러 타임아웃 값 저장 (완료 대기 시 사용)
    admin->timeout = ctrl->handle.timeout;

    *handle = admin;
    return 0;
}


/* remove_admin - 로컬 Admin 큐 쌍 디스크립터를 해제하는 헬퍼 함수. */
static void remove_admin(struct local_admin* admin)
{
    if (admin != NULL)
    {
        // DMA 매핑 해제
        nvm_dma_unmap(admin->qmem);
        // local_admin 메모리 해제
        free(admin);
    }
}



/* nvm_raw_rpc - RPC 바인딩 참조를 통해 Admin 명령을 실행한다.
 * 참조 락을 잡고 스텁 함수를 호출한다.
 * 로컬 모드에서는 execute_command가 호출되고, 원격 모드에서는 네트워크 RPC 스텁이 호출된다.
 */
int nvm_raw_rpc(nvm_aq_ref ref, nvm_cmd_t* cmd, nvm_cpl_t* cpl)
{
    int err;

    // 참조 락 획득 (Admin 명령은 한 번에 하나씩 실행)
    err = _nvm_mutex_lock(&ref->lock);
    if (err != 0)
    {
        dprintf("Failed to take reference lock\n");
        return NVM_ERR_PACK(NULL, err);
    }

    // 스텁이 바인딩되지 않았으면 에러
    if (ref->stub == NULL)
    {
        _nvm_mutex_unlock(&ref->lock);
        dprintf("Reference is not bound!\n");
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 스텁 함수를 호출하여 Admin 명령 실행
    err = ref->stub(ref->data, cmd, cpl);

    _nvm_mutex_unlock(&ref->lock);

    // 에러 코드와 NVMe 상태를 합쳐서 반환
    return NVM_ERR_PACK(cpl, err);
}



/* _nvm_rpc_bind - Admin 참조를 원격 핸들에 바인딩한다.
 * 스텁 함수와 인스턴스 데이터를 설정하여 이후 nvm_raw_rpc 호출 시 사용되게 한다.
 * 이미 바인딩된 상태이면 실패한다.
 */
int _nvm_rpc_bind(nvm_aq_ref ref, void* data, rpc_free_binding_t release, rpc_stub_t stub)
{
    int err;

    // 참조 락 획득
    err = _nvm_mutex_lock(&ref->lock);
    if (err != 0)
    {
        dprintf("Failed to take reference lock\n");
        return err;
    }

    // 이미 바인딩되어 있는지 확인
    if (ref->data != NULL || ref->stub != NULL)
    {
        _nvm_mutex_unlock(&ref->lock);
        dprintf("Reference is already bound!\n");
        return EINVAL;
    }

    // 바인딩 정보 설정
    ref->data = data;          // 인스턴스 데이터
    ref->release = release;    // 해제 콜백
    ref->stub = stub;          // RPC 스텁 함수

    _nvm_mutex_unlock(&ref->lock);
    return 0;
}



/* nvm_aq_create - 로컬 Admin 큐 쌍을 생성한다.
 * Admin 참조를 할당하고, 로컬 Admin 디스크립터를 생성하고, 컨트롤러를 리셋하여
 * Admin 큐를 활성화한다. 이후 이 참조를 사용하여 Admin 명령을 실행할 수 있다.
 */
int nvm_aq_create(nvm_aq_ref* handle, const nvm_ctrl_t* ctrl, const nvm_dma_t* window)
{
    int err;
    nvm_aq_ref ref;

    *handle = NULL;

    // Allocate reference
    // Admin 참조 할당 (컨트롤러 참조 카운트 증가)
    err = _nvm_ref_get(&ref, ctrl);
    if (err != 0)
    {
        return err;
    }

    // Allocate admin descriptor
    // 로컬 Admin 큐 쌍 디스크립터 생성 (DMA 윈도우에서 CQ/SQ 초기화)
    err = create_admin((struct local_admin**) &ref->data, ref->ctrl, window);
    if (err != 0)
    {
        _nvm_ref_put(ref);
        return err;
    }

    // 스텁을 로컬 실행 함수로 설정
    ref->stub = (rpc_stub_t) execute_command;
    // 해제 콜백을 Admin 디스크립터 제거 함수로 설정
    ref->release = (rpc_free_binding_t) &remove_admin;

    // Reset controller
    // 컨트롤러 리셋 수행 (CC.EN=0→큐 설정→CC.EN=1)
    // ACQ 주소=ioaddrs[0], ASQ 주소=ioaddrs[1]
    const struct local_admin* admin = (const struct local_admin*) ref->data;
    nvm_raw_ctrl_reset(ctrl, admin->qmem->ioaddrs[0], admin->qmem->ioaddrs[1]);
    //printf("admin sq vaddr: %p\tsq ioaddr: %lx\n", admin->qmem->vaddr, admin->qmem->ioaddrs[0]);
    //printf("admin cq vaddr: %p\tcq ioaddr: %lx\n", admin->qmem->vaddr+4096, admin->qmem->ioaddrs[1]);

    *handle = ref;
    return 0;
}



/* nvm_aq_destroy - Admin 큐 쌍을 파괴한다.
 * Admin 참조를 해제하여 DMA 매핑, 큐 디스크립터, 컨트롤러 참조를 모두 정리한다.
 */
void nvm_aq_destroy(nvm_aq_ref ref)
{
    if (ref != NULL)
    {
        _nvm_ref_put(ref);
    }
}



/* nvm_ctrl_from_aq_ref - Admin 참조에서 컨트롤러 핸들을 추출한다. */
const nvm_ctrl_t* nvm_ctrl_from_aq_ref(nvm_aq_ref ref)
{
    if (ref != NULL)
    {
        return &ref->ctrl->handle;
    }

    return NULL;
}



/* _nvm_local_admin - 로컬 Admin 참조를 통해 직접 명령을 실행한다.
 * nvm_raw_rpc와 유사하지만, 스텁이 execute_command인지 검증하여 로컬 전용임을 보장한다.
 */
int _nvm_local_admin(nvm_aq_ref ref, const nvm_cmd_t* cmd, nvm_cpl_t* cpl)
{
    // 참조 락 획득
    int err = _nvm_mutex_lock(&ref->lock);
    if (err != 0)
    {
        dprintf("Failed to take reference lock: %s\n", strerror(err));
        return NVM_ERR_PACK(NULL, err);
    }

    // 스텁이 execute_command가 아니면 이 참조는 원격 바인딩이므로 에러
    if (ref->stub != (rpc_stub_t) execute_command)
    {
        _nvm_mutex_unlock(&ref->lock);
        dprintf("Reference is not local descriptor\n");
        return NVM_ERR_PACK(NULL, EINVAL);
    }

    // 로컬 Admin 큐를 통해 직접 명령 실행
    err = execute_command((struct local_admin*) ref->data, cmd, cpl);

    _nvm_mutex_unlock(&ref->lock);
    return NVM_ERR_PACK(NULL, err);
}



/* nvm_rpc_unbind - RPC 바인딩을 해제한다.
 * Admin 참조의 모든 리소스를 정리한다.
 */
void nvm_rpc_unbind(nvm_aq_ref ref)
{
    if (ref != NULL)
    {
        //if (ref->stub != (rpc_stub_t) execute_command)
        //{
            _nvm_ref_put(ref);
        //}
    }
}
