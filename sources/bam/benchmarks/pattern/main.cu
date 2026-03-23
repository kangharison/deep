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
 * BaM Pattern 벤치마크 (benchmarks/pattern/main.cu)
 * =============================================================================
 *
 * [목적]
 * 다양한 I/O 접근 패턴(순차/랜덤/스트라이드/Zipfian power-law)으로 NVMe SSD에
 * 저장된 데이터를 GPU에서 읽는 벤치마크이다. GPU 메모리, UVM, BaFS 등 다양한
 * 메모리 모드에서 동일 패턴을 실행하여 I/O 경로별 성능(IOPS, 대역폭)을 비교한다.
 *
 * [지원 접근 패턴 (impl_type)]
 * - SEQUENTIAL(0):          스레드 ID 순서대로 순차 접근 (GPU 메모리)
 * - SEQUENTIAL_PC(1):       순차 접근 + BaM page cache
 * - SEQUENTIAL_WARP(2):     워프 단위 NVMe 페이지 정렬 순차 접근 (GPU 메모리)
 * - SEQUENTIAL_WARP_PC(3):  워프 단위 순차 + BaM page cache
 * - RANDOM(4):              랜덤 페이지 접근 (GPU 메모리)
 * - RANDOM_PC(5):           랜덤 접근 + BaM page cache
 * - RANDOM_WARP(6):         워프 단위 랜덤 페이지 접근 (GPU 메모리)
 * - RANDOM_WARP_PC(7):      워프 단위 랜덤 + BaM page cache
 * - STRIDE(8):              스트라이드 접근 (GPU 메모리)
 * - STRIDE_PC(9):           스트라이드 + BaM page cache
 * - POWERLAW_WARP(13):      Zipfian 분포 페이지 접근 (GPU 메모리) - 핫 페이지에 접근 집중
 * - POWERLAW_WARP_PC(15):   Zipfian + BaM page cache
 *
 * [메모리 모드 (mem_type)]
 * - GPUMEM(0):       cudaMalloc + cudaMemcpy (전통적 GPU 메모리)
 * - UVM_READONLY(1): UVM + SetReadMostly 힌트
 * - UVM_DIRECT(2):   UVM + O_DIRECT + SetAccessedBy 힌트
 * - BAFS_DIRECT(6):  BaM page cache로 GPU가 NVMe SSD에서 직접 fetch
 *
 * [벤치마크 구조]
 * 1. 설정 파싱 및 CUDA 디바이스 초기화
 * 2. 입력 파일 로드 (메모리 모드에 따라 분기)
 * 3. 접근 패턴에 따라 assignment 배열 생성 (랜덤/Zipfian 모드)
 * 4. NVMe 컨트롤러 + page cache 초기화 (BAFS 모드)
 * 5. 커널 11회 반복 실행 (1회 웜업 + 10회 측정)
 * 6. IOPS, 대역폭 계산 및 출력
 */

#include <cuda.h>                  // CUDA 런타임 API
#include <fstream>                 // 파일 입출력 스트림
#include <stdint.h>                // 정수 타입 정의 (uint64_t 등)
#include <stdio.h>                 // C 표준 I/O (printf 등)
#include <iostream>                // C++ 스트림 I/O
#include <string.h>                // 문자열 처리 (memset 등)
#include <getopt.h>                // 커맨드라인 옵션 파싱
//#include "helper_cuda.h"
#include <algorithm>               // STL 알고리즘
#include <vector>                  // STL 벡터 컨테이너
#include <numeric>                 // 수치 연산 (accumulate 등)
#include <iterator>                // 반복자 유틸리티
#include <math.h>                  // 수학 함수 (ceil 등)
#include <chrono>                  // 고해상도 시간 측정
#include <ctime>                   // C 시간 함수
#include <ratio>                   // 시간 비율 타입
#include <cstdio>                  // C 표준 I/O (중복 포함)
#include <cstdint>                 // C++ 정수 타입
#include <cstring>                 // C++ 문자열 함수
#include <fcntl.h>                 // 파일 제어 (O_RDONLY, O_DIRECT 등)
#include <unistd.h>                // POSIX API (close 등)
#include <sys/mman.h>              // 메모리 매핑 (mmap, 미사용)
#include <stdexcept>               // 런타임 예외

/* BaM/libnvm 관련 헤더: NVMe 컨트롤러 제어, I/O 큐, 페이지 캐시 */
#include <nvm_ctrl.h>              // NVMe 컨트롤러 제어 인터페이스
#include <nvm_types.h>             // NVMe 타입 정의
#include <nvm_queue.h>             // NVMe I/O 큐 관리
#include <nvm_util.h>              // NVMe 유틸리티 함수
#include <nvm_admin.h>             // NVMe Admin 큐 커맨드
#include <nvm_error.h>             // NVMe 에러 코드
#include <nvm_cmd.h>               // NVMe I/O 커맨드 빌더
#include <buffer.h>                // DMA 버퍼 관리
#include "settings.h"              // 벤치마크 설정 구조체
#include <ctrl.h>                  // BaM Controller 클래스
#include <event.h>                 // 이벤트 동기화
#include <queue.h>                 // BaM 큐 래퍼
#include <nvm_parallel_queue.h>    // GPU 병렬 NVMe 큐
#include <nvm_io.h>                // NVMe I/O 헬퍼
#include <page_cache.h>            // page_cache_t, range_t, array_t, bam_ptr 등 BaM 핵심 타입
#include <util.h>                  // cuda_err_chk 등 유틸리티 매크로


#include <iterator>
#include <numeric>
#include <functional>              // 함수 객체 (plus 등)

#include "zip.h"                   // Zipfian 분포 생성기 (zipf_distribution 클래스)


#define UINT64MAX 0xFFFFFFFFFFFFFFFF    // uint64_t 최댓값
#define Align(size,alignment) (size+alignment-1) & ~(alignment-1)  // 정렬 매크로: size를 alignment의 배수로 올림

using error = std::runtime_error;
using std::string;
//const char* const ctrls_paths[] = {"/dev/libnvmpro0", "/dev/libnvmpro1", "/dev/libnvmpro2", "/dev/libnvmpro3", "/dev/libnvmpro4", "/dev/libnvmpro5", "/dev/libnvmpro6", "/dev/libnvmpro7"};
//const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
/* BaM 커널 모듈이 노출하는 NVMe 컨트롤러 디바이스 경로 목록. settings.n_ctrls개만큼 순서대로 사용한다. */
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm4", "/dev/libnvm9", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8"};

