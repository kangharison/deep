/* event.h - CUDA 이벤트 래퍼 구조체
 * GPU 커널 실행 시간을 측정하기 위한 CUDA 이벤트를 RAII 패턴으로 감싸는 유틸리티 구조체이다.
 * 생성 시 자동으로 이벤트를 기록하고, 두 이벤트 간 차이로 경과 시간(마이크로초)을 계산할 수 있다.
 */
#ifndef __BENCHMARK_EVENT_H__
#define __BENCHMARK_EVENT_H__
// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include "cuda.h"
#include <string>
#include <stdexcept>


/* Event 구조체: CUDA 이벤트를 감싸서 GPU 커널 실행 시간 측정을 쉽게 해주는 RAII 래퍼이다. */
struct Event
{
    cudaEvent_t event; // CUDA 이벤트 핸들, 타임스탬프 기록에 사용

    /* 생성자: CUDA 이벤트를 생성하고 지정된 스트림에 기록한다.
     * stream이 0이면 기본 스트림에 기록한다.
     */
    inline Event(cudaStream_t stream = 0)
    {
        // CUDA 이벤트를 기본 플래그로 생성한다
        auto err = cudaEventCreateWithFlags(&event, cudaEventDefault);
        if (err != cudaSuccess)
        {
            throw std::runtime_error(std::string("Failed to create event: ") + cudaGetErrorString(err));
        }

        // 이벤트를 지정된 스트림에 기록하여 타임스탬프를 찍는다
        err = cudaEventRecord(event, stream);
        if (err != cudaSuccess)
        {
            throw std::runtime_error(std::string("Failed to record event on stream: ") + cudaGetErrorString(err));
        }

    }

    /* 소멸자: CUDA 이벤트 리소스를 해제한다 */
    inline ~Event()
    {
        cudaEventDestroy(event);
    }

    /* operator-: 두 이벤트 간의 경과 시간을 마이크로초(us) 단위로 반환한다.
     * cudaEventElapsedTime은 밀리초를 반환하므로 1e3을 곱해 마이크로초로 변환한다.
     */
    inline double operator-(const Event& other) const
    {
        float msecs = 0;
        // other 이벤트부터 this 이벤트까지의 경과 시간(ms)을 계산한다
        auto err = cudaEventElapsedTime(&msecs, other.event, event);
        if (err != cudaSuccess)
        {
            throw std::runtime_error(std::string("Could not calculate elapsed time: ") + cudaGetErrorString(err));
        }

        // 밀리초를 마이크로초로 변환하여 반환한다
        return ((double) msecs) * 1e3;
    }
};


#endif
