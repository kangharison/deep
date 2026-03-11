# [O2] TERAIO: Cost-Efficient LLM Training with Lifetime-Aware Tensor Offloading

- **학회/연도:** NeurIPS 2025
- **저자:**
- **분류:** GPU-NVMe 오프로드

## 핵심 요약 (1~2문장)
LLM 학습에서 텐서의 생존 시간(lifetime)을 분석하여 SSD 오프로드 대상과 배치를 최적화하는 프레임워크. SSD 수명(endurance)을 고려한 chunk 기반(2MiB) 텐서 관리로 성능과 SSD 마모를 동시에 최적화.

## 읽기 전 질문
- 텐서의 "lifetime"을 어떻게 정의하고 측정하며, 이를 기반으로 오프로드 결정을 어떻게 내리는가?
- SSD 수명(write endurance)을 고려한 오프로드 정책이 성능에 어떤 trade-off를 만드는가?
- 기존 SSDTrain/FlashNeuron 대비 어떤 차원에서 개선이 이루어졌는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 기존 NVMe 오프로드 시스템들은 SSD의 수명(DWPD, TBW)을 고려하지 않음
- LLM 학습은 수일~수주 동안 지속되며, 반복적인 대량 쓰기가 SSD를 빠르게 마모시킴
- 모든 텐서를 동일하게 취급하는 coarse-grained 오프로드는 비효율적
  - 짧은 생존 시간의 텐서는 오프로드 비용 대비 이득이 낮음
  - 큰 텐서와 작은 텐서를 동일하게 처리하면 I/O 효율 저하
- GPU 메모리 절약과 SSD 수명 보존 사이의 균형 필요

### 2. 제안 방법 (Approach)
- **Lifetime-Aware Profiling**: 학습 초기 iteration에서 각 텐서의 생성-소멸 시간 프로파일링
- **2MiB Chunk 기반 관리**: 텐서를 2MiB 단위로 분할하여 NVMe SSD의 최적 I/O 크기에 맞춤
- **Lifetime-Aware Scheduling**: 생존 시간이 긴 텐서를 우선적으로 SSD에 오프로드
- **Write Amplification 최소화**: chunk 정렬과 배치를 통해 SSD 내부 GC로 인한 write amplification 감소
- **동적 오프로드 정책**: SSD의 잔여 수명을 모니터링하여 오프로드 강도를 실시간 조절

### 3. 핵심 아키텍처/설계

```
  ┌──────────────────────────────────────────┐
  │            TERAIO Runtime                 │
  │  ┌──────────────┐  ┌──────────────────┐  │
  │  │  Lifetime    │  │  Chunk Manager   │  │
  │  │  Profiler    │  │  (2MiB 단위)     │  │
  │  │  (텐서 생존  │  │                  │  │
  │  │   시간 분석) │  │  ┌──┬──┬──┬──┐   │  │
  │  └──────┬───────┘  │  │C1│C2│C3│C4│   │  │
  │         │          │  └──┴──┴──┴──┘   │  │
  │         ▼          └────────┬─────────┘  │
  │  ┌──────────────┐          │             │
  │  │  Offload     │◄─────────┘             │
  │  │  Scheduler   │                        │
  │  │  (수명 인식) │                        │
  │  └──────┬───────┘                        │
  └─────────┼────────────────────────────────┘
            │
    ┌───────▼────────┐
    │  GPU VRAM      │
    │  ┌────────┐    │     비동기 DMA
    │  │ Hot    │    │◄──────────────┐
    │  │ Tensor │    │               │
    │  └────────┘    │     ┌─────────▼──────┐
    │  ┌────────┐    │     │   NVMe SSD     │
    │  │ Cold   │────┼────►│  ┌──────────┐  │
    │  │ Tensor │    │     │  │Cold Store│  │
    │  └────────┘    │     │  └──────────┘  │
    └────────────────┘     │  ┌──────────┐  │
                           │  │Lifetime  │  │
                           │  │Monitor   │  │
                           │  └──────────┘  │
                           └────────────────┘

  텐서 분류 기준:
  ─────────────────────────────────────────
  Lifetime 긴 텐서  → SSD 오프로드 (이득 큼)
  Lifetime 짧은 텐서 → GPU 유지 (오프로드 비용 > 이득)
  크기 작은 텐서    → GPU 유지 (I/O 효율 낮음)
```

### 4. 실험 결과 (Key Results)
- GPT-3, LLaMA, OPT 등 대규모 모델에서 실험
- SSDTrain 대비 **SSD 쓰기량 40~60% 감소** (수명 보존)
- 학습 속도는 SSDTrain과 유사하거나 약간 향상 (lifetime-aware 스케줄링으로 불필요한 I/O 제거)
- GPU 메모리 절약 효과 유지하면서 **SSD 수명 2~3배 연장**
- 2MiB chunk 정렬로 NVMe SSD의 **순차 쓰기 성능 90% 이상 달성**
- Recomputation 대비 학습 시간 **30~50% 단축**

### 5. 한계점 및 향후 연구
- Profiling 단계에서 추가 오버헤드 발생 (첫 수 iteration)
- 모델 구조가 바뀌면 재프로파일링 필요
- SSD 수명 모니터링은 SMART 데이터에 의존 → 벤더별 정확도 차이
- 분산 학습 환경에서의 확장성 검증 부족
- PCIe Gen5 이상 고속 NVMe에서의 성능 특성 분석 필요

## 다른 논문과의 관계
- 선행 연구: SSDTrain [O1] (GDS 기반 activation 오프로드), FlashNeuron [P5] (최초 SSD 오프로드)
- 후속 연구: SSD 수명 인식 스토리지 시스템으로의 확장 가능
- 비교 대상: MLP-Offload [O8] (다중 경로 오프로드와 비교)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 텐서 lifetime 분포 히스토그램, SSD 쓰기량 비교 그래프, chunk 기반 I/O 패턴 분석
- 핵심 수치/데이터: SSD 쓰기량 40~60% 감소, SSD 수명 2~3배 연장, 2MiB chunk로 순차 쓰기 성능 90% 달성

## 메모
