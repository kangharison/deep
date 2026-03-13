# BaM 시스템 초기화 흐름 상세 분석

## 개요

BaM(Big accelerator Memory)은 GPU에서 NVMe SSD로 직접 I/O를 수행하는 시스템이다. 커널 NVMe 드라이버를 우회하고, 유저스페이스 라이브러리와 커널 모듈을 통해 GPU 스레드가 NVMe 명령을 직접 SQ(Submission Queue)에 삽입하고 CQ(Completion Queue)를 폴링할 수 있도록 한다. 이 문서에서는 시스템 초기화의 전체 흐름을 코드 레벨로 추적한다.

## 전체 초기화 아키텍처

```
┌─────────────────────────────────────────────────────────────────────┐
│                        사용자 애플리케이션 (main.cu)                 │
│   Controller ctrl("/dev/libnvm0", ns_id, cudaDevice, qDepth, nQ)   │
│   page_cache_t h_pc(page_size, n_pages, cudaDevice, ctrl, ...)     │
│   range_t<T> h_range(...)                                          │
│   array_t<T> a(...)                                                │
└────────────────────────────┬────────────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        ▼                    ▼                    ▼
┌──────────────┐   ┌─────────────────┐   ┌──────────────────┐
│ Controller   │   │ page_cache_t    │   │ range_t / array_t│
│ 초기화       │   │ 캐시 초기화     │   │ 데이터 범위 등록 │
└──────┬───────┘   └────────┬────────┘   └────────┬─────────┘
       │                    │                     │
       ▼                    ▼                     ▼
┌──────────────────────────────────────────────────────────────┐
│              libnvm 유저스페이스 라이브러리                    │
│  nvm_ctrl_init → nvm_aq_create → nvm_admin_* → QueuePair    │
└──────────────────────────┬───────────────────────────────────┘
                           │ ioctl
                           ▼
┌──────────────────────────────────────────────────────────────┐
│                   커널 모듈 (module/pci.c)                    │
│  PCI 프로브 → BAR0 매핑 → DMA 설정 → /dev/libnvm* 생성      │
└──────────────────────────────────────────────────────────────┘
```

## Phase 1: 커널 모듈 로드 및 PCI 디바이스 프로브

### 1.1 모듈 초기화

커널 모듈이 `insmod`으로 로드되면 `libnvm_helper_entry()` 함수가 호출된다.

**파일**: `module/pci.c:314`
```c
static int __init libnvm_helper_entry(void)
{
    list_init(&ctrl_list);      // 컨트롤러 리스트 초기화
    list_init(&host_list);      // 호스트 메모리 매핑 리스트
    list_init(&device_list);    // GPU 메모리 매핑 리스트

    alloc_chrdev_region(&dev_first, 0, max_num_ctrls, DRIVER_NAME);
    dev_class = class_create(THIS_MODULE, DRIVER_NAME);
    pci_register_driver(&driver);   // PCI 드라이버 등록
}
```

함수 호출 체인:
```
libnvm_helper_entry()
  ├── list_init(&ctrl_list)
  ├── list_init(&host_list)
  ├── list_init(&device_list)
  ├── alloc_chrdev_region()         // 캐릭터 디바이스 번호 할당
  ├── class_create()                // 디바이스 클래스 생성
  └── pci_register_driver(&driver)  // PCI 드라이버 등록 → 프로브 시작
```

### 1.2 PCI 디바이스 프로브

PCI 서브시스템이 NVMe 클래스(0x010802)에 해당하는 디바이스를 발견하면 `add_pci_dev()`를 호출한다.

**파일**: `module/pci.c:198`
```c
static int add_pci_dev(struct pci_dev* dev, const struct pci_device_id* id)
{
    ctrl = ctrl_get(&ctrl_list, dev_class, dev, curr_ctrls);  // 컨트롤러 참조 생성
    pci_request_region(dev, 0, DRIVER_NAME);                  // BAR0 영역 요청
    pci_enable_device(dev);                                    // PCI 디바이스 활성화
    ctrl_chrdev_create(ctrl, dev_first, &dev_fops);           // /dev/libnvm* 생성
    pci_set_master(dev);                                       // DMA 버스 마스터 활성화
}
```

