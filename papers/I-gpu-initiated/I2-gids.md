# [I2] GIDS: Accelerating Sampling and Aggregation Operations in GNN Frameworks with GPU Initiated Direct Storage Accesses ★필독

- **학회/연도:** VLDB 2024
- **저자:** Jeongmin Brian Park, Vikram Sharma Mailthody, Zaid Qureshi, Wen-mei Hwu
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
BaM의 GPU-initiated I/O를 GNN(Graph Neural Network) 학습 파이프라인에 적용한 논문. GPU 스레드가 SSD에서 직접 노드 feature를 fetch하여 DGL 프레임워크의 sampling + feature gathering 병목을 해소하고, 기존 CPU-mediated 방식 대비 최대 582배 가속 달성.

## 읽기 전 질문
- GNN 학습에서 feature gathering이 왜 병목인가? CPU-GPU 간 데이터 이동 패턴은 어떤가?
- BaM의 random access 능력이 GNN의 불규칙 접근 패턴과 어떻게 맞아떨어지는가?
- GPU-initiated I/O를 GNN 프레임워크(DGL/PyG)에 통합할 때 API 수준의 변경은 얼마나 필요한가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GNN 학습의 데이터 병목**: GNN은 미니배치 학습 시 (1) 이웃 노드 sampling → (2) 샘플된 노드의 feature gathering → (3) GNN 연산 순서로 진행
- **Feature 데이터 크기 문제**: 대규모 그래프(ogbn-papers100M 등)의 노드 feature 테이블이 수백 GB에 달해 GPU VRAM에 적재 불가
- **기존 DGL 파이프라인의 한계**: CPU에서 sampling + feature gathering 수행 후 GPU로 전송. CPU의 random access 처리 능력이 GPU 연산 속도를 따라가지 못함 → GPU idle 시간 발생
- **UVA(Unified Virtual Addressing) 방식의 한계**: 호스트 메모리에 feature를 두고 GPU가 PCIe를 통해 접근하면, DRAM 용량도 초과하는 데이터셋에서는 사용 불가

### 2. 제안 방법 (Approach)
- **BaM 기반 스토리지 직접 접근**: GPU 스레드가 SSD에 저장된 feature 테이블을 직접 읽어옴
- **3단계 파이프라인 통합**:
  1. Sampling: GPU에서 수행 (기존 DGL의 GPU sampling과 동일)
  2. Feature Gathering: GPU 스레드가 BaM을 통해 SSD에서 직접 feature fetch (핵심 기여)
  3. GNN 연산: 기존과 동일하게 GPU에서 수행
- **캐시 최적화**: GPU VRAM의 일부를 feature 캐시로 사용. 인기 노드(hub node)의 feature는 캐시에 유지
- **배치 I/O 최적화**: 같은 페이지에 속하는 feature 요청을 합쳐(coalescing) SSD I/O 횟수 절감

### 3. 핵심 아키텍처/설계

```
┌─────────────────────────────────────────────┐
│              DGL + GIDS Pipeline            │
│                                             │
│  ┌──────────┐  ┌──────────────┐  ┌───────┐ │
│  │ GPU      │  │ GPU Feature  │  │ GNN   │ │
│  │ Sampling │→│ Gathering    │→│ Train  │ │
│  │ (기존)   │  │ (GIDS 핵심)  │  │ (기존) │ │
│  └──────────┘  └──────┬───────┘  └───────┘ │
│                       │                     │
│              ┌────────▼────────┐            │
│              │ Feature Cache   │            │
│              │ (GPU VRAM)      │            │
│              │ Hit → 즉시반환  │            │
│              │ Miss ↓          │            │
│              └────────┬────────┘            │
│                       │                     │
│              ┌────────▼────────┐            │
│              │ BaM I/O Layer   │            │
│              │ NVMe SQ/CQ      │            │
│              │ (GPU VRAM)      │            │
│              └────────┬────────┘            │
└───────────────────────┼─────────────────────┘
                        │ PCIe P2P
                   ┌────▼─────┐
                   │ NVMe SSD │
                   │ (Feature │
                   │  Table)  │
                   └──────────┘
```

- **Feature Table 레이아웃**: 노드 ID 순서로 feature 벡터를 SSD에 연속 배치. 노드 ID로 바로 오프셋 계산 가능
- **I/O Coalescing**: 같은 4KB 페이지에 속하는 복수 노드 feature 요청을 하나의 NVMe read로 합침
- **Degree-aware 캐시**: 높은 degree를 가진 허브 노드는 자주 샘플링되므로 캐시 우선순위 부여

### 4. 실험 결과 (Key Results)
- **데이터셋**: ogbn-papers100M (feature 크기 ~60GB), MAG240M (feature 크기 ~380GB)
- **기존 DGL 대비 성능**:
  - CPU feature gathering 방식 대비 **최대 582배 가속** (end-to-end 학습 시간)
  - 이 수치는 CPU의 느린 random access + PCIe 전송 병목 제거에 기인
- **UVA 대비**: 호스트 메모리 기반 UVA 방식 대비 **2~8배** 빠름 (데이터가 DRAM에 들어가는 경우에도)
- **캐시 효과**: GPU VRAM 캐시 크기가 전체 feature의 10~20%만 되어도 상당한 hit rate 달성
- **학습 정확도**: 기존 방식과 동일한 학습 정확도 유지 (데이터 접근 방식만 변경, 알고리즘 변경 없음)
- **SSD 대역폭 활용**: 4개 SSD 구성에서 ~20 GB/s 이상의 feature read 처리량

### 5. 한계점 및 향후 연구
- **BaM 의존성**: BaM의 한계(동기식 I/O, 파일시스템 미지원 등)를 그대로 상속
- **Feature 크기 고정 가정**: 모든 노드의 feature 벡터 크기가 동일해야 효율적 주소 계산 가능. 가변 길이 feature에 대한 고려 없음
- **DGL 전용**: DGL 프레임워크에 맞춰 구현. PyG 등 다른 GNN 프레임워크로의 일반화 필요
- **Write 미지원**: 학습 중 생성되는 embedding 업데이트를 SSD에 쓰는 것은 미지원
- **Multi-GPU 학습**: 분산 학습 환경에서의 확장성 미검증

## 다른 논문과의 관계
- 선행 연구: BaM [I1] (핵심 I/O 인프라), EMOGI [P4] (GPU 그래프 처리)
- 후속 연구: GoFS [I4], GeminiFS [I5] (파일시스템 추상화 추가)
- 비교 대상: DGL 기본 파이프라인, UVA 방식, PyTorch-Direct
- 의의: BaM의 첫 번째 "킬러 애플리케이션"으로, GPU-initiated I/O의 실용성을 GNN 도메인에서 입증

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure 1: GNN 학습 파이프라인에서 feature gathering 병목 시각화
  - Figure 3: GIDS 아키텍처 다이어그램
  - Table 2: 데이터셋별 end-to-end 학습 시간 비교
- 핵심 수치/데이터:
  - DGL CPU 대비 582배 가속
  - UVA 대비 2~8배 빠름
  - 학습 정확도 동일 유지

## 메모
- GNN의 mini-batch 학습 특성(random neighbor sampling)이 BaM의 fine-grained random read와 완벽하게 매칭되는 좋은 사례
- 향후 LLM 학습에서의 대규모 embedding table 접근에도 유사한 패턴 적용 가능
