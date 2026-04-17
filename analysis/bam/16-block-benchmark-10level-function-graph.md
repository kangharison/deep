# BaM benchmarks/block/main.cu — 초기화 & I/O Path 최대 10단계 함수 그래프

> 이 문서는 `sources/bam/benchmarks/block/main.cu`의 초기화 경로와 per-I/O 경로를 **최대 10단계 깊이**로 추적한 분석이다. 각 단계에서 어떤 syscall / ioctl / MMIO / atomic 이 실행되고 어떤 struct 필드가 채워지는지까지 원본 코드를 열어 확인한 내용을 기반으로 한다.
>
> 관련 문서:
> - `01-initialization-flow.md` — 개괄적 초기화 흐름
> - `02-io-read-flow.md` — 개괄적 I/O 경로
> - `04-parallel-queue.md` — 티켓 기반 lock-free 큐
> - `05-dma-mapping.md` — DMA 매핑 메커니즘
> - `12-kernel-module-to-block-benchmark-flow.md` — 커널 모듈 연동
> - `14-cudaHostRegister-mechanism.md` — GPU에서 BAR0 접근 원리

표기 규칙:

- `Ln` = 호출 깊이 n 단계
- `[file:line]` = 근거 소스 위치
- `★` = 특별히 중요한 지점
- 10단계를 넘는 경우는 PCIe 하드웨어 동작으로 전환

---

## 0. 요약: 3개의 축

benchmarks/block/main.cu는 세 가지 경로를 동시에 이해해야 한다.

| 축 | 호스트/디바이스 | 무엇을 하는가 |
|---|---|---|
| **A. 초기화 Path** | Host CPU | NVMe 컨트롤러 탐색, Admin Queue 구축, I/O Queue 생성, GPU에 매핑 |
| **B. page_cache_t 생성** | Host CPU | GPU VRAM에 캐시 버퍼 할당, NVMe P2P 매핑, PRP 리스트 구축 |
| **C. per-I/O Path** | GPU Device | CID 할당 → SQE 구성 → doorbell → CQ poll → 완료 (CPU 미개입) |

---

## A. 초기화 Path — Host CPU 측 (최대 10단계)

### A-1. 진입부

```
L1  main(argc, argv)                              [main.cu:403]
      │
      ├─ Settings::parseArguments                 [settings.h]
      │     getopt_long으로 --queue_depth, --num_queues, --pages,
      │     --threads, --random, --access_type, --n_ctrls, --blk_size 파싱
      │
      ├─ cudaGetDeviceProperties(&prop, cudaDevice)
      ├─ cudaSetDevice(cudaDevice)
      │
      └─ [선택] 입력 파일 mmap                     [main.cu:444-465]
           open(O_RDWR) → fstat → mmap(PROT_R|W, MAP_SHARED) → close(fd)
```

### A-2. 컨트롤러 초기화 — n_ctrls 개 루프

`Controller` 객체 하나는 NVMe 컨트롤러 한 장을 완전히 장악한다. 다음은 그 생성자가 10단계 깊이로 어떤 일을 하는지다.

