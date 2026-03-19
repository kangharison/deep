# AI/ML 워크로드 I/O 특성 분석 논문 모음 (순수 Characterization)

이 디렉토리는 AI/ML/DL 워크로드의 **I/O 패턴을 측정/관찰/분석**하는 것이 주요 기여인 논문만 수집한다. 새로운 시스템, 프레임워크, 최적화 기법을 제안하는 논문은 제외한다.

---

## 논문 목록 (18편)

| ID | 연도 | 논문 | Venue | 핵심 기여 |
|----|------|------|-------|-----------|
| W1 | 2018 | Characterizing Deep-Learning I/O Workloads in TensorFlow | PDSW-DISCS | TF 데이터 파이프라인 I/O 성능 특성, 스레딩/프리페칭 효과 측정 |
| W2 | 2019 | Characterizing Deep Learning Training Workloads on Alibaba-PAI | IISWC | 수만 건 프로덕션 DL 작업의 Td/Tc/Tw 시간 분해 |
| W3 | 2019 | I/O Characterization and Performance Evaluation of BeeGFS for Deep Learning | ICPP | BeeGFS 위 DL 워크로드 I/O 및 메타데이터 특성 |
| W4 | 2019 | Scalable Deep Learning via I/O Analysis and Optimization | ACM TOPC | LMDB I/O 분석: I/O가 훈련 시간의 최대 90% 차지 |
| W5 | 2020 | tf-Darshan: Understanding Fine-grained I/O Performance in ML Workloads | IEEE CLUSTER | TF Profiler + Darshan 결합한 ML I/O 미세 분석 |
| W6 | 2021 | Characterizing Machine Learning I/O Workloads on Leadership Scale HPC Systems | MASCOTS | Summit 23,389건 ML I/O 작업 1년간 darshan 분석 |
| W7 | 2021 | DLIO: A Data-Centric Benchmark for Scientific Deep Learning Applications | IEEE CLUSTER | Theta 슈퍼컴퓨터 DL 워크로드 I/O 프로파일링 기반 벤치마크 |
| W8 | 2021 | Characterization and Prediction of Deep Learning Workloads in Large-Scale GPU Datacenters | SC21 | SenseTime GPU 클러스터 대규모 DL 작업 추적 분석 |
| W9 | 2021 | Analyzing the I/O Patterns of Deep Learning Applications | JCC-BD&ET | TF2/PyTorch + MNIST/CIFAR-10의 I/O 패턴 방법론 |
| W10 | 2022 | Understanding Data Storage and Ingestion for Large-Scale Deep Recommendation Model Training | ISCA | Meta DLRM 엑사바이트급 스토리지, TB/s 대역폭 수요 특성 |
| W11 | 2022 | I/O Performance Analysis of Machine Learning Workloads on Leadership Scale Supercomputer | Perf. Eval. | Summit ML 워크로드 I/O 성능의 체계적 특성 분석 (W6 확장) |
| W12 | 2022 | File Access Patterns of Distributed Deep Learning Applications | JCC-BD&ET | 분산 DL 훈련 단계의 파일 접근 패턴 분석 |
| W13 | 2023 | Analyzing I/O Performance of a Hierarchical HPC Storage System for Distributed DL | arXiv | 계층적 HPC 스토리지에서 DDNN 훈련의 darshan 기반 I/O 분석 |
| W14 | 2024 | Characterization of Large Language Model Development in the Datacenter | NSDI | 6개월간 LLM 개발 워크로드 추적: GPU 활용, 실패, 자원 패턴 |
| W15 | 2024 | Lotus: Characterization of ML Preprocessing Pipelines | IISWC | PyTorch DataLoader 전처리 파이프라인 프로파일링 |
| W16 | 2024 | I/O in Machine Learning Applications on HPC Systems: A 360-degree Survey | ACM Comput. Surv. | 6년간(2019-2024) ML I/O 패턴 종합 서베이 (39편 분석) |
| W17 | 2025 | An I/O Characterizing Study of Offloading LLM Models and KV Caches to NVMe SSD | CHEOPS | DeepSpeed/FlexGen의 NVMe 오프로딩 블록 계층 I/O 추적 분석 |
| W18 | 2025 | Parallel I/O Analysis in Distributed Deep Learning Applications on HPC | J. Supercomput. | DLIO + DeepGalaxy로 파일 포맷/접근 모드/Lustre 스트라이프 영향 정량 분석 |

