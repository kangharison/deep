# [G1] Quantifying Performance Gains of GPUDirect Storage

- **학회/연도:** IEEE NAS 2022
- **저자:** Inupakutika et al.
- **분류:** GPUDirect Storage I/O

## 핵심 요약 (1~2문장)
GDS의 성능 이점을 정량적으로 측정한 최초의 학술 논문. CPU 바운스 버퍼 경유 경로 대비 GDS의 throughput 향상(최대 2배 이상)과 CPU 사용률 감소를 다양한 워크로드에서 체계적으로 벤치마크.

## 읽기 전 질문
- GDS가 기존 POSIX I/O + cudaMemcpy 대비 실질적으로 얼마나 빠른가?
- GDS의 성능 이점이 극대화되는 I/O 패턴(크기, 큐 깊이)은 무엇인가?
- GDS 사용 시 CPU 사용률은 얼마나 줄어드는가?

## 주요 내용 정리

### 1. 문제 정의 (Problem)
- GPU 컴퓨팅 워크로드(AI/ML, HPC, 데이터 분석)에서 스토리지 데이터를 GPU 메모리로 로드하는 과정이 병목
- 전통적 I/O 경로: SSD → 커널 버퍼(page cache) → 사용자 버퍼 → cudaMemcpy → GPU 메모리
  - **2번의 메모리 복사** + CPU 개입 필수
  - CPU가 데이터 전달에 사용되어 연산에 사용할 CPU 사이클 낭비
- NVIDIA가 GDS(GPUDirect Storage)를 제안했지만, 학술적으로 정량적 성능 분석이 부재

### 2. 제안 방법 (Approach)
- GDS의 cuFile API를 사용하여 다양한 벤치마크 수행
- 비교 대상: 전통적 POSIX read + cudaMemcpy 경로 vs GDS (cuFileRead/cuFileWrite)
- 측정 항목: throughput, latency, CPU 사용률
- 다양한 파라미터 변화: I/O 크기(4KB~수MB), 큐 깊이, 파일 크기, sequential vs random

### 3. 핵심 아키텍처/설계

```
  전통적 경로 (Bounce Buffer):
  ┌──────┐    ┌──────────┐    ┌───────────┐    ┌──────────┐    ┌─────┐
  │ NVMe ├───→│ 커널 버퍼 ├───→│ User 버퍼  ├───→│cudaMemcpy├───→│ GPU │
  │ SSD  │    │(PageCache)│    │  (CPU RAM) │    │          │    │ Mem │
  └──────┘    └──────────┘    └───────────┘    └──────────┘    └─────┘
       DMA         CPU copy        CPU copy         PCIe DMA
       (1회)       (1회)           (1회)             (1회)

  GDS 경로 (Direct Path):
  ┌──────┐                                                     ┌─────┐
  │ NVMe ├────────────── PCIe P2P DMA ────────────────────────→│ GPU │
  │ SSD  │              (CPU 바운스 버퍼 제거)                    │ Mem │
  └──────┘                                                     └─────┘
       DMA (1회, CPU 개입 최소화)
```

- GDS 핵심 구성요소:
  - **cuFile API**: 사용자 공간에서 GPU 메모리와 파일 간 직접 I/O
  - **nvidia-fs 커널 모듈**: NVMe 드라이버와 GPU 드라이버를 연결
  - **BAR1 매핑**: GPU 메모리를 PCIe BAR1 영역에 매핑하여 NVMe DMA 대상으로 설정

### 4. 실험 결과 (Key Results)
- **Sequential Read**: GDS가 전통적 경로 대비 최대 **2배 이상** throughput 향상
- **대용량 I/O (1MB+)**: GDS 이점이 가장 두드러짐 — 바운스 버퍼 복사 오버헤드가 큰 구간
- **소규모 I/O (4KB~64KB)**: GDS 이점이 상대적으로 작음 — cuFile API 오버헤드가 상대적으로 큼
- **CPU 사용률**: GDS 사용 시 CPU 사용률이 현저히 감소 (데이터 복사 CPU 사이클 제거)
- **다중 GPU**: GPU 수 증가 시 GDS의 scalability가 전통적 경로보다 우수
- **Write 성능**: Read와 유사한 패턴으로 GDS가 우위

### 5. 한계점 및 향후 연구
- 단일 노드, 로컬 NVMe SSD만 테스트 — 분산 환경 미검증
- GDS의 compatibility mode (CPU 폴백) 시 오히려 POSIX보다 느릴 수 있음
- 소규모 I/O에서는 cuFile API 오버헤드로 인해 이점 제한적
- 특정 NVIDIA GPU + 특정 NVMe SSD 조합에서만 테스트
- 실제 애플리케이션(AI 학습, 추론) 워크로드에서의 E2E 성능은 미평가

## 다른 논문과의 관계
- 선행 연구: NVIDIA GDS 기술 문서, GPUDirect RDMA
- 후속 연구: [G3] 분산 파일시스템 확장, [G5] NIXL 벤치마킹
- 비교 대상: BaM [I1] (GPU Initiated vs CPU Initiated GDS), SPIN [G2] (OS 통합 P2P)

## 발표 자료 활용 포인트
- 이 논문에서 발표에 인용할 핵심 Figure/Table: GDS vs POSIX throughput 비교 그래프, CPU 사용률 비교 차트
- 핵심 수치/데이터: Sequential read에서 최대 2배 throughput, CPU 사용률 대폭 감소
- **활용**: GDS의 기본 성능 baseline으로 사용, "왜 GDS가 필요한가"의 정량적 근거

## 메모
- GDS 성능 분석의 기초 논문으로, 이후 모든 GDS 관련 연구의 비교 기준점 역할
