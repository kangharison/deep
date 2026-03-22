/*
 * =============================================================================
 * BaM VectorAdd/Scan 벤치마크 (benchmarks/vectoradd/scan.cu)
 * =============================================================================
 *
 * [목적]
 * 이 파일은 두 가지 기능을 포함한다:
 * 1. Prefix Sum (Scan): 그래프의 vertex별 NVMe 페이지 접근 분포를 분석하기 위한 전처리 도구.
 *    vertex list를 입력으로 받아 각 NVMe 페이지에 몇 개의 edge가 매핑되는지 히스토그램을 생성하고,
 *    Blelloch의 work-efficient parallel prefix sum 알고리즘으로 누적 합을 계산한다.
 *
 * 2. Connected Components (CC): 그래프의 연결 요소를 구하는 GPU 커널들.
 *    다양한 메모리 접근 패턴(coalesce, hash, coarse)으로 edge list를 탐색하며,
 *    GPU 메모리, UVM, 향후 BaM page cache 등 다양한 I/O 경로에서의 성능을 비교한다.
 *
 * [Prefix Sum 알고리즘 (Blelloch Scan)]
 * scan 커널은 Blelloch의 work-efficient parallel scan을 구현한다:
 *   - Phase 1 (Up-sweep / Reduce): stride를 1에서 BLOCKSIZE까지 2배씩 늘리며
 *     인접 쌍을 합산하여 트리의 루트에 전체 합을 모은다.
 *   - Phase 2 (Down-sweep / Distribution): stride를 BLOCKSIZE/2에서 1까지 반씩 줄이며
 *     부분합을 하위 노드로 전파하여 inclusive prefix sum을 완성한다.
 *   - 시간복잡도: O(n) work, O(log n) span (depth)
 *   - 블록 크기 제한으로 2*BLOCKSIZE개를 초과하는 입력은 multi-pass가 필요하므로
 *     intermediate 배열과 finalsum 커널로 블록 간 합산을 처리한다.
 *
 * [CC 알고리즘 (Label Propagation)]
 * Shiloach-Vishkin 스타일의 반복적 label propagation:
 *   - 각 vertex는 자신의 component ID (초기값 = vertex ID)를 가짐
 *   - 매 반복마다 이웃 vertex의 component ID를 확인하고 더 작은 값으로 갱신 (atomicMin)
 *   - 변경이 없을 때까지 반복 -> 수렴 시 같은 component의 vertex들은 같은 ID를 공유
 *
 * [CC 커널 변형들]
 * - kernel_coalesce:            워프 단위 coalesced edge 접근 (기본)
 * - kernel_coalesce_coarse:     coarsening: 한 워프가 여러 vertex 처리
 * - kernel_coalesce_hash:       hash 기반 워프-vertex 재매핑 (SSD 채널 분산)
 * - kernel_coalesce_hash_half:  16-lane half-warp 기반 hash
 * - kernel_coalesce_coarse_hash: coarsening + hash 결합
 *
 * [메모리 모드]
 * - GPUMEM (0):           GPU VRAM에 edge list 복사 후 연산
 * - UVM_READONLY (1):     UVM + SetReadMostly
 * - UVM_DIRECT (2):       UVM + O_DIRECT + SetAccessedBy
 * - UVM_READONLY_NVLINK (3): UVM + NVLink 원격 프리페치
 * - UVM_DIRECT_NVLINK (4):   UVM Direct + NVLink
 *
 * References:
 *      Coalesce
 *          Hong, Sungpack, et al.
 *          "Accelerating CUDA graph algorithms at maximum warp."
 *          Acm Sigplan Notices 46.8 (2011): 267-276.
 *
 */

#include <cuda.h>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "helper_cuda.h"
#include <algorithm>
#include <vector>
#include <numeric>
#include <fstream>
#include <iterator>
#include <math.h>
#include <chrono>
#include <ctime>
#include <ratio>

#include <fcntl.h>
#include <unistd.h>

#include <page_cache.h>
#include <cuMemcpy.h>

#define WARP_SHIFT 5       /* log2(32) = 5 */
#define WARP_SIZE 32        /* NVIDIA 워프 크기 */

#define CHUNK_SHIFT 3
#define CHUNK_SIZE (1 << CHUNK_SHIFT)

#define BLOCK_NUM 1024ULL   /* 2D 그리드 x 방향 블록 수 */
#define BLOCKSIZE 512       /* scan 커널의 블록 크기. shared memory에 2*BLOCKSIZE개 원소 저장 */
#define MAXWARP 64          /* SM당 최대 워프 수 */

//#define COARSE 4
typedef uint64_t EdgeT;     /* 에지(인접 vertex ID) 타입 */

/*
 * 커널 구현 타입 열거형:
 * - COALESCE(0):              워프 단위 coalesced edge 탐색
 * - COALESCE_COARSE(1):       coarsening: 한 워프가 coarse개 vertex 처리
 * - COALESCE_CHUNK(2):        chunk 단위 edge 탐색 (미구현/legacy)
 * - COALESCE_PC(4):           coalesce + BaM page cache (미구현)
 * - COALESCE_CHUNK_PC(5):     chunk + page cache (미구현)
 * - SCAN(6):                  prefix sum 전처리 (NVMe 페이지별 edge 분포 분석)
 * - COALESCE_HASH(7):         hash 기반 워프-vertex 재매핑 (SSD 채널 분산)
 * - COALESCE_COARSE_HASH(18): coarsening + hash 결합
 * - COALESCE_HASH_HALF(20):   16-lane half-warp hash
 */
typedef enum {
    COALESCE = 0,
    COALESCE_COARSE = 1,
    COALESCE_CHUNK = 2,
    COALESCE_PC = 4,
    COALESCE_CHUNK_PC =5,
    SCAN =6,
    COALESCE_HASH= 7,
    COALESCE_COARSE_HASH= 18,
    COALESCE_HASH_HALF= 20,
} impl_type;

