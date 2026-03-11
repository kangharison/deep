CPU Initiated I/O, GPUDirect Storage I/O, GPU Initiated I/O 세 가지 분류로 최근 3개년(2023~2025) 핵심 논문들을 정리하겠습니다.

좋습니다. 검색 결과를 바탕으로 세 가지 I/O 타입별로 논문을 분류해서 정리하겠습니다.

---

## 논문 분류 리딩 리스트

### 분류 1: CPU Initiated I/O (기존 방식 — 기반 이해용)

이 분류는 현재 검증 플랫폼의 근간이 되는 기술이므로, 발표에서 "현재 상태(AS-IS)"를 설명할 때 참조합니다.

**[C1] "Performance Characterization of Modern Storage Stacks: POSIX I/O, libaio, SPDK, and io_uring"** (CHEOPS 2023)
- 핵심: fio 기반으로 POSIX I/O, libaio, io_uring, SPDK 네 가지 I/O 스택의 CPU 효율성과 성능을 체계적으로 비교
- 읽어야 하는 이유: CPU Initiated I/O의 각 계층별 오버헤드를 정량적으로 보여주며, SPDK가 왜 13배 더 효율적인지 설명. GPU Initiated I/O와의 차이를 논할 때 CPU 경로의 한계를 수치로 제시할 수 있음

**[C2] "What Modern NVMe Storage Can Do, And How To Exploit It"** (VLDB 2023)
- 핵심: 최신 NVMe SSD의 성능을 DBMS 워크로드에서 SPDK, io_uring 등으로 측정하고, CPU 코어의 절반이 I/O 처리에 소모됨을 증명
- 읽어야 하는 이유: CPU 기반 I/O의 구조적 병목을 데이터베이스 관점에서 실증. "CPU가 I/O 처리에 소모되면 연산에 쓸 코어가 부족해진다"는 논점이 GPU Initiated I/O 도입의 동기와 직결됨

**[C3] "SPDK: A Development Kit to Build High Performance Storage Applications"** (CloudCom 2017, 기초 참조)
- 핵심: SPDK의 설계 철학. 유저스페이스 드라이버, 폴링 모드, 락프리 구조로 커널 오버헤드 제거
- 읽어야 하는 이유: CPU Initiated I/O의 최적 형태인 SPDK의 구조를 이해해야 GPU Initiated I/O(BaM)가 이를 어떻게 GPU로 확장했는지 설명 가능

---

### 분류 2: GPUDirect Storage I/O (중간 단계 — CPU가 발행, DMA만 P2P)

이 분류는 "GPU P2P 경로를 사용하지만 I/O 발행은 여전히 CPU"인 중간 단계입니다. GPU Initiated I/O와의 차이를 명확히 하는 데 핵심입니다.

**[G1] "Quantifying Performance Gains of GPUDirect Storage"** (IEEE 2022/2023)
- 핵심: GDS의 성능 이점을 정량적으로 측정. CPU 경유 경로 대비 GDS의 throughput 향상과 latency 감소를 벤치마크
- 읽어야 하는 이유: GDS의 구조(CPU가 cuFile API로 I/O 발행, DMA 경로만 GPU VRAM으로 변경)를 이해하고, GPU Initiated I/O와의 구조적 차이를 명확히 구분하는 데 필수

**[G2] "Insights into GPUDirect Data Transfer through NIXL Benchmarking"** (2025)
- 핵심: GDS와 NVIDIA NIXL 라이브러리를 비교 벤치마크. gdsio를 사용한 다양한 I/O 크기, 배치 크기에서의 성능 분석
- 읽어야 하는 이유: 가장 최근의 GDS 성능 데이터를 포함하며, GDS의 한계(배치 크기가 성능에 영향을 주지 않는 등)를 실증

**[G3] "Accelerating AI With High Performance Storage" (Solidigm/NVIDIA 백서, 2024~2025)**
- 핵심: GPUDirect Storage + NVMe-oF + DPU 조합으로 원격 스토리지에서도 로컬 NVMe 수준의 성능 달성
- 읽어야 하는 이유: GDS가 실제 AI 인프라에서 어떻게 배포되는지의 production 사례. 발표에서 "실사용 환경" 설명에 활용

