# [G2] FlashGNN: An In-SSD Accelerator for GNN Training

- **학회/연도:** HPCA 2024
- **저자:** Jie Zhang et al. (Peking University)
- **분류:** GNN 스토리지 가속

## 핵심 요약 (1~2문장)
SSD 내부에 GNN 트레이닝 전용 가속기를 탑재하여 PCIe 병목을 우회하는 In-SSD 아키텍처. SSD에서 읽은 플래시 데이터를 외부로 전송하지 않고 SSD 내부에서 GNN 연산을 수행하여, PCIe 대역폭 한계를 극복하고 데이터 재사용을 극대화한다.

## 읽기 전 질문
- In-SSD 가속기가 GNN 트레이닝의 어떤 연산을 처리하는가?
- PCIe 병목이 GNN 성능에 미치는 영향은 어느 정도인가?
- SSD 내부의 제한된 연산 자원으로 트레이닝이 가능한가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- SSD 기반 GNN 트레이닝: 그래프를 SSD에 저장하고 GPU/CPU에서 연산
- **핵심 병목: PCIe 버스**
  - SSD 내부 NAND 대역폭은 수십 GB/s이지만, PCIe 외부 인터페이스는 수 GB/s
  - GNN의 랜덤 접근 패턴이 PCIe 효율을 더 낮춤
  - 결국 SSD의 내부 대역폭을 외부에서 활용할 수 없음

### 2. 제안 방법 (Approach)

```
기존:
  NAND Flash ──[내부 수십GB/s]──→ SSD 컨트롤러 ──[PCIe 수GB/s]──→ Host ──→ GPU
                                                    ↑ 병목

FlashGNN:
  NAND Flash ──[내부 수십GB/s]──→ In-SSD 가속기 ──[연산 결과만]──→ Host ──→ GPU
                                  (GNN 연산 수행)
                                  PCIe 전송량 대폭 감소
```

**In-SSD 가속기 설계:**
- SSD 컨트롤러 옆에 GNN 전용 가속기 배치
- 플래시에서 읽은 데이터를 SSD 내부 DRAM에 캐싱
- 이웃 집계(aggregation) 연산을 SSD 내부에서 수행
- 결과(집계된 특성 벡터)만 호스트로 전송

### 3. 실험 결과
- PCIe 병목 해소로 GNN 트레이닝 속도 대폭 향상
- SSD 내부 대역폭을 효과적으로 활용
- HPCA 2024에서 BeaconGNN과 동시 발표

### 4. 한계점 및 향후 연구
- SSD 하드웨어 수정 필요 (FPGA/ASIC 추가)
- SSD 내부 DRAM 용량에 의한 캐시 크기 제한
- 트레이닝의 backward pass(기울기 계산)까지 SSD 내부에서 수행하기 어려움
- GNN 모델별 최적화 필요

## 다른 논문과의 관계
- 유사: [G1] BeaconGNN (같은 학회, 유사 접근, 다른 설계 포인트)
  - BeaconGNN: Out-of-Order 샘플링 + 다계층 NDP
  - FlashGNN: PCIe 병목 해소 + 데이터 재사용
- gpu-ssd 연결: GIDS [I2]가 GPU→SSD 직접 접근으로 GNN 가속, FlashGNN은 SSD 내부에서 연산
- 선행: BaM [I1], EMOGI [P4]

## 발표 자료 활용 포인트
- "SSD 내부 대역폭 vs PCIe 대역폭" 격차를 보여주는 다이어그램
- BeaconGNN과의 비교 — 같은 문제에 대한 다른 설계 선택

## 메모
- HPCA 2024에서 GNN+SSD 논문이 2편(BeaconGNN, FlashGNN) 동시 발표 → In-Storage GNN이 학술 트렌드로 자리잡음
- PCIe 내부/외부 대역폭 격차는 NVMe 연구의 근본적 문제 중 하나 — CXL이 이를 완화할 수 있을지 주목