/*
 * 메모리 할당 모드 열거형:
 * - GPUMEM(0):              cudaMalloc + cudaMemcpy
 * - UVM_READONLY(1):        UVM + SetReadMostly
 * - UVM_DIRECT(2):          UVM + O_DIRECT + SetAccessedBy
 * - UVM_READONLY_NVLINK(3): UVM + NVLink 프리페치
 * - UVM_DIRECT_NVLINK(4):   UVM Direct + NVLink
 */
typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    UVM_READONLY_NVLINK = 3,
    UVM_DIRECT_NVLINK = 4,
} mem_type;

typedef uint64_t VertT;     /* vertex 인덱스/값 타입 */


/*
 * preprocess_kernel: 각 vertex의 edge가 어떤 NVMe 페이지에 속하는지 카운팅하여 히스토그램 생성
 *
 * [GPU 스레드 구조]
 * - 2D 그리드: tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x
 * - 각 스레드가 하나의 vertex를 처리
 *
 * [알고리즘]
 * - vertices[tid]는 vertex tid의 edge list 시작 오프셋 (CSR 포맷의 row pointer)
 * - 이 오프셋을 num_elems_per_page로 나누면 해당 edge가 속한 NVMe 페이지 번호를 얻음
 * - atomicAdd로 outarray[page_num]을 증가시켜 페이지별 edge 분포 히스토그램을 생성
 * - 이 히스토그램에 prefix sum을 적용하면 페이지별 누적 edge 수를 알 수 있음
 */
__global__
void preprocess_kernel(VertT* vertices, uint64_t vertex_count, uint64_t num_elems_per_page, uint64_t n_pages, VertT* outarray){
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;

    if(tid<vertex_count){

        unsigned long long int val = vertices[tid] / (num_elems_per_page);
        //if(val>=n_pages)
        //    printf("val: %llu \t update: %llu\n", val, 1);
        unsigned long long int update = atomicAdd((unsigned long long int*)&outarray[val], 1);
    }
}


/*
 * finalsum: multi-block scan의 마지막 단계. 각 블록의 prefix sum 결과에 이전 블록들의 총합을 더한다.
 *
 * [알고리즘]
 * - blockIdx.x == 0인 첫 블록은 이전 블록이 없으므로 스킵
 * - input[blockIdx.x - 1]: 이전 블록까지의 누적 합 (scan의 intermediate 결과)
 * - output의 해당 원소들에 이 값을 더해서 전역 prefix sum을 완성
 *
 * [스레드 매핑]
 * - 각 스레드가 2개 원소를 처리: output[idx]와 output[idx+BLOCKSIZE]
 */
__global__ void finalsum (VertT *input, VertT* output, uint64_t len){
   if(blockIdx.x == 0) return;  /* 첫 블록은 보정 불필요 */

   uint64_t idx = 2*blockIdx.x *BLOCKSIZE + threadIdx.x;

   if(idx < len)
       output[idx] += input[blockIdx.x -1];
   if((idx + BLOCKSIZE) < len)
       output[idx+BLOCKSIZE] += input[blockIdx.x -1];
}



/*
 * scan: Blelloch의 work-efficient parallel inclusive prefix sum
 *
 * [GPU 스레드 구조]
 * - 블록: BLOCKSIZE개 스레드 (512)
 * - shared memory: 2*BLOCKSIZE개 원소 (각 블록이 1024개 원소를 처리)
 * - 각 스레드가 2개 원소를 shared memory에 로드
 *
 * [알고리즘]
 * Phase 1: Up-sweep (Reduce)
 *   - stride를 1 -> 2 -> 4 -> ... -> BLOCKSIZE까지 2배씩 증가
 *   - 각 단계에서 index = (tid+1)*2*stride - 1 위치의 원소에 stride만큼 떨어진 원소를 더함
 *   - 트리 구조로 부분합을 상위로 전파: 완료 시 sharedMem[2*BLOCKSIZE-1]에 전체 합이 저장됨
 *
 * Phase 2: Down-sweep (Distribution)
 *   - stride를 BLOCKSIZE/2 -> BLOCKSIZE/4 -> ... -> 1까지 반씩 감소
 *   - 각 단계에서 index = (tid+1)*stride*2 - 1 위치의 값을 index+stride 위치에 더함
 *   - 상위 노드의 부분합을 하위 노드로 전파하여 inclusive prefix sum 완성
 *
 * [intermediate 배열]
 * - intermediate != NULL이면 각 블록의 마지막 원소(전체 합)를 intermediate[blockIdx.x]에 저장
 * - 이 값들에 재귀적으로 scan을 적용한 뒤 finalsum으로 블록 간 합산을 완성
 *
 * [시간복잡도] O(n) work, O(log n) depth. 총 3-pass: scan -> scan(intermediate) -> finalsum
 */