#define WARP_SHIFT 5       // log2(32) = 5, 워프 내 lane 인덱스 비트 시프트
#define WARPSIZE 32         // NVIDIA GPU 워프 크기: 32 스레드가 SIMT로 동시 실행

#define CHUNK_SHIFT 3       // log2(8) = 3, chunk 단위 접근용
#define CHUNK_SIZE (1 << CHUNK_SHIFT)  // 8개 원소 단위 chunk

#define BLOCK_NUM 1024ULL   // 2D 그리드 x 방향 블록 수 상한

#define MAXWARP 64          // SM당 최대 워프 수

typedef uint64_t EdgeT;     // 원소 타입: 8바이트 unsigned integer

/*
 * impl_type: I/O 접근 패턴 열거형 (-v 옵션)
 *
 * [순차 패턴]
 * - SEQUENTIAL(0):          각 스레드가 자신의 tid부터 stride=gridDim*blockDim으로 순차 접근
 * - SEQUENTIAL_PC(1):       SEQUENTIAL + BaM page cache (bam_ptr로 NVMe에서 on-demand fetch)
 * - SEQUENTIAL_WARP(2):     워프 단위: 각 워프가 NVMe 페이지 경계에 정렬하여 순차 접근
 * - SEQUENTIAL_WARP_PC(3):  SEQUENTIAL_WARP + BaM page cache + stride 기반 워프 재매핑
 *
 * [랜덤 패턴]
 * - RANDOM(4):              CPU에서 rand()로 생성한 assignment 배열 기반 랜덤 원소 접근
 * - RANDOM_PC(5):           RANDOM + BaM page cache
 * - RANDOM_WARP(6):         CPU에서 rand()로 생성한 assignment 배열 기반 랜덤 페이지 접근 (워프 단위)
 * - RANDOM_WARP_PC(7):      RANDOM_WARP + BaM page cache + stride 재매핑
 *
 * [스트라이드 패턴]
 * - STRIDE(8):              scan 패턴과 유사한 2-element stride 접근
 * - STRIDE_PC(9):           STRIDE + BaM page cache
 * - STRIDE_WARP(10):        워프 단위 stride (미구현)
 * - STRIDE_WARP_PC(11):     워프 단위 stride + BaM page cache (미구현)
 *
 * [Power-law / Zipfian 패턴]
 * - POWERLAW_WARP(13):      Zipfian 분포로 페이지 할당 (소수 핫 페이지에 접근 집중, GPU 메모리)
 * - POWERLAW_WARP_PC(15):   POWERLAW_WARP + BaM page cache
 */
typedef enum {
    SEQUENTIAL         = 0,
    SEQUENTIAL_PC      = 1,
    SEQUENTIAL_WARP    = 2,
    SEQUENTIAL_WARP_PC = 3,
    RANDOM             = 4,
    RANDOM_PC          = 5,
    RANDOM_WARP        = 6,
    RANDOM_WARP_PC     = 7,
    STRIDE             = 8,
    STRIDE_PC          = 9,
    STRIDE_WARP        = 10,
    STRIDE_WARP_PC     = 11,
    POWERLAW_WARP      = 13,
    POWERLAW_WARP_PC   = 15,
} impl_type;

/*
 * mem_type: 데이터 적재 방식 (-m 옵션)
 * - GPUMEM(0):       cudaMalloc + cudaMemcpy (전통적 GPU 메모리)
 * - UVM_READONLY(1): cudaMallocManaged + SetReadMostly 힌트 (GPU 읽기 전용 복제본)
 * - UVM_DIRECT(2):   cudaMallocManaged + O_DIRECT + SetAccessedBy 힌트
 * - BAFS_DIRECT(6):  BaM page cache로 GPU가 NVMe에서 직접 fetch (CPU 사전 로드 불필요)
 */
typedef enum {
    GPUMEM = 0,
    UVM_READONLY = 1,
    UVM_DIRECT = 2,
    BAFS_DIRECT= 6,
} mem_type;


/*
 * kernel_sequential: 순차 접근 패턴 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - 각 스레드가 tid부터 시작하여 stride=blockDim.x*gridDim.x 간격으로 원소를 순회
 * - num_elems개 원소를 모두 합산하여 val에 누적
 *
 * [데이터 접근 패턴]
 * - 전형적인 grid-stride 루프. 모든 스레드가 연속 주소를 접근하므로 완벽한 coalesced access
 * - 결과는 output[0]에 기록 (경쟁 조건 있지만 정확성보다 패턴 테스트가 목적)
 *
 * [launch_bounds(64,32)]
 * SM당 최대 64 스레드/블록, 32 블록 동시 실행. 레지스터 사용량 제한으로 occupancy 확보
 */
template<typename T>
__global__ __launch_bounds__(64,32)
void kernel_sequential(T *input, uint64_t num_elems, unsigned long long int* output){
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;  // 전역 스레드 ID

    uint64_t val=0;
    for(uint64_t i=tid; i < num_elems; i+= blockDim.x*gridDim.x){  // grid-stride 루프: 모든 원소를 분담 처리
        val += input[i];                                             // 순차적으로 원소를 합산
    }

    if(threadIdx.x ==0)
        //atomicAdd(&(output[0]), val);
        output[0] = val;   // 블록의 첫 스레드가 결과를 기록 (정확한 reduction은 아님, 패턴 테스트용)
}

/*
 * kernel_sequential_pc: 순차 접근 + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr 사용]
 * - array_d_t<T>*를 감싸는 bam_ptr<T>를 통해 NVMe SSD에서 on-demand로 데이터를 fetch
 * - ptr[i] 접근 시 page cache lookup -> miss이면 NVMe READ 발행 -> cache에 적재 후 반환
 * - 나머지 알고리즘은 kernel_sequential과 동일
 */
template<typename T>
__global__ //__launch_bounds__(64,32)
void kernel_sequential_pc(array_d_t<T>* dr, T *input, uint64_t num_elems, unsigned long long int* output){
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
	bam_ptr<T> ptr(dr);   // bam_ptr 생성: NVMe 투명 접근 제공

    uint64_t val=0;
    for(uint64_t i=tid; i < num_elems; i+= blockDim.x*gridDim.x){
        val += ptr[i];     // bam_ptr의 operator[]: cache miss시 NVMe I/O 발생
        //atomicAdd(&(output[0]), ptr[i]);
    }

    if(threadIdx.x ==0)
        //atomicAdd(&(output[0]), val);
        output[0] = val;
}


