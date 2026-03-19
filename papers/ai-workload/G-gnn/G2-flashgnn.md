# [G2] FlashGNN: An In-SSD Accelerator for GNN Training

- **학회/연도:** HPCA 2024, pp. 361-378
- **저자:** Fuping Niu, Jianhui Yue, Jiangqiu Shen, Xiaofei Liao, Hai Jin (Huazhong Univ. of Science and Technology)
- **분류:** GNN 스토리지 가속

## 핵심 요약 (1~2문장)
SSD 내부에 GNN 트레이닝 전용 가속기를 탑재하여 PCIe 병목을 근본적으로 우회하는 In-SSD 아키텍처. **Node-wise GNN 트레이닝**, 플래시 요청 스케줄링 알고리즘, 고성능 서브그래프 생성 기법의 3가지 혁신으로 SSD 내부 대역폭을 최대한 활용한다.

## 읽기 전 질문
- SSD 내부 NAND 대역폭과 외부 PCIe 대역폭의 격차는 어느 정도인가?
- In-SSD 가속기가 GNN 트레이닝의 어떤 연산을 처리하는가?
- SSD 내부의 제한된 DRAM으로 효율적인 데이터 재사용이 가능한가?

## 주요 내용 정리

### 1. 문제 정의: PCIe 병목

```
SSD 내부:
  NAND Flash ──[내부 채널: 수십 GB/s 집계]──→ SSD 컨트롤러

SSD 외부:
  SSD 컨트롤러 ──[PCIe Gen4 x4: ~7 GB/s]──→ Host

격차: 내부 수십 GB/s vs 외부 ~7 GB/s = 수 배 차이
GNN의 랜덤 접근 패턴이 PCIe 효율을 더 악화시킴
→ SSD 내부 대역폭의 대부분이 낭비됨
```

- SSD 기반 GNN 트레이닝: 그래프를 SSD에 저장, CPU/GPU에서 연산
- PCIe 버스가 **단일 병목점** → SSD를 더 빠르게 만들어도 개선 불가
- CPU/GPU 활용률 저하: PCIe 대기 시간이 연산 시간보다 김

### 2. 3가지 핵심 기법

#### 기법 1: Node-wise GNN Training
- 기존 mini-batch 트레이닝: 다수 노드를 배치로 묶어 처리 → 큰 서브그래프 필요 → 대량 I/O
- **Node-wise**: 노드 하나씩 처리하여 필요한 데이터 최소화
- SSD 내부 제한된 DRAM(수 GB)에 맞는 워킹셋 크기로 조절

#### 기법 2: 플래시 요청 스케줄링 알고리즘
- 여러 GNN 요청을 수집하여 플래시 채널/다이별로 재정렬
- 플래시 내부 병렬성 극대화 (BeaconGNN의 OoO와 유사한 철학)
- 같은 페이지에 속하는 데이터 요청을 결합 → 읽기 증폭 감소

#### 기법 3: 고성능 서브그래프 생성
- 이웃 샘플링 결과로 서브그래프 구성을 SSD 내부에서 수행
- 서브그래프 데이터가 호스트로 전송되지 않음 → PCIe 부담 제거
- SSD 내부 DRAM에서 효율적인 데이터 재사용 (같은 노드가 여러 서브그래프에 등장)

### 3. In-SSD 가속기 구조

```
┌─────────────────────────────────────────────────┐
│                   FlashGNN SSD                   │
│                                                  │
│  ┌────────────────────────────┐                 │
│  │ SSD 컨트롤러 + GNN 가속기  │                 │
│  │                            │                 │
│  │ [요청 스케줄러]  [집계 엔진] │                │
│  │ [서브그래프 생성] [DRAM 캐시]│                │
│  └─────────┬──────────────────┘                 │
│            │                                     │
│  ┌─────────▼──────────────────┐                 │
│  │ NAND Flash 어레이           │                │
│  │ Ch0 │ Ch1 │ Ch2 │ Ch3 ... │                 │
│  │ (내부 수십 GB/s 집계)       │                │
│  └────────────────────────────┘                 │
│                                                  │
│  결과만 PCIe로 호스트 전송 (집계된 특성 벡터)    │
└─────────────────────────────────────────────────┘
```

### 4. BeaconGNN [G1]과의 비교

| 특성 | BeaconGNN [G1] | FlashGNN [G2] |
|------|:-------------:|:------------:|
| **학회** | HPCA 2024 | HPCA 2024 |
| **수상** | Best Paper HM | — |
| **핵심 혁신** | OoO 샘플링 + 3계층 NDP | Node-wise 트레이닝 + 스케줄링 |
| **NDP 계층** | 컨트롤러/채널/다이 3단계 | 컨트롤러 레벨 |
| **다이 최적화** | 부분 읽기 (바이트 추출) | 미적용 |
| **그래프 포맷** | 물리적 레이아웃 맞춤 재배치 | 기존 포맷 유지 |
| **주요 최적화** | 플래시 병렬성 활용 | 데이터 재사용 극대화 |
| **트레이닝 방식** | 기존 mini-batch | Node-wise (신규) |

→ 두 논문은 같은 문제(In-Storage GNN)를 다른 각도에서 공격: BeaconGNN은 **하드웨어 병렬성**, FlashGNN은 **소프트웨어 효율성**

### 5. 한계점 및 향후 연구
- SSD 하드웨어 수정 필요 (FPGA/ASIC 추가)
- SSD 내부 DRAM 용량 제한 → 매우 큰 그래프의 워킹셋에 한계
- 트레이닝의 backward pass까지 SSD 내부에서 수행하기 어려움
- GNN 모델별(GCN, GraphSAGE, GAT) 최적화 필요

## 다른 논문과의 관계
- 유사: [G1] BeaconGNN (같은 학회, 같은 방향, 다른 설계)
- gpu-ssd 연결: GIDS [I2]가 GPU→SSD 직접 접근으로 GNN 가속, FlashGNN은 SSD 내부 연산
- 선행: BaM [I1], EMOGI [P4]
- 유사 방향: [T3] Smart-Infinity, [S4] INF² (In/Near-Storage AI 연산)

## 발표 자료 활용 포인트
- "SSD 내부 대역폭 수십GB/s vs PCIe ~7GB/s" 격차 다이어그램
- BeaconGNN vs FlashGNN 비교표: 같은 문제, 다른 접근
- Node-wise 트레이닝: mini-batch vs node-wise의 I/O 비교

## 메모
- HPCA 2024에서 GNN+SSD 논문 2편 동시 발표 → In-Storage GNN이 학술 트렌드 확정
- PCIe 내부/외부 대역폭 격차는 NVMe 연구의 근본적 문제 → CXL이 이를 완화할 수 있을지 주목
- FlashGNN의 저자(HUST)는 중국 스토리지 시스템 연구의 주요 그룹
