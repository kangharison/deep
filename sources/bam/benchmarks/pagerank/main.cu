/* References:
 *
 *      Baseline
 *          Harish, Pawan, and P. J. Narayanan.
 *          "Accelerating large graph algorithms on the GPU using CUDA."
 *          International conference on high-performance computing.
 *          Springer, Berlin, Heidelberg, 2007.
 *
 *      Coalesce
 *          Hong, Sungpack, et al.
 *          "Accelerating CUDA graph algorithms at maximum warp."
 *          Acm Sigplan Notices 46.8 (2011): 267-276.
 *
 */

/*
 * [BaM PageRank 벤치마크 - 메인 파일]
 *
 * PageRank(페이지랭크) 그래프 알고리즘의 GPU 벤치마크.
 * 각 정점의 중요도를 반복적으로 계산하며, delta-based push 방식을 사용한다.
 *
 * PageRank 알고리즘 (Delta-Based Push):
 *   1. 초기화: value[v] = 1-alpha, delta[v] = (1-alpha)*alpha / degree(v), residual[v] = 0
 *   2. Scatter: 활성 정점 v의 delta[v]를 모든 이웃 u에게 atomicAdd로 residual[u]에 분배
 *   3. Update: residual[v] > tolerance인 정점만 활성화
 *      - value[v] += residual[v]
 *      - delta[v] = residual[v] * alpha / degree(v)
 *      - residual[v] = 0
 *   4. 수렴 조건: tolerance를 초과하는 residual이 없거나 최대 100회 반복
 *   - alpha = 0.85 (damping factor), tolerance = 0.001
 *
 * CSR(Compressed Sparse Row) 포맷:
 *   vertexList[v] ~ vertexList[v+1]: 정점 v의 이웃 리스트 범위 (edgeList 인덱스)
 *   edgeList[i]: 간선의 목적지 정점 ID
 *   degree(v) = vertexList[v+1] - vertexList[v]
 *
 * 커널 변형:
 *   BASELINE(0)           - 스레드당 1 정점, 순차 이웃 순회
 *   COALESCE(1)           - 워프(32스레드)당 1 정점, coalesced 메모리 접근
 *   COALESCE_CHUNK(2)     - 워프당 CHUNK_SIZE(8)개 정점 순차 처리
 *   *_PC 변형(3,4,5)      - BaM page_cache의 seq_read()로 SSD 직접 I/O
 *   *_HASH 변형(6,7,8)    - 정점 ID 인터리빙으로 SM 간 부하 분산
 *   *_HASH_PC 변형(9,10,11) - 해시 + 페이지 캐시 결합
 *
 * BaM page_cache를 통한 GPU-SSD 직접 I/O 패턴:
 *   BAFS_DIRECT(6) 모드에서:
 *   1. Controller 생성 -> page_cache_t 생성 -> range_t -> array_t
 *   2. 커널에서 da->seq_read(i)로 edgeList 접근
 *   3. 해당 페이지가 캐시에 없으면 NVMe SSD에서 P2P DMA로 자동 fetch
 *
 * shift_start = start & 0xFFFFFFFFFFFFFFF0:
 *   16-원소(128바이트) 경계 정렬로 GPU 메모리 coalescing 최적화.
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
#include <page_cache.h>
#include <util.h>

using error = std::runtime_error;
using std::string;
//const char* const ctrls_paths[] = {"/dev/libnvmpro0", "/dev/libnvmpro1", "/dev/libnvmpro2", "/dev/libnvmpro3", "/dev/libnvmpro4", "/dev/libnvmpro5", "/dev/libnvmpro6", "/dev/libnvmpro7"};
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7"};
//const char* const ctrls_paths[] = {"/dev/libnvmpro0", "/dev/libnvmpro2", "/dev/libnvmpro3", "/dev/libnvmpro4", "/dev/libnvmpro5", "/dev/libnvmpro6", "/dev/libnvmpro7"};
//const char* const ctrls_paths[] = {"/dev/libnvmpro0"};


#define MYINFINITY 0xFFFFFFFF

#define WARP_SHIFT 5
#define WARP_SIZE 32

#define CHUNK_SHIFT 3
#define CHUNK_SIZE (1 << CHUNK_SHIFT)

#define BLOCK_SIZE 1024ULL

#define MAXWARP 64

typedef uint64_t EdgeT;

typedef float ValueT;

typedef enum {
    BASELINE = 0,
    COALESCE = 1,
    COALESCE_CHUNK = 2,
    BASELINE_PC = 3,
    COALESCE_PC = 4,
    COALESCE_CHUNK_PC =5,
    BASELINE_HASH= 6,
    COALESCE_HASH= 7,
    COALESCE_CHUNK_HASH= 8,
    BASELINE_HASH_PC= 9,
    COALESCE_HASH_PC= 10,
    COALESCE_CHUNK_HASH_PC= 11,

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

/* initialize: PageRank 초기값 설정 커널.
 *   value[v] = 1 - alpha (모든 정점의 초기 PageRank 값)
 *   delta[v] = (1-alpha) * alpha / degree(v) (첫 반복에서 이웃에 분배할 값)
 *   residual[v] = 0
 *   label[v] = true (모든 정점을 활성 상태로 설정) */