```
L1  main                                          [main.cu:403]
└─L2 new Controller(path, nsid, cudaDev, QD, nQ) [ctrl.h:401-454]
   │   필드 초기화:
   │     this->ctrl       (nvm_ctrl_t*)          ← nvm_ctrl_init
   │     this->aq_mem     (DmaPtr)               ← createDma (admin용 3 페이지)
   │     this->aq_ref     (Admin Queue ref)      ← nvm_aq_create
   │     this->info/ns    (식별 결과)             ← nvm_admin_*
   │     this->n_sqs/n_cqs                       ← Get Features
   │     this->n_qps      (= min(n_sqs, n_cqs, nQ))
   │     this->d_qps[]    (GPU 측 QueuePair 배열)
   │     this->d_ctrl_ptr (GPU 측 Controller 복사본)
   │
   ├─L3 open("/dev/libnvmX", O_RDWR)             [ctrl.h:406]
   │     → 커널 모듈이 등록한 char device 오픈
   │     → VFS → module/pci.c의 file_operations (open, mmap, ioctl)
   │
   ├─L3 nvm_ctrl_init(&this->ctrl, fd)           [src/linux/device.cpp]
   │  │   fd를 통해 BAR0을 유저 VA로 mmap
   │  │
   │  └─L4 _nvm_ctrl_init(...)                   [src/ctrl.cpp:301-357]
   │     │   nvm_ctrl_t 구조체 할당 + CAP 레지스터 읽기
   │     │
   │     ├─L5 mmap(NULL, NVM_CTRL_MEM_MINSIZE,
   │     │       PROT_READ|PROT_WRITE,
   │     │       MAP_SHARED|MAP_LOCKED, fd, 0)
   │     │     ── BAR0(최소 128KB) 유저 VA 매핑
   │     │     ── MAP_LOCKED: 스왑아웃 방지
   │     │
   │     │  └─L6 sys_mmap (커널)
   │     │     └─L7 module/pci.c의 mmap 핸들러
   │     │        └─L8 io_remap_pfn_range(BAR0 phys → user VA)
   │     │           ── PCIe BAR0의 물리 주소를
   │     │              유저스페이스 가상 주소로 매핑
   │     │           ── 이후 *ptr = x → PCIe MMIO Write TLP
   │     │
   │     ├─L5 CAP$MPSMIN(mm_ptr), CAP$MPSMAX      [regs.h]
   │     │     ── BAR0+0x00의 CAP 레지스터 8B 읽음
   │     │     ── volatile 64-bit load = PCIe Memory Read TLP
   │     │     ── MPSMIN/MAX: 지원하는 host page size 범위
   │     ├─L5 CAP$DSTRD(mm_ptr)
   │     │     ── doorbell stride: 각 doorbell 레지스터 간격 = 4<<DSTRD
   │     ├─L5 CAP$TO(mm_ptr)
   │     │     ── 초기화 timeout (500ms 단위)
   │     ├─L5 CAP$MQES(mm_ptr)
   │     │     ── max queue entries - 1 (0-based)
   │     │
   │     └─  ctrl->page_size / dstrd / timeout / max_qs 세팅
   │
   ├─L3 createDma(this->ctrl, page_size*3, ...)  [ctrl.h:422, src/dma.cpp]
   │  │   Admin Queue 용 3페이지(SQ, CQ, 식별 결과 버퍼)
   │  │
   │  ├─L4 posix_memalign or cudaMallocHost
   │  │
   │  └─L4 _nvm_dma_init                         [src/dma.cpp]
   │     │   DmaPtr = shared_ptr<nvm_dma_t> 구성
   │     │   nvm_dma_t 필드:
   │     │     .vaddr, .ioaddrs[], .n_ioaddrs, .page_size, .contiguous
   │     │
   │     └─L5 dma_map(device callback)           [src/dma.cpp:326-357]
   │        │   ioctl 경로로 커널에 페이지 고정 + 물리주소 획득
   │        │
   │        └─L6 ioctl(fd, NVM_MAP_HOST_MEMORY,
   │                    &{.vaddr_start, .n_pages, .ioaddrs[]})
   │           │   [src/linux/device.cpp:180]
   │           │
   │           └─L7 module/map.c의 ioctl 핸들러
   │              │
   │              ├─L8 get_user_pages_fast(vaddr, n_pages, ...)
   │              │     ── 유저 페이지를 커널에 고정(pin)
   │              │     ── 스왑아웃 방지, 물리 페이지 확보
   │              │
   │              ├─L8 dma_map_page(dev, page, 0, size, DMA_BIDIRECTIONAL)
   │              │     ── IOMMU가 IOVA 발급 (있으면)
   │              │     ── 없으면 물리주소 그대로
   │              │
   │              └─L9 struct pci_dev의 DMA ops
   │                 └─L10 IOMMU driver (intel_iommu/amd_iommu)의
   │                         iova_allocator + page table 엔트리 작성
   │                   ── NVMe 컨트롤러가 접근 가능한 IOVA 반환
   │                   ── ioaddrs[]에 채워 유저스페이스로 복사
   │
   ├─L3 cudaHostRegisterIoMemory(mm_ptr, MINSIZE) [ctrl.h:425]
   │     ★★ BaM 핵심 ★★
   │     ── BAR0(CPU VA)을 GPU의 주소 공간에도 매핑
   │     ── 이후 GPU 커널이 doorbell 레지스터에 직접 MMIO Write 가능
   │     ── 내부적으로 CUDA 드라이버가 IOMMU에 GPU-visible mapping 생성
   │     ── 14-cudaHostRegister-mechanism.md 참조
   │
   ├─L3 initializeController(*this, ns_id)       [ctrl.h:306-339]
   │  │
   │  ├─L4 nvm_aq_create(&aq_ref, ctrl, aq_mem)  [src/rpc.cpp, nvm_aq.h]
   │  │  │   Admin SQ/CQ 할당 + 컨트롤러 초기화 시퀀스
   │  │  │
   │  │  └─L5 nvm_raw_ctrl_reset(ctrl, acq_pa, asq_pa)  [src/ctrl.cpp:393-464]
   │  │     │   NVMe 스펙 7.6.1 준수하는 reset 시퀀스
   │  │     │   (모든 L6 단계는 volatile store/load = PCIe TLP)
   │  │     │
   │  │     ├─L6 CC$EN(mm_ptr) = 0                 ── CC.EN 비트0 클리어 (비활성화)
   │  │     ├─L6 while(CSTS$RDY) spin              ── CSTS.RDY=0 될 때까지 대기
   │  │     ├─L6 AQA = (ACQS<<16) | ASQS           ── 큐 엔트리 수
   │  │     │     ACQS = page/16 - 1 (CQE 16B), ASQS = page/64 - 1 (SQE 64B)
   │  │     ├─L6 ACQ = acq_physical                ── Admin CQ 물리주소
   │  │     ├─L6 ASQ = asq_physical                ── Admin SQ 물리주소
   │  │     ├─L6 atomic_thread_fence(seq_cst)      ── MMIO write 순서 보장
   │  │     ├─L6 CC = MPS|IOSQES|IOCQES|CSS=0|EN=1
   │  │     └─L6 while(!CSTS$RDY) spin             ── 컨트롤러 ready 대기
   │  │
   │  ├─L4 nvm_admin_ctrl_info(aq_ref, &info,
   │  │                        vaddr_offset2, ioaddr2)  [src/admin.cpp:329-380]
   │  │  │   Identify Controller (opcode 0x06, CNS=0x01)
   │  │  │   결과 4KB 구조체: VID/SN/MN/FR/MDTS/SQES/CQES/MAXCMD/NN
   │  │  │
   │  │  └─L5 nvm_raw_rpc(aq_ref, &cmd, &cpl)     [src/rpc.cpp]
   │  │     │   Admin 명령 1회 동기 실행
   │  │     │
   │  │     ├─L6 acquire aq_ref lock (admin 명령 직렬화)
   │  │     ├─L6 execute_command                   [rpc.cpp, inline]
   │  │     │  │
   │  │     │  ├─L7 memcpy cmd → asq->vaddr[tail]  ── SQE 배치 (64B)
   │  │     │  ├─L7 tail = (tail+1) & qs_mask
   │  │     │  ├─L7 *(asq->db) = tail              ── ★ Admin SQ doorbell MMIO
   │  │     │  │                                      (PCIe Memory Write 4B)
   │  │     │  │   └─L8 NVMe 컨트롤러: SQE fetch(Memory Read TLP)
   │  │     │  │       └─L9 opcode=0x06 dispatch, Identify 수행
   │  │     │  │          └─L10 DMA Write로 PRP1 주소에 4KB 결과 기록
   │  │     │  │                 + CQE 작성 + phase bit
   │  │     │  ├─L7 while (cpl.phase != expected) spin ── CQ poll
   │  │     │  ├─L7 copy cpl ← acq->vaddr[head]
   │  │     │  ├─L7 head = (head+1) & qs_mask
   │  │     │  └─L7 *(acq->db) = head              ── Admin CQ doorbell
   │  │     └─L6 release lock
   │  │
   │  ├─L4 nvm_admin_ns_info(aq_ref, &ns, ns_id,…)  [src/admin.cpp:385+]
   │  │     Identify Namespace (opcode 0x06, CNS=0x00, NSID=ns_id)
   │  │     결과: NSZE/NCAP/NUSE/FLBAS/LBAF
   │  │     ★ ns.lba_data_size = 2^(LBAF[FLBAS].LBADS)
   │  │       → blk_size_log 결정. 이후 GPU 커널에서 LBA 계산에 사용
   │  │     └─ 내부적으로 L5 nvm_raw_rpc (위와 동일)
   │  │
   │  └─L4 nvm_admin_get_num_queues(aq_ref, &n_cqs, &n_sqs)
   │        Get Features (opcode 0x0A, FID=0x07)
   │        CPL의 dword[0]: 상위16=NCQR, 하위16=NSQR (모두 0-based)
   │        n_cqs = NCQR+1, n_sqs = NSQR+1
   │        └─ L5 nvm_raw_rpc
   │
   ├─L3 reserveQueues(MAX_QUEUES)                 [ctrl.h:530]
   │     └─L4 Set Features (FID=0x07) — 사용할 큐 수 통보
   │
   └─L3 for q in 1..n_qps:
         new QueuePair(*this, settings, q)        [queue.h:253~]
         │   ── q번째 I/O SQ/CQ 쌍 생성
         │
         ├─L4 createDma(SQ, QD*64B, cudaDev,
         │              P2P=true)                  [src/dma.cpp]
         │  │   ★ SQ/CQ 메모리 자체를 GPU VRAM에 할당
         │  │
         │  ├─L5 cudaMalloc(&gpu_ptr, size)        ── GPU VRAM 확보
         │  ├─L5 nvm_dma_map_device(...)           [src/linux/dma.cpp]
         │  │  │   GPU 페이지를 NVMe가 P2P로 볼 수 있는
         │  │  │   PCIe 버스 주소로 매핑
         │  │  │
         │  │  └─L6 ioctl(fd, NVM_MAP_DEVICE_MEMORY, ...)
         │  │     └─L7 module/pci.c ioctl 핸들러
         │  │        └─L8 nvidia_p2p_get_pages(...)  [NVIDIA 전용 API]
         │  │           │   ── GPU 페이지를 커널이 얻음
         │  │           │   ── p2p_page->physical_address (PCIe BAR1)
         │  │           └─L9 struct nvidia_p2p_page_table 생성
         │  │              └─L10 IOMMU에 P2P mapping 등록
         │  │                 ── NVMe→GPU 직접 DMA 가능 상태
         │  │
         │  └─ DmaPtr 반환: .vaddr(GPU VA), .ioaddrs[] (P2P bus addr)
         │
         ├─L4 createDma(CQ, QD*16B, ...)           ── 동일 경로
         │
         ├─L4 nvm_admin_cq_create(aq_ref, q, cq_mem, ...) [src/admin.cpp]
         │  │   Create I/O CQ (opcode 0x05)
         │  │   CDW10: (QSIZE-1)<<16 | QID
         │  │   CDW11: IV<<16 | IEN<<1 | PC
         │  │   PRP1:  cq_mem->ioaddrs[0] (GPU 물리주소)
         │  └─L5 nvm_raw_rpc ─ L6~L10 위와 동일
         │
         ├─L4 nvm_admin_sq_create(aq_ref, q, sq_mem, cq_id)
         │     Create I/O SQ (opcode 0x01)
         │     CDW10: (QSIZE-1)<<16 | QID
         │     CDW11: CQID<<16 | QPRIO<<1 | PC       ← SQ를 CQ에 바인딩
         │     PRP1:  sq_mem->ioaddrs[0]
         │
         ├─L4 doorbell 오프셋 계산 (NVMe spec 7.10.2)
         │     sq.db_addr = mm_ptr + 0x1000 + (2*q+0) * (4<<dstrd)
         │     cq.db_addr = mm_ptr + 0x1000 + (2*q+1) * (4<<dstrd)
         │
         ├─L4 cudaHostGetDevicePointer(&sq.db_dev, sq.db_addr, 0)
         │     cudaHostGetDevicePointer(&cq.db_dev, cq.db_addr, 0)
         │   ── GPU 커널이 접근 가능한 doorbell 주소로 변환
         │   ── (cudaHostRegisterIoMemory 덕분에 가능)
         │
         └─L4 init_gpu_specific_struct(this)        [queue.h:187-214]
            │   GPU 병렬 큐용 동기화 구조 할당
            │
            ├─L5 createBuffer(sq_tickets, qs*sizeof(padded_struct), GPU)
            │     sq.tickets[pos] — 각 슬롯의 라운드 ID
            ├─L5 createBuffer(sq_tail_mark, qs*…)
            │     sq.tail_mark[pos] — "이 슬롯 준비됨" 플래그
            ├─L5 createBuffer(sq_cid, 65536*…)
            │     sq.cid[id].val — CID 점유 상태 (LOCKED/UNLOCKED)
            ├─L5 createBuffer(cq_head_mark, qs*…)
            │     cq.head_mark[pos] — 완료 처리 플래그
            ├─L5 createBuffer(cq_pos_locks, qs*…)
            │     cq.pos_locks[pos] — CQ 슬롯 2단 스핀락
            │
            └─  qs_minus_1, qs_log2 계산 (bitwise modulo/shift 용)
```

