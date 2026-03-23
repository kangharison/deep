/*
 * =============================================================================
 * BaM Reduction 벤치마크 (benchmarks/reduction/main.cu)
 * =============================================================================
 *
 * [목적]
 * NVMe SSD에 저장된 대규모 uint64_t 배열에 대해 GPU에서 parallel reduction(합산)을
 * 수행하는 벤치마크이다. GPU 메모리, UVM(Unified Virtual Memory), BaFS(BaM File System)
 * 등 다양한 메모리 할당 모드에서 동일 커널을 실행하여 I/O 경로별 성능을 비교한다.
 *
 * [지원 모드]
 * - GPUMEM (0):       호스트에서 파일을 읽고 cudaMemcpy로 GPU 메모리에 복사 후 연산
 * - UVM_READONLY (1): cudaMallocManaged + SetReadMostly 힌트로 UVM 페이지 캐싱 활용
 * - UVM_DIRECT (2):   cudaMallocManaged + SetAccessedBy 힌트로 GPU가 직접 접근 (zero-copy 유사)
 * - BAFS_DIRECT (6):  BaM page cache를 통해 GPU가 NVMe SSD에서 직접 데이터를 fetch
 *
 * [커널 구현 종류]
 * - BASELINE (0):      블록 단위 shared memory reduction, 각 블록이 2*blockDim.x개 원소 처리
 * - OPTIMIZED (1):     워프 단위 page-aligned reduction, 한 워프가 2개 NVMe 페이지를 처리
 * - BASELINE_PC (2):   BASELINE과 동일 알고리즘이지만 bam_ptr로 NVMe SSD에서 on-demand fetch
 * - OPTIMIZED_PC (3):  OPTIMIZED와 동일 알고리즘이지만 bam_ptr로 NVMe SSD에서 on-demand fetch
 *
 * [I/O 흐름 (BAFS_DIRECT 모드)]
 * 1. Controller 객체가 /dev/libnvm* 디바이스를 열어 NVMe 컨트롤러에 직접 접근
 * 2. page_cache_t가 GPU 메모리에 페이지 캐시를 생성
 * 3. range_t로 파일의 논리적 범위를 NVMe LBA에 매핑
 * 4. array_t가 range들을 묶어 하나의 논리 배열로 추상화
 * 5. 커널 내부에서 bam_ptr[i] 접근 시 page cache miss가 발생하면
 *    GPU 스레드가 직접 NVMe SQ에 I/O 커맨드를 submit하고 CQ에서 완료를 polling
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

/* BaM/libnvm 관련 헤더: NVMe 컨트롤러 제어, 큐 관리, I/O 커맨드 발행 */
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
#include <page_cache.h>      /* page_cache_t, range_t, array_t, bam_ptr 등 BaM 핵심 타입 정의 */
#include <util.h>

#include <iterator>
#include <numeric>
#include <functional>
#include <cub/cub.cuh>


#define UINT64MAX 0xFFFFFFFFFFFFFFFF

using error = std::runtime_error;
using std::string;

/* BaM 커널 모듈이 노출하는 NVMe 컨트롤러 디바이스 경로 목록. BAFS_DIRECT 모드에서 settings.n_ctrls개만큼 사용한다. */
//const char* const ctrls_paths[] = {"/dev/libnvmpro0", "/dev/libnvmpro1", "/dev/libnvmpro2", "/dev/libnvmpro3", "/dev/libnvmpro4", "/dev/libnvmpro5", "/dev/libnvmpro6", "/dev/libnvmpro7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm4", "/dev/libnvm9", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8"};

#define WARP_SHIFT 5       /* log2(WARP_SIZE) = 5, 워프 내 lane 인덱스 계산용 시프트 값 */
#define WARP_SIZE 32        /* NVIDIA GPU 워프 크기: 32 스레드가 SIMT로 동시 실행 */

#define CHUNK_SHIFT 3       /* log2(CHUNK_SIZE) = 3, chunk 기반 coalesced 접근 시 사용 */
#define CHUNK_SIZE (1 << CHUNK_SHIFT)  /* 8개 원소 단위의 chunk 크기 */

#define BLOCK_NUM 1024ULL   /* 2D 그리드 사용 시 x 방향 블록 수 상한 */

