/*
 * BaM I/O Depth Block Benchmark (main.cu)
 * ========================================
 * GPU에서 NVMe SSD로 직접 블록 I/O를 수행하되, **비동기 I/O depth**를 조절하여
 * 파이프라인 효과를 측정하는 벤치마크이다.
 *
 * block 벤치마크와의 핵심 차이점:
 *   - block: 각 스레드가 동기적으로 1회씩 I/O (read_data/write_data)
 *   - iodepth-block: 각 스레드가 n개의 비동기 I/O를 먼저 발행(access_data_async)한 후
 *     일괄 폴링(poll_async)하여 I/O 파이프라인을 형성한다.
 *
 * 이 방식은 NVMe SSD의 내부 병렬성을 최대한 활용한다:
 *   - n=1: 동기 I/O와 동일 (발행→완료→발행→완료)
 *   - n=2~4: 여러 커맨드를 SQ에 먼저 쌓아놓고 한꺼번에 폴링
 *     → SSD 내부의 여러 채널/다이에서 병렬 처리 가능
 *
 * 커널은 템플릿 파라미터 n으로 I/O depth를 컴파일 타임에 결정하며,
 * #pragma unroll을 통해 루프를 완전 언롤링하여 레지스터 기반 배열로 최적화한다.
 *
 * 전체 흐름:
 *   1) 커맨드라인 인자 파싱 (Settings)
 *   2) NVMe 컨트롤러 초기화 및 GPU 페이지 캐시 생성
 *   3) numReqs(=n) 값에 따라 템플릿 커널 인스턴스를 선택하여 실행
 *   4) IOPS 및 대역폭 측정/출력
 */

#include <cuda.h>
/* libnvm 헤더: GPU에서 NVMe를 직접 제어하기 위한 사용자 공간 드라이버 */
#include <nvm_ctrl.h>        /* NVMe 컨트롤러 제어 구조체 */
#include <nvm_types.h>       /* NVMe 명령어/큐 관련 타입 정의 */
#include <nvm_queue.h>       /* NVMe Submission/Completion Queue 조작 */
#include <nvm_util.h>        /* NVMe 유틸리티 매크로 */
#include <nvm_admin.h>       /* NVMe Admin 커맨드 */
#include <nvm_error.h>       /* NVMe 에러 처리 */
#include <nvm_cmd.h>         /* NVMe I/O 커맨드 빌더 */
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>        /* mmap: 호스트 파일 메모리 매핑 */
#include <sys/stat.h>
/* BaM 프레임워크 헤더 */
#include <ctrl.h>            /* Controller: NVMe 컨트롤러 추상화 */
#include <buffer.h>          /* DMA 버퍼 관리 */
#include "settings.h"        /* Settings: 벤치마크 파라미터 */
#include <event.h>           /* Event: CUDA 이벤트 기반 타이밍 (마이크로초) */
#include <queue.h>           /* QueuePair: NVMe SQ/CQ 쌍 */
#include <nvm_parallel_queue.h> /* 병렬 큐 접근용 락프리 구조 */
#include <nvm_io.h>          /* NVMe I/O 헬퍼 */
#include <page_cache.h>      /* page_cache_t/d_t: GPU 페이지 캐시 + access_data_async/poll_async */
#include <util.h>            /* warp_memcpy, hexdump, lane_id, get_smid 등 */
#include <iostream>
#include <fstream>
#include <byteswap.h>
#ifdef __DIS_CLUSTER__
#include <sisci_api.h>       /* Dolphin SmartIO 분산 클러스터용 (선택적) */
#endif

using error = std::runtime_error;
using std::string;



/* NVMe 컨트롤러 디바이스 경로 배열 (최대 32개).
 * libnvm 커널 모듈이 /dev/libnvm0~31 디바이스를 생성하며,
 * 각 디바이스는 하나의 물리 NVMe SSD에 대응한다. */