함수 호출 체인:
```
add_pci_dev()
  ├── ctrl_get()                    // module/ctrl.c:12 - kmalloc + 리스트 삽입
  │     ├── kmalloc(sizeof(struct ctrl))
  │     ├── snprintf(ctrl->name, "libnvm%d")
  │     └── list_insert(list, &ctrl->list)
  ├── pci_request_region(dev, 0)    // BAR0 리소스 예약
  ├── pci_enable_device(dev)        // PCI 디바이스 활성화
  ├── ctrl_chrdev_create()          // module/ctrl.c:97
  │     ├── cdev_init(&ctrl->cdev, fops)
  │     ├── cdev_add(&ctrl->cdev, ctrl->rdev, 1)
  │     └── device_create(ctrl->cls, NULL, ctrl->rdev, NULL, ctrl->name)
  └── pci_set_master(dev)           // DMA 버스 마스터 설정
```

캐릭터 디바이스의 file_operations:
```c
static const struct file_operations dev_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = map_ioctl,      // DMA 매핑 ioctl 처리
    .mmap           = mmap_registers, // BAR0 레지스터 mmap
};
```

### 1.3 BAR0 레지스터 mmap

유저스페이스에서 `/dev/libnvm0`을 mmap하면 `mmap_registers()`가 호출되어 NVMe 컨트롤러의 BAR0 레지스터를 사용자 공간에 매핑한다.

**파일**: `module/pci.c:66`
```c
static int mmap_registers(struct file* file, struct vm_area_struct* vma)
{
    ctrl = ctrl_find_by_inode(&ctrl_list, file->f_inode);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);   // non-cacheable 매핑
    return vm_iomap_memory(vma, pci_resource_start(ctrl->pdev, 0), ...);
}
```

## Phase 2: 유저스페이스 컨트롤러 초기화

### 2.1 nvm_ctrl_init - 컨트롤러 핸들 생성

**파일**: `src/linux/device.cpp:129`

사용자 애플리케이션이 `open("/dev/libnvm0", O_RDWR)`로 파일 디스크립터를 얻은 후 `nvm_ctrl_init()`을 호출한다.

```cpp
int nvm_ctrl_init(nvm_ctrl_t** ctrl, int filedes)
{
    dev = malloc(sizeof(struct device));
    dev->fd = dup(filedes);             // FD 복제
    fcntl(dev->fd, F_SETFD, O_RDWR);   // 읽기/쓰기 모드 설정

    // NVMe 컨트롤러 레지스터를 mmap
    void* mm_ptr = mmap(NULL, NVM_CTRL_MEM_MINSIZE,
                        PROT_READ|PROT_WRITE,
                        MAP_SHARED|MAP_FILE|MAP_LOCKED,
                        dev->fd, 0);

    _nvm_ctrl_init(ctrl, dev, &ops, DEVICE_TYPE_IOCTL, mm_ptr, mm_size);
}
```

### 2.2 _nvm_ctrl_init - 내부 초기화

**파일**: `src/ctrl.cpp:147`

BAR0 레지스터에서 컨트롤러 속성을 읽어온다.

```cpp
int _nvm_ctrl_init(nvm_ctrl_t** handle, struct device* dev,
                   const struct device_ops* ops, enum device_type type,
                   volatile void* mm_ptr, size_t mm_size)
{
    container = create_handle(dev, ops, type);

    ctrl->mm_ptr = mm_ptr;
    ctrl->mm_size = mm_size;

    // 컨트롤러 레지스터에서 속성 읽기
    ctrl->page_size = page_size;           // 시스템 페이지 크기
    ctrl->dstrd = CAP$DSTRD(mm_ptr);      // 도어벨 스트라이드
    ctrl->timeout = CAP$TO(mm_ptr) * 500; // 타임아웃 (ms)
    ctrl->max_qs = CAP$MQES(mm_ptr) + 1;  // 최대 큐 엔트리 수
}
```

