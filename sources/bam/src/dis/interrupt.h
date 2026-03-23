/* interrupt.h - DIS/SISCI 데이터 인터럽트 관리 헤더
 *
 * 이 파일은 DIS 패브릭을 통한 노드 간 데이터 인터럽트 통신을 위한
 * 로컬/원격 인터럽트 구조체와 함수 선언을 제공한다.
 * SISCI 데이터 인터럽트는 PCIe 패브릭 위에서 노드 간에 소량의 데이터를
 * 저지연으로 전송하는 메커니즘으로, RPC 요청/응답에 핵심적으로 사용된다.
 */
#ifndef __NVM_INTERNAL_DIS_INTERRUPT_H__
#define __NVM_INTERNAL_DIS_INTERRUPT_H__
#ifdef _SISCI

/* SISCI 클러스터 모드 활성화를 보장하는 매크로 */
#ifndef __DIS_CLUSTER__
#define __DIS_CLUSTER__
#endif

/* 필수 헤더 포함 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sisci_types.h>  /* sci_desc_t, sci_local_data_interrupt_t, sci_remote_data_interrupt_t 등 */



/* 전방 선언 */
struct local_intr;
struct remote_intr;



/*
 * intr_callback_t - 인터럽트 수신 시 호출되는 콜백 함수 타입
 *
 * @user_data: 인터럽트 생성 시 등록한 사용자 데이터 포인터
 * @recv_data: 인터럽트와 함께 수신된 데이터 버퍼
 * @length:    수신 데이터의 길이(바이트)
 */
typedef void (*intr_callback_t)(void* user_data, void* recv_data, uint16_t length);



/*
 * local_intr - 로컬 데이터 인터럽트 디스크립터
 *
 * SCICreateDataInterrupt()로 생성한 로컬 인터럽트를 관리한다.
 * 원격 노드가 이 인터럽트 번호로 SCITriggerDataInterrupt()를 호출하면
 * 등록된 콜백이 실행되거나, SCIWaitForDataInterrupt()에서 블로킹 대기가 해제된다.
 * data는 사용자가 수동으로 해제해야 한다.
 */
struct local_intr
{
    sci_desc_t                  sd;         /* SISCI 가상 디바이스 디스크립터: 이 인터럽트 전용 세션 */
    sci_local_data_interrupt_t  intr;       /* SISCI 로컬 데이터 인터럽트 핸들 */
    uint32_t                    adapter;    /* DIS 어댑터 번호: 이 인터럽트가 바인딩된 패브릭 어댑터 */
    uint32_t                    intr_no;    /* 인터럽트 번호: SCICreateDataInterrupt()가 자동 할당한 고유 번호 */
    uint32_t                    node_id;    /* 이 노드의 DIS 노드 식별자: SCIGetLocalNodeId()로 조회 */
    void*                       data;       /* 콜백에 전달될 사용자 데이터 포인터 */
    intr_callback_t             callback;   /* 인터럽트 수신 시 호출되는 콜백 함수 */
};



/*
 * remote_intr - 원격 데이터 인터럽트 디스크립터
 *
 * SCIConnectDataInterrupt()로 원격 노드의 인터럽트에 연결한 핸들을 관리한다.
 * 이 핸들을 통해 SCITriggerDataInterrupt()로 원격 노드에 인터럽트를 발생시킬 수 있다.
 */
struct remote_intr
{
    sci_desc_t                  sd;         /* SISCI 가상 디바이스 디스크립터: 이 연결 전용 세션 */
    sci_remote_data_interrupt_t intr;       /* SISCI 원격 데이터 인터럽트 핸들: 원격 노드의 인터럽트에 대한 참조 */
};



