# GPGPU-Sim 벤치마크 시나리오 및 설정 가이드

## 1. GPGPU-Sim에서 벤치마크를 실행하는 방법

### 1.1 기본 실행 원리

GPGPU-Sim은 CUDA 런타임 라이브러리(`libcudart.so`)를 대체하는 방식으로 동작한다. 즉, CUDA 애플리케이션을 재컴파일하지 않고도 시뮬레이터 위에서 실행할 수 있다. `LD_LIBRARY_PATH`를 GPGPU-Sim의 라이브러리로 변경하면, 모든 CUDA API 호출이 시뮬레이터로 리다이렉트된다.

```
실행 흐름:
  CUDA 애플리케이션 (바이너리)
         │
         ▼
  libcudart_gpgpu-sim.so (GPGPU-Sim 런타임)
         │
         ├── cudaMemcpy() → 시뮬레이터 메모리에 데이터 복사
         ├── cudaLaunch() → 커널을 시뮬레이터에 등록
         └── GPU 커널 실행 → gpgpu_sim::cycle() 루프
                │
                ├── PTX 기반 시뮬레이션 (기본)
                │   → CUDA 바이너리에서 PTX 추출
                │   → PTX 명령어를 해석하며 사이클 시뮬레이션
                │
                └── SASS 기반 시뮬레이션 (Accel-Sim 확장)
                    → NVBit으로 SASS 트레이스 생성
                    → 트레이스 기반으로 사이클 시뮬레이션
```

### 1.2 실행 단계별 절차

#### Step 1: 환경 설정

```bash
# CUDA 툴킷 경로 설정
export CUDA_INSTALL_PATH=/usr/local/cuda

# GPGPU-Sim 소스 디렉토리로 이동
cd ~/sources/gpgpu-sim_distribution

# 빌드 환경 설정 (release 또는 debug)
source setup_environment release

# 빌드
make -j$(nproc)
```

#### Step 2: CUDA 애플리케이션 컴파일

```bash
# 동적 링크로 컴파일 (중요: -lcudart 필수)
nvcc -o my_app my_app.cu -lcudart

# 링크 확인: libcudart.so가 GPGPU-Sim 디렉토리를 가리키는지 확인
ldd my_app | grep cudart
# 출력 예: libcudart.so => /path/to/gpgpu-sim_distribution/lib/.../libcudart.so
```

#### Step 3: 설정 파일 복사 및 실행

```bash
# 작업 디렉토리 생성
mkdir -p ~/sim_run/my_benchmark && cd ~/sim_run/my_benchmark

# GPU 설정 파일 복사 (QV100 예시)
cp ~/sources/gpgpu-sim_distribution/configs/tested-cfgs/SM7_QV100/* .

# 시뮬레이션 실행 (source setup_environment가 되어 있어야 함)
./my_app [arguments]
```

#### Step 4: PTX 덤프 최적화 (대규모 애플리케이션용)

대규모 애플리케이션은 매 실행마다 PTX를 추출하는 데 시간이 걸린다. 이를 최적화하려면:

```bash
# 첫 실행: PTX 파일 저장
# gpgpusim.config에 -save_embedded_ptx 1 추가 후 실행
./my_app

# _cuobjdump_complete_output_* 과 _1.ptx 파일이 생성됨

# 이후 실행: 환경변수 설정으로 PTX 재추출 건너뛰기
export PTX_SIM_USE_PTX_FILE=_1.ptx
export PTX_SIM_KERNELFILE=_1.ptx
export CUOBJDUMP_SIM_FILE=_cuobjdump_complete_output_*

# -save_embedded_ptx 0으로 변경 후 재실행
./my_app
```

### 1.3 Accel-Sim을 통한 SASS 트레이스 기반 실행

GPGPU-Sim 4.0부터 Accel-Sim 프레임워크와 연동하여 NVIDIA SASS 트레이스를 재생할 수 있다. 이 방식이 실제 하드웨어와 더 높은 상관관계를 보인다.

