# [N2] The Next Step for AI Storage: GPU-initiated and CPU-initiated Storage

- **학회/연도:** Marvell Blog, 2025년 1월
- **저자:** Marvell Technology (SSD 컨트롤러 시장 주요 벤더)
- **분류:** 산업 동향 및 표준

## 핵심 요약 (1~2문장)
SSD 컨트롤러 시장 리더인 Marvell이 AI 스토리지의 미래를 GPU-initiated와 CPU-initiated의 이중 경로로 정의하고, SSD 컨트롤러가 두 가지 I/O 패턴을 모두 효율적으로 처리할 수 있어야 한다는 SSD 벤더 관점의 전략 문서.

## 읽기 전 질문
- SSD 컨트롤러 입장에서 GPU-initiated I/O는 기존 CPU I/O와 어떤 차이가 있는가?
- Marvell은 GPU-initiated I/O 지원을 위해 컨트롤러 설계를 어떻게 바꾸려 하는가?
- AI 워크로드의 I/O 패턴이 기존 엔터프라이즈 워크로드와 어떻게 다른가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **AI 워크로드의 스토리지 요구 변화:**
  - LLM 학습: 체크포인트 저장/복원 시 대용량 순차 쓰기/읽기 (수백 GB 단위)
  - 추론: KV 캐시 오프로드, 모델 파라미터 로딩 → 빈번한 랜덤 읽기
  - GNN/추천 시스템: 임베딩 테이블 접근 → 매우 높은 IOPS의 소규모 랜덤 읽기
- **현재 스토리지 스택의 병목:**
  - CPU가 모든 I/O를 중재 → GPU 연산과 I/O 간 동기화 오버헤드
  - GPUDirect Storage(GDS)도 CPU가 I/O를 발행 → CPU가 병목
  - GPU-initiated I/O는 CPU를 우회하지만, 현재 SSD는 이를 고려하지 않은 설계
- **두 가지 I/O 패턴의 공존 필요:**
  - CPU-initiated: 전통적 파일시스템 경유, 대용량 순차 I/O에 최적
  - GPU-initiated: BaM 방식, 소규모 랜덤 I/O에 최적, 매우 높은 큐 깊이

### 2. 제안 방법 (Approach)
- **Dual-Path SSD 컨트롤러 아키텍처:**
  - 하나의 SSD가 CPU-initiated와 GPU-initiated I/O를 동시에 처리
  - 각 경로에 최적화된 큐 관리 및 스케줄링
- **GPU-initiated I/O 최적화를 위한 SSD 설계 원칙:**
  1. **고밀도 큐 지원:** 수천 개의 NVMe 큐를 효율적으로 관리
  2. **소규모 I/O 최적화:** 512B~4KB 크기의 랜덤 읽기 성능 극대화
  3. **초저지연 경로:** 컨트롤러 내부 커맨드 처리 파이프라인 단축
  4. **높은 IOPS 확장성:** 수백만 IOPS를 단일 SSD에서 달성
- **Gen5 NVMe 활용:**
  - PCIe Gen5 x4 대역폭(~14GB/s)을 활용한 GPU-SSD 직접 연결
  - Gen5의 높은 대역폭이 GPU-initiated의 소규모 랜덤 I/O에도 유리

### 3. 핵심 아키텍처/설계

