# NVIDIA Developer 학습 리소스 가이드 (100+ 리소스)

GPU 아키텍처를 깊이 이해하고, PCIe 기반 NVMe 스토리지와의 상호작용을 파악하기 위한 학습 리소스를 단계별로 정리한다. 총 110개 리소스를 8개 카테고리로 분류.

---

## 카테고리 요약

| # | 카테고리 | 리소스 수 | 설명 |
|---|----------|-----------|------|
| 1 | GPU 아키텍처 기초 & CUDA | 18개 | CUDA 프로그래밍 모델, SM 구조, 메모리 계층 |
| 2 | 세대별 아키텍처 진화 | 16개 | Fermi → Blackwell 세대별 백서/블로그 |
| 3 | GPU 메모리 심화 | 14개 | Unified Memory, Virtual Memory, 캐시 제어 |
| 4 | GPU 실행 모델 & 동기화 | 10개 | Warp, Stream, Cooperative Groups, Atomic |
| 5 | GPU-NVMe 연결 (GPUDirect Storage) | 16개 | GDS 아키텍처, cuFile API, P2P DMA |
| 6 | GPU 인터커넥트 (NVLink/NVSwitch/PCIe) | 12개 | 다중 GPU 연결, PCIe BAR, Resizable BAR |
| 7 | 시스템 아키텍처 & 가상화 | 12개 | DGX 시스템, MIG, SR-IOV, DPU |
| 8 | 성능 분석 & 최적화 도구 | 12개 | Nsight, Profiling, 전력 관리 |
| **합계** | | **110개** | |

---

## 1. GPU 아키텍처 기초 & CUDA 프로그래밍 (18개)

### 1-1. CUDA Programming Guide (공식 문서)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 1 | CUDA C++ Programming Guide (Legacy) | https://docs.nvidia.com/cuda/cuda-c-programming-guide/ | GPU 아키텍처 공식 교과서 |
| 2 | CUDA Programming Guide (신규) | https://docs.nvidia.com/cuda/cuda-programming-guide/index.html | 최신 버전 통합 가이드 |
| 3 | CUDA Programming Model | https://docs.nvidia.com/cuda/cuda-programming-guide/01-introduction/programming-model.html | 스레드/블록/그리드 모델 |
| 4 | CUDA Best Practices Guide | https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/ | 실전 최적화 가이드라인 |
| 5 | CUDA Toolkit Documentation | https://docs.nvidia.com/cuda/ | CUDA 전체 문서 허브 |

### 1-2. CUDA 입문 & 튜토리얼 (블로그)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 6 | An Even Easier Introduction to CUDA | https://developer.nvidia.com/blog/even-easier-introduction-cuda/ | 첫 CUDA 프로그래밍 실습 |
| 7 | CUDA Refresher: Programming Model | https://developer.nvidia.com/blog/cuda-refresher-cuda-programming-model/ | Host-Device 데이터 전송, PCIe 버스 |
| 8 | CUDA 8 Features Revealed | https://developer.nvidia.com/blog/cuda-8-features-revealed/ | Pascal 지원, Unified Memory 확장 |
| 9 | CUDA 11 Features Revealed | https://developer.nvidia.com/blog/cuda-11-features-revealed/ | Ampere 지원, L2 캐시 상주 제어 |
| 10 | CUDA Toolkit 12.0 Released | https://developer.nvidia.com/blog/cuda-toolkit-12-0-released-for-general-availability/ | Hopper TMA, 비동기 트랜잭션 배리어 |
| 11 | CUDA Toolkit 12.2 Features | https://developer.nvidia.com/blog/nvidia-cuda-toolkit-12-2-unleashes-powerful-features-for-boosting-applications/ | P2PDMA NVMe 지원 확대 |
| 12 | CUDA Toolkit 12.8 Blackwell Support | https://developer.nvidia.com/blog/cuda-toolkit-12-8-delivers-nvidia-blackwell-support | Blackwell 아키텍처 지원 |
| 13 | What's New in CUDA Toolkit 13.0 | https://developer.nvidia.com/blog/whats-new-and-important-in-cuda-toolkit-13-0/ | 최신 기능 |

### 1-3. CUDA Driver & Runtime

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 14 | CUDA Context-Independent Module Loading | https://developer.nvidia.com/blog/cuda-context-independent-module-loading/ | CUDA 12.0 컨텍스트 독립 로딩 |
| 15 | Dynamic Loading in CUDA Runtime | https://developer.nvidia.com/blog/dynamic-loading-in-the-cuda-runtime | Runtime/Driver API 구분 |
| 16 | CUDA Occupancy API | https://developer.nvidia.com/blog/cuda-pro-tip-occupancy-api-simplifies-launch-configuration/ | 최적 스레드 블록 크기 계산 |
| 17 | CUDA GPU Compute Capability 목록 | https://developer.nvidia.com/cuda/gpus | GPU별 컴퓨트 능력 참조 |
| 18 | Architecture Support 가이드 | https://developer.nvidia.com/blog/navigating-gpu-architecture-support-a-guide-for-nvidia-cuda-developers | 아키텍처별 호환성 |

