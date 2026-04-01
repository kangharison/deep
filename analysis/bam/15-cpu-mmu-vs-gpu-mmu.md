# CPU MMU vs GPU MMU 비교 분석

## 1. 왜 GPU에도 MMU가 필요한가

```
초기 GPU (2000년대 초반):
  GPU는 물리 주소로 VRAM에 직접 접근했다.
  → 프로세스 간 격리 불가
  → VRAM 파편화 문제
  → 가상 메모리 없음

현대 GPU (Fermi/2010~ 이후):
  GPU도 CPU처럼 가상 메모리 시스템을 갖추게 됨.
  → CUDA 프로그램마다 독립된 가상 주소 공간
  → 여러 프로세스가 GPU를 안전하게 공유
  → Unified Virtual Addressing (UVA)
  → cudaMalloc, cudaHostRegister 등이 이 위에 구축됨
```

## 2. CPU MMU 구조 (x86-64)

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CPU MMU (x86-64)                            │
│                                                                     │
│  가상 주소 (48-bit, 또는 57-bit with LA57)                          │
│  ┌────────┬────────┬────────┬────────┬──────────────┐              │
│  │ PML4   │ PDPT   │ PD     │ PT     │ Page Offset  │              │
│  │ 9 bits │ 9 bits │ 9 bits │ 9 bits │ 12 bits      │              │
│  └───┬────┴───┬────┴───┬────┴───┬────┴──────────────┘              │
│      │        │        │        │                                   │
│      ▼        ▼        ▼        ▼                                   │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                               │
│  │PML4E │→│PDPTE │→│ PDE  │→│ PTE  │→ 물리 주소 + Page Offset      │
│  │512개 │ │512개 │ │512개 │ │512개 │                               │
│  └──────┘ └──────┘ └──────┘ └──────┘                               │
│                                                                     │
│  4단계 페이지 테이블 워크 (4KB 페이지 기준)                         │
│                                                                     │
│  CR3 레지스터 → PML4 테이블의 물리 주소                             │
│  프로세스 전환 시 CR3 변경 → 주소 공간 전환                         │
│                                                                     │
│  TLB (Translation Lookaside Buffer):                                │
│    최근 변환 결과 캐시                                              │
│    L1 TLB: ~64 엔트리, 1 사이클                                    │
│    L2 TLB: ~1024-1536 엔트리, ~10 사이클                           │
│    TLB miss → 페이지 테이블 워크 (~100-1000 사이클)                 │
│                                                                     │
│  PTE (Page Table Entry) 구조 (64-bit):                              │
│  ┌─────────────────────────────────────────────────────┐            │
│  │ bit 63:    NX (No Execute)                          │            │
│  │ bits 51-12: 물리 페이지 프레임 번호 (PFN)           │            │
│  │ bit 8:     Global                                   │            │
│  │ bit 7:     Page Size (2MB/1GB huge page)            │            │
│  │ bit 6:     Dirty (쓰기 발생)                        │            │
│  │ bit 5:     Accessed (접근 발생)                     │            │
│  │ bit 4:     PCD (Page Cache Disable)                 │            │
│  │ bit 3:     PWT (Page Write Through)                 │            │
│  │ bit 2:     U/S (User/Supervisor)                    │            │
│  │ bit 1:     R/W (Read/Write)                         │            │
│  │ bit 0:     Present (유효)                           │            │
│  └─────────────────────────────────────────────────────┘            │
│                                                                     │
│  페이지 크기:                                                       │
│    4KB   (기본, PT 레벨)                                            │
│    2MB   (Huge Page, PD 레벨)                                       │
│    1GB   (Giant Page, PDPT 레벨)                                    │
│                                                                     │
│  Page Fault 처리:                                                   │
│    PTE.Present = 0 → #PF 예외 → OS 커널이 처리                     │
│    → 디스크에서 로드 (swap in)                                      │
│    → 새 물리 페이지 할당                                            │
│    → PTE 업데이트 후 재시도                                         │
│    → COW (Copy-on-Write), 지연 할당 등                              │
│                                                                     │
│  특징:                                                              │
│    - 프로세스당 1개의 페이지 테이블 (CR3)                            │
│    - 코어당 TLB (공유 안 함)                                        │
│    - OS 커널이 페이지 테이블 관리                                   │
│    - Page Fault = 소프트웨어 예외 → OS가 처리                       │
│    - 콘텍스트 스위칭 시 TLB 플러시 (PCID로 완화)                    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

