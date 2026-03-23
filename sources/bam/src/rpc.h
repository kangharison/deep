/* rpc.h - RPC 내부 타입 및 함수 선언 헤더
 *
 * NVMe Admin 명령을 실행하기 위한 RPC(Remote Procedure Call) 인프라의 내부 인터페이스를 정의한다.
 * 로컬 모드(직접 Admin 큐 접근)와 원격 모드(네트워크 RPC)를 추상화하는 콜백 타입,
 * Admin 참조 관리, 바인딩 핸들 삽입/제거 함수를 선언한다.
 * 이 헤더는 내부 전용이며 외부 API에는 포함되지 않는다.
 */
#ifndef __NVM_INTERNAL_RPC_H__
#define __NVM_INTERNAL_RPC_H__

#include <nvm_types.h>    // nvm_cmd_t, nvm_cpl_t, nvm_ctrl_t, nvm_aq_ref 타입
#include <nvm_queue.h>    // nvm_queue_t 타입
#include <stdbool.h>      // bool 타입
#include <stdint.h>       // 정수형 타입
#include <pthread.h>      // pthread 타입 (뮤텍스 등)


/* 전방 선언 */
struct nvm_admin_reference;



/* rpc_free_handle_t - 로컬 바인딩 핸들의 인스턴스 데이터를 해제하는 콜백 타입.
 * RPC 서버측에서 클라이언트별 핸들을 제거할 때 호출된다.
 * key: 핸들 식별 키, data: 해제할 인스턴스 데이터
 */
typedef void (*rpc_free_handle_t)(uint32_t key, void* data);



/* rpc_free_binding_t - 원격 바인딩 참조의 인스턴스 데이터를 해제하는 콜백 타입.
 * Admin 참조가 해제될 때 바인딩된 데이터를 정리하기 위해 호출된다.
 * 로컬 모드에서는 local_admin 구조체를 해제하는 remove_admin이 설정된다.
 */
typedef void (*rpc_free_binding_t)(void* data);



/* rpc_stub_t - RPC 클라이언트측 스텁 함수 타입.
 * Admin 명령 실행 시 호출되는 함수 포인터이다.
 * 로컬 모드: execute_command (직접 ASQ에 명령 제출)
 * 원격 모드: 네트워크 전송 스텁 (명령 마샬링 → 전송 → 완료 대기 → 언마샬링)
 *
 * 수행해야 하는 동작:
 *   - 명령을 마샬링
 *   - 원격 호스트로 명령 전송
 *   - 완료를 대기 (또는 타임아웃)
 *   - 완료를 언마샬링하고 상태 반환
 */
typedef int (*rpc_stub_t)(void*, nvm_cmd_t*, nvm_cpl_t*);



/* _nvm_ref_get - Admin 참조 래퍼를 할당하고 컨트롤러 참조를 증가시킨다.
 * 새로운 nvm_admin_reference를 동적 할당하고, 뮤텍스를 초기화하고,
 * 컨트롤러의 참조 카운트를 증가시킨다.
 */
int _nvm_ref_get(nvm_aq_ref* handle, const nvm_ctrl_t* ctrl);



/* _nvm_ref_put - Admin 참조 래퍼를 해제하고 컨트롤러 참조를 감소시킨다.
 * 모든 바인딩 핸들을 제거하고, 인스턴스 데이터를 해제하고,
 * 컨트롤러 참조 카운트를 감소시킨 후 메모리를 반환한다.
 */
void _nvm_ref_put(nvm_aq_ref ref);



/* _nvm_rpc_handle_insert - 서버의 바인딩 핸들 리스트에 새 핸들을 삽입한다.
 * 키가 이미 존재하면 실패한다 (중복 방지).
 * 연결 리스트의 끝에 새 핸들 노드를 추가한다.
 */
int _nvm_rpc_handle_insert(nvm_aq_ref ref, uint32_t key, void* data, rpc_free_handle_t release);



/* _nvm_rpc_handle_remove - 지정된 키의 바인딩 핸들을 리스트에서 제거한다.
 * 제거 시 release 콜백을 호출하여 인스턴스 데이터를 정리한다.
 */
void _nvm_rpc_handle_remove(nvm_aq_ref ref, uint32_t key);



/* _nvm_rpc_bind - Admin 참조를 원격 핸들에 바인딩한다.
 * 스텁 함수와 인스턴스 데이터를 설정한다.
 * 이미 바인딩된 상태이면 실패한다 (이중 바인딩 방지).
 */
int _nvm_rpc_bind(nvm_aq_ref ref, void* data, rpc_free_binding_t release, rpc_stub_t stub);



/* _nvm_local_admin - 로컬 Admin 참조를 통해 직접 명령을 실행한다.
 * 스텁이 execute_command인지 검증하여 로컬 전용임을 보장한다.
 * 원격 바인딩된 참조로 호출하면 에러를 반환한다.
 */
int _nvm_local_admin(nvm_aq_ref ref, const nvm_cmd_t* cmd, nvm_cpl_t* cpl);



#endif /* __NVM_INTERNAL_RPC_H__ */
