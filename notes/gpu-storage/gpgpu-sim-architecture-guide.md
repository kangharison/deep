# GPGPU-Sim 완전 가이드: GPU 아키텍처 시뮬레이터 A-Z (Part 1)

## 1. GPGPU-Sim 개요

### 1.1 프로젝트 목적과 역사

GPGPU-Sim은 캐나다 University of British Columbia(UBC)의 Tor M. Aamodt 교수 연구실에서 개발한 사이클 정확(cycle-accurate) GPU 시뮬레이터이다. 2009년 첫 릴리스 이후 Wilson W.L. Fung, Ali Bakhoda, George L. Yuan, Andrew Turner, Ivan Sham 등이 초기 개발에 참여했으며, 이후 Northwestern University(Vijay Kandiah, Nikos Hardavellas), Purdue University(Mahmoud Khairy, Timothy G. Rogers, Junrui Pan)가 합류하여 Accel-Sim(SASS 기반 트레이스 시뮬레이션), AccelWattch(전력 모델) 등 주요 확장을 추가했다.

GPGPU-Sim의 핵심 목표는 **실제 NVIDIA GPU 하드웨어를 소스 코드 수준에서 모델링**하여, GPU 아키텍처 연구자가 새로운 하드웨어 설계를 실험하고 성능 병목을 분석할 수 있게 하는 것이다. 실제 GPU 칩 내부의 SIMT 코어(SM) 파이프라인, 메모리 계층(L1/L2 캐시, DRAM), 인터커넥트 네트워크(NoC), 워프 스케줄러 등을 상세하게 시뮬레이션한다.

### 1.2 시뮬레이션 범위

GPGPU-Sim이 시뮬레이션하는 것과 하지 않는 것을 정확히 이해하는 것이 중요하다.

| 시뮬레이션 O | 시뮬레이션 X |
|---|---|
| SIMT 코어(SM) 파이프라인 전체 | CPU 코어 실행 |
| 워프 스케줄링 (LRR, GTO, Two-Level 등) | PCIe 버스 레이턴시 |
| L1 I-Cache, L1 D-Cache, L1 Texture/Constant Cache | 호스트 메모리 (CPU DRAM) |
| L2 통합 캐시 | OS 스케줄링 |
| DRAM 컨트롤러 및 타이밍 모델 (FR-FCFS) | NVLink / PCIe 프로토콜 |
| 인터커넥트 네트워크 (BookSim 기반 NoC) | 디스플레이/그래픽스 파이프라인 |
| 오퍼랜드 컬렉터, 스코어보드, SIMT 스택 | DMA 엔진 |
| AccelWattch 전력 모델 | CPU-GPU 통신 오버헤드 |
| CUDA Dynamic Parallelism (CDP) | Multi-GPU 통신 |

요약하면, **GPU 칩 내부만** 시뮬레이션하며, **PCIe 이하의 시스템 레벨은 제외**한다. `cudaMemcpy`는 시뮬레이션 내 메모리 복사로 구현되지만, 실제 PCIe 전송 레이턴시는 모델링하지 않는다.

### 1.3 지원하는 GPU 아키텍처

GPGPU-Sim은 `gpgpusim.config` 설정 파일을 통해 다양한 GPU 아키텍처를 모델링한다. 주요 설정 프로필:

| 아키텍처 | SM 수 | 워프/SM | 레지스터/SM | 공유메모리 | L1 D-Cache | L2 캐시 |
|---|---|---|---|---|---|---|
| Fermi (GTX 480) | 15 | 48 | 32K | 48KB | 16KB | 768KB |
| Kepler (GTX 780) | 12 | 64 | 64K | 48KB | -- | 1.5MB |
| Pascal (GTX 1080) | 20 | 64 | 64K | 96KB | 48KB | 2MB |
| Volta (Titan V) | 80 | 64 | 64K | 96KB | 128KB | 6MB |

설정 파일에서 SM 수, 워프 크기, 캐시 설정, DRAM 파라미터, 스케줄러 종류 등 수백 가지 파라미터를 조정할 수 있다. Volta 설정에서는 INT 유닛이 SP와 분리되어 동시 실행이 가능하고, Tensor Core도 지원된다.

### 1.4 정확도

GPGPU-Sim의 IPC(Instructions Per Cycle) 오차는 실제 GPU 대비 약 **15-25%** 범위이다. 절대적인 성능 수치보다는 **상대적 비교**에 적합하다. 예를 들어 "스케줄러 A가 B보다 10% 빠르다"는 결론은 신뢰할 수 있지만, "이 커널의 실행 시간이 정확히 1.23ms이다"는 부정확할 수 있다. Accel-Sim(SASS 트레이스 기반)을 사용하면 PTX 기반보다 더 높은 정확도를 얻을 수 있다.

### 1.5 소스 코드 구조 개요

```
sources/gpgpu-sim_distribution/
├── src/
│   ├── gpgpu-sim/           ← GPU 타이밍 시뮬레이션 핵심
│   │   ├── gpu-sim.cc/h     ← gpgpu_sim 클래스: 메인 사이클 루프, 커널 관리
│   │   ├── shader.cc/h      ← shader_core_ctx: SM 파이프라인 전체
│   │   ├── scoreboard.cc/h  ← 레지스터 해저드 감지 (RAW/WAW)
│   │   ├── stack.cc/h       ← SIMT 재수렴 스택 (유틸리티)
│   │   ├── gpu-cache.cc/h   ← L1/L2 캐시 모델
│   │   ├── dram.cc/h        ← DRAM 타이밍 모델 (FR-FCFS 스케줄러)
│   │   ├── l2cache.cc/h     ← L2 캐시 및 메모리 서브파티션
│   │   ├── icnt_wrapper.cc/h ← BookSim 인터커넥트 래퍼
│   │   ├── mem_fetch.cc/h   ← 메모리 요청 패킷 (코어↔메모리)
│   │   └── addrdec.cc/h     ← 주소 디코딩 (물리→DRAM 주소)
│   ├── cuda-sim/            ← PTX 기능적 시뮬레이션
│   │   ├── cuda-sim.cc/h    ← PTX 명령어 해석/실행 엔진
│   │   ├── ptx_ir.cc/h      ← PTX 중간 표현 (명령어, 심볼)
│   │   ├── ptx_parser.cc/h  ← PTX 파서 (flex/bison)
│   │   └── ptx_sim.cc/h     ← PTX 스레드 시뮬레이션
│   ├── intersim2/           ← BookSim2 NoC 시뮬레이터
│   ├── accelwattch/         ← AccelWattch 전력 모델
│   ├── abstract_hardware_model.cc/h ← 핵심 데이터 구조 (kernel_info_t, simt_stack, warp_inst_t)
│   ├── stream_manager.cc/h  ← CUDA 스트림/비동기 연산 관리
│   └── gpgpusim_entrypoint.cc/h ← 시뮬레이션 스레드 시작점
├── libcuda/                 ← CUDA API 인터셉션 레이어
│   ├── cuda_runtime_api.cc  ← cudaMalloc, cudaLaunch 등 가로채기
│   ├── gpgpu_context.h      ← 전역 시뮬레이터 컨텍스트 (싱글턴)
│   └── cuda_api_object.h    ← CUDA API 객체 래퍼
└── configs/                 ← GPU 아키텍처별 설정 파일
    ├── tested-cfgs/
    │   ├── SM2_GTX480/      ← Fermi
    │   ├── SM6_TITANX/      ← Pascal
    │   └── SM7_TITANV/      ← Volta
    └── ...
```

---

## 2. 시뮬레이션 전체 흐름 (Top-Level Flow)

### 2.1 CUDA 프로그램이 GPGPU-Sim에서 실행되는 전체 과정

GPGPU-Sim은 CUDA 프로그램의 **바이너리를 수정하지 않고** 실행한다. 핵심 트릭은 **라이브러리 인터셉션**이다. GPGPU-Sim이 `libcudart.so`(또는 `libcuda.so`)로 컴파일되면, `LD_PRELOAD` 또는 직접 링크를 통해 실제 NVIDIA 런타임 대신 로드된다. CUDA 앱이 `cudaMalloc()`을 호출하면 NVIDIA GPU 대신 GPGPU-Sim 시뮬레이터가 동작한다.

