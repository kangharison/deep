# GPU-Storage I/O 논문 리딩 리스트 (총 41편)

## 디렉토리 구조

| 분류 | 디렉토리 | 편수 | 설명 |
|------|----------|------|------|
| P | `P-prereq/` | 7편 | 선행 연구 (2014~2021) |
| C | `C-cpu-io/` | 4편 | CPU Initiated I/O |
| G | `G-gds/` | 8편 | GPUDirect Storage I/O |
| I | `I-gpu-initiated/` | 9편 | GPU Initiated I/O (핵심) |
| O | `O-offload/` | 8편 | GPU-NVMe 오프로드 |
| N | `N-industry/` | 5편 | 산업 동향 및 표준 |

---

## 추천 읽기 순서

### 1단계 — 기초 이해 (CPU I/O)
1. [C1] [Storage Stacks 비교](C-cpu-io/C1-storage-stacks.md) — POSIX/libaio/io_uring/SPDK 비교
2. [C2] [Modern NVMe](C-cpu-io/C2-modern-nvme.md) — NVMe의 DBMS 워크로드 성능
3. [C4] [120M IOPS](C-cpu-io/C4-120m-iops.md) — CPU I/O의 현재 최고 성능

### 2단계 — 중간 단계 (GDS)
4. [G1] [GDS 성능 측정](G-gds/G1-gds-perf.md) — GDS 최초 학술 벤치마크
5. [G2] [SPIN](G-gds/G2-spin.md) — P2P DMA의 OS 통합 이슈
6. [O1] [SSDTrain](O-offload/O1-ssdtrain.md) — AI 워크로드의 GDS 활용

### 3단계 — 핵심 (GPU Initiated I/O) ★
7. **[I1] [BaM](I-gpu-initiated/I1-bam.md)** — 원조 논문 ★필독
8. **[I2] [GIDS](I-gpu-initiated/I2-gids.md)** — BaM의 GNN 적용 ★필독
9. **[I3] [AGILE](I-gpu-initiated/I3-agile.md)** — BaM의 비동기 발전 ★필독
10. [I4] [GoFS](I-gpu-initiated/I4-gofs.md) — GPU 파일시스템
11. [I5] [GeminiFS](I-gpu-initiated/I5-geminifs.md) — GPU companion FS

### 4단계 — 큰 그림
12. [I7] [GPU-Initiated Survey](I-gpu-initiated/I7-gpu-initiated-survey.md) — 전체 조감도
13. [N1] [SNIA NVMe-GPU](N-industry/N1-snia-nvme-gpu.md) — NVMe 스펙 관점
14. [N4] [NVIDIA SCADA](N-industry/N4-nvidia-scada.md) — Production 전환

---

## 전체 논문 인덱스

### P — 선행 연구
| ID | 파일 | 논문명 | 학회/연도 |
|----|------|--------|-----------|
| P1 | [P1-gpufs.md](P-prereq/P1-gpufs.md) | GPUfs: Integrating a File System with GPUs | ASPLOS 2014 |
| P2 | [P2-nvmmu.md](P-prereq/P2-nvmmu.md) | NVMMU: Non-volatile Memory Management Unit for GPU-SSD | PACT 2015 |
| P3 | [P3-flashgpu.md](P-prereq/P3-flashgpu.md) | FlashGPU: Placing New Flash Next to GPU Cores | DAC 2019 |
| P4 | [P4-emogi.md](P-prereq/P4-emogi.md) | EMOGI: Efficient Memory-access for Out-of-memory Graph-traversal | VLDB 2021 |
| P5 | [P5-flashneuron.md](P-prereq/P5-flashneuron.md) | FlashNeuron: SSD-Enabled Large-Batch Training | FAST 2021 |
| P6 | [P6-zero-infinity.md](P-prereq/P6-zero-infinity.md) | ZeRO-Infinity: Breaking the GPU Memory Wall | SC 2021 |
| P7 | [P7-spdk.md](P-prereq/P7-spdk.md) | SPDK: High Performance Storage Applications | CloudCom 2017 |

### C — CPU Initiated I/O
| ID | 파일 | 논문명 | 학회/연도 |
|----|------|--------|-----------|
| C1 | [C1-storage-stacks.md](C-cpu-io/C1-storage-stacks.md) | Performance Characterization of Modern Storage Stacks | CHEOPS 2023 |
| C2 | [C2-modern-nvme.md](C-cpu-io/C2-modern-nvme.md) | What Modern NVMe Storage Can Do | VLDB 2023 |
| C3 | [C3-io-passthru.md](C-cpu-io/C3-io-passthru.md) | I/O Passthru: Flexible and Efficient I/O Path in Linux | FAST 2024 |
| C4 | [C4-120m-iops.md](C-cpu-io/C4-120m-iops.md) | 120 Million IOPS with Standard 2U Intel Xeon | SPDK Blog 2023 |

