# [O8] MLP-Offload: Multi-Level, Multi-Path Offloading for LLM

- **학회/연도:** SC 2025
- **저자:**
- **분류:** GPU-NVMe 오프로드

## 핵심 요약 (1~2문장)
DeepSpeed ZeRO-3/Infinity의 NVMe 오프로드 경로를 다중 레벨(GPU↔CPU↔SSD)과 다중 경로(여러 NVMe 디바이스 병렬)로 재설계하여 대역폭 활용을 극대화하는 프레임워크. 기존 단일 경로 오프로드의 병목을 제거하고 LLM 학습에서 NVMe 오프로드 효율성을 크게 개선.

## 읽기 전 질문
- "Multi-Level"과 "Multi-Path"가 각각 구체적으로 어떤 계층과 경로를 의미하는가?
- DeepSpeed ZeRO-3의 기존 NVMe 오프로드 구현에서 가장 큰 병목은 어디인가?
- 여러 NVMe SSD를 병렬로 사용할 때 소프트웨어 레벨에서의 최적화 방법은 무엇인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- DeepSpeed ZeRO-Infinity는 NVMe 오프로드를 지원하지만 효율이 낮음
  - **단일 경로 병목**: 모든 데이터가 하나의 I/O 경로를 통해 이동 → NVMe 대역폭 미활용
  - **동기적 I/O**: 오프로드와 연산이 순차적으로 수행 → 파이프라인 비효율
  - **coarse-grained 관리**: 모든 텐서를 동일하게 취급하여 I/O 우선순위 미적용
- 서버에 NVMe SSD가 여러 개 장착되어 있어도 대역폭을 충분히 활용하지 못함
- CPU DRAM 중간 버퍼링이 불필요한 경우에도 강제로 CPU를 경유하는 비효율

### 2. 제안 방법 (Approach)
- **Multi-Level Offload**: 텐서 종류에 따라 최적 오프로드 레벨 선택
  - Level 0: GPU VRAM (오프로드 안 함)
  - Level 1: CPU DRAM (빠른 접근, 제한된 용량)
  - Level 2: NVMe SSD (대용량, 느린 접근)
  - 텐서별로 최적 레벨 자동 결정
- **Multi-Path Transfer**: 여러 NVMe SSD에 데이터를 분산하여 병렬 전송
  - RAID-0 스타일의 스트라이핑을 소프트웨어 레벨에서 구현
  - NVMe 디바이스별 독립 I/O 큐 할당
- **비동기 파이프라이닝**: 연산, CPU-GPU 전송, SSD I/O를 3단계로 오버랩
- **Priority-Based Scheduling**: 곧 필요한 텐서를 우선적으로 전송
- **GDS(GPUDirect Storage) 선택적 활용**: 가능한 경우 CPU 바이패스

### 3. 핵심 아키텍처/설계

```
  MLP-Offload 다중 레벨/다중 경로 아키텍처:
  ┌─────────────────────────────────────────────┐
  │           MLP-Offload Scheduler              │
  │  ┌───────────────────────────────────────┐   │
  │  │  Tensor Classifier                    │   │
  │  │  - Parameters → Level 2 (SSD)         │   │
  │  │  - Optimizer State → Level 2 (SSD)    │   │
  │  │  - Gradients → Level 1 (CPU DRAM)     │   │
  │  │  - Activations → Level 0/1 (GPU/CPU)  │   │
  │  └───────────────┬───────────────────────┘   │
  │                  │                            │
  │  ┌───────────────▼───────────────────────┐   │
  │  │  Priority Queue                       │   │
  │  │  - 긴급도(urgency) 기반 I/O 스케줄링  │   │
  │  │  - 다음 사용 시점까지의 시간 기반 정렬 │   │
  │  └───────────────┬───────────────────────┘   │
  └──────────────────┼───────────────────────────┘
                     │
       ┌─────────────┼─────────────┐
       ▼             ▼             ▼
  ┌─────────┐  ┌──────────┐  ┌──────────────┐
  │Level 0  │  │Level 1   │  │Level 2       │
  │GPU VRAM │  │CPU DRAM  │  │NVMe SSDs     │
  │         │  │          │  │              │
  │활성 텐서│  │Gradient  │  │┌────┐┌────┐  │
  │         │  │버퍼      │  ││SSD0││SSD1│  │
  └────┬────┘  └────┬─────┘  │└──┬─┘└──┬─┘  │
       │            │        │┌──┴─┐┌──┴─┐  │
       │            │        ││SSD2││SSD3│  │
       │            │        │└────┘└────┘  │
       └────────────┘        └──────────────┘
         PCIe                  Multi-Path I/O

  기존 ZeRO-Infinity vs MLP-Offload:
  ─────────────────────────────────────────
  ZeRO-Infinity:
  GPU ←→ CPU DRAM ←→ SSD (단일 경로, 순차)

  MLP-Offload:
  GPU ←→ CPU DRAM ←→ SSD0 ┐
       ↕ (GDS)       SSD1 ├─ 병렬 전송
                      SSD2 │
                      SSD3 ┘

  파이프라인 비교:
  ZeRO-Infinity:
  [SSD Read][CPU→GPU][Compute][GPU→CPU][SSD Write]

  MLP-Offload:
  [SSD Read (4x 병렬)][Compute          ]
            [CPU→GPU  ][SSD Read (next)  ]
                       [GPU→CPU][SSD Write (4x 병렬)]
```

### 4. 실험 결과 (Key Results)
- GPT-NeoX, LLaMA, OPT 등 대규모 LLM에서 실험
- DeepSpeed ZeRO-Infinity 대비 **1.5~3x 학습 처리량(throughput) 향상**
- NVMe SSD 4개 사용 시 단일 SSD 대비 **대역폭 약 3.5~3.8x 확장** (이론적 4x에 근접)
- Multi-level 배치로 CPU DRAM 사용량을 **40~50% 절감** (일부 텐서를 SSD로 이동)
- 비동기 파이프라이닝으로 I/O 대기 시간 **60~70% 감소**
- 100B+ 모델을 단일 노드(8 GPU)에서 안정적으로 학습 가능
- GDS 활용 시 CPU 경유 대비 추가 **15~25% 성능 향상**

### 5. 한계점 및 향후 연구
- DeepSpeed 프레임워크에 의존적 → 다른 학습 프레임워크(PyTorch FSDP 등)에는 직접 적용 어려움
- NVMe SSD 개수 증가 시 PCIe 레인 경쟁 문제 → PCIe 스위치나 CXL 필요
- SSD 수명(endurance) 고려 부족 → TERAIO와 결합 가능성
- Multi-node 분산 학습에서의 네트워크 I/O와의 간섭 분석 부족
- Priority scheduling의 오버헤드가 소규모 모델에서는 오히려 성능 저하 가능

## 다른 논문과의 관계
- 선행 연구: ZeRO-Infinity [P6] (기반 시스템), DeepSpeed ZeRO-3 (확장 대상)
- 후속 연구: NVMe 오프로드 경로 최적화의 발전
- 비교 대상: TERAIO [O2] (수명 인식 오프로드), SSDTrain [O1] (activation 오프로드)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: Multi-path I/O 아키텍처도, ZeRO-Infinity 대비 성능 비교 그래프, NVMe 개수별 대역폭 확장 그래프
- 핵심 수치/데이터: ZeRO-Infinity 대비 1.5~3x throughput 향상, 4개 SSD로 3.5~3.8x 대역폭, I/O 대기 60~70% 감소

## 메모