### A-3. 호스트 → GPU 전송

```
L1  main (계속)
└─L2 한 번 더 cudaMemcpy로 h_qps → d_qps[]       [ctrl.h:444]
     QueuePair 한 개당 64B~128B 구조체 GPU 복사
     GPU 커널은 d_ctrl_ptr → d_qps[queue] → db/vaddr 접근
```

---

## B. page_cache_t 생성 — 최대 10단계

```
L1  main                                           [main.cu:516]
└─L2 page_cache_t h_pc(page_size, n_pages,
                       cudaDev, ctrls[0][0],
                       cacheline=64, ctrls)        [page_cache.h:1221]
   │   생성 8단계 (line 1223-1414)
   │
   ├─L3 [1] 원자적 카운터/락 할당                   [page_cache.h:1224-1234]
   │  │     pdt.ctrl_counter  — 컨트롤러 라운드로빈
   │  │     pdt.q_head/tail/lock — double-read prefetch 큐
   │  │     pdt.extra_reads   — 투기적 읽기 카운터
   │  │
   │  └─L4 createBuffer(sizeof(simt::atomic<u64,scope_device>), dev)
   │     └─L5 cudaMalloc(&ptr, size)
   │
   ├─L3 [2] 캐시 파라미터 계산                     [page_cache.h:1236-1245]
   │        page_size_minus_1, page_size_log, n_pages_minus_1
   │        pdt.ctrl_page_size = ctrl.ctrl->page_size
   │        pdt.n_blocks_per_page = ps / blk_size
   │
   ├─L3 [3] 컨트롤러 포인터 배열 GPU 복사            [page_cache.h:1242-1252]
   │  │     for k in 0..n_ctrls:
   │  │       cudaMemcpy(pdt.d_ctrls[k], &ctrls[k]->d_ctrl_ptr,
   │  │                  sizeof(Controller*), HostToDevice)
   │  │     → GPU에서 pc->d_ctrls[k]->d_qps[queue] 접근 경로 확보
   │  │
   │  └─L4 createBuffer + cudaMemcpy
   │
   ├─L3 [4] Range/페이지 메타                     [page_cache.h:1254-1268]
   │        pdt.range_cap = max_range
   │        ranges_buf, ranges_page_starts_buf, ranges_dists_buf 할당
   │
   ├─L3 [5] cache_page_t[n_pages] 할당 및 초기화  [page_cache.h:1278-1296]
   │        tps = new cache_page_t[n_pages]
   │        for i: tps[i].page_take_lock = FREE(=2)
   │        cudaMemcpy → pdt.cache_pages
   │        pdt.page_ticket — 라운드로빈 슬롯 선택 atomic
   │
   ├─L3 [6] ★ GPU VRAM 캐시 버퍼 + P2P 매핑 ★     [page_cache.h:1300-1303]
   │  │
   │  └─L4 createDma(ctrl.ctrl,
   │                 NVM_PAGE_ALIGN(cache_size, 64KB),
   │                 cudaDevice)                  [src/dma.cpp]
   │     │   cache_size = page_size * n_pages
   │     │
   │     ├─L5 cudaMalloc(&gpu_ptr, size)
   │     │     ── 수십 MB~GB의 GPU VRAM 예약
   │     │
   │     └─L5 nvm_dma_map_device(ctrl, gpu_ptr, size)
   │        │   ── 이 VRAM 영역을 NVMe P2P 타겟으로 등록
   │        │
   │        └─L6 ioctl(fd, NVM_MAP_DEVICE_MEMORY, …)
   │           │
   │           └─L7 module/pci.c ioctl 핸들러
   │              │
   │              └─L8 nvidia_p2p_get_pages(
   │                    0, 0, gpu_va, size,
   │                    &page_table, cb, NULL)
   │                  │   ── NVIDIA 커널 API
   │                  │   ── GPU 페이지를 PCIe에서 접근 가능한 형태로 표출
   │                  │
   │                  └─L9 nvidia_p2p_page_table로부터
   │                         physical_address[] 추출
   │                     ── 각 64KB 청크의 PCIe BAR1 주소
   │                     │
   │                     └─L10 IOMMU에 mapping 등록
   │                           ── DMA_BIDIRECTIONAL
   │                           ── NVMe → GPU VRAM DMA Write 경로 완성
   │
   │   반환 DmaPtr:
   │     .vaddr    → GPU VA (base_addr)
   │     .ioaddrs[]→ PCIe bus addr 배열 (PRP용)
   │     .n_ioaddrs, .page_size, .contiguous
   │
   ├─L3 [7] PRP 리스트 구축 (3 분기)               [page_cache.h:1305-1409]
   │  │
   │  │ 분기 A: ps <= ctrl_page_size  (cache page가 작음)
   │  │          [page_cache.h:1312-1338]
   │  │   how_many_in_one = ctrl_page_size / ps
   │  │   for i, j: temp[i*M+j] = ioaddrs[i] + j*ps
   │  │   cudaMemcpy(pdt.prp1, temp, …)
   │  │   prps = false (PRP2 불필요)
   │  │
   │  │ 분기 B: ctrl_ps < ps <= 2*ctrl_ps
   │  │          [page_cache.h:1343-1364]
   │  │   temp1[i] = ioaddrs[2i]       → pdt.prp1
   │  │   temp2[i] = ioaddrs[2i+1]     → pdt.prp2
   │  │   prps = true
   │  │
   │  │ 분기 C: ps > 2*ctrl_ps         [page_cache.h:1368-1409]
   │  │   how_many_in_one = ps / ctrl_ps
   │  │   prp_list_dma = createDma(ps/ctrl_ps * np * 8B)
   │  │                   ── 별도 DMA 버퍼에 PRP list 저장
   │  │   temp1[i] = ioaddrs[i*M]           → pdt.prp1 (첫 페이지)
   │  │   temp2[i] = prp_list_dma.ioaddrs[i] → pdt.prp2 (list 주소)
   │  │   temp3[i*U + j] = ioaddrs[i*M+j+1] → prp_list 내용
   │  │   cudaMemcpy ×3
   │  │   prps = true
   │  │
   │  └─ 모든 분기에서: NVMe 컨트롤러가 PRP1/PRP2를 읽어
   │                    GPU VRAM의 실제 물리 주소를 알게 됨
   │
   └─L3 [8] page_cache_d_t을 GPU로 복사           [page_cache.h:1411-1414]
      │     pdt의 모든 포인터/상수를 디바이스 구조체로 packing
      │
      └─L4 cudaMemcpy(d_pc_ptr, &pdt,
                      sizeof(page_cache_d_t), HostToDevice)
         → 이후 커널은 page_cache_d_t* d_pc로 접근
```