**핵심 학습 포인트:**

```
┌─────────────────────────────────────────────────────┐
│                    GPU (GPC × N)                    │
│  ┌───────────────────────────────────────────────┐  │
│  │              GPC (Graphics                    │  │
│  │           Processing Cluster)                 │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐      │  │
│  │  │   TPC   │  │   TPC   │  │   TPC   │ ...  │  │
│  │  │ ┌─────┐ │  │ ┌─────┐ │  │ ┌─────┐ │      │  │
│  │  │ │ SM  │ │  │ │ SM  │ │  │ │ SM  │ │      │  │
│  │  │ └─────┘ │  │ └─────┘ │  │ └─────┘ │      │  │
│  │  └─────────┘  └─────────┘  └─────────┘      │  │
│  └───────────────────────────────────────────────┘  │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │
│  │ L2 Cache │  │ Mem Ctrl │  │ PCIe / NVLink    │  │
│  └──────────┘  └──────────┘  └──────────────────┘  │
└─────────────────────────────────────────────────────┘
```

**SM (Streaming Multiprocessor) 내부 구조:**

```
┌─────────────────────────────────────────────┐
│              SM (Streaming Multiprocessor)    │
│                                              │
│  ┌────────────────────────────────────────┐  │
│  │          Warp Scheduler × 4            │  │
│  │  (각 스케줄러가 32-thread warp 관리)    │  │
│  └────────────────────────────────────────┘  │
│                                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐    │
│  │ FP32     │ │ INT32    │ │ Tensor   │    │
│  │ CUDA Core│ │ CUDA Core│ │ Core     │    │
│  │ (128개)  │ │ (128개)  │ │ (4개)    │    │
│  └──────────┘ └──────────┘ └──────────┘    │
│                                              │
│  ┌──────────────────────────────────────┐    │
│  │     Register File (256 KB)           │    │
│  └──────────────────────────────────────┘    │
│  ┌──────────────────────────────────────┐    │
│  │  Shared Memory / L1 Cache (256 KB)   │    │
│  │  (설정에 따라 비율 조정 가능)         │    │
│  └──────────────────────────────────────┘    │
└─────────────────────────────────────────────┘
```

**GPU 메모리 계층 (빠른순):**

```
Register (per thread)     ← 가장 빠름, ~1 cycle
    ↓
Shared Memory / L1 Cache  ← SM 내부, ~20-30 cycles
    ↓
L2 Cache                  ← GPU 전체 공유
    ↓
Global Memory (HBM/GDDR)  ← ~200-400 cycles
    ↓
System Memory (CPU DRAM)   ← PCIe 경유, ~10,000+ cycles
    ↓
Storage (NVMe SSD)         ← PCIe 경유, ~100,000+ cycles
```

---

## 2. 세대별 GPU 아키텍처 진화 (16개)

### 아키텍처 세대별 진화 요약

```
Fermi(2010) → Kepler(2012) → Maxwell(2014) → Pascal(2016) → Volta(2017)
  PCIe 2.0     PCIe 3.0       PCIe 3.0        PCIe 3.0      PCIe 3.0
  Tensor X     Tensor X       Tensor X        Tensor X      Tensor 1
  GDDR5        GDDR5          GDDR5           HBM2/GDDR5X   HBM2
  NVLink X     NVLink X       NVLink X        NVLink 1      NVLink 2

→ Turing(2018) → Ampere(2020) → Hopper(2022) → Blackwell(2024)
    PCIe 3.0      PCIe 4.0      PCIe 5.0       PCIe 5.0
    Tensor 2      Tensor 3      Tensor 4       Tensor 5
    GDDR6         HBM2e         HBM3           HBM3e
    NVLink X      NVLink 3      NVLink 4       NVLink 5
```

### 2-1. 초기 세대 (Fermi ~ Maxwell)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 19 | 5 Things About Maxwell Architecture | https://developer.nvidia.com/blog/5-things-you-should-know-about-new-maxwell-gpu-architecture/ | SM 효율 40% 향상, 공유 메모리 분리 |
| 20 | Maxwell: Most Advanced CUDA GPU | https://developer.nvidia.com/blog/maxwell-most-advanced-cuda-gpu-ever-made/ | Kepler 대비 2배 효율, Dynamic Parallelism |
| 21 | Maxwell Architecture Page | https://developer.nvidia.com/maxwell-compute-architecture | Maxwell 공식 개요 |
| 22 | GPU Pro Tip: Fast Histograms (Maxwell) | https://developer.nvidia.com/blog/gpu-pro-tip-fast-histograms-using-shared-atomics-maxwell/ | Maxwell 공유 메모리 네이티브 Atomic |

