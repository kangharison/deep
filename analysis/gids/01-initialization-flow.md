# GIDS 초기화 플로우 상세 분석

## 1. Python → C++ 초기화 체인

```
homogenous_train.py :: track_acc_GIDS()
│
├── GIDS_Loader = GIDS.GIDS(              ← GIDS_Setup/GIDS/GIDS.py:192
│       page_size=8,                       페이지 크기 (× 512B = 4KB)
│       off=0,                             SSD 내 feature 시작 오프셋
│       cache_dim=1024,                    feature 벡터 차원
│       num_ele=300*1000*1000*1024,        전체 요소 수 (논리 주소 공간)
│       num_ssd=4,                         SSD 개수
│       ssd_list=[0,1,2,3],               SSD 인덱스 리스트
│       cache_size=8,                      GPU 캐시 크기 (GB)
│       ctrl_idx=0,                        CUDA 디바이스 인덱스
│       window_buffer=False,               Window Buffering 플래그
│       wb_size=8,                         Window Buffer 크기 (GB)
│       accumulator_flag=False,            Storage Accumulator 플래그
│       long_type=False,                   float(False) vs int64_t(True)
│       heterograph=False,                 이종 그래프 플래그
│       heterograph_map=None               노드 타입별 오프셋 맵
│   )
│
└── GIDS.__init__() 내부:
    │
    ├─[1] BAM_Feature_Store 인스턴스 생성
    │     if long_type:
    │       BAM_FS = BAM_Feature_Store.BAM_Feature_Store_long()
    │     else:
    │       BAM_FS = BAM_Feature_Store.BAM_Feature_Store_float()
    │
    │     → gids_nvme.cu에서 PyBind11으로 바인딩된 C++ 객체
    │     → 이 시점에서는 빈 객체 (컨트롤러 미초기화)
    │
    ├─[2] GIDS_Controllers 생성
    │     GIDS_controller = BAM_Feature_Store.GIDS_Controllers()
    │
    │     → bam_nvme.h:27 구조체:
    │       ctrls_paths[6] = {"/dev/libnvm0", ..., "/dev/libnvm5"}
    │       ctrls = std::vector<Controller*>()   (빈 벡터)
    │       n_ctrls = 1
    │       queueDepth = 1024
    │       numQueues = 128
    │       cudaDevice = 0
    │       nvmNamespace = 1
    │
    ├─[3] NVMe 컨트롤러 초기화
    │     GIDS_controller.init_GIDS_controllers(
    │         num_ssd, 1024, 128, ssd_list)
    │
    │     → gids_nvme.cu :: init_GIDS_controllers():
    │       for i in ssd_list:
    │         ctrls.push_back(
    │           new Controller(ctrls_paths[i],
    │                          nvmNamespace,
    │                          cudaDevice,
    │                          queueDepth,
    │                          numQueues))
    │
    │     각 Controller 내부 (BaM ctrl.h):
    │     ┌──────────────────────────────────────────────────┐
    │     │ Controller("/dev/libnvm0", 1, 0, 1024, 128)      │
    │     │ │                                                 │
    │     │ ├── fd = open("/dev/libnvm0", O_RDWR)             │
    │     │ │   → BaM 커널 모듈의 char device                │
    │     │ │                                                 │
    │     │ ├── nvm_ctrl_init(&ctrl, fd)                      │
    │     │ │   → mmap(fd) → BAR0 레지스터 매핑              │
    │     │ │   → CAP, CC, CSTS 등 NVMe 레지스터 읽기        │
    │     │ │   → nvm_ctrl_t { mm_ptr, page_size, dstrd,     │
    │     │ │                   timeout, max_qs } 반환        │
    │     │ │                                                 │
    │     │ ├── cudaHostRegister(ctrl->mm_ptr,                │
    │     │ │       NVM_CTRL_MEM_MINSIZE,                     │
    │     │ │       cudaHostRegisterIoMemory)                  │
    │     │ │   → BAR0를 GPU에서 접근 가능하게 등록           │
    │     │ │                                                 │
    │     │ ├── Admin Queue 생성                              │
    │     │ │   ├── nvm_dma_map_host(&aq_mem) → ioctl()      │
    │     │ │   ├── nvm_aq_create(&aq_ref, ctrl, aq_mem)     │
    │     │ │   └── nvm_raw_ctrl_reset()                      │
    │     │ │       CC.EN=0 → AQA/ASQ/ACQ 설정 → CC.EN=1    │
    │     │ │                                                 │
    │     │ ├── nvm_admin_ctrl_info() → 컨트롤러 정보        │
    │     │ │   nvm_admin_ns_info()  → 네임스페이스 정보     │
    │     │ │   → block_size, block_size_log 계산             │
    │     │ │                                                 │
    │     │ ├── reserveQueues(MAX_QUEUES, MAX_QUEUES)         │
    │     │ │   → Admin 명령으로 I/O 큐 개수 요청            │
    │     │ │   → n_qps = min(n_sqs, n_cqs, numQueues)       │
    │     │ │                                                 │
    │     │ └── for i = 1 to n_qps (128개):                  │
    │     │     h_qps[i] = new QueuePair(ctrl, cudaDev,       │
    │     │                    ns, info, aq_ref, i, 1024)     │
    │     │     │                                             │
    │     │     ├── sq_mem = createDma(ctrl, size, cudaDev)   │
    │     │     │   → cudaMalloc() + ioctl(MAP_DEVICE_MEMORY) │
    │     │     │   → GPU VRAM에 SQ 할당, DMA 주소 획득      │
    │     │     │                                             │
    │     │     ├── cq_mem = createDma(ctrl, size, cudaDev)   │
    │     │     │   → GPU VRAM에 CQ 할당                     │
    │     │     │                                             │
    │     │     ├── nvm_admin_cq_create() → SSD에 CQ 등록    │
    │     │     ├── cudaHostGetDevicePointer(cq.db)           │
    │     │     │   → CQ Doorbell GPU 포인터                  │
    │     │     │                                             │
    │     │     ├── nvm_admin_sq_create() → SSD에 SQ 등록    │
    │     │     ├── cudaHostGetDevicePointer(sq.db)           │
    │     │     │   → SQ Doorbell GPU 포인터                  │
    │     │     │                                             │
    │     │     └── init_gpu_specific_struct()                │
    │     │         → tickets[], tail_mark[], head_mark[]     │
    │     │         → cid[], pos_locks[] (GPU 메모리)         │
    │     │                                                   │
    │     │     cudaMemcpy(d_qps + i, h_qps[i],              │
    │     │                sizeof(QueuePair), H2D)            │
    │     └──────────────────────────────────────────────────┘
    │
    ├─[4] BAM_Feature_Store 컨트롤러 초기화
    │     BAM_FS.init_controllers(
    │         GIDS_controller,     ← 방금 초기화한 컨트롤러들
    │         page_size,           ← 8 (= 4KB)
    │         off,                 ← 0 (읽기 오프셋)
    │         cache_size,          ← 8 (GB)
    │         num_ele,             ← 논리 주소 공간 크기
    │         num_ssd              ← SSD 개수
    │     )
    │
    │     → gids_nvme.cu :: init_controllers():
    │
    │     ┌── page_cache_t 생성 ──────────────────────────────┐
    │     │ n_pages = cache_size * 1024³ / (pageSize * 512)    │
    │     │ 예: 8GB / 4096B = 2,097,152 페이지                 │
    │     │                                                    │
    │     │ h_pc = new page_cache_t(                           │
    │     │   pageSize,          4096 (바이트)                  │
    │     │   n_pages,           2M 페이지                     │
    │     │   cudaDevice,        0                             │
    │     │   ctrls[0][0],       첫 번째 컨트롤러 참조         │
    │     │   64,                사전 할당 엔트리               │
    │     │   ctrls              전체 컨트롤러 배열             │
    │     │ )                                                  │
    │     │ → GPU VRAM에 n_pages × pageSize 크기의 버퍼 할당   │
    │     │ → data_page_t 메타데이터 배열 할당 (상태 추적)     │
    │     │ → 해시 테이블 (페이지 룩업용) 초기화               │
    │     └────────────────────────────────────────────────────┘
    │
    │     ┌── range_t<float> 생성 ────────────────────────────┐
    │     │ h_range = new range_t<float>(                      │
    │     │   0,                 시작 요소 인덱스               │
    │     │   numElems,          전체 요소 수                   │
    │     │   0,                 시작 페이지                    │
    │     │   n_pages,           페이지 수                     │
    │     │   0,                 읽기 오프셋                    │
    │     │   pageSize,          페이지 크기                    │
    │     │   h_pc,              페이지 캐시 참조               │
    │     │   ctrls[0][0],       컨트롤러 참조                  │
    │     │   STRIPE             데이터 분산 모드               │
    │     │ )                                                  │
    │     │ → 논리 주소 → 물리 SSD/LBA 매핑 테이블 구성        │
    │     │ → STRIPE: page_id % num_ssd = SSD 번호             │
    │     │          page_id / num_ssd = SSD 내 LBA             │
    │     └────────────────────────────────────────────────────┘
    │
    │     ┌── array_t<float> 생성 ────────────────────────────┐
    │     │ vr.push_back(h_range)                              │
    │     │ a = new array_t<float>(numElems, 0, vr, cudaDev)   │
    │     │ → GPU에서 접근 가능한 가상 배열                    │
    │     │ → array[i] 접근 시:                                │
    │     │   1. i → range에서 페이지 번호 계산                │
    │     │   2. page_cache에서 해당 페이지 검색               │
    │     │   3. Miss → NVMe read → 캐시에 로드 → 값 반환     │
    │     └────────────────────────────────────────────────────┘
    │
    │     d_cpu_access 할당 (GPU device 메모리)
    │     → CPU 버퍼 접근 횟수 카운터
    │
    ├─[5] 멤버 변수 설정
    │     self.gids_device = torch.device(f'cuda:{ctrl_idx}')
    │     self.GIDS_time = 0
    │     self.WB_time = 0
    │     self.return_torch_buffer = None
    │     self.window_buffer = []   (window buffering용)
    │     self.wb_init = True
    │
    └─[6] 옵셋 설정 (선택적)
          BAM_FS.set_offsets(in_off, index_off, data_off)
          → 3원소 배열: [input_offset, index_offset, data_offset]
```

