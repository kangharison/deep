/* interrupt.c - DIS/SISCI 데이터 인터럽트 관리 구현
 *
 * 이 파일은 DIS 패브릭을 통한 노드 간 데이터 인터럽트 메커니즘을 구현한다.
 * SISCI 데이터 인터럽트는 PCIe 패브릭 위에서 노드 간에 소량의 데이터를
 * 저지연으로 전송하는 수단으로, RPC 명령 전달 및 완료 통보에 사용된다.
 *
 * 인터럽트 흐름:
 *   송신측: SCIConnectDataInterrupt → SCITriggerDataInterrupt → SCIDisconnectDataInterrupt
 *   수신측: SCICreateDataInterrupt → (콜백 또는 SCIWaitForDataInterrupt) → SCIRemoveDataInterrupt
 */

/* _SISCI 매크로가 정의되지 않으면 컴파일을 중단한다 */
#ifndef _SISCI
#error "Must compile with SISCI support"
#endif

/* DIS 클러스터 모드 활성화 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "dis/interrupt.h"   /* 로컬/원격 인터럽트 구조체 및 함수 선언 */
#include "dprintf.h"         /* 디버그 출력 매크로 */
#include <sisci_types.h>     /* SISCI 타입 정의 */
#include <sisci_error.h>     /* SISCI 에러 코드 */
#include <sisci_api.h>       /* SISCI API 함수 선언 */



/*
 * interrupt_callback - SISCI 데이터 인터럽트 콜백 래퍼
 *
 * SISCI가 데이터 인터럽트 수신 시 호출하는 콜백 함수이다.
 * 기본적인 상태 검증을 수행한 후, 사용자가 등록한 콜백(intr_callback_t)을 호출한다.
 *
 * SISCI 콜백은 SCI_CALLBACK_CONTINUE를 반환하면 계속 수신 대기하고,
 * SCI_CALLBACK_CANCEL을 반환하면 인터럽트 수신을 중단한다.
 *
 * @interrupt: 인터럽트 컨텍스트 (local_intr 구조체)
 * @intr:      SISCI 인터럽트 핸들
 * @data:      수신된 데이터 버퍼
 * @length:    수신 데이터 길이
 * @status:    SISCI 상태 코드
 * @return:    SCI_CALLBACK_CONTINUE(계속 수신) 또는 SCI_CALLBACK_CANCEL(수신 중단)
 */
static sci_callback_action_t interrupt_callback(struct local_intr* interrupt,
                                                sci_local_data_interrupt_t intr,
                                                void* data,
                                                uint32_t length,
                                                sci_error_t status)
{
#ifndef NDEBUG
    /* 인터럽트 상태가 정상인지 확인한다 */
    if (status != SCI_ERR_OK)
    {
        dprintf("Unexpected status in interrupt handler routine: %s\n", _SCIGetErrorString(status));
        return SCI_CALLBACK_CANCEL;      /* 비정상 상태이면 콜백 수신 중단 */
    }

    /* 전달된 인터럽트 핸들이 구조체에 저장된 것과 일치하는지 확인 (메모리 손상 감지) */
    if (intr != interrupt->intr)
    {
        dprintf("Possible memory corruption\n");
        return SCI_CALLBACK_CANCEL;
    }
#endif

    /* 사용자가 등록한 콜백 함수를 호출한다.
     * interrupt->data는 사용자 컨텍스트 포인터, data는 수신 데이터, length는 데이터 길이 */
    interrupt->callback(interrupt->data, data, length);

    return SCI_CALLBACK_CONTINUE;        /* 인터럽트 계속 수신 대기 */
}



/*
 * _nvm_local_intr_get - 로컬 데이터 인터럽트를 생성한다.
 *
 * 새로운 SISCI 세션을 열고, 로컬 노드 ID를 조회한 뒤,
 * 데이터 인터럽트를 생성한다. 콜백 함수가 지정되면 콜백 모드로,
 * 지정되지 않으면 폴링/대기(SCIWaitForDataInterrupt) 모드로 동작한다.
 *
 * @intr:    초기화할 로컬 인터럽트 구조체
 * @adapter: DIS 어댑터 번호
 * @cb_data: 콜백에 전달할 사용자 데이터 포인터
 * @cb:      인터럽트 수신 시 호출할 콜백 함수 (NULL이면 대기 모드)
 * @return:  성공 시 0, 실패 시 errno 값
 */
int _nvm_local_intr_get(struct local_intr* intr, uint32_t adapter, void* cb_data, intr_callback_t cb)
{
    sci_error_t err = SCI_ERR_OK;

    /* SCIGetLocalNodeId: 지정된 어댑터의 로컬 DIS 노드 식별자를 조회한다.
     * 이 노드 ID는 원격 노드가 이 인터럽트에 연결할 때 필요하다. */
    SCIGetLocalNodeId(adapter, &intr->node_id, 0, &err);
#ifndef NDEBUG
    if (err != SCI_ERR_OK)
    {
        dprintf("Unexpected error: %s\n", _SCIGetErrorString(err));
        return EIO;
    }
#endif

    /* SCIOpen: 이 인터럽트 전용으로 새로운 SISCI 세션을 연다.
     * 각 인터럽트가 독립된 세션을 가지므로 서로 영향을 주지 않는다. */
    SCIOpen(&intr->sd, 0, &err);
#ifndef NDEBUG
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to open SISCI virtual device: %s\n", _SCIGetErrorString(err));
        return EIO;
    }
