# [T1] DataStates-LLM: Lazy Asynchronous Checkpointing for Large Language Models

- **학회/연도:** HPDC 2024 (Pisa, Italy)
- **저자:** Avinash Maurya, M. Mustafa Rafique (Rochester Institute of Technology), Robert Underwood, Franck Cappello, Bogdan Nicolae (Argonne National Laboratory)
- **분류:** 트레이닝 스토리지 최적화
- **소스코드:** https://github.com/DataStates/datastates-llm

## 핵심 요약 (1~2문장)
LLM 트레이닝의 체크포인트 I/O 오버헤드를 줄이기 위해, forward/backward pass 중 모델 파라미터의 불변성을 활용한 lazy 비동기 체크포인트 아키텍처. ALCF Polaris(512 A100 GPU)에서 DeepSpeed/Megatron 기반 3B~70B 모델을 검증하여, 동기 대비 최대 48배, TorchSnapshot 대비 4.7배 빠른 체크포인트 throughput을 달성한다.

## 읽기 전 질문
- "Lazy" 체크포인트란 무엇이며, 기존 동기/비동기 체크포인트와 정확히 어떻게 다른가?
- Forward/backward pass 중 파라미터가 불변이라는 관찰이 왜 핵심인가?
- 기존 비동기 방식(Nebula, CheckFreq)이 왜 이론적 대역폭의 일부만 달성하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)

**기존 체크포인트의 낭비:**
- Alibaba Unicron 보고: LLM 트레이닝 **43.4% 장애율**, 37% 하드웨어 장애
- 기존 비동기 솔루션의 실제 대역폭:
  - Gemini: 3.13 GB/s
  - REFT: 6 GB/s (PCIe 대역폭의 38%)
  - TRANSOM: ~1.2 GB/s
  - Nebula (DeepSpeed): 1~4 GB/s
- **이론적으로 50+ GB/s 가능한데 실제는 1~6 GB/s** → 10배 이상의 gap

**근본 원인:**
- 매 체크포인트마다 Host Memory 할당/해제 오버헤드
- GPU→Host 복사가 트레이닝 통신과 PCIe 경합
- 글로벌 동기화 대기

### 2. 핵심 관찰: 파라미터 불변성 (Figure 4)

```
하나의 트레이닝 iteration:

  ┌─────────────┐   ┌─────────────┐   ┌──────┐
  │ Forward Pass │──→│Backward Pass│──→│Update│
  │   (F)       │   │   (B)       │   │ (U)  │
  └─────────────┘   └─────────────┘   └──────┘
       ◀── 모델 파라미터+옵티마이저 상태 불변 ──▶  변경

  대형 모델에서 F+B가 iteration의 >95% 차지, U는 무시할 수준
  → F+B 동안 GPU→Host 비동기 복사해도 일관성 문제 없음!
```

**핵심 통찰:** "Both the model parameters and the optimizer state remain immutable during both the forward pass and the backward pass. Thus, any copies from the GPU memory to the host memory can be issued asynchronously during the forward pass and the backward pass without causing coherency issues."

### 3. 제안 아키텍처: 4가지 설계 원칙

#### 원칙 1: GPU 샤드 결합 (Coalescing)
- 사전 할당된 고정(pinned) Host Memory 버퍼에 모든 샤드를 결합
- 매번 할당/해제하지 않고 순환 버퍼 재사용 → 할당 오버헤드 제거

#### 원칙 2: Lazy Non-Blocking 복사
```
기존 비동기:
  Iteration N  ──→ STOP ──[GPU→Host 복사(blocking)]──→ [Host→Disk 비동기] ──→ N+1

DataStates-LLM (Lazy):
  Iteration N  ──→  Iteration N+1 [F+B 동안 GPU→Host 비동기 복사 진행]
                     │                                       │
                     └── 체크포인트 요청 시점                  └── 복사 완료
                     (blocking 없이 즉시 다음 iteration 시작)
```
- PCIe 링크를 활용 (NVLink/GPUDirect RDMA와 다른 물리적 경로 → 트레이닝 통신과 비경합)

#### 원칙 3: 스트리밍 다단계 플러싱
- GPU→Host 부분 복사 완료 즉시 Host→Disk 쓰기 시작 (대기 없음)
- GPU-to-Host HW 복사 엔진과 Host-to-Disk I/O 경로가 별도 물리 링크 → 동시 수행

#### 원칙 4: 비동기 분산 합의
- 2-phase commit: GPU 샤드 플러시 완료 후 검증
- 계층적 검증: 단일 GPU → 같은 노드 → 크로스 노드
- 합의 프로토콜이 다음 iteration과 중첩

### 4. 실험 환경: ALCF Polaris

