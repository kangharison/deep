# BaM Kernel Module ↔ Block Benchmark 연결 플로우 상세 분석

## 전체 시스템 레이어 구조

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        GPU (CUDA Device)                                │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │  CUDA Kernel: sequential_access_kernel / random_access_kernel    │  │
│  │  (benchmarks/block/main.cu)                                      │  │
│  │                                                                   │  │
│  │  GPU Thread #0 ─┬─ read_data() ─┬─ sq_enqueue() ──► SQ[slot]    │  │
│  │  GPU Thread #1 ─┤               ├─ *sq.db = tail  ──► Doorbell   │  │
│  │  GPU Thread #2 ─┤               ├─ cq_poll()      ◄── CQ[slot]  │  │
│  │  ...            ─┤               └─ cq_dequeue()   ──► CQ DB     │  │
│  │  GPU Thread #N ─┘                                                 │  │
│  └───────────┬──────────────┬──────────────┬────────────────────────┘  │
│              │              │              │                            │
│         SQ vaddr       CQ vaddr      Doorbell DB                       │
│         (GPU mem)      (GPU mem)     (BAR0 mmap)                       │
│              │              │              │                            │
└──────────────┼──────────────┼──────────────┼────────────────────────────┘
               │              │              │
         ══════╪══════════════╪══════════════╪═══  PCIe Bus  ═══════════
               │              │              │
┌──────────────┼──────────────┼──────────────┼────────────────────────────┐
│   NVMe SSD   ▼              ▼              ▼                            │
│         DMA Read       DMA Write     Doorbell Reg                       │
│         (Fetch Cmd)    (Post Cpl)    (Tail Update)                      │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Phase 1: 초기화 플로우 (Host CPU에서 실행)