```
  AI 스토리지의 이중 I/O 경로
  ═══════════════════════════════════════════════

  ┌──────────────────────────────────────────┐
  │              AI 서버                      │
  │                                          │
  │  ┌─────────┐         ┌─────────────┐     │
  │  │  CPU    │         │    GPU      │     │
  │  │         │         │  (H100 등)   │     │
  │  └────┬────┘         └──────┬──────┘     │
  │       │                     │            │
  │       │ CPU-initiated       │ GPU-initiated
  │       │ (파일시스템 경유)      │ (NVMe 직접)
  │       │ 대용량 순차 I/O      │ 소규모 랜덤 I/O
  │       │                     │            │
  └───────┼─────────────────────┼────────────┘
          │    PCIe Gen5        │
          ▼                     ▼
  ┌──────────────────────────────────────────┐
  │         NVMe SSD (Gen5)                  │
  │  ┌─────────────────────────────────────┐ │
  │  │     Marvell SSD 컨트롤러             │ │
  │  │  ┌──────────┐  ┌─────────────────┐  │ │
  │  │  │CPU Path  │  │ GPU Path        │  │ │
  │  │  │• 소수 큐  │  │• 다수 큐 (수천)  │  │ │
  │  │  │• 대용량IO │  │• 소규모 랜덤 IO  │  │ │
  │  │  │• 높은 BW  │  │• 높은 IOPS      │  │ │
  │  │  └──────────┘  └─────────────────┘  │ │
  │  │         ┌──────────────┐            │ │
  │  │         │  통합 FTL     │            │ │
  │  │         │  + 스케줄러   │            │ │
  │  │         └──────────────┘            │ │
  │  └─────────────────────────────────────┘ │
  │  ┌─────────────────────────────────────┐ │
  │  │         NAND Flash Array             │ │
  │  └─────────────────────────────────────┘ │
  └──────────────────────────────────────────┘
```

### 4. 실험 결과 (Key Results)
- Marvell 블로그 포스트는 벤더 전략 문서이므로 상세 벤치마크보다 방향성 제시 중심
- 제시된 핵심 수치/트렌드:
  - AI 학습 데이터셋 크기: 수 TB ~ 수십 TB로 지속 증가
  - GPU-initiated I/O 시 필요 IOPS: 단일 GPU 당 수백만 IOPS (4KB 랜덤 읽기)
  - Gen5 NVMe SSD의 이론적 랜덤 읽기 IOPS: ~2M IOPS (4KB)
  - CPU-initiated 대비 GPU-initiated의 지연 시간 감소: CPU 오버헤드 제거로 ~50% 감소 기대
- Marvell Bravera SC5 컨트롤러 기반 Gen5 SSD 레퍼런스 플랫폼 언급

### 5. 한계점 및 향후 연구
- **소프트웨어 에코시스템 미성숙:** GPU-initiated I/O를 활용하는 표준 소프트웨어 스택 부재
- **SSD 펌웨어 복잡도 증가:** 이중 경로 지원으로 FTL 설계 복잡도 상승
- **상호운용성:** NVIDIA GPU에 최적화 → AMD, Intel GPU와의 호환성 미지수
- **NVMe 표준 미확정:** GPU-initiated 관련 NVMe 스펙이 아직 확정되지 않음
- **향후 방향:**
  - CXL 메모리를 활용한 GPU-SSD 간 공유 버퍼 탐색
  - 컨트롤러 내부 AI 가속기 내장 (computational storage와의 융합)
  - NVMe over Fabrics를 통한 원격 GPU-initiated I/O

## 다른 논문과의 관계
- 선행 연구: BaM [I1] — GPU-initiated I/O 개념의 학술적 원조
- 후속 연구: NVIDIA SCADA [N4] — NVIDIA의 production 솔루션
- 비교 대상: DapuStor [N3] — 다른 SSD 벤더의 BaM 데모 접근
- 관련: SNIA NVMe-GPU [N1] — NVMe 스펙 관점의 분석

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - CPU-initiated vs GPU-initiated I/O 경로 비교 다이어그램
  - AI 워크로드별 I/O 패턴 분류 표
  - SSD 컨트롤러 이중 경로 아키텍처 개념도
- 핵심 수치/데이터:
  - SSD 컨트롤러 시장에서 Marvell의 위치 (주요 벤더)
  - Gen5 NVMe SSD 스펙 (대역폭, IOPS)
  - "SSD 벤더도 GPU-initiated I/O를 인식하고 대응 시작"이라는 산업 동향의 근거

## 메모
- Marvell은 SSD 컨트롤러 IP를 삼성, SK하이닉스 등 주요 SSD 제조사에 공급
- 이 블로그 포스트는 사실상 SSD 컨트롤러 로드맵의 공개적 선언
- GPU-initiated I/O가 "학술 연구"에서 "SSD 벤더의 제품 전략"으로 전환된 시점의 증거
- Marvell Bravera SC5는 PCIe Gen5 NVMe SSD 컨트롤러
