/* mutex.h - 플랫폼 독립적 뮤텍스 래퍼 헤더
 *
 * NVMe 드라이버 내부의 동기화에 사용되는 뮤텍스 인터페이스를 정의한다.
 * 현재는 POSIX pthread 뮤텍스만 지원하며, 다른 OS는 컴파일 에러를 발생시킨다.
 * 참조 카운트 보호, 큐 접근 동기화, RPC 참조 락 등에 사용된다.
 */
#ifndef __NVM_INTERNAL_MUTEX_H__
#define __NVM_INTERNAL_MUTEX_H__

/* 전방 선언 */
struct mutex;


/* 지원하는 OS 확인.
 * pthread를 가진 Unix 계열만 지원한다.
 */
#if defined( __unix__ )
#include <pthread.h>      // pthread_mutex_t 타입
#else
#error "OS is not supported"
#endif



/* 뮤텍스 구조체 정의.
 * 포인터를 통한 간접 참조 없이 직접 pthread_mutex_t를 포함하여 성능을 최적화한다.
 * 따라서 구현 세부 사항이 헤더에 노출되지만, 이는 의도된 설계이다.
 */
#if defined( __unix__ )
struct mutex
{
    pthread_mutex_t mutex;    // POSIX 스레드 뮤텍스 핸들
};
#endif



/* _nvm_mutex_init - 뮤텍스를 초기화한다.
 * 사용 전 반드시 호출해야 한다.
 * 성공 시 0, 실패 시 에러 코드를 반환한다.
 */
int _nvm_mutex_init(struct mutex* mtx);



/* _nvm_mutex_free - 뮤텍스를 파괴한다.
 * 더 이상 사용하지 않을 때 호출한다.
 * 뮤텍스가 잠겨있으면 EBUSY를 반환할 수 있다.
 */
int _nvm_mutex_free(struct mutex* mtx);



/* _nvm_mutex_lock - 뮤텍스를 잠근다 (임계 영역 진입).
 * 이미 다른 스레드가 잠갔으면 해제될 때까지 블로킹된다.
 * 성공 시 0, 실패 시 에러 코드를 반환한다.
 */
int _nvm_mutex_lock(struct mutex* mtx);



/* _nvm_mutex_unlock - 뮤텍스를 해제한다 (임계 영역 탈출).
 * 잠금을 잡은 스레드만 호출해야 한다.
 */
void _nvm_mutex_unlock(struct mutex* mtx);



#endif /* __NVM_INTERNAL_MUTEX_H__ */
