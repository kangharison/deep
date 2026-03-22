/* References:
 *
 *      Coalesce
 *          Hong, Sungpack, et al.
 *          "Accelerating CUDA graph algorithms at maximum warp."
 *          Acm Sigplan Notices 46.8 (2011): 267-276.
 *
 */

/*
 * [BaM cc/scan.cu - CC + Prefix Sum 전처리 벤치마크]
 *
 * 이 파일은 Connected Components(CC) 알고리즘에 Prefix Sum 기반 전처리를 결합한 벤치마크이다.
 * scan/scan.cu와 동일한 코드이며, getopt 기반 커맨드라인 파서를 사용한다.
 *
 * CC 알고리즘 (Label Propagation):
 *   - 각 정점의 comp[v]를 자기 자신의 ID로 초기화
 *   - 매 반복마다 이웃의 comp 값을 비교하여 더 작은 값으로 갱신 (atomicMin)
 *   - 변경이 없을 때까지 반복 -> 같은 연결 요소의 정점들은 동일한 comp 값을 가짐
 *
 * Prefix Sum 전처리 (SCAN 모드):
 *   - preprocess_kernel: 각 정점의 이웃 시작 오프셋이 속하는 페이지 번호를 히스토그램으로 집계
 *   - scan 커널: Blelloch scan(Up-Sweep + Down-Sweep)으로 히스토그램의 prefix sum 계산
 *   - 페이지 재사용 패턴 분석 (BaM 페이지 캐시 효율 측정용)
 *
 * 커널 변형 설명:
 *   - kernel_coalesce: 워프(32 스레드) 단위 그래프 순회
 *   - kernel_coalesce_coarse: 코어스닝 (워프당 여러 정점 처리)
 *   - kernel_coalesce_hash: SM 간 부하 분산을 위한 정점 ID 인터리빙
 *   - kernel_coalesce_hash_half: 16-스레드(하프 워프) 단위 처리
 *   - kernel_coalesce_coarse_hash: 코어스닝 + 해시 결합
 *
 * CSR(Compressed Sparse Row) 포맷:
 *   vertexList[v] ~ vertexList[v+1] 범위가 정점 v의 이웃 정점들의 edgeList 인덱스.
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

#define WARP_SHIFT 5
#define WARP_SIZE 32

#define CHUNK_SHIFT 3
#define CHUNK_SIZE (1 << CHUNK_SHIFT)

#define BLOCK_NUM 1024ULL
#define BLOCKSIZE 512
#define MAXWARP 64

//#define COARSE 4
typedef uint64_t EdgeT;

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

typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    UVM_READONLY_NVLINK = 3,
    UVM_DIRECT_NVLINK = 4,
} mem_type;

typedef uint64_t VertT;


/* preprocess_kernel: 각 정점의 이웃 시작 오프셋이 속하는 페이지 번호를 히스토그램으로 집계 */
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


/* finalsum: 블록 간 prefix sum 결과를 최종 합산 */
__global__ void finalsum (VertT *input, VertT* output, uint64_t len){
   if(blockIdx.x == 0) return;

   uint64_t idx = 2*blockIdx.x *BLOCKSIZE + threadIdx.x;

   if(idx < len)
       output[idx] += input[blockIdx.x -1];
   if((idx + BLOCKSIZE) < len)
       output[idx+BLOCKSIZE] += input[blockIdx.x -1];
}



/* scan: Blelloch scan 커널. Up-Sweep(부분합 축적) + Down-Sweep(부분합 전파)으로 inclusive scan 수행.
 * 각 블록이 2*BLOCKSIZE개 원소를 shared memory에서 처리. */