## 2. 초기화 후 메모리 레이아웃

```
초기화 완료 후 GPU VRAM 사용 현황:

┌─────────────────────── GPU VRAM ──────────────────────────┐
│                                                           │
│  ┌─ Page Cache (8GB) ─────────────────────────────────┐   │
│  │ data_page_t[0]: state=INVALID, offset=0            │   │
│  │ data_page_t[1]: state=INVALID, offset=4096         │   │
│  │ ...                                                │   │
│  │ data_page_t[2097151]: state=INVALID                │   │
│  │                                                    │   │
│  │ 실제 데이터 버퍼: 2M × 4KB = 8GB                   │   │
│  └────────────────────────────────────────────────────┘   │
│                                                           │
│  ┌─ SQ/CQ × 4 SSDs × 128 QPs ────────────────────────┐   │
│  │ SSD0: QP[1].sq, QP[1].cq, ..., QP[128].sq/cq      │   │
│  │ SSD1: QP[1].sq, QP[1].cq, ..., QP[128].sq/cq      │   │
│  │ SSD2: ...                                           │   │
│  │ SSD3: ...                                           │   │
│  │ 각 QP의 tickets[], tail_mark[], head_mark[] 등     │   │
│  └────────────────────────────────────────────────────┘   │
│                                                           │
│  ┌─ BaM 메타데이터 ──────────────────────────────────┐   │
│  │ range_d_t, array_d_t, page_cache_d_t (디바이스용)  │   │
│  │ 해시 테이블, TLB 엔트리                            │   │
│  └────────────────────────────────────────────────────┘   │
│                                                           │
│  ┌─ d_cpu_access (4 bytes) ──────────────────────────┐   │
│  │ unsigned int: CPU 버퍼 접근 카운터                  │   │
│  └────────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────┘

BAR0 매핑 (각 SSD):
  CPU Virtual → mmap → NVMe BAR0 레지스터
  GPU Pointer → cudaHostGetDevicePointer → Doorbell 접근
```

## 3. 초기화 시 커널 모듈과의 상호작용 요약

```
초기화 중 ioctl 호출 횟수 (SSD 4개, QP 128개 기준):

서비스 ①: mmap (BAR0)
  → 4회 (SSD당 1회)

서비스 ②: ioctl(NVM_MAP_HOST_MEMORY)
  → 4회 (SSD당 Admin Queue 1회)

서비스 ③: ioctl(NVM_MAP_DEVICE_MEMORY)
  → 4 × 128 × 2 = 1,024회
    (SSD당 QP당 SQ 1개 + CQ 1개)
  → + page_cache DMA 매핑 추가

총 커널 모듈 호출: ~1,032회
초기화 후: 커널 모듈 호출 0회 (GPU가 직접 I/O)
```
