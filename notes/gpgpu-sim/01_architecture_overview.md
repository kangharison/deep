# GPGPU-Sim 아키텍처 전체 개요

> 분석 대상: `/home/ubuntu/deep/sources/gpgpu-sim_distribution/`
> GPGPU-Sim은 NVIDIA GPU의 마이크로아키텍처를 사이클 레벨로 시뮬레이션하는 학술 연구용 시뮬레이터이다.

---

## 1. 전체 아키텍처 개요

### 1.1 시스템 전체 구조 (ASCII Diagram)

```
 ┌─────────────────────────────────────────────────────────────────────────┐
 │                    CUDA Application (사용자 프로그램)                   │
 │                  cudaMalloc, cudaMemcpy, kernel<<<>>>()                 │
 └─────────────────────────────┬───────────────────────────────────────────┘
                               │ LD_PRELOAD 또는 링크 시점 인터셉션
 ┌─────────────────────────────▼───────────────────────────────────────────┐
 │                     libcuda/ (CUDA API 인터셉션 계층)                   │
 │  cuda_runtime_api.cc ─── CUDA Runtime API 구현체 (가로채기)             │
 │  gpgpu_context.h ──────── 글로벌 시뮬레이터 컨텍스트 관리               │
 │  cuda_api_object.h ────── CUDA 오브젝트 래핑 (device, context 등)       │
 └────────────┬─────────────────────┬──────────────────────────────────────┘
              │                     │
              ▼                     ▼
 ┌────────────────────┐  ┌─────────────────────────────────────────────────┐
 │  stream_manager    │  │         src/gpgpusim_entrypoint.cc              │
 │  (스트림 관리자)    │  │  - 시뮬레이션 스레드 생성/관리                  │
 │  - stream_zero     │  │  - sequential / concurrent 모드                 │
 │  - 다중 스트림     │  │  - 메인 루프: init() → cycle() → deadlock_check │
 │  - 커널 디스패치   │  │  - SST 연동 인터페이스                          │
 └────────┬───────────┘  └──────────────────┬──────────────────────────────┘
          │                                 │
          └──────────────┬──────────────────┘
                         ▼
 ┌─────────────────────────────────────────────────────────────────────────┐
 │                  gpgpu_sim (GPU 시뮬레이터 코어 엔진)                   │
 │                    src/gpgpu-sim/gpu-sim.cc/.h                          │
 │                                                                         │
 │  ┌─────────────────────────────────────────────────────────────────┐    │
 │  │                     Clock Domain Manager                        │    │
 │  │  next_clock_domain() → CORE | ICNT | L2 | DRAM 마스크 반환     │    │
 │  └─────────────────────────────────────────────────────────────────┘    │
 │                                                                         │
 │  ┌─────────────┐  ┌──────────────┐  ┌──────────┐  ┌──────────────┐    │
 │  │ SIMT Cluster│  │ Interconnect │  │ L2 Cache │  │   DRAM       │    │
 │  │  Array      │  │   Network    │  │ Partition│  │  Controller  │    │
 │  │ (m_cluster) │  │  (ICNT)      │  │          │  │              │    │
 │  │             │  │              │  │          │  │              │    │
 │  │ ┌─────────┐│  │  icnt_push() │  │ cache_   │  │ dram_cycle() │    │
 │  │ │Shader   ││◄─┤  icnt_pop()  ├─►│ cycle()  ├─►│              │    │
 │  │ │Core 0   ││  │  icnt_       │  │          │  │ FIFO/FRFCFS  │    │
 │  │ ├─────────┤│  │  transfer()  │  │ push()   │  │ scheduling   │    │
 │  │ │Shader   ││  │              │  │ pop()    │  │              │    │
 │  │ │Core 1   ││  │              │  │ top()    │  │              │    │
 │  │ ├─────────┤│  │              │  │          │  │              │    │
 │  │ │  ...    ││  │              │  │          │  │              │    │
 │  │ └─────────┘│  │              │  │          │  │              │    │
 │  └─────────────┘  └──────────────┘  └──────────┘  └──────────────┘    │
 │                                                                         │
 │  ┌─────────────────────────────────────────────────────────────────┐    │
 │  │                 Kernel Management                               │    │
 │  │  m_running_kernels[]  launch()  select_kernel()  finished()     │    │
 │  └─────────────────────────────────────────────────────────────────┘    │
 └─────────────────────────────────────────────────────────────────────────┘

 ┌─────────────────────────────────────────────────────────────────────────┐
 │                  CUDA Functional Simulation (cuda-sim/)                 │
 │  PTX 파싱 → 명령어 디코딩 → 기능적 실행 (warp 단위)                    │
 │  ptx_ir.cc/h, cuda-sim.cc/h, instructions.cc, memory.cc               │
 └─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 핵심 모듈 간 데이터 흐름

```
  CUDA App
    │ cudaLaunch()
    ▼
  libcuda: kernel_config 생성 → kernel_info_t 생성
    │
    ▼
  stream_manager::push(stream_kernel_launch)
    │
    ▼
  stream_manager::operation() → do_operation() → gpu->launch(kernel)
    │
    ▼
  gpgpu_sim::launch() → m_running_kernels[]에 등록
    │
    ▼
  gpgpu_sim::cycle() [매 사이클 반복]
    │
    ├──► CORE clock: m_cluster[i]->icnt_cycle()  (ICNT→Core 응답 수신)
    │                m_cluster[i]->core_cycle()  (파이프라인 실행)
    │                issue_block2core()           (CTA 배정)
    │
    ├──► ICNT clock: mem_sub_partition→ICNT 응답 전송
    │                icnt_transfer()             (네트워크 시뮬레이션)
    │
    ├──► L2 clock:   ICNT→mem_sub_partition 요청 수신
    │                cache_cycle()               (L2 캐시 처리)
    │
    └──► DRAM clock: dram_cycle()                (DRAM 스케줄링 및 타이밍)
