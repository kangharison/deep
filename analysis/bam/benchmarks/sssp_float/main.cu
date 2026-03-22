/* References:
 *
 *      Coalesce
 *          Hong, Sungpack, et al.
 *          "Accelerating CUDA graph algorithms at maximum warp."
 *          Acm Sigplan Notices 46.8 (2011): 267-276.
 *
 */

/*
 * =============================================================================
 * BaM SSSP_float 벤치마크 - 부동소수점 가중치 최단 경로
 * =============================================================================
 *
 * [알고리즘 개요]
 * sssp/main.cu와 동일한 Bellman-Ford 기반 SSSP이지만, 가중치 타입이 float이다.
 * CUDA에서 float에 대한 atomicMin이 기본 제공되지 않으므로,
 * AtomicMin() 함수를 atomicCAS 루프로 직접 구현한다.
 *
 * [sssp/main.cu와의 차이점]
 * - WeightT = float (sssp는 uint32_t)
 * - AtomicMin() 디바이스 함수: float 값을 int로 재해석하여 atomicCAS로 비교-교환
 * - 초기 비용값: 0xFF memset 대신 costList_h에 1000000000.0f를 채워서 cudaMemcpy
 * - UVM_DIRECT 모드에서 O_DIRECT 최적화가 없음 (단순 cudaMallocManaged + AccessedBy)
 *
 * [그래프 저장 형식 - CSR]
 * sssp/main.cu와 동일: .col(정점), .dst(간선), .val(부동소수점 가중치)
 *
 * [커널 변형 및 BaM I/O 패턴]
 * sssp/main.cu와 완전히 동일한 구조. atomicMin만 AtomicMin()으로 교체.
 * =============================================================================
 */

#include <cuda.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <getopt.h>
//#include "helper_cuda.h"
#include <algorithm>
#include <vector>
#include <numeric>
#include <iterator>
#include <math.h>
#include <chrono>
#include <ctime>
#include <ratio>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdexcept>

#include <nvm_ctrl.h>
#include <nvm_types.h>
#include <nvm_queue.h>
#include <nvm_util.h>
#include <nvm_admin.h>
#include <nvm_error.h>
#include <nvm_cmd.h>
#include <buffer.h>
#include "settings.h"
#include <ctrl.h>
#include <event.h>
#include <queue.h>
#include <nvm_parallel_queue.h>
#include <nvm_io.h>
#include <page_cache.h>      // page_cache_t, range_t, array_t, array_d_t 정의
#include <util.h>


using error = std::runtime_error;
using std::string;
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0"};

#define MYINFINITY 0xFFFFFFFF       // 무한대 비용 (미도달 정점)

#define WARP_SHIFT 5                // log2(WARP_SIZE)
#define WARP_SIZE 32                // CUDA 워프 크기

#define CHUNK_SHIFT 3               // log2(CHUNK_SIZE)
#define CHUNK_SIZE (1 << CHUNK_SHIFT) // COALESCE_CHUNK 모드에서 워프당 정점 수 (8)

#define BLOCK_NUM 1024ULL           // 2D 그리드 x 차원 블록 수

typedef uint64_t EdgeT;             // 간선 타입 (목적지 정점 ID)
typedef float WeightT;              // 가중치 타입 (부동소수점, sssp는 uint32_t)

typedef enum {
    BASELINE = 0,
    COALESCE = 1,
    COALESCE_CHUNK = 2,
    BASELINE_PC = 3,
    COALESCE_PC = 4,
    COALESCE_CHUNK_PC =5,
} impl_type;

typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    UVM_READONLY_NVLINK = 3,
    UVM_DIRECT_NVLINK = 4,
    DRAGON_MAP = 5,
    BAFS_DIRECT= 6,
} mem_type;


/* AtomicMin for float: CUDA에는 float용 atomicMin이 없으므로 CAS 루프로 구현.
 * float 값을 uint32_t로 재해석(bit cast)하여 atomicCAS로 비교-교환한다.
 * IEEE 754 부동소수점의 비트 순서 특성상, 양수 float의 정수 재해석 비교가 올바르게 동작한다.
 * 음수 가중치가 있으면 이 구현은 정확하지 않을 수 있다. */