읽어오는 레지스터 필드들 (`src/regs.h`):
| 필드 | 레지스터 오프셋 | 비트 | 설명 |
|------|----------------|------|------|
| CAP.MQES | 0x0000 | [15:0] | Maximum Queue Entries Supported |
| CAP.TO | 0x0000 | [31:24] | Timeout (500ms 단위) |
| CAP.DSTRD | 0x0000 | [35:32] | Doorbell Stride |
| CAP.MPSMIN | 0x0000 | [51:48] | Memory Page Size Minimum |
| CAP.MPSMAX | 0x0000 | [55:52] | Memory Page Size Maximum |

## Phase 3: Admin 큐 생성 및 컨트롤러 리셋

### 3.1 nvm_aq_create - Admin 큐 페어 생성

**파일**: `src/rpc.cpp:461`

```cpp
int nvm_aq_create(nvm_aq_ref* handle, const nvm_ctrl_t* ctrl, const nvm_dma_t* window)
{
    _nvm_ref_get(&ref, ctrl);                              // 참조 카운트 증가
    create_admin(&ref->data, ref->ctrl, window);           // 로컬 admin 디스크립터 생성
    ref->stub = (rpc_stub_t) execute_command;              // RPC 스텁 설정
    nvm_raw_ctrl_reset(ctrl, acq_ioaddr, asq_ioaddr);     // 컨트롤러 리셋
}
```

### 3.2 create_admin - Admin 큐 메모리 설정

**파일**: `src/rpc.cpp:329`

```cpp
static int create_admin(struct local_admin** handle, const struct controller* ctrl,
                        const nvm_dma_t* window)
{
    nvm_dma_remap(&copy, window);                  // DMA 윈도우 복제
    memset(admin->qmem->vaddr, 0, 2 * page_size); // 큐 메모리 클리어

    // ACQ 초기화 (페이지 0)
    nvm_queue_clear(&admin->acq, ctrl, true, 0,
                    page_size/sizeof(nvm_cpl_t), local, vaddr, ioaddrs[0]);

    // ASQ 초기화 (페이지 1)
    nvm_queue_clear(&admin->asq, ctrl, false, 0,
                    page_size/sizeof(nvm_cmd_t), local, vaddr+page_size, ioaddrs[1]);
}
```

### 3.3 nvm_raw_ctrl_reset - 컨트롤러 리셋 시퀀스

**파일**: `src/ctrl.cpp:209`

NVMe 스펙에 따른 컨트롤러 리셋 절차:

```
┌────────────────────────────────────────────────┐
│ 1. CC.EN = 0 (컨트롤러 비활성화)               │
│ 2. CSTS.RDY == 0 될 때까지 대기               │
│ 3. AQA 레지스터에 Admin 큐 크기 설정           │
│ 4. ACQ 레지스터에 Admin CQ 물리 주소 설정      │
│ 5. ASQ 레지스터에 Admin SQ 물리 주소 설정      │
│ 6. CC 레지스터 설정 (MPS, CQES, SQES, EN=1)   │
│ 7. CSTS.RDY == 1 될 때까지 대기               │
└────────────────────────────────────────────────┘
```

```cpp
int nvm_raw_ctrl_reset(const nvm_ctrl_t* ctrl, uint64_t acq_addr, uint64_t asq_addr)
{
    *CC(mm_ptr) = *CC(mm_ptr) & ~1;                    // CC.EN = 0
    while (CSTS$RDY(mm_ptr) != 0) { /* 대기 */ }      // CSTS.RDY → 0

    *AQA(mm_ptr) = AQA$ACQS(cq_max) | AQA$ASQS(sq_max); // Admin 큐 크기
    *ACQ(mm_ptr) = acq_addr;                            // ACQ 물리 주소
    *ASQ(mm_ptr) = asq_addr;                            // ASQ 물리 주소

    *CC(mm_ptr) = CC$IOCQES(cqes) | CC$IOSQES(sqes)   // CC 설정
                | CC$MPS(mps) | CC$CSS(0) | CC$EN(1);

    while (CSTS$RDY(mm_ptr) != 1) { /* 대기 */ }      // CSTS.RDY → 1
}
```

