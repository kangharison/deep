# Technical Paths to the New Era of GPU Initiated Storage

- **출처**: OCP (Open Compute Project) 공식 채널 YouTube
- **URL**: https://www.youtube.com/watch?v=_JPSyy0M87s
- **발표일**: 2025-10-22
- **길이**: 21:31
- **발표자**:
  - CJ Newburn, Distinguished Engineer — NVIDIA
  - Vikram Sharma Mailthody, Sr Research Scientist — NVIDIA
- **관련 기술**: SCADA (Scalable Accelerated Data Access), GPU-initiated Storage, Storage-Next

---

## 1. 핵심 주장 (Key Thesis)

NVIDIA가 OCP에서 공식적으로 제시한 메시지:

> **"We are entering a new era where the need to constrain data-intensive apps to fit into memory is ending."**

- Data-intensive 앱을 메모리(DRAM/HBM)에 억지로 맞출 필요가 없어지는 시대가 도래
- 스토리지(SSD)에 직접 접근하면 훨씬 큰 데이터셋을 처리 가능하고, TCO도 유리
- 이를 위해 GPU가 CPU 없이 직접 스토리지에 접근하는 **SCADA** 아키텍처를 제안

---

## 2. Data-Intensive App의 정의

발표 설명에서 직접 정의:

> **"Data-intensive apps process large quantities of data but focus more on data access than computation."**

| 구분 | Compute-Intensive | Data-Intensive |
|------|-------------------|----------------|
| **초점** | 연산(computation) | 데이터 접근(data access) |
| **병목** | GPU ALU / Tensor Core | 메모리/스토리지 대역폭 |
| **데이터 재사용** | 높음 (같은 데이터 반복 연산) | 낮음 (대량 데이터 1회 접근) |
| **대표 워크로드** | LLM Training (행렬곱), CNN Training | 추천(DLRM), GNN, Vector Search, RAG, LLM Decode |
| **GPU 스레드 특성** | 적은 스레드가 큰 데이터 블록 처리 | 100,000+ 스레드가 각각 작은 데이터 요청 |

---

## 3. GPU vs CPU: 왜 GPU가 스토리지에 직접 접근해야 하는가

발표에서 제시한 GPU의 장점:

- CPU 대비 **100x 더 많은 스레드**
- 더 높은 메모리 대역폭
- Data-intensive 워크로드에서 GPU가 CPU에 스토리지 접근을 위임하면 CPU가 병목

```
기존 방식 (CPU-mediated):
  GPU ──요청──→ CPU ──NVMe cmd──→ SSD ──데이터──→ CPU ──전송──→ GPU
                 ↑
            CPU가 병목: context switch, 커널 오버헤드, 수십 μs 지연

SCADA 방식 (GPU-initiated):
  GPU ──NVMe cmd──→ SSD ──데이터──→ GPU
    ↑
  100,000 스레드가 각각 독립적으로 스토리지 요청
  GPU-side cache로 locality 활용
  PCIe bandwidth에만 제한됨
```

---

## 4. SCADA (Scalable Accelerated Data Access)

### 4.1 개요

NVIDIA가 제안한 GPU-initiated storage 프레임워크:

- **100,000 GPU 스레드**가 동시에 (concurrently) 각자의 스토리지 요청을 발행
- GPU 내부에 **GPU-side cache**를 두어 locality가 있는 접근을 최적화
- 보안(secure) 접근 보장
- 성능 상한: **PCIe bandwidth에만 제한**

### 4.2 SCADA의 위치 (관련 기술과의 비교)

| 기술 | 주체 | 데이터 경로 | 특징 |
|------|------|-----------|------|
| 기존 Kernel I/O | CPU | SSD → CPU DRAM → GPU | 커널 오버헤드, bounce buffer |
| GPUDirect Storage (GDS) | CPU 주도 | SSD → GPU (DMA) | CPU가 명령, 데이터만 직접 전송 |
| BaM | GPU | SSD ↔ GPU (P2P) | GPU 커널에서 직접 NVMe 큐 접근 |
| **SCADA** | GPU | SSD ↔ GPU | BaM 발전형, 100K 스레드 동시 접근, GPU cache, 보안 |

