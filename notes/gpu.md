# GPU-Storage I/O 논문 리딩 리스트 (총 41편)

GPU-Storage I/O 관련 논문을 6개 분류로 체계적으로 정리.
각 논문의 상세 노트는 `notes/papers/` 하위 디렉토리에 개별 파일로 관리.

---

## 분류 개요

| 분류 | 디렉토리 | 편수 | 설명 |
|------|----------|------|------|
| P | `papers/P-prereq/` | 7편 | 선행 연구 (2014~2021) — GPU-Storage 접근의 기초 |
| C | `papers/C-cpu-io/` | 4편 | CPU Initiated I/O — 현재 검증 환경의 기반 |
| G | `papers/G-gds/` | 8편 | GPUDirect Storage I/O — CPU 발행 + P2P DMA |
| I | `papers/I-gpu-initiated/` | 9편 | GPU Initiated I/O — 핵심 분류, BaM 계보 |
| O | `papers/O-offload/` | 8편 | GPU-NVMe 오프로드 — AI 워크로드 I/O 패턴 |
| N | `papers/N-industry/` | 5편 | 산업 동향 및 표준 |

---

## 추천 읽기 순서

### 1단계 — 기초 이해 (CPU I/O)
1. [C1] [Storage Stacks 비교](papers/C-cpu-io/C1-storage-stacks.md) — POSIX/libaio/io_uring/SPDK 비교
2. [C2] [Modern NVMe](papers/C-cpu-io/C2-modern-nvme.md) — NVMe의 DBMS 워크로드 성능
3. [C4] [120M IOPS](papers/C-cpu-io/C4-120m-iops.md) — CPU I/O의 현재 최고 성능

### 2단계 — 중간 단계 (GDS)
4. [G1] [GDS 성능 측정](papers/G-gds/G1-gds-perf.md) — GDS 최초 학술 벤치마크
5. [G2] [SPIN](papers/G-gds/G2-spin.md) — P2P DMA의 OS 통합 이슈
6. [O1] [SSDTrain](papers/O-offload/O1-ssdtrain.md) — AI 워크로드의 GDS 활용

### 3단계 — 핵심 (GPU Initiated I/O) ★
7. **[I1] [BaM](papers/I-gpu-initiated/I1-bam.md)** — 원조 논문 ★필독
8. **[I2] [GIDS](papers/I-gpu-initiated/I2-gids.md)** — BaM의 GNN 적용 ★필독
9. **[I3] [AGILE](papers/I-gpu-initiated/I3-agile.md)** — BaM의 비동기 발전 ★필독
10. [I4] [GoFS](papers/I-gpu-initiated/I4-gofs.md) — GPU 파일시스템
11. [I5] [GeminiFS](papers/I-gpu-initiated/I5-geminifs.md) — GPU companion FS

### 4단계 — 큰 그림
12. [I7] [GPU-Initiated Survey](papers/I-gpu-initiated/I7-gpu-initiated-survey.md) — 전체 조감도
13. [N1] [SNIA NVMe-GPU](papers/N-industry/N1-snia-nvme-gpu.md) — NVMe 스펙 관점
14. [N4] [NVIDIA SCADA](papers/N-industry/N4-nvidia-scada.md) — Production 전환

---

## 전체 논문 인덱스

### P — 선행 연구 (7편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| P1 | [GPUfs](papers/P-prereq/P1-gpufs.md) | ASPLOS 2014 | GPU에서 파일 API를 제공하는 최초 연구 |
| P2 | [NVMMU](papers/P-prereq/P2-nvmmu.md) | PACT 2015 | GPU-SSD 직접 접근을 위한 HW 메모리 관리 유닛 |
| P3 | [FlashGPU](papers/P-prereq/P3-flashgpu.md) | DAC 2019 | GPU 코어 옆에 플래시를 직접 배치하는 아키텍처 |
| P4 | [EMOGI](papers/P-prereq/P4-emogi.md) | VLDB 2021 | GPU에서 호스트 메모리 zero-copy 접근으로 대규모 그래프 처리 |
| P5 | [FlashNeuron](papers/P-prereq/P5-flashneuron.md) | FAST 2021 | NVMe SSD를 DNN 학습 백업 스토어로 활용 |
| P6 | [ZeRO-Infinity](papers/P-prereq/P6-zero-infinity.md) | SC 2021 | GPU/CPU/NVMe 통합 이기종 오프로드, DeepSpeed |
| P7 | [SPDK](papers/P-prereq/P7-spdk.md) | CloudCom 2017 | 유저스페이스 NVMe 드라이버 설계 철학 |