__global__ void initialize(bool *label, ValueT *delta, ValueT *residual, ValueT *value, const uint64_t vertex_count, const uint64_t *vertexList, ValueT alpha) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    if (tid < vertex_count) {
        value[tid] = 1.0f - alpha;
        delta[tid] = (1.0f - alpha) * alpha / (vertexList[tid+1] - vertexList[tid]);
        residual[tid] = 0.0f;
        label[tid] = true;
	}
}

/* kernel_baseline: 스레드당 1 정점 처리하는 기본 PageRank scatter 커널.
 * 활성 정점 tid의 delta[tid]를 모든 이웃에게 atomicAdd로 분배한다.
 * 분배 완료 후 label[tid] = false로 비활성화. */
__global__ void kernel_baseline(bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    // const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if(tid < vertex_count && label[tid]) {
        const uint64_t start = vertexList[tid];
        // const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[tid+1];

        for(uint64_t i = start; i < end; i += 1){
            // if (i >= start){
                EdgeT next = edgeList[i];
                atomicAdd(&residual[next], delta[tid]);
            // }
        }
        label[tid] = false;
    }
}

/* kernel_baseline_pc: BaM page_cache를 통해 SSD에서 직접 edgeList를 읽는 baseline 커널.
 * da->seq_read(i)로 접근하면 해당 페이지가 캐시에 없을 때 NVMe SSD에서 P2P DMA로 fetch. */
__global__ __launch_bounds__(1024,2)
void kernel_baseline_pc(array_d_t<uint64_t>* da, bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    // const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    // array_d_t<uint64_t> d_array = *da;

    if(tid < vertex_count && label[tid]) {
        const uint64_t start = vertexList[tid];
        // const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[tid+1];

        for(uint64_t i = start; i < end; i += 1){
            // if (i >= start){
                // EdgeT next = edgeList[i];
                EdgeT next = da->seq_read(i);
                atomicAdd(&residual[next], delta[tid]);
            // }
        }
        label[tid] = false;
    }
}



/* kernel_baseline_hash: 해시 기반 정점 재배치로 SM 간 부하를 분산하는 baseline 커널.
 * oldtid를 STRIDE 간격으로 인터리빙: tid = (old/nep) + (old%nep)*STRIDE */
__global__ void kernel_baseline_hash(bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, int sm_count) {
    const uint64_t oldtid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    // const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = sm_count * MAXWARP;

    if(oldtid < vertex_count){
        uint64_t tid;
        const uint64_t nep = vertex_count/STRIDE;
        if(oldtid <(STRIDE*nep)){
            tid = (oldtid/nep) + ((oldtid % nep)*STRIDE);
        }
        else{
            tid = oldtid;
        }


        if(label[tid]) {
            const uint64_t start = vertexList[tid];
            // const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[tid+1];

            for(uint64_t i = start; i < end; i += 1){
                // if (i >= start){
                    EdgeT next = edgeList[i];
                    atomicAdd(&residual[next], delta[tid]);
                // }
            }
            label[tid] = false;
        }
    }
}

