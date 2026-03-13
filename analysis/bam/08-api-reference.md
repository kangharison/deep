# 08. BaM API 레퍼런스

## 1. 컨트롤러 초기화 API (nvm_ctrl.h)

### nvm_ctrl_init
```c
int nvm_ctrl_init(nvm_ctrl_t** ctrl, int fd);
```
커널 모듈 캐릭터 디바이스의 파일 디스크립터로 NVMe 컨트롤러 핸들을 초기화한다. 내부적으로 BAR0을 mmap하고 CAP 레지스터를 읽어 페이지 크기, 도어벨 스트라이드, 타임아웃 등을 파싱한다.
- **fd**: `/dev/libnvmN`을 `O_RDWR`로 연 파일 디스크립터
- **반환**: 0 성공, 그 외 errno

### nvm_raw_ctrl_init
```c
int nvm_raw_ctrl_init(nvm_ctrl_t** ctrl, volatile void* mm_ptr, size_t mm_size);
```
BAR0에 대한 메모리 맵 포인터를 직접 전달하여 컨트롤러 핸들을 초기화한다. VFIO 등을 통해 BAR0을 직접 mmap한 경우 사용한다.
- **mm_ptr**: BAR0 MMIO 포인터 (최소 `NVM_CTRL_MEM_MINSIZE` = 0x2000바이트)
- **mm_size**: 매핑 영역 크기

### nvm_raw_ctrl_reset
```c
int nvm_raw_ctrl_reset(const nvm_ctrl_t* ctrl, uint64_t acq_ioaddr, uint64_t asq_ioaddr);
```
NVMe 컨트롤러를 리셋한다. CC.EN=0 → CSTS.RDY=0 대기 → AQA/ASQ/ACQ 설정 → CC.EN=1 → CSTS.RDY=1 대기 순서로 진행한다.
- **acq_ioaddr**: Admin Completion Queue의 물리/버스 주소
- **asq_ioaddr**: Admin Submission Queue의 물리/버스 주소

### nvm_ctrl_free
```c
void nvm_ctrl_free(nvm_ctrl_t* ctrl);
```
컨트롤러 핸들을 해제하고 할당된 리소스를 반환한다.

### nvm_dis_ctrl_init (DIS 전용)
```c
int nvm_dis_ctrl_init(nvm_ctrl_t** ctrl, uint32_t smartio_fdid);
```
SmartIO Fabric Device ID를 통해 원격 NVMe 컨트롤러 핸들을 초기화한다.


## 2. Admin Queue API (nvm_aq.h)

### nvm_aq_create
```c
int nvm_aq_create(nvm_aq_ref* ref, const nvm_ctrl_t* ctrl, const nvm_dma_t* dma_window);
```
어드민 큐 쌍(ASQ + ACQ)을 생성하고 컨트롤러의 배타적 소유권을 획득한다. 내부적으로 `nvm_raw_ctrl_reset()`을 호출하여 컨트롤러를 리셋한다.
- **dma_window**: 어드민 큐 메모리 (최소 2페이지: ASQ 1페이지 + ACQ 1페이지, 식별 버퍼용 3번째 페이지 권장)
- **반환**: 0 성공, 그 외 errno

### nvm_aq_destroy
```c
void nvm_aq_destroy(nvm_aq_ref ref);
```
어드민 큐와 참조를 파괴한다. 이후 모든 어드민 큐 참조가 무효화된다.


## 3. Admin 커맨드 API (nvm_admin.h)

### nvm_admin_ctrl_info
```c
int nvm_admin_ctrl_info(nvm_aq_ref ref, struct nvm_ctrl_info* info,
                        void* buffer, uint64_t ioaddr);
```
IDENTIFY CONTROLLER 커맨드를 실행하여 컨트롤러 정보(모델명, 시리얼, MDTS, MQES 등)를 조회한다.
- **buffer**: 4KB 이상의 DMA 임시 버퍼 (가상 주소)
- **ioaddr**: 버퍼의 컨트롤러 관점 버스 주소

### nvm_admin_ns_info
```c
int nvm_admin_ns_info(nvm_aq_ref ref, struct nvm_ns_info* info,
                      uint32_t ns_id, void* buffer, uint64_t ioaddr);
```
IDENTIFY NAMESPACE 커맨드를 실행하여 네임스페이스 정보(크기, LBA 블록 크기, 메타데이터 크기 등)를 조회한다.