```bash
# Accel-Sim 프레임워크 클론
git clone https://github.com/accel-sim/accel-sim-framework.git
cd accel-sim-framework

# Accel-Sim 빌드
source ./gpu-simulator/setup_environment.sh
make -j -C ./gpu-simulator

# 사전 생성된 Rodinia 트레이스 다운로드 (Tesla V100)
wget https://engineering.purdue.edu/tgrogers/accel-sim/traces/tesla-v100/latest/rodinia_2.0-ft.tgz
mkdir -p ./hw_run
tar -xzvf rodinia_2.0-ft.tgz -C ./hw_run

# SASS 트레이스 기반 시뮬레이션 실행
./util/job_launching/run_simulations.py \
    -C QV100-SASS \
    -B rodinia_2.0-ft \
    -T ./hw_run/rodinia_2.0-ft/9.1 \
    -N myTest

# 결과 모니터링
./util/job_launching/monitor_func_test.py -v -N myTest
```


## 2. 설정 파일(gpgpusim.config) 상세 해석 (SM7_QV100 기준)

SM7_QV100 설정 파일은 NVIDIA Quadro V100 (Volta 아키텍처)를 모델링한다.

### 2.1 기능 시뮬레이터 설정

```
-gpgpu_ptx_instruction_classification 0   # PTX 명령어 분류 비활성
-gpgpu_ptx_sim_mode 0                     # 0=성능 시뮬레이션, 1=기능만 시뮬레이션
-gpgpu_ptx_force_max_capability 70        # SM 7.0 (Volta) 강제
```

`ptx_sim_mode`를 1로 설정하면 타이밍 정보 없이 기능적으로만 실행하므로 결과 검증에 유용하다. 0으로 설정하면 사이클 정확 시뮬레이션을 수행한다.

### 2.2 디바이스 한계 설정

```
-gpgpu_stack_size_limit 1024              # 스레드당 스택 크기 (바이트)
-gpgpu_heap_size_limit 8388608            # 디바이스 힙 크기 (8MB)
-gpgpu_runtime_sync_depth_limit 2         # 동적 병렬성 동기화 깊이
-gpgpu_runtime_pending_launch_count_limit 2048  # 대기중 커널 런치 수
-gpgpu_kernel_launch_latency 5000         # 커널 런치 레이턴시 (사이클)
-gpgpu_TB_launch_latency 0               # Thread Block 런치 레이턴시
-gpgpu_max_concurrent_kernel 128          # 최대 동시 커널 수
```

### 2.3 고수준 아키텍처 설정

```
-gpgpu_n_clusters 80                      # SM 클러스터 수 (QV100 = 80 SM)
-gpgpu_n_cores_per_cluster 1              # 클러스터당 코어 수
-gpgpu_n_mem 32                           # 메모리 파티션 수 (HBM2 32채널)
-gpgpu_n_sub_partition_per_mchannel 2     # 채널당 서브파티션 (L2 뱅크)
                                          # 총 서브파티션 = 32 x 2 = 64
```

이 설정은 QV100의 물리적 구조를 반영한다: 80개 SM, 32개 HBM2 메모리 채널, 64개 L2 캐시 뱅크.

### 2.4 클록 도메인

```
-gpgpu_clock_domains 1132.0:1132.0:1132.0:850.0
# 순서: Core:Interconnect:L2:DRAM (MHz)
```

QV100의 기본 클록은 1132 MHz이며, HBM2 메모리는 850 MHz로 동작한다. 부스트 모드에서는 Core/ICNT/L2가 1628 MHz까지 올라갈 수 있다.

### 2.5 Shader Core 파이프라인