### 2-2. Pascal (2016) — HBM2 & NVLink 최초 도입

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 23 | Inside Pascal Architecture | https://developer.nvidia.com/blog/inside-pascal/ | GP100: 3840 코어, HBM2, NVLink 1세대 |
| 24 | Pascal Architecture Page | https://developer.nvidia.com/pascal | Pascal 공식 개요 |
| 25 | NVLink, Pascal and Stacked Memory | https://developer.nvidia.com/blog/nvlink-pascal-stacked-memory-feeding-appetite-big-data/ | NVLink 최초 해설, HBM2 스택 메모리 |
| 26 | Beyond GPU Memory Limits (Pascal) | https://developer.nvidia.com/blog/beyond-gpu-memory-limits-unified-memory-pascal/ | Pascal의 페이지 폴트 기반 Unified Memory |

### 2-3. Volta (2017) — Tensor Core 최초 도입

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 27 | Volta Tensor Core Performance | https://developer.nvidia.com/blog/tensor-core-ai-performance-milestones/ | V100: 640 Tensor Core, 125 TFLOPS |
| 28 | Programming Tensor Cores in CUDA 9 | https://developer.nvidia.com/blog/programming-tensor-cores-cuda-9/ | Tensor Core 4x4x4 MMA, FP16→FP32 |

### 2-4. Turing (2018) — GDDR6 & RT Core 최초 도입

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 29 | Turing Architecture In-Depth | https://developer.nvidia.com/blog/nvidia-turing-architecture-in-depth/ | 64 FP32+64 INT32 독립경로, RT Core, GDDR6 |

**핵심 스펙:** SM당 64 FP32 + 64 INT32 (독립 실행 → 36% 추가 처리량), 8 Tensor Cores + 1 RT Core per SM, L2 6MB (Pascal 2배), GDDR6 14Gbps

### 2-5. Ampere (2020) — PCIe 4.0 & MIG 도입 ★

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 30 | Ampere Architecture In-Depth | https://developer.nvidia.com/blog/nvidia-ampere-architecture-in-depth/ | A100: 108 SM, HBM2e 2TB/s, PCIe 4.0, MIG |
| 31 | Ampere Compatibility Guide | https://docs.nvidia.com/cuda/ampere-compatibility-guide/ | Ampere 호환성 세부사항 |

**핵심 스펙:** 108 SM, HBM2e 80GB 2TB/s, L2 40MB (캐시 상주 제어), **PCIe Gen 4** (64 GB/s), NVLink 3 600 GB/s, MIG

### 2-6. Hopper (2022) — PCIe 5.0 & TMA 도입 ★★

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 32 | Hopper Architecture In-Depth | https://developer.nvidia.com/blog/nvidia-hopper-architecture-in-depth/ | H100: 132 SM, HBM3 3TB/s, PCIe 5.0, TMA |

**핵심 스펙:** 132 SM (SM당 128 FP32), HBM3 80GB 3TB/s, L2 50MB, Shared Memory SM당 228KB, **PCIe Gen 5** (128 GB/s), **NVLink 4** (900 GB/s, PCIe 5 대비 7배), NVLink Switch (256 GPU, 57.6 TB/s), TMA

### 2-7. Blackwell (2024) — 최신 세대

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 33 | Blackwell Architecture Technical Brief | https://resources.nvidia.com/en-us-blackwell-architecture | 208B 트랜지스터, 듀얼 다이 |
| 34 | RTX Blackwell Whitepaper (PDF) | https://images.nvidia.com/aem-dam/Solutions/geforce/blackwell/nvidia-rtx-blackwell-gpu-architecture.pdf | RTX Blackwell 전체 백서 |
| 35 | Blackwell In-Depth Blog | https://developer.nvidia.com/blog/inside-nvidia-blackwell-ultra-the-chip-powering-the-ai-factory-era/ | Blackwell Ultra 상세 |
| 36 | Blackwell Tuning Guide | https://docs.nvidia.com/cuda/blackwell-tuning-guide/index.html | Blackwell 최적화 가이드 |
| 37 | Blackwell Microbenchmark 분석 | https://arxiv.org/html/2507.10789v2 | SM 파이프라인, Tensor Core FP4/FP6 분석 |

### 2-8. Grace Hopper Superchip — CPU-GPU 통합

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 38 | Grace Hopper Superchip In-Depth | https://developer.nvidia.com/blog/nvidia-grace-hopper-superchip-architecture-in-depth/ | NVLink-C2C 900GB/s, 하드웨어 일관성 |
| 39 | Simplifying GPU Programming (Grace Hopper) | https://developer.nvidia.com/blog/simplifying-gpu-programming-for-hpc-with-the-nvidia-grace-hopper-superchip/ | 코히런트 메모리로 명시적 관리 불필요 |
| 40 | Inside NVIDIA Grace CPU | https://developer.nvidia.com/blog/inside-nvidia-grace-cpu-nvidia-amps-up-superchip-engineering-for-hpc-and-ai/ | Grace CPU 아키텍처 |
| 41 | GH200 NVL32 소개 | https://developer.nvidia.com/blog/one-giant-superchip-for-llms-recommenders-and-gnns-introducing-nvidia-gh200-nvl32/ | 32 GPU 단일 메모리 도메인 |

