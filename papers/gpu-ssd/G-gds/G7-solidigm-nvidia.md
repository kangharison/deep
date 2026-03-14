# [G7] Accelerating AI With High Performance Storage

- **학회/연도:** Solidigm/NVIDIA 백서, 2025
- **저자:** Solidigm, NVIDIA
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
Solidigm D7-PS1030 NVMe SSD + NVIDIA GPU + BlueField DPU를 결합한 AI 스토리지 아키텍처 제안. GDS와 NVMe-oF를 통해 원격(disaggregated) 스토리지에서도 로컬 NVMe에 근접하는 성능을 달성하는 프로덕션 레벨 구성을 검증.

## 읽기 전 질문
- NVMe-oF + GDS 조합에서 네트워크 latency가 성능에 얼마나 영향을 미치는가?
- DPU(BlueField)가 GDS 성능에 어떤 역할을 하는가?
- 스토리지 disaggregation이 AI 학습 성능에 미치는 실질적 영향은?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- AI/LLM 학습 클러스터에서 GPU 노드와 스토리지 노드를 분리(disaggregate)하는 추세
  - GPU 노드는 비싸므로 로컬 SSD 대신 공유 스토리지 사용이 경제적
  - 하지만 네트워크를 통한 데이터 접근은 latency 증가와 throughput 감소 위험
- 기존 NFS/CIFS 기반 원격 스토리지는 AI 워크로드에 부적합
- GPU에 데이터를 빠르게 공급하면서 스토리지를 분리하는 아키텍처 필요

### 2. 제안 방법 (Approach)
- **3계층 아키텍처**: GPU 노드 + 네트워크(InfiniBand) + 스토리지 노드(Solidigm SSD)
- 핵심 기술 스택:
  - **GDS (GPUDirect Storage)**: GPU 메모리로 직접 데이터 전송
  - **NVMe-oF (NVMe over Fabrics)**: 네트워크를 통해 원격 NVMe SSD에 접근
  - **NVIDIA BlueField DPU**: 스토리지 노드에서 NVMe-oF 타겟 역할, CPU 오프로드
  - **Solidigm D7-PS1030**: 고성능 데이터센터 NVMe SSD
- 테스트: gdsio, fio, 실제 AI 학습 워크로드

### 3. 핵심 아키텍처/설계

```
  Solidigm/NVIDIA AI 스토리지 아키텍처:

  ┌──────────────────────┐        ┌──────────────────────┐
  │    GPU Compute Node  │        │   Storage Node       │
  │                      │        │                      │
  │  ┌──────┐  ┌──────┐ │        │  ┌──────────────┐    │
  │  │ GPU  │  │ GPU  │ │        │  │ Solidigm     │    │
  │  │(H100)│  │(H100)│ │        │  │ D7-PS1030    │    │
  │  └──┬───┘  └──┬───┘ │        │  │ NVMe SSD x N │    │
  │     │  PCIe   │     │        │  └──────┬───────┘    │
  │  ┌──▼─────────▼──┐  │        │         │            │
  │  │    PCIe Switch │  │        │  ┌──────▼───────┐    │
  │  └───────┬────────┘  │        │  │  BlueField   │    │
  │          │           │        │  │  DPU         │    │
  │  ┌───────▼────────┐  │  IB    │  │  (NVMe-oF    │    │
  │  │  ConnectX NIC  ├──┼────────┼──┤   Target)    │    │
  │  │  (200/400Gbps) │  │  RDMA  │  └──────────────┘    │
  │  └────────────────┘  │        └──────────────────────┘
  │                      │
  │  GDS + NVMe-oF       │
  │  Initiator           │
  └──────────────────────┘

  데이터 흐름:
  Solidigm SSD → BlueField DPU → InfiniBand RDMA → ConnectX NIC
                                                        │
                                          GDS: NIC → GPU (P2P DMA)
                                          (CPU 바운스 버퍼 제거)

  전통적 경로와 비교:
  Legacy: SSD → DPU → IB → NIC → CPU RAM → cudaMemcpy → GPU
  GDS:    SSD → DPU → IB → NIC ──── P2P DMA ────────→ GPU
```

### 4. 실험 결과 (Key Results)
- **원격 스토리지 GDS**: 로컬 NVMe 성능의 **~90% 이상** 달성 (InfiniBand HDR/NDR 사용 시)
- **Throughput**: 다수 Solidigm SSD + BlueField 구성에서 **수십 GB/s** aggregate read
- **CPU 오프로드**: BlueField DPU가 NVMe-oF 처리를 담당하여 호스트 CPU 부하 최소화
- **스케일링**: 스토리지 노드 수 증가에 따라 선형적 throughput 확장
- **AI 학습**: 데이터 로딩 파이프라인에서 스토리지 병목 제거 확인
- **비용 효율**: GPU 노드에서 로컬 SSD 제거로 하드웨어 비용 절감

### 5. 한계점 및 향후 연구
- 벤더 공동 백서로 최적 조건에서의 결과 — 실제 프로덕션 환경과 차이 가능
- Solidigm/NVIDIA 하드웨어에 특화된 구성 — 타 벤더 호환성 미검증
- NVMe-oF 구성 복잡도가 높아 운영 부담 존재
- 네트워크 혼잡(congestion) 상황에서의 성능 저하 분석 부족
- 소규모 random I/O 워크로드에서의 원격 GDS 성능 미검증
- DPU 비용이 추가되므로 TCO 분석이 더 상세해야 함

## 다른 논문과의 관계
- 선행 연구: GDS [G1] — 기본 GDS 성능 검증
- 후속 연구: 차세대 DPU(BlueField-4) + GDS 통합
- 비교 대상: [G3] 분산 파일시스템 GDS — 유사 목표(원격 스토리지 + GDS)를 다른 방식으로 접근

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 3계층 아키텍처 다이어그램, 원격 vs 로컬 throughput 비교
- 핵심 수치/데이터: 로컬 NVMe의 ~90% 성능 달성, aggregate throughput 수치
- **활용**: "프로덕션 AI 클러스터에서 GDS가 어떻게 배치되는가"의 실제 사례, 스토리지 disaggregation 트렌드 설명

## 메모
- 실제 데이터센터 아키텍처에 가장 가까운 구성. GDS의 산업 적용 현황을 보여주는 핵심 자료.