```
┌──────────────────────────────────────────────────────────────────┐
│  CUDA Application  (수정 없이 그대로 실행)                         │
│  ┌──────────┐ ┌─────────────┐ ┌───────────┐ ┌────────────────┐  │
│  │cudaMalloc│ │cudaMemcpy   │ │kernel<<<>>>│ │cudaDeviceSync  │  │
│  └────┬─────┘ └──────┬──────┘ └─────┬─────┘ └──────┬─────────┘  │
│       │              │              │               │            │
└───────┼──────────────┼──────────────┼───────────────┼────────────┘
        ▼              ▼              ▼               ▼
┌──────────────────────────────────────────────────────────────────┐
│  libcuda/cuda_runtime_api.cc  (GPGPU-Sim의 CUDA API 인터셉션)    │
│                                                                   │
│  cudaMalloc → gpu_malloc()  (시뮬레이션 메모리 할당)               │
│  cudaMemcpy → memcpy_to_gpu()  (호스트↔GPU 데이터 복사)          │
│  kernel<<<>>> → kernel_info_t 생성 → stream_manager.push()       │
│  cudaDeviceSync → synchronize()  (시뮬레이션 완료 대기)           │
└──────────────────────┬───────────────────────────────────────────┘
                       ▼
┌──────────────────────────────────────────────────────────────────┐
│  stream_manager  (비동기 연산 관리)                                │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                         │
│  │Stream 0  │ │Stream 1  │ │Stream 2  │ ...                     │
│  │[memcpy]  │ │[kernel]  │ │[memcpy]  │                         │
│  │[kernel]  │ │[kernel]  │ │          │                         │
│  └──────────┘ └──────────┘ └──────────┘                         │
└──────────────────────┬───────────────────────────────────────────┘
                       ▼
┌──────────────────────────────────────────────────────────────────┐
│  gpgpu_sim::cycle()  (메인 시뮬레이션 루프)                       │
│                                                                   │
│  매 사이클: 코어 → ICNT → L2 → DRAM → L2 → ICNT → 코어          │
└──────────────────────────────────────────────────────────────────┘
```

### 2.2 전역 컨텍스트: gpgpu_context

모든 서브시스템은 `gpgpu_context` 싱글턴 객체를 통해 연결된다. `GPGPU_Context()` 함수가 전역 인스턴스를 반환한다.

```cpp
class gpgpu_context {
 public:
  gpgpu_context() {
    // 전역 카운터 초기화
    kernel_info_m_next_uid = 1;     // 커널 고유 ID (1부터 시작)
    warp_inst_sm_next_uid = 0;      // 워프 명령어 고유 ID
    // ...

    // 서브시스템 객체 생성
    api = new cuda_runtime_api(this);      // CUDA API 인터셉션
    ptx_parser = new ptx_recognizer(this); // PTX 파서
    the_gpgpusim = new GPGPUsim_ctx(this); // 시뮬레이터 코어
    func_sim = new cuda_sim(this);         // 기능적 시뮬레이션
    device_runtime = new cuda_device_runtime(this); // CDP 지원
  }

  // 서브시스템 포인터들
  cuda_runtime_api *api;          // CUDA API 인터셉션
  GPGPUsim_ctx *the_gpgpusim;     // 시뮬레이터 메인 (gpgpu_sim, stream_manager)
  cuda_sim *func_sim;             // PTX 명령어 실행 엔진
  ptx_recognizer *ptx_parser;     // PTX 파서
  cuda_device_runtime *device_runtime; // CDP
};
```

접근 경로 예시:
- `GPGPU_Context()->api→...` : CUDA API 인터셉션
- `GPGPU_Context()->the_gpgpusim→g_the_gpu` : GPU 시뮬레이터 객체
- `GPGPU_Context()->the_gpgpusim→g_stream_manager` : 스트림 매니저

### 2.3 cudaMalloc → cudaMemcpy → kernel<<<>>> → cudaDeviceSynchronize 각 단계

**단계 1: `__cudaRegisterFatBinary()`** — 프로그램 로드 시 자동 호출. CUDA 바이너리에서 PTX 코드를 `cuobjdump`로 추출하여 파싱한다. 결과로 전역 심볼 테이블(`g_global_allfiles_symbol_table`)이 생성된다.

**단계 2: `__cudaRegisterFunction()`** — 호스트 함수 포인터와 PTX 커널 이름을 매핑. 이후 `kernel<<<>>>`에서 호스트 포인터로 커널을 식별한다.

**단계 3: `cudaMalloc()`** — `gpu_malloc()`을 통해 시뮬레이션 메모리 공간에 할당. 실제 DRAM이 아닌 소프트웨어 메모리 공간을 사용한다. 반환된 포인터는 가상 GPU 주소이다.

**단계 4: `cudaMemcpy(H2D)`** — 스트림 연산 `stream_memcpy_host_to_device`로 스트림에 push된다. `stream_operation::do_operation()`에서 `gpu->memcpy_to_gpu()`를 호출하여 호스트 메모리의 데이터를 시뮬레이션 GPU 메모리로 복사한다.

**단계 5: `cudaConfigureCall()`** — grid/block 차원 설정. `<<<gridDim, blockDim>>>`에 해당한다.

**단계 6: `cudaSetupArgument()`** — 커널 인자를 바이트 배열로 설정.

**단계 7: `cudaLaunch()`** — 핵심 단계. `kernel_info_t`를 생성하고 `stream_manager`에 `stream_kernel_launch` 연산으로 push한다.

**단계 8: `cudaDeviceSynchronize()`** — 모든 스트림이 비워질 때까지 호스트 스레드가 busy-wait한다.

### 2.4 kernel_info_t 구조체 상세

`kernel_info_t`는 하나의 커널 런치를 완전히 기술하는 핵심 객체이다.

```cpp
kernel_info_t::kernel_info_t(dim3 gridDim, dim3 blockDim,
                             function_info *entry, unsigned long long streamID) {
  m_kernel_entry = entry;   // PTX 커널 함수 (인자, 레지스터 수, 공유메모리 크기)
  m_grid_dim = gridDim;     // 블록 수 = gridDim.x × gridDim.y × gridDim.z
  m_block_dim = blockDim;   // 블록당 스레드 수 = blockDim.x × blockDim.y × blockDim.z
  m_next_cta = {0, 0, 0};  // 다음 디스패치할 CTA 좌표 (x→y→z 순서로 순회)
  m_num_cores_running = 0;  // 이 커널을 실행 중인 SM 수
  m_uid = kernel_info_m_next_uid++;  // 전역 고유 ID
  m_streamID = streamID;    // CUDA 스트림 ID

  // 런치 레이턴시 계산
  m_launch_latency = g_kernel_launch_latency;
  m_kernel_TB_latency = g_kernel_launch_latency
                      + num_blocks() * g_TB_launch_latency;
}
```

**CTA 관리 핵심**: `m_next_cta`는 3D 좌표 `(x, y, z)`로, `increment_x_then_y_then_z()`를 호출할 때마다 x가 먼저 증가하고, x가 `gridDim.x`에 도달하면 0으로 리셋되면서 y가 증가하는 식이다. `no_more_ctas_to_run()`은 `m_next_cta`가 `gridDim`을 넘어가면 true를 반환한다.

**커널 완료 조건**: `done() = no_more_ctas_to_run() && !running()` — 더 이상 할당할 CTA가 없고, 실행 중인 SM도 없을 때 완료.

### 2.5 시뮬레이션 스레드 모델

GPGPU-Sim은 호스트 스레드(CUDA API 호출)와 별도의 시뮬레이션 스레드에서 GPU 사이클을 실행한다. 두 가지 모드가 있다:

**순차 모드 (`gpgpu_sim_thread_sequential`)**: 한 번에 하나의 커널만 실행. 커널이 완전히 끝나야 다음 커널을 시작한다.
```
sem_wait(g_sim_signal_start);   // 시작 신호 대기
g_the_gpu->init();              // GPU 초기화
while (g_the_gpu->active()) {   // GPU가 활성인 동안
    g_the_gpu->cycle();         // 한 사이클 시뮬레이션
    g_the_gpu->deadlock_check(); // 교착 상태 검출
}
g_the_gpu->print_stats();      // 통계 출력
sem_post(g_sim_signal_finish);  // 완료 신호
```

**동시 모드 (`gpgpu_sim_thread_concurrent`)**: 여러 커널을 동시에 실행할 수 있다. `stream_manager`가 작업을 지속적으로 공급하고, 스트림 매니저가 비어있을 때까지 루프를 돈다.

### 2.6 gpgpu_sim::cycle() 메인 루프 상세

이 함수가 시뮬레이터의 심장부이다. 매 호출마다 GPU의 모든 컴포넌트가 한 사이클씩 진행한다.