**[G4] Micron 9400 NVMe SSD Performance with GPUDirect Storage (백서, 2023)**
- 핵심: GDS 활성화 시 legacy I/O 대비 최대 1.5배 성능 향상과 1.5배 응답 시간 개선을 실측
- 읽어야 하는 이유: 특정 SSD 제품에서 GDS의 실질적 성능 차이를 보여주는 벤더 데이터

---

### 분류 3: GPU Initiated I/O (GPU가 직접 NVMe에 커맨드 발행)

이 분류가 발표의 핵심이며, 본 과제의 직접적 기반입니다.

**[I1] "GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture"** (ASPLOS 2023) — **필독**
- 저자: Zaid Qureshi, Vikram Sharma Mailthody, Isaac Gelado 외 (UIUC, NVIDIA, IBM)
- 핵심: GPU 커널이 NVMe SQ에 직접 커맨드를 작성하고 doorbell을 발행하는 완전한 GPU Initiated I/O 구현. GPU VRAM에 NVMe 큐와 I/O 버퍼를 배치하고, GPUDirect RDMA API를 활용하여 GPU 스레드가 NVMe BAR 영역에 접근
- 읽어야 하는 이유: 본 과제의 직접적 기반 플랫폼. GPU Initiated I/O의 아키텍처, 구현 방법, 성능 특성을 모두 포함

**[I2] "Accelerating Sampling and Aggregation Operations in GNN Frameworks with GPU Initiated Direct Storage Accesses" (GIDS)** (VLDB 2024) — **필독**
- 저자: Jeongmin Brian Park, Vikram Sharma Mailthody, Zaid Qureshi, Wen-mei Hwu
- 핵심: BaM을 GNN 학습에 적용. GPU 스레드가 직접 스토리지에서 feature 데이터를 fetch하여, DGL GNN 학습 파이프라인을 최대 582배 가속
- 읽어야 하는 이유: BaM의 실제 AI 워크로드 적용 사례. GPU Initiated I/O가 만들어내는 실제 I/O 패턴(소량 랜덤 read 고밀도)을 보여줌

**[I3] "AGILE: Lightweight and Efficient Asynchronous GPU-SSD Integration"** (arXiv 2025) — **필독**
- 핵심: BaM의 동기식 I/O 모델의 한계를 지적하고, GPU-centric 비동기 I/O 모델을 제안. 데드락 방지, 유연한 캐시 정책 등을 구현
- 읽어야 하는 이유: BaM 이후의 최신 발전 방향. BaM의 한계(동기식 모델로 인한 GPU 스레드 대기)와 이를 극복하는 접근을 이해하면 본 과제의 향후 확장 방향 설정에 유용

**[I4] NVIDIA SCADA (Storage Control-path And Data-path Acceleration)** (2025년 11월 발표)
- 핵심: GPU로 스토리지 제어 경로까지 오프로드하는 NVIDIA의 최신 기술. GPU Initiated I/O의 production 적용을 위한 NVIDIA의 공식 방향
- 읽어야 하는 이유: GPU Initiated I/O가 연구 프로토타입에서 production 기술로 전환되고 있음을 보여주는 최신 근거

---

### 읽는 순서 제안

발표 구성(CPU → GDS → GPU Initiated)에 맞춰서, 먼저 **[C1]**으로 CPU I/O 스택의 구조와 한계를 이해하고, **[G1]**으로 GDS가 이 한계를 어떻게 부분적으로 해결하는지 파악한 뒤, **[I1] BaM 논문**을 집중적으로 읽는 것을 권장합니다. 이후 **[I2] GIDS**로 실제 적용 사례를, **[I3] AGILE**로 최신 발전 방향을 확인하는 순서가 효율적입니다.

이 리스트에서 추가하거나 빼고 싶은 논문 방향이 있으면 말씀해 주세요.



검색 결과를 종합하여, 기존 리스트에 각 분류별로 논문을 추가 확장하겠습니다.

---

## 확장된 논문 분류 리딩 리스트

### 분류 1: CPU Initiated I/O