__device__ void AtomicMin(float * const address, const float value)
{
	if (*address <= value)
		return;

	uint32_t * const address_as_i = (uint32_t*)address;
    uint32_t old = *address_as_i, assumed;

	do {
        assumed = old;
		if (__int_as_float(assumed) <= value)
			break;

        old = atomicCAS(address_as_i, assumed, __int_as_float(value));
    } while (assumed != old);
}



/* =====================================================================
 * SSSP_float CUDA 커널들
 * =====================================================================
 * sssp/main.cu와 로직 동일. 유일한 차이는 atomicMin 대신 AtomicMin(CAS 기반)을 사용한다는 점.
 * BASELINE: 정점당 1스레드, 순차 간선 접근
 * COALESCE: 정점당 1워프, stride 접근으로 메모리 coalescing
 * COALESCE_CHUNK: 워프당 8정점 묶음
 * *_PC 변형: de->seq_read(i), dw->seq_read(i)로 BaM page cache SSD 직접 I/O
 * =====================================================================
 */

__global__ void kernel_baseline(bool *label, const WeightT *costList, WeightT *newCostList, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, const WeightT *weightList) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    // const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if (tid < vertex_count && label[tid]) {
        uint64_t start = vertexList[tid];
        // const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        uint64_t end = vertexList[tid+1];

        WeightT cost = newCostList[tid];

        // for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
        for(uint64_t i = start; i < end; i += 1) {
            if (newCostList[tid] != cost)
                break;
            const EdgeT next = edgeList[i];
            const WeightT weight = weightList[i];
            if (newCostList[next] > cost + weight && i >= start)
                AtomicMin(&(newCostList[next]), cost + weight);
        }

        label[tid] = false;
    }
}
__global__ __launch_bounds__(1024,2)
void kernel_baseline_pc(array_d_t<uint64_t>* de,array_d_t<WeightT>* dw, bool *label, const WeightT *costList, WeightT *newCostList, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, const WeightT *weightList) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    // const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    // array_d_t<uint64_t> d_earray = *de;
    // array_d_t<WeightT> d_warray = *dw;
    if (tid < vertex_count && label[tid]) {
        uint64_t start = vertexList[tid];
        // const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        uint64_t end = vertexList[tid+1];

        WeightT cost = newCostList[tid];

        // for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
        for(uint64_t i = start; i < end; i += 1) {
            if (newCostList[tid] != cost)
                break;
            // const EdgeT next = edgeList[i];
            EdgeT next = de->seq_read(i);
            // const WeightT weight = weightList[i];
            WeightT weight = dw->seq_read(i);

            if (newCostList[next] > cost + weight && i >= start)
                AtomicMin(&(newCostList[next]), cost + weight);
        }

        label[tid] = false;
    }
}




__global__ void kernel_coalesce(bool *label, const WeightT *costList, WeightT *newCostList, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, const WeightT *weightList) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if (warpIdx < vertex_count && label[warpIdx]) {
        uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        uint64_t end = vertexList[warpIdx+1];

        WeightT cost = newCostList[warpIdx];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (newCostList[warpIdx] != cost)
                break;
            const EdgeT next = edgeList[i];
            const WeightT weight = weightList[i];
            if (newCostList[next] > cost + weight && i >= start)
                AtomicMin(&(newCostList[next]), cost + weight);
        }

        label[warpIdx] = false;
    }
}

__global__ __launch_bounds__(1024,2)
void kernel_coalesce_pc(array_d_t<uint64_t>* de,array_d_t<WeightT>* dw, bool *label, const WeightT *costList, WeightT *newCostList, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, const WeightT *weightList) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    // array_d_t<uint64_t> d_earray = *de;
    // array_d_t<WeightT> d_warray = *dw;
    if (warpIdx < vertex_count && label[warpIdx]) {
        uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        uint64_t end = vertexList[warpIdx+1];

        WeightT cost = newCostList[warpIdx];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (newCostList[warpIdx] != cost)
                break;
            // const EdgeT next = edgeList[i];
            EdgeT next = de->seq_read(i);
            // const WeightT weight = weightList[i];
            WeightT weight = dw->seq_read(i);
            if (newCostList[next] > cost + weight && i >= start)
                AtomicMin(&(newCostList[next]), cost + weight);
        }

        label[warpIdx] = false;
    }
}