/* kernel_baseline_hash_pc: 해시 + BaM page_cache 결합 baseline 커널 */
__global__ //__launch_bounds__(1024,2)
void kernel_baseline_hash_pc(array_d_t<uint64_t>* da, bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, int sm_count) {
     const uint64_t oldtid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    // const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = sm_count * MAXWARP;

    if(oldtid < vertex_count){
        uint64_t tid;
        const uint64_t nep = vertex_count/STRIDE;
        if(oldtid <(STRIDE*nep)){
            tid = (oldtid/nep) + ((oldtid % nep)*STRIDE);
        }
        else{
            tid = oldtid;
        }


        if(label[tid]) {
            const uint64_t start = vertexList[tid];
            // const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[tid+1];

            for(uint64_t i = start; i < end; i += 1){
                // if (i >= start){
                    // EdgeT next = edgeList[i];
                    EdgeT next = da->seq_read(i);
                    atomicAdd(&residual[next], delta[tid]);
                // }
            }
            label[tid] = false;
        }
    }
}



/* kernel_coalesce: 워프(32스레드)당 1 정점을 처리하는 coalesced PageRank scatter 커널.
 * 워프 내 스레드들이 shift_start부터 WARP_SIZE 간격으로 edgeList를 병렬 접근. */
__global__ void kernel_coalesce(bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if(warpIdx < vertex_count && label[warpIdx]) {
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE)
            if (i >= start){
                EdgeT next = edgeList[i];
                atomicAdd(&residual[next], delta[warpIdx]);
            }

        label[warpIdx] = false;
    }
}

/* kernel_coalesce_pc: coalesce + BaM page_cache 결합 커널 */
__global__ //__launch_bounds__(128,16)
void kernel_coalesce_pc(array_d_t<uint64_t>* da, bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    // array_d_t<uint64_t> d_array = *da;
    if(warpIdx < vertex_count && label[warpIdx]) {
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE)
            if (i >= start){
                // EdgeT next = edgeList[i];
                // EdgeT next = d_array.seq_read(i);
                EdgeT next = da->seq_read(i);
                atomicAdd(&residual[next], delta[warpIdx]);
            }

        label[warpIdx] = false;
    }
}


/* kernel_coalesce_hash: 해시 기반 워프 재배치 + coalesced 접근 */
__global__ void kernel_coalesce_hash(bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, int sm_count) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = sm_count * MAXWARP;

    if(oldwarpIdx < vertex_count){
        uint64_t warpIdx;
        const uint64_t nep = vertex_count/STRIDE;
        if(oldwarpIdx <(STRIDE*nep)){
                warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*STRIDE);
        }
        else{
                warpIdx = oldwarpIdx;
        }

        if(label[warpIdx]) {
            const uint64_t start = vertexList[warpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[warpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE)
                if (i >= start){
                    EdgeT next = edgeList[i];
                    atomicAdd(&residual[next], delta[warpIdx]);
                }

            label[warpIdx] = false;
        }
    }
}

/* kernel_coalesce_hash_pc: 해시 + coalesce + BaM page_cache 결합 커널 */
__global__ //__launch_bounds__(128,16)
void kernel_coalesce_hash_pc(array_d_t<uint64_t>* da, bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, int sm_count) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = sm_count * MAXWARP;

    if(oldwarpIdx < vertex_count){
        uint64_t warpIdx;
        const uint64_t nep = vertex_count/STRIDE;
        if(oldwarpIdx <(STRIDE*nep)){
                warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*STRIDE);
        }
        else{
                warpIdx = oldwarpIdx;
        }

        if(label[warpIdx]) {
            const uint64_t start = vertexList[warpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[warpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE)
                if (i >= start){
                    // EdgeT next = edgeList[i];
                    // EdgeT next = d_array.seq_read(i);
                    EdgeT next = da->seq_read(i);
                    atomicAdd(&residual[next], delta[warpIdx]);
                }

            label[warpIdx] = false;
        }
    }
}