## Phase 4: IDENTIFY 및 큐 예약

### 4.1 Identify Controller

**파일**: `include/ctrl.h:94`의 `initializeController()`

```cpp
static void initializeController(struct Controller& ctrl, uint32_t ns_id)
{
    nvm_aq_create(&ctrl.aq_ref, ctrl.ctrl, ctrl.aq_mem.get());
    nvm_admin_ctrl_info(ctrl.aq_ref, &ctrl.info, buffer, ioaddr);
    nvm_admin_ns_info(ctrl.aq_ref, &ctrl.ns, ns_id, buffer, ioaddr);
    nvm_admin_get_num_queues(ctrl.aq_ref, &ctrl.n_cqs, &ctrl.n_sqs);
}
```

`nvm_admin_ctrl_info()` (`src/admin.cpp:103`)에서 읽어오는 정보:
- NVMe 버전, 페이지 크기, 도어벨 스트라이드
- PCI 벤더 ID, 시리얼 번호, 모델명, 펌웨어
- MDTS(Maximum Data Transfer Size), SQES, CQES
- 최대 아웃스탠딩 명령 수, 최대 네임스페이스 수

`nvm_admin_ns_info()` (`src/admin.cpp:158`)에서 읽어오는 정보:
- 네임스페이스 크기(NSZE), 용량(NCAP), 이용률(NUSE)
- LBA 데이터 크기(LBADS), 메타데이터 크기(MS)

### 4.2 I/O 큐 수 예약

**파일**: `src/admin.cpp:481`

```cpp
int nvm_admin_request_num_queues(nvm_aq_ref ref, uint16_t* n_cqs, uint16_t* n_sqs)
{
    // Set Features 명령 (Feature ID = 0x07: Number of Queues)
    admin_current_num_queues(&command, true, *n_cqs, *n_sqs);
    nvm_raw_rpc(ref, &command, &completion);
    *n_sqs = (completion.dword[0] >> 16) + 1;
    *n_cqs = (completion.dword[0] & 0xffff) + 1;
}
```

## Phase 5: Controller C++ 래퍼 초기화

### 5.1 Controller 생성자

**파일**: `include/ctrl.h:148`

```cpp
Controller::Controller(const char* path, uint32_t ns_id,
                       uint32_t cudaDevice, uint64_t queueDepth, uint64_t numQueues)
{
    int fd = open(path, O_RDWR);
    nvm_ctrl_init(&ctrl, fd);
    aq_mem = createDma(ctrl, ctrl->page_size * 3);  // Admin 큐 DMA 메모리
    initializeController(*this, ns_id);

    // BAR0를 CUDA 디바이스 메모리에 등록 (GPU에서 도어벨 접근용)
    cudaHostRegister((void*) ctrl->mm_ptr, NVM_CTRL_MEM_MINSIZE, cudaHostRegisterIoMemory);

    reserveQueues(MAX_QUEUES, MAX_QUEUES);  // 최대 1024개 큐 요청
    n_qps = min(n_sqs, n_cqs, numQueues);

    // 각 QueuePair 생성 (GPU 메모리에 SQ/CQ 할당)
    for (i = 0; i < n_qps; i++) {
        h_qps[i] = new QueuePair(ctrl, cudaDevice, ns, info, aq_ref, i+1, queueDepth);
        cudaMemcpy(d_qps+i, h_qps[i], sizeof(QueuePair), cudaMemcpyHostToDevice);
    }

    // Controller 구조체를 GPU 메모리에 복사
    d_ctrl_buff = createBuffer(sizeof(Controller), cudaDevice);
    cudaMemcpy(d_ctrl_ptr, this, sizeof(Controller), cudaMemcpyHostToDevice);
}
```

### 5.2 QueuePair 생성

**파일**: `include/queue.h:89`