### C — CPU Initiated I/O (4편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| C1 | [Storage Stacks 비교](papers/C-cpu-io/C1-storage-stacks.md) | CHEOPS 2023 | POSIX/libaio/io_uring/SPDK CPU 효율성 비교. SPDK가 13배 효율적 |
| C2 | [Modern NVMe](papers/C-cpu-io/C2-modern-nvme.md) | VLDB 2023 | NVMe DBMS 워크로드에서 CPU 코어 절반이 I/O에 소모 |
| C3 | [I/O Passthru](papers/C-cpu-io/C3-io-passthru.md) | FAST 2024 | Linux 커널 NVMe I/O 경로 최적화 |
| C4 | [120M IOPS](papers/C-cpu-io/C4-120m-iops.md) | SPDK Blog 2023 | CPU Initiated I/O의 현재 최고 성능 수준 |

### G — GPUDirect Storage I/O (8편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| G1 | [GDS 성능 측정](papers/G-gds/G1-gds-perf.md) | IEEE NAS 2022 | GDS 성능을 정량적으로 최초 측정 |
| G2 | [SPIN](papers/G-gds/G2-spin.md) | ATC 2023 | P2P DMA를 OS 파일 I/O 스택에 통합 |
| G3 | [GDS + 분산 FS](papers/G-gds/G3-gds-distributed-fs.md) | CHEOPS 2025 | GDS를 Lustre 등 분산 파일시스템과 결합 |
| G4 | [ESPN](papers/G-gds/G4-espn.md) | J. Supercomputing 2025 | GDS 활용 GPU 중심 정보 검색, 쿼리 latency 3.9배 감소 |
| G5 | [NIXL](papers/G-gds/G5-nixl.md) | 2025 | GDS와 NVIDIA NIXL 비교 벤치마크 |
| G6 | [Micron GDS](papers/G-gds/G6-micron-gds.md) | Micron 백서 2023 | Micron 9400 SSD의 GDS 성능 실측 |
| G7 | [Solidigm/NVIDIA](papers/G-gds/G7-solidigm-nvidia.md) | Solidigm/NVIDIA 2025 | GDS + NVMe-oF + DPU production 아키텍처 |
| G8 | [KIOXIA GDS](papers/G-gds/G8-kioxia-gds.md) | KIOXIA 백서 2023~2024 | KIOXIA SSD의 GDS 성능 데이터 |

### I — GPU Initiated I/O (9편) ★핵심 분류

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| **I1** | [**BaM**](papers/I-gpu-initiated/I1-bam.md) | **ASPLOS 2023** | **GPU가 NVMe SQ에 직접 커맨드 발행. 원조 논문 ★필독** |
| **I2** | [**GIDS**](papers/I-gpu-initiated/I2-gids.md) | **VLDB 2024** | **BaM을 GNN 학습에 적용, DGL 대비 582배 가속 ★필독** |
| **I3** | [**AGILE**](papers/I-gpu-initiated/I3-agile.md) | **SC 2025** | **BaM의 동기식 한계를 극복하는 비동기 GPU-SSD 통합 ★필독** |
| I4 | [GoFS](papers/I-gpu-initiated/I4-gofs.md) | SOSP 2025 | GPU Initiated I/O 위에 파일시스템 추상화 제공 |
| I5 | [GeminiFS](papers/I-gpu-initiated/I5-geminifs.md) | FAST 2025 | GPU companion 파일시스템, 호스트 FS와 공존 |
| I6 | [Phoenix](papers/I-gpu-initiated/I6-phoenix.md) | SC 2025 | GDS의 phony buffer 문제 해결, 리팩토링된 I/O 스택 |
| I7 | [GPU-Initiated Survey](papers/I-gpu-initiated/I7-gpu-initiated-survey.md) | DaMoN 2025 | Samsung의 GPU Initiated I/O 서베이 및 적용 경로 분석 |
| I8 | [CXL-BaM](papers/I-gpu-initiated/I8-cxl-bam.md) | 2024 | BaM을 CXL 기반 µs급 외부 메모리로 확장 |
| I9 | [GMT](papers/I-gpu-initiated/I9-gmt.md) | ASPLOS 2024 | GPU가 3계층 메모리(VRAM/DRAM/SSD)를 직접 오케스트레이션 |