**[C1] "Performance Characterization of Modern Storage Stacks: POSIX I/O, libaio, SPDK, and io_uring"** (CHEOPS 2023)
- fio 기반 네 가지 I/O 스택의 CPU 효율성 및 성능 체계적 비교. SPDK가 io_uring 대비 13배 효율적임을 실증

**[C2] "What Modern NVMe Storage Can Do, And How To Exploit It"** (VLDB 2023)
- 최신 NVMe SSD의 DBMS 워크로드 성능 측정. CPU 코어의 절반이 I/O 처리에 소모되는 병목을 실증하고, SPDK vs io_uring vs libaio 비교

**[C3] "SPDK: A Development Kit to Build High Performance Storage Applications"** (CloudCom 2017)
- SPDK의 설계 철학과 구조. 유저스페이스 폴링 모드 NVMe 드라이버의 기반 이해용

**[C4] "120 Million I/O Per Second with a Standard 2U Intel Xeon System"** (SPDK Blog, 2023)
- SPDK NVMe 드라이버의 코어당 IOPS 스케일링 분석. 단일 코어 10M+ IOPS, 시스템 전체 120M IOPS 달성. CPU Initiated I/O의 현재 최고 수준을 보여주는 자료

**[C5] "io_uring" 관련 — "Efficient IO with io_uring"** (Kernel.org, 2023 업데이트)
- Linux 커널의 최신 비동기 I/O 인터페이스. CPU Initiated I/O의 최신 커널 측 발전 방향 이해용

---

### 분류 2: GPUDirect Storage I/O

**[G1] "Quantifying Performance Gains of GPUDirect Storage"** (IEEE, 2022)
- GDS의 성능 이점을 정량적으로 측정한 최초의 학술 논문. CPU 경유 대비 throughput 향상 및 latency 감소 벤치마크

**[G2] "Insights into GPUDirect Data Transfer through NIXL Benchmarking"** (2025)
- GDS와 NVIDIA NIXL 라이브러리 비교. 가장 최근의 GDS 성능 데이터 포함, 배치 크기 영향 등 세부 분석

**[G3] Micron 9400 NVMe SSD Performance with GPUDirect Storage (백서, 2023)
- SSD 벤더 관점에서 GDS 활성화 시 legacy I/O 대비 최대 1.5배 성능 향상 실측

**[G4] "Accelerating AI With High Performance Storage"** (Solidigm/NVIDIA, 2025)
- GDS + NVMe-oF + DPU 조합의 production 아키텍처. 원격 스토리지에서 로컬 NVMe 수준 성능 달성

**[G5] "SSDTrain: An Activation Offloading Framework to SSDs for Faster Large Language Model Training"** (arXiv, 2024)
- LLM 학습 시 activation 텐서를 NVMe SSD로 오프로드하는 프레임워크. GPUDirect Storage를 활용한 GPU-SSD 간 데이터 전송 최적화. AI 워크로드에서 GPU↔NVMe I/O가 실제로 어떤 패턴으로 발생하는지 이해하는 데 유용

**[G6] "SPIN: Seamless Operating System Integration of Peer-to-Peer DMA Between SSDs and GPUs"** (ATC 2023)
- P2P DMA를 OS 파일 I/O 스택에 통합하는 연구. GDS처럼 CPU가 I/O를 발행하되 P2P 경로를 사용하는 구조에서 page cache 일관성, read-ahead 등 OS 호환성 문제를 다룸. P2P 경로의 시스템 수준 이슈를 이해하는 데 중요

**[G7] KIOXIA GPUDirect Storage Performance Report (백서, 2023~2024)**
- KIOXIA NVMe SSD에서의 GDS 성능 리포트. 벤더별 GDS 성능 차이를 비교하는 관점에서 참조 가치

---

### 분류 3: GPU Initiated I/O

**[I1] "GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture"** (ASPLOS 2023) — **필독**
- GPU Initiated I/O의 원조 논문. GPU 커널이 NVMe SQ에 직접 커맨드를 작성하고 doorbell을 발행하는 전체 아키텍처와 구현

**[I2] "Accelerating Sampling and Aggregation Operations in GNN Frameworks with GPU Initiated Direct Storage Accesses (GIDS)"** (VLDB 2024) — **필독**
- BaM을 GNN 학습에 적용. DGL 학습 파이프라인을 최대 582배 가속. GPU Initiated I/O의 실제 AI 워크로드 적용 사례

