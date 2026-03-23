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
 * =============================================================================
 * BaM BFS (Breadth-First Search) 벤치마크 - GPU 기반 너비 우선 탐색
 * =============================================================================
 *
 * [알고리즘 개요]
 * BFS(너비 우선 탐색)를 레벨 동기(level-synchronous) 방식으로 GPU에서 수행한다.
 * 시작 정점(src)으로부터 레벨 0에서 출발하여, 매 반복(iteration)마다 현재 레벨의
 * 모든 정점에서 인접 정점을 탐색하고 미방문 정점에 다음 레벨 번호를 부여한다.
 * 새로 방문한 정점이 없을 때까지(changed == false) 반복한다.
 *
 * [그래프 저장 형식 - CSR (Compressed Sparse Row)]
 * - vertexList[] (offsets 배열): 크기 = vertex_count + 1
 *     vertexList[v] ~ vertexList[v+1] 범위가 정점 v의 인접 간선 인덱스이다.
 * - edgeList[] (edges 배열): 크기 = edge_count
 *     edgeList[i]는 i번째 간선이 가리키는 목적지 정점 ID이다.
 *
 * [CUDA 커널 변형 (impl_type)]
 * - BASELINE: 정점당 1스레드. 각 스레드가 자기 정점의 인접 리스트를 순회한다.
 * - COALESCE: 정점당 1워프(32스레드). 워프 내 스레드들이 인접 리스트를 stride 접근하여 메모리 coalescing을 달성한다.
 * - COALESCE_CHUNK: 워프당 CHUNK_SIZE(8)개 정점을 묶어서 처리한다.
 * - COALESCE_HASH: 워프-정점 매핑에 stride 기반 해싱을 적용하여 SSD 페이지 캐시 히트율을 높인다.
 * - COALESCE_COARSE: 스레드 coarsening으로 워프당 여러 정점을 순차 처리한다.
 * - FRONTIER: 프론티어 기반 방식. 현재 레벨 정점만 큐에 넣고 다음 레벨 정점을 생성한다.
 * - OPTIMIZED: 캐시라인(타일) 단위로 워프를 할당하여 SSD I/O 효율을 극대화한다.
 * - *_PC / *_PTR_PC: page_cache(BaM)를 통해 GPU에서 SSD로 직접 I/O하는 변형이다.
 *
 * [메모리 모드 (mem_type)]
 * - GPUMEM: edgeList를 GPU 메모리에 전부 올림 (cudaMalloc)
 * - UVM_READONLY: CUDA Unified Memory + ReadMostly 힌트
 * - UVM_DIRECT: CUDA Unified Memory + AccessedBy 힌트 (zero-copy 방식)
 * - BAFS_DIRECT: BaM page_cache를 통한 GPU-SSD 직접 I/O (핵심 모드)
 *
 * [BaM page_cache를 통한 GPU-SSD 직접 I/O 패턴]
 * BAFS_DIRECT 모드에서는 edgeList를 GPU 메모리에 올리지 않는다.
 * 대신 page_cache_t 객체를 생성하고, range_t/array_t를 통해 SSD 상의 데이터 영역을 매핑한다.
 * CUDA 커널 내에서 da->seq_read(i) 또는 bam_ptr<uint64_t> ptr(da); ptr[i] 형태로
 * edgeList[i]에 접근하면, BaM 런타임이 GPU에서 NVMe SSD로 직접 PCIe DMA를 발행하여
 * 필요한 페이지를 GPU 메모리의 페이지 캐시로 가져온다.
 *
 * [호스트 초기화 흐름]
 * 1. Settings 파싱 (커맨드라인 인자)
 * 2. 그래프 파일 읽기: .col(정점 오프셋), .dst(간선 리스트)
 * 3. GPU 메모리 할당: vertexList_d, label_d, changed_d
 * 4. 메모리 모드에 따라 edgeList 준비 (GPUMEM/UVM/BAFS_DIRECT)
 * 5. BAFS_DIRECT인 경우: Controller 생성 -> page_cache_t 생성 -> range_t/array_t 생성
 * 6. BFS 반복 실행: 소스 정점에서 시작, do-while 루프로 레벨별 커널 호출
 * 7. 결과 출력 및 자원 해제
 * =============================================================================
 */

/* 표준 라이브러리 및 CUDA 헤더 */
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

/* BaM(libnvm) 헤더: NVMe 컨트롤러 제어, GPU 페이지 캐시, P2P I/O */
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
#include <page_cache.h>      // page_cache_t, range_t, array_t, array_d_t, bam_ptr 정의
#include <util.h>
#include <chrono>
#include <iostream>

using error = std::runtime_error;
using std::string;
//const char* const ctrls_paths[] = {"/dev/libnvm0","/dev/libnvm1",   "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
const char* const sam_ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm4", "/dev/libnvm9", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8"};
const char* const intel_ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
//const char* const ctrls_paths[] = {"/dev/libnvm0"};
#define UINT64MAX 0xFFFFFFFFFFFFFFFF

#define MYINFINITY 0xFFFFFFFF       // BFS에서 미방문 정점의 레이블 값 (무한대)

#define WARP_SHIFT 5                // log2(WARP_SIZE) = 5, 워프 인덱스 계산용 비트 시프트
#define WARP_SIZE 32                // CUDA 워프 크기 (32스레드)

#define CHUNK_SHIFT 3               // log2(CHUNK_SIZE) = 3
#define CHUNK_SIZE (1 << CHUNK_SHIFT) // COALESCE_CHUNK 모드에서 워프당 처리할 정점 수 (8개)

#define BLOCK_NUM 1024ULL           // 2D 그리드의 x 차원 블록 수

typedef uint64_t EdgeT;             // 간선 타입: 64비트 정수 (목적지 정점 ID)

/* impl_type: CUDA 커널 구현 방식 열거형
 * - BASELINE(0): 정점당 1스레드, 순차 간선 접근
 * - COALESCE(1): 정점당 1워프, 워프 내 stride 접근으로 메모리 coalescing
 * - COALESCE_CHUNK(2): 워프당 CHUNK_SIZE개 정점 묶음 처리
 * - *_PC(3,4,5): page_cache 사용 (da->seq_read()로 SSD 직접 접근)
 * - FRONTIER_*(6~9): 프론티어 큐 기반 (활성 정점만 처리)
 * - *_PTR_PC(10~14,16): bam_ptr<T>로 SSD 직접 접근 (seq_read 대신 [] 연산자)
 * - COALESCE_HASH*(15,16): stride 해싱으로 워프-정점 매핑 분산 (캐시 히트율 향상)
 * - COALESCE_COARSE*(18~21): 스레드 coarsening (워프당 여러 정점 순차 처리)
 * - COALESCE_HASH_HALF*(22,23): 하프 워프(16스레드) 단위 처리
 * - OPTIMIZED(26,27): 캐시라인/타일 단위 워프 할당 (SSD I/O 최적)
 */
typedef enum {
    BASELINE = 0,
    COALESCE = 1,
    COALESCE_CHUNK = 2,
    BASELINE_PC = 3,
    COALESCE_PC = 4, 
    COALESCE_CHUNK_PC = 5,
    FRONTIER_BASELINE = 6,
    FRONTIER_COALESCE = 7,
    FRONTIER_BASELINE_PC = 8,
    FRONTIER_COALESCE_PC = 9,
    BASELINE_PTR_PC = 10,
    COALESCE_PTR_PC = 11,
    COALESCE_CHUNK_PTR_PC = 12,
    FRONTIER_BASELINE_PTR_PC = 13,
    FRONTIER_COALESCE_PTR_PC = 14,
    COALESCE_HASH = 15,
    COALESCE_HASH_PTR_PC = 16,
    COALESCE_COARSE = 18, 
    COALESCE_HASH_COARSE = 19, 
    COALESCE_COARSE_PTR_PC = 20, 
    COALESCE_HASH_COARSE_PTR_PC = 21, 
    COALESCE_HASH_HALF = 22, 
    COALESCE_HASH_HALF_PTR_PC = 23, 
    OPTIMIZED=26,
    OPTIMIZED_PC=27,
} impl_type;

/* mem_type: 메모리 할당 방식 열거형
 * - GPUMEM(0): 호스트에서 파일을 읽어 cudaMalloc 영역에 복사 (전통적 방식)
 * - UVM_READONLY(1): cudaMallocManaged + ReadMostly 힌트 (GPU 로컬 복제본 생성)
 * - UVM_DIRECT(2): cudaMallocManaged + AccessedBy 힌트 (GPU가 호스트 메모리에 zero-copy 접근)
 * - BAFS_DIRECT(6): BaM page_cache 사용. GPU가 NVMe SSD에 직접 P2P DMA로 접근한다.
 *   이 모드에서는 edgeList를 호스트/GPU 메모리에 올리지 않고, 커널이 필요할 때 SSD에서 페이지를 가져온다.
 */
typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    UVM_READONLY_NVLINK = 3,
    UVM_DIRECT_NVLINK = 4,
    DRAGON_MAP = 5,
    BAFS_DIRECT = 6,
} mem_type;



/* =====================================================================
 * OPTIMIZED 모드 전처리 커널: firstVertexList 생성
 * =====================================================================
 * OPTIMIZED 커널은 edgeList를 캐시라인(타일) 단위로 워프에 할당한다.
 * 이를 위해 각 캐시라인에 포함된 "첫 번째 정점"을 사전 계산해야 한다.
 * step1: 각 정점이 속하는 캐시라인 ID를 계산하고, atomicMin으로 그 캐시라인의 "최소 정점 ID"를 winnerList에 기록
 * step2: winnerList를 이용해 각 캐시라인의 firstVertex를 확정하고, 비어있는 캐시라인은 역추적으로 채움
 */

