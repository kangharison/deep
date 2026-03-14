# NVIDIA Developer 학습 리소스 가이드

GPU 아키텍처를 깊이 이해하고, PCIe 기반 NVMe 스토리지와의 상호작용을 파악하기 위한 학습 리소스를 단계별로 정리한다.

---

## 1단계: GPU 아키텍처 기초

GPU의 내부 구조를 이해하는 것이 모든 학습의 출발점이다.

### 1-1. CUDA Programming Guide (필독)

GPU 아키텍처의 공식 교과서. SM 구조, 메모리 계층, 스레드 실행 모델을 체계적으로 설명한다.

| 리소스 | URL |
|--------|-----|
| CUDA C++ Programming Guide | https://docs.nvidia.com/cuda/cuda-c-programming-guide/ |
| CUDA Programming Guide (신규) | https://docs.nvidia.com/cuda/cuda-programming-guide/index.html |
| CUDA Best Practices Guide | https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/ |
| An Even Easier Introduction to CUDA | https://developer.nvidia.com/blog/even-easier-introduction-cuda/ |
| CUDA Refresher: Programming Model | https://developer.nvidia.com/blog/cuda-refresher-cuda-programming-model/ |

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
│  │  │ │     │ │  │ │     │ │  │ │     │ │      │  │
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

### 1-2. GPU Memory 이해

| 리소스 | URL |
|--------|-----|
| Understanding Memory (CUDA Guide) | https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/understanding-memory.html |
| Heterogeneous Memory Management (HMM) | https://developer.nvidia.com/blog/simplifying-gpu-application-development-with-heterogeneous-memory-management/ |

**핵심:** HMM은 PCIe 연결 GPU에서 GPU 스레드가 시스템 메모리에 직접 접근할 수 있게 하는 기능이다. 명시적 메모리 관리 없이도 GPU 프로그래밍이 가능해진다. 이것이 GPUDirect Storage로 확장되는 개념적 기반이다.

---

## 2단계: 세대별 GPU 아키텍처 진화 (Whitepaper)

각 세대의 아키텍처 백서는 GPU 설계 철학의 변화를 이해하는 데 핵심이다. 특히 PCIe 세대 변화, 메모리 대역폭 진화, NVLink 등장에 주목해야 한다.

### 아키텍처 세대별 진화 요약

```
Pascal(2016) → Volta(2017) → Turing(2018) → Ampere(2020) → Hopper(2022) → Blackwell(2024)
  PCIe 3.0      PCIe 3.0      PCIe 3.0       PCIe 4.0      PCIe 5.0       PCIe 5.0
  NVLink 1      NVLink 2      (없음)         NVLink 3      NVLink 4       NVLink 5
  GDDR5X        HBM2          GDDR6          HBM2e         HBM3           HBM3e
  Tensor X      Tensor 1      Tensor 2       Tensor 3      Tensor 4       Tensor 5
```

### 2-1. Turing Architecture (2018) — GDDR6 최초 도입

| 리소스 | URL |
|--------|-----|
| Turing Architecture In-Depth (Blog) | https://developer.nvidia.com/blog/nvidia-turing-architecture-in-depth/ |

**핵심 스펙:**
- SM당 64 FP32 + 64 INT32 코어 (독립 실행 경로 → 36% 추가 처리량)
- 8 Tensor Cores + 1 RT Core per SM
- 96 KB unified L1/Shared Memory (크기 비율 동적 조정)
- L2 캐시 6 MB (Pascal 대비 2배)
- GDDR6 최초 도입 (14 Gbps, GDDR5X 대비 20% 전력 효율 향상)
- 메모리 압축 알고리즘 → 실효 대역폭 50% 향상

**NVMe 학습 관점:** Turing 세대는 PCIe 3.0 x16 (32 GB/s) 유지. 이 대역폭에서 NVMe SSD (PCIe 3.0 x4 = 3.9 GB/s)와 GPU의 대역폭 비율 파악이 중요.