### G — GPUDirect Storage
| ID | 파일 | 논문명 | 학회/연도 |
|----|------|--------|-----------|
| G1 | [G1-gds-perf.md](G-gds/G1-gds-perf.md) | Quantifying Performance Gains of GPUDirect Storage | IEEE NAS 2022 |
| G2 | [G2-spin.md](G-gds/G2-spin.md) | SPIN: P2P DMA Between SSDs and GPUs | ATC 2023 |
| G3 | [G3-gds-distributed-fs.md](G-gds/G3-gds-distributed-fs.md) | GDS with High-Performance Distributed Filesystems | CHEOPS 2025 |
| G4 | [G4-espn.md](G-gds/G4-espn.md) | ESPN: GPU-centric Information Retrieval | J. Supercomputing 2025 |
| G5 | [G5-nixl.md](G-gds/G5-nixl.md) | GPUDirect Data Transfer through NIXL Benchmarking | 2025 |
| G6 | [G6-micron-gds.md](G-gds/G6-micron-gds.md) | Micron 9400 with GPUDirect Storage | Micron 백서 2023 |
| G7 | [G7-solidigm-nvidia.md](G-gds/G7-solidigm-nvidia.md) | Accelerating AI With High Performance Storage | Solidigm/NVIDIA 2025 |
| G8 | [G8-kioxia-gds.md](G-gds/G8-kioxia-gds.md) | KIOXIA GPUDirect Storage Performance Report | KIOXIA 백서 2023~2024 |

### I — GPU Initiated I/O
| ID | 파일 | 논문명 | 학회/연도 |
|----|------|--------|-----------|
| **I1** | [**I1-bam.md**](I-gpu-initiated/I1-bam.md) | **BaM: GPU-Initiated Storage Access** | **ASPLOS 2023** |
| **I2** | [**I2-gids.md**](I-gpu-initiated/I2-gids.md) | **GIDS: GNN with GPU Initiated Direct Storage** | **VLDB 2024** |
| **I3** | [**I3-agile.md**](I-gpu-initiated/I3-agile.md) | **AGILE: Asynchronous GPU-SSD Integration** | **SC 2025** |
| I4 | [I4-gofs.md](I-gpu-initiated/I4-gofs.md) | GoFS: Scalable Direct Storage for GPUs | SOSP 2025 |
| I5 | [I5-geminifs.md](I-gpu-initiated/I5-geminifs.md) | GeminiFS: Companion File System for GPUs | FAST 2025 |
| I6 | [I6-phoenix.md](I-gpu-initiated/I6-phoenix.md) | Phoenix: Refactored I/O Stack for GPU Direct Storage | SC 2025 |
| I7 | [I7-gpu-initiated-survey.md](I-gpu-initiated/I7-gpu-initiated-survey.md) | Path to GPU-Initiated I/O (Samsung) | DaMoN 2025 |
| I8 | [I8-cxl-bam.md](I-gpu-initiated/I8-cxl-bam.md) | GPU Graph Processing on CXL-Based External Memory | 2024 |
| I9 | [I9-gmt.md](I-gpu-initiated/I9-gmt.md) | GMT: GPU Orchestrated Memory Tiering | ASPLOS 2024 |

### O — GPU-NVMe 오프로드
| ID | 파일 | 논문명 | 학회/연도 |
|----|------|--------|-----------|
| O1 | [O1-ssdtrain.md](O-offload/O1-ssdtrain.md) | SSDTrain: Activation Offloading for LLM Training | arXiv 2024 |
| O2 | [O2-teraio.md](O-offload/O2-teraio.md) | TERAIO: Lifetime-Aware Tensor Offloading | NeurIPS 2025 |
| O3 | [O3-llm-nvme-characterization.md](O-offload/O3-llm-nvme-characterization.md) | I/O Characterizing of LLM NVMe Offload | CHEOPS 2025 |
| O4 | [O4-fuyou.md](O-offload/O4-fuyou.md) | Fuyou: 100B Model Fine-tuning on Single GPU | arXiv 2024 |
| O5 | [O5-lohan.md](O-offload/O5-lohan.md) | LoHan: Low-Cost LLM Fine-tuning | 2024 |
| O6 | [O6-g10.md](O-offload/O6-g10.md) | G10: Unified GPU Memory and Storage | MICRO 2023 |
| O7 | [O7-hetcache.md](O-offload/O7-hetcache.md) | HetCache: NVMe Storage and GPU Acceleration | CIDR 2023 |
| O8 | [O8-mlp-offload.md](O-offload/O8-mlp-offload.md) | MLP-Offload: Multi-Level Offloading for LLM | SC 2025 |

### N — 산업 동향
| ID | 파일 | 논문명 | 출처/연도 |
|----|------|--------|-----------|
| N1 | [N1-snia-nvme-gpu.md](N-industry/N1-snia-nvme-gpu.md) | NVMe Evolution for GPU Storage Access | SNIA 2024 |
| N2 | [N2-marvell-gpu-storage.md](N-industry/N2-marvell-gpu-storage.md) | Next Step for AI Storage | Marvell 2025 |
| N3 | [N3-dapustor-bam.md](N-industry/N3-dapustor-bam.md) | DapuStor BaM Demo | Blocks and Files 2024 |
| N4 | [N4-nvidia-scada.md](N-industry/N4-nvidia-scada.md) | NVIDIA SCADA | NVIDIA 2025 |
| N5 | [N5-nvidia-data-access.md](N-industry/N5-nvidia-data-access.md) | GPUs as Data Access Engines | FMS 2024 |