```
benchmarks/block/main.cu :: main()
│
├─[1] Settings::parseArguments(argc, argv)          ← 커맨드라인 파싱
│     settings.h에서 정의:
│     --gpu, --n_ctrls, --threads, --blk_size,
│     --queue_depth, --num_queues, --random, --access_type 등
│
├─[2] cudaSetDevice(settings.cudaDevice)            ← GPU 선택
│
│   ┌─────────────────────────────────────────────────────────────┐
│   │ [3] Controller 생성 (반복: i = 0 ~ n_ctrls-1)              │
│   │                                                             │
│   │  ctrl.h :: Controller(path, ns, cudaDev, qDepth, nQueues)   │
│   │  │                                                          │
│   │  ├─[3a] fd = open("/dev/libnvm0", O_RDWR)                  │
│   │  │       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~               │
│   │  │       이 fd가 커널 모듈과의 유일한 연결점                 │
│   │  │       /dev/libnvm0 = module/pci.c가 생성한 char device    │
│   │  │                                                          │
│   │  ├─[3b] nvm_ctrl_init(&ctrl, fd)                            │
│   │  │       src/ctrl.cpp                                       │
│   │  │       │                                                  │
│   │  │       ├── mmap(fd, 0, 0x2000, PROT_READ|PROT_WRITE,     │
│   │  │       │        MAP_SHARED|MAP_FILE|MAP_LOCKED)           │
│   │  │       │   ════════════════════════════════════════        │
│   │  │       │   이 mmap이 커널 모듈의 mmap_registers() 호출     │
│   │  │       │   module/pci.c:66-85                             │
│   │  │       │   → vm_iomap_memory()로 BAR0를 userspace에 매핑  │
│   │  │       │   → pgprot_noncached() 설정 (캐시 비활성화)      │
│   │  │       │   ════════════════════════════════════════        │
│   │  │       │                                                  │
│   │  │       ├── BAR0 레지스터에서 컨트롤러 속성 읽기:           │
│   │  │       │   CAP.MQES  → max queue entries                  │
│   │  │       │   CAP.DSTRD → doorbell stride                    │
│   │  │       │   CAP.TO    → timeout                            │
│   │  │       │   CAP.MPSMIN/MAX → page size range               │
│   │  │       │                                                  │
│   │  │       └── nvm_ctrl_t 구조체 반환                         │
│   │  │            { mm_ptr, page_size, dstrd, timeout, max_qs } │
│   │  │                                                          │
│   │  ├─[3c] cudaHostRegister(ctrl->mm_ptr, 0x2000,             │
│   │  │              cudaHostRegisterIoMemory)                    │
│   │  │       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~          │
│   │  │       BAR0 메모리를 GPU에서 접근 가능하도록 등록          │
│   │  │       이후 cudaHostGetDevicePointer()로 GPU 포인터 획득  │
│   │  │                                                          │
│   │  ├─[3d] Admin Queue 생성                                    │
│   │  │       │                                                  │
│   │  │       ├── nvm_dma_map_host(&aq_mem, ctrl, buffer, size)  │
│   │  │       │   src/linux/dma.cpp                              │
│   │  │       │   │                                              │
│   │  │       │   └── ioctl(fd, NVM_MAP_HOST_MEMORY, &request)   │
│   │  │       │       ═══════════════════════════════════════     │
│   │  │       │       커널 모듈 map_ioctl() 진입                  │
│   │  │       │       module/pci.c:89 → map.c:map_userspace()    │
│   │  │       │       │                                          │
│   │  │       │       ├── get_user_pages() : 페이지 핀닝          │
│   │  │       │       ├── dma_map_page()  : DMA 주소 획득        │
│   │  │       │       └── copy_to_user()  : DMA 주소 반환        │
│   │  │       │       ═══════════════════════════════════════     │
│   │  │       │                                                  │
│   │  │       ├── nvm_aq_create(&aq_ref, ctrl, aq_mem)           │
│   │  │       │   src/rpc.cpp                                    │
│   │  │       │   │                                              │
│   │  │       │   ├── nvm_queue_clear() : ACQ/ASQ 초기화         │
│   │  │       │   └── nvm_raw_ctrl_reset()                       │
│   │  │       │       src/ctrl.cpp                               │
│   │  │       │       │                                          │
│   │  │       │       ├── CC.EN = 0        (컨트롤러 비활성화)   │
│   │  │       │       ├── wait CSTS.RDY=0  (비활성화 대기)       │
│   │  │       │       ├── AQA  = (cq_size | sq_size)             │
│   │  │       │       ├── ACQ  = cq_dma_addr                     │
│   │  │       │       ├── ASQ  = sq_dma_addr                     │
│   │  │       │       ├── CC   = MPS|IOSQES|IOCQES|EN=1         │
│   │  │       │       └── wait CSTS.RDY=1  (활성화 대기)         │
│   │  │       │                                                  │
│   │  │       └── nvm_admin_ctrl_info()  : Identify Controller   │
│   │  │           nvm_admin_ns_info()    : Identify Namespace    │
│   │  │           → block_size, ns_size 등 획득                  │
│   │  │                                                          │
│   │  └─[3e] I/O Queue Pair 생성 (반복: qp = 1 ~ numQueues)     │
│   │         queue.h :: QueuePair(ctrl, cudaDev, ns, info,       │
│   │                              aq_ref, qp_id, queueDepth)    │
│   │         │                                                   │
│   │         │  ┌─── 이 부분이 핵심: GPU 메모리에 큐 할당 ───┐   │
│   │         │  │                                             │   │
│   │         ├──┤ sq_mem = createDma(ctrl, sq_size, cudaDev)  │   │
│   │         │  │   내부: cudaMalloc() → ioctl(NVM_MAP_       │   │
│   │         │  │          DEVICE_MEMORY)                      │   │
│   │         │  │   ═══════════════════════════════════        │   │
│   │         │  │   커널 모듈 map_ioctl() 진입                 │   │
│   │         │  │   module/pci.c:132 → map.c:map_device_      │   │
│   │         │  │   memory()                                   │   │
│   │         │  │   │                                          │   │
│   │         │  │   ├── nvidia_p2p_get_pages()                 │   │
│   │         │  │   │   GPU 페이지 테이블 획득 (64KB 단위)     │   │
│   │         │  │   │                                          │   │
│   │         │  │   ├── nvidia_p2p_dma_map_pages(pdev, ...)    │   │
│   │         │  │   │   GPU 메모리의 DMA 주소 획득             │   │
│   │         │  │   │   NVMe 컨트롤러가 직접 접근 가능한 주소  │   │
│   │         │  │   │                                          │   │
│   │         │  │   └── copy_to_user(ioaddrs)                  │   │
│   │         │  │       DMA 주소 배열을 userspace로 반환       │   │
│   │         │  │   ═══════════════════════════════════        │   │
│   │         │  │                                             │   │
│   │         ├──┤ cq_mem = createDma(ctrl, cq_size, cudaDev)  │   │
│   │         │  │   (위와 동일한 과정)                         │   │
│   │         │  └─────────────────────────────────────────────┘   │
│   │         │                                                   │
│   │         ├── nvm_admin_cq_create(aq_ref, &cq, qp_id,        │
│   │         │                       cq_mem, 0, cq_size)         │
│   │         │   Admin 명령으로 CQ 생성 → SSD 펌웨어에 등록      │
│   │         │                                                   │
│   │         ├── cudaHostGetDevicePointer(&devPtr, cq.db, 0)     │
│   │         │   CQ Doorbell의 GPU device pointer 획득           │
│   │         │   cq.db = (volatile uint32_t*)devPtr              │
│   │         │                                                   │
│   │         ├── nvm_admin_sq_create(aq_ref, &sq, &cq, qp_id,   │
│   │         │                       sq_mem, 0, sq_size)         │
│   │         │   Admin 명령으로 SQ 생성 → SSD 펌웨어에 등록      │
│   │         │                                                   │
│   │         ├── cudaHostGetDevicePointer(&devPtr, sq.db, 0)     │
│   │         │   SQ Doorbell의 GPU device pointer 획득           │
│   │         │   sq.db = (volatile uint32_t*)devPtr              │
│   │         │                                                   │
│   │         └── init_gpu_specific_struct(cudaDev)               │
│   │             GPU 메모리에 동기화 구조체 할당:                 │
│   │             sq.tickets[]    - 슬롯 라운드로빈 티켓          │
│   │             sq.tail_mark[]  - 커맨드 준비 완료 표시         │
│   │             sq.head_mark[]  - 슬롯 재사용 가능 표시         │
│   │             sq.cid[]        - Command ID 할당               │
│   │             cq.pos_locks[]  - CQ 위치 잠금                  │
│   │                                                             │
│   │  cudaMemcpy(d_qps + i, h_qps[i], sizeof(QueuePair),        │
│   │             cudaMemcpyHostToDevice)                         │
│   │  → QueuePair 구조체를 GPU 메모리로 복사                     │
│   │                                                             │
│   └─────────────────────────────────────────────────────────────┘
│
├─[4] Page Cache 생성 (선택적, block benchmark에서 사용)
│     page_cache_t h_pc(page_size, n_pages, cudaDev, ctrls[0],
│                       64, ctrls)
│     → GPU 메모리에 데이터 캐싱 버퍼 할당
│
└─[5] CUDA Kernel 런치 → Phase 2로 진행
```

