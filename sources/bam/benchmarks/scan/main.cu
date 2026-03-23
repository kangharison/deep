/* References:
 *
 *      Coalesce
 *          Hong, Sungpack, et al.
 *          "Accelerating CUDA graph algorithms at maximum warp."
 *          Acm Sigplan Notices 46.8 (2011): 267-276.
 *
 */

/*
 * [BaM Scan 벤치마크 - 메인 파일]
 *
 * Prefix Sum(접두사 합, Inclusive Scan) 벤치마크.
 * 입력 배열 A[0..n-1]에 대해 output[i] = A[0] + A[1] + ... + A[i]를 계산한다.
 *
 * 알고리즘: Blelloch Scan (Work-Efficient Parallel Prefix Sum)
 *   1단계 (Up-Sweep / Reduce): 트리 구조로 부분합을 축적
 *     - stride를 1부터 BLOCKSIZE까지 2배씩 증가시키며 sharedMem[index] += sharedMem[index-stride]
 *   2단계 (Down-Sweep): 축적된 부분합을 하위로 전파
 *     - stride를 BLOCKSIZE/2부터 1까지 절반씩 줄이며 sharedMem[index+stride] += sharedMem[index]
 *   3단계 (finalsum): 각 블록의 마지막 원소(블록 합)를 CUB InclusiveSum으로 스캔한 뒤,
 *     해당 블록의 모든 원소에 이전 블록 합을 더해 최종 결과를 완성한다.
 *
 * GPU 스레드 구조:
 *   - 각 CUDA 블록이 2*BLOCKSIZE개의 원소를 처리 (각 스레드가 2개 원소 담당)
 *   - shared memory에 2*BLOCKSIZE개 원소를 로드하여 블록 내 scan 수행
 *   - 블록 간 scan은 intermediate 배열 + CUB InclusiveSum으로 처리
 *
 * 데이터 접근 패턴:
 *   - BASELINE: GPU 메모리(또는 UVM)에서 직접 읽기
 *   - BASELINE_PC: BaM page_cache(array_d_t/bam_ptr)를 통해 SSD에서 GPU로 직접 I/O
 *     GPU 커널이 bam_ptr[i]로 접근하면 page_cache가 해당 페이지를 SSD에서 fetch한다.
 *
 * CSR 포맷: 이 벤치마크에서는 그래프가 아닌 일반 배열의 scan을 수행.
 *   입력 파일은 .dst 확장자의 바이너리 파일 (8바이트 헤더 2개 + uint64_t 배열).
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

#include <iterator>
#include <numeric>
#include <functional>
#include <cub/cub.cuh>


#define UINT64MAX 0xFFFFFFFFFFFFFFFF

using error = std::runtime_error;
using std::string;
//const char* const ctrls_paths[] = {"/dev/libnvmpro0", "/dev/libnvmpro1", "/dev/libnvmpro2", "/dev/libnvmpro3", "/dev/libnvmpro4", "/dev/libnvmpro5", "/dev/libnvmpro6", "/dev/libnvmpro7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm4", "/dev/libnvm9", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8"};

#define WARP_SHIFT 5
#define WARP_SIZE 32

#define CHUNK_SHIFT 3
#define CHUNK_SIZE (1 << CHUNK_SHIFT)

#define BLOCK_NUM 1024ULL

#define MAXWARP 64

typedef uint64_t EdgeT;

/* impl_type: 커널 구현 방식
 *   BASELINE(0)    - GPU 메모리에서 직접 읽어 scan 수행
 *   BASELINE_PC(3) - BaM 페이지 캐시(bam_ptr)를 통해 SSD에서 직접 읽어 scan 수행 */
typedef enum {
    BASELINE = 0,
    BASELINE_PC = 3,
} impl_type;

/* mem_type: 데이터 적재 방식
 *   GPUMEM(0)       - cudaMalloc + cudaMemcpy로 GPU 메모리에 전부 적재
 *   UVM_READONLY(1) - cudaMallocManaged + ReadMostly 힌트
 *   UVM_DIRECT(2)   - cudaMallocManaged + AccessedBy 힌트 + O_DIRECT fread
 *   BAFS_DIRECT(6)  - BaM page_cache를 통해 GPU가 NVMe SSD에서 직접 I/O (CPU 개입 없음) */
typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    BAFS_DIRECT= 6,
} mem_type;

/* finalsum 커널: 블록 간 scan 결과를 합산하는 커널.
 * 각 블록의 scan 결과(intermediate)를 CUB로 prefix sum한 후,
 * 이 커널이 각 원소에 해당 블록 이전까지의 누적합을 더한다.
 * blockIdx.x == 0인 블록은 이전 블록이 없으므로 스킵한다. */
__global__
void finalsum (EdgeT *input, EdgeT* output, uint64_t len){
   if(blockIdx.x == 0) return;

   uint64_t BLOCKSIZE = blockDim.x;
   uint64_t idx = 2*blockIdx.x *BLOCKSIZE + threadIdx.x;

   if(idx < len)
       output[idx] += input[blockIdx.x -1];
   if((idx + BLOCKSIZE) < len)
       output[idx+BLOCKSIZE] += input[blockIdx.x -1];
}

/* kernel_scan_baseline: GPU 메모리에서 데이터를 읽어 Blelloch scan을 수행하는 CUDA 커널.
 *
 * Blelloch Scan 알고리즘 (Work-Efficient Parallel Prefix Sum):
 *   - 각 블록이 2*BLOCKSIZE개의 원소를 shared memory에 로드
 *   - 1단계 (Up-Sweep): stride를 1->2->4->...->BLOCKSIZE로 증가시키며 부분합 축적
 *     sharedMem[(threadIdx+1)*2*stride - 1] += sharedMem[(threadIdx+1)*2*stride - 1 - stride]
 *   - 2단계 (Down-Sweep): stride를 BLOCKSIZE/2->...->1로 감소시키며 부분합 전파
 *     sharedMem[index+stride] += sharedMem[index]
 *   - 결과를 output에 기록하고, 블록의 마지막 원소(총합)를 intermediate에 저장
 *
 * 매개변수:
 *   input        - 입력 배열 (GPU 메모리)
 *   output       - 출력 배열 (scan 결과)
 *   intermediate - 각 블록의 총합 저장 (블록 간 scan용, NULL이면 스킵)
 *   len          - 배열 길이 */
__global__
void kernel_scan_baseline(EdgeT *input, EdgeT *output, EdgeT *intermediate,  uint64_t len){

    uint64_t BLOCKSIZE = blockDim.x;
    extern __shared__ EdgeT sharedMem[];

    uint64_t idx = 2*blockIdx.x * BLOCKSIZE + threadIdx.x;

    /* 글로벌 메모리에서 shared memory로 2개 원소 로드 (범위 초과 시 0) */
    if(idx < len)
        sharedMem[threadIdx.x] = input[idx];
    else
        sharedMem[threadIdx.x] = 0;

    if((idx + BLOCKSIZE) < len)
        sharedMem[threadIdx.x + BLOCKSIZE] = input[idx + BLOCKSIZE];
    else
        sharedMem[threadIdx.x + BLOCKSIZE] = 0;

    __syncthreads();

    /* Up-Sweep (Reduce) 단계: 트리 구조로 부분합 축적 */
    for(uint64_t stride = 1; stride <= BLOCKSIZE; stride*=2){
        uint64_t index = (threadIdx.x+1)*2*stride -1;
        if(index <2 * BLOCKSIZE)
            sharedMem[index] += sharedMem[index-stride];
        __syncthreads();
    }

    /* Down-Sweep 단계: 축적된 부분합을 하위 노드로 전파하여 inclusive scan 완성 */
    for(uint64_t stride = BLOCKSIZE/2; stride>0; stride/=2){
        uint64_t index = (threadIdx.x+1)*stride*2 - 1;
        if(index + stride < (2*BLOCKSIZE))
            sharedMem[index+stride] += sharedMem[index];
        __syncthreads();
    }

    /* scan 결과를 글로벌 메모리에 기록 */
    if(idx<len) output[idx] = sharedMem[threadIdx.x];
    if(idx+BLOCKSIZE < len) output[idx+BLOCKSIZE] = sharedMem[threadIdx.x + BLOCKSIZE];

    /* 블록의 마지막 원소(블록 총합)를 intermediate에 저장 (블록 간 scan에 사용) */
    if(intermediate !=NULL && threadIdx.x == 0)
        intermediate[blockIdx.x] = sharedMem[2*BLOCKSIZE-1];
}