__global__ void scan(VertT *input, VertT *output, VertT *intermediate,  uint64_t len){

    __shared__ VertT sharedMem[BLOCKSIZE*2];   /* 블록당 2*512 = 1024개 원소 저장 */

    uint64_t idx = 2*blockIdx.x * BLOCKSIZE + threadIdx.x;

    /* 각 스레드가 2개 원소를 global -> shared memory로 로드 */
    if(idx < len)
        sharedMem[threadIdx.x] = input[idx];
    else
        sharedMem[threadIdx.x] = 0;

    if((idx + BLOCKSIZE) < len)
        sharedMem[threadIdx.x + BLOCKSIZE] = input[idx + BLOCKSIZE];
    else
        sharedMem[threadIdx.x + BLOCKSIZE] = 0;

    __syncthreads();

    /* Phase 1: Up-sweep (Reduce) - 부분합을 트리 상위로 전파 */
    for(uint64_t stride = 1; stride <= BLOCKSIZE; stride*=2){
        uint64_t index = (threadIdx.x+1)*2*stride -1;
        if(index <2 * BLOCKSIZE)
            sharedMem[index] += sharedMem[index-stride];
        __syncthreads();
    }

    /* Phase 2: Down-sweep (Distribution) - inclusive prefix sum 완성 */
    for(uint64_t stride = BLOCKSIZE/2; stride>0; stride/=2){
        uint64_t index = (threadIdx.x+1)*stride*2 - 1;
        if(index + stride < (2*BLOCKSIZE))
            sharedMem[index+stride] += sharedMem[index];
        __syncthreads();
    }

    /* 결과를 global memory로 기록 */
    if(idx<len) output[idx] = sharedMem[threadIdx.x];
    if(idx+BLOCKSIZE < len) output[idx+BLOCKSIZE] = sharedMem[threadIdx.x + BLOCKSIZE];

    /* intermediate 배열에 이 블록의 총합을 저장 (multi-block scan용) */
    if(intermediate !=NULL && threadIdx.x == 0)
        intermediate[blockIdx.x] = sharedMem[2*BLOCKSIZE-1];
}


/*
 * cc_compute: Connected Components 핵심 연산 (디바이스 함수)
 *
 * [알고리즘: Shiloach-Vishkin label propagation]
 * - vertex cid와 이웃 vertex next의 component ID를 비교
 * - 서로 다르면 더 작은 ID를 가진 쪽의 component로 병합 (atomicMin)
 * - 변경이 발생하면 next_visit과 changed를 true로 설정하여 다음 반복을 트리거
 *
 * [매개변수]
 * - cid: 현재 vertex ID
 * - comp: 각 vertex의 component ID 배열 (comp[v] = v가 속한 component의 최소 vertex ID)
 * - next: 이웃 vertex ID (edge list에서 읽음)
 * - next_visit: 다음 반복에서 방문할 vertex 표시
 * - changed: 이번 반복에서 변경이 있었는지 여부
 */
__device__ void cc_compute(uint64_t cid, unsigned long long *comp, EdgeT next, bool *next_visit, bool *changed){

    unsigned long long comp_src = comp[cid];
    unsigned long long comp_next = comp[next];
    unsigned long long comp_target;
    EdgeT next_target;

    if (comp_next != comp_src) {
       if (comp_src < comp_next) {
          next_target = next;
          comp_target = comp_src;
       }
       else {
          next_target = cid;
          comp_target = comp_next;
       }

       atomicMin(&comp[next_target], comp_target);
       next_visit[next_target] = true;
       *changed = true;
    }
}


/*
 * kernel_coalesce_coarse: coarsening을 적용한 coalesced CC 커널
 *
 * [GPU 스레드 구조]
 * - 워프 단위: 32개 lane이 하나의 vertex의 edge list를 coalesced하게 탐색
 * - coarsening: 한 워프가 coarse개의 연속 vertex를 순차 처리 (루프)
 * - 이를 통해 워프당 처리량을 높이고 커널 launch 오버헤드를 줄임
 *
 * [데이터 접근 패턴]
 * - shift_start = start & 0xFFFFFFFFFFFFFFF0: edge list 시작 주소를 16 단위로 정렬
 *   -> 캐시 라인 경계에 맞춰 메모리 접근을 정렬하여 coalescing 효율 극대화
 * - 워프 내 32개 lane이 stride=32로 edge list를 순회
 */
__global__ void kernel_coalesce_coarse(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed, uint64_t coarse) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> (WARP_SHIFT);
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);

    for(uint64_t j=0; j< coarse; j++){
        uint64_t cwarpIdx = warpIdx*coarse+j;
        if ( (cwarpIdx) < vertex_count) {
            if(curr_visit[cwarpIdx] == true) {
               const uint64_t start = vertexList[cwarpIdx];
               const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;  /* 16 원소 정렬 */
               const uint64_t end = vertexList[cwarpIdx+1];

               for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
                   if (i >= start) {
                       const EdgeT next = edgeList[i];
                       cc_compute(cwarpIdx, comp, next, next_visit, changed);
                   }
               }
            }
        }
    }
}

/*
 * kernel_coalesce: 기본 워프 단위 coalesced CC 커널
 *
 * [GPU 스레드 구조]
 * - 전역 워프 ID: warpIdx = tid >> 5 (각 워프가 하나의 vertex 담당)
 * - lane ID: laneIdx = tid & 31 (워프 내 스레드 위치)
 *
 * [데이터 접근 패턴]
 * - vertexList[warpIdx], vertexList[warpIdx+1]: CSR 포맷에서 vertex의 edge 범위
 * - shift_start로 16 원소 정렬 후, 32개 lane이 stride=32로 edge list를 순회
 * - 이 접근 패턴은 GPU의 memory coalescing을 활용하여 메모리 대역폭을 극대화
 */
__global__ void kernel_coalesce(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);


        if ((warpIdx) < vertex_count) {
            if(curr_visit[warpIdx] == true) {
               const uint64_t start = vertexList[warpIdx];
               const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
               const uint64_t end = vertexList[warpIdx+1];

               for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
                   if (i >= start) {
                       const EdgeT next = edgeList[i];
                       cc_compute(warpIdx, comp, next, next_visit, changed);
                   }
               }
            }
        }
}



