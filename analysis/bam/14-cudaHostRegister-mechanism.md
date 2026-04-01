# cudaHostRegister() 동작 메커니즘 — GPU가 CPU 메모리에 접근하는 원리

## 1. 왜 필요한가

```
문제: GPU(CUDA 커널)는 기본적으로 자기 VRAM만 접근할 수 있다.
      CPU의 시스템 메모리(DRAM)나 다른 PCIe 장치의 BAR 영역은
      GPU 입장에서 "보이지 않는" 주소 공간이다.

BaM에서의 구체적 상황:
  NVMe BAR0 = SSD 컨트롤러의 레지스터 (Doorbell 포함)
  → CPU는 mmap()으로 접근 가능 (ctrl->mm_ptr)
  → GPU 스레드가 *(sq.db) = tail 로 Doorbell에 쓰려면?
  → GPU가 이 BAR0 주소에 접근할 수 있어야 한다
  → cudaHostRegister(mm_ptr, size, cudaHostRegisterIoMemory)
```

## 2. 기본 원리: 주소 공간 매핑

```
CPU와 GPU는 각각 독립적인 가상 주소 공간과 MMU를 가진다.
하지만 둘 다 같은 PCIe 버스에 연결되어 있다.

┌──────────────┐           PCIe Bus           ┌──────────────┐
│     CPU      │ ◄══════════════════════════► │     GPU      │
│              │                              │              │
│  MMU         │                              │  GPU MMU     │
│  ┌────────┐  │                              │  ┌────────┐  │
│  │ Page   │  │                              │  │ Page   │  │
│  │ Table  │  │                              │  │ Table  │  │
│  └────────┘  │                              │  └────────┘  │
│       │      │                              │       │      │
│       ▼      │                              │       ▼      │
│  VA → PA     │                              │  VA → PA     │
│  변환        │                              │  변환        │
└──────────────┘                              └──────────────┘

핵심: GPU의 페이지 테이블에 CPU 메모리(또는 다른 PCIe 장치의 BAR)의
      물리 주소를 등록하면 GPU도 그 메모리에 접근할 수 있다.
      cudaHostRegister()가 바로 이 등록을 수행한다.
```

## 3. cudaHostRegister()의 두 가지 모드

### 3.1 일반 모드: cudaHostRegisterDefault (시스템 DRAM)

```
cudaHostRegister(host_ptr, size, cudaHostRegisterDefault)

사용 상황: CPU의 malloc/posix_memalign으로 할당한 시스템 메모리를
           GPU에서 접근 가능하게 만들고 싶을 때

내부 동작:

[Step 1] 페이지 핀닝 (Page Pinning)
│
│  문제: 일반 malloc 메모리는 OS가 언제든 swap out 가능
│        GPU가 접근하는 도중에 swap되면 → 잘못된 주소 접근 → 크래시
│
│  해결: OS에게 "이 페이지들은 물리 메모리에 고정(pin)해라" 요청
│        → get_user_pages() (Linux 커널)
│        → 해당 페이지들이 swap 불가 상태가 됨
│        → 물리 주소가 고정됨
│
│  host_ptr (가상)  ──MMU──►  물리 주소 0x1_0000_0000 (고정됨)
│                              이 물리 주소는 바뀌지 않음

[Step 2] GPU 페이지 테이블에 매핑 등록
│
│  NVIDIA 드라이버가 GPU MMU(GPU의 페이지 테이블)에
│  이 물리 주소를 등록한다:
│
│  GPU VA 0xFFFF_0000 ──GPU MMU──► 물리 주소 0x1_0000_0000
│                                  (= CPU 메모리의 물리 주소)
│
│  이제 GPU 스레드가 GPU VA 0xFFFF_0000에 접근하면:
│  GPU → GPU MMU → 물리 주소 0x1_0000_0000
│      → PCIe 트랜잭션 → 시스템 메모리 컨트롤러 → DRAM
│
│  ★ GPU가 "자기 VRAM"인 것처럼 접근하지만
│    실제로는 PCIe를 통해 CPU의 DRAM에 도달한다

[Step 3] cudaHostGetDevicePointer()로 GPU 주소 획득
│
│  cudaHostGetDevicePointer(&devicePtr, host_ptr, 0)
│
│  host_ptr   = CPU 가상 주소 (CPU에서 사용)
│  devicePtr  = GPU 가상 주소 (GPU 커널에서 사용)
│
│  같은 물리 메모리를 가리키는 두 개의 주소:
│  CPU: host_ptr   ──CPU MMU──► 물리 0x1_0000_0000 ──► DRAM
│  GPU: devicePtr  ──GPU MMU──► 물리 0x1_0000_0000 ──► DRAM


PCIe 트랜잭션 흐름 (GPU가 CPU DRAM 읽기):

  GPU Core                GPU MMU              PCIe Root Complex      DRAM
     │                      │                       │                  │
     │  load(devicePtr)     │                       │                  │
     │ ────────────────►    │                       │                  │
     │                      │  VA → PA 변환          │                  │
     │                      │  PA = 0x1_0000_0000   │                  │
     │                      │                       │                  │
     │                      │  PCIe Memory Read     │                  │
     │                      │  Request (PA)         │                  │
     │                      │ ─────────────────►    │                  │
     │                      │                       │  Memory Read      │
     │                      │                       │ ────────────────► │
     │                      │                       │                  │
     │                      │                       │  ◄── Data ─────  │
     │                      │  ◄── Completion ──── │                  │
     │  ◄── Data ───────── │                       │                  │
     │                      │                       │                  │

  지연: ~1-5μs (PCIe 왕복 + DRAM 접근)
  대역폭: PCIe Gen3 x16 ~15.75 GB/s, Gen4 ~31.5 GB/s
```

