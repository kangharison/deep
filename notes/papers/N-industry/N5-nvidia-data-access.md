# [N5] GPUs as Data Access Engines

- **학회/연도:** Flash Memory Summit (FMS) 2024
- **저자:** Chris Newburn (NVIDIA), Micron 공동 발표
- **분류:** 산업 동향 및 표준

## 핵심 요약 (1~2문장)
NVIDIA와 Micron이 FMS 2024에서 공동 발표한 키노트로, GPU를 단순 연산 장치가 아닌 "데이터 접근 엔진"으로 재정의. BaM/GIDS를 Gen5 NVMe SSD(Micron)에서 구동한 실제 성능 데이터를 포함하며, GPU-initiated I/O가 차세대 AI 스토리지의 핵심 패러다임임을 산업계에 제시.

## 읽기 전 질문
- Gen5 NVMe SSD에서 GPU-initiated I/O 성능이 Gen4 대비 얼마나 향상되는가?
- NVIDIA와 Micron이 협업하는 구체적 기술 접점은 무엇인가?
- GPU가 "데이터 접근 엔진"이 되면 기존 CPU 중심 I/O 스택은 어떻게 변하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **AI 워크로드의 데이터 접근 패턴 변화:**
  - LLM 학습: 수 TB~PB 규모 데이터셋, GPU VRAM에 다 담을 수 없음
  - GNN: 그래프 구조의 불규칙한 랜덤 접근 패턴, 매우 높은 IOPS 요구
  - 추천 시스템: 임베딩 테이블이 수 TB → SSD에 저장 후 on-demand 접근
  - 추론: KV 캐시, LoRA 어댑터 등의 동적 로딩/언로딩
- **현재 스토리지 스택의 근본적 한계:**
  - CPU가 모든 I/O를 중재 → GPU 연산과 I/O가 직렬화
  - GPU가 데이터를 필요로 하는 시점과 CPU가 데이터를 준비하는 시점 간 갭
  - 프리페치 기반 접근: 접근 패턴이 불규칙할 때 효과 없음
- **GPU 활용률 저하:**
  - 데이터 대기 중 GPU SM(Streaming Multiprocessor)이 유휴 상태
  - GPU 가격이 매우 높으므로(H100: ~$30K+) 유휴 시간은 큰 비용 낭비

### 2. 제안 방법 (Approach)
- **GPU를 데이터 접근 엔진으로 활용:**
  - GPU 스레드가 연산 중 필요한 데이터를 직접 SSD에서 fetch
  - "Compute + I/O"를 GPU 내부에서 통합 → 연산과 I/O의 오버랩
  - CPU는 초기 설정과 예외 처리만 담당
- **BaM + Gen5 NVMe SSD 조합:**
  - BaM 프레임워크를 Micron Gen5 NVMe SSD에서 구동
  - Gen5의 높은 대역폭(~14GB/s)과 IOPS(~2M+)를 GPU-initiated로 활용
  - PCIe Gen5의 향상된 대역폭이 소규모 랜덤 I/O에도 유리함을 실증
- **GIDS(GPU-Initiated Direct Storage) 시연:**
  - GNN 학습에서 GPU가 직접 노드 feature를 SSD에서 읽어오는 데모
  - Micron 9400 등의 Gen5 SSD 활용

### 3. 핵심 아키텍처/설계

```
  GPU as Data Access Engine — 패러다임 전환
  ═══════════════════════════════════════════════

  [기존 패러다임] CPU = 데이터 관리자, GPU = 연산 전용
  ─────────────────────────────────────────────

  ┌────────┐  데이터   ┌────────┐  I/O    ┌────────┐
  │  GPU   │◀─ 전달 ──│  CPU   │◀─ 처리 ──│  SSD   │
  │(연산만)│          │(I/O+관리)│         │        │
  └────────┘          └────────┘         └────────┘
  문제: GPU 대기 ↑, CPU 병목 ↑

  [새 패러다임] GPU = 데이터 접근 + 연산 통합
  ─────────────────────────────────────────────

  ┌──────────────────────────┐
  │          GPU              │
  │  ┌─────────┐ ┌─────────┐ │        ┌────────┐
  │  │ 연산    │ │ I/O     │ │        │        │
  │  │ Warps   │ │ Warps   │─┼───────▶│ NVMe   │
  │  │         │ │(BaM)    │ │ PCIe   │ SSD    │
  │  │         │ │         │◀┼────────│(Gen5)  │
  │  └────┬────┘ └────┬────┘ │        │        │
  │       │    데이터   │      │        │ Micron │
  │       │◀───공유───│      │        │ 9400   │
  │       │   (VRAM)   │      │        │        │
  │  ┌────▼────────────▼────┐ │        └────────┘
  │  │      GPU VRAM        │ │
  │  │  (SQ/CQ + 데이터버퍼) │ │
  │  └──────────────────────┘ │
  └──────────────────────────┘

  BaM/GIDS + Gen5 NVMe 성능 스택
  ═══════════════════════════════════════════════

  ┌─────────────────────────────────────────┐
  │             성능 비교                    │
  │                                         │
  │  Gen4 NVMe (x4):                       │
  │  ├── 대역폭: ~7 GB/s                    │
  │  ├── 4KB 랜덤 읽기: ~1.0-1.5M IOPS     │
  │  └── GPU-initiated 유효 IOPS: ~800K     │
  │                                         │
  │  Gen5 NVMe (x4) — Micron 9400:         │
  │  ├── 대역폭: ~14 GB/s                   │
  │  ├── 4KB 랜덤 읽기: ~2.0M+ IOPS        │
  │  └── GPU-initiated 유효 IOPS: ~1.5M+    │
  │                                         │
  │  향상: 대역폭 2x, IOPS ~1.5-2x         │
  │  GPU 활용률 향상: I/O 대기 시간 감소     │
  └─────────────────────────────────────────┘

  GNN 워크로드 (GIDS) 예시
  ═══════════════════════════════════════════════

  GPU Thread (Warp):
    1. 샘플링: 이웃 노드 ID 결정
    2. ┌─ I/O 발행: 노드 feature를 SSD에서 읽기 ──▶ NVMe SSD
    3. │  (다른 warp는 연산 계속)                      │
    4. └─ I/O 완료: feature 도착 ◀───────────────────┘
    5. 집계(Aggregation): 이웃 feature로 연산
    6. GNN 레이어 업데이트

  핵심: 단계 2-3에서 I/O와 연산이 동시 실행 (overlap)
```