```
-gpgpu_shader_registers 65536             # SM당 레지스터 파일 크기 (32비트 레지스터)
-gpgpu_shader_core_pipeline 2048:32       # 최대 스레드:워프 크기
                                          # 2048/32 = 최대 64 워프/SM
-gpgpu_shader_cta 32                      # SM당 최대 CTA(Thread Block) 수
-gpgpu_simd_model 1                       # SIMT 모델 사용

# 파이프라인 폭과 FU 수 (Volta: 4-wide)
-gpgpu_pipeline_widths 4,4,4,4,4,4,4,4,4,4,8,4,4
# 순서: ID_OC_SP, ID_OC_DP, ID_OC_INT, ID_OC_SFU, ID_OC_MEM,
#       OC_EX_SP, OC_EX_DP, OC_EX_INT, OC_EX_SFU, OC_EX_MEM,
#       EX_WB, ID_OC_TENSOR, OC_EX_TENSOR
-gpgpu_num_sp_units 4                     # SP(단정밀도) 유닛 수
-gpgpu_num_sfu_units 4                    # SFU(특수함수) 유닛 수
-gpgpu_num_dp_units 4                     # DP(배정밀도) 유닛 수
-gpgpu_num_int_units 4                    # INT(정수) 유닛 수
-gpgpu_tensor_core_avail 1               # 텐서 코어 활성화
-gpgpu_num_tensor_core_units 4            # 텐서 코어 유닛 수
```

Volta는 각 SM에 4개의 서브코어(processing block)를 가지며, 각 서브코어는 독립된 워프 스케줄러, 레지스터 파일, 실행 유닛을 보유한다.

### 2.6 명령어 레이턴시

```
-ptx_opcode_latency_int 4,13,4,5,145,21
# ADD=4, MAX=13, MUL=4, MAD=5, DIV=145, SHFL=21

-ptx_opcode_latency_fp 4,13,4,5,39
# FP ADD=4, FP MAX=13, FP MUL=4, FP MAD=5, FP DIV=39

-ptx_opcode_latency_dp 8,19,8,8,330
# DP ADD=8, DP MAX=19, DP MUL=8, DP MAD=8, DP DIV=330

-ptx_opcode_latency_sfu 100              # SFU 연산 레이턴시
-ptx_opcode_latency_tesnor 64            # 텐서 코어 레이턴시
```

### 2.7 L1 Data Cache 설정

```
-gpgpu_cache:dl1 S:4:128:64,L:T:m:L:L,A:512:8,16:0,32
```

이 설정 문자열의 해석:

```
포맷: <sector?>:<nsets>:<bsize>:<assoc>,<rep>:<wr>:<alloc>:<wr_alloc>:<idx>,<mshr>:<N>:<merge>,<mq>:<fifo>

S          : 섹터 캐시 (32B 섹터, 128B 라인에 4 섹터)
4          : 세트 수 = 4
128        : 블록 크기 = 128 바이트
64         : 연관도 = 64-way
L          : LRU 교체 정책
T          : write-through (쓰기 정책)  → 실제로는 write-evict
m          : allocate on miss (write allocate)
L          : write allocate policy = LAZY_FETCH_ON_READ
L          : set index function = Linear
A          : MSHR type = Associative
512        : MSHR 엔트리 수 = 512
8          : MSHR merge 수 = 8
16         : miss queue 크기 = 16
0          : (예약)
32         : FIFO 엔트리 (텍스처 캐시용)

총 L1D 용량: 4 x 128 x 64 = 32 KB (기본)
-gpgpu_unified_l1d_size 128 → L1D+Shared = 128 KB
-gpgpu_adaptive_cache_config 1 → 동적 분할 활성화
```

### 2.8 L2 Cache 설정

```
-gpgpu_cache:dl2 S:32:128:24,L:B:m:L:P,A:192:4,32:0,32

S          : 섹터 캐시
32         : 세트 수 = 32
128        : 블록 크기 = 128 바이트
24         : 연관도 = 24-way
L          : LRU 교체 정책
B          : write-back (쓰기 정책)
m          : allocate on miss
L          : write allocate = LAZY_FETCH_ON_READ
P          : set index = Partition based

MSHR: Associative, 192 엔트리, merge 4, miss queue 32

서브파티션당 L2 용량: 32 x 128 x 24 = 96 KB
총 L2 용량: 96 KB x 64 서브파티션 = 6 MB
```

### 2.9 DRAM 설정 (HBM2)

