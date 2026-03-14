# [N3] DapuStor BaM Demo 분석

- **학회/연도:** Blocks and Files 보도, 2024년 7월 (FMS 2024 전후)
- **저자:** DapuStor (중국 SSD 벤더, Haishen 시리즈)
- **분류:** 산업 동향 및 표준

## 핵심 요약 (1~2문장)
중국 SSD 벤더 DapuStor가 자사 NVMe SSD에서 BaM(GPU Initiated I/O) 프레임워크를 실제 구동하여 데모한 사례. 학술 연구(BaM)가 상용 SSD에서 동작 가능함을 검증하고, SSD 벤더가 GPU-initiated I/O를 제품 차별화 포인트로 활용하기 시작한 산업 전환점.

## 읽기 전 질문
- BaM 프레임워크가 상용 SSD에서 수정 없이 동작 가능한가?
- DapuStor SSD의 어떤 특성이 GPU-initiated I/O에 유리한가?
- 중국 SSD 벤더가 이 기술에 투자하는 전략적 이유는 무엇인가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- **GPU-initiated I/O의 실용성 검증 필요:**
  - BaM [I1]은 ASPLOS 2023에서 발표된 학술 연구
  - 연구 당시 사용된 SSD는 특정 벤더(삼성, Intel Optane 등)의 제품
  - 다른 SSD에서도 동작하는지, 성능은 어떤지 검증 필요
- **AI 워크로드의 스토리지 수요:**
  - LLM, 추천 시스템, GNN 등에서 GPU-SSD 직접 접근 요구 증가
  - 기존 GPUDirect Storage(GDS)는 CPU 관여 필요 → 확장성 한계
- **SSD 벤더의 차별화 압력:**
  - 범용 NVMe SSD 시장은 가격 경쟁 심화
  - AI/GPU 워크로드 특화 SSD로 차별화 필요

### 2. 제안 방법 (Approach)
- **DapuStor Haishen SSD + BaM 프레임워크 통합:**
  - DapuStor의 PCIe Gen4/Gen5 NVMe SSD에 BaM 소프트웨어 스택 적용
  - GPU(NVIDIA)에서 DapuStor SSD의 NVMe 큐에 직접 커맨드 발행
  - 별도의 SSD 펌웨어 수정 없이, 표준 NVMe 인터페이스로 동작
- **데모 구성:**
  - NVIDIA GPU + DapuStor Haishen 시리즈 SSD
  - BaM 프레임워크를 이용한 GPU-initiated 랜덤 읽기 워크로드
  - 그래프 분석(BFS, PageRank 등) 또는 유사 데이터 집약적 워크로드 시연

### 3. 핵심 아키텍처/설계

```
  DapuStor BaM 데모 구성
  ═══════════════════════════════════════════════

  ┌──────────────────────────────────────────┐
  │              데모 서버                     │
  │                                          │
  │  ┌──────────────────────┐                │
  │  │     NVIDIA GPU       │                │
  │  │  ┌────────────────┐  │                │
  │  │  │ BaM Runtime    │  │                │
  │  │  │ ┌────────────┐ │  │                │
  │  │  │ │GPU Threads │ │  │                │
  │  │  │ │(Graph BFS) │ │  │                │
  │  │  │ └─────┬──────┘ │  │                │
  │  │  │       │on-demand│  │                │
  │  │  │       │I/O 발행  │  │                │
  │  │  │ ┌─────▼──────┐ │  │                │
  │  │  │ │NVMe SQ/CQ  │ │  │                │
  │  │  │ │(GPU VRAM)  │ │  │                │
  │  │  │ └─────┬──────┘ │  │                │
  │  │  └───────┼────────┘  │                │
  │  └──────────┼───────────┘                │
  │             │ PCIe P2P                   │
  │             │ (CPU 우회)                  │
  │             ▼                            │
  │  ┌──────────────────────┐                │
  │  │  DapuStor Haishen    │                │
  │  │  NVMe SSD            │                │
  │  │  ┌────────────────┐  │                │
  │  │  │ 표준 NVMe I/F  │  │                │
  │  │  │ • Doorbell     │  │                │
  │  │  │ • SQ/CQ        │  │                │
  │  │  │ • DMA Engine   │  │                │
  │  │  └────────────────┘  │                │
  │  │  특징:               │                │
  │  │  • 높은 랜덤 읽기 IOPS │               │
  │  │  • 낮은 tail latency  │               │
  │  │  • 안정적 QoS         │               │
  │  └──────────────────────┘                │
  └──────────────────────────────────────────┘

  핵심 포인트:
  ① SSD 펌웨어 수정 없이 표준 NVMe로 동작
  ② GPU→SSD PCIe P2P DMA (CPU 개입 없음)
  ③ DapuStor SSD의 높은 4KB 랜덤 읽기 성능 활용
```