/* kernel_coalesce_chunk: CHUNK_SIZE(8)개 정점을 하나의 워프가 순차 처리하는 커널 */
__global__ void kernel_coalesce_chunk(bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
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
        if(label[i]) {
            const uint64_t start = vertexList[i];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[i+1];

            for(uint64_t j = shift_start + laneIdx; j < end; j += WARP_SIZE)
                if (j >= start){
                    EdgeT next = edgeList[j];
                    atomicAdd(&residual[next], delta[i]);
                }

            label[i] = false;
        }
    }
}

/* kernel_coalesce_chunk_pc: chunk + BaM page_cache 결합 커널 */
__global__ //__launch_bounds__(1024,2)
void kernel_coalesce_chunk_pc(array_d_t<uint64_t>* da, bool* label, ValueT *delta, ValueT *residual, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    const uint64_t chunkIdx = warpIdx * CHUNK_SIZE;
    uint64_t chunk_size = CHUNK_SIZE;
    // array_d_t<uint64_t> d_array = *da;
    if((chunkIdx + CHUNK_SIZE) > vertex_count) {
        if ( vertex_count > chunkIdx )
            chunk_size = vertex_count - chunkIdx;
        else
            return;
    }

    for(uint32_t i = chunkIdx; i < chunk_size + chunkIdx; i++) {
        if(label[i]) {
            const uint64_t start = vertexList[i];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[i+1];

            for(uint64_t j = shift_start + laneIdx; j < end; j += WARP_SIZE)
                if (j >= start){
                    // EdgeT next = edgeList[j];
                    // EdgeT next = d_array.seq_read(j);
                    EdgeT next = da->seq_read(j);
                    atomicAdd(&residual[next], delta[i]);
                }

            label[i] = false;
        }
    }
}




/* update: PageRank 수렴 판정 + 값 갱신 커널.
 * residual[tid] > tolerance인 정점만 활성화하여 다음 반복을 준비한다.
 *   value[tid] += residual[tid]
 *   delta[tid] = residual[tid] * alpha / degree(tid)
 *   residual[tid] = 0, label[tid] = true, *changed = true */
__global__ void update(bool *label, ValueT *delta, ValueT *residual, ValueT *value, const uint64_t vertex_count, const uint64_t *vertexList, ValueT tolerance, ValueT alpha, bool *changed) {
    const uint64_t tid = blockDim.x * BLOCK_SIZE * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    if (tid < vertex_count && residual[tid] > tolerance) {
        value[tid] += residual[tid];
        delta[tid] = residual[tid] * alpha / (vertexList[tid+1] - vertexList[tid]);
        residual[tid] = 0.0f;
        label[tid] = true;
        *changed = true;
	}
}

__global__ void throttle_memory(uint32_t *pad) {
    pad[1] = pad[0];
}

