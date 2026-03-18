# [G1] BeaconGNN: Large-Scale GNN Acceleration with Out-of-Order Streaming In-Storage Computing ★필독

- **학회/연도:** HPCA 2024 (Best Paper Honorable Mention)
- **저자:** Yuyue Wang, Xiurui Pan, Yuda An, Jie Zhang, Glenn Reinman (UCLA)
- **분류:** GNN 스토리지 가속

## 핵심 요약 (1~2문장)
SSD 내부에서 GNN의 이웃 샘플링, 특성 테이블 조회, GNN 연산을 모두 수행하는 In-Storage Computing 시스템. 기존 In-Storage 솔루션의 한계(순차 샘플링, 작은 I/O, 펌웨어 병목)를 Out-of-Order 샘플링과 다계층 Near-Data Processing으로 해결한다.

## 읽기 전 질문
- GNN 워크로드의 I/O가 왜 기존 스토리지에서 비효율적인가?
- "Out-of-Order" 이웃 샘플링이 플래시 병렬성을 어떻게 활용하는가?
- SSD 내부의 다계층(컨트롤러, 채널, 다이) Near-Data Processing이란?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- 대규모 그래프(수십~수백 GB)는 GPU 메모리에 담기지 않음 → SSD 저장 필수
- GNN의 이웃 샘플링: 그래프 구조를 따라 이웃 노드를 탐색 → **극소 랜덤 읽기(~64B)**
- 이 I/O 크기는 NVMe의 최소 접근 단위(4KB 페이지)보다 훨씬 작음 → 심각한 읽기 증폭
- 호스트↔SSD 간 PCIe 전송이 병목 — 데이터의 대부분이 불필요한 패딩

**기존 In-Storage 접근의 한계:**
- **순차 샘플링**: GNN의 이웃 탐색 순서를 그대로 따르면 플래시 내부 병렬성 활용 불가
- **최소 접근 단위 불일치**: 64B 데이터를 위해 4KB 페이지 전체를 읽음
- **펌웨어 병목**: FTL(Flash Translation Layer) 처리가 I/O 처리량 제한

### 2. 제안 방법 (Approach)

```
┌─────────────────────────────────────────────────────────┐
│                    BeaconGNN SSD                         │
│                                                         │
│  ┌─────────────────────────────────────────────┐       │
│  │  SSD Controller 레벨                         │       │
│  │  - GNN Task 스케줄링                         │       │
│  │  - Out-of-Order 샘플링 엔진                  │       │
│  │  - 결과 집계 (Aggregation)                   │       │
│  └──────────┬──────────────────────────────────┘       │
│             │                                           │
│  ┌──────────▼──────────────────────────────────┐       │
│  │  Channel 레벨 (채널별 NDP 엔진)              │       │
│  │  - 특성 테이블 로컬 조회                      │       │
│  │  - 부분 GNN 연산                              │       │
│  │  Ch0 │ Ch1 │ Ch2 │ Ch3 │ ...                 │       │
│  └──────┼──────┼──────┼──────┼──────────────────┘       │
│         │      │      │      │                          │
│  ┌──────▼──────▼──────▼──────▼──────────────────┐       │
│  │  Die 레벨 (다이별 센싱 최적화)                │       │
│  │  - 플래시 페이지 내 부분 읽기                 │       │
│  │  - 불필요한 데이터 전송 차단                  │       │
│  └──────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────┘
```

**핵심 기법:**
- **Out-of-Order 샘플링**: 이웃 탐색 순서를 재배열하여 같은 플래시 페이지/채널에 있는 노드를 묶어 접근 → 플래시 내부 병렬성 극대화
- **새로운 그래프 포맷**: 그래프 구조를 플래시의 물리적 레이아웃에 맞게 재배치
- **다계층 NDP**: 컨트롤러, 채널, 다이 각 레벨에 연산 엔진 배치 → 데이터 이동 최소화

### 3. 실험 결과
- 기존 호스트 기반 GNN 대비 대폭적인 성능 향상
- 읽기 증폭 대폭 절감 (64B 유효 데이터 / 4KB 페이지 → 페이지 내 유효 데이터만 추출)
- **HPCA 2024 Best Paper Honorable Mention** 수상

### 4. 한계점 및 향후 연구
- SSD 펌웨어/하드웨어 수정 필요 — 상용 SSD에 바로 적용 불가
- 그래프 포맷 변환(전처리) 오버헤드
- GNN 모델 종류에 따른 범용성 검증 필요
- 동적 그래프(노드/엣지 추가/삭제)에 대한 대응 미비

## 다른 논문과의 관계
- 선행: BaM [I1], GIDS [I2] — GPU에서 그래프 데이터를 SSD에서 직접 접근
- 유사 방향: [G2] FlashGNN (같은 HPCA 2024, In-SSD GNN), [T3] Smart-Infinity (Near-Storage LLM)
- 비교: BaM은 "GPU→SSD 접근 경로 최적화", BeaconGNN은 "SSD 내부에서 연산" — 상호 보완적
- gpu-ssd 연결: GIDS [I2]가 BaM으로 GNN I/O를 최적화했다면, BeaconGNN은 I/O 자체를 SSD 내부에서 해결

## 발표 자료 활용 포인트
- HPCA Best Paper HM — 학술적 임팩트 높음
- 다계층 In-Storage 아키텍처 다이어그램이 매우 인상적
- GNN I/O의 특수성(극소 랜덤 읽기)을 잘 보여줌
- BaM(GPU→SSD) vs BeaconGNN(SSD 내부 연산) 비교 프레임

## 메모
- GNN의 극소 랜덤 I/O 문제는 BaM [I1]도 겪는 문제 — BaM은 GPU 측에서 최적화, BeaconGNN은 SSD 측에서 최적화
- 장기적으로 GPU-initiated I/O + In-Storage Computing의 하이브리드가 최적일 수 있음
- HPCA 2024에서 BeaconGNN과 FlashGNN이 동시 발표 → GNN 스토리지가 핫 토픽
