# [T3] Smart-Infinity: Fast Large Language Model Training using Near-Storage Processing on a Real System

- **학회/연도:** HPCA 2024
- **저자:** Hongsun Jang, Jaeyong Song, Jaewon Jung, Jaeyoung Park (Seoul National Univ.), Youngsok Kim (UT Austin → Yonsei Univ.), Jinho Lee (Seoul National Univ.)
- **분류:** 트레이닝 스토리지 최적화
- **소스코드:** https://github.com/AIS-SNU/smart-infinity

## 핵심 요약 (1~2문장)
Samsung SmartSSD(NVMe + Xilinx FPGA)를 활용한 Near-Storage Processing으로 LLM 트레이닝을 가속하는 실제 시스템. 옵티마이저 업데이트를 SSD 측 FPGA에서 수행하여 시스템 인터커넥트 트래픽을 75% 절감하고, 10대 SmartSSD에서 ZeRO-Infinity 대비 최대 **2.11× 트레이닝 속도 향상**을 달성한다.

## 읽기 전 질문
- Near-Storage Processing으로 LLM 트레이닝의 어떤 단계를 가속할 수 있는가?
- SmartSSD의 FPGA가 어떤 연산을 GPU 대신 수행하는가?
- 시스템 인터커넥트 병목이 SSD 추가로 해결되지 않는 이유는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)

**ZeRO-Infinity의 스토리지 병목:**
- LLM 파라미터가 GPU VRAM에 담기지 않아 SSD로 오프로드 (ZeRO-Infinity [P6])
- **88% 이상의 트레이닝 시간이 스토리지 데이터 전송에 소비** (Figure 3a)
- Update phase가 전체의 **>80%** 차지 (GPT-2 8.4B, 6 SSDs 기준)

**RAID0 확장의 한계:**
- SSD 4개 이후 **속도 향상 포화** (Figure 3b)
- 이유: 공유 시스템 인터커넥트(PCIe)가 새로운 병목 → SSD를 아무리 추가해도 개선 안 됨

### 2. Samsung SmartSSD 하드웨어 사양

| 컴포넌트 | 사양 |
|---------|------|
| **FPGA** | Xilinx Kintex UltraScale+ KU15P |
| **SSD** | 4TB NVMe |
| **LUT** | ~522K |
| **BRAM** | 984개 |
| **URAM** | 128개 |
| **DSP** | 1,968개 |
| **FPGA DRAM** | 4GB DDR4 |
| **내부 PCIe** | Gen3.0 x4 (SSD↔FPGA 전용 P2P 경로) |
| **가격** | ~$2,400/개 (일반 NVMe ~$400 대비 6배) |

### 3. 핵심 아키텍처: SmartUpdate + SmartComp

#### SmartUpdate: Near-Storage 옵티마이저 업데이트

```
기존 ZeRO-Infinity:
  SSD ──[6M: 옵티마이저 상태 읽기]──→ Host CPU ──[업데이트 연산]──→ SSD에 다시 쓰기
  SSD ──[2M: 그래디언트 읽기]────────→
  시스템 인터커넥트 트래픽: 읽기 8M + 쓰기 8M = 16M

SmartUpdate:
  SSD ──[내부 P2P]──→ FPGA ──[옵티마이저 업데이트 연산]──→ SSD에 내부 P2P 쓰기
                                     │
                                     └──→ 업데이트된 파라미터 2M만 GPU로 전송
  시스템 인터커넥트 트래픽: 읽기 2M + 쓰기 2M = 4M (75% 절감!)
```

**핵심 통찰:** 옵티마이저 상태(momentum, variance)는 update phase에서만 사용되지만 전체 스토리지 대역폭의 **75%를 소비**. 이를 CSD 내부에서 처리하면 시스템 인터커넥트 부담 제거.

#### SmartComp: GPU 그래디언트 압축 + CSD 복원

- GPU에서 Top-K 크기 기반 그래디언트 압축 (기본 c=2%)
- 압축된 그래디언트(index-value pairs)를 CSD로 전송
- FPGA에서 스파스 그래디언트를 full 벡터로 복원 후 업데이트 수행
- **시스템 인터커넥트 쓰기 트래픽: 2M → c%×2M** (추가 절감)

### 4. FPGA 가속기 설계 (Figure 7)

**Adam Updater:**
- 다수의 SIMD AXPBY 연산 유닛 (A×α + B×β)
- FP32 정밀도로 옵티마이저 상태 업데이트
- Throughput: **>7 GB/s** (NVMe 읽기/쓰기 대역폭 초과 → FPGA가 병목이 아님)
- 리소스 사용: LUT 33.66%, BRAM 27.13%, URAM 34.38%, DSP 11.03%

**Top-K Decompressor:**
- 추가 리소스 최소 (LUT 34.12%, BRAM 동일)
- Throughput: SSD 읽기 대역폭 약간 초과

**확장성:** SGD, AdaGrad 등 다른 옵티마이저도 계수 조정만으로 지원

### 5. 최적화: 버퍼 사전할당 + 스왑 중첩 (Figure 5)

```
Naive:  [할당→로드→업데이트→저장→해제] → [할당→로드→...] (순차)

최적화: Thread 0: [업데이트→파라미터 즉시 GPU 전송] → [다음 서브그룹 업데이트]
        Thread 1: [다음 서브그룹 로드]      → [momentum/variance 지연 쓰기]
        (GPU가 F/B를 더 빨리 시작 가능, 쓰기가 읽기와 중첩)
```

