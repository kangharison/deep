/* dprintf.h - 디버그 출력 매크로 헤더
 *
 * NDEBUG가 정의되지 않은 디버그 빌드에서 stderr로 디버그 메시지를 출력하는 매크로를 제공한다.
 * 릴리즈 빌드(NDEBUG 정의 시)에서는 모든 디버그 출력이 컴파일 시점에 제거된다.
 * dprintf 매크로는 자동으로 호출 함수 이름을 앞에 붙여준다.
 */
#ifndef __NVM_INTERNAL_DPRINTF_H__
#define __NVM_INTERNAL_DPRINTF_H__

#ifndef NDEBUG
/* 디버그 모드: 실제 출력 함수 및 매크로 정의 */

#include <nvm_util.h>     // 유틸리티 매크로
#include <nvm_error.h>    // nvm_strerror 함수
#include <stdio.h>        // fprintf, vfprintf, stderr
#include <stdarg.h>       // 가변 인자 처리 (va_list, va_start, va_end)

/* _nvm_dprintf - 디버그 메시지를 stderr로 출력하는 내부 함수.
 * 함수 이름을 대괄호로 감싸서 접두사로 출력하고, 이어서 형식 문자열을 출력한다.
 * 예: [nvm_raw_ctrl_reset] Timeout exceeded while waiting for controller reset
 */
static void _nvm_dprintf(const char* func, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    // 함수 이름을 접두사로 출력
    fprintf(stderr, "[%s] ", func);
    // 가변 인자 형식 문자열 출력
    vfprintf(stderr, format, args);
    va_end(args);
}

/* dprintf - 디버그 출력 매크로. __func__을 자동으로 전달한다. */
#define dprintf(...)            _nvm_dprintf(__func__, __VA_ARGS__)

/* _nvm_strerror - NVMe 에러 상태를 문자열로 변환하는 디버그 전용 매크로 */
#define _nvm_strerror(status)   nvm_strerror(status)
/* _SCIGetErrorString - SISCI 에러를 문자열로 변환하는 디버그 전용 매크로 */
#define _SCIGetErrorString(err) SCIGetErrorString(err)

#endif /* ! NDEBUG */



/* 디버그 출력이 비활성화된 경우(NDEBUG 정의 시) 매크로를 빈 문장으로 치환 */
#ifndef dprintf
#define dprintf(...)
#endif



/* 에러 문자열 변환도 비활성화 (릴리즈 빌드에서는 에러 문자열 조회를 건너뜀) */
#ifndef _nvm_strerror
#define _nvm_strerror(status)
#define _SCIGetErrorString(err)
#endif


#endif /* __NVM_INTERNAL_DPRINTF_H__ */