---

## Phase 1 요약: 커널 모듈이 제공하는 3가지 서비스

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Userspace (Host CPU)                              │
│                                                                     │
│  benchmarks/block/main.cu                                           │
│       │          │           │                                      │
│       │ open()   │ mmap()    │ ioctl()                              │
│       ▼          ▼           ▼                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │              /dev/libnvm0  (Character Device)                │    │
│  └───────┬─────────────┬─────────────────┬─────────────────────┘    │
└──────────┼─────────────┼─────────────────┼──────────────────────────┘
           │             │                 │
═══════════╪═════════════╪═════════════════╪══ Kernel Boundary ═══════
           │             │                 │
┌──────────┼─────────────┼─────────────────┼──────────────────────────┐
│          ▼             ▼                 ▼                           │
│  module/pci.c    module/pci.c      module/pci.c                     │
│  add_pci_dev()   mmap_registers()  map_ioctl()                      │
│                                                                     │
│  서비스 ①         서비스 ②           서비스 ③                        │
│  PCI 장치 탐지    BAR0 레지스터      DMA 주소 변환                    │
│  & char dev 생성  userspace 매핑     Host/GPU 메모리                  │
│                                                                     │
│  ┌─────────┐    ┌─────────────┐    ┌──────────────────────────┐     │
│  │ NVMe    │    │ BAR0:       │    │ NVM_MAP_HOST_MEMORY:     │     │
│  │ PCI     │    │ CAP, CC,    │    │  get_user_pages()        │     │
│  │ class   │    │ CSTS, AQA,  │    │  dma_map_page()          │     │
│  │ 0x010802│    │ ASQ, ACQ,   │    │  → returns ioaddrs[]     │     │
│  │         │    │ Doorbells   │    │                          │     │
│  │ 최대 64 │    │             │    │ NVM_MAP_DEVICE_MEMORY:   │     │
│  │ 컨트롤러│    │ noncached   │    │  nvidia_p2p_get_pages()  │     │
│  │         │    │ mapping     │    │  nvidia_p2p_dma_map()    │     │
│  └─────────┘    └─────────────┘    │  → returns ioaddrs[]     │     │
│                                    │                          │     │
│                                    │ NVM_UNMAP_MEMORY:        │     │
│                                    │  cleanup & release       │     │
│                                    └──────────────────────────┘     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Phase 2: I/O 실행 플로우 (GPU에서 직접 실행 — 커널 모듈 개입 없음)