### 3.2 IoMemory 모드: cudaHostRegisterIoMemory (PCIe BAR)

```
cudaHostRegister(mm_ptr, 0x2000, cudaHostRegisterIoMemory)

사용 상황: PCIe 장치의 BAR(Base Address Register) 영역을
           GPU에서 접근 가능하게 만들고 싶을 때

BaM에서: NVMe SSD의 BAR0 (컨트롤러 레지스터 + Doorbell)

★ 일반 모드와의 핵심 차이:

  일반: host_ptr → 시스템 DRAM의 물리 주소
  IoMemory: mm_ptr → PCIe 장치의 BAR 물리 주소 (DRAM이 아님!)

  BAR는 시스템 DRAM이 아니라 PCIe 장치 내부의 레지스터/메모리이다.
  MMIO(Memory-Mapped I/O) 영역이므로 페이지 핀닝이 의미 없다.
  (이미 물리적으로 고정된 하드웨어 주소)


내부 동작:

[Step 1] 페이지 핀닝 → 스킵 (MMIO이므로 불필요)
│
│  mm_ptr = mmap(fd, ...) 결과
│  → CPU에서 이미 BAR0에 대한 MMIO 매핑이 존재
│  → 물리 주소 = PCIe BAR0 주소 (예: 0xFE00_0000)
│  → 이것은 DRAM이 아니라 PCIe 주소 공간의 일부
│  → swap 불가 (하드웨어 주소이므로)
│  → get_user_pages() 대신 다른 경로로 물리 주소 획득

[Step 2] GPU 페이지 테이블에 MMIO 매핑 등록
│
│  NVIDIA 드라이버가 GPU MMU에 BAR0의 물리(PCIe) 주소를 등록:
│
│  GPU VA 0xAAAA_0000 ──GPU MMU──► PCIe 주소 0xFE00_0000
│                                  (= NVMe BAR0)
│
│  ★ 캐싱 정책: Uncacheable (UC) 또는 Write-Combining (WC)
│    MMIO 접근이므로 GPU L1/L2 캐시를 거치지 않는다
│    매번 PCIe 트랜잭션이 직접 발생한다

[Step 3] cudaHostGetDevicePointer()로 GPU 주소 획득
│
│  cudaHostGetDevicePointer(&devicePtr, mm_ptr, 0)
│
│  mm_ptr    = CPU 가상 주소 (CPU에서 BAR0 접근용)
│  devicePtr = GPU 가상 주소 (GPU에서 BAR0 접근용)
│
│  같은 PCIe BAR0를 가리키는 두 개의 주소:
│  CPU: mm_ptr    ──CPU MMU──► PCIe 0xFE00_0000 ──► NVMe BAR0
│  GPU: devicePtr ──GPU MMU──► PCIe 0xFE00_0000 ──► NVMe BAR0


PCIe 트랜잭션 흐름 (GPU가 NVMe BAR0 Doorbell 쓰기):

  GPU Core            GPU MMU           PCIe Switch/Fabric      NVMe SSD
     │                   │                    │                    │
     │  store(sq.db,     │                    │                    │
     │        tail)      │                    │                    │
     │ ─────────────►    │                    │                    │
     │                   │  VA → PA 변환       │                    │
     │                   │  PA = 0xFE00_1000  │                    │
     │                   │  (BAR0 + Doorbell  │                    │
     │                   │   offset)          │                    │
     │                   │                    │                    │
     │                   │  PCIe Memory Write │                    │
     │                   │  Request           │                    │
     │                   │  (Addr=0xFE00_1000,│                    │
     │                   │   Data=tail)       │                    │
     │                   │ ──────────────►    │                    │
     │                   │                    │  PCIe Memory Write │
     │                   │                    │  (Peer-to-Peer)    │
     │                   │                    │ ──────────────────►│
     │                   │                    │                    │
     │                   │                    │              Doorbell│
     │                   │                    │              Register│
     │                   │                    │              Updated!│
     │                   │                    │                    │
     │                   │                    │  SSD가 새 커맨드   │
     │                   │                    │  있음을 인식       │
     │                   │                    │                    │

  ★ PCIe Memory Write는 Posted Transaction (응답 불필요)
    → GPU는 쓰기 후 즉시 다음 명령 진행 가능
    → SSD는 비동기적으로 Doorbell 값을 확인하고 커맨드 처리 시작
    → 지연: PCIe hop 수에 따라 수백 ns ~ 수 μs
```

