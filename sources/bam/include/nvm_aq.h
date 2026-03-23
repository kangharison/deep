/* nvm_aq.h - NVMe 어드민 큐 쌍 생성/파괴 및 원격 RPC 제어 API 헤더 */
/* 컨트롤러 소유권 획득(리셋 + 어드민 큐 설정)과 원격 어드민 커맨드 중계를 관리한다 */
#ifndef __NVM_AQ_H__
#define __NVM_AQ_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <nvm_types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>



/*
 * Create admin queue pair
 * 어드민 큐 쌍(Admin SQ + Admin CQ)을 생성한다.
 *
 * NVMe 컨트롤러의 배타적 소유권을 획득한다.
 * 이 함수는 컨트롤러를 리셋하고 어드민 큐를 설정한다.
 * 반환되는 참조 핸들은 이후 어드민 RPC 호출에 사용된다.
 * dma_window는 어드민 큐 메모리로 사용할 DMA 매핑이며,
 * 최소 2페이지(ASQ 1페이지 + ACQ 1페이지)가 필요하다.
 *
 * Take exclusive ownership of an NVM controller. This function resets the
 * controller and configures NVM admin queues.
 *
 * Returns a reference handle that can be used for admin RPC calls.
 */
int nvm_aq_create(nvm_aq_ref* ref,              /* [out] 생성된 어드민 큐 참조 핸들 */
                  const nvm_ctrl_t* ctrl,        /* NVMe 컨트롤러 핸들 */
                  const nvm_dma_t* dma_window);  /* 어드민 큐 메모리로 사용할 DMA 윈도우 (최소 2페이지) */


/*
 * Destroy admin queues and references.
 * 어드민 큐와 참조를 파괴한다.
 *
 * NVM abort 커맨드를 컨트롤러에 전송하고 어드민 큐를 해제한다.
 * 이 함수 호출 후 모든 어드민 큐 참조(로컬 및 원격)가 무효화된다.
 * 원격 참조의 바인딩 해제에도 사용할 수 있다.
 *
 * Send NVM abort command to controller and deallocate admin queues.
 *
 * After calling this function, all admin queue references are invalid.
 * This also means that remote references will no longer be valid.
 *
 * This function will also work for unbinding remote references.
 */
void nvm_aq_destroy(nvm_aq_ref ref);  /* 파괴할 어드민 큐 참조 핸들 */



//int nvm_tcp_rpc_enable(nvm_aq_ref ref, uint16_t port, nvm_rpc_cb_t filter, void* data);
//int nvm_tcp_rpc_disable(nvm_aq_ref ref, uint16_t port);



#ifdef __DIS_CLUSTER__


/*
 * Callback function invoked whenever a remote NVM admin command is received.
 * 원격 NVM 어드민 커맨드가 수신될 때마다 호출되는 콜백 함수 타입.
 *
 * 원격 어드민 커맨드의 수락 여부를 반환값으로 결정한다.
 * 필요한 경우 원격 커맨드를 수정할 수도 있다.
 *
 * Should indicate whether or not a remote admin command is accepted and can
 * be enqueued by using the return value.
 *
 * The remote command can also be modified if necessary.
 */
typedef bool (*nvm_dis_rpc_cb_t)(nvm_cmd_t* cmd,        /* 수신된 NVMe 어드민 커맨드 (수정 가능) */
                                 uint32_t dis_adapter,   /* 원격 요청이 도착한 DIS 어댑터 */
                                 uint32_t dis_node_id);  /* 원격 요청을 보낸 노드 ID */



/*
 * Enable remote admin commands.
 * 원격 어드민 커맨드를 활성화한다.
 *
 * 원격 프로세스가 NVM 어드민 커맨드를 로컬 프로세스로 중계할 수 있게 한다.
 * DIS 인터럽트를 통해 원격 요청을 수신한다.
 *
 * Allows remote processes to relay NVM admin commands to the local process.
 */
int nvm_dis_rpc_enable(nvm_aq_ref ref,               // NVM admin queue-pair reference (어드민 큐 쌍 참조 핸들)
                       uint32_t dis_adapter,         // Local adapter to enable interrupt on (인터럽트를 활성화할 로컬 DIS 어댑터)
                       nvm_dis_rpc_cb_t filter);     // Filter callback (can be NULL) (필터 콜백, NULL이면 모든 커맨드 수락)



/*
 * Disable remote admin commands.
 * 원격 어드민 커맨드를 비활성화한다.
 *
 * 원격 프로세스의 어드민 커맨드 처리를 중지한다.
 *
 * Stop processing admin commands from remote processes.
 */
void nvm_dis_rpc_disable(nvm_aq_ref ref,         /* 어드민 큐 쌍 참조 핸들 */
                         uint32_t dis_adapter);   /* 인터럽트를 비활성화할 DIS 어댑터 */

#endif /* __DIS_CLUSTER__ */




#endif /* #ifdef __NVM_AQ_H__ */
