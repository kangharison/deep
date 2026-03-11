# [G4] Storage Access Optimization for Efficient GPU-centric Information Retrieval (ESPN)

- **학회/연도:** J. Supercomputing 2025
- **저자:**
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
GDS를 활용한 GPU 중심 정보 검색(Information Retrieval) 시스템 ESPN 제안. 인덱스 데이터를 SSD에서 GPU 메모리로 직접 전송하여 쿼리 latency를 최대 3.9배 감소시키고, 대규모 인덱스의 GPU 메모리 한계를 극복.

## 읽기 전 질문
- GPU 메모리에 다 올리지 못하는 대규모 검색 인덱스를 어떻게 효율적으로 처리하는가?
- GDS가 정보 검색 워크로드에서 구체적으로 어떤 단계의 latency를 줄이는가?
- 기존 CPU 기반 검색 엔진과 비교 시 GPU+GDS 조합의 비용 대비 성능은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 대규모 정보 검색(IR) 시스템에서 인덱스 크기가 GPU 메모리를 초과하는 경우가 빈번
- 기존 GPU 검색 시스템: 인덱스 전체를 GPU 메모리에 로드 → 메모리 한계에 직면
- 인덱스를 SSD에 두고 필요 부분만 GPU로 가져오려면 I/O 경로가 병목
- 전통적 경로(SSD → CPU → GPU)는 데이터 복사 오버헤드로 쿼리 latency 증가

### 2. 제안 방법 (Approach)
- **ESPN (Efficient Storage-GPU Pipeline for Nearest-neighbor search)**:
  - GDS(cuFile API)를 사용하여 SSD에서 GPU 메모리로 인덱스 세그먼트를 직접 전송
  - 쿼리 처리 파이프라인을 I/O와 GPU 연산으로 분리하여 오버랩
  - 인덱스 파티셔닝: 자주 접근하는 파티션은 GPU 메모리에 캐시, 나머지는 SSD
  - 비동기 I/O + GPU 연산 파이프라이닝으로 I/O latency 은폐

### 3. 핵심 아키텍처/설계

```
  ESPN 파이프라인 아키텍처:

  ┌─────────────────────────────────────────────┐
  │              Query Processing               │
  │                                             │
  │  ① Query 수신 → 필요 인덱스 파티션 결정       │
  │  ② GDS로 SSD → GPU 직접 전송 (비동기)        │
  │  ③ GPU에서 검색 연산 (ANN/KNN)               │
  │  ④ 결과 반환                                 │
  └─────────────────────────────────────────────┘

  인덱스 관리:
  ┌──────────────────┐
  │    GPU Memory    │
  │ ┌──────────────┐ │
  │ │ Hot Partitions│ │  ← 자주 접근, 상주
  │ │ (캐시됨)      │ │
  │ ├──────────────┤ │
  │ │ I/O Buffer   │ │  ← GDS로 동적 로드
  │ └──────────────┘ │
  └──────────────────┘
          ▲ GDS (P2P DMA)
          │
  ┌──────────────────┐
  │    NVMe SSD      │
  │ ┌──────────────┐ │
  │ │Cold Partitions│ │  ← 필요 시 로드
  │ │ (전체 인덱스) │ │
  │ └──────────────┘ │
  └──────────────────┘

  파이프라이닝:
  Time ──────────────────────────────────►
  Query 1: [I/O Load][GPU Search][Return]
  Query 2:      [I/O Load][GPU Search][Return]
  Query 3:           [I/O Load][GPU Search][Return]
           (I/O와 연산 오버랩으로 latency 은폐)
```

### 4. 실험 결과 (Key Results)
- **쿼리 Latency**: 전통적 CPU→GPU 경로 대비 **3.9배 감소**
- **Throughput**: GPU 기반 검색 + GDS로 QPS(Queries Per Second) 대폭 향상
- **메모리 효율**: GPU 메모리의 일부만으로 전체 인덱스 수준의 검색 품질 유지
- **SSD 대역폭 활용**: GDS로 NVMe SSD 대역폭을 거의 풀로 활용
- **스케일링**: 인덱스 크기 증가에도 성능 저하가 완만 (SSD 용량으로 확장)

### 5. 한계점 및 향후 연구
- GPU 메모리 캐시 관리 정책(어떤 파티션을 유지할지)의 최적화 여지
- 매우 작은 I/O(개별 벡터 단위)에서는 GDS 오버헤드가 상대적으로 큼
- 단일 GPU, 단일 SSD 구성에서 주로 실험 — 멀티 GPU/SSD 스케일링 미검증
- 인덱스 업데이트(삽입/삭제) 시나리오는 다루지 않음
- 특정 ANN(Approximate Nearest Neighbor) 알고리즘에 최적화 — 범용성 제한

## 다른 논문과의 관계
- 선행 연구: GDS [G1], GPU 기반 ANN 검색 연구 (FAISS 등)
- 후속 연구: GPU 기반 벡터 DB + GDS 통합
- 비교 대상: BaM [I1] — GPU-initiated I/O로 유사 문제 해결

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 쿼리 latency 비교 차트, 파이프라인 구조도
- 핵심 수치/데이터: 쿼리 latency 3.9배 감소, SSD 대역폭 활용률
- **활용**: GDS의 실제 애플리케이션 적용 사례 — "벤치마크를 넘어 실제 워크로드에서의 GDS 효과"

## 메모
- RAG/벡터 검색이 중요해지는 LLM 시대에 특히 관련성 높은 연구
