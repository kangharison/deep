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
 * BaM SSSP (Single Source Shortest Path) 벤치마크 - 정수 가중치 최단 경로
 * =============================================================================
 *
 * [알고리즘 개요]
 * Bellman-Ford 기반의 레벨 동기 SSSP를 GPU에서 수행한다.
 * 시작 정점(src)의 비용을 0으로 설정하고, 반복적으로 모든 활성 정점의 이웃에 대해
 * 완화(relaxation)를 수행한다: newCostList[next] = min(newCostList[next], cost + weight).
 * 매 반복 후 update 커널이 costList와 newCostList를 비교하여 갱신된 정점을 활성 표시한다.
 * 더 이상 갱신이 없을 때까지(changed == false) 반복한다.
 *
 * [BFS와의 차이점]
 * - BFS는 레이블(레벨 번호)만 사용하지만, SSSP는 costList(확정 비용), newCostList(후보 비용),
 *   weightList(간선 가중치) 세 배열이 필요하다.
 * - BFS는 한 번 방문하면 끝이지만, SSSP는 더 짧은 경로를 발견하면 재방문할 수 있다.
 * - atomicMin으로 경쟁적 완화를 처리한다.
 * - 커널 실행 후 별도의 update 커널이 비용 갱신 및 활성 정점 표시를 수행한다.
 *
 * [그래프 저장 형식 - CSR (Compressed Sparse Row)]
 * - vertexList[]: offsets 배열. vertexList[v]~vertexList[v+1]이 정점 v의 간선 범위.
 * - edgeList[]: 각 간선의 목적지 정점 ID (uint64_t).
 * - weightList[]: 각 간선의 가중치 (uint32_t). edgeList와 1:1 대응.
 * - 파일 형식: .col(정점 오프셋), .dst(간선), .val(가중치)
 *
 * [CUDA 커널 변형] (BFS와 동일한 구조)
 * - BASELINE: 정점당 1스레드
 * - COALESCE: 정점당 1워프 (메모리 coalescing)
 * - COALESCE_CHUNK: 워프당 CHUNK_SIZE(8)개 정점
 * - *_PC: BaM page cache로 edgeList/weightList SSD 직접 접근 (da->seq_read(i))
 *
 * [BaM page_cache I/O 패턴]
 * SSSP는 edgeList와 weightList 두 개의 SSD 배열을 사용하므로,
 * page_cache_t 하나에 range_t<uint64_t>(edge)와 range_t<WeightT>(weight) 두 개를 등록한다.
 * 각각 SSD의 서로 다른 오프셋 영역(ofileoffset, wfileoffset)에 매핑된다.
 *
 * [호스트 초기화 흐름]
 * 1. Settings 파싱 -> 그래프 파일(.col, .dst, .val) 읽기
 * 2. 메모리 모드별 edgeList/weightList 준비
 * 3. BAFS_DIRECT: Controller -> page_cache_t -> range_t(edge) + range_t(weight) -> array_t
 * 4. SSSP 반복: do { kernel_* -> update } while(changed)
 * 5. 결과 출력 및 자원 해제
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
/* NVMe 컨트롤러 디바이스 경로 배열. BaM 커널 모듈이 /dev/libnvm* 디바이스를 노출한다. */
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0"};

#define MYINFINITY 0xFFFFFFFF       // SSSP에서 무한대 비용 (미도달 정점)

#define WARP_SHIFT 5                // log2(WARP_SIZE)
#define WARP_SIZE 32                // CUDA 워프 크기

#define CHUNK_SHIFT 3               // log2(CHUNK_SIZE)
#define CHUNK_SIZE (1 << CHUNK_SHIFT) // COALESCE_CHUNK 모드에서 워프당 정점 수 (8)

#define BLOCK_NUM 1024ULL           // 2D 그리드 x 차원 블록 수

typedef uint64_t EdgeT;             // 간선 타입 (목적지 정점 ID, 64비트)
typedef uint32_t WeightT;           // 가중치 타입 (정수, 32비트)

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

