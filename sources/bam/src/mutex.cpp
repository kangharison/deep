/* mutex.cpp - 플랫폼 독립적 뮤텍스 래퍼 구현 파일
 *
 * POSIX pthread 뮤텍스를 감싸는 래퍼 함수들을 제공한다.
 * NVMe 드라이버 내부에서 참조 카운트, 큐 접근 등의 동기화에 사용된다.
 * 현재는 Unix/Linux 계열만 지원한다.
 */
#ifdef __unix__
#include <pthread.h>      // POSIX 스레드 뮤텍스 API
#include <string.h>       // strerror
#endif

#include "mutex.h"        // 뮤텍스 구조체 및 함수 선언
#include "dprintf.h"      // 디버그 출력 매크로



/* _nvm_mutex_init - 뮤텍스를 초기화한다.
 * pthread_mutex_init을 호출하여 뮤텍스를 기본 속성으로 초기화한다.
 * 성공 시 0, 실패 시 에러 코드를 반환한다.
 */
#ifdef __unix__
int _nvm_mutex_init(struct mutex* mtx)
{
    int err;

    // pthread 뮤텍스를 기본 속성(NULL)으로 초기화
    err = pthread_mutex_init(&mtx->mutex, NULL);
    if (err != 0)
    {
        dprintf("Failed to initialize mutex: %s\n", strerror(err));
        return err;
    }

    return 0;
}
#endif



/* _nvm_mutex_free - 뮤텍스를 파괴한다.
 * pthread_mutex_destroy를 호출한다.
 * 뮤텍스가 잠겨있으면 EBUSY를 반환할 수 있다.
 */
#ifdef __unix__
int _nvm_mutex_free(struct mutex* mtx)
{
    // pthread 뮤텍스 파괴 (잠겨있으면 EBUSY 반환)
    return pthread_mutex_destroy(&mtx->mutex);
}
#endif



/* _nvm_mutex_lock - 뮤텍스를 잠근다 (임계 영역 진입).
 * 다른 스레드가 잠금을 해제할 때까지 블로킹된다.
 */
#ifdef __unix__
int _nvm_mutex_lock(struct mutex* mtx)
{
    // pthread 뮤텍스 잠금 (블로킹)
    pthread_mutex_lock(&mtx->mutex);
    return 0;
}
#endif



/* _nvm_mutex_unlock - 뮤텍스를 해제한다 (임계 영역 탈출).
 * 대기 중인 다른 스레드가 있으면 깨워준다.
 */
#ifdef __unix__
void _nvm_mutex_unlock(struct mutex* mtx)
{
    // pthread 뮤텍스 잠금 해제
    pthread_mutex_unlock(&mtx->mutex);
}
#endif