---

## 핵심 정량적 발견 종합

### 1. Read/Write 비율

| 출처 | 발견 |
|------|------|
| W6 (MASCOTS 2021) | ML 워크로드는 대다수 소규모 read 집중적: 수많은 작은 파일 read가 지배적, 쓰기는 체크포인트에 집중 |
| W16 (Survey 2024) | "ML workloads perform small reads spread across a large number of random files" |
| W17 (CHEOPS 2025) | 모델 오프로딩: 128KiB read 지배적. KV 캐시 오프로딩: read 평균 2.0 GiB/s, write 평균 11.0 MiB/s (read >> write) |
| 업계 통합 관측 | AI DL I/O 패턴은 거의 100% read 워크로드, 랜덤 소형-중형 I/O 크기 지배적 (전통 HPC의 대형 순차 I/O와 대조적) |

### 2. I/O 요청 크기

| 출처 | 발견 |
|------|------|
| W16 (Survey: BERT) | 샘플당 2,500 bytes, TFRecord 파일 10개에 131,530 샘플 분산 |
| W16 (Survey: Unet3D) | 샘플당 ~146 MiB, NPZ 파일 168개 |
| W17 (CHEOPS 2025) | DeepSpeed/FlexGen 모두 블록 계층에서 128 KiB read/write가 지배적 |
| MLPerf Storage | ResNet50: ~100KB 샘플을 100MB+ 파일에서 동시 read. Unet3D: 대형 파일 순차 read. CosmoFlow: 수백KB~수백MB 범위 |
| W6 (MASCOTS 2021) | ML 작업은 소규모 read/write 접근 패턴이 일반적 |

### 3. 대역폭 / IOPS

| 출처 | 발견 |
|------|------|
| W10 (Meta ISCA 2022) | 엑사바이트급 훈련 데이터, 수십 TB/s 서빙 필요 |
| W17 (CHEOPS 2025) | KV 캐시 오프로딩 read: 2.0 GiB/s, write: 11.0 MiB/s. 모델 오프로딩은 NVMe SSD를 포화시키지 않음 |
| W18 (J. Supercomput. 2025) | Lustre 스트라이프 최적화 시 최대 18 GiB/s, IOPS 5배 향상 |
| W1 (PDSW-DISCS 2018) | 스레드 수 증가로 TF 대역폭 최대 2.3x~7.8x 향상. 버스트 버퍼로 체크포인트 2.6x 향상 |
| W16 (Survey: Perlmutter) | 집계 대역폭 5 TB/s 이상, 4M IOPS (4KiB 랜덤) |

### 4. 접근 패턴 (순차 vs 랜덤)

| 출처 | 발견 |
|------|------|
| 업계 통합 | DL 훈련은 랜덤 read 집중적 (전통 HPC의 순차 대형 write와 대조적) |
| W7 (DLIO) | 94% 상관도로 실제 DL 앱의 순차/연속 접근 비율 재현 |
| W16 (Survey: Unet3D) | 동일 rank가 매 epoch마다 전체 파일에 순차 접근 |
| W16 (Survey: BERT) | 배치 단위로 TFRecord 파일에서 샘플 read (배치 크기에 따라 read 패턴 변화) |
| W17 (CHEOPS 2025) | 모델 오프로딩은 128KiB 크기의 read 요청 패턴이 지배적 |

### 5. I/O가 전체 훈련 시간에서 차지하는 비중

