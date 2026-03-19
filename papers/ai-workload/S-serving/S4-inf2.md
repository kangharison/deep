# [S4] INF²: High-Throughput Generative Inference of Large Language Models using Near-Storage Processing

- **학회/연도:** arXiv 2025 (2502.09921)
- **저자:** Hongsun Jang, Siung Noh, Changmin Shin, Jaewon Jung, Jaeyong Song, Jinho Lee (Seoul National Univ.), Youngsok Kim (Yonsei Univ.)
- **분류:** LLM 추론/서빙 스토리지
- **참고:** [T3] Smart-Infinity와 같은 연구 그룹 (SNU), 같은 하드웨어(SmartSSD)

## 핵심 요약 (1~2문장)
CSD(Samsung SmartSSD) 내부의 FPGA 가속기에서 LLM Self-Attention 연산을 수행하여, KV Cache를 GPU로 전송하는 대신 연산 결과만 반환하는 Near-Storage Attention 시스템. 16대 CSD에서 FlexGen 대비 최대 **3.46× throughput 향상**을 달성하며, "데이터 이동 대신 연산을 데이터로 보낸다"는 Near-Data Processing의 극치를 LLM 추론에 적용한다.

## 읽기 전 질문
- KV Cache 전체를 GPU로 전송하는 것과 Attention 결과만 받는 것의 데이터량 차이는?
- CSD의 제한된 연산 자원(FPGA)으로 Attention을 실시간 처리할 수 있는가?
- Multi-GPU 시스템 대비 비용 효율성은 어떻게 되는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)

**LLM 추론의 KV Cache 병목:**
- 대형 모델(175B) + 긴 컨텍스트(16K~32K) → KV Cache가 GPU HBM 초과
- NVMe SSD로 오프로드 시: 매 decoding step마다 전체 KV Cache를 GPU로 전송 필요
- **>80%의 시간이 KV Cache I/O에 소비** (Figure 3b)
- RAID0 확장: 2~3개 SSD에서 포화 (공유 PCIe 병목, Figure 3c)

**데이터 전송량 분석:**
- 기존: 매 step마다 **s × 2 × b × h × d × 2 bytes** 전송 (s=시퀀스 길이, 전체 KV)
- 시퀀스 길이에 비례하여 전송량 선형 증가 → 32K 토큰이면 막대한 트래픽

### 2. Attention-Near-Storage (ANS) 설계

```
기존 오프로드:
  SSD ──[전체 KV Cache: s×d bytes]──→ GPU ──[Attention 연산]──→ 결과

INF² (ANS):
  GPU ──[Query: d bytes]──→ CSD(FPGA)
  SSD ──[내부 P2P: KV Cache]──→ FPGA ──[Attention: softmax(Q·K^T/√d)·V]
  CSD ──[결과: d bytes]──→ GPU

  시스템 인터커넥트 전송량: O(s×d) → O(d)
  시퀀스 길이에 무관! (s-배 절감)
```

**핵심 통찰:**
- Self-Attention에서 Q(Query)는 작음 (현재 토큰 1개), K/V(KV Cache)는 큼 (전체 컨텍스트)
- Q만 GPU→CSD로 전송하고, 거대한 K/V는 CSD 내부에서 접근
- 결과 벡터도 작음 (d bytes) → GPU←CSD 전송도 경미
- **전송 절감 비율: ~(s+1)/2 ≈ s배** (시퀀스 길이에 비례하여 효과 증가)

### 3. CSD 가속기 마이크로아키텍처

**하드웨어:** Samsung SmartSSD (Xilinx KU15P FPGA, 4GB DDR4, 4TB NVMe)

**가속기 구조:**
- **4개 병렬 GEMV 블록**: 각 32 MAC 유닛
  - Q·K^T 곱셈과 softmax·V 곱셈 수행
  - 32×32 블록 단위 처리
- **Transpose 모듈**: 32×32 BRAM 기반 K 행렬 전치
- **Softmax 모듈 (3단계 파이프라인)**:
  1. 마스킹 + 최대값 탐색 (32원소 그룹 계층적 리덕션)
  2. 지수 연산 + FP16 누적
  3. 나눗셈 정규화

**리소스 사용:**

| 리소스 | 사용률 |
|--------|:------:|
| LUT | 47.36% |
| FF | 32.72% |
| BRAM | 47.10% |
| URAM | 9.38% |
| DSP | 13.26% |
| **전력** | **14.21W/CSD** |

→ 16대 CSD: ~227W (GPU 1대 수백W와 비슷)

### 4. 데이터 흐름 상세

**Decoding 매 iteration:**
- **GPU→CSD**: Query/Key/Value 프로젝션 결과 (b·h·d·2 bytes, FP16) — 매우 작음
- **SSD→FPGA (내부 P2P)**: 기존 KV Cache 전체 — 대용량이지만 시스템 인터커넥트 미사용
- **CSD→GPU**: Attention 출력 (b·h·d·2 bytes) — 매우 작음

**KV Cache 분산:**
- 배치 차원 + 어텐션 헤드 차원으로 CSD에 분산
- CSD 간 통신 불필요 (각 CSD가 독립적으로 자신의 헤드 처리)