### page_cache_d_t 주요 필드

| 필드 | 의미 |
|---|---|
| `base_addr` | GPU VA, 실 데이터가 로드되는 곳 |
| `prp1[n_pages]`, `prp2[n_pages]` | 각 슬롯별 NVMe DMA 대상 PCIe 주소 |
| `prps` | PRP2 사용 여부 (bool) |
| `cache_pages[n_pages]` | 슬롯 상태 (lock, translation) |
| `page_ticket` | 슬롯 선택용 atomic counter |
| `ctrl_counter` | 컨트롤러 라운드로빈 atomic |
| `d_ctrls[n_ctrls]` | Controller* 배열 (각각 GPU 측 d_qps 보유) |
| `page_size`, `page_size_log`, `n_pages`, `n_ctrls` | 상수 |

---

## C. GPU Per-I/O Path — 최대 10단계

가장 핵심인 **read_data** 경로. L1은 커널 내 호출이므로 진정한 L1은 커널 런치다.

### C-1. 커널 런치와 워프 단위 자원 할당

```
L1  main: random_access_kernel<<<g,b>>>(...)      [main.cu:592]
   │   또는 sequential_access_kernel<<<g,b>>>(...)
   │
   └─L2 스레드별 실행                              [main.cu:308-382]
      │   tid    = blockIdx.x*blockDim.x + threadIdx.x
      │   laneid = lane_id()  (0~31, 인라인 PTX)
      │   smid   = get_smid()
      │
      ├─L3 [laneid==0만] 컨트롤러/큐 선택
      │    ctrl  = pc->ctrl_counter->fetch_add(1, relaxed) % n_ctrls
      │    queue = ctrls[ctrl]->queue_counter.fetch_add(1, relaxed)
      │              % n_qps                 (random kernel의 경우)
      │         or smid % n_qps              (sequential의 경우)
      │
      ├─L3 __shfl_sync(0xFFFFFFFF, ctrl, 0)
      │    __shfl_sync(0xFFFFFFFF, queue, 0)
      │    ── 워프 내 32개 스레드가 동일 SQ/CQ 쌍을 쓰게 만듦
      │       → doorbell 경쟁 최소화, tail_lock 경합 증가 방지
      │
      └─L3 read_data(pc, &d_qps[queue], start_block, n_blocks, tid)
```