```
main.cu :: main() 계속
│
├─[5] CUDA Kernel 런치
│     if (settings.random)
│       random_access_kernel<<<grid, block>>>(d_ctrls, d_pc, ...)
│     else
│       sequential_access_kernel<<<grid, block>>>(d_ctrls, d_pc, ...)
│
│     ┌═══════════════════════════════════════════════════════════════┐
│     ║              GPU 내부 (수천 개 스레드 병렬 실행)              ║
│     ║                                                              ║
│     ║  ┌──────────────────────────────────────────────────────┐    ║
│     ║  │ 각 GPU Thread의 실행 흐름 (main.cu:98-199)          │    ║
│     ║  │                                                      │    ║
│     ║  │  tid = blockIdx.x * blockDim.x + threadIdx.x         │    ║
│     ║  │                                                      │    ║
│     ║  │  [A] 컨트롤러 & 큐 선택                              │    ║
│     ║  │  │                                                    │    ║
│     ║  │  │  // Warp의 lane 0이 대표로 atomic 연산             │    ║
│     ║  │  │  if (lane_id == 0)                                 │    ║
│     ║  │  │    ctrl_idx = pc->ctrl_counter.fetch_add(1)        │    ║
│     ║  │  │               % num_ctrls                          │    ║
│     ║  │  │  ctrl_idx = __shfl_sync(0xFFFFFFFF, ctrl_idx, 0)  │    ║
│     ║  │  │  //  → 같은 warp의 모든 스레드가 같은 컨트롤러    │    ║
│     ║  │  │                                                    │    ║
│     ║  │  │  queue_idx = smid % ctrls[ctrl_idx]->n_qps        │    ║
│     ║  │  │  qp = &ctrls[ctrl_idx]->d_qps[queue_idx]          │    ║
│     ║  │  │                                                    │    ║
│     ║  │  [B] LBA 계산                                         │    ║
│     ║  │  │                                                    │    ║
│     ║  │  │  Sequential:                                       │    ║
│     ║  │  │    start_lba = (tid * req_size) >> block_size_log  │    ║
│     ║  │  │  Random:                                           │    ║
│     ║  │  │    start_lba = (assignment[tid] * req_size)        │    ║
│     ║  │  │               >> block_size_log                    │    ║
│     ║  │  │  n_blocks = req_size >> block_size_log             │    ║
│     ║  │  │                                                    │    ║
│     ║  │  [C] I/O 요청 루프 (reqs_per_thread 회 반복)          │    ║
│     ║  │  │                                                    │    ║
│     ║  │  │  for (i = 0; i < reqs_per_thread; i++)             │    ║
│     ║  │  │    │                                               │    ║
│     ║  │  │    └── read_data(pc, qp, start_lba, n_blocks, ..) │    ║
│     ║  │  │        (또는 write_data / access_data)             │    ║
│     ║  │  │                                                    │    ║
│     ║  │  │        ┌─────────────────────────────────────┐     │    ║
│     ║  │  │        │ page_cache.h :: read_data() 상세    │     │    ║
│     ║  │  │        │                                     │     │    ║
│     ║  │  │        │ [C1] CID 획득                       │     │    ║
│     ║  │  │        │ │  cid = get_cid(&qp->sq)           │     │    ║
│     ║  │  │        │ │  // atomic ticket으로 고유 CID     │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ [C2] NVMe 커맨드 구성               │     │    ║
│     ║  │  │        │ │  nvm_cmd_header(&cmd, cid,         │     │    ║
│     ║  │  │        │ │    NVM_IO_READ, namespace)         │     │    ║
│     ║  │  │        │ │  nvm_cmd_data_ptr(&cmd, prp1,prp2) │     │    ║
│     ║  │  │        │ │  nvm_cmd_rw_blks(&cmd,             │     │    ║
│     ║  │  │        │ │    start_lba, n_blocks)            │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ │  cmd는 64바이트 NVMe 표준 구조:    │     │    ║
│     ║  │  │        │ │  ┌────────────────────────────┐    │     │    ║
│     ║  │  │        │ │  │ DW0: opcode(READ) + CID    │    │     │    ║
│     ║  │  │        │ │  │ DW1: namespace ID          │    │     │    ║
│     ║  │  │        │ │  │ DW6-7: PRP1 (GPU DMA addr) │    │     │    ║
│     ║  │  │        │ │  │ DW8-9: PRP2 (GPU DMA addr) │    │     │    ║
│     ║  │  │        │ │  │ DW10-11: Starting LBA      │    │     │    ║
│     ║  │  │        │ │  │ DW12: Number of blocks     │    │     │    ║
│     ║  │  │        │ │  └────────────────────────────┘    │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ │  ★ PRP1/PRP2에 들어가는 주소 =     │     │    ║
│     ║  │  │        │ │    Phase 1에서 커널 모듈이 반환한   │     │    ║
│     ║  │  │        │ │    GPU 메모리의 DMA 주소            │     │    ║
│     ║  │  │        │ │    (nvidia_p2p_dma_map_pages 결과) │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ [C3] SQ Enqueue (핵심 동기화)        │     │    ║
│     ║  │  │        │ │  sq_enqueue(&qp->sq, &cmd)         │     │    ║
│     ║  │  │        │ │  nvm_parallel_queue.h:496-758      │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ │  상세 플로우는 아래 Phase 2A 참조  │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ [C4] CQ Poll (완료 대기)             │     │    ║
│     ║  │  │        │ │  cq_poll(&qp->cq, cid)             │     │    ║
│     ║  │  │        │ │  nvm_parallel_queue.h:898-973      │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ │  상세 플로우는 아래 Phase 2B 참조  │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ [C5] CQ Dequeue (완료 처리)          │     │    ║
│     ║  │  │        │ │  cq_dequeue(&qp->cq, cq_pos)       │     │    ║
│     ║  │  │        │ │  nvm_parallel_queue.h:999-1140     │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ │  상세 플로우는 아래 Phase 2C 참조  │     │    ║
│     ║  │  │        │ │                                    │     │    ║
│     ║  │  │        │ [C6] CID 반환                        │     │    ║
│     ║  │  │        │    put_cid(&qp->sq, cid)             │     │    ║
│     ║  │  │        └─────────────────────────────────────┘     │    ║
│     ║  │  │                                                    │    ║
│     ║  │  └── atomicAdd(req_count, 1)  // 완료 카운터 증가     │    ║
│     ║  └──────────────────────────────────────────────────────┘    ║
│     ║                                                              ║
│     ╚══════════════════════════════════════════════════════════════╝
│
├─[6] cudaDeviceSynchronize()     ← 모든 GPU 스레드 완료 대기
│
└─[7] 결과 출력
      elapsed = after - before (microseconds)
      iops = total_ios / (elapsed / 1e6)
      bandwidth = total_data / (elapsed / 1e6) / 1e9
```