**[I3] "AGILE: Lightweight and Efficient Asynchronous GPU-SSD Integration"** (arXiv, 2025) — **필독**
- BaM의 동기식 모델 한계를 극복하는 비동기 GPU-SSD 통합. 데드락 방지, 유연한 캐시 정책 등 BaM 이후의 최신 발전 방향

**[I4] "The Landscape of GPU-Centric Communication"** (arXiv, 2024)
- GPU 중심 통신 기술의 전체 조감도. GPUDirect P2P, RDMA, NVLink, NVSwitch를 포함한 GPU-centric 데이터 이동 기술의 계보를 정리. GPU Initiated I/O를 더 큰 GPU-centric 아키텍처 흐름 속에 위치시키는 데 유용

**[I5] "FlashGPU: Placing New Flash Next to GPU Cores"** (DAC 2019, 선행 연구)
- GPU 코어 옆에 플래시 스토리지를 직접 배치하는 아키텍처 연구. GPU에서 스토리지에 직접 접근하는 개념의 하드웨어 수준 선행 연구로, BaM이 소프트웨어적으로 구현한 것을 하드웨어 관점에서 탐구

**[I6] "Managing Scalable Direct Storage Accesses for GPUs with GoFS"** (SOSP 2025)
- GPU의 직접 스토리지 접근을 확장 가능하게 관리하는 파일시스템. GPU Initiated I/O 위에 파일 수준 추상화를 제공하는 최신 연구. BaM이 raw block 접근인 반면, GoFS는 GPU에서 파일 시스템을 사용할 수 있게 하는 상위 계층

**[I7] "TERAIO: Cost-Efficient LLM Training with Lifetime-Aware Tensor Offloading"** (NeurIPS 2025)
- LLM 학습에서 텐서를 SSD로 fine-grained 오프로드. GPU 메모리를 NVMe SSD로 확장하는 프레임워크. GPU↔NVMe 간 대용량 데이터 이동 패턴(2MiB chunk 등)을 이해하는 데 유용

**[I8] "HetCache: Synergising NVMe Storage and GPU Acceleration"** (CIDR 2023)
- GPU-NVMe 가속 서버에서의 이기종 메모리 계층 최적화. CPU DRAM, GPU VRAM, NVMe SSD를 통합하는 캐시 전략. 복잡한 메모리 계층에서 데이터 배치를 최적화하는 연구

---

### 추가 분류: NVMe 프로토콜/컨트롤러 관점 (발표 보조 자료)

**[N1] "Why does NVMe Need to Evolve for Efficient Storage Access from GPUs?"** (SNIA, 2024)
- NVMe 프로토콜이 GPU 스토리지 접근을 위해 어떻게 진화해야 하는지를 논의하는 SNIA 발표. NVMe 스펙 관점에서 GPU Initiated I/O의 요구사항을 정리

**[N2] DapuStor BaM Demo 분석** (Blocks and Files, 2024)
- SSD 벤더(DapuStor)가 BaM을 자사 SSD로 데모한 사례. SSD 벤더가 GPU Initiated I/O를 어떻게 인식하고 대응하는지 보여주는 산업 동향 자료

---

### 읽는 순서 제안 (업데이트)

**1단계 — 기초 이해 (CPU I/O):** [C1] → [C2] → [C4]로 CPU I/O 스택의 구조, 한계, 현재 최고 성능을 파악

**2단계 — 중간 단계 (GDS):** [G1] → [G6] → [G5]로 GDS의 구조, P2P 경로의 시스템 이슈, AI 워크로드 적용 사례를 이해

**3단계 — 핵심 (GPU Initiated I/O):** [I1] BaM → [I2] GIDS → [I3] AGILE → [I6] GoFS 순으로 GPU Initiated I/O의 기원에서 최신 발전까지 추적

**4단계 — 큰 그림:** [I4]로 GPU-centric 통신의 전체 조감도를 보고, [N1]으로 NVMe 스펙 관점의 시사점을 확인

총 20편 이상으로 확장했습니다. 특정 분류를 더 보강하거나, 특정 논문에 대해 더 자세한 내용이 필요하면 말씀해 주세요.


