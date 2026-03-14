# [I7] Path to GPU-Initiated I/O for Data-Intensive Systems

- **학회/연도:** DaMoN 2025
- **저자:** Torp et al. (Samsung)
- **분류:** GPU Initiated I/O

## 핵심 요약 (1~2문장)
GPU-initiated I/O의 현재 상태를 체계적으로 분류(taxonomy)하고, 데이터베이스, 그래프 분석, ML 등 데이터 집약적 시스템에 적용하기 위한 경로(path)를 분석한 서베이 논문. Samsung 연구진이 참여하여 산업계 관점에서의 하드웨어 지원 현황과 과제를 함께 다룸.

## 읽기 전 질문
- GPU-initiated I/O 연구를 어떤 축(taxonomy)으로 분류하는가?
- 산업계(Samsung, NVIDIA)에서 GPU-initiated I/O를 제품화하는 데 가장 큰 장벽은 무엇인가?
- 데이터베이스 시스템에 GPU-initiated I/O를 통합하려면 쿼리 엔진의 어떤 부분을 수정해야 하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GPU-initiated I/O의 파편화**: BaM, GIDS, GoFS, GeminiFS 등 다양한 연구가 등장했지만 체계적 정리가 부재
- **실용화 장벽 불명확**: 학계 연구와 산업 현실 간의 간극. 실제 제품/시스템에 어떻게 적용할지에 대한 로드맵 부재
- **응용 분야별 요구사항 다름**: 데이터베이스, 그래프 분석, ML 학습/추론 등 각 분야가 요구하는 I/O 패턴과 추상화 수준이 상이
- **하드웨어 제약**: PCIe P2P 지원, NVMe 큐 수, GPU BAR 크기 등 하드웨어 레벨 제약에 대한 종합적 분석 필요

### 2. 제안 방법 (Approach)
- **체계적 분류(Taxonomy)**:
  1. **I/O 주도 주체**: CPU-mediated (GDS) vs GPU-initiated (BaM 계열) vs Hybrid (GeminiFS)
  2. **추상화 수준**: Raw block (BaM) vs File system (GoFS, GeminiFS) vs Application-specific (GIDS)
  3. **I/O 모델**: Synchronous (BaM) vs Asynchronous (AGILE)
  4. **캐시 위치**: GPU VRAM cache vs Host DRAM cache vs No cache
- **응용별 적용 경로 분석**:
  - 데이터베이스: 대규모 테이블 스캔, 인덱스 lookup에서의 GPU-initiated I/O 활용
  - 그래프 분석: BFS/SSSP에서의 불규칙 접근 패턴 처리
  - ML/DL: embedding table, feature store 접근
- **하드웨어 요구사항 정리**: PCIe Gen4/5 대역폭, P2P DMA 지원 SSD 목록, GPU BAR 크기 제약 등

### 3. 핵심 아키텍처/설계

```
┌──────────────────────────────────────────────────────┐
│          GPU-Initiated I/O Taxonomy                   │
│                                                      │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────┐  │
│  │ I/O 주도    │  │ 추상화 수준  │  │ I/O 모델   │  │
│  │             │  │              │  │            │  │
│  │ CPU-med.───→│  │ Raw Block──→ │  │ Sync ────→ │  │
│  │  (GDS)      │  │  (BaM)       │  │  (BaM)     │  │
│  │             │  │              │  │            │  │
│  │ GPU-init.──→│  │ File Sys ──→ │  │ Async ───→ │  │
│  │  (BaM)      │  │  (GoFS)      │  │  (AGILE)   │  │
│  │             │  │              │  │            │  │
│  │ Hybrid ───→ │  │ App-spec.──→ │  │            │  │
│  │  (GeminiFS) │  │  (GIDS)      │  │            │  │
│  └─────────────┘  └──────────────┘  └────────────┘  │
└──────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│       응용 분야별 적합한 접근법                       │
│                                                      │
│  데이터베이스  ──→ File system 추상화 필요            │
│                    (GoFS, GeminiFS)                  │
│                                                      │
│  그래프 분석   ──→ Fine-grained random read          │
│                    (BaM 직접 사용 적합)               │
│                                                      │
│  ML Feature    ──→ Application-specific 최적화       │
│  Store             (GIDS 스타일)                     │
│                                                      │
│  LLM 추론     ──→ 대규모 weight offloading           │
│                    (GMT 스타일 메모리 계층 관리)       │
└──────────────────────────────────────────────────────┘
```