__global__ void kernel_coalesce_chunk(bool *label, const WeightT *costList, WeightT *newCostList, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, const WeightT *weightList) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    const uint64_t chunkIdx = warpIdx * CHUNK_SIZE;
    uint64_t chunk_size = CHUNK_SIZE;

    if((chunkIdx + CHUNK_SIZE) > vertex_count) {
        if ( vertex_count > chunkIdx )
            chunk_size = vertex_count - chunkIdx;
        else
            return;
    }

    for(uint32_t i = chunkIdx; i < chunk_size + chunkIdx; i++) {
        if (label[i]) {
            uint64_t start = vertexList[i];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            uint64_t end = vertexList[i+1];

            WeightT cost = newCostList[i];

            for(uint64_t j = shift_start + laneIdx; j < end; j += WARP_SIZE) {
                if (newCostList[i] != cost)
                    break;
                const EdgeT next = edgeList[j];
                const WeightT weight = weightList[j];
                if (newCostList[next] > cost + weight && j >= start)
                    AtomicMin(&(newCostList[next]), cost + weight);
            }

            label[i] = false;
        }
    }
}

__global__ __launch_bounds__(1024,2)
void kernel_coalesce_chunk_pc(array_d_t<uint64_t>* de,array_d_t<WeightT>* dw, bool *label, const WeightT *costList, WeightT *newCostList, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, const WeightT *weightList) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    const uint64_t chunkIdx = warpIdx * CHUNK_SIZE;
    uint64_t chunk_size = CHUNK_SIZE;
    // array_d_t<uint64_t> d_earray = *de;
    // array_d_t<WeightT> d_warray = *dw;
    if((chunkIdx + CHUNK_SIZE) > vertex_count) {
        if ( vertex_count > chunkIdx )
            chunk_size = vertex_count - chunkIdx;
        else
            return;
    }

    for(uint32_t i = chunkIdx; i < chunk_size + chunkIdx; i++) {
        if (label[i]) {
            uint64_t start = vertexList[i];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            uint64_t end = vertexList[i+1];

            WeightT cost = newCostList[i];

            for(uint64_t j = shift_start + laneIdx; j < end; j += WARP_SIZE) {
                if (newCostList[i] != cost)
                    break;
                // const EdgeT next = edgeList[j];
                EdgeT next = de->seq_read(j);
                // const WeightT weight = weightList[j];
                WeightT weight = dw->seq_read(j);
                if (newCostList[next] > cost + weight && j >= start)
                    AtomicMin(&(newCostList[next]), cost + weight);
            }

            label[i] = false;
        }
    }
}



/* update 커널: 완화 결과를 확정. newCostList < costList인 정점을 갱신하고 재활성화. */
__global__ void update(bool *label, WeightT *costList, WeightT *newCostList, const uint32_t vertex_count, bool *changed) {
	uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;

    if (tid < vertex_count) {
        if (newCostList[tid] < costList[tid]) {
            costList[tid] = newCostList[tid];
            label[tid] = true;
            *changed = true;
        }
    }
}

/* GPU 메모리 압박 시뮬레이션용 헬퍼 커널 */
__global__ void throttle_memory(uint32_t *pad) {
    pad[1] = pad[0];
}

/* =============================================================================
 * main() 함수: SSSP_float 벤치마크 진입점
 * =============================================================================
 * sssp/main.cu와 동일한 흐름. 차이점:
 * - 초기 비용: 0xFF memset 대신 costList_h에 1000000000.0f를 채워 cudaMemcpy로 전송
 * - UVM_DIRECT: O_DIRECT 없이 단순 cudaMallocManaged + AccessedBy
 * =============================================================================
 */
