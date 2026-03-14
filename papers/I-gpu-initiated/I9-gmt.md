# [I9] GMT: GPU Orchestrated Memory Tiering for the Big Data Era

- **학회/연도:** ASPLOS 2024
- **저자:** Qureshi et al. (UIUC 등, BaM 후속)
- **분류:** GPU Initiated I/O + Memory Tiering

## 핵심 요약 (1~2문장)
GPU가 주도하여 3계층 메모리(GPU VRAM, 호스트 DRAM, NVMe SSD)를 통합 관리하는 메모리 티어링 시스템. GPU 커널이 데이터 접근 패턴을 실시간으로 모니터링하고, hot/warm/cold 데이터를 적절한 계층에 배치하는 GPU-orchestrated 메모리 관리 정책으로 대규모 데이터 처리 성능을 극대화.

## 읽기 전 질문
- GPU가 메모리 티어링을 직접 관리할 때, CPU OS의 페이지 관리(Linux mm)와 어떻게 차별화되는가?
- 3계층 메모리 간 데이터 이동(promotion/demotion) 결정을 GPU가 얼마나 빠르게 내릴 수 있는가?
- BaM의 on-demand access와 GMT의 tiering은 어떤 관계인가? 상호 보완적인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GPU 메모리 용량 부족**: GPU VRAM(40~80GB)으로는 TB급 데이터셋 처리 불가. 호스트 DRAM도 수백 GB 수준에서 한계
- **기존 메모리 관리의 비효율**:
  - **CPU 주도 티어링**: CPU OS가 페이지 교체를 관리하면 GPU 접근 패턴을 정확히 파악 불가. GPU와 CPU 간 동기화 오버헤드
  - **정적 데이터 배치**: 데이터를 VRAM/DRAM/SSD에 미리 고정 배치하면, 접근 패턴 변화에 대응 불가
  - **NVIDIA Unified Memory**: 페이지 폴트 기반 관리는 대규모 random access에서 성능 저하 심각
- **3계층 관리 복잡성**: VRAM↔DRAM, DRAM↔SSD 두 경계에서의 데이터 이동을 조율하는 것이 어려움

### 2. 제안 방법 (Approach)
- **GPU-Orchestrated Tiering**: GPU 커널 자체가 데이터 접근을 모니터링하고, promotion(SSD→DRAM→VRAM)/demotion(VRAM→DRAM→SSD) 결정을 내림
- **3계층 통합 관리**:
  1. **Hot 데이터**: GPU VRAM에 유지 (최고 대역폭, 최저 지연)
  2. **Warm 데이터**: 호스트 DRAM에 배치 (UVA/P2P로 접근, 중간 성능)
  3. **Cold 데이터**: NVMe SSD에 저장 (BaM 스타일 GPU-initiated I/O로 접근)
- **접근 카운터 기반 정책**: GPU가 페이지/블록별 접근 빈도를 추적하여 hot/warm/cold 분류
- **비동기 데이터 이동**: 데이터 이동(migration)을 백그라운드에서 비동기적으로 수행하여, 연산과 데이터 이동을 overlap

### 3. 핵심 아키텍처/설계

```
┌──────────────────────────────────────────────────────┐
│                  GPU Kernel                           │
│                                                      │
│  ┌──────────────────────────────────────┐            │
│  │     Access Monitor (접근 패턴 추적)  │            │
│  │  page_id → access_count 테이블      │            │
│  │  Hot/Warm/Cold 분류 결정            │            │
│  └──────────────┬───────────────────────┘            │
│                 │                                    │
│  ┌──────────────▼───────────────────────┐            │
│  │     Tier Manager (GPU-orchestrated)  │            │
│  │                                      │            │
│  │  ┌─────────────────────────┐         │            │
│  │  │ Promote: Cold→Warm→Hot │         │            │
│  │  │ Demote:  Hot→Warm→Cold │         │            │
│  │  │ (비동기 백그라운드)     │         │            │
│  │  └─────────────────────────┘         │            │
│  └──────────────────────────────────────┘            │
│                                                      │
│  ┌─────────────────────────────────────────────────┐ │
│  │              3-Tier Memory Hierarchy             │ │
│  │                                                 │ │
│  │  Tier 0: GPU VRAM (Hot)                        │ │
│  │  ┌──────────────────────┐   ~80GB, ~2TB/s     │ │
│  │  │ ████████ Hot Data    │                      │ │
│  │  └──────────────────────┘                      │ │
│  │           ▲ promote / ▼ demote                 │ │
│  │  Tier 1: Host DRAM (Warm)                      │ │
│  │  ┌──────────────────────┐   ~256GB, ~50GB/s   │ │
│  │  │ ████████ Warm Data   │   (PCIe P2P)        │ │
│  │  └──────────────────────┘                      │ │
│  │           ▲ promote / ▼ demote                 │ │
│  │  Tier 2: NVMe SSD (Cold)                       │ │
│  │  ┌──────────────────────┐   ~TB, ~6GB/s       │ │
│  │  │ ████████ Cold Data   │   (GPU-initiated)   │ │
│  │  └──────────────────────┘                      │ │
│  └─────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

**데이터 이동 정책**:
```
접근 빈도 기반 분류:
  access_count > T_hot   → Tier 0 (VRAM) 으로 promote
  T_warm < count < T_hot → Tier 1 (DRAM) 에 유지
  count < T_warm         → Tier 2 (SSD) 로 demote

