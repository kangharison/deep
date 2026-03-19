# [R1] RAGX: In-Storage Acceleration of Retrieval-Augmented Generation as a Service

- **학회/연도:** ISCA 2025 (52nd Annual International Symposium on Computer Architecture, Tokyo)
- **저자:** Rohan Mahapatra, Harsha Santhanam, Christopher Priebe, Hanyang Xu, Hadi Esmaeilzadeh (UC San Diego)
- **분류:** RAG 스토리지 가속
- **소스코드:** https://github.com/rohanmahapatra/ragx

## 핵심 요약 (1~2문장)
RAG 파이프라인의 검색/증강 단계를 SSD 내부에서 수행하는 **메타모픽(Metamorphic) In-Storage 가속기** 아키텍처. Systolic 모드(임베딩 생성)와 Vector 모드(유사도 검색)를 동적으로 전환하며, Xeon CPU+NVMe 대비 최대 **4.3× end-to-end throughput 향상**, GPU-DRAM 대비 **최대 150× 에너지 효율** 향상을 달성한다.

## 읽기 전 질문
- RAG 파이프라인에서 스토리지 병목은 정확히 어디에서 발생하는가?
- "메타모픽" 가속기가 두 가지 모드를 전환하는 메커니즘은?
- NAND 직접 접근이 기존 NVMe I/O 스택 대비 어떤 이점을 제공하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- RAG: LLM에 외부 지식을 검색하여 제공 → 할루시네이션 감소, 최신 정보 반영
- RAG 파이프라인: **Query → 임베딩 생성 → 벡터 검색(ANN) → 문서 검색 → 증강 → LLM 생성**
- 벡터 DB + 문서 저장소가 NVMe SSD에 위치 → 검색 단계의 I/O가 병목
- 기존: CPU가 NVMe에서 벡터 읽기 → ANN 검색 → GPU로 전달 → 다단계 데이터 이동

### 2. 메타모픽 가속기 아키텍처

```
┌─────────────────────────────────────────────────┐
│              RAGX In-Storage 가속기              │
│                                                  │
│  ┌────────────────────────────────────────┐     │
│  │       Metamorphic Processing Array     │     │
│  │                                        │     │
│  │  Mode 1: Systolic Array               │     │
│  │  ┌──┬──┬──┬──┐                        │     │
│  │  │PE│PE│PE│PE│ → 임베딩 생성 (추론)   │     │
│  │  │PE│PE│PE│PE│   쿼리 인코딩          │     │
│  │  │PE│PE│PE│PE│                        │     │
│  │  └──┴──┴──┴──┘                        │     │
│  │                  ↕ 동적 전환           │     │
│  │  Mode 2: Vector Processors            │     │
│  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐        │     │
│  │  │SIMD│ │SIMD│ │SIMD│ │SIMD│        │     │
│  │  │ VP │ │ VP │ │ VP │ │ VP │        │     │
│  │  └────┘ └────┘ └────┘ └────┘        │     │
│  │  → 유사도 검색 (ANN, HNSW, 역색인)   │     │
│  └────────────────────────────────────────┘     │
│                                                  │
│  ┌────────────────────────────────────────┐     │
│  │  Metadata Navigation Unit (MNU)        │     │
│  │  NAND 배열에서 직접 데이터 로드        │     │
│  │  (NVMe 스택 우회)                      │     │
│  └────────────────────────────────────────┘     │
│                                                  │
│  ┌────────────────────────────────────────┐     │
│  │  NAND Flash Array                      │     │
│  │  임베딩 + 포스팅 리스트 + 문서         │     │
│  └────────────────────────────────────────┘     │
└─────────────────────────────────────────────────┘
```

**두 가지 동적 모드:**
1. **Systolic 모드**: 시스톨릭 배열로 신경망 추론 수행 → 쿼리 임베딩 생성
2. **Vector 모드**: 시스톨릭 배열의 열(column)을 SIMD 벡터 프로세서 집합으로 재구성 → ANN 검색

**Metadata Navigation Unit (MNU):**
- NAND 배열에서 임베딩이나 포스팅 리스트를 **직접 로드** (NVMe 스택 우회)
- 기존 경로: NAND → SSD 컨트롤러 → PCIe → 호스트 → GPU (다단계)
- RAGX: NAND → MNU → 가속기 온칩 메모리 (단일 단계)

**지원 알고리즘:**
- HNSW (Hierarchical Navigable Small World) — 벡터 검색
- Inverted Index — 스파스 검색
- SPLADEv2 등 다양한 임베딩 모델
- 컴파일러가 유사도 검사용 파라메트릭 커널을 자동 생성

### 3. 하드웨어 구현

| 항목 | 사양 |
|------|------|
| **구현** | Verilog, Synopsys Design Compiler 2023.09 |
| **공정** | FreePDK 45nm |
| **클록** | 1 GHz |
| **전력** | SSD의 **15W 전력 예산** 내 동작 |

### 4. 실험 결과

**평가 환경:** BioASQ 데이터셋, 3,800개 쿼리, median 지연시간 기준

| 비교 기준선 | Throughput 향상 | 에너지 효율 |
|-----------|:-----------:|:---------:|
| Xeon CPU + NVMe (검색) + DGX A100 (LLM) | **최대 4.3×** | — |
| A100 GPU + DRAM (검색) | **최대 1.5×** | **평균 50×, 최대 150×** |
| 벤치마크 범위 | **3.5× ~ 7.0×** | — |

### 5. 한계점 및 향후 연구
- 커스텀 하드웨어 필요 (45nm 시뮬레이션 기반 — 실제 제작 아님)
- 벡터 DB 업데이트(삽입/삭제)의 In-Storage 처리 복잡도
- 멀티모달 RAG(이미지+텍스트)에 대한 확장 필요

## 다른 논문과의 관계
- In-Storage AI 계보: [G1] BeaconGNN (GNN), [T3] Smart-Infinity (트레이닝), [S4] INF² (추론), **[R1] RAGX (RAG)**
- 공통 철학: "데이터를 이동시키지 말고 연산을 데이터로 보낸다"
- RAG I/O 특성: 벡터 유사도 검색 = 고차원 작은 랜덤 읽기 → GNN 그래프 탐색과 유사

## 발표 자료 활용 포인트
- **ISCA 2025** (컴퓨터 아키텍처 최고 학회) — 학술적 권위
- 메타모픽 가속기: Systolic↔Vector 동적 전환 다이어그램
- MNU의 NAND 직접 접근 vs 기존 NVMe 스택 경로 비교
- 에너지 효율 150× — 지속가능성 관점에서 임팩트

## 메모
- In-Storage Computing 논문들을 모으면 하나의 서베이 가능: BeaconGNN+FlashGNN(GNN) + Smart-Infinity+INF²(LLM) + RAGX(RAG)
- 메타모픽 = 하나의 가속기가 두 가지 역할 → 면적 효율성 극대화
- 15W 전력 예산 = SSD의 기존 전력 예산 내 → 추가 전력 인프라 불필요