### nvm_admin_cq_create
```c
int nvm_admin_cq_create(nvm_aq_ref ref, nvm_queue_t* cq, uint16_t id,
                        const nvm_dma_t* dma, size_t page_offset,
                        size_t qs, bool need_prp = false);
```
CREATE IO COMPLETION QUEUE 커맨드로 I/O CQ를 생성한다.
- **id**: 큐 식별자 (1 이상, 고유)
- **dma**: CQ 메모리의 DMA 핸들 (GPU 메모리 가능)
- **page_offset**: DMA 핸들 내 페이지 오프셋
- **qs**: 큐 엔트리 수 (0이면 한 페이지 기본값)
- **need_prp**: true이면 비연속 메모리용 PRP 리스트 사용

### nvm_admin_sq_create
```c
int nvm_admin_sq_create(nvm_aq_ref ref, nvm_queue_t* sq, const nvm_queue_t* cq,
                        uint16_t id, const nvm_dma_t* dma, size_t page_offset,
                        size_t qs, bool need_prp = false);
```
CREATE IO SUBMISSION QUEUE 커맨드로 I/O SQ를 생성한다. 각 SQ는 반드시 하나의 CQ와 쌍을 이루어야 한다.

### nvm_admin_cq_delete / nvm_admin_sq_delete
```c
int nvm_admin_cq_delete(nvm_aq_ref ref, nvm_queue_t* cq);
int nvm_admin_sq_delete(nvm_aq_ref ref, nvm_queue_t* sq, const nvm_queue_t* cq);
```
I/O CQ/SQ를 삭제한다. SQ를 먼저 삭제한 후 CQ를 삭제해야 한다.

### nvm_admin_set_num_queues / nvm_admin_get_num_queues
```c
int nvm_admin_set_num_queues(nvm_aq_ref ref, uint16_t n_cqs, uint16_t n_sqs);
int nvm_admin_get_num_queues(nvm_aq_ref ref, uint16_t* n_cqs, uint16_t* n_sqs);
```
SET/GET FEATURES (Number of Queues) 커맨드로 I/O 큐 수를 설정/조회한다.

### nvm_admin_request_num_queues
```c
int nvm_admin_request_num_queues(nvm_aq_ref ref, uint16_t* n_cqs, uint16_t* n_sqs);
```
큐 수를 요청하고 실제 할당된 수를 받는 편의 함수이다. `nvm_admin_set_num_queues`와 `nvm_admin_get_num_queues`를 결합한다.

### nvm_admin_get_log_page
```c
int nvm_admin_get_log_page(nvm_aq_ref ref, uint32_t ns_id, void* ptr,
                           uint64_t ioaddr, uint8_t log_id, uint64_t log_offset);
```
GET LOG PAGE 커맨드로 에러 로그, SMART/Health 정보 등을 조회한다.


## 4. DMA 매핑 API (nvm_dma.h)

### nvm_dma_map
```c
int nvm_dma_map(nvm_dma_t** map, const nvm_ctrl_t* ctrl, void* vaddr,
                size_t page_size, size_t n_pages, const uint64_t* page_addrs);
```
사용자가 이미 물리/버스 주소를 알고 있을 때 DMA 매핑 디스크립터를 생성한다. 커널 경유 없이 직접 주소를 설정한다.

### nvm_dma_map_host
```c
int nvm_dma_map_host(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                     void* vaddr, size_t size);
```
호스트 메모리 가상 주소를 커널 모듈을 통해 DMA 매핑한다. 내부적으로 `get_user_pages()` + `dma_map_page()`를 수행한다.
- **vaddr**: 시스템 페이지 크기에 정렬된 호스트 가상 주소

### nvm_dma_map_device
```c
int nvm_dma_map_device(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                       void* devptr, size_t size);
```
CUDA GPU 디바이스 메모리를 커널 모듈을 통해 DMA 매핑한다. 내부적으로 `nvidia_p2p_get_pages()` + `nvidia_p2p_dma_map_pages()`를 수행한다.
- **devptr**: 64KB 정렬된 CUDA 디바이스 포인터

### nvm_dma_remap
```c
int nvm_dma_remap(nvm_dma_t** new_map, const nvm_dma_t* other_map);
```
기존 DMA 매핑을 참조 카운트 공유 방식으로 복제한다. 디바이스 매핑을 다시 수행하지 않는다.