### 4. 실험 결과 (Key Results)
- **Gen5 NVMe SSD 성능 데이터 (Micron 9400 기반):**
  - PCIe Gen5 x4에서 순차 읽기: ~13-14 GB/s
  - 4KB 랜덤 읽기 IOPS: ~2M+ (SSD 단독)
  - GPU-initiated(BaM) 4KB 랜덤 읽기: Gen4 대비 ~1.5-2배 IOPS 향상
- **GNN 워크로드 (GIDS):**
  - DGL 기반 GNN 학습에서 CPU-initiated 대비 최대 수백 배 가속 (I2에서 582x 보고)
  - Gen5 SSD 사용 시 Gen4 대비 추가 ~30-50% 학습 시간 단축
  - 대규모 그래프 (수십억 노드)에서 GPU VRAM 초과 데이터를 SSD에서 직접 처리
- **GPU 활용률:**
  - 기존 (CPU-initiated): GPU SM 활용률 ~40-60% (I/O 대기)
  - BaM/GIDS (GPU-initiated): GPU SM 활용률 ~70-90% (I/O-연산 오버랩)
- **Micron 협업 의의:**
  - SSD 벤더(Micron)가 GPU-initiated I/O 성능 검증에 적극 참여
  - Gen5 SSD가 GPU-initiated I/O의 성능 잠재력을 완전히 활용 가능함을 실증

### 5. 한계점 및 향후 연구
- **SSD 수량 확장성:** 단일 SSD 중심 결과, 다수 SSD 병렬 시 PCIe 스위치 오버헤드 미검증
- **쓰기 워크로드:** 읽기 중심 결과, 체크포인트 등 쓰기 워크로드의 GPU-initiated 성능 미발표
- **소프트웨어 성숙도:** BaM/GIDS는 여전히 연구 프레임워크 → production 수준이 아님
- **GPU 세대 의존성:** 특정 NVIDIA GPU(A100, H100) 기준 결과
- **향후 방향:**
  - SCADA [N4]로의 기술 발전 (production 수준 GPU-initiated I/O)
  - Gen6 PCIe + NVMe 2.x에서의 추가 성능 향상
  - CXL 메모리를 활용한 GPU-SSD 간 버퍼 계층 추가
  - 다중 GPU-다중 SSD 토폴로지 최적화

## 다른 논문과의 관계
- 선행 연구: BaM [I1] — GPU-initiated I/O 프레임워크 (이 발표의 핵심 기술)
- 선행 연구: GIDS [I2] — GNN에 대한 BaM 적용 (이 발표의 워크로드 사례)
- 후속 연구: NVIDIA SCADA [N4] — 이 발표의 비전을 production으로 구현
- 관련: Micron GDS [G6] — 동일 Micron SSD의 GDS 모드 성능 비교
- 관련: DapuStor BaM [N3] — 다른 SSD 벤더의 유사 데모

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - GPU as Data Access Engine 개념 다이어그램
  - Gen4 vs Gen5 NVMe SSD의 GPU-initiated IOPS 비교 그래프
  - GNN 학습 파이프라인에서의 I/O-연산 오버랩 시각화
  - NVIDIA-Micron 공동 벤치마크 결과 표
- 핵심 수치/데이터:
  - Gen5 NVMe에서의 GPU-initiated 랜덤 읽기 IOPS (~2M+)
  - GNN 학습 가속 비율 (수백 배)
  - GPU 활용률 향상 (~40-60% → ~70-90%)
  - "GPU = 연산 전용"에서 "GPU = 연산 + 데이터 접근"으로의 패러다임 전환 근거

## 메모
- Chris Newburn은 NVIDIA의 스토리지/메모리 아키텍처 시니어 연구원
- FMS(Flash Memory Summit)는 플래시 스토리지 업계 최대 행사 → 산업 영향력 큼
- NVIDIA와 Micron의 공동 발표 → GPU 벤더 + SSD 벤더 간 협업 공식화
- 이 발표는 SCADA [N4] 이전 단계로, GPU-initiated I/O의 비전을 업계에 알린 역할
- "Data Access Engine" 용어는 GPU의 역할 재정의를 위한 NVIDIA의 전략적 브랜딩