/*
 * kernel_random: 랜덤 접근 패턴 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - grid-stride 루프로 모든 원소를 분담
 * - assignment[i]: CPU에서 rand()로 미리 생성한 랜덤 인덱스 배열
 * - input[assignment[i]]: 랜덤 위치의 원소를 읽음
 *
 * [데이터 접근 패턴]
 * - 워프 내 32개 스레드가 각기 다른 랜덤 주소를 접근하므로 coalescing이 불가능
 * - 캐시 효율이 매우 낮음 -> BaM page cache에서의 miss rate가 높아 NVMe I/O가 빈번
 */
template<typename T>
__global__ __launch_bounds__(64,32)
void kernel_random(T *input, uint64_t* assignment, uint64_t num_elems, unsigned long long int* output){
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;

    uint64_t val=0;
    for(uint64_t i=tid; i < num_elems; i+= blockDim.x*gridDim.x){
        if(i < num_elems)
            val += input[assignment[i]];   // 랜덤 인덱스를 통한 간접 접근 (scattered read)
    }

    if(threadIdx.x ==0)
        output[0] = val;
}

/*
 * kernel_random_pc: 랜덤 접근 + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr + 랜덤 접근]
 * - 랜덤 인덱스 접근이므로 page cache miss rate가 높음
 * - 각 miss마다 NVMe READ가 발행되어 latency가 크게 증가
 * - 이 패턴은 BaM page cache의 worst-case 성능을 측정하는 데 사용
 * - 주석 처리된 stride/nep 코드: 워프 재매핑을 적용하려 했으나 현재 비활성화
 */
template<typename T>
__global__ //__launch_bounds__(64,32)
void kernel_random_pc(array_d_t<T>* dr, T *input, uint64_t* assignment, uint64_t num_elems, unsigned long long int* output){
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
	bam_ptr<T> ptr(dr);
    //uint64_t stride = 8192;
    //uint64_t nep = (num_elems+stride)/stride;
    //uint64_t ntid = (tid/nep) + ((tid % nep)* stride);
    uint64_t ntid = tid;   // 현재는 워프 재매핑 미적용 (stride 코드가 주석 처리됨)
    uint64_t val=0;
    for(uint64_t i=ntid; i < num_elems; i+= blockDim.x*gridDim.x){
        if(i < num_elems)
            val += ptr[assignment[i]];   // bam_ptr로 랜덤 위치 NVMe 접근
    }

    if(threadIdx.x ==0)
        output[0] = val;
}



/*
 * kernel_stride: 스트라이드 접근 패턴 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - 각 블록이 2*blockDim.x개 원소를 처리 (scan 패턴과 유사)
 * - 스레드 tid가 input[tid]과 input[tid+stride] 두 원소를 읽어 합산
 *
 * [데이터 접근 패턴]
 * - blockDim.x 간격의 stride 접근: 첫 접근은 연속이지만, 두 번째 접근은 stride만큼 떨어짐
 * - 이는 scan 커널의 shared memory 로드 패턴과 유사 (BaM에서 scan I/O 패턴 시뮬레이션)
 */
template<typename T>
__global__ __launch_bounds__(64,32)
//launch 2*threads as blocksize- more of scan pattern.
void kernel_stride(T *input, uint64_t num_elems, unsigned long long int* output){

    uint64_t stride = blockDim.x;                                    // stride = 블록 크기
    uint64_t tid = 2*blockIdx.x * blockDim.x + threadIdx.x;         // 각 블록이 2*blockDim.x 구간 담당


    uint64_t val=0;
    if(tid <num_elems)
        val = input[tid];                // 첫 번째 원소 읽기 (연속 주소)

    if((tid+stride)<num_elems)
        val += input[tid+stride];        // 두 번째 원소 읽기 (stride만큼 떨어진 주소)

    __syncthreads();                     // 블록 내 동기화 (불필요하지만 원본 유지)

    if(threadIdx.x ==0)
        output[0] = val;
}


/*
 * kernel_sequential_warp: 워프 단위 NVMe 페이지 정렬 순차 접근 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - n_warps개 워프가 각각 n_pages_per_warp개의 NVMe 페이지를 담당
 * - 워프 내 32개 lane이 stride=32로 페이지 내 원소를 순회
 *
 * [데이터 접근 패턴 - NVMe 페이지 정렬]
 * - start_page = n_pages_per_warp * warp_id: 이 워프의 시작 페이지
 * - 워프 내 모든 lane이 같은 NVMe 페이지를 접근 -> page cache hit rate 극대화
 * - BaM page cache 사용 시 워프당 1번의 NVMe fetch만 필요 (coalesced NVMe access)
 */
template<typename T>
__global__ __launch_bounds__(64,32)
void kernel_sequential_warp(T *input, uint64_t n_elems,  uint64_t n_pages_per_warp, unsigned long long* sum,  uint64_t n_warps, size_t page_size) {

    const uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const uint64_t lane = tid % 32;                                  // 워프 내 lane 인덱스 (0~31)
    const uint64_t warp_id = tid / 32;                               // 전역 워프 ID
    const uint64_t n_elems_per_page = page_size / sizeof(T);         // 한 NVMe 페이지의 원소 수 (예: 4096/8=512)
    T v = 0;
    uint64_t idx=0;

    if (warp_id < n_warps) {
        size_t start_page = n_pages_per_warp * warp_id;;             // 이 워프가 담당하는 시작 페이지
        //if (lane == 0) printf("start_page: %llu\n", (unsigned long long) start_page);
        for (size_t i = 0; i < n_pages_per_warp; i++) {              // 워프당 n_pages_per_warp개 페이지 순회
            size_t cur_page = start_page + i;
            //printf("warp_id: %llu\tcur_page: %llu\n", (unsigned long long) warp_id, (unsigned long long) cur_page);
            size_t start_idx = cur_page * n_elems_per_page + lane;   // 이 lane의 시작 인덱스 (페이지 내 오프셋)

            for (size_t j = 0; j < n_elems_per_page; j += WARPSIZE) {  // 페이지 내 원소를 stride=32로 순회
                    //printf("startidx: %llu\n", (unsigned long long) (start_idx+j));
                    idx = start_idx + j;
                    if(idx < n_elems)
                        v += input[idx];                             // 순차적 coalesced 읽기
            }

        }
          sum[0] = v;                                                // 결과 기록 (패턴 테스트용)
        //atomicAdd(&sum[0], v);
    }

}

/*
 * kernel_sequential_warp_pc: 워프 단위 순차 접근 + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr + stride 기반 워프 재매핑]
 * - bam_ptr<T>을 통해 NVMe SSD에서 직접 데이터를 fetch
 * - stride 파라미터로 워프 ID를 재매핑하여 인접 워프들이 서로 다른 NVMe 컨트롤러/채널의
 *   페이지를 접근하도록 분산 (interleaving으로 다중 SSD 대역폭 극대화)
 *
 * [워프 ID 재매핑 공식]
 * nep = (n_warps + stride - 1) / stride
 * warp_id = (old_warp_id / nep) + ((old_warp_id % nep) * stride)
 * -> 연속된 old_warp_id를 stride 간격으로 분산시켜 NVMe 채널 병렬성 활용
 */