### 2-2. Ampere Architecture (2020) — PCIe 4.0 도입

| 리소스 | URL |
|--------|-----|
| Ampere Architecture In-Depth (Blog) | https://developer.nvidia.com/blog/nvidia-ampere-architecture-in-depth/ |
| Ampere Tuning Guide | https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/ |
| Ampere Compatibility Guide | https://docs.nvidia.com/cuda/ampere-compatibility-guide/ |

**핵심 스펙 (A100):**
- 108 SM, SM당 64 FP32 코어 (6,912 총)
- 3세대 Tensor Cores (TF32 지원)
- HBM2e: 80 GB, 2 TB/s 대역폭 (V100 대비 1.7배)
- L2 캐시 40 MB, 캐시 상주 제어(residency control) 도입
- **PCIe Gen 4 도입 (64 GB/s, Gen 3 대비 2배)**
- NVLink 3: 600 GB/s
- Multi-Instance GPU (MIG): GPU 분할 사용

**NVMe 학습 관점:** PCIe 4.0으로 전환되면서 NVMe SSD도 PCIe 4.0 (x4 = 7.88 GB/s)으로 전환. GPU-NVMe 간 대역폭이 2배 증가. GPUDirect Storage가 이 세대부터 본격화.

### 2-3. Hopper Architecture (2022) — PCIe 5.0 도입 (★중요)

| 리소스 | URL |
|--------|-----|
| Hopper Architecture In-Depth (Blog) | https://developer.nvidia.com/blog/nvidia-hopper-architecture-in-depth/ |

**핵심 스펙 (H100 SXM5):**
- **132 SM**, SM당 128 FP32 코어 (16,896 총)
- 4세대 Tensor Cores (FP8 지원, A100 대비 4배 MMA 성능)
- **HBM3: 80 GB, 3 TB/s 대역폭 (A100 대비 2배)**
- L2 캐시 50 MB
- Shared Memory: SM당 최대 228 KB (설정 가능)
- **PCIe Gen 5: 128 GB/s (Gen 4 대비 2배)**
- **NVLink 4: 900 GB/s (18 링크), PCIe 5 대비 7배 빠름**
- NVLink Switch: 최대 256 GPU 연결, 57.6 TB/s
- TMA (Tensor Memory Accelerator): 글로벌↔Shared Memory 간 대형 블록 전송 가속
- Thread Block Clusters: 여러 SM 걸친 협업 데이터 교환

**NVMe 학습 관점:** PCIe 5.0 x4 NVMe = 15.75 GB/s. H100의 PCIe 5.0 x16 = 128 GB/s이므로, 이론적으로 NVMe 8개를 동시에 GPU에 직결할 수 있다. GPUDirect Storage의 실질적 활용 가능 지점.

### 2-4. Blackwell Architecture (2024) — 최신 세대

| 리소스 | URL |
|--------|-----|
| Blackwell Architecture Technical Brief | https://resources.nvidia.com/en-us-blackwell-architecture |
| RTX Blackwell Whitepaper (PDF) | https://images.nvidia.com/aem-dam/Solutions/geforce/blackwell/nvidia-rtx-blackwell-gpu-architecture.pdf |
| Blackwell In-Depth Blog | https://developer.nvidia.com/blog/inside-nvidia-blackwell-ultra-the-chip-powering-the-ai-factory-era/ |
| Blackwell Tuning Guide | https://docs.nvidia.com/cuda/blackwell-tuning-guide/index.html |
| Blackwell Microbenchmark 분석 | https://arxiv.org/html/2507.10789v2 |

**핵심 스펙:**
- 208 billion 트랜지스터, TSMC 4NP
- 2개의 reticle-limited die → 10 TB/s chip-to-chip interconnect로 연결
- 5세대 Tensor Cores (FP4, FP6 지원)
- HBM3e, NVLink 5
- PCIe 5.0 유지

---