```

---

## 2. 디렉토리 구조 및 각 모듈의 역할

```
gpgpu-sim_distribution/
├── libcuda/                    ← CUDA Runtime API 인터셉션 계층
│   ├── cuda_runtime_api.cc     ← cudaMalloc, cudaMemcpy, cudaLaunch 등 구현
│   ├── gpgpu_context.h         ← 글로벌 시뮬레이터 컨텍스트 (gpgpu_context 클래스)
│   └── cuda_api_object.h       ← _cuda_device_id, CUctx_st 등 CUDA 오브젝트
│
├── src/
│   ├── gpgpusim_entrypoint.cc  ← 시뮬레이션 스레드 진입점, 메인 루프
│   ├── gpgpusim_entrypoint.h   ← GPGPUsim_ctx 클래스 (세마포어, GPU 포인터)
│   ├── stream_manager.cc/h     ← 스트림 관리, 커널/memcpy 오퍼레이션 큐
│   ├── abstract_hardware_model.cc/h ← 하드웨어 추상화 (kernel_info_t, warp_inst_t 등)
│   ├── option_parser.cc/h      ← 설정 파일 파싱 프레임워크
│   │
│   ├── gpgpu-sim/              ← 성능 시뮬레이션 핵심 모듈
│   │   ├── gpu-sim.cc/h        ← gpgpu_sim 클래스 (메인 시뮬레이터 엔진)
│   │   ├── shader.cc/h         ← Shader Core 파이프라인 (fetch, decode, issue, execute, writeback)
│   │   ├── gpu-cache.cc/h      ← L1/L2 캐시 모델 (data cache, texture cache, const cache)
│   │   ├── dram.cc/h           ← DRAM 타이밍 모델 (bank, row buffer 등)
│   │   ├── dram_sched.cc/h     ← DRAM 스케줄러 (FIFO, FR-FCFS)
│   │   ├── mem_fetch.cc/h      ← 메모리 요청 패킷 (mem_fetch 클래스)
│   │   ├── l2cache.cc/h        ← L2 캐시 및 memory_partition_unit
│   │   ├── icnt_wrapper.cc/h   ← 인터커넥트 네트워크 래퍼
│   │   ├── addrdec.cc/h        ← 주소 디코딩 (chip/bank/row/col 매핑)
│   │   ├── scoreboard.cc/h     ← 레지스터 스코어보드 (WAR/WAW 해저드 탐지)
│   │   ├── stack.cc/h          ← SIMT 스택 (분기 관리)
│   │   ├── local_interconnect.cc/h ← SM 내부 로컬 인터커넥트
│   │   └── power_stat.cc/h     ← 전력 통계 수집
│   │
│   ├── cuda-sim/               ← CUDA 기능적(functional) 시뮬레이션
│   │   ├── cuda-sim.cc/h       ← PTX 기능적 실행 엔진
│   │   ├── ptx_ir.cc/h         ← PTX 중간 표현 (IR)
│   │   ├── ptx_sim.cc/h        ← PTX 명령어별 시뮬레이션
│   │   ├── instructions.cc     ← PTX 명령어 구현 (add, mul, ld, st 등)
│   │   ├── memory.cc/h         ← 메모리 공간 관리 (global, shared, local)
│   │   ├── ptx_loader.cc/h     ← PTX 바이너리 로딩
│   │   ├── ptx_parser.cc/h     ← PTX 텍스트 파싱 (flex/bison)
│   │   └── cuda_device_runtime.cc/h ← CUDA Dynamic Parallelism (CDP) 지원
│   │
│   ├── intersim2/              ← 인터커넥트 네트워크 시뮬레이터 (BookSim 기반)
│   └── accelwattch/            ← AccelWattch 전력 모델
│
├── configs/                    ← GPU 설정 프로파일 (SM75_RTX2060 등)
│   └── tested-cfgs/
│       ├── SM7_QV100/          ← Volta V100 설정
│       ├── SM75_RTX2060/       ← Turing RTX 2060 설정
│       └── ...
│
└── setup_environment           ← 환경 변수 설정 스크립트
```

---

## 3. 시뮬레이터 초기화 흐름

CUDA 애플리케이션의 첫 번째 CUDA API 호출 시, 시뮬레이터가 초기화된다. 전체 초기화 흐름은 다음과 같다.

### 3.1 초기화 호출 체인

```
CUDA App: cudaMalloc() 또는 cudaGetDeviceCount() 등 최초 호출
    │
    ▼
GPGPU_Context()                          [cuda_runtime_api.cc:260]
    │  static gpgpu_context를 싱글턴으로 생성
    │  gpgpu_context 생성자가 다음 서브시스템 초기화:
    │    - cuda_runtime_api (api)
    │    - ptxinfo_data (ptxinfo)
    │    - ptx_recognizer (ptx_parser)
    │    - GPGPUsim_ctx (the_gpgpusim)
    │    - cuda_sim (func_sim)
    │    - cuda_device_runtime (device_runtime)
    │    - ptx_stats (stats)
    ▼
GPGPUSim_Context(ctx)                    [cuda_runtime_api.cc:249]
    │
    ▼
ctx->GPGPUSim_Init()                     [cuda_runtime_api.cc:197]
    │
    ├──► gpgpu_ptx_sim_init_perf()       [gpgpusim_entrypoint.cc:304]
    │       │
    │       ├── print_splash()           ← 배너 출력
    │       ├── read_sim_environment_variables()
    │       ├── option_parser_create()   ← 옵션 파서 생성
    │       │
    │       ├── ptx_reg_options(opp)     ← PTX 관련 옵션 등록
    │       ├── icnt_reg_options(opp)    ← 인터커넥트 옵션 등록
    │       ├── g_the_gpu_config->reg_options(opp)  ← GPU 마이크로아키텍처 옵션 등록
    │       │     ├── m_shader_config.reg_options()  ← Shader Core 설정
    │       │     ├── m_memory_config.reg_options()  ← 메모리 시스템 설정
    │       │     └── power_config::reg_options()    ← 전력 모델 설정
    │       │
    │       ├── option_parser_cmdline(opp, sg_argc, sg_argv)
    │       │     → "-config" "gpgpusim.config" 파싱
    │       │
    │       ├── g_the_gpu_config->init()  ← 설정 값 초기화
    │       │     ├── m_shader_config.init()
    │       │     ├── m_memory_config.init()
    │       │     ├── init_clock_domains()   ← core/icnt/l2/dram 주파수 설정
    │       │     └── power_config::init()
    │       │
    │       ├── new exec_gpgpu_sim(config, ctx)  ← GPU 시뮬레이터 객체 생성
    │       │     └── gpgpu_sim 생성자:
    │       │         ├── shader_core_stats 생성
    │       │         ├── memory_stats_t 생성
    │       │         ├── power_stat_t 생성
    │       │         ├── memory_partition_unit[n_mem] 생성  ← DRAM 파티션
    │       │         │     └── memory_sub_partition 연결
    │       │         ├── icnt_wrapper_init() + icnt_create() ← 인터커넥트 초기화
    │       │         ├── m_running_kernels 벡터 초기화
    │       │         └── createSIMTCluster()  ← SIMT 클러스터 배열 생성
    │       │
    │       ├── new stream_manager(g_the_gpu, cuda_launch_blocking)
    │       │
    │       └── 세마포어 초기화 (g_sim_signal_start/finish/exit)
    │
    ├──► cudaDeviceProp 설정 (이름, 스레드 수, 공유 메모리 등)
    │
    └──► start_sim_thread(1)             [gpgpusim_entrypoint.cc:348]
            │
            └── pthread_create(gpgpu_sim_thread_concurrent)
                  ← 시뮬레이션 전용 스레드 생성
