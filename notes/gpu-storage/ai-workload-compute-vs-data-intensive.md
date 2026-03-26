# AI/GPU Workload — Compute vs. Data-Intensive 종합 정리

- **정리일**: 2026-03-26
- **관련 발표**: NVIDIA OCP 2025, FMS 2024 (CJ Newburn), GTC 2025
- **관련 자료**: KIOXIA Whitepaper (2025.02), Marvell Blog (2025.11), DaMoN '25 논문

---

## 1. Data-Intensive의 정의

NVIDIA CJ Newburn (OCP 2025, FMS 2024)의 공식 정의:

> **"Data-intensive apps process large quantities of data but focus more on data access than computation."**
> (데이터를 대량 처리하되, 연산보다 데이터 접근에 초점을 맞춘 애플리케이션)

| 구분 | Compute-Intensive | Data-Intensive |
|------|-------------------|----------------|
| **초점** | 연산 (computation) | 데이터 접근 (data access) |
| **병목** | GPU ALU / Tensor Core | 메모리/스토리지 대역폭 |
| **데이터 재사용** | 높음 (같은 데이터 반복 연산) | 낮음 (대량 데이터 1회 접근) |
| **GPU 스레드 특성** | 소수 스레드가 큰 데이터 블록 처리 | 100,000+ 스레드가 각각 작은 데이터 요청 |
| **Latency Tolerance** | Cache misses (수십 ns) | Network/media access (수 μs~ms) |
| **Arithmetic Intensity** | 높음 (FLOPs/Byte ↑) | 낮음 (FLOPs/Byte ↓) |

---

## 2. AI 워크로드 분류

### 2.1 Compute-Intensive vs Data-Intensive

```
              AI/GPU Workload
             ┌───────┴───────┐
      Compute-Intensive    Data-Intensive
      ├─ LLM Training      ├─ Recommendation (DLRM)
      ├─ CNN Training       ├─ GNN
      ├─ Diffusion Model    ├─ LLM Inference (Decode)
      ├─ LLM Prefill        ├─ Vector Search / RAG
      └─ HPC Simulation     ├─ KV-Cache Offload
                             └─ Data Preprocessing
```

**핵심 관찰**: 8개 워크로드 중 6개가 Data-Intensive이다.

### 2.2 모델이 아니라 "단계"가 결정한다

같은 모델이라도 Training과 Inference에서 특성이 다르다:

| 모델 | Training | Inference |
|------|----------|-----------|
| **LLM (Transformer)** | Compute-Intensive (대형 GEMM) | Data-Intensive (Decode, memory-bound) |
| **CNN** | Compute-Intensive | Compute-Intensive (경량) |
| **DLRM (추천)** | Data-Intensive (임베딩 접근) | Data-Intensive (임베딩 lookup) |
| **GNN** | Data-Intensive (이웃 탐색) | Data-Intensive (이웃 탐색) |

### 2.3 Data-Intensive 워크로드의 CPU/GPU-Initiated 분류

```
           Data-Intensive
          ┌──────┴──────┐
   CPU-Initiated       GPU-Initiated
   ├─ Data Preprocessing  ├─ Recommendation (DLRM)
   ├─ Data Ingestion      ├─ GNN
   └─ Checkpointing       ├─ LLM Inference (Decode)
                           ├─ Vector Search / RAG
                           └─ KV-Cache Offload
```

**차이**: CPU-Initiated는 사전에 접근 패턴이 정해져 있고 (Sequential, prefetch 가능), GPU-Initiated는 런타임에 GPU 스레드가 어떤 데이터가 필요한지 결정한다 (Random, data-dependent, prefetch 불가).

---

## 3. Predictive AI vs Generative AI

AI를 목적으로 분류하면:

| 분류 | 하는 일 | 대표 모델 | I/O 특성 |
|------|--------|---------|---------|
| **Predictive AI** | 과거 데이터로 미래 예측 (분류/회귀) | DLRM, XGBoost, GNN | 임베딩 테이블 Random Read, 초고 IOPS |
| **Generative AI** | 새로운 콘텐츠 생성 (텍스트, 이미지) | LLM, Diffusion | Weight 로딩 + KV-cache swap |

**둘 다 Inference 단계에서 실행되며, 둘 다 Data-Intensive해질 수 있다.**

---

## 4. 왜 Training에서 Inference 중심으로 전환되는가