//TODO: Templatize
//TODO: winnerList is initialized to UINT64MAX
// launch params - number of vertices and each thread does a scatter operation.
__global__ __launch_bounds__(128,16)
void kernel_first_vertex_step1(uint64_t vertex_count, uint64_t *vertexList, uint64_t num_elems_in_cl, unsigned long long int *winnerList){
   const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x; 

   if(tid < vertex_count){
       unsigned long long int clid = (unsigned long long int) vertexList[tid]/(unsigned long long int)num_elems_in_cl;
       atomicMin(&(winnerList[clid]), tid);
   }
}



//TODO: Templatize the kernel for clstart and clend.
//TODO: launch param: number of CL lines in the data. 
__global__ __launch_bounds__(128,16)
void kernel_first_vertex_step2(uint64_t n_cachelines, uint64_t *vertexList, unsigned long long int *winnerList, uint64_t num_elems_in_cl, uint64_t *firstVertexList){

    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x; 
    
    //const uint64_t clstart = tid*num_elems_in_cl; 
    //const uint64_t clend   = (tid+1)*num_elems_in_cl; 

    //check if the cacheline is filled by backtracking. If the winner array has value of RANDMAX, then it should be filled by the backtracking.
    if(tid < n_cachelines){
        uint64_t wid = winnerList[tid]; 
        if(wid!=UINT64MAX){

            uint64_t winVertval = vertexList[wid];

            uint64_t fringes = winVertval % num_elems_in_cl;
            if((fringes == 0)){
                firstVertexList[tid] = wid;
                bool backtrack = false; 
                uint64_t i =1; 
                while(!backtrack){
     //               printf("I got called : %llu\n", i);
                    backtrack = true; 
                    int64_t tmptid = (int64_t)tid- (int64_t)i;
                    if(tmptid>=0){
                        uint64_t pwid = winnerList[tid-i]; 
                        if(pwid == UINT64MAX){
                            backtrack  = false; 
                            firstVertexList[tid-i] = wid-1; 
                        }
                        else {
                            backtrack = true; 
                        }
                    }
                    i++; 
                }
            } else {
                wid                  = wid - 1; 
                uint64_t currVertval = vertexList[wid]; 
                uint64_t nsize       = winVertval - currVertval - fringes; 

                uint64_t backtrackItr = (nsize + num_elems_in_cl)/num_elems_in_cl; 
                for(uint64_t i = 0; i < backtrackItr ; i++){
                    if( (tid-i)>=0 )
                       firstVertexList[tid-i] = wid; //TODO: does this required to be atomicMin?  
                }
            }
        }
    }
}

__global__ __launch_bounds__(128,16)
void kernel_verify(uint64_t count, unsigned long long int *list, uint64_t condval, uint8_t type){
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x; 
    
    if(tid < count){
        switch(type){
            case 1: {
                    if(list[tid] != condval){
                        printf("index %llu is incorrect and has value :%llu \n",(unsigned long long int) tid, list[tid]);
                    }
                    break;
            }
            case 2: {
                    if(list[tid] == condval){
                        printf("index %llu is incorrect and has value :%llu \n",(unsigned long long int) tid, list[tid]);
                    }
                    break;
            }
        }
    }
}






/* =====================================================================
 * 프론티어 기반 BFS 커널들 (FRONTIER_BASELINE, FRONTIER_COALESCE, *_PC, *_PTR_PC)
 * =====================================================================
 * 레벨 동기 방식과 달리, 현재 레벨의 활성 정점(프론티어)만 curr_frontier[] 배열에 넣고
 * 처리한다. 새로 발견된 정점은 next_frontier[]에 추가하고, changed에 카운트를 누적한다.
 * 각 반복 후 curr_frontier와 next_frontier 포인터를 스왑한다.
 *
 * GPU 스레드 구조:
 * - FRONTIER_BASELINE: 프론티어 정점당 1스레드. 스레드가 해당 정점의 간선을 순차 탐색.
 * - FRONTIER_COALESCE: 프론티어 정점당 1워프. 워프 내 32스레드가 간선을 stride 접근.
 * - *_PC 변형: edgeList 접근 시 da->seq_read(i)로 BaM page cache를 통한 SSD I/O.
 * - *_PTR_PC 변형: bam_ptr<uint64_t> ptr(da); ptr[i]로 SSD I/O.
 *
 * 워프 레벨 최적화:
 * - __activemask()로 활성 레인 마스크 획득
 * - 리더 레인이 atomicAdd로 next_frontier 위치 확보
 * - __shfl_sync로 리더의 위치를 전체 워프에 브로드캐스트
 * - __popc(mask & ((1 << laneIdx) - 1))로 각 레인의 상대 오프셋 계산
 */
__global__ __launch_bounds__(128,16)
void kernel_frontier_baseline(unsigned int *label, const unsigned int level, const uint64_t vertex_count,
                                const uint64_t *vertexList, const EdgeT *edgeList, const uint64_t curr_frontier_size, unsigned long long int *changed,
                                const uint32_t *curr_frontier, uint32_t *next_frontier) {
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    //const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint32_t laneIdx = tid &  ((1 << WARP_SHIFT) - 1);

    if (tid < curr_frontier_size) {
        const uint32_t nid = curr_frontier[tid];
        const uint64_t start = vertexList[nid];
        const uint64_t end = vertexList[nid+1];

        for (uint64_t i = start; i < end; i++) {
            const EdgeT next = edgeList[i];

            if(label[next] == MYINFINITY) {
                //unsigned int prev = atomicExch(label+next, level+1);
                //if (prev == MYINFINITY) {
                    //performance code
                    // unsigned int pre_val = atomicCAS(&(label[next]),(unsigned int)MYINFINITY,(unsigned int)(level+1));
                    // if(pre_val == MYINFINITY){
                    //     atomicAdd(&globalvisitedcount_d[0], (unsigned long long int)(vertexList[next+1] - vertexList[next]));
                    // }
                    // *changed = true;

                    // uint32_t mask = __activemask();

                    // int leader = __ffs(mask) - 1;
                    // unsigned long long int pos;
                    // if (laneIdx == leader) {
                    //     pos = atomicAdd(changed, (unsigned long long int)__popc(mask));

                    // }
                    label[next] = level + 1;
                    uint64_t mypos = atomicAdd(changed, 1);
                    //pos = __shfl_sync(mask, pos, leader);

                    //unsigned long long int mypos = (pos) + __popc(mask & ((1 << laneIdx) - 1));

                    next_frontier[mypos] = next;

                //}

            }
        }
    }


}

__global__ __launch_bounds__(128,16)
void kernel_frontier_coalesce(unsigned int *label, const unsigned int level, const uint64_t vertex_count,
                                const uint64_t *vertexList, const EdgeT *edgeList, const uint64_t curr_frontier_size, unsigned long long int *changed,
                                const uint32_t *curr_frontier, uint32_t *next_frontier) {
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if (warpIdx < curr_frontier_size) {
        const uint32_t nid = curr_frontier[warpIdx];
        const uint64_t start = vertexList[nid];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[nid+1];


        for (uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (i >= start) {
                const EdgeT next = edgeList[i];

                if(label[next] == MYINFINITY) {
                    uint32_t prev = atomicExch(label+next, level+1);
                    if (prev == MYINFINITY) {


                        //label[next] = level + 1;
                        uint32_t mask = __activemask();
                        uint32_t leader = __ffs(mask) - 1;
                        unsigned long long pos;
                        if (laneIdx == leader)
                            pos = atomicAdd(changed, __popc(mask));
                        pos = __shfl_sync(mask, pos, leader);
                        uint64_t mypos = pos + __popc(mask & ((1 << laneIdx) - 1));

                        //uint64_t mypos = atomicAdd(changed, 1);
                        next_frontier[mypos] = next;
                    }

                }
            }
        }
    }


}

__global__ __launch_bounds__(128,16)
void kernel_frontier_baseline_pc(unsigned int *label, const unsigned int level, const uint64_t vertex_count,
                                const uint64_t *vertexList, array_d_t<uint64_t>* da, const uint64_t curr_frontier_size, unsigned long long int *changed,
                                const uint32_t *curr_frontier, uint32_t *next_frontier) {
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    //const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint32_t laneIdx = tid &  ((1 << WARP_SHIFT) - 1);

    if (tid < curr_frontier_size) {
        const uint32_t nid = curr_frontier[tid];
        const uint64_t start = vertexList[nid];
        const uint64_t end = vertexList[nid+1];

        for (uint64_t i = start; i < end; i++) {
            const EdgeT next = da->seq_read(i);

            if(label[next] == MYINFINITY) {
                unsigned int prev = atomicExch(label+next, level+1);
                if (prev == MYINFINITY) {
                    //performance code
                    // unsigned int pre_val = atomicCAS(&(label[next]),(unsigned int)MYINFINITY,(unsigned int)(level+1));
                    // if(pre_val == MYINFINITY){
                    //     atomicAdd(&globalvisitedcount_d[0], (unsigned long long int)(vertexList[next+1] - vertexList[next]));
                    // }
                    // *changed = true;

                    uint32_t mask = __activemask();

                    int leader = __ffs(mask) - 1;
                    unsigned long long int pos;
                    if (laneIdx == leader) {
                        pos = atomicAdd(changed, (unsigned long long int)__popc(mask));

                    }
                    label[next] = level + 1;
                    //uint64_t mypos = atomicAdd(changed, 1);
                    pos = __shfl_sync(mask, pos, leader);

                    unsigned long long int mypos = (pos) + __popc(mask & ((1 << laneIdx) - 1));

                    next_frontier[mypos] = next;

                }

            }
        }
    }


}