```
gpgpu_sim::cycle() {
    clock_mask = next_clock_domain();  // 이번에 실행할 클록 도메인 결정

    // ═══ Phase 1: 코어에서 ICNT 응답 수신 ═══
    if (CORE) for each cluster: cluster->icnt_cycle();

    // ═══ Phase 2: 메모리→ICNT 응답 전송 ═══
    if (ICNT) for each sub_partition:
        mf = sub_partition->top();     // 완료된 응답 확인
        icnt_push(mem→core, mf);       // ICNT에 주입

    // ═══ Phase 3: DRAM 스케줄러 실행 ═══
    if (DRAM) for each partition:
        partition->dram_cycle();        // FR-FCFS 스케줄러

    // ═══ Phase 4: L2 캐시 처리 ═══
    if (L2) for each sub_partition:
        mf = icnt_pop(core→mem);       // ICNT에서 요청 수신
        sub_partition->push(mf);       // L2에 요청 전달
        sub_partition->cache_cycle();  // L2 캐시 처리

    // ═══ Phase 5: ICNT 전송 ═══
    if (ICNT) icnt_transfer();         // 패킷 라우팅/전송

    // ═══ Phase 6: 셰이더 코어 파이프라인 실행 ★핵심★ ═══
    if (CORE) for each cluster:
        if (not_completed || more_cta_left)
            cluster->core_cycle();     // SM 파이프라인 실행

    gpu_sim_cycle++;                   // 사이클 카운터 증가

    // ═══ Phase 7: CTA 발행 ═══
    issue_block2core();                // 라운드 로빈으로 CTA 분배

    // ═══ Phase 8: 기타 ═══
    decrement_kernel_latency();        // 런치 레이턴시 감소
    // 캐시 플러시, 통계 수집, 데드락 감지 등
}
```

**클록 도메인**: GPU 내부에는 4개의 독립 클록 도메인이 있다.

| 도메인 | 비트마스크 | 역할 |
|---|---|---|
| CORE | `0x01` | 셰이더 파이프라인, 워프 스케줄링, 명령어 실행 |
| L2 | `0x02` | L2 캐시, ICNT↔L2↔DRAM 데이터 흐름 |
| DRAM | `0x04` | DRAM 스케줄러, 뱅크 상태 관리, 타이밍 모델 |
| ICNT | `0x08` | NoC 패킷 전송, 라우팅 |

`next_clock_domain()`이 각 도메인의 클록 주파수를 기반으로 이번 스텝에서 어떤 도메인이 실행되어야 하는지를 비트마스크로 반환한다. 예를 들어 코어가 1.4GHz이고 DRAM이 900MHz이면, 코어가 DRAM보다 더 자주 실행된다.

### 2.7 CTA 스케줄링: issue_block2core()

CTA(Thread Block)를 SM에 분배하는 알고리즘이다. **클러스터 단위 라운드 로빈**으로 동작한다.

```cpp
void gpgpu_sim::issue_block2core() {
  unsigned last_issued = m_last_cluster_issue;
  for (unsigned i = 0; i < n_simt_clusters; i++) {
    unsigned idx = (i + last_issued + 1) % n_simt_clusters;  // 라운드 로빈
    unsigned num = m_cluster[idx]->issue_block2core();        // CTA 발행 시도
    if (num) {
      m_last_cluster_issue = idx;       // 마지막 발행 클러스터 기록
      m_total_cta_launched += num;      // 총 발행 CTA 수 갱신
    }
  }
}
```

각 SM에 CTA를 할당할 때는 **리소스 체크**가 필수적이다. `occupy_shader_resource_1block()`이 다음 4가지를 확인한다:

1. **스레드 수**: `m_occupied_n_threads + padded_cta_size <= n_thread_per_shader`
2. **HW 스레드 ID**: `find_available_hwtid()`로 연속된 빈 스레드 블록 탐색
3. **공유 메모리**: `m_occupied_shmem + kernel_smem <= gpgpu_shmem_size`
4. **레지스터**: `padded_cta_size * ceil4(regs) <= gpgpu_shader_registers`
5. **CTA 슬롯**: `m_occupied_ctas + 1 <= max_cta_per_core`

CTA 크기는 워프 크기(32)의 배수로 올림(padding)된다. 레지스터는 4의 배수로 정렬된다 (레지스터 파일 뱅크 정렬).

---

## 3. SIMT 코어 (SM) 파이프라인 상세

### 3.1 파이프라인 개요

GPGPU-Sim의 SM 파이프라인은 6개 주요 스테이지로 구성된다:

```
┌────────────────────────────────────────────────────────────────────────┐
│                    SIMT Core (SM) Pipeline                             │
│                                                                        │
│  ┌───────┐   ┌────────┐   ┌───────┐   ┌──────────┐   ┌─────────┐    │
│  │ Fetch │──▶│ Decode │──▶│ Issue │──▶│ Read     │──▶│Execute  │    │
│  │       │   │        │   │(Sched)│   │ Operands │   │(SP/SFU/ │    │
│  │I-Cache│   │I-Buffer│   │Score- │   │(Operand  │   │DP/INT/  │    │
│  │Access │   │Fill    │   │board  │   │Collector)│   │Tensor/  │──┐ │
│  └───────┘   └────────┘   └───────┘   └──────────┘   │LDST)    │  │ │
│       ▲                                               └─────────┘  │ │
│       │                                                            │ │
│       │              ┌───────────┐                                 │ │
│       │              │ Writeback │◀────────────────────────────────┘ │
│       │              │ Scoreboard│                                   │
│       │              │ Release   │                                   │
│       │              └───────────┘                                   │
│       │                                                              │
│       └──────── 워프 PC 기반 라운드 로빈 페치 ───────────────────────│
└────────────────────────────────────────────────────────────────────────┘
```

### 3.2 shader_core_ctx::cycle() — 역순 실행의 이유

```cpp
void shader_core_ctx::cycle() {
  if (!isactive() && get_not_completed() == 0) return;  // 비활성이면 스킵

  m_stats->shader_cycles[m_sid]++;
  writeback();          // 6. 라이트백 (가장 먼저 실행)
  execute();            // 5. 실행
  read_operands();      // 4. 오퍼랜드 읽기
  issue();              // 3. 이슈 (스케줄러)
  for (unsigned i = 0; i < inst_fetch_throughput; ++i) {
    decode();           // 2. 디코드
    fetch();            // 1. 페치 (가장 나중에 실행)
  }
}
```

**역순 실행 이유**: 하드웨어에서는 모든 스테이지가 동시에 동작하지만, 소프트웨어 시뮬레이터에서는 순차적으로 호출해야 한다. **뒤쪽(downstream) 스테이지를 먼저 실행해야** 앞쪽 스테이지의 결과가 같은 사이클에 소비되는 것을 방지한다. writeback이 파이프라인 레지스터를 비워야 execute가 결과를 넣을 수 있고, execute가 이슈 포트를 비워야 issue가 새 명령어를 넣을 수 있다. 이 순서가 **실제 하드웨어의 동시 동작을 정확하게 시뮬레이션**한다.

### 3.3 파이프라인 레지스터 전달 구조

각 파이프라인 스테이지 사이에는 `register_set`이라는 파이프라인 레지스터가 존재한다. 이것이 `warp_inst_t` 포인터를 담는 슬롯 배열이다.

```
주요 파이프라인 레지스터:
  m_inst_fetch_buffer  : Fetch → Decode (ifetch_buffer_t)
  I-Buffer (per warp)  : Decode → Issue  (shd_warp_t 내부, 2슬롯)
  m_pipeline_reg[ID_OC]: Issue → Operand Collector
  m_pipeline_reg[OC_EX]: Operand Collector → Execute
  m_pipeline_reg[EX_WB]: Execute → Writeback
```

### 3.4 Warp 상태 관리 (shd_warp_t)

`shd_warp_t`는 워프의 전체 상태를 관리하는 핵심 클래스이다. SM당 최대 48~64개의 워프 인스턴스가 존재한다.

```cpp
class shd_warp_t {
  // ─── I-Buffer: 페치된 명령어 보관 (2슬롯) ───
  struct ibuffer_entry {
    const warp_inst_t *m_inst;  // 명령어 포인터
    bool m_valid;               // 유효 여부
  } m_ibuffer[IBUFFER_SIZE];    // IBUFFER_SIZE = 2
  unsigned m_next;               // 다음에 이슈할 슬롯 인덱스

  // ─── 실행 상태 ───
  address_type m_next_pc;        // 다음 명령어의 PC
  unsigned m_warp_id;            // 워프 ID (SM 내부)
  unsigned m_dynamic_warp_id;    // 동적 워프 ID (전역, 생성 순서)
  unsigned m_cta_id;             // 소속 CTA ID
  std::bitset<MAX_WARP_SIZE> m_active_threads;  // 활성 스레드 마스크
  unsigned n_completed;          // 완료된 스레드 수

  // ─── 동기화/메모리 상태 ───
  unsigned m_stores_outstanding;  // 미완료 스토어 수
  unsigned m_inst_in_pipeline;    // 파이프라인 내 명령어 수
  unsigned m_n_atomic;            // 미완료 아토믹 연산 수
  bool m_membar;                  // 메모리 배리어 대기 중
  bool m_imiss_pending;           // I-캐시 미스 대기 중
  bool m_done_exit;               // 워프 종료 등록됨
};
```