---

## Phase 2A: sq_enqueue() 상세 — GPU 스레드가 NVMe 커맨드를 SQ에 넣는 과정

```
nvm_parallel_queue.h :: sq_enqueue(&sq, &cmd)
│
│  수천 개의 GPU 스레드가 동시에 이 함수를 호출한다.
│  Lock-free 티켓 시스템으로 충돌 없이 슬롯을 분배한다.
│
├─[Step 1] 티켓 획득 (Atomic)
│  ticket = sq->in_ticket.fetch_add(1, relaxed)
│  pos = ticket & sq->qs_minus_1          ← 큐 내 위치 (모듈러)
│  id  = ticket >> sq->qs_log2            ← 라운드 번호
│
│  예시: qs=1024, ticket=2050
│        pos = 2050 & 1023 = 2
│        id  = 2050 >> 10  = 2  (두 번째 라운드)
│
├─[Step 2] 내 차례 대기 (Spin)
│  while (sq->tickets[pos] != id):
│    __nanosleep(ns)        ← 지수 백오프: 8ns → 16ns → 32ns → ...
│
│  ┌────────────────────────────────────────────────────┐
│  │  왜 필요한가?                                      │
│  │  같은 pos에 여러 라운드의 스레드가 매핑될 수 있다.  │
│  │  tickets[pos] == id 일 때만 해당 라운드의 스레드가 │
│  │  슬롯을 사용할 수 있다.                            │
│  │                                                    │
│  │  라운드 0: tickets[2]=0 → 스레드 A 진입            │
│  │  라운드 1: tickets[2]=0 → 스레드 B 대기            │
│  │  스레드 A 완료 → tickets[2]=1 → 스레드 B 진입      │
│  └────────────────────────────────────────────────────┘
│
├─[Step 3] 커맨드 복사 (64바이트)
│  queue_loc = sq->vaddr + pos * 64
│  queue_loc[0] = cmd[0]     ← 32바이트 (DW0~DW7)
│  queue_loc[1] = cmd[1]     ← 32바이트 (DW8~DW15)
│
│  ★ sq->vaddr = GPU 메모리 (Phase 1에서 cudaMalloc + DMA 매핑)
│    NVMe SSD가 PCIe P2P로 이 GPU 메모리를 직접 읽을 수 있다
│
├─[Step 4] 준비 완료 표시
│  sq->tail_mark[pos].store(LOCKED, release)
│
├─[Step 5] Tail 이동 & Doorbell 쓰기
│  │
│  ├── head_lock 획득 (atomic CAS spin)
│  │   while (!sq->head_lock.compare_exchange(UNLOCKED, LOCKED)):
│  │     __nanosleep(ns)
│  │
│  ├── move_tail(sq, cur_tail)
│  │   // tail_mark[pos]==LOCKED인 연속 슬롯만큼 tail 전진
│  │   // 예: tail_mark[5]=LOCKED, [6]=LOCKED, [7]=UNLOCKED
│  │   //     → tail을 5→7로 이동 (2개 전진)
│  │
│  ├── ★★★ Doorbell 쓰기 ★★★
│  │   *(sq->db) = new_tail
│  │   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
│  │   sq->db = BAR0의 SQ Tail Doorbell 레지스터
│  │           (Phase 1에서 cudaHostGetDevicePointer로 획득)
│  │
│  │   이 한 줄의 메모리 쓰기가:
│  │   GPU → PCIe Bus → NVMe Controller BAR0 Doorbell Register
│  │   로 전달되어 SSD에 "새 커맨드가 있다"고 알린다
│  │   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
│  │
│  └── head_lock 해제
│
├─[Step 6] 완료 대기 (head_mark 폴링)
│  while (sq->head_mark[pos] == LOCKED):
│    __nanosleep(ns)
│
│  // CQ 처리 스레드가 이 슬롯의 완료를 확인하면
│  // head_mark[pos] = UNLOCKED으로 변경한다
│
└─[Step 7] 티켓 반환
   sq->tickets[pos].fetch_add(1, release)
   // 다음 라운드의 스레드가 이 슬롯을 사용할 수 있게 됨
```