__global__ __launch_bounds__(128,16)
void kernel_frontier_coalesce_pc(unsigned int *label, const unsigned int level, const uint64_t vertex_count,
                                const uint64_t *vertexList, array_d_t<uint64_t>* da, const uint64_t curr_frontier_size, unsigned long long int *changed,
                                const uint32_t *curr_frontier, uint32_t *next_frontier) {
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if (warpIdx < curr_frontier_size) {
        const uint32_t nid = curr_frontier[warpIdx];
        const uint64_t start = vertexList[nid];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[nid+1];


        for (uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (i >= start) {
                const EdgeT next = da->seq_read(i);

                if(label[next] == MYINFINITY) {
                    uint32_t prev = atomicExch(label+next, level+1);
                    if (prev == MYINFINITY) {


                        //label[next] = level + 1;
                        uint32_t mask = __activemask();
                        uint32_t leader = __ffs(mask) - 1;
                        unsigned long long pos;
                        if (laneIdx == leader)
                            pos = atomicAdd(changed, __popc(mask));
                        pos = __shfl_sync(mask, pos, leader);
                        uint64_t mypos = pos + __popc(mask & ((1 << laneIdx) - 1));

                        //uint64_t mypos = atomicAdd(changed, 1);
                        next_frontier[mypos] = next;
                    }

                }
            }
        }
    }
}


__global__ __launch_bounds__(128,16)
void kernel_frontier_coalesce_ptr_pc(unsigned int *label, const unsigned int level, const uint64_t vertex_count,
                                const uint64_t *vertexList, array_d_t<uint64_t>* da, const uint64_t curr_frontier_size, unsigned long long int *changed,
                                const uint32_t *curr_frontier, uint32_t *next_frontier) {
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    if (warpIdx < curr_frontier_size) {
        bam_ptr<uint64_t> ptr(da);
        const uint32_t nid = curr_frontier[warpIdx];
        const uint64_t start = vertexList[nid];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[nid+1];


        for (uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (i >= start) {
                //const EdgeT next = da->seq_read(i);
                const EdgeT next = ptr[i];

                if(label[next] == MYINFINITY) {
                    uint32_t prev = atomicExch(label+next, level+1);
                    if (prev == MYINFINITY) {
                        //label[next] = level + 1;
                        uint32_t mask = __activemask();
                        uint32_t leader = __ffs(mask) - 1;
                        unsigned long long pos;
                        if (laneIdx == leader)
                            pos = atomicAdd(changed, __popc(mask));
                        pos = __shfl_sync(mask, pos, leader);
                        uint64_t mypos = pos + __popc(mask & ((1 << laneIdx) - 1));

                        //uint64_t mypos = atomicAdd(changed, 1);
                        next_frontier[mypos] = next;
                    }

                }
            }
        }
    }
}



/* =====================================================================
 * 레벨 동기 BFS 커널들 (BASELINE, COALESCE, COALESCE_CHUNK 및 *_PC/*_PTR_PC 변형)
 * =====================================================================
 *
 * [공통 로직]
 * 매 반복마다 전체 정점(또는 할당된 정점)을 검사하여, label[v] == level인 정점의
 * 인접 리스트를 탐색한다. 미방문 이웃(label[next] == MYINFINITY)을 발견하면
 * label[next] = level + 1로 설정하고 changed 플래그를 true로 만든다.
 *
 * [BASELINE vs COALESCE]
 * - BASELINE: tid가 정점 ID. 스레드 1개가 vertexList[tid]~vertexList[tid+1] 전체를 순회.
 *   메모리 접근이 비연속적이라 coalescing이 안 됨.
 * - COALESCE: warpIdx가 정점 ID. 워프의 32스레드가 shift_start+laneIdx부터 WARP_SIZE 간격으로 접근.
 *   shift_start = start & 0xFFFFFFFFFFFFFFF0 으로 16-원소(128바이트) 정렬하여 메모리 coalescing 달성.
 *
 * [*_PC 변형: BaM page cache를 통한 SSD 직접 I/O]
 * edgeList를 GPU 메모리 배열 대신 array_d_t<uint64_t>* da로 전달받는다.
 * da->seq_read(i)를 호출하면 BaM 런타임이:
 *   1) 해당 인덱스가 속하는 SSD 페이지가 GPU page cache에 있는지 확인
 *   2) 없으면 NVMe SSD로 PCIe P2P DMA 요청을 발행하여 페이지를 GPU 메모리로 fetch
 *   3) 캐시된 페이지에서 값을 읽어 반환
 *
 * [*_PTR_PC 변형: bam_ptr을 통한 SSD 직접 I/O]
 * bam_ptr<uint64_t> ptr(da)로 스마트 포인터를 생성하고 ptr[i]로 접근한다.
 * seq_read()와 기능은 동일하지만, 연속 접근 시 페이지 경계를 추적하여 불필요한 lookup을 줄인다.
 * =====================================================================
 */

__global__ void kernel_baseline(uint32_t *label, const uint32_t level, const uint64_t vertex_count,
                        const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, unsigned long long int *globalvisitedcount_d, unsigned long long int *vertexVisitCount_d
    ) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;

    // if(tid==0)
    //         printf("Warning: The code is not optimal because of additional counters added for profiling\n");

    if(tid < vertex_count && label[tid] == level) {
        const uint64_t start = vertexList[tid];
        const uint64_t end = vertexList[tid+1];

        for(uint64_t i = start; i < end; i++) {
            const EdgeT next = edgeList[i];
            //performance code
            // atomicAdd(&vertexVisitCount_d[next], 1);

            if(label[next] == MYINFINITY) {
                //performance code
                // unsigned int pre_val = atomicCAS(&(label[next]),(unsigned int)MYINFINITY,(unsigned int)(level+1));
                // if(pre_val == MYINFINITY){
                //     atomicAdd(&globalvisitedcount_d[0], (unsigned long long int)(vertexList[next+1] - vertexList[next]));
                // }
                // *changed = true;

                label[next] = level + 1;
                *changed = true;
            }
        }
    }
}



__global__ __launch_bounds__(128,16)
void kernel_baseline_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count,
                        const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, unsigned long long int *globalvisitedcount_d, unsigned long long int *vertexVisitCount_d
    ) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;

//    array_d_t<uint64_t> d_array = *da;
    // if(tid==0)
    //         printf("Warning: The code is not optimal because of additional counters added for profiling\n");

    if(tid < vertex_count && label[tid] == level) {
        const uint64_t start = vertexList[tid];
        const uint64_t end = vertexList[tid+1];

        for(uint64_t i = start; i < end; i++) {
            //EdgeT next = da->seq_read(i);
            EdgeT next = da->seq_read(i);
//                printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);
            //performance code
            // atomicAdd(&vertexVisitCount_d[next], 1);

            if(label[next] == MYINFINITY) {
                //performance code
                // unsigned int pre_val = atomicCAS(&(label[next]),(unsigned int)MYINFINITY,(unsigned int)(level+1));
                // if(pre_val == MYINFINITY){
                //     atomicAdd(&globalvisitedcount_d[0], (unsigned long long int)(vertexList[next+1] - vertexList[next]));
                // }
                // *changed = true;

                label[next] = level + 1;
                *changed = true;
            }
        }
    }
}