```

### 3.2 gpgpu_sim 생성자 상세 (`gpu-sim.cc:971`)

```cpp
gpgpu_sim::gpgpu_sim(const gpgpu_sim_config &config, gpgpu_context *ctx)
    : gpgpu_t(config, ctx),   // 부모 클래스: gpu_sim_cycle, gpu_tot_sim_cycle, 메모리 공간 초기화
      m_config(config)
{
    // 1. 설정 포인터 연결
    m_shader_config = &m_config.m_shader_config;   // Shader Core 설정
    m_memory_config = &m_config.m_memory_config;   // 메모리 시스템 설정

    // 2. 통계 객체 생성
    m_shader_stats = new shader_core_stats(m_shader_config);
    m_memory_stats = new memory_stats_t(...);
    m_power_stats  = new power_stat_t(...);

    // 3. 카운터 초기화
    gpu_sim_insn = 0;
    gpu_tot_sim_insn = 0;
    gpu_tot_issued_cta = 0;
    gpu_deadlock = false;

    // 4. 메모리 시스템 생성 (SST 모드가 아닌 경우)
    m_memory_partition_unit = new memory_partition_unit*[m_n_mem];
    for (i = 0; i < m_n_mem; i++) {
        m_memory_partition_unit[i] = new memory_partition_unit(i, ...);
        // 각 파티션의 서브파티션을 m_memory_sub_partition[]에 연결
    }

    // 5. 인터커넥트 생성
    icnt_wrapper_init();
    icnt_create(n_simt_clusters, n_mem_sub_partition);

    // 6. 커널 관리 초기화
    m_running_kernels.resize(max_concurrent_kernel, NULL);
    m_last_issued_kernel = 0;
}
```

---

## 4. 시뮬레이션 메인 루프 (cycle() 함수) 상세 분석

### 4.1 시뮬레이션 스레드 진입점

GPGPU-Sim은 별도 pthread에서 시뮬레이션을 수행한다. 두 가지 모드가 있다.

**Sequential 모드** (`gpgpu_sim_thread_sequential`, `gpgpusim_entrypoint.cc:61`):
```
한 번에 하나의 커널만 실행한다.
sem_wait(g_sim_signal_start)  ← 호스트가 커널 제출할 때까지 대기
    → gpu->init()
    → while(gpu->active()) { gpu->cycle(); gpu->deadlock_check(); }
    → gpu->print_stats()
sem_post(g_sim_signal_finish) ← 호스트에게 완료 알림
```

**Concurrent 모드** (`gpgpu_sim_thread_concurrent`, `gpgpusim_entrypoint.cc:91`):
```
여러 커널을 동시에 실행할 수 있다. stream_manager가 작업을 관리한다.
while (!g_sim_done) {
    스트림에 작업이 들어올 때까지 스핀 대기
    gpu->init()
    do {
        stream_manager->operation()  ← 커널 완료 체크 + 새 커널 디스패치
        if (functional_sim) → PTX 기능적 실행
        if (gpu->active()) → gpu->cycle() + deadlock_check
    } while (active && !g_sim_done)
    print_stats()
}
```

### 4.2 cycle() 함수 라인별 분석 (`gpu-sim.cc:1970-2210`)

`cycle()`은 GPU의 한 사이클을 시뮬레이션하는 핵심 함수이다. 4개의 클록 도메인(CORE, ICNT, L2, DRAM)을 독립적으로 관리한다.

```cpp
void gpgpu_sim::cycle() {
    // ========== 1단계: 클록 도메인 결정 ==========
    int clock_mask = next_clock_domain();
    // next_clock_domain()은 각 도메인의 다음 rising edge 시간을 비교하여
    // 이번 사이클에 어떤 도메인이 틱(tick)해야 하는지 비트마스크로 반환한다.
    // CORE=0x01, L2=0x02, DRAM=0x04, ICNT=0x08
    //
    // 동작 원리:
    //   smallest = min(core_time, icnt_time, dram_time, l2_time)
    //   해당 시간에 도달한 도메인의 비트를 mask에 OR
    //   해당 도메인의 time += period  (다음 rising edge 계산)
    //
    // 이를 통해 서로 다른 주파수의 클록 도메인을 정확하게 시뮬레이션한다.
    // 예: Core=1.4GHz, ICNT=1.4GHz, L2=1.4GHz, DRAM=850MHz

    // ========== 2단계: CORE 클록 - ICNT에서 Shader Core로 응답 수신 ==========
    if (clock_mask & CORE) {
        for (i = 0; i < n_simt_clusters; i++)
            m_cluster[i]->icnt_cycle();
        // 각 SIMT 클러스터가 인터커넥트에서 메모리 응답(mem_fetch)을 pop하여
        // 해당 shader core의 응답 큐에 전달한다.
    }

    // ========== 3단계: ICNT 클록 - Memory → ICNT 응답 전송 ==========
    if (clock_mask & ICNT) {
        for (i = 0; i < n_mem_sub_partition; i++) {
            mem_fetch *mf = m_memory_sub_partition[i]->top();
            if (mf) {
                // 메모리 서브파티션 출력 큐의 맨 앞 요청을 인터커넥트로 push
                // 읽기 응답은 데이터 크기, 쓰기 응답은 컨트롤 크기만큼
                unsigned response_size = mf->get_is_write() ?
                    mf->get_ctrl_size() : mf->size();
                if (icnt_has_buffer(mem2device(i), response_size)) {
                    mf->set_return_timestamp(gpu_sim_cycle + gpu_tot_sim_cycle);
                    mf->set_status(IN_ICNT_TO_SHADER, ...);
                    icnt_push(mem2device(i), mf->get_tpc(), mf, response_size);
                    m_memory_sub_partition[i]->pop();
                } else {
                    gpu_stall_icnt2sh++;  // 인터커넥트 버퍼 풀 → stall 카운트
                }
            }
        }
    }

    // ========== 4단계: DRAM 클록 - DRAM 커맨드 스케줄링 ==========
    if (clock_mask & DRAM) {
        for (i = 0; i < n_mem; i++) {
            if (simple_dram_model)
                m_memory_partition_unit[i]->simple_dram_model_cycle();
            else
                m_memory_partition_unit[i]->dram_cycle();
            // DRAM 스케줄러 실행 (FIFO 또는 FR-FCFS)
            // row buffer hit/miss 판단, bank 충돌 처리
            // tRCD, tRAS, tRP 등 DRAM 타이밍 파라미터 적용
            //
            // 전력 통계 업데이트: n_cmd, n_activity, n_act, n_pre, n_rd, n_wr
        }
    }

    // ========== 5단계: L2 클록 - ICNT에서 L2/Memory로 요청 수신 ==========
    if (clock_mask & L2) {
        for (i = 0; i < n_mem_sub_partition; i++) {
            // 5a. 인터커넥트에서 요청 pop → 서브파티션으로 push
            if (m_memory_sub_partition[i]->full(SECTOR_CHUNCK_SIZE)) {
                gpu_stall_dramfull++;  // 서브파티션 버퍼 풀 → stall
            } else {
                mem_fetch *mf = icnt_pop(mem2device(i));
                m_memory_sub_partition[i]->push(mf, cycle);
            }

            // 5b. L2 캐시 사이클 실행
            m_memory_sub_partition[i]->cache_cycle(cycle);
            // L2 캐시 룩업, MSHR 관리, fill/writeback 처리
        }
    }

    // ========== 6단계: ICNT 내부 전송 ==========
    if (clock_mask & ICNT) {
        icnt_transfer();
        // 인터커넥트 네트워크의 내부 라우팅/스위칭 시뮬레이션
        // BookSim2 기반 또는 간단한 fixed-latency 모델
    }

    // ========== 7단계: CORE 클록 - Shader Core 파이프라인 실행 ==========
    if (clock_mask & CORE) {
        for (i = 0; i < n_simt_clusters; i++) {
            if (m_cluster[i]->get_not_completed() || get_more_cta_left()) {
                m_cluster[i]->core_cycle();
                // 각 SIMT 클러스터 내의 모든 shader core에 대해:
                //   fetch → decode → issue → execute → writeback
                // 파이프라인 스테이지를 한 사이클씩 진행한다.
            }
        }

        // 파이프라인 duty cycle 통계
        for (i = 0; i < num_shader(); i++)
            temp += m_pipeline_duty_cycle[i];
        *average_pipeline_duty_cycle += temp / num_shader();

        gpu_sim_cycle++;  // ★ 코어 사이클 카운터 증가

        // ========== 8단계: CTA 블록 배정 ==========
        issue_block2core();
        // 라운드 로빈으로 각 SIMT 클러스터에 새 CTA(thread block)를 배정한다.
        // 클러스터가 새 CTA를 받을 수 있는지 (레지스터, 공유메모리, 워프 슬롯) 확인 후 배정.

        decrement_kernel_latency();
        // 커널 실행 대기 레이턴시 감소

        // ========== 9단계: 캐시 플러시 (설정에 따라) ==========
        if (gpgpu_flush_l1_cache) → 모든 스레드 완료 시 L1 무효화
        if (gpgpu_flush_l2_cache) → 모든 스레드 완료 시 L2 플러시

        // ========== 10단계: 주기적 통계 수집 ==========
        if (!(gpu_sim_cycle % gpu_stat_sample_freq))
            // 런타임 통계 출력 (DRAM 대역폭, L1 미스율 등)
            // Liveness 메시지 (IPC, occupancy, 경과 시간)

        // ========== 11단계: 데드락 감지 ==========
        if (!(gpu_sim_cycle % 50000))
            if (gpu_sim_insn == last_gpu_sim_insn)
                gpu_deadlock = true;  // 50000 사이클 동안 명령어 실행 없으면 데드락

        // ========== 12단계: CDP 디바이스 커널 런칭 ==========
        gpgpu_ctx->device_runtime->launch_one_device_kernel();
    }
}
```

### 4.3 next_clock_domain() 클록 도메인 관리 (`gpu-sim.cc:1932`)

```cpp
int gpgpu_sim::next_clock_domain(void) {
    double smallest = min3(core_time, icnt_time, dram_time);
    int mask = 0x00;

    // 가장 빠른 rising edge를 찾아서 해당 도메인들을 활성화
    if (l2_time <= smallest) {
        smallest = l2_time;
        mask |= L2;              // L2 도메인 활성화
        l2_time += m_config.l2_period;    // 다음 L2 rising edge
    }
    if (icnt_time <= smallest) {
        mask |= ICNT;
        icnt_time += m_config.icnt_period;
    }
    if (dram_time <= smallest) {
        mask |= DRAM;
        dram_time += m_config.dram_period;
    }
    if (core_time <= smallest) {
        mask |= CORE;
        core_time += m_config.core_period;
    }
    return mask;
}
```

이 함수를 통해 서로 다른 주파수로 동작하는 클록 도메인을 비트마스크로 관리한다. 예를 들어 Core가 1400MHz이고 DRAM이 850MHz이면, Core는 DRAM보다 약 1.65배 더 자주 tick한다.

### 4.4 issue_block2core() CTA 배정 (`gpu-sim.cc:1955`)

```cpp
void gpgpu_sim::issue_block2core() {
    unsigned last_issued = m_last_cluster_issue;
    for (i = 0; i < n_simt_clusters; i++) {
        // 라운드 로빈: 마지막으로 배정한 클러스터의 다음부터 시작
        unsigned idx = (i + last_issued + 1) % n_simt_clusters;
        unsigned num = m_cluster[idx]->issue_block2core();
        if (num) {
            m_last_cluster_issue = idx;
            m_total_cta_launched += num;
        }
    }
}
```

---

## 5. gpgpu_sim 클래스의 주요 멤버와 역할

`gpgpu_sim`은 GPU 시뮬레이터의 중심 클래스이다. 상속 구조: `gpgpu_t` → `gpgpu_sim` → `exec_gpgpu_sim` (또는 `sst_gpgpu_sim`)

### 5.1 클래스 계층 구조

```
gpgpu_t (abstract_hardware_model.h:587)
  │  - gpu_sim_cycle, gpu_tot_sim_cycle (사이클 카운터)
  │  - m_global_mem, m_tex_mem, m_surf_mem (메모리 공간)
  │  - gpu_malloc(), memcpy_to_gpu(), memcpy_from_gpu()
  │
  └── gpgpu_sim (gpu-sim.h:575)
        │  - m_config (gpgpu_sim_config: 전체 설정)
        │  - m_cluster[] (simt_core_cluster 배열: SM 클러스터)
        │  - m_memory_partition_unit[] (DRAM 파티션)
        │  - m_memory_sub_partition[] (L2 + DRAM 서브파티션)
        │  - m_running_kernels[] (현재 실행 중인 커널 목록)
        │  - cycle(), init(), active(), launch() 등 핵심 메서드
        │
        ├── exec_gpgpu_sim (gpu-sim.h:807)
        │     일반 시뮬레이션 모드. createSIMTCluster() 구현.
        │
        └── sst_gpgpu_sim (gpu-sim.h:821)
              SST(Structural Simulation Toolkit) 연동 모드.
              SST 메모리 백엔드 사용, icnt/dram 시뮬레이션 건너뜀.
