/*
 * BaM Block-level I/O Benchmark (main.cu)
 * ========================================
 * GPU에서 NVMe SSD로 직접 블록 I/O를 수행하는 벤치마크.
 * BaM의 page_cache를 통해 GPU 스레드가 NVMe 커맨드를 직접 발행하여
 * 순차(sequential) 또는 랜덤(random) 읽기/쓰기/혼합 I/O 성능을 측정한다.
 *
 * 전체 흐름:
 *   1) 커맨드라인 인자를 파싱하여 Settings 구조체에 저장
 *   2) NVMe 컨트롤러를 초기화하고 GPU 메모리에 페이지 캐시를 생성
 *   3) 순차 또는 랜덤 접근 커널을 실행 (각 GPU 스레드가 NVMe I/O를 직접 수행)
 *   4) 경과 시간을 측정하여 IOPS와 대역폭을 출력
 */

#include <cuda.h>
/* libnvm 헤더: GPU에서 NVMe를 직접 제어하기 위한 사용자 공간 드라이버 */
#include <nvm_ctrl.h>        /* NVMe 컨트롤러 제어 구조체 */
#include <nvm_types.h>       /* NVMe 명령어/큐 관련 타입 정의 */
#include <nvm_queue.h>       /* NVMe Submission/Completion Queue 조작 */
#include <nvm_util.h>        /* NVMe 유틸리티 매크로 (정렬, 크기 계산 등) */
#include <nvm_admin.h>       /* NVMe Admin 커맨드 (Identify, Set Features 등) */
#include <nvm_error.h>       /* NVMe 에러 코드 및 상태 처리 */
#include <nvm_cmd.h>         /* NVMe I/O 커맨드 빌더 (Read/Write 커맨드 구성) */
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>        /* mmap: 호스트 파일을 메모리 매핑하여 결과를 기록할 때 사용 */
#include <sys/stat.h>
/* BaM 프레임워크 헤더 */
#include <ctrl.h>            /* Controller 클래스: NVMe 컨트롤러 추상화 (큐쌍 관리 포함) */
#include <buffer.h>          /* DMA 버퍼 관리: GPU-accessible 메모리 할당 */
#include "settings.h"        /* Settings 구조체: 벤치마크 파라미터 파싱 */
#include <event.h>           /* Event 클래스: CUDA 이벤트 기반 타이밍 측정 (마이크로초) */
#include <queue.h>           /* QueuePair: NVMe SQ/CQ 쌍을 GPU 메모리에 매핑 */
#include <nvm_parallel_queue.h> /* 병렬 큐 접근을 위한 락프리 큐 구조 */
#include <nvm_io.h>          /* NVMe I/O 헬퍼 함수 (커맨드 인큐/디큐) */
#include <page_cache.h>      /* page_cache_t/page_cache_d_t: GPU 메모리 페이지 캐시 관리 */
#include <util.h>            /* GPU 유틸리티: warp_memcpy, hexdump, lane_id, get_smid 등 */
#include <iostream>
#include <fstream>
#include <byteswap.h>
#ifdef __DIS_CLUSTER__
#include <sisci_api.h>       /* Dolphin SmartIO 분산 클러스터용 API (선택적) */
#endif

using error = std::runtime_error;
using std::string;



/* NVMe 컨트롤러 디바이스 경로 배열.
 * libnvm 커널 모듈이 /dev/libnvm0 ~ /dev/libnvm9 디바이스를 생성하며,
 * 각 디바이스는 하나의 물리 NVMe SSD에 대응한다.
 * Samsung SSD와 Intel SSD 경로가 분리되어 있으나 현재 동일한 경로를 사용한다. */
//uint32_t n_ctrls = 1;
const char* const sam_ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
const char* const intel_ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9"};
//const char* const ctrls_paths[] = {"/dev/libnvm0", "/dev/libnvm1", "/dev/libnvm2", "/dev/libnvm3", "/dev/libnvm4", "/dev/libnvm5", "/dev/libnvm6", "/dev/libnvm7", "/dev/libnvm8", "/dev/libnvm9", "/dev/libnvm10", "/dev/libnvm11", "/dev/libnvm12", "/dev/libnvm13", "/dev/libnvm14", "/dev/libnvm15", "/dev/libnvm16", "/dev/libnvm17", "/dev/libnvm18", "/dev/libnvm19", "/dev/libnvm20", "/dev/libnvm21", "/dev/libnvm22", "/dev/libnvm23", "/dev/libnvm24","/dev/libnvm25", "/dev/libnvm26", "/dev/libnvm27", "/dev/libnvm28", "/dev/libnvm29", "/dev/libnvm30", "/dev/libnvm31"};