### C-2. read_data — 최대 10단계 드릴다운

```
L3 read_data(pc, qp, start_lba, n_blocks, pc_entry)  [page_cache.h:3305-3363]
   │   스택 로컬: nvm_cmd_t cmd (64B)
   │
   ├─L4 cid = get_cid(&qp->sq)                       [nvm_parallel_queue.h:137-181]
   │  │
   │  │  do {
   │  ├─L5   id = sq->cid_ticket.fetch_add(1, relaxed) & 65535
   │  │        ── GPU device-scope atomic (thread_scope_device)
   │  │        ── PTX: atom.relaxed.global.add.u64
   │  ├─L5   old = sq->cid[id].val.fetch_or(LOCKED, acquire)
   │  │        ── PTX: atom.acquire.global.or.u32
   │  │        ── old==0: 획득 성공, old==1: 이미 점유 → 재시도
   │  │  } while (old == LOCKED)
   │  │  return id
   │  │
   │  └─ 결과: 0~65535 범위의 유일 CID, 다른 스레드와 절대 충돌 없음
   │
   ├─L4 nvm_cmd_header(&cmd, cid, NVM_IO_READ=0x02, nsid) [nvm_cmd.h]
   │        cmd.dword[0] = (cid<<16) | (FUSE=0<<8) | opcode
   │        cmd.dword[1] = nsid (네임스페이스 ID)
   │        cmd.dword[2..5] = reserved/meta (0)
   │
   ├─L4 prp1 = pc->prp1[pc_entry]                    [GPU L2 캐시에서 load]
   │    prp2 = pc->prps ? pc->prp2[pc_entry] : 0
   │
   ├─L4 nvm_cmd_data_ptr(&cmd, prp1, prp2)           [nvm_cmd.h:308-334]
   │        cmd.dword[0] &= ~((3<<14)|(3<<8))   (PSDT, FUSE clear)
   │        cmd.dword[6] = prp1 & 0xFFFFFFFF
   │        cmd.dword[7] = prp1 >> 32
   │        cmd.dword[8] = prp2 & 0xFFFFFFFF
   │        cmd.dword[9] = prp2 >> 32
   │
   ├─L4 nvm_cmd_rw_blks(&cmd, start_lba, n_blocks)   [nvm_cmd.h:364-383]
   │        cmd.dword[10] = start_lba & 0xFFFFFFFF
   │        cmd.dword[11] = start_lba >> 32
   │        cmd.dword[12] = (cmd.dword[12] & 0xFFFF0000)
   │                        | ((n_blocks-1) & 0xFFFF)
   │          ★ NVMe NLB는 0-based: 1 블록 읽으려면 0 기록
   │
   ├─L4 sq_pos = sq_enqueue(&qp->sq, &cmd)           [nvm_parallel_queue.h:496-785]
   │  │
   │  ├─L5 ticket = sq->in_ticket.fetch_add(1, relaxed)  (line 519)
   │  │    pos    = ticket & qs_minus_1
   │  │    id     = get_id(ticket, qs_log2)  ── ticket >> qs_log2
   │  │
   │  ├─L5 [관문] 내 차례까지 대기                    (line 552-593)
   │  │    unsigned ns=8
   │  │    while (sq->tickets[pos].val.load(relaxed) != id) {
   │  │      __nanosleep(ns); ns = min(ns*2, 256)
   │  │    }
   │  │    while (sq->tickets[pos].val.load(acquire) != id) {
   │  │      __nanosleep(ns); ns = min(ns*2, 256)
   │  │    }
   │  │    ── 한 바퀴 돌 때까지 해당 슬롯 점유자가 바뀌었는지 확인
   │  │    ── 2-phase (relaxed → acquire)로 fence 비용 최소화
   │  │
   │  ├─L5 [SQE copy] sq->vaddr[pos] ← cmd           (line 623-643)
   │  │    #pragma unroll
   │  │    for (i=0; i<64/sizeof(copy_type); i++)
   │  │      queue_loc[i] = cmd_[i]
   │  │    ── 32B 벡터 store 2회로 64B SQE 한 번에 기록
   │  │    ── sq->vaddr는 GPU VRAM 내 SQ ring (P2P mapping되어 있음)
   │  │
   │  ├─L5 tail_mark[pos].val.store(LOCKED, release)  (line 676)
   │  │    ── release fence: 앞의 SQE copy가 전역에 visible해진 뒤
   │  │       tail_mark가 LOCKED로 관찰되도록 보장
   │  │
   │  ├─L5 [Doorbell phase] cont=true                 (line 698-752)
   │  │  │   while (cont) {
   │  │  │     new_cont = sq->tail_lock.fetch_or(LOCKED, acquire) == LOCKED
   │  │  │     if (!new_cont) {  // tail_lock 획득
   │  │  │
   │  │  ├─L6   cur_tail = sq->tail.load(relaxed)
   │  │  │
   │  │  ├─L6   move_tail(sq, cur_tail)               [nvm_parallel_queue.h:226-269]
   │  │  │  │
   │  │  │  │   count=0; pass=true
   │  │  │  │   while(pass){
   │  │  │  ├─L7   pass = queue not full 체크
   │  │  │  │        ((cur_tail+count+1)&mask) != (head.load(relaxed)&mask)
   │  │  │  ├─L7   if pass:
   │  │  │  │        pass = tail_mark[(cur_tail+count)&mask]
   │  │  │  │                 .val.exchange(UNLOCKED, relaxed) == LOCKED
   │  │  │  │        if pass: count++
   │  │  │  │   }
   │  │  │  │   head_lock.fetch_add(1, acq_rel)
   │  │  │  │   return count
   │  │  │  └─ 연속적으로 준비된 슬롯 수 반환 (batching)
   │  │  │
   │  │  ├─L6   if (tail_move_count) {
   │  │  │        new_tail = cur_tail + tail_move_count
   │  │  │        new_db   = new_tail & qs_minus_1
   │  │  │
   │  │  ├─L6 ★★ *(sq->db) = new_db ★★           (line 741)
   │  │  │  │     ── sq->db: volatile uint32_t*
   │  │  │  │         GPU 주소 공간에 매핑된 NVMe SQ tail doorbell
   │  │  │  │         (BAR0 + 0x1000 + 2*qid * (4<<dstrd))
   │  │  │  │     ── PTX: st.volatile.global.u32 [addr], val
   │  │  │  │
   │  │  │  └─L7 GPU MMU → GPU BAR1 경유 → PCIe Root Complex
   │  │  │     └─L8 PCIe Root Complex가 라우팅
   │  │  │          ── P2P 지원: GPU→NVMe 직접 (CPU Root 통과 X)
   │  │  │          ── IOMMU가 있으면 ATS/translation
   │  │  │        └─L9 PCIe Memory Write TLP 생성
   │  │  │             Header: TLP Type=MWr, length=1 DW(4B),
   │  │  │                     Address=NVMe BAR0 doorbell,
   │  │  │                     Requester=GPU BDF
   │  │  │             Payload: new_db (32-bit)
   │  │  │           └─L10 NVMe 컨트롤러 BAR0 레지스터 업데이트
   │  │  │                  SQ tail 값이 컨트롤러 내부 상태로 진입
   │  │  │                  → 이후 하드웨어 처리 (아래 C-3)
   │  │  │
   │  │  ├─L6   sq->tail.store(new_tail, release)
   │  │  │  }
   │  │  ├─L6   sq->tail_lock.store(UNLOCKED, release)
   │  │  │  }  // 락 획득 못하면 다른 스레드가 처리 중이므로 대기
   │  │  │
   │  │  └─L6 cont = tail_mark[pos].load(relaxed) == LOCKED
   │  │       if (cont) __nanosleep(ns); ns*=2 (≤256)
   │  │       — 내 tail_mark가 UNLOCK될 때까지 = 내 슬롯이 컨트롤러에
   │  │         "알려졌을 때"까지 대기 (다른 스레드의 doorbell write에 편승 가능)
   │  │
   │  └─L5 sq->tickets[pos].val.fetch_add(1, acq_rel)  (line 782)
   │       ── 다음 라운드(ticket이 qs만큼 증가한)의 스레드를 풀어줌
   │       return pos
   │
   ├─L4 (SQE submission 후 NVMe 하드웨어 동시 처리 — C-3 섹션 참조)
   │
   ├─L4 cq_pos = cq_poll(&qp->cq, cid, &head, &loc_)  [nvm_parallel_queue.h:897-973]
   │  │
   │  │   while(true) {
   │  ├─L5   head = cq->head.load(relaxed)
   │  │
   │  ├─L5   for i=0..qs_minus_1:
   │  │        cur_head = head + i
   │  │        search_phase = (~(cur_head >> qs_log2)) & 1
   │  │        loc = cur_head & qs_minus_1
   │  │
   │  ├─L6     cpl = ((nvm_cpl_t*)cq->vaddr)[loc].dword[3]
   │  │        ── GPU VRAM 내 CQ ring, NVMe가 DMA로 덮어씀
   │  │        ── volatile load: GPU L2를 bypass해야 최신 값 보임
   │  │           (CUDA: __ldcv() 또는 volatile 포인터)
   │  │        cid_got = cpl & 0xFFFF
   │  │        phase   = (cpl >> 16) & 1
   │  │
   │  ├─L6     if cid_got==search_cid && phase==search_phase:
   │  │          return loc                 ── 내 명령 완료!
   │  │
   │  ├─L6     if phase != search_phase: break  ── 이 너머는 stale
   │  │
   │  └─L5   __nanosleep(ns); ns=min(ns*2, 256)
   │       }
   │
   │   ★ phase bit 메커니즘:
   │     - 컨트롤러가 CQ 한 바퀴 돌 때마다 phase 토글 (0↔1)
   │     - 소프트웨어는 예상 phase와 비교해 "새 CQE인지" 판정
   │     - 덕분에 head/tail 외에 별도 "valid" 비트 불필요
   │
   ├─L4 cq_dequeue(&qp->cq, cq_pos, &qp->sq, head, loc_)  [nvm_parallel_queue.h:998-1181]
   │  │
   │  ├─L5 cq->tail.fetch_add(1, acq_rel)              (line 1006)
   │  │    ── 논리적 "내가 이 CQE를 처리 중" 표시
   │  │
   │  ├─L5 pos_locks[pos] 2단 스핀락 획득                (line 1019-1046)
   │  │    while (pos_locks[pos].load(relaxed) != 0) __nanosleep
   │  │    while (pos_locks[pos].fetch_or(1, acquire) != 0) __nanosleep
   │  │    ── 같은 pos를 처리하려는 다른 스레드와 배타적 처리
   │  │
   │  ├─L5 cq->head_mark[pos].store(LOCKED, release)   (line 1054)
   │  │
   │  ├─L5 [CQ doorbell phase]                          (line 1079-1105)
   │  │    while(cont) {
   │  │      new_cont = cq->head_lock.fetch_or(LOCKED, acquire) == LOCKED
   │  │      if (!new_cont) {
   │  │
   │  │  ├─L6  cur_head = cq->head.load(relaxed)
   │  │  │
   │  │  ├─L6  move_head_cq(cq, cur_head, sq)
   │  │  │     ── head_mark 배열 스캔, 연속 처리된 CQE 수 반환
   │  │  │     ── 내부에서 sq의 cid도 해제 알림 전달
   │  │  │
   │  │  ├─L6  if (head_move_count) {
   │  │  │       new_head = cur_head + head_move_count
   │  │  │       new_db   = new_head & qs_minus_1
   │  │  │
   │  │  ├─L6★★ *(cq->db) = new_db ★★              (line 1092)
   │  │  │  │    ── NVMe CQ head doorbell MMIO Write
   │  │  │  │       (BAR0 + 0x1000 + (2*qid+1)*(4<<dstrd))
   │  │  │  │    ── 컨트롤러에 "여기까지 CQE 소비했음" 통지
   │  │  │  │    ── 컨트롤러는 이 이전 CQE 슬롯을 재사용 가능
   │  │  │  │
   │  │  │  └─L7~L10: SQ doorbell과 동일한 PCIe MWr TLP 경로
   │  │  │
   │  │  └─L6  cq->head.store(new_head, release) }
   │  │       cq->head_lock.store(UNLOCKED, release)
   │  │    }
   │  │
   │  ├─L5 wait: head가 내 loc_를 지나갈 때까지          (line 1141-1173)
   │  │    ── wrap-around 안전 비교 (new_head 크기 비교 분기)
   │  │
   │  └─L5 cq->pos_locks[pos].store(0, release)         (line 1180)
   │
   └─L4 put_cid(&qp->sq, cid)                          [nvm_parallel_queue.h:195-204]
          sq->cid[id].val.store(UNLOCKED, release)
          ── 이제 이 CID를 다른 스레드가 get_cid로 획득 가능
```