## 3단계: GPU-NVMe 연결 핵심 — GPUDirect Storage (★★★)

GPU 아키텍처 이해 후, 가장 중요한 응용 분야가 GPUDirect Storage이다. 이것은 GPU와 NVMe 스토리지를 PCIe를 통해 직접 연결하는 기술이다.

### 3-1. GPUDirect 기술 개요

| 리소스 | URL |
|--------|-----|
| GPUDirect 전체 개요 | https://developer.nvidia.com/gpudirect |
| GPUDirect Storage 메인 | https://developer.nvidia.com/gpudirect-storage |
| GDS Overview Guide | https://docs.nvidia.com/gpudirect-storage/overview-guide/index.html |
| GDS Design Guide | https://docs.nvidia.com/gpudirect-storage/design-guide/index.html |
| GDS Configuration Guide | https://docs.nvidia.com/gpudirect-storage/configuration-guide/index.html |
| GDS Blog (핵심 해설) | https://developer.nvidia.com/blog/gpudirect-storage/ |

### 3-2. 전통적 I/O vs GPUDirect Storage

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

### 3-3. PCIe 토폴로지와 성능 영향

GPUDirect Storage 성능은 PCIe 토폴로지에 크게 좌우된다:

```
[DGX A100 시스템 예시 — PCIe 트리 1개]

         CPU Socket
            │
       PCIe Root Complex
            │
       PCIe Switch
      ╱    │     ╲
    GPU   GPU   NVMe  NVMe
     0     1    SSD0  SSD1
```

**라우팅 모드:**
- `GPU_MEM_NVLINKS`: GPU 간 NVLink 사용 (최고 성능)
- `GPU_MEM`: GPU 간 PCIe P2P
- `SYS_MEM`: GPU와 NVMe가 다른 PCIe 루트에 있을 때 시스템 메모리 경유
- `P2P`: PCIe 직접 전송 (같은 PCIe 트리에 있을 때 최적)

### 3-4. GDS 성능 수치

| 경로 | 대역폭 |
|------|--------|
| NVMe (로컬, DGX-2) | 53.3 GB/s (4경로) |
| 원격 RAID | 112 GB/s (8경로) |
| NIC (NVMe-oF) | 84 GB/s (8경로) |
| 모든 경로 합산 | ~215 GB/s |

**개선 효과:**
- CSV 데이터 로딩: 2x-8x 처리량 향상
- 명시적 전송 시 end-to-end 지연: 3.8x 감소
- TPC-H 쿼리 벤치마크: 6.7x-32.8x 속도 향상

### 3-5. 기술 구현 세부사항

GDS의 핵심 구현:
1. **cuFile API** — POSIX와 유사한 인터페이스로 GPU-Storage 직접 전송
2. **nvidia-fs.ko** — GPU 가상 주소를 물리 주소로 변환하는 커널 드라이버
3. **DMA 엔진 프로그래밍** — 스토리지 컨트롤러가 GPU 메모리 주소로 직접 DMA 수행
4. **CUDA 12.8+** — NVMe용 upstream kernel PCI P2PDMA 지원 (nvidia-fs.ko 의존성 제거)

---

## 4단계: PCIe와 GPU-NVMe 대역폭 관계 정리

### PCIe 세대별 대역폭과 GPU/NVMe 대비

```
PCIe Gen │ x1 속도  │ x4 (NVMe)  │ x16 (GPU)  │ NVMe/GPU 비율
─────────┼──────────┼────────────┼────────────┼──────────────
Gen 3    │ 0.98 GB/s│  3.94 GB/s │  15.75 GB/s│  25%
Gen 4    │ 1.97 GB/s│  7.88 GB/s │  31.51 GB/s│  25%
Gen 5    │ 3.94 GB/s│ 15.75 GB/s │  63.02 GB/s│  25%
Gen 6    │ 7.56 GB/s│ 30.25 GB/s │ 121.0 GB/s │  25%
```