### nvm_dma_unmap
```c
void nvm_dma_unmap(nvm_dma_t* map);
```
DMA 매핑을 해제한다. 참조 카운트가 0이 되면 실제 디바이스 언매핑도 수행한다.

### nvm_dma_create (non-CUDA 전용)
```c
int nvm_dma_create(nvm_dma_t** map, const nvm_ctrl_t* ctrl, size_t size);
```
페이지 정렬된 호스트 버퍼를 할당하고 DMA 매핑을 생성하는 단축 함수이다.

### DIS 전용 DMA 함수
```c
int nvm_dis_dma_map_local(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                          uint32_t adapter, sci_local_segment_t segment, bool map_vaddr);
int nvm_dis_dma_map_remote(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                           sci_remote_segment_t segment, bool map_vaddr, bool map_wc);
int nvm_dis_dma_create(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                       size_t size, unsigned int mem_hints);
int nvm_dis_dma_map_host(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                         void* vaddr, size_t size);
int nvm_dis_dma_map_device(nvm_dma_t** map, const nvm_ctrl_t* ctrl,
                           void* devptr, size_t size);
```


## 5. 표준 큐 연산 API (nvm_queue.h)

Admin Queue에서 사용하는 단일 스레드 큐 연산 함수들이다. GPU 병렬 큐는 `nvm_parallel_queue.h`를 사용한다.

### nvm_queue_clear
```c
int nvm_queue_clear(nvm_queue_t* q, const nvm_ctrl_t* ctrl, bool cq,
                    uint16_t no, uint32_t qs, bool local,
                    volatile void* vaddr, uint64_t ioaddr);
```
큐 디스크립터를 초기화하고 도어벨 포인터를 설정한다.

### nvm_sq_enqueue
```c
nvm_cmd_t* nvm_sq_enqueue(nvm_queue_t* sq);
```
SQ에 커맨드 슬롯을 할당한다. 큐가 가득 차면 NULL을 반환한다.

### nvm_cq_poll
```c
nvm_cpl_t* nvm_cq_poll(const nvm_queue_t* cq);
```
CQ의 현재 head 위치에 새 완료가 있는지 위상 비트로 확인한다.

### nvm_cq_dequeue
```c
nvm_cpl_t* nvm_cq_dequeue(nvm_queue_t* cq);
```
CQ에서 완료 엔트리를 디큐하고 head 포인터를 전진시킨다.

### nvm_cq_dequeue_block
```c
nvm_cpl_t* nvm_cq_dequeue_block(nvm_queue_t* cq, uint64_t timeout);
```
타임아웃까지 블로킹 방식으로 완료를 기다린다.

### nvm_sq_submit
```c
void nvm_sq_submit(nvm_queue_t* sq);
```
SQ tail 도어벨을 울려 인큐된 커맨드를 컨트롤러에 제출한다.

### nvm_sq_update
```c
void nvm_sq_update(nvm_queue_t* sq);
```
완료 처리 후 SQ head를 전진시켜 슬롯을 해제한다.

### nvm_cq_update
```c
void nvm_cq_update(nvm_queue_t* cq);
```
CQ head 도어벨을 울려 처리 완료를 컨트롤러에 알린다.


## 6. 커맨드 빌드 API (nvm_cmd.h)

### nvm_cmd_header
```c
void nvm_cmd_header(nvm_cmd_t* cmd, uint16_t cid, uint8_t opcode, uint32_t ns_id);
```
NVMe 커맨드의 DWORD0(CID, opcode)과 DWORD1(NSID)을 설정한다.

### nvm_cmd_data_ptr
```c
void nvm_cmd_data_ptr(nvm_cmd_t* cmd, uint64_t prp1, uint64_t prp2);
```
커맨드의 데이터 포인터 필드(DWORD6-9)를 PRP 방식으로 설정한다.

### nvm_cmd_rw_blks
```c
void nvm_cmd_rw_blks(nvm_cmd_t* cmd, uint64_t start_lba, uint16_t n_blks);
```
Read/Write 커맨드의 시작 LBA와 블록 수를 설정한다. `n_blks`는 1-based이며 내부에서 0-based로 변환한다.