**산업계 관점 (Samsung)**:
- 차세대 SSD에서의 P2P DMA 최적화 방향
- CXL 메모리와의 통합 가능성
- NVMe 스펙 확장 제안 (GPU-aware 큐 관리)

### 4. 실험 결과 (Key Results)
- **성능 비교 벤치마크** (서베이 논문이므로 기존 결과 종합):
  - BaM: random read에서 GDS 대비 최대 5배
  - GIDS: GNN 학습에서 CPU 대비 582배
  - GoFS: 멀티 SSD에서 선형 스케일링
- **하드웨어 특성 분석**:
  - PCIe Gen4 P2P 대역폭: ~25 GB/s (이론), 실측 ~20 GB/s
  - NVMe SSD P2P 지원 현황: Samsung PM9A3, Intel Optane P5800X 등에서 검증
  - GPU BAR 크기: A100 기준 256MB BAR1 (확장 가능), NVMe 큐 배치에 충분
- **병목 분석**: PCIe 토폴로지가 성능에 미치는 영향 정량화 (same switch vs cross switch)

### 5. 한계점 및 향후 연구
- **표준화 부재**: GPU-initiated I/O에 대한 NVMe 스펙 차원의 표준이 없음. 각 연구마다 독자적 구현
- **보안/멀티테넌시**: GPU가 NVMe 큐를 직접 조작하는 것에 대한 보안 프레임워크 미정립
- **CXL 통합**: CXL 메모리가 보편화되면 GPU-SSD 직접 접근의 필요성이 줄어들 수 있음 (CXL로 대용량 메모리 확보 가능)
- **소프트웨어 에코시스템**: 실제 애플리케이션(데이터베이스, ML 프레임워크)이 GPU-initiated I/O를 활용하려면 상당한 코드 수정 필요
- **에러 처리**: NVMe 에러(media error, timeout 등)를 GPU 커널에서 어떻게 처리할 것인가에 대한 합의 부재

## 다른 논문과의 관계
- 선행 연구: BaM [I1] 및 이 카테고리의 모든 논문을 서베이 대상으로 포함
- 후속 연구: 서베이 논문 자체가 향후 연구 방향 제시
- 비교 대상: GDS [G1] 계열 vs GPU-initiated [I1] 계열의 체계적 비교
- 의의: Samsung의 산업 관점이 포함되어, 학계 연구의 실용화 가능성을 현실적으로 평가

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - Figure: GPU-initiated I/O 연구의 분류 체계(taxonomy) 다이어그램
  - Table: 기존 연구 비교표 (BaM, GIDS, GoFS, GeminiFS, AGILE, Phoenix 등)
  - Figure: PCIe 토폴로지별 성능 차이 그래프
- 핵심 수치/데이터:
  - 각 시스템의 성능 수치 종합 비교
  - PCIe P2P 대역폭 실측치
  - 하드웨어 호환성 매트릭스

## 메모
- 서베이 논문이므로 전체 분야를 조감하는 데 가장 유용. 발표 도입부에서 "왜 GPU-initiated I/O가 필요한가"를 설명할 때 활용
- Samsung이 참여했다는 것은 SSD 벤더가 이 분야에 관심을 갖고 있다는 의미. 향후 SSD 펌웨어/하드웨어 레벨 최적화 기대