template<typename T>
__global__ //__launch_bounds__(64,32)
void kernel_sequential_warp_pc(array_d_t<T>* dr, T *input, uint64_t n_elems, uint64_t n_pages_per_warp, unsigned long long* sum,  uint64_t n_warps, size_t page_size, uint64_t stride) {

    const uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const uint64_t lane = tid % 32;
    const uint64_t old_warp_id = tid / 32;
    const uint64_t n_elems_per_page = page_size / sizeof(T);
    T v = 0;
    uint64_t idx =0;
    /* 워프 ID 재매핑: stride를 사용하여 인접 워프를 서로 다른 NVMe 채널로 분산 */
    uint64_t nep = (n_warps+stride-1)/stride;
    uint64_t warp_id = (old_warp_id/nep) + ((old_warp_id % nep)* stride);

    if (warp_id < n_warps) {
		bam_ptr<T> ptr(dr);                                          // bam_ptr 생성: NVMe 투명 접근
        size_t start_page = n_pages_per_warp * warp_id;;
        //	if (lane == 0) printf("start_page: %llu\n", (unsigned long long) start_page);
        for (size_t i = 0; i < n_pages_per_warp; i++) {
            size_t cur_page = start_page + i;
            //	    printf("warp_id: %llu\tcur_page: %llu\n", (unsigned long long) warp_id, (unsigned long long) cur_page);
            size_t start_idx = cur_page * n_elems_per_page + lane;

            for (size_t j = 0; j < n_elems_per_page; j += WARPSIZE) {
                    //printf("startidx: %llu\n", (unsigned long long) (start_idx+j));
                    idx = start_idx + j;
                    if(idx < n_elems)
                        v += ptr[idx];                               // bam_ptr[]: page cache miss시 NVMe READ 발행
                        //v = ptr[idx];
                        //atomicAdd(&sum[0], v);
            }
        }
        sum[0] = v;
    }
}

/*
 * kernel_random_warp: 워프 단위 랜덤 페이지 접근 커널 (GPU 메모리 / UVM 모드용)
 *
 * [GPU 스레드 구조]
 * - 각 워프가 assignment[warp_id]에 지정된 랜덤 시작 페이지부터 n_pages_per_warp개 페이지를 읽음
 * - stride 파라미터로 워프 ID를 재매핑 (NVMe 채널 분산)
 *
 * [데이터 접근 패턴]
 * - 워프 간: 각 워프의 시작 페이지가 랜덤이므로 워프 간 접근이 비연속적 -> page cache miss 빈번
 * - 워프 내: 같은 페이지를 32개 lane이 coalesced하게 접근 -> 페이지 당 1번의 NVMe fetch
 * - RANDOM vs RANDOM_WARP: RANDOM은 원소 단위 랜덤, RANDOM_WARP은 페이지 단위 랜덤
 */
template<typename T>
__global__ __launch_bounds__(64,32)
void kernel_random_warp(T *input,uint64_t n_elems, uint64_t n_pages_per_warp, unsigned long long* sum,  uint64_t* assignment, uint64_t n_warps, size_t page_size, uint64_t stride) {

    const uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const uint64_t lane = tid % 32;
    const uint64_t old_warp_id = tid / 32;
    const uint64_t n_elems_per_page = page_size / sizeof(T);

    /* 워프 ID 재매핑 */
    uint64_t nep = (n_warps+stride-1)/stride;
    uint64_t warp_id = (old_warp_id/nep) + ((old_warp_id % nep)* stride);

    T v = 0;
    uint64_t idx=0;
    if (warp_id < n_warps) {
        size_t start_page = assignment[warp_id];                     // 랜덤 시작 페이지 (CPU에서 미리 생성)
        //	if (lane == 0) printf("start_page: %llu\n", (unsigned long long) start_page);
        for (size_t i = 0; i < n_pages_per_warp; i++) {
            size_t cur_page = start_page + i;
            //	    printf("warp_id: %llu\tcur_page: %llu\n", (unsigned long long) warp_id, (unsigned long long) cur_page);
            size_t start_idx = cur_page * n_elems_per_page + lane;

            for (size_t j = 0; j < n_elems_per_page; j += 32) {     // 페이지 내 원소를 stride=32로 순회
            //		printf("startidx: %llu\n", (unsigned long long) (start_idx+j));
                    idx = start_idx + j;
                    if(idx < n_elems)
                        v += input[idx];
            }

        }
        *sum = v;
    }

}


/*
 * kernel_random_warp_pc: 워프 단위 랜덤 페이지 접근 + BaM page cache 버전
 *
 * [핵심 차이점: bam_ptr로 NVMe 직접 접근]
 * - kernel_random_warp과 동일한 랜덤 페이지 접근 패턴이지만 bam_ptr을 사용
 * - 랜덤 페이지 접근이므로 page cache의 locality가 낮아 miss rate가 높음
 * - 이 커널은 POWERLAW_WARP_PC에서도 재사용됨 (assignment 배열만 Zipfian 분포로 변경)
 */
template<typename T>
__global__ //__launch_bounds__(64,32)
void kernel_random_warp_pc(array_d_t<T>* dr, T *input, uint64_t n_elems, uint64_t n_pages_per_warp, unsigned long long* sum,  uint64_t* assignment, uint64_t n_warps, size_t page_size, uint64_t stride) {

    const uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const uint64_t lane = tid % 32;
    const uint64_t old_warp_id = tid / 32;
    const uint64_t n_elems_per_page = page_size / sizeof(T);

    /* 워프 ID 재매핑: 인접 워프를 서로 다른 NVMe 채널로 분산 */
    uint64_t nep = (n_warps+stride-1)/stride;
    uint64_t warp_id = (old_warp_id/nep) + ((old_warp_id % nep)* stride);
    //uint64_t warp_id = old_warp_id;
    T v = 0;
    if (warp_id < n_warps) {
		bam_ptr<T> ptr(dr);                                          // bam_ptr 생성
        size_t start_page = assignment[warp_id];                     // 랜덤(또는 Zipfian) 시작 페이지
        //	if (lane == 0) printf("start_page: %llu\n", (unsigned long long) start_page);
        for (size_t i = 0; i < n_pages_per_warp; i++) {
            size_t cur_page = start_page + i;
            //	    printf("warp_id: %llu\tcur_page: %llu\n", (unsigned long long) warp_id, (unsigned long long) cur_page);
            size_t start_idx = cur_page * n_elems_per_page + lane;

            for (size_t j = 0; j < n_elems_per_page; j += 32) {
            //		printf("startidx: %llu\n", (unsigned long long) (start_idx+j));
                        v += ptr[start_idx+j];                       // bam_ptr[]: cache miss시 NVMe READ 발행
            }
        }
        *sum = v;
    }
}