---

## Phase 2B: cq_poll() 상세 — GPU 스레드가 완료를 감지하는 과정

```
nvm_parallel_queue.h :: cq_poll(&cq, search_cid)
│
│  NVMe SSD가 완료 시:
│  SSD → PCIe DMA Write → GPU 메모리(CQ vaddr)에 Completion Entry 기록
│  (커널 모듈이나 CPU 개입 없이 하드웨어가 직접 GPU 메모리에 쓴다)
│
├─[Loop] 완료될 때까지 반복
│  │
│  ├── head = cq->head.load(relaxed)
│  │
│  ├── CQ 엔트리 스캔 (head부터 순회)
│  │   for i in 0..qs_minus_1:
│  │     cur_head = head + i
│  │     loc = cur_head & qs_minus_1
│  │     expected_phase = (~(cur_head >> qs_log2)) & 0x01
│  │
│  │     // CQ 엔트리 읽기 (GPU 메모리에서)
│  │     cpl_entry = cq->vaddr[loc].dword[3]
│  │
│  │     ┌──────────────────────────────────────────┐
│  │     │  CQ Entry DWord 3 구조:                  │
│  │     │  Bits [15:0]  = Command ID (CID)         │
│  │     │  Bit  [16]    = Phase Tag (P)            │
│  │     │  Bits [31:17] = Status Field             │
│  │     └──────────────────────────────────────────┘
│  │
│  │     cid   = cpl_entry & 0xFFFF
│  │     phase = (cpl_entry >> 16) & 0x1
│  │
│  │     if (cid == search_cid && phase == expected_phase):
│  │       return loc    ← 찾았다! 내 커맨드 완료됨
│  │
│  │     if (phase != expected_phase):
│  │       break          ← 여기부터는 아직 유효한 완료 없음
│  │
│  └── __nanosleep(ns)
│      ns *= 2            ← 지수 백오프
│
│  ┌────────────────────────────────────────────────────┐
│  │  Phase Tag 동작 원리:                              │
│  │                                                    │
│  │  CQ가 원형 버퍼이므로 한 바퀴 돌면 같은 위치에     │
│  │  새 엔트리가 덮어써진다. Phase bit으로 구분:        │
│  │                                                    │
│  │  1번째 순회: phase=1 → SSD가 1로 쓴다              │
│  │  2번째 순회: phase=0 → SSD가 0으로 쓴다            │
│  │  3번째 순회: phase=1 → ...                         │
│  │                                                    │
│  │  GPU가 읽은 phase가 expected_phase와 일치하면       │
│  │  이 엔트리는 현재 라운드의 새 데이터이다.           │
│  └────────────────────────────────────────────────────┘
│
└── 반환: (loc, cur_head) → cq_dequeue에 전달
```

---

## Phase 2C: cq_dequeue() 상세 — 완료 처리 및 Doorbell 업데이트

```
nvm_parallel_queue.h :: cq_dequeue(&cq, pos, &sq, loc, cur_head)
│
├─[Step 1] CQ tail 증가
│  cq->tail.fetch_add(1, relaxed)
│
├─[Step 2] pos_lock 획득
│  while (!cq->pos_locks[pos].CAS(UNLOCKED, LOCKED)):
│    __nanosleep(ns)
│
├─[Step 3] SQ head_mark 업데이트
│  sq->head_mark[pos].store(LOCKED, release)
│  // "이 SQ 슬롯의 커맨드가 완료되었다"
│
├─[Step 4] CQ Head 이동 & Doorbell 쓰기
│  │
│  ├── head_lock 획득
│  │
│  ├── move_head_cq(cq, sq, cur_head)
│  │   // head_mark[i]==LOCKED인 연속 슬롯만큼 CQ head 전진
│  │   // 동시에 SQ의 head도 같이 전진시킨다
│  │   // head_mark[i] = UNLOCKED으로 변경 (SQ 슬롯 재사용 허용)
│  │
│  ├── ★★★ CQ Doorbell 쓰기 ★★★
│  │   *(cq->db) = new_head
│  │   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
│  │   GPU → PCIe → NVMe BAR0 CQ Head Doorbell Register
│  │   SSD에게 "여기까지 처리했다"고 알린다
│  │   SSD는 이 head 이후의 CQ 슬롯을 재사용할 수 있다
│  │   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
│  │
│  └── head_lock 해제
│
├─[Step 5] head_mark 완료 대기
│  while (sq->head_mark[pos] == LOCKED):
│    __nanosleep(ns)
│  // move_head_cq()에서 UNLOCKED로 바꿔줄 때까지 대기
│
└─[Step 6] pos_lock 해제
   cq->pos_locks[pos].store(UNLOCKED, release)
```

