/* util.h - BaM 프로젝트 공통 유틸리티 함수 및 매크로 모음
 * CUDA 에러 체크 매크로, 올림(ceil) 매크로, 메모리 hex 덤프, warp 단위 memcpy 등
 * GPU/CPU 양쪽에서 공통으로 사용하는 유틸리티를 제공한다.
 */
#ifndef __UTIL_H__
#define __UTIL_H__

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif
#ifndef __forceinline__
#define __forceinline__ inline
#endif



#include "cuda.h"
#include "nvm_util.h"
#include "host_util.h"
//#include <ctype>
#include <cstdio>


/* cuda_err_chk: CUDA API 호출 결과를 확인하는 매크로. 에러 발생 시 파일명과 줄 번호를 출력한다. */
#define cuda_err_chk(ans) { gpuAssert((ans), __FILE__, __LINE__); }

/* CUDA 컴파일러가 아닌 경우의 gpuAssert: 일반 정수 에러 코드를 받아 0이 아니면 에러를 출력한다. */
#ifndef __CUDACC__
inline void gpuAssert(int code, const char *file, int line, bool abort=false)
{
    if (code != 0)
    {
	fprintf(stderr,"Assert: %i %s %d\n", code, file, line);
	if (abort) exit(1);
    }
}
#else
/* CUDA 컴파일러인 경우의 gpuAssert: cudaError_t를 받아 cudaSuccess가 아니면 에러 메시지를 출력한다. */
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=false)
{
    if (code != cudaSuccess)
    {
	fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
	if (abort) exit(1);
    }
}
#endif

/* CEIL 매크로: (X + Y - 1)을 Z 비트만큼 오른쪽 시프트하여 올림 나눗셈을 수행한다.
 * 예: CEIL(num_bytes, block_size, block_size_log) => 바이트 수를 블록 수로 올림 변환
 */
#define CEIL(X, Y, Z) ((X + Y - 1) >> Z)


/* hexdump: 메모리 내용을 16진수로 덤프하는 디버그용 디바이스 함수이다.
 * NVMe 커맨드 또는 데이터 버퍼의 내용을 확인할 때 사용한다.
 */
#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16 // 한 줄에 출력할 바이트 수
#endif
inline __device__ void hexdump(void *mem, unsigned int len)
{
        unsigned int i;

        // 전체 길이를 HEXDUMP_COLS 단위로 정렬하여 반복한다
        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* 줄 시작마다 오프셋을 출력한다 */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("\n0x%06x: ", i);
                }

                /* 유효 범위이면 16진수 바이트 값을 출력한다 */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* 블록 끝 정렬을 위해 빈 공간을 출력한다 */
                {
                        printf("   ");
                }

                /* ASCII 덤프 부분은 주석 처리되어 있다 */
//                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
//                {
//                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
//                        {
//                                if(j >= len) /* end of block, not really printing */
//                                {
//                                        printf(' ');
//                                }
//                                else if(isprint(((char*)mem)[j])) /* printable char */
//                                {
//                                        printf(0xFF & ((char*)mem)[j]);
//                                }
//                                else /* other char */
//                                {
//                                        putchar('.');
//                                }
//                        }
//                        putchar('\n');
//                }
        }
        printf("\n");
}

/* __ignore: 사용하지 않는 변수의 컴파일러 경고를 억제하기 위한 템플릿 함수이다.
 * 예: __ignore(ptr) 로 호출하면 ptr이 사용된 것으로 처리된다.
 */
template <typename T>
void __ignore(T &&)
{ }

/* warp_memcpy: warp 내 스레드들이 협력하여 메모리를 복사하는 디바이스 함수이다.
 * 각 스레드가 자신의 레인에 해당하는 원소를 담당하여 병렬로 복사한다.
 * T 타입 단위로 정렬되어 있어야 하며, num은 T 타입 원소 개수이다.
 */
template <typename T>
inline __device__
void warp_memcpy(T* dest, const T* src, size_t num) {
#ifndef __CUDACC__
    uint32_t mask = 1; // 호스트에서는 단일 스레드
#else
    uint32_t mask = __activemask(); // 현재 활성 스레드 마스크를 가져온다
#endif
        uint32_t active_cnt = __popc(mask);        // 활성 스레드 수를 센다
        uint32_t lane = lane_id();                  // 현재 스레드의 warp 내 레인 ID를 가져온다
        uint32_t prior_mask = mask >> (32 - lane);  // 자신보다 앞 레인의 마스크를 계산한다
        uint32_t prior_count = __popc(prior_mask);  // 자신보다 앞의 활성 스레드 수를 센다

        // 각 스레드가 stride(active_cnt) 간격으로 원소를 복사한다
        for(size_t i = prior_count; i < num; i+=active_cnt)
                dest[i] = src[i];
}

//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#undef __forceinline__
//#endif

#endif // __UTIL_H__