```

### 5.2 주요 멤버 변수

| 멤버 | 타입 | 역할 |
|------|------|------|
| `m_config` | `gpgpu_sim_config &` | 전체 GPU 설정 (const ref) |
| `m_shader_config` | `shader_core_config *` | Shader Core 설정 포인터 |
| `m_memory_config` | `memory_config *` | 메모리 시스템 설정 포인터 |
| `m_cluster[]` | `simt_core_cluster **` | SIMT 클러스터 배열 (각 클러스터에 여러 SM 포함) |
| `m_memory_partition_unit[]` | `memory_partition_unit **` | DRAM 메모리 파티션 배열 |
| `m_memory_sub_partition[]` | `memory_sub_partition **` | L2 캐시 + DRAM 서브파티션 |
| `m_running_kernels` | `vector<kernel_info_t *>` | 현재 실행 중인 커널 슬롯 (max_concurrent_kernel 크기) |
| `m_last_issued_kernel` | `unsigned` | 마지막으로 배정한 커널 인덱스 |
| `m_last_cluster_issue` | `unsigned` | 마지막으로 CTA를 배정한 클러스터 인덱스 |
| `gpu_sim_insn` | `unsigned long long` | 현재 커널의 실행 명령어 수 |
| `gpu_tot_sim_insn` | `unsigned long long` | 누적 실행 명령어 수 |
| `gpu_sim_cycle` | `unsigned long long` | 현재 커널의 사이클 수 (gpgpu_t에서 상속) |
| `gpu_tot_sim_cycle` | `unsigned long long` | 누적 사이클 수 (gpgpu_t에서 상속) |
| `core_time, icnt_time, dram_time, l2_time` | `double` | 각 클록 도메인의 다음 rising edge 시간 |
| `m_shader_stats` | `shader_core_stats *` | Shader Core 통계 |
| `m_memory_stats` | `memory_stats_t *` | 메모리 시스템 통계 |
| `m_power_stats` | `power_stat_t *` | 전력 통계 |
| `gpu_stall_dramfull` | `unsigned int` | DRAM 버퍼 풀로 인한 stall 횟수 |
| `gpu_stall_icnt2sh` | `unsigned int` | ICNT→Shader 버퍼 풀 stall 횟수 |

### 5.3 주요 메서드

| 메서드 | 역할 |
|--------|------|
| `init()` | 시뮬레이션 시작 전 카운터 리셋, 클러스터 reinit, 클록 도메인 초기화 |
| `cycle()` | 한 사이클 시뮬레이션 (4개 클록 도메인 처리) |
| `active()` | GPU가 아직 활성 상태인지 확인 (미완료 워프, busy 메모리, 남은 CTA 체크) |
| `launch(kernel_info_t*)` | 새 커널을 m_running_kernels에 등록 |
| `select_kernel()` | 실행할 커널 선택 (CTA가 남은 커널 중 선택) |
| `issue_block2core()` | 라운드 로빈으로 CTA를 SIMT 클러스터에 배정 |
| `finished_kernel()` | 완료된 커널의 UID 반환 |
| `deadlock_check()` | 50000 사이클 동안 진행 없으면 데드락 선언 |
| `print_stats()` | 시뮬레이션 통계 출력 |

---

## 6. CUDA API 인터셉션 메커니즘

### 6.1 인터셉션 원리

GPGPU-Sim은 `libcuda`라는 공유 라이브러리를 제공한다. 이 라이브러리는 NVIDIA의 실제 `libcudart.so`와 동일한 함수 심볼을 export한다. 빌드 시 CUDA 애플리케이션이 GPGPU-Sim의 `libcudart.so`에 링크되면, 실제 GPU 대신 시뮬레이터에서 실행된다.

```
실제 CUDA:     App → libcudart.so → libcuda.so → GPU Driver → GPU Hardware
GPGPU-Sim:     App → libcudart.so (GPGPU-Sim) → Simulator Engine
```

환경 변수 `LD_LIBRARY_PATH`를 GPGPU-Sim의 lib 디렉토리로 설정하면, 동적 링커가 GPGPU-Sim의 라이브러리를 먼저 찾아 로딩한다.

### 6.2 주요 인터셉션 함수 (`libcuda/cuda_runtime_api.cc`)

**메모리 관리:**
```
cudaMalloc()       → gpu_malloc()으로 시뮬레이터 메모리 공간에 할당
cudaMemcpy()       → stream_manager에 memcpy 오퍼레이션 push
                     H2D: memcpy_to_gpu()
                     D2H: memcpy_from_gpu()
