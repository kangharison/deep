/* host_util.h - CUDA 내장 함수의 호스트 측 폴백(fallback) 구현
 * CUDA 커널에서 사용하는 __nanosleep, __activemask, __popc, __ffs, __syncwarp, __shfl_sync, __match_any_sync 등의
 * 내장 함수들을 CUDA 컴파일러 없이도 호스트 코드에서 컴파일할 수 있도록 대체 구현을 제공한다.
 * 호스트에서는 단일 스레드로 동작하므로, warp 관련 함수들은 단순화된 동작을 한다.
 */
#ifndef __HOST_UTIL_H_
#define __HOST_UTIL_H_

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif
#ifndef __forceinline__
#define __forceinline__ inline
#endif

#include <ctime>

/* CUDA 컴파일러가 아닌 경우에만 호스트 폴백 함수들을 정의한다 */
#ifndef __CUDACC__

/* __nanosleep: 호스트 측에서 나노초 단위 sleep을 수행하는 폴백 함수이다.
 * GPU에서는 CUDA 내장 __nanosleep을 사용하지만, CPU에서는 POSIX nanosleep을 사용한다.
 */
template<typename T>
inline __host__
void __nanosleep(T ns) {
        struct timespec time1,time2;
        time1.tv_sec  = 0;     // 초 단위는 0으로 설정한다
        time2.tv_nsec = ns;    // 나노초 단위로 대기 시간을 설정한다
        nanosleep(&time1, &time2); // POSIX nanosleep 호출로 스레드를 잠시 대기시킨다
}

/* __activemask: 호스트에서는 warp 개념이 없으므로 항상 1을 반환한다.
 * GPU에서는 현재 활성화된 스레드의 비트마스크를 반환하지만, CPU에서는 단일 스레드만 있다.
 */
template<typename T>
inline __host__
T __activemask() {
    T var;
    (void) var; // 사용하지 않는 변수 경고를 억제한다
    return (T)1; // 호스트에서는 항상 1개 스레드만 활성이므로 1을 반환한다
}

/* __popc: 비트 단위 population count (1인 비트의 개수)를 반환한다.
 * GCC 내장 함수 __builtin_popcount/__builtin_popcountll을 사용한다.
 */
template<typename T>
inline __host__
int __popc(T v) {
    if (sizeof(T) == 4)
        return __builtin_popcount((unsigned)v);       // 32비트 정수의 popcount
    if (sizeof(T) == 8)
        return __builtin_popcountll((unsigned long long)v); // 64비트 정수의 popcount
    return 0;

}

/* __ffs: find first set bit - 가장 낮은 위치의 1인 비트 위치를 반환한다 (1-indexed).
 * GCC 내장 함수 __builtin_ffs/__builtin_ffsll을 사용한다.
 */
template<typename T>
inline __host__
int __ffs(T v) {
    if (sizeof(T) == 4)
        return __builtin_ffs((int)v);          // 32비트 정수에서 최하위 set bit 위치 반환
    if (sizeof(T) == 8)
        return __builtin_ffsll((long long)v);  // 64비트 정수에서 최하위 set bit 위치 반환
    return 0;

}

/* __syncwarp: 호스트에서는 warp 동기화가 필요 없으므로 아무 동작도 하지 않는다.
 * GPU에서는 warp 내 스레드 동기화 배리어로 사용된다.
 */
template<typename T>
inline __host__
void __syncwarp(T mask) {
    (void) mask; // mask 파라미터를 무시한다
    return;      // 호스트에서는 동기화할 필요 없다
}

/* __shfl_sync: warp 내 shuffle 연산의 호스트 폴백이다.
 * GPU에서는 지정된 레인의 값을 읽어오지만, 호스트에서는 자기 자신의 값을 그대로 반환한다.
 */
template<typename T>
inline __host__
T __shfl_sync(unsigned mask, T var, int srcLane, int width=32) {
    (void) mask;    // 마스크 무시
    (void) srcLane; // 소스 레인 무시
    (void) width;   // warp 폭 무시
    return var;     // 호스트에서는 자신의 값을 그대로 반환한다
}

/* __match_any_sync: warp 내에서 같은 값을 가진 스레드들의 마스크를 반환하는 함수의 호스트 폴백이다.
 * 호스트에서는 단일 스레드이므로 항상 1을 반환한다.
 */
template<typename T>
inline __host__
unsigned int __match_any_sync(unsigned mask, T var) {
    (void) mask; // 마스크 무시
    (void) var;  // 비교 값 무시
    return 1;    // 호스트에서는 항상 자기 자신만 매칭되므로 1을 반환한다
}

#endif

//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#undef __forceinline__
//#endif

#endif // __HOST_UTIL_H_