**핵심 인사이트:** NVMe x4는 항상 GPU x16 대역폭의 25%. 따라서 GPU 연산 대비 스토리지 I/O가 병목이 된다. 이를 해결하기 위한 방법:
- 여러 NVMe를 동시에 사용 (4~8개로 GPU 대역폭에 근접)
- NVMe-oF (네트워크 경유 원격 스토리지 풀)
- CPU bounce buffer 제거 (GPUDirect Storage)
- GPU-initiated I/O (BaM 등 — GPU가 직접 NVMe 제어)

---

## 5단계: 추가 심화 리소스

### 5-1. CUDA Compute Capability 레퍼런스

| 리소스 | URL |
|--------|-----|
| CUDA GPU Compute Capability 목록 | https://developer.nvidia.com/cuda/gpus |
| Architecture Support 가이드 | https://developer.nvidia.com/blog/navigating-gpu-architecture-support-a-guide-for-nvidia-cuda-developers |

### 5-2. GPU Gems (역사적 참고)

| 리소스 | URL |
|--------|-----|
| GPU Gems — GPGPU Primer | https://developer.nvidia.com/gpugems/gpugems2/part-iv-general-purpose-computation-gpus-primer |
| GeForce 6 아키텍처 해설 | https://developer.nvidia.com/gpugems/gpugems2/part-iv-general-purpose-computation-gpus-primer/chapter-30-geforce-6-series-gpu |

### 5-3. 외부 학습 리소스 (비공식이지만 유용)

| 리소스 | URL |
|--------|-----|
| GPU Architecture — D2L Compiler | https://tvm.d2l.ai/chapter_gpu_schedules/arch.html |
| Demystifying GPU Architectures (Part 1) | https://learnopencv.com/demystifying-gpu-architectures-for-deep-learning/ |
| SM 구조 상세 해설 | https://www.abhik.ai/concepts/gpu/shared-multiprocessor |
| Intro to GPUs (Modular) | https://docs.modular.com/mojo/manual/gpu/architecture/ |

---

## 추천 학습 순서

```
Phase 1: GPU 기초 (1~2주)
├── CUDA Programming Guide 통독
├── "An Even Easier Introduction to CUDA" 실습
└── SM, Warp, Memory Hierarchy 개념 정립

Phase 2: 아키텍처 진화 (1주)
├── Ampere In-Depth Blog → Hopper In-Depth Blog 순서로 읽기
├── 각 세대의 PCIe/NVLink 스펙 변화 추적
└── Blackwell Whitepaper로 최신 동향 파악

Phase 3: GPU-NVMe 연결 (1~2주)
├── GPUDirect Storage Blog (핵심 개념)
├── GDS Overview Guide + Design Guide (구현 세부)
├── PCIe 토폴로지와 성능 관계 이해
└── cuFile API 예제 코드 실습

Phase 4: 심화 — 기존 논문 노트와 연결
├── notes/gpu.md의 논문 읽기 순서와 병행
├── BaM (GPU-initiated I/O) 코드 분석
└── SPDK와 GPUDirect Storage 비교
```

---

## 이 프로젝트와의 연결점

| 기존 노트 | GPU 리소스와의 연결 |
|-----------|---------------------|
| `notes/nvme_driver.md` | NVMe 드라이버가 PCIe BAR를 통해 SQ/CQ에 접근하는 방식 → GPU도 PCIe BAR로 자신의 메모리를 노출 |
| `notes/spdk.md` | SPDK가 kernel bypass하는 것처럼 → GPUDirect Storage도 CPU bounce buffer를 bypass |
| `notes/bam.md` | BaM은 GPU가 직접 NVMe SQ/CQ를 제어 → CUDA Programming Guide의 메모리 모델 이해 필요 |
| `notes/gpu-pcie-nvme-experiments.md` | PCIe 토폴로지 실험 → GDS Configuration Guide의 토폴로지 요구사항과 직결 |