```
-gpgpu_n_mem_per_ctrlr 1                  # 컨트롤러당 DRAM 모듈 수
-gpgpu_dram_buswidth 16                   # 버스 폭 = 16 바이트 (128 비트)
-gpgpu_dram_burst_length 2               # 버스트 길이 = 2
-dram_data_command_freq_ratio 2           # DDR (HBM은 DDR)
-dram_dual_bus_interface 1                # HBM 듀얼 버스 인터페이스

# DRAM 타이밍 (850 MHz 기준)
-gpgpu_dram_timing_opt "nbk=16:CCD=1:RRD=3:RCD=12:RAS=28:RP=12:RC=40:
                        CL=12:WL=2:CDLR=3:WR=10:nbkgrp=4:CCDL=2:RTPL=3"
```

HBM2의 특징: 듀얼 버스 인터페이스로 한 번에 두 개의 column/row 명령을 발행할 수 있다. 128비트 버스 x 2(DDR) x 850 MHz = 채널당 약 3.4 GB/s. 32채널 합산 약 109 GB/s.

### 2.10 인터커넥트 설정

```
-network_mode 2                           # 내장 로컬 crossbar 사용
-icnt_in_buffer_limit 512                 # 입력 버퍼 크기
-icnt_out_buffer_limit 512                # 출력 버퍼 크기
-icnt_subnets 2                           # 서브넷 수 (요청/응답 분리)
-icnt_flit_size 40                        # Flit 크기 = 40 바이트
-icnt_arbiter_algo 1                      # 중재 알고리즘 (iSLIP)
```

`network_mode 2`는 GPGPU-Sim 내장 로컬 crossbar를 사용한다. `network_mode 1`을 사용하면 BookSim 외부 시뮬레이터와 별도 설정 파일(`config_volta_islip.icnt`)을 사용할 수 있다.

### 2.11 통계 수집 설정

```
-gpgpu_memlatency_stat 14                 # 메모리 레이턴시 통계 세분화 수준
-gpgpu_runtime_stat 500                   # 런타임 통계 출력 주기 (사이클)
-enable_ptx_file_line_stats 1             # PTX 파일/라인별 통계 활성화
-visualizer_enabled 0                     # AerialVision 비활성 (0=off, 1=on)
```


## 3. 지원되는 GPU 아키텍처 모델들

`configs/tested-cfgs/` 디렉토리에 검증된 아키텍처 설정이 포함되어 있다.

| 디렉토리 | GPU | 아키텍처 | Compute Capability | SM 수 | 메모리 | DRAM |
|---------|-----|---------|-------------------|------|-------|------|
| SM2_GTX480 | GeForce GTX 480 | Fermi | 2.0 | 15 | 6 채널 | GDDR5 |
| SM3_KEPLER_TITAN | GeForce GTX Titan | Kepler | 3.0 | - | - | GDDR5 |
| SM6_TITANX | Titan X (Pascal) | Pascal | 6.0 | - | - | GDDR5X |
| SM7_GV100 | Tesla GV100 | Volta | 7.0 | - | - | HBM2 |
| SM7_QV100 | Quadro V100 | Volta | 7.0 | 80 | 32 채널 | HBM2 |
| SM7_TITANV | Titan V | Volta | 7.0 | - | - | HBM2 |
| SM75_RTX2060 | GeForce RTX 2060 | Turing | 7.5 | - | - | GDDR6 |
| SM75_RTX2060_S | GeForce RTX 2060 Super | Turing | 7.5 | - | - | GDDR6 |
| SM86_RTX3070 | GeForce RTX 3070 | Ampere | 8.6 | - | - | GDDR6 |

### 아키텍처별 주요 차이점 비교 (GTX480 vs QV100)

```
                        GTX480 (Fermi)          QV100 (Volta)
─────────────────────────────────────────────────────────────
SM 수                   15                      80
워프 스케줄러/SM        2                       4
최대 워프/SM            48 (1536/32)            64 (2048/32)
최대 CTA/SM             8                       32
SP 유닛/SM              2                       4
DP 유닛/SM              0 (SFU에서 실행)        4
텐서 코어              없음                     4 유닛/SM
서브코어 모델           없음                     4 서브코어/SM
L1D 캐시               32KB (Non-sector)       32~128KB (Sector)
L1D+Shared             48+16 or 16+48 KB       128 KB (동적)
L2 캐시                ~786 KB                  6 MB
DRAM 종류              GDDR5                   HBM2
메모리 채널            6                        32
DRAM 클록              924 MHz                  850 MHz
코어 클록              700 MHz                  1132 MHz
스케줄러               GTO                      LRR
인터커넥트             BookSim (iSLIP)          내장 Crossbar
ROP 레이턴시           120 사이클               160 사이클
L1 레이턴시            35 사이클                20 사이클
```