__global__ void scan(VertT *input, VertT *output, VertT *intermediate,  uint64_t len){

    __shared__ VertT sharedMem[BLOCKSIZE*2];

    uint64_t idx = 2*blockIdx.x * BLOCKSIZE + threadIdx.x;

    if(idx < len)
        sharedMem[threadIdx.x] = input[idx];
    else
        sharedMem[threadIdx.x] = 0;

    if((idx + BLOCKSIZE) < len)
        sharedMem[threadIdx.x + BLOCKSIZE] = input[idx + BLOCKSIZE];
    else
        sharedMem[threadIdx.x + BLOCKSIZE] = 0;

    __syncthreads();

    /* Up-Sweep */
    for(uint64_t stride = 1; stride <= BLOCKSIZE; stride*=2){
        uint64_t index = (threadIdx.x+1)*2*stride -1;
        if(index <2 * BLOCKSIZE)
            sharedMem[index] += sharedMem[index-stride];
        __syncthreads();
    }

    /* Down-Sweep */
    for(uint64_t stride = BLOCKSIZE/2; stride>0; stride/=2){
        uint64_t index = (threadIdx.x+1)*stride*2 - 1;
        if(index + stride < (2*BLOCKSIZE))
            sharedMem[index+stride] += sharedMem[index];
        __syncthreads();
    }

    if(idx<len) output[idx] = sharedMem[threadIdx.x];
    if(idx+BLOCKSIZE < len) output[idx+BLOCKSIZE] = sharedMem[threadIdx.x + BLOCKSIZE];

    if(intermediate !=NULL && threadIdx.x == 0)
        intermediate[blockIdx.x] = sharedMem[2*BLOCKSIZE-1];
}


/* cc_compute: CC 핵심 연산. 두 정점의 comp를 비교하여 더 작은 값으로 갱신 (atomicMin) */
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


/* kernel_coalesce_coarse: 코어스닝 + 워프 기반 CC. 워프당 coarse개 정점을 순차 처리.
 * 같은 SSD 페이지의 연속 정점을 하나의 워프가 처리하여 캐시 히트율을 높인다. */
__global__ void kernel_coalesce_coarse(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed, uint64_t coarse) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x; // 전역 스레드 ID
    const uint64_t warpIdx = tid >> (WARP_SHIFT);  // 워프 인덱스
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1); // 레인 번호

    for(uint64_t j=0; j< coarse; j++){             // coarse개 정점을 순차 처리
        uint64_t cwarpIdx = warpIdx*coarse+j;       // 실제 정점 ID
        if ( (cwarpIdx) < vertex_count) {            // 유효 범위 내
            if(curr_visit[cwarpIdx] == true) {       // 활성 정점만 처리
               const uint64_t start = vertexList[cwarpIdx];              // 인접 리스트 시작
               const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;  // 128B 정렬
               const uint64_t end = vertexList[cwarpIdx+1];              // 인접 리스트 끝

               for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) { // 워프 stride 접근
                   if (i >= start) {                  // 정렬 패딩 건너뛰기
                       const EdgeT next = edgeList[i]; // 이웃 정점 ID 읽기
                       cc_compute(cwarpIdx, comp, next, next_visit, changed); // 라벨 전파
                   }
               }
            }
        }
    }
}

/* kernel_coalesce: 기본 워프 기반 CC. 워프(32스레드)가 하나의 정점을 담당.
 * 32개 레인이 인접 리스트를 stride 접근하여 GPU 메모리 coalescing을 달성한다. */
__global__ void kernel_coalesce(bool *curr_visit, bool *next_visit, uint64_t vertex_count, uint64_t *vertexList, EdgeT *edgeList, unsigned long long *comp, bool *changed) {
    const uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x; // 전역 스레드 ID
    const uint64_t warpIdx = tid >> WARP_SHIFT;    // 워프 인덱스 = 담당 정점 ID
    const uint64_t laneIdx = tid & ((1 << WARP_SHIFT) - 1); // 레인 번호 (0~31)

        if ((warpIdx) < vertex_count) {             // 유효 정점 범위 내
            if(curr_visit[warpIdx] == true) {        // 활성 정점만 처리
               const uint64_t start = vertexList[warpIdx];              // 인접 리스트 시작
               const uint64_t shift_start = start & 0xFFFFFFFFFFFFFFF0;  // 16-원소(128B) 경계 정렬
               const uint64_t end = vertexList[warpIdx+1];              // 인접 리스트 끝

               for(uint64_t i = shift_start + laneIdx; i < end; i += WARP_SIZE) { // 워프 stride 접근
                   if (i >= start) {                  // 정렬 패딩 영역 건너뛰기
                       const EdgeT next = edgeList[i]; // GPU 메모리에서 이웃 정점 읽기
                       cc_compute(warpIdx, comp, next, next_visit, changed); // 라벨 전파
                   }
               }
            }
        }
}



/* kernel_coalesce_hash: 해시 기반 정점 인터리빙으로 SM 간 부하 분산 */
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

/* kernel_coalesce_hash_half: 16-스레드 하프 워프 + 해시 기반 CC */
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

/* kernel_coalesce_coarse_hash: 코어스닝 + 해시 재배치 결합 */
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