#define MAXWARP 64          /* SM당 상주 가능한 최대 워프 수 (occupancy 제한용 상수) */

/* 배열 원소 타입: 8바이트 unsigned integer. NVMe 페이지 하나에 page_size/8개 원소가 들어간다. */
typedef uint64_t ElemT;

/*
 * 커널 구현 타입 열거형 (커맨드라인 -v 옵션으로 선택):
 * - BASELINE(0):     블록 단위 reduction. 각 블록이 2*blockDim.x개 원소를 shared memory에 로드 후 tree reduction
 * - OPTIMIZED(1):    워프 단위 page-aligned reduction. 한 워프가 2개 NVMe 페이지(page_size 바이트)를 처리
 * - BASELINE_PC(2):  BASELINE 알고리즘 + BaM page cache(bam_ptr<ElemT>) 사용. GPU가 NVMe에서 직접 fetch
 * - OPTIMIZED_PC(3): OPTIMIZED 알고리즘 + BaM page cache(bam_ptr<ElemT>) 사용
 */
typedef enum {
    BASELINE = 0,
    OPTIMIZED=1,
    BASELINE_PC = 2,
    OPTIMIZED_PC=3,
} impl_type;

/*
 * 메모리 할당 모드 열거형 (커맨드라인 -m 옵션으로 선택):
 * - GPUMEM(0):       cudaMalloc으로 GPU 전용 VRAM 할당 후 cudaMemcpy로 호스트->디바이스 복사
 * - UVM_READONLY(1): cudaMallocManaged + cudaMemAdviseSetReadMostly. GPU 페이지 테이블에 읽기 전용 복제본 캐싱
 * - UVM_DIRECT(2):   cudaMallocManaged + cudaMemAdviseSetAccessedBy. O_DIRECT로 파일을 UVM 영역에 직접 읽고 GPU 접근
 * - BAFS_DIRECT(6):  BaM page cache를 통해 GPU 커널이 NVMe SSD에서 on-demand로 4KB~64KB 페이지를 fetch
 */
typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    BAFS_DIRECT= 6,
} mem_type;



/*
 * kernel_reduce_baseline: 기본 블록 단위 parallel reduction 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - 그리드: numblocks = ceil(n_elems / (2 * numthreads))개 블록
 * - 블록: numthreads개 스레드 (shared memory 크기 = 2 * numthreads * sizeof(ElemT))
 * - 각 블록이 2 * blockDim.x개 원소를 담당
 *
 * [데이터 접근 패턴]
 * - 각 스레드가 input[start+tid]와 input[blockDim.x+start+tid] 두 원소를 shared memory에 로드
 * - stride를 blockDim.x에서 1까지 반씩 줄이면서 tree reduction 수행
 * - 최종 결과는 output[blockIdx.x]에 저장 (블록당 하나의 partial sum)
 *
 * [알고리즘]
 * Brent-Kung 스타일 parallel reduction: O(n/p + log p) 시간복잡도
 * shared memory bank conflict를 줄이기 위해 sequential addressing 사용
 */
__global__
void kernel_reduce_baseline(ElemT *input, ElemT *output, uint64_t len){

    extern __shared__ ElemT sharedMem[];

    uint64_t tid = threadIdx.x;
    /* start: 이 블록이 담당하는 입력 배열의 시작 인덱스. 블록당 2*blockDim.x개 원소를 처리하므로 2배를 곱함 */
    uint64_t start = 2*blockDim.x*blockIdx.x;

    /* 첫 번째 절반: shared memory[0..blockDim.x-1]에 input의 전반부 로드 */
    if((start+tid) < len)
        sharedMem[tid] = input[start+tid];
    else
        sharedMem[tid] = 0;

    /* 두 번째 절반: shared memory[blockDim.x..2*blockDim.x-1]에 input의 후반부 로드 */
    if((blockDim.x+start+tid) < len)
        sharedMem[blockDim.x+tid] = input[blockDim.x+start+tid];
    else
        sharedMem[blockDim.x+tid] = 0;

    /* Tree reduction: stride를 blockDim.x부터 1까지 반씩 줄이면서 인접 원소 합산 */
    for(uint64_t stride = blockDim.x; stride>0; stride >>=1){
        __syncthreads();

        if( tid < stride)
            sharedMem[tid] += sharedMem[stride+tid];
    }

    /* 블록의 최종 partial sum을 output 배열에 기록. 호스트에서 이 partial sum들을 다시 합산해야 전체 합을 얻는다 */
    if(threadIdx.x == 0)
        output[blockIdx.x] = sharedMem[0];

}