int main(int argc, char *argv[]) {
    using namespace std::chrono; 

    Settings settings; 
    try
    {
        settings.parseArguments(argc, argv);
    }
    catch (const string& e)
    {
        fprintf(stderr, "%s\n", e.c_str());
        fprintf(stderr, "%s\n", Settings::usageString(argv[0]).c_str());
        return 1;
    }

    cudaDeviceProp properties;
    if (cudaGetDeviceProperties(&properties, settings.cudaDevice) != cudaSuccess)
    {
        fprintf(stderr, "Failed to get CUDA device properties\n");
        return 1;
    }
    
    std::ifstream file, file2;
    std::string vertex_file, edge_file, weight_file;
    std::string filename;

    bool changed_h, *changed_d, *label_d; // changed_h: 호스트 수렴 플래그, label_d: 활성 정점 배열
    int total_run = 1; // 총 실행 횟수
    int num_run = 0; // 유효 실행 카운터
    impl_type type; // 커널 구현 타입
    mem_type mem; // 메모리 할당 방식
    uint32_t *pad; // GPU 메모리 throttle용 더미 배열
    uint32_t one, iter; // one: label 초기화용, iter: SSSP 반복 카운터
    WeightT offset = 0; // 가중치 오프셋 (현재 0)
    WeightT zero; // 시작 정점 비용 초기값 (0.0f)
    WeightT *costList_d, *newCostList_d, *costList_h, *weightList_h, *weightList_d; // costList_h: float 무한대 초기화용 호스트 배열
    uint64_t *vertexList_h, *vertexList_d; // CSR 정점 오프셋 (호스트/디바이스)
    EdgeT *edgeList_h, *edgeList_d; // CSR 간선 리스트 (호스트/디바이스)
    uint64_t vertex_count, edge_count, weight_count, vertex_size, edge_size, weight_size; // 그래프 크기 정보
    uint64_t typeT, src; // typeT: 파일 헤더 타입, src: 시작 정점
    uint64_t numblocks_kernel, numblocks_update, numthreads; // CUDA 그리드 차원 변수
    size_t freebyte, totalbyte; // GPU 메모리 조회용
    // EdgeT *edgeList_dtmp;
    // WeightT *weightList_dtmp;

    float milliseconds; // 커널 실행 시간
    double avg_milliseconds; // 평균 실행 시간 누적

    uint64_t pc_page_size; // BaM page cache 페이지 크기
    uint64_t pc_pages; // BaM page cache 총 페이지 수


    try{
        // 설정값으로부터 실행 파라미터 준비
        filename = std::string(settings.input); // 그래프 파일 기본 경로

        if(settings.src == 0) {
                total_run = settings.repeat; // 랜덤 시작점으로 repeat회 반복
                src = 0;
        }
        else {
                total_run = 1; // 특정 시작점이면 1회만 실행
                src = settings.src;
        }

        type = (impl_type) settings.type; // 커널 구현 타입
        mem = (mem_type) settings.memalloc; // 메모리 할당 방식

        pc_page_size = settings.pageSize; // BaM page cache 페이지 크기
        pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size); // 최대 캐시 크기에서 페이지 수 계산

        numthreads = settings.numThreads; // CUDA 블록당 스레드 수
        
        cuda_err_chk(cudaSetDevice(settings.cudaDevice));
    
        cudaEvent_t start, end;
        cuda_err_chk(cudaEventCreate(&start));
        cuda_err_chk(cudaEventCreate(&end));

        /* CSR 그래프 파일: .col=정점 오프셋, .dst=간선, .val=부동소수점 가중치 */
        vertex_file = filename + ".col";
        edge_file = filename + ".dst";
        weight_file = filename + ".val";

        std::cout << filename << std::endl;
        fprintf(stderr, "File %s\n", filename.c_str());
        // Read files
        // Start reading vertex list
        file.open(vertex_file.c_str(), std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            fprintf(stderr, "Vertex file open failed\n");
            exit(1);
        };

        file.read((char*)(&vertex_count), 8);
        file.read((char*)(&typeT), 8);

        vertex_count--;

        printf("Vertex: %llu, ", vertex_count);
        vertex_size = (vertex_count+1) * sizeof(uint64_t);

        vertexList_h = (uint64_t*)malloc(vertex_size);

        file.read((char*)vertexList_h, vertex_size);
        file.close();

        // Start reading edge list
        file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            fprintf(stderr, "Edge file open failed\n");
            exit(1);
        };

        file.read((char*)(&edge_count), 8);
        file.read((char*)(&typeT), 8);

        printf("Edge: %llu, ", edge_count);
        fflush(stdout);
        edge_size = edge_count * sizeof(EdgeT);
        edge_size = edge_size + (4096 - (edge_size & 0xFFFULL));

        edgeList_h = NULL;

        // Start reading edge weight list
        file2.open(weight_file.c_str(), std::ios::in | std::ios::binary);
        if (!file2.is_open()) {
            fprintf(stderr, "Edge file open failed\n");
            exit(1);
        };

        file2.read((char*)(&weight_count), 8);
        file2.read((char*)(&typeT), 8);

        printf("Weight: %llu\n", weight_count);
        fflush(stdout);
        weight_size = weight_count * sizeof(WeightT);
        weight_size = weight_size + (4096 - (weight_size & 0xFFFULL));

        weightList_h = NULL;

        /* 메모리 할당 방식별 edgeList/weightList 적재 */
        switch (mem) {
            case GPUMEM:
                /* GPUMEM: 호스트에서 파일 읽기 → cudaMemcpy로 GPU에 전부 적재 */
                edgeList_h = (EdgeT*)malloc(edge_size);
                weightList_h = (WeightT*)malloc(weight_size);
                file.read((char*)edgeList_h, edge_size);
                file2.read((char*)weightList_h, weight_size);
                cuda_err_chk(cudaMalloc((void**)&edgeList_d, edge_size));
                cuda_err_chk(cudaMalloc((void**)&weightList_d, weight_size));

                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_h[i] += offset; // 가중치 오프셋 적용 (현재 0)

                break;
            case UVM_READONLY:
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size));
                cuda_err_chk(cudaMallocManaged((void**)&weightList_d, weight_size));
                file.read((char*)edgeList_d, edge_size);
                file2.read((char*)weightList_d, weight_size);

                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_d[i] += offset;

                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(weightList_d, weight_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));

                cuda_err_chk(cudaMemGetInfo(&freebyte, &totalbyte));
                if (totalbyte < 16*1024*1024*1024ULL)
                    printf("total memory sizeo of current GPU is %llu byte, no need to throttle\n", totalbyte);
                else {
                    printf("total memory sizeo of current GPU is %llu byte, throttling %llu byte.\n", totalbyte, totalbyte - 16*1024*1024*1024ULL);
                    cuda_err_chk(cudaMalloc((void**)&pad, totalbyte - 16*1024*1024*1024ULL));
                    throttle_memory<<<1,1>>>(pad);
                }
                break;
            case UVM_DIRECT:
            {
                /* UVM_DIRECT: cudaMallocManaged + AccessedBy 힌트.
                 * sssp의 O_DIRECT 최적화와 달리 단순 fread 방식을 사용한다. */
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size)); // UVM 간선 배열
                cuda_err_chk(cudaMallocManaged((void**)&weightList_d, weight_size)); // UVM 가중치 배열
                file.read((char*)edgeList_d, edge_size); // UVM 주소에 직접 읽기
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();
                file2.read((char*)weightList_d, weight_size); // 가중치 파일 읽기 시간 측정
                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "weight file read time: "<< time_span.count() <<std::endl;

                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_d[i] += offset;

                /* AccessedBy: 호스트 메모리에 데이터 유지, GPU가 PCIe로 원격 접근 */
                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(weightList_d, weight_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                break;
            }
            case BAFS_DIRECT:
                cuda_err_chk(cudaMemGetInfo(&freebyte, &totalbyte));
                if (totalbyte < 16*1024*1024*1024ULL)
                    printf("total memory sizeo of current GPU is %llu byte, no need to throttle\n", totalbyte);
                else {
                    printf("total memory sizeo of current GPU is %llu byte, throttling %llu byte.\n", totalbyte, totalbyte - 16*1024*1024*1024ULL);
                    cuda_err_chk(cudaMalloc((void**)&pad, totalbyte - 16*1024*1024*1024ULL));
                    throttle_memory<<<1,1>>>(pad);
                }
                break;

        }

        file.close();
        file2.close();

        /* float용 무한대 초기화: 0xFF memset으로는 NaN이 되므로,
         * 호스트에서 1e9f로 채운 배열을 GPU로 복사하는 방식을 사용한다. */
        /* float용 무한대 초기화: uint32_t SSSP에서는 0xFF memset으로 MYINFINITY(0xFFFFFFFF)를 설정할 수 있지만,
         * float의 경우 0xFF memset은 NaN이 되므로, 호스트에서 큰 값(1e9)으로 채운 배열을 cudaMemcpy로 전송한다. */
        costList_h = (WeightT*)malloc(weight_size); // 호스트 비용 초기화 배열

        for (uint64_t i = 0; i < weight_count; i++) {
            costList_h[i] = 1000000000.0f; // 1e9를 무한대 대용으로 사용
        }

        // GPU 메모리 할당: SSSP 알고리즘에 필요한 배열들
        cuda_err_chk(cudaMalloc((void**)&vertexList_d, vertex_size)); // CSR 정점 오프셋 (GPU)
        cuda_err_chk(cudaMalloc((void**)&label_d, vertex_count * sizeof(bool))); // 활성 정점 플래그
        cuda_err_chk(cudaMalloc((void**)&changed_d, sizeof(bool))); // 수렴 판정 플래그
        cuda_err_chk(cudaMalloc((void**)&costList_d, vertex_count * sizeof(WeightT))); // 확정 비용
        cuda_err_chk(cudaMalloc((void**)&newCostList_d, vertex_count * sizeof(WeightT))); // 후보 비용

        printf("Allocation finished\n");
        fflush(stdout);

        // Initialize values
        cuda_err_chk(cudaMemcpy(vertexList_d, vertexList_h, vertex_size, cudaMemcpyHostToDevice));

        if (mem == GPUMEM) {
            cuda_err_chk(cudaMemcpy(edgeList_d, edgeList_h, edge_size, cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(weightList_d, weightList_h, weight_size, cudaMemcpyHostToDevice));
        }


        switch (type) {
            case BASELINE:
            case BASELINE_PC:
                numblocks_kernel = ((vertex_count+numthreads)/numthreads);
                break;
            case COALESCE:
            case COALESCE_PC:
                numblocks_kernel = ((vertex_count * WARP_SIZE + numthreads) / numthreads);
                break;
            case COALESCE_CHUNK:
            case COALESCE_CHUNK_PC:
                numblocks_kernel = ((vertex_count * (WARP_SIZE / CHUNK_SIZE) + numthreads) / numthreads);
                break;
            default:
                fprintf(stderr, "Invalid type\n");
                exit(1);
                break;
        }

        numblocks_update = ((vertex_count + numthreads) / numthreads);

        dim3 blockDim_kernel(BLOCK_NUM, (numblocks_kernel+BLOCK_NUM)/BLOCK_NUM);
        dim3 blockDim_update(BLOCK_NUM, (numblocks_update+BLOCK_NUM)/BLOCK_NUM);

        avg_milliseconds = 0.0f;

        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
                printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
        }

        std::vector<Controller*> ctrls(settings.n_ctrls);
        if(mem == BAFS_DIRECT){
            cuda_err_chk(cudaSetDevice(settings.cudaDevice));
            for (size_t i = 0 ; i < settings.n_ctrls; i++)
                ctrls[i] = new Controller(ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);
            printf("Controllers Created\n");
        }

        printf("Initialization done\n");
        fflush(stdout);

        /* BaM page cache 객체: edgeList(h_erange/h_earray)와 weightList(h_wrange/h_warray)
         * 두 개의 SSD 영역을 하나의 page_cache_t에서 관리한다. */
        page_cache_t* h_pc;

        range_t<uint64_t>* h_erange;
        range_t<WeightT>* h_wrange;
        std::vector<range_t<uint64_t>*> vec_erange(1);
        std::vector<range_t<WeightT>*> vec_wrange(1);
        array_t<uint64_t>* h_earray;
        array_t<WeightT>* h_warray;

        uint64_t n_epages = ceil(((float)edge_size)/pc_page_size);   // edgeList SSD 페이지 수
        uint64_t n_wpages = ceil(((float)weight_size)/pc_page_size); // weightList SSD 페이지 수


        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
            h_pc =new page_cache_t(pc_page_size, pc_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
            // h_erange = new range_t<uint64_t>((int)0 , (uint64_t)edge_count, (int) 0, (uint64_t)n_epages, (int)0, (uint64_t)pc_page_size, h_pc, settings, (uint8_t*)edgeList_d);
            // h_wrange = new range_t<WeightT>((int)0 ,     (uint64_t)weight_count, (int) 0, (uint64_t)n_wpages, (int)0,      (uint64_t)pc_page_size, h_pc, settings, (uint8_t*)weightList_d);
            h_erange = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)edge_count, (uint64_t) (ceil(settings.ofileoffset*1.0/pc_page_size)),(uint64_t)n_epages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice); 
            h_wrange = new range_t<WeightT>((uint64_t)0 ,(uint64_t)weight_count,(uint64_t) (ceil(settings.wfileoffset*1.0/pc_page_size)),(uint64_t)n_wpages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice); 
            
            vec_erange[0] = h_erange;
            vec_wrange[0] = h_wrange;
            h_earray = new array_t<uint64_t>(edge_count, settings.ofileoffset, vec_erange, settings.cudaDevice);
            h_warray = new array_t<WeightT>(weight_count,settings.wfileoffset, vec_wrange, settings.cudaDevice);

            printf("Page cache initialized\n");
            fflush(stdout);
        }



        /* SSSP 반복 실행 루프 */
        for (int i = 0; i < total_run; i++) {
            zero = 0.0f; // 시작 정점 비용 = 0.0
            one = 1;
            /* float 비용 초기화: memset 대신 costList_h(1e9로 채운 배열)을 cudaMemcpy로 전송 */
            cuda_err_chk(cudaMemcpy(costList_d, costList_h, vertex_count * sizeof(WeightT), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(newCostList_d, costList_h, vertex_count * sizeof(WeightT), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemset(label_d, 0x0, vertex_count * sizeof(bool)));
            cuda_err_chk(cudaMemcpy(&label_d[src], &one, sizeof(bool), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(&costList_d[src], &zero, sizeof(WeightT), cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(&newCostList_d[src], &zero, sizeof(WeightT), cudaMemcpyHostToDevice));

            iter = 0;

            cuda_err_chk(cudaEventRecord(start, 0));

            // Run SSSP
            do {
                changed_h = false;
                cuda_err_chk(cudaMemcpy(changed_d, &changed_h, sizeof(bool), cudaMemcpyHostToDevice));
                auto start = std::chrono::system_clock::now();
                switch (type) {
                    case BASELINE:
                        kernel_baseline<<<blockDim_kernel, numthreads>>>(label_d, costList_d, newCostList_d, vertex_count, vertexList_d, edgeList_d, weightList_d);
                        break;
                    case COALESCE:
                        kernel_coalesce<<<blockDim_kernel, numthreads>>>(label_d, costList_d, newCostList_d, vertex_count, vertexList_d, edgeList_d, weightList_d);
                        break;
                    case COALESCE_CHUNK:
                        kernel_coalesce_chunk<<<blockDim_kernel, numthreads>>>(label_d, costList_d, newCostList_d, vertex_count, vertexList_d, edgeList_d, weightList_d);
                        break;
                    case BASELINE_PC:
                        kernel_baseline_pc<<<blockDim_kernel, numthreads>>>(h_earray->d_array_ptr,h_warray->d_array_ptr,label_d, costList_d, newCostList_d, vertex_count, vertexList_d, edgeList_d, weightList_d);
                        break;
                    case COALESCE_PC:
                        kernel_coalesce_pc<<<blockDim_kernel, numthreads>>>(h_earray->d_array_ptr,h_warray->d_array_ptr, label_d, costList_d, newCostList_d, vertex_count, vertexList_d, edgeList_d, weightList_d);
                        break;
                    case COALESCE_CHUNK_PC:
                        kernel_coalesce_chunk_pc<<<blockDim_kernel, numthreads>>>(h_earray->d_array_ptr,h_warray->d_array_ptr, label_d, costList_d, newCostList_d, vertex_count, vertexList_d, edgeList_d, weightList_d);
                        break;
                    default:
                        fprintf(stderr, "Invalid type\n");
                        exit(1);
                        break;
                }

                update<<<blockDim_update, numthreads>>>(label_d, costList_d, newCostList_d, vertex_count, changed_d);

                iter++;

                cuda_err_chk(cudaMemcpy(&changed_h, changed_d, sizeof(bool), cudaMemcpyDeviceToHost));
                auto end = std::chrono::system_clock::now();
/*
                if(mem == BAFS_DIRECT) {
                    h_earray->print_reset_stats();
                    h_warray->print_reset_stats();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    std::cout << std::dec << "Time: " << elapsed.count() << " ms" << std::endl;
                }*/
                //break;
            } while(changed_h);

            cuda_err_chk(cudaEventRecord(end, 0));
            cuda_err_chk(cudaEventSynchronize(end));
            cuda_err_chk(cudaEventElapsedTime(&milliseconds, start, end));
            if(iter > 1){
                  printf("run %*d: ", 3, i);
                  printf("src %*u, ", 10, src);
                  printf("iteration %*u, ", 3, iter);
                  printf("time %*f ms\n", 12, milliseconds);
                  if(mem == BAFS_DIRECT) {
                     h_earray->print_reset_stats();
                     h_warray->print_reset_stats();
                  }
                  fflush(stdout);

                  avg_milliseconds += (double)milliseconds;
                  num_run++; 
            }
            src += vertex_count / total_run;

            /*if (i < total_run - 1) {
                EdgeT *edgeList_temp;
                WeightT *weightList_temp;

                switch (mem) {
                    case UVM_READONLY:
                        cuda_err_chk(cudaMallocManaged((void**)&edgeList_temp, edge_size));
                        cuda_err_chk(cudaMallocManaged((void**)&weightList_temp, weight_size));
                        memcpy(edgeList_temp, edgeList_d, edge_size);
                        memcpy(weightList_temp, weightList_d, weight_size);
                        cuda_err_chk(cudaFree(edgeList_d));
                        cuda_err_chk(cudaFree(weightList_d));
                        edgeList_d = edgeList_temp;
                        weightList_d = weightList_temp;
                        cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, 0));
                        cuda_err_chk(cudaMemAdvise(weightList_d, weight_size, cudaMemAdviseSetReadMostly, 0));
                        break;
                    case UVM_READONLY_NVLINK:
                        cuda_err_chk(cudaSetDevice(2));
                        cuda_err_chk(cudaMallocManaged((void**)&edgeList_temp, edge_size));
                        cuda_err_chk(cudaMallocManaged((void**)&weightList_temp, weight_size));
                        memcpy(edgeList_temp, edgeList_d, edge_size);
                        memcpy(weightList_temp, weightList_d, weight_size);
                        cuda_err_chk(cudaFree(edgeList_d));
                        cuda_err_chk(cudaFree(weightList_d));
                        edgeList_d = edgeList_temp;
                        weightList_d = weightList_temp;
                        cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, 0));
                        cuda_err_chk(cudaMemAdvise(weightList_d, weight_size, cudaMemAdviseSetReadMostly, 0));
                        cuda_err_chk(cudaMemPrefetchAsync(edgeList_d, edge_size, 2, 0));
                        cuda_err_chk(cudaMemPrefetchAsync(weightList_d, weight_size, 2, 0));
                        cuda_err_chk(cudaDeviceSynchronize());
                        cuda_err_chk(cudaSetDevice(0));
                        break;
                    default:
                        break;
                }
            }*/
        }

        //printf("Average run time %f ms\n", avg_milliseconds / num_run);
        printf("\nSSSP_F Graph:%s \t Impl: %d \t SSD: %d \t PageSize: %d \t AvgTime %f ms\n", filename.c_str(), type, settings.n_ctrls, settings.pageSize, avg_milliseconds / num_run);




        free(vertexList_h);
        if (edgeList_h)
            free(edgeList_h);
        if (weightList_h)
            free(weightList_h);
        if (costList_h)
            free(costList_h);

        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
                delete h_pc;
                delete h_erange;
                delete h_earray;
                //    delete h_wpc;
                delete h_wrange;
                delete h_warray;
        }

        cuda_err_chk(cudaFree(vertexList_d));
        cuda_err_chk(cudaFree(costList_d));
        cuda_err_chk(cudaFree(newCostList_d));
        cuda_err_chk(cudaFree(label_d));
        cuda_err_chk(cudaFree(changed_d));
        
        if(mem!=BAFS_DIRECT){
            cuda_err_chk(cudaFree(weightList_d));
            cuda_err_chk(cudaFree(edgeList_d));
        }

        for (size_t i = 0 ; i < settings.n_ctrls; i++)
             delete ctrls[i];
    }
    catch (const error& e){
        fprintf(stderr, "Unexpected error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
