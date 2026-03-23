/* nvm_rpc.h - NVMe 어드민 커맨드 RPC(Remote Procedure Call) 헤더 */
/* 원격 프로세스로의 어드민 큐 바인딩 및 커맨드 중계 기능을 선언한다 */
/* 여러 프로세스/노드가 하나의 NVMe 컨트롤러를 공유할 때 사용된다 */
#ifndef __NVM_RPC_H__
#define __NVM_RPC_H__

// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <nvm_types.h>
#include <stdint.h>


//int nvm_tcp_rpc_bind(nvm_aq_ref* ref, const char* hostname, uint16_t port);



#ifdef __DIS_CLUSTER__

/*
 * Bind admin queue-pair reference to remote handle.
 * 어드민 큐 쌍 참조를 원격 핸들에 바인딩한다.
 *
 * DIS 클러스터에서 원격 노드의 어드민 큐에 접근하기 위한 참조를 생성한다.
 * 바인딩을 해제하려면 nvm_aq_destroy()를 호출한다.
 *
 * The user should call the nvm_aq_destroy() to remove binding.
 */
int nvm_dis_rpc_bind(nvm_aq_ref* ref,          /* [out] 바인딩된 어드민 큐 참조 핸들 */
                     const nvm_ctrl_t* ctrl,    /* NVMe 컨트롤러 핸들 */
                     uint32_t adapter);          /* 원격 접속에 사용할 DIS 어댑터 */

#endif



/*
 * Unbind admin queue-pair reference.
 * 어드민 큐 쌍 참조의 바인딩을 해제한다.
 *
 * 참조가 바인딩되지 않은 경우(로컬 참조)에는 아무 동작도 하지 않는다.
 *
 * If reference is not bound (i.e., it is local), this function will do nothing.
 */
void nvm_rpc_unbind(nvm_aq_ref ref);  /* 바인딩 해제할 어드민 큐 참조 핸들 */



/*
 * Relay NVM admin command.
 * NVMe 어드민 커맨드를 중계한다.
 *
 * 로컬 AQ 쌍 참조를 사용하여 NVMe 어드민 커맨드를 ASQ에 제출하고,
 * ACQ에서 대응하는 컴플리션을 받을 때까지 블로킹한다.
 * 타임아웃이 발생하거나 커맨드가 완료될 때까지 대기한다.
 *
 * 반환값:
 * - 0: 성공
 * - 양수: errno (시스템 에러)
 * - 음수: NVMe 에러 (nvm_error.h의 매크로로 해석 가능)
 *
 * Use a local AQ pair reference to relay a NVM admin command to ASQ and get
 * a corresponding completion from the ACQ. This function will block until
 * either a timeout occurs or until the command is completed.
 *
 * Return value:
 * - If return value is zero, it indicates success.
 * - If return value is positive, it indicates an errno.
 * - If return value is negative, it indicates an NVM error.
 *
 * Use the error handling macros in nvm_error.h
 *
 * Note: The command can be modified.
 */
int nvm_raw_rpc(nvm_aq_ref ref,   /* 어드민 큐 쌍 참조 핸들 */
                nvm_cmd_t* cmd,    /* [in/out] 제출할 NVMe 어드민 커맨드 (수정될 수 있음) */
                nvm_cpl_t* cpl);   /* [out] 수신된 컴플리션 결과 */




#endif /* #ifdef __NVM_RPC_H__ */