### nvm_cmd_data
```c
size_t nvm_cmd_data(nvm_cmd_t* cmd, size_t n_lists, const nvm_prp_list_t* lists,
                    size_t n_pages, const uint64_t* ioaddrs);
```
전송 크기에 따라 적절한 PRP 방식(직접 PRP1/PRP2 또는 PRP 리스트 체인)을 자동 선택하여 커맨드의 데이터 포인터를 설정한다.

### nvm_prp_list / nvm_prp_list_chain
```c
size_t nvm_prp_list(const nvm_prp_list_t* list, size_t n_pages, const uint64_t* ioaddrs);
size_t nvm_prp_list_chain(size_t n_lists, const nvm_prp_list_t* lists,
                          size_t n_pages, const uint64_t* ioaddrs);
```
PRP 리스트 페이지를 채우고 체인으로 연결한다.

### I/O Opcode 상수
```c
enum nvm_io_command_set {
    NVM_IO_FLUSH        = 0x00,  // 캐시 플러시
    NVM_IO_WRITE        = 0x01,  // 쓰기
    NVM_IO_READ         = 0x02,  // 읽기
    NVM_IO_WRITE_ZEROES = 0x08   // 제로 쓰기
};
```


## 7. GPU 병렬 큐 API (nvm_parallel_queue.h)

수천 개의 GPU 스레드가 동시에 사용할 수 있는 lock-free 큐 연산이다. 04-parallel-queue.md에 상세 분석이 있다.

### get_cid
```c
__device__ uint16_t get_cid(nvm_queue_t* sq);
```
티켓 기반 lock-free 방식으로 고유한 Command ID를 할당한다. 65536개 CID를 관리한다.

### put_cid
```c
__device__ void put_cid(nvm_queue_t* sq, uint16_t cid);
```
사용 완료한 CID를 반환하여 재사용 가능하게 한다.

### sq_enqueue
```c
__device__ nvm_cmd_t* sq_enqueue(nvm_queue_t* sq, nvm_queue_t* cq, uint16_t cid);
```
SQ에 커맨드를 인큐한다. 티켓 할당 → 슬롯 대기 → 커맨드 복사 → tail mark → 도어벨 → 티켓 업데이트의 6단계로 진행한다.

### cq_poll
```c
__device__ nvm_cpl_t* cq_poll(nvm_queue_t* cq, uint16_t cid);
```
CID 기반으로 CQ를 검색하여 특정 커맨드의 완료를 찾는다.

### cq_dequeue
```c
__device__ void cq_dequeue(nvm_queue_t* cq, uint16_t cid);
```
완료 처리 후 CQ head mark를 설정하고 head 이동을 수행한다.

### move_tail / move_head_cq / move_head_sq
```c
__device__ void move_tail(nvm_queue_t* sq, uint32_t cur_tail);
__device__ void move_head_cq(nvm_queue_t* cq, uint32_t cur_head, nvm_queue_t* sq);
__device__ void move_head_sq(nvm_queue_t* sq);
```
연속 완료된 슬롯들을 찾아 tail/head를 일괄 이동하는 함수이다.


## 8. GPU 전용 API

### cudaHostRegister (IoMemory)
```c
// include/ctrl.h:203
cudaHostRegister((void*) ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE, cudaHostRegisterIoMemory);
```
NVMe BAR0 MMIO 레지스터를 GPU가 직접 접근할 수 있도록 등록한다. `cudaHostRegisterIoMemory` 플래그가 핵심이다.

### cudaHostGetDevicePointer (도어벨)
```c
// include/queue.h:195
cudaHostGetDevicePointer(&devicePtr, (void*) sq->db, 0);
sq->db = (volatile uint32_t*) devicePtr;
```
호스트 측 도어벨 MMIO 주소를 GPU 디바이스 포인터로 변환한다.

### PTX MMIO 도어벨 쓰기
```c
// include/nvm_util.h
__device__ void write_mmio(volatile uint32_t* addr, uint32_t val) {
    asm volatile("st.mmio.relaxed.sys.global.u32 [%0], %1;" :: "l"(addr), "r"(val));
}
```
최적화된 MMIO 쓰기로 도어벨 레지스터에 직접 쓴다.

### GPU 유틸리티 함수 (nvm_util.h)
```c
__device__ uint32_t lane_id();      // 현재 warp 내 lane ID (0-31)
__device__ uint32_t warp_id();      // 현재 블록 내 warp ID
__device__ uint32_t get_smid();     // 현재 SM(Streaming Multiprocessor) ID
```