---

## 3. GPU 메모리 심화 (14개)

### 3-1. Unified Memory

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 42 | Unified Memory for CUDA Beginners | https://developer.nvidia.com/blog/unified-memory-cuda-beginners/ | cudaMallocManaged 기초, 자동 마이그레이션 |
| 43 | Unified Memory in CUDA 6 | https://developer.nvidia.com/blog/unified-memory-in-cuda-6/ | Unified Memory 최초 도입 배경 |
| 44 | Maximizing Unified Memory Performance | https://developer.nvidia.com/blog/maximizing-unified-memory-performance-cuda/ | Prefetch, Access Counter, TLB 동작 |
| 45 | HMM (Heterogeneous Memory Management) | https://developer.nvidia.com/blog/simplifying-gpu-application-development-with-heterogeneous-memory-management/ | PCIe GPU에서 시스템 메모리 직접 접근 |
| 46 | Understanding Memory (CUDA Guide) | https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/understanding-memory.html | GPU 메모리 계층 공식 문서 |

### 3-2. Virtual Memory & 주소 변환

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 47 | Low-Level GPU Virtual Memory Management | https://developer.nvidia.com/blog/introducing-low-level-gpu-virtual-memory-management/ | cuMemCreate/cuMemMap, 가상→물리 매핑 |
| 48 | GPU Memory Oversubscription | https://developer.nvidia.com/blog/improving-gpu-memory-oversubscription-performance/ | GPU 메모리 초과 사용 시 페이지 폴트 |

### 3-3. 메모리 할당 & 풀

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 49 | Stream-Ordered Memory Allocator Part 1 | https://developer.nvidia.com/blog/using-cuda-stream-ordered-memory-allocator-part-1/ | cudaMallocAsync/cudaFreeAsync |
| 50 | Stream-Ordered Memory Allocator Part 2 | https://developer.nvidia.com/blog/using-cuda-stream-ordered-memory-allocator-part-2/ | 메모리 풀 재사용, 2-5x 성능 개선 |
| 51 | CUDA 11.2 Memory Allocation Features | https://developer.nvidia.com/blog/enhancing-memory-allocation-with-new-cuda-11-2-features/ | 비동기 메모리 할당, 디프래그 |
| 52 | RAPIDS Memory Manager (RMM) | https://developer.nvidia.com/blog/fast-flexible-allocation-for-cuda-with-rapids-memory-manager/ | 커스텀 GPU 메모리 풀 관리 |

### 3-4. 메모리 접근 패턴 & 캐시

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 53 | Using Shared Memory in CUDA | https://developer.nvidia.com/blog/using-shared-memory-cuda-cc/ | Bank Conflict, Coalescing, Shared Memory 활용 |
| 54 | Global Memory Access Optimization | https://developer.nvidia.com/blog/unlock-gpu-performance-global-memory-access-in-cuda/ | 32-byte 트랜잭션, Coalesced vs Strided |
| 55 | Register Spilling to Shared Memory | https://developer.nvidia.com/blog/how-to-improve-cuda-kernel-performance-with-shared-memory-register-spilling/ | CUDA 13.0 레지스터 스필 최적화 |

---

## 4. GPU 실행 모델 & 동기화 (10개)

### 4-1. Warp & Thread 실행

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 56 | Using CUDA Warp-Level Primitives | https://developer.nvidia.com/blog/using-cuda-warp-level-primitives/ | __shfl_sync, __ballot_sync, SIMT 모델 |
| 57 | Cooperative Groups | https://developer.nvidia.com/blog/cooperative-groups/ | 유연한 스레드 그룹 프로그래밍, 그리드 동기화 |
| 58 | CUDA Dynamic Parallelism API | https://developer.nvidia.com/blog/cuda-dynamic-parallelism-api-principles/ | GPU 커널 내부에서 커널 발사 |
| 59 | Voting and Shuffling for Atomic Ops | https://developer.nvidia.com/blog/voting-and-shuffling-optimize-atomic-operations/ | Warp 수준 Atomic 최적화 |

### 4-2. Streams & 비동기 실행

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 60 | CUDA 7 Streams Simplify Concurrency | https://developer.nvidia.com/blog/gpu-pro-tip-cuda-7-streams-simplify-concurrency/ | Per-thread default stream, 동시 실행 |
| 61 | How to Overlap Data Transfers | https://developer.nvidia.com/blog/how-overlap-data-transfers-cuda-cc/ | 커널 실행과 데이터 전송 오버랩 |

### 4-3. 데이터 전송 & DMA

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 62 | How to Optimize Data Transfers | https://developer.nvidia.com/blog/how-optimize-data-transfers-cuda-cc/ | Pinned Memory, DMA, PCIe 대역폭 |
| 63 | Controlling Data Movement (Ampere) | https://developer.nvidia.com/blog/controlling-data-movement-to-boost-performance-on-ampere-architecture/ | Async Copy, cuda::memcpy_async |

