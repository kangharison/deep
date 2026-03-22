/*
 * =============================================================================
 * BaM VectorAdd 벤치마크 (benchmarks/vectoradd/main.cu)
 * =============================================================================
 *
 * [목적]
 * NVMe SSD에 저장된 두 uint64_t 벡터 A, B를 GPU에서 원소별로 더해 C = A + B를 계산하는
 * 벤치마크이다. GPU 메모리, UVM, BaFS 등 다양한 메모리 모드에서 동일 연산을 수행하여
 * I/O 경로별 성능(throughput, latency)을 비교한다.
 *
 * [지원 모드]
 * - GPUMEM (0):           호스트에서 파일 읽기 -> cudaMemcpy -> GPU VRAM에서 연산
 * - UVM_READONLY (1):     cudaMallocManaged + SetReadMostly 힌트
 * - UVM_DIRECT (2):       O_DIRECT로 파일을 UVM 영역에 직접 fread + SetAccessedBy 힌트
 * - UVM_READONLY_NVLINK (3): UVM + NVLink을 통한 원격 GPU 프리페치
 * - UVM_DIRECT_NVLINK (4):   UVM Direct + NVLink 경로
 * - BAFS_DIRECT (6):      BaM page cache를 통해 GPU가 NVMe SSD에서 직접 fetch
 *
 * [커널 구현 종류]
 * - BASELINE (0):     각 스레드가 하나의 원소를 처리. C[tid] = A[tid] + B[tid]. 단순 1:1 매핑
 * - OPTIMIZED (1):    워프 단위 page-aligned 접근. 한 워프가 NVMe 페이지 경계에 정렬하여 순차 접근
 * - BASELINE_PC (2):  BASELINE + BaM page cache (bam_ptr). 3개 배열(A, B, C) 모두 bam_ptr 사용
 * - OPTIMIZED_PC (3): OPTIMIZED + BaM page cache. 페이지 정렬 + on-demand NVMe fetch + 결과 write-back
 *
 * [I/O 흐름 (BAFS_DIRECT 모드)]
 * 1. Controller -> page_cache_t -> range_t -> array_t -> bam_ptr 체인으로 추상화
 * 2. 읽기: bam_ptr<uint64_t> Aptr(da); val = Aptr[idx] -> cache miss시 NVMe READ
 * 3. 쓰기: bam_ptr<uint64_t> Cptr(dc); Cptr[idx] = val -> dirty 페이지로 마킹 후 flush_cache()로 write-back
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

/* BaM/libnvm 관련 헤더 */
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
#include <page_cache.h>      /* page_cache_t, range_t, array_t, bam_ptr 등 BaM 핵심 타입 */
#include <util.h>

#include <iterator>
#include <functional>

#define UINT64MAX 0xFFFFFFFFFFFFFFFF

using error = std::runtime_error;
using std::string;

/* BaM 커널 모듈이 노출하는 NVMe 컨트롤러 디바이스 경로 목록 */
//const char* const ctrls_paths[] = {"/dev/libnvmpro0", "/dev/libnvmpro1", "/dev/libnvmpro2", "/dev/libnvmpro3", "/dev/libnvmpro4", "/dev/libnvmpro5", "/dev/libnvmpro6", "/dev/libnvmpro7"};
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};

#define WARP_SHIFT 5       /* log2(32) = 5, 워프 내 lane 인덱스 비트 시프트 */
#define WARP_SIZE 32        /* NVIDIA GPU 워프 크기 */

#define CHUNK_SHIFT 3       /* log2(8) = 3, chunk 단위 접근용 */
#define CHUNK_SIZE (1 << CHUNK_SHIFT)  /* 8개 원소 단위 chunk */

#define BLOCK_NUM 1024ULL   /* 2D 그리드의 x 방향 블록 수 상한 */

#define MAXWARP 64          /* SM당 최대 워프 수 */

typedef uint64_t EdgeT;     /* 원소 타입: 8바이트 unsigned integer */

/*
 * 커널 구현 타입 열거형 (-v 옵션):
 * - BASELINE(0):     스레드 1개 = 원소 1개. 가장 단순한 element-wise 매핑
 * - OPTIMIZED(1):    워프 단위 page-aligned 접근. 워프가 NVMe 페이지 경계에 맞춰 순차 탐색
 * - BASELINE_PC(2):  BASELINE + BaM page cache (bam_ptr로 NVMe 직접 접근)
 * - OPTIMIZED_PC(3): OPTIMIZED + BaM page cache (페이지 정렬 + on-demand fetch)
 */