/*
 * kernel_reduce_optimized: 워프 단위 page-aligned reduction 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - n_warps = ceil(n_pages / 2): 워프 수 (각 워프가 2개 NVMe 페이지를 담당)
 * - 그리드: numblocks = ceil(n_warps * 32 / numthreads)
 * - 블록: numthreads개 스레드
 *
 * [데이터 접근 패턴 - NVMe 페이지 정렬 접근]
 * - 각 워프가 2개 연속 NVMe 페이지(각 page_size 바이트, page_size/8개 uint64_t 원소)를 처리
 * - 워프 내 32개 lane이 stride=32로 페이지 내 원소를 순회하며 shared memory에 로드
 * - 이 패턴은 NVMe 페이지 경계에 정렬되어 BaM page cache의 hit rate를 극대화
 *
 * [알고리즘]
 * 워프 내에서 __syncwarp()로 동기화하며 tree reduction 수행.
 * 블록 내 동기화(__syncthreads) 대신 워프 동기화만 사용하여 오버헤드 감소.
 */
__global__
void kernel_reduce_optimized(ElemT *input, ElemT *output, uint64_t len, uint64_t page_size, uint64_t n_warps){
    extern __shared__ ElemT sharedMem[];

    uint64_t tid = blockDim.x*blockIdx.x+threadIdx.x;
    uint64_t laneid = tid % WARP_SIZE;    /* 워프 내 lane 인덱스 (0~31) */
    uint64_t warp_id = tid / WARP_SIZE;   /* 전역 워프 ID */
    uint64_t num_elems_per_cl = page_size/sizeof(ElemT); /* 한 NVMe 페이지에 들어가는 원소 수 (예: 4096/8 = 512) */

    /* start: 이 워프가 담당하는 입력 배열의 시작 인덱스. 워프당 2개 페이지를 처리 */
    uint64_t start = 2*warp_id * num_elems_per_cl;

    /* 워프 내 32개 lane이 stride=WARP_SIZE로 페이지 내 모든 원소를 shared memory에 로드 */
    for(size_t i=laneid; i<num_elems_per_cl;i+=WARP_SIZE){

        if(((start+i)<len))
            sharedMem[i] = input[start+i];
        else
            sharedMem[i] =0;

        if((num_elems_per_cl+start+i)<len)
            sharedMem[num_elems_per_cl+i] = input[num_elems_per_cl+start+i];
        else
            sharedMem[i] =0;

        __syncwarp(); /* 워프 내 모든 lane이 현재 batch 로드를 완료할 때까지 대기 */
    }

    /* Tree reduction: stride를 num_elems_per_cl에서 1까지 반씩 줄이면서 합산 */
    for(uint64_t stride = num_elems_per_cl; stride>0; stride>>=1){
        __syncwarp();

        for(size_t i =0; i<num_elems_per_cl; i+=WARP_SIZE){
            uint64_t idx = i*WARP_SIZE + laneid;

            if(idx <stride)
                sharedMem[idx] += sharedMem[stride+idx];
        }
    }

    /* 워프의 최종 partial sum을 output[warp_id]에 기록 */
    if(laneid == 0)
        output[warp_id] = sharedMem[0];
}



/*
 * kernel_reduce_baseline_ptr_pc: BASELINE reduction + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr 사용]
 * - 입력이 ElemT* 포인터 대신 array_d_t<ElemT>* (디바이스 측 배열 디스크립터)로 전달됨
 * - bam_ptr<ElemT> input(da)로 스마트 포인터를 생성하면, input[i] 접근 시:
 *   1) page_cache에서 해당 페이지가 GPU 메모리에 있는지 확인 (cache lookup)
 *   2) cache hit이면 GPU 메모리의 캐시된 페이지에서 직접 읽음
 *   3) cache miss이면 GPU 스레드가 NVMe SQ에 READ 커맨드를 submit하고,
 *      CQ polling으로 완료를 기다린 뒤 캐시에 페이지를 올리고 데이터를 반환
 * - 나머지 reduction 알고리즘은 kernel_reduce_baseline과 완전히 동일
 *
 * [I/O 흐름]
 * bam_ptr[i] -> page_cache lookup -> miss시 NVMe I/O -> GPU 메모리 캐시 -> 데이터 반환
 */
