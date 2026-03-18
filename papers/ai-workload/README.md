# AI 워크로드 스토리지 I/O 논문 리딩 리스트 (총 16편)

최근 2~3년(2023~2025) AI 시스템에서 발생하는 스토리지 I/O 워크로드와 최적화 기법을 다룬 논문을 5개 분류로 체계적으로 정리.
GPU-Storage I/O 논문(`papers/gpu-ssd/`)의 연장선으로, LLM/GNN/RAG 등 최신 AI 워크로드의 스토리지 병목과 해결책을 다룬다.

---

## 분류 개요

| 분류 | 디렉토리 | 편수 | 설명 |
|------|----------|------|------|
| W | `W-characterization/` | 3편 | 워크로드 특성화 & 서베이 — AI I/O 패턴의 전체 그림 |
| T | `T-training/` | 4편 | 트레이닝 스토리지 최적화 — 체크포인트, 데이터 로딩 |
| S | `S-serving/` | 5편 | LLM 추론/서빙 — KV Cache 오프로드, 다계층 캐싱 |
| G | `G-gnn/` | 2편 | GNN 스토리지 가속 — In-Storage/Near-Storage GNN |
| R | `R-rag/` | 2편 | RAG & 벡터 DB — 검색 증강 생성의 I/O |

---

## 추천 읽기 순서

### 1단계 — 전체 그림 파악 (워크로드 특성화)
1. **[W1] [ML I/O 360° 서베이](W-characterization/W1-ml-io-survey.md)** — AI I/O 전체 라이프사이클 서베이 ★필독
2. [W2] [LLM 데이터센터 특성화](W-characterization/W2-llm-datacenter.md) — 프로덕션 LLM 워크로드 실측
3. [W3] [85,000 체크포인트 분석](W-characterization/W3-85k-checkpoints.md) — 체크포인트 I/O 실태

### 2단계 — 트레이닝 I/O 병목과 해결
4. [T1] [DataStates-LLM](T-training/T1-datastates-llm.md) — 비동기 체크포인트 아키텍처
5. [T2] [PCcheck](T-training/T2-pccheck.md) — 병렬 체크포인트 (ASPLOS 2025)
6. [T3] [Smart-Infinity](T-training/T3-smart-infinity.md) — Near-Storage Processing으로 LLM 트레이닝 가속
7. [T4] [LMStor](T-training/T4-lmstor.md) — LLM 전용 스토리지 프레임워크

### 3단계 — 추론/서빙 스토리지 (최근 가장 활발) ★
8. **[S1] [Mooncake](S-serving/S1-mooncake.md)** — KVCache 중심 서빙 아키텍처 ★필독
9. [S2] [CachedAttention](S-serving/S2-cachedattention.md) — 멀티턴 대화 KV 재사용
10. **[S3] [NVMe KV Cache 오프로드](S-serving/S3-nvme-kv-offload.md)** — KV Cache→NVMe I/O 실측 ★필독
11. [S4] [INF²](S-serving/S4-inf2.md) — Near-Storage Attention 연산
12. [S5] [IMPRESS](S-serving/S5-impress.md) — 다계층 Prefix KV 스토리지

### 4단계 — GNN & RAG 특화 I/O
13. **[G1] [BeaconGNN](G-gnn/G1-beacongnn.md)** — In-Storage GNN 가속 ★필독
14. [G2] [FlashGNN](G-gnn/G2-flashgnn.md) — In-SSD GNN 가속기
15. [R1] [RAGX](R-rag/R1-ragx.md) — In-Storage RAG 가속
16. [R2] [Athena](R-rag/R2-athena.md) — RAG 파이프라인 벤치마크

---

## 전체 논문 인덱스

### W — 워크로드 특성화 & 서베이 (3편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| W1 | [**ML I/O 360° 서베이**](W-characterization/W1-ml-io-survey.md) | ACM Comp. Surveys 2024 | ML 라이프사이클 전체 I/O 패턴 서베이 ★필독 |
| W2 | [LLM 데이터센터 특성화](W-characterization/W2-llm-datacenter.md) | NSDI 2024 | GPU 데이터센터 6개월 LLM 워크로드 트레이스 분석 |
| W3 | [85,000 체크포인트](W-characterization/W3-85k-checkpoints.md) | PDSW 2025 | 18개 클러스터, 40개 작업의 체크포인트 I/O 실태 |

### T — 트레이닝 스토리지 최적화 (4편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| T1 | [DataStates-LLM](T-training/T1-datastates-llm.md) | HPDC 2024 | Lazy 비동기 체크포인트, State Provider 추상화 |
| T2 | [PCcheck](T-training/T2-pccheck.md) | ASPLOS 2025 | Persistent Concurrent Checkpointing |
| T3 | [Smart-Infinity](T-training/T3-smart-infinity.md) | HPCA 2024 | Samsung SmartSSD 기반 Near-Storage LLM 트레이닝 |
| T4 | [LMStor](T-training/T4-lmstor.md) | WWW Journal 2025 | 비동기 계층적 체크포인트 + 다단계 복구 |

### S — LLM 추론/서빙 스토리지 (5편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| S1 | [**Mooncake**](S-serving/S1-mooncake.md) | FAST 2025 | 더 많은 스토리지로 재계산 절약, KVCache 중심 서빙 ★필독 |
| S2 | [CachedAttention](S-serving/S2-cachedattention.md) | ATC 2024 | 멀티턴 대화에서 KV Cache 재사용으로 비용 절감 |
| S3 | [**NVMe KV 오프로드**](S-serving/S3-nvme-kv-offload.md) | CHEOPS 2025 | LLM 모델+KV Cache의 NVMe 오프로드 I/O 실측 ★필독 |
| S4 | [INF²](S-serving/S4-inf2.md) | arXiv 2025 | Near-Storage Processing으로 Attention 연산 오프로드 |
| S5 | [IMPRESS](S-serving/S5-impress.md) | FAST 2025 | 중요도 기반 다계층 Prefix KV 스토리지 |