//uint32_t n_ctrls = 1;
const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9", "/dev/libnvm10", "/dev/libnvm11", "/dev/libnvm12", "/dev/libnvm13", "/dev/libnvm14", "/dev/libnvm15", "/dev/libnvm16", "/dev/libnvm17", "/dev/libnvm18", "/dev/libnvm19", "/dev/libnvm20", "/dev/libnvm21", "/dev/libnvm22", "/dev/libnvm23", "/dev/libnvm24","/dev/libnvm25", "/dev/libnvm26", "/dev/libnvm27", "/dev/libnvm28", "/dev/libnvm29", "/dev/libnvm30", "/dev/libnvm31"};

/* hexdump용 바이트 수: 8페이지 = 32KB */
#define SIZE (8*4096)

/*
 * print_cache_kernel: 디버그용 커널. 페이지 캐시 시작 주소부터 SIZE 바이트를 hexdump 출력.
 * 단일 스레드(tid==0)만 실행한다.
 */
__global__
void print_cache_kernel(page_cache_d_t* pc) {
    uint64_t tid = threadIdx.x + blockIdx.x * blockDim.x;

    if (tid == 0) {
        hexdump(pc->base_addr, SIZE);
    }
}

/*
 * new_kernel: warp 단위 메모리 복사 테스트 커널 (디버그/성능 테스트용).
 */
__global__
void new_kernel(ulonglong4* dst, ulonglong4* src, size_t num) {
    warp_memcpy<ulonglong4>(dst, src, num);

}
/*
__device__ void read_data(page_cache_t* pc, QueuePair* qp, const uint64_t starting_lba, const uint64_t n_blocks, const unsigned long long pc_entry) {
    //uint64_t starting_lba = starting_byte >> qp->block_size_log;
    //uint64_t rem_bytes = starting_byte & qp->block_size_minus_1;
    //uint64_t end_lba = CEIL((starting_byte+num_bytes), qp->block_size);

    //uint16_t n_blocks = CEIL(num_bytes, qp->block_size, qp->block_size_log);



    nvm_cmd_t cmd;
    uint16_t cid = get_cid(&(qp->sq));
    //printf("cid: %u\n", (unsigned int) cid);


    nvm_cmd_header(&cmd, cid, NVM_IO_READ, qp->nvmNamespace);
    uint64_t prp1 = pc->prp1[pc_entry];
    uint64_t prp2 = 0;
    if (pc->prps)
        prp2 = pc->prp2[pc_entry];
    //printf("tid: %llu\tstart_lba: %llu\tn_blocks: %llu\tprp1: %p\n", (unsigned long long) threadIdx.x, (unsigned long long) starting_lba, (unsigned long long) n_blocks, (void*) prp1);
    nvm_cmd_data_ptr(&cmd, prp1, prp2);
    nvm_cmd_rw_blks(&cmd, starting_lba, n_blocks);
    uint16_t sq_pos = sq_enqueue(&qp->sq, &cmd);

    uint32_t cq_pos = cq_poll(&qp->cq, cid);
    sq_dequeue(&qp->sq, sq_pos);
    cq_dequeue(&qp->cq, cq_pos);


    put_cid(&qp->sq, cid);


}

*/