## 4. BaM에서의 실제 사용 (ctrl.h:170, queue.h:190-212)

```
[1] BAR0를 CPU에서 mmap (nvm_ctrl_init, ctrl.cpp)
    ─────────────────────────────────────────────

    fd = open("/dev/libnvm0", O_RDWR)

    ctrl->mm_ptr = mmap(NULL, 0x2000,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_FILE | MAP_LOCKED,
                        fd, 0)

    → 커널 모듈(module/pci.c)의 mmap_registers():
      vm_iomap_memory()로 BAR0 PCI 리소스를 userspace에 매핑
      pgprot_noncached() → 캐시 비활성화 (MMIO이므로)

    결과:
      ctrl->mm_ptr = CPU 가상 주소 (예: 0x7f1234000000)
      이 주소 뒤에 있는 물리 주소 = PCIe BAR0 주소 (예: 0xFE000000)


[2] BAR0를 GPU에 등록 (Controller 생성자, ctrl.h:170)
    ──────────────────────────────────────────────────

    cudaHostRegister((void*)ctrl->mm_ptr,
                     NVM_CTRL_MEM_MINSIZE,     // 0x2000 = 8KB
                     cudaHostRegisterIoMemory)

    NVIDIA 드라이버 내부:
    ├── mm_ptr의 VMA(Virtual Memory Area)에서 물리 주소 추출
    │   → PFN(Page Frame Number) 또는 PCIe BAR 물리 주소 획득
    │
    ├── GPU 페이지 테이블에 등록
    │   GPU VA (새로 할당) → PCIe 0xFE000000 (BAR0)
    │   페이지 속성: Uncacheable, MMIO
    │
    └── cudaHostRegisterIoMemory 플래그의 역할:
        → "이것은 DRAM이 아니라 I/O 메모리다"
        → 페이지 핀닝(get_user_pages) 시도하지 않음
        → MMIO 특성 보존 (non-cacheable, non-prefetchable)
        → GPU MMU에 적절한 PTE 속성 설정


[3] Doorbell GPU 포인터 획득 (QueuePair 생성자, queue.h:190-195)
    ─────────────────────────────────────────────────────────────

    // CQ Doorbell
    nvm_admin_cq_create(aq_ref, &this->cq, ...)
    → cq.db = BAR0 + 0x1000 + (2*qp_id+1) * (4 << dstrd)
              ~~ 이 시점에서 cq.db는 CPU mmap 주소 ~~

    cudaHostGetDevicePointer(&devicePtr, (void*)this->cq.db, 0)
    → NVIDIA 드라이버가 이미 등록된 BAR0 범위 내에서
      cq.db의 CPU VA에 대응하는 GPU VA를 찾아서 반환

    this->cq.db = (volatile uint32_t*)devicePtr
    → 이제 cq.db는 GPU VA
    → GPU 스레드에서 *(cq.db) = head 쓰면
      GPU MMU → PCIe BAR0 Doorbell에 직접 도달


    주소 변환 체인:

    cq.db (GPU VA, 예: 0xAAAA_1008)
      │
      ▼ GPU MMU
    PCIe 주소 (예: 0xFE00_1008)
      │
      ▼ PCIe Bus
    NVMe BAR0 offset 0x1008
      │
      ▼ NVMe 컨트롤러 내부
    CQ #1 Head Doorbell Register


    오프셋 계산:
    BAR0 base                          = 0xFE00_0000 (PCIe)
    Doorbell 시작                      = BAR0 + 0x1000
    SQ #1 Tail Doorbell                = BAR0 + 0x1000 + 0 * stride
    CQ #1 Head Doorbell                = BAR0 + 0x1000 + 1 * stride
    SQ #2 Tail Doorbell                = BAR0 + 0x1000 + 2 * stride
    CQ #2 Head Doorbell                = BAR0 + 0x1000 + 3 * stride
    ...
    stride = 4 << dstrd (dstrd는 CAP 레지스터에서 읽음, 보통 0 → 4바이트)
```