## 9. C++ 래퍼 API

### Controller (include/ctrl.h)
```cpp
struct Controller {
    nvm_ctrl_t*  ctrl;       // libnvm 컨트롤러 핸들
    nvm_aq_ref   aq_ref;     // Admin Queue 참조
    nvm_ctrl_info info;      // 컨트롤러 식별 정보
    nvm_ns_info  ns;         // 네임스페이스 정보
    uint16_t     n_qps;      // Queue Pair 수
    QueuePair**  h_qps;      // 호스트 QueuePair 배열
    QueuePair*   d_qps;      // GPU QueuePair 배열
    void*        d_ctrl_ptr; // GPU 복사본 포인터

    Controller(const char* path, uint32_t ns_id, uint32_t cudaDevice,
               uint64_t queueDepth, uint64_t numQueues);
    void reserveQueues(uint16_t numSubs, uint16_t numCpls);
    void print_reset_stats();
    ~Controller();
};
```

### QueuePair (include/queue.h)
```cpp
struct QueuePair {
    nvm_queue_t  sq;             // Submission Queue
    nvm_queue_t  cq;             // Completion Queue
    uint32_t     pageSize;       // 컨트롤러 페이지 크기
    uint32_t     block_size;     // LBA 블록 크기
    uint32_t     nvmNamespace;   // 네임스페이스 ID

    QueuePair(const nvm_ctrl_t* ctrl, uint32_t cudaDevice,
              const nvm_ns_info ns, const nvm_ctrl_info info,
              nvm_aq_ref& aq_ref, uint16_t qp_id, uint64_t queueDepth);
};
```

### page_cache_t (include/page_cache.h)
```cpp
struct page_cache_t {
    page_cache_d_t   pdt;        // GPU 전용 디스크립터
    page_cache_d_t*  d_pc_ptr;   // GPU 복사본 포인터

    page_cache_t(uint64_t ps, uint64_t np, uint32_t cudaDevice,
                 const Controller& ctrl, uint64_t max_range,
                 const std::vector<Controller*>& ctrls);

    template<typename T>
    void add_range(range_t<T>* range);
    void flush_cache();
    ~page_cache_t();
};
```

### range_t<T> (include/page_cache.h)
```cpp
template <typename T>
struct range_t {
    range_d_t<T>  rdt;          // GPU 전용 디스크립터
    range_d_t<T>* d_range_ptr;  // GPU 복사본 포인터

    range_t(uint64_t is, uint64_t count, uint64_t ps, uint64_t pc,
            uint64_t pso, uint64_t p_size, page_cache_t* c_h,
            uint32_t cudaDevice, data_dist_t dist = REPLICATE);
};
```

### array_t<T> / array_d_t<T> (include/page_cache.h)
```cpp
template <typename T>
struct array_d_t {
    // GPU 커널에서 사용하는 디바이스 구조체
    T operator[](const size_t i);              // 읽기 접근 (캐시 경유)
    void operator()(const size_t i, const T val); // 쓰기 접근
};

template <typename T>
struct array_t {
    array_d_t<T>* d_array_ptr;  // GPU 복사본 포인터

    array_t(uint64_t n_elems, uint64_t offset,
            const std::vector<range_t<T>*>& ranges, uint32_t cudaDevice);
};
```

### bam_ptr<T> (include/page_cache.h)
```cpp
template <typename T>
struct bam_ptr {
    bam_ptr(array_d_t<T>* a);
    T operator[](const size_t i) const;  // 읽기 (자동 페이지 관리)
    T& operator[](const size_t i);       // 쓰기 (dirty 플래그 설정)
    ~bam_ptr();                          // 자동 페이지 해제
};
```

### bam_ptr_tlb<T> (include/page_cache.h)
```cpp
template <typename T, size_t n = 32, simt::thread_scope _scope = simt::thread_scope_device>
struct bam_ptr_tlb {
    bam_ptr_tlb(array_d_t<T>* a, tlb<T,n,_scope>* t);
    T operator[](const size_t i) const;  // TLB 경유 읽기
    T& operator[](const size_t i);       // TLB 경유 쓰기
    ~bam_ptr_tlb();
};
```

