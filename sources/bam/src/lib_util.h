/* lib_util.h - 내부 유틸리티 함수 및 매크로 헤더
 *
 * BAM 라이브러리 내부에서 공통으로 사용하는 유틸리티 함수와 매크로를 정의한다.
 * container_of 매크로, 최소/최대 매크로, log2 계산, 지연 함수, 시스템 페이지 크기 조회 등을 포함한다.
 */
#ifndef __NVM_INTERNAL_UTIL_H__
#define __NVM_INTERNAL_UTIL_H__

#include <nvm_util.h>     // 공개 유틸리티 매크로
#include <stddef.h>       // offsetof, size_t
#include <stdint.h>       // 정수형 타입

#if defined( __unix__ )
#include <time.h>         // clock_nanosleep, struct timespec
#include <unistd.h>       // sysconf(_SC_PAGESIZE)
#endif

#ifndef NDEBUG
#include <string.h>       // strerror (디버그 모드 전용)
#include <errno.h>        // errno (디버그 모드 전용)
#include "dprintf.h"      // 디버그 출력 (디버그 모드 전용)
#endif


/* _nvm_container_of - 구조체 멤버 포인터에서 포함하는 구조체의 포인터를 역추적하는 매크로.
 * Linux 커널의 container_of와 동일한 패턴이다.
 * 예: nvm_ctrl_t* 포인터에서 struct controller*를 얻을 때 사용한다.
 */
#if defined( __clang__ ) || defined( __GNUC__ )
// GCC/Clang: typeof를 사용한 타입 안전 버전
#define _nvm_container_of(ptr, type, member) ({                 \
        const typeof( ((type *) 0)->member )* __mptr = (ptr);   \
        (type *) (((unsigned char*) __mptr) - offsetof(type, member)); })
#else
// 다른 컴파일러: typeof 없이 단순 주소 계산
#define _nvm_container_of(ptr, type, member) \
    ((type *) (((unsigned char*) (ptr)) - ((unsigned char*) (&((type *) 0)->member))))
#endif


/* _MIN - 두 값 중 작은 값을 반환하는 매크로 */
#define _MIN(a, b) ( (a) <= (b) ? (a) : (b) )


/* _MAX - 두 값 중 큰 값을 반환하는 매크로 */
#define _MAX(a, b) ( (a) > (b) ? (a) : (b) )



/* _nvm_b2log - 정수 n의 밑이 2인 로그를 계산한다 (floor(log2(n))).
 * NVMe 레지스터의 크기 인코딩(예: CC.MPS, SQES, CQES)에 사용된다.
 * 예: _nvm_b2log(4096) = 12, _nvm_b2log(64) = 6
 */
static inline uint32_t _nvm_b2log(uint32_t n)
{
    uint32_t count = 0;

    // n이 0이 될 때까지 오른쪽으로 1비트씩 시프트하며 카운트
    while (n > 0)
    {
        ++count;
        n >>= 1;
    }

    // count-1이 floor(log2(n))과 같다 (n>0일 때)
    return count - 1;
}


#if defined( __unix__ )
/* _nvm_delay_remain - 최소 1밀리초만큼 대기하고 남은 시간을 반환한다.
 * NVMe 컨트롤러 리셋이나 CQ 폴링에서 타임아웃 대기에 사용된다.
 * remaining_nanoseconds가 1ms보다 작으면 그 시간만큼만 대기한다.
 */
static inline uint64_t _nvm_delay_remain(uint64_t remaining_nanoseconds)
{
    struct timespec ts;

    // 남은 시간이 0이면 바로 반환
    if (remaining_nanoseconds == 0)
    {
        return 0;
    }

    ts.tv_sec = 0;
    // 대기 시간: 남은 시간과 1ms(1,000,000ns) 중 작은 값
    ts.tv_nsec = _MIN(1000000UL, remaining_nanoseconds);

    // 실제 나노초 단위 슬립 수행 (CLOCK_REALTIME 기준)
    clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);

    // 남은 시간에서 대기한 만큼 차감
    remaining_nanoseconds -= _MIN(1000000UL, remaining_nanoseconds);
    return remaining_nanoseconds;
}
#endif


#if defined( __unix__ )
/* _nvm_host_page_size - 호스트 시스템의 페이지 크기를 반환한다.
 * sysconf를 통해 OS의 기본 메모리 페이지 크기를 조회한다 (보통 4KB = 4096).
 * NVMe 컨트롤러 초기화 시 호스트 페이지 크기와 컨트롤러 페이지 크기의 호환성을 검증할 때 사용한다.
 */
static inline size_t _nvm_host_page_size()
{
    // POSIX sysconf로 시스템 페이지 크기 조회
    long page_size = sysconf(_SC_PAGESIZE);

#ifndef NDEBUG
    // 디버그 모드에서 조회 실패 시 에러 출력
    if (page_size < 0)
    {
        dprintf("Failed to look up system page size: %s\n", strerror(errno));
        return 0;
    }
#endif

    return page_size;
}
#else
/* 비 Unix 환경에서는 기본 4KB 페이지 크기를 사용한다 */
#define _nvm_host_page_size()   0x1000
#endif


#endif /* __NVM_INTERNAL_UTIL_H__ */