### C-3. SQ doorbell 이후 NVMe 컨트롤러 측 (L8~L10, 하드웨어)

```
L8  NVMe 컨트롤러: SQ tail doorbell 수신
    ── 내부 FIFO에 "큐 qid의 tail이 new_tail로 전진" 이벤트 추가
   │
   ├─L9 컨트롤러 DMA 엔진: SQE fetch
   │    PCIe MRd TLP 발행 (Requester=NVMe BDF, Addr=SQ vaddr[old_tail])
   │    ── SQ가 GPU VRAM에 있으므로 이것도 P2P Read
   │    ── TLP payload 64B = 1 SQE
   │    ── 이 TLP는 PCIe switch/RC를 거쳐 GPU로
   │   │
   │   └─L10 GPU가 MRd에 응답 (Cpl with Data TLP, 64B payload)
   │        ── 컨트롤러 내부 command slot에 SQE 적재
   │        ── opcode=0x02 (READ) 디코드
   │
   ├─L9 NAND 또는 내부 DRAM 캐시에서 데이터 읽기
   │    ── 캐시 히트: 마이크로초 미만
   │    ── 캐시 미스: 수십~수백 μs (TLC/QLC NAND access)
   │
   ├─L9 PCIe MWr TLP 발행 (P2P DMA Write)
   │    Requester=NVMe BDF, Addr=PRP1/PRP2 (GPU P2P 주소)
   │    payload=블록 데이터 (req_size 바이트, 통상 4KB)
   │    ── n_blocks > PRP1 크기면 PRP2가 가리키는 list 참조해 split
   │   │
   │   └─L10 GPU MMU가 IOMMU translation 해석 → GPU VRAM write
   │        ── pages_dma 영역(cache base_addr + pc_entry*page_size)
   │        ── GPU L2는 stale (volatile load가 필요한 이유)
   │
   └─L9 CQE 16B 작성
        dword[0..2] = command-specific 결과, reserved
        dword[3]    = status<<17 | phase<<16 | cid
        PCIe MWr TLP → cq->vaddr[head+n] (GPU VRAM)
        ── 이 TLP 완료되면 GPU 커널의 cq_poll이 볼 수 있음
```