지금까지의 검색 결과를 종합하여 전수 조사에 가까운 형태로 논문 리스트를 정리하겠습니다. BaM 논문의 인용 논문들, 각 논문의 Related Work에서 언급되는 논문들, 그리고 주요 학회별로 추적한 결과를 모두 포함합니다.

---

## 전수 조사 기반 논문 리딩 리스트 (2020~2025)

### 분류 0: 선행 연구 (Pre-2023, GPU-Storage 접근의 기초)

| # | 논문명 | 학회/연도 | 핵심 |
|---|--------|-----------|------|
| P1 | GPUfs: Integrating a File System with GPUs | Silberstein et al., ASPLOS 2014 | GPU에서 파일 API를 제공하는 최초 연구. CPU가 I/O를 처리하되 GPU 커널에서 파일 접근 코드를 작성 |
| P2 | NVMMU: A Non-volatile Memory Management Unit for Heterogeneous GPU-SSD Architectures | Zhang et al., PACT 2015 | GPU-SSD 간 직접 접근을 위한 하드웨어 메모리 관리 유닛 제안 |
| P3 | FlashGPU: Placing New Flash Next to GPU Cores | Zhang et al., DAC 2019 | GPU 코어 옆에 플래시를 직접 배치하는 아키텍처 수준 연구 |
| P4 | EMOGI: Efficient Memory-access for Out-of-memory Graph-traversal in GPUs | Min et al., VLDB 2021 | GPU에서 호스트 메모리를 zero-copy로 직접 접근하여 대규모 그래프를 처리. PCIe 대역폭을 최대 활용하는 접근 방식으로 BaM의 직접적 선행 연구 |
| P5 | FlashNeuron: SSD-Enabled Large-Batch Training of Very Deep Neural Networks | Bae et al., FAST 2021 | NVMe SSD를 DNN 학습의 백업 스토어로 사용하는 최초 시스템. GPU-SSD 간 P2P DMA를 활용한 텐서 오프로드 |
| P6 | ZeRO-Infinity: Breaking the GPU Memory Wall with Heterogeneous System Offload | Rajbhandari et al., SC 2021 | GPU, CPU, NVMe를 통합한 이기종 오프로드. 조 단위 파라미터 모델 학습 가능. DeepSpeed에 통합 |
| P7 | SPDK: A Development Kit to Build High Performance Storage Applications | Yang et al., CloudCom 2017 | 유저스페이스 NVMe 드라이버의 설계. BaM이 GPU측에서 유사한 접근을 구현 |

---

### 분류 1: CPU Initiated I/O (현재 검증 환경의 기반)

| # | 논문명 | 학회/연도 | 핵심 |
|---|--------|-----------|------|
| C1 | Performance Characterization of Modern Storage Stacks: POSIX I/O, libaio, SPDK, and io_uring | CHEOPS 2023 | 네 가지 I/O 스택의 CPU 효율성 체계적 비교. SPDK가 13배 효율적 |
| C2 | What Modern NVMe Storage Can Do, And How To Exploit It | Haas & Leis, VLDB 2023 | 최신 NVMe의 DBMS 워크로드 성능. CPU 코어 절반이 I/O 처리에 소모됨을 실증 |
| C3 | I/O Passthru: Upstreaming a Flexible and Efficient I/O Path in Linux | Joshi et al., FAST 2024 | Linux 커널의 NVMe I/O 경로 최적화. 기존 커널 드라이버의 오버헤드 분석 |
| C4 | 120 Million IOPS with Standard 2U Intel Xeon System | SPDK Blog, 2023 | CPU Initiated I/O의 현재 최고 성능 수준 |

---

### 분류 2: GPUDirect Storage I/O (CPU가 발행, DMA만 P2P)