**I-Buffer**: 2슬롯 구조로, 한 번의 페치로 최대 2개 명령어를 저장한다. `ibuffer_fill(slot, inst)`로 채우고, `ibuffer_next_inst()`로 다음에 이슈할 명령어를 가져오며, `ibuffer_step()`으로 슬롯을 순환한다. `ibuffer_flush()`는 분기 예측 실패(control hazard) 시 I-Buffer를 완전히 비운다.

**하드웨어 완료 조건**: `hardware_done() = functional_done() && stores_done() && !inst_in_pipeline()` — 모든 스레드가 기능적 실행을 완료하고, 모든 스토어가 ACK를 받고, 파이프라인이 비어야 진정한 완료.

**대기 조건**: `waiting()`은 배리어(`m_membar`), 미완료 아토믹(`m_n_atomic`), 비동기 메모리 복사 대기(`m_waiting_ldgsts`) 등을 검사한다.

### 3.5 Fetch 단계

`shader_core_ctx::fetch()`의 동작:

```
fetch() {
  if (페치 버퍼가 비어있다) {
    if (I-캐시에서 응답 도착) {
      // I-캐시 히트 응답 처리
      m_inst_fetch_buffer = {PC, size, warp_id};
      m_inst_fetch_buffer.m_valid = true;
      해당 워프의 imiss_pending 클리어;
    } else {
      // 새로운 페치 요청 생성
      for (라운드 로빈으로 모든 워프 순회) {
        // 1. 완료된 워프 정리 (hardware_done 체크)
        if (워프가 hw_done && !pending_writes && !done_exit) {
          스레드 비활성화, CTA 종료 등록;
        }

        // 2. 페치 가능한 워프 찾기
        if (!functional_done && !imiss_pending && ibuffer_empty) {
          pc = 워프의 현재 PC;
          ppc = pc + PROGRAM_MEM_START;  // 물리 주소 변환
          nbytes = 16;  // 한 번에 16바이트 페치

          // I-캐시 접근
          status = m_L1I->access(ppc, mf);
          if (MISS) 워프에 imiss_pending 설정;
          if (HIT)  m_inst_fetch_buffer에 저장;
          break;  // 한 사이클에 하나의 워프만 페치
        }
      }
    }
  }
  m_L1I->cycle();  // I-캐시 자체 사이클 (미스 처리 등)
}
```

핵심 포인트:
- **한 사이클에 하나의 워프만 페치**한다 (라운드 로빈)
- I-캐시 미스 시 해당 워프는 `imiss_pending` 상태가 되어 다른 워프가 페치된다
- `perfect_inst_const_cache` 옵션을 켜면 I-캐시는 항상 히트한다
- 페치 크기는 16바이트이며, I-캐시 라인 경계를 넘지 않도록 조정된다

### 3.6 Decode 단계

`shader_core_ctx::decode()`는 페치 버퍼의 내용을 해석하여 I-Buffer에 1~2개의 명령어를 채운다.

```cpp
void shader_core_ctx::decode() {
  if (m_inst_fetch_buffer.m_valid) {
    address_type pc = m_inst_fetch_buffer.m_pc;
    const warp_inst_t *pI1 = get_next_inst(warp_id, pc);
    m_warp[warp_id]->ibuffer_fill(0, pI1);   // 슬롯 0에 첫 번째 명령어
    m_warp[warp_id]->inc_inst_in_pipeline();  // 파이프라인 내 명령어 수 증가

    if (pI1) {
      const warp_inst_t *pI2 = get_next_inst(warp_id, pc + pI1->isize);
      if (pI2) {
        m_warp[warp_id]->ibuffer_fill(1, pI2);  // 슬롯 1에 두 번째 명령어
        m_warp[warp_id]->inc_inst_in_pipeline();
      }
    }
    m_inst_fetch_buffer.m_valid = false;  // 페치 버퍼 소비 완료
  }
}
```

`get_next_inst(warp_id, pc)`는 PC 값으로 미리 파싱된 PTX 명령어(`warp_inst_t`)를 조회한다. 이 명령어에는 opcode, 소스/목적지 레지스터, 메모리 공간, 연산 유형, 레이턴시 등 모든 정보가 포함되어 있다.

### 3.7 Issue 단계

`shader_core_ctx::issue()`는 모든 워프 스케줄러를 라운드 로빈으로 호출한다. SM에는 보통 4개의 스케줄러가 있다 (Volta 기준 4개 sub-core에 각 1개).

```cpp
void shader_core_ctx::issue() {
  for (unsigned i = 0; i < schedulers.size(); i++) {
    unsigned j = (Issue_Prio + i) % schedulers.size();
    schedulers[j]->cycle();  // 각 스케줄러의 이슈 로직 실행
  }
  Issue_Prio = (Issue_Prio + 1) % schedulers.size();  // 우선순위 회전
}
```

스케줄러 호출 순서가 매 사이클 회전하여 공평성을 보장한다.

---

## 4. 워프 스케줄링 (Warp Scheduling)

### 4.1 scheduler_unit 기본 클래스

모든 워프 스케줄러의 기본 클래스이다. 핵심 멤버:

```cpp
class scheduler_unit {
protected:
  std::vector<shd_warp_t *> m_supervised_warps;           // 이 스케줄러가 관리하는 워프들
  std::vector<shd_warp_t *> m_next_cycle_prioritized_warps;  // 이번 사이클 우선순위 목록
  std::vector<shd_warp_t *>::const_iterator m_last_supervised_issued;  // 마지막 이슈 워프

  Scoreboard *m_scoreboard;       // 해저드 감지용 스코어보드
  register_set *m_sp_out;         // SP 유닛 이슈 포트
  register_set *m_dp_out;         // DP 유닛 이슈 포트
  register_set *m_sfu_out;        // SFU 유닛 이슈 포트
  register_set *m_int_out;        // INT 유닛 이슈 포트
  register_set *m_tensor_core_out; // Tensor Core 이슈 포트
  register_set *m_mem_out;        // LDST 유닛 이슈 포트

public:
  virtual void order_warps() = 0;  // 순수 가상: 파생 클래스가 정렬 정책 구현
  void cycle();                     // 공통 이슈 로직
};
```

SM에 4개의 스케줄러가 있으면, 전체 워프(예: 64개)를 4등분하여 각 스케줄러에 16개씩 할당(`m_supervised_warps`). 각 스케줄러는 자신이 관리하는 워프만 이슈한다.

### 4.2 scheduler_unit::cycle() — 공통 이슈 로직

모든 스케줄러가 공유하는 메인 이슈 루프이다. 매우 중요한 함수이므로 상세히 설명한다.

```
scheduler_unit::cycle() {
  order_warps();  // ★ 가상함수: 파생 클래스별 우선순위 정렬

  for (각 우선순위 워프 in m_next_cycle_prioritized_warps) {
    if (워프가 done_exit) continue;  // 종료된 워프 스킵

    while (!waiting && !ibuffer_empty && checked < max_issue) {
      pI = ibuffer_next_inst();  // I-Buffer에서 다음 명령어

      // ─── Control Hazard 감지 ───
      get_pdom_stack_top_info(warp_id, &pc, &rpc);
      if (pc != pI->pc) {
        // SIMT 스택의 PC와 I-Buffer의 PC가 불일치
        // → 분기 때문에 I-Buffer의 명령어가 잘못됨
        warp.set_next_pc(pc);
        warp.ibuffer_flush();    // I-Buffer 전체 무효화
        break;
      }

      // ─── Scoreboard 체크 (데이터 해저드) ───
      if (!scoreboard->checkCollision(warp_id, pI)) {
        // 충돌 없음 → 이슈 가능

        // ─── 실행 유닛 선택 및 이슈 ───
        if (LOAD/STORE/MEMBAR) {
          if (m_mem_out->has_free()) {
            issue_warp(*m_mem_out, pI, active_mask, warp_id);
            previous_exec_type = MEM;
          }
        }
        else if (ALU/INT/SP) {
          // INT 유닛이 있으면: INT→정수, SP→FP32
          // INT 유닛이 없으면: SP가 모두 처리
          if (INT 유닛 가용 && op != SP_OP)
            issue_warp(*m_int_out, ...); exec_type = INT;
          else if (SP 유닛 가용)
            issue_warp(*m_sp_out, ...); exec_type = SP;
        }
        else if (DP_OP && dp_units > 0) {
          issue_warp(*m_dp_out, ...); exec_type = DP;
        }
        else if (SFU_OP || (DP_OP && dp_units == 0)) {
          // DP 유닛이 없으면 SFU가 DP도 처리 (Fermi)
          issue_warp(*m_sfu_out, ...); exec_type = SFU;
        }
        else if (TENSOR_CORE_OP) {
          issue_warp(*m_tensor_core_out, ...); exec_type = TENSOR;
        }
      }

      if (warp_inst_issued) {
        ibuffer_step();  // I-Buffer 다음 슬롯으로 이동
        issued++;
      }
      checked++;
    }

    if (issued) {
      m_last_supervised_issued = 이 워프의 반복자;
      break;  // ★ 한 워프에서 이슈하면 다음 워프로 넘어가지 않음
    }
  }
}
```