typedef enum {
    BASELINE = 0,
    OPTIMIZED= 1,
    BASELINE_PC = 2,
    OPTIMIZED_PC= 3,
} impl_type;

/*
 * 메모리 할당 모드 열거형 (-m 옵션):
 * - GPUMEM(0):              cudaMalloc + cudaMemcpy (전통적 GPU 메모리)
 * - UVM_READONLY(1):        UVM + SetReadMostly (GPU 측 읽기 전용 복제본)
 * - UVM_DIRECT(2):          UVM + O_DIRECT + SetAccessedBy (zero-copy 유사)
 * - UVM_READONLY_NVLINK(3): UVM + NVLink 경유 원격 GPU 프리페치
 * - UVM_DIRECT_NVLINK(4):   UVM Direct + NVLink
 * - BAFS_DIRECT(6):         BaM page cache로 GPU-NVMe 직접 접근
 */
typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    UVM_READONLY_NVLINK = 3,
    UVM_DIRECT_NVLINK = 4,
    BAFS_DIRECT= 6,
} mem_type;


/*
 * kernel_baseline: 기본 element-wise vector addition 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - 그리드: numblocks = ceil(n_elems / numthreads)
 * - 블록: numthreads개 스레드
 * - 전역 스레드 ID tid = blockDim.x * blockIdx.x + threadIdx.x
 * - 각 스레드가 정확히 하나의 원소를 처리: sum[tid] = A[tid] + B[tid]
 *
 * [데이터 접근 패턴]
 * - A, B, sum 배열 모두 연속 주소를 stride=1로 접근 -> 워프 내 32개 스레드가 연속 메모리를 읽으므로 완벽한 coalesced access
 * - __launch_bounds__(64,32): SM당 최대 64 스레드/블록, 32 블록 -> occupancy 힌트
 */
__global__ //__launch_bounds__(64,32)
void kernel_baseline(uint64_t n_elems, uint64_t *A, uint64_t *B, unsigned long long int *sum){
    //uint64_t tid = blockDim.x * BLOCK_NUM * blockIdx.y + blockDim.x * blockIdx.x + threadIdx.x;
    uint64_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    if(tid<n_elems){
       sum[tid]= A[tid] + B[tid];
       //uint64_t val = A[tid] + B[tid];
       //atomicAdd(&sum[0], val);
       //printf("tid: %llu A:%llu B:%llu \n",tid,  A[tid], B[tid]);
    }
}

/*
 * kernel_baseline_ptr_pc: BASELINE vector addition + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr 3개 사용 (읽기 2개 + 쓰기 1개)]
 * - bam_ptr<uint64_t> Aptr(da): 입력 벡터 A를 NVMe SSD에서 on-demand 읽기
 * - bam_ptr<uint64_t> Bptr(db): 입력 벡터 B를 NVMe SSD에서 on-demand 읽기
 * - bam_ptr<uint64_t> Cptr(dc): 결과 벡터 C를 NVMe SSD에 on-demand 쓰기
 *
 * [I/O 흐름]
 * - Aptr[tid] 접근 시: page cache lookup -> miss시 NVMe READ -> cache에 페이지 적재 -> 데이터 반환
 * - Cptr[tid] = val 쓰기 시: page cache에서 해당 페이지를 dirty로 마킹
 * - 커널 완료 후 h_pc->flush_cache()로 dirty 페이지를 NVMe에 write-back
 *
 * [launch_bounds(64,32)]
 * SM당 최대 64 스레드/블록, 32 블록 동시 실행을 컴파일러에 힌트.
 * bam_ptr의 내부 상태(레지스터 사용량)를 고려한 occupancy 최적화.
 */