### O — GPU-NVMe 오프로드 (8편)

| ID | 논문명 | 학회/연도 | 핵심 |
|----|--------|-----------|------|
| O1 | [SSDTrain](papers/O-offload/O1-ssdtrain.md) | arXiv 2024 | LLM 학습 시 activation을 NVMe SSD로 오프로드 |
| O2 | [TERAIO](papers/O-offload/O2-teraio.md) | NeurIPS 2025 | 텐서 수명 인식 기반 fine-grained SSD 오프로드 |
| O3 | [LLM NVMe 특성 분석](papers/O-offload/O3-llm-nvme-characterization.md) | CHEOPS 2025 | LLM 모델/KV 캐시의 NVMe 오프로드 I/O 특성 프로파일링 |
| O4 | [Fuyou](papers/O-offload/O4-fuyou.md) | arXiv 2024 | 단일 GPU에서 100B 모델 파인튜닝을 NVMe로 구현 |
| O5 | [LoHan](papers/O-offload/O5-lohan.md) | 2024 | DRAM + NVMe 혼합 activation 오프로드 최적화 |
| O6 | [G10](papers/O-offload/O6-g10.md) | MICRO 2023 | GPU 메모리와 스토리지 통합 아키텍처 |
| O7 | [HetCache](papers/O-offload/O7-hetcache.md) | CIDR 2023 | CPU DRAM/GPU VRAM/NVMe SSD 이기종 캐시 전략 |
| O8 | [MLP-Offload](papers/O-offload/O8-mlp-offload.md) | SC 2025 | DeepSpeed ZeRO-3 NVMe 오프로드 다중 레벨/경로 최적화 |

### N — 산업 동향 및 표준 (5편)

| ID | 자료명 | 출처/연도 | 핵심 |
|----|--------|-----------|------|
| N1 | [SNIA NVMe-GPU](papers/N-industry/N1-snia-nvme-gpu.md) | SNIA 2024 | NVMe 스펙이 GPU 접근을 위해 진화해야 하는 이유 |
| N2 | [Marvell GPU Storage](papers/N-industry/N2-marvell-gpu-storage.md) | Marvell 2025 | GPU Initiated I/O가 SSD 설계에 미치는 영향 |
| N3 | [DapuStor BaM](papers/N-industry/N3-dapustor-bam.md) | Blocks and Files 2024 | SSD 벤더가 BaM을 자사 SSD로 시연한 사례 |
| N4 | [NVIDIA SCADA](papers/N-industry/N4-nvidia-scada.md) | NVIDIA 2025 | GPU로 스토리지 제어/데이터 경로 모두 가속 |
| N5 | [GPUs as Data Access Engines](papers/N-industry/N5-nvidia-data-access.md) | FMS 2024 | NVIDIA/Micron 공동 — BaM/GIDS의 Gen5 NVMe SSD 성능 데이터 |

---

## 총 정리

| 분류 | 논문 수 | 비고 |
|------|---------|------|
| 선행 연구 (P) | 7편 | 2014~2021, GPU-Storage의 기초 |
| CPU Initiated I/O (C) | 4편 | 현재 검증 환경 기반 |
| GPUDirect Storage (G) | 8편 | 중간 단계, CPU 발행 + P2P DMA |
| GPU Initiated I/O (I) | 9편 | 핵심 분류, BaM 계보 |
| GPU-NVMe 오프로드 (O) | 8편 | AI 워크로드 I/O 패턴 이해 |
| 산업 동향 (N) | 5편 | 발표 맥락 설정 |
| **합계** | **41편** | |