/* kernel_scan_baseline_ptr_pc: BaM 페이지 캐시를 통해 SSD에서 직접 데이터를 읽어 scan하는 커널.
 *
 * kernel_scan_baseline과 동일한 Blelloch scan 알고리즘이지만,
 * 입력 데이터를 GPU 메모리 대신 bam_ptr을 통해 접근한다.
 * bam_ptr[idx] 접근 시 해당 페이지가 GPU 메모리의 page_cache에 없으면
 * NVMe SSD에서 GPU 메모리로 P2P DMA를 통해 자동으로 fetch된다.
 *
 * array_d_t<EdgeT>*da: BaM의 디바이스 측 배열 디스크립터. SSD 상의 데이터 위치,
 * 페이지 캐시 매핑 정보, NVMe 큐 정보 등을 포함한다. */
__global__
void kernel_scan_baseline_ptr_pc(array_d_t<EdgeT>*da, EdgeT *input, EdgeT *output, EdgeT *intermediate,  uint64_t len){

    bam_ptr<EdgeT> ptr(da);
    uint64_t BLOCKSIZE = blockDim.x;
    extern __shared__ EdgeT sharedMem[];

    uint64_t idx = 2*blockIdx.x * BLOCKSIZE + threadIdx.x;

    /* BaM 페이지 캐시를 통해 SSD에서 shared memory로 데이터 로드 */
    if(idx < len)
        sharedMem[threadIdx.x] = ptr[idx];
    else
        sharedMem[threadIdx.x] = 0;

    if((idx + BLOCKSIZE) < len)
        sharedMem[threadIdx.x + BLOCKSIZE] = ptr[idx + BLOCKSIZE];
    else
        sharedMem[threadIdx.x + BLOCKSIZE] = 0;

    __syncthreads();

    /* Up-Sweep 단계 */
    for(uint64_t stride = 1; stride <= BLOCKSIZE; stride*=2){
        uint64_t index = (threadIdx.x+1)*2*stride -1;
        if(index <2 * BLOCKSIZE)
            sharedMem[index] += sharedMem[index-stride];
        __syncthreads();
    }

    /* Down-Sweep 단계 */
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


/* main(): Scan 벤치마크 진입점.
 *
 * 전체 흐름:
 *   1. 설정 파싱 및 CUDA 디바이스 초기화
 *   2. 입력 파일(.dst) 로드 (메모리 할당 방식에 따라 GPUMEM/UVM/BAFS 분기)
 *   3. BAFS_DIRECT인 경우: NVMe 컨트롤러 생성 -> page_cache 초기화 -> array_t 생성
 *   4. scan 커널 실행: 블록 내 scan -> CUB InclusiveSum으로 블록 합 scan -> finalsum으로 합산
 *   5. 결과 검증 (CPU에서 순차 합산한 값과 GPU 결과 비교)
 *   6. 성능 측정 및 출력 (CUDA 이벤트 타이머) */
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
    std::string a_file;
    std::string a_file_bin;
    std::string filename;

    impl_type type;
    mem_type mem;
    uint64_t *a_h, *a_d;
    uint64_t numblocks, numthreads;

    float milliseconds;

    uint64_t pc_page_size;
    uint64_t pc_pages;

    try{

        a_file = std::string(settings.input_a);

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


        a_file_bin = a_file + ".dst";

        std::cout << "A: " << a_file_bin  << std::endl;

        uint64_t n_elems = settings.n_elems;
        uint64_t n_elems_size = n_elems * sizeof(uint64_t);
        printf("Total elements: %llu \n", n_elems);
        uint64_t tmp;

        /* 입력 파일 읽기: .dst 파일 포맷 (8바이트 헤더 2개 + uint64_t 배열) */
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

        /* 메모리 할당 방식에 따른 데이터 적재 */
        switch (mem) {
            case GPUMEM:
                {
                /* GPU 메모리에 전부 적재: cudaMalloc + cudaMemcpy */
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
                /* UVM ReadMostly: GPU 페이지 테이블에 읽기 전용 복사본 유지 */
                cuda_err_chk(cudaMallocManaged((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                cuda_err_chk(cudaMemAdvise(a_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                break;
                }
            case UVM_DIRECT:
                {
                /* UVM Direct: O_DIRECT로 파일을 UVM 영역에 직접 fread, GPU가 접근 시 페이지 마이그레이션 */
                filea.close();
                fileb.close();
                int fda = open(a_file_bin.c_str(), O_RDONLY | O_DIRECT);
                FILE *fa_tmp= fdopen(fda, "rb");
                if ((fa_tmp == NULL) || (fda == -1)) {
                    printf("A file fd open failed\n");
                    exit(1);
                }

                /* 4KB 정렬: O_DIRECT는 4KB 정렬된 버퍼와 크기가 필요 */
                uint64_t count_4k_aligned = ((n_elems + 2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t));
                //uint64_t count_4k_aligned = n_elems;
                uint64_t size_4k_aligned = count_4k_aligned * sizeof(uint64_t);

                cuda_err_chk(cudaMallocManaged((void**)&a_d, size_4k_aligned));
                cuda_err_chk(cudaMemAdvise(a_d, size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();

                if (fread(a_d, sizeof(uint64_t), count_4k_aligned, fa_tmp)) {
                    printf("A file fread failed: %llu \t %llu\n", count_4k_aligned, n_elems+2);
                    exit(1);
                }
                fclose(fa_tmp);
                close(fda);

                a_d = a_d + 2;  /* 8바이트 헤더 2개 건너뛰기 */

                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "file read time: "<< time_span.count() <<std::endl;

                break;
                }
            case BAFS_DIRECT:
                {
                /* BAFS_DIRECT: CPU에서 데이터를 로드하지 않음.
                 * GPU 커널이 bam_ptr을 통해 접근할 때 page_cache가 SSD에서 자동 fetch */
                break;
                }
        }


        uint64_t n_pages = ceil(((float)n_elems_size)/pc_page_size);

        /* numblocks 계산: 각 블록이 2*numthreads개의 원소를 처리하므로 ceil(n_elems / (2*numthreads)) */
        switch (type) {
            case BASELINE:
            case BASELINE_PC:
                numblocks = ((n_elems/(2*numthreads)) + 1);
                break;
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

        /* 결과 저장용 GPU 메모리 할당 */
        EdgeT *result_h;
        EdgeT *result_d;
        EdgeT *dev2out_d;  /* 블록 합의 scan 결과 저장용 */
        EdgeT *int_d;      /* 각 블록의 총합(intermediate) 저장용 */

        cuda_err_chk(cudaMalloc((void**)&int_d, (numblocks)*sizeof(EdgeT)));
        cuda_err_chk(cudaMalloc((void**)&dev2out_d, (numblocks)*sizeof(EdgeT)));
        cuda_err_chk(cudaMalloc((void**)&result_d, (n_elems+1)*sizeof(EdgeT)));
        result_h = (EdgeT*) malloc(n_elems* sizeof(EdgeT));
        cuda_err_chk(cudaMemset(result_d, 0, (n_elems+1)*sizeof(EdgeT)));

		printf("Allocation finished\n");
        fflush(stdout);

        /* BAFS_DIRECT: NVMe 컨트롤러 초기화 (libnvm 디바이스 파일 사용) */
        std::vector<Controller*> ctrls(settings.n_ctrls);
        if(mem == BAFS_DIRECT){
            cuda_err_chk(cudaSetDevice(settings.cudaDevice));
            for (size_t i = 0 ; i < settings.n_ctrls; i++)
                ctrls[i] = new Controller(ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);
            printf("Controllers Created\n");
        }
        printf("Initialization done\n");
        fflush(stdout);

        /* BaM 페이지 캐시 초기화 (BASELINE_PC 모드에서만 사용)
         * page_cache_t: GPU 메모리에 페이지 캐시 할당
         * range_t: SSD 상의 데이터 범위를 페이지 캐시와 매핑
         * array_t: 디바이스 측에서 접근 가능한 배열 디스크립터 생성 */
        page_cache_t* h_pc;
        range_t<uint64_t>* h_Arange;
        std::vector<range_t<uint64_t>*> vec_Arange(1);
        array_t<uint64_t>* h_Aarray;


        if((type == BASELINE_PC)) {
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

        /* 벤치마크 실행 (2회 반복: 1회차는 웜업, 2회차가 실제 측정) */
        for(int titr=0; titr<2; titr+=1){
            cuda_err_chk(cudaEventRecord(start, 0));

            auto itrstart = std::chrono::system_clock::now();

            switch (type) {
                case BASELINE:{
                    /* 3단계 scan: (1) 블록 내 scan -> (2) 블록 합의 prefix sum (CUB) -> (3) 블록 합 합산 */
                    kernel_scan_baseline<<<blockDim, numthreads, 2*numthreads*sizeof(EdgeT)>>>(a_d, (&result_d[1]), int_d, n_elems);
                    cub::DeviceScan::InclusiveSum(d_tmp, tmp_size, int_d, dev2out_d, numblocks);
                    cuda_err_chk(cudaMalloc(&d_tmp, tmp_size));
                    cub::DeviceScan::InclusiveSum(d_tmp, tmp_size, int_d, dev2out_d, numblocks);
                    finalsum<<<blockDim, numthreads>>>(dev2out_d, (&result_d[1]), n_elems);
                    break;
                }
                case BASELINE_PC:{
                    /* BaM 페이지 캐시 버전: 입력을 bam_ptr로 SSD에서 직접 읽되, 나머지 로직은 동일 */
                    printf("launching PC: blockDim.x :%llu blockDim.y :%llu numthreads:%llu\n", blockDim.x, blockDim.y, numthreads);
                    kernel_scan_baseline_ptr_pc<<<blockDim, numthreads, 2*numthreads*sizeof(EdgeT)>>>(h_Aarray->d_array_ptr, a_d, (&result_d[1]), int_d, n_elems);
                    cub::DeviceScan::InclusiveSum(d_tmp, tmp_size, int_d, dev2out_d, numblocks);
                    cuda_err_chk(cudaMalloc(&d_tmp, tmp_size));
                    cub::DeviceScan::InclusiveSum(d_tmp, tmp_size, int_d, dev2out_d, numblocks);
                    finalsum<<<blockDim, numthreads>>>(dev2out_d, (&result_d[1]), n_elems);
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

            /* 결과 검증: GPU scan 결과의 마지막 원소(총합)를 CPU 순차 합산과 비교 */
            cuda_err_chk(cudaMemcpy(result_h, (result_d), (n_elems+1)*sizeof(EdgeT), cudaMemcpyDeviceToHost));
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

            //std::vector<uint64_t> a_h_vec (a_h, a_h+n_elems);
            //uint64_t total = std::accumulate(a_h_vec.begin(), a_h_vec.begin()+n_elems, 0, std::plus<uint64_t>());
            if(mem != BAFS_DIRECT){
                uint64_t total = 0;
                for(uint64_t count=0; count<n_elems; count++)
                    total+=a_h[count];
                printf("total in cpu: %llu \n", total);
            }
            printf("total in gpu: %llu \n ", result_h[n_elems]);
            auto itrend = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(itrend - itrstart);

            //if(mem == BAFS_DIRECT) {
            //         h_Aarray->print_reset_stats();
		    // printf("VA SSD: %d PageSize: %d itrTime: %f\n", settings.n_ctrls, settings.pageSize, (double)elapsed.count());
            //}



            if(mem == BAFS_DIRECT) {
                 h_Aarray->print_reset_stats();
                 cuda_err_chk(cudaDeviceSynchronize());
            }
            printf("\nVA %d A:%s Impl: %d \t SSD: %d \t CL: %d \t Cache: %llu \t TotalTime %f ms\n", titr, a_file_bin.c_str(), type, settings.n_ctrls, settings.pageSize,settings.maxPageCacheSize, milliseconds);
            fflush(stdout);
        }

        /* 자원 해제 */
        if(mem!=BAFS_DIRECT){
           free(a_h);
         }
        free(result_h);

        if((type == BASELINE_PC)) {
            //TODO: Fix this
            delete h_pc;
            delete h_Arange;
            delete h_Aarray;
        }

        if(mem!=BAFS_DIRECT){
            if(mem==UVM_DIRECT){
              a_d = a_d-2;  /* UVM_DIRECT에서 +2 했던 것을 원복 후 free */
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