**지연 쓰기 (Delayed Writeback):**
- 새 KV 엔트리(256B)를 Host Memory에 임시 버퍼링
- c=2 iteration마다 SSD에 일괄 쓰기 (512B 최소 쓰기 단위 맞춤)
- FPGA에는 버퍼에서 직접 전달 (SSD 지연 회피)

**X-Cache 최적화:**
- KV Cache 대신 입력 활성화 X를 저장 (크기 50% 절감: X는 K+V의 절반)
- K=X·W_K, V=X·W_V를 GPU에서 매 step 재계산 (무시할 수준의 오버헤드)
- 시스템 메모리가 클수록 X-cache 효과 증가

### 5. 실험 환경

| 항목 | 사양 |
|------|------|
| **GPU** | A100 (40GB), H100 (80GB) |
| **CPU** | Xeon Gold 6342, 2×48코어 |
| **Host Memory** | 32×32GB DDR4-3200 (기본 512GB) |
| **CSD** | **16× Samsung SmartSSD** (4TB NVMe + KU15P FPGA) |
| **PCIe** | GPU: Gen4 ×16, CSD: Gen3 ×4/개 |
| **모델** | OPT-30B, 66B, 175B / LLaMA-2 7B |
| **기준선** | FlexGen (RAID0), DeepSpeed ZeRO-Inference, HF Accelerate |

### 6. 성능 결과 (상세)

#### Throughput 향상 (16 CSD, vs FlexGen)

| 모델 | 컨텍스트 | 배치 | 향상 |
|------|:-------:|:----:|:----:|
| OPT-30B | 16K | 32 | **2.67×** |
| OPT-66B | 16K | 32 | ~2.5× |
| OPT-175B | 16K | 32 | **2.3×** |
| OPT-30B | 32K | 32 | **3.46×** (최대) |

**핵심: 컨텍스트가 길수록, 출력이 길수록 효과 증가** (KV Cache 전송 절감량 비례)

#### 출력 길이별 가속 (16K 컨텍스트, H100)
- 64 토큰: ~2.5×
- 256 토큰: ~3.0×
- 512 토큰: **최대 3.46×**

#### 메모리 예산 민감도 (175B, 32K, H100)
- 256GB: 2.26× / 512GB: 2.45× / 1TB: **3.18×**
- 기준선: 0.032→0.034 tok/s (메모리 증가에 거의 무반응)

#### 비용 효율성 비교

| 구성 | 처리량 | 비용 | 처리량/$ |
|------|:------:|:----:|:-------:|
| 8×A100 (SmoothQuant) | 3.88 tok/s | $199,000 | 1.95e-5 |
| **H100 + 16 CSD** | 1.99 tok/s | $98,400 | **2.02e-5** |

→ **비용 효율성은 CSD 시스템이 더 높음** (절반 가격에 절반 처리량)

#### GPU 업그레이드 vs CSD 추가
- A100→H100 업그레이드: 8~19% 개선 (I/O 병목은 여전)
- 8 SSD→8 CSD 교체: 1.21~1.69× 개선 ($12,800)
- 16 SSD→16 CSD 교체: 2.15~2.19× 개선

### 7. Ablation Study (16 CSD, 배치 32, 16K)

| 구성 | 추가 향상 |
|------|:--------:|
| ANS only | 기준선 |
| + X-cache | 1.07~1.19× |
| + Delayed writeback | 1.10~1.12× |
| + 둘 다 | **1.1~1.57×** |

### 8. 한계점 및 향후 연구
- MHA에 최적화 → GQA/MQA 지원을 위한 가속기 확장 필요 (FPGA 리소스 여유 있음)
- 짧은 컨텍스트(1K)에서는 KV Cache가 메모리에 fit → 이점 미미
- CSD 간 KV Cache 파티셔닝이 불균형하면 성능 저하 가능

## 다른 논문과의 관계
- 같은 그룹 선행: [T3] Smart-Infinity (트레이닝의 Near-Storage)
- I/O 실측: [S3] NVMe KV 오프로드 (INF²가 해결하려는 I/O 병목의 실측)
- 비교: BaM [I1] — GPU→SSD I/O 경로 최적화 vs INF² — I/O 자체 제거
- 보완: [S1] Mooncake — KV Cache 캐싱으로 재연산 절약 vs INF² — KV Cache 전송 절약

## 발표 자료 활용 포인트
- "O(s×d) → O(d): 시퀀스 길이에 무관한 전송량" — 핵심 메시지
- BaM(GPU→SSD 직접 접근) vs INF²(SSD에서 직접 연산) 비교 프레임
- Smart-Infinity(트레이닝)+INF²(추론): 같은 하드웨어로 AI 라이프사이클 양면 커버
- 비용 효율성: multi-GPU($199K)와 비슷한 처리량/$를 $98K로 달성

## 메모
- Smart-Infinity와 INF²를 함께 보면 Samsung SmartSSD의 AI 활용 가능성이 선명
- "GPU 업그레이드보다 CSD 추가가 더 비용 효율적" — 스토리지 하드웨어 투자의 새로운 근거
- 14.21W/CSD × 16 = 227W ← GPU 1대(~300W)보다 낮은 전력으로 유의미한 가속
- Off-the-shelf 컴포넌트로 구현 → 시뮬레이션이 아닌 실제 시스템이라는 점이 강점
