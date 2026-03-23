/* buffer.h - DMA 버퍼 및 GPU 디바이스 메모리 할당/매핑 유틸리티
 * NVMe 컨트롤러가 PCIe를 통해 직접 접근할 수 있는 DMA 매핑 메모리와
 * GPU CUDA 디바이스 메모리를 할당하고 관리하는 함수들을 제공한다.
 * std::shared_ptr 기반 RAII 패턴으로 메모리 해제를 자동화한다.
 */
#ifndef __BENCHMARK_BUFFER_H__
#define __BENCHMARK_BUFFER_H__


// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <memory>
#include <cstddef>
#include <cstdint>
#include "cuda.h"
#include "nvm_types.h"
#include "nvm_dma.h"
#include "nvm_util.h"
#include "nvm_error.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <new>
#include <cstdlib>
#include <iostream>
#include "util.h"

using error = std::runtime_error;
using std::string;



/* DmaPtr: NVMe DMA 매핑 메모리의 shared_ptr 타입. 소멸 시 자동으로 DMA 언매핑 및 메모리 해제를 수행한다. */
typedef std::shared_ptr<nvm_dma_t> DmaPtr;

/* BufferPtr: GPU 디바이스 메모리 또는 호스트 메모리의 shared_ptr 타입. 소멸 시 자동으로 cudaFree를 수행한다. */
typedef std::shared_ptr<void> BufferPtr;



/* createDma: 호스트 메모리를 할당하고 NVMe 컨트롤러용 DMA 매핑을 생성한다 */
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size);

/* createDma: CUDA 디바이스 메모리를 할당하고 NVMe 컨트롤러용 DMA 매핑을 생성한다 */
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size, int cudaDevice);

/* createDma: DIS 클러스터 환경에서 DMA 매핑을 생성한다 */
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size, uint32_t adapter, uint32_t id);

/* createDma: DIS 클러스터 + CUDA 디바이스 환경에서 DMA 매핑을 생성한다 */
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size, int cudaDevice, uint32_t adapter, uint32_t id);

/* createBuffer: 호스트 핀 메모리(cudaHostAlloc)를 할당한다 */
BufferPtr createBuffer(size_t size);

/* createBuffer: CUDA 디바이스 메모리를 할당하고 0으로 초기화한다 */
BufferPtr createBuffer(size_t size, int cudaDevice);

/* getDeviceMemory: CUDA 디바이스에 GPU 메모리를 할당하고 64KB 정렬된 포인터를 반환한다.
 * NVMe DMA 전송을 위해 64KB 정렬이 필요하다 (NVMe 페이지 크기 요구사항).
 * bufferPtr: 64KB 정렬된 GPU 메모리 포인터 (DMA용)
 * devicePtr: 64KB 정렬된 디바이스 포인터
 * origPtr: 실제 cudaMalloc이 반환한 원본 포인터 (해제 시 사용)
 */
static void getDeviceMemory(int device, void*& bufferPtr, void*& devicePtr, size_t size, void*& origPtr)
{
    bufferPtr = nullptr;
    devicePtr = nullptr;

    // 대상 CUDA 디바이스를 설정한다
    cudaError_t err = cudaSetDevice(device);
    if (err != cudaSuccess)
    {
        throw error(string("Failed to set CUDA device: ") + cudaGetErrorString(err));
    }
    // 64KB 정렬을 위해 추가 공간을 확보한다
    size += 64*1024;
    // GPU 메모리를 할당한다
    err = cudaMalloc(&bufferPtr, size);
    if (err != cudaSuccess)
    {
        throw error(string("Failed to allocate device memory: ") + cudaGetErrorString(err));
    }

    // 포인터 속성을 조회하여 디바이스 포인터를 얻는다
    cudaPointerAttributes attrs;
    err = cudaPointerGetAttributes(&attrs, bufferPtr);
    if (err != cudaSuccess)
    {
        cudaFree(bufferPtr);
        throw error(string("Failed to get pointer attributes: ") + cudaGetErrorString(err));
    }

    // 원본 포인터를 저장한다 (나중에 cudaFree에 사용)
    origPtr = bufferPtr;
    // 64KB 경계로 올림 정렬한다: (addr + 64K) & ~0xFFFF
    devicePtr = (void*) ((((uint64_t)attrs.devicePointer) + (64*1024)) & 0xffffffffff0000);
    bufferPtr = (void*) ((((uint64_t)bufferPtr) + (64*1024))  & 0xffffffffff0000);
}

/* getDeviceMemory2: GPU 메모리를 할당하고 128바이트 정렬된 포인터를 반환한다.
 * 캐시 라인 정렬을 위해 128바이트(GPU L2 캐시라인 크기)로 정렬한다.
 * 주로 page_cache 메타데이터 구조체 등에 사용된다.
 */
static void getDeviceMemory2(int device, void*& bufferPtr, size_t size, void*& origPtr)
{
    bufferPtr = nullptr;
    // 128바이트 정렬을 위한 추가 공간 확보
    size += 128;
    cudaError_t err = cudaSetDevice(device);
    if (err != cudaSuccess)
    {
        throw error(string("Failed to set CUDA device: ") + cudaGetErrorString(err));
    }
    // GPU 메모리를 할당한다
    err = cudaMalloc(&bufferPtr, size);
    if (err != cudaSuccess)
    {
        throw error(string("Failed to allocate device memory: ") + cudaGetErrorString(err));
    }

    // 할당된 메모리를 0으로 초기화한다 (atomic 변수 등의 초기값 보장)
    err = cudaMemset(bufferPtr, 0, size);
    if (err != cudaSuccess)
    {
        cudaFree(bufferPtr);
        throw error(string("Failed to clear device memory: ") + cudaGetErrorString(err));
    }

    // 원본 포인터를 저장하고 32바이트 경계로 정렬한다
    origPtr = bufferPtr;
    bufferPtr = (void*) ((((uint64_t)bufferPtr) + (128))  & 0xffffffffffffe0);
}