/*
 * random_access_kernel<n>: I/O depth를 적용한 랜덤 블록 I/O 벤치마크 커널
 * =========================================================================
 * 템플릿 파라미터 n은 각 스레드가 동시에 발행하는 NVMe 커맨드 수(I/O depth)이다.
 *
 * 동작 방식 (2-phase I/O pipeline):
 *   Phase 1 (발행): n개의 NVMe READ 커맨드를 access_data_async()로 SQ에 인큐한다.
 *     - 각 커맨드의 CID(Command ID)와 SQ 위치를 cids[], sq_poss[] 배열에 저장
 *     - access_data_async는 SQ에 인큐만 하고 완료를 기다리지 않는다 (비동기)
 *   Phase 2 (폴링): n개의 완료를 poll_async()로 순차 확인한다.
 *     - CQ를 폴링하여 해당 CID의 완료를 확인하고, SQ/CQ를 정리한다
 *
 *   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
 *   │ async_submit │→ │ async_submit │→ │ async_submit │ ... │  poll_wait  │→ │  poll_wait  │→ │  poll_wait  │
 *   │   cmd[0]     │  │   cmd[1]     │  │   cmd[n-1]   │     │   cmd[0]    │  │   cmd[1]    │  │   cmd[n-1]  │
 *   └─────────────┘  └─────────────┘  └─────────────┘     └─────────────┘  └─────────────┘  └─────────────┘
 *
 * #pragma unroll: n은 컴파일 타임 상수이므로 루프가 완전히 언롤링되어
 *   cids[]와 sq_poss[]가 레지스터에 배치된다 → 메모리 접근 오버헤드 제거.
 *
 * 컨트롤러/큐 선택 전략:
 *   - 컨트롤러: SM ID 기반 (smid % n_ctrls) → 같은 SM의 warp들이 같은 컨트롤러 사용
 *   - 큐: atomic counter 라운드로빈
 *   - 결과를 __shfl_sync로 warp 전체(32스레드)에 브로드캐스트
 *
 * @tparam n                 I/O depth (동시 발행 커맨드 수, 1~4)
 * @param ctrls              NVMe 컨트롤러 배열 (디바이스 포인터의 포인터)
 * @param pc                 페이지 캐시 디스크립터 (디바이스)
 * @param req_size           요청 1건의 크기 (바이트, = page_size)
 * @param n_reqs             총 요청 수 (= 총 스레드 수)
 * @param req_count          완료 카운터 (현재 미사용)
 * @param num_ctrls          NVMe 컨트롤러 수
 * @param assignment         랜덤 블록 인덱스 배열 (호스트에서 rand()로 생성)
 * @param reqs_per_thread    스레드당 요청 수 (= n, 템플릿과 동일 값)
 * @param access_type        접근 유형 (현재 READ 고정)
 * @param access_type_assignment  MIXED 모드용 (현재 미사용)
 */