```
QueuePair 생성 흐름:
┌──────────────────────────────────────────────────┐
│ 1. CAP 레지스터에서 CQR, MQES 읽기              │
│ 2. SQ/CQ 크기 결정 (최대 64KB 이내)             │
│ 3. GPU 메모리에 SQ DMA 할당 (createDma)         │
│ 4. GPU 메모리에 CQ DMA 할당 (createDma)         │
│ 5. Admin 명령으로 CQ 생성 (nvm_admin_cq_create) │
│ 6. CQ 도어벨을 GPU 포인터로 변환                │
│ 7. Admin 명령으로 SQ 생성 (nvm_admin_sq_create) │
│ 8. SQ 도어벨을 GPU 포인터로 변환                │
│ 9. GPU 전용 동기화 구조체 초기화                 │
└──────────────────────────────────────────────────┘
```

QueuePair 생성에서 특히 중요한 것은 도어벨 레지스터를 GPU에서 접근 가능하게 만드는 과정이다:

```cpp
// CQ 도어벨을 GPU 디바이스 포인터로 변환
cudaHostGetDevicePointer(&devicePtr, (void*) this->cq.db, 0);
this->cq.db = (volatile uint32_t*) devicePtr;

// SQ 도어벨을 GPU 디바이스 포인터로 변환
cudaHostGetDevicePointer(&devicePtr, (void*) this->sq.db, 0);
this->sq.db = (volatile uint32_t*) devicePtr;
```

### 5.3 GPU 전용 구조체 초기화

**파일**: `include/queue.h:58`의 `init_gpu_specific_struct()`

```cpp
void init_gpu_specific_struct(const uint32_t cudaDevice) {
    sq_tickets   = createBuffer(sq.qs * sizeof(padded_struct), cudaDevice);
    sq_tail_mark = createBuffer(sq.qs * sizeof(padded_struct), cudaDevice);
    sq_cid       = createBuffer(65536 * sizeof(padded_struct), cudaDevice);
    cq_head_mark = createBuffer(cq.qs * sizeof(padded_struct), cudaDevice);
    cq_pos_locks = createBuffer(cq.qs * sizeof(padded_struct), cudaDevice);

    sq.qs_minus_1 = sq.qs - 1;      // 빠른 모듈로 연산용
    sq.qs_log2    = log2(sq.qs);     // 티켓 ID 계산용
}
```

이 구조체들은 병렬 큐 접근을 위한 lock-free 동기화에 사용된다.

## Phase 6: 페이지 캐시 초기화

### 6.1 page_cache_t 생성자

**파일**: `include/page_cache.h:697`

```cpp
page_cache_t(uint64_t ps, uint64_t np, uint32_t cudaDevice,
             const Controller& ctrl, uint64_t max_range,
             const std::vector<Controller*>& ctrls)
{
    // 캐시 메타데이터 버퍼 할당 (GPU 메모리)
    cache_pages_buf = createBuffer(np * sizeof(cache_page_t), cudaDevice);
    page_ticket_buf = createBuffer(sizeof(padded_struct_pc), cudaDevice);
    ranges_buf = createBuffer(max_range * sizeof(pages_t), cudaDevice);

    // 캐시 데이터 영역 할당 (GPU DMA 메모리)
    pages_dma = createDma(ctrl.ctrl, page_size * n_pages, cudaDevice);
    pdt.base_addr = (uint8_t*) pages_dma.get()->vaddr;

    // PRP 주소 리스트 구성 (캐시 페이지 → NVMe I/O 주소 매핑)
    // 케이스 1: page_size <= ctrl_page_size → prp1만 사용
    // 케이스 2: page_size <= 2*ctrl_page_size → prp1 + prp2
    // 케이스 3: page_size > 2*ctrl_page_size → prp1 + prp_list

    // 모든 캐시 페이지를 FREE 상태로 초기화
    for (i = 0; i < np; i++)
        tps[i].page_take_lock = FREE;

    // page_cache_d_t를 GPU에 복사
    cudaMemcpy(d_pc_ptr, &pdt, sizeof(page_cache_d_t), cudaMemcpyHostToDevice);
}
```