### 4-4. Tensor Core 프로그래밍

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 64 | Tips for Optimizing Tensor Cores | https://developer.nvidia.com/blog/optimizing-gpu-performance-tensor-cores/ | Tensor Core 최적화 실전 가이드 |
| 65 | Accelerating AI Training with TF32 | https://developer.nvidia.com/blog/accelerating-ai-training-with-tf32-tensor-cores/ | Ampere TF32 정밀도 |

---

## 5. GPU-NVMe 연결 핵심 — GPUDirect Storage (16개) ★★★

### 5-1. GPUDirect 기술 전체

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 66 | GPUDirect 전체 개요 | https://developer.nvidia.com/gpudirect | GPUDirect P2P, RDMA, Storage, Video |
| 67 | GPUDirect RDMA 문서 | https://docs.nvidia.com/cuda/gpudirect-rdma/ | GPU BAR 노출, PCIe P2P 메커니즘 |
| 68 | Benchmarking GPUDirect RDMA | https://developer.nvidia.com/blog/benchmarking-gpudirect-rdma-on-modern-server-platforms/ | PCIe 토폴로지별 RDMA 성능 측정 |
| 69 | GPU-Accelerated RDMA (DOCA GPUNetIO) | https://developer.nvidia.com/blog/unlocking-gpu-accelerated-rdma-with-nvidia-doca-gpunetio/ | GPU 커널이 NIC와 직접 통신 |

### 5-2. GPUDirect Storage

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 70 | GPUDirect Storage 메인 | https://developer.nvidia.com/gpudirect-storage | GDS 제품 페이지 |
| 71 | GDS Blog (핵심 해설) | https://developer.nvidia.com/blog/gpudirect-storage/ | CPU Bounce Buffer 제거, DMA 직접 전송 |
| 72 | GDS Overview Guide | https://docs.nvidia.com/gpudirect-storage/overview-guide/index.html | GDS 전체 아키텍처 |
| 73 | GDS Design Guide | https://docs.nvidia.com/gpudirect-storage/design-guide/index.html | PCIe 토폴로지 설계 지침 |
| 74 | GDS Configuration Guide | https://docs.nvidia.com/gpudirect-storage/configuration-guide/index.html | 벤치마크, gdsio 도구 |
| 75 | GDS Best Practices Guide | https://docs.nvidia.com/gpudirect-storage/best-practices-guide/index.html | ACS 비활성화, 정렬 요구사항 |
| 76 | GDS Troubleshooting Guide | https://docs.nvidia.com/gpudirect-storage/troubleshooting-guide/index.html | 문제 해결, 성능 디버깅 |
| 77 | GDS cuFile API Reference | https://docs.nvidia.com/gpudirect-storage/api-reference-guide/index.html | cuFileRead/Write API 상세 |

### 5-3. PCIe P2P DMA & Linux 커널

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 78 | Linux Kernel PCI P2PDMA 문서 | https://docs.kernel.org/driver-api/pci/p2pdma.html | P2PDMA 커널 프레임워크 |
| 79 | GDS Data Pipeline Blog | https://developer.nvidia.com/blog/gpudirect-storage-moving-data-pipeline-into-the-fast-lane/ | GDS 파이프라인 실전 활용 |
| 80 | GDS + RAPIDS cuDF Throughput | https://developer.nvidia.com/blog/boosting-data-ingest-throughput-with-gpudirect-storage-and-rapids-cudf/ | GDS 기반 데이터 로딩 8x 속도 |
| 81 | GPU I/O on Platforms of Today/Tomorrow | https://developer.nvidia.com/blog/making-gpu-i-o-scream-on-platforms-of-today-and-tomorrow/ | GPU I/O 극한 성능 추구 |

### 5-4. 전통적 I/O vs GPUDirect Storage

```
[전통적 방식 — CPU Bounce Buffer]

  NVMe SSD ──PCIe──→ CPU Memory (bounce buffer) ──PCIe──→ GPU Memory
                         ↑
                    CPU가 복사 관리
                    (대역폭 낭비 + 지연 증가)


[GPUDirect Storage — Direct DMA]

  NVMe SSD ──────────── PCIe ──────────────→ GPU Memory
                    ↑
               DMA 엔진이 직접 전송
               CPU 개입 없음
               (대역폭 절약 + 지연 감소)
```

**GDS 핵심 구현:**
1. **cuFile API** — POSIX pread/pwrite와 유사, GPU 메모리 주소로 직접 I/O
2. **nvidia-fs.ko** — GPU 가상 주소를 물리 주소로 변환하는 커널 드라이버
3. **DMA 엔진** — 스토리지 컨트롤러가 GPU BAR를 통해 직접 DMA
4. **CUDA 12.8+** — upstream kernel PCI P2PDMA 활용 (nvidia-fs.ko 의존성 제거)

---