/* =====================================================================
 * SSSP CUDA 커널들
 * =====================================================================
 *
 * [공통 완화(relaxation) 로직]
 * 활성 정점(label[v]==true)에 대해, 인접 간선의 목적지 next와 가중치 weight를 읽어
 * newCostList[next] > cost + weight이면 atomicMin으로 갱신한다.
 * 처리 후 label[v] = false로 비활성화한다.
 *
 * [BASELINE vs COALESCE vs COALESCE_CHUNK]
 * BFS와 동일한 스레드-정점 매핑 전략을 사용한다.
 * - BASELINE: 스레드당 1정점, 간선 순차 접근
 * - COALESCE: 워프당 1정점, shift_start로 128바이트 정렬 후 stride 접근 (coalescing)
 * - COALESCE_CHUNK: 워프당 8정점 묶음 처리
 *
 * [*_PC 변형: BaM page cache SSD 직접 I/O]
 * array_d_t<uint64_t>* de (edge)와 array_d_t<WeightT>* dw (weight) 두 개의 SSD 배열을 사용.
 * de->seq_read(i)로 edgeList[i], dw->seq_read(i)로 weightList[i]에 접근하면
 * BaM이 필요한 SSD 페이지를 GPU page cache로 fetch한다.
 *
 * [수렴 조건]
 * 메인 커널 후 update 커널이 newCostList[v] < costList[v]인 정점을 찾아
 * costList를 갱신하고 label을 true로, changed를 true로 설정한다.
 * changed가 false이면 모든 정점이 최단 비용에 수렴한 것이다.
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
                atomicMin(&(newCostList[next]), cost + weight);
        }

        label[tid] = false;
    }
}
__global__ __launch_bounds__(128,16)
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
                atomicMin(&(newCostList[next]), cost + weight);
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
                atomicMin(&(newCostList[next]), cost + weight);
        }

        label[warpIdx] = false;
    }
}

__global__ __launch_bounds__(128,16)
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
                atomicMin(&(newCostList[next]), cost + weight);
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
                    atomicMin(&(newCostList[next]), cost + weight);
            }

            label[i] = false;
        }
    }
}

__global__ __launch_bounds__(128,16)
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
                    atomicMin(&(newCostList[next]), cost + weight);
            }

            label[i] = false;
        }
    }
}



/* update 커널: SSSP 완화 결과를 확정하는 별도 커널.
 * newCostList[v] < costList[v]인 정점을 찾아 costList를 갱신하고,
 * 해당 정점을 다시 활성(label=true)으로 표시하며, changed=true로 설정한다.
 * 이 커널이 끝난 후 호스트에서 changed를 읽어 수렴 여부를 판단한다. */
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

__global__ void throttle_memory(uint32_t *pad) {
    pad[1] = pad[0];
}