### 6.2 range_t 생성자

**파일**: `include/page_cache.h:985`

```cpp
range_t<T>::range_t(uint64_t is, uint64_t count, uint64_t ps, uint64_t pc,
                    uint64_t pso, uint64_t p_size, page_cache_t* c_h,
                    uint32_t cudaDevice, data_dist_t dist)
{
    rdt.page_size = c_h->pdt.page_size;
    rdt.n_elems_per_page = rdt.page_size / sizeof(T);

    // 각 데이터 페이지 상태를 INVALID로 초기화
    pages_buff = createBuffer(pc * sizeof(data_page_t), cudaDevice);
    for (i = 0; i < pc; i++)
        ts[i].state = INVALID;

    // range를 page_cache에 등록
    c_h->add_range(this);
}
```

## 전체 초기화 시퀀스 다이어그램

```
  App                 libnvm               커널 모듈              NVMe SSD
   │                    │                     │                     │
   │ insmod             │                     │                     │
   │ ──────────────────────────────────────>  │                     │
   │                    │    pci_register     │                     │
   │                    │    ──────────────>  │                     │
   │                    │                     │  PCI 프로브         │
   │                    │                     │  BAR0 요청          │
   │                    │                     │  DMA 마스터 활성화  │
   │                    │                     │  /dev/libnvm0 생성  │
   │                    │                     │                     │
   │ open("/dev/libnvm0")                     │                     │
   │ ──────────────────────────────────────>  │                     │
   │                    │                     │                     │
   │ nvm_ctrl_init(fd)  │                     │                     │
   │ ──────────────>    │                     │                     │
   │                    │  mmap(BAR0)         │                     │
   │                    │  ──────────────────>│                     │
   │                    │  <──── mm_ptr ──────│                     │
   │                    │                     │                     │
   │                    │  CAP/VER 레지스터 읽기                    │
   │ <── ctrl 핸들 ──── │                     │                     │
   │                    │                     │                     │
   │ createDma(admin)   │                     │                     │
   │ ──────────────>    │  ioctl(MAP_HOST)    │                     │
   │                    │  ──────────────────>│  get_user_pages     │
   │                    │                     │  dma_map_page       │
   │ <── aq_mem ─────── │                     │                     │
   │                    │                     │                     │
   │ nvm_aq_create      │                     │                     │
   │ ──────────────>    │                     │                     │
   │                    │  CC.EN=0            │                     │
   │                    │  ─────────────────────────────────────>   │
   │                    │  CSTS.RDY=0 대기    │                     │
   │                    │  AQA/ASQ/ACQ 설정   │                     │
   │                    │  CC.EN=1            │                     │
   │                    │  ─────────────────────────────────────>   │
   │                    │  CSTS.RDY=1 대기    │                     │
   │ <── aq_ref ─────── │                     │                     │
   │                    │                     │                     │
   │ admin_ctrl_info    │                     │                     │
   │ ──────────────>    │  IDENTIFY cmd       │                     │
   │                    │  ─────────────────────────────────────>   │
   │                    │  <─── completion ──────────────────────   │
   │ <── ctrl_info ──── │                     │                     │
   │                    │                     │                     │
   │ admin_ns_info      │  IDENTIFY NS cmd    │                     │
   │ ──────────────>    │  ─────────────────────────────────────>   │
   │ <── ns_info ────── │                     │                     │
   │                    │                     │                     │
   │ reserveQueues      │  SET FEATURES cmd   │                     │
   │ ──────────────>    │  ─────────────────────────────────────>   │
   │ <── n_sqs,n_cqs ── │                     │                     │
   │                    │                     │                     │
   │ new QueuePair[n]   │                     │                     │
   │ ──────────────>    │                     │                     │
   │                    │  cudaMalloc(SQ/CQ)  │                     │
   │                    │  ioctl(MAP_DEVICE)  │                     │
   │                    │  ──────────────────>│ nvidia_p2p_get_pages│
   │                    │                     │ nvidia_p2p_dma_map  │
   │                    │  CREATE CQ cmd      │                     │
   │                    │  ─────────────────────────────────────>   │
   │                    │  CREATE SQ cmd      │                     │
   │                    │  ─────────────────────────────────────>   │
   │                    │  cudaHostGetDevicePointer(doorbell)       │
   │ <── QueuePair ──── │                     │                     │
   │                    │                     │                     │
   │ cudaHostRegister(BAR0, IoMemory)         │                     │
   │ ──────> GPU가 BAR0 도어벨 접근 가능      │                     │
   │                    │                     │                     │
   │ page_cache_t()     │                     │                     │
   │ ──────────────>    │  cudaMalloc(캐시)   │                     │
   │                    │  createDma(캐시DMA) │                     │
   │                    │  PRP 리스트 구성     │                     │
   │ <── page_cache ─── │                     │                     │
   │                    │                     │                     │
   │ range_t()          │                     │                     │
   │ ──────────────>    │  페이지 상태 INVALID 초기화               │
   │ <── range ──────── │                     │                     │
```

