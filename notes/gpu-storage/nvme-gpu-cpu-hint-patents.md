# NVMe Device에 GPU/CPU IO 출처 Hint를 전달하는 관련 특허

## 배경

NVMe SSD 컨트롤러가 수신한 I/O 요청이 CPU에서 발생한 것인지 GPU에서 발생한 것인지 구분할 수 있다면, 내부 스케줄링·데이터 배치·QoS 최적화에 활용할 수 있다. 현재 NVMe 표준에는 "이 IO는 GPU에서 왔다"는 명시적 필드가 없으므로, 다양한 우회 메커니즘과 특허가 존재한다.

## 직접 관련 특허

### 1. US20250045094A1 — "GPU-Initiated Data Access of Scaled Storage" (NVIDIA)

- **출원일**: 2023-08-03
- **공개일**: 2025-02-06
- **링크**: https://patents.justia.com/patent/20250045094
- **핵심**: GPU(PPU)가 직접 스토리지에 데이터 접근을 시작(initiate)하는 구조. 데이터가 어느 tier(GPU 접근 가능 tier vs 서버 인터페이스 필요 tier)에 있는지에 따라 접근 방식을 분기
- **GPU/CPU 구분 방식**: NVIDIA SCADA의 특허 기반. GPU가 NVMe SSD의 doorbell register에 직접 쓰기 → SSD 컨트롤러는 GPU 메모리의 NVMe 큐에서 command를 fetch하므로, DMA 목적지 주소가 GPU 메모리 영역이라는 것 자체가 암묵적 hint가 됨

### 2. US10,884,920 — "Metadata-based Operations for Solid State Devices"

- **등록일**: 2021-01-05
- **링크**: https://patents.justia.com/patent/10884920
- **핵심**: NVMe I/O command에 metadata를 인코딩하여 host가 SSD 컨트롤러에게 hint를 전달. 컨트롤러는 metadata를 파싱하여 hot/cold 데이터, sequential/random 등의 hint를 추출하고, 실제 device 동작과의 상관관계를 분석하여 최적화에 활용
- **GPU/CPU 구분 방식**: 이 메커니즘을 확장하면 "이 IO는 GPU에서 온 것" 같은 originator 정보를 metadata에 넣을 수 있음

### 3. US20220404999A1 — "System and Method to Create Persistent Host Metadata Logs in NVMe SSD"

- **공개일**: 2022-12-22
- **링크**: https://patents.justia.com/patent/20220404999
- **핵심**: NVMe Set command의 vendor-specific 영역을 통해 host가 telemetry 정보와 metadata를 SSD에 기록. 어떤 종류의 workload가 SSD를 사용하는지 추적 가능
- **GPU/CPU 구분 방식**: GPU/CPU workload 구분 정보를 vendor-specific metadata 채널로 전달 가능

### 4. CN210776403U — "GPUDirect Storage 호환 서버 아키텍처"

- **링크**: https://patents.google.com/patent/CN210776403U/en
- **핵심**: PCIe Switch를 통해 GPU가 NVMe에 직접 접근하는 하드웨어 토폴로지 특허. CPU 경유 NVMe, GPU 직접 NVMe, 혼합 모드 3가지를 지원
- **GPU/CPU 구분 방식**: PCIe 경로 자체가 물리적으로 분리됨

## NVMe 스펙 수준에서의 hint 메커니즘

현재 NVMe 표준에 "CPU vs GPU" 구분 전용 command field는 없지만, 활용 가능한 기존 메커니즘이 있다:

```
NVMe Command (Submission Queue Entry, 64 bytes)
┌─────────────────────────────────────────────┐
│ CDW0: Opcode, FUSE, CID                     │
│ CDW1: NSID                                  │
│ CDW2-3: Reserved                            │
│ CDW4-5: Metadata Pointer   ← hint 전달 가능  │
│ CDW6-9: PRP/SGL            ← DMA 주소로 추론  │
│ CDW10: Slba (lower)                         │
│ CDW11: Slba (upper)                         │
│ CDW12: NLB, DTYPE, PRINFO  ← Directive Type │
│ CDW13: DSM (Access Freq/Lat/Seq hints)      │
│ CDW14-15: Reserved                          │
└─────────────────────────────────────────────┘
```