#endif

    /* 인터럽트 구조체 필드 설정 */
    intr->adapter = adapter;             /* 바인딩할 어댑터 번호 */
    intr->data = cb_data;                /* 사용자 컨텍스트 데이터 */
    intr->callback = cb;                 /* 사용자 콜백 함수 */

    uint32_t flags = 0;
    void* data = NULL;
    sci_cb_data_interrupt_t callback = NULL;

    /* 콜백이 지정되면 SISCI 콜백 모드로 설정한다 */
    if (cb != NULL)
    {
        data = (void*) intr;             /* 콜백에 전달할 컨텍스트로 intr 구조체 자체를 전달 */
        callback = (sci_cb_data_interrupt_t) interrupt_callback;  /* SISCI 콜백 래퍼 등록 */
        flags |= SCI_FLAG_USE_CALLBACK;  /* 콜백 모드 활성화 플래그 */
    }

    /* SCICreateDataInterrupt: 로컬 데이터 인터럽트를 생성한다.
     * intr_no에 SISCI가 자동 할당한 인터럽트 번호가 저장된다.
     * 원격 노드는 이 (node_id, intr_no) 쌍으로 이 인터럽트에 연결할 수 있다.
     * callback이 NULL이 아니면 인터럽트 수신 시 자동으로 콜백이 호출된다. */
    SCICreateDataInterrupt(intr->sd, &intr->intr, adapter, &intr->intr_no, callback, data, flags, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to create data interrupt: %s\n", _SCIGetErrorString(err));
        SCIClose(intr->sd, 0, &err);    /* 실패 시 세션 닫기 */
        return ENOSPC;
    }

    return 0;
}



/*
 * _nvm_local_intr_put - 로컬 데이터 인터럽트를 제거한다.
 *
 * SCIRemoveDataInterrupt()로 인터럽트를 해제하고 SCIClose()로 세션을 닫는다.
 * BUSY 상태가 해소될 때까지 제거를 재시도한다.
 *
 * @intr: 제거할 로컬 인터럽트 구조체
 */
void _nvm_local_intr_put(struct local_intr* intr)
{
    sci_error_t err = SCI_ERR_OK;

    /* SCIRemoveDataInterrupt: 로컬 데이터 인터럽트를 제거한다.
     * BUSY이면 아직 콜백이 실행 중이거나 대기 중인 상태이므로 재시도한다. */
    do
    {
        SCIRemoveDataInterrupt(intr->intr, 0, &err);
    }
    while (err == SCI_ERR_BUSY);

    /* 이 인터럽트 전용 SISCI 세션을 닫는다 */
    SCIClose(intr->sd, 0, &err);
}



/*
 * _nvm_local_intr_wait - 데이터 인터럽트 수신을 블로킹 대기한다.
 *
 * 콜백 모드가 아닌 경우에 사용되며, 지정된 timeout 밀리초 동안
 * 데이터 인터럽트가 도착하기를 블로킹 대기한다.
 *
 * @intr:    대기할 로컬 인터럽트 구조체
 * @data:    수신 데이터를 저장할 버퍼
 * @maxlen:  버퍼의 최대 크기(바이트)
 * @timeout: 대기 타임아웃(밀리초)
 * @return:  성공 시 0, 타임아웃 시 ETIMEDOUT, 기타 에러 시 EIO
 */
int _nvm_local_intr_wait(struct local_intr* intr, void* data, uint16_t maxlen, uint32_t timeout)
{
    sci_error_t err = SCI_ERR_OK;
    uint32_t len = maxlen;               /* 수신 가능한 최대 데이터 길이 */

    /* SCIWaitForDataInterrupt: 데이터 인터럽트가 도착할 때까지 블로킹 대기한다.
     * 인터럽트가 도착하면 data 버퍼에 수신 데이터가 복사되고 len에 실제 길이가 설정된다.
     * timeout 밀리초 내에 도착하지 않으면 SCI_ERR_TIMEOUT이 반환된다. */
    SCIWaitForDataInterrupt(intr->intr, data, &len, timeout, 0, &err);

    switch (err)
    {
        case SCI_ERR_OK:
            return 0;                    /* 인터럽트 수신 성공 */

        case SCI_ERR_TIMEOUT:
            return ETIMEDOUT;            /* 타임아웃 발생 */

        default:
            dprintf("Waiting for data interrupt unexpectedly failed: %s\n", _SCIGetErrorString(err));
            return EIO;
    }
}



