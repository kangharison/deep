/* ctrl.h - NVMe 컨트롤러 관리 구조체
 * NVMe 컨트롤러 초기화, Admin Queue 생성, I/O Queue Pair 생성, GPU 메모리 매핑 등
 * BaM 시스템에서 NVMe 디바이스를 GPU에서 직접 접근하기 위한 전체 셋업을 담당한다.
 * GPU 스레드들이 NVMe 큐를 통해 직접 I/O를 발행할 수 있도록 컨트롤러 상태를 GPU 메모리에 복사한다.
 */

#ifndef __BENCHMARK_CTRL_H__
#define __BENCHMARK_CTRL_H__

// #ifndef __CUDACC__
// #define __device__
// #define __host__
// #endif

#include <cstdint>
#include "buffer.h"
#include "nvm_types.h"
#include "nvm_ctrl.h"
#include "nvm_aq.h"
#include "nvm_admin.h"
#include "nvm_util.h"
#include "nvm_error.h"
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <algorithm>
#include <simt/atomic>

#include "queue.h"


#define MAX_QUEUES 1024 // 최대 큐 수: Admin 커맨드로 요청할 SQ/CQ 쌍의 최대 개수


/* Controller 구조체: NVMe 컨트롤러와 관련된 모든 상태를 관리한다.
 * 호스트 측 컨트롤러 핸들, Admin Queue, I/O Queue Pair, GPU 복사본 등을 포함한다.
 * GPU 커널에서 직접 NVMe I/O를 발행하기 위해 이 구조체의 디바이스 복사본이 사용된다.
 */
struct Controller
{
    simt::atomic<uint64_t, simt::thread_scope_device> access_counter; // SSD 접근 횟수 카운터 (GPU에서 atomic 증가)
    nvm_ctrl_t*             ctrl;       // libnvm NVMe 컨트롤러 핸들
    nvm_aq_ref              aq_ref;     // Admin Queue 참조 핸들 (Admin 커맨드 발행용)
    DmaPtr                  aq_mem;     // Admin Queue DMA 메모리 (SQ + CQ + 식별 버퍼)
    struct nvm_ctrl_info    info;       // 컨트롤러 식별 정보 (Identify Controller 결과)
    struct nvm_ns_info      ns;         // 네임스페이스 식별 정보 (LBA 크기 등)
    uint16_t                n_sqs;      // 할당받은 Submission Queue 수
    uint16_t                n_cqs;      // 할당받은 Completion Queue 수
    uint16_t                n_qps;      // 실제 사용할 Queue Pair 수 (min(n_sqs, n_cqs, numQueues))
    uint32_t                deviceId;   // 사용할 CUDA GPU 디바이스 ID
    QueuePair**             h_qps;      // 호스트 측 QueuePair 포인터 배열
    QueuePair*              d_qps;      // GPU 디바이스 측 QueuePair 배열 (GPU 커널에서 사용)

    simt::atomic<uint64_t, simt::thread_scope_device> queue_counter; // 큐 라운드로빈 카운터

    uint32_t page_size;     // NVMe 컨트롤러 페이지 크기 (보통 4KB)
    uint32_t blk_size;      // LBA 데이터 크기 (보통 512B 또는 4KB)
    uint32_t blk_size_log;  // LBA 크기의 log2 값 (비트 시프트 연산용)


    void* d_ctrl_ptr;           // GPU 메모리에 복사된 Controller 구조체의 디바이스 포인터
    BufferPtr d_ctrl_buff;      // GPU 메모리 버퍼 (Controller 구조체 복사본)
#ifdef __DIS_CLUSTER__
    /* DIS 클러스터 환경 생성자 */
    Controller(uint64_t controllerId, uint32_t nvmNamespace, uint32_t adapter, uint32_t segmentId);
#endif

    /* 기본 생성자: NVMe 디바이스 경로, 네임스페이스 ID, CUDA 디바이스, 큐 깊이, 큐 수를 받는다 */
    Controller(const char* path, uint32_t nvmNamespace, uint32_t cudaDevice, uint64_t queueDepth, uint64_t numQueues);

    /* 큐 예약 함수들: Admin 커맨드로 NVMe 컨트롤러에 I/O 큐를 요청한다 */
    void reserveQueues();
    void reserveQueues(uint16_t numSubmissionQueues);
    void reserveQueues(uint16_t numSubmissionQueues, uint16_t numCompletionQueues);

    /* print_reset_stats: GPU에서 수집된 SSD 접근 통계를 출력하고 카운터를 리셋한다 */
    void print_reset_stats(void);

    ~Controller();
};



using error = std::runtime_error;
using std::string;


