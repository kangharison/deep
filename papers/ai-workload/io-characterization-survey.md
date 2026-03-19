# AI/ML/DL 워크로드 I/O 특성화 논문 종합 조사

웹 검색을 통해 수집한 AI/ML/DL 워크로드의 스토리지 I/O 특성을 분석한 논문 및 연구 자료의 종합 목록. 기존 `papers/ai-workload/` 컬렉션(16편)과 중복되지 않는 논문을 중심으로 정리하되, 중복되는 경우 기존 ID를 표시한다.

---

## 목차

1. [I/O 특성화 서베이 & 종합 분석](#1-io-특성화-서베이--종합-분석)
2. [GPU 데이터센터 워크로드 특성화](#2-gpu-데이터센터-워크로드-특성화)
3. [트레이닝 데이터 로딩 I/O 분석](#3-트레이닝-데이터-로딩-io-분석)
4. [체크포인트 I/O 특성화 & 최적화](#4-체크포인트-io-특성화--최적화)
5. [추천 모델(DLRM) I/O 특성화](#5-추천-모델dlrm-io-특성화)
6. [AI 스토리지 벤치마크](#6-ai-스토리지-벤치마크)
7. [스토리지 캐싱 & 데이터 파이프라인](#7-스토리지-캐싱--데이터-파이프라인)
8. [NVMe/SSD 오프로딩 I/O 분석](#8-nvmessd-오프로딩-io-분석)
9. [분산 파일시스템 & AI 전용 스토리지](#9-분산-파일시스템--ai-전용-스토리지)

---

## 1. I/O 특성화 서베이 & 종합 분석

### [S1] Parallel I/O Characterization and Optimization on Large-Scale HPC Systems: A 360-Degree Survey
- **저자**: (다수 저자)
- **연도/학회**: 2024, arXiv (서베이)
- **URL**: https://arxiv.org/abs/2501.00203
- **핵심 기여**: ACM DL, IEEE Xplore 등에서 131편의 논문을 분석하여 대규모 HPC 시스템의 병렬 I/O 특성화, 분석, 최적화에 대한 택소노미 제시. AI, 데이터 사이언스, 고해상도 시뮬레이션이 I/O를 복잡하게 만드는 현황 정리.
- **주요 발견**: 벤치마킹 도구, 프로파일링/모니터링 시스템, 최적화 전략을 체계적으로 분류
- **워크로드**: HPC + AI 혼합 워크로드
- **스토리지**: 병렬 파일시스템 (Lustre, GPFS 등)
- **비고**: 기존 W1과 상호 보완적 (W1은 ML 특화, 이 논문은 HPC 전반)

### [S2] Characterizing Machine Learning I/O Workloads on Leadership Scale HPC Systems
- **저자**: Arnab K. Paul, Ahmad Maroof Karimi, Feiyi Wang (ORNL)
- **연도/학회**: 2021, MASCOTS (29th International Symposium)
- **URL**: https://ieeexplore.ieee.org/document/9614303/
- **핵심 기여**: Summit 슈퍼컴퓨터에서 1년간 수집한 23,000+개 ML I/O 작업의 Darshan 로그를 체계적으로 분석
- **주요 발견**:
  - 과학 분야별로 I/O 행동이 현저히 다름
  - 워크로드 스케일에 따른 I/O 패턴 변화 분석
  - 병렬 파일시스템과 버스트 버퍼의 ML 워크로드별 사용 패턴 차이
- **워크로드**: 과학 ML 전반 (물리, 생물, 기후 등)
- **스토리지**: GPFS (Alpine), 버스트 버퍼 (Spectrum Scale)

### [S3] tf-Darshan: Understanding Fine-grained I/O Performance in Machine Learning Workloads
- **저자**: S.W. Chien, A. Podobas, I.B. Peng, S. Markidis
- **연도/학회**: 2020, IEEE CLUSTER
- **URL**: https://arxiv.org/abs/2008.04395
- **핵심 기여**: TensorFlow Profiler와 Darshan을 통합하여 ML 워크로드의 시스템 레벨 I/O를 프레임워크 레벨과 연결하는 도구 개발
- **주요 발견**:
  - tf-Darshan으로 최적화 가이드 시 POSIX I/O 대역폭 최대 19% 향상
  - ImageNet 분류와 Malware 분류의 I/O 패턴 차이 규명
- **워크로드**: ImageNet 이미지 분류, Malware 분류
- **스토리지**: POSIX, 계층형 스토리지

---

## 2. GPU 데이터센터 워크로드 특성화

### [S4] Characterization and Prediction of Deep Learning Workloads in Large-Scale GPU Datacenters
- **저자**: Qinghao Hu, Peng Sun, Shengen Yan, Yonggang Wen, Tianwei Zhang (NTU, SenseTime)
- **연도/학회**: 2021, SC '21 (Supercomputing)
- **URL**: https://arxiv.org/abs/2109.01313
- **핵심 기여**: SenseTime GPU 데이터센터의 실제 작업 트레이스를 대규모 분석. 클러스터/작업/사용자 관점에서 DL 워크로드 특성화 및 예측 프레임워크 제시.
- **주요 발견**:
  - Quasi-Shortest-Service-First 스케줄링으로 평균 작업 완료 시간 최대 6.5배 단축
  - 클러스터 에너지 절약 서비스로 전체 활용률 최대 13% 개선
- **워크로드**: DL 트레이닝 전반 (이미지, NLP, 추천 등)
- **스토리지**: 공유 분산 스토리지

### [S5] Analysis of Large-Scale Multi-Tenant GPU Clusters for DNN Training Workloads ★
- **저자**: Myeongjae Jeon, Shivaram Venkataraman, Amar Phanishayee, Junjie Qian, Wencong Xiao, Fan Yang (Microsoft)
- **연도/학회**: 2019, USENIX ATC '19
- **URL**: https://www.usenix.org/conference/atc19/presentation/jeon
- **핵심 기여**: Microsoft의 다중 테넌트 GPU 클러스터에서 2개월간의 DNN 트레이닝 워크로드 트레이스를 상세 분석. 갱 스케줄링, 지역성 제약, 장애의 영향 연구.
- **주요 발견**:
  - GPU는 모놀리식 자원으로 사용자 간 세밀한 공유 불가
  - 지역성 제약이 큐잉과 GPU 활용률에 큰 영향
  - 트레이닝 중 장애(failure) 패턴 분석
- **워크로드**: DNN 트레이닝 (Microsoft 프로덕션)
- **스토리지**: 공유 클러스터 스토리지

### [S6] Characterization of Large Language Model Development in the Datacenter ★
- **저자**: Qinghao Hu, Zhisheng Ye, Zerui Wang 외 (NTU, Peking Univ.)
- **연도/학회**: 2024, NSDI '24
- **URL**: https://www.usenix.org/conference/nsdi24/presentation/hu
- **핵심 기여**: GPU 데이터센터 "Acme"에서 6개월간 수집한 LLM 개발 워크로드 트레이스를 심층 분석. 빈번한 하드웨어 장애, 복잡한 병렬화 전략, 불균형한 자원 활용 문제 규명.
- **주요 발견**:
  - 체크포인트, 진단, 복구 모듈이 LLM 사전학습 프레임워크의 핵심 구성 요소
  - 장애 복구 효율이 낮고, 데이터 로딩 성능이 불충분
  - 장기간 체크포인트 중단이 트레이닝 효율 저하의 주요 원인
- **워크로드**: LLM 사전학습 (GPT 계열)
- **스토리지**: 분산 파일시스템
- **비고**: 기존 W2와 동일 논문

### [S7] Characterizing Deep Learning Training Workloads on Alibaba-PAI
- **저자**: (Alibaba 연구팀)
- **연도/학회**: 2019, IISWC '19
- **URL**: https://arxiv.org/pdf/1910.05930
- **핵심 기여**: Alibaba PAI 플랫폼의 DL 트레이닝 워크로드 특성화. 워크로드 유형별 하드웨어 병목 식별.
- **주요 발견**:
  - 1w1g(1 worker, 1 GPU) 워크로드: GPU 메모리 대역폭에 가장 민감
  - 1wng(1 worker, n GPU) 워크로드: PCIe 대역폭 변화에 가장 민감
  - PS/Worker 분산: 이더넷 대역폭에 가장 의존
- **워크로드**: 이미지 분류, NLP, 추천 모델 등
- **스토리지**: Alibaba 클라우드 스토리지

---

## 3. 트레이닝 데이터 로딩 I/O 분석

### [S8] Analyzing and Mitigating Data Stalls in DNN Training (CoorDL) ★
- **저자**: Jayashree Mohan, Amar Phanishayee, Ashish Raniwala, Vijay Chidambaram (Microsoft Research)
- **연도/학회**: 2021, VLDB (PVLDB Vol.14)
- **URL**: https://vldb.org/pvldb/vol14/p771-mohan.pdf
- **핵심 기여**: DNN 트레이닝에서 "데이터 스톨(data stall)"—스토리지에서 데이터를 가져오고 전처리하는 대기 시간—이 트레이닝 시간의 대부분을 차지함을 규명. CoorDL 라이브러리로 해결.
- **주요 발견**:
  - 많은 DNN 트레이닝에서 데이터 스톨이 총 시간의 대부분 차지
  - CoorDL의 3가지 기법: MinIO(DNN 특화 캐시), 파티션 캐싱(분산 학습), 코디네이트 전처리(하이퍼파라미터 탐색)
  - DALI 대비 DNN 트레이닝 시간 최대 5배 단축 (단일 서버)
  - 학습 정확도에 영향 없음
- **워크로드**: ImageNet, 다양한 CNN 모델
- **스토리지**: 로컬 SSD, NFS, 원격 스토리지

### [S9] Understanding Data Storage and Ingestion for Large-Scale Deep Recommendation Model Training ★
- **저자**: Mark Zhao, Niket Agarwal, Aarti Basant 외 (Meta/Stanford)
- **연도/학회**: 2022, ISCA '22 (49th)
- **URL**: https://arxiv.org/abs/2108.09373
- **핵심 기여**: Meta 프로덕션 환경에서 DLRM 트레이닝의 데이터 스토리지 및 인제스트(DSI) 파이프라인을 심층 특성화. 수천 대의 도메인 특화 가속기에 엑사바이트급 데이터를 초당 수십 테라바이트로 서빙하는 시스템 분석.
- **주요 발견**:
  - DSI 파이프라인 구성: 중앙 데이터 웨어하우스(분산 스토리지) + DPP(Data PreProcessing Service)
  - DSI 파이프라인이 전체 DNN 트레이닝 용량과 성능의 지배적 요인
  - DPP 스케일링으로 데이터 스톨 제거
- **워크로드**: DLRM (Deep Learning Recommendation Model)
- **스토리지**: 분산 데이터 웨어하우스, Tectonic FS

### [S10] Lotus: Characterization of Machine Learning Preprocessing Pipelines
- **저자**: Rajveer Bachkaniwala, Harshith Lanka, Kexin Rong, Ada Gavrilovska (Georgia Tech)
- **연도/학회**: 2024, IISWC '24
- **URL**: https://ieeexplore.ieee.org/document/10763539/
- **핵심 기여**: ML 전처리 파이프라인의 CPU 아키텍처 레벨 성능 특성화 도구(Lotus) 개발. PyTorch DataLoader 기반 전처리의 세밀한 프로파일링.
- **주요 발견**:
  - LotusTrace: PyTorch 라이브러리 계측으로 최소 오버헤드의 미세 시간 프로파일링
  - LotusMap: Python 함수와 하위 C++ 함수 간 매핑으로 하드웨어 카운터와 연결
  - 전처리 단계별 마이크로아키텍처 병목 식별
- **워크로드**: 이미지 분류, 다양한 ML 파이프라인
- **스토리지**: 로컬 SSD, 분산 FS

### [S11] cedar: Optimized and Unified Machine Learning Input Data Pipelines
- **저자**: Mark Zhao, Christos Kozyrakis 외 (Stanford)
- **연도/학회**: 2025, VLDB (PVLDB Vol.18)
- **URL**: https://arxiv.org/abs/2401.08895
- **핵심 기여**: ML 입력 데이터 파이프라인의 통합 프로그래밍 프레임워크. 오프로딩, 캐싱, 프리페칭, 퓨전, 리오더링 등 복합 최적화를 체계적으로 적용.
- **주요 발견**:
  - 8개 파이프라인에서 기존 최고 시스템 대비 1.87~10.65배 성능 향상
  - 데이터 볼륨과 트레이닝 처리량 요구 급증으로 입력 데이터 시스템이 점점 중요해짐
- **워크로드**: 다양한 ML 트레이닝 (이미지, 텍스트, 추천 등)
- **스토리지**: 로컬 SSD, 원격 스토리지, 클라우드 오브젝트 스토어

---

## 4. 체크포인트 I/O 특성화 & 최적화

### [S12] LLM Training in Practice: Insights from 85,000 Checkpoints
- **저자**: Glenn K. Lockwood (VAST Data)
- **연도/학회**: 2025, PDSW '25 (SC 워크숍)
- **URL**: https://pdsw.org/pdsw25/papers/wips/ws_pdswwip108s2-file1.pdf
- **핵심 기여**: 18개 AI 트레이닝 클러스터, 40개 프로덕션 LLM 트레이닝 작업에서 생성된 85,000+개 체크포인트의 I/O 실태 분석.
- **주요 발견**:
  - 트릴리언 파라미터 규모에서도 체크포인트에 필요한 대역폭은 수백 GB/s를 초과하지 않아 매우 modest
  - 3B 파라미터 모델 + 4 GPU: rank당 수십 GB/체크포인트
  - 기존 HPC 앱과는 현저히 다른 I/O 특성
- **워크로드**: LLM 사전학습 (다양한 규모)
- **스토리지**: 분산 파일시스템, NVMe SSD
- **비고**: 기존 W3와 동일 논문

### [S13] Understanding LLM Checkpoint/Restore I/O Strategies and Patterns
- **저자**: (다수 저자)
- **연도/학회**: 2024, arXiv
- **URL**: https://arxiv.org/abs/2512.24511
- **핵심 기여**: LLM 체크포인트/복원의 I/O 행동을 liburing(커널 지원 I/O 라이브러리) 중심으로 심층 조사.
- **주요 발견**:
  - 비합체(uncoalesced), 소규모 버퍼 연산이 합성 워크로드 대비 처리량을 절반으로 저하
  - 집합(aggregation)으로 대역폭 복원 가능
  - 파일시스템 인식 집합 및 I/O 합체 전략 채택이 핵심
- **워크로드**: LLM 사전학습 (GPT, LLaMA 계열)
- **스토리지**: NVMe SSD, 분산 FS

### [S14] FastPersist: Accelerating Model Checkpointing in Deep Learning
- **저자**: Microsoft DeepSpeed 팀
- **연도/학회**: 2024, arXiv (Microsoft Research)
- **URL**: https://arxiv.org/abs/2406.13768
- **핵심 기여**: NVMe 최적화 + 효율적 쓰기 병렬화 + 체크포인트와 학습 연산 오버래핑의 3가지 기법으로 체크포인트 생성 가속.
- **주요 발견**:
  - 기준 대비 체크포인트 생성 최대 116배 가속
  - 이터레이션별 체크포인트가 무시할 수 있는 오버헤드로 가능
  - 8x Gen5 NVMe에서 20배 이상 속도 향상
- **워크로드**: LLM 트레이닝 (GPT 계열, 대규모)
- **스토리지**: NVMe SSD (Gen4/Gen5)

### [S15] Optimizing Multi-Level Checkpointing for Distributed Deep Learning
- **저자**: Y. Cho 외
- **연도/학회**: 2024, IEEE ACCESS
- **URL**: https://discos.sogang.ac.kr/file/2024/intl_jour/IEEE_ACCESS_2024_Y_Cho.pdf
- **핵심 기여**: 분산 딥러닝을 위한 다단계 체크포인트 최적화. 로컬 SSD, 노드 간 복제, 원격 PFS의 다단계 체크포인트 전략.
- **워크로드**: 분산 DL 트레이닝
- **스토리지**: 로컬 NVMe SSD, 노드 간 RDMA, 병렬 FS

### [S16] Revisiting Reliability in Large-Scale Machine Learning Research Clusters
- **저자**: Kokolis 외
- **연도/학회**: 2025, HPCA '25
- **URL**: http://kokolis2.web.engr.illinois.edu/pubs/Reliability_HPCA2025.pdf
- **핵심 기여**: 대규모 ML 연구 클러스터의 신뢰성을 재검토. 장애 패턴과 체크포인트 복구 전략의 효과 분석.
- **워크로드**: 대규모 ML 연구 클러스터
- **스토리지**: 다단계 스토리지 계층

---

## 5. 추천 모델(DLRM) I/O 특성화

### [S17] EVStore: Storage and Caching Capabilities for Scaling Embedding Tables
- **저자**: (UChicago 연구팀)
- **연도/학회**: 2023, ASPLOS '23
- **URL**: https://ucare.cs.uchicago.edu/pdf/asplos23-EVStore.pdf
- **핵심 기여**: 추천 모델의 임베딩 테이블 스케일링을 위한 스토리지 및 캐싱 시스템. 임베딩 벡터(EV) 조회가 종단 추론 지연의 40%를 차지함을 규명.
- **주요 발견**:
  - EV 조회가 종단 추론 지연의 40% 차지
  - 임베딩 테이블 간 대역폭과 크기의 높은 편차
  - 일부 테이블은 쿼리당 수천 회 접근 (높은 메모리 대역폭 필요)
- **워크로드**: DLRM 추론
- **스토리지**: SSD, DRAM 캐시, 계층형 스토리지

### [S18] Machine Learning-Guided Memory Optimization for DLRM
- **저자**: (PASA Lab 연구팀)
- **연도/학회**: 2025, HPCA '25
- **URL**: https://www.pasalabs.org/papers/2025/HPCA25_DLRM.pdf
- **핵심 기여**: DLRM 임베딩 벡터 접근의 재사용 거리(reuse distance) 분석 및 ML 기반 메모리 최적화.
- **주요 발견**:
  - 임베딩 벡터 접근의 20%가 2^20 이상의 재사용 거리 → 대부분의 소프트웨어 관리 GPU 버퍼 크기 초과
  - 불규칙한 메모리 접근 패턴
- **워크로드**: DLRM 트레이닝/추론
- **스토리지**: GPU HBM, DRAM, SSD 계층

---

## 6. AI 스토리지 벤치마크

### [S19] MLPerf Storage Benchmark ★
- **저자**: Oana Balmau 외 (MLCommons)
- **연도/학회**: 2022 (ACM SIGMOD Record), 2023+ (벤치마크 릴리스)
- **URL**: https://dl.acm.org/doi/10.1145/3572751.3572765
- **핵심 기여**: ML 트레이닝 워크로드에서 스토리지 성능을 측정하는 최초의 오픈 표준 벤치마크. 가속기 "think time"을 시뮬레이션하여 실제 학습 없이 정확한 스토리지 패턴 생성.
- **주요 발견**:
  - 3가지 대표 모델: 3D-UNet, ResNet50, CosmoFlow
  - 샘플 크기: 수백 KB ~ 수백 MB의 다양한 범위
  - Think time: 수 밀리초 ~ 수백 밀리초
  - 동시 데이터 인제스트+트레이닝, 추론이 가장 스토리지 집약적
  - v2.0: 26개 조직에서 200+ 성능 결과 제출, 체크포인트 테스트 추가
- **워크로드**: 이미지 분할(3D-UNet), 이미지 분류(ResNet50), 우주론 시뮬레이션(CosmoFlow)
- **스토리지**: 벤더 중립, 다양한 스토리지 시스템

### [S20] DLIO: A Data-Centric Benchmark for Scientific Deep Learning Applications ★
- **저자**: Hariharan Devarajan, Huihuo Zheng, Anthony Kougkas, Xian-He Sun, Venkatram Vishwanath (Argonne/IIT)
- **연도/학회**: 2021, CCGrid '21 (Best Paper Award)
- **URL**: https://ieeexplore.ieee.org/document/9499416
- **핵심 기여**: 과학 DL 앱의 I/O 행동을 에뮬레이션하는 데이터 중심 벤치마크. ALCF Theta에서 8개 과학 DL 앱의 I/O 특성을 프로파일링하고, 이를 재현 가능한 벤치마크로 구현.
- **주요 발견**:
  - 고수준 + 저수준 I/O 프로파일링 도구로 데이터 접근의 전체적 관점 확보
  - I/O 최적화 가이드를 통해 트레이닝 시간 최대 6.7배 단축
  - 과학 DL 앱 간 I/O 패턴의 현저한 차이
- **워크로드**: 8개 과학 DL 앱 (ALCF Theta)
- **스토리지**: Lustre 병렬 FS, 버스트 버퍼

---

## 7. 스토리지 캐싱 & 데이터 파이프라인

### [S21] SHADE: Enable Fundamental Cacheability for Distributed Deep Learning Training ★
- **저자**: (다수 저자)
- **연도/학회**: 2023, FAST '23
- **URL**: https://www.usenix.org/conference/fast23/presentation/khan
- **핵심 기여**: 분산 DL 트레이닝에서 데이터 캐시 가능성(cacheability)의 근본적 분석. DLT의 고유한 I/O 워크로드가 스토리지 시스템 설계에 미치는 영향 규명.
- **주요 발견**:
  - I/O가 총 트레이닝 시간의 85~90%를 차지할 수 있음
  - 샘플의 26.5~26.6%가 에폭 간 2회 이상 접근, 9.6%는 3회 이상 접근
  - DLT 데이터의 캐시 가능성이 존재하며, 이를 활용한 근본적 최적화 가능
- **워크로드**: 다양한 DNN 트레이닝 (이미지, NLP 등)
- **스토리지**: 원격 분산 스토리지, 로컬 캐시

### [S22] Quiver: An Informed Storage Cache for Deep Learning
- **저자**: Abhishek Vijaya Kumar, Muthian Sivathanu (Microsoft Research India)
- **연도/학회**: 2020, FAST '20
- **URL**: https://www.usenix.org/conference/fast20/presentation/kumar
- **핵심 기여**: DL 도메인 특화 캐시 인텔리전스를 스토리지 캐싱 레이어에 적용. 보안 해시 기반 주소 지정으로 작업/사용자 간 캐시 데이터 투명 재사용.
- **주요 발견**:
  - 대체 가능한 캐시 히트(substitutable cache hits)로 캐시 스래싱 방지
  - 캐시 용량이 작업 세트보다 훨씬 작을 때도 효과적
  - 동적 캐시 할당으로 이득이 큰 작업에 우선 배정
- **워크로드**: PyTorch DL 트레이닝
- **스토리지**: SSD 캐시, 원격 스토리지

### [S23] DIESEL: A Dataset-Based Distributed Storage and Caching System for Large-Scale Deep Learning Training
- **저자**: Lipeng Wang, Songgao Ye 외 (SenseTime)
- **연도/학회**: 2020, ICPP '20
- **URL**: https://dl.acm.org/doi/10.1145/3404397.3404472
- **핵심 기여**: 대규모 DLT를 위한 데이터셋 기반 분산 스토리지+캐싱 코디자인. 소규모 파일 대량 접근, 노드 장애 복구, 셔플 읽기 비효율 문제 해결.
- **주요 발견**:
  - 메타데이터 처리와 저장의 분리, 데이터셋별 메타데이터 스냅샷
  - 청크 기반 셔플로 랜덤 접근 성능 향상 + 학습 정확도 유지
  - 실제 DLT 작업에서 데이터 접근 시간 절반 감소, 트레이닝 시간 수 시간 단축
- **워크로드**: 이미지 분류 등 대규모 DLT
- **스토리지**: 분산 스토리지, 분산 인메모리 캐시

### [S24] Tectonic-Shift: A Composite Storage Fabric for Large-Scale ML Training ★
- **저자**: Mark Zhao 외 (Stanford, Meta)
- **연도/학회**: 2023, USENIX ATC '23
- **URL**: https://www.usenix.org/conference/atc23/presentation/zhao
- **핵심 기여**: Meta 프로덕션 ML 트레이닝 인프라의 스토리지 패브릭. HDD 기반 Tectonic에 플래시 스토리지 계층(Shift)을 결합한 복합 아키텍처.
- **주요 발견**:
  - 심층 워크로드 특성화로 하드웨어/소프트웨어 설계 공간 탐색 가이드
  - 트레이닝 데이터셋 명세에서 미래 접근 패턴을 추론하는 앱 인식 캐시 정책
  - 전통 LRU 플래시 캐시 대비 1.51~3.28배 더 많은 I/O 흡수
  - 페타바이트급 프로덕션 클러스터에서 전력 수요 29% 절감
- **워크로드**: Meta ML 트레이닝 (DLRM 포함)
- **스토리지**: HDD(Tectonic) + Flash SSD(Shift) 복합

---

## 8. NVMe/SSD 오프로딩 I/O 분석

### [S25] An I/O Characterizing Study of Offloading LLM Models and KV Caches to NVMe SSD ★
- **저자**: (VU Amsterdam, AtLarge Research)
- **연도/학회**: 2025, CHEOPS '25 (EuroSys 워크숍)
- **URL**: https://atlarge-research.com/pdfs/2025-cheops-llm.pdf
- **핵심 기여**: DeepSpeed와 FlexGen의 모델/KV Cache NVMe 오프로딩의 블록 레이어 I/O 트레이스 수집 및 특성화.
- **주요 발견**:
  - 모델 오프로딩: 128 KiB 읽기가 지배적 (DeepSpeed, FlexGen 모두)
  - 모델 오프로딩은 NVMe SSD를 포화시키지 않음
  - KV Cache 오프로딩: 128 KiB 요청의 읽기/쓰기 혼합, 읽기 평균 대역폭(2.0 GiB/s)이 쓰기(11.0 MiB/s)보다 현저히 높음
- **워크로드**: LLM 추론 (DeepSpeed, FlexGen)
- **스토리지**: NVMe SSD
- **비고**: 기존 S3와 동일 논문

### [S26] FlashNeuron: SSD-Enabled Large-Batch Training of Very Deep Neural Networks
- **저자**: Jonghyun Bae, Jongsung Lee 외 (SNU)
- **연도/학회**: 2021, FAST '21
- **URL**: https://www.usenix.org/conference/fast21/presentation/bae
- **핵심 기여**: NVMe SSD를 백업 스토어로 사용하는 최초의 DNN 트레이닝 시스템. GPU-SSD 간 P2P 직접 통신으로 CPU 간섭 최소화.
- **주요 발견**:
  - 오프로딩 스케줄러: 텐서 크기, 전송 시간, forward/backward pass 런타임 고려하여 선택적 오프로드
  - 압축 포맷으로 오프로드하여 DNN 평가 시간 증가 없이 GPU 메모리 절약
  - GPU와 SSD가 직접 통신하므로 CPU 프로세스에 최소 간섭
- **워크로드**: 대규모 CNN/DNN 트레이닝 (VGG, ResNet 등)
- **스토리지**: NVMe SSD (P2P DMA)

### [S27] SSDTrain: An Activation Offloading Framework to SSDs for Faster Large Language Model Training
- **저자**: (Google Research 외)
- **연도/학회**: 2024, arXiv (IEEE 게재)
- **URL**: https://arxiv.org/abs/2408.10013
- **핵심 기여**: LLM 트레이닝에서 활성화(activation)를 고용량 NVMe SSD로 적응적 오프로딩. 데이터 전송을 계산과 완전히 오버랩.
- **주요 발견**:
  - 활성화 피크 메모리 사용량 47% 감소
  - I/O와 계산의 완벽한 오버랩으로 무시할 수 있는 오버헤드
  - PyTorch, Megatron, DeepSpeed와 호환
  - GPT, BERT, T5에서 검증
- **워크로드**: LLM 트레이닝 (GPT, BERT, T5)
- **스토리지**: NVMe SSD

---

## 9. 분산 파일시스템 & AI 전용 스토리지

### [S28] 3FS (Fire-Flyer File System) — DeepSeek
- **저자**: DeepSeek AI
- **연도/학회**: 2025, 오픈소스 릴리스 (관련 논문: SC '24)
- **URL**: https://github.com/deepseek-ai/3FS
- **핵심 기여**: AI 트레이닝/추론 워크로드를 위한 고성능 분산 파일시스템. 수천 개 SSD와 수백 개 스토리지 노드의 RDMA 네트워크를 결합하여 위치 무관(locality-oblivious) 접근.
- **주요 발견**:
  - 6.6 TB/s 읽기 처리량 (트레이닝 작업 병행 시 추가 1.4 TB/s)
  - 110.5 TiB 정렬: 30분 14초, 평균 3.66 TiB/min
  - 랜덤 읽기 속도를 최우선 최적화, 읽기 캐시를 거의 제거
  - CRAQ(Chain Replication with Apportioned Queries)로 강일관성
  - FoundationDB 기반 상태 비저장 메타데이터 서비스
- **워크로드**: AI 트레이닝 (데이터 로딩), AI 추론
- **스토리지**: NVMe SSD 클러스터, RDMA 네트워크

### [S29] HVAC: Removing I/O Bottleneck for Large-Scale Deep Learning Applications
- **저자**: Christopher Zimmer 외 (ORNL)
- **연도/학회**: 2022, IEEE CLUSTER '22
- **URL**: https://www.osti.gov/biblio/1902810
- **핵심 기여**: 대규모 DL 앱의 I/O 병목 해소를 위한 분산 읽기 캐시 계층(High-Velocity AI Cache). 노드 로컬 또는 근거리 스토리지 기술을 활용.
- **주요 발견**:
  - 대규모 DL 학습에서 병렬 스토리지 시스템 I/O에 상당 시간 소비
  - 기존 솔루션의 HPC 슈퍼컴퓨터 적용 한계: 극단적 스케일 비성능/장애, 이식성 부족, 복잡한 배포
  - 노드 로컬 스토리지 기반 분산 읽기 캐시로 해결
- **워크로드**: 대규모 과학 DL 트레이닝 (ORNL)
- **스토리지**: 병렬 FS(Lustre) + 노드 로컬 NVMe SSD 캐시

### [S30] EMLIO: Minimizing I/O Latency and Energy Consumption for Large-Scale AI Training
- **저자**: (다수 저자)
- **연도/학회**: 2025, SC '25 Workshops
- **URL**: https://dl.acm.org/doi/10.1145/3731599.3767566
- **핵심 기여**: 스토리지 노드에 경량 데이터 서빙 데몬을 배치하여 원본 샘플을 직렬화/배치 처리하는 효율적 ML I/O 서비스. 종단 데이터 로딩 지연과 I/O 에너지 소비를 동시 최소화.
- **워크로드**: 대규모 AI 트레이닝
- **스토리지**: 분산 스토리지 노드

---

## 종합 통계

| 항목 | 수치 |
|------|------|
| 총 수집 논문 수 | 30편 |
| 기존 컬렉션과 중복 | 4편 (W2=S6, W3=S12, S3=S25, 일부 O-offload) |
| 순수 신규 논문 | ~26편 |
| 연도 범위 | 2019~2025 |
| 주요 학회 | FAST, ATC, SC, ISCA, HPCA, NSDI, VLDB, ASPLOS, IISWC, CLUSTER, MASCOTS, CCGrid |

---

## I/O 특성 핵심 수치 요약

| 워크로드 | 읽기/쓰기 비율 | I/O 크기 | 대역폭 | 접근 패턴 |
|----------|---------------|----------|--------|-----------|
| DL 트레이닝 데이터 로딩 | 읽기 지배적 (>95%) | 수 KB ~ 수 MB (이미지), 수십 MB (3D) | 수 GB/s ~ 수십 TB/s (프로덕션) | 에폭별 셔플 랜덤 |
| LLM 체크포인트 | 쓰기 지배적 | 수백 MB ~ 수 TB/체크포인트 | 수십 ~ 수백 GB/s | 주기적 대량 순차 쓰기 |
| DLRM 임베딩 | 읽기 지배적 | 수십 ~ 수백 바이트/벡터 | 높은 IOPS 요구 | 불규칙 랜덤 |
| LLM 추론 모델 로딩 | 읽기 전용 | 128 KiB 지배적 | ~2 GiB/s | 순차 |
| LLM KV Cache 오프로드 | 읽기+쓰기 | 128 KiB 지배적 | 읽기 2.0 GiB/s, 쓰기 11 MiB/s | 동적 |
| GNN 그래프 탐색 | 읽기 지배적 | 극소 (~64B) | 낮은 대역폭, 높은 IOPS | 불규칙 랜덤 |
| DL 트레이닝 I/O 스톨 | — | — | — | 총 시간의 85~90% (캐시 없을 때) |

---

## 참고 자료 저장소

- [Storage-for-AI-Paper (GitHub)](https://github.com/hegongshan/Storage-for-AI-Paper) — AI 트레이닝/추론 가속을 스토리지 관점에서 다룬 필독 논문 큐레이션
- [ML Systems Papers (GitHub)](https://github.com/byungsoo-oh/ml-systems-papers) — ML 시스템 논문 큐레이션

---

## 수집 날짜
2026-03-19, 웹 검색 기반 종합 조사
