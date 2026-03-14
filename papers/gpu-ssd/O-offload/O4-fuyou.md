# [O4] Fuyou: Adding NVMe SSDs to Enable and Accelerate 100B Model Fine-tuning on a Single GPU

- **학회/연도:** arXiv 2024
- **저자:**
- **분류:** GPU-NVMe 오프로드

## 핵심 요약 (1~2문장)
단일 GPU(24GB~80GB)에서 100B 이상의 초대규모 모델을 NVMe SSD를 활용하여 파인튜닝할 수 있게 하는 시스템. 모델 파라미터, optimizer state, gradient를 NVMe SSD로 오프로드하고 효율적인 스와핑 메커니즘으로 ZeRO-Infinity 대비 높은 성능 달성.

## 읽기 전 질문
- 24GB GPU 하나로 100B 모델을 파인튜닝하려면 최소 몇 TB의 NVMe SSD 용량이 필요한가?
- ZeRO-Infinity와 비교하여 Fuyou의 핵심 아키텍처 차이는 무엇인가?
- 단일 GPU 파인튜닝에서 SSD 대역폭이 학습 속도의 병목이 되는 시점은 언제인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 100B+ 모델(GPT-3 175B, LLaMA-65B 등)의 파인튜닝에는 수 TB의 메모리가 필요
  - 모델 파라미터: FP16 기준 175B 모델 = 350GB
  - Optimizer state (Adam): 파라미터의 4배 = 1.4TB
  - Gradient: 파라미터와 동일 크기 = 350GB
- 대부분의 연구자는 고가의 multi-GPU 시스템을 사용해야 함
- ZeRO-Infinity는 NVMe 오프로드를 지원하지만 multi-GPU 분산 학습에 초점
- 단일 GPU에서 효율적으로 NVMe 오프로드하는 전용 시스템 부재

### 2. 제안 방법 (Approach)
- **계층적 오프로드 전략**: GPU VRAM → CPU DRAM → NVMe SSD 3단계 메모리 계층 활용
- **레이어 단위 스와핑**: 한 번에 하나의 레이어만 GPU에 로드하여 처리
- **비동기 프리페칭**: 현재 레이어 연산 중 다음 레이어를 미리 SSD에서 로드
- **Gradient Checkpointing 통합**: activation recomputation과 SSD 오프로드를 결합
- **LoRA/QLoRA 호환**: 파라미터 효율적 파인튜닝과 결합하여 메모리/I/O 부담 감소
- **동적 메모리 관리**: CPU DRAM을 캐시로 활용하여 자주 접근하는 텐서를 유지

### 3. 핵심 아키텍처/설계

```
  Fuyou 시스템 아키텍처:
  ┌──────────────────────────────────────────┐
  │              Training Loop               │
  │  for each iteration:                     │
  │    Forward: Layer 0 → 1 → ... → N       │
  │    Backward: Layer N → ... → 1 → 0      │
  │    Optimizer Step (레이어별)              │
  └────────────────┬─────────────────────────┘
                   │
  ┌────────────────▼─────────────────────────┐
  │  메모리 계층 관리자                       │
  │                                          │
  │  ┌─────────┐  활성 레이어                │
  │  │GPU VRAM │  (현재 연산 중인 레이어)    │
  │  │ 24~80GB │  파라미터 + gradient         │
  │  └────┬────┘                             │
  │       │ PCIe                             │
  │  ┌────▼────┐  캐시 레이어                │
  │  │CPU DRAM │  (다음 레이어 프리페치)      │
  │  │64~256GB │  optimizer state 일부        │
  │  └────┬────┘                             │
  │       │ PCIe / DMA                       │
  │  ┌────▼────┐  콜드 스토리지              │
  │  │NVMe SSD │  전체 파라미터              │
  │  │ 2~8 TB  │  전체 optimizer state       │
  │  └─────────┘  gradient history           │
  └──────────────────────────────────────────┘

  레이어별 스와핑 타임라인:
  ┌────────────────────────────────────────┐
  │ GPU:  [Compute L_i] [Compute L_i+1]   │
  │ DMA:  [Load L_i+1 ] [Load L_i+2   ]   │
  │ SSD:  [Read L_i+2 ] [Read L_i+3   ]   │
  └────────────────────────────────────────┘
  → 연산과 데이터 전송을 파이프라인으로 오버랩
```

### 4. 실험 결과 (Key Results)
- **단일 A100 80GB GPU**에서 GPT-NeoX 175B 모델 파인튜닝 성공
- **단일 RTX 3090 24GB GPU**에서 LLaMA-65B 파인튜닝 가능
- ZeRO-Infinity (단일 GPU 모드) 대비 **1.5~2.5x 학습 속도 향상**
- NVMe SSD 4개 RAID-0 구성 시 **대역폭 약 12~14 GB/s** 활용
- LoRA와 결합 시 메모리 요구량 추가 **60~80% 감소**
- CPU DRAM 캐시로 SSD I/O 횟수 **30~40% 절감**
- 파인튜닝 정확도는 full-GPU 학습과 동일 (bit-exact)

### 5. 한계점 및 향후 연구
- 학습 속도가 multi-GPU 시스템 대비 여전히 느림 (비용 효율성은 우수)
- NVMe SSD 개수에 비례하여 성능이 향상되지만, PCIe 레인 제한으로 확장성 한계
- Pre-training에는 적용 어려움 (파인튜닝에 초점)
- SSD 수명에 대한 고려 부족 (optimizer state 반복 쓰기)
- CPU DRAM이 매우 적은 환경에서는 성능 급격히 저하

## 다른 논문과의 관계
- 선행 연구: ZeRO-Infinity [P6] (NVMe 오프로드의 기초), FlashNeuron [P5] (SSD 오프로드 개척)
- 후속 연구: LoHan [O5] (비용 효율적 파인튜닝)
- 비교 대상: ZeRO-Infinity [P6] (주요 베이스라인), SSDTrain [O1] (activation 오프로드)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 3단계 메모리 계층 아키텍처도, ZeRO-Infinity 대비 성능 비교 그래프, 모델 크기별 SSD 용량 요구량 표
- 핵심 수치/데이터: 단일 24GB GPU에서 65B 파인튜닝, ZeRO-Infinity 대비 1.5~2.5x 속도 향상, NVMe 4개 RAID-0으로 12~14 GB/s

## 메모