**듀얼 발행 조건**: `gpgpu_max_insn_issue_per_warp > 1`이고 `gpgpu_dual_issue_diff_exec_units = 1`이면, **같은 워프에서 서로 다른 실행 유닛 타입**의 명령어 2개를 동시에 이슈할 수 있다. 예: SP 명령어 + MEM 명령어. 같은 유닛 타입(예: SP + SP)의 듀얼 이슈는 불허한다 (Maxwell/Pascal 이후의 실제 GPU 동작과 동일).

### 4.3 order_warps() — 각 스케줄러의 정렬 정책

#### LRR (Loose Round Robin)

```cpp
void lrr_scheduler::order_warps() {
  order_lrr(m_next_cycle_prioritized_warps, m_supervised_warps,
            m_last_supervised_issued, m_supervised_warps.size());
}
```

마지막으로 이슈한 워프의 다음 워프부터 순서대로 순환한다. 모든 워프가 공평하게 이슈 기회를 얻지만, 하나의 워프가 캐시 라인을 가져와도 다음 워프로 넘어가므로 캐시 지역성이 낮다.

```
순서 예시 (4개 워프, 마지막 이슈 = W2):
  W3 → W0 → W1 → W2
```

#### GTO (Greedy Then Oldest)

```cpp
void gto_scheduler::order_warps() {
  order_by_priority(m_next_cycle_prioritized_warps, m_supervised_warps,
                    m_last_supervised_issued, m_supervised_warps.size(),
                    ORDERING_GREEDY_THEN_PRIORITY_FUNC,
                    sort_warps_by_oldest_dynamic_id);
}
```

**핵심 아이디어**: 마지막에 이슈한 워프를 최우선으로 계속 실행(greedy). 그 워프가 stall하면, `dynamic_warp_id`가 가장 작은(= 가장 오래된) 워프를 선택한다.

```
정렬 기준:
  1순위: 마지막 이슈 워프 (greedy)
  2순위: dynamic_warp_id가 작은 순 (oldest)

done_exit 또는 waiting 상태의 워프는 항상 후순위
```

GTO의 장점은 **캐시 지역성 극대화**이다. 하나의 워프가 L1 캐시에 데이터를 가져오면 계속 그 워프를 실행하므로, 해당 캐시 라인이 다른 워프에 의해 밀려나기 전에 최대한 활용할 수 있다. NVIDIA GPU에서 실제로 사용되는 것으로 추정되는 정책이다.

#### Two-Level Active

```
구조:
  ┌─────────────────────────┐
  │  Active Group            │  ← 이슈 대상 (m_next_cycle_prioritized_warps)
  │  [W0] [W3] [W5] [W7]    │     최대 m_max_active_warps개
  └─────────┬───────────────┘
            ↕ promote/demote
  ┌─────────────────────────┐
  │  Pending Group           │  ← 대기 중 (m_pending_warps)
  │  [W1] [W2] [W4] [W6]    │
  └─────────────────────────┘
```

**동작 원리**: 워프를 Active/Pending 두 그룹으로 나눈다. Active 그룹만 이슈 대상이며, Active 워프가 long-latency 연산(글로벌 메모리 접근 등)으로 stall하면 Pending으로 강등하고, Pending에서 하나를 Active로 승격한다. 이렇게 하면 Active 그룹의 워프 수가 제한되어 L1 캐시 thrashing을 방지할 수 있다.

설정 형식: `two_level_active:max_active:inner_policy:outer_policy`

- `max_active`: Active 그룹의 최대 워프 수
- `inner_policy`: Active 그룹 내부 정렬 (LRR 또는 GTO)
- `outer_policy`: Active↔Pending 간 전환 정렬

#### Oldest-First

```cpp
void oldest_scheduler::order_warps() {
  order_by_priority(m_next_cycle_prioritized_warps, m_supervised_warps,
                    m_last_supervised_issued, m_supervised_warps.size(),
                    ORDERED_PRIORITY_FUNC_ONLY,  // greedy 없음
                    sort_warps_by_oldest_dynamic_id);
}
```

GTO와 달리 greedy 동작 없이 순수하게 `dynamic_warp_id`가 가장 작은 워프를 항상 최우선으로 이슈한다.

### 4.4 issue_warp() 함수 상세

스케줄러가 워프를 선택한 후 실제로 파이프라인에 투입하는 함수이다.

```cpp
void shader_core_ctx::issue_warp(register_set &pipe_reg_set,
                                 const warp_inst_t *next_inst,
                                 const active_mask_t &active_mask,
                                 unsigned warp_id, unsigned sch_id) {
  // 1. 빈 파이프라인 레지스터 슬롯 확보
  warp_inst_t **pipe_reg = pipe_reg_set.get_free(sub_core_model, sch_id);

  // 2. I-Buffer에서 명령어 해제
  m_warp[warp_id]->ibuffer_free();

  // 3. 명령어를 파이프라인 레지스터에 복사 + 동적 정보 설정
  **pipe_reg = *next_inst;                    // 정적 정보 복사
  (*pipe_reg)->issue(active_mask, warp_id, cycle, dynamic_warp_id, ...);

  // 4. ★ 기능적 실행 (PTX 명령어 실행, 메모리 주소 생성)
  func_exec_inst(**pipe_reg);
  // → execute_warp_inst_t(): 각 활성 스레드에서 PTX 명령어 실행
  // → generate_mem_accesses(): load/store이면 접근 주소 생성

  // 5. 배리어/메모리 배리어 처리
  if (BARRIER_OP) → m_barriers.warp_reaches_barrier();
  if (MEMORY_BARRIER_OP) → m_warp[warp_id]->set_membar();

  // 6. SIMT 스택 업데이트 (분기 발산/재수렴)
  updateSIMTStack(warp_id, *pipe_reg);

  // 7. 스코어보드에 출력 레지스터 예약
  m_scoreboard->reserveRegisters(*pipe_reg);

  // 8. 다음 PC 업데이트
  m_warp[warp_id]->set_next_pc(next_inst->pc + next_inst->isize);
}
```

중요한 점: GPGPU-Sim에서 **기능적 실행은 이슈 시점**에 이루어진다. 실제 GPU에서는 실행 유닛에서 연산이 수행되지만, 시뮬레이터에서는 이슈 시 미리 결과를 계산하고, 이후의 파이프라인 스테이지는 **타이밍만** 시뮬레이션한다. 이 분리(기능적 시뮬레이션 + 타이밍 시뮬레이션)가 GPGPU-Sim의 핵심 설계이다.

---

## 5. 오퍼랜드 컬렉터 (Operand Collector)

### 5.1 왜 필요한가

GPU의 레지스터 파일은 여러 **뱅크(bank)**로 나뉘어 있다. 한 사이클에 같은 뱅크에 여러 접근이 발생하면 **뱅크 충돌(bank conflict)**이 생겨 파이프라인이 stall한다. 오퍼랜드 컬렉터는 이 문제를 해결하기 위한 하드웨어 구조로, 레지스터 읽기를 **여러 사이클에 걸쳐 분산**시킨다.

```
레지스터 파일 뱅크 충돌 예시:
  명령어: ADD R4, R1, R5   (R1과 R5가 같은 뱅크)

  뱅크 충돌 없으면: 1사이클에 R1, R5 동시 읽기
  뱅크 충돌 발생:   사이클1에 R1 읽기, 사이클2에 R5 읽기
```

### 5.2 opndcoll_rfu_t 구조