### DMA/Buffer 유틸리티 (include/buffer.h)
```cpp
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size);                     // 호스트 DMA
DmaPtr createDma(const nvm_ctrl_t* ctrl, size_t size, int cudaDevice);     // GPU DMA
BufferPtr createBuffer(size_t size);                                        // 호스트 핀 메모리
BufferPtr createBuffer(size_t size, int cudaDevice);                        // GPU 메모리

void getDeviceMemory(int device, void*& bufferPtr, void*& devicePtr,
                     size_t size, void*& origPtr);                         // 64KB 정렬 GPU 할당
void getDeviceMemory2(int device, void*& bufferPtr, size_t size,
                      void*& origPtr);                                     // 128B 정렬 GPU 할당
```


## 10. 사용 예제

### 10.1 기본 초기화 및 순차 읽기

```cpp
// 1. Controller 생성 (NVMe 초기화, Admin Queue, I/O Queue 생성)
cuda_err_chk(cudaSetDevice(gpu_id));
std::vector<Controller*> ctrls(n_ctrls);
for (int i = 0; i < n_ctrls; i++)
    ctrls[i] = new Controller("/dev/libnvm0", 1, gpu_id, 1024, 16);

// 2. Page Cache 생성 (GPU 메모리에 소프트웨어 캐시)
page_cache_t h_pc(page_size, n_pages, gpu_id, *ctrls[0], 64, ctrls);

// 3. Range 생성 (데이터 범위 정의)
range_t<uint64_t> h_range(0, n_elems, 0, n_pages_data, 0, page_size,
                          &h_pc, gpu_id, STRIPE);

// 4. Array 생성 (GPU 접근 인터페이스)
std::vector<range_t<uint64_t>*> vr = {&h_range};
array_t<uint64_t> a(n_elems, 0, vr, gpu_id);

// 5. GPU 커널 실행
my_kernel<<<grid, block>>>(a.d_array_ptr, n_elems);
cudaDeviceSynchronize();

// 6. 정리
for (int i = 0; i < n_ctrls; i++) delete ctrls[i];
```

### 10.2 GPU 커널에서의 데이터 접근

```cpp
__global__ void my_kernel(array_d_t<uint64_t>* arr, uint64_t n) {
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // operator[]가 캐시 확인 → 미스 시 NVMe I/O → 데이터 반환
        uint64_t val = (*arr)[tid];
        // ... val 사용 ...
    }
}
```

### 10.3 bam_ptr로 페이지 단위 접근 최적화

```cpp
__global__ void optimized_kernel(array_d_t<uint64_t>* arr, uint64_t n) {
    uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    bam_ptr<uint64_t> ptr(arr);  // 페이지 단위 캐싱

    for (uint64_t i = tid; i < n; i += blockDim.x * gridDim.x) {
        uint64_t val = ptr[i];   // 같은 페이지 내 접근은 SSD I/O 없이 캐시에서
        // ... val 사용 ...
    }
    // ~bam_ptr() 소멸자가 자동으로 페이지 ref count 감소
}
```


## 11. 관련 파일 요약

| 파일 경로 | API 카테고리 |
|-----------|-------------|
| `include/nvm_ctrl.h` | 컨트롤러 초기화/해제 |
| `include/nvm_aq.h` | Admin Queue 생성/파괴 |
| `include/nvm_admin.h` | Admin 커맨드 (Identify, Create Queue 등) |
| `include/nvm_dma.h` | DMA 매핑 (호스트/GPU/DIS) |
| `include/nvm_queue.h` | 표준 큐 연산 (단일 스레드용) |
| `include/nvm_parallel_queue.h` | GPU 병렬 큐 연산 (lock-free) |
| `include/nvm_cmd.h` | NVMe 커맨드 빌드 + PRP |
| `include/nvm_types.h` | 핵심 타입 정의 (nvm_ctrl_t, nvm_dma_t, nvm_queue_t 등) |
| `include/nvm_util.h` | 유틸리티 매크로 + GPU 인트린직 |
| `include/page_cache.h` | 페이지 캐시, range, array, bam_ptr, TLB |
| `include/ctrl.h` | C++ Controller 래퍼 |
| `include/queue.h` | C++ QueuePair 래퍼 |
| `include/buffer.h` | C++ DMA/Buffer 유틸리티 |
| `include/bafs_ptr.h` | bafs_ptr<T> 스마트 포인터 |
| `include/host_util.h` | CUDA 인트린직 호스트 폴백 |