## 3. GPU MMU 구조 (NVIDIA)

```
┌─────────────────────────────────────────────────────────────────────┐
│                         GPU MMU (NVIDIA)                            │
│                                                                     │
│  가상 주소 (49-bit, GPU 아키텍처에 따라 다름)                       │
│  ┌────────┬────────┬────────┬────────┬──────────────┐              │
│  │ PDE3   │ PDE2   │ PDE1   │ PDE0   │ Page Offset  │              │
│  │ (가변) │ (가변) │ (가변) │ (가변) │ (가변)       │              │
│  └───┬────┴───┬────┴───┬────┴───┬────┴──────────────┘              │
│      │        │        │        │                                   │
│      ▼        ▼        ▼        ▼                                   │
│  다단계 페이지 테이블 (2~5단계, 구성에 따라 가변)                    │
│                                                                     │
│  인스턴스 메모리 (Instance Block):                                   │
│    CPU의 CR3에 해당                                                 │
│    CUDA 컨텍스트(= GPU 프로세스)마다 1개                            │
│    GPU 채널(Channel) 생성 시 할당                                   │
│                                                                     │
│  PDE/PTE 구조 (NVIDIA 내부, 공식 비공개):                            │
│  ┌─────────────────────────────────────────────────────┐            │
│  │ 물리 주소 필드                                      │            │
│  │ Aperture: LOCAL(VRAM) / SYSTEM(DRAM) / PEER(타GPU)  │            │
│  │ Cache Policy: L1/L2 Cached / Uncached / Streaming   │            │
│  │ Valid bit                                           │            │
│  │ Read-Only bit                                       │            │
│  │ Volatile bit (캐시 우회)                            │            │
│  │ Kind: 압축/타일링 메타데이터                         │            │
│  └─────────────────────────────────────────────────────┘            │
│                                                                     │
│  ★ Aperture 필드 (CPU에는 없는 개념):                               │
│    GPU PTE가 가리키는 물리 주소가 "어디에 있는 메모리인지" 명시      │
│                                                                     │
│    LOCAL:  GPU 자체 VRAM (HBM/GDDR)                                │
│            → GPU 내부 메모리 컨트롤러로 접근                        │
│            → 지연: ~100ns, 대역폭: ~900 GB/s                       │
│                                                                     │
│    SYSTEM: CPU 시스템 메모리 (DRAM)                                  │
│            → PCIe를 통해 CPU 메모리 컨트롤러로 접근                 │
│            → cudaHostRegister(Default)로 매핑된 영역               │
│            → 지연: ~1-5μs, 대역폭: ~15-32 GB/s                    │
│                                                                     │
│    PEER:   다른 GPU의 VRAM 또는 PCIe 장치의 BAR                    │
│            → NVLink 또는 PCIe P2P를 통해 접근                      │
│            → cudaHostRegister(IoMemory)로 매핑된 BAR               │
│            → 지연: ~0.5-5μs                                        │
│                                                                     │
│  페이지 크기:                                                       │
│    4KB   (기본, 가장 세밀한 제어)                                    │
│    64KB  (Big Page, 가장 많이 사용)                                  │
│    2MB   (Huge Page, 연속 할당 시)                                   │
│    512MB (Ampere+에서 지원)                                          │
│                                                                     │
│    ★ GPU는 64KB를 기본 단위로 선호 (VRAM 대역폭 최적화)             │
│    ★ BaM의 nvidia_p2p_get_pages()가 64KB 페이지를 사용하는 이유     │
│                                                                     │
│  TLB 구조:                                                          │
│    L1 TLB: SM(Streaming Multiprocessor)당 1개                       │
│            ~32-128 엔트리                                           │
│            1-2 사이클                                               │
│                                                                     │
│    L2 TLB: GPU 전체 공유 (GPC당 또는 전체)                          │
│            ~4096-16384 엔트리                                       │
│            ~20-50 사이클                                            │
│                                                                     │
│    TLB miss → GPU 페이지 테이블 워크                                │
│              GPU 내부의 Page Table Walker 하드웨어가 수행            │
│              페이지 테이블이 VRAM에 있으므로 비교적 빠름             │
│              ~100-500 사이클                                        │
│                                                                     │
│  Fault 처리:                                                        │
│    ┌─── Pre-Pascal (이전) ─────────────────────────────────┐        │
│    │ GPU Page Fault 미지원                                  │        │
│    │ 모든 메모리가 미리 매핑되어 있어야 함                  │        │
│    │ 매핑 안 된 주소 접근 → GPU hang 또는 에러              │        │
│    └────────────────────────────────────────────────────────┘        │
│                                                                     │
│    ┌─── Pascal (2016) 이후 ────────────────────────────────┐        │
│    │ GPU Page Fault 지원 (ATS/ATC with IOMMU)              │        │
│    │ 매핑 안 된 주소 접근 시:                               │        │
│    │   GPU가 Fault 신호 → NVIDIA 드라이버(CPU) → 처리      │        │
│    │   → 물리 페이지 할당 → PTE 업데이트 → GPU 재시도      │        │
│    │                                                        │        │
│    │ ★ CPU Page Fault와의 핵심 차이:                        │        │
│    │   CPU: 즉시 처리 (동기적, 해당 코어만 멈춤)            │        │
│    │   GPU: 드라이버 경유 (비동기적, 전체 SM이 stall 가능)  │        │
│    │        → 매우 비싸므로 가능한 한 미리 매핑             │        │
│    │                                                        │        │
│    │ CUDA Managed Memory (cudaMallocManaged):              │        │
│    │   이 Page Fault 메커니즘 위에 구축됨                   │        │
│    │   CPU/GPU가 같은 포인터로 접근                         │        │
│    │   접근 시 자동으로 데이터 마이그레이션                 │        │
│    └────────────────────────────────────────────────────────┘        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

## 4. CPU MMU vs GPU MMU 상세 비교

```
┌──────────────────────┬────────────────────────┬─────────────────────────┐
│ 항목                  │ CPU MMU (x86-64)       │ GPU MMU (NVIDIA)        │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 가상 주소 비트        │ 48-bit (256TB)          │ 49-bit (512TB)          │
│                      │ LA57: 57-bit           │ 아키텍처마다 다름       │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 페이지 테이블 단계    │ 4단계 (PML4→PDP→PD→PT) │ 2~5단계 (가변)          │
│                      │ 5단계 (LA57)           │ 페이지 크기에 따라 조정 │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 기본 페이지 크기      │ 4KB                    │ 4KB / 64KB              │
│                      │ Huge: 2MB, 1GB         │ Big: 64KB (주력)        │
│                      │                        │ Huge: 2MB, 512MB        │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 페이지 테이블 위치    │ 시스템 DRAM             │ GPU VRAM                │
│                      │                        │ (일부 시스템 DRAM 가능)  │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 관리 주체             │ OS 커널                │ NVIDIA 드라이버         │
│                      │ (Linux mm subsystem)   │ (커널 모드 드라이버)    │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 주소 공간 기준        │ 프로세스당 1개          │ CUDA 컨텍스트당 1개     │
│ (= 페이지 테이블      │ (CR3 레지스터)         │ (Instance Block)        │
│  루트 포인터)         │                        │ 채널(Channel)과 연결    │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ TLB 구조              │ 코어당 독립 L1 TLB     │ SM당 독립 L1 TLB        │
│                      │ 공유 L2 TLB            │ 공유 L2 TLB (GPC/전체)  │
│                      │                        │                         │
│ TLB 엔트리 수         │ L1: ~64                │ L1: ~32-128 (SM당)      │
│                      │ L2: ~1024-1536         │ L2: ~4096-16384         │
│                      │                        │                         │
│ TLB miss 비용         │ ~100-1000 사이클       │ ~100-500 사이클         │
│                      │ (DRAM에서 PT 읽기)     │ (VRAM에서 PT 읽기, 빠름)│
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ PTE 특수 필드         │ NX (No Execute)        │ Aperture (메모리 종류)  │
│                      │ Dirty, Accessed        │  - LOCAL (VRAM)         │
│                      │ PCD, PWT (캐시 정책)   │  - SYSTEM (DRAM)        │
│                      │ U/S (권한 레벨)        │  - PEER (타 장치)       │
│                      │                        │ Cache Policy            │
│                      │                        │ Kind (압축/타일링)      │
│                      │                        │ Volatile                │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ Page Fault            │ #PF 예외 → OS 즉시 처리│ GPU fault →             │
│                      │ 해당 코어만 stall      │ NVIDIA 드라이버(CPU)    │
│                      │ 매우 일반적/효율적     │ → 처리 후 GPU 재시도    │
│                      │ demand paging 기반     │ 매우 비싸 (수십 μs)     │
│                      │                        │ 가능한 한 회피          │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 동시 접근 스레드 수    │ 코어당 1-2 (HT)       │ SM당 ~2048 스레드       │
│                      │ 총 ~수십 스레드        │ 총 ~수만 스레드         │
│                      │                        │                         │
│ TLB 압박              │ 낮음 (스레드 적음)     │ 높음 (수만 스레드)      │
│                      │                        │ → 64KB 페이지 선호      │
│                      │                        │ → TLB 커버리지 중요     │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 외부 메모리 접근      │ 없음                   │ Aperture로 구분:        │
│ (자기 DRAM 외)        │ (MMIO는 별도 매핑)     │ LOCAL/SYSTEM/PEER를     │
│                      │                        │ PTE 레벨에서 구분       │
│                      │                        │ → HW가 자동 라우팅     │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ 콘텍스트 스위칭       │ CR3 변경 + TLB 플러시  │ 채널 스위칭 +           │
│                      │ PCID로 일부 보존 가능  │ Instance Block 변경     │
│                      │ ~수 μs                 │ GPU 컨텍스트 스위칭은   │
│                      │                        │ 매우 비싸 (~ms 단위)    │
│                      │                        │ → 가능한 한 피함       │
│                      │                        │                         │
├──────────────────────┼────────────────────────┼─────────────────────────┤
│                      │                        │                         │
│ IOMMU 관계            │ CPU 자체는 IOMMU 불필요│ GPU의 PCIe 접근은       │
│                      │ (직접 DRAM 접근)       │ IOMMU를 통과            │
│                      │ IOMMU는 장치용         │ (VT-d/AMD-Vi 활성 시)   │
│                      │                        │ GPU→DRAM 접근 권한 체크 │
│                      │                        │                         │
└──────────────────────┴────────────────────────┴─────────────────────────┘
```

## 5. Aperture — GPU MMU만의 핵심 개념

```
CPU MMU의 PTE:
  가상 주소 → 물리 주소 (끝)
  물리 주소는 항상 시스템 DRAM을 가리킨다.
  (MMIO 영역은 OS가 별도로 ioremap 등으로 관리)