cudaFree()         → 시뮬레이터 메모리 해제
```

**커널 실행:**
```
cudaConfigureCall(gridDim, blockDim, sharedMem, stream)  [line 874]
    → g_cuda_launch_stack에 kernel_config push
    → gridDim, blockDim, sharedMem, stream 정보 저장

cudaSetupArgument(arg, size, offset)  [line 945]
    → g_cuda_launch_stack.back()의 kernel_config에 인자 추가

cudaLaunch(hostFun)  [line 969]
    → GPGPUSim_Context(ctx) 로 시뮬레이터 초기화 확인
    → PTX 파싱 및 function_info 조회
    → kernel_info_t 생성 (gridDim, blockDim, function_info)
    → stream_operation(kernel, sim_mode, stream) 생성
    → stream_manager->push(op) 로 실행 큐에 등록
    → 시뮬레이션 스레드가 이 커널을 감지하고 실행 시작
```

**CUDA 11+ 통합 API:**
```
cudaLaunchKernel(hostFun, gridDim, blockDim, args, sharedMem, stream)  [line 2031]
    → cudaConfigureCallInternal() + cudaSetupArgument() + cudaLaunchInternal()
    → 위와 동일한 흐름으로 처리
```

### 6.3 컨텍스트 관리 (`libcuda/gpgpu_context.h`)

`gpgpu_context`는 시뮬레이터의 전역 싱글턴 컨텍스트이다.

```cpp
class gpgpu_context {
    // UID 관리 (각 객체 타입별 고유 ID)
    unsigned sm_next_access_uid;         // 메모리 접근 UID
    unsigned warp_inst_sm_next_uid;      // warp 명령어 UID
    unsigned kernel_info_m_next_uid;     // 커널 UID (1부터 시작)

    // 서브시스템 포인터
    cuda_runtime_api *api;        // CUDA API 인터셉션 계층
    ptx_recognizer *ptx_parser;   // PTX 파서
    GPGPUsim_ctx *the_gpgpusim;   // 시뮬레이터 코어 컨텍스트
    cuda_sim *func_sim;           // 기능적 시뮬레이션 엔진
    cuda_device_runtime *device_runtime;  // CDP 지원
    ptx_stats *stats;             // PTX 통계

    // 핵심 메서드
    gpgpu_sim *gpgpu_ptx_sim_init_perf();  // 성능 시뮬레이터 초기화
    void start_sim_thread(int api);         // 시뮬레이션 스레드 시작
    GPGPUSim_Init();                        // GPU 디바이스 초기화
    synchronize();                          // cudaDeviceSynchronize 구현
};
```

`GPGPUsim_ctx` (`gpgpusim_entrypoint.h:40`)는 시뮬레이션 스레드의 동기화와 GPU 객체를 관리한다:
```cpp
class GPGPUsim_ctx {
    sem_t g_sim_signal_start;    // 호스트→시뮬 "시작" 신호
    sem_t g_sim_signal_finish;   // 시뮬→호스트 "완료" 신호
    sem_t g_sim_signal_exit;     // 시뮬레이션 종료 신호

    gpgpu_sim_config *g_the_gpu_config;  // GPU 설정
    gpgpu_sim *g_the_gpu;                // GPU 시뮬레이터 인스턴스
    stream_manager *g_stream_manager;    // 스트림 관리자

    pthread_mutex_t g_sim_lock;  // 호스트-시뮬 스레드 동기화
    bool g_sim_active;           // 시뮬레이션 진행 중 플래그
    bool g_sim_done;             // 시뮬레이션 종료 플래그
};
```

### 6.4 GPGPU_Context() 싱글턴 (`cuda_runtime_api.cc:260`)

```cpp
gpgpu_context *GPGPU_Context() {
    static gpgpu_context *gpgpu_ctx = NULL;
    if (gpgpu_ctx == NULL) {
        gpgpu_ctx = new gpgpu_context();
    }
    return gpgpu_ctx;
}
```

최초 호출 시 전체 시뮬레이터 컨텍스트가 생성되며, 이후 동일한 인스턴스가 반환된다.

---

## 7. GPU 설정 파라미터 체계

### 7.1 설정 파일 로딩 경로

```
sgp_argv = {"", "-config", "gpgpusim.config"}
    ↓
option_parser_cmdline(opp, sg_argc, sg_argv)
    ↓
gpgpusim.config 파일을 현재 디렉토리에서 읽음
    ↓