__global__ void throttle_memory(uint32_t *pad) {
    pad[1] = pad[0];
}

/* main(): CC + Scan 전처리 벤치마크
 * getopt 기반 커맨드라인 파서를 사용하며 (Settings 클래스의 parseArguments 대신)
 * SCAN 모드에서는 prefix sum으로 페이지 재사용 패턴을 분석한다.
 *
 * 전체 흐름:
 * (1) getopt로 파일명, 커널 타입, 메모리 모드, 페이지 캐시 설정 파싱
 * (2) CSR 그래프 파일 로드 → vertexList, edgeList
 * (3) 메모리 모드별 edgeList 할당 (GPUMEM/UVM/O_DIRECT)
 * (4) CC 반복 루프: SCAN 모드면 히스토그램 + prefix sum, 아니면 CC 커널 실행
 * (5) 결과 출력 (컴포넌트 수, 반복 횟수, 실행 시간)
 */
int main(int argc, char *argv[]) {
    using namespace std::chrono;
    std::ifstream file;                             // 그래프 파일 입력 스트림
    std::string vertex_file, edge_file;            // .col, .dst 파일 경로
    std::string filename;                          // 기본 파일명

    bool changed_h, *changed_d;                    // CC 수렴 검사 플래그 (호스트/디바이스)
    bool *curr_visit_d, *next_visit_d, *comp_check; // 방문 배열(더블 버퍼), 컴포넌트 중복 검사
    int c, arg_num = 0;                            // getopt 변수
    impl_type type;                                // 커널 구현 방식
    mem_type mem;                                  // 메모리 할당 방식
    uint32_t *pad;                                 // 메모리 스로틀링용
    uint32_t iter, comp_total = 0;                 // CC 반복 카운터, 컴포넌트 수
    unsigned long long *comp_d, *comp_h;           // 컴포넌트 라벨 배열
    uint64_t *vertexList_h, *vertexList_d;         // CSR 정점 오프셋 배열
    EdgeT *edgeList_h, *edgeList_d;                // CSR 간선 배열
    uint64_t vertex_count, edge_count, vertex_size, edge_size; // 그래프 크기
    uint64_t typeT;                                // 파일 헤더 타입 (미사용)
    uint64_t numblocks, numthreads;                // CUDA 그리드/블록 차원
    size_t freebyte, totalbyte;                    // GPU 메모리 조회용
    EdgeT *edgeList_dtmp;

    float milliseconds;                            // 실행 시간

    Settings settings;
    uint64_t pc_page_size = 4096;                  // 페이지 캐시 페이지 크기 (기본 4KB)
    uint64_t pc_pages = 2*1024*1024;               // 페이지 캐시 엔트리 수 (2M * 4KB = 8GB)

    uint64_t stride;                               // 해시 인터리빙 stride
    uint64_t coarse;                               // 코어스닝 팩터

    cudaEvent_t start, end;                        // CUDA 이벤트 타이밍

    while ((c = getopt(argc, argv, "f:t:m:p:n:s:l:h")) != -1) {
        switch (c) {
            case 'f':
                filename = optarg;
                arg_num++;
                break;
            case 't':
                type = (impl_type)atoi(optarg);
                arg_num++;
                break;
            case 'm':
                mem = (mem_type)atoi(optarg);
                arg_num++;
                break;
            case 'p':
                //Need to add type condition check.
                pc_page_size = atoi(optarg);
                arg_num++;
                break;
            case 'n':
                pc_pages = atoi(optarg);
                arg_num++;
                break;
            case 's':
                stride = (uint64_t) atoi(optarg);
                arg_num++;
                break;
            case 'l':
                coarse = (uint64_t) atoi(optarg);
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

    vertex_file = filename + ".col";
    edge_file = filename + ".dst";

    std::cout << filename << std::endl;
    fprintf(stderr, "File %s\n", filename.c_str());

    /* CSR 그래프 로드 */
    file.open(vertex_file.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        printf("vertex file open failed\n");
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
        printf("edge file open failed\n");
        exit(1);
    };

    file.read((char*)(&edge_count), 8);
    file.read((char*)(&typeT), 8);

    printf("Edge: %llu\n", edge_count);
    fflush(stdout);
    edge_size = edge_count * sizeof(EdgeT);
    edge_size = edge_size + (4096 - (edge_size & 0xFFFULL));

    edgeList_h = NULL;

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
            break;
        case UVM_DIRECT:
            {
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
    }

    file.close();

    /* CC 초기화 */
    comp_h = (unsigned long long*)malloc(vertex_count * sizeof(unsigned long long));
    comp_check = (bool*)malloc(vertex_count * sizeof(bool));
    checkCudaErrors(cudaMalloc((void**)&vertexList_d, vertex_size));
    checkCudaErrors(cudaMalloc((void**)&curr_visit_d, vertex_count * sizeof(bool)));
    checkCudaErrors(cudaMalloc((void**)&next_visit_d, vertex_count * sizeof(bool)));
    checkCudaErrors(cudaMalloc((void**)&comp_d, vertex_count * sizeof(unsigned long long)));
    checkCudaErrors(cudaMalloc((void**)&changed_d, sizeof(bool)));

    printf("Allocation finished\n");
    fflush(stdout);

    for (uint64_t i = 0; i < vertex_count; i++)
        comp_h[i] = i;

    memset(comp_check, 0, vertex_count * sizeof(bool));

    checkCudaErrors(cudaMemset(curr_visit_d, 0x01, vertex_count * sizeof(bool)));
    checkCudaErrors(cudaMemset(next_visit_d, 0x00, vertex_count * sizeof(bool)));
    checkCudaErrors(cudaMemcpy(comp_d, comp_h, vertex_count * sizeof(uint64_t), cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(vertexList_d, vertexList_h, vertex_size, cudaMemcpyHostToDevice));

    if (mem == GPUMEM)
        checkCudaErrors(cudaMemcpy(edgeList_d, edgeList_h, edge_size, cudaMemcpyHostToDevice));

    numthreads = BLOCKSIZE;

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

    dim3 blockDim(BLOCK_NUM, (numblocks+BLOCK_NUM)/BLOCK_NUM);

    printf("Initialization done\n");
    fflush(stdout);

    if((type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
            printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
    }

    page_cache_t* h_pc;
    range_t<uint64_t>* h_range;
    std::vector<range_t<uint64_t>*> vec_range(1);
    array_t<uint64_t>* h_array;

    uint64_t n_pages = ceil(((float)edge_size)/pc_page_size);//fix needed. TODO
    if((type == COALESCE_PC) ||(type == COALESCE_CHUNK_PC)){
       h_pc =new page_cache_t(pc_page_size, pc_pages, settings, (uint64_t) 1);
       h_range = new range_t<uint64_t>((int)0 ,(uint64_t)edge_count, (int) 0,(uint64_t)n_pages, (int)0, (uint64_t)pc_page_size, h_pc, settings, (uint8_t*)edgeList_d);
       vec_range[0] = h_range;
       h_array = new array_t<uint64_t>(edge_count, 0, vec_range, settings);

       printf("Page cache initialized\n");
       fflush(stdout);
    }

    iter = 0;
    checkCudaErrors(cudaEventRecord(start, 0));

    /* CC 반복 루프 */
    do {
        changed_h = false;
        checkCudaErrors(cudaMemcpy(changed_d, &changed_h, sizeof(bool), cudaMemcpyHostToDevice));

        VertT* intermediate_d;
        VertT* dev2out_d;
        VertT* result_d;
        VertT* outarray_h;
        VertT* result_h;
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
                /* 페이지 히스토그램 + prefix sum 수행 */
                uint64_t k = pc_page_size/sizeof(VertT);
                VertT* outarray_d;
                checkCudaErrors(cudaMalloc((void**)&outarray_d, (n_pages)*sizeof(unsigned long long int)));
                checkCudaErrors(cudaMemset(outarray_d, 0, (n_pages)*sizeof(unsigned long long int)));
                preprocess_kernel<<<blockDim, numthreads>>>(vertexList_d, vertex_count, k,n_pages, outarray_d);
                cuda_err_chk(cudaMemcpy(outarray_h, outarray_d, n_pages*sizeof(unsigned long long int), cudaMemcpyDeviceToHost));
                scan<<<blockDim, numthreads>>>(outarray_d, (&result_d[1]), intermediate_d, n_pages);
                scan<<<dim3(1,1,1), numthreads>>>(intermediate_d, dev2out_d, NULL, BLOCKSIZE*2);
                finalsum<<<blockDim, numthreads>>>(dev2out_d, (&result_d[1]), n_pages);
                checkCudaErrors(cudaMemcpy(result_h, (result_d), (n_pages+1)*sizeof(VertT), cudaMemcpyDeviceToHost));

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


    return 0;
}