/* hexdump용 바이트 수: 8페이지 = 32KB */
#define SIZE (8*4096)

/*
 * print_cache_kernel: 디버그용 커널. 페이지 캐시의 시작 주소부터 SIZE 바이트를 hexdump로 출력한다.
 * 단일 스레드(tid==0)만 실행하며, 캐시에 적재된 데이터를 직접 확인할 때 사용한다.
 *
 * @param pc  GPU 메모리에 있는 페이지 캐시 디스크립터 (디바이스 포인터)
 */
__global__
void print_cache_kernel(page_cache_d_t* pc) {
    uint64_t tid = threadIdx.x + blockIdx.x * blockDim.x;

    if (tid == 0) {
        hexdump(pc->base_addr, SIZE);
    }
}

/*
 * new_kernel: warp 단위 메모리 복사 테스트 커널.
 * warp_memcpy를 사용하여 src에서 dst로 ulonglong4(32바이트) 단위로 num개 복사한다.
 * 성능 테스트 또는 디버그 목적의 유틸리티 커널이다.
 *
 * @param dst  복사 대상 (GPU 메모리)
 * @param src  복사 원본 (GPU 메모리)
 * @param num  복사할 ulonglong4 원소 개수
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
 * sequential_access_kernel: 순차 블록 I/O 벤치마크 커널
 * ======================================================
 * 각 GPU 스레드가 자신의 tid에 비례하는 연속적인 LBA 주소로 NVMe I/O를 수행한다.
 * 즉, 스레드 0은 블록 0부터, 스레드 1은 블록 (req_size/block_size)부터 읽는다.
 *
 * launch_bounds(64,32): 블록당 최대 64스레드, SM당 최대 32블록으로 제한하여
 * 레지스터 사용량을 최적화하고 SM 점유율을 극대화한다.
 *
 * 컨트롤러/큐 선택 전략:
 *   - 컨트롤러: warp의 lane 0이 atomic counter로 라운드로빈 선택 → __shfl_sync로 warp 전체에 전파
 *   - 큐: SM ID를 기반으로 큐를 선택 (같은 SM의 warp들이 같은 큐를 사용하여 지역성 향상)
 *
 * @param ctrls              NVMe 컨트롤러 배열 (디바이스 포인터의 포인터)
 * @param pc                 페이지 캐시 디스크립터 (디바이스)
 * @param req_size           요청 1건의 크기 (바이트). page_size와 동일하게 설정
 * @param n_reqs             총 요청 수 (= 총 스레드 수)
 * @param req_count          완료된 요청 카운터 (현재 미사용)
 * @param num_ctrls          사용 가능한 NVMe 컨트롤러 수
 * @param reqs_per_thread    각 스레드가 반복할 I/O 횟수
 * @param access_type        접근 유형: READ(0), WRITE(1), MIXED(2)
 * @param access_type_assignment  MIXED 모드 시 스레드별 읽기/쓰기 지정 배열
 */