## 6. GPU 인터커넥트: NVLink, NVSwitch, PCIe (12개)

### 6-1. NVLink & NVSwitch

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 82 | How NVLink Enables Multi-GPU | https://developer.nvidia.com/blog/how-nvlink-will-enable-faster-easier-multi-gpu-computing/ | NVLink 1세대 소개 |
| 83 | NVSwitch: Leveraging NVLink | https://developer.nvidia.com/blog/nvswitch-leveraging-nvlink-to-maximum-effect/ | NVSwitch 비차단(non-blocking) 설계 |
| 84 | NVSwitch Accelerates DGX-2 | https://developer.nvidia.com/blog/nvswitch-accelerates-nvidia-dgx2/ | 16 GPU 풀메시 연결 |
| 85 | 3rd Gen NVSwitch (Hopper) | https://developer.nvidia.com/blog/?p=53977 | NVSwitch 3세대, 900 GB/s |
| 86 | NVLink/NVSwitch for LLM Inference | https://developer.nvidia.com/blog/nvidia-nvlink-and-nvidia-nvswitch-supercharge-large-language-model-inference/ | LLM에서의 GPU 간 통신 최적화 |
| 87 | GB200 NVL72 System | https://developer.nvidia.com/blog/upgrading-multi-gpu-interconnectivity-with-the-third-generation-nvidia-nvswitch/ | 72 GPU NVLink 도메인 |

### 6-2. PCIe BAR & Resizable BAR

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 88 | PCI BARs (envytools 문서) | https://envytools.readthedocs.io/en/latest/hw/bus/bars.html | NVIDIA GPU BAR0(제어)/BAR1(VRAM) 매핑 |
| 89 | GPU Virtualization: PCIe & MMIO 소개 | https://topofmind.dev/blog/2026/02/15/gpu-virtualization-part-1-an-introduction-to-pcie-and-mmio/ | GPU PCIe BAR, MMIO, Doorbell 레지스터 |
| 90 | Resizable BAR (NVIDIA GeForce) | https://www.nvidia.com/en-us/geforce/news/geforce-rtx-30-series-resizable-bar-support/ | PCIe ReBAR로 전체 VRAM 접근 (256MB→전체) |

### 6-3. NCCL (Collective Communication)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 91 | NCCL 메인 페이지 | https://developer.nvidia.com/nccl | AllReduce, PCIe/NVLink 최적화 |
| 92 | Fast Multi-GPU Collectives with NCCL | https://developer.nvidia.com/blog/fast-multi-gpu-collectives-nccl/ | Ring AllReduce, 토폴로지 인식 |
| 93 | NCCL Deep Dive: Cross Data Center | https://developer.nvidia.com/blog/nccl-deep-dive-cross-data-center-communication-and-network-topology-awareness | 네트워크 토폴로지 인식 통신 |

---

## 7. 시스템 아키텍처 & 가상화 (12개)

### 7-1. DGX 시스템 아키텍처

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 94 | DGX-1 System Architecture | https://developer.nvidia.com/blog/dgx-1-fastest-deep-learning-system/ | 8x V100, NVLink 하이브리드 메시 |
| 95 | DGX A100 System Architecture | https://developer.nvidia.com/blog/defining-ai-innovation-with-dgx-a100/ | 8x A100, PCIe 4.0, 15TB NVMe |
| 96 | DGX GH200 (100TB GPU Memory) | https://developer.nvidia.com/blog/announcing-nvidia-dgx-gh200-first-100-terabyte-gpu-memory-system/ | 256 GPU NVLink 도메인, 1 Exaflop |

### 7-2. Multi-Instance GPU (MIG)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 97 | Getting Most Out of A100 with MIG | https://developer.nvidia.com/blog/getting-the-most-out-of-the-a100-gpu-with-multi-instance-gpu/ | A100 최대 7개 인스턴스 분할 |
| 98 | MIG + NUMA Node Localization | https://developer.nvidia.com/blog/accelerating-data-processing-with-nvidia-multi-instance-gpu-and-numa-node-localization | MIG + PCIe NUMA 최적화 |

### 7-3. GPU 가상화

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 99 | NVIDIA vGPU 19.0 (Blackwell) | https://developer.nvidia.com/blog/nvidia-vgpu-19-0-enables-graphics-and-ai-virtualization-on-nvidia-blackwell-gpus/ | SR-IOV + MIG로 최대 48 VM |
| 100 | DGX-2 Server Virtualization | https://developer.nvidia.com/blog/dgx2-server-virtualization-nvswitch-faster-gpu-virtual-machines/ | NVSwitch 기반 GPU 가상화 |

### 7-4. BlueField DPU & 스토리지 오프로드

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 101 | BlueField DPU Workload Offloading | https://developer.nvidia.com/blog/offloading-and-isolating-data-center-workloads-with-bluefield-dpu/ | NVMe-oF, RDMA 스토리지 오프로드 |
| 102 | BlueField-3 + DDN Storage | https://developer.nvidia.com/blog/accelerate-ai-infrastructure-using-an-nvidia-bluefield-3-dpu-integration-with-ddn-storage/ | DPU 기반 AI 스토리지 가속 |