각 "-옵션명 값" 쌍을 파싱하여 등록된 변수에 할당
```

설정 파일은 CUDA 애플리케이션의 **현재 작업 디렉토리(CWD)**에 있어야 한다. 일반적으로 `configs/tested-cfgs/` 아래의 프로파일을 심볼릭 링크하여 사용한다.

### 7.2 설정 계층 구조

```
gpgpu_sim_config (gpu-sim.h:408)
  ├── power_config                    ← 전력 시뮬레이션 설정
  ├── gpgpu_functional_sim_config     ← 기능적 시뮬레이션 설정
  ├── shader_core_config m_shader_config  ← Shader Core 설정
  │     ├── n_simt_clusters           ← SIMT 클러스터 수
  │     ├── n_simt_cores_per_cluster  ← 클러스터당 SM 수
  │     ├── warp_size                 ← 워프 크기 (32)
  │     ├── n_thread_per_shader       ← SM당 최대 스레드 수
  │     ├── max_warps_per_shader      ← SM당 최대 워프 수
  │     ├── num_sched_per_core        ← SM당 워프 스케줄러 수
  │     ├── m_L1I_config              ← L1 명령어 캐시 설정
  │     ├── m_L1D_config              ← L1 데이터 캐시 설정
  │     ├── m_L1T_config              ← L1 텍스처 캐시 설정
  │     ├── m_L1C_config              ← L1 상수 캐시 설정
  │     └── gpgpu_shmem_size          ← 공유 메모리 크기
  │
  └── memory_config m_memory_config       ← 메모리 시스템 설정
        ├── m_n_mem                       ← 메모리 컨트롤러 수
        ├── m_n_sub_partition_per_memory_channel ← 채널당 서브파티션 수
        ├── m_L2_config                   ← L2 캐시 설정
        ├── nbk, tCCD, tRRD, tRCD, tRAS, tRP, tRC ← DRAM 타이밍 파라미터
        ├── CL, WL, BL                   ← CAS latency, Write latency, Burst Length
        ├── scheduler_type               ← DRAM 스케줄러 (FIFO/FR-FCFS)
        └── icnt_flit_size               ← 인터커넥트 flit 크기
```

### 7.3 주요 설정 옵션 (`gpu-sim.cc:667 reg_options()`)

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `-gpgpu_max_cycle` | 0 | 최대 시뮬레이션 사이클 수 (0=무제한) |
| `-gpgpu_max_insn` | 0 | 최대 명령어 수 (0=무제한) |
| `-gpgpu_max_cta` | 0 | 최대 CTA 수 (0=무제한) |
| `-gpgpu_clock_domains` | 500:2000:2000:2000 | Core:ICNT:L2:DRAM 주파수(MHz) |
| `-gpgpu_max_concurrent_kernel` | 32 | 동시 실행 가능한 최대 커널 수 |
| `-gpgpu_compute_capability_major` | 7 | Compute Capability Major |
| `-gpgpu_compute_capability_minor` | 0 | Compute Capability Minor |
| `-gpgpu_flush_l1_cache` | 0 | 커널 종료 시 L1 캐시 플러시 |
| `-gpgpu_flush_l2_cache` | 0 | 커널 종료 시 L2 캐시 플러시 |
| `-gpgpu_deadlock_detect` | 1 | 데드락 감지 활성화 |
| `-gpgpu_ptx_sim_mode` | 0 | 0=성능 시뮬, 1=기능적 시뮬 |
| `-gpgpu_runtime_stat` | 10000:0 | 통계 샘플링 빈도:플래그 |
| `-gpgpu_kernel_launch_latency` | 0 | 커널 런칭 레이턴시 (사이클) |
| `-gpgpu_TB_launch_latency` | 0 | Thread Block 런칭 레이턴시 (사이클) |
| `-power_simulation_enabled` | 0 | AccelWattch 전력 시뮬레이션 on/off |

### 7.4 클록 도메인 초기화

설정에서 `-gpgpu_clock_domains "1400.0:1400.0:1400.0:850.0"` 같이 지정하면, `init_clock_domains()`가 각 도메인의 period를 계산한다:

```
core_freq  = 1400 MHz → core_period  = 1/1400e6
icnt_freq  = 1400 MHz → icnt_period  = 1/1400e6
l2_freq    = 1400 MHz → l2_period    = 1/1400e6
dram_freq  = 850 MHz  → dram_period  = 1/850e6
```

`reinit_clock_domains()`는 `init()` 때 모든 도메인 시간을 0으로 리셋한다.

---

## 8. 핵심 데이터 구조

### 8.1 kernel_info_t (`abstract_hardware_model.h:226`)

커널 실행 정보를 담는 구조체이다. CUDA 커널 한 번의 launch에 대한 모든 메타데이터를 관리한다.

```cpp
class kernel_info_t {
    // ─── 식별 정보 ───
    unsigned m_uid;                    // 커널 고유 ID (전역 증가)
    unsigned long long m_streamID;     // 소속 스트림 ID

    // ─── 그리드/블록 차원 ───
    dim3 m_grid_dim;                   // 그리드 차원 (예: {128, 1, 1})
    dim3 m_block_dim;                  // 블록 차원 (예: {256, 1, 1})
    dim3 m_next_cta;                   // 다음에 발행할 CTA의 3D 인덱스
    dim3 m_next_tid;                   // CTA 내 다음 스레드 인덱스

    // ─── 실행 상태 ───
    unsigned m_num_cores_running;      // 이 커널을 실행 중인 SM 수
    function_info *m_kernel_entry;     // PTX 함수 정보 (진입점)
    memory_space *m_param_mem;         // 커널 파라미터 메모리

    // ─── 타이밍 ───
    unsigned long long launch_cycle;   // 런칭 사이클
    unsigned long long start_cycle;    // 첫 CTA 실행 시작 사이클
    unsigned long long end_cycle;      // 마지막 CTA 완료 사이클
    unsigned m_launch_latency;         // 런칭 레이턴시 (설정 가능)

    // ─── CDP (Dynamic Parallelism) ───
    kernel_info_t *m_parent_kernel;    // 부모 커널 (없으면 NULL)
    list<kernel_info_t *> m_child_kernels;  // 자식 커널 목록

    // ─── 주요 메서드 ───
    num_blocks()           → 총 블록 수 = gridDim.x * y * z
    threads_per_cta()      → CTA당 스레드 수 = blockDim.x * y * z
    no_more_ctas_to_run()  → 모든 CTA가 발행되었는지 확인
    done()                 → no_more_ctas_to_run() && !running()
    increment_cta_id()     → 다음 CTA로 진행 (x→y→z 순서)
};
```

### 8.2 warp_inst_t (`abstract_hardware_model.h:1061`)

워프 단위 명령어를 나타내는 클래스이다. `inst_t`를 상속받아 워프 수준의 실행 정보를 추가한다.

```cpp
class inst_t {
    // ─── 명령어 기본 정보 ───
    address_type pc;                   // Program Counter
    unsigned isize;                    // 명령어 크기 (바이트)
    op_type op;                        // 연산 타입 (ALU_OP, LOAD_OP, STORE_OP 등)
    special_ops sp_op;                 // 세부 연산 (FP_MUL_OP, INT_DIV_OP 등)
    operation_pipeline op_pipe;        // 파이프라인 (SP, SFU, MEM, TENSOR_CORE 등)

