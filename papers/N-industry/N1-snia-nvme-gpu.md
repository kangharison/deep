# [N1] Why does NVMe Need to Evolve for Efficient Storage Access from GPUs?

- **학회/연도:** SNIA SDC 2024
- **저자:** SNIA NVMe Working Group (Samsung, NVIDIA, Intel 등 참여)
- **분류:** 산업 동향 및 표준

## 핵심 요약 (1~2문장)
현재 NVMe 스펙이 GPU에서의 직접 스토리지 접근에 적합하지 않은 구조적 문제를 분석하고, NVMe 프로토콜이 GPU Initiated I/O를 네이티브로 지원하기 위해 필요한 스펙 변경 사항을 제안한 SNIA 표준 관점의 발표 자료.

## 읽기 전 질문
- 현재 NVMe 스펙의 어떤 부분이 GPU 직접 접근에 병목인가?
- NVMe 큐(SQ/CQ) 구조가 GPU 스레드 모델과 왜 맞지 않는가?
- NVMe 스펙 개정 시 하위 호환성은 어떻게 보장하는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **CPU 중심 설계:** 현재 NVMe 스펙은 CPU가 I/O를 발행하는 것을 전제로 설계됨
  - Doorbell 레지스터가 MMIO 기반 → GPU에서 접근 시 PCIe 트랜잭션 오버헤드 큼
  - Submission Queue(SQ)/Completion Queue(CQ) 구조가 CPU 코어 단위로 최적화
- **GPU 아키텍처와의 불일치:**
  - GPU는 수천 개의 스레드가 동시에 실행 → NVMe 큐 수가 부족 (최대 64K개지만 실질적 제약)
  - GPU 스레드 간 큐 공유 시 동기화(lock) 오버헤드 발생
  - GPU warp 내 스레드들이 개별 I/O를 발행하면 doorbell storm 발생
- **메모리 모델 차이:**
  - NVMe PRP/SGL이 호스트 메모리(CPU DRAM) 기반
  - GPU 메모리(VRAM)를 직접 가리키는 메커니즘 부재
  - BAR 영역을 통한 우회 접근은 대역폭 제한

### 2. 제안 방법 (Approach)
- **NVMe 스펙 확장 방향 제시:**
  1. **Shadow Doorbell 개선:** GPU에서 doorbell 쓰기를 효율화하는 메커니즘
  2. **큐 공유 모델:** 다수의 GPU 스레드가 하나의 SQ를 안전하게 공유하는 방법
     - Atomic 연산 기반 SQ tail 포인터 업데이트
  3. **GPU 메모리 주소 지원:** PRP/SGL에 GPU 물리 주소를 직접 기술
  4. **Batched Doorbell:** 다수의 커맨드를 한 번의 doorbell로 통지
  5. **Completion 통지 최적화:** GPU 폴링에 적합한 CQ 처리 방식

- **단계적 접근:**
  - 단기: 기존 스펙 내에서 BaM 방식의 소프트웨어 우회 (현재 상태)
  - 중기: NVMe 2.x 스펙에 GPU 관련 optional feature 추가
  - 장기: GPU-native NVMe 프로파일 정의

### 3. 핵심 아키텍처/설계

```
  현재 NVMe I/O 경로 (CPU 중심)
  ═══════════════════════════════════════════════

  ┌──────────┐     ┌──────────┐     ┌──────────┐
  │ CPU Core │────▶│ NVMe     │────▶│ NVMe SSD │
  │          │     │ Driver   │     │          │
  └──────────┘     └──────────┘     └──────────┘
       │                │                │
       │  1. SQ Entry   │  2. Doorbell   │
       │     작성       │     Write      │
       │                │  3. DMA Read   │
       │                │◀───────────────│
       │                │  4. Completion │
       │◀───────────────│     (IRQ)      │

  GPU Initiated I/O를 위한 NVMe 진화 방향
  ═══════════════════════════════════════════════

  ┌─────────────────┐
  │   GPU (수천 SM)  │
  │  ┌───┐┌───┐┌───┐│     ┌──────────┐
  │  │SM0││SM1││SM2││     │ NVMe SSD │
  │  └─┬─┘└─┬─┘└─┬─┘│     │          │
  │    │    │    │   │     │          │
  │  ┌─▼────▼────▼─┐│     │          │
  │  │Shared SQ    ││────▶│  SQ      │
  │  │(Atomic Tail)││     │          │
  │  └─────────────┘│     │          │
  │  ┌─────────────┐│     │          │
  │  │Batched      ││────▶│ Doorbell │
  │  │Doorbell     ││     │          │
  │  └─────────────┘│     │          │
  │  ┌─────────────┐│◀────│          │
  │  │CQ (Polling) ││     │  CQ      │
  │  └─────────────┘│     │          │
  └─────────────────┘     └──────────┘

  핵심 변경점:
  ① SQ를 GPU VRAM에 배치 (GPU 물리주소 지원)
  ② Atomic 연산으로 SQ tail 동시 업데이트
  ③ Batched doorbell로 doorbell storm 방지
  ④ GPU 폴링 기반 completion 처리
```

### 4. 실험 결과 (Key Results)
- 이 발표는 표준 제안이므로 직접적 벤치마크 데이터보다는 문제 분석 중심
- BaM [I1] 연구 결과를 인용하여 현재 소프트웨어 우회 방식의 성능 한계를 제시:
  - GPU doorbell 쓰기 당 ~1-2μs의 PCIe 라운드트립 오버헤드
  - 큐 공유 시 lock contention으로 인한 throughput 저하 (~30-40%)
  - 스펙 수준 지원 시 이론적으로 2-3배 IOPS 향상 기대

### 5. 한계점 및 향후 연구
- **표준화 속도:** NVMe 스펙 개정은 업계 합의 필요 → 수년 소요 예상
- **하위 호환성:** 기존 SSD 컨트롤러와의 호환성 유지 과제
- **GPU 벤더 종속성:** NVIDIA GPU 아키텍처 중심 논의 → AMD/Intel GPU 고려 부족
- **보안 이슈:** GPU가 직접 NVMe 디바이스에 접근 시 격리/보안 모델 미정립
- **향후 방향:**
  - NVMe 2.1+ 스펙에 GPU 접근 관련 TP(Technical Proposal) 제출 예정
  - CXL과의 연계 (GPU-SSD 간 shared memory 활용) 검토

## 다른 논문과의 관계
- 선행 연구: BaM [I1] — GPU Initiated I/O의 소프트웨어 구현체로 문제점 도출
- 후속 연구: NVIDIA SCADA [N4] — 스펙 변경 없이 production 솔루션 제공
- 비교 대상: GPU Initiated Survey [I7] — 학술적 관점의 종합 분석

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - NVMe 큐 구조와 GPU 스레드 모델 불일치 다이어그램
  - Doorbell storm 문제 시각화
  - NVMe 스펙 진화 로드맵
- 핵심 수치/데이터:
  - GPU doorbell MMIO 오버헤드: ~1-2μs per doorbell
  - NVMe 최대 큐 수(65,535) vs GPU 동시 스레드 수(수만~수십만)
  - 표준 관점에서 본 GPU-NVMe 접근의 3가지 핵심 과제 (큐 공유, doorbell 효율, 메모리 주소)

## 메모
- SNIA는 스토리지 업계 표준 단체로, 이 발표는 NVMe 스펙 방향성에 직접 영향
- BaM이 학술 연구에서 도출한 문제를 산업 표준 관점에서 재정리한 의의
- GPU Initiated I/O가 "연구 프로토타입"에서 "표준화 대상"으로 격상된 시점의 기록