### 활용 가능한 경로

1. **Directive Type (DTYPE in CDW12)**: NVMe Streams Directive로 GPU stream / CPU stream 분리 가능
2. **CDW13 Dataset Management**: Access Frequency, Access Latency, Sequential Request 힌트. GPU 워크로드 특성(random, latency-tolerant)을 encoding 가능
3. **Metadata Pointer (CDW4-5)**: Vendor-specific metadata에 originator 정보 포함 (US10,884,920)
4. **PRP/SGL 주소 (CDW6-9)**: DMA 주소가 GPU BAR 영역이면 → GPU IO로 추론 가능. SSD 컨트롤러가 PCIe TLP의 requester ID로도 구분 가능

## NVIDIA SCADA에서의 실질적 구분 방식

```
┌──────────┐         ┌──────────────┐        ┌─────────┐
│   GPU    │         │  PCIe Switch │        │  NVMe   │
│          │         │              │        │  SSD    │
│ NVMe     │────────>│   Requester  │───────>│         │
│ Driver   │  TLP    │   ID = GPU   │  TLP   │ 컨트롤러│
│ (in GPU) │         │   BDF        │        │         │
└──────────┘         └──────────────┘        └─────────┘

┌──────────┐         ┌──────────────┐        ┌─────────┐
│   CPU    │         │  PCIe Switch │        │  NVMe   │
│          │         │              │        │  SSD    │
│ NVMe     │────────>│   Requester  │───────>│         │
│ Driver   │  TLP    │   ID = CPU   │  TLP   │ 컨트롤러│
│ (kernel) │         │   BDF        │        │         │
└──────────┘         └──────────────┘        └─────────┘
```

SSD 컨트롤러가 구분할 수 있는 방법:
1. **PCIe TLP의 Requester ID (BDF)** → GPU vs CPU 물리적 구분
2. **NVMe Queue 번호** → GPU용 SQ/CQ와 CPU용 SQ/CQ 분리
3. **NVMe Namespace** → GPU 전용 namespace 할당
4. **Command metadata** → host-side 드라이버가 hint 삽입

## 특허 요약 비교

| 특허 | 핵심 아이디어 | GPU/CPU 구분 방식 |
|------|-------------|------------------|
| **US20250045094** (NVIDIA, 2025) | GPU가 직접 스토리지 접근 시작 | GPU 자체가 NVMe driver 역할 → SSD는 별도 큐로 구분 |
| **US10,884,920** (2021) | NVMe metadata로 hint 전달 | metadata field에 originator 정보 encoding |
| **US20220404999** (2022) | NVMe vendor-specific metadata 로깅 | telemetry로 workload type 기록 |
| **CN210776403U** (중국) | GPUDirect 호환 HW 토폴로지 | PCIe 경로 자체가 물리적으로 분리 |

## 업계 방향

NVMe 표준에 "이 IO는 GPU에서 왔다"는 명시적 필드를 추가하기보다는, **SCADA처럼 GPU가 자체 NVMe 큐를 소유**하게 하여 SSD 컨트롤러가 큐 단위로 자연스럽게 구분하는 방향으로 진행되고 있다.

## 참고 자료

- [NVIDIA SCADA offloads storage control path to GPU](https://blocksandfiles.com/2025/11/25/scada-nvidia/)
- [Marvell - GPU-initiated and CPU-initiated Storage](https://www.marvell.com/blogs/the-next-step-for-ai-storage-gpu-initiated-cpu-initiated-storage.html)
- [NVIDIA GPUDirect Storage Design Guide](https://docs.nvidia.com/gpudirect-storage/design-guide/index.html)
- [BaM: GPU-Initiated On-Demand Storage Access (arXiv)](https://arxiv.org/abs/2203.04910)