### 4. 실험 결과 (Key Results)
- **데모 수준의 결과** (Blocks and Files 보도 기반):
  - DapuStor Haishen SSD에서 BaM 프레임워크가 정상 동작 확인
  - GPU-initiated 랜덤 읽기로 높은 IOPS 달성
  - CPU 개입 없이 GPU가 직접 SSD에서 데이터 읽기 시연
- **DapuStor SSD 스펙 (Haishen 시리즈):**
  - PCIe Gen4 x4: 순차 읽기 ~7GB/s, 랜덤 읽기 ~1.5M IOPS (4KB)
  - PCIe Gen5 x4: 순차 읽기 ~14GB/s, 랜덤 읽기 ~2M+ IOPS (4KB)
  - 엔터프라이즈 급 endurance 및 QoS 보장
- **의의:** 학술 프로토타입(BaM)이 특정 연구용 SSD가 아닌 상용 SSD에서도 동작함을 실증

### 5. 한계점 및 향후 연구
- **데모 수준:** 체계적 벤치마크가 아닌 전시/데모 수준의 검증
- **단일 SSD 테스트:** 다수 SSD 병렬 구성에서의 확장성 미검증
- **쓰기 경로 미포함:** BaM 데모는 주로 읽기 중심, 쓰기 워크로드는 미시연
- **소프트웨어 성숙도:** BaM 프레임워크 자체가 연구 코드 수준
- **향후 방향:**
  - SSD 펌웨어 레벨의 GPU-initiated I/O 최적화 (큐 관리, 스케줄링)
  - Gen5 SSD에서의 본격적 성능 검증
  - DapuStor 자체 GPU-initiated I/O SDK 개발 가능성

## 다른 논문과의 관계
- 선행 연구: BaM [I1] — 데모에 사용된 GPU-initiated I/O 프레임워크
- 후속 연구: Marvell GPU Storage [N2] — SSD 컨트롤러 벤더의 전략적 대응
- 비교 대상: Micron GDS [G6], KIOXIA GDS [G8] — 다른 SSD 벤더의 GPU 스토리지 접근 (GDS 방식)
- 관련: NVIDIA SCADA [N4] — GPU 측에서의 production 솔루션

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table:
  - DapuStor 데모 구성 사진/다이어그램
  - BaM 프레임워크가 상용 SSD에서 동작하는 모습
- 핵심 수치/데이터:
  - "학술 연구 → 상용 SSD 데모"로의 기술 전환 타임라인 (BaM: 2023 → DapuStor 데모: 2024)
  - 중국 SSD 벤더의 GPU-storage 생태계 참여라는 지정학적 의미
  - BaM이 표준 NVMe 인터페이스만으로 동작 가능하다는 실증적 근거

## 메모
- DapuStor는 중국 심천 기반 SSD 벤더로, 데이터센터용 NVMe SSD 시장에서 성장 중
- Blocks and Files는 스토리지 업계 전문 미디어
- 이 데모의 가장 큰 의의: BaM이 "특정 연구 환경에서만 동작하는 것이 아님"을 증명
- SSD 벤더들이 GPU-initiated I/O를 경쟁 무기로 인식하기 시작한 초기 신호