__global__ __launch_bounds__(64,32)
void kernel_baseline_ptr_pc(array_d_t<uint64_t>* da, array_d_t<uint64_t>* db, uint64_t n_elems, array_d_t<uint64_t>* dc,  unsigned long long int *sum){
    uint64_t  tid = blockDim.x * blockIdx.x + threadIdx.x;

    /* bam_ptr 생성: array_d_t 디스크립터를 감싸서 operator[]로 투명한 NVMe 접근 제공 */
    bam_ptr<uint64_t> Aptr(da);
    bam_ptr<uint64_t> Bptr(db);
    bam_ptr<uint64_t> Cptr(dc);

    if(tid<n_elems){
       Cptr[tid] = Aptr[tid] + Bptr[tid];   /* 읽기 2회(A,B) + 쓰기 1회(C), 각각 cache miss시 NVMe I/O 발생 */
       //uint64_t val = Aptr[tid] + Bptr[tid];
       //uint64_t val = A[tid] + B[tid];
       //sum[tid] = val;
       //atomicAdd(&sum[0], val);
    }
}


/*
 * kernel_sequential_warp: 워프 단위 page-aligned vector addition (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - n_warps = ceil(n_elems / n_elems_per_page): 총 워프 수 (각 워프가 n_pages_per_warp개 페이지 처리)
 * - 전역 워프 ID: warp_id = tid / 32, lane: tid % 32
 *
 * [데이터 접근 패턴 - NVMe 페이지 정렬]
 * - 각 워프가 연속된 NVMe 페이지들을 순차적으로 처리
 * - 페이지 내에서 32개 lane이 stride=32로 원소를 순회
 * - 이 패턴은 NVMe 페이지 경계에 정렬되어 있어서:
 *   (1) BaM page cache 사용 시 같은 워프의 모든 lane이 같은 페이지를 접근 -> cache hit 극대화
 *   (2) UVM 사용 시 4KB 페이지 단위 fault가 워프 내에서 1번만 발생
 *
 * [n_pages_per_warp 파라미터]
 * 현재 main에서 1로 고정 전달. 각 워프가 정확히 1개 NVMe 페이지를 담당한다는 의미.
 */
template<typename T>
__global__ __launch_bounds__(64,32)
void kernel_sequential_warp(T *A, T *B, uint64_t n_elems,  uint64_t n_pages_per_warp, unsigned long long* sum,  uint64_t n_warps, size_t page_size) {

    const uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const uint64_t lane = tid % 32;          /* 워프 내 lane 인덱스 (0~31) */
    const uint64_t warp_id = tid / 32;       /* 전역 워프 ID */
    const uint64_t n_elems_per_page = page_size / sizeof(T);  /* 한 NVMe 페이지의 원소 수 (예: 4096/8=512) */
    T val = 0;
    uint64_t idx=0;

    if(tid ==0)
        printf("n_elems_per_page: %llu\n", n_elems_per_page);
    if (warp_id < n_warps) {
        size_t start_page = n_pages_per_warp * warp_id;;
        /* 이 워프가 담당하는 NVMe 페이지들을 순차 탐색 */
        for (size_t i = 0; i < n_pages_per_warp; i++) {
            size_t cur_page = start_page + i;
            size_t start_idx = cur_page * n_elems_per_page + lane;

            /* 페이지 내 원소들을 워프 32개 lane이 stride=32로 순회 */
            for (size_t j = 0; j < n_elems_per_page; j += WARP_SIZE) {
               idx = start_idx + j;
               if(idx < n_elems){
                   val  = A[idx] + B[idx];
                   sum[idx] = val;
                   //atomicAdd(&sum[0], val);
       //            printf("tid: %llu A:%llu B:%llu \n",idx,  A[tid], B[tid]);
               }
            }
        }
    }
}

/*
 * kernel_sequential_warp_ptr_pc: 워프 단위 page-aligned vector addition + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr + stride 기반 워프 재매핑]
 * - 3개 bam_ptr (Aptr, Bptr, Cptr)로 NVMe SSD에서 직접 읽기/쓰기
 * - stride 파라미터를 사용한 워프 ID 재매핑:
 *   nep = (n_warps + stride - 1) / stride
 *   warp_id = (old_warp_id / nep) + ((old_warp_id % nep) * stride)
 *   이 재매핑은 인접 워프들이 서로 다른 NVMe 컨트롤러/채널의 페이지에 접근하도록 분산시켜
 *   여러 SSD를 병렬 활용할 때 대역폭을 극대화한다 (interleaving 효과)
 *
 * [I/O 흐름]
 * - 워프 내 32개 lane이 같은 NVMe 페이지를 접근하므로 page cache에서 1번만 fetch
 * - Cptr[idx] = val: dirty 페이지로 마킹. 커널 후 flush_cache()로 NVMe에 write-back
 */