| # | 논문명 | 학회/연도 | 핵심 |
|---|--------|-----------|------|
| G1 | Quantifying Performance Gains of GPUDirect Storage | Inupakutika et al., IEEE NAS 2022 | GDS 성능을 정량적으로 최초 측정한 학술 논문 |
| G2 | SPIN: Seamless Operating System Integration of Peer-to-Peer DMA Between SSDs and GPUs | Markussen et al., ATC 2023 | P2P DMA를 OS 파일 I/O 스택에 통합. page cache 일관성, read-ahead 등 시스템 수준 이슈 |
| G3 | Using GPU Direct Storage with High-Performance Distributed Filesystems | Hoozemans et al., CHEOPS 2025 | GDS를 분산 파일시스템(Lustre 등)과 결합한 성능 분석 |
| G4 | Storage Access Optimization for Efficient GPU-centric Information Retrieval (ESPN) | J. Supercomputing 2025 | GDS를 활용한 GPU 중심 정보 검색 아키텍처. 쿼리 latency 3.9배 감소 |
| G5 | Insights into GPUDirect Data Transfer through NIXL Benchmarking | Muradli et al., 2025 | GDS와 NVIDIA NIXL 비교 벤치마크. 가장 최근 GDS 성능 데이터 |
| G6 | Micron 9400 NVMe SSD with GPUDirect Storage | Micron 백서, 2023 | SSD 벤더 관점 GDS 성능 실측 |
| G7 | Accelerating AI With High Performance Storage | Solidigm/NVIDIA 백서, 2025 | GDS + NVMe-oF + DPU production 아키텍처 |
| G8 | KIOXIA GPUDirect Storage Performance Report | KIOXIA 백서, 2023~2024 | KIOXIA SSD에서의 GDS 성능 데이터 |

---

### 분류 3: GPU Initiated I/O (GPU가 직접 NVMe 커맨드 발행)

| # | 논문명 | 학회/연도 | 핵심 |
|---|--------|-----------|------|
| **I1** | **GPU-Initiated On-Demand High-Throughput Storage Access in the BaM System Architecture** | **Qureshi et al., ASPLOS 2023** | **원조 논문. GPU 커널이 NVMe SQ에 직접 커맨드 발행. 필독** |
| **I2** | **GIDS: Accelerating Sampling and Aggregation Operations in GNN Frameworks with GPU Initiated Direct Storage Accesses** | **Park et al., VLDB 2024** | **BaM을 GNN 학습에 적용. DGL 파이프라인 582배 가속. 필독** |
| **I3** | **AGILE: Lightweight and Efficient Asynchronous GPU-SSD Integration** | **Yang et al., SC 2025** | **BaM의 동기식 한계를 극복하는 비동기 GPU-SSD 통합. 필독** |
| I4 | GoFS: Managing Scalable Direct Storage Accesses for GPUs | Li et al., SOSP 2025 | GPU Initiated I/O 위에 파일시스템 추상화를 제공. 멀티 GPU, 멀티 SSD 스케일링 |
| I5 | GeminiFS: A Companion File System for GPUs | Qiu et al., FAST 2025 | GPU를 위한 companion 파일시스템. NVMe 드라이버를 확장하여 CPU-GPU 간 공유 제어 플레인 구현 |
| I6 | Phoenix: A Refactored I/O Stack for GPU Direct Storage without Phony Buffers | Yan et al., SC 2025 | GDS의 phony buffer 문제를 해결하는 리팩토링된 I/O 스택. 네트워크 스토리지까지 확장 가능 |
| I7 | Path to GPU-Initiated I/O for Data-Intensive Systems | Torp et al. (Samsung), DaMoN 2025 | GPU Initiated I/O의 현 상태를 서베이하고 데이터 집약적 시스템에의 적용 경로를 분석 |
| I8 | GPU Graph Processing on CXL-Based Microsecond-Latency External Memory | 2024 | BaM 구조를 CXL 기반 저지연 메모리로 확장. BaM vs EMOGI 비교 |
| I9 | GMT: GPU Orchestrated Memory Tiering for the Big Data Era | ASPLOS 2024 | GPU가 오케스트레이션하는 3계층 메모리 (GPU VRAM, 호스트 DRAM, SSD) 관리 |

---

### 분류 4: GPU-NVMe 오프로드/메모리 확장 (AI 워크로드 I/O 패턴 이해용)