/* main(): PageRank 벤치마크 진입점.
 *
 * 전체 흐름:
 *   1. 설정 파싱 및 CUDA 디바이스 초기화
 *   2. CSR 그래프 파일 로드 (.col: vertexList, .dst: edgeList)
 *   3. 메모리 할당 방식에 따라 edgeList를 GPU에 적재
 *   4. BAFS_DIRECT인 경우: NVMe 컨트롤러 생성 -> page_cache -> range_t -> array_t
 *   5. initialize 커널로 PageRank 초기값 설정
 *   6. 반복: scatter 커널 -> update 커널 -> 수렴 확인 (최대 100회)
 *   7. 성능 측정 및 결과 출력 */
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

    std::ifstream file;
    std::string vertex_file, edge_file;
    std::string filename;

    bool changed_h, *changed_d, *label_d; // changed_h: 수렴 플래그, label_d: 활성 정점 배열
    // int c, arg_num = 0;
    impl_type type; // 커널 구현 타입 (BASELINE/COALESCE/HASH 등)
    mem_type mem; // 메모리 할당 방식 (GPUMEM/UVM/BAFS_DIRECT)
    ValueT *delta_d, *residual_d, *value_d, *value_h, *delta_h; // delta: 이웃에 분배할 값, residual: 누적 수신값, value: PageRank 값
    ValueT tolerance, alpha; // alpha: damping factor, tolerance: 수렴 임계값
    uint32_t iter; // PageRank 반복 카운터
    uint64_t *vertexList_h, *vertexList_d; // CSR 정점 오프셋 (호스트/디바이스)
    EdgeT *edgeList_h, *edgeList_d; // CSR 간선 리스트 (호스트/디바이스)
    uint64_t vertex_count, edge_count, vertex_size, edge_size; // 그래프 크기 정보
    uint64_t numblocks, numblocks_update, numthreads; // CUDA 그리드 차원 변수
    uint64_t typeT; // 파일 헤더 타입 필드
    size_t freebyte, totalbyte; // GPU 메모리 조회용
    uint32_t *pad; // GPU 메모리 throttle용 더미 배열
    EdgeT *edgeList_dtmp; // UVM 재할당 시 임시 포인터 (미사용)


    float milliseconds; // 총 커널 실행 시간 (CUDA 이벤트)
    double avg_milliseconds; // 평균 실행 시간

    uint64_t pc_page_size; // BaM page cache 페이지 크기
    uint64_t pc_pages; // BaM page cache 총 페이지 수

    alpha = 0.85;       /* PageRank damping factor: 랜덤 서핑 모델에서 링크를 따를 확률 */
    tolerance = 0.001;  /* 수렴 판정 임계값: residual > tolerance인 정점만 활성 */

    try{
        // 설정값으로부터 실행 파라미터 준비
        filename = std::string(settings.input); // 그래프 파일 기본 경로

        // if(settings.src == 0) {
        //         num_run = settings.repeat;
        //         src = 0;
        // }
        // else {
        //         num_run = 1;
        //         src = settings.src;
        // }

        type = (impl_type) settings.type; // 커널 구현 타입 선택
        mem = (mem_type) settings.memalloc; // 메모리 할당 방식 선택

        pc_page_size = settings.pageSize; // BaM page cache 페이지 크기
        pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size); // 최대 캐시 크기에서 페이지 수 계산

        numthreads = settings.numThreads; // CUDA 블록당 스레드 수

        cuda_err_chk(cudaSetDevice(settings.cudaDevice));

        cudaEvent_t start, end;
        cuda_err_chk(cudaEventCreate(&start));
        cuda_err_chk(cudaEventCreate(&end));

        vertex_file = filename + ".col";
        edge_file = filename + ".dst";

        std::cout << filename << std::endl;
        fprintf(stderr, "File %s\n", filename.c_str());
        /* CSR 그래프 로드: vertexList(.col)와 edgeList(.dst) */
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

        file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            fprintf(stderr, "Edge file open failed\n");
            exit(1);
        };

        file.read((char*)(&edge_count), 8);
        file.read((char*)(&typeT), 8);

        printf("Edge: %llu\n", edge_count);
        fflush(stdout);
        edge_size = edge_count * sizeof(EdgeT); // 간선 배열 크기 (바이트)
        edge_size = edge_size + (4096 - (edge_size & 0xFFFULL)); // 4KB 정렬 (NVMe 블록 크기 대응)

        edgeList_h = NULL; // GPUMEM 모드에서만 호스트에 할당됨

        /* GPU 메모리 할당: PageRank 알고리즘에 필요한 배열들 */
        cuda_err_chk(cudaMalloc((void**)&label_d, vertex_count * sizeof(bool))); // 활성 정점 플래그
        cuda_err_chk(cudaMalloc((void**)&vertexList_d, vertex_size)); // CSR 정점 오프셋
        cuda_err_chk(cudaMalloc((void**)&changed_d, sizeof(bool))); // 수렴 판정 플래그
        cuda_err_chk(cudaMalloc((void**)&delta_d, vertex_count * sizeof(ValueT))); // 이웃에 분배할 delta 값
        cuda_err_chk(cudaMalloc((void**)&residual_d, vertex_count * sizeof(ValueT))); // 이웃으로부터 누적 수신한 residual
        cuda_err_chk(cudaMalloc((void**)&value_d, vertex_count * sizeof(ValueT))); // 최종 PageRank 값

        value_h = (ValueT*)malloc(vertex_count * sizeof(ValueT)); // 결과 확인용 호스트 배열
        delta_h = (ValueT*)malloc(vertex_count * sizeof(ValueT)); // 결과 확인용 호스트 배열

        /* 메모리 할당 방식에 따른 edgeList 적재
         * PageRank는 edgeList만 대용량이고 delta/residual/value는 정점 수만큼이므로 작다 */
        switch (mem) {
            case GPUMEM:
                /* GPUMEM: 간선 배열을 전용 GPU 메모리에 할당 (호스트에서 읽은 후 cudaMemcpy) */
                cuda_err_chk(cudaMalloc((void**)&edgeList_d, edge_size));
                break;
            case UVM_READONLY:
                /* UVM_READONLY: cudaMallocManaged + ReadMostly → GPU에 읽기 전용 복제본 생성 */
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size));
                file.read((char*)edgeList_d, edge_size); // UVM 주소에 직접 파일 읽기
                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));

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
                /* UVM_DIRECT + O_DIRECT: 파일시스템 캐시 바이패스, AccessedBy로 GPU 원격 접근 */
                file.close();
                /* O_DIRECT는 파일 헤더(16B=2 uint64_t)를 포함하므로 vertexList 오프셋을 +2 보정 */
                for (uint64_t i = 0; i < vertex_count + 1; i++) {
                    vertexList_h[i] += 2;
                }
                int fd = open(edge_file.c_str(), O_RDONLY | O_DIRECT); // O_DIRECT: 커널 페이지 캐시 바이패스
                FILE *file_temp = fdopen(fd, "rb");
                if ((file_temp == NULL) || (fd == -1)) {
                    printf("edge file fd open failed\n");
                    exit(1);
                }
                uint64_t edge_count_4k_aligned = ((edge_count + 2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t));
                uint64_t edge_size_4k_aligned = edge_count_4k_aligned * sizeof(uint64_t);
                cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size_4k_aligned));
                cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
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
                break;
            }
            case BAFS_DIRECT:
                 /* BAFS_DIRECT: GPU 메모리 throttle만 수행. 데이터는 page_cache를 통해 on-demand fetch */
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

        printf("Allocation finished\n");
        fflush(stdout);

        cuda_err_chk(cudaMemcpy(vertexList_d, vertexList_h, vertex_size, cudaMemcpyHostToDevice));

        if (mem == GPUMEM)
            cuda_err_chk(cudaMemcpy(edgeList_d, edgeList_h, edge_size, cudaMemcpyHostToDevice));

        /* 커널 타입별 CUDA 그리드 블록 수 계산
         * BASELINE/HASH: 정점당 1스레드
         * COALESCE: 정점당 1워프(32스레드)
         * COALESCE_CHUNK: 정점당 (32/8)=4스레드 */
        switch (type) {
            case BASELINE:
            case BASELINE_PC:
            case BASELINE_HASH:
            case BASELINE_HASH_PC:
                numblocks = ((vertex_count+numthreads)/numthreads);
                break;
            case COALESCE:
            case COALESCE_PC:
            case COALESCE_HASH:
            case COALESCE_HASH_PC:
               numblocks = ((vertex_count * WARP_SIZE + numthreads) / numthreads);
                break;
            case COALESCE_CHUNK:
            case COALESCE_CHUNK_PC:
            case COALESCE_CHUNK_HASH:
            case COALESCE_CHUNK_HASH_PC:
                numblocks = ((vertex_count * (WARP_SIZE / CHUNK_SIZE) + numthreads) / numthreads);
                break;
            default:
                fprintf(stderr, "Invalid type\n");
                exit(1);
                break;
        }

        numblocks_update = ((vertex_count + numthreads) / numthreads); // update 커널은 항상 정점당 1스레드

        /* 2D 그리드: x = BLOCK_SIZE(1024), y = 나머지 블록 분배 */
        dim3 blockDim(BLOCK_SIZE, (numblocks+BLOCK_SIZE)/BLOCK_SIZE);
        dim3 blockDim_update(BLOCK_SIZE, (numblocks_update+BLOCK_SIZE)/BLOCK_SIZE);

        /* BAFS_DIRECT: NVMe 컨트롤러 초기화 */
        std::vector<Controller*> ctrls(settings.n_ctrls);
        if(mem == BAFS_DIRECT){
            cuda_err_chk(cudaSetDevice(settings.cudaDevice));
            for (size_t i = 0 ; i < settings.n_ctrls; i++)
                ctrls[i] = new Controller(ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);
            printf("Controllers Created\n");
        }
        printf("Initialization done\n");
        fflush(stdout);

        avg_milliseconds = 0.0f;
        iter = 0;

        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC) || (type == BASELINE_HASH_PC) || (type == COALESCE_HASH_PC) ||(type == COALESCE_CHUNK_HASH_PC)){
            printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
        }

        /* BaM 페이지 캐시 초기화 */
        page_cache_t* h_pc;
        range_t<uint64_t>* h_range;
        std::vector<range_t<uint64_t>*> vec_range(1);
        array_t<uint64_t>* h_array;
        uint64_t n_pages = ceil(((float)edge_size)/pc_page_size);

        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC) || (type == BASELINE_HASH_PC) || (type == COALESCE_HASH_PC) ||(type == COALESCE_CHUNK_HASH_PC)){
            h_pc =new page_cache_t(pc_page_size, pc_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
            h_range = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)edge_count, (uint64_t) (ceil(settings.ofileoffset*1.0/pc_page_size)),(uint64_t)n_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice); //, (uint8_t*)edgeList_d);
            vec_range[0] = h_range;
            h_array = new array_t<uint64_t>(edge_count, settings.ofileoffset, vec_range, settings.cudaDevice);

            printf("Page cache initialized\n");
            fflush(stdout);
         }

        cuda_err_chk(cudaEventRecord(start, 0)); // 타이밍 시작
        // printf("*****baseaddr: %p\n", h_pc->pdt.base_addr);
        // fflush(stdout);
        /* PageRank 초기화: value=1-alpha, delta=(1-alpha)*alpha/degree, residual=0, label=true */
        initialize <<<blockDim_update, numthreads>>> (label_d, delta_d, residual_d, value_d, vertex_count, vertexList_d, alpha);

        /* PageRank 반복 루프: scatter -> update -> 수렴 확인 (최대 100회) */
        do {
            changed_h = false;
            cuda_err_chk(cudaMemcpy(changed_d, &changed_h, sizeof(bool), cudaMemcpyHostToDevice));

            auto itrstart = std::chrono::system_clock::now();
            switch (type) {
                case BASELINE:
                    kernel_baseline<<<blockDim, numthreads>>>(label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d);
                    break;
                case COALESCE:
                    kernel_coalesce<<<blockDim, numthreads>>>(label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d);
                    break;
                case COALESCE_CHUNK:
                    kernel_coalesce_chunk<<<blockDim, numthreads>>>(label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d);
                    break;
                case BASELINE_PC:
                    kernel_baseline_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d);
                    break;
                case COALESCE_PC:
                    kernel_coalesce_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d);
                    break;
                case COALESCE_CHUNK_PC:
                    kernel_coalesce_chunk_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d);
                    break;
                case BASELINE_HASH:
                    kernel_baseline_hash<<<blockDim, numthreads>>>(label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d, properties.multiProcessorCount);
                    break;
                case COALESCE_HASH:
                    kernel_coalesce_hash<<<blockDim, numthreads>>>(label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d, properties.multiProcessorCount);
                    break;
                // case COALESCE_HASH_CHUNK:
                //     kernel_coalesce_hash_chunk<<<blockDim, numthreads>>>(label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d, properties.multiProcessorCount);
                //     break;
                case BASELINE_HASH_PC:
                    kernel_baseline_hash_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d, properties.multiProcessorCount);
                    break;
                case COALESCE_HASH_PC:
                    kernel_coalesce_hash_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d, properties.multiProcessorCount);
                    break;
                // case COALESCE_CHUNK_HASH_PC:
                //     kernel_coalesce_chunk_hash_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, delta_d, residual_d, vertex_count, vertexList_d, edgeList_d, properties.multiProcessorCount);
                //     break;
                default:
                    fprintf(stderr, "Invalid type\n");
                    exit(1);
                    break;
            }
            cuda_err_chk(cudaDeviceSynchronize());

            iter++;
            auto itrend = std::chrono::system_clock::now();
            /* update 커널: residual > tolerance인 정점만 활성화 */
            update <<<blockDim_update, numthreads>>> (label_d, delta_d, residual_d, value_d, vertex_count, vertexList_d, tolerance, alpha, changed_d);

            cuda_err_chk(cudaMemcpy(&changed_h, changed_d, sizeof(bool), cudaMemcpyDeviceToHost));

	        //std::chrono::duration<double> elapsed_seconds = itrend-itrstart;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(itrend - itrstart);
            if(mem == BAFS_DIRECT) {
                  h_array->print_reset_stats();
		          //std::cout<< "itr time: "<< elapsed.count() << " ms" <<std::endl;
                  printf("\nPageRank SSD: %d \t PageSize: %d \t ItrTime %f ms\n", settings.n_ctrls, settings.pageSize,(double)elapsed.count());
                  for (auto ct : ctrls) {
                      ct->print_reset_stats();
                  }
            }
        } while(changed_h && iter < 100);


        cuda_err_chk(cudaEventRecord(end, 0));
        cuda_err_chk(cudaEventSynchronize(end));
        cuda_err_chk(cudaEventElapsedTime(&milliseconds, start, end));

        printf("pg iteration %*u, ", 3, iter);