```
┌──────────────────────────────────────────────────────────┐
│  Operand Collector Unit                                   │
│                                                           │
│  ┌────────────┐    ┌───────────┐    ┌──────────────────┐ │
│  │ Register   │    │ Arbiter   │    │ Collector Units  │ │
│  │ File       │◀──▶│ (wavefront│──▶ │ CU-0: [R1][__]  │ │
│  │ Bank 0     │    │  matching)│    │ CU-1: [R3][R5]  │ │
│  │ Bank 1     │    │           │    │ CU-2: [__][__]  │ │
│  │ Bank 2     │    └───────────┘    │ CU-3: [R7][R8]  │ │
│  │ Bank 3     │                     └──────┬───────────┘ │
│  │ ...        │                            │             │
│  └────────────┘                     ┌──────▼───────────┐ │
│                                     │ Dispatch Units   │ │
│                                     │ → 실행 유닛으로   │ │
│                                     │   오퍼랜드 전달   │ │
│                                     └──────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

### 5.3 step() — 매 사이클 동작

```cpp
void opndcoll_rfu_t::step() {
  dispatch_ready_cu();   // 1. 모든 오퍼랜드가 수집된 CU → 실행 유닛으로 디스패치
  allocate_reads();      // 2. arbiter가 뱅크 충돌 없는 읽기를 선택
  for (각 포트 p) allocate_cu(p);  // 3. 이슈 포트에서 빈 CU에 명령어 할당
  process_banks();       // 4. 뱅크 할당 상태 리셋
}
```

### 5.4 arbiter_t::allocate_reads() — Wavefront 알고리즘

BookSim에서 차용한 wavefront 매칭 알고리즘으로 뱅크↔CU 간 충돌 없는 할당을 결정한다.

```cpp
std::list<op_t> arbiter_t::allocate_reads() {
  // 입력: 뱅크(banks), 출력: CU(collectors)
  int _inputs = m_num_banks;
  int _outputs = m_num_collectors;
  int _square = max(_inputs, _outputs);

  // 매칭 초기화
  for (i) _inmatch[i] = -1;   // 뱅크 매칭 초기화
  for (j) _outmatch[j] = -1;  // CU 매칭 초기화

  // 요청 행렬 구성
  for (각 뱅크 i) {
    if (!m_queue[i].empty()) {
      oc_id = m_queue[i].front().get_oc_id();
      _request[i][oc_id] = 1;  // 뱅크 i가 CU oc_id로 읽기 요청
    }
    if (m_allocated_bank[i].is_write()) {
      _inmatch[i] = 0;  // ★ 쓰기가 예약된 뱅크는 읽기보다 우선
    }
  }

  // Wavefront 매칭: 대각선을 따라 순회하며 그리디하게 매칭
  for (p = 0; p < _square; p++) {
    output = (_pri + p) % _outputs;
    for (input = 0; input < _inputs; input++) {
      if (_inmatch[input] == -1 && _request[input][output]) {
        _inmatch[input] = output;   // 뱅크 → CU 매칭 성립
        _outmatch[output] = input;
      }
      output = (output + 1) % _outputs;
    }
  }
  _pri = (_pri + 1) % _outputs;  // 우선순위 회전 (공평성)

  // 매칭 결과 적용: 성공한 읽기를 뱅크에서 pop하여 결과 리스트에 추가
  for (각 뱅크 i) {
    if (_inmatch[i] != -1 && !m_allocated_bank[i].is_write()) {
      op = m_queue[i].pop_front();
      result.push_back(op);
      m_allocated_bank[i].alloc_read(op);
    }
  }
  return result;
}
```

### 5.5 뱅크 충돌 예시와 해결 과정

```
명령어: MUL R10, R1, R5    (R1 → Bank 1, R5 → Bank 1, R10 → Bank 2)

사이클 1: CU에 명령어 할당
  CU-0 = {warp 3, MUL, 소스: R1(Bank1), R5(Bank1)}
  arbiter 큐: Bank1 = [R1(CU-0), R5(CU-0)]

사이클 2: allocate_reads()
  Bank 1에서 CU-0으로 R1 읽기 성공 (뱅크 1개만 허용)
  R5는 같은 뱅크이므로 다음 사이클로 연기
  CU-0: [R1=완료][R5=대기]

사이클 3: allocate_reads()
  Bank 1에서 CU-0으로 R5 읽기 성공
  CU-0: [R1=완료][R5=완료] → 모든 오퍼랜드 수집 완료

사이클 4: dispatch_ready_cu()
  CU-0이 ready → 실행 유닛으로 디스패치
```

뱅크 충돌이 없는 경우 (R1 → Bank 1, R5 → Bank 3이면):
```
사이클 2: Bank 1→R1, Bank 3→R5 동시 읽기 성공
사이클 3: 바로 디스패치 (1사이클 절약)
```

**레지스터→뱅크 매핑**: `bank = register_bank(reg, warp_id, num_banks, ...)` — 레지스터 번호와 워프 ID를 조합하여 뱅크를 결정한다. 같은 레지스터 번호라도 다른 워프이면 다른 뱅크에 매핑될 수 있다 (뱅크 충돌 분산).

---

## 6. 실행 유닛 (Execution Units)

### 6.1 pipelined_simd_unit 기본 클래스

모든 파이프라인 실행 유닛의 기본 클래스이다. 내부에 **시프트 레지스터** 형태의 파이프라인을 가진다.

```cpp
class pipelined_simd_unit {
protected:
  unsigned m_pipeline_depth;        // 파이프라인 깊이
  warp_inst_t **m_pipeline_reg;     // 파이프라인 스테이지 배열
  register_set *m_result_port;      // 결과 출력 포트 (EX_WB 레지스터)
  warp_inst_t *m_dispatch_reg;      // 디스패치 레지스터 (이슈 시 명령어 수신)
};
```

**cycle() 동작**:

```cpp
void pipelined_simd_unit::cycle() {
  // 1. 파이프라인 최하단(stage 0)이 비어있지 않으면 → 결과 포트로 전달
  if (!m_pipeline_reg[0]->empty()) {
    m_result_port->move_in(m_pipeline_reg[0]);  // EX_WB로 이동
    active_insts_in_pipeline--;
  }

  // 2. 파이프라인 시프트: 각 스테이지의 명령어를 한 칸 아래로 이동
  if (active_insts_in_pipeline) {
    for (stage = 0; (stage+1) < m_pipeline_depth; stage++)
      move_warp(m_pipeline_reg[stage], m_pipeline_reg[stage+1]);
  }

  // 3. 디스패치 레지스터에 명령어가 있으면 → 파이프라인에 삽입
  if (!m_dispatch_reg->empty()) {
    if (!m_dispatch_reg->dispatch_delay()) {
      // 삽입 위치 = latency - initiation_interval
      int start_stage = latency - initiation_interval;
      if (m_pipeline_reg[start_stage]->empty()) {
        move_warp(m_pipeline_reg[start_stage], m_dispatch_reg);
        active_insts_in_pipeline++;
      }
    }
  }
}
```

```
파이프라인 시프트 레지스터 동작:

  디스패치 레지스터 → [stage N-1] → [stage N-2] → ... → [stage 1] → [stage 0] → 결과 포트
                      ▲                                                          │
                      │                                                          ▼
                   삽입 위치                                                   EX_WB
                   = latency - initiation_interval
```

**latency vs initiation_interval**: latency는 명령어가 결과를 생산하기까지의 사이클 수이고, initiation_interval은 다음 명령어를 투입할 수 있는 최소 간격이다. 예를 들어 SFU에서 `sin()` 연산이 latency=20, initiation_interval=4이면, 20사이클 후에 결과가 나오지만 4사이클마다 새 명령어를 받을 수 있다 (파이프라인이 겹침).

### 6.2 각 실행 유닛의 특성

| 유닛 | 클래스 | 처리 연산 | 전형적 레이턴시 | 비고 |
|---|---|---|---|---|
| SP | `sp_unit` | FP32, (INT*) | 1-6 사이클 | *INT 유닛이 없으면 정수도 처리 |
| SFU | `sfu` | sin, cos, sqrt, rsqrt, exp, log | 20-40 사이클 | Fermi에서는 DP도 처리 |
| DP | `dp_unit` | FP64 (dadd, dmul, dfma) | 8-64 사이클 | Volta 이후 독립, 0이면 SFU가 대행 |
| INT | `int_unit` | 정수 ALU (iadd, imul, iand) | 1-4 사이클 | Volta 이후 SP와 분리 |
| Tensor Core | `tensor_core` | WMMA (행렬 곱셈-누적) | ~32 사이클 | Volta 이후, 16x16x16 FP16→FP32 |
| LDST | `ldst_unit` | load/store (메모리 접근) | 가변 (4~수백) | stallable, 자체 writeback 경로 |

**실행 유닛별 연산 라우팅 결정 로직** (scheduler_unit::cycle에서):

```
if (LOAD_OP || STORE_OP || MEMORY_BARRIER_OP || TENSOR_CORE_LOAD/STORE)
    → m_mem_out (LDST 유닛)

else if (TENSOR_CORE_OP)
    → m_tensor_core_out

else if (SFU_OP || ALU_SFU_OP || (DP_OP && dp_units == 0))
    → m_sfu_out

else if (DP_OP && dp_units > 0)
    → m_dp_out

else if (SP_OP && int_units == 0)
    → m_sp_out  (정수도 SP에서 처리: Fermi/Pascal)

else if (SP_OP && int_units > 0)
    → m_sp_out  (FP32 전용: Volta)

else if (ALU_OP/INTP_OP && int_units > 0)
    → m_int_out (정수 전용: Volta)