__global__
void kernel_reduce_baseline_ptr_pc(array_d_t<ElemT> *da, ElemT *output, uint64_t len){

    extern __shared__ ElemT sharedMem[];

    /* bam_ptr: BaM의 핵심 추상화. array_d_t를 감싸서 operator[]로 투명한 NVMe 접근을 제공 */
    bam_ptr<ElemT> input(da);

    uint64_t tid = threadIdx.x;
    uint64_t start = 2*blockDim.x*blockIdx.x;

    if((start+tid) < len)
        sharedMem[tid] = input[start+tid];     /* bam_ptr의 operator[]: cache miss시 NVMe I/O 발생 */
    else
        sharedMem[tid] = 0;

    if((blockDim.x+start+tid) < len)
        sharedMem[blockDim.x+tid] = input[blockDim.x+start+tid];
    else
        sharedMem[blockDim.x+tid] = 0;

    for(uint64_t stride = blockDim.x; stride>0; stride >>=1){
        __syncthreads();

        if( tid < stride)
            sharedMem[tid] += sharedMem[stride+tid];
    }

    if(threadIdx.x == 0)
        output[blockIdx.x] = sharedMem[0];

}

/*
 * kernel_reduce_optimized_ptr_pc: OPTIMIZED (워프 단위 page-aligned) reduction + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr + 페이지 정렬 접근]
 * - kernel_reduce_optimized와 동일한 워프 단위 page-aligned 알고리즘이지만
 *   입력 데이터를 bam_ptr<ElemT>을 통해 NVMe SSD에서 직접 fetch
 * - 각 워프가 2개 NVMe 페이지를 담당하고, 워프 내 lane들이 같은 페이지를 동시 접근하므로
 *   BaM page cache에서 같은 페이지에 대한 중복 I/O가 방지됨 (coalesced NVMe access)
 * - 이것이 BaM 논문의 핵심 최적화: GPU 워프 스케줄링과 NVMe 페이지 접근을 정렬하여
 *   page cache hit rate와 NVMe bandwidth utilization을 동시에 극대화
 */
__global__
void kernel_reduce_optimized_ptr_pc(array_d_t<ElemT> *da, ElemT *output, uint64_t len, uint64_t page_size, uint64_t n_warps){
    extern __shared__ ElemT sharedMem[];
    bam_ptr<ElemT> input(da);

    uint64_t tid = blockDim.x*blockIdx.x+threadIdx.x;
    uint64_t laneid = tid % WARP_SIZE;
    uint64_t warp_id = tid / WARP_SIZE;
    uint64_t num_elems_per_cl = page_size/sizeof(ElemT);

    uint64_t start = 2*warp_id * num_elems_per_cl;

    for(size_t i=laneid; i<num_elems_per_cl;i+=WARP_SIZE){

        if(((start+i)<len))
            sharedMem[i] = input[start+i];      /* bam_ptr[]: page cache miss시 NVMe READ 발행 */
        else
            sharedMem[i] =0;

        if((num_elems_per_cl+start+i)<len)
            sharedMem[num_elems_per_cl+i] = input[num_elems_per_cl+start+i];
        else
            sharedMem[i] =0;

        __syncwarp();
    }

    for(uint64_t stride = num_elems_per_cl; stride>0; stride>>=1){
        __syncwarp();

        for(size_t i =0; i<num_elems_per_cl; i+=WARP_SIZE){
            uint64_t idx = i*WARP_SIZE + laneid;

            if(idx <stride)
                sharedMem[idx] += sharedMem[stride+idx];
        }
    }

    if(laneid == 0)
        output[warp_id] = sharedMem[0];
}

/*
 * =============================================================================
 * main 함수: 벤치마크 설정 파싱 -> 데이터 로드 -> 커널 실행 -> 결과 검증
 * =============================================================================
 */