### C-4. 페이지 캐시 연동 지점

```
실제 block 벤치마크에서는 read_data를 직접 호출하지만,
bam::array<T> 또는 range_d_t 접근 시엔:

   operator[] (page_cache.h:3xxx)
   ├─ acquire_page(block_addr)
   │    ├─ state = cache_pages[slot].state.fetch_or(BUSY, acquire)
   │    ├─ if state & VALID: return base_addr + slot*page_size
   │    ├─ if state & BUSY: spin until VALID
   │    └─ else (INVALID):
   │        read_data(pc, qp, start_block, n_blocks, slot) ← 여기서 호출
   │        cache_pages[slot].state.fetch_xor(
   │          BUSY | VALID, release)  ── BUSY→0, 0→VALID
   │
   └─ release_page: reference count 감소
```

block 벤치마크는 이 경로를 거치지 않고 **모든 스레드가 무조건 read_data를 호출**하므로, 순수한 **NVMe 경로 상한선** 측정에 적합하다.

---

## D. 원자연산/메모리 순서 요약

| 연산 | 쓰임 | 파일:라인 | 의미 |
|---|---|---|---|
| `fetch_add(1, relaxed)` | ticket 발급 | nvm_parallel_queue.h:155,519 | 순서 fence 없는 증가 |
| `fetch_or(LOCKED, acquire)` | CID/tail_lock 획득 | :166,716,1035,1070 | 이후 읽기가 앞당겨지지 않음 |
| `fetch_xor(BUSY\|VALID, acq_rel)` | cache 상태 토글 | page_cache.h | BUSY 해제 + VALID 세팅 동시 |
| `exchange(UNLOCKED, relaxed)` | move_tail 슬롯 클리어 | :261 | old 반환 + 새 값 쓰기 |
| `store(x, release)` | 락 해제, mark 해제 | :203,676,752,1054,1092,1180 | 앞선 쓰기가 visible해진 뒤 observable |
| `load(acquire)` | 락 관문 통과 후 load | :582,1073 | 이후 읽기 순서 보장 |