### 7-5. DPDK + GPU 패킷 처리

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 103 | DPDK GPUdev Inline Packet Processing | https://developer.nvidia.com/blog/optimizing-inline-packet-processing-using-dpdk-and-gpudev-with-gpus/ | DPDK + GPU 직접 패킷 처리 |
| 104 | DOCA GPUNetIO Inline Processing | https://developer.nvidia.com/blog/inline-gpu-packet-processing-with-nvidia-doca-gpunetio/ | GPU 커널이 NIC 직접 제어, CPU 바이패스 |
| 105 | DPDK 메인 페이지 | https://developer.nvidia.com/networking/dpdk | NVIDIA DPDK 공식 |

---

## 8. 성능 분석 & 최적화 도구 (12개)

### 8-1. Nsight 프로파일링 도구

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 106 | Nsight Compute 메인 | https://developer.nvidia.com/nsight-compute | 커널 수준 성능 분석, 메모리 활용 |
| 107 | Nsight Systems 메인 | https://developer.nvidia.com/nsight-systems | 시스템 전체 GPU/CPU/네트워크 프로파일링 |
| 108 | Nsight Systems GPU Optimization | https://developer.nvidia.com/blog/nsight-systems-exposes-gpu-optimization/ | GPU 최적화 기회 발견 |
| 109 | Analysis-Driven Optimization Part 1 | https://developer.nvidia.com/blog/analysis-driven-optimization-preparing-for-analysis-with-nvidia-nsight-compute-part-1/ | Nsight Compute 분석 워크플로우 |
| 110 | Analysis-Driven Optimization Part 2 | https://developer.nvidia.com/blog/analysis-driven-optimization-analyzing-and-improving-performance-with-nvidia-nsight-compute-part-2/ | 실전 성능 개선 사례 |

### 8-2. Magnum IO (통합 I/O 프레임워크)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 111 | Magnum IO SDK | https://developer.nvidia.com/magnum-io | GPU-Storage-Network 통합 I/O |
| 112 | Magnum IO Architecture | https://developer.nvidia.com/blog/accelerating-io-in-the-modern-data-center-magnum-io-architecture/ | 데이터센터 I/O 아키텍처 |
| 113 | Magnum IO Storage | https://developer.nvidia.com/blog/accelerating-io-in-the-modern-data-center-magnum-io-storage/ | 스토리지 I/O 가속 |
| 114 | Magnum IO Network IO | https://developer.nvidia.com/blog/accelerating-io-in-the-modern-data-center-network-io/ | 네트워크 I/O 가속 |
| 115 | Magnum IO Data Movement | https://developer.nvidia.com/blog/optimizing-data-movement-in-gpu-apps-with-magnum-io-developer-environment/ | GPU 앱 데이터 이동 최적화 |

### 8-3. 전력 & 성능 관리

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 116 | GPU Boost and K80 Autoboost | https://developer.nvidia.com/blog/increase-performance-gpu-boost-k80-autoboost/ | GPU 클럭 부스트 메커니즘 |
| 117 | GPU Energy & Power Efficiency | https://developer.nvidia.com/blog/maximizing-energy-and-power-efficiency-in-applications-with-nvidia-gpus/ | 주파수 스케일링, 전력 프로파일 |

### 8-4. AI 데이터 파이프라인

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| 118 | NVIDIA DALI (Data Loading Library) | https://developer.nvidia.com/dali | GPU 기반 데이터 전처리 가속 |
| 119 | Scaling Storage for AI Training | https://developer.nvidia.com/blog/tips-on-scaling-storage-for-ai-training-and-inferencing/ | AI 학습용 스토리지 확장 전략 |
| 120 | Data Loading Bottlenecks | https://developer.nvidia.com/blog/machine-learning-frameworks-interoperability-part-2-data-loading-and-data-transfer-bottlenecks/ | CPU 데이터 로딩 병목 분석 |

---

## 보너스: 외부 학습 리소스 (비공식이지만 유용)