---

## 전체 데이터 흐름 타임라인 (단일 Read I/O)

```
시간 ──────────────────────────────────────────────────────────────►

GPU Thread              NVMe SSD              Kernel Module
    │                      │                      │
    │  ══ Phase 1 (초기화, CPU에서 실행) ══════════│
    │                      │                      │
    │                      │   open(/dev/libnvm0) │
    │                      │   ──────────────────►│
    │                      │                      │ add_pci_dev()
    │                      │                      │ ctrl_chrdev_create()
    │                      │                      │
    │                      │   mmap(fd, BAR0)     │
    │                      │   ──────────────────►│
    │                      │                      │ mmap_registers()
    │                      │   ◄── BAR0 ptr ──────│ vm_iomap_memory()
    │                      │                      │
    │                      │   ioctl(MAP_DEVICE)  │
    │                      │   ──────────────────►│
    │                      │                      │ map_ioctl()
    │                      │                      │ nvidia_p2p_get_pages()
    │                      │                      │ nvidia_p2p_dma_map()
    │                      │   ◄── DMA addrs[] ──│
    │                      │                      │
    │                      │   Admin: Create CQ   │
    │                      │   ──────────────────►│ (BAR0 직접 접근,
    │                      ◄── CQ created ────────│  모듈 불필요)
    │                      │                      │
    │                      │   Admin: Create SQ   │
    │                      │   ──────────────────►│
    │                      ◄── SQ created ────────│
    │                      │                      │
    │  ══ Phase 2 (I/O, GPU에서 직접 실행) ════════│
    │                      │                      │
    │  ★ 커널 모듈 개입 완전히 없음 ★              │
    │                      │                      │
    │  get_cid()           │                      │
    │  build NVMe cmd      │                      │
    │  (PRP = GPU DMA addr)│                      │
    │  │                   │                      │
    │  sq_enqueue()        │                      │
    │  copy cmd → SQ[pos]  │                      │
    │  │                   │                      │
    │  *sq.db = tail ──────────────────────────►  │
    │  (Doorbell Write)    │  "새 커맨드 있음"    │
    │                      │                      │
    │                      │  SQ[pos] DMA Read    │
    │                      │  ◄── PCIe P2P ──►    │
    │                      │  (GPU 메모리에서      │
    │                      │   커맨드 가져옴)      │
    │                      │                      │
    │                      │  NVMe 커맨드 실행     │
    │                      │  (NAND Flash 읽기)    │
    │                      │                      │
    │                      │  PRP1 주소로 DMA Write│
    │                      │  ──── PCIe P2P ────►  │
    │                      │  (읽은 데이터를        │
    │                      │   GPU 메모리에 직접 씀)│
    │                      │                      │
    │                      │  CQ에 Completion 기록 │
    │                      │  ──── PCIe P2P ────►  │
    │                      │  (GPU 메모리의 CQ에   │
    │  ◄──────────────────────  완료 엔트리 씀)    │
    │                      │                      │
    │  cq_poll() 성공!     │                      │
    │  (phase tag 일치)    │                      │
    │                      │                      │
    │  cq_dequeue()        │                      │
    │  *cq.db = head ──────────────────────────►  │
    │  (Doorbell Write)    │  "여기까지 처리함"   │
    │                      │                      │
    │  put_cid()           │                      │
    │  ── I/O 완료 ──      │                      │
    │                      │                      │
    ▼                      ▼                      ▼
```

---

## 메모리 주소 변환 관계도

