# GPU Architecture Deep Dive — JAX Scaling Book 정리

> 출처: https://jax-ml.github.io/scaling-book/gpus

## 1. GPU 기본 구조

GPU는 행렬 곱셈에 특화된 수백 개의 Streaming Multiprocessor(SM)와 고속 메모리(HBM)로 구성된다.

| GPU | SM 수 |
|-----|-------|
| H100 | 132 |
| B200 | 148 |

### SM 내부 구조

각 SM은 4개의 동일한 subpartition으로 나뉘며, 각 subpartition은 다음을 포함한다:

- **Tensor Core** 1개 (행렬 곱셈 담당)
- **레지스터** 16,384개 (32-bit, 즉 64KB/subpartition)
- **Warp Scheduler** (SIMD/SIMT 벡터 연산 스케줄링)
- **CUDA Cores** (32개 fp32, 더 적은 수의 int32/fp64)

## 2. 메모리 계층 구조

### H100 메모리 계층

| 메모리 종류 | SM당 용량 | 총 용량 | 대역폭 | 특징 |
|------------|----------|--------|--------|------|
| 레지스터 | 256KB | 32MB | 가장 빠름 | 스레드당 할당, 프로그래머 제어 불가 |
| SMEM (L1) | 256KB | 32MB | 매우 빠름 | 프로그래머가 직접 관리 가능 |
| L2 캐시 | — | ~50MB | ~5.5TB/s | SM 간 공유 |
| HBM | — | 80GB | 3.35TB/s | 메인 메모리 |

### B200 추가 사항

- **TMEM (Tensor Memory)**: SM당 256KB, Tensor Core 전용 메모리
- **HBM**: 192GB로 대폭 증가

## 3. 연산 유닛

### CUDA Cores (벡터 연산)

SM당 4 subpartitions × 32 fp32 cores = 128 CUDA cores/SM

| GPU | 총 fp32 CUDA Cores | Boost Clock | Vector FLOPs (fp32) |
|-----|-------------------|-------------|---------------------|
| H100 | 132 × 128 = 16,896 | 1.98 GHz | 33.5 TFLOPS |
| B200 | 148 × 128 = 18,944 | — | — |

### Tensor Cores (행렬 곱셈)

각 subpartition에 Tensor Core 1개 → SM당 4개

세대별 Tensor Core 성능 (FLOPs/TC/cycle):

| 세대 | GPU | FLOPs/TC/cycle |
|------|-----|---------------|
| Volta | V100 | 256 |
| Ampere | A100 | 512 |
| Hopper | H100 | 1,024 |
| Blackwell | B200 | ~2,048 (추정) |

### 총 연산 성능 비교

| GPU | bf16/fp16 (TFLOPS) | fp8/int8 (TFLOPS) | fp4 (TFLOPS) |
|-----|-------------------|-------------------|-------------|
| H100 | 990 | 2,000 | — |
| B200 | 2,250 | 4,500 | 9,000 |

## 4. Arithmetic Intensity (연산 강도)

Arithmetic Intensity = matmul FLOPs/s ÷ memory bandwidth

이 값보다 큰 배치 크기를 사용해야 compute-bound(GPU 연산 유닛이 병목)가 된다.

| GPU | 데이터 타입 | Intensity (FLOPs/byte) |
|-----|-----------|----------------------|
| H100 | fp16 | 990 / 3.35 = **295** |
| H100 | fp8 | 2000 / 3.35 = **590** |
| B200 | fp16 | 2250 / 8.0 = **281** |
| B200 | fp8 | 4500 / 8.0 = **562** |

**의미**: 배치 크기가 ~280–300 이상이어야 compute-bound를 달성할 수 있다.

## 5. SIMT 프로그래밍 모델

### SIMT vs SIMD

| 항목 | SIMT (GPU) | SIMD (TPU) |
|------|-----------|-----------|
| 명령 포인터 | 스레드별 독립 | 공유 |
| 분기 처리 | warp divergence 허용 | 불가 |
| 장점 | 유연한 프로그래밍 | 단순/효율적 |
| 단점 | divergence 시 성능 저하 | 유연성 부족 |

### Warp와 스레드

- SM당 최대 **64개 resident warps**
- 각 warp = **32개 스레드**
- Warp scheduler가 메모리 로드 지연을 다른 warp 실행으로 숨김 (latency hiding)

## 6. 네트워킹 계층

### Intra-Node: NVLink

| 항목 | H100 | B200 | GB200 NVL72 |
|------|------|------|-------------|
| NVLink 버전 | 4.0 | 5.0 | 5.0 |
| Link당 대역폭 (full-duplex) | 25 GB/s | 50 GB/s | 50 GB/s |
| GPU당 Link 수 | 18 | 18 | 18 |
| GPU egress 대역폭 | 450 GB/s | 900 GB/s | 900 GB/s |
| NVSwitch 수/노드 | 4 | 4 | — |
| GPU 수/노드 | 8 | 8 | 72 |
| 노드 bisection BW | 3.6 TB/s | 3.6 TB/s | — |

GB200 NVL72는 72개 GPU가 하나의 NVLink 도메인을 형성한다.

### Inter-Node: InfiniBand

H100 SuperPod (1024 GPU) Fat Tree 구조:

| 레벨 | GPU 수 | Switch 수 | Link BW |
|------|--------|----------|---------|
| Node | 8 | 4 NVSwitch | 3.6 TB/s |
| Leaf (SU) | 256 | 8 IB switches | 12.8 TB/s |
| Spine | 1024 | 16 IB switches | 51.2 TB/s |

GPU당 effective collective bandwidth: **~400 GB/s** (노드 외부)

Fat Tree 토폴로지로 모든 레벨에서 bisection bandwidth를 보장한다.

## 7. Collective 연산 성능

### AllGather / ReduceScatter (Intra-node)

```
T_comms = B × (N-1) / (N × W_egress) ≈ B / W_egress
```

H100 (450 GB/s 기준): bf16[1024, 16384] AllGather ≈ **75 μs**

### AllToAll

```
T_AllToAll = B × (N-1) / (W × N²) ≈ B / (W × N)
```

Sparse AllToAll (k/N non-zero): 대역폭이 k/N으로 감소

### In-Network Reductions (SHARP)

이론적 AllReduce 개선: 2배. 실측: **~30% 개선**

```
T_SHARP_AR = B / W_egress
```

### Cross-node Collectives

```
T = B / W_node_egress = B / 400e9   (H100 기준)
```

Ring 기반:

```
T = B × max_i [ (D_i - 1) / (D_i × W_link_i) ]
```

## 8. GPU vs TPU 비교

| 항목 | GPU (H100) | TPU (v5p) |
|------|-----------|----------|
| 기본 단위 | SM (132개) | Tensor Core (2개) |
| Vector ALU | CUDA Core (528 SIMD units) | VPU (8 slots) |
| 총 ALU | 16,896 | 8,192 |
| L1/VMEM | 32 MB | 128 MB |
| 컴파일러 의존도 | 낮음 | 높음 |

**GPU 장점**: 모듈식, 유연함, 새 워크로드에 쉽게 적용
**GPU 단점**: L2 캐시가 느림, 메모리 최적화 어려움
**TPU 장점**: 더 많은 빠른 메모리(VMEM), 일관된 성능
**TPU 단점**: 컴파일러 의존도 높음

## 9. LLM 훈련 Roofline 분석

### Data Parallelism

Compute-bound 조건:

```
B / X > C / W_collective
```

- 노드 내: 배치 크기 > **2,200 tokens/GPU**
- SU/Spine 레벨: 배치 크기 > **2,475 tokens/GPU**
- MoE 모델: E/k배 증가 (E = expert 수, k = activated experts)

### Tensor Parallelism

```
Y < F × W_collective / C
```

- H100: ~F/2200 (대략 **8-way** intra-node)
- 2-node span 시 16-way 가능

### Expert Parallelism (AllToAll 비용, multi-node)

```
T = 2 × B × D × (Z-8)/Z × min(8k/Z, 1)
```

### Pipeline Parallelism

```
T_per-layer ≈ 1.5 × 2BD / (W × N_layers)
```

장점: 거의 무료 수준의 통신 비용
단점: 코드 복잡도 증가, FSDP와의 상충

## 10. 실제 훈련 사례

### DeepSeek V3 (2,048 H800 GPU)

- 64-way Expert Parallelism (8노드 span)
- 16-way Pipeline Parallelism
- 2-way ZeRO-1 Data Parallelism
- 배치 크기: 4M tokens (30k tokens/GPU)
- NVLink 대역폭: 300 GB/s (H800은 H100보다 낮음)

### LLaMA-3 (16,384 H100 GPU)

- 8-way Tensor Parallelism (단일 노드)
- 16-way Pipeline Parallelism
- 128-way ZeRO-1 Data Parallelism
- 배치 크기: 16M tokens (1k tokens/GPU)

## 11. 실측 성능

### AllReduce

- 이론: 450 GB/s (H100)
- 실측: **~370 GB/s**
- 10 GB 메시지에서도 peak 도달 어려움

### AllGather

- GPU: 작은 메시지에서 빠르게 대역폭 증가
- TPU: 더 작은 메시지에서 peak 달성

## 12. GB200 NVL72 특수성

| 메트릭 | H100 8-way | B200 8-way | GB200 NVL72 |
|--------|-----------|-----------|-------------|
| GPU/node | 8 | 8 | 72 |
| GPU egress | 450 GB/s | 900 GB/s | 900 GB/s |
| Node egress | 400 GB/s | 400 GB/s | 3,600 GB/s |

### Grace Hopper (GH200/GB200)

- 1–2 GPU + 1 Grace CPU 조합
- NVLink C2C: 전체 NVLink 대역폭으로 CPU-GPU 연결
- Host RAM offloading 이점

## 13. 핵심 공식 모음

| 공식 | 의미 |
|------|------|
| `T_math = 2 × B × D × F / C` | Matmul 실행 시간 |
| `T_comms = bytes / W` | 통신 시간 |
| `I = C / W` | Critical Intensity (compute-bound 임계값) |
| `T_ring = B × (N-1) / (N × W)` | Ring Reduction 비용 |
| `BW_tree = min_i(D_i × W_link_i / (D_i - 1))` | Tree Reduction 대역폭 |

### 변수 정의

| 변수 | 의미 |
|------|------|
| B | 배치 크기 (tokens) |
| D | 임베딩/모델 차원 |
| F | Feed-forward 차원 |
| C | FLOPs/s (연산 용량) |
| W | 대역폭 (bytes/s) |
| N | Parallel degree |
| Z | Expert 수 |
| k | Activated experts |