| 출처 | 발견 |
|------|------|
| W4 (TOPC 2019) | LMDB 기반 DL에서 I/O가 총 훈련 시간의 **최대 90%** 차지 |
| W2 (Alibaba-PAI 2019) | 단일노드/단일GPU: 데이터 I/O ~10%. 분산 훈련: ~3%. 가중치/그래디언트 통신이 평균 62% 지배 |
| W15 (Lotus IISWC 2024) | 전처리가 epoch 시간의 **최대 65%** (이미지 분류, 객체 탐지, 오디오 분류) |
| W14 (NSDI 2024) | 평가 작업에서 모델 로딩 오버헤드가 지속시간의 29.5%, 메트릭 계산 유휴가 GPU 시간의 19.0% |
| 업계 관측 | 비효율적 파일 포맷 사용 시 I/O가 총 훈련 시간의 90% 이상 차지 가능 |

### 6. 스토리지 트래픽 구성

| 구성 요소 | 특성 |
|-----------|------|
| **데이터 로딩** | 훈련의 지배적 I/O: 수많은 소형 파일에서 반복적 read. epoch마다 전체 데이터셋 재접근 |
| **체크포인트** | 주기적 대형 write 버스트. 3B 모델/4GPU: 42GB, 132파일/체크포인트. 대형 LLM: rank당 수십GB. 체크포인트만의 훈련 시간 오버헤드: 1~3% |
| **셔플** | 부분 셔플링 시 워커당 데이터셋의 ~0.03% 로컬 저장 |
| **평가** | 전체 작업 수의 92.9%지만 GPU 자원의 0.8%만 사용 (LLM 개발) |

### 7. 훈련 단계별 I/O 변화 (W2, Alibaba-PAI)

```
단일노드-단일GPU (1w1g):
  Td(데이터 I/O) ≈ 10%  |  Tc(연산) ≈ 주요  |  Tw(통신) ≈ 미미

단일노드-멀티GPU (1wng):
  Td ≈ 3%  |  Tc ≈ 주요  |  Tw ≈ 증가 (NVLink/PCIe)

분산-PS/Worker:
  Td ≈ 3%  |  Tc ≈ 35%  |  Tw ≈ 62% (Ethernet 병목)
  >40% 작업이 총 시간의 >80%를 통신에 소비
```

### 8. LLM 체크포인트 I/O 상세 (W14 + 추가 연구)

```
3B 모델 (4x A100):   42 GB/체크포인트, 132 파일
7B 모델 (8 GPU):     비례 증가
BLOOM-176B (384 GPU): 3.5개월 훈련, ~1M GPU-hours

Polaris 테스트베드 (560 노드, 4xA100/노드):
  PFS 집계 대역폭: 650 GB/s
  Lustre: 40 MDS, 160 OST, 64MB 스트라이프

Write 처리량 비교:
  liburing 기반: DataStates-LLM 대비 3.9x, TorchSnapshot 대비 7.6x
  O_DIRECT write: 4.8x (liburing), 2.2x (POSIX) 향상
  File-per-tensor: 집계 대비 ~34% 처리량 저하

Read 처리량:
  Write 대비 ~2x 낮음
  버퍼 read (≤1GB): O_DIRECT 대비 2.3x 빠름
```

### 9. Meta DLRM 스토리지 특성 (W10)

```
데이터 규모: 엑사바이트급 훈련 데이터
서빙 대역폭: 수십 TB/s
실제 read 비율: 저장된 바이트의 20~37%만 read (ranking 모델)
Feature flattening: 2~2.3x 처리량 향상
Feature reordering: 전송량 45~55% 감소, 서비스 시간 30~70% 개선
Client-side rebatching: 20~40% rows/s 향상
FlatMaps: 9~17% 처리량 향상
전력 최적화: 35~45% 데이터 수집 전력 예산 개선
데이터셋 증가: 2년간 1.75~2x, 처리량 수요: 2년간 3~4x
```

### 10. LLM 개발 클러스터 특성 (W14, NSDI 2024)