/*
 * =============================================================================
 * main 함수: 설정 파싱 -> 데이터 로드 -> 접근 패턴 생성 -> 커널 실행 -> 성능 출력
 * =============================================================================
 *
 * [전체 흐름]
 * 1. Settings::parseArguments()로 커맨드라인 옵션 파싱
 * 2. CUDA 디바이스 초기화 및 이벤트 생성
 * 3. 입력 파일(.dst) 로드: 메모리 모드에 따라 GPUMEM/UVM/BAFS 분기
 * 4. 접근 패턴에 따라 assignment 배열 생성:
 *    - RANDOM/RANDOM_WARP: rand()로 랜덤 페이지 인덱스 생성
 *    - POWERLAW_WARP: zipf_distribution(s=1.8)으로 Zipfian 분포 페이지 인덱스 생성
 * 5. BAFS 모드: NVMe 컨트롤러 + page_cache + range + array 초기화
 * 6. 커널 11회 반복 (0회차는 웜업, 1~10회차의 평균이 실제 측정값)
 * 7. IOPS = ios * 1000 / time_ms, bandwidth = data * 1000 / time_ms
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

    std::ifstream filea;          // 입력 파일 스트림
    std::string a_file;           // 입력 파일 기본명
    std::string a_file_bin;       // 입력 파일 .dst 경로
    std::string filename;

    impl_type type;               // 커널 구현 타입 (접근 패턴)
    mem_type mem;                 // 메모리 할당 모드
    uint64_t *a_h, *a_d;         // 호스트/디바이스 입력 배열 포인터
    uint64_t numblocks, numthreads;  // CUDA 그리드/블록 크기

    float milliseconds;           // 커널 실행 시간 (밀리초)

    uint64_t pc_page_size;        // BaM page cache 페이지 크기 (바이트)
    uint64_t pc_pages;            // page cache 총 페이지 수

    try{

        a_file = std::string(settings.input_a);

        type = (impl_type) settings.type;
        mem = (mem_type) settings.memalloc;

        pc_page_size = settings.pageSize;
        /* page cache 총 페이지 수 = maxPageCacheSize / pageSize (올림) */
        pc_pages = ceil((float)settings.maxPageCacheSize/pc_page_size);

        numthreads = settings.numThreads;

        cuda_err_chk(cudaSetDevice(settings.cudaDevice));

        /* CUDA 이벤트: 커널 실행 시간 측정용 */
        cudaEvent_t start, end, tstart, tend;
        cuda_err_chk(cudaEventCreate(&start));
        cuda_err_chk(cudaEventCreate(&end));
        cuda_err_chk(cudaEventCreate(&tstart));
        cuda_err_chk(cudaEventCreate(&tend));


        /* 입력 파일 경로: .bel 그래프 파일의 .dst 확장자 사용 (바이너리: [8B hdr][8B hdr][data...]) */
        a_file_bin = a_file + ".dst";

        std::cout << "A: " << a_file_bin  << std::endl;

        uint64_t n_elems = settings.n_elems;              // 벡터 원소 수
        uint64_t n_elems_size = n_elems * sizeof(uint64_t);  // 전체 데이터 크기 (바이트)
        printf("Total elements: %llu \n", n_elems);
        uint64_t tmp;

        /* 파일 읽기: 처음 16바이트 헤더 스킵 후 데이터 로드 */
        filea.open(a_file_bin.c_str(), std::ios::in | std::ios::binary);
        if (!filea.is_open()) {
            printf("A file open failed\n");
            exit(1);
        };

        filea.read((char*)(&tmp), 16);                     // 16바이트 헤더 읽기 (vertex_count + typeT)
        if(mem != BAFS_DIRECT)
            a_h = (uint64_t*)calloc(n_elems_size, sizeof(uint64_t));  // BAFS가 아니면 호스트 버퍼 할당
        if((mem!=BAFS_DIRECT) &&  (mem != UVM_DIRECT)){
             filea.read((char*)a_h, n_elems_size);         // 호스트 버퍼에 데이터 읽기
             filea.close();
        }

        /*
         * 메모리 모드별 데이터 준비:
         * - GPUMEM: cudaMalloc + cudaMemcpy로 GPU VRAM에 전부 적재
         * - UVM_READONLY: UVM 할당 + 복사 + SetReadMostly 힌트 (GPU 측 읽기 캐싱)
         * - UVM_DIRECT: O_DIRECT로 파일을 UVM 영역에 직접 fread + SetAccessedBy 힌트
         * - BAFS_DIRECT: 사전 로드 불필요 (bam_ptr이 on-demand fetch)
         */
        switch (mem) {
            case GPUMEM:
                {
                cuda_err_chk(cudaMalloc((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                break;
                }
            case UVM_READONLY:
                {
                cuda_err_chk(cudaMallocManaged((void**)&a_d, n_elems_size));
                cuda_err_chk(cudaMemcpy(a_d, a_h, n_elems_size, cudaMemcpyHostToDevice));
                cuda_err_chk(cudaMemAdvise(a_d, n_elems_size, cudaMemAdviseSetReadMostly, settings.cudaDevice));
                break;
                }
            case UVM_DIRECT:
                {
                filea.close();
                /* O_DIRECT: OS 페이지 캐시를 우회하여 SSD에서 직접 UVM 영역으로 읽음 */
                int fda = open(a_file_bin.c_str(), O_RDONLY | O_DIRECT);
                FILE *fa_tmp= fdopen(fda, "rb");
                if ((fa_tmp == NULL) || (fda == -1)) {
                    printf("A file fd open failed\n");
                    exit(1);
                }

                /* 4K 정렬: O_DIRECT 요구사항. 버퍼와 크기를 4KB 경계에 올림 정렬 */
                uint64_t count_4k_aligned = ((n_elems + 2 + 4096 / sizeof(uint64_t)) / (4096 / sizeof(uint64_t))) * (4096 / sizeof(uint64_t));
                //uint64_t count_4k_aligned = n_elems;
                uint64_t size_4k_aligned = count_4k_aligned * sizeof(uint64_t);

                cuda_err_chk(cudaMallocManaged((void**)&a_d, size_4k_aligned));
                /* SetAccessedBy: GPU가 이 메모리에 접근할 것임을 CUDA 드라이버에 힌트 */
                cuda_err_chk(cudaMemAdvise(a_d, size_4k_aligned, cudaMemAdviseSetAccessedBy, settings.cudaDevice));
                high_resolution_clock::time_point ft1 = high_resolution_clock::now();

                if (fread(a_d, sizeof(uint64_t), count_4k_aligned, fa_tmp)) {
                    printf("A file fread failed: %llu \t %llu\n", count_4k_aligned, n_elems+2);
                    exit(1);
                }
                fclose(fa_tmp);
                close(fda);

                a_d = a_d + 2;      // 16바이트 헤더(2개 uint64_t)를 건너뛰기 위해 포인터 +2

                high_resolution_clock::time_point ft2 = high_resolution_clock::now();
                duration<double> time_span = duration_cast<duration<double>>(ft2 -ft1);
                std::cout<< "file read time: "<< time_span.count() <<std::endl;

                break;
                }
            case BAFS_DIRECT:
                {
                /* BAFS_DIRECT: 파일 사전 로드 불필요. bam_ptr이 커널 내에서 on-demand로 NVMe에서 fetch */
                break;
                }
        }


        uint64_t n_pc_pages = ceil(((float)n_elems_size)/pc_page_size);  // 전체 데이터가 차지하는 NVMe 페이지 수
        uint64_t blocksize = 64;       // CUDA 스레드 블록 크기 (패턴 벤치마크에서 고정)
        uint64_t n_warps =0;           // 워프 수 (WARP 모드에서만 사용)


        /*
         * 접근 패턴별 그리드 크기 계산:
         * - SEQUENTIAL/RANDOM: 스레드 단위 -> numblocks = ceil(numthreads / blocksize)
         * - *_WARP: 워프 단위 -> n_warps = total_threads / 32, n_warps가 n_pc_pages보다 크면 에러
         */
        switch (type) {
            case SEQUENTIAL:
            case RANDOM:
            case SEQUENTIAL_PC:
            case RANDOM_PC:
            //case POWERLAW:
            {
				numblocks = ((numthreads+blocksize-1)/blocksize);
                printf("numblocks:%llu \n", numblocks);
                break;
            }
			case SEQUENTIAL_WARP:
            case RANDOM_WARP:
            case POWERLAW_WARP:
            case SEQUENTIAL_WARP_PC:
            case RANDOM_WARP_PC:
            case POWERLAW_WARP_PC:
			{
                 numblocks = (numthreads + blocksize - 1)/blocksize;//80*16;
                 n_warps = blocksize * numblocks/ WARPSIZE;  // 총 워프 수
                 if(n_warps > n_pc_pages){
                     printf("Error: Cannot have n_warps greater than n_elems.\n");
                     printf("n_warps: %llu \t n_pc_pages:%llu \n", n_warps, n_pc_pages);
                 }
                 printf("n_warps: %llu \t numblocks:%llu \n", n_warps, numblocks);
                break;
			}
            default:
                fprintf(stderr, "Invalid type\n");
                exit(1);
                break;
        }

        //dim3 blockDim(BLOCK_NUM, (numblocks+BLOCK_NUM)/BLOCK_NUM);
        dim3 blockDim((numblocks));
        if((type == SEQUENTIAL_PC) || (type == SEQUENTIAL_WARP_PC) || (type == RANDOM_PC) || (type == RANDOM_WARP_PC) || (type == STRIDE_PC) || (type == STRIDE_WARP_PC) || (type == POWERLAW_WARP_PC)) {
                printf("page size: %d, pc_entries: %llu\n", pc_page_size, pc_pages);
        }

        /* 결과 저장용 메모리 할당 */
        unsigned long long int *output_h;
        output_h = (unsigned long long int*) malloc(sizeof(unsigned long long int));
		output_h[0] = 0;
        uint64_t* assignment_h;    // 호스트 측 페이지/원소 할당 배열 (랜덤/Zipfian 패턴용)
        uint64_t* assignment_d;    // 디바이스 측 할당 배열

        uint64_t n_data_pages = (uint64_t) n_elems_size/pc_page_size;  // 데이터가 차지하는 NVMe 페이지 수

        /*
         * RANDOM 패턴: CPU에서 rand()로 랜덤 원소 인덱스(페이지 번호) 생성
         * 각 스레드에 랜덤 페이지를 할당하여 scattered read 패턴 생성
         */
        if((type == RANDOM) || (type == RANDOM_PC)){
            printf("I am called %llu\n", n_data_pages);
            assignment_h = (uint64_t*) malloc (numthreads*sizeof(uint64_t));
            for(uint64_t i=0; i< numthreads; i++){
                uint64_t page = rand() % n_data_pages;   // [0, n_data_pages) 범위의 랜덤 페이지
                assignment_h[i] = page;
            }
            cuda_err_chk(cudaMalloc(&assignment_d, numthreads*sizeof(uint64_t)));
            cuda_err_chk(cudaMemcpy(assignment_d, assignment_h, numthreads*sizeof(uint64_t), cudaMemcpyHostToDevice));
        }
        /*
         * RANDOM_WARP 패턴: 워프 단위 랜덤 페이지 할당
         * 각 워프에 랜덤 시작 페이지를 할당하여 워프 내에서는 순차, 워프 간에는 랜덤 접근
         */
        if((type == RANDOM_WARP) || (type == RANDOM_WARP_PC)){
            assignment_h = (uint64_t*) malloc (n_warps*sizeof(uint64_t));
            //std::random_device rd;  //Will be used to obtain a seed for the random number engine
            //std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
            //std::uniform_int_distribution<uint64_t> distrib(0, n_data_pages);

            for(uint64_t i=0; i< n_warps; i++){
                //uint64_t page = distrib(gen);
                uint64_t page = rand() % n_data_pages;   // 각 워프에 랜덤 시작 페이지 할당
                assignment_h[i] = page;
                //printf("%llu \t", page);
            }
            cuda_err_chk(cudaMalloc(&assignment_d, n_warps*sizeof(uint64_t)));
            cuda_err_chk(cudaMemcpy(assignment_d, assignment_h, n_warps*sizeof(uint64_t), cudaMemcpyHostToDevice));
        }


        /*
         * POWERLAW_WARP 패턴: Zipfian 분포로 워프별 시작 페이지 할당
         * - zipf_distribution(unique_keys, 1.8): s=1.8 -> 매우 skewed, 소수 핫 페이지에 접근 집중
         * - s=1.25 -> 70/30, s=1.45 -> 80/20, s=1.8 -> 90/10 접근 분포
         * - unique_keys = n_data_pages: 모든 페이지가 잠재적 접근 대상
         * - 시드: settings.seed > 0이면 사용자 지정, 아니면 하드웨어 랜덤
         * - 이 패턴으로 BaM page cache의 LRU eviction 효과를 평가 (핫 페이지 캐싱 효율)
         */
        if((type == POWERLAW_WARP) || (type == POWERLAW_WARP_PC)){
            std::random_device rd;
            unsigned seed;
            if(settings.seed >0)
               seed = settings.seed;       // 사용자 지정 시드 (재현성 보장)
            else
                seed = rd();               // 하드웨어 엔트로피 기반 시드
            std::mt19937 gen(seed);        // Mersenne Twister PRNG
            uint64_t unique_keys = n_data_pages;//pc_pages*8; // 8 times is picked for making sure all keys do not fall within the cache and hence there are misses.
			//if(n_warps < unique_keys)
			//	printf("WARNING: powerlaw pattern requires unique keys (%llu) to be smaller than the n_warps (%llu). Either reduce the cache size or increase n_warps.\n", unique_keys, n_warps);
			//TODO: Control alpha or zipf coefficient.
			zipf_distribution<uint64_t> zipf(unique_keys, 1.8);  // Zipfian 분포 (s=1.8, 매우 skewed)
            assignment_h = (uint64_t*) malloc (n_warps*sizeof(uint64_t));
            for(uint64_t i=0; i< n_warps; i++){
                assignment_h[i] = zipf(gen);   // Zipfian 분포에서 페이지 번호 샘플링
            }
            cuda_err_chk(cudaMalloc(&assignment_d, n_warps*sizeof(uint64_t)));
            cuda_err_chk(cudaMemcpy(assignment_d, assignment_h, n_warps*sizeof(uint64_t), cudaMemcpyHostToDevice));
        }

		printf("Allocation finished\n");
        fflush(stdout);

        /*
         * BaM NVMe 컨트롤러 초기화 (BAFS_DIRECT 전용):
         * Controller(path, namespace, cudaDevice, queueDepth, numQueues)
         * - NVMe admin 큐로 컨트롤러를 초기화하고 GPU 접근 가능한 메모리에 I/O SQ/CQ를 할당
         */
        std::vector<Controller*> ctrls(settings.n_ctrls);
        if(mem == BAFS_DIRECT){
            for (size_t i = 0 ; i < settings.n_ctrls; i++)
                ctrls[i] = new Controller(ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);
            printf("Controllers Created\n");
        }
        printf("Initialization done\n");
        fflush(stdout);

        /*
         * BaM page cache 및 배열 추상화 초기화 (_PC 모드 전용):
         * - page_cache_t: GPU VRAM에 pc_pages개 페이지 슬롯 할당. LRU eviction 정책
         * - range_t<uint64_t>: 논리적 원소 범위 [0, n_elems)를 NVMe LBA 범위에 매핑
         * - array_t<uint64_t>: range들을 묶어 단일 논리 배열로 추상화. d_array_ptr이 커널에 전달됨
         */
        page_cache_t* h_pc;
        range_t<uint64_t>* h_Arange;
        std::vector<range_t<uint64_t>*> vec_Arange(1);
        array_t<uint64_t>* h_Aarray;


        if((type == SEQUENTIAL_PC) || (type == SEQUENTIAL_WARP_PC) || (type == RANDOM_PC) || (type == RANDOM_WARP_PC) || (type == STRIDE_PC) || (type == STRIDE_WARP_PC) || (type == POWERLAW_WARP_PC)) {
            h_pc =new page_cache_t(pc_page_size, pc_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
            h_Arange = new range_t<uint64_t>((uint64_t)0 ,(uint64_t)n_elems, (uint64_t) (ceil(settings.afileoffset*1.0/pc_page_size)),(uint64_t)n_pc_pages, (uint64_t)0, (uint64_t)pc_page_size, h_pc, settings.cudaDevice);
            vec_Arange[0] = h_Arange;
            h_Aarray = new array_t<uint64_t>(n_elems, settings.afileoffset, vec_Arange, settings.cudaDevice);

            printf("Page cache initialized\n");
            fflush(stdout);
        }

		uint64_t n_pages_per_warp = settings.coarse;       // 워프당 처리할 NVMe 페이지 수 (coarsening 팩터)
		uint64_t n_elems_per_page = pc_page_size/sizeof(uint64_t);  // 한 NVMe 페이지의 원소 수 (예: 4096/8=512)

		unsigned long long int* output_d;
        cuda_err_chk(cudaMalloc(&output_d, sizeof(unsigned long long)));


        float totaltime = 0;      // 누적 실행 시간 (웜업 제외)
        float avgtime = 0;        // 평균 실행 시간

        /* 벤치마크 반복 실행: 11회 (0회차는 웜업, 1~10회차가 실제 측정) */
        for(int titr=0; titr<11; titr+=1){
            cuda_err_chk(cudaEventRecord(start, 0));
        	cuda_err_chk(cudaMemset(output_d, 0, sizeof(unsigned long long)));  // 결과 초기화

            auto itrstart = std::chrono::system_clock::now();

            /* 접근 패턴별 커널 디스패치 */
            switch (type) {
                case SEQUENTIAL:{
					kernel_sequential<uint64_t><<<blockDim, blocksize>>>(a_d, numthreads, output_d);
                    break;
                }
                case SEQUENTIAL_PC:{
                    //printf("blockDim.x is %llu \t blocksize: %llu\n", blockDim.x, blocksize );
                    kernel_sequential_pc<uint64_t><<<blockDim, blocksize>>>(h_Aarray->d_array_ptr,a_d, numthreads, output_d);
                    break;
                }
                case SEQUENTIAL_WARP:{
                    kernel_sequential_warp<uint64_t><<<blockDim, blocksize>>>(a_d,n_elems, n_pages_per_warp, output_d, n_warps, pc_page_size);
                    break;
                }
                case SEQUENTIAL_WARP_PC:{
                    //printf("blockDim.x is %llu \t blocksize: %llu\n", blockDim.x, blocksize );
                    kernel_sequential_warp_pc<uint64_t><<<blockDim, blocksize>>>(h_Aarray->d_array_ptr, a_d,n_elems, n_pages_per_warp, output_d, n_warps, pc_page_size, settings.stride);
                    break;
                }

                case RANDOM:{
                    //printf("blockDim.x is %llu \t blocksize: %llu\n", blockDim.x, blocksize );
                    kernel_random<uint64_t><<<blockDim, blocksize>>>(a_d, assignment_d, numthreads, output_d);
                    break;
                }
                case RANDOM_PC:{
                    //printf("blockDim.x is %llu \t blocksize: %llu\n", blockDim.x, blocksize );
                    kernel_random_pc<uint64_t><<<blockDim, blocksize>>>(h_Aarray->d_array_ptr, a_d, assignment_d, numthreads, output_d);
                    break;
                }
                case RANDOM_WARP:{
                    kernel_random_warp<uint64_t><<<blockDim, blocksize>>>(a_d,n_elems, n_pages_per_warp, output_d, assignment_d, n_warps, pc_page_size, settings.stride);
                    break;
                }
                case RANDOM_WARP_PC:{
                    //printf("blockDim.x is %llu \t blocksize: %llu\n", blockDim.x, blocksize );
                    kernel_random_warp_pc<uint64_t><<<blockDim, blocksize>>>(h_Aarray->d_array_ptr, a_d,n_elems,  n_pages_per_warp, output_d, assignment_d, n_warps, pc_page_size, settings.stride);
                    break;
                }
                case POWERLAW_WARP:{
                    /* POWERLAW_WARP: kernel_random_warp를 재사용. assignment만 Zipfian 분포 */
                    kernel_random_warp<uint64_t><<<blockDim, blocksize>>>(a_d, n_elems, n_pages_per_warp, output_d, assignment_d, n_warps, pc_page_size, settings.stride);
                    break;
                }
                case POWERLAW_WARP_PC:{
                    /* POWERLAW_WARP_PC: kernel_random_warp_pc를 재사용. assignment만 Zipfian 분포 */
                    kernel_random_warp_pc<uint64_t><<<blockDim, blocksize>>>(h_Aarray->d_array_ptr, a_d, n_elems, n_pages_per_warp, output_d, assignment_d, n_warps, pc_page_size, settings.stride);
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

            /* 웜업(titr==0)을 제외한 실행 시간 누적 및 평균 계산 */
            if(titr>0){
                totaltime +=milliseconds;
                avgtime = totaltime/(titr);
            }

            cuda_err_chk(cudaMemcpy(output_h, (output_d), (1)*sizeof(unsigned long long int), cudaMemcpyDeviceToHost));
            //printf("\n******\n");
            //fflush(stdout);
            //if(mem != BAFS_DIRECT){
            //   printf("Input list::");
            //   for (uint64_t i=n_elems-100; i< n_elems; i++)
            //       printf("%llu\t", a_h[i]);
            //}

            //if(mem != BAFS_DIRECT){
            //    uint64_t total = 0;
            //    for(uint64_t count=0; count<n_elems; count++)
            //        total+=a_h[count];
            //    printf("total in cpu: %llu \n", total);
            //}
            printf("val in gpu: %llu \n", output_h[0]);
            auto itrend = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(itrend - itrstart);

            /* 성능 메트릭 계산: IOPS와 대역폭 */
            uint64_t ios =numthreads;                        // I/O 연산 수 (스레드 수 = 원소 접근 수)
			uint64_t data = ios*sizeof(uint64_t);            // 전송 데이터량 (바이트)
			double iops = ((double) ios*1000/ (milliseconds));   // IOPS = ios / (time in seconds)
			double bandwidth = (((double) data*1000/(milliseconds))/(1024ULL*1024ULL*1024ULL));  // 대역폭 (GBps)

			double avgiops = ((double) ios*1000/ (avgtime));     // 평균 IOPS
			double avgbandwidth = (((double) data*1000/(avgtime))/(1024ULL*1024ULL*1024ULL));    // 평균 대역폭

            /* WARP 모드에서는 워프당 처리한 전체 원소 수로 보정 */
            if((type == SEQUENTIAL_WARP) || (type == SEQUENTIAL_WARP_PC) || (type == RANDOM_WARP) || (type == RANDOM_WARP_PC) || (type == POWERLAW_WARP) || (type == POWERLAW_WARP_PC)){

				//ios = n_warps*n_pages_per_warp*n_elems_per_page;
				ios = n_warps*n_pages_per_warp*n_elems_per_page;   // 실제 접근한 총 원소 수
                iops = ((double) ios*1000/ (milliseconds));
				data = ios*sizeof(uint64_t);
				bandwidth = (((double) data*1000/(milliseconds))/(1024ULL*1024ULL*1024ULL));
			    avgiops = ((double) ios*1000/ (avgtime));
			    avgbandwidth = (((double) data*1000/(avgtime))/(1024ULL*1024ULL*1024ULL));
            }

            /* BAFS 모드: page cache 통계 출력 (hit/miss 수 등) 및 디바이스 동기화 */
			if(mem == BAFS_DIRECT) {
                 h_Aarray->print_reset_stats();
                 cuda_err_chk(cudaDeviceSynchronize());
            }
            /* 성능 결과 출력: 반복 번호, 구현 타입, SSD 수, 워프 수, 페이지/워프, 원소/페이지, IO 수, IOPS, 데이터량, 대역폭 */
			printf("P:%d Impl: %llu \t SSD: %llu \t n_warps:%llu \t n_pages_per_warp: %llu \t n_elems_per_page:%llu \t ios: %llu \t IOPs: %f \t data:%llu \t bandwidth: %f GBps \t avgiops: %f \t avgbandwidth: %f \n",titr, type, settings.n_ctrls, n_warps, n_pages_per_warp, n_elems_per_page, ios, iops, data, bandwidth, avgiops, avgbandwidth );
            //printf("\nVA %d A:%s Impl: %d \t SSD: %d \t CL: %d \t Cache: %llu \t TotalTime %f ms\n", titr, a_file_bin.c_str(), type, settings.n_ctrls, settings.pageSize,settings.maxPageCacheSize, milliseconds);
            fflush(stdout);
        }

        /* 자원 해제 */
        if(mem!=BAFS_DIRECT){
           free(a_h);
         }
        free(output_h);

        if((type == SEQUENTIAL_PC) || (type == SEQUENTIAL_WARP_PC) || (type == RANDOM_PC) || (type == RANDOM_WARP_PC) || (type == STRIDE_PC) || (type == STRIDE_WARP_PC) || (type == POWERLAW_WARP_PC)) {
            delete h_pc;
            delete h_Arange;
            delete h_Aarray;
        }

        if(mem!=BAFS_DIRECT){
            if(mem==UVM_DIRECT){
              a_d = a_d-2;    // UVM_DIRECT에서 +2 했던 것을 원복 후 free
            }
            cuda_err_chk(cudaFree(a_d));
            cuda_err_chk(cudaFree(output_d));
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