| 항목 | 사양 |
|------|------|
| **노드** | 560개 (최대 128노드 사용) |
| **GPU** | 4× A100 40GB/노드 (최대 512 GPU) |
| **CPU** | 32코어 AMD Zen 3 (Milan) |
| **Host Memory** | 512 GB DDR4 |
| **로컬 SSD** | 2× 1.6 TB (2 GB/s) |
| **GPU→Host 대역폭** | 25 GB/s (PCIe Gen4, pinned) |
| **GPU↔GPU 대역폭** | 85 GB/s (NVLink) |
| **PFS** | Lustre: 160 OST, 40 MDS, **650 GB/s** 집계 대역폭 |

**모델 구성 (Table 1):**

| 모델 | 레이어 | Hidden Dim | Attn Heads | 노드 | TP | PP |
|------|--------|-----------|-----------|------|:--:|:--:|
| BLOOM 3B | 30 | 2560 | 32 | 1 | 4 | 1 |
| LLaMA 7B | 32 | 4096 | 32 | 2 | 4 | 2 |
| LLaMA2 13B | 40 | 5120 | 40 | 4 | 4 | 4 |
| LLaMA 30B | 60 | 6656 | 52 | 8 | 4 | 8 |
| LLaMA2 70B | 80 | 8192 | 64 | 20 | 4 | 20 |

- 데이터셋: OSCAR-en (79K records), 시퀀스 길이 2048, 마이크로 배치 16
- 병렬화: TP=4 (노드 내), PP=노드 수, ZeRO Stage 1

### 5. 성능 결과 (상세)

#### 집계 체크포인트 Throughput (Figure 7)

| 방식 | 3B | 13B | 30B | 70B |
|------|:--:|:---:|:---:|:---:|
| DeepSpeed 동기 | ~5 GB/s | ~10 GB/s | ~3 GB/s | ~5 GB/s |
| 비동기(CheckFreq) | ~15 GB/s | ~20 GB/s | ~10 GB/s | ~18 GB/s |
| TorchSnapshot | ~30 GB/s | ~35 GB/s | ~12 GB/s | ~40 GB/s |
| **DataStates-LLM** | **~80 GB/s** | **~100 GB/s** | **~140 GB/s** | **~180 GB/s** |

#### 기준선 대비 배수

| 대비 | 3B | 13B | 30B | 70B |
|------|:--:|:---:|:---:|:---:|
| vs DeepSpeed 동기 | 4× | 2.8× | **48×** | **34×** |
| vs 비동기 | — | 1.75× | 4.12× | — |
| vs TorchSnapshot | — | 1.78× | **4.7×** | — |

#### End-to-End 트레이닝 가속

| 모델 | 가속 범위 |
|------|----------|
| 7B (빈도별) | 최대 **3.86×** 빠른 종단간 트레이닝 |
| 13B (빈도별) | 최대 **3.86×** |
| 30B (DP 1→16) | 2.5× ~ 1.86× |
| 전체 | **1.3× ~ 3.86×** 종단간 가속 |

#### 체크포인트 크기 (Figure 3)
- GPU당 체크포인트 크기: 모델 크기 무관하게 **10~15 GB/GPU** (DeepSpeed의 균형 분배)
- 집계 크기: 3B ~30GB → 70B ~1.5TB

### 6. 한계점 및 향후 연구
- 높은 체크포인트 빈도 + 작은 모델: Host Memory에 데이터 누적 속도가 플러시 속도 초과 가능
- 매 샤드를 개별 파일로 저장 → 메타데이터 병목 (파일 집계 전략 필요)
- 향후: 차등 체크포인트, 압축 기법, 다계층 메모리 오프로딩

## 다른 논문과의 관계
- 동기 부여: [W2] Acme 장애율 40%, [W3] 체크포인트 실측
- 비교: [T2] PCcheck (persistent concurrent — 다른 접근), [T4] LMStor (계층적)
- 기반: SPDK [P7]의 비동기 I/O 철학을 체크포인트에 적용
- gpu-ssd 연결: GPU→NVMe 직접 체크포인트에 GDS P2P DMA로 PCIe 경합 해소 가능

## 발표 자료 활용 포인트
- Figure 4: F/B pass 중 파라미터 불변성 → "lazy 복사의 창" 시각화
- Figure 7: 4개 방식의 throughput 비교 바 차트 (DataStates-LLM이 압도적)
- "기존 방식이 이론 대역폭의 6~12%만 사용" → DataStates-LLM이 이를 해결
- 핵심 수치: 70B 모델에서 180 GB/s throughput, 동기 대비 34×

## 메모
- PCIe GPU→Host와 NVLink/RDMA 트레이닝 통신이 물리적으로 다른 경로 → lazy 복사가 트레이닝에 영향 안 줌
- GPU당 10~15GB = PCIe Gen4 25GB/s로 ~0.5초면 복사 완료 → F+B가 수초~수십초이므로 충분한 시간적 여유
- 오픈소스 → 직접 실험 가능: https://github.com/DataStates/datastates-llm