/* =============================================================================
 * main() 함수: SSSP 벤치마크 진입점
 * =============================================================================
 * 전체 흐름:
 * (1) 커맨드라인 파싱 -> Settings
 * (2) 그래프 파일 읽기: .col(정점), .dst(간선), .val(가중치)
 * (3) 메모리 모드별 edgeList/weightList 준비 (GPUMEM/UVM/BAFS_DIRECT)
 * (4) BAFS_DIRECT: Controller -> page_cache_t -> range_t(edge)+range_t(weight) -> array_t
 * (5) SSSP 반복: do { kernel_* -> update } while(changed)
 *     - costList/newCostList를 0xFF(MYINFINITY)로 초기화, src만 0
 *     - label[src] = true로 시작
 * (6) 결과 출력 및 자원 해제
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

    cudaDeviceProp properties; // GPU 디바이스 속성 구조체
    if (cudaGetDeviceProperties(&properties, settings.cudaDevice) != cudaSuccess)
    {
        fprintf(stderr, "Failed to get CUDA device properties\n");
        return 1;
    }

    std::ifstream file, file2; // file: 정점/간선 파일, file2: 가중치 파일
    std::string vertex_file, edge_file, weight_file; // CSR 파일 경로들
    std::string filename; // 입력 파일 기본 경로 (.bel 확장자 없이)

    bool changed_h, *changed_d, *label_d; // changed_h: 호스트 수렴 플래그, changed_d: 디바이스 수렴 플래그, label_d: 활성 정점 배열
    int total_run = 1; // 총 실행 횟수 (src=0이면 repeat만큼 반복)
    int num_run = 0; // 유효 실행 카운터 (iter>1인 경우만)
    impl_type type; // 커널 구현 타입 (BASELINE/COALESCE/COALESCE_CHUNK 등)
    mem_type mem; // 메모리 할당 방식 (GPUMEM/UVM_READONLY/UVM_DIRECT/BAFS_DIRECT)
    uint32_t *pad; // GPU 메모리 throttle용 더미 배열
    uint32_t one, iter; // one: label 초기화용, iter: SSSP 반복 카운터
    WeightT offset = 0; // 가중치에 더하는 오프셋 (현재 0)
    WeightT zero; // 시작 정점 비용 초기값 (0)
    WeightT *costList_d, *newCostList_d, *weightList_h, *weightList_d; // costList_d: 확정 비용(GPU), newCostList_d: 후보 비용(GPU), weightList_h/d: 간선 가중치
    uint64_t *vertexList_h, *vertexList_d; // CSR 정점 오프셋 배열 (호스트/디바이스)
    EdgeT *edgeList_h, *edgeList_d; // CSR 간선 리스트 (호스트/디바이스)
    uint64_t vertex_count, edge_count, weight_count, vertex_size, edge_size, weight_size; // 그래프 크기 정보
    uint64_t typeT, src; // typeT: 파일 헤더의 타입 필드, src: SSSP 시작 정점
    uint64_t numblocks_kernel, numblocks_update, numthreads; // CUDA 그리드 차원 변수들
    size_t freebyte, totalbyte; // GPU 메모리 사용량 조회용
    // EdgeT *edgeList_dtmp;
    // WeightT *weightList_dtmp;

    float milliseconds; // 커널 실행 시간 (CUDA 이벤트)
    double avg_milliseconds; // 평균 실행 시간 누적

    uint64_t pc_page_size; // BaM page cache 페이지 크기 (바이트)
    uint64_t pc_pages; // BaM page cache 총 페이지 수


    try{
        // 설정값으로부터 실행 파라미터 준비
        filename = std::string(settings.input); // 그래프 파일 기본 경로

        if(settings.src == 0) {
                total_run = settings.repeat; // src=0이면 랜덤 시작점으로 repeat회 반복
                src = 0;
        }
        else {
                total_run = 1; // 특정 시작점이 지정되면 1회만 실행
                src = settings.src;
        }

        type = (impl_type) settings.type; // 커널 구현 타입 선택
        mem = (mem_type) settings.memalloc; // 메모리 할당 방식 선택

        pc_page_size = settings.pageSize; // BaM page cache 페이지 크기
        pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size); // 최대 캐시 크기에서 페이지 수 계산

        numthreads = settings.numThreads; // CUDA 블록당 스레드 수

        cuda_err_chk(cudaSetDevice(settings.cudaDevice)); // GPU 디바이스 선택

        cudaEvent_t start, end; // CUDA 이벤트 기반 타이밍 측정용
        cuda_err_chk(cudaEventCreate(&start));
        cuda_err_chk(cudaEventCreate(&end));

        /* CSR 그래프 파일: .col=정점 오프셋, .dst=간선 리스트, .val=간선 가중치 */
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

        file.read((char*)(&vertex_count), 8); // 파일 헤더: 정점 수 읽기
        file.read((char*)(&typeT), 8); // 파일 헤더: 타입 필드 읽기

        vertex_count--; // 헤더의 vertex_count는 (실제 정점 수 + 1)이므로 1 감소

        printf("Vertex: %llu, ", vertex_count);
        vertex_size = (vertex_count+1) * sizeof(uint64_t); // +1: 마지막 정점의 끝 오프셋

        vertexList_h = (uint64_t*)malloc(vertex_size); // 호스트에 정점 오프셋 배열 할당

        file.read((char*)vertexList_h, vertex_size); // 정점 오프셋 데이터 읽기
        file.close();

        // 간선 리스트(.dst) 파일 읽기
        file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            fprintf(stderr, "Edge file open failed\n");
            exit(1);
        };

        file.read((char*)(&edge_count), 8); // 간선 수 읽기
        file.read((char*)(&typeT), 8);

        printf("Edge: %llu, ", edge_count);
        fflush(stdout);
        edge_size = edge_count * sizeof(EdgeT); // 간선 배열 크기 (바이트)
        edge_size = edge_size + (4096 - (edge_size & 0xFFFULL)); // 4KB 정렬 (NVMe 블록 크기 대응)

        edgeList_h = NULL; // GPUMEM 모드에서만 할당됨

        // 간선 가중치(.val) 파일 읽기
        file2.open(weight_file.c_str(), std::ios::in | std::ios::binary);
        if (!file2.is_open()) {
            fprintf(stderr, "Edge file open failed\n");
            exit(1);
        };

        file2.read((char*)(&weight_count), 8); // 가중치 수 읽기
        file2.read((char*)(&typeT), 8);

        printf("Weight: %llu\n", weight_count);
        fflush(stdout);
        weight_size = weight_count * sizeof(WeightT); // 가중치 배열 크기 (바이트)
        weight_size = weight_size + (4096 - (weight_size & 0xFFFULL)); // 4KB 정렬

        weightList_h = NULL; // GPUMEM 모드에서만 할당됨

        /* 메모리 할당 방식에 따른 edgeList/weightList 적재 */
        switch (mem) {
            case GPUMEM:
                /* GPUMEM: 호스트에서 파일 읽기 → cudaMalloc → cudaMemcpy로 GPU에 전부 적재 */
                edgeList_h = (EdgeT*)malloc(edge_size); // 호스트 간선 배열
                weightList_h = (WeightT*)malloc(weight_size); // 호스트 가중치 배열
                file.read((char*)edgeList_h, edge_size); // 간선 데이터 파일 읽기
                file2.read((char*)weightList_h, weight_size); // 가중치 데이터 파일 읽기
                cuda_err_chk(cudaMalloc((void**)&edgeList_d, edge_size)); // GPU 간선 배열 할당
                cuda_err_chk(cudaMalloc((void**)&weightList_d, weight_size)); // GPU 가중치 배열 할당

                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_h[i] += offset; // 가중치에 오프셋 적용 (현재 0)

                break;
            case UVM_READONLY:
                /* UVM_READONLY: cudaMallocManaged로 할당 후 ReadMostly 힌트로 GPU에 복제본 생성 */
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size)); // UVM 간선 배열
                cuda_err_chk(cudaMallocManaged((void**)&weightList_d, weight_size)); // UVM 가중치 배열
                file.read((char*)edgeList_d, edge_size); // UVM 주소에 직접 파일 읽기
                file2.read((char*)weightList_d, weight_size);

                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_d[i] += offset;

                /* ReadMostly 힌트: GPU에 읽기 전용 복제본을 생성하여 PCIe 트래픽 감소 */
                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(weightList_d, weight_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));

                /* GPU 메모리 throttle: GPU VRAM > 16GB이면 나머지를 더미 할당하여 UVM 페이지 폴트를 강제로 유발 */
                cuda_err_chk(cudaMemGetInfo(&freebyte, &totalbyte));
                if (totalbyte < 16*1024*1024*1024ULL)
                    printf("total memory sizeo of current GPU is %llu byte, no need to throttle\n", totalbyte);
                else {
                    printf("total memory sizeo of current GPU is %llu byte, throttling %llu byte.\n", totalbyte, totalbyte - 16*1024*1024*1024ULL);
                    cuda_err_chk(cudaMalloc((void**)&pad, totalbyte - 16*1024*1024*1024ULL));
                    throttle_memory<<<1,1>>>(pad); // 더미 커널로 할당 확정
                }
                break;
            case UVM_DIRECT:
            {
                /* UVM_DIRECT + O_DIRECT: 파일 시스템 캐시를 바이패스하여 DMA 직접 읽기
                 * AccessedBy 힌트: 데이터를 호스트 메모리에 두고 GPU가 PCIe로 접근 */
/*
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size));
                cuda_err_chk(cudaMallocManaged((void**)&weightList_d, weight_size));
                file.read((char*)edgeList_d, edge_size);
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();
                file2.read((char*)weightList_d, weight_size);
                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "weight file read time: "<< time_span.count() <<std::endl;

                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_d[i] += offset;

                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(weightList_d, weight_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                break;
  */

                file.close();
                /* O_DIRECT 사용 시 파일 헤더(count+type = 16B)가 포함되므로 vertexList 오프셋을 +2 보정 */
                for (uint64_t i = 0; i < vertex_count + 1; i++) {
                    vertexList_h[i] += 2;
                }
                int fd = open(edge_file.c_str(), O_RDONLY | O_DIRECT); // O_DIRECT: 파일시스템 캐시 바이패스
                FILE *file_temp = fdopen(fd, "rb");
                if ((file_temp == NULL) || (fd == -1)) {
                    printf("edge file fd open failed\n");
                    exit(1);
                }   
                /* 4KB 정렬: O_DIRECT는 읽기 크기가 블록 경계에 정렬되어야 함 */
                uint64_t edge_count_4k_aligned = ((edge_count + 2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t));
                uint64_t edge_size_4k_aligned = edge_count_4k_aligned * sizeof(uint64_t);
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size_4k_aligned)); // UVM 할당 (4KB 정렬 크기)
                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice)); // AccessedBy: 호스트에 데이터 유지, GPU가 원격 접근
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();
                      
                if (fread(edgeList_d, sizeof(uint64_t), edge_count_4k_aligned, file_temp) != edge_count + 2) {
                    printf("edge file fread failed\n");
                    exit(1);
                }   
                fclose(file_temp);                                                                                                              
                close(fd);
                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "edge file read time: "<< time_span.count() <<std::endl;
                      
                file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
                if (!file.is_open()) {
                    printf("edge file open failed\n");
                    exit(1);
                }  
             
                file2.close();
                int fdw = open(weight_file.c_str(), O_RDONLY | O_DIRECT); 
                FILE *fw_tmp = fdopen(fdw, "rb"); 
                if ((fw_tmp == NULL) || (fdw == -1)) {
                    printf("Weight file fd open failed\n");
                    exit(1);
                }
                 
                uint64_t weight_count_4k_aligned = ((weight_count +2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t)); 
                uint64_t weight_size_4k_aligned = weight_count_4k_aligned * sizeof(uint64_t); 
                 
                cuda_err_chk(cudaMallocManaged((void**)&weightList_d, weight_size_4k_aligned));
                high_resolution_clock::time_point ftw1 = high_resolution_clock::now();                                                                                                                          
                 
                 
                if (fread(weightList_d, sizeof(uint64_t), weight_count_4k_aligned, file_temp) != weight_count + 2) {
                    printf("Weight file fread failed\n");
                    exit(1);
                }
                high_resolution_clock::time_point ftw2 = high_resolution_clock::now();
                duration<double> wtime_span = duration_cast<duration<double>>(ftw2 -ftw1);
                std::cout<< "weight file read time: "<< wtime_span.count() <<std::endl;
                 
                for (uint64_t i = 0; i < weight_count; i++)
                    weightList_d[i] += offset;
                cuda_err_chk(cudaMemAdvise(edgeList_d, weight_size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                 
                file2.open(weight_file.c_str(), std::ios::in | std::ios::binary);
                if (!file2.is_open()) {
                    printf("weight file open failed\n");
                    exit(1);
                }
                break;

            }
            case BAFS_DIRECT:
                /* BAFS_DIRECT: edgeList/weightList를 GPU 메모리에 적재하지 않음.
                 * 대신 page_cache를 통해 커널에서 on-demand로 SSD에서 fetch한다.
                 * GPU 메모리 throttle만 수행하여 UVM과의 공정한 비교를 위해 가용 VRAM을 제한 */
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

        // GPU 메모리 할당: SSSP 알고리즘에 필요한 배열들
        cuda_err_chk(cudaMalloc((void**)&vertexList_d, vertex_size)); // CSR 정점 오프셋 (GPU)
        cuda_err_chk(cudaMalloc((void**)&label_d, vertex_count * sizeof(bool))); // 활성 정점 플래그 배열
        cuda_err_chk(cudaMalloc((void**)&changed_d, sizeof(bool))); // 수렴 판정 플래그
        cuda_err_chk(cudaMalloc((void**)&costList_d, vertex_count * sizeof(WeightT))); // 확정 비용 배열
        cuda_err_chk(cudaMalloc((void**)&newCostList_d, vertex_count * sizeof(WeightT))); // 후보 비용 배열 (완화 결과)

        printf("Allocation finished\n");
        fflush(stdout);

        // 정점 오프셋 배열을 GPU로 전송
        cuda_err_chk(cudaMemcpy(vertexList_d, vertexList_h, vertex_size, cudaMemcpyHostToDevice));

        /* GPUMEM 모드에서만 간선/가중치를 호스트→GPU로 복사 (UVM/BAFS는 불필요) */
        if (mem == GPUMEM) {
            cuda_err_chk(cudaMemcpy(edgeList_d, edgeList_h, edge_size, cudaMemcpyHostToDevice));
            cuda_err_chk(cudaMemcpy(weightList_d, weightList_h, weight_size, cudaMemcpyHostToDevice));
        }

        /* 커널 타입별 CUDA 그리드 블록 수 계산
         * BASELINE: 정점당 1스레드 → vertex_count / numthreads
         * COALESCE: 정점당 1워프(32스레드) → vertex_count * 32 / numthreads
         * COALESCE_CHUNK: 정점당 (32/8)=4스레드 → vertex_count * 4 / numthreads */
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

        numblocks_update = ((vertex_count + numthreads) / numthreads); // update 커널은 항상 정점당 1스레드

        /* 2D 그리드: x 차원 = BLOCK_NUM(1024), y 차원으로 나머지 블록 분배 */
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

        /* BaM page cache 객체들:
         * SSSP는 edgeList와 weightList 두 개의 SSD 배열이 필요하므로,
         * 하나의 page_cache_t에 두 개의 range_t(h_erange, h_wrange)를 등록한다.
         * h_erange: SSD의 ofileoffset 위치에 저장된 edgeList 매핑
         * h_wrange: SSD의 wfileoffset 위치에 저장된 weightList 매핑
         * 커널에 h_earray->d_array_ptr, h_warray->d_array_ptr을 전달하여 SSD 접근 */
        page_cache_t* h_pc;

        range_t<uint64_t>* h_erange;
        range_t<WeightT>* h_wrange;
        std::vector<range_t<uint64_t>*> vec_erange(1);
        std::vector<range_t<WeightT>*> vec_wrange(1);
        array_t<uint64_t>* h_earray;
        array_t<WeightT>* h_warray;

        uint64_t n_epages = ceil(((float)edge_size)/pc_page_size);   // edgeList가 차지하는 SSD 페이지 수
        uint64_t n_wpages = ceil(((float)weight_size)/pc_page_size); // weightList가 차지하는 SSD 페이지 수


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



        /* SSSP 반복 실행 루프: 시작 정점을 바꿔가며 total_run회 실행 */
        for (int i = 0; i < total_run; i++) {
            zero = 0; // 시작 정점의 비용 = 0
            one = 1;
            /* SSSP 초기화: 모든 비용을 0xFFFFFFFF(무한대)로 설정 */
            cuda_err_chk(cudaMemset(costList_d, 0xFF, vertex_count * sizeof(WeightT)));
            cuda_err_chk(cudaMemset(newCostList_d, 0xFF, vertex_count * sizeof(WeightT)));
            cuda_err_chk(cudaMemset(label_d, 0x0, vertex_count * sizeof(bool))); // 모든 정점 비활성
            cuda_err_chk(cudaMemcpy(&label_d[src], &one, sizeof(bool), cudaMemcpyHostToDevice)); // 시작 정점만 활성
            cuda_err_chk(cudaMemcpy(&costList_d[src], &zero, sizeof(WeightT), cudaMemcpyHostToDevice)); // 시작 정점 비용 = 0
            cuda_err_chk(cudaMemcpy(&newCostList_d[src], &zero, sizeof(WeightT), cudaMemcpyHostToDevice));

            iter = 0;

            cuda_err_chk(cudaEventRecord(start, 0)); // 타이밍 시작

            /* SSSP 메인 루프: 수렴할 때까지(changed_h == false) 반복
             * 매 반복: (1) 완화 커널 실행 → (2) update 커널로 비용 갱신 및 수렴 확인 */
            do {
                changed_h = false; // 매 반복 시작 시 수렴 플래그 초기화
                cuda_err_chk(cudaMemcpy(changed_d, &changed_h, sizeof(bool), cudaMemcpyHostToDevice));
                auto start = std::chrono::system_clock::now(); // 반복당 시간 측정
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

                /* update 커널: newCostList < costList인 정점을 갱신하고 재활성화 */
                update<<<blockDim_update, numthreads>>>(label_d, costList_d, newCostList_d, vertex_count, changed_d);

                iter++; // 반복 카운터 증가

                /* 수렴 플래그를 GPU→호스트로 복사하여 루프 종료 조건 확인 */
                cuda_err_chk(cudaMemcpy(&changed_h, changed_d, sizeof(bool), cudaMemcpyDeviceToHost));
                auto end = std::chrono::system_clock::now();

                //if(mem == BAFS_DIRECT) {
                //    h_earray->print_reset_stats();
                //    h_warray->print_reset_stats();
                //    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                //    std::cout << std::dec << "Time: " << elapsed.count() << " ms" << std::endl;
                //}
                //break;
            } while(changed_h); // changed_h가 false이면 모든 정점이 수렴 → 루프 종료

            cuda_err_chk(cudaEventRecord(end, 0)); // 타이밍 종료
            cuda_err_chk(cudaEventSynchronize(end)); // GPU 작업 완료 대기
            cuda_err_chk(cudaEventElapsedTime(&milliseconds, start, end)); // 경과 시간 계산 (밀리초)
            /* iter > 1: 1회만 반복한 그래프(간선이 거의 없는 시작점)는 통계에서 제외 */
            if(iter > 1){
                  printf("run %*d: ", 3, i);
                  printf("src %*u, ", 10, src);
                  printf("iteration %*u, ", 3, iter);
                  printf("time %*f ms\n", 12, milliseconds);
                  if(mem == BAFS_DIRECT) {
                     h_earray->print_reset_stats(); // BaM page cache 히트/미스 통계 출력
                     h_warray->print_reset_stats();
                  }
                  fflush(stdout);

                  avg_milliseconds += (double)milliseconds; // 평균 시간 누적
                  num_run++;
            }
            src += vertex_count / total_run; // 다음 실행의 시작 정점을 균등 분배

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

//        printf("Average run time %f ms\n", avg_milliseconds / num_run);
        printf("\nSSSP Graph:%s \t Impl: %d \t SSD: %d \t PageSize: %d \t AvgTime %f ms\n", filename.c_str(), type, settings.n_ctrls, settings.pageSize, avg_milliseconds / num_run);




        /* 호스트 메모리 해제 */
        free(vertexList_h);
        if (edgeList_h)
            free(edgeList_h);
        if (weightList_h)
            free(weightList_h);

        /* BaM page cache 관련 객체 해제 (PC 타입에서만 생성됨) */
        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
                delete h_pc;
                delete h_erange;
                delete h_earray;
                //    delete h_wpc;
                delete h_wrange;
                delete h_warray;
        }

        /* GPU 메모리 해제 */
        cuda_err_chk(cudaFree(vertexList_d));
        cuda_err_chk(cudaFree(costList_d));
        cuda_err_chk(cudaFree(newCostList_d));
        cuda_err_chk(cudaFree(label_d));
        cuda_err_chk(cudaFree(changed_d));

        /* BAFS_DIRECT에서는 edgeList/weightList가 GPU에 할당되지 않으므로 해제하지 않음 */
        if(mem!=BAFS_DIRECT){
            cuda_err_chk(cudaFree(weightList_d));
            cuda_err_chk(cudaFree(edgeList_d));
        }

        /* NVMe 컨트롤러 객체 해제 */
        for (size_t i = 0 ; i < settings.n_ctrls; i++)
             delete ctrls[i];
    }
    catch (const error& e){
        fprintf(stderr, "Unexpected error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