int main(int argc, char *argv[]) {
    using namespace std::chrono;

    /* 커맨드라인 인자 파싱: settings.h에 정의된 Settings 구조체가 모든 옵션을 관리 */
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

    std::ifstream filea, fileb;
    std::string a_file;
    std::string a_file_bin;
    std::string filename;

    impl_type type;
    mem_type mem;
    uint64_t *a_h, *a_d;
    uint64_t numblocks, numthreads;

    float milliseconds;

    uint64_t pc_page_size;       /* BaM page cache의 페이지 크기 (바이트). 기본값 4096 */
    uint64_t pc_pages;           /* page cache에 보관할 총 페이지 수. maxPageCacheSize / pageSize로 계산 */

    try{

        a_file = std::string(settings.input_a);

        type = (impl_type) settings.type;
        mem = (mem_type) settings.memalloc;

        pc_page_size = settings.pageSize;
        pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size);

        numthreads = settings.numThreads;

        cuda_err_chk(cudaSetDevice(settings.cudaDevice));

        /* CUDA 이벤트: 커널 실행 시간 측정용 */
        cudaEvent_t start, end, tstart, tend;
        cuda_err_chk(cudaEventCreate(&start));
        cuda_err_chk(cudaEventCreate(&end));
        cuda_err_chk(cudaEventCreate(&tstart));
        cuda_err_chk(cudaEventCreate(&tend));


        /* 입력 파일 경로: .bel 그래프 파일의 .dst (destination/edge) 확장자를 사용 */
        a_file_bin = a_file + ".dst";

        std::cout << "A: " << a_file_bin  << std::endl;

        uint64_t n_elems = settings.n_elems;
        uint64_t n_elems_size = n_elems * sizeof(uint64_t);
        printf("Total elements: %llu \n", n_elems);
        uint64_t tmp;

        /* 파일 읽기: 처음 16바이트는 헤더(vertex_count, typeT)이므로 스킵 */
        filea.open(a_file_bin.c_str(), std::ios::in | std::ios::binary);
        if (!filea.is_open()) {
            printf("A file open failed\n");
            exit(1);
        };

        filea.read((char*)(&tmp), 16);
        if(mem != BAFS_DIRECT)
            a_h = (uint64_t*)calloc(n_elems_size, sizeof(uint64_t));
        if((mem!=BAFS_DIRECT) &&  (mem != UVM_DIRECT)){
             filea.read((char*)a_h, n_elems_size);
             filea.close();
        }

        /*
         * 메모리 할당 모드별 데이터 로드 경로:
         * - GPUMEM:       호스트 메모리 -> cudaMemcpy -> GPU VRAM
         * - UVM_READONLY: UVM 관리 메모리에 복사 후 SetReadMostly 힌트 설정
         * - UVM_DIRECT:   O_DIRECT로 파일을 UVM 영역에 직접 fread, SetAccessedBy 힌트로 GPU 접근
         * - BAFS_DIRECT:  파일 로드 불필요 (커널에서 bam_ptr이 on-demand로 NVMe에서 fetch)
         */
        switch (mem) {
            case GPUMEM:
                {
                cuda_err_chk(cudaMalloc((void**)&a_d, n_elems_size));
                high_resolution_clock::time_point mc1 = high_resolution_clock::now();
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                high_resolution_clock::time_point mc2 = high_resolution_clock::now();
                duration<double> mc_time_span = duration_cast<duration<double>>(mc2 -mc1);
                std::cout<< "Memcpy time for loading the inputs: "<< mc_time_span.count() <<std::endl;
                break;
                }
            case UVM_READONLY:
                {
                cuda_err_chk(cudaMallocManaged((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                /* SetReadMostly: CUDA 런타임이 GPU 메모리에 읽기 전용 복제본을 캐싱하도록 힌트 */
                cuda_err_chk(cudaMemAdvise(a_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                break;
                }
            case UVM_DIRECT:
                {
                filea.close();
                fileb.close();
                /* O_DIRECT: OS 페이지 캐시를 우회하여 디스크에서 직접 UVM 영역으로 읽음 */
                int fda = open(a_file_bin.c_str(), O_RDONLY | O_DIRECT);
                FILE *fa_tmp= fdopen(fda, "rb");
                if ((fa_tmp == NULL) || (fda == -1)) {
                    printf("A file fd open failed\n");
                    exit(1);
                }

                /* 4K 정렬: O_DIRECT는 버퍼가 섹터 크기(4KB)에 정렬되어야 함 */
                uint64_t count_4k_aligned = ((n_elems + 2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t));
                //uint64_t count_4k_aligned = n_elems;
                uint64_t size_4k_aligned = count_4k_aligned * sizeof(uint64_t);

                cuda_err_chk(cudaMallocManaged((void**)&a_d, size_4k_aligned));
                /* SetAccessedBy: GPU가 이 메모리에 직접 접근할 것임을 CUDA 드라이버에 알림. 페이지 마이그레이션 최적화 */
                cuda_err_chk(cudaMemAdvise(a_d, size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();

                if (fread(a_d, sizeof(uint64_t), count_4k_aligned, fa_tmp)) {
                    printf("A file fread failed: %llu \t %llu\n", count_4k_aligned, n_elems+2);
                    exit(1);
                }
                fclose(fa_tmp);
                close(fda);

                /* 헤더 2개 uint64_t를 건너뛰기 위해 포인터를 +2 이동 */
                a_d = a_d + 2;

                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "file read time: "<< time_span.count() <<std::endl;

                break;
                }
            case BAFS_DIRECT:
                {
                /* BAFS_DIRECT: 파일을 미리 로드하지 않음. 커널에서 bam_ptr이 on-demand로 NVMe에서 fetch */
                break;
                }
        }


        /* 전체 데이터가 차지하는 NVMe 페이지 수 계산 */
        uint64_t n_pages = ceil(((float)n_elems_size)/pc_page_size);

        uint64_t n_warps = 0;

        /*
         * 커널 타입에 따른 그리드 크기(numblocks) 계산:
         * - BASELINE/BASELINE_PC: 각 블록이 2*numthreads개 원소 처리 -> numblocks = ceil(n_elems / (2*numthreads))
         * - OPTIMIZED/OPTIMIZED_PC: 각 워프가 2개 NVMe 페이지 처리 -> n_warps = ceil(n_pages/2), numblocks = ceil(n_warps*32/numthreads)
         */
        switch (type) {
            case BASELINE:
            case BASELINE_PC:{
                numblocks = ((n_elems/(2*numthreads)) + 1);
                break;
            }
            case OPTIMIZED:
            case OPTIMIZED_PC:{
                uint64_t num_elems_per_cl = pc_page_size/sizeof(ElemT);
                n_warps = ceil(n_pages/2);
                numblocks = ceil(n_warps*WARP_SIZE/numthreads);
                break;
            }

            default:
                fprintf(stderr, "Invalid type\n");
                exit(1);
                break;
        }

        //dim3 blockDim(BLOCK_NUM, (numblocks+BLOCK_NUM)/BLOCK_NUM);
        dim3 blockDim((numblocks));
        if((type == BASELINE_PC)) {
                printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
        }

        /* GPU 결과 저장용 메모리 할당: 각 블록/워프의 partial sum을 저장 */
        ElemT *result_h;
        ElemT *result_d;
        ElemT *dev2out_d;
        ElemT *int_d;

        cuda_err_chk(cudaMalloc((void**)&int_d, (numblocks)*sizeof(ElemT)));
        cuda_err_chk(cudaMalloc((void**)&dev2out_d, (numblocks)*sizeof(ElemT)));
        cuda_err_chk(cudaMalloc((void**)&result_d, (n_elems+1)*sizeof(ElemT)));
        result_h = (ElemT*) malloc(n_elems* sizeof(ElemT));
        cuda_err_chk(cudaMemset(result_d, 0, (n_elems+1)*sizeof(ElemT)));

		printf("Allocation finished\n");
        fflush(stdout);

        /*
         * BaM NVMe 컨트롤러 초기화 (BAFS_DIRECT 모드 전용):
         * Controller 객체가 /dev/libnvm* 캐릭터 디바이스를 열고, NVMe admin 큐를 통해
         * 컨트롤러를 초기화하며, I/O 큐(SQ/CQ)를 GPU 접근 가능한 메모리에 할당한다.
         * queueDepth와 numQueues 파라미터로 큐 크기와 개수를 제어한다.
         */
        std::vector<Controller*> ctrls(settings.n_ctrls);
        if(mem == BAFS_DIRECT){
            cuda_err_chk(cudaSetDevice(settings.cudaDevice));
            for (size_t i = 0 ; i < settings.n_ctrls; i++)
                ctrls[i] = new Controller(ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);
            printf("Controllers Created\n");
        }
        printf("Initialization done\n");
        fflush(stdout);

        /*
         * BaM page cache 및 array 추상화 초기화 (BASELINE_PC / OPTIMIZED_PC 전용):
         *
         * page_cache_t(pc_page_size, pc_pages, cudaDevice, ctrl, 64, ctrls):
         *   - GPU 메모리에 pc_pages개의 페이지 슬롯(각 pc_page_size 바이트)을 할당
         *   - 64는 배치 I/O 시 한번에 처리할 최대 페이지 수
         *   - LRU 기반 eviction 정책으로 캐시 관리
         *
         * range_t<uint64_t>(start_elem, n_elems, start_page, n_pages, 0, page_size, pc, device):
         *   - 논리적 원소 범위 [0, n_elems)를 NVMe LBA 범위에 매핑
         *   - afileoffset: SSD 내 파일 데이터의 시작 오프셋 (바이트)
         *
         * array_t<uint64_t>(n_elems, fileoffset, ranges, device):
         *   - 하나 이상의 range_t를 묶어 단일 논리 배열로 추상화
         *   - d_array_ptr: 디바이스(GPU)에서 접근 가능한 array descriptor 포인터
         */
        page_cache_t* h_pc;
        range_t<uint64_t>* h_Arange;
        std::vector<range_t<uint64_t>*> vec_Arange(1);
        array_t<uint64_t>* h_Aarray;


        if((type == BASELINE_PC) || (type == OPTIMIZED_PC)) {
            //TODO: fix for 2 arrays
            h_pc =new page_cache_t(pc_page_size, pc_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
            h_Arange = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)n_elems, (uint64_t) (ceil(settings.afileoffset*1.0/pc_page_size)),(uint64_t)n_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice);
            vec_Arange[0] = h_Arange;
            h_Aarray = new array_t<uint64_t>(n_elems, settings.afileoffset, vec_Arange, settings.cudaDevice);

            printf("Page cache initialized\n");
            fflush(stdout);
        }

        void *d_tmp = NULL;
        size_t tmp_size =0;

        /* 벤치마크 실행 루프 (현재 1회 반복) */
        for(int titr=0; titr<1; titr+=1){
            cuda_err_chk(cudaEventRecord(start, 0));

            auto itrstart = std::chrono::system_clock::now();

            /* 커널 타입에 따라 해당 reduction 커널을 launch */
            switch (type) {
                case BASELINE:{
                    printf("launching PC: blockDim.x :%llu blockDim.y :%llu numthreads:%llu sharedMemSize: %llu\n", blockDim.x, blockDim.y, numthreads, 2*numblocks*sizeof(ElemT));
                    /* shared memory 크기: 2 * numthreads * sizeof(ElemT). 블록당 2배 원소를 로드하므로 */
                    kernel_reduce_baseline<<<blockDim, numthreads, 2*numthreads*sizeof(ElemT)>>>(a_d, result_d, n_elems);
                    break;
                }
                case OPTIMIZED:{
                    printf("launching PC: blockDim.x :%llu blockDim.y :%llu numthreads:%llu sharedMemSize: %llu\n", blockDim.x, blockDim.y, numthreads, 2*pc_page_size);
                    /* shared memory 크기: 2 * pc_page_size. 워프당 2개 NVMe 페이지 분량 */
                    kernel_reduce_optimized<<<blockDim, numthreads, 2*pc_page_size>>>(a_d, result_d, n_elems, settings.pageSize, n_warps);
                    break;

                    }
                case BASELINE_PC:{

                    printf("launching PC: blockDim.x :%llu blockDim.y :%llu numthreads:%llu\n", blockDim.x, blockDim.y, numthreads);
                    /* h_Aarray->d_array_ptr: GPU에서 접근 가능한 array_d_t 포인터. bam_ptr의 입력이 됨 */
                    kernel_reduce_baseline_ptr_pc<<<blockDim, numthreads, 2*numthreads*sizeof(ElemT)>>>(h_Aarray->d_array_ptr, result_d, n_elems);
                    break;
                }
                case OPTIMIZED_PC:{
                    printf("launching PC: blockDim.x :%llu blockDim.y :%llu numthreads:%llu sharedMemSize: %llu\n", blockDim.x, blockDim.y, numthreads, 2*pc_page_size);
                    kernel_reduce_optimized_ptr_pc<<<blockDim, numthreads, 2*pc_page_size>>>(h_Aarray->d_array_ptr, result_d, n_elems, settings.pageSize, n_warps);
                    break;
                }
                default:
                    fprintf(stderr, "Invalid type\n");
                    exit(1);
                    break;
            }
            cuda_err_chk(cudaEventRecord(end, 0));
            cuda_err_chk(cudaEventSynchronize(end));
            cuda_err_chk(cudaEventElapsedTime(&milliseconds, start, end));

            /* 결과를 호스트로 복사하여 검증 */
            cuda_err_chk(cudaMemcpy(result_h, (result_d), (n_elems+1)*sizeof(ElemT), cudaMemcpyDeviceToHost));
            //printf("\n******\n");
            //fflush(stdout);
            //if(mem != BAFS_DIRECT){
            //   printf("Input list::");
            //   for (uint64_t i=n_elems-100; i< n_elems; i++)
            //       printf("%llu\t", a_h[i]);
            //}
            //printf("\n\nScan result:");
            //for (uint64_t i=n_elems-100; i< n_elems; i++)
            //    printf("%llu\t", result_h[i]);
            //printf("\n******\n");

            /* CPU에서 참조 합계 계산 (BAFS_DIRECT 모드에서는 호스트에 데이터가 없으므로 스킵) */
            //std::vector<uint64_t> a_h_vec (a_h, a_h+n_elems);
            //uint64_t total = std::accumulate(a_h_vec.begin(), a_h_vec.begin()+n_elems, 0, std::plus<uint64_t>());
            uint64_t total = 0;
            if(mem != BAFS_DIRECT){
                for(uint64_t count=0; count<n_elems; count++)
                    total+=a_h[count];
                printf("total in cpu: %llu \n", total);
            }

            /* GPU 결과: 각 블록/워프의 partial sum들을 호스트에서 합산하여 최종 결과 산출 */
            total =0;
            for(uint64_t count=0; count<numblocks ;count++){
                total += result_h[count];
            }
            printf("total in gpu: %llu \n ", total);
            auto itrend = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(itrend - itrstart);

            //if(mem == BAFS_DIRECT) {
            //         h_Aarray->print_reset_stats();
		    // printf("VA SSD: %d PageSize: %d itrTime: %f\n", settings.n_ctrls, settings.pageSize, (double)elapsed.count());
            //}



            /* BAFS_DIRECT 모드: page cache 통계(hit/miss 비율 등) 출력 후 리셋 */
            if(mem == BAFS_DIRECT) {
                 h_Aarray->print_reset_stats();
                 cuda_err_chk(cudaDeviceSynchronize());
            }
            printf("\nVA %d A:%s Impl: %d \t SSD: %d \t CL: %d \t Cache: %llu \t TotalTime %f ms\n", titr, a_file_bin.c_str(), type, settings.n_ctrls, settings.pageSize,settings.maxPageCacheSize, milliseconds);
            fflush(stdout);
        }

        /* 메모리 해제 */
        if(mem!=BAFS_DIRECT){
           free(a_h);
         }
        free(result_h);

        if((type == BASELINE_PC) || (type == OPTIMIZED_PC)) {
            //TODO: Fix this
            delete h_pc;
            delete h_Arange;
            delete h_Aarray;
        }

        if(mem!=BAFS_DIRECT){
            if(mem==UVM_DIRECT){
              a_d = a_d-2;  /* UVM_DIRECT에서 +2 했던 포인터를 원래 위치로 복원 후 free */
            }
            cuda_err_chk(cudaFree(a_d));
            cuda_err_chk(cudaFree(int_d));
            cuda_err_chk(cudaFree(dev2out_d));
            cuda_err_chk(cudaFree(result_d));
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