GPU MMU의 PTE:
  가상 주소 → (Aperture, 물리 주소)
  Aperture가 "이 물리 주소가 어떤 종류의 메모리인지" 알려준다.
  GPU 하드웨어가 Aperture를 보고 접근 경로를 자동 선택한다.


GPU 스레드가 주소 X에 접근할 때:

  GPU MMU가 PTE를 읽음
      │
      ├── Aperture = LOCAL
      │   → GPU 내부 메모리 컨트롤러 → VRAM (HBM/GDDR)
      │   → Crossbar → Memory Partition → HBM Stack
      │   지연: ~100ns
      │
      ├── Aperture = SYSTEM
      │   → GPU PCIe 인터페이스 → PCIe 버스 → Root Complex
      │   → CPU 메모리 컨트롤러 → DRAM
      │   지연: ~1-5μs
      │
      └── Aperture = PEER
          → GPU PCIe 인터페이스 → PCIe 버스 → 대상 장치
          │
          ├── 다른 GPU의 VRAM (NVLink 또는 PCIe)
          │   지연: ~1-10μs
          │
          └── PCIe 장치의 BAR (NVMe SSD 등)
              지연: ~0.5-2μs


BaM에서 cudaHostRegister(IoMemory) 후:

  GPU PTE for sq.db:
    Aperture      = PEER (또는 SYSTEM, MMIO 특성)
    Physical Addr = 0xFE00_1000 (NVMe BAR0 Doorbell)
    Cache Policy  = Uncached
    Valid         = 1

  GPU 스레드가 *(sq.db) = tail 실행:
    GPU MMU → PTE 조회 → Aperture=PEER, PA=0xFE00_1000
    → GPU가 PCIe Memory Write 트랜잭션 생성
    → PCIe Fabric → NVMe SSD BAR0 Doorbell 도달