## 핵심 데이터 구조 관계도

```
┌─ Controller ─────────────────────────┐
│  nvm_ctrl_t* ctrl                    │
│  nvm_aq_ref  aq_ref                  │
│  DmaPtr      aq_mem                  │
│  nvm_ctrl_info info                  │
│  nvm_ns_info   ns                    │
│  QueuePair** h_qps ──┐              │
│  QueuePair*  d_qps   │  (GPU)       │
│  void* d_ctrl_ptr ───┼─ (GPU copy)  │
└──────────────────────┼──────────────┘
                       │
            ┌──────────▼──────────┐
            │    QueuePair        │
            │  nvm_queue_t sq ────┼──→ GPU 메모리 (SQ 엔트리)
            │  nvm_queue_t cq ────┼──→ GPU 메모리 (CQ 엔트리)
            │  sq.db ─────────────┼──→ BAR0 도어벨 (GPU 매핑)
            │  cq.db ─────────────┼──→ BAR0 도어벨 (GPU 매핑)
            │  padded_struct*     │
            │    tickets/cid/     │
            │    tail_mark/       │
            │    head_mark        │
            └─────────────────────┘

┌─ page_cache_t ──────────────────────┐
│  page_cache_d_t pdt ─────────┐     │
│  DmaPtr pages_dma ───────────┼──→ GPU DMA 메모리 (캐시 데이터)
│  page_cache_d_t* d_pc_ptr    │     │
└──────────────────────────────┼─────┘
                               │
            ┌──────────────────▼──────┐
            │   page_cache_d_t (GPU)  │
            │  base_addr ─────────────┼──→ 캐시 데이터 시작
            │  cache_pages[] ─────────┼──→ 페이지 메타데이터
            │  prp1[] ────────────────┼──→ PRP1 주소 배열
            │  prp2[] ────────────────┼──→ PRP2 주소 배열
            │  ranges[] ──────────────┼──→ range별 페이지 상태
            │  d_ctrls[] ─────────────┼──→ Controller 포인터 배열
            └─────────────────────────┘
```

## 요약

BaM 시스템 초기화는 6단계로 구성된다:
1. **커널 모듈 로드**: PCI NVMe 디바이스 프로브, BAR0 매핑, `/dev/libnvm*` 캐릭터 디바이스 생성
2. **컨트롤러 핸들 생성**: BAR0 mmap, CAP 레지스터에서 컨트롤러 속성 읽기
3. **Admin 큐 생성 및 리셋**: CC.EN 토글, AQA/ASQ/ACQ 레지스터 설정
4. **IDENTIFY 및 큐 예약**: 컨트롤러/네임스페이스 식별, Set Features로 I/O 큐 수 확보
5. **I/O QueuePair 생성**: GPU 메모리에 SQ/CQ 할당, 도어벨을 GPU 포인터로 변환, 병렬 동기화 구조체 초기화
6. **페이지 캐시 초기화**: GPU DMA 메모리 할당, PRP 리스트 구성, 캐시 메타데이터 초기화