//        printf("time %*f ms\n", 12, milliseconds);
        printf("\nPageRank Graph:%s \t Impl: %d \t SSD: %d \t PageSize: %d \t TotalTime %f ms\n", filename.c_str(), type, settings.n_ctrls, settings.pageSize, milliseconds);
        fflush(stdout);

        avg_milliseconds += (double)milliseconds;

        cuda_err_chk(cudaMemcpy(delta_h, delta_d, vertex_count * sizeof(ValueT), cudaMemcpyDeviceToHost));
        cuda_err_chk(cudaMemcpy(value_h, value_d, vertex_count * sizeof(ValueT), cudaMemcpyDeviceToHost));

        // for(uint64_t tid = 0; tid<32; tid++){
        //         printf("Value of tid: %llu is %f and delta is %f\n", tid, value_h[tid], delta_h[tid]);
        // }

        /* 자원 해제: BaM page cache 객체 (PC 타입에서만 생성됨) */
        if((type == BASELINE_PC) || (type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC) || (type == BASELINE_HASH_PC) || (type == COALESCE_HASH_PC) ||(type == COALESCE_CHUNK_HASH_PC)){
            delete h_pc;
            delete h_range;
            delete h_array;
        }
        /* 호스트/GPU 메모리 해제 */
        free(value_h);
        cuda_err_chk(cudaFree(label_d));
        cuda_err_chk(cudaFree(changed_d));
        cuda_err_chk(cudaFree(vertexList_d));
        cuda_err_chk(cudaFree(delta_d));
        cuda_err_chk(cudaFree(residual_d));
        cuda_err_chk(cudaFree(value_d));

        /* BAFS_DIRECT에서는 edgeList가 GPU에 할당되지 않으므로 해제하지 않음 */
        if(mem!=BAFS_DIRECT)
            cuda_err_chk(cudaFree(edgeList_d));

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