| # | 리소스 | URL | 핵심 |
|---|--------|-----|------|
| B1 | GPU Architecture (D2L Compiler) | https://tvm.d2l.ai/chapter_gpu_schedules/arch.html | SM, 메모리 계층 교육용 |
| B2 | Demystifying GPU Architectures | https://learnopencv.com/demystifying-gpu-architectures-for-deep-learning/ | 딥러닝 관점 GPU 해설 |
| B3 | SM 구조 상세 해설 | https://www.abhik.ai/concepts/gpu/shared-multiprocessor | Warp Scheduler 시각화 |
| B4 | Intro to GPUs (Modular) | https://docs.modular.com/mojo/manual/gpu/architecture/ | 최신 GPU 개념 정리 |
| B5 | GPU Gems — GPGPU Primer | https://developer.nvidia.com/gpugems/gpugems2/part-iv-general-purpose-computation-gpus-primer | GPU 범용 컴퓨팅의 역사 |
| B6 | P2P DMA ssd-gpu-dma (GitHub) | https://github.com/enfiskutensykkel/ssd-gpu-dma | NVMe-GPU 직접 DMA 오픈소스 |
| B7 | p2pmem-test (GitHub) | https://github.com/sbates130272/p2pmem-test | P2PDMA 테스트 프레임워크 |
| B8 | PCIe Part 2: MMIO, DMA, TLPs | https://ctf.re/kernel/pcie/tutorial/dma/mmio/tlp/2024/03/26/pcie-part-2/ | PCIe 패킷 구조 심화 |
| B9 | SPIN: P2P DMA OS Integration | https://dl.acm.org/doi/fullHtml/10.1145/3309987 | GPU-NVMe P2P DMA OS 통합 논문 |
| B10 | NVMe CMB/PMR Ecosystem (NVM Express) | https://nvmexpress.org/wp-content/uploads/Enabling-the-NVMe-CMB-and-PMR-Ecosystem.pdf | NVMe Controller Memory Buffer |

---

## PCIe 세대별 대역폭과 GPU/NVMe 대비

```
PCIe Gen │ x1 속도  │ x4 (NVMe)  │ x16 (GPU)  │ NVMe/GPU 비율
─────────┼──────────┼────────────┼────────────┼──────────────
Gen 3    │ 0.98 GB/s│  3.94 GB/s │  15.75 GB/s│  25%
Gen 4    │ 1.97 GB/s│  7.88 GB/s │  31.51 GB/s│  25%
Gen 5    │ 3.94 GB/s│ 15.75 GB/s │  63.02 GB/s│  25%
Gen 6    │ 7.56 GB/s│ 30.25 GB/s │ 121.0 GB/s │  25%
```

**핵심 인사이트:** NVMe x4는 항상 GPU x16 대역폭의 25%. 이 구조적 병목을 해결하는 전략:
- 여러 NVMe를 동시에 사용 (4~8개로 GPU 대역폭에 근접)
- NVMe-oF (네트워크 경유 원격 스토리지 풀)
- CPU bounce buffer 제거 (GPUDirect Storage)
- GPU-initiated I/O (BaM 등 — GPU가 직접 NVMe 제어)

---

## 추천 학습 순서

```
Phase 1: GPU 기초 (1~2주)
├── #6 An Even Easier Introduction to CUDA → 실습
├── #1 CUDA Programming Guide → SM, Warp, Memory 개념
├── #53 Shared Memory 활용 → #54 Global Memory 접근 패턴
└── #16 Occupancy API → 최적 블록 크기

Phase 2: 아키텍처 진화 (1주)
├── #23 Pascal → #27 Volta → #29 Turing (역사 흐름)
├── #30 Ampere → #32 Hopper (PCIe 4.0→5.0 전환점)
├── #33-37 Blackwell (최신)
└── #82-87 NVLink/NVSwitch 진화 추적

Phase 3: GPU 메모리 심화 (1주)
├── #42-44 Unified Memory (기초→최적화)
├── #47 Virtual Memory Management
├── #62-63 Data Transfer 최적화
└── #45 HMM → GDS 개념 연결

Phase 4: GPU-NVMe 연결 (1~2주) ★핵심
├── #66-69 GPUDirect 전체 이해
├── #70-77 GDS 문서 전체 (Overview→Design→API)
├── #78-81 P2P DMA, Linux 커널 연동
└── #88-90 PCIe BAR, Resizable BAR

Phase 5: 시스템 & 응용 (1주)
├── #94-96 DGX 시스템 토폴로지
├── #101-105 DPU, DPDK, GPU 패킷 처리
├── #111-115 Magnum IO 통합 프레임워크
└── papers/README.md 논문 읽기 순서와 병행
```

---

## 이 프로젝트와의 연결점

| 기존 노트 | GPU 리소스와의 연결 |
|-----------|---------------------|
| `notes/kernel/nvme-driver.md` | NVMe 드라이버가 PCIe BAR를 통해 SQ/CQ에 접근하는 방식 → GPU도 PCIe BAR로 자신의 메모리를 노출 (#88, #89) |
| `notes/kernel/spdk.md` | SPDK가 kernel bypass하는 것처럼 → GPUDirect Storage도 CPU bounce buffer를 bypass (#70-77) |
| `analysis/bam/` | BaM은 GPU가 직접 NVMe SQ/CQ를 제어 → CUDA Programming Guide의 메모리 모델 이해 필요 (#1, #46) |
| `notes/gpu-storage/experiment-design.md` | PCIe 토폴로지 실험 → GDS Configuration Guide의 토폴로지 요구사항과 직결 (#73-75) |
| `papers/` 논문 | P2P DMA 관련 논문들 → Linux P2PDMA 문서 (#78), ssd-gpu-dma (#B6), SPIN (#B9) |