cudaMalloc() 후:

  GPU PTE for gpu_buffer:
    Aperture      = LOCAL
    Physical Addr = 0x0000_1000_0000 (VRAM 내)
    Cache Policy  = L2 Cached (기본)
    Valid         = 1

  GPU 스레드가 gpu_buffer[i] 접근:
    GPU MMU → PTE 조회 → Aperture=LOCAL, PA=VRAM 내
    → GPU 내부 Crossbar → Memory Partition → HBM 접근
    → L1/L2 캐시 히트 시 훨씬 빠름
```

## 6. TLB 비교: 왜 GPU는 큰 페이지를 선호하는가

```
CPU 시나리오:
  4개 코어 × HyperThread 2 = 8 스레드
  각 스레드가 독립적 워킹셋
  L1 TLB 64 엔트리 × 4KB = 256KB 커버
  L2 TLB 1536 엔트리 × 4KB = 6MB 커버
  → 대부분의 워크로드에 충분

GPU 시나리오:
  80 SM × warp 64개 × 32 스레드 = ~160,000 스레드
  SM당 L1 TLB: ~128 엔트리

  4KB 페이지 사용 시:
    128 × 4KB = 512KB 커버 (SM당)
    SM에서 2048 스레드가 512KB 내에서만 TLB hit
    → 대규모 데이터 접근 시 TLB miss 폭발

  64KB 페이지 사용 시:
    128 × 64KB = 8MB 커버 (SM당)
    → 16배 넓은 커버리지
    → TLB miss 대폭 감소

  2MB 페이지 사용 시:
    128 × 2MB = 256MB 커버 (SM당)
    → 거의 TLB miss 없음

  ★ 이것이 NVIDIA가 64KB를 기본 페이지로 선호하는 이유
  ★ cudaMalloc()은 내부적으로 64KB 또는 2MB 페이지 사용
  ★ nvidia_p2p_get_pages()의 GPU_PAGE_SIZE = 64KB인 이유


