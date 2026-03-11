# [G5] Insights into GPUDirect Data Transfer through NIXL Benchmarking

- **학회/연도:** 2025
- **저자:** Muradli et al.
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
NVIDIA NIXL(Networking Infrastructure eXtension Layer) 라이브러리를 통한 GPUDirect 데이터 전송을 체계적으로 벤치마킹. GDS, GPUDirect RDMA 등 다양한 전송 경로의 성능을 배치 크기, 전송 크기, 토폴로지 등 다각도로 분석한 최신(2025) 성능 데이터 제공.

## 읽기 전 질문
- NIXL이 기존 cuFile/GDS API 대비 어떤 추가 기능을 제공하는가?
- 배치 크기(batch size)가 GDS 성능에 어떤 영향을 미치는가?
- 최신 하드웨어(H100/B200 + Gen5 NVMe)에서 GDS 성능 한계는?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- AI/LLM 학습 및 추론에서 데이터 전송(SSD↔GPU, GPU↔GPU, NIC↔GPU)이 핵심 병목
- GPUDirect 패밀리(GDS, RDMA, P2P)의 성능 특성이 워크로드에 따라 크게 다름
- 최신 하드웨어에서의 체계적 성능 데이터 부족
- 배치 크기, 전송 크기, 동시성 등 세부 파라미터가 성능에 미치는 영향이 불명확
- NVIDIA의 새로운 NIXL 라이브러리의 성능 특성 검증 필요

### 2. 제안 방법 (Approach)
- **NIXL 기반 벤치마크 프레임워크** 구축
- 다양한 GPUDirect 전송 경로를 통일된 인터페이스로 측정:
  - GDS (SSD → GPU)
  - GPUDirect RDMA (NIC → GPU)
  - GPUDirect P2P (GPU → GPU)
- 주요 측정 파라미터:
  - 전송 크기 (4KB ~ 수GB)
  - 배치 크기 (단일 요청 vs 다수 요청 일괄)
  - 동시 전송 수
  - PCIe 토폴로지 영향
- 최신 HW: NVIDIA H100/A100, Gen4/Gen5 NVMe SSD

### 3. 핵심 아키텍처/설계

```
  NIXL 아키텍처:

  ┌──────────────────────────────────────┐
  │         Application Layer            │
  │    (AI Training / Inference)         │
  ├──────────────────────────────────────┤
  │              NIXL API                │
  │  (통합 데이터 전송 추상화 계층)        │
  ├────────┬────────┬───────────────────┤
  │  GDS   │ RDMA   │    P2P            │
  │Backend │Backend │  Backend          │
  ├────────┼────────┼───────────────────┤
  │cuFile  │ IB     │  CUDA IPC /       │
  │nvidia-fs│Verbs  │  NVLink           │
  └───┬────┴───┬────┴──────┬────────────┘
      │        │           │
  ┌───▼──┐ ┌──▼───┐  ┌───▼────┐
  │NVMe  │ │ NIC  │  │  GPU   │
  │SSD   │ │(IB)  │  │(Peer)  │
  └──────┘ └──────┘  └────────┘

  벤치마크 구성:
  ┌─────────────────────────────────────────┐
  │  Benchmark Runner                       │
  │  ┌─────────────────────────────────┐    │
  │  │ 파라미터 조합 생성               │    │
  │  │ (크기 × 배치 × 동시성 × 경로)    │    │
  │  ├─────────────────────────────────┤    │
  │  │ NIXL API 호출                   │    │
  │  ├─────────────────────────────────┤    │
  │  │ 성능 수집 (BW, Latency, IOPS)   │    │
  │  └─────────────────────────────────┘    │
  └─────────────────────────────────────────┘
```

### 4. 실험 결과 (Key Results)
- **GDS throughput**: 최신 NVMe SSD에서 단일 SSD 기준 **~6-7 GB/s** read 대역폭 달성
- **배치 크기 영향**: 배치 크기 증가 시 throughput 향상 — 단일 요청 대비 배치 처리가 최대 2-3배 효율적
- **전송 크기 임계점**: 128KB 이상에서 GDS 이점 본격화, 1MB+ 에서 최대 대역폭 도달
- **다중 SSD**: SSD 수 증가에 따라 aggregate throughput 선형 스케일 (PCIe 대역폭 한계까지)
- **PCIe 토폴로지**: GPU와 NVMe가 같은 PCIe 스위치 하위일 때 최적 성능
- **RDMA 비교**: GPUDirect RDMA는 소규모 전송에서 GDS보다 latency 낮음

### 5. 한계점 및 향후 연구
- NIXL이 아직 초기 단계 — API 안정성 및 에코시스템 성숙도 부족
- 벤치마크 중심 연구로 실제 애플리케이션 워크로드 검증 제한적
- Write 경로 분석이 Read 대비 상대적으로 부족
- NVIDIA 하드웨어 전용 — AMD/Intel GPU에서의 비교 없음
- Gen5 NVMe SSD의 잠재력을 완전히 활용하지 못하는 경우에 대한 분석 필요

## 다른 논문과의 관계
- 선행 연구: GDS [G1] — 초기 GDS 성능 baseline
- 후속 연구: NIXL 기반 AI 프레임워크 통합
- 비교 대상: [G6] Micron, [G8] KIOXIA — 벤더별 GDS 성능 데이터

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 배치 크기별 throughput 곡선, 전송 크기별 대역폭 그래프, PCIe 토폴로지 영향 분석
- 핵심 수치/데이터: 최신 HW에서 GDS 최대 대역폭, 배치 크기에 따른 성능 변화 배수
- **활용**: 가장 최신 GDS 성능 데이터로 활용, NIXL의 통합 전송 추상화 개념 소개

## 메모
- 2025년 기준 가장 최신 GDS 성능 데이터를 담고 있어 현재 상태(state-of-the-art) 파악에 필수