    // ─── 레지스터 정보 ───
    unsigned out[8];                   // 출력 레지스터 번호
    unsigned in[24];                   // 입력 레지스터 번호
    struct { int dst[32]; int src[32]; } arch_reg;  // 아키텍처 레지스터

    // ─── 메모리 접근 정보 ───
    memory_space_t space;              // 메모리 공간 (global, shared, local 등)
    _memory_op_t memory_op;            // load/store 구분
    unsigned data_size;                // 데이터 크기
    cache_operator_type cache_op;      // 캐시 연산자 (.ca, .cg 등)

    // ─── 타이밍 ───
    unsigned latency;                  // 연산 레이턴시
    unsigned initiation_interval;      // 파이프라인 initiation interval
};

class warp_inst_t : public inst_t {
    // ─── 워프 실행 상태 ───
    unsigned m_uid;                    // 명령어 고유 ID
    unsigned long long m_streamID;     // 스트림 ID
    active_mask_t m_warp_active_mask;  // 활성 스레드 마스크 (32비트)
    unsigned m_warp_id;                // 워프 ID
    unsigned m_dynamic_warp_id;        // 동적 워프 ID

    // ─── 스레드별 메모리 주소 ───
    struct per_thread_info {
        new_addr_type memreqaddr[MAX_ACCESSES_PER_INSN_PER_THREAD];
        // 각 스레드의 메모리 접근 주소
    };
    vector<per_thread_info> m_per_scalar_thread;

    // ─── 메모리 접근 큐 ───
    list<mem_access_t> m_accessq;      // coalescing 결과로 생성된 메모리 접근 목록

    // ─── Coalescing 정보 ───
    struct transaction_info {
        bitset<4> chunks;              // 접근된 32바이트 청크 비트마스크
        mem_access_byte_mask_t bytes;  // 바이트 단위 접근 마스크
        active_mask_t active;          // 이 트랜잭션에 참여하는 스레드
    };

    // ─── 주요 메서드 ───
    issue(mask, warp_id, cycle, ...)   // 명령어 발행
    completed(cycle)                   // 완료 통계
    accessq_empty()                    // 메모리 접근 큐 비어있는지
    accessq_back()/front()/pop_back()  // 접근 큐 조작
};
```

### 8.3 mem_fetch (`gpgpu-sim/mem_fetch.h:54`)

인터커넥트와 메모리 시스템을 통해 이동하는 메모리 요청/응답 패킷이다.

```cpp
class mem_fetch {
    // ─── 요청 출처 ───
    unsigned m_request_uid;            // 요청 고유 ID (전역 증가)
    unsigned m_sid;                    // 소스 Shader Core ID
    unsigned m_tpc;                    // Texture Processing Cluster ID
    unsigned m_wid;                    // 워프 ID

    // ─── 상태 ───
    mem_fetch_status m_status;         // 현재 위치/상태
    // 가능한 상태:
    //   IN_SHADER_FETCHED, IN_SHADER_LDST_UNIT,
    //   IN_ICNT_TO_MEM, IN_PARTITION_ROP,
    //   IN_PARTITION_ICNT_TO_L2_QUEUE, IN_PARTITION_L2_TO_DRAM_QUEUE,
    //   IN_PARTITION_DRAM_LATENCY_QUEUE,
    //   IN_ICNT_TO_SHADER, ...

    // ─── 접근 정보 ───
    mem_access_t m_access;             // 접근 타입, 주소, 크기, 마스크
    unsigned m_data_size;              // 데이터 크기
    unsigned m_ctrl_size;              // 제어 메타데이터 크기
    new_addr_type m_partition_addr;    // DRAM 파티션 내 주소
    addrdec_t m_raw_addr;              // 디코딩된 DRAM 주소 (chip/bank/row/col)
    mf_type m_type;                    // READ_REQUEST, READ_REPLY, WRITE_REQUEST, WRITE_ACK

    // ─── 타이밍 통계 ───
    unsigned m_timestamp;              // 생성 시 사이클
    unsigned m_timestamp2;             // ICNT→Shader push 시 사이클
    unsigned m_icnt_receive_time;      // ICNT 수신 시 사이클

    // ─── 원본 명령어 ───
    warp_inst_t m_inst;                // 이 요청을 생성한 원본 명령어
    unsigned long long m_streamID;     // 스트림 ID

    // ─── 주요 메서드 ───
    set_status(status, cycle)          // 상태 전이 기록
    set_reply()                        // 요청→응답 전환
    get_addr()                         // 메모리 주소
    get_is_write()                     // 쓰기 요청 여부
    get_access_type()                  // 접근 타입 (GLOBAL_ACC_R, L1_WRBK_ACC 등)
};
```

### 8.4 stream_operation과 stream_manager (`stream_manager.h`)

CUDA 스트림을 통한 비동기 작업 관리 시스템이다.

```cpp
// 스트림 오퍼레이션 타입
enum stream_operation_type {
    stream_no_op,                      // 빈 작업
    stream_memcpy_host_to_device,      // H2D memcpy
    stream_memcpy_device_to_host,      // D2H memcpy
    stream_memcpy_device_to_device,    // D2D memcpy
    stream_memcpy_to_symbol,           // 심볼로 memcpy
    stream_memcpy_from_symbol,         // 심볼에서 memcpy
    stream_kernel_launch,              // 커널 실행
    stream_event,                      // 이벤트 기록
    stream_wait_event                  // 이벤트 대기
};

class stream_operation {
    stream_operation_type m_type;       // 작업 타입
    kernel_info_t *m_kernel;           // 커널 (kernel_launch인 경우)
    size_t m_device_address_dst/src;   // 디바이스 주소
    void *m_host_address_dst;          // 호스트 주소
    const void *m_host_address_src;    // 호스트 소스 주소
    size_t m_cnt;                      // 바이트 수
    bool m_sim_mode;                   // 기능적 시뮬레이션 모드

    do_operation(gpgpu_sim *gpu);      // 실제 작업 수행
    // kernel_launch: gpu->launch(kernel) 호출
    // memcpy: gpu->memcpy_to_gpu() 등 호출
};

class stream_manager {
    gpgpu_sim *m_gpu;                  // GPU 시뮬레이터
    list<CUstream_st *> m_streams;     // 사용자 생성 스트림 목록
    CUstream_st m_stream_zero;         // 디폴트 스트림 (스트림 0)
    map<unsigned, CUstream_st *> m_grid_id_to_stream;  // 커널 UID→스트림 매핑
    bool m_cuda_launch_blocking;       // 블로킹 모드 (CUDA_LAUNCH_BLOCKING=1)

