# [N4] NVIDIA SCADA (Storage Control-path And Data-path Acceleration)

- **학회/연도:** NVIDIA, 2025년 (GTC 2025 등에서 공개)
- **저자:** NVIDIA (GPU Direct Storage 팀, Isaac Gelado 등)
- **분류:** 산업 동향 및 표준

## 핵심 요약 (1~2문장)
NVIDIA가 GPUDirect Storage(GDS)의 후속으로 발표한 SCADA는 스토리지의 제어 경로(control-path)와 데이터 경로(data-path) 모두를 GPU로 가속하는 프레임워크. BaM에서 시작된 GPU Initiated I/O 개념을 NVIDIA가 production 수준으로 제품화한 것으로, 연구 프로토타입에서 상용 기술로의 전환을 공식적으로 보여주는 핵심 발표.

## 읽기 전 질문
- SCADA는 기존 GPUDirect Storage(GDS)와 구체적으로 어떻게 다른가?
- "Control-path acceleration"이란 정확히 무엇을 의미하는가?
- SCADA가 BaM의 어떤 부분을 계승하고, 어떤 부분을 개선했는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GPUDirect Storage(GDS)의 한계:**
  - GDS는 data-path만 최적화: NVMe DMA가 GPU 메모리로 직접 전송 (bounce buffer 제거)
  - 하지만 control-path는 여전히 CPU가 담당: I/O 발행, 큐 관리, completion 처리 모두 CPU
  - CPU가 I/O 발행의 병목 → GPU가 아무리 빨라도 CPU 속도에 제한
- **AI 워크로드의 스케일 문제:**
  - 수천 개의 GPU가 동시에 스토리지 접근하는 대규모 학습 클러스터
  - 각 GPU마다 독립적으로 I/O를 발행해야 하지만 CPU 코어 수가 GPU 수보다 부족
  - 체크포인트 저장/복원 시 모든 GPU가 동시에 대량 I/O 발생 → CPU 과부하
- **BaM의 한계를 넘어서는 과제:**
  - BaM은 연구 프로토타입 → 안정성, 에러 처리, 파일시스템 통합 미흡
  - Production 환경에서 사용하려면 NVIDIA 소프트웨어 스택과의 통합 필요

### 2. 제안 방법 (Approach)
- **SCADA: Storage Control-path And Data-path Acceleration**
  - Control-path 가속: GPU가 직접 NVMe 커맨드를 생성하고 SQ에 기록
  - Data-path 가속: 기존 GDS의 P2P DMA를 계승하여 데이터 직접 전송
  - 두 경로를 모두 GPU로 이전하여 CPU를 I/O 루프에서 완전히 제거
- **핵심 구성 요소:**
  1. **GPU-side NVMe Driver:** GPU 커널 내에서 실행되는 경량 NVMe 드라이버
  2. **Queue Management:** GPU 스레드가 NVMe SQ/CQ를 직접 관리
  3. **Completion Polling:** GPU 스레드가 CQ를 폴링하여 I/O 완료 감지
  4. **CUDA 통합:** CUDA 프로그래밍 모델에 자연스럽게 통합된 API
- **GDS와의 차별화:**
  - GDS: CPU가 I/O 발행 + GPU 메모리로 DMA (data-path only)
  - SCADA: GPU가 I/O 발행 + GPU 메모리로 DMA (control + data path)

### 3. 핵심 아키텍처/설계

```
  GDS vs SCADA 아키텍처 비교
  ═══════════════════════════════════════════════

  [GPUDirect Storage (GDS)] — Data-path만 가속
  ─────────────────────────────────────────────
  ┌─────────┐  1.I/O 요청  ┌─────────┐
  │  GPU    │─────────────▶│  CPU    │
  │  (App)  │              │         │
  │         │              │ 2.NVMe  │
  │         │              │  커맨드  │
  │         │              │  발행   │
  │         │              └────┬────┘
  │         │                   │
  │         │   3.DMA 직접전송  │
  │  VRAM ◀─┼───────────────────┘
  │         │              ┌─────────┐
  │         │◀─ ─ ─ P2P ──│ NVMe SSD│
  └─────────┘  (bounce     └─────────┘
               buffer 없음)

  [SCADA] — Control-path + Data-path 모두 가속
  ─────────────────────────────────────────────
  ┌──────────────────────┐
  │         GPU          │
  │  ┌────────────────┐  │
  │  │  Application   │  │     ┌─────────┐
  │  └───────┬────────┘  │     │         │
  │          │           │     │         │
  │  ┌───────▼────────┐  │     │         │
  │  │ GPU NVMe       │  │     │         │
  │  │ Control-path   │  │     │         │
  │  │ ┌────────────┐ │  │     │ NVMe   │
  │  │ │SQ 커맨드    │──────────▶│ SSD    │
  │  │ │작성+Doorbell│ │  │     │         │
  │  │ └────────────┘ │  │     │         │
  │  │ ┌────────────┐ │  │     │         │
  │  │ │CQ 폴링     │◀──────────│         │
  │  │ │+Completion │ │  │     │         │
  │  │ └────────────┘ │  │     │         │
  │  └────────────────┘  │     │         │
  │                      │     │         │
  │  VRAM ◀── ── P2P DMA ──── │         │
  │  (Data-path)         │     │         │
  └──────────────────────┘     └─────────┘
       CPU는 초기 설정만 담당
       (PCIe config, 큐 할당 등)

  ┌─────────────────────────────────────────┐
  │          성능 향상 요인 정리             │
  │                                         │
  │  GDS:    GPU ──req──▶ CPU ──cmd──▶ SSD  │
  │          지연: GPU↔CPU 통신 + CPU 처리   │
  │                                         │
  │  SCADA:  GPU ─────cmd─────▶ SSD         │
  │          지연: GPU 처리만 (CPU 제거)      │
  │                                         │
  │  절감: CPU 컨텍스트 스위치, 인터럽트,     │
  │        커널 드라이버 오버헤드 전부 제거    │
  └─────────────────────────────────────────┘
```