- 모델은 한 번 만들고, 추론은 매일 수십억 번 한다
- Inference가 전체 AI 컴퓨팅의 ~70% 이상 차지 (Meta 기준)
- AI가 연구(Training)에서 서비스(Inference)로 전환
- 시간이 지날수록 Inference 비용이 Training을 압도

이 전환이 SSD에 미치는 영향:

| | Training 시대 | Inference 시대 |
|---|---|---|
| 지배적 I/O | Sequential Read/Write | **Random Read** |
| 중요 지표 | Throughput (GB/s) | **IOPS + Latency** |
| 블록 크기 | 큰 블록 (MB 단위) | **소블록 (4KB 이하)** |
| Prefetch 효과 | 높음 (예측 가능) | **낮음 (data-dependent)** |
| 필요 IOPS | 수만~수십만 | **수천만~1억** |

---

## 5. 실제 대화 (LLM Inference)의 특성 분석

Claude와의 대화를 예시로:

- **Compute vs Data-Intensive**: Data-Intensive (Decode 단계 지배적)
- **AI 종류**: Generative AI (텍스트를 새로 생성)
- **사용 모델**: LLM (Transformer, Decoder-only)
- **SSD 접근 (단일 유저)**: 보통 발생하지 않음 (모델이 GPU 메모리에 있으므로)
- **SSD 접근 (Multi-User)**: 동시 유저 수십 명 이상이면 KV-cache가 VRAM 초과하여 SSD swap 발생

### Multi-User 환경에서의 SSD Swap 발생 조건 (H100 80GB 기준)

```
모델 weight:         ~30~40 GB
KV-Cache 가용 VRAM:  ~30~40 GB
유저 1명 평균 KV:     ~1~5 GB
```

- 동시 유저 ~10명: VRAM 한계 도달, SSD swap 시작
- 동시 유저 ~50명: SSD swap 필수
- 동시 유저 수백~수천명 (실서비스): 상시 SSD swap 발생

### SSD Swap 발생 시 I/O 특성

- **크기**: 수 KB ~ 수 MB (토큰별 KV 벡터)
- **패턴**: Random Read/Write
- **빈도**: 토큰 생성할 때마다
- **Latency 민감도**: 극히 높음 (1μs 지연 → GPU idle)

---

## 6. AI 파이프라인 전체와 SSD I/O

AI 워크로드는 Training과 Inference만 있는 것이 아니라 전체 파이프라인이 존재한다:

| 단계 | 빈도 | I/O 패턴 | 핵심 SSD 지표 |
|------|------|---------|-------------|
| ① Data Collection | 주기적 | Seq Write | Capacity, Write BW |
| ② Data Preprocessing | 주기적 | Seq R/W | Read/Write BW |
| ③ Training | 1회~수회 | Seq Read + Checkpoint Write | Read BW, Write burst |
| ④ Evaluation | 수회 | Seq Read | Read BW |
| ⑤ Fine-tuning/RLHF | 빈번해지는 중 | ③과 유사 | Read BW |
| ⑥ Optimization | 1회 | 모델 R/W | 미미 |
| ⑦ **Serving/Inference** | **24/7 상시** | **Random Read** | **IOPS, Latency** |
| ⑧ Monitoring | 상시 | Seq Write | Write BW |

시간적으로 가장 길고, 인프라 비용이 가장 크고, SSD 성능이 가장 직접적으로 영향을 미치는 단계는 **⑦ Serving/Inference**이다.

---

## 7. Multi-GPU / Multi-User 서빙 환경

실제 Inference 서빙에서는 여러 GPU가 하나의 SSD에 접근하고, 하나의 GPU는 여러 유저 요청을 동시 처리한다:

```
     수천~수만 유저 요청
           │
     ┌─────┴─────┐
     │ Load      │
     │ Balancer  │
     └─────┬─────┘
           │
  ┌────────┼────────┐
  ▼        ▼        ▼
GPU 0    GPU 1    GPU 2      ← 각 GPU가 여러 유저 동시 처리
  │        │        │           (Continuous Batching)
  └────────┼────────┘
           │
  ┌────────┼────────┐
  ▼        ▼        ▼
SSD 0    SSD 1    SSD 2      ← 동시에 수만 개 Random Read 요청
```

SSD에 대한 요구사항:

| 항목 | Training | Inference 서빙 |
|------|----------|---------------|
| SSD 접근 주체 | GPU 1개 (또는 소수) | 여러 GPU × 여러 유저 |
| 동시 I/O 요청 수 | 수십~수백 | **수만~수십만** |
| 요청 패턴 | 예측 가능 (순차) | **예측 불가 (data-dependent)** |
| QoS 요구 | 없음 (배치) | **p99 latency 보장 필수** |
| 유저 간 공정성 | 불필요 | **공정한 latency 분배 필요** |

---

## 8. GPU-Initiated Storage (SCADA)

NVIDIA가 이 전환에 대응하여 제안한 아키텍처:

> **"SCADA allows each of 100,000 threads to concurrently initiate their own requests for data in storage."** (OCP 2025)

### 기술 비교

| 기술 | 주체 | 데이터 경로 | 특징 |
|------|------|-----------|------|
| 기존 Kernel I/O | CPU | SSD → CPU DRAM → GPU | 커널 오버헤드 |
| GPUDirect Storage (GDS) | CPU 주도 | SSD → GPU (DMA) | CPU가 명령 |
| BaM | GPU | SSD ↔ GPU (P2P) | GPU 커널에서 직접 NVMe 접근 |
| **SCADA** | GPU | SSD ↔ GPU | 100K 스레드 동시, GPU cache, 보안 |

### SCADA 성능 수치 (FMS 2024, A100 기준)

```
GPU 요청 생성: 45M IOPS (NVGNN)
Cache Hit: 150M IOPS, 600 GB/s
Cache Miss: ~100M IOPS, 400 GB/s
IO Processing: 16M IOPS (16 × 4KB = 64 GB/s)
NVMe 접근: 4 Gen4 drives, 6M IOPS @ 4KB, 24 GB/s

Bottleneck = NVMe and PCIe pin bandwidth, not GPU code
```

---

## 9. SSD Benchmark 현황

| 벤치마크 | 측정 대상 | Inference 커버 |
|---------|----------|--------------|
| MLPerf Inference | GPU 추론 성능 | 스토리지 측정 안 함 |
| MLPerf Storage (v2.0) | Training 데이터 로딩 + Checkpointing | Training만 |
| fio | 범용 스토리지 I/O | 커스텀 프로파일 필요 |

**Inference 전용 SSD 벤치마크는 아직 표준이 없다.** Samsung, Micron 등이 자체 벤치마크를 만들고 있는 상황이다.

---

## 10. 핵심 결론

1. **AI 워크로드는 대부분 Data-Intensive하다** — Compute-Intensive한 것은 Training의 forward/backward pass 뿐이고, 나머지 대부분의 단계와 워크로드는 Data-Intensive하다.
2. **산업이 Training에서 Inference 중심으로 전환 중이다** — Inference의 비중이 70%+ 이며, 이는 곧 Random Read 중심의 Data-Intensive 워크로드 비중이 커지고 있음을 의미한다.
3. **SSD는 이 전환에 맞춰 설계되어야 한다** — Sequential throughput 중심에서 소블록 Random Read의 IOPS와 Latency 중심으로 전환이 필요하다.
4. **GPU-Initiated Storage (SCADA)가 필요하다** — Data-Intensive 워크로드에서 CPU를 거치면 병목이 발생하므로, GPU가 직접 SSD에 접근하는 아키텍처가 요구된다.

---

## 11. 출처

### 학술 논문
- Data Movement is All You Need (MLSys '21) — Transformer의 37%가 memory-bound
- BaM: GPU-Initiated On-Demand Storage Access (ASPLOS '23)
- Path to GPU-Initiated I/O for Data-Intensive Systems (DaMoN '25)
- Demystifying MLPerf Training Benchmark (IISWC '20)
- Understanding Data Storage for Large-Scale DLRM Training (ISCA '22)
- FlexGen: High-Throughput LLM Inference (ICML '23)
- A Systematic Characterization of LLM Inference on GPUs (arXiv '24)

### 산업 자료
- CJ Newburn, "GPUs as Data Access Engines", FMS 2024.08
- CJ Newburn, "Technical Paths to the New Era of GPU Initiated Storage", OCP 2025.10
- CJ Newburn, "Speed-of-Light Data Movement Between Storage and the GPU", GTC 2025
- Marvell, "The Next Step for AI Storage: GPU-initiated and CPU-initiated Storage", 2025.11
- KIOXIA, "Understanding AI Storage Challenges and the SSD Advantage", 2025.02
- Samsung, "Scaling AI Inference with KV Cache Offloading" Whitepaper