/*
 * kernel_coalesce_hash: hash 기반 워프-vertex 재매핑을 적용한 CC 커널
 *
 * [핵심 아이디어: SSD 채널 분산을 위한 워프 재매핑]
 * - 기본 coalesce에서는 인접 워프가 인접 vertex(= 인접 NVMe 페이지)를 접근
 *   -> 동일 NVMe 채널에 I/O가 집중되어 대역폭 낭비
 * - hash 재매핑으로 인접 워프들이 서로 다른 NVMe 채널의 페이지를 접근하도록 분산
 *
 * [워프 ID 재매핑 공식]
 * STRIDE = sm_count * MAXWARP (GPU의 총 동시 실행 워프 수)
 * nep = (vertex_count + STRIDE) / STRIDE
 * warpIdx = (oldwarpIdx / nep) + ((oldwarpIdx % nep) * STRIDE)
 *
 * 이 매핑은 연속된 oldwarpIdx를 STRIDE 간격으로 분산시켜
 * 인접 워프들이 물리적으로 멀리 떨어진 NVMe 페이지를 접근하게 만든다.
 */
//__global__ __launch_bounds__(128,16)
__global__
void kernel_coalesce_hash(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed, int sm_count) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = tid >> WARP_SHIFT;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    uint64_t STRIDE = sm_count * MAXWARP;

    uint64_t warpIdx;
    const uint64_t nep = (vertex_count+STRIDE)/STRIDE;
    warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*STRIDE);

    if (warpIdx < vertex_count){
       if(curr_visit[warpIdx] == true){
            const uint64_t start = vertexList[warpIdx];
            const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
            const uint64_t end = vertexList[warpIdx+1];

            for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
                if (i >= start) {
                    const EdgeT next = edgeList[i];

                    cc_compute(warpIdx, comp, next, next_visit, changed);
                }
            }
        }
   }
}

/*
 * kernel_coalesce_hash_half: 16-lane half-warp 기반 hash CC 커널
 *
 * [차이점: 16 스레드 = half warp]
 * - oldwarpIdx를 tid >> 4 (16으로 나눔)로 계산하여 half-warp 단위로 vertex 할당
 * - laneIdx도 tid & 0xF (16개 lane)
 * - shift_start 정렬도 0xFFFFFFFFFFFFFFFC (4 원소 단위)로 더 세밀하게 조정
 * - half-warp는 degree가 작은 그래프에서 워프 내 idle lane을 줄여 효율성 향상
 */
__global__
void kernel_coalesce_hash_half(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed, int sm_count) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = tid >> 4;
    // const uint64_t warpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << 4) - 1);
    uint64_t STRIDE = sm_count;

    const uint64_t nep = (vertex_count+STRIDE)/STRIDE;
    uint64_t warpIdx = (oldwarpIdx/nep) + ((oldwarpIdx % nep)*STRIDE);

    if (warpIdx < vertex_count){
            if(curr_visit[warpIdx] == true){
                const uint64_t start = vertexList[warpIdx];
                const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFFC;
                //const uint64_t shift_start = start;
                const uint64_t end = vertexList[warpIdx+1];

                for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
                    if (i >= start) {
                    //{
                        const EdgeT next = edgeList[i];
                        cc_compute(warpIdx, comp, next, next_visit, changed);
                    }
                }
            }
    }
}

/*
 * kernel_coalesce_coarse_hash: coarsening + hash 재매핑을 결합한 CC 커널
 *
 * [결합 전략]
 * - hash로 워프를 NVMe 채널에 분산 + coarsening으로 워프당 처리량 증가
 * - stride 파라미터로 분산 정도를 제어하고, coarse 파라미터로 워프당 vertex 수를 제어
 * - nep = (vertex_count + STRIDE*coarse) / (STRIDE*coarse)
 * - cwarpIdx = (coldwarpIdx / nep) + ((coldwarpIdx % nep) * STRIDE)
 * - warpIdx = cwarpIdx * coarse + j (j = 0..coarse-1)
 */
__global__
void kernel_coalesce_coarse_hash(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed, int sm_count, uint64_t coarse, uint64_t stride) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    const uint64_t oldwarpIdx = tid >> WARP_SHIFT;
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1);
    //uint64_t STRIDE = sm_count * MAXWARP;
    uint64_t STRIDE = stride;//sm_count * MAXWARP;

    //const uint64_t coldwarpIdx = oldwarpIdx * coarse;
    const uint64_t coldwarpIdx = oldwarpIdx;

    const uint64_t nep = (vertex_count+(STRIDE*coarse))/(STRIDE*coarse);
    uint64_t cwarpIdx = (coldwarpIdx/nep) + ((coldwarpIdx % nep)*(STRIDE));
    //uint64_t cwarpIdx = coldwarpIdx;

    for(uint64_t j=0; j<coarse; j++){
        uint64_t warpIdx = cwarpIdx*coarse+j;
        if ((warpIdx) < vertex_count){

           if(curr_visit[warpIdx] == true){
                const uint64_t start = vertexList[warpIdx];
                const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;
                const uint64_t end = vertexList[warpIdx+1];

                for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) {
                    if (i >= start) {
                        const EdgeT next = edgeList[i];
                        cc_compute(warpIdx, comp, next, next_visit, changed);
                    }
                }
            }
        }
    }
}




/* throttle_memory: GPU 메모리를 의도적으로 점유하여 가용 메모리를 줄이는 헬퍼 커널 (UVM 스래싱 테스트용) */
__global__ void throttle_memory(uint32_t *pad) {
    pad[1] = pad[0];
}

/*
 * =============================================================================
 * main 함수: 그래프 파일 로드 -> CC/Scan 커널 실행 -> 결과 출력
 * =============================================================================
 *
 * [입력 파일 형식: .bel 그래프]
 * - .col 파일: CSR의 row pointer (vertex list). [8B vertex_count][8B typeT][data...]
 * - .dst 파일: CSR의 column indices (edge list). [8B edge_count][8B typeT][data...]
 *
 * [실행 흐름]
 * 1. 커맨드라인 파싱 (-f 파일, -t 타입, -m 메모리, -p 페이지크기, -n 캐시엔트리, -s stride, -l coarse)
 * 2. vertex/edge 파일 로드
 * 3. 메모리 모드에 따라 edge list를 GPU/UVM에 배치
 * 4. CC 알고리즘을 changed == false 될 때까지 반복 실행
 * 5. component 수 카운트 및 실행 시간 출력
 */