## 5. 두 모드 비교 요약

```
┌──────────────────────┬─────────────────────────┬─────────────────────────┐
│                      │ cudaHostRegisterDefault  │ cudaHostRegisterIoMemory│
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ 대상 메모리           │ 시스템 DRAM             │ PCIe BAR (MMIO)         │
│                      │ (malloc, memalign 등)    │ (mmap된 디바이스 레지스터)│
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ 페이지 핀닝          │ ✅ 수행                  │ ✗ 불필요 (HW 고정)      │
│ (get_user_pages)     │ swap 방지               │ MMIO는 swap 불가        │
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ GPU MMU 매핑          │ GPU VA → DRAM PA        │ GPU VA → PCIe BAR PA    │
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ 캐싱 정책            │ Cached 가능             │ Uncacheable (UC)        │
│                      │ (GPU L2 활용)           │ 매번 PCIe 트랜잭션      │
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ PCIe 경로            │ GPU → Root Complex      │ GPU → PCIe Switch       │
│                      │     → Memory Controller │     → Target Device     │
│                      │     → DRAM              │     → BAR Register      │
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ 지연                 │ ~1-5μs (DRAM 접근)      │ ~0.5-2μs (PCIe P2P)    │
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ BaM에서의 용도        │ Admin Queue 메모리       │ NVMe BAR0 (Doorbell)    │
│                      │ (aq_mem은 Host DMA)     │ ctrl->mm_ptr 등록       │
├──────────────────────┼─────────────────────────┼─────────────────────────┤
│ GPU에서의 접근        │ 읽기/쓰기 모두          │ 주로 쓰기 (Doorbell)    │
│                      │                         │ 읽기 (CAP 등 레지스터)  │
└──────────────────────┴─────────────────────────┴─────────────────────────┘
```

## 6. GPU MMU (페이지 테이블)의 구조

```
GPU도 CPU처럼 가상 메모리 시스템을 갖고 있다:

┌─────────────────── GPU 가상 주소 공간 ──────────────────────┐
│                                                             │
│  0x0000_0000_0000 ┬── GPU VRAM (cudaMalloc 영역)            │
│                   │   GPU 로컬 메모리, 가장 빠름              │
│                   │   대역폭: ~900 GB/s (HBM2)              │
│                   │                                         │
│  0x????_????_???? ┬── System Memory 매핑 영역               │
│                   │   cudaHostRegister(Default)로 등록된 영역 │
│                   │   PCIe를 통해 CPU DRAM에 도달             │
│                   │   대역폭: ~15-32 GB/s (PCIe)            │
│                   │                                         │
│  0x????_????_???? ┬── MMIO 매핑 영역                        │
│                   │   cudaHostRegister(IoMemory)로 등록된 영역│
│                   │   PCIe를 통해 다른 장치의 BAR에 도달       │
│                   │   대역폭: 가변 (Doorbell은 4B 단위)      │
│                   │                                         │
│  0x????_????_???? ┬── Peer Device Memory 영역               │
│                   │   다른 GPU의 VRAM (NVLink/PCIe P2P)      │
│                   │                                         │
└─────────────────────────────────────────────────────────────┘

GPU PTE (Page Table Entry) 구조 (NVIDIA 내부, 비공개):
┌──────────────────────────────────────────────────────────┐
│ GPU PTE:                                                 │
│   Physical Address: 물리 주소 (VRAM, DRAM, 또는 PCIe)    │
│   Aperture:         LOCAL(VRAM) / SYSTEM(DRAM) / PEER    │
│   Cache Policy:     Cached / Uncached / Write-Combining  │
│   Read/Write:       접근 권한                            │
│   Valid:            유효 비트                             │
└──────────────────────────────────────────────────────────┘

cudaHostRegister(IoMemory) 시 생성되는 PTE:
  Physical Address = PCIe BAR0 주소 (0xFE000000)
  Aperture         = SYSTEM (PCIe 접근)
  Cache Policy     = Uncached (MMIO)
  Read/Write       = RW
  Valid            = 1
```

## 7. IOMMU가 있는 경우