__global__ void kernel_coalesce(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    
    if(warpIdx < vertex_count && label[warpIdx] == level) {
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
//        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

            if (i >= start) {
                const EdgeT next = edgeList[i];
  //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {

                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}




__global__ __launch_bounds__(128,16)
void kernel_coalesce_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    //array_d_t<uint64_t> d_array = *da;
    if(warpIdx < vertex_count && label[warpIdx] == level) {
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (i >= start) {
                //const EdgeT next = edgeList[i];
                //EdgeT next = da->seq_read(i);
                EdgeT next = da->seq_read(i);
//                printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {
                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu\n", tid, (unsigned long long)level, (unsigned long long)next);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}

__global__ __launch_bounds__(128,16)
void kernel_coalesce_ptr_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    //array_d_t<uint64_t> d_array = *da;
    if(warpIdx < vertex_count && label[warpIdx] == level) {
        bam_ptr<uint64_t> ptr(da);
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (i >= start) {
                //const EdgeT next = edgeList[i];
                //EdgeT next = da->seq_read(i);
                EdgeT next = ptr[i];
//                printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {
                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu\n", tid, (unsigned long long)level, (unsigned long long)next);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}



/* kernel_coalesce_coarse: 스레드 coarsening 최적화.
 * 기본 COALESCE에서 워프 1개가 정점 1개를 처리하지만, coarsening에서는
 * 워프 1개가 coarse개 정점을 순차 처리한다 (cwarpIdx = warpIdx*coarse + j).
 * 이렇게 하면 커널 launch 시 필요한 워프 수가 coarse배 줄어들고,
 * 같은 SSD 페이지에 속하는 연속 정점을 하나의 워프가 처리하여 캐시 히트율이 올라간다.
 */
__global__ __launch_bounds__(128,16)
void kernel_coalesce_coarse(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t coarse) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    
    for(uint64_t j = 0; j < coarse; j++){
        uint64_t cwarpIdx = warpIdx * coarse + j;
        if(cwarpIdx < vertex_count && label[cwarpIdx] == level) {
            const uint64_t start = vertexList[cwarpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[cwarpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
    //        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

                if (i >= start) {
                    const EdgeT next = edgeList[i];
    //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                    if(label[next] == MYINFINITY) {

                    //    if(level ==0)
                    //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                        label[next] = level + 1;
                        *changed = true;
                    }
                }
            }
        }
    }
}




__global__ __launch_bounds__(128,16)
void kernel_coalesce_coarse_ptr_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t coarse) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    bam_ptr<uint64_t> ptr(da);

    for(uint64_t j = 0; j < coarse; j++){
        uint64_t cwarpIdx = warpIdx * coarse + j;
        if(cwarpIdx < vertex_count && label[cwarpIdx] == level) {
            const uint64_t start = vertexList[cwarpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[cwarpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
    //        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

                if (i >= start) {
                    // const EdgeT next = edgeList[i];
                    EdgeT next = ptr[i];
    //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);
                    if(label[next] == MYINFINITY) {
                    //    if(level ==0)
                    //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                        label[next] = level + 1;
                        *changed = true;
                    }
                }
            }
        }
    }
}






/* kernel_coalesce_hash: stride 해싱으로 워프-정점 매핑을 분산한다.
 * 기본 COALESCE에서는 워프 0이 정점 0, 워프 1이 정점 1, ... 을 담당하므로
 * 인접한 워프가 인접한 SSD 페이지를 동시에 접근하여 핫스팟이 발생할 수 있다.
 * 해싱은 warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*STRIDE) 공식으로
 * 워프가 접근하는 정점 ID를 STRIDE 간격으로 분산시켜 서로 다른 SSD 페이지에 접근하게 한다.
 * 이는 page cache 경합을 줄이고 NVMe 큐의 병렬성을 극대화한다.
 */
__global__ __launch_bounds__(128,16)
void kernel_coalesce_hash(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t stride) {
    const uint64_t oldtid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = oldtid >> WARP_SHIFT;
    const uint64_t laneIdx = oldtid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = stride; 
    
    const uint64_t nep = (vertex_count+(STRIDE))/(STRIDE); 
    uint64_t warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*(STRIDE));
    
    if(warpIdx < vertex_count && label[warpIdx] == level) {
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
//        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

            if (i >= start) {
                const EdgeT next = edgeList[i];
  //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {

                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}



__global__ __launch_bounds__(128,16)
void kernel_coalesce_hash_ptr_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t stride) {
    const uint64_t oldtid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = oldtid >> WARP_SHIFT;
    const uint64_t laneIdx = oldtid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = stride; 
    const uint64_t nep = (vertex_count+(STRIDE))/(STRIDE); 
    uint64_t warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*(STRIDE));

    //array_d_t<uint64_t> d_array = *da;
    if(warpIdx < vertex_count && label[warpIdx] == level) {
        bam_ptr<uint64_t> ptr(da);
        const uint64_t start = vertexList[warpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[warpIdx+1];

        for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
            if (i >= start) {
                //const EdgeT next = edgeList[i];
                //EdgeT next = da->seq_read(i);
                EdgeT next = ptr[i];
//                printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {
                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu\n", tid, (unsigned long long)level, (unsigned long long)next);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}




__global__ __launch_bounds__(128,16)
void kernel_coalesce_hash_coarse(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed,uint64_t coarse, uint64_t stride) {
    const uint64_t oldtid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = oldtid >> WARP_SHIFT;
    const uint64_t laneIdx = oldtid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = stride; 
    
    const uint64_t nep = (vertex_count+(STRIDE*coarse))/(STRIDE*coarse); 
    uint64_t cwarpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*(STRIDE));
    
    for(uint64_t j=0; j<coarse; j++){
        uint64_t warpIdx = cwarpIdx*coarse+j;
        if(warpIdx < vertex_count && label[warpIdx] == level) {
            const uint64_t start = vertexList[warpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[warpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
    //        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

                if (i >= start) {
                    const EdgeT next = edgeList[i];
    //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                    if(label[next] == MYINFINITY) {

                    //    if(level ==0)
                    //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                        label[next] = level + 1;
                        *changed = true;
                    }
                }
            }
        } 
    }
}




__global__ __launch_bounds__(128,16)
void kernel_coalesce_hash_coarse_ptr_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t coarse, uint64_t stride) {
    const uint64_t oldtid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = oldtid >> WARP_SHIFT;
    const uint64_t laneIdx = oldtid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = stride; 
    bam_ptr<uint64_t> ptr(da);
    
    const uint64_t nep = (vertex_count+(STRIDE*coarse))/(STRIDE*coarse); 
    uint64_t cwarpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*(STRIDE));
    
    for(uint64_t j=0; j<coarse; j++){
        uint64_t warpIdx = cwarpIdx*coarse+j;
        if(warpIdx < vertex_count && label[warpIdx] == level) {
            const uint64_t start = vertexList[warpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[warpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
    //        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

                if (i >= start) {
                    // const EdgeT next = edgeList[i];
                    EdgeT next = ptr[i];
                    //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                    if(label[next] == MYINFINITY) {

                    //    if(level ==0)
                    //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                        label[next] = level + 1;
                        *changed = true;
                    }
                }
            }
        } 
    }
}
 



/* kernel_coalesce_hash_half: 하프 워프(16스레드) 단위로 정점을 처리하는 변형.
 * 워프(32스레드) 대신 16스레드가 하나의 정점을 담당한다. 차수(degree)가 작은 그래프에서
 * 워프의 절반이 idle 상태가 되는 것을 방지하여 GPU 활용도를 높인다.
 */
__global__ __launch_bounds__(128,16)
void kernel_coalesce_hash_half(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t stride) {
    const uint64_t oldtid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldhalfwarpIdx = oldtid >> 4;
    const uint64_t halflaneIdx = oldtid & ((1 << 4) - 1);
    uint64_t STRIDE = stride; 
    
    const uint64_t nep = (vertex_count+(STRIDE))/(STRIDE); 
    uint64_t halfwarpIdx = (oldhalfwarpIdx/nep) + ((oldhalfwarpIdx % nep)*(STRIDE));
    
    if(halfwarpIdx < vertex_count && label[halfwarpIdx] == level) {
        const uint64_t start = vertexList[halfwarpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[halfwarpIdx+1];

        for(uint64_t i = shift_start + halflaneIdx; i < end; i += 16) {
//        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

            if (i >= start) {
                const EdgeT next = edgeList[i];
  //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {

                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}



__global__ __launch_bounds__(128,16)
void kernel_coalesce_hash_half_ptr_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t stride) {
    const uint64_t oldtid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldhalfwarpIdx = oldtid >> 4;
    const uint64_t halflaneIdx = oldtid & ((1 << 4) - 1);
    uint64_t STRIDE = stride; 
    
    const uint64_t nep = (vertex_count+(STRIDE))/(STRIDE); 
    uint64_t halfwarpIdx = (oldhalfwarpIdx/nep) + ((oldhalfwarpIdx % nep)*(STRIDE));
    
    if(halfwarpIdx < vertex_count && label[halfwarpIdx] == level) {
        const uint64_t start = vertexList[halfwarpIdx];
        const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
        const uint64_t end = vertexList[halfwarpIdx+1];
        bam_ptr<uint64_t> ptr(da);

        for(uint64_t i = shift_start + halflaneIdx; i < end; i += 16) {
//        printf("Inside kernel %llu %llu %llu\n", (unsigned long long) i, (unsigned long long)start, (unsigned long long) (end-start));

            if (i >= start) {
                EdgeT next = ptr[i];
  //printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                if(label[next] == MYINFINITY) {

                //    if(level ==0)
                //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                    label[next] = level + 1;
                    *changed = true;
                }
            }
        }
    }
}




/* kernel_coalesce_chunk: CHUNK_SIZE(8)개 정점을 워프 1개가 묶어서 처리.
 * COALESCE와 유사하지만 워프가 한 정점이 아닌 8개 연속 정점의 간선을 처리한다.
 * 차수가 작은 정점이 많은 그래프에서 워프 활용도를 높이는 전략이다.
 */
__global__ void kernel_coalesce_chunk(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed) {
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
        if(label[i] == level) {
            const uint64_t start = vertexList[i];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[i+1];

            for(uint64_t j = shift_start + laneIdx; j < end; j += WARP_SIZE) {
                if (j >= start) {
                    const EdgeT next = edgeList[j];
          
                    if(label[next] == MYINFINITY) {
                        label[next] = level + 1;
                        *changed = true;
                    }
                }
            }
        }
    }
}


__global__  __launch_bounds__(1024,2)
void kernel_coalesce_chunk_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    const uint64_t chunkIdx = warpIdx * CHUNK_SIZE;
    uint64_t chunk_size = CHUNK_SIZE;
    bam_ptr<uint64_t> ptr(da);

    //array_d_t<uint64_t> d_array = *da;
    if((chunkIdx + CHUNK_SIZE) > vertex_count) {
        if ( vertex_count > chunkIdx )
            chunk_size = vertex_count - chunkIdx;
        else
            return;
    }

    for(uint32_t i = chunkIdx; i < chunk_size + chunkIdx; i++) {
        if(label[i] == level) {
            const uint64_t start = vertexList[i];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[i+1];

            for(uint64_t j = shift_start + laneIdx; j < end; j += WARP_SIZE) {
                if (j >= start) {
                    // const EdgeT next = edgeList[j];
                    //EdgeT next = da->seq_read(j);
                    EdgeT next = ptr[j];
                    // printf("tid: %llu, idx: %llu next: %llu\n", (unsigned long long) tid, (unsigned long long) i, (unsigned long long) next);

                    if(label[next] == MYINFINITY) {
                        label[next] = level + 1;
                        *changed = true;
                    }
                }
            }
        }
    }
}



/* kernel_optimized: 캐시라인(타일) 인식 BFS 커널.
 * 다른 커널들이 "정점 단위"로 워프를 할당하는 것과 달리, 이 커널은 "캐시라인 단위"로 워프를 할당한다.
 * warpIdx가 캐시라인(타일) ID에 매핑되며, 해당 캐시라인이 커버하는 edgeList 범위를 처리한다.
 * first_vertex[warpIdx]로 해당 캐시라인의 첫 번째 정점을 알아내고, 캐시라인 경계 내에서
 * 여러 정점의 간선을 연속 처리한다. SSD에서 읽어온 1개 페이지 데이터를 최대한 활용하므로
 * I/O 효율이 극대화된다. stride 해싱도 적용하여 캐시라인 접근 순서를 분산한다.
 */
//TODO: change launch parameters. The number of warps to be launched equal to the number of cachelines. Each warp works on a cacheline.
//TODO: make it templated.
__global__ __launch_bounds__(128,16)
void kernel_optimized(uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t* first_vertex, uint64_t num_elems_in_cl, uint64_t n_pages, uint64_t stride){
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t cwarpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);


    uint64_t nep = (n_pages+stride)/stride; 
    uint64_t warpIdx = (cwarpIdx/nep) + ((cwarpIdx % nep)*stride);

    if(warpIdx < n_pages){
        const uint64_t clstart = warpIdx*num_elems_in_cl; 
        const uint64_t clend   = (warpIdx+1)*num_elems_in_cl; 
        //start vertex
        uint64_t cur_vertexid = first_vertex[warpIdx]; 

        //for the first vertex iterator start is the clstart point while the end is either the clend or the cur_vertexid neighborlist end
        uint64_t start = clstart; 
        uint64_t end   = vertexList[cur_vertexid+1];
        bool stop      = false;

        // uint64_t itr = 0;
        if((cur_vertexid < vertex_count) && (warpIdx < n_pages) ) {
           //printf("warpidx: %llu laneidx:%llu clstart: %llu clend: %llu start: %llu end: %llu cur_vertexid : %llu\n", warpIdx, laneIdx, clstart, clend, start, end, cur_vertexid);
            while(!stop){
            //if (cur_vertexid < vertex_count && curr_visit[cur_vertexid] == true) {
                //check if the fetched end of cur_vertexid is beyond clend. If yes, then trim end to clend and this is the last while loop iteration.
                if(end >= clend){
                    end  = clend;
                    stop = true;
                    //           printf("called end >=clend and end is:%llu\n", end);
                }

                if(label[cur_vertexid] == level){
                   for(uint64_t i = start + laneIdx; i < end; i += WARP_SIZE){
                    //            uint64_t val = (uint64_t)atomicAdd(&(totalcount_d[0]), 1);
                    //            printf("itr:%llu i:%llu laneIdx: %llu starts:%llu end:%llu cur_vertexid: %llu pre_atomicval:%llu\n",itr, i, laneIdx,start,  end, cur_vertexid, val);
                       EdgeT next = edgeList[i];
                       if(label[next] == MYINFINITY) {
                    //    if(level ==0)
                    //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                            label[next] = level + 1;
                            *changed = true;
                        }
                   }
                }
                // itr++; 
                
                //this implies there are more vertices to compute in the cacheline. So repeat the loop.
                if(end < clend){
                    cur_vertexid = cur_vertexid + 1; //go to next elem in the vertexlist
                    for (;(cur_vertexid < vertex_count) && (end == vertexList[cur_vertexid+1]);cur_vertexid++) {}
                    if(cur_vertexid < vertex_count){
                        start        = vertexList[cur_vertexid]; 
                        end          = vertexList[cur_vertexid+1]; 
                    }
                    else {
                        stop = true; 
                    }
                }
            }
        }
    }
}



//TODO: change launch parameters. The number of warps to be launched equal to the number of cachelines. Each warp works on a cacheline. 
//TODO: make it templated. 
__global__ __launch_bounds__(128,16)
void kernel_optimized_ptr_pc(array_d_t<uint64_t>* da, uint32_t *label, const uint32_t level, const uint64_t vertex_count, const uint64_t *vertexList, const EdgeT *edgeList, uint64_t *changed, uint64_t* first_vertex, uint64_t num_elems_in_cl, uint64_t n_pages, uint64_t stride, uint64_t iter){
    //const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;

    const uint64_t cwarpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    
    uint64_t nep = (n_pages+stride)/stride; 
    uint64_t warpIdx = (cwarpIdx/nep) + ((cwarpIdx % nep)*stride);

    if(warpIdx < n_pages){
         const uint64_t clstart = warpIdx*num_elems_in_cl; 
         const uint64_t clend   = (warpIdx+1)*num_elems_in_cl; 
         //start vertex
         uint64_t cur_vertexid = first_vertex[warpIdx]; 

         //for the first vertex iterator start is the clstart point while the end is either the clend or the cur_vertexid neighborlist end
         uint64_t start = clstart; 
         uint64_t end   = vertexList[cur_vertexid+1];
         bool stop      = false;
         bam_ptr<uint64_t> ptr(da);

         //uint64_t itr = 0;
         if((cur_vertexid < vertex_count) && (warpIdx < n_pages) ) {
            //printf("warpidx: %llu laneidx:%llu clstart: %llu clend: %llu start: %llu end: %llu cur_vertexid : %llu\n", warpIdx, laneIdx, clstart, clend, start, end, cur_vertexid);
             while(!stop){
             //if (cur_vertexid < vertex_count && curr_visit[cur_vertexid] == true) {
                 //check if the fetched end of cur_vertexid is beyond clend. If yes, then trim end to clend and this is the last while loop iteration.
                 if(end >= clend){
                     end  = clend;
                     stop = true;
                     //           printf("called end >=clend and end is:%llu\n", end);
                 }

                 if(label[cur_vertexid] == level){
                    for(uint64_t i = start + laneIdx; i < end; i += WARP_SIZE){
                     //            uint64_t val = (uint64_t)atomicAdd(&(totalcount_d[0]), 1);
                            //printf("itr:%llu i:%llu laneIdx: %llu starts:%llu end:%llu cur_vertexid: %llu pre_atomicval:%llu\n",itr, i, laneIdx,start,  end, cur_vertexid, 0);
                        EdgeT next = ptr[i];
                        //EdgeT next = da->seq_read(i); //[i];
                        if(label[next] == MYINFINITY) {
                     //    if(level ==0)
                     //            printf("tid:%llu, level:%llu, next: %llu start:%llu end:%llu\n", tid, (unsigned long long)level, (unsigned long long)next, (unsigned long long)start, (unsigned long long)end);
                             label[next] = level + 1;
                             *changed = true;
                         }
                    }
                 }
                 //itr++; 
                 
                 //this implies there are more vertices to compute in the cacheline. So repeat the loop.
                 if(end < clend){
                     cur_vertexid = cur_vertexid + 1; //go to next elem in the vertexlist
                     for (;(cur_vertexid < vertex_count) && (end == vertexList[cur_vertexid+1]);cur_vertexid++) {}
                     if(cur_vertexid < vertex_count){
                         start        = vertexList[cur_vertexid]; 
                         end          = vertexList[cur_vertexid+1]; 
                     }
                     else {
                         stop = true; 
                     }
                 }
             }
         }
    }
}



/* throttle_memory: GPU 메모리를 의도적으로 소비하여 가용 메모리를 줄이는 헬퍼 커널.
 * UVM_READONLY 모드에서 GPU 메모리가 16GB 이상이면, 초과분을 할당하여
 * 실제 SSD 접근이 발생하도록 GPU 메모리 압박을 시뮬레이션한다. */
__global__ void throttle_memory(uint32_t *pad) {
    pad[1] = pad[0];
}





/* =============================================================================
 * main() 함수: BFS 벤치마크 진입점
 * =============================================================================
 * 전체 흐름:
 * (1) 커맨드라인 인자 파싱 -> Settings 구조체
 * (2) 그래프 파일(.col, .dst) 읽기 -> vertexList_h, edgeList_h (호스트)
 * (3) 메모리 모드에 따라 GPU 메모리 할당 및 데이터 전송
 *     - GPUMEM: cudaMalloc + cudaMemcpy
 *     - UVM_READONLY: cudaMallocManaged + ReadMostly
 *     - UVM_DIRECT: cudaMallocManaged + AccessedBy (O_DIRECT로 읽기)
 *     - BAFS_DIRECT: Controller 생성 -> page_cache_t -> range_t -> array_t
 * (4) BFS 실행 루프: src에서 시작, do-while로 레벨별 커널 호출
 *     - changed_d 플래그로 수렴 검사 (새 정점 발견 여부)
 *     - 반복 후 cudaEventElapsedTime으로 실행 시간 측정
 * (5) 결과 출력, 메모리 해제
 * =============================================================================
 */
int main(int argc, char *argv[]) {
    using namespace std::chrono;

    /* (1) 커맨드라인 인자 파싱 */
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

    uint64_t changed_h, *changed_d;// no_src = false;
    int num_run = 0;// arg_num = 0;
    int total_run = 1;// arg_num = 0;
    impl_type type;
    mem_type mem;
    uint32_t *pad;
    uint32_t *label_d, level, zero, iter;
    uint64_t *vertexList_h, *vertexList_d;
    EdgeT *edgeList_h, *edgeList_d;
    uint64_t vertex_count, edge_count, vertex_size, edge_size;
    uint64_t typeT, src;
    uint64_t numblocks, numthreads;
    size_t freebyte, totalbyte;

    float milliseconds;
    double avg_milliseconds;

    uint64_t pc_page_size;  // BaM page cache의 페이지 크기
    uint64_t pc_pages;      // BaM page cache의 총 페이지 수

    try{
         /* (2) 설정값 준비 및 그래프 파일 읽기 */
         filename = std::string(settings.input); 

         if(settings.src == 0) {
                 total_run = settings.repeat; 
                 src = 0;
         }
         else {
                 total_run = 2; 
                 src = settings.src; 
         }

         type = (impl_type) settings.type; 
         mem = (mem_type) settings.memalloc; 

         pc_page_size = settings.pageSize; 

         uint64_t tile_size = 4096;
         if(settings.tsize == 0)
             tile_size = pc_page_size; 
         else
             tile_size = settings.tsize; 

         pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size);

         numthreads = settings.numThreads;
         
         cuda_err_chk(cudaSetDevice(settings.cudaDevice));
         
         cudaEvent_t start, end;
         cuda_err_chk(cudaEventCreate(&start));
         cuda_err_chk(cudaEventCreate(&end));

         /* CSR 그래프 파일: .col = 정점 오프셋 배열(vertexList), .dst = 간선 배열(edgeList) */
         vertex_file = filename + ".col";
         edge_file = filename + ".dst";

         std::cout << filename << std::endl;
         fprintf(stderr, "File %s\n", filename.c_str());
         // Read files
         file.open(vertex_file.c_str(), std::ios::in | std::ios::binary);
         if (!file.is_open()) {
             fprintf(stderr, "Vertex file open failed\n");
             exit(1);
         };

         file.read((char*)(&vertex_count), 8);
         file.read((char*)(&typeT), 8);

        /*        
        vertex_count = 19;
        edge_count = 41; 
        */
         vertex_count--;

         printf("Vertex: %llu, ", vertex_count);
         vertex_size = (vertex_count+1) * sizeof(uint64_t);

         vertexList_h = (uint64_t*)malloc(vertex_size);

         file.read((char*)vertexList_h, vertex_size);
         file.close();

        /*
        uint64_t  edgesample[] ={1,2,0,2,3,4,0,1,3,4,1,2,4,1,2,3,4,6,0,0,1,2,3,4,5,6,7,8,9,10,12,13,14,15,16,17,18,0,1,2,3}; 
        uint64_t  vertsample[] ={0,2,6,10,13,13,13,16,18,19,19,19,19,19,19,19,19,19,37,41} ;

        for(int vi = 0; vi < vertex_count+1; vi++)
            vertexList_h[vi] = vertsample[vi];
        */

         file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
         if (!file.is_open()) {
             fprintf(stderr, "Edge file open failed\n");
             exit(1);
         };

         file.read((char*)(&edge_count), 8);
         file.read((char*)(&typeT), 8);

         printf("Edge: %llu\n", edge_count);
         fflush(stdout);
         edge_size = edge_count * sizeof(EdgeT); //4096 padding for weights and edges. 
         edge_size = edge_size + (4096 - (edge_size & 0xFFFULL));

         edgeList_h = NULL;
         edgeList_d = NULL;

         // Allocate memory for GPU
         cuda_err_chk(cudaMalloc((void**)&vertexList_d, vertex_size));
         cuda_err_chk(cudaMalloc((void**)&label_d, vertex_count * sizeof(uint32_t)));
         cuda_err_chk(cudaMalloc((void**)&changed_d, sizeof(uint64_t)));
     
         std::vector<unsigned long long int> vertexVisitCount_h;
         unsigned long long int* vertexVisitCount_d;
         unsigned long long int globalvisitedcount_h;
         unsigned long long int* globalvisitedcount_d;
     
         vertexVisitCount_h.resize(vertex_count);
         cuda_err_chk(cudaMalloc((void**)&globalvisitedcount_d, sizeof(unsigned long long int)));
         cuda_err_chk(cudaMemset(globalvisitedcount_d, 0, sizeof(unsigned long long int)));
         cuda_err_chk(cudaMalloc((void**)&vertexVisitCount_d, vertex_count*sizeof(unsigned long long int)));
         cuda_err_chk(cudaMemset(vertexVisitCount_d, 0, vertex_count*sizeof(unsigned long long int)));

         /* (3) 메모리 모드별 edgeList 준비 */
         switch (mem) {
             case GPUMEM:
                 /* GPUMEM: 호스트에서 파일을 읽어 GPU 메모리로 복사 */
                 edgeList_h = (EdgeT*)malloc(edge_size);
                 file.read((char*)edgeList_h, edge_size);
                /*
                for(int ei = 0; ei < edge_count; ei++)
                    edgeList_h[ei] = edgesample[ei];
                */
                 cuda_err_chk(cudaMalloc((void**)&edgeList_d, edge_size));
                 file.close();
                 break;
             case UVM_READONLY:
                 cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size));
                 file.read((char*)edgeList_d, edge_size);
                 cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
     
                 cuda_err_chk(cudaMemGetInfo(&freebyte, &totalbyte));
                 if (totalbyte < 16*1024*1024*1024ULL)
                     printf("total memory sizeo of current GPU is %llu byte, no need to throttle\n", totalbyte);
                 else {
                     printf("total memory sizeo of current GPU is %llu byte, throttling %llu byte.\n", totalbyte, totalbyte - 16*1024*1024*1024ULL);
                     cuda_err_chk(cudaMalloc((void**)&pad, totalbyte - 16*1024*1024*1024ULL));
                     throttle_memory<<<1,1>>>(pad);
                 }
                 file.close();
                 break;
             case UVM_DIRECT:
             {
             /*    cuda_err_chk(cudaMallocManaged((void**)&edgeList_d, edge_size));
                 // printf("Address is %p   %p\n", edgeList_d, &edgeList_d[0]); 
                 high_resolution_clock::time_point ft1 = high_resolution_clock::now();
                 file.read((char*)edgeList_d, edge_size);
                 file.close();
                 high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                 duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                 std::cout<< "edge file read time: "<< time_span.count() <<std::endl;
                 cuda_err_chk(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                 break;
             */

                 file.close();
                 for (uint64_t i = 0; i < vertex_count + 1; i++) {
                     vertexList_h[i] += 2;
                 }   
                 int fd = open(edge_file.c_str(), O_RDONLY | O_DIRECT);
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
                 std::cout<< "Edge file read time: "<< time_span.count() <<std::endl;
                       
                 file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
                 if (!file.is_open()) {
                     printf("edge file open failed\n");
                     exit(1);
                 }   
                 break;
             }
             case BAFS_DIRECT: 
                 //cuda_err_chk(cudaMemGetInfo(&freebyte, &totalbyte));
                 //if (totalbyte < 16*1024*1024*1024ULL)
                 //    printf("total memory sizeo of current GPU is %llu byte, no need to throttle\n", totalbyte);
                 //else {
                 //    printf("total memory sizeo of current GPU is %llu byte, throttling %llu byte.\n", totalbyte, totalbyte - 16*1024*1024*1024ULL);
                 //    cuda_err_chk(cudaMalloc((void**)&pad, totalbyte - 16*1024*1024*1024ULL));
                 //    throttle_memory<<<1,1>>>(pad);
                 //}
                 break;
              default: 
                 printf("ERROR: Invalid Mem type specified\n");
                 break;
         }
     
     
         printf("Allocation finished\n");
         fflush(stdout);
         uint64_t n_pages = ceil(((float)edge_size)/pc_page_size);
         uint64_t n_tiles = ceil(((float)edge_size)/tile_size);
         // Initialize values
         cuda_err_chk(cudaMemcpy(vertexList_d, vertexList_h, vertex_size, cudaMemcpyHostToDevice));

         if (mem == GPUMEM){
             cuda_err_chk(cudaMemcpy(edgeList_d, edgeList_h, edge_size, cudaMemcpyHostToDevice));
         }
    

         switch (type) {
             case BASELINE:
             case BASELINE_PC:
                 numblocks = ((vertex_count + numthreads) / numthreads);
                 break;
             case COALESCE:
             case COALESCE_HASH:
             case COALESCE_PC:
             case COALESCE_PTR_PC:
             case COALESCE_HASH_PTR_PC:
                 numblocks = ((vertex_count * WARP_SIZE + numthreads) / numthreads);
                 break;
             case COALESCE_CHUNK:
             case COALESCE_CHUNK_PC:
                 numblocks = ((vertex_count * (WARP_SIZE / CHUNK_SIZE) + numthreads) / numthreads);
                 break;
             case COALESCE_COARSE:
             case COALESCE_HASH_COARSE:
             case COALESCE_COARSE_PTR_PC:
             case COALESCE_HASH_COARSE_PTR_PC:
                   numblocks = ((vertex_count * (WARP_SIZE / settings.coarse) + numthreads) / numthreads);
                   break;
             case COALESCE_HASH_HALF:
             case COALESCE_HASH_HALF_PTR_PC:
                   numblocks = ((vertex_count * (WARP_SIZE / 2) + numthreads) / numthreads);
                   break;

             case FRONTIER_BASELINE:
             case FRONTIER_COALESCE:
             case FRONTIER_BASELINE_PC:
             case FRONTIER_COALESCE_PC:
             case FRONTIER_COALESCE_PTR_PC:
                 break;
            
             case OPTIMIZED:
             case OPTIMIZED_PC:
                  numblocks = (n_tiles*WARP_SIZE+numthreads)/numthreads;
                  break; 
             default:
                 fprintf(stderr, "Invalid type\n");
                 exit(1);
                 break;
         }
    
         //TODO : FIX THIS. 
         dim3 blockDim(BLOCK_NUM, (numblocks+BLOCK_NUM)/BLOCK_NUM);

         avg_milliseconds = 0.0f;


         if((type==BASELINE_PC)||(type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)||(type==FRONTIER_BASELINE_PC)||(type == FRONTIER_COALESCE_PC) || (type== FRONTIER_COALESCE_PTR_PC) ||(type == COALESCE_COARSE_PTR_PC) ||(type == COALESCE_HASH_PTR_PC) || (type == COALESCE_HASH_COARSE_PTR_PC) || (type == COALESCE_HASH_HALF_PTR_PC ) || (type == OPTIMIZED_PC)){
                printf("page size: %d, pc_entries: %llu tile_size:%llu\n", pc_page_size, pc_pages, tile_size);
                fflush(stdout);
         }


         /* BAFS_DIRECT 모드: NVMe 컨트롤러 초기화
          * Controller 객체는 /dev/libnvm* 캐릭터 디바이스를 열어 NVMe 컨트롤러와 통신한다.
          * 각 컨트롤러에 queueDepth/numQueues 만큼의 NVMe I/O 큐를 생성한다. */
         std::vector<Controller*> ctrls(settings.n_ctrls);
         if(mem == BAFS_DIRECT){
             cuda_err_chk(cudaSetDevice(settings.cudaDevice));
             for (size_t i = 0 ; i < settings.n_ctrls; i++)
                 ctrls[i] = new Controller(settings.ssdtype == 0 ? sam_ctrls_paths[i] : intel_ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);
             printf("Controllers Created\n");
         }
         char gdevst[15];
         cuda_err_chk(cudaDeviceGetPCIBusId(gdevst, 15, settings.cudaDevice));
         std::cout << "GPUID: "<< gdevst << std::endl;

         printf("Initialization done.\n");
         fflush(stdout);
         
         /* BaM page cache 관련 객체들:
          * - page_cache_t: GPU 메모리에 페이지 캐시를 생성. pc_page_size 크기의 페이지를 pc_pages개 관리.
          * - range_t<uint64_t>: SSD 상의 연속 데이터 영역(edgeList)을 페이지 캐시와 매핑.
          *   ofileoffset부터 시작하는 n_pages개 SSD 페이지를 관리한다.
          * - array_t<uint64_t>: range_t를 감싸서 논리적 배열 인터페이스를 제공.
          *   커널에 전달되는 d_array_ptr을 통해 GPU에서 SSD 데이터에 접근한다. */
         page_cache_t* h_pc;
         range_t<uint64_t>* h_range;
         std::vector<range_t<uint64_t>*> vec_range(1);
         array_t<uint64_t>* h_array;
         uint32_t* curr_frontier_d;
         uint32_t* next_frontier_d;
         
         if((type==BASELINE_PC)||(type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)||(type==FRONTIER_BASELINE_PC)||(type == FRONTIER_COALESCE_PC) || (type== FRONTIER_COALESCE_PTR_PC) ||(type == COALESCE_COARSE_PTR_PC) ||(type == COALESCE_HASH_PTR_PC) || (type == COALESCE_HASH_COARSE_PTR_PC) || (type == COALESCE_HASH_HALF_PTR_PC ) || (type == OPTIMIZED_PC)){            
            h_pc =new page_cache_t(pc_page_size, pc_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
            h_range = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)edge_count, (uint64_t) (ceil(settings.ofileoffset*1.0/pc_page_size)),(uint64_t)n_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice); //, (uint8_t*)edgeList_d);
            vec_range[0] = h_range; 
            h_array = new array_t<uint64_t>(edge_count, settings.ofileoffset, vec_range, settings.cudaDevice);
            
            printf("Page cache initialized\n");
            fflush(stdout);
         }
         if ((type==FRONTIER_BASELINE)||(type == FRONTIER_COALESCE) ||(type==FRONTIER_BASELINE_PC)||(type == FRONTIER_COALESCE_PC)||(type==FRONTIER_COALESCE_PTR_PC)){
             cuda_err_chk(cudaMalloc((void**)&curr_frontier_d,  vertex_count * sizeof(uint32_t)));
             cuda_err_chk(cudaMalloc((void**)&next_frontier_d,  vertex_count * sizeof(uint32_t)));
         }
         uint32_t* tmp_front;




        uint64_t *firstVertexList_d;
        unsigned long long  int *winnerList_d; 
        uint64_t num_elems_in_cl = (uint32_t) tile_size / (sizeof(uint64_t));
        //preprocessing for the optimized implementation.


        if((type == OPTIMIZED_PC) || (type == OPTIMIZED)){
            
            printf("n_tiles: %llu num_elems_in_tile:%llu \n", n_tiles, num_elems_in_cl);
            n_tiles = (edge_count+num_elems_in_cl) / num_elems_in_cl;
            printf("n_tiles: %llu num_elems_in_tile:%llu \n", n_tiles, num_elems_in_cl);
            cuda_err_chk(cudaMalloc((void**)&winnerList_d,   (n_tiles)* sizeof(unsigned long long int)));
            cuda_err_chk(cudaMemset(winnerList_d, UINT64MAX, (n_tiles)* sizeof(unsigned long long int)));
            //printf("UNIT64MAX is: %llu\n", UINT64MAX);
            //uint64_t nblocks = (n_tiles+numthreads)/numthreads;
            //dim3 verifyBlockDim(nblocks); 
            //kernel_verify<<<verifyBlockDim,numthreads>>>(n_tiles,winnerList_d, UINT64MAX, 1);

            // cuda_err_chk(cudaDeviceSynchronize());
            //num_elems_in_cl  = 6;
            printf("Allocating %f MB for FirstVertexList with n_tiles: %llu numelemspercl: %llu\n", ((double)n_tiles*sizeof(uint64_t)/(1024*1024)), n_tiles, num_elems_in_cl);
            cuda_err_chk(cudaMalloc((void**)&firstVertexList_d, n_tiles * sizeof(uint64_t)));

            uint64_t nblocks_step1 = (vertex_count+1+numthreads)/numthreads; 
            uint64_t nblocks_step2 = (n_tiles+numthreads)/numthreads; 
            printf("Launching step1 in generation of FirstVertexList: numblocks: %llu numthreads: %llu\n", nblocks_step1, numthreads);
            dim3 step1blockdim(nblocks_step1);
            dim3 step2blockdim(nblocks_step2);
            kernel_first_vertex_step1<<<step1blockdim,numthreads>>>(vertex_count+1, vertexList_d, num_elems_in_cl, winnerList_d);
            //uint64_t *winnerList_h; 
            //uint64_t copysize = (n_tiles) * sizeof(unsigned long long int);
            //winnerList_h = (uint64_t*)malloc(copysize);
            //cuda_err_chk(cudaMemcpy((void**)winnerList_h, (void**)winnerList_d, copysize, cudaMemcpyDeviceToHost));
            //printf("winnerlist values: \n");
            //for(uint64_t i=0; i< n_tiles; i++){
            //      printf("i: %llu, winner: %llu\n",i, winnerList_h[i]);
            //}
            //printf("\n");
            kernel_first_vertex_step2<<<step2blockdim,numthreads>>>(n_tiles, vertexList_d, winnerList_d, num_elems_in_cl, firstVertexList_d);
            
            //uint64_t *firstVertexList_h; 
            //uint64_t copysize2 = n_tiles * sizeof(unsigned long long int);
            //firstVertexList_h = (uint64_t*) malloc(copysize2); 
            //cuda_err_chk(cudaMemcpy((void**)firstVertexList_h, (void**)firstVertexList_d, copysize2, cudaMemcpyDeviceToHost));
            //printf("Firstvertex values\n");
            //for(uint64_t i=0; i< n_tiles; i++){
            //    printf("%llu\n", firstVertexList_h[i]);
            //}
            uint64_t nblocks = (n_tiles+numthreads)/numthreads;
            //dim3 verifyBlockDim(nblocks); 
            //kernel_verify<<<verifyBlockDim,numthreads>>>(n_tiles, (unsigned long long int*) firstVertexList_d, UINT64MAX, 2);
            
            cuda_err_chk(cudaDeviceSynchronize());
            cuda_err_chk(cudaFree(winnerList_d));
            //free(winnerList_h);
        }

 
         /* (4) BFS 실행 루프: 여러 소스 정점에 대해 반복 실행
          * - label_d를 0xFF(=MYINFINITY)로 초기화 -> 시작 정점만 0으로 설정
          * - do-while 루프: 매 반복마다 현재 level의 정점에서 이웃 탐색
          * - changed_d가 0이면 수렴(새 정점 없음) -> 종료 */
         // Set root
         for (int i = 0; i < total_run; i++) {
             zero = 0;
             cuda_err_chk(cudaMemset(label_d, 0xFF, vertex_count * sizeof(uint32_t)));
             cuda_err_chk(cudaMemcpy(&label_d[src], &zero, sizeof(uint32_t), cudaMemcpyHostToDevice));
             if ((type==FRONTIER_BASELINE)||(type == FRONTIER_COALESCE) ||(type==FRONTIER_BASELINE_PC)||(type == FRONTIER_COALESCE_PC)||(type==FRONTIER_COALESCE_PTR_PC)){
                 cuda_err_chk(cudaMemcpy(curr_frontier_d, &src, sizeof(uint32_t), cudaMemcpyHostToDevice));
             }

             level = 0;
             iter = 0;

             cuda_err_chk(cudaEventRecord(start, 0));
   // printf("*****baseaddr: %p\n", h_pc->pdt.base_addr);
   //          fflush(stdout);

             // Run BFS
             changed_h = 1;

             //printf("Hash Stride: %llu Coarse: %llu\n", (settings.stride), settings.coarse);
             
             do {
                 uint64_t active = changed_h;
                 changed_h = 0;
                 cuda_err_chk(cudaMemcpy(changed_d, &changed_h, sizeof(uint64_t), cudaMemcpyHostToDevice));
                 auto start = std::chrono::system_clock::now();
                 switch (type) {
                     case BASELINE:
                         kernel_baseline<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, globalvisitedcount_d, vertexVisitCount_d);
                         break;
                     case COALESCE:
                         kernel_coalesce<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d);
                         break;
                     case COALESCE_HASH:
                         //TODO: fix the stride 
                         kernel_coalesce_hash<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.stride);
                         break;
                     case COALESCE_CHUNK:
                         kernel_coalesce_chunk<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d);
                         break;
                     case BASELINE_PC:
                         //printf("Calling Page cache enabled baseline kernel\n");
                         kernel_baseline_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, globalvisitedcount_d, vertexVisitCount_d);
                         break;
                     case COALESCE_PC:
                         //printf("Calling Page cache enabled coalesce kernel\n");
                         kernel_coalesce_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d);
                         break;
                     case COALESCE_PTR_PC:
                         //printf("Calling Page cache enabled coalesce kernel\n");
                         kernel_coalesce_ptr_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d);
                         break;
                     case COALESCE_HASH_PTR_PC:
                         {
                            //  //TODO: fix the stride
                            //  //printf("Calling transposed kernel\n");
                            //  uint64_t stride = settings.stride; 
                            //  if(iter == 6){
                            //      printf("changing stride\n");
                            //      fflush(stdout);
                            //      stride = 768; 
                            //  }
                             kernel_coalesce_hash_ptr_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.stride);
                             break;
                         }
                    case COALESCE_CHUNK_PC:
                         //printf("Calling Page cache enabled coalesce chunk kernel\n");
                         kernel_coalesce_chunk_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d);
                         break;

                    case COALESCE_COARSE:
                         {
                             kernel_coalesce_coarse<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.coarse);
                             break;
                         }
                    case COALESCE_HASH_COARSE:
                         {
                            kernel_coalesce_hash_coarse<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.coarse, settings.stride);
                             break;
                         }
                    case COALESCE_COARSE_PTR_PC:
                         {
                             kernel_coalesce_coarse_ptr_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.coarse);
                             break;
                         }
                    case COALESCE_HASH_COARSE_PTR_PC:
                         {
                             kernel_coalesce_hash_coarse_ptr_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.coarse, settings.stride);
                             break;
                         }
                    case COALESCE_HASH_HALF:
                         {
                             kernel_coalesce_hash_half<<<blockDim, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.stride);
                             break;
                         }
                    case COALESCE_HASH_HALF_PTR_PC:
                         {
                             kernel_coalesce_hash_half_ptr_pc<<<blockDim, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d, settings.stride);
                             break;
                         }
                     case FRONTIER_BASELINE:
                          // kernel_frontier_baseline(uint32_t *label, const uint32_t level, const uint64_t vertex_count,
                          //       const uint64_t *vertexList, const EdgeT *edgeList, const uint64_t curr_frontier_size, unsigned long long *changed,
                          //                          const uint32_t *curr_frontier, uint32_t *next_frontier)
                         numblocks = ((active + numthreads) / numthreads);
                         assert(numblocks <= 0xFFFFFFFF);
                         kernel_frontier_baseline<<<numblocks, numthreads>>>((unsigned int*)label_d, (unsigned int) level, vertex_count, vertexList_d, edgeList_d, active,(unsigned long long int*)changed_d, curr_frontier_d, next_frontier_d);
                         tmp_front = curr_frontier_d;
                         curr_frontier_d = next_frontier_d;
                         next_frontier_d = tmp_front;
                         break;
                     case FRONTIER_COALESCE:
                         // kernel_frontier_baseline(uint32_t *label, const uint32_t level, const uint64_t vertex_count,
                         //       const uint64_t *vertexList, const EdgeT *edgeList, const uint64_t curr_frontier_size, unsigned long long *changed,
                         //                          const uint32_t *curr_frontier, uint32_t *next_frontier)
                         numblocks = ((active * WARP_SIZE + numthreads) / numthreads);
                         assert(numblocks <= 0xFFFFFFFF);
                         //printf("numblocks: %llu\n", numblocks);
                         kernel_frontier_coalesce<<<numblocks, numthreads>>>((unsigned int*)label_d, (unsigned int) level, vertex_count, vertexList_d, edgeList_d, active,(unsigned long long int*)changed_d, curr_frontier_d, next_frontier_d);
                         tmp_front = curr_frontier_d;
                         curr_frontier_d = next_frontier_d;
                         next_frontier_d = tmp_front;
                         break;
                     case FRONTIER_BASELINE_PC:
                          // kernel_frontier_baseline(uint32_t *label, const uint32_t level, const uint64_t vertex_count,
                          //       const uint64_t *vertexList, const EdgeT *edgeList, const uint64_t curr_frontier_size, unsigned long long *changed,
                          //                          const uint32_t *curr_frontier, uint32_t *next_frontier)
                         numblocks = ((active + numthreads) / numthreads);
                         assert(numblocks <= 0xFFFFFFFF);
                         kernel_frontier_baseline_pc<<<numblocks, numthreads>>>((unsigned int*)label_d, (unsigned int) level, vertex_count, vertexList_d, h_array->d_array_ptr, active,(unsigned long long int*)changed_d, curr_frontier_d, next_frontier_d);
                         tmp_front = curr_frontier_d;
                         curr_frontier_d = next_frontier_d;
                         next_frontier_d = tmp_front;
                         break;
                     case FRONTIER_COALESCE_PC:
                         // kernel_frontier_baseline(uint32_t *label, const uint32_t level, const uint64_t vertex_count,
                         //       const uint64_t *vertexList, const EdgeT *edgeList, const uint64_t curr_frontier_size, unsigned long long *changed,
                         //                          const uint32_t *curr_frontier, uint32_t *next_frontier)
                         numblocks = ((active * WARP_SIZE + numthreads) / numthreads);
                         assert(numblocks <= 0xFFFFFFFF);
                         //printf("numblocks: %llu\t", numblocks);
                         kernel_frontier_coalesce_pc<<<numblocks, numthreads>>>((unsigned int*)label_d, (unsigned int) level, vertex_count, vertexList_d, h_array->d_array_ptr, active,(unsigned long long int*)changed_d, curr_frontier_d, next_frontier_d);
                         tmp_front = curr_frontier_d;
                         curr_frontier_d = next_frontier_d;
                         next_frontier_d = tmp_front;
                         break;
                     case FRONTIER_COALESCE_PTR_PC:
                         numblocks = ((active * WARP_SIZE + numthreads) / numthreads);
                         assert(numblocks <= 0xFFFFFFFF);
                         //printf("numblocks: %llu\t", numblocks);
                         kernel_frontier_coalesce_ptr_pc<<<numblocks, numthreads>>>((unsigned int*)label_d, (unsigned int) level, vertex_count, vertexList_d, h_array->d_array_ptr, active,(unsigned long long int*)changed_d, curr_frontier_d, next_frontier_d);
                         tmp_front = curr_frontier_d;
                         curr_frontier_d = next_frontier_d;
                         next_frontier_d = tmp_front;
                         break;
                     case OPTIMIZED: 
                            kernel_optimized<<<numblocks, numthreads>>>(label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d,firstVertexList_d, num_elems_in_cl, n_tiles,settings.stride);
                            break;
                     case OPTIMIZED_PC: 
                           kernel_optimized_ptr_pc<<<numblocks, numthreads>>>(h_array->d_array_ptr, label_d, level, vertex_count, vertexList_d, edgeList_d, changed_d,firstVertexList_d, num_elems_in_cl, n_tiles, settings.stride, iter);
                           break;
                     default:
                         fprintf(stderr, "Invalid type\n");
                         exit(1);
                         break;
                 }

                 iter++;
                 level++;

                 cuda_err_chk(cudaMemcpy(&changed_h, changed_d, sizeof(uint64_t), cudaMemcpyDeviceToHost));
                 //auto end = std::chrono::system_clock::now();
                 //if(mem == BAFS_DIRECT) {
                 //     h_array->print_reset_stats();

                 // }
                 //auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                 //std::cout << "Iter "<< iter << " Time: " << elapsed.count() << " ms" << std::endl;

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
                     h_array->print_reset_stats();
                 }
                 fflush(stdout);
                 avg_milliseconds += (double)milliseconds;
				 num_run++; 
			 }
			 else {
                 avg_milliseconds += 0;
			 }
            
             if(settings.src == 0)
                   src += vertex_count / total_run;
             printf("\nBFS-%d Graph:%s \t Impl: %d \t SSD: %d \t CL: %d \t Cache: %llu \t Stride: %llu \t Coarse: %d \t AvgTime %f ms\n", i, filename.c_str(), type, settings.n_ctrls, settings.pageSize, settings.maxPageCacheSize, settings.stride, settings.coarse, milliseconds);
         }
         
         free(vertexList_h);
         if((type==BASELINE_PC)||(type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)||(type==FRONTIER_BASELINE_PC)||(type == FRONTIER_COALESCE_PC) || (type== FRONTIER_COALESCE_PTR_PC) ||(type == COALESCE_COARSE_PTR_PC) ||(type == COALESCE_HASH_PTR_PC) || (type == COALESCE_HASH_COARSE_PTR_PC) || (type == COALESCE_HASH_HALF_PTR_PC ) || (type == OPTIMIZED_PC)){            delete h_pc; 
            delete h_range; 
            delete h_array;
         }
         if (edgeList_h)
             free(edgeList_h);
         cuda_err_chk(cudaFree(vertexList_d));
         cuda_err_chk(cudaFree(label_d));
         cuda_err_chk(cudaFree(changed_d));

         cuda_err_chk(cudaFree(globalvisitedcount_d));
         cuda_err_chk(cudaFree(vertexVisitCount_d));
         vertexVisitCount_h.clear();

         if (edgeList_d)
             cuda_err_chk(cudaFree(edgeList_d));
         
         for (size_t i = 0 ; i < settings.n_ctrls; i++)
             delete ctrls[i];
    }
    catch (const error& e){
        fprintf(stderr, "Unexpected error: %s\n", e.what());
        return 1;
    }
    return 0;
}