TLB miss 영향 비교:

  CPU TLB miss:
    → Page Table Walker가 DRAM에서 PTE 읽기
    → ~100-1000 사이클 지연
    → 해당 스레드만 stall, 다른 코어는 영향 없음
    → OS는 TLB miss를 성능 최적화 관점에서만 관리

  GPU TLB miss:
    → Page Table Walker가 VRAM에서 PTE 읽기
    → ~100-500 사이클 지연
    → ★ 해당 SM의 전체 warp 스케줄러가 영향받을 수 있음
    → 수천 스레드가 같은 TLB를 공유하므로 miss가 전파됨
    → TLB miss = 처리량(throughput) 직접 감소
    → GPU는 latency hiding으로 완화하지만 한계 있음
```

## 7. Page Fault 처리 비교

```
┌──── CPU Page Fault ──────────────────────────────────────────────┐
│                                                                  │
│  스레드가 매핑 안 된 VA 접근                                     │
│  → CPU가 #PF 예외 발생                                          │
│  → 즉시 OS 커널의 do_page_fault() 진입 (~수백 ns)               │
│  → 원인 분석:                                                    │
│     ├── 유효한 접근 (demand paging)                               │
│     │   → 물리 페이지 할당                                       │
│     │   → swap에서 로드 (필요 시)                                │
│     │   → PTE 설정                                               │
│     │   → 재시도 (~수 μs ~ 수 ms)                               │
│     │                                                            │
│     └── 무효한 접근 (segfault)                                   │
│         → SIGSEGV 시그널 → 프로세스 종료                         │
│                                                                  │
│  특징:                                                           │
│  - 해당 코어/스레드만 stall                                      │
│  - 다른 코어는 정상 실행                                         │
│  - 매우 일반적이고 효율적 (모든 OS가 의존)                       │
│  - demand paging, COW, mmap 등의 기반                            │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘

┌──── GPU Page Fault (Pascal+) ────────────────────────────────────┐
│                                                                  │
│  GPU 스레드가 매핑 안 된 VA 접근                                 │
│  → GPU MMU가 fault 감지                                         │
│  → GPU가 fault 정보를 NVIDIA 드라이버(CPU)에 전달                │
│     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~          │
│     이 과정 자체가 느림:                                         │
│     GPU → PCIe 인터럽트 → CPU → 드라이버 ISR → 워크큐           │
│     ~수십 μs                                                     │
│  → 드라이버가 처리:                                              │
│     ├── 물리 페이지 할당 (VRAM 또는 DRAM)                        │
│     ├── GPU 페이지 테이블 업데이트                                │
│     ├── TLB 무효화                                               │
│     └── GPU에 "재시도" 신호                                      │
│  → GPU가 해당 warp 재시도                                        │
│     총 지연: ~수십 μs ~ 수백 μs                                  │
│                                                                  │
│  특징:                                                           │
│  - CPU 개입 필요 (GPU 혼자 해결 불가)                            │
│  - 해당 SM의 많은 warp가 영향받을 수 있음                        │
│  - CPU Page Fault보다 10-100배 비쌈                              │
│  - 그래서 cudaMalloc()은 미리 전체 매핑을 설정                   │
│  - cudaMallocManaged()만 이 메커니즘 사용                        │
│                                                                  │
│  ★ BaM이 Page Fault를 사용하지 않는 이유:                        │
│    모든 메모리를 미리 할당+매핑 (cudaMalloc + DMA map)            │
│    GPU 커널 실행 중 Page Fault 발생 = 성능 재앙                  │
│    수만 스레드가 동시에 I/O하는데 Fault마다 CPU 왕복 불가         │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## 8. BaM에서 GPU MMU가 관여하는 모든 접근

```
BaM의 GPU 커널이 접근하는 주소와 GPU MMU의 PTE 설정:

┌────────────────────────┬──────────┬───────────┬───────────────────┐
│ 접근 대상               │ Aperture │ 캐시 정책  │ 등록 방법          │
├────────────────────────┼──────────┼───────────┼───────────────────┤
│ SQ 데이터 (nvm_cmd_t)  │ LOCAL    │ L2 Cached │ cudaMalloc()      │
│ CQ 데이터 (nvm_cpl_t)  │ LOCAL    │ L2 Cached │ cudaMalloc()      │
│ 캐시 페이지 (pages_dma) │ LOCAL    │ L2 Cached │ cudaMalloc()      │
│ 동기화 배열 (tickets 등)│ LOCAL    │ L2 Cached │ cudaMalloc()      │
│ Controller copy        │ LOCAL    │ L2 Cached │ cudaMalloc()      │
│ page_cache_d_t copy    │ LOCAL    │ L2 Cached │ cudaMalloc()      │
├────────────────────────┼──────────┼───────────┼───────────────────┤
│ SQ Doorbell (sq.db)    │ PEER     │ Uncached  │ cudaHostRegister  │
│ CQ Doorbell (cq.db)    │ PEER     │ Uncached  │ (IoMemory)        │
│ BAR0 레지스터 (CAP 등)  │ PEER     │ Uncached  │                   │
├────────────────────────┼──────────┼───────────┼───────────────────┤
│ CPU Buffer (GIDS용)    │ SYSTEM   │ Uncached  │ cudaHostRegister  │
│                        │          │           │ (Default)         │
└────────────────────────┴──────────┴───────────┴───────────────────┘

GPU 스레드의 한 번의 read_data() 동안 MMU가 처리하는 접근:

  [1] Controller* 읽기              → LOCAL (TLB hit 가능)
  [2] QueuePair* 읽기               → LOCAL (TLB hit 가능)
  [3] sq.in_ticket atomic           → LOCAL
  [4] sq.tickets[pos] 폴링          → LOCAL
  [5] sq.vaddr[pos] = cmd 복사      → LOCAL (SQ 데이터)
  [6] sq.tail_mark[pos] 쓰기        → LOCAL
  [7] *(sq.db) = tail 쓰기          → PEER (BAR0 Doorbell, PCIe)
                                      ★ 유일한 외부 접근
  [8] cq.vaddr[loc] 읽기 (polling)  → LOCAL (CQ 데이터)
                                      ★ 하지만 SSD가 DMA Write한 데이터
                                        L2 캐시에 없으므로 VRAM 직접 읽기
  [9] *(cq.db) = head 쓰기          → PEER (BAR0 Doorbell, PCIe)
  [10] 캐시 페이지 데이터 읽기       → LOCAL (pages_dma)

  총 10번의 MMU 변환 중:
    LOCAL: 8번 (GPU VRAM, 빠름)
    PEER:  2번 (PCIe BAR0, 느림)
```