template<typename T>
__global__ //__launch_bounds__(64,32)
void kernel_sequential_warp_ptr_pc(array_d_t<T> *da, array_d_t<T> *db, uint64_t n_elems,  uint64_t n_pages_per_warp, array_d_t<T> *dc, unsigned long long* sum,  uint64_t n_warps, size_t page_size, uint64_t stride) {

    bam_ptr<uint64_t> Aptr(da);
    bam_ptr<uint64_t> Bptr(db);
    bam_ptr<uint64_t> Cptr(dc);

    const uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const uint64_t lane = tid % 32;
    const uint64_t old_warp_id = tid / 32;
    const uint64_t n_elems_per_page = page_size / sizeof(T);
    T val = 0;
    uint64_t idx=0;
    /* 워프 ID 재매핑: stride를 사용하여 인접 워프를 서로 다른 NVMe 페이지 영역으로 분산 */
    uint64_t nep = (n_warps+stride-1)/stride;
    uint64_t warp_id = (old_warp_id/nep) + ((old_warp_id % nep)* stride);

    if (warp_id < n_warps) {
        size_t start_page = n_pages_per_warp * warp_id;;
        for (size_t i = 0; i < n_pages_per_warp; i++) {
            size_t cur_page = start_page + i;
            size_t start_idx = cur_page * n_elems_per_page + lane;

            for (size_t j = 0; j < n_elems_per_page; j += WARP_SIZE) {
               idx = start_idx + j;
               if(idx < n_elems){
                   val  = Aptr[idx] + Bptr[idx];    /* bam_ptr 읽기: cache miss시 NVMe READ 발행 */
                   Cptr[idx] = val;                  /* bam_ptr 쓰기: dirty 페이지로 마킹 */
                   //sum[idx] = val;
                   //atomicAdd(&sum[0], val);
       //            printf("tid: %llu A:%llu B:%llu \n",idx,  A[tid], B[tid]);
               }
            }
        }
        //sum[0] =val;
    }
}