Hysteresis 적용:
  - promote 임계값과 demote 임계값을 다르게 설정하여 thrashing 방지
  - 최근 접근 시간도 고려 (LRU + frequency 결합)
```

- **BaM과의 통합**: Tier 2(SSD) 접근 시 BaM의 GPU-initiated NVMe I/O를 그대로 활용
- **Prefetch 엔진**: 접근 패턴 분석을 통해 다음에 필요할 데이터를 미리 상위 계층으로 이동

### 4. 실험 결과 (Key Results)
- **기존 방식 대비 성능**:
  - NVIDIA Unified Memory 대비 **2~5배** 성능 향상 (대규모 random access 워크로드)
  - 정적 데이터 배치 대비 **1.5~3배** 성능 향상 (접근 패턴이 동적으로 변하는 워크로드)
- **워크로드별 성능**:
  - 그래프 분석(BFS/PageRank): VRAM에 모든 데이터가 들어가지 않는 대규모 그래프에서 **BaM(단독) 대비 30~50% 추가 향상** (hot 데이터를 VRAM에 유지하는 효과)
  - 추천 시스템(embedding lookup): 인기 embedding을 VRAM에 캐시하여 평균 접근 지연 대폭 절감
- **메모리 효율**: 전체 데이터의 **10~20%**만 VRAM에 유지해도 대부분의 성능 확보 (power-law 접근 패턴 활용)
- **데이터 이동 오버헤드**: 비동기 이동으로 연산 성능에 미치는 영향 **5% 미만**
- **3계층 vs 2계층**: VRAM+SSD 2계층만 사용하는 것 대비, DRAM을 중간 계층으로 추가하면 **20~40%** 추가 성능 향상

### 5. 한계점 및 향후 연구
- **접근 모니터링 오버헤드**: GPU에서 페이지별 접근 카운터를 유지하려면 추가 메모리와 연산 필요. 매우 fine-grained 추적은 비용이 높음
- **정책 최적화 어려움**: 임계값(T_hot, T_warm) 설정이 워크로드에 따라 크게 달라짐. 자동 튜닝 메커니즘 필요
- **Write 워크로드**: Read 위주 워크로드에 최적화. Write-heavy 워크로드에서는 demotion 시 write-back 비용 발생
- **Multi-GPU**: 여러 GPU 간 데이터 공유/이동에 대한 고려 제한적
- **CXL 메모리 통합**: CXL 메모리가 추가되면 4계층 관리가 필요해져 복잡성 증가 (→ CXL-BaM [I8]과 결합 가능)

## 다른 논문과의 관계
- 선행 연구: BaM [I1] (Tier 2 SSD 접근의 기반 인프라)
- 후속 연구: CXL 메모리를 포함한 4계층 관리, AI 학습 전용 메모리 관리
- 비교 대상: G10 [O6] (GPU 메모리 관리), HetCache [O7] (이종 캐시), NVIDIA Unified Memory
- 관련: CXL-BaM [I8] (CXL 계층 추가 시 GMT 확장 가능)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure: 3계층 메모리 아키텍처 다이어그램
  - Figure: 접근 패턴에 따른 데이터 이동(promotion/demotion) 시각화
  - Table: Unified Memory, BaM, GMT 성능 비교
  - Figure: 메모리 비율별 성능 곡선 (VRAM에 할당하는 비율 vs 성능)
- 핵심 수치/데이터:
  - Unified Memory 대비 2~5배 성능 향상
  - 데이터의 10~20%만 VRAM에 유지해도 대부분의 성능 확보
  - DRAM 중간 계층 추가로 20~40% 추가 성능 향상

## 메모
- GMT는 BaM의 "상위 레이어"로 이해 가능: BaM이 SSD 접근 메커니즘이라면, GMT는 언제 어떤 데이터를 SSD에서 가져올지 결정하는 정책
- CPU의 Linux mm (페이지 교체, NUMA balancing 등)과 개념적으로 유사하지만, GPU 워크로드에 특화
- 향후 LLM 추론에서 KV cache + weight offloading의 멀티 티어 관리에 직접 적용 가능한 아이디어