/* print_reset_stats: GPU 디바이스 메모리에서 access_counter를 호스트로 복사하여 출력하고 0으로 리셋한다 */
inline void Controller::print_reset_stats(void) {
    // GPU에서 호스트로 접근 카운터를 복사한다
    cuda_err_chk(cudaMemcpy(&access_counter, d_ctrl_ptr, sizeof(simt::atomic<uint64_t, simt::thread_scope_device>), cudaMemcpyDeviceToHost));
    std::cout << "------------------------------------" << std::endl;
    std::cout << std::dec << "#SSDAccesses:\t" << access_counter << std::endl;

    // GPU 메모리의 카운터를 0으로 리셋한다
    cuda_err_chk(cudaMemset(d_ctrl_ptr, 0, sizeof(simt::atomic<uint64_t, simt::thread_scope_device>)));
}

/* initializeController: Admin Queue를 통해 컨트롤러를 초기화한다.
 * 1) Admin Queue 생성 (컨트롤러 리셋 포함)
 * 2) Identify Controller 커맨드로 컨트롤러 정보 조회
 * 3) Identify Namespace 커맨드로 네임스페이스 정보 조회 (LBA 크기 등)
 * 4) Get Number of Queues로 지원되는 큐 수 조회
 */
static void initializeController(struct Controller& ctrl, uint32_t ns_id)
{
    // Admin Queue 참조를 생성한다 (내부적으로 컨트롤러 리셋 수행)
    int status = nvm_aq_create(&ctrl.aq_ref, ctrl.ctrl, ctrl.aq_mem.get());
    if (!nvm_ok(status))
    {
        throw error(string("Failed to reset controller: ") + nvm_strerror(status));
    }

    // Identify Controller 커맨드로 컨트롤러 정보를 조회한다
    status = nvm_admin_ctrl_info(ctrl.aq_ref, &ctrl.info, NVM_DMA_OFFSET(ctrl.aq_mem, 2), ctrl.aq_mem->ioaddrs[2]);
    if (!nvm_ok(status))
    {
        throw error(nvm_strerror(status));
    }

    // Identify Namespace 커맨드로 네임스페이스 정보를 조회한다 (LBA 데이터 크기 등)
    status = nvm_admin_ns_info(ctrl.aq_ref, &ctrl.ns, ns_id, NVM_DMA_OFFSET(ctrl.aq_mem, 2), ctrl.aq_mem->ioaddrs[2]);
    if (!nvm_ok(status))
    {
        throw error(nvm_strerror(status));
    }

    // Set/Get Number of Queues 커맨드로 지원되는 CQ/SQ 수를 조회한다
    status = nvm_admin_get_num_queues(ctrl.aq_ref, &ctrl.n_cqs, &ctrl.n_sqs);
    if (!nvm_ok(status))
    {
        throw error(nvm_strerror(status));
    }
}



#ifdef __DIS_CLUSTER__
/* DIS 클러스터 환경용 생성자: DIS 컨트롤러 ID로 원격 NVMe 컨트롤러에 접근한다 */
Controller::Controller(uint64_t ctrl_id, uint32_t ns_id, uint32_t)
    : ctrl(nullptr)
    , aq_ref(nullptr)
{
    // DIS를 통해 원격 NVMe 컨트롤러 참조를 얻는다
    int status = nvm_dis_ctrl_init(&ctrl, ctrl_id);
    if (!nvm_ok(status))
    {
        throw error(string("Failed to get controller reference: ") + nvm_strerror(status));
    }

    // Admin Queue 메모리를 생성한다 (3 페이지: ASQ + ACQ + 식별 버퍼)
    aq_mem = createDma(ctrl, ctrl->page_size * 3, 0, 0);

    initializeController(*this, ns_id);
}
#endif



/* Controller 생성자 (로컬 NVMe):
 * 1) NVMe 디바이스를 열고 컨트롤러 핸들을 초기화한다
 * 2) Admin Queue를 생성하여 컨트롤러를 초기화한다
 * 3) NVMe 컨트롤러의 MMIO 레지스터를 GPU가 접근할 수 있도록 cudaHostRegister(IoMemory)한다
 * 4) 지정된 수만큼 I/O Queue Pair를 생성하고 GPU 메모리에 복사한다
 * 5) Controller 구조체 전체를 GPU 메모리에 복사하여 GPU 커널이 사용할 수 있게 한다
 */