### G — GNN 스토리지 가속 (2편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| G1 | [**BeaconGNN**](G-gnn/G1-beacongnn.md) | HPCA 2024 | In-Storage GNN, Out-of-Order 샘플링, Best Paper HM ★필독 |
| G2 | [FlashGNN](G-gnn/G2-flashgnn.md) | HPCA 2024 | In-SSD GNN 가속기, PCIe 병목 우회 |

### R — RAG 스토리지 가속 (2편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| R1 | [RAGX](R-rag/R1-ragx.md) | ISCA 2025 | In-Storage RAG 가속, 4.3× 처리량 향상 |
| R2 | [Athena](R-rag/R2-athena.md) | IISWC 2025 | RAG 파이프라인 벤치마크 프레임워크 |

---

## AI 워크로드별 I/O 특성 요약

```
┌──────────────────────────────────────────────────────────────────┐
│                  AI 워크로드 스토리지 I/O 전체 지도               │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─── 트레이닝 ─────────────────────────────────────────────┐   │
│  │                                                           │   │
│  │  데이터 로딩        체크포인트          통신               │   │
│  │  ┌──────────┐      ┌──────────┐      ┌──────────┐       │   │
│  │  │순차 읽기  │      │대량 쓰기  │      │All-Reduce│       │   │
│  │  │셔플→랜덤 │      │수백GB~TB  │      │Gradient  │       │   │
│  │  │멀티모달↑ │      │주기적     │      │Sync      │       │   │
│  │  └──────────┘      └──────────┘      └──────────┘       │   │
│  │  [W1,W2]           [W3,T1~T4]                            │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─── 추론/서빙 ────────────────────────────────────────────┐   │
│  │                                                           │   │
│  │  모델 로딩          KV Cache            Prefix 공유       │   │
│  │  ┌──────────┐      ┌──────────┐      ┌──────────┐       │   │
│  │  │순차 읽기  │      │동적 증가  │      │공통 접두사│       │   │
│  │  │수십~수백GB│      │컨텍스트↑ │      │캐싱/재사용│       │   │
│  │  │콜드스타트 │      │NVMe오프로드│     │다계층저장 │       │   │
│  │  └──────────┘      └──────────┘      └──────────┘       │   │
│  │  [S3]              [S1~S5]            [S2,S5]             │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─── GNN ──────────┐  ┌─── RAG ──────────────────────────┐    │
│  │                   │  │                                   │    │
│  │  그래프 탐색       │  │  임베딩 검색      문서 검색        │    │
│  │  ┌──────────┐    │  │  ┌──────────┐   ┌──────────┐    │    │
│  │  │극소 랜덤  │    │  │  │ANN 검색   │   │Top-K 읽기│    │    │
│  │  │읽기(~64B) │    │  │  │벡터 DB    │   │NVMe 계층 │    │    │
│  │  │불규칙패턴 │    │  │  │DRAM+NVMe │   │소규모I/O  │    │    │
│  │  └──────────┘    │  │  └──────────┘   └──────────┘    │    │
│  │  [G1,G2]          │  │  [R1,R2]                         │    │
│  └───────────────────┘  └──────────────────────────────────┘    │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

## GPU-Storage I/O 논문(`gpu-ssd/`)과의 연결

| AI 워크로드 I/O 패턴 | 관련 gpu-ssd 논문 | 연결 포인트 |
|----------------------|-------------------|------------|
| KV Cache → NVMe 오프로드 | BaM [I1], GDS [G1] | GPU-initiated I/O로 KV Cache 직접 읽기 |
| GNN 그래프 탐색 | GIDS [I2], BaM [I1] | 불규칙 랜덤 읽기 = BaM의 핵심 사용 사례 |
| 체크포인트 대량 쓰기 | GDS [G1], SPIN [G2] | P2P DMA로 GPU→SSD 직접 체크포인트 |
| RAG 벡터 검색 | FlashNeuron [P5] | NVMe를 AI 데이터의 확장 메모리로 활용 |
| Near-Storage AI 연산 | — (새로운 방향) | SSD 내부에서 AI 연산 수행 (BeaconGNN, INF²) |

---

## 핵심 기술 트렌드 (2023~2025)

1. **KV Cache가 새로운 I/O 병목**: LLM 컨텍스트 길이 증가(4K→128K→1M)로 KV Cache가 수십 GB까지 증가. GPU HBM 한계를 넘어 DRAM→NVMe 계층 오프로드 필수화
2. **체크포인트 = 트레이닝의 숨은 비용**: 모델 크기 증가로 체크포인트가 수백 GB~TB 단위. 비동기/병렬/압축 기법으로 GPU idle 시간 최소화가 핵심
3. **Near/In-Storage Processing 부활**: CSD(Computational Storage Device)를 활용해 데이터 이동 없이 SSD 내부에서 AI 연산 수행. GNN, RAG, Attention 등에 적용
4. **AI I/O는 전통 HPC I/O와 다르다**: 작은 랜덤 읽기, 불규칙 접근 패턴, 에폭 반복 등 기존 스토리지 최적화가 맞지 않는 새로운 I/O 특성