---

## GPU-NVMe 최적화의 역사: GDS/GPU-Initiated I/O 이전의 노력들

> 상세 문서: [gpu-nvme-optimization-history.md](gpu-nvme-optimization-history.md)

### 원래의 문제

전통적 데이터 경로: `NVMe → 커널 버퍼 → User 버퍼 → cudaMemcpy → GPU`
- 데이터가 CPU 메모리를 2번 거침 (바운스 버퍼)
- CPU 코어 절반 이상이 I/O 처리에 소모 [C2]
- GPU 커널이 데이터 대기 중 멈춰야 함

### 5가지 접근법과 한계

| # | 접근법 | 대표 논문 | 해결한 것 | 남은 한계 |
|---|--------|----------|----------|----------|
| ① | CPU I/O 스택 최적화 | SPDK [P7], io_uring [C1], io_passthru [C3], 120M IOPS [C4] | CPU 측 SW 오버헤드 제거 (코어당 100x IOPS↑) | GPU는 여전히 CPU에 요청하고 대기 |
| ② | GPU 파일 API / 직접 접근 | GPUfs [P1], EMOGI [P4] | GPU 프로그래밍 편의, 호스트 메모리 zero-copy | CPU 병목 (GPUfs), SSD 접근 불가 (EMOGI) |
| ③ | HW 아키텍처 제안 | NVMMU [P2], FlashGPU [P3] | 이상적 GPU-SSD 직접 경로 제시 | 시뮬레이션만, GPU 칩 수정 필요 |
| ④ | 메모리 계층 오프로드 | FlashNeuron [P5], ZeRO-Infinity [P6], G10 [O6], HetCache [O7] | GPU 메모리 용량 확장, 대규모 모델 학습 | CPU 오케스트레이션, 불규칙 접근에 취약 |
| ⑤ | P2P DMA (GDS) | GDS [G1], SPIN [G2] | 바운스 버퍼 제거, throughput 2~6x↑ | CPU가 I/O 발행, fine-grained random 부적합 |

**공통 한계: "CPU가 중간에 끼어있다"** → BaM [I1]이 GPU-Initiated I/O로 돌파

### 발전 흐름 요약

```
GPUfs(2014) → NVMMU(2015) → SPDK(2017) → FlashGPU(2019)
    → EMOGI/FlashNeuron/ZeRO-Infinity(2021)
    → GDS(2022) → io_passthru/SPIN/SPDK 120M(2023)
    → ★ BaM(2023): GPU가 NVMe SQ에 직접 커맨드 발행 (모든 한계 돌파)
```

BaM이 가능했던 이유 = 선행 연구 통찰의 조합:
- GPUfs → GPU 스토리지 접근 개념 / EMOGI → massive parallelism으로 대역폭 활용
- SPDK → 유저스페이스 NVMe 큐 직접 조작 / GDS → P2P DMA 인프라

---

## 핵심 기술 계보

```
[2014] GPUfs (P1) — GPU에서 파일 접근 개념
   ↓
[2015] NVMMU (P2) — GPU-SSD 직접 접근 HW 제안
   ↓
[2019] FlashGPU (P3) — GPU 옆에 플래시 배치
   ↓
[2021] EMOGI (P4) — GPU에서 호스트 메모리 직접 접근
   ↓
[2023] ★ BaM (I1) — GPU가 NVMe에 직접 커맨드 발행 (원조)
   ├── [2024] GIDS (I2) — GNN 워크로드 적용
   ├── [2024] GMT (I9) — 3계층 메모리 오케스트레이션
   ├── [2024] CXL-BaM (I8) — CXL 메모리로 확장
   ├── [2025] AGILE (I3) — 비동기 모델로 발전
   ├── [2025] GoFS (I4) — GPU 파일시스템
   ├── [2025] GeminiFS (I5) — Companion 파일시스템
   └── [2025] NVIDIA SCADA (N4) — Production 전환
```