```

### 6.3 execute() 스테이지

```cpp
void shader_core_ctx::execute() {
  // 1. 결과 버스 시프트 (시간 경과)
  for (각 결과 버스) *m_result_bus[i] >>= 1;

  // 2. 각 실행 유닛 처리
  for (각 FU n) {
    // clock_multiplier: 일부 유닛은 코어보다 빠른 클록으로 동작
    for (c = 0; c < m_fu[n]->clock_multiplier(); c++)
      m_fu[n]->cycle();  // 파이프라인 시프트

    // 이슈 포트에 준비된 명령어 확인
    if (issue_inst.has_ready() && m_fu[n]->can_issue(**ready_reg)) {
      if (!stallable) {  // SP, SFU, DP, INT, Tensor Core
        resbus = test_res_bus(latency);  // 결과 버스 예약
        if (resbus != -1) {
          m_result_bus[resbus]->set(latency);
          m_fu[n]->issue(issue_inst);
        }
      } else {  // LDST 유닛 (stallable)
        m_fu[n]->issue(issue_inst);  // 결과 버스 예약 없음
      }
    }
  }
}
```

**결과 버스**: 비트 벡터로, 특정 사이클 후에 EX_WB 레지스터에 결과가 도착함을 예약한다. `test_res_bus(latency)`는 `latency` 사이클 후에 빈 결과 버스 슬롯이 있는지 확인한다. 빈 슬롯이 없으면 이슈가 지연된다 (구조적 해저드).

**LDST 유닛의 특수성**: `stallable() = true`로, 결과 버스를 사용하지 않고 자체 writeback 경로를 가진다. 메모리 레이턴시가 가변적이므로 결과 도착 시점을 미리 예약할 수 없기 때문이다.

---

## 7. Scoreboard (해저드 감지)

### 7.1 데이터 해저드 종류

| 해저드 | 설명 | GPGPU-Sim에서 |
|---|---|---|
| **RAW** (Read-After-Write) | 앞 명령어가 쓰기 완료 전에 뒤 명령어가 읽기 | **감지 + 방지** |
| **WAW** (Write-After-Write) | 두 명령어가 동일 레지스터에 쓰기 순서 역전 | **감지 + 방지** |
| **WAR** (Write-After-Read) | 뒤 명령어가 쓰기 전에 앞 명령어가 읽기 완료 필요 | **발생하지 않음** |

**WAR이 발생하지 않는 이유**: GPGPU-Sim은 in-order issue 방식이다. 읽기(오퍼랜드 수집)는 이슈 직후에 시작되고, 쓰기는 파이프라인을 거친 후 writeback에서 수행된다. 따라서 읽기가 항상 쓰기보다 먼저 완료된다.

### 7.2 Scoreboard 클래스 상세

```cpp
class Scoreboard {
private:
  unsigned m_sid;  // SM ID

  // ★ 핵심 자료구조 ★
  std::vector<std::set<unsigned>> reg_table;   // reg_table[warp_id] = {예약된 레지스터 번호들}
  std::vector<std::set<unsigned>> longopregs;  // longopregs[warp_id] = {long op 레지스터들}
};
```

`reg_table`은 **워프별로** 현재 "쓰기 대기 중"인 레지스터 번호의 집합이다. 명령어가 이슈되면 그 명령어의 목적지 레지스터가 여기에 추가되고, writeback이 완료되면 제거된다.

`longopregs`는 글로벌/로컬/텍스처 메모리 로드처럼 **수백 사이클**이 걸리는 연산의 목적지 레지스터를 별도로 추적한다. 워프 스케줄러가 "이 워프는 메모리 응답 대기 중"임을 빠르게 파악하여 다른 워프로 전환하는 데 사용한다.

### 7.3 reserveRegisters()

```cpp
void Scoreboard::reserveRegisters(const warp_inst_t *inst) {
  // 모든 출력(dest) 레지스터를 예약
  for (r = 0; r < MAX_OUTPUT_VALUES; r++) {
    if (inst->out[r] > 0) {
      reserveRegister(inst->warp_id(), inst->out[r]);
      // → reg_table[warp_id].insert(regnum)
      // 이미 예약된 레지스터를 다시 예약하면 abort() (WAW 방지)
    }
  }

  // Long operation 추적 (글로벌/로컬/텍스처/파라미터 로드)
  if (inst->is_load() && (global || local || param || tex)) {
    for (r = 0; r < MAX_OUTPUT_VALUES; r++)
      if (inst->out[r] > 0)
        longopregs[warp_id].insert(inst->out[r]);
  }
}
```

### 7.4 releaseRegisters()

```cpp
void Scoreboard::releaseRegisters(const warp_inst_t *inst) {
  for (r = 0; r < MAX_OUTPUT_VALUES; r++) {
    if (inst->out[r] > 0) {
      releaseRegister(warp_id, inst->out[r]);
      // → reg_table[warp_id].erase(regnum)
      longopregs[warp_id].erase(inst->out[r]);  // long op도 해제
    }
  }
}
```

writeback 단계에서 호출된다:
```cpp
void shader_core_ctx::writeback() {
  pipe_reg = m_pipeline_reg[EX_WB].get_ready();
  while (pipe_reg && !pipe_reg->empty()) {
    m_operand_collector.writeback(*pipe_reg);       // 레지스터 파일 기록
    m_scoreboard->releaseRegisters(pipe_reg);       // ★ 스코어보드 해제
    m_warp[warp_id]->dec_inst_in_pipeline();
    warp_inst_complete(*pipe_reg);
    pipe_reg->clear();
  }
}
```

### 7.5 checkCollision()

```cpp
bool Scoreboard::checkCollision(unsigned wid, const inst_t *inst) const {
  // 명령어의 모든 입력 + 출력 레지스터를 수집
  std::set<int> inst_regs;
  for (iii) inst_regs.insert(inst->out[iii]);   // 출력 (WAW 검사)
  for (jjj) inst_regs.insert(inst->in[jjj]);    // 입력 (RAW 검사)
  if (inst->pred > 0) inst_regs.insert(inst->pred);  // 프레디케이트
  if (inst->ar1 > 0)  inst_regs.insert(inst->ar1);   // 주소 레지스터 1
  if (inst->ar2 > 0)  inst_regs.insert(inst->ar2);   // 주소 레지스터 2

  // 수집된 레지스터와 스코어보드의 교집합 검사
  for (각 레지스터 r in inst_regs)
    if (reg_table[wid].find(r) != reg_table[wid].end())
      return true;   // ★ 충돌 → 이 명령어는 이슈 불가 (stall)

  return false;  // 충돌 없음 → 이슈 가능
}
```

`checkCollision()`은 `scheduler_unit::cycle()`에서 명령어를 이슈하기 전에 호출된다:
```cpp
if (!m_scoreboard->checkCollision(warp_id, pI)) {
  // 충돌 없음 → 이슈 진행
  ready_inst = true;
  ...
} else {
  // 충돌 → stall, 이 워프는 이번 사이클에 이슈 불가
}
```

---

## 8. SIMT 스택 (분기 발산/재수렴)

### 8.1 SIMT 실행 모델과 분기 발산 문제

GPU에서 워프의 32개 스레드는 **동일한 명령어를 동시에** 실행한다 (SIMT: Single Instruction, Multiple Thread). 그러나 조건 분기에서 일부 스레드는 then 경로, 나머지는 else 경로를 실행해야 하는 상황이 발생한다. 이것이 **분기 발산(branch divergence)**이다.

```
워프 내 8개 스레드 (간략화, 실제는 32개):

if (threadIdx.x < 4) {   // 스레드 0,1,2,3은 true, 스레드 4,5,6,7은 false
    A();  // then 경로
} else {
    B();  // else 경로
}
C();  // 재수렴 지점 (post-dominator)
```

분기 발산이 발생하면 **두 경로를 순차적으로** 실행해야 한다. 이때 비활성 스레드는 마스크되어 연산 결과가 무효화된다.

### 8.2 PDOM(Post-Dominator) 기반 재수렴

GPGPU-Sim은 **Immediate Post-Dominator (IPDOM)** 기반 재수렴 알고리즘을 사용한다. 컴파일러가 각 분기 명령어에 대해 재수렴 지점 PC(IPDOM PC)를 계산하고, 시뮬레이터가 이를 활용하여 스택 기반으로 분기/재수렴을 관리한다.

### 8.3 SIMT 스택 구조

```cpp
struct simt_stack_entry {
  address_type m_pc;          // 이 경로의 다음 실행 PC
  unsigned int m_calldepth;   // 함수 호출 깊이
  simt_mask_t m_active_mask;  // 활성 스레드 비트마스크 (예: "11110000")
  address_type m_recvg_pc;    // 재수렴 PC (이 경로가 여기에 도달하면 팝)
  stack_entry_type m_type;    // NORMAL 또는 CALL
};