| # | 논문명 | 학회/연도 | 핵심 |
|---|--------|-----------|------|
| O1 | SSDTrain: An Activation Offloading Framework to SSDs for Faster LLM Training | Wu et al., arXiv 2024 | LLM 학습 시 activation을 NVMe SSD로 오프로드. GPU↔NVMe I/O 패턴 분석 |
| O2 | TERAIO: Cost-Efficient LLM Training with Lifetime-Aware Tensor Offloading | NeurIPS 2025 | 텐서 활동 패턴 기반 fine-grained SSD 오프로드 |
| O3 | An I/O Characterizing Study of Offloading LLM Models and KV Caches to NVMe SSD | CHEOPS 2025 | LLM 모델/KV 캐시의 NVMe 오프로드 시 I/O 특성을 상세 프로파일링 |
| O4 | Fuyou: Adding NVMe SSDs to Enable and Accelerate 100B Model Fine-tuning on a Single GPU | arXiv 2024 | 단일 GPU에서 100B 모델 파인튜닝을 NVMe로 가능하게 함. ZeRO-Infinity 대비 성능 비교 |
| O5 | LoHan: Low-Cost High-Performance Framework for LLM Fine-tuning | 2024 | 메인 메모리와 NVMe SSD를 혼합하여 activation 오프로드 최적화 |
| O6 | G10: Enabling An Efficient Unified GPU Memory and Storage Architecture with Smart Tensor Migrations | MICRO 2023 | GPU 메모리와 스토리지를 통합하는 아키텍처. 텐서 마이그레이션 최적화 |
| O7 | HetCache: Synergising NVMe Storage and GPU Acceleration | CIDR 2023 | CPU DRAM, GPU VRAM, NVMe SSD 이기종 캐시 전략 최적화 |
| O8 | MLP-Offload: Multi-Level, Multi-Path Offloading for LLM | SC 2025 | DeepSpeed ZeRO-3의 NVMe 오프로드 경로를 다중 레벨/다중 경로로 최적화 |

---

### 분류 5: 산업 동향 및 표준 (발표 맥락 설정용)

| # | 자료명 | 출처/연도 | 핵심 |
|---|--------|-----------|------|
| N1 | Why does NVMe Need to Evolve for Efficient Storage Access from GPUs? | SNIA 2024 | NVMe 스펙이 GPU 접근을 위해 진화해야 하는 이유 |
| N2 | The Next Step for AI Storage: GPU-initiated and CPU-initiated Storage | Marvell Blog, 2025 | GPU Initiated I/O가 SSD 설계에 미치는 영향. SSD 벤더의 대응 방향 |
| N3 | DapuStor BaM Demo | Blocks and Files, 2024 | SSD 벤더가 BaM을 자사 SSD로 시연한 산업 사례 |
| N4 | NVIDIA SCADA (Storage Control-path And Data-path Acceleration) | NVIDIA, 2025.11 | GPU로 스토리지 제어 경로까지 오프로드하는 NVIDIA 최신 기술 |
| N5 | GPUs as Data Access Engines | Newburn (NVIDIA/Micron), FMS 2024 | BaM/GIDS의 Gen5 NVMe SSD 성능 데이터를 포함한 발표 자료 |

---

## 총 정리

| 분류 | 논문 수 | 비고 |
|------|---------|------|
| 선행 연구 (P) | 7편 | 2014~2021, GPU-Storage의 기초 |
| CPU Initiated I/O (C) | 4편 | 현재 검증 환경 기반 |
| GPUDirect Storage (G) | 8편 | 중간 단계, CPU 발행 + P2P DMA |
| GPU Initiated I/O (I) | 9편 | 핵심 분류, BaM 계보 |
| GPU-NVMe 오프로드 (O) | 8편 | AI 워크로드 I/O 패턴 이해 |
| 산업 동향 (N) | 5편 | 발표 맥락 설정 |
| **합계** | **41편** | |

이전 20편 대비 두 배 이상 확장했고, 특히 BaM 인용 논문에서 발견한 GoFS(SOSP 2025), GeminiFS(FAST 2025), Phoenix(SC 2025), GMT(ASPLOS 2024)와, AI 워크로드 I/O 패턴을 이해하기 위한 오프로드 연구들(G10, TERAIO, Fuyou, MLP-Offload 등)을 체계적으로 포함했습니다.

추가로 보강하거나, 특정 분류를 더 깊이 파고 싶은 부분이 있으면 말씀해 주세요.