## 4. 벤치마크 실행 흐름과 시뮬레이터 연결

### 4.1 CUDA 애플리케이션 실행 시 내부 동작

```
┌──────────────────────────────────────────────────────────────┐
│ 1. 프로그램 시작                                              │
│    → main() 호출                                             │
│    → CUDA 런타임 초기화 (libcudart_gpgpu-sim.so 로드)         │
│    → gpgpu_context 생성 및 GPU 설정 파일 파싱                  │
│    → gpgpusim.config, accelwattch_*.xml 읽기                 │
└──────────────────────────┬───────────────────────────────────┘
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ 2. 디바이스 메모리 할당/복사                                   │
│    → cudaMalloc() → 시뮬레이터 가상 메모리 할당                │
│    → cudaMemcpy(H2D) → 호스트→디바이스 데이터 복사             │
│      → gpgpu_perf_sim_memcpy 옵션에 따라:                    │
│        0: 즉시 복사 (레이턴시 무시)                            │
│        1: L2 캐시에 복사 (force_l2_tag_update)               │
└──────────────────────────┬───────────────────────────────────┘
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ 3. 커널 런치                                                  │
│    → kernel<<<grid,block>>>(args) 호출                       │
│    → gpgpu_sim에 커널 등록 (m_running_kernels에 추가)          │
│    → kernel_launch_latency (5000 사이클) 후 CTA 배포 시작     │
│    → issue_block2core(): CTA를 SM에 배포                     │
│    → 각 SM에서 워프 생성 및 실행 시작                          │
└──────────────────────────┬───────────────────────────────────┘
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ 4. 사이클 시뮬레이션 루프                                      │
│    → gpgpu_sim::cycle() 반복 호출                            │
│    → 모든 워프 완료까지 실행                                   │
│    → 주기적으로 통계 출력 (gpgpu_runtime_stat 간격)            │
│    → 완료 시 커널 통계 덤프                                   │
└──────────────────────────┬───────────────────────────────────┘
                           ▼
┌──────────────────────────────────────────────────────────────┐
│ 5. 결과 반환                                                  │
│    → cudaMemcpy(D2H) → 디바이스→호스트 결과 복사               │
│    → cudaFree() → 디바이스 메모리 해제                         │
│    → 시뮬레이션 통계 출력                                      │
│      → IPC, 캐시 적중률, DRAM 접근 수, 전력 등                │
└──────────────────────────────────────────────────────────────┘
```

### 4.2 멀티커널 실행

GPGPU-Sim은 최대 `gpgpu_max_concurrent_kernel`(QV100: 128)개의 커널을 동시에 실행할 수 있다. 각 커널은 독립적으로 CTA를 생성하며, SM 자원(레지스터, 공유 메모리, 워프 슬롯)이 허용하는 범위에서 여러 커널의 CTA가 같은 SM에 공존할 수 있다.


## 5. 성능 통계 수집 및 분석 방법

### 5.1 주요 통계 출력 항목

시뮬레이션 완료 시 stdout에 출력되는 주요 통계:

```
핵심 성능 지표:
  gpu_sim_cycle                    : 총 시뮬레이션 사이클 수
  gpu_sim_insn                     : 총 실행 명령어 수
  gpu_ipc                          : IPC (Instructions Per Cycle)
  gpu_occupancy                    : SM 점유율

캐시 통계:
  gpgpu_n_load_insn / gpgpu_n_store_insn : Load/Store 명령어 수
  L1D_total_cache_accesses         : L1D 총 접근 수
  L1D_total_cache_misses           : L1D 총 미스 수
  L1D_total_cache_miss_rate        : L1D 미스율
  L2_total_cache_accesses          : L2 총 접근 수
  L2_total_cache_misses            : L2 총 미스 수
  L2_total_cache_miss_rate         : L2 미스율

DRAM 통계:
  gpgpu_n_dram_reads               : 총 DRAM 읽기 수
  gpgpu_n_dram_writes              : 총 DRAM 쓰기 수
  gpgpu_n_dram_activate            : 총 DRAM activate 수
  gpgpu_n_dram_commands            : 총 DRAM 명령 수

메모리 레이턴시:
  averagemflatency                 : 평균 mem_fetch 레이턴시
  LDmemlatdist                     : Load 레이턴시 분포
  STmemlatdist                     : Store 레이턴시 분포

인터커넥트 통계:
  traffic_breakdown_coretomem[*]   : 코어→메모리 트래픽 분류별 바이트
  traffic_breakdown_memtocore[*]   : 메모리→코어 트래픽 분류별 바이트

파이프라인 스톨:
  gpgpu_n_stall_shd_mem            : 메모리 관련 스톨 수
  gpu_stall_dramfull               : DRAM 큐 포화 스톨
  gpu_stall_icnt2sh                : ICNT→Shader 버퍼 포화 스톨
```

### 5.2 AccelWattch 전력 통계

`-power_simulation_enabled 1` 설정 시 AccelWattch가 활성화되며, 다음 전력 통계가 수집된다:

```
전력 모델 통계 (power_stat.cc 기반):
  Core 레벨:
    - 파이프라인 duty cycle
    - FP/INT/DP/SFU/Tensor 연산 접근 수
    - 레지스터 파일 읽기/쓰기 수
    - 활성 SP/SFU 레인 수
    - Load/Store 큐 명령어 수

  메모리 레벨:
    - L1/L2 캐시 read/write hit/miss
    - 공유 메모리 접근 수
    - DRAM read/write/precharge 수
    - 인터커넥트 SM→MEM / MEM→SM 트래픽

전력 시뮬레이션 모드:
  0 = AccelWattch_SASS_SIM (시뮬레이터 카운터 사용)
  1 = AccelWattch_SASS_HW (하드웨어 카운터 사용)
  2 = AccelWattch_SASS_HYBRID (혼합)
```

### 5.3 AerialVision 시각화

`-visualizer_enabled 1`로 설정하면 gzip 압축된 시각화 로그가 생성된다. AerialVision 도구로 다음을 시각화할 수 있다:

- SM별 활성 CTA 수 변화
- L2 캐시 적중/미스 추이
- DRAM 접근 패턴
- 메모리 레이턴시 분포
- 전력 소비 변화
- IPC 추이


## 6. 테스트 스크립트 분석

### 6.1 short-tests.sh (PTX 기반 실행 모드)

```bash
# 필요 환경변수:
#   CUDA_INSTALL_PATH : CUDA 툴킷 경로
#   CONFIG            : GPU 설정 이름 (define-standard-cfgs.yml에서 선택)
#   GPUAPPS_ROOT      : 컴파일된 벤치마크 위치

# 동작 순서:
# 1. GPGPU-Sim 빌드 (source setup_environment + make)
# 2. accel-sim-framework 클론
# 3. rodinia_2.0-ft 벤치마크를 지정된 CONFIG로 실행
# 4. monitor_func_test.py로 결과 검증
```

이 스크립트는 execution-driven 시뮬레이션을 수행한다. CUDA 바이너리가 직접 시뮬레이터 위에서 실행되며, PTX를 추출하여 시뮬레이션한다.

### 6.2 short-tests-accelsim.sh (SASS 트레이스 기반 실행 모드)

```bash
# 추가 환경변수:
#   ACCELSIM_BRANCH : accel-sim-framework 브랜치 이름

# 동작 순서:
# 1. GPGPU-Sim 빌드
# 2. accel-sim-framework 클론 및 지정 브랜치 checkout
# 3. Accel-Sim GPU 시뮬레이터 별도 빌드
# 4. 사전 생성된 Rodinia SASS 트레이스 다운로드 (Tesla V100 기준)
# 5. QV100-SASS 설정으로 트레이스 재생
# 6. monitor_func_test.py로 결과 검증
```