```
최신 시스템에서는 IOMMU(Intel VT-d / AMD-Vi)가 존재한다.
IOMMU는 PCIe 장치의 DMA 접근을 중개한다.

GPU가 CPU DRAM이나 다른 장치 BAR에 접근할 때:

  Without IOMMU:
    GPU → PCIe → 물리 주소 직접 접근

  With IOMMU:
    GPU → PCIe → IOMMU → 물리 주소 접근
                  │
                  └── IOMMU 페이지 테이블에서
                      GPU의 PCIe 요청 주소를 실제 물리 주소로 변환
                      + 접근 권한 체크

  cudaHostRegister() 시 NVIDIA 드라이버는:
    1. GPU MMU 페이지 테이블 설정 (GPU VA → PA)
    2. IOMMU 페이지 테이블에도 등록 (GPU가 이 PA에 접근 허용)

  BaM 커널 모듈에서 nvidia_p2p_dma_map_pages()도 마찬가지:
    GPU VRAM의 물리 주소를 IOMMU에 등록하여
    NVMe SSD가 GPU VRAM에 DMA Write 할 수 있게 한다


전체 주소 변환 레이어:

  GPU 스레드: *(sq.db) = tail

  GPU VA (sq.db)
    │
    ▼ GPU MMU (GPU 내부)
  GPU PA = PCIe BAR0 주소
    │
    ▼ IOMMU (시스템 보드, 있는 경우)
  시스템 PA = PCIe BAR0 주소 (변환 또는 패스스루)
    │
    ▼ PCIe Fabric
  NVMe SSD BAR0 Doorbell Register

  ★ 총 2~3단계 주소 변환을 거치지만
    하드웨어에서 수행되므로 소프트웨어 오버헤드는 없다
```

## 8. cudaHostRegister vs cudaHostAlloc vs cudaMalloc 비교

```
┌──────────────────────┬─────────────────┬─────────────────┬─────────────────┐
│                      │ cudaMalloc      │ cudaHostAlloc   │cudaHostRegister │
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ 메모리 위치           │ GPU VRAM        │ CPU DRAM (핀닝) │ 기존 메모리에    │
│                      │                 │                 │ 핀닝+매핑 추가  │
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ 할당 주체             │ CUDA 드라이버    │ CUDA 드라이버    │ 사용자 (malloc  │
│                      │                 │                 │ mmap 등)       │
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ GPU 접근              │ ✅ 직접 (빠름)   │ ✅ PCIe (느림)   │ ✅ PCIe (느림)   │
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ CPU 접근              │ ✗ (cudaMemcpy  │ ✅ 직접          │ ✅ 직접          │
│                      │  필요)          │                 │                 │
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ 페이지 핀닝           │ N/A (VRAM)      │ ✅ 할당 시 자동  │ ✅ 등록 시 수행  │
│                      │                 │                 │ (IoMemory: 스킵)│
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ BaM에서의 용도        │ SQ/CQ 데이터    │ BufferPtr용     │ BAR0 Doorbell   │
│                      │ 캐시 데이터     │ (createBuffer)  │ (ctrl->mm_ptr)  │
│                      │ 동기화 배열     │                 │                 │
├──────────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ DMA 가능 여부         │ ✅ (P2P DMA)    │ ✅ (Host DMA)   │ ✅ (BAR은 DMA   │
│                      │ nvidia_p2p 사용 │ ioctl 사용      │  대상이 아님)   │
└──────────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

## 9. BaM에서의 전체 메모리 접근 경로 정리

```
GPU 커널이 접근하는 3종류의 메모리와 각각의 경로:

[경로 1] GPU VRAM 접근 (가장 빠름)
  SQ/CQ 데이터, 캐시 페이지, 동기화 배열

  GPU Core → GPU L1 → GPU L2 → GPU VRAM (HBM)
  지연: ~100ns
  대역폭: ~900 GB/s


[경로 2] CPU 피닝 메모리 접근 (GIDS의 CPU Buffer 등)
  cudaHostAlloc() 또는 cudaHostRegister(Default)

  GPU Core → GPU MMU → PCIe → Root Complex → Memory Controller → DRAM
  지연: ~1-5μs
  대역폭: ~15-32 GB/s (PCIe)


[경로 3] NVMe BAR0 접근 (Doorbell)
  cudaHostRegister(IoMemory) → cudaHostGetDevicePointer

  GPU Core → GPU MMU → PCIe → [PCIe Switch] → NVMe SSD BAR0
  지연: ~0.5-2μs (Posted Write는 응답 불필요)
  대역폭: 4바이트 단위 (Doorbell은 32-bit 레지스터)

  ★ Doorbell Write는 PCIe Posted Memory Write
    → GPU는 쓰기만 하고 응답을 기다리지 않음
    → 실질적으로 fire-and-forget
    → 이것이 BaM이 빠른 이유 중 하나
```