/*
 * _nvm_remote_intr_get - 원격 노드의 데이터 인터럽트에 연결한다.
 *
 * 새로운 SISCI 세션을 열고, 지정된 원격 노드의 인터럽트 번호에 연결한다.
 * 연결이 성공하면 SCITriggerDataInterrupt()로 원격 인터럽트를 트리거할 수 있다.
 *
 * @intr:    초기화할 원격 인터럽트 구조체
 * @adapter: DIS 어댑터 번호
 * @node:    대상 원격 노드의 DIS 노드 식별자
 * @no:      대상 인터럽트 번호
 * @return:  성공 시 0, 실패 시 ECONNREFUSED
 */
int _nvm_remote_intr_get(struct remote_intr* intr, uint32_t adapter, uint32_t node, uint32_t no)
{
    sci_error_t err = SCI_ERR_OK;

    /* 원격 인터럽트 연결 전용 SISCI 세션을 연다 */
    SCIOpen(&intr->sd, 0, &err);
#ifndef NDEBUG
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to open SISCI virtual device: %s\n", _SCIGetErrorString(err));
        return EIO;
    }
#endif

    /* SCIConnectDataInterrupt: 원격 노드의 데이터 인터럽트에 연결한다.
     * node는 대상 노드 ID, adapter는 사용할 DIS 어댑터, no는 인터럽트 번호이다.
     * SCI_INFINITE_TIMEOUT으로 연결될 때까지 무한 대기한다. */
    SCIConnectDataInterrupt(intr->sd, &intr->intr, node, adapter, no, SCI_INFINITE_TIMEOUT, 0, &err);
    if (err != SCI_ERR_OK)
    {
        SCIClose(intr->sd, 0, &err);    /* 연결 실패 시 세션 닫기 */
        return ECONNREFUSED;
    }

    return 0;
}



/*
 * _nvm_remote_intr_put - 원격 데이터 인터럽트 연결을 해제한다.
 *
 * SCIDisconnectDataInterrupt()로 원격 인터럽트 연결을 끊고
 * SCIClose()로 전용 세션을 닫는다.
 *
 * @intr: 연결 해제할 원격 인터럽트 구조체
 */
void _nvm_remote_intr_put(struct remote_intr* intr)
{
    sci_error_t err = SCI_ERR_OK;
    /* SCIDisconnectDataInterrupt: 원격 데이터 인터럽트 연결을 해제한다 */
    SCIDisconnectDataInterrupt(intr->intr, 0, &err);
    /* 전용 SISCI 세션을 닫는다 */
    SCIClose(intr->sd, 0, &err);
}



/*
 * _nvm_remote_intr_trigger - 원격 인터럽트를 트리거하여 데이터를 전송한다.
 *
 * 이미 연결된 원격 인터럽트에 데이터를 실어 보낸다.
 * 원격 노드에서는 콜백이 호출되거나 SCIWaitForDataInterrupt()가 깨어난다.
 *
 * @intr:   트리거할 원격 인터럽트 핸들
 * @data:   전송할 데이터 버퍼
 * @length: 전송 데이터 길이(바이트)
 * @return: 성공 시 0, 실패 시 ENOTCONN
 */
int _nvm_remote_intr_trigger(const struct remote_intr* intr, void* data, uint16_t length)
{
    sci_error_t err = SCI_ERR_OK;

    /* SCITriggerDataInterrupt: 원격 데이터 인터럽트를 트리거한다.
     * data와 length로 지정된 데이터가 원격 노드로 전송된다.
     * 이것은 DIS 패브릭의 저지연 메시징 메커니즘이다. */
    SCITriggerDataInterrupt(intr->intr, data, length, 0, &err);
    if (err != SCI_ERR_OK)
    {
        dprintf("Failed to trigger data interrupt\n");
        return ENOTCONN;
    }

    return 0;
}



/*
 * _nvm_remote_intr_fire_and_forget - 원격 인터럽트에 연결, 전송, 연결 해제를 한번에 수행한다.
 *
 * 일회성 인터럽트 전송을 위한 편의 함수이다.
 * 내부적으로 연결 → 트리거 → 연결 해제 순서로 진행한다.
 * RPC 응답을 보내는 등 단발성 통신에 적합하다.
 *
 * @adapter: DIS 어댑터 번호
 * @node:    대상 원격 노드 식별자
 * @no:      대상 인터럽트 번호
 * @data:    전송할 데이터 버퍼
 * @len:     전송 데이터 길이
 * @return:  성공 시 0, 실패 시 적절한 errno 값
 */
int _nvm_remote_intr_fire_and_forget(uint32_t adapter, uint32_t node, uint32_t no, void* data, uint16_t len)
{
    int status = 0;
    struct remote_intr intr;

    /* 1단계: 원격 인터럽트에 연결 */
    status = _nvm_remote_intr_get(&intr, adapter, node, no);
    if (status != 0)
    {
        return status;
    }

    /* 2단계: 인터럽트를 트리거하여 데이터 전송 */
    status = _nvm_remote_intr_trigger(&intr, data, len);
    /* 3단계: 전송 성공/실패에 관계없이 연결을 해제한다 */
    _nvm_remote_intr_put(&intr);
    return status;
}