### 4.3 Storage-Next 파트너십

- SCADA는 **Storage-Next 파트너**들과 함께 기술적 과제를 해결 중
- OCP를 통해 업계 표준화 추진
- SSD 벤더(Samsung, Kioxia, Solidigm 등)와 협력하여 GPU-initiated I/O에 최적화된 SSD 인터페이스 개발

---

## 5. AI 워크로드별 스토리지 요구사항 종합

이 OCP 발표 + Marvell 블로그 + KIOXIA 백서 + 학술 논문을 종합한 분류:

### 5.1 Data-Intensive AI 워크로드 (GPU-initiated storage 대상)

| 워크로드 | 왜 Data-Intensive인가 | I/O 패턴 | 필요 SSD 특성 |
|---------|----------------------|---------|-------------|
| **DLRM (추천)** | 수백GB~TB 임베딩 테이블 조회, 연산은 간단한 lookup | Random Read (불규칙, 소블록) | 초고 IOPS, low-latency |
| **GNN** | 이웃 노드 접근이 불규칙, 그래프가 VRAM 초과 | Random Read (irregular) | 초고 IOPS, low-latency |
| **LLM Decode** | 토큰 1개당 전체 weight 읽기, 연산량 적음 | Sequential Read (memory-bound) | 높은 BW, low-latency |
| **KV-Cache Offload** | VRAM 초과 시 KV-cache를 SSD로 swap | Random R/W (latency 극민감) | 극저 latency, 높은 IOPS |
| **Vector Search / RAG** | 임베딩 인덱스 탐색 + 문서 fetch | Random Read (4K~64K 소블록) | 초고 IOPS, low-latency |
| **Data Preprocessing** | 대량 데이터 읽기 → 변환 → 쓰기 | Sequential R/W | 높은 BW, 대용량 |
| **Checkpointing** | 모델 가중치 주기적 저장 (수백GB~TB) | Sequential Write (burst) | 높은 BW, 대용량 |

### 5.2 Compute-Intensive AI 워크로드 (기존 CPU-initiated 충분)

| 워크로드 | 왜 Compute-Intensive인가 | I/O 패턴 | SSD 요구 |
|---------|------------------------|---------|---------|
| **LLM Training (Forward/Backward)** | 행렬곱 지배적, 데이터 재사용률 높음 | 학습 시작 시 대량 로딩 후 I/O 적음 | 낮은 BW (0.15 GB/s for GPT-3) |
| **CNN Training** | Conv 연산 = 행렬곱 | 배치 로딩 시에만 I/O | 중간 BW |
| **LLM Prefill** | 긴 prompt 전체를 한번에 행렬곱 | 1회성 weight 로딩 | 낮음 |

### 5.3 핵심 관찰

> **AI 워크로드 대부분이 data-intensive이다.**
> Compute-intensive한 것은 Training의 forward/backward pass 뿐이고, 그마저도 데이터 로딩·체크포인팅·전처리 단계에서는 data-intensive하다.
> 특히 AI가 Training에서 Inference 중심으로 전환되면서, data-intensive 비중은 더 커지고 있다.

---

## 6. SSD 설계에 대한 시사점

### 6.1 NVIDIA SCADA가 요구하는 SSD 특성

| 요구사항 | 이유 | 목표 수치 |
|---------|------|----------|
| **초고 IOPS** | 100K GPU 스레드의 동시 소블록 요청 처리 | 100M IOPS (PCIe 7.0, Marvell 기준) |
| **4KB 이하 소블록 최적화** | Data-intensive 워크로드의 접근 단위가 작음 | sub-4KB Random Read 최적화 |
| **극저 Latency** | GPU idle 시간 최소화 | μs 단위 응답 |
| **GPU-direct 인터페이스** | CPU bypass, PCIe P2P | SCADA / GDS 호환 NVMe 컨트롤러 |
| **높은 QD (Queue Depth)** | 100K 동시 요청 처리 | NVMe 큐 깊이 확장 |
| **보안 접근** | multi-tenant GPU 환경 | NVMe namespace 격리 등 |