    operation(bool *sim)               // 완료 커널 체크 + 다음 작업 디스패치
    front()                            // 다음 실행할 작업 선택 (라운드 로빈)
    push(stream_operation op)          // 새 작업 추가
    check_finished_kernel()            // 완료된 커널 정리
    stop_all_running_kernels()         // 모든 실행 중 커널 중지
};
```

### 8.5 gpgpu_sim_config 설정 클래스 (`gpu-sim.h:408`)

```cpp
class gpgpu_sim_config : public power_config,
                         public gpgpu_functional_sim_config {
    shader_core_config m_shader_config;   // Shader Core 전체 설정
    memory_config m_memory_config;        // 메모리 시스템 전체 설정

    // 클록 도메인 주파수/주기
    double core_freq, icnt_freq, dram_freq, l2_freq;
    double core_period, icnt_period, dram_period, l2_period;

    // 시뮬레이션 종료 조건
    unsigned long long gpu_max_cycle_opt;  // 최대 사이클
    unsigned long long gpu_max_insn_opt;   // 최대 명령어 수
    unsigned gpu_max_cta_opt;              // 최대 CTA 수
    unsigned max_concurrent_kernel;        // 동시 커널 수

    // 기타 설정
    bool gpgpu_flush_l1_cache;
    bool gpgpu_flush_l2_cache;
    bool gpu_deadlock_detect;
    char *gpgpu_clock_domains;             // 클록 도메인 문자열
    unsigned checkpoint_option;
};
```

---

## 9. init() 함수 상세 분석 (`gpu-sim.cc:1187`)

```cpp
void gpgpu_sim::init() {
    // ─── 1. 사이클/명령어 카운터 리셋 ───
    gpu_sim_cycle = 0;
    gpu_sim_insn = 0;
    last_gpu_sim_insn = 0;
    m_total_cta_launched = 0;
    gpu_completed_cta = 0;

    // ─── 2. 메모리 통계 리셋 ───
    partiton_reqs_in_parallel = 0;
    partiton_replys_in_parallel = 0;

    // ─── 3. 전력 모델 초기화 (McPAT/AccelWattch) ───
    if (g_power_simulation_enabled)
        init_mcpat(m_config, m_gpgpusim_wrapper, ...);

    // ─── 4. 클록 도메인 리셋 ───
    reinit_clock_domains();
    // core_time = icnt_time = dram_time = l2_time = 0

    // ─── 5. 각 SIMT 클러스터 초기화 ───
    for (i = 0; i < n_simt_clusters; i++)
        m_cluster[i]->reinit();
    // 각 SM의 파이프라인 레지스터, 스코어보드, 스케줄러 등을 리셋

    // ─── 6. 통계 객체 리셋 ───
    m_shader_stats->new_grid();

    // ─── 7. 시각화/로깅 초기화 ───
    if (g_visualizer_enabled)
        create_thread_CFlogger(...);
    shader_CTA_count_create(...);

    // ─── 8. 인터커넥트 초기화 ───
    if (g_network_mode) icnt_init();
}
```

---

## 10. active() 함수 분석 (`gpu-sim.cc:1142`)

GPU 시뮬레이터가 아직 실행을 계속해야 하는지 판단하는 함수이다.

```cpp
bool gpgpu_sim::active() {
    // 1. 종료 조건 체크 (하나라도 해당되면 false)
    if (gpu_max_cycle_opt && cycles >= gpu_max_cycle_opt) return false;
    if (gpu_max_insn_opt && insns >= gpu_max_insn_opt) return false;
    if (gpu_max_cta_opt && issued_cta >= gpu_max_cta_opt) return false;
    if (gpu_deadlock_detect && gpu_deadlock) return false;

    // 2. 아직 완료되지 않은 워프가 있는 클러스터가 있으면 active
    for (i = 0; i < n_simt_clusters; i++)
        if (m_cluster[i]->get_not_completed() > 0) return true;

    // 3. DRAM에 처리 중인 요청이 있으면 active
    for (i = 0; i < n_mem; i++)
        if (m_memory_partition_unit[i]->busy() > 0) return true;

    // 4. 인터커넥트에 in-flight 패킷이 있으면 active
    if (icnt_busy()) return true;

    // 5. 아직 발행하지 않은 CTA가 남아있으면 active
    if (get_more_cta_left()) return true;

    return false;  // 모든 작업 완료
}
```

이 함수는 `cycle()` 루프의 종료 조건으로 사용된다. GPU의 모든 컴포넌트(SM, 인터커넥트, DRAM)가 idle이고 더 이상 발행할 CTA가 없을 때 false를 반환한다.

---

## 11. 스트림 관리자와 커널 디스패치 흐름

### 11.1 커널 실행 전체 흐름

```
1. cudaLaunch() 호출
   └─ kernel_info_t 생성
   └─ stream_operation(kernel, sim_mode, stream) 생성
   └─ stream_manager->push(op)
        └─ CUstream_st에 operation 추가

2. 시뮬레이션 스레드 (concurrent 모드):
   └─ stream_manager->operation(&sim_cycles)
        └─ check_finished_kernel()     ← 완료 커널 정리
        └─ front()                      ← 다음 실행할 operation 선택
        └─ op.do_operation(gpu)
             └─ stream_kernel_launch인 경우:
                  └─ gpu->can_start_kernel() 확인
                  └─ gpu->launch(kernel)
                       └─ m_running_kernels에 등록
                       └─ kernel 타이밍 기록

3. cycle()의 issue_block2core():
   └─ select_kernel() → 실행할 커널 선택
   └─ 각 클러스터의 issue_block2core() 호출
   └─ SM이 CTA를 받아 워프를 생성하고 실행 시작

4. 커널 완료:
   └─ 모든 CTA의 모든 워프 완료
   └─ set_kernel_done(kernel)
   └─ finished_kernel()이 UID 반환
   └─ stream_manager가 stream에서 operation 제거
   └─ kernel_info_t 삭제
```

### 11.2 stream_manager::front() 작업 선택 로직

```
1. 먼저 stream_zero (디폴트 스트림) 확인
   - 비어있지 않고 busy가 아니면 → stream_zero에서 next() 반환

2. stream_zero가 비었으면 사용자 스트림을 라운드 로빈으로 탐색
   - m_last_stream 다음부터 순회
   - busy가 아니고 비어있지 않은 첫 번째 스트림에서 next() 반환

3. 선택된 operation이 커널이면
   - grid_id → stream 매핑을 m_grid_id_to_stream에 기록
```

---

## 요약

GPGPU-Sim은 다음의 핵심 설계 원칙을 따른다:

1. **CUDA API 투명 인터셉션**: `libcuda`가 실제 CUDA 라이브러리와 동일한 심볼을 제공하여, 기존 CUDA 바이너리를 수정 없이 시뮬레이션 가능하다.

2. **호스트-시뮬레이터 스레드 분리**: CUDA API 호출은 호스트 스레드에서, 실제 GPU 시뮬레이션은 별도 pthread에서 수행한다. stream_manager와 세마포어/뮤텍스로 동기화한다.

3. **다중 클록 도메인**: Core, ICNT, L2, DRAM이 각각 독립적인 주파수로 동작하며, `next_clock_domain()`이 이를 정밀하게 관리한다.

4. **계층적 메모리 시스템**: L1 (per-SM) → Interconnect → L2 (per-partition) → DRAM으로 이어지는 완전한 메모리 계층을 사이클 레벨로 시뮬레이션한다.

5. **모듈식 설정**: `gpgpusim.config` 파일로 SM 수, 캐시 크기, DRAM 타이밍 등 모든 마이크로아키텍처 파라미터를 변경할 수 있다.