---

## E. 10단계 단일 체인 (가장 깊은 경로)

```
L1  main                                               [main.cu:403]
L2   └─ random_access_kernel<<<>>>                     [main.cu:592]
L3      └─ read_data                                   [page_cache.h:3305]
L4         └─ sq_enqueue                               [nvm_parallel_queue.h:496]
L5            └─ doorbell write *(sq->db)=new_db       [line 741]
L6               └─ GPU MMU/BAR1 → PCIe Root Complex
L7                  └─ PCIe switch 라우팅 (P2P)
L8                     └─ NVMe BAR0 doorbell 레지스터 업데이트
L9                        └─ NVMe 컨트롤러 DMA 엔진: PCIe MRd로 SQE fetch
L10                          └─ NAND/DRAM read → PCIe MWr로 GPU VRAM에 P2P write
```

CPU는 `cudaDeviceSynchronize()`에서 대기만 하고, 실제 I/O 경로에는 **한 번도 관여하지 않는다** — 이것이 BaM의 핵심이며, IOPS 상한이 "GPU→NVMe doorbell + SQE fetch + NAND + CQE" 레이턴시의 역수로 결정되는 이유다.

---

## F. 핵심 포인트 정리

1. **BAR0 매핑의 이중 경로**: mmap()으로 CPU VA → `cudaHostRegisterIoMemory()`로 GPU VA. 두 세계에서 동시에 NVMe doorbell 접근 가능.
2. **GPU VRAM이 I/O 버퍼**: SQ/CQ/데이터 버퍼 모두 `nvidia_p2p_get_pages()`로 PCIe bus 주소 확보 → NVMe가 직접 DMA 대상으로 사용.
3. **Lock-free 큐의 핵심은 ticket + phase**:
   - SQ: ticket이 wrap-around 안전한 순서를 보장 (`tickets[pos]` 라운드 ID 매칭)
   - CQ: phase bit이 "이번 바퀴에서 컨트롤러가 방금 쓴 것"을 구분
4. **Doorbell batching**: tail_lock을 잡은 한 스레드가 `move_tail`로 여러 슬롯을 훑어 하나의 doorbell write에 묶음 제출 → 수천 스레드의 doorbell 폭주 완화.
5. **CPU는 주도권 포기**: 초기화가 끝나면 CPU는 `cudaDeviceSynchronize()`에서만 활동. 모든 per-I/O 제어는 GPU가 직접 수행.

## G. 관련 읽기

- `14-cudaHostRegister-mechanism.md` — BAR0 GPU 매핑 상세
- `04-parallel-queue.md` — ticket 기반 큐 심층 분석
- `05-dma-mapping.md` — `nvidia_p2p_get_pages` 경로
- `06-kernel-module.md` — `/dev/libnvmX` 커널 모듈 내부
- `13-data-structure-relationships.md` — 각 struct 간 포인터 관계