### 6.2 Marvell이 제시한 미래 SSD 아키텍처

- GPU-initiated (Inference): PCIe switch를 통해 GPU ↔ SSD 직결 (최대 GPU 4개 × SSD 8개)
- CPU-initiated (Training): PCIe + Ethernet dual interface SSD
- PCIe 7.0 SSD에서 100M IOPS 목표

### 6.3 KIOXIA 백서의 스토리지 단계별 요구사항

```
Data Ingestion    → Capacity / Bandwidth (대용량 순차 쓰기)
Data Preparation  → Bandwidth (높은 R/W 처리량)
Training          → Low-Latency (체크포인트, 데이터 로딩)
Inferencing + RAG → Very Low-Latency + Large Capacity (소블록 랜덤 읽기)
```

---

## 7. 관련 논문 및 자료 (출처 정리)

### 학술 논문

| 논문 | 학회/년도 | 핵심 기여 |
|------|----------|----------|
| Data Movement is All You Need | MLSys '21 | Transformer의 37%가 memory-bound 연산임을 정량 증명 |
| BaM: GPU-Initiated On-Demand Storage Access | ASPLOS '23 | GPU가 직접 NVMe에 접근하는 시스템 아키텍처 |
| Path to GPU-Initiated I/O for Data-Intensive Systems | DaMoN '25 | BaM vs SPDK 비교, Samsung+ITU Copenhagen |
| Demystifying MLPerf Training Benchmark | IISWC '20 | 대부분의 ML 벤치마크가 memory-bound임을 roofline으로 증명 |
| Understanding Data Storage for Large-Scale DLRM Training | ISCA '22 | Meta DLRM의 스토리지 파이프라인 상세 분석 |
| FlexGen: High-Throughput LLM Inference | ICML '23 | GPU+CPU+SSD 3계층 LLM 추론 |
| A Systematic Characterization of LLM Inference on GPUs | arXiv '24 | LLM prefill=compute, decode=memory-bound 분류 |

### 산업 자료

| 자료 | 출처/년도 | 핵심 내용 |
|------|----------|----------|
| Technical Paths to the New Era of GPU Initiated Storage | OCP/NVIDIA, 2025.10 | SCADA 아키텍처, data-intensive 정의 |
| The Next Step for AI Storage | Marvell Blog, 2025.11 | GPU-initiated vs CPU-initiated 분류표 |
| Understanding AI Storage Challenges and the SSD Advantage | KIOXIA Whitepaper, 2025.02 | AI 단계별 스토리지 요구사항, SNIA 대역폭 데이터 |
| NVIDIA SCADA offloads storage control path to GPU | BlocksandFiles, 2025.11 | SCADA 아키텍처 상세 |

---

## 8. 핵심 인용문 모음

**Data-Intensive 정의 (NVIDIA, OCP):**
> "Data-intensive apps process large quantities of data but focus more on data access than computation."

**메모리 제약 종료 선언 (NVIDIA, OCP):**
> "We are entering a new era where the need to constrain data-intensive apps to fit into memory is ending."

**GPU의 장점 (NVIDIA, OCP):**
> "GPUs offer significant advantages over CPUs, including 100x more threads and higher memory bandwidth."

**SCADA 핵심 (NVIDIA, OCP):**
> "SCADA allows each of 100,000 threads to concurrently initiate their own requests for data in storage, offers a GPU-side cache to harvest any available locality, and securely performs accesses to storage at rates bounded only by PCIe bandwidth."

**Training의 data movement 비중 (Data Movement is All You Need):**
> "37% of total training time is spent on memory-bound data movement operations."

**MLPerf 벤치마크 (Demystifying MLPerf):**
> "All MLPerf workloads are memory-bound."

**DLRM 임베딩 (Supporting Massive DLRM Inference):**
> "Embedding lookups are memory bandwidth intensive while dense MLP layers are compute intensive."

**KIOXIA 스토리지 소비전력 (KIOXIA Whitepaper, ISCA '22 인용):**
> "In the case of DLRMs in Meta's data centers, the storage and pre-processing can consume more power than the actual GPUs during the training."