template<size_t n>
__global__ __launch_bounds__(64,32)
void random_access_kernel(Controller** ctrls, page_cache_d_t* pc,  uint32_t req_size, uint32_t n_reqs, unsigned long long* req_count, uint32_t num_ctrls, uint64_t* assignment, uint64_t reqs_per_thread, uint32_t access_type, uint8_t* access_type_assignment) {
    //printf("in threads\n");
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t laneid = lane_id();     /* warp 내 레인 번호 (0~31) */
    uint32_t bid = blockIdx.x;
    uint32_t smid = get_smid();      /* 현재 SM ID */

    /* 컨트롤러/큐 선택: lane 0만 연산하고 warp 전체에 브로드캐스트 */
    uint32_t ctrl;
    uint32_t queue;
    if (laneid == 0) {
        /* SM ID 기반으로 컨트롤러를 선택 → 같은 SM의 warp들이 같은 컨트롤러에 집중 */
	ctrl = smid % (pc->n_ctrls);
        //ctrl = pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
        /* 큐는 atomic counter로 라운드로빈 배정 */
        queue = ctrls[ctrl]->queue_counter.fetch_add(1, simt::memory_order_relaxed) %  (ctrls[ctrl]->n_qps);
        //queue = smid % (ctrls[ctrl]->n_qps);
    }
    ctrl =  __shfl_sync(0xFFFFFFFF, ctrl, 0);   /* lane 0 → warp 전체 전파 */
    queue =  __shfl_sync(0xFFFFFFFF, queue, 0);


    if (tid < n_reqs) {
        /* 랜덤 접근: assignment[tid]에 저장된 랜덤 블록 인덱스로 시작 LBA 계산 */
        uint64_t start_block = (assignment[tid]*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //uint64_t start_block = (tid*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //start_block = tid;
        uint64_t n_blocks = req_size >> ctrls[ctrl]->d_qps[queue].block_size_log; /// ctrls[ctrl].ns.lba_data_size;;
        //printf("tid: %llu\tstart_block: %llu\tn_blocks: %llu\n", (unsigned long long) tid, (unsigned long long) start_block, (unsigned long long) n_blocks);

        uint8_t opcode;
        uint16_t cids[n];       /* n개 커맨드의 Command ID를 저장하는 배열 (레지스터) */
        uint16_t sq_poss[n];    /* n개 커맨드의 SQ 위치를 저장하는 배열 (레지스터) */

        /* Phase 1: n개의 NVMe READ 커맨드를 비동기로 발행 (SQ에 인큐만, 완료 대기 없음) */
        #pragma unroll
        for (size_t i = 0; i < n; i++) {
            access_data_async(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid, NVM_IO_READ, cids+i, sq_poss+i);

        }
        /* Phase 2: n개 커맨드의 완료를 순차적으로 폴링하여 회수 */
        #pragma unroll
        for (size_t i = 0; i < n; i++) {
            poll_async((ctrls[ctrl]->d_qps)+(queue), cids[i], sq_poss[i]);

        }
        //read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
        //read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
        //read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
        //__syncthreads();
        //read_data(pc, (ctrls[ctrl].d_qps)+(queue),start_block*2, n_blocks, tid);
        //printf("tid: %llu finished\n", (unsigned long long) tid);

    }

}

/*
 * sequential_access_kernel<n>: I/O depth를 적용한 순차 블록 I/O 벤치마크 커널
 * ===========================================================================
 * random_access_kernel<n>과 동일한 2-phase 비동기 I/O 파이프라인 구조이지만,
 * 랜덤 assignment 배열 대신 tid 기반의 연속 주소로 I/O를 수행한다.
 *
 * start_block = (tid * req_size) / block_size  (순차적 LBA 주소)
 *
 * 순차 접근에서도 I/O depth를 높이면 SSD 내부 프리페치/읽기-ahead가 활성화되어
 * 추가 성능 향상을 기대할 수 있다.
 *
 * @tparam n  I/O depth (동시 발행 커맨드 수, 1~4)
 * (나머지 파라미터는 random_access_kernel과 동일)
 */
template<size_t n>
__global__ __launch_bounds__(64,32)
void sequential_access_kernel(Controller** ctrls, page_cache_d_t* pc,  uint32_t req_size, uint32_t n_reqs, unsigned long long* req_count, uint32_t num_ctrls, uint64_t* assignment, uint64_t reqs_per_thread, uint32_t access_type, uint8_t* access_type_assignment) {
    //printf("in threads\n");
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t laneid = lane_id();
    uint32_t bid = blockIdx.x;
    uint32_t smid = get_smid();

    uint32_t ctrl;
    uint32_t queue;
    if (laneid == 0) {
        /* SM 기반 컨트롤러 선택 + atomic 라운드로빈 큐 선택 */
	ctrl = smid % (pc->n_ctrls);
        //ctrl = pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
        queue = ctrls[ctrl]->queue_counter.fetch_add(1, simt::memory_order_relaxed) %  (ctrls[ctrl]->n_qps);
        //queue = smid % (ctrls[ctrl]->n_qps);
    }
    ctrl =  __shfl_sync(0xFFFFFFFF, ctrl, 0);
    queue =  __shfl_sync(0xFFFFFFFF, queue, 0);


    if (tid < n_reqs) {
        /* 순차 접근: tid 기반의 연속적인 시작 블록 주소 */
        uint64_t start_block = (tid*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //uint64_t start_block = (tid*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //start_block = tid;
        uint64_t n_blocks = req_size >> ctrls[ctrl]->d_qps[queue].block_size_log; /// ctrls[ctrl].ns.lba_data_size;;
        //printf("tid: %llu\tstart_block: %llu\tn_blocks: %llu\n", (unsigned long long) tid, (unsigned long long) start_block, (unsigned long long) n_blocks);

        uint8_t opcode;
        uint16_t cids[n];       /* n개 커맨드의 CID 배열 */
        uint16_t sq_poss[n];    /* n개 커맨드의 SQ 위치 배열 */

        /* Phase 1: n개 비동기 READ 커맨드 발행 */
        #pragma unroll
        for (size_t i = 0; i < n; i++) {
            access_data_async(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid, NVM_IO_READ, cids+i, sq_poss+i);

        }
        /* Phase 2: n개 완료 폴링 */
        #pragma unroll
        for (size_t i = 0; i < n; i++) {
            poll_async((ctrls[ctrl]->d_qps)+(queue), cids[i], sq_poss[i]);

        }
        //read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
        //read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
        //read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
        //__syncthreads();
        //read_data(pc, (ctrls[ctrl].d_qps)+(queue),start_block*2, n_blocks, tid);
        //printf("tid: %llu finished\n", (unsigned long long) tid);

    }

}




/*
 * main: I/O Depth 벤치마크 진입점
 * ================================
 * block 벤치마크의 main과 유사하지만, 커널 실행 시
 * numReqs 값(1~4)에 따라 해당 템플릿 인스턴스를 switch로 선택한다.
 *
 * 예: --reqs=4 → random_access_kernel<4> 또는 sequential_access_kernel<4> 실행
 *     → 각 스레드가 4개의 비동기 NVMe 커맨드를 파이프라인으로 처리
 *
 * 성능 지표는 총 I/O = 스레드수 × numReqs로 계산되므로,
 * I/O depth가 높을수록 같은 스레드 수에서 더 많은 I/O가 발생한다.
 */
int main(int argc, char** argv) {

    /* ---- (1) 커맨드라인 인자 파싱 ---- */
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


    /* ---- CUDA 디바이스 속성 확인 ---- */
    cudaDeviceProp properties;
    if (cudaGetDeviceProperties(&properties, settings.cudaDevice) != cudaSuccess)
    {
        fprintf(stderr, "Failed to get CUDA device properties\n");
        return 1;
    }

    try {

        /* ---- (선택) 입력 파일 mmap: SSD에 쓸 데이터를 호스트 파일에서 가져옴 ---- */
        const char* input_f;

        input_f = settings.input;

        void* map_in;
        int fd_in;
        struct stat sb_in;

        if (input_f != nullptr) {
            if((fd_in = open(input_f, O_RDWR)) == -1){
                fprintf(stderr, "Input file cannot be opened\n");
                return 1;
            }

            fstat(fd_in, &sb_in);

            map_in = mmap(NULL, sb_in.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_in, 0);

            if((map_in == (void*)-1)){
                fprintf(stderr,"Input file map failed %d\n",map_in);
                return 1;
            }

            close(fd_in);
        }

        /* ---- (2) NVMe 컨트롤러 초기화 ----
         * n_ctrls 개의 NVMe 컨트롤러를 생성한다.
         * (block과 달리 SSD 타입 구분 없이 ctrls_paths 단일 배열 사용) */
        cuda_err_chk(cudaSetDevice(settings.cudaDevice));
        std::vector<Controller*> ctrls(settings.n_ctrls);
        for (size_t i = 0 ; i < settings.n_ctrls; i++)
            ctrls[i] = new Controller(ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);

        //auto dma = createDma(ctrl.ctrl, NVM_PAGE_ALIGN(64*1024*10, 1UL << 16), settings.cudaDevice, settings.adapter, settings.segmentId);

        //std::cout << dma.get()->vaddr << std::endl;
        //QueuePair h_qp(ctrl, settings, 1);
        //std::cout << "in main: " << std::hex << h_qp.sq.cid << "raw: " << h_qp.sq.cid<< std::endl;
        //std::memset(&h_qp, 0, sizeof(QueuePair));
        //prepareQueuePair(h_qp, ctrl, settings, 1);
        //const uint32_t ps, const uint64_t np, const uint64_t c_ps, const Settings& settings, const Controller& ctrl)
        //
        /*
        Controller** d_ctrls;
        cuda_err_chk(cudaMalloc(&d_ctrls, n_ctrls*sizeof(Controller*)));
        for (size_t i = 0; i < n_ctrls; i++)
            cuda_err_chk(cudaMemcpy(d_ctrls+i, &(ctrls[i]->d_ctrl), sizeof(Controller*), cudaMemcpyHostToDevice));
        */

        /* ---- CUDA 그리드/블록 차원 계산 ---- */
        uint64_t b_size = settings.blkSize;                          /* CUDA 블록당 스레드 수 (기본 64) */
        uint64_t g_size = (settings.numThreads + b_size - 1)/b_size; /* 필요한 CUDA 블록 수 */
        uint64_t n_threads = b_size * g_size;                        /* 실제 총 스레드 수 */


        /* ---- (3) 페이지 캐시 생성 ---- */
        uint64_t page_size = settings.pageSize;
        uint64_t n_pages = settings.numPages;
        uint64_t total_cache_size = (page_size * n_pages);
        //uint64_t n_pages = total_cache_size/page_size;
        //
        if (n_pages < n_threads) {
            std::cerr << "Please provide enough pages. Number of pages must be greater than or equal to the number of threads!\n";
            exit(1);
        }


        page_cache_t h_pc(page_size, n_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
        std::cout << "finished creating cache\n";

        //QueuePair* d_qp;
        page_cache_d_t* d_pc = (page_cache_d_t*) (h_pc.d_pc_ptr);
        #define TYPE uint64_t
        uint64_t n_blocks = settings.numBlks;
        //uint64_t t_size = n_blocks * sizeof(TYPE);

        // range_t<uint64_t> h_range((uint64_t)0, (uint64_t)n_elems, (uint64_t)0, (uint64_t)(t_size/page_size), (uint64_t)0, (uint64_t)page_size, &h_pc, settings.cudaDevice);
        // range_t<uint64_t>* d_range = (range_t<uint64_t>*) h_range.d_range_ptr;

        // std::vector<range_t<uint64_t>*> vr(1);
        // vr[0] = & h_range;
        // //(const uint64_t num_elems, const uint64_t disk_start_offset, const std::vector<range_t<T>*>& ranges, Settings& settings)
        // array_t<uint64_t> a(n_elems, 0, vr, settings.cudaDevice);


        //std::cout << "finished creating range\n";




        unsigned long long* d_req_count;
        cuda_err_chk(cudaMalloc(&d_req_count, sizeof(unsigned long long)));
        cuda_err_chk(cudaMemset(d_req_count, 0, sizeof(unsigned long long)));

        /* GPU PCIe Bus ID 출력 (디버그용) */
        char st[15];
        cuda_err_chk(cudaDeviceGetPCIBusId(st, 15, settings.cudaDevice));
        std::cout << st << std::endl;

        /* ---- 랜덤 모드: 랜덤 블록 인덱스 배열 생성 ---- */
        uint64_t* assignment;
        uint64_t* d_assignment;
        if (settings.random) {
            assignment = (uint64_t*) malloc(n_threads*sizeof(uint64_t));
            for (size_t i = 0; i< n_threads; i++)
                assignment[i] = rand() % (n_blocks);


            cuda_err_chk(cudaMalloc(&d_assignment, n_threads*sizeof(uint64_t)));
            cuda_err_chk(cudaMemcpy(d_assignment, assignment,  n_threads*sizeof(uint64_t), cudaMemcpyHostToDevice));
        }

        /* 타이밍 시작 */
        Event before;

        /* ---- MIXED 모드: 스레드별 읽기/쓰기 비율 배정 ---- */
        uint8_t* access_assignment;
        uint8_t* d_access_assignment = NULL;
        if (settings.accessType == 2) {
            access_assignment = (uint8_t*) malloc(n_threads*sizeof(uint8_t));
            for (size_t i = 0; i < n_threads; i++)
                access_assignment[i] = (((rand() % 100) + 1) <= settings.ratio) ? NVM_IO_READ : NVM_IO_WRITE;

            cuda_err_chk(cudaMalloc(&d_access_assignment, n_threads*sizeof(uint8_t)));
            cuda_err_chk(cudaMemcpy(d_access_assignment, access_assignment, n_threads*sizeof(uint8_t), cudaMemcpyHostToDevice));
        }

        /* ---- (4) 커널 실행: numReqs에 따라 템플릿 인스턴스 선택 ----
         * 템플릿 파라미터는 컴파일 타임 상수여야 하므로 switch문으로 분기한다.
         * n=1: 동기와 동일, n=2~4: I/O 파이프라인 depth 증가 */
        std::cout << "atlaunch kernel\n";
        if (settings.random) {
            switch (settings.numReqs) {
            case 1:
                random_access_kernel<1><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            case 2:
                random_access_kernel<2><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            case 3:
                random_access_kernel<3><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            case 4:
                random_access_kernel<4><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            default:
                std::cout << "Invalid num reqs\n";
                break;
            }
        }
        else {
            switch (settings.numReqs) {
            case 1:
                sequential_access_kernel<1><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            case 2:
                sequential_access_kernel<2><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            case 3:
                sequential_access_kernel<3><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            case 4:
                sequential_access_kernel<4><<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
                break;
            default:
                std::cout << "Invalid num reqs\n";
                break;
            }
        }

        /* 타이밍 종료 */
        Event after;

        //print_cache_kernel<<<1,1>>>(d_pc);
        //new_kernel<<<1,1>>>();
        //uint8_t* ret_array = (uint8_t*) malloc(n_pages*page_size);

        //cuda_err_chk(cudaMemcpy(ret_array, h_pc.base_addr,page_size*n_pages, cudaMemcpyDeviceToHost));
        cuda_err_chk(cudaDeviceSynchronize());

        /* 입력 파일이 지정된 경우 캐시 내용을 호스트로 복사 */
        if (input_f != nullptr) {
            cuda_err_chk(cudaMemcpy(map_in, h_pc.pdt.base_addr,  std::min((uint64_t)sb_in.st_size, total_cache_size), cudaMemcpyDeviceToHost));
            munmap(map_in, sb_in.st_size);
        }

        /* ---- (5) 성능 결과 계산 ----
         * ios = 스레드수 × I/O depth(numReqs): I/O depth가 높을수록 총 I/O 증가
         * IOPS와 대역폭을 I/O depth별로 비교하면 파이프라인 효과를 측정할 수 있다 */
        double elapsed = after - before;
        uint64_t ios = g_size*b_size*settings.numReqs;
        uint64_t data = ios*page_size;
        double iops = ((double)ios)/(elapsed/1000000);
        double bandwidth = (((double)data)/(elapsed/1000000))/(1024ULL*1024ULL*1024ULL);
        std::cout << std::dec << "Elapsed Time: " << elapsed << "\tNumber of Ops: "<< ios << "\tData Size (bytes): " << data << std::endl;
        std::cout << std::dec << "Ops/sec: " << iops << "\tEffective Bandwidth(GB/S): " << bandwidth << std::endl;
        //std::cout << std::dec << ctrls[0]->ns.lba_data_size << std::endl;

        //std::ofstream ofile("../data", std::ios::binary | std::ios::trunc);
        //ofile.write((char*)ret_array, data);
        //ofile.close();

        /* 리소스 정리 */
        for (size_t i = 0 ; i < settings.n_ctrls; i++)
            delete ctrls[i];
        //hexdump(ret_array, n_pages*page_size);
/*
        cudaFree(d_qp);
        cudaFree(d_pc);
        cudaFree(d_req_count);
        free(ret_array);
*/

        //std::cout << "END\n";

        //std::cout << RAND_MAX << std::endl;

    }
    catch (const error& e) {
        fprintf(stderr, "Unexpected error: %s\n", e.what());
        return 1;
    }



}