### 4. 실험 결과 (Key Results)
- **NVIDIA 공식 발표 기준:**
  - GDS 대비 SCADA의 I/O 지연시간: ~50% 이상 감소 (CPU 오버헤드 제거)
  - 단일 GPU에서의 IOPS: GDS 대비 2-3배 향상 (소규모 랜덤 I/O)
  - 대규모 체크포인트 저장: 수백 GPU 클러스터에서 체크포인트 시간 대폭 단축
  - 다수 NVMe SSD 병렬 접근 시 CPU가 병목이 되지 않음 → 선형 확장
- **활용 사례:**
  - LLM 학습 체크포인트: 수 TB 데이터를 GPU에서 직접 NVMe에 저장
  - 추론 시 KV 캐시 관리: 실시간 KV 캐시 페이지 교체
  - 데이터 전처리: GPU에서 직접 raw 데이터를 SSD에서 읽어 전처리
- **지원 플랫폼:**
  - NVIDIA Hopper (H100) 이상의 GPU
  - NVIDIA BlueField DPU와의 연계 가능성
  - 표준 NVMe SSD와 호환 (특별한 SSD 필요 없음)

### 5. 한계점 및 향후 연구
- **NVIDIA 종속:** CUDA + NVIDIA GPU에서만 동작 → 벤더 락인
- **NVMe 스펙 제약:** 현재 NVMe 스펙은 GPU-initiated를 공식 지원하지 않음 → 우회 구현
- **파일시스템 통합:** 블록 레벨 접근만 지원, 파일시스템 레벨 추상화는 별도 필요
- **에러 처리:** GPU에서의 NVMe 에러 복구 메커니즘이 CPU 대비 제한적
- **멀티테넌시:** 여러 GPU가 동일 SSD를 공유할 때의 격리/보안 과제
- **향후 방향:**
  - NVIDIA Blackwell 아키텍처에서의 개선된 SCADA 지원
  - NVMe 스펙과의 공식 협력 (SNIA [N1]과 연계)
  - CXL 메모리를 활용한 GPU-SSD 간 메타데이터 공유
  - NIXL [G5]과의 통합으로 분산 환경 확장

## 다른 논문과의 관계
- 선행 연구: BaM [I1] — GPU-initiated I/O의 학술적 원조 (SCADA의 지적 기반)
- 선행 연구: GDS [G1] — SCADA의 직접적 전신 (data-path 가속)
- 관련: SNIA NVMe-GPU [N1] — NVMe 스펙 관점에서 SCADA가 필요로 하는 표준 변경
- 관련: FMS 2024 [N5] — SCADA 이전 단계의 NVIDIA 비전 발표
- 비교: GoFS [I4], GeminiFS [I5] — 학술적 GPU 파일시스템 vs NVIDIA의 상용 접근

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - GDS vs SCADA 아키텍처 비교 다이어그램
  - Control-path vs Data-path 분리 개념도
  - 성능 비교 그래프 (GDS vs SCADA)
- 핵심 수치/데이터:
  - "CPU를 I/O 경로에서 완전히 제거"라는 아키텍처 전환의 의미
  - BaM(2023) → GDS(2020~) → SCADA(2025) 기술 진화 타임라인
  - NVIDIA가 GPU-initiated I/O를 공식 제품으로 채택한 최초의 사례

## 메모
- SCADA는 BaM 논문의 공저자인 Isaac Gelado(NVIDIA)가 주도한 것으로 추정
- BaM의 학술 성과가 NVIDIA 내부에서 제품화된 대표적 사례
- GDS → SCADA 전환은 "data-path only" → "full-path" 가속의 자연스러운 진화
- NVIDIA의 AI 인프라 전략에서 스토리지가 핵심 구성 요소로 격상된 신호
- Production 배포 시 기존 GDS 사용자의 마이그레이션 경로가 중요 과제