/*
 * =============================================================================
 * main 함수: 설정 파싱 -> 입력 파일 A,B 로드 -> 커널 실행 -> 결과 출력
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

    std::ifstream filea, fileb;
    std::string a_file, b_file;
    std::string a_file_bin, b_file_bin;
    std::string filename;

    impl_type type;
    mem_type mem;
    uint32_t *pad;
    uint64_t *a_h, *a_d;      /* 호스트/디바이스 벡터 A 포인터 */
    uint64_t *b_h, *b_d;      /* 호스트/디바이스 벡터 B 포인터 */
    uint64_t *c_h, *c_d;      /* 호스트/디바이스 결과 벡터 C 포인터 */
    uint64_t n_elems, n_size;
    uint64_t typeT;
    uint64_t numblocks, numthreads;
    size_t freebyte, totalbyte;

    float milliseconds;

    uint64_t pc_page_size;     /* BaM page cache 페이지 크기 (바이트) */
    uint64_t pc_pages;         /* page cache 총 페이지 수 */

    try{

        a_file = std::string(settings.input_a);
        b_file = std::string(settings.input_b);

        type = (impl_type) settings.type;
        mem = (mem_type) settings.memalloc;

        pc_page_size = settings.pageSize;
        pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size);

        numthreads = settings.numThreads;

        cuda_err_chk(cudaSetDevice(settings.cudaDevice));

        cudaEvent_t start, end, tstart, tend;
        cuda_err_chk(cudaEventCreate(&start));
        cuda_err_chk(cudaEventCreate(&end));
        cuda_err_chk(cudaEventCreate(&tstart));
        cuda_err_chk(cudaEventCreate(&tend));

        /* 입력 파일: .bel 그래프의 .dst 확장자 사용. 바이너리 형식: [8B vertex_count][8B typeT][data...] */
        a_file_bin = a_file + ".dst";
        b_file_bin = b_file + ".dst";

        std::cout << "A: " << a_file_bin << " B: " << b_file_bin << std::endl;

        uint64_t n_elems = settings.n_elems;
        uint64_t n_elems_size = n_elems * sizeof(uint64_t);
        printf("Total elements: %llu \n", n_elems);
        uint64_t tmp;

        /* 파일 A 읽기: 처음 16바이트 헤더 스킵 후 n_elems * 8바이트 데이터 로드 */
        filea.open(a_file_bin.c_str(), std::ios::in | std::ios::binary);
        if (!filea.is_open()) {
            printf("A file open failed\n");
            exit(1);
        };

        filea.read((char*)(&tmp), 16);
        if(mem != BAFS_DIRECT)
            a_h = (uint64_t*)malloc(n_elems_size);
        if((mem!=BAFS_DIRECT) &&  (mem != UVM_DIRECT)){
             //printf("before mem switch\n");
             //fflush(stdout);
             filea.read((char*)a_h, n_elems_size);
             filea.close();
        }

        /* 파일 B 읽기: A와 동일한 형식 */
        fileb.open(b_file_bin.c_str(), std::ios::in | std::ios::binary);
        if (!fileb.is_open()) {
            printf("A file open failed\n");
            exit(1);
        };

        fileb.read((char*)(&tmp), 16);
        if(mem != BAFS_DIRECT)
            b_h = (uint64_t*)malloc(n_elems_size);
        if((mem!=BAFS_DIRECT) && (mem != UVM_DIRECT)){
            fileb.read((char*)b_h, n_elems_size);
            fileb.close();
        }


        /*
         * 메모리 모드별 데이터 준비:
         * - GPUMEM: cudaMalloc으로 GPU VRAM 할당 후 cudaMemcpy로 A,B 전송
         * - UVM_READONLY: UVM 할당 + SetReadMostly 힌트로 GPU 측 읽기 복제본 생성
         * - UVM_DIRECT: O_DIRECT로 파일을 UVM 영역에 직접 fread + SetAccessedBy 힌트
         * - BAFS_DIRECT: 사전 로드 불필요 (bam_ptr이 on-demand fetch)
         */
        switch (mem) {
            case GPUMEM:
                {
                cuda_err_chk(cudaMalloc((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMalloc((void**)&b_d, n_elems_size));
                high_resolution_clock::time_point mc1 = high_resolution_clock::now();
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                cuda_err_chk(cudaMemcpy(b_d, b_h, n_elems_size, cudaMemcpyHostToDevice));
                high_resolution_clock::time_point mc2 = high_resolution_clock::now();
                duration<double> mc_time_span = duration_cast<duration<double>>(mc2 -mc1);
                std::cout<< "Memcpy time for loading the inputs: "<< mc_time_span.count() <<std::endl;
                break;
                }
            case UVM_READONLY:
                {
                cuda_err_chk(cudaMallocManaged((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMallocManaged((void**)&b_d, n_elems_size));
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                cuda_err_chk(cudaMemcpy(b_d, b_h, n_elems_size, cudaMemcpyHostToDevice));
                //TODO: we can move that read op here.
                //file.read((char*)edgeList_d, edge_size);
                /* SetReadMostly: GPU에 읽기 전용 복제본 캐싱. CPU와 GPU 모두 읽을 수 있지만 GPU 페이지 fault 감소 */
                cuda_err_chk(cudaMemAdvise(a_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(b_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                cuda_err_chk(cudaMemGetInfo(&freebyte, &totalbyte));
                break;
                }
            case UVM_DIRECT:
                {
                filea.close();
                fileb.close();
                /* O_DIRECT: OS 페이지 캐시를 우회하여 SSD에서 직접 UVM 영역으로 읽음. 대규모 데이터에서 메모리 절약 */
                int fda = open(a_file_bin.c_str(), O_RDONLY | O_DIRECT);
                int fdb = open(b_file_bin.c_str(), O_RDONLY | O_DIRECT);
                FILE *fa_tmp= fdopen(fda, "rb");
                if ((fa_tmp == NULL) || (fda == -1)) {
                    printf("A file fd open failed\n");
                    exit(1);
                }
                FILE *fb_tmp= fdopen(fdb, "rb");
                if ((fb_tmp == NULL) || (fdb == -1)) {
                    printf("A file fd open failed\n");
                    exit(1);
                }

                /* 4K 정렬: O_DIRECT 요구사항. 버퍼 크기를 4KB 경계에 올림 정렬 */
                uint64_t count_4k_aligned = ((n_elems + 2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t));
                //uint64_t count_4k_aligned = n_elems;
                uint64_t size_4k_aligned = count_4k_aligned * sizeof(uint64_t);

                cuda_err_chk(cudaMallocManaged((void**)&a_d, size_4k_aligned));
                cuda_err_chk(cudaMallocManaged((void**)&b_d, size_4k_aligned));
                /* SetAccessedBy: GPU가 이 메모리에 접근할 것임을 CUDA 드라이버에 힌트. 페이지 마이그레이션 최적화 */
                cuda_err_chk(cudaMemAdvise(a_d, size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(b_d, size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();

                if (fread(a_d, sizeof(uint64_t), count_4k_aligned, fa_tmp) <0) {
                    printf("A file fread failed: %llu \t %llu\n", count_4k_aligned, n_elems+2);
                    exit(1);
                }
                fclose(fa_tmp);
                close(fda);

                if (fread(b_d, sizeof(uint64_t), count_4k_aligned, fb_tmp) <0) {
                    printf("B file fread failed\n");
                    exit(1);
                }
                fclose(fb_tmp);
                close(fdb);


                /* 헤더 2개 uint64_t(16바이트)를 건너뛰기 위해 포인터 +2 */
                a_d = a_d + 2;
                b_d = b_d + 2;

                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "file read time: "<< time_span.count() <<std::endl;

                /* //THIS DOES NOT WORK
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();
                cuda_err_chk(cudaMallocManaged((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMallocManaged((void**)&b_d, n_elems_size));
                filea.read((char*)a_d, n_elems_size);
                fileb.read((char*)b_d, n_elems_size);
                cuda_err_chk(cudaMemAdvise(a_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                cuda_err_chk(cudaMemAdvise(b_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                //cuda_err_chk(cudaMemAdvise(a_d, n_elems_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                //cuda_err_chk(cudaMemAdvise(b_d, n_elems_size, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "file read time: "<< time_span.count() <<std::endl;
                */


                break;
                }
            case BAFS_DIRECT:
                {
                /* BAFS_DIRECT: 파일 사전 로드 불필요. bam_ptr이 커널 내에서 on-demand로 NVMe에서 fetch */
                break;
                }
        }


        /* 전체 데이터가 차지하는 NVMe 페이지 수 계산 */
        uint64_t n_pages = ceil(((float)n_elems_size)/pc_page_size);

        /* 결과 저장용 GPU 메모리 할당 */
        unsigned long long int *sum_d;
        unsigned long long int *sum_h;
        sum_h = (unsigned long long int*) malloc(n_elems*sizeof(unsigned long long int));

        cuda_err_chk(cudaMalloc((void**)&sum_d, n_elems*sizeof(unsigned long long int)));

		printf("Allocation finished\n");
        fflush(stdout);

        uint64_t n_warps;

        /*
         * 커널 타입에 따른 그리드 크기 계산:
         * - BASELINE/BASELINE_PC: 스레드 1개 = 원소 1개 -> numblocks = ceil(n_elems / numthreads)
         * - OPTIMIZED/OPTIMIZED_PC: 워프 1개 = NVMe 페이지 1개 -> n_warps개 워프 필요
         */
        switch (type) {
            case BASELINE:
            case BASELINE_PC:
                numblocks = ((n_elems+numthreads-1)/numthreads);
                break;
            case OPTIMIZED:
            case OPTIMIZED_PC:{
                uint64_t n_elems_per_page = pc_page_size/sizeof(uint64_t);
                n_warps = (n_elems + n_elems_per_page-1)/n_elems_per_page;
                numblocks = (n_warps * WARP_SIZE + numthreads-1) / numthreads;
                break;
                           }
            default:
                fprintf(stderr, "Invalid type\n");
                exit(1);
                break;
        }

        //dim3 blockDim(BLOCK_NUM, (numblocks+BLOCK_NUM)/BLOCK_NUM);
        dim3 blockDim(numblocks);

        if((type == BASELINE_PC) || (type==OPTIMIZED_PC)) {
                printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
        }

        /*
         * BaM NVMe 컨트롤러 초기화 (BAFS_DIRECT 전용):
         * Controller(path, namespace, cudaDevice, queueDepth, numQueues)
         * - NVMe admin 큐로 컨트롤러를 초기화하고
         * - GPU 접근 가능한 메모리에 I/O SQ/CQ를 할당
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
         * BaM page cache 및 배열 추상화 초기화 (BASELINE_PC / OPTIMIZED_PC 전용):
         *
         * page_cache_t: GPU VRAM에 pc_pages개 페이지 슬롯 할당. LRU eviction 정책.
         *
         * range_t<uint64_t>: 논리적 원소 범위를 NVMe LBA 범위에 매핑
         *   - h_Arange: 벡터 A의 [0, n_elems) 범위, SSD 오프셋 = afileoffset
         *   - h_Brange: 벡터 B의 [0, n_elems) 범위, SSD 오프셋 = bfileoffset
         *   - h_Crange: 결과 벡터 C의 범위, SSD 오프셋 = cfileoffset (720GB 지점)
         *
         * array_t<uint64_t>: range들을 묶어 하나의 논리 배열로 추상화.
         *   d_array_ptr를 커널에 전달하면 bam_ptr로 감싸서 사용
         */
        page_cache_t* h_pc;
        range_t<uint64_t>* h_Arange;
        range_t<uint64_t>* h_Brange;
        range_t<uint64_t>* h_Crange;
        std::vector<range_t<uint64_t>*> vec_Arange(1);
        std::vector<range_t<uint64_t>*> vec_Brange(1);
        std::vector<range_t<uint64_t>*> vec_Crange(1);
        array_t<uint64_t>* h_Aarray;
        array_t<uint64_t>* h_Barray;
        array_t<uint64_t>* h_Carray;


        /* cfileoffset: 결과 벡터 C의 SSD 저장 위치. A, B와 겹치지 않도록 720GB 오프셋 사용 */
        uint64_t cfileoffset = 720*1024*1024*1024;
        if((type == BASELINE_PC) || (type == OPTIMIZED_PC)) {
            //TODO: fix for 2 arrays
            h_pc =new page_cache_t(pc_page_size, pc_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
            h_Arange = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)n_elems, (uint64_t) (ceil(settings.afileoffset*1.0/pc_page_size)),(uint64_t)n_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice);
            h_Brange = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)n_elems, (uint64_t) (ceil(settings.bfileoffset*1.0/pc_page_size)),(uint64_t)n_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice);
            h_Crange = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)n_elems, (uint64_t) (ceil(cfileoffset*1.0/pc_page_size)),(uint64_t)n_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice);
            vec_Arange[0] = h_Arange;
            vec_Brange[0] = h_Brange;
            vec_Crange[0] = h_Crange;
            h_Aarray = new array_t<uint64_t>(n_elems, settings.afileoffset, vec_Arange, settings.cudaDevice);
            h_Barray = new array_t<uint64_t>(n_elems, settings.bfileoffset, vec_Brange, settings.cudaDevice);
            h_Carray = new array_t<uint64_t>(n_elems, cfileoffset, vec_Crange, settings.cudaDevice);

            printf("Page cache initialized\n");
            fflush(stdout);
        }

        //cuda_err_chk(cudaMalloc((void**)&c_d, n_elems_size));

        /* 벤치마크 실행 루프 (현재 1회 반복) */
        for(int titr=0; titr<1; titr+=1){
            cuda_err_chk(cudaEventRecord(start, 0));

            auto itrstart = std::chrono::system_clock::now();
            cuda_err_chk(cudaMemset(sum_d, 0, n_elems*sizeof(unsigned long long int)));

            switch (type) {
                case BASELINE:
                    printf("launching baseline: blockDim.x :%llu blockDim.y :%llu numthreads:%llu\n", blockDim.x, blockDim.y, numthreads);
                    kernel_baseline<<<blockDim, numthreads>>>(n_elems, a_d, b_d, sum_d);
                    //kernel_baseline<<<blockDim, numthreads>>>(n_elems,  sum_d);
                    break;
                case BASELINE_PC:
                    printf("launching baseline_pc: blockDim.x :%llu blockDim.y :%llu numthreads:%llu\n", blockDim.x, blockDim.y, numthreads);
                    /* 3개 array의 d_array_ptr을 전달: A(읽기), B(읽기), C(쓰기) */
                    kernel_baseline_ptr_pc<<<blockDim, numthreads>>>(h_Aarray->d_array_ptr, h_Barray->d_array_ptr, n_elems, h_Carray->d_array_ptr, sum_d);
                    /* flush_cache: dirty 페이지(C 벡터 쓰기 결과)를 NVMe SSD에 write-back */
                    h_pc->flush_cache();
                    //uint64_t n_pages = pc_pages;
                    //numblocks = (n_pages+numthreads-1)/numthreads;
                    //cache_flush<uint64_t><<<numblocks, numthreads>>>(h_Carray->d_array_ptr, n_pages, settings.pageSize);
                    break;

                case OPTIMIZED:
                    printf("launching optimized: blockDim.x :%llu numthreads:%llu\n", blockDim.x, numthreads);
                    kernel_sequential_warp<uint64_t><<<blockDim, numthreads>>>(a_d, b_d, n_elems, 1, sum_d, n_warps, settings.pageSize);
                    break;

                case OPTIMIZED_PC:
                    printf("launching optimized: blockDim.x :%llu numthreads:%llu\n", blockDim.x, numthreads);
                    kernel_sequential_warp_ptr_pc<uint64_t><<<blockDim, numthreads>>>(h_Aarray->d_array_ptr, h_Barray->d_array_ptr, n_elems, 1, h_Carray->d_array_ptr, sum_d, n_warps, settings.pageSize, settings.stride);
                    /* flush_cache: 결과 벡터 C의 dirty 페이지를 NVMe에 write-back */
                    h_pc->flush_cache();
                    break;
                default:
                    fprintf(stderr, "Invalid type\n");
                    exit(1);
                    break;
            }
            cuda_err_chk(cudaEventRecord(end, 0));
            cuda_err_chk(cudaEventSynchronize(end));
            cuda_err_chk(cudaEventElapsedTime(&milliseconds, start, end));

            //cuda_err_chk(cudaMemcpy(c_d, sum_h, n_elems_size, cudaMemcpyDeviceToHost));
            cuda_err_chk(cudaMemcpy(sum_h, sum_d, n_elems*sizeof(unsigned long long int), cudaMemcpyDeviceToHost));
            printf("sum: %llu\n", sum_h[0]);

            auto itrend = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(itrend - itrstart);

            //if(mem == BAFS_DIRECT) {
            //         h_Aarray->print_reset_stats();
            //         h_Barray->print_reset_stats();
            //         h_Carray->print_reset_stats();
		    // printf("VA SSD: %d PageSize: %d itrTime: %f\n", settings.n_ctrls, settings.pageSize, (double)elapsed.count());
            //}



            /* BAFS_DIRECT: page cache 통계(hit/miss, I/O 횟수 등) 출력 */
            if(mem == BAFS_DIRECT) {
                 h_Aarray->print_reset_stats();
                 h_Barray->print_reset_stats();
                 cuda_err_chk(cudaDeviceSynchronize());
            }
            printf("\nVA %d A:%s \t B:%s Impl: %d \t SSD: %d \t CL: %d \t Cache: %llu \t Stride: %llu \t TotalTime %f ms\n", titr, a_file_bin.c_str(), b_file_bin.c_str(), type, settings.n_ctrls, settings.pageSize,settings.maxPageCacheSize, settings.stride, milliseconds);
            fflush(stdout);
        }

        /* 메모리 해제 */
        if(mem!=BAFS_DIRECT){
           free(a_h);
           free(b_h);
         }

        if((type == BASELINE_PC) || (type == OPTIMIZED_PC)) {
            //TODO: Fix this
            delete h_pc;
            delete h_Arange;
            delete h_Brange;
            delete h_Aarray;
            delete h_Barray;
        }

        cuda_err_chk(cudaFree(sum_d));
        if(mem!=BAFS_DIRECT){
            if(mem==UVM_DIRECT){
              a_d = a_d-2;    /* UVM_DIRECT에서 +2 했던 포인터 복원 */
              b_d = b_d-2;
            }
            cuda_err_chk(cudaFree(a_d));
            cuda_err_chk(cudaFree(b_d));
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