/*
 * _nvm_local_intr_get - 로컬 데이터 인터럽트를 생성한다.
 *
 * SCIOpen()으로 새 세션을 열고, SCICreateDataInterrupt()로 인터럽트를 생성한다.
 * cb가 NULL이 아니면 콜백 모드로 동작하고, NULL이면 폴링/대기 모드로 동작한다.
 *
 * @intr:    초기화할 로컬 인터럽트 구조체
 * @adapter: DIS 어댑터 번호
 * @cb_data: 콜백에 전달할 사용자 데이터
 * @cb_func: 인터럽트 수신 시 호출할 콜백 (NULL이면 콜백 없이 대기 모드)
 */
int _nvm_local_intr_get(struct local_intr* intr,
                        uint32_t adapter,
                        void* cb_data,
                        intr_callback_t cb_func);



/*
 * _nvm_local_intr_put - 로컬 데이터 인터럽트를 제거한다.
 *
 * SCIRemoveDataInterrupt()로 인터럽트를 해제하고 SCIClose()로 세션을 닫는다.
 */
void _nvm_local_intr_put(struct local_intr* intr);



/*
 * _nvm_local_intr_wait - 데이터 인터럽트 수신을 블로킹 대기한다.
 *
 * SCIWaitForDataInterrupt()를 호출하여 timeout 밀리초 동안 대기한다.
 * 수신된 데이터는 data 버퍼에 복사된다.
 *
 * @intr:    대기할 로컬 인터럽트
 * @data:    수신 데이터를 저장할 버퍼
 * @maxlen:  버퍼 최대 크기
 * @timeout: 대기 타임아웃(밀리초)
 */
int _nvm_local_intr_wait(struct local_intr* intr, void* data, uint16_t maxlen, uint32_t timeout);



/*
 * _nvm_remote_intr_get - 원격 노드의 데이터 인터럽트에 연결한다.
 *
 * SCIConnectDataInterrupt()를 호출하여 지정된 노드/인터럽트 번호에 연결한다.
 *
 * @intr:     초기화할 원격 인터럽트 구조체
 * @adapter:  DIS 어댑터 번호
 * @node_id:  대상 원격 노드의 DIS 노드 식별자
 * @intr_no:  대상 인터럽트 번호
 */
int _nvm_remote_intr_get(struct remote_intr* intr, uint32_t adapter, uint32_t node_id, uint32_t intr_no);



/*
 * _nvm_remote_intr_put - 원격 데이터 인터럽트 연결을 해제한다.
 *
 * SCIDisconnectDataInterrupt()로 연결을 끊고 SCIClose()로 세션을 닫는다.
 */
void _nvm_remote_intr_put(struct remote_intr* intr);



/*
 * _nvm_remote_intr_trigger - 원격 인터럽트를 트리거하여 데이터를 전송한다.
 *
 * SCITriggerDataInterrupt()를 호출하여 원격 노드에 인터럽트와 데이터를 보낸다.
 *
 * @intr:   트리거할 원격 인터럽트 핸들
 * @data:   전송할 데이터
 * @len:    전송 데이터 길이
 */
int _nvm_remote_intr_trigger(const struct remote_intr* intr, void* data, uint16_t len);



/*
 * _nvm_remote_intr_fire_and_forget - 원격 인터럽트에 연결하고, 데이터를 보내고, 즉시 연결을 해제한다.
 *
 * 일회성 인터럽트 전송을 위한 편의 함수로, 내부적으로
 * _nvm_remote_intr_get() → _nvm_remote_intr_trigger() → _nvm_remote_intr_put() 순서로 호출한다.
 *
 * @adapter: DIS 어댑터 번호
 * @node_id: 대상 원격 노드 식별자
 * @intr_no: 대상 인터럽트 번호
 * @data:    전송할 데이터
 * @len:     전송 데이터 길이
 */
int _nvm_remote_intr_fire_and_forget(uint32_t adapter,
                                     uint32_t node_id,
                                     uint32_t intr_no,
                                     void* data,
                                     uint16_t len);

#endif /* _SISCI */
#endif /* __NVM_INTERNAL_DIS_INTERRUPT_H__ */