```
클러스터 규모: 4,704 A100 GPU (Seren 2,288 + Kalos 2,416)
추적 기간: 6개월

작업 구성:
  사전훈련: 전체 작업 수의 0.9~3.2% → GPU 시간의 69.5~94.0% 소비
  평가: 전체 작업의 92.9% → GPU 자원의 0.8%만 사용
  작업 중위 지속시간: 2분 (이전 DL 추적 대비 1.7~7.2x 짧음)

GPU 활용:
  GPU 활용률 중위값: 97~99% (이전 데이터센터의 4~48% 대비)
  GPU 메모리: 50%의 GPU가 75% 이상(60GB+) 사용
  SM activity 중위값: ~40% (이전 연구의 20% 대비 2배)

네트워크/CPU:
  InfiniBand: 60% 시간 유휴, 활성 시 최대 대역폭의 25% 미만
  CPU 활용: 50% 미만으로 유지
  스토리지 NIC: 25Gb/s (병목 가능)

실패:
  ~60% 작업 실패, 완료된 작업은 GPU 자원의 20~30%만 소비
  NVLink 오류: 54건 (GPU 시간 30.25% 손실)
  평균 실패까지 시간: 868분(NVLink), 923분(CUDA)
  복구 시간: 평균 95.6분(NVLink), 78.3분(CUDA)

전력:
  유휴 GPU: ~60W, 피크: 22.1%가 400W TDP 초과
  GPU가 서버 전체 전력의 ~66.7%
```

---

## 추천 읽기 순서

### 1단계: 전체 맥락 파악
1. **W16** (360-degree Survey) -- ML I/O의 전체 그림, 39편 논문 종합
2. **W2** (Alibaba-PAI) -- Td/Tc/Tw 분해 프레임워크 이해

### 2단계: HPC 환경 대규모 측정
3. **W6** (MASCOTS: Summit 23K jobs) -- 리더십급 HPC에서 ML I/O 실태
4. **W11** (Performance Evaluation) -- W6 확장, 버스트 버퍼 분석 추가
5. **W7** (DLIO) -- I/O 프로파일링 기반 벤치마크 설계

### 3단계: 프로덕션 환경 특성
6. **W10** (Meta DLRM) -- 산업 스케일 데이터 수집 특성
7. **W14** (LLM Datacenter) -- LLM 개발의 실제 자원 사용 패턴
8. **W8** (SenseTime SC21) -- GPU 데이터센터 DL 작업 분석

### 4단계: 세부 I/O 분석
9. **W1** (TF I/O) -- TensorFlow 데이터 파이프라인 I/O 특성
10. **W5** (tf-Darshan) -- 미세 입도 I/O 프로파일링
11. **W4** (LMDB Analysis) -- I/O가 90% 차지하는 병목 분석
12. **W15** (Lotus) -- 전처리 파이프라인이 65% 차지

### 5단계: LLM/NVMe 특화
13. **W17** (CHEOPS: NVMe LLM) -- LLM 오프로딩의 NVMe I/O 추적
14. **W13** (Hierarchical Storage) -- 계층적 HPC 스토리지 DL I/O

### 6단계: 파일 포맷 / 접근 패턴
15. **W18** (J. Supercomput.) -- HDF5/NPZ/TFRecord 포맷별 I/O 영향
16. **W9/W12** (JCC-BD&ET) -- 분산 DL 파일 접근 패턴

---

## 분류별 논문 상세