이 스크립트는 trace-driven 시뮬레이션을 수행한다. NVBit으로 실제 GPU에서 추출한 SASS 트레이스를 재생하므로, 실제 명령어 스케줄링과 레지스터 사용을 정확히 반영한다.

### 6.3 short-tests-cmake.sh (CMake 빌드 시스템)

```bash
# short-tests.sh와 동일하나 빌드 시스템만 CMake 사용
# cmake -B build → cmake --build build → cmake --install build → source setup
```

### 6.4 Rodinia 벤치마크 스위트

테스트 스크립트에서 사용하는 rodinia_2.0-ft는 다음 벤치마크를 포함한다:

```
Rodinia 2.0 Functional Test 벤치마크:
  backprop       : 신경망 역전파 학습
  bfs            : 너비 우선 탐색 (그래프)
  heartwall      : 의료 영상 심벽 추적
  hotspot        : 열 시뮬레이션
  kmeans         : K-means 클러스터링
  lud            : LU 분해
  needle         : Needleman-Wunsch 정렬
  nn             : k-최근접 이웃
  nw             : Needleman-Wunsch 변형
  pathfinder     : 최단 경로
  srad           : 이방성 확산
  streamcluster  : 스트림 클러스터링
```


## 7. 세미나에서 데모할 수 있는 벤치마크 시나리오 제안

### 시나리오 1: 메모리 계층 영향 분석

**목적**: L1/L2 캐시 크기와 적중률이 성능에 미치는 영향 시각화

```bash
# 작업 디렉토리 준비
mkdir -p ~/demo/cache_impact && cd ~/demo/cache_impact

# 실험 1: L1 캐시 활성화 (기본 설정)
cp ~/sources/gpgpu-sim_distribution/configs/tested-cfgs/SM7_QV100/* .
# gpgpusim.config에서 -gpgpu_gmem_skip_L1D 0 확인
./hotspot 512 2 2 ./data/temp_512 ./data/power_512 output.out

# 실험 2: L1 캐시 비활성화
# gpgpusim.config에서 -gpgpu_gmem_skip_L1D 1 로 변경
./hotspot 512 2 2 ./data/temp_512 ./data/power_512 output.out

# 비교 항목: IPC, L1 미스율, L2 접근 수, DRAM 접근 수, 총 사이클
```

**예상 발표 포인트**: L1 캐시를 건너뛰면 모든 글로벌 메모리 접근이 L2까지 가야 하므로 인터커넥트 트래픽이 증가하고 레이턴시가 늘어난다. 캐시 적중률이 높은 워크로드(hotspot처럼 공간 지역성이 있는)에서 특히 큰 차이를 보인다.

### 시나리오 2: DRAM 스케줄러 정책 비교

**목적**: FIFO vs FR-FCFS 스케줄링의 DRAM 활용도 차이

```bash
# 실험 1: FR-FCFS 스케줄러 (기본)
# -gpgpu_dram_scheduler 1

# 실험 2: FIFO 스케줄러
# -gpgpu_dram_scheduler 0

# 비교 항목: DRAM row hit rate, 총 DRAM 명령 수, IPC
```

**예상 발표 포인트**: FR-FCFS는 같은 row에 대한 접근을 우선 처리하여 row buffer hit을 극대화한다. 메모리 집약적인 워크로드에서 FIFO 대비 현저한 성능 개선을 보여준다.

### 시나리오 3: GPU 아키텍처 세대 비교

**목적**: Fermi(GTX480) vs Volta(QV100)의 아키텍처 진화를 성능 숫자로 보여주기

```bash
# 동일한 벤치마크를 두 설정으로 실행
# GTX480: configs/tested-cfgs/SM2_GTX480/
# QV100:  configs/tested-cfgs/SM7_QV100/

# 비교 항목:
#   - SM 수 차이 (15 vs 80)
#   - L1/L2 캐시 크기 차이
#   - 메모리 대역폭 차이 (GDDR5 vs HBM2)
#   - IPC 차이
#   - 메모리 레이턴시 분포 차이
```