### 6. 실험 환경

| 항목 | 사양 |
|------|------|
| **CPU** | Xeon Gold 6342 (2×48코어) |
| **Host Memory** | 32×32GB DDR4-3200 |
| **GPU** | RTX A5000 (24GB) / Tesla A100 (40GB) / RTX A4000 (16GB) |
| **SmartSSD** | 최대 10대 (H3 Falcon 4109 PCIe 확장) |
| **모델** | GPT-2: 4.0B, 8.4B, 16.6B, 33.0B / BERT 345M / BLOOM |
| **기준선** | DeepSpeed ZeRO-Infinity + RAID0 |
| **소프트웨어** | Ubuntu 20.04, PyTorch 1.12.1, CUDA 11.6.2 |

### 7. 성능 결과 (상세)

#### 트레이닝 속도 향상 (vs ZeRO-Infinity RAID0 기준선)

| 모델 | 6 SSDs | 10 SSDs |
|------|:------:|:-------:|
| GPT-2 4.0B | 1.85~1.98× | **1.98~2.11×** |
| GPT-2 8.4B | 1.85~1.97× | 1.94~2.09× |
| GPT-2 16.6B | 1.82~1.95× | 1.92~2.08× |
| GPT-2 33.0B | 1.37~1.70× | 1.88~2.01× |

#### SSD 수에 따른 스케일링 (A100, GPT-2 4.0B)

| SSD 수 | 기준선 (RAID0) | SmartUpdate | SU+최적화 | SU+최적화+압축 |
|--------|:-------------:|:-----------:|:--------:|:------------:|
| 1 | 1.0× | -0.05× (약간 감소) | — | — |
| 4 | ~1.2× (포화) | 1.28× | — | — |
| 6 | ~1.2× | 1.54× | 1.60× | 1.85× |
| 10 | ~1.2× | 1.60× | 1.66× | **2.11×** |

**핵심:** 기준선은 4개 SSD에서 포화(PCIe 병목), SmartUpdate는 거의 선형 스케일링 (내부 P2P 대역폭이 독립적으로 확장)

#### Fine-tuning 정확도 (BERT-345M, GLUE 벤치마크)

| 구성 | 속도 향상 | MNLI m/mm | QQP | SST-2 | QNLI |
|------|:--------:|:---------:|:---:|:-----:|:----:|
| 기준선 | 1.0× | 89.60/89.46 | 91.95 | 93.98 | 94.23 |
| SU+O | 1.10× | 89.60/89.46 | 91.95 | 93.98 | 94.23 |
| SU+O+C(2%) | 1.38× | 89.50/89.39 | 91.80 | 95.41 | 94.30 |
| SU+O+C(1%) | 1.40× | 89.61/89.33 | 91.75 | 95.53 | 94.03 |

**SmartUpdate는 lossless (정확도 완전 동일), 압축 2%도 정확도 차이 무시 가능**

#### 비용 효율성 (GFLOPS/$)
- SmartSSD 1~3개: 프리미엄 미정당화 (기준선보다 비효율)
- SmartSSD 4개 이상: RAID0 기준선보다 **더 비용 효율적**
- 이유: 기준선은 SSD 추가해도 PCIe 포화로 성능 불변 → $/성능 악화

### 8. 한계점 및 향후 연구
- SmartSSD 전용 하드웨어 의존 → 범용 NVMe에 적용 불가
- FPGA 연산 능력 제한 → 복잡한 연산(attention 등)은 불가
- Gen3 x4 내부 대역폭 → Gen5로 업그레이드 시 추가 개선 기대
- PCIe 토폴로지 공유 시 성능 저하 (GPU+SSD가 같은 스위치 → 경합)

## 다른 논문과의 관계
- 선행: ZeRO-Infinity [P6] (오프로드 개념), SPDK [P7] (유저스페이스 NVMe)
- 같은 저자 후속: [S4] INF² (추론에서의 Near-Storage Processing)
- 비교: BaM [I1] — "GPU→SSD 직접 접근" vs Smart-Infinity "SSD에서 직접 연산"
- 유사: [G1] BeaconGNN, [G2] FlashGNN (In-Storage Computing for GNN)

## 발표 자료 활용 포인트
- "88% 시간이 데이터 전송, SSD 추가해도 4개에서 포화" → 문제 동기
- "옵티마이저 상태가 대역폭 75% 차지하는데 CSD 안에서 해결" → 핵심 아이디어
- RAID0 포화 vs SmartUpdate 선형 스케일링 그래프 (Figure 11)
- 비용 효율성 분석: 4개 이상에서 SmartSSD가 유리

## 메모
- Smart-Infinity(트레이닝)와 INF²(추론)는 같은 하드웨어(SmartSSD)로 AI 라이프사이클 양면을 커버
- "데이터를 이동시키지 말고, 연산을 데이터로 보내라" — Near-Data Processing의 핵심 철학
- Samsung SmartSSD가 상용 제품이라는 점이 학술적으로만 가능한 연구와의 차별점
- FPGA 리소스 ~66% 남음 → 추가 연산(gradient accumulation 등) 구현 여지