```
┌─────────────────────────────────────────────────────────────────────┐
│                        주소 공간 매핑                                │
│                                                                     │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────────┐     │
│  │ GPU Virtual │    │ GPU Physical │    │ PCIe Bus Address    │     │
│  │ Address     │    │ (GPU BAR)    │    │ (DMA Address)       │     │
│  ├─────────────┤    ├──────────────┤    ├─────────────────────┤     │
│  │             │    │              │    │                     │     │
│  │ SQ vaddr ───┼───►│ GPU VRAM  ───┼───►│ ioaddrs[0]          │     │
│  │ (cudaMalloc)│    │ 페이지 0    │    │ (nvidia_p2p_dma_    │     │
│  │             │    │              │    │  map_pages 결과)    │     │
│  │ CQ vaddr ───┼───►│ GPU VRAM  ───┼───►│ ioaddrs[1]          │     │
│  │ (cudaMalloc)│    │ 페이지 1    │    │                     │     │
│  │             │    │              │    │                     │     │
│  │ Data buf ───┼───►│ GPU VRAM  ───┼───►│ ioaddrs[2]          │     │
│  │ (PRP target)│    │ 페이지 2    │    │ (NVMe cmd의 PRP에   │     │
│  │             │    │              │    │  이 주소가 들어감)  │     │
│  └─────────────┘    └──────────────┘    └──────────┬──────────┘     │
│                                                     │               │
│                          NVMe SSD가 이 DMA 주소로    │               │
│                          직접 GPU 메모리를 읽고 쓴다 │               │
│                                                     │               │
│  ┌──────────────────────────────────────────────────┘               │
│  │                                                                  │
│  │  NVMe SSD 입장에서:                                              │
│  │  ┌───────────────────────────────────────────┐                   │
│  │  │ SQ DMA Read:  ioaddrs[0] → 커맨드 가져옴  │                   │
│  │  │ Data Write:   ioaddrs[2] → 데이터 전송     │                   │
│  │  │ CQ DMA Write: ioaddrs[1] → 완료 통보       │                   │
│  │  └───────────────────────────────────────────┘                   │
│  │                                                                  │
│  │  모든 DMA 전송이 GPU ↔ SSD 간 PCIe P2P로 수행                    │
│  │  CPU와 시스템 메모리를 전혀 거치지 않는다                         │
│  │                                                                  │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │                    Doorbell 주소 체인                         │   │
│  │                                                              │   │
│  │  NVMe BAR0 (물리)                                            │   │
│  │    │                                                         │   │
│  │    ├── mmap() → CPU Virtual Address (ctrl->mm_ptr)           │   │
│  │    │             module/pci.c :: mmap_registers()             │   │
│  │    │                                                         │   │
│  │    ├── cudaHostRegister(mm_ptr, cudaHostRegisterIoMemory)    │   │
│  │    │                                                         │   │
│  │    └── cudaHostGetDevicePointer() → GPU Device Pointer       │   │
│  │          sq.db, cq.db에 저장                                 │   │
│  │          GPU 스레드가 이 포인터로 직접 Doorbell에 쓸 수 있다  │   │
│  └──────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 핵심 요약: 커널 모듈의 역할은 "다리 놓기"

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│   Phase 1: 커널 모듈이 하는 일 (초기화 시 1회)                       │
│   ═══════════════════════════════════════════                        │
│                                                                     │
│   ① /dev/libnvm0 생성 → userspace가 접근할 수 있는 인터페이스       │
│   ② BAR0 mmap        → NVMe 레지스터를 userspace에 노출            │
│   ③ Host DMA 매핑     → get_user_pages + dma_map_page               │
│   ④ GPU DMA 매핑      → nvidia_p2p_get_pages + nvidia_p2p_dma_map  │
│                                                                     │
│   이 4가지가 끝나면 커널 모듈의 역할은 완료된다.                     │
│                                                                     │
│   ─────────────────────────────────────────────                     │
│                                                                     │
│   Phase 2: GPU가 직접 하는 일 (런타임, 반복)                        │
│   ═══════════════════════════════════════════                        │
│                                                                     │
│   커널 모듈 개입 = 0                                                │
│   CPU 개입 = 0                                                      │
│   인터럽트 = 0 (Polling 방식)                                       │
│                                                                     │
│   GPU Thread ──► SQ에 cmd 쓰기 (GPU mem)                            │
│              ──► Doorbell 쓰기 (PCIe BAR0)                          │
│              ◄── CQ 폴링 (GPU mem)                                  │
│              ──► CQ Doorbell 쓰기 (PCIe BAR0)                       │
│                                                                     │
│   NVMe SSD  ──► SQ에서 cmd 읽기 (PCIe P2P DMA → GPU mem)           │
│              ──► 데이터 전송 (PCIe P2P DMA → GPU mem)               │
│              ──► CQ에 완료 쓰기 (PCIe P2P DMA → GPU mem)            │
│                                                                     │
│   ★ 모든 데이터 경로: GPU ←─ PCIe P2P ─→ NVMe SSD                  │
│     CPU/System RAM을 완전히 우회                                    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```
