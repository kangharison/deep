# [G3] Using GPU Direct Storage with High-Performance Distributed Filesystems

- **학회/연도:** CHEOPS 2025
- **저자:** Hoozemans et al.
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
GDS를 분산 파일시스템(Lustre, GPFS/Spectrum Scale, BeeGFS 등)과 결합했을 때의 성능을 체계적으로 분석. 로컬 NVMe 대비 네트워크 스토리지에서 GDS의 실효성과 병목 지점을 규명.

## 읽기 전 질문
- 분산 파일시스템에서 GDS를 사용하면 로컬 NVMe GDS 대비 얼마나 성능이 나오는가?
- 네트워크(RDMA/TCP) 병목이 GDS의 P2P DMA 이점을 상쇄하지는 않는가?
- 어떤 분산 파일시스템이 GDS와 가장 잘 호환되는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- HPC/AI 클러스터에서 학습 데이터는 보통 분산 파일시스템(Lustre, GPFS 등)에 저장
- GDS는 로컬 NVMe에서 검증되었지만, 분산 파일시스템과의 통합 성능은 미지수
- 분산 FS에서 GDS 사용 시 네트워크 스택이 추가되므로 P2P DMA 이점이 감소할 수 있음
- 실제 HPC 센터에서 GDS 도입을 결정하려면 분산 환경 성능 데이터가 필수

### 2. 제안 방법 (Approach)
- 다양한 분산 파일시스템에서 GDS 성능을 체계적으로 벤치마크
- 비교 축:
  - GDS vs 전통적 경로 (POSIX + cudaMemcpy)
  - 로컬 NVMe vs 네트워크 스토리지
  - 다양한 분산 FS 간 비교 (Lustre, BeeGFS 등)
- NVIDIA cuFile API 기반 벤치마크 도구 사용
- I/O 크기, 동시성, 네트워크 구성 등 파라미터 변화

### 3. 핵심 아키텍처/설계

```
  분산 파일시스템 + GDS 아키텍처:

  ┌──────────────┐     RDMA/TCP      ┌──────────────────┐
  │ Storage Node │◄──────────────────►│   Compute Node   │
  │              │    Network         │                  │
  │ ┌──────────┐ │                    │ ┌──────┐ ┌─────┐ │
  │ │ NVMe SSD │ │                    │ │ GPU  │ │ NIC │ │
  │ └──────────┘ │                    │ └──┬───┘ └──┬──┘ │
  │ ┌──────────┐ │                    │    │  PCIe  │    │
  │ │FS Server │ │                    │    └────┬───┘    │
  │ │(Lustre   │ │                    │         │        │
  │ │ MDS/OSS) │ │                    │  ┌──────▼──────┐ │
  │ └──────────┘ │                    │  │  GDS + cuFile│ │
  └──────────────┘                    │  │  nvidia-fs   │ │
                                      │  └─────────────┘ │
                                      └──────────────────┘

  데이터 흐름 (GDS + 분산 FS):
  Storage NVMe → Network(RDMA) → NIC → [GDS: NIC→GPU P2P] → GPU Memory
                                     또는
                                 → CPU Memory → cudaMemcpy → GPU Memory
```

- GDS의 분산 FS 지원 방식:
  - **GDS compatibility mode**: 분산 FS에서 GDS가 직접 P2P를 못하면 CPU 경유 폴백
  - **Native GDS 지원**: Lustre 등 일부 FS에서 GDS 네이티브 플러그인 제공
  - **GPUDirect RDMA 연계**: NIC에서 GPU 메모리로 직접 RDMA 가능 시 최대 성능

### 4. 실험 결과 (Key Results)
- **로컬 NVMe GDS**: 기존 대비 최대 2배 이상 throughput (baseline 확인)
- **Lustre + GDS**: GDS 네이티브 지원 시 전통적 경로 대비 유의미한 향상 확인
- **네트워크 병목**: 고속 네트워크(InfiniBand HDR 200Gbps 이상)에서만 GDS 이점 발현
- **대용량 I/O**: 1MB 이상 I/O에서 GDS 이점 극대화, 소규모 I/O에서는 네트워크 오버헤드가 지배적
- **Compatibility mode**: 네이티브 GDS 미지원 FS에서는 오히려 오버헤드 발생 가능
- **멀티 GPU 스케일링**: 분산 환경에서도 GPU 수 증가 시 aggregate throughput 향상

### 5. 한계점 및 향후 연구
- 분산 FS별 GDS 네이티브 지원 수준이 다름 — Lustre는 지원, 다른 FS는 제한적
- 네트워크 대역폭이 병목일 때 GDS의 P2P DMA 이점이 상쇄됨
- 메타데이터 연산(open, stat 등)은 GDS와 무관 — 이 부분이 별도 병목 가능
- 실제 AI 학습 파이프라인에서의 E2E 성능 영향은 추가 연구 필요
- GDS compatibility mode의 성능 특성에 대한 더 깊은 분석 필요

## 다른 논문과의 관계
- 선행 연구: GDS [G1] — 로컬 NVMe GDS 성능 baseline
- 후속 연구: 분산 GDS 최적화 연구
- 비교 대상: [G7] Solidigm/NVIDIA — 유사하게 원격 스토리지 GDS 다룸

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: 로컬 vs 분산 GDS throughput 비교, 분산 FS별 성능 비교 표
- 핵심 수치/데이터: Lustre GDS 네이티브 모드에서의 throughput, 네트워크 병목 발생 임계점
- **활용**: "GDS가 분산 환경에서도 유효한가?"라는 현실적 질문에 대한 답변 근거

## 메모
- HPC 센터 도입 관점에서 가장 실용적인 논문. 분산 FS별 GDS 지원 현황 정리에 유용.