__global__ __launch_bounds__(64,32)
void sequential_access_kernel(Controller** ctrls, page_cache_d_t* pc,  uint32_t req_size, uint32_t n_reqs, unsigned long long* req_count, uint32_t num_ctrls, uint64_t reqs_per_thread, uint32_t access_type, uint8_t* access_type_assignment) {
    //printf("in threads\n");
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t laneid = lane_id();     /* warp 내 레인 번호 (0~31) */
    uint32_t bid = blockIdx.x;
    uint32_t smid = get_smid();      /* 현재 스레드가 실행 중인 SM(Streaming Multiprocessor) ID */

    /* 컨트롤러 및 큐 선택: warp의 lane 0만 atomic 연산을 수행하여
     * 컨트롤러를 라운드로빈으로 선택하고, SM ID 기반으로 큐를 배정한다.
     * 결과를 __shfl_sync로 warp 전체(32스레드)에 브로드캐스트한다. */
    uint32_t ctrl;
    uint32_t queue;
    if (laneid == 0) {
        ctrl = pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
        //queue = ctrls[ctrl]->queue_counter.fetch_add(1, simt::memory_order_relaxed) %  (ctrls[ctrl]->n_qps);
        queue = smid % (ctrls[ctrl]->n_qps);
    }
    ctrl =  __shfl_sync(0xFFFFFFFF, ctrl, 0);   /* lane 0의 ctrl 값을 warp 전체에 전파 */
    queue =  __shfl_sync(0xFFFFFFFF, queue, 0);  /* lane 0의 queue 값을 warp 전체에 전파 */



    if (tid < n_reqs) {
        /* 순차 접근: tid에 비례하여 시작 블록 번호를 계산한다.
         * start_block = (tid * req_size) / block_size
         * block_size_log를 사용한 비트 시프트로 나눗셈을 대체한다. */
        uint64_t start_block = (tid*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //uint64_t start_block = (tid*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //start_block = tid;
        /* 요청당 블록 수 = req_size / block_size */
        uint64_t n_blocks = req_size >> ctrls[ctrl]->d_qps[queue].block_size_log; /// ctrls[ctrl].ns.lba_data_size;;
        //printf("tid: %llu\tstart_block: %llu\tn_blocks: %llu\n", (unsigned long long) tid, (unsigned long long) start_block, (unsigned long long) n_blocks);

        /* reqs_per_thread 만큼 I/O를 반복 수행한다.
         * access_type에 따라 read_data / write_data / access_data 중 하나를 호출한다.
         * 이 함수들은 page_cache.h에 정의된 __device__ 함수로,
         * NVMe SQ에 커맨드를 인큐하고 CQ에서 완료를 폴링한다. */
        uint8_t opcode;
        for (size_t i = 0; i < reqs_per_thread; i++) {
            if (access_type == MIXED) {
                opcode = access_type_assignment[tid];
                access_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid, opcode);
            }
            else if (access_type == READ) {
                read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);

            }
            else {
                write_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
            }
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
 * random_access_kernel: 랜덤 블록 I/O 벤치마크 커널
 * ==================================================
 * sequential_access_kernel과 동일한 구조이지만,
 * tid 기반의 순차 주소 대신 미리 생성된 랜덤 assignment 배열을 사용하여
 * 임의의 LBA 주소로 I/O를 수행한다.
 *
 * 컨트롤러/큐 선택 전략:
 *   - 컨트롤러: atomic counter 라운드로빈 (sequential과 동일)
 *   - 큐: atomic counter 라운드로빈 (sequential과 다름. 랜덤 접근은 SM 지역성 이점이 없으므로)
 *
 * @param ctrls              NVMe 컨트롤러 배열 (디바이스 포인터의 포인터)
 * @param pc                 페이지 캐시 디스크립터 (디바이스)
 * @param req_size           요청 1건의 크기 (바이트)
 * @param n_reqs             총 요청 수 (= 총 스레드 수)
 * @param req_count          완료된 요청 카운터 (현재 미사용)
 * @param num_ctrls          사용 가능한 NVMe 컨트롤러 수
 * @param assignment         랜덤 블록 인덱스 배열 (호스트에서 rand()로 생성, GPU로 복사)
 * @param reqs_per_thread    각 스레드가 반복할 I/O 횟수
 * @param access_type        접근 유형: READ(0), WRITE(1), MIXED(2)
 * @param access_type_assignment  MIXED 모드 시 스레드별 읽기/쓰기 지정 배열
 */
__global__ //__launch_bounds__(64,32)
void random_access_kernel(Controller** ctrls, page_cache_d_t* pc,  uint32_t req_size, uint32_t n_reqs, unsigned long long* req_count, uint32_t num_ctrls, uint64_t* assignment, uint64_t reqs_per_thread, uint32_t access_type, uint8_t* access_type_assignment) {
    //printf("in threads\n");
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    uint32_t laneid = lane_id();
    uint32_t bid = blockIdx.x;
    uint32_t smid = get_smid();

    uint32_t ctrl;
    uint32_t queue;
    if (laneid == 0) {
	//ctrl = smid % (pc->n_ctrls);
        ctrl = pc->ctrl_counter->fetch_add(1, simt::memory_order_relaxed) % (pc->n_ctrls);
        /* 랜덤 접근에서는 큐도 atomic counter로 라운드로빈 배정한다.
         * (순차와 달리 SM 기반 지역성이 무의미하므로) */
        queue = ctrls[ctrl]->queue_counter.fetch_add(1, simt::memory_order_relaxed) %  (ctrls[ctrl]->n_qps);
        //queue = smid % (ctrls[ctrl]->n_qps);
    }
    ctrl =  __shfl_sync(0xFFFFFFFF, ctrl, 0);
    queue =  __shfl_sync(0xFFFFFFFF, queue, 0);


    if (tid < n_reqs) {
        /* 랜덤 접근: assignment[tid]에 저장된 랜덤 블록 인덱스를 사용하여
         * 시작 블록 주소를 계산한다. assignment는 호스트에서 rand() % n_blocks로 생성됨. */
        uint64_t start_block = (assignment[tid]*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //uint64_t start_block = (tid*req_size) >> ctrls[ctrl]->d_qps[queue].block_size_log;
        //start_block = tid;
        uint64_t n_blocks = req_size >> ctrls[ctrl]->d_qps[queue].block_size_log; /// ctrls[ctrl].ns.lba_data_size;;
        //printf("tid: %llu\tstart_block: %llu\tn_blocks: %llu\n", (unsigned long long) tid, (unsigned long long) start_block, (unsigned long long) n_blocks);

        uint8_t opcode;
        for (size_t i = 0; i < reqs_per_thread; i++) {
            if (access_type == MIXED) {
                opcode = access_type_assignment[tid];
                access_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid, opcode);
            }
            else if (access_type == READ) {
                read_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);

            }
            else {
                write_data(pc, (ctrls[ctrl]->d_qps)+(queue),start_block, n_blocks, tid);
            }
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
 * main: 벤치마크 진입점
 * ====================
 * 1) 커맨드라인 인자를 파싱 (Settings::parseArguments)
 * 2) CUDA 디바이스를 선택하고 NVMe 컨트롤러들을 초기화
 * 3) GPU 메모리에 페이지 캐시를 생성
 * 4) 랜덤 모드면 랜덤 블록 인덱스 배열을 생성
 * 5) MIXED 모드면 스레드별 읽기/쓰기 비율에 따라 opcode 배열을 생성
 * 6) 커널을 실행하고 CUDA 이벤트로 경과 시간을 측정
 * 7) IOPS, 대역폭, 캐시 통계를 출력
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

        /* ---- (선택) 입력 파일을 mmap: SSD에 쓸 데이터를 호스트 파일에서 가져올 때 사용 ---- */
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

            /* 파일 전체를 읽기/쓰기 가능하게 메모리 매핑한다.
             * 벤치마크 종료 후 캐시 데이터를 이 매핑에 복사하여 파일로 기록할 수 있다. */
            map_in = mmap(NULL, sb_in.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_in, 0);

            if((map_in == (void*)-1)){
                fprintf(stderr,"Input file map failed %d\n",map_in);
                return 1;
            }

            close(fd_in);
        }

        /* ---- (2) NVMe 컨트롤러 초기화 ----
         * settings.n_ctrls 개의 NVMe 컨트롤러를 생성한다.
         * Controller 생성자는 내부적으로:
         *   - /dev/libnvmX를 열어 NVMe BAR를 GPU 메모리에 매핑
         *   - Admin Queue를 통해 Identify 커맨드를 보내 네임스페이스 정보 획득
         *   - numQueues 개의 I/O QueuePair를 생성하고 GPU 메모리에 배치
         * ssdtype: 0=Samsung, 1=Intel (경로 선택용) */
        cuda_err_chk(cudaSetDevice(settings.cudaDevice));
        std::vector<Controller*> ctrls(settings.n_ctrls);
        for (size_t i = 0 ; i < settings.n_ctrls; i++)
            ctrls[i] = new Controller(settings.ssdtype == 0 ? sam_ctrls_paths[i] : intel_ctrls_paths[i], settings.nvmNamespace, settings.cudaDevice, settings.queueDepth, settings.numQueues);

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
        uint64_t b_size = settings.blkSize;                      /* CUDA 블록당 스레드 수 (기본 64) */
        uint64_t g_size = (settings.numThreads + b_size - 1)/b_size; /* 필요한 CUDA 블록 수 (올림 나눗셈) */
        uint64_t n_threads = b_size * g_size;                    /* 실제 실행되는 총 스레드 수 */


        /* ---- (3) 페이지 캐시 생성 ---- */
        uint64_t page_size = settings.pageSize;        /* 캐시 페이지 크기 (기본 4096B). I/O 요청 단위이기도 함 */
        uint64_t n_pages = settings.numPages;          /* 캐시에 할당할 페이지 수 */
        uint64_t total_cache_size = (page_size * n_pages);  /* 총 캐시 크기 (바이트) */
        //uint64_t n_pages = total_cache_size/page_size;
        //
        /* 페이지 수가 스레드 수보다 적으면 1:1 매핑이 불가능하므로 에러 */
        if (n_pages < n_threads) {
            std::cerr << "Please provide enough pages. Number of pages must be greater than or equal to the number of threads!\n";
            exit(1);
        }


        /* page_cache_t 생성: GPU 메모리에 n_pages 개의 page_size 크기 슬롯을 할당한다.
         * 내부적으로 cudaMalloc으로 base_addr을 할당하고,
         * 각 페이지의 PRP(Physical Region Page) 주소를 NVMe 커맨드에 사용할 수 있도록 준비한다.
         * 64는 캐시 라인 크기를 의미한다. */
        page_cache_t h_pc(page_size, n_pages, settings.cudaDevice, ctrls[0][0], (uint64_t) 64, ctrls);
        std::cout << "finished creating cache\n";

        //QueuePair* d_qp;
        page_cache_d_t* d_pc = (page_cache_d_t*) (h_pc.d_pc_ptr);  /* 디바이스 측 캐시 디스크립터 포인터 */
        #define TYPE uint64_t
        uint64_t n_blocks = settings.numBlks;   /* backing 배열의 총 페이지(블록) 수. SSD에서 접근 가능한 범위 */
        //uint64_t t_size = n_blocks * sizeof(TYPE);

        // range_t<uint64_t> h_range((uint64_t)0, (uint64_t)n_elems, (uint64_t)0, (uint64_t)(t_size/page_size), (uint64_t)0, (uint64_t)page_size, &h_pc, settings.cudaDevice);
        // range_t<uint64_t>* d_range = (range_t<uint64_t>*) h_range.d_range_ptr;

        // std::vector<range_t<uint64_t>*> vr(1);
        // vr[0] = & h_range;
        // //(const uint64_t num_elems, const uint64_t disk_start_offset, const std::vector<range_t<T>*>& ranges, Settings& settings)
        // array_t<uint64_t> a(n_elems, 0, vr, settings.cudaDevice);


        //std::cout << "finished creating range\n";




        /* 요청 카운트용 디바이스 메모리 (현재 커널 내에서는 미사용) */
        unsigned long long* d_req_count;
        cuda_err_chk(cudaMalloc(&d_req_count, sizeof(unsigned long long)));
        cuda_err_chk(cudaMemset(d_req_count, 0, sizeof(unsigned long long)));

        /* GPU의 PCIe Bus ID를 출력 (디버그/확인용) */
        char st[15];
        cuda_err_chk(cudaDeviceGetPCIBusId(st, 15, settings.cudaDevice));
        std::cout << st << std::endl;

        /* ---- (4) 랜덤 모드: 랜덤 블록 인덱스 배열 생성 ----
         * 각 스레드가 접근할 블록 인덱스를 호스트에서 rand()로 생성하고
         * GPU 메모리로 복사한다. 범위는 [0, n_blocks). */
        uint64_t* assignment;
        uint64_t* d_assignment;
        if (settings.random) {
            assignment = (uint64_t*) malloc(n_threads*sizeof(uint64_t));
            for (size_t i = 0; i< n_threads; i++)
                assignment[i] = rand() % (n_blocks);


            cuda_err_chk(cudaMalloc(&d_assignment, n_threads*sizeof(uint64_t)));
            cuda_err_chk(cudaMemcpy(d_assignment, assignment,  n_threads*sizeof(uint64_t), cudaMemcpyHostToDevice));
        }

        /* ---- 타이밍 시작: CUDA 이벤트 기록 ----
         * Event 생성자에서 cudaEventRecord가 호출된다. */
        Event before;

        /* ---- (5) MIXED 모드: 스레드별 읽기/쓰기 비율 배정 ----
         * settings.ratio(%)에 따라 각 스레드를 READ 또는 WRITE로 분류한다.
         * 예: ratio=70이면 약 70%의 스레드가 READ, 30%가 WRITE를 수행한다. */
        uint8_t* access_assignment;
        uint8_t* d_access_assignment = NULL;
        if (settings.accessType == 2) {
            access_assignment = (uint8_t*) malloc(n_threads*sizeof(uint8_t));
            for (size_t i = 0; i < n_threads; i++)
                access_assignment[i] = (((rand() % 100) + 1) <= settings.ratio) ? NVM_IO_READ : NVM_IO_WRITE;

            cuda_err_chk(cudaMalloc(&d_access_assignment, n_threads*sizeof(uint8_t)));
            cuda_err_chk(cudaMemcpy(d_access_assignment, access_assignment, n_threads*sizeof(uint8_t), cudaMemcpyHostToDevice));
        }

        /* ---- (6) 커널 실행 ----
         * random=true이면 random_access_kernel, 아니면 sequential_access_kernel을 실행한다.
         * 커널은 비동기적으로 시작되며, cudaDeviceSynchronize에서 완료를 기다린다. */
        std::cout << "atlaunch kernel\n";
        if (settings.random)
            random_access_kernel<<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, d_assignment, settings.numReqs, settings.accessType, d_access_assignment);
        else
            sequential_access_kernel<<<g_size, b_size>>>(h_pc.pdt.d_ctrls, d_pc, page_size, n_threads, d_req_count, settings.n_ctrls, settings.numReqs, settings.accessType, d_access_assignment);

        /* 타이밍 종료: CUDA 이벤트 기록 */
        Event after;

        //print_cache_kernel<<<1,1>>>(d_pc);
        //new_kernel<<<1,1>>>();
        //uint8_t* ret_array = (uint8_t*) malloc(n_pages*page_size);

        //cuda_err_chk(cudaMemcpy(ret_array, h_pc.base_addr,page_size*n_pages, cudaMemcpyDeviceToHost));

        /* GPU 커널 완료 대기 */
        cuda_err_chk(cudaDeviceSynchronize());

        /* 입력 파일이 지정된 경우, 캐시 내용을 호스트 mmap 영역에 복사 (결과 확인용) */
        if (input_f != nullptr) {
            cuda_err_chk(cudaMemcpy(map_in, h_pc.pdt.base_addr,  std::min((uint64_t)sb_in.st_size, total_cache_size), cudaMemcpyDeviceToHost));
            munmap(map_in, sb_in.st_size);
        }

        /* ---- (7) 성능 결과 계산 및 출력 ----
         * elapsed: 마이크로초 단위의 경과 시간 (Event::operator- 로 계산)
         * ios: 총 I/O 요청 수 = 총 스레드 수 × 스레드당 요청 수
         * data: 총 전송 데이터 크기 = ios × page_size
         * iops: 초당 I/O 연산 수 = ios / (elapsed / 1,000,000)
         * bandwidth: 유효 대역폭(GB/s) = data / elapsed(초) / 1GiB */
        double elapsed = after - before;
        uint64_t ios = g_size*b_size*settings.numReqs;
        uint64_t data = ios*page_size;
        double iops = ((double)ios)/(elapsed/1000000);
        double bandwidth = (((double)data)/(elapsed/1000000))/(1024ULL*1024ULL*1024ULL);
        std::cout << std::dec << "Elapsed Time: " << elapsed << "\tNumber of Ops: "<< ios << "\tData Size (bytes): " << data << std::endl;
        std::cout << std::dec << "Ops/sec: " << iops << "\tEffective Bandwidth(GB/S): " << bandwidth << std::endl;

        /* 페이지 캐시 적중률, 미스율 등 통계를 출력하고 초기화 */
        h_pc.print_reset_stats();
        //std::cout << std::dec << ctrls[0]->ns.lba_data_size << std::endl;

        //std::ofstream ofile("../data", std::ios::binary | std::ios::trunc);
        //ofile.write((char*)ret_array, data);
        //ofile.close();

        /* 리소스 정리: NVMe 컨트롤러 객체 해제 */
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