### W1: Characterizing Deep-Learning I/O Workloads in TensorFlow (2018)
- **저자:** Chien, Peng, Markidis 등
- **Venue:** PDSW-DISCS 2018
- **arXiv:** [1810.03035](https://arxiv.org/abs/1810.03035)
- **연구 대상:** AlexNet mini-application, TensorFlow 데이터 파이프라인
- **주요 발견:**
  - 스레드 수 증가로 TF 대역폭 최대 2.3x~7.8x 향상
  - TF prefetcher가 가속기 연산과 CPU 입력 파이프라인을 완전히 오버랩하여 I/O 비용 제거
  - 버스트 버퍼 사용 시 체크포인트 성능 2.6x 향상

### W2: Characterizing Deep Learning Training Workloads on Alibaba-PAI (2019)
- **저자:** Wang, Meng, Wu 등 (HKU + Alibaba)
- **Venue:** IISWC 2019
- **arXiv:** [1910.05930](https://arxiv.org/abs/1910.05930)
- **연구 대상:** Alibaba PAI 플랫폼, 2018.12~2019.01 수만 건 프로덕션 작업
- **모델:** ResNet50, NMT, BERT, Speech, Multi-Interests, GCN
- **하드웨어:** 11 TFLOPS GPU, 25Gbps Ethernet, PCIe 10GB/s, NVLink 50GB/s
- **주요 발견:**
  - Ttotal = Td(데이터I/O) + Tc(연산) + Tw(통신)
  - 단일GPU: Td ~10%, 5% 작업이 Td에 50%+ 소비
  - 멀티GPU: Td ~3%, Tw가 지배적
  - PS/Worker: Tw가 평균 62%, >40% 작업이 >80% 시간을 통신에 소비
  - 메모리 바운드 연산이 컴퓨트 바운드를 모든 워크로드에서 초과
  - PS/Worker 작업: 전체의 29%지만 자원의 81% 소비

### W3: I/O Characterization and Performance Evaluation of BeeGFS for Deep Learning (2019)
- **저자:** Florida State Univ. + LLNL
- **Venue:** ICPP 2019
- **DOI:** 10.1145/3337821.3337902
- **연구 대상:** BeeGFS 병렬 파일시스템, AlexNet/ResNet-50, ImageNet, LBANN/TF+Horovod
- **주요 발견:**
  - 계층적 디렉토리 구조가 DL 워크로드의 메타데이터 관리에 유리
  - 평면 디렉토리 대비 계층적 배치가 모든 메타데이터 연산에서 성능 우위

### W4: Scalable Deep Learning via I/O Analysis and Optimization (2019)
- **저자:** Pumma, Si, Feng, Balaji (Argonne)
- **Venue:** ACM Transactions on Parallel Computing
- **DOI:** 10.1145/3331526
- **연구 대상:** LMDB (가장 널리 쓰이는 DL I/O 하위 시스템) 분석
- **스케일:** 9,216 코어 시스템
- **주요 발견:**
  - I/O가 총 훈련 시간의 **최대 90%** 차지
  - LMDB의 여러 단점 식별 (최적화 후 최대 65배 성능 향상)

### W5: tf-Darshan: Understanding Fine-grained I/O Performance in ML Workloads (2020)
- **저자:** Chien, Podobas, Peng, Markidis
- **Venue:** IEEE CLUSTER 2020
- **arXiv:** [2008.04395](https://arxiv.org/abs/2008.04395)
- **DOI:** 10.1109/CLUSTER49012.2020.00046
- **주요 발견:**
  - TF Profiler를 Darshan으로 확장하여 시스템 수준 I/O 정보 제공
  - POSIX I/O 대역폭 최대 19% 향상

### W6: Characterizing Machine Learning I/O Workloads on Leadership Scale HPC Systems (2021)
- **저자:** Paul 등 (ORNL)
- **Venue:** MASCOTS 2021
- **연구 대상:** Summit 슈퍼컴퓨터 (세계 2위), darshan 로그 분석
- **스케일:** 23,389건 ML I/O 작업, 1년간 추적
- **주요 발견:**
  - ML 워크로드는 소규모 read/write 접근 패턴이 일반적
  - 버스트 버퍼가 ML 워크로드에 더 적합 (GPFS 대비)
  - GPU 전용 작업이 CPU 전용보다 더 심한 I/O 병목 유발
  - ML 워크로드의 I/O 활동이 지수적 증가 추세

### W7: DLIO: A Data-Centric Benchmark for Scientific Deep Learning Applications (2021)
- **저자:** Devarajan, Zheng 등 (Argonne)
- **Venue:** IEEE CLUSTER 2021
- **연구 대상:** Theta 슈퍼컴퓨터 DL 워크로드 I/O 프로파일링
- **프로파일 대상:** CosmoFlow, BERT, UNet3D 등
- **지원 포맷:** TFRecord, HDF5, NPZ, CSV, JPG
- **주요 발견:**
  - 실제 DL 앱과 94% 상관도로 I/O 패턴 재현
  - I/O 병목 해소로 훈련 시간 최대 6.7x 단축 가능
  - 유사도 지표: 전체 I/O 시간, 데이터 read, 전송 크기 분포, 연산당 대역폭, 순차/연속 접근 비율, read 파일 수

### W8: Characterization and Prediction of Deep Learning Workloads in Large-Scale GPU Datacenters (2021)
- **저자:** Hu, Sun, Yan, Wen, Zhang (NTU + SenseTime)
- **Venue:** SC21
- **arXiv:** [2109.01313](https://arxiv.org/abs/2109.01313)
- **연구 대상:** SenseTime GPU 클러스터 실제 작업 추적
- **주요 발견:**
  - 클러스터, 작업, 사용자 관점에서의 DL 워크로드 분석
  - 자원 스케줄링 최적화를 위한 워크로드 예측 프레임워크

### W9: Analyzing the I/O Patterns of Deep Learning Applications (2021)
- **저자:** Parraga, Leon, Mendez, Rexachs, Luque (UAB)
- **Venue:** JCC-BD&ET 2021
- **연구 대상:** TensorFlow2, PyTorch + MNIST, CIFAR-10
- **주요 발견:**
  - 전통 HPC는 write 버스트 지배적이나 AI 워크로드는 read 지배적
  - DL I/O 패턴 분석 방법론 제시

### W10: Understanding Data Storage and Ingestion for Large-Scale Deep Recommendation Model Training (2022)
- **저자:** Zhao 등 (Meta/Facebook)
- **Venue:** ISCA 2022
- **arXiv:** [2108.09373](https://arxiv.org/abs/2108.09373)
- **연구 대상:** Meta 프로덕션 DLRM 훈련 인프라
- **주요 발견:**
  - 엑사바이트급 훈련 데이터, 수십 TB/s 서빙
  - 모델이 저장된 바이트의 20~37%만 read
  - 데이터셋 크기 2년간 1.75~2x 성장
  - 처리량 수요 2년간 3~4x 성장
  - DSI 파이프라인이 훈련 성능의 지배적 제약 요인

### W11: I/O Performance Analysis of ML Workloads on Leadership Scale Supercomputer (2022)
- **저자:** Paul 등 (ORNL)
- **Venue:** Performance Evaluation (Vols 157-158)
- **연구 대상:** W6 확장 -- Summit ML 워크로드 I/O
- **주요 발견:**
  - GPU 전용 작업이 CPU 전용보다 더 심한 I/O 병목
  - 버스트 버퍼 사용률이 낮으며 효율적 사용은 더 드묾
  - ML 워크로드의 I/O 활동이 지수적 증가 추세

### W12: File Access Patterns of Distributed Deep Learning Applications (2022)
- **저자:** Parraga, Leon, Mendez, Rexachs, Luque (UAB)
- **Venue:** JCC-BD&ET 2022
- **연구 대상:** 분산 DL 훈련 단계의 파일 접근 패턴
- **주요 발견:**
  - 분산 시스템에서 대량 데이터의 지속적 파일 접근이 공유 파일시스템을 압도 가능

### W13: Analyzing I/O Performance of a Hierarchical HPC Storage System for Distributed DL (2023)
- **저자:** (arXiv 2301.01494)
- **연구 대상:** 계층적 HPC 스토리지에서 DDNN 훈련
- **도구:** Darshan I/O 프로파일링
- **주요 발견:**
  - 미래 HPC 스토리지 설계를 위한 성능 향상 및 용량 요구 정량화

### W14: Characterization of Large Language Model Development in the Datacenter (2024)
- **저자:** Hu 등 (NTU)
- **Venue:** NSDI 2024
- **arXiv:** [2403.07648](https://arxiv.org/abs/2403.07648)
- **연구 대상:** "Acme" GPU 데이터센터 6개월 LLM 개발 추적
- **스케일:** 4,704 A100 GPU (2개 클러스터)
- **주요 발견:** (상세 수치는 위 "LLM 개발 클러스터 특성" 섹션 참조)

### W15: Lotus: Characterization of ML Preprocessing Pipelines (2024)
- **저자:** Bachkaniwala, Lanka, Rong, Gavrilovska (Georgia Tech)
- **Venue:** IISWC 2024
- **도구:** LotusTrace (PyTorch 계측) + LotusMap (Python-C++ 매핑)
- **연구 대상:** 이미지 분류, 객체 탐지, 오디오 분류 파이프라인
- **주요 발견:**
  - 전처리가 epoch 시간의 **최대 65%** 차지
  - 모든 파이프라인에 10ms 미만 (100us까지) 짧은 연산 존재
  - 샘플링 기반 프로파일러로는 포착 불가능한 미세 패턴 발견

### W16: I/O in Machine Learning Applications on HPC Systems: A 360-degree Survey (2024)
- **저자:** Lewis, Bez, Byna
- **Venue:** ACM Computing Surveys
- **arXiv:** [2404.10386](https://arxiv.org/abs/2404.10386)
- **범위:** 2019~2024 6년간 39편 논문 분석
- **핵심 요약:**
  - ML 워크로드: "small reads spread across a large number of random files"
  - Perlmutter: 5 TB/s 집계 대역폭, 4M IOPS
  - BERT 샘플: 2,500 bytes, Unet3D 샘플: 146 MiB
  - 데이터 준비, 훈련, 추론 단계별 I/O 패턴 분류
  - 오프라인 데이터 준비, 훈련, 추론의 I/O 최적화 기법 정리

### W17: An I/O Characterizing Study of Offloading LLM Models and KV Caches to NVMe SSD (2025)
- **저자:** Ren, Doekemeijer 등 (VU Amsterdam)
- **Venue:** CHEOPS 2025 (EuroSys Workshop)
- **DOI:** 10.1145/3719330.3721230
- **연구 대상:** DeepSpeed/FlexGen의 NVMe 오프로딩 블록 계층 I/O 추적
- **모델:** OPT-13B (DeepSpeed), OPT-30B (FlexGen)
- **주요 발견:**
  - libaio 기반 텐서 오프로딩이 POSIX보다 높은 I/O 대역폭
  - 모델 오프로딩: 블록 계층에서 128 KiB read 지배적
  - 모델 오프로딩은 NVMe SSD를 포화시키지 않음
  - KV 캐시 오프로딩: read + write 혼합, 128 KiB 지배적
  - KV 캐시 read 평균 2.0 GiB/s, write 평균 11.0 MiB/s

### W18: Parallel I/O Analysis in Distributed Deep Learning Applications on HPC (2025)
- **저자:** Parraga, Leon, Mendez, Rexachs, Suppi, Luque (UAB)
- **Venue:** The Journal of Supercomputing, Vol. 81, No. 16
- **연구 대상:** DLIO 벤치마크 + DeepGalaxy 애플리케이션
- **파일시스템:** Lustre
- **주요 발견:**
  - 스트라이프 카운트 최적화로 I/O 및 실행 시간 감소
  - 최대 18 GiB/s 대역폭, IOPS 5x 향상
  - TFRecord: 대형 전송 시 최고 대역폭
  - HDF5: 확장성과 메모리 효율의 균형
  - 공유 접근 모드가 독립 접근 대비 일관되게 오버헤드 감소

---

## 핵심 메시지 요약

1. **AI 훈련 I/O는 read-dominant**: 전통 HPC의 write-dominant 패턴과 근본적으로 다르다.
2. **소형 랜덤 read가 지배적**: 수천~수백만 개의 작은 파일/샘플에서 반복적으로 read한다.
3. **I/O 병목은 실재한다**: 비효율적 구성 시 I/O가 훈련 시간의 90%까지 차지할 수 있다.
4. **체크포인트는 주기적 write 버스트**: 모델 크기에 비례하며, 대형 LLM은 rank당 수십 GB를 생성한다.
5. **스케일에 따라 병목이 이동**: 단일 GPU에서는 데이터 I/O(~10%)가 의미 있으나, 분산 훈련에서는 통신(~62%)이 지배한다.
6. **메타데이터 병목**: 수많은 소형 파일의 메타데이터 요청이 병렬 파일시스템 성능을 제한한다.
7. **전처리 비용 과소평가**: 데이터 전처리가 epoch의 65%까지 차지할 수 있으며, 기존 프로파일러로는 포착이 어렵다.
8. **LLM 시대의 새로운 I/O**: 엑사바이트급 데이터, TB/s 대역폭, 수백~수천 파일/체크포인트의 새로운 스케일.