int main(int argc, char *argv[]) {
    using namespace std::chrono;
    std::ifstream file;
    std::string vertex_file, edge_file;
    std::string filename;

    bool changed_h, *changed_d;
    bool *curr_visit_d, *next_visit_d, *comp_check;
    int c, arg_num = 0;
    impl_type type;
    mem_type mem;
    uint32_t *pad;
    uint32_t iter, comp_total = 0;
    unsigned long long *comp_d, *comp_h;
    uint64_t *vertexList_h, *vertexList_d;
    EdgeT *edgeList_h, *edgeList_d;
    uint64_t vertex_count, edge_count, vertex_size, edge_size;
    uint64_t typeT;
    uint64_t numblocks, numthreads;
    size_t freebyte, totalbyte;
    EdgeT *edgeList_dtmp;

    float milliseconds;

    Settings settings;
    uint64_t pc_page_size = 4096;             /* BaM page cache 페이지 크기. -p 옵션으로 변경 가능 */
    uint64_t pc_pages = 2*1024*1024;          /* 페이지 캐시 엔트리 수. 2M * 4096 = 8GB. -n 옵션으로 변경 가능 */

    uint64_t stride;  /* hash 워프 재매핑 stride */
    uint64_t coarse;  /* coarsening 팩터 */

    cudaEvent_t start, end;

    /* 커맨드라인 옵션 파싱 (getopt 사용) */
    while ((c = getopt(argc, argv, "f:t:m:p:n:s:l:h")) != -1) {
        switch (c) {
            case 'f':
                filename = optarg;   /* 그래프 파일 기본명 (.col/.dst 확장자 없이) */
                arg_num++;
                break;
            case 't':
                type = (impl_type)atoi(optarg);  /* 커널 타입 (0~20) */
                arg_num++;
                break;
            case 'm':
                mem = (mem_type)atoi(optarg);     /* 메모리 모드 (0~4) */
                arg_num++;
                break;
            case 'p':
                //Need to add type condition check.
                pc_page_size = atoi(optarg);      /* page cache 페이지 크기 (바이트) */
                arg_num++;
                break;
            case 'n':
                pc_pages = atoi(optarg);          /* page cache 엔트리 수 */
                arg_num++;
                break;
            case 's':
                stride = (uint64_t) atoi(optarg); /* hash stride 팩터 */
                arg_num++;
                break;
            case 'l':
                coarse = (uint64_t) atoi(optarg); /* coarsening 팩터 */
                arg_num++;
                break;
            case 'h':
                printf("\t-f | input file name (must end with .bel)\n");
                printf("\t-t | type of CC to run.\n");
                printf("\t   | COALESCE = 1, COALESCE_CHUNK = 2\n");
                printf("\t   | COALESCE_PC = 4, COALESCE_CHUNK_PC = 5\n");
                printf("\t-m | memory allocation.\n");
                printf("\t   | GPUMEM = 0, UVM_READONLY = 1, UVM_DIRECT = 2,\n");
                printf("\t   | UVM_READONLY_NVLINK = 3, UVM_DIRECT_NVLINK = 4, DRAGON_MAP = 5\n");
                printf("\t-p | (applies only for PC) page cache page size in bytes\n");
                printf("\t-n | (applies only for PC) number of entries in page cache\n");
                printf("\t-h | help message\n");
                return 0;
            case '?':
                break;
            default:
                break;
        }
    }


    if (arg_num < 3) {
                printf("\t-f | input file name (must end with .bel)\n");
                printf("\t-t | type of CC to run.\n");
                printf("\t   | COALESCE = 1, COALESCE_CHUNK = 2\n");
                printf("\t   | COALESCE_PC = 4, COALESCE_CHUNK_PC = 5\n");
                printf("\t-m | memory allocation.\n");
                printf("\t   | GPUMEM = 0, UVM_READONLY = 1, UVM_DIRECT = 2,\n");
                printf("\t   | UVM_READONLY_NVLINK = 3, UVM_DIRECT_NVLINK = 4, DRAGON_MAP = 5\n");
                printf("\t-p | (applies only for PC) page cache page size in bytes\n");
                printf("\t-s | (applies only for PC) number of entries in page cache\n");
                printf("\t-h | help message\n");
        return 0;
    }

    printf("coarse: %llu stride: %llu \n", coarse, stride);
    cudaDeviceProp properties;
    if (cudaGetDeviceProperties(&properties, 0) != cudaSuccess)
    {
        fprintf(stderr, "Failed to get CUDA device properties\n");
        return 1;
    }

    checkCudaErrors(cudaEventCreate(&start));
    checkCudaErrors(cudaEventCreate(&end));

    /* CSR 그래프 파일: .col = vertex list (row pointers), .dst = edge list (column indices) */
    vertex_file = filename + ".col";
    edge_file = filename + ".dst";

    std::cout << filename << std::endl;
    fprintf(stderr, "File %s\n", filename.c_str());

    /* Vertex list 로드: CSR 포맷의 row pointer 배열 */
    file.open(vertex_file.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        printf("vertex file open failed\n");
        exit(1);
    };

    file.read((char*)(&vertex_count), 8);
    file.read((char*)(&typeT), 8);

    vertex_count--;  /* 파일의 vertex_count는 row pointer 수 = vertex 수 + 1 */

    printf("Vertex: %llu, ", vertex_count);
    vertex_size = (vertex_count+1) * sizeof(uint64_t);

    vertexList_h = (uint64_t*)malloc(vertex_size);

    file.read((char*)vertexList_h, vertex_size);
    file.close();

    /* Edge list 로드: CSR 포맷의 column index 배열 */
    file.open(edge_file.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        printf("edge file open failed\n");
        exit(1);
    };

    file.read((char*)(&edge_count), 8);
    file.read((char*)(&typeT), 8);

    printf("Edge: %llu\n", edge_count);
    fflush(stdout);
    /* edge_size를 4KB 경계에 올림 정렬 (O_DIRECT 호환용) */
    edge_size = edge_count * sizeof(EdgeT);
    edge_size = edge_size + (4096 - (edge_size & 0xFFFULL));

    edgeList_h = NULL;

    /*
     * 메모리 모드별 edge list 배치:
     * - GPUMEM: 호스트에서 읽고 cudaMemcpy로 GPU VRAM에 복사
     * - UVM_READONLY: UVM에 직접 읽고 SetReadMostly 힌트
     * - UVM_DIRECT: O_DIRECT로 UVM에 직접 fread + SetAccessedBy 힌트 + vertex list 오프셋 보정(+2)
     * - UVM_READONLY_NVLINK: device 0에서 UVM 할당 후 device 2로 프리페치
     * - UVM_DIRECT_NVLINK: device 2에서 UVM 할당 후 프리페치, device 0에서 접근
     */
    switch (mem) {
        case GPUMEM:
            edgeList_h = (EdgeT*)malloc(edge_size);
            file.read((char*)edgeList_h, edge_size);
            checkCudaErrors(cudaMalloc((void**)&edgeList_d, edge_size));
            break;
        case UVM_READONLY:
            checkCudaErrors(cudaMallocManaged((void**)&edgeList_d, edge_size));
            file.read((char*)edgeList_d, edge_size);
            checkCudaErrors(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, 0));

            //checkCudaErrors(cudaMemGetInfo(&freebyte, &totalbyte));
            //if (totalbyte < 16*1024*1024*1024ULL)
            //    printf("total memory sizeo of current GPU is %llu byte, no need to throttle\n", totalbyte);
            //else {
            //    printf("total memory sizeo of current GPU is %llu byte, throttling %llu byte.\n", totalbyte, totalbyte - 16*1024*1024*1024ULL);
            //    checkCudaErrors(cudaMalloc((void**)&pad, totalbyte - 16*1024*1024*1024ULL));
            //    throttle_memory<<<1,1>>>(pad);
            //}
            break;
        case UVM_DIRECT:
            {/*
            checkCudaErrors(cudaMallocManaged((void**)&edgeList_d, edge_size));
            file.read((char*)edgeList_d, edge_size);
            checkCudaErrors(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetAccessedBy, 0));
            break;
            */
            file.close();
            /* vertex list 오프셋 보정: UVM_DIRECT에서는 파일 헤더(16B = 2개 uint64_t)를 포함하여 읽으므로 vertex list의 edge 오프셋을 +2 */
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
            checkCudaErrors(cudaMallocManaged((void**)&edgeList_d, edge_size_4k_aligned));
            checkCudaErrors(cudaMemAdvise(edgeList_d, edge_size_4k_aligned, cudaMemAdviseSetAccessedBy, 0));
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
        case UVM_READONLY_NVLINK:
            checkCudaErrors(cudaSetDevice(0));
            checkCudaErrors(cudaMallocManaged((void**)&edgeList_d, edge_size));
            file.read((char*)edgeList_d, edge_size);
            checkCudaErrors(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetReadMostly, 0));
            checkCudaErrors(cudaSetDevice(2));
            checkCudaErrors(cudaMemPrefetchAsync(edgeList_d, edge_size, 2, 0));
            checkCudaErrors(cudaDeviceSynchronize());
            checkCudaErrors(cudaSetDevice(0));
            break;
        case UVM_DIRECT_NVLINK:
            checkCudaErrors(cudaSetDevice(2));
            checkCudaErrors(cudaMallocManaged((void**)&edgeList_d, edge_size));
            file.read((char*)edgeList_d, edge_size);
            checkCudaErrors(cudaMemPrefetchAsync(edgeList_d, edge_size, 2, 0));
            checkCudaErrors(cudaDeviceSynchronize());
            checkCudaErrors(cudaSetDevice(0));
            checkCudaErrors(cudaMemAdvise(edgeList_d, edge_size, cudaMemAdviseSetAccessedBy, 0));
            break;
        //case DRAGON_MAP:
        //    if((dragon_map(edge_file.c_str(), (edge_size+16), D_F_READ, (void**) &edgeList_dtmp)) != D_OK){
        //          printf("Dragon Map Failed for edgelist\n");
        //          return -1;
        //    }
        //    edgeList_d = edgeList_dtmp+2;
        //    break;
    }

    file.close();

    /* CC 알고리즘용 GPU 메모리 할당 */
    comp_h = (unsigned long long*)malloc(vertex_count * sizeof(unsigned long long));
    comp_check = (bool*)malloc(vertex_count * sizeof(bool));
    checkCudaErrors(cudaMalloc((void**)&vertexList_d, vertex_size));
    checkCudaErrors(cudaMalloc((void**)&curr_visit_d, vertex_count * sizeof(bool)));   /* 현재 반복에서 방문할 vertex 마스크 */
    checkCudaErrors(cudaMalloc((void**)&next_visit_d, vertex_count * sizeof(bool)));   /* 다음 반복에서 방문할 vertex 마스크 */
    checkCudaErrors(cudaMalloc((void**)&comp_d, vertex_count * sizeof(unsigned long long)));  /* component ID 배열 */
    checkCudaErrors(cudaMalloc((void**)&changed_d, sizeof(bool)));   /* 이번 반복에서 변경 발생 여부 */

    printf("Allocation finished\n");
    fflush(stdout);

    /* CC 초기화: 각 vertex의 component ID를 자기 자신으로 설정 */
    for (uint64_t i = 0; i < vertex_count; i++)
        comp_h[i] = i;

    memset(comp_check, 0, vertex_count * sizeof(bool));

    /* 첫 반복: 모든 vertex를 방문 대상으로 설정 */
    checkCudaErrors(cudaMemset(curr_visit_d, 0x01, vertex_count * sizeof(bool)));
    checkCudaErrors(cudaMemset(next_visit_d, 0x00, vertex_count * sizeof(bool)));
    checkCudaErrors(cudaMemcpy(comp_d, comp_h, vertex_count * sizeof(uint64_t), cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(vertexList_d, vertexList_h, vertex_size, cudaMemcpyHostToDevice));

    if (mem == GPUMEM)
        checkCudaErrors(cudaMemcpy(edgeList_d, edgeList_h, edge_size, cudaMemcpyHostToDevice));

    numthreads = BLOCKSIZE;

    /*
     * 커널 타입에 따른 그리드 크기 계산:
     * - SCAN: 블록당 2*BLOCKSIZE개 원소 처리 -> numblocks = ceil(vertex_count / (2*BLOCKSIZE))
     * - COALESCE/HASH: 워프당 1 vertex -> numblocks = ceil(vertex_count * 32 / numthreads)
     * - COALESCE_COARSE: coarsening으로 워프당 coarse개 vertex -> numblocks 감소
     * - CHUNK: chunk 단위 -> numblocks = ceil(vertex_count * (32/8) / numthreads)
     */
    switch (type) {
        case SCAN:
            numblocks = ((vertex_count / (numthreads*2))+1);
            break;
        case COALESCE_COARSE:
        case COALESCE_COARSE_HASH:
        case COALESCE_HASH_HALF:
            numblocks = ((vertex_count * (WARP_SIZE/coarse) + numthreads) / numthreads);
            break;
        case COALESCE:
        case COALESCE_PC:
        case COALESCE_HASH:
            numblocks = ((vertex_count * WARP_SIZE + numthreads) / numthreads);
            break;
        case COALESCE_CHUNK:
        case COALESCE_CHUNK_PC:
            numblocks = ((vertex_count * (WARP_SIZE / CHUNK_SIZE) + numthreads) / numthreads);
            break;
        default:
            fprintf(stderr, "Invalid type\n");
            exit(1);
            break;
    }

    /* 2D 그리드: x = min(numblocks, BLOCK_NUM), y = ceil(numblocks / BLOCK_NUM) */
    dim3 blockDim(BLOCK_NUM, (numblocks+BLOCK_NUM)/BLOCK_NUM);

    printf("Initialization done\n");
    fflush(stdout);

    if((type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
            printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
    }

    /* BaM page cache 초기화 (COALESCE_PC / COALESCE_CHUNK_PC 전용) */
    page_cache_t* h_pc;
    range_t<uint64_t>* h_range;
    std::vector<range_t<uint64_t>*> vec_range(1);
    array_t<uint64_t>* h_array;

    uint64_t n_pages = ceil(((float)edge_size)/pc_page_size);//fix needed. TODO
    if((type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
//     page_cache_t h_pc(pc_page_size, pc_pages, settings, (uint64_t) 64);
//     range_t<uint64_t>* d_range = (range_t<uint64_t>*) h_range.d_range_ptr;
       h_pc =new page_cache_t(pc_page_size, pc_pages, settings, (uint64_t) 1);
       h_range = new range_t<uint64_t>((int)0 ,(uint64_t)edge_count, (int) 0,(uint64_t)n_pages, (int)0, (uint64_t)pc_page_size, h_pc, settings, (uint8_t*)edgeList_d);
       vec_range[0] = h_range;
       h_array = new array_t<uint64_t>(edge_count, 0, vec_range, settings);

       printf("Page cache initialized\n");
       fflush(stdout);
    }

    iter = 0;
    checkCudaErrors(cudaEventRecord(start, 0));

    /*
     * CC 메인 루프: changed_h == false가 될 때까지 반복
     * 매 반복마다:
     * 1. changed_d를 false로 리셋
     * 2. 선택된 CC 커널 실행 (또는 SCAN이면 prefix sum 실행)
     * 3. changed_d를 호스트로 복사하여 수렴 여부 확인
     * 4. curr_visit와 next_visit를 스왑 (다음 반복 준비)
     */
    do {
        changed_h = false;
        checkCudaErrors(cudaMemcpy(changed_d, &changed_h, sizeof(bool), cudaMemcpyHostToDevice));

        VertT* intermediate_d;   /* multi-block scan의 중간 결과 */
        VertT* dev2out_d;        /* intermediate의 scan 결과 */
        VertT* result_d;         /* 최종 prefix sum 결과 */
        VertT* outarray_h;       /* 히스토그램 호스트 복사본 */
        VertT* result_h;         /* prefix sum 호스트 복사본 */
        checkCudaErrors(cudaMalloc((void**)&result_d, (n_pages+1)*sizeof(VertT)));
        checkCudaErrors(cudaMalloc((void**)&intermediate_d, BLOCKSIZE*2*sizeof(VertT)));
        checkCudaErrors(cudaMalloc((void**)&dev2out_d, BLOCKSIZE*2*sizeof(VertT)));
        result_h = (VertT*) malloc(n_pages* sizeof(VertT));
        outarray_h = (VertT*) malloc(n_pages * sizeof(VertT));
        checkCudaErrors(cudaMemset(result_d, 0, (n_pages+1)*sizeof(VertT)));

        switch (type) {
            case COALESCE:
                kernel_coalesce<<<blockDim, numthreads>>>(curr_visit_d, next_visit_d, vertex_count, vertexList_d, edgeList_d, comp_d, changed_d);
                break;
            case COALESCE_COARSE:
                kernel_coalesce_coarse<<<blockDim, numthreads>>>(curr_visit_d, next_visit_d, vertex_count, vertexList_d, edgeList_d, comp_d, changed_d, coarse);
                break;

            case SCAN:
            {
                /*
                 * Prefix Sum 실행 흐름 (3-pass):
                 * 1. preprocess_kernel: vertex별 페이지 히스토그램 생성
                 * 2. scan: 히스토그램에 block-level prefix sum 적용
                 * 3. scan(intermediate): 블록 간 합산을 위한 intermediate scan
                 * 4. finalsum: intermediate 결과를 각 블록의 prefix sum에 더하여 전역 결과 완성
                 */
                uint64_t k = pc_page_size/sizeof(VertT);  /* 페이지당 원소 수 */
                VertT* outarray_d;
                checkCudaErrors(cudaMalloc((void**)&outarray_d, (n_pages)*sizeof(unsigned long long int)));
                checkCudaErrors(cudaMemset(outarray_d, 0, (n_pages)*sizeof(unsigned long long int)));
                /* Pass 1: vertex별 페이지 히스토그램 생성 */
                preprocess_kernel<<<blockDim, numthreads>>>(vertexList_d, vertex_count, k,n_pages, outarray_d);
                cuda_err_chk(cudaMemcpy(outarray_h, outarray_d, n_pages*sizeof(unsigned long long int), cudaMemcpyDeviceToHost));
                /* Pass 2: 히스토그램에 prefix sum 적용. result_d[1:]에 저장 (result_d[0] = 0으로 exclusive scan 효과) */
                scan<<<blockDim, numthreads>>>(outarray_d, (&result_d[1]), intermediate_d, n_pages);
                /* Pass 3: intermediate에 대해 단일 블록 scan */
                scan<<<dim3(1,1,1), numthreads>>>(intermediate_d, dev2out_d, NULL, BLOCKSIZE*2);
                /* Pass 4: 블록 간 합산으로 전역 prefix sum 완성 */
                finalsum<<<blockDim, numthreads>>>(dev2out_d, (&result_d[1]), n_pages);
                checkCudaErrors(cudaMemcpy(result_h, (result_d), (n_pages+1)*sizeof(VertT), cudaMemcpyDeviceToHost));

                /* 결과 디버그 출력: 처음 64개 값 */
                printf("\n******\n");
                fflush(stdout);
                printf("Vertex list::");
                for (uint64_t i=0; i< 64; i++)
                    printf("%llu\t", vertexList_h[i]);
                printf("\n\nPage reuse hist:");
                for (uint64_t i=0; i< 64; i++)
                    printf("%llu\t", outarray_h[i]);
                printf("\n\nPage reuse Scan result:");
                for (uint64_t i=0; i< 64; i++)
                    printf("%llu\t", result_h[i]);
                printf("\n******\n");
                break;
             }
            case COALESCE_HASH:
                kernel_coalesce_hash<<<blockDim, numthreads>>>(curr_visit_d, next_visit_d, vertex_count, vertexList_d, edgeList_d, comp_d, changed_d, properties.multiProcessorCount);
                break;
            case COALESCE_COARSE_HASH:
                kernel_coalesce_coarse_hash<<<blockDim, numthreads>>>(curr_visit_d, next_visit_d, vertex_count, vertexList_d, edgeList_d, comp_d, changed_d, properties.multiProcessorCount, coarse, stride);
                break;
            case COALESCE_HASH_HALF:
                //printf("blockDim: %d %d numthreads: %d\n", blockDim.x,blockDim.y, numthreads);
                kernel_coalesce_hash_half<<<blockDim, numthreads>>>(curr_visit_d, next_visit_d, vertex_count, vertexList_d, edgeList_d, comp_d, changed_d, stride);
                break;
            default:
                fprintf(stderr, "Invalid type\n");
                exit(1);
                break;
        }

        iter++;

        checkCudaErrors(cudaMemcpy(&changed_h, changed_d, sizeof(bool), cudaMemcpyDeviceToHost));
    } while(changed_h);

    checkCudaErrors(cudaEventRecord(end, 0));
    checkCudaErrors(cudaEventSynchronize(end));
    checkCudaErrors(cudaEventElapsedTime(&milliseconds, start, end));

    /* 결과 검증: component 수 카운트 */
    checkCudaErrors(cudaMemcpy(comp_h, comp_d, vertex_count * sizeof(unsigned long long), cudaMemcpyDeviceToHost));

    for (uint64_t i = 0; i < vertex_count; i++) {
        if (comp_check[comp_h[i]] == false) {
            comp_check[comp_h[i]] = true;
            comp_total++;
        }
    }

    printf("total iterations: %u\n", iter);
    printf("total components: %u\n", comp_total);
    printf("total time: %f ms\n", milliseconds);
    fflush(stdout);

    /* 메모리 해제 */
    free(vertexList_h);

    if((type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
       delete h_pc;
       delete h_range;
       delete h_array;
    }

    if (edgeList_h)
        free(edgeList_h);
    free(comp_check);
    free(comp_h);
    checkCudaErrors(cudaFree(vertexList_d));
    checkCudaErrors(cudaFree(changed_d));
    checkCudaErrors(cudaFree(comp_d));
    checkCudaErrors(cudaFree(curr_visit_d));
    checkCudaErrors(cudaFree(next_visit_d));

    //if(mem == DRAGON_MAP){
    //    if(dragon_unmap(edgeList_dtmp) != D_OK){
    //        printf("Unmap failed for edgelist\n");
    //        return -1;
    //    }
    //}else {
    //    checkCudaErrors(cudaFree(edgeList_d));
    //}


    return 0;
}