std::deque<simt_stack_entry> m_stack;  // TOS = m_stack.back()
```

스택의 **최상단(TOS)** 엔트리가 현재 실행할 경로를 결정한다:
- `m_pc` → 다음 명령어의 PC
- `m_active_mask` → 어떤 스레드가 이 경로를 실행하는지
- `m_recvg_pc` → 이 경로가 끝나는 지점 (재수렴 PC)

### 8.4 if-else 분기 예시: 단계별 스택 동작

```
코드:
  PC=100: if (cond)    // 스레드 0,1 = true, 스레드 2,3 = false
  PC=200: A()          // then 경로
  PC=300: B()          // else 경로
  PC=400: C()          // 재수렴 지점 (IPDOM)
```

**초기 상태** (분기 전):
```
스택 (bottom → top):
  ┌─────────────────────────────────────┐
  │ PC=100, mask=1111, recvg=∞         │ ← TOS
  └─────────────────────────────────────┘
  모든 스레드가 활성, PC=100 실행
```

**분기 발산 후** (PC=100의 if문 실행):
```
simt_stack::update() 호출:
  - 스레드 0,1은 next_pc=200 (then)
  - 스레드 2,3은 next_pc=300 (else)
  → 2개의 분기 경로 식별

스택 (분기 후):
  ┌─────────────────────────────────────┐
  │ PC=400, mask=1111, recvg=∞         │  수렴 엔트리 (bottom)
  ├─────────────────────────────────────┤
  │ PC=300, mask=0011, recvg=400       │  else 경로 (not-taken 먼저 push)
  ├─────────────────────────────────────┤
  │ PC=200, mask=1100, recvg=400       │ ← TOS: then 경로
  └─────────────────────────────────────┘

  → TOS의 PC=200, mask=1100으로 then 경로 실행 (스레드 0,1만 활성)
```

**then 경로 완료** (PC가 400에 도달):
```
PC=200의 실행 완료, 다음 PC가 400 (= recvg_pc)

simt_stack::update() 호출:
  - tmp_next_pc == top_recvg_pc (400 == 400)
  → 이 경로는 수렴점에 도달 → TOS 팝

스택:
  ┌─────────────────────────────────────┐
  │ PC=400, mask=1111, recvg=∞         │  수렴 엔트리
  ├─────────────────────────────────────┤
  │ PC=300, mask=0011, recvg=400       │ ← TOS: else 경로
  └─────────────────────────────────────┘

  → TOS의 PC=300, mask=0011으로 else 경로 실행 (스레드 2,3만 활성)
```

**else 경로 완료** (PC가 400에 도달):
```
PC=300의 실행 완료, 다음 PC가 400 (= recvg_pc)

simt_stack::update() 호출:
  - tmp_next_pc == top_recvg_pc (400 == 400)
  → TOS 팝

스택:
  ┌─────────────────────────────────────┐
  │ PC=400, mask=1111, recvg=∞         │ ← TOS: 재수렴 완료!
  └─────────────────────────────────────┘

  → 모든 스레드(mask=1111)가 PC=400에서 합류하여 함께 실행
```

### 8.5 중첩 분기 처리

분기 안에 또 다른 분기가 있으면 스택이 더 깊어진다:

```
코드:
  PC=100: if (cond1)         // 외부 분기, IPDOM=500
  PC=200:   if (cond2)       // 내부 분기, IPDOM=400
  PC=250:     X()            // then-then
  PC=300:     Y()            // then-else
  PC=400:   Z()              // 내부 재수렴
  PC=450: W()                // 외부 else
  PC=500: END                // 외부 재수렴
```

스택 최대 깊이 (외부 then, 내부 분기 시):
```
  ┌──────────────────────────────────────┐
  │ PC=500, mask=full, recvg=∞          │  외부 수렴
  ├──────────────────────────────────────┤
  │ PC=450, mask=outer_else, recvg=500  │  외부 else
  ├──────────────────────────────────────┤
  │ PC=400, mask=outer_then, recvg=500  │  내부 수렴
  ├──────────────────────────────────────┤
  │ PC=300, mask=inner_else, recvg=400  │  내부 else
  ├──────────────────────────────────────┤
  │ PC=250, mask=inner_then, recvg=400  │ ← TOS
  └──────────────────────────────────────┘
```

### 8.6 simt_stack::update() 알고리즘 상세

이 함수는 분기 명령어 실행 후 호출되어 SIMT 스택을 갱신하는 **핵심 알고리즘**이다.

```
simt_stack::update(thread_done, next_pc[], recvg_pc, op_type, ...) {

  // ═══ 1단계: 활성 스레드들의 다음 PC를 수집하여 분기 경로 식별 ═══
  divergent_paths = {};  // map<다음PC, 활성마스크>

  while (top_active_mask에 처리할 스레드가 남아있음) {
    for (lane = warp_size-1; lane >= 0; lane--) {  // 높은 lane부터
      if (활성 스레드) {
        if (thread_done) → 활성 마스크에서 제거
        else if (첫 번째 발견) → 이 PC를 기준 그룹으로 설정
        else if (같은 PC) → 같은 그룹에 추가
        // 다른 PC는 다음 반복에서 처리
      }
    }
    divergent_paths[tmp_next_pc] = tmp_active_mask;
    num_divergent_paths++;
  }

  assert(num_divergent_paths <= 2);  // 최대 2개 경로 (taken/not-taken)

  // ═══ 2단계: 분기 경로를 스택에 반영 ═══
  not_taken_pc = next_inst_pc + next_inst_size;  // 순차 다음 명령어

  for (각 경로) {
    // not-taken을 먼저 처리 (스택에 먼저 들어가 나중에 실행됨)

    // ─── CALL/RET 특수 처리 ───
    if (CALL_OPS) {
      새 엔트리(type=CALL)를 push;
      return;
    }
    if (RET_OPS && TOS가 CALL) {
      TOS pop;
      return;
    }

    // ─── 수렴점 도달 확인 ───
    if (next_pc == top_recvg_pc) continue;  // 이 경로는 수렴점 → 스킵

    // ─── 분기 처리 ───
    if (num_paths > 1 && !warp_diverged) {
      warp_diverged = true;
      // TOS를 수렴 엔트리로 변환:
      m_stack.back().m_pc = recvg_pc;  // PC를 수렴점으로
      m_stack.push_back(empty);        // 새 엔트리를 위한 공간
    }

    if (warp_diverged && next_pc == new_recvg_pc) continue;  // 수렴점 스킵

    // ─── TOS에 경로 정보 설정 ───
    m_stack.back().m_pc = tmp_next_pc;
    m_stack.back().m_active_mask = tmp_active_mask;
    m_stack.back().m_recvg_pc = warp_diverged ? new_recvg_pc : top_recvg_pc;

    m_stack.push_back(empty);  // 다음 경로를 위한 공간
  }

  m_stack.pop_back();  // 마지막 빈 엔트리 제거

  if (warp_diverged)
    통계 기록: ptx_file_line_stats_add_warp_divergence(top_pc, 1);
}
```

알고리즘의 핵심 아이디어:
1. **not-taken 경로를 먼저 push** → taken 경로가 TOS에 위치 → taken이 먼저 실행됨
2. **수렴 엔트리**: 분기 발생 시 기존 TOS를 수렴 PC로 변환하여 스택 중간에 배치
3. **수렴점 도달**: 어떤 경로의 다음 PC가 `recvg_pc`와 같으면, 별도 스택 엔트리 불필요 (자동으로 수렴 엔트리에 합류)
4. **CALL/RET**: 함수 호출/반환은 분기가 아닌 별도 메커니즘으로 처리. CALL은 `STACK_ENTRY_TYPE_CALL`로 push하고, RET에서 이를 pop한다.

---

이 문서는 GPGPU-Sim의 Part 1로, 전체 아키텍처와 SIMT 코어 파이프라인을 다루었다. Part 2에서는 메모리 시스템(L1/L2 캐시, DRAM, 인터커넥트), Part 3에서는 전력 모델(AccelWattch)과 설정 파라미터를 다룰 예정이다.

---

**소스 파일 참조 위치** (이 문서에서 분석한 핵심 소스):
- `libcuda/cuda_runtime_api.cc` — CUDA API 인터셉션
- `libcuda/gpgpu_context.h` — 전역 컨텍스트 싱글턴
- `src/gpgpusim_entrypoint.cc` — 시뮬레이션 스레드
- `src/stream_manager.cc` — 스트림/비동기 연산 관리
- `src/gpgpu-sim/gpu-sim.cc` — 메인 시뮬레이션 루프 (`cycle()`, `issue_block2core()`)
- `src/gpgpu-sim/shader.cc` — SM 파이프라인 (`fetch()` ~ `writeback()`)
- `src/gpgpu-sim/shader.h` — 워프 스케줄러, 실행 유닛, 오퍼랜드 컬렉터 클래스
- `src/gpgpu-sim/scoreboard.cc/h` — 레지스터 해저드 감지
- `src/gpgpu-sim/stack.cc/h` — SIMT 재수렴 스택 유틸리티
- `src/abstract_hardware_model.cc/h` — `kernel_info_t`, `simt_stack`, 메모리 공간 정의