**예상 발표 포인트**: 단순히 SM이 많아진 것뿐 아니라, 섹터 캐시 도입(Volta), 서브코어 모델(Volta), HBM2 메모리 등 마이크로아키텍처 변화가 성능에 미치는 복합적 영향을 분석할 수 있다.

### 시나리오 4: 메모리 접근 패턴별 성능 차이

**목적**: Coalesced vs Uncoalesced 메모리 접근의 극적인 성능 차이 시연

간단한 CUDA 프로그램을 작성하여 두 가지 패턴을 비교한다:

```cpp
// coalesced_test.cu
__global__ void coalesced(float *data, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < N) data[idx] = data[idx] * 2.0f;   // 연속 접근
}

__global__ void strided(float *data, int N, int stride) {
    int idx = (blockIdx.x * blockDim.x + threadIdx.x) * stride;
    if (idx < N) data[idx] = data[idx] * 2.0f;    // stride 접근
}
```

**예상 발표 포인트**: Coalesced 접근은 하나의 128B 트랜잭션으로 합쳐지지만, stride 접근은 32개 독립 트랜잭션을 생성하여 인터커넥트와 캐시에 극심한 부하를 준다. GPGPU-Sim의 traffic_breakdown 통계를 통해 실제 생성되는 트래픽 양을 정량적으로 비교할 수 있다.

### 시나리오 5: AccelWattch 전력 분석

**목적**: 연산 집약 vs 메모리 집약 워크로드의 전력 프로파일 차이

```bash
# AccelWattch 활성화
# gpgpusim.config에 추가:
# -power_simulation_enabled 1
# -power_simulation_mode 0
# -accelwattch_xml_file accelwattch_sass_sim.xml

# 연산 집약: matmul 또는 SGEMM
# 메모리 집약: BFS 또는 streamcluster

# 비교 항목: 코어 전력 vs 메모리 전력 비율
```

**예상 발표 포인트**: 연산 집약 워크로드는 SP/SFU 유닛과 레지스터 파일에서 전력이 지배적이고, 메모리 집약 워크로드는 DRAM과 인터커넥트에서 전력이 지배적이다. 이를 통해 전력 최적화 방향을 제시할 수 있다.

### 시나리오 6: 워프 스케줄링 정책 비교

**목적**: GTO vs LRR 스케줄러가 캐시 활용에 미치는 영향

```bash
# 실험 1: LRR (Loose Round Robin) - QV100 기본
# -gpgpu_scheduler lrr

# 실험 2: GTO (Greedy-then-Oldest) - GTX480 기본
# -gpgpu_scheduler gto

# 비교 항목: L1 캐시 미스율, IPC, 워프 활성도
```

**예상 발표 포인트**: GTO는 하나의 워프를 집중 실행하여 L1 캐시 재사용을 극대화하지만, LRR은 공정하게 분배하여 latency hiding에 유리하다. 워크로드 특성에 따라 최적 정책이 달라진다.

### 데모 팁

```
세미나 데모 시 실용적인 조언:

1. 시뮬레이션 시간 관리:
   - 작은 입력 크기 사용 (예: hotspot 64x64 대신 512x512도 줄이기)
   - 사전에 결과를 수집해두고, 라이브에서는 로그만 보여주기
   - debug 빌드보다 release 빌드 사용 (2~5배 빠름)

2. 결과 정리 스크립트:
   grep "gpu_sim_cycle\|gpu_ipc\|L1D_total_cache_miss_rate\|L2_total_cache_miss_rate\|gpgpu_n_dram" output.log

3. 시각적 비교:
   - 두 실험 결과를 나란히 보여주는 표 준비
   - 핵심 지표만 하이라이트 (IPC, 캐시 미스율, DRAM 접근)
   - ASCII 그래프나 간단한 bar chart 사용

4. 소스 코드 연결:
   - 특정 설정이 코드의 어느 부분에서 사용되는지 보여주기
   - 예: rop_latency → memory_sub_partition::push()의 r.ready_cycle
```