inline Controller::Controller(const char* path, uint32_t ns_id, uint32_t cudaDevice, uint64_t queueDepth, uint64_t numQueues)
    : ctrl(nullptr)
    , aq_ref(nullptr)
    , deviceId(cudaDevice)
{
    // NVMe 디바이스 파일을 연다 (/dev/nvmeXnY 또는 BaM 자체 드라이버 경로)
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        throw error(string("Failed to open descriptor: ") + strerror(errno));
    }

    // 파일 디스크립터로 NVMe 컨트롤러 핸들을 초기화한다 (MMIO BAR 매핑 포함)
    int status = nvm_ctrl_init(&ctrl, fd);
    if (!nvm_ok(status))
    {
        throw error(string("Failed to get controller reference: ") + nvm_strerror(status));
    }

    // Admin Queue 메모리를 호스트에 할당한다 (3 페이지)
    aq_mem = createDma(ctrl, ctrl->page_size * 3);

    // Admin Queue를 통해 컨트롤러를 초기화한다 (리셋, 식별, 큐 수 조회)
    initializeController(*this, ns_id);

    // NVMe 컨트롤러의 MMIO 레지스터(도어벨 포함)를 GPU가 직접 접근할 수 있도록 등록한다
    // cudaHostRegisterIoMemory 플래그가 핵심: GPU가 PCIe MMIO를 직접 읽고 쓸 수 있게 한다
    cudaError_t err = cudaHostRegister((void*) ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE, cudaHostRegisterIoMemory);
    if (err != cudaSuccess)
    {
        throw error(string("Unexpected error while mapping IO memory (cudaHostRegister): ") + cudaGetErrorString(err));
    }

    // 큐 라운드로빈 카운터를 초기화한다
    queue_counter = 0;
    // 컨트롤러 페이지 크기를 저장한다
    page_size = ctrl->page_size;
    // LBA 데이터 크기와 log2 값을 저장한다
    blk_size = this->ns.lba_data_size;
    blk_size_log = std::log2(blk_size);

    // 컨트롤러에 최대 큐 수를 요청한다 (Set Features: Number of Queues)
    reserveQueues(MAX_QUEUES,MAX_QUEUES);

    // 실제 사용할 Queue Pair 수를 결정한다 (SQ수, CQ수, 요청 큐수 중 최소값)
    n_qps = std::min(n_sqs, n_cqs);
    n_qps = std::min(n_qps, (uint16_t)numQueues);
    printf("SQs: %d\tCQs: %d\tn_qps: %d\n", n_sqs, n_cqs, n_qps);

    // 호스트 측 QueuePair 포인터 배열을 할당한다
    h_qps = (QueuePair**) malloc(sizeof(QueuePair)*n_qps);
    // GPU 측 QueuePair 배열을 할당한다
    cuda_err_chk(cudaMalloc((void**)&d_qps, sizeof(QueuePair)*n_qps));

    // 각 Queue Pair를 생성하고 GPU 메모리에 복사한다
    for (size_t i = 0; i < n_qps; i++) {
        // I/O Queue Pair를 생성한다 (SQ + CQ + 도어벨 GPU 매핑)
        h_qps[i] = new QueuePair(ctrl, cudaDevice, ns, info, aq_ref, i+1, queueDepth);
        // 생성된 QueuePair를 GPU 메모리로 복사한다
        cuda_err_chk(cudaMemcpy(d_qps+i, h_qps[i], sizeof(QueuePair), cudaMemcpyHostToDevice));
    }

    // 디바이스 파일을 닫는다 (MMIO 매핑은 유지됨)
    close(fd);

    // Controller 구조체를 GPU 메모리에 복사한다 (GPU 커널이 접근할 수 있도록)
    d_ctrl_buff = createBuffer(sizeof(Controller), cudaDevice);
    d_ctrl_ptr = d_ctrl_buff.get();
    cuda_err_chk(cudaMemcpy(d_ctrl_ptr, this, sizeof(Controller), cudaMemcpyHostToDevice));
}



/* 소멸자: GPU QueuePair 메모리, 호스트 QueuePair, Admin Queue, 컨트롤러 핸들을 해제한다 */
inline Controller::~Controller()
{
    cudaFree(d_qps);             // GPU 측 QueuePair 배열 해제
    for (size_t i = 0; i < n_qps; i++) {
        delete h_qps[i];         // 각 호스트 QueuePair 해제 (내부 DMA 메모리 포함)
    }
    free(h_qps);                 // 호스트 포인터 배열 해제
    nvm_aq_destroy(aq_ref);      // Admin Queue 참조 해제
    nvm_ctrl_free(ctrl);         // NVMe 컨트롤러 핸들 해제 (MMIO 언매핑 포함)

}



/* reserveQueues: 현재 n_sqs, n_cqs 값으로 큐를 예약한다 */
inline void Controller::reserveQueues()
{
    reserveQueues(n_sqs, n_cqs);
}



/* reserveQueues: 지정된 SQ 수와 현재 CQ 수로 큐를 예약한다 */
inline void Controller::reserveQueues(uint16_t numSubmissionQueues)
{
    reserveQueues(numSubmissionQueues, n_cqs);
}



/* reserveQueues: NVMe Admin 커맨드(Set Features: Number of Queues)로
 * 지정된 수의 SQ와 CQ를 컨트롤러에 요청한다.
 * 컨트롤러가 실제로 할당한 큐 수를 n_sqs, n_cqs에 저장한다.
 */
inline void Controller::reserveQueues(uint16_t numSubs, uint16_t numCpls)
{
    // Admin 커맨드로 큐 수를 요청하고 실제 할당된 수를 받는다
    int status = nvm_admin_request_num_queues(aq_ref, &numSubs, &numCpls);
    if (!nvm_ok(status))
    {
        throw error(string("Failed to reserve queues: ") + nvm_strerror(status));
    }

    n_sqs = numSubs; // 실제 할당된 SQ 수를 저장한다
    n_cqs = numCpls; // 실제 할당된 CQ 수를 저장한다

}



#endif