/* getDeviceMemory 오버로드: devicePtr이 필요 없는 경우의 간단한 버전 */
static void getDeviceMemory(int device, void*& bufferPtr, size_t size)
{
    void* notUsed = nullptr;
    getDeviceMemory(device, bufferPtr, notUsed, size, notUsed);
}




/* createDma (호스트 버전): posix_memalign으로 4KB 정렬 호스트 메모리를 할당하고
 * NVMe 컨트롤러용 DMA 매핑을 생성한다.
 * shared_ptr 커스텀 삭제자가 소멸 시 DMA 언매핑과 메모리 해제를 자동 수행한다.
 */
inline DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size)
{
    nvm_dma_t* dma = nullptr;
    void* buffer = nullptr;

    // 4KB(페이지) 정렬된 호스트 메모리를 할당한다
    int err  = posix_memalign(&buffer, 4096, size);

    if (err) {
        throw error(string("Failed to allocate host memory: ") + std::to_string(err));
    }
    // 할당된 호스트 메모리를 NVMe 컨트롤러의 DMA 주소 공간에 매핑한다
    int status = nvm_dma_map_host(&dma, ctrl, buffer, size);
    if (!nvm_ok(status))
    {
        free(buffer);
        throw error(string("Failed to map host memory: ") + nvm_strerror(status));
    }

    // shared_ptr에 커스텀 삭제자를 설정한다: DMA 언매핑 후 호스트 메모리를 해제한다
    return DmaPtr(dma, [buffer](nvm_dma_t* dma) {
        nvm_dma_unmap(dma);
        free(buffer);
    });
}



/* createDma (GPU 디바이스 버전): GPU 메모리를 할당하고 NVMe 컨트롤러가
 * PCIe를 통해 직접 접근할 수 있도록 DMA 매핑한다.
 * 이것이 BaM의 핵심: GPU 메모리를 NVMe가 DMA로 직접 읽고 쓸 수 있게 한다.
 */
inline DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size, int cudaDevice)
{
    // cudaDevice가 음수이면 호스트 DMA로 폴백한다
    if (cudaDevice < 0)
    {
        return createDma(ctrl, size);
    }

    nvm_dma_t* dma = nullptr;
    void* bufferPtr = nullptr;
    void* devicePtr = nullptr;
    void* origPtr = nullptr;

    // GPU 디바이스 메모리를 64KB 정렬하여 할당한다
    getDeviceMemory(cudaDevice, bufferPtr, devicePtr, size, origPtr);

    // GPU 메모리를 NVMe 컨트롤러의 DMA 주소 공간에 매핑한다
    int status = nvm_dma_map_device(&dma, ctrl, bufferPtr, size);
    if (!nvm_ok(status))
    {
        throw error(string("Failed to map device memory: ") + nvm_strerror(status));
    }
    // GPU 메모리를 0으로 초기화한다
    cudaError_t err = cudaMemset(bufferPtr, 0, size);
    if (err != cudaSuccess)
    {
        cudaFree(bufferPtr);
        throw error(string("Failed to clear device memory: ") + cudaGetErrorString(err));
    }
    // DMA 구조체에 정렬된 가상 주소를 설정한다
    dma->vaddr = bufferPtr;

    // shared_ptr 커스텀 삭제자: DMA 언매핑 후 원본 포인터로 GPU 메모리를 해제한다
    return DmaPtr(dma, [bufferPtr, origPtr](nvm_dma_t* dma) {
        nvm_dma_unmap(dma);
        cudaFree(origPtr);
    });
}



/* createBuffer (호스트 버전): cudaHostAlloc으로 페이지 잠금(pinned) 호스트 메모리를 할당한다.
 * 핀 메모리는 DMA 전송에 적합하며, 페이지 아웃되지 않는다.
 */
inline BufferPtr createBuffer(size_t size)
{
    void* buffer = nullptr;

    // CUDA 핀 메모리를 할당한다 (기본 플래그)
    cudaError_t err = cudaHostAlloc(&buffer, size, cudaHostAllocDefault);
    if (err != cudaSuccess)
    {
        throw error(string("Failed to allocate host memory: ") + cudaGetErrorString(err));
    }

    // shared_ptr에 cudaFreeHost 삭제자를 설정한다
    return BufferPtr(buffer, [](void* ptr) {

        cudaFreeHost(ptr);
    });
}



/* createBuffer (GPU 디바이스 버전): GPU 디바이스 메모리를 128바이트 정렬하여 할당하고
 * 0으로 초기화한다. 주로 page cache 메타데이터 버퍼로 사용된다.
 */
inline BufferPtr createBuffer(size_t size, int cudaDevice)
{
    // cudaDevice가 음수이면 호스트 버퍼로 폴백한다
    if (cudaDevice < 0)
    {
        return createBuffer(size);
    }

    void* bufferPtr = nullptr;
    void* origPtr = nullptr;

    // GPU 메모리를 128바이트 정렬하여 할당하고 0으로 초기화한다
    getDeviceMemory2(cudaDevice, bufferPtr, size, origPtr);

    // shared_ptr에 cudaFree 삭제자를 설정한다 (원본 포인터로 해제)
    return BufferPtr(bufferPtr, [origPtr](void* ptr) {
        __ignore(ptr);
        cudaFree(origPtr);
    });
}

#endif
