# AccelWattch 전력 모델링 심층 분석

## 1. AccelWattch 전력 모델링 개요

AccelWattch는 GPGPU-Sim 시뮬레이터에 통합된 GPU 전력 모델링 프레임워크이다. McPAT(Multi-core Power, Area, and Timing) 프레임워크를 GPU 아키텍처에 맞게 확장한 것으로, CACTI 라이브러리를 활용하여 캐시/메모리 구조의 전력과 면적을 분석적으로(analytically) 모델링한다. 하드웨어 성능 카운터 기반의 활동 계수(activity factor) 스케일링을 통해 런타임 전력을 추정한다.

### 1.1 전체 아키텍처 다이어그램

```
 +------------------------------------------------------------------+
 |                    GPGPU-Sim Simulator                            |
 |                                                                  |
 |  +------------------+     +----------------------------------+   |
 |  | Shader Core (SM) |     | power_stat_t                     |   |
 |  | - 명령어 실행     |---->| - 성능 카운터 수집               |   |
 |  | - 캐시 접근       |     | - inst/cache/mem/noc 통계        |   |
 |  | - 메모리 접근     |     +------------------+---------------+   |
 |  +------------------+                         |                   |
 |                                               v                   |
 |  +------------------------------------------------------------+  |
 |  |              power_interface.cc                             |  |
 |  |  mcpat_cycle() / calculate_hw_mcpat()                      |  |
 |  |  - 성능 카운터 -> wrapper에 전달                           |  |
 |  |  - 주기적으로 전력 계산 호출 (stat_sample_freq 마다)       |  |
 |  +-----------------------------+------------------------------+  |
 |                                |                                  |
 +--------------------------------|----------------------------------+
                                  |
                                  v
 +------------------------------------------------------------------+
 |              gpgpu_sim_wrapper (AccelWattch Wrapper)              |
 |                                                                   |
 |  set_*_power()    : 성능 카운터 입력 (+ scaling_coefficients)     |
 |  compute()        : Processor::compute() 호출                     |
 |  update_coefficients() : 전력 계수 갱신 (CACTI 기반)              |
 |  update_components_power() : 구성요소별 전력 분해                 |
 |  calculate_static_power()  : 정적 전력 계산 (쓰레드 발산 모델)    |
 |                                                                   |
 |  +---------+   +---------+   +---------+   +--------+   +------+ |
 |  |ParseXML |   |Processor|   |  Core   |   |Shared  |   | NoC  | |
 |  |XML 설정 |-->|전체 칩  |-->|SM 모델  |   |Cache   |   |모델  | |
 |  |파싱     |   |모델     |   |         |   |L2 모델 |   |      | |
 |  +---------+   |         |   +---------+   +--------+   +------+ |
 |                |         |   +--------+    +----------+           |
 |                |         |-->|MemCtrl |    |Interconn.|           |
 |                |         |   |DRAM모델|    |Wire 모델 |           |
 |                +---------+   +--------+    +----------+           |
 |                                                                   |
 |                     CACTI Library                                  |
 |  +-----------------------------------------------------------+   |
 |  | ArrayST: SRAM/CAM 전력/면적/타이밍 분석적 모델링           |   |
 |  | Wire:    배선 지연/전력 모델링                              |   |
 |  | Router:  NoC 라우터 버퍼/크로스바/아비터 모델링             |   |
 |  +-----------------------------------------------------------+   |
 +-------------------------------------------------------------------+
```

### 1.2 세 가지 시뮬레이션 모드

AccelWattch는 `g_power_simulation_mode`에 따라 세 가지 모드를 지원한다.

| 모드 | 값 | 설명 |
|------|-----|------|
| AccelWattch-SIM | 0 | GPGPU-Sim의 기능 시뮬레이터에서 수집한 성능 카운터만 사용 |
| AccelWattch-HW | 1 | 실제 하드웨어 프로파일링 데이터(CSV)에서 성능 카운터 읽기 |
| AccelWattch-Hybrid | 2 | HW 데이터와 시뮬레이션 데이터를 선택적으로 혼합 |

## 2. 전력 모델 구성 요소별 분석

### 2.1 전력 구성 요소 열거 (pwr_cmp_t)

`gpgpu_sim_wrapper.cc`에서 정의된 33개 전력 구성 요소이다.

```
enum pwr_cmp_t {
  IBP = 0,        // Instruction Buffer Power
  ICP,             // Instruction Cache Power
  DCP,             // Data Cache (L1D) Power
  TCP,             // Texture Cache Power
  CCP,             // Constant Cache Power
  SHRDP,           // Shared Memory Power
  RFP,             // Register File Power
  INTP,            // Integer ALU Power
  FPUP,            // FPU (Single Precision) Power
  DPUP,            // DPU (Double Precision) Power
  INT_MUL24P,      // Integer MUL24 Power
  INT_MUL32P,      // Integer MUL32 Power
  INT_MULP,        // Integer MUL Power
  INT_DIVP,        // Integer DIV Power
  FP_MULP,         // FP MUL Power
  FP_DIVP,         // FP DIV Power
  FP_SQRTP,        // SQRT Power
  FP_LGP,          // LOG Power
  FP_SINP,         // SIN/COS Power
  FP_EXP,          // EXP Power
  DP_MULP,         // DP MUL Power
  DP_DIVP,         // DP DIV Power
  TENSORP,         // Tensor Core Power
  TEXP,            // Texture Unit Power
  SCHEDP,          // Scheduler Power
  L2CP,            // L2 Cache Power
  MCP,             // Memory Controller Power
  NOCP,            // NoC (Network on Chip) Power
  DRAMP,           // DRAM Power
  PIPEP,           // Pipeline Power
  IDLE_COREP,      // Idle Core Power
  CONSTP,          // Constant Dynamic Power (회귀 모델로 추정)
  STATICP          // Static Power (쓰레드 발산 기반 선형 모델)
};
```

### 2.2 코어 (Core) 전력 모델

**소스**: `src/accelwattch/core.h`, `core.cc`

Core 클래스는 GPU SM(Streaming Multiprocessor) 하나를 모델링한다. 내부 서브 컴포넌트 구조는 다음과 같다.

```
Core
 |
 +-- InstFetchU (ifu)          : 명령어 페치 유닛
 |    +-- icache (InstCache)   : I-Cache (CACTI ArrayST)
 |    +-- IB (ArrayST)         : Instruction Buffer
 |    +-- BTB (ArrayST)        : Branch Target Buffer
 |    +-- BPT (BranchPredictor): 분기 예측기
 |    +-- ID_inst, ID_operand, ID_misc (inst_decoder)
 |
 +-- LoadStoreU (lsu)          : Load/Store 유닛
 |    +-- dcache (DataCache)   : L1 Data Cache
 |    +-- ccache (DataCache)   : Constant Cache
 |    +-- tcache (DataCache)   : Texture Cache
 |    +-- sharedmemory (DataCache) : 공유 메모리
 |    +-- xbar_shared (Crossbar)   : 공유 크로스바
 |
 +-- EXECU (exu)               : 실행 유닛
 |    +-- rfu (RegFU)          : 레지스터 파일 유닛
 |    |    +-- IRF, FRF (ArrayST)  : 정수/부동소수점 레지스터 파일
 |    |    +-- OPC (ArrayST)       : 오퍼랜드 컬렉터
 |    |    +-- xbar_rfu (Crossbar) : RF 크로스바
 |    |    +-- arbiter_rfu         : RF 아비터
 |    +-- scheu (SchedulerU)   : 스케줄러
 |    +-- exeu (FunctionalUnit): 정수 ALU
 |    +-- fp_u (FunctionalUnit): FPU
 |    +-- mul (FunctionalUnit) : MUL/DIV (SFU에 매핑)
 |    +-- bypass interconnects : 바이패스 네트워크
 |
 +-- MemManU (mmu)             : 메모리 관리 유닛 (TLB)
 +-- Pipeline (corepipe)       : 파이프라인 전력
 +-- UndiffCore (undiffCore)   : 미분류 코어 전력
```

각 서브 컴포넌트는 CACTI의 `ArrayST`를 사용하여 SRAM/CAM 구조를 모델링하고, `per_access_energy`와 `leakage` 값을 산출한다.

**코어의 전력 계수 산출 함수들 (core.h에 정의)**:

- `get_coefficient_ialu_accesses()`: `exeu->per_access_energy * g_tp.sckt_co_eff`
- `get_coefficient_fpu_accesses()`: `fp_u->per_access_energy * g_tp.sckt_co_eff`
- `get_coefficient_sfu_accesses()`: `mul->per_access_energy * g_tp.sckt_co_eff`
- `get_coefficient_dcache_readhits()`: `dcache.caches->power.readOp.dynamic + xbar_shared->power.readOp.dynamic`
- `get_coefficient_regreads_accesses()`: IRF read + crossbar + arbiter + OPC 전력의 합

### 2.3 캐시 (SharedCache) 전력 모델

**소스**: `src/accelwattch/sharedcache.h`, `sharedcache.cc`

SharedCache 클래스는 L2/L3 캐시를 모델링한다. 내부 `DataCache unicache` 구조를 통해 다음 CACTI 구조를 생성한다.

```
SharedCache (unicache)
 +-- caches (ArrayST)      : 실제 캐시 데이터/태그 배열
 +-- missb (ArrayST)       : Miss Buffer (MSHR)
 +-- ifb (ArrayST)         : Fill Buffer
 +-- prefetchb (ArrayST)   : Prefetch Buffer
 +-- wbb (ArrayST)         : Write-Back Buffer
```

L2 전력 계수는 `Processor` 클래스에서 산출한다.

```cpp
// processor.h에서 발췌
double get_coefficient_l2_read_hits() {
    return l2array[0]->unicache.caches->local_result.power.readOp.dynamic;
}
double get_coefficient_l2_write_misses() {
    // 태그 배열 + 데이터 배열 + miss buffer + fill buffer +
    // prefetch buffer + write-back buffer의 전력 합산
    value += l2array[0]->unicache.caches->local_result.tag_array2->power.writeOp.dynamic;
    value += l2array[0]->unicache.caches->local_result.power.writeOp.dynamic;
    value += l2array[0]->unicache.missb->local_result.power.searchOp.dynamic;
    // ... (6개 더)
}
```

### 2.4 메모리 컨트롤러 (MemoryController) 전력 모델

**소스**: `src/accelwattch/memoryctrl.h`, `memoryctrl.cc`

메모리 컨트롤러는 세 개의 주요 서브 컴포넌트로 구성된다.

```
MemoryController
 +-- MCFrontEnd (frontend)     : 프론트엔드
 |    +-- frontendBuffer       : 프론트엔드 버퍼
 |    +-- readBuffer           : 읽기 버퍼
 |    +-- writeBuffer          : 쓰기 버퍼
 |    +-- PRT (ArrayST)        : Page Request Table (coalescing)
 |    +-- threadMasks (ArrayST): Thread Masks (coalescing)
 |    +-- PRC (ArrayST)        : Page Request Counter (coalescing)
 |    +-- MC_arb               : 아비터
 |
 +-- MCBackend (transecEngine) : 백엔드 트랜잭션 엔진
 |    - DDR2/DDR3 프로토콜 컨트롤러 기반 경험적 모델
 |    - Cadence ChipEstimator 데이터 포인트 기반
 |
 +-- MCPHY (PHY)               : 물리 계층
 |    - 트랜시버 전력 모델
 |
 +-- DRAM (dram)               : DRAM 전력 모델
 |    - GDDR3 / GDDR5 지원
 |    - 경험적 계수: rd_coeff, wr_coeff, pre_coeff
 |
 +-- Pipeline (pipeLogic)      : MC 파이프라인
```

DRAM 전력은 경험적 계수(empirical coefficient)를 사용한다. XML 설정에서 `dram_rd_coeff`, `dram_wr_coeff`, `dram_pre_coeff` 값을 읽어 메모리 읽기/쓰기/프리차지 접근 횟수와 곱한다.

### 2.5 인터커넥트 (interconnect) 전력 모델

**소스**: `src/accelwattch/interconnect.h`, `interconnect.cc`

인터커넥트는 CACTI의 Wire 모델을 사용하여 배선 전력을 계산한다.

```cpp
// interconnect.cc - compute()
void interconnect::compute() {
    Wire *wtemp1 = new Wire(wt, length, 1, width_scaling, space_scaling);
    delay = wtemp1->delay;
    power.readOp.dynamic = wtemp1->power.readOp.dynamic;
    power.readOp.leakage = wtemp1->power.readOp.leakage;
    power.readOp.gate_leakage = wtemp1->power.readOp.gate_leakage;
}
```

파이프라이너블 배선의 경우, throughput을 만족할 때까지 wire width를 2배씩 스케일링하고, 여전히 만족하지 못하면 파이프라인 스테이지를 삽입한다.

### 2.6 NoC (Network on Chip) 전력 모델

**소스**: `src/accelwattch/noc.h`, `noc.cc`

NoC는 라우터 기반(type=1) 또는 버스 기반(type=0) 두 가지 모드를 지원한다.

```
NoC
 +-- router (MCPAT_Router)     : 라우터 (type=1인 경우)
 |    +-- buffer               : 입력 버퍼
 |    +-- crossbar             : 크로스바 스위치
 |    +-- arbiter              : 아비터
 |
 +-- link_bus (interconnect)   : 링크/버스 (배선 모델)
```

NoC 전력 계수는 라우터의 버퍼/크로스바/아비터 접근당 에너지를 합산한 것이다.

```cpp
// processor.h
double get_coefficient_noc_accesses() {
    read_coef += nocs[0]->router->buffer.power.readOp.dynamic;
    read_coef += nocs[0]->router->buffer.power.writeOp.dynamic;
    read_coef += nocs[0]->router->crossbar.power.readOp.dynamic;
    read_coef += nocs[0]->router->arbiter.power.readOp.dynamic;
    return read_coef;
}
```

## 3. 전력 계산 방식

### 3.1 전체 전력 계산 흐름

```
  시뮬레이터 실행
       |
       v (매 stat_sample_freq 사이클마다)
  power_interface.cc::mcpat_cycle()
       |
       +-- set_inst_power()        : 명령어 통계 설정
       +-- set_regfile_power()     : 레지스터 파일 통계
       +-- set_icache_power()      : I-cache 통계
       +-- set_l1cache_power()     : L1D cache 통계
       +-- set_l2cache_power()     : L2 cache 통계
       +-- set_mem_ctrl_power()    : DRAM 접근 통계
       +-- set_exec_unit_power()   : 실행 유닛 통계
       +-- set_NoC_power()         : NoC 플릿 통계
       |
       v
  wrapper->compute()
       |  --> Processor::compute()
       |      --> Core::compute() (각 코어)
       |      --> SharedCache::computeEnergy(false) (L2)
       |      --> MemoryController::computeEnergy(false)
       |      --> NoC::computeEnergy(false)
       v
  wrapper->update_components_power()
       |
       +-- update_coefficients()   : CACTI 기반 전력 계수 갱신
       +-- 구성요소별 전력 분해     : sample_cmp_pwr[i] 계산
       +-- calculate_static_power(): 정적 전력 계산
       +-- DVFS 스케일링            : 전압비 기반 보정
       v
  wrapper->power_metrics_calculations()
       |
       +-- 평균/최대/최소 전력 추적
       v
  결과 출력 (power trace, metric trace)
```

### 3.2 동적 전력 (Dynamic Power) 계산

동적 전력은 `접근 횟수 x 접근당 에너지` 모델을 사용한다. 핵심은 **이중 스케일링** 구조이다.

**1단계: 성능 카운터에 scaling_coefficient 적용**

XML 설정에서 각 성능 카운터별 활동 계수(activity factor)를 정의한다. 이 값은 시뮬레이터의 성능 카운터에 곱해진다.

```cpp
// gpgpu_sim_wrapper.cc::set_inst_power()
p->sys.core[0].total_instructions =
    tot_inst * p->sys.scaling_coefficients[TOT_INST];  // XML의 TOT_INST=10
```

**2단계: McPAT/CACTI 기반 에너지 계수와 곱합**

```cpp
// update_coefficients()에서
effpower_coeff[IC_H] = initpower_coeff[IC_H] * p->sys.scaling_coefficients[IC_H];
// initpower_coeff[IC_H] = CACTI가 계산한 I-cache hit당 에너지
// scaling_coefficients[IC_H] = XML에서 읽은 활동 계수 (예: 8.59)
```

**3단계: 구성 요소 전력 = 런타임 동적 전력 / 실행 시간**

```cpp
// update_components_power()에서
sample_cmp_pwr[ICP] = proc->cores[0]->ifu->icache.rt_power.readOp.dynamic
                      / (proc->cores[0]->executionTime);
```

### 3.3 SFU/FPU 전력의 명령어 유형별 분배

SFU(Special Function Unit)와 FPU의 전체 전력을 개별 명령어 유형별로 비례 분배한다.

```cpp
// FPU 전력 분배: FP_ACC와 DP_ACC에 접근 비율로 분배
if (tot_fpu_accesses != 0) {
    sample_cmp_pwr[FPUP] = sample_fp_pwr * sample_perf_counters[FP_ACC] / tot_fpu_accesses;
    sample_cmp_pwr[DPUP] = sample_fp_pwr * sample_perf_counters[DP_ACC] / tot_fpu_accesses;
}

// SFU 전력 분배: INT_MUL, FP_MUL, FP_DIV, SQRT, LOG, SIN, EXP, TENSOR, TEX 등에 분배
if (tot_sfu_accesses != 0) {
    sample_cmp_pwr[INT_MUL24P] = sample_sfu_pwr * sample_perf_counters[INT_MUL24_ACC]
                                 / tot_sfu_accesses;
    // ... 14개 유형에 대해 동일 패턴
}
```

### 3.4 정적 전력 (Static Power) 계산

AccelWattch는 GPU의 쓰레드 발산(thread divergence)이 정적 전력에 미치는 영향을 선형 모델로 포착한다.

```cpp
// calculate_static_power()
total_static_power = base_static_power + ((avg_threads_per_warp - 1.0) * lane_static_power);
return total_static_power * per_active_core;
```

활성 명령어 유형 조합에 따라 다른 base/addlane 계수를 선택한다.

| 카테고리 | 조건 | XML 파라미터 |
|----------|------|-------------|
| CAT1 | INT only (ADD+MUL) | static_cat1_flane, static_cat1_addlane |
| CAT2 | INT + FP | static_cat2_flane, static_cat2_addlane |
| CAT3 | INT + FP + DP | static_cat3_flane, static_cat3_addlane |
| CAT4 | INT + FP + SFU | static_cat4_flane, static_cat4_addlane |
| CAT5 | INT + FP + TEX | static_cat5_flane, static_cat5_addlane |
| CAT6 | INT + FP + TENSOR | static_cat6_flane, static_cat6_addlane |
| INT_ADD | INT ADD only | static_intadd_flane, static_intadd_addlane |
| INT_MUL | INT MUL only | static_intmul_flane, static_intmul_addlane |
| LIGHT_SM | 실행 유닛 접근 없음 | static_light_flane, static_light_addlane |
| L1 only | L1 접근만 있음 | static_l1_flane |
| Shared only | 공유 메모리 접근만 | static_shared_flane |
| L2 only | L2 접근만 있음 | static_l2_flane |
| GEOMEAN | 위 어느 것에도 해당 안 됨 | static_geomean_flane, static_geomean_addlane |

### 3.5 누설 전력 (Leakage Power)

누설 전력은 CACTI 내부에서 트랜지스터 레벨로 계산된다. 두 가지 유형이 있다.

**서브스레숄드 누설 (Subthreshold Leakage)**:

```cpp
// memoryctrl.cc - MCBackend::compute()
power_t.readOp.leakage =
    area.get_area() / 2 * (g_tp.scaling_factor.core_tx_density) *
    cmos_Isub_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
    g_tp.peri_global.Vdd;
```

**게이트 누설 (Gate Leakage)**:

```cpp
power_t.readOp.gate_leakage =
    area.get_area() / 2 * (g_tp.scaling_factor.core_tx_density) *
    cmos_Ig_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
    g_tp.peri_global.Vdd;
```

장채널 디바이스(longer channel device)를 사용할 경우 누설 감소 계수가 적용된다.

### 3.6 DVFS (Dynamic Voltage and Frequency Scaling) 지원

DVFS가 활성화되면 전력 값에 전압비 스케일링이 적용된다.

```cpp
// update_components_power()
if (g_dvfs_enabled) {
    double voltage_ratio = modeled_chip_voltage / p->sys.modeled_chip_voltage_ref;
    sample_cmp_pwr[IDLE_COREP] *= voltage_ratio;          // 정적 전력: V 비례
    sample_cmp_pwr[STATICP] *= voltage_ratio;              // 정적 전력: V 비례
    for (unsigned i = 0; i < num_pwr_cmps; i++) {
        if ((i != IDLE_COREP) && (i != STATICP)) {
            sample_cmp_pwr[i] *= voltage_ratio * voltage_ratio; // 동적 전력: V^2 비례
        }
    }
}
```

## 4. CACTI 라이브러리 통합

### 4.1 CACTI의 역할

CACTI(Cache Access and Cycle Time Estimator)는 캐시/메모리 구조의 전력, 면적, 타이밍을 분석적으로 추정하는 라이브러리이다. AccelWattch에서 CACTI는 다음 역할을 한다.

```
CACTI 라이브러리 (src/accelwattch/cacti/)
 |
 +-- ArrayST : SRAM/CAM 배열 모델링
 |    - 주어진 크기, 라인 크기, 결합도, 뱅크 수에서
 |    - 동적 에너지 (read/write/search), 누설 전력, 면적 산출
 |    - 캐시 태그/데이터 배열, 레지스터 파일, 명령어 버퍼, MSHR 등
 |
 +-- Wire : 배선 모델링
 |    - 와이어 길이, 폭, 간격에서 지연과 전력 산출
 |    - interconnect 클래스에서 사용
 |
 +-- Router (MCPAT_Router) : NoC 라우터 모델링
 |    - 버퍼, 크로스바, 아비터 전력/면적
 |
 +-- FunctionalUnit : 실행 유닛 모델링
 |    - per_access_energy, base_energy 산출
 |
 +-- technology parameters (g_tp)
      - 공정 기술 노드별 파라미터
      - Vdd, 트랜지스터 밀도, 소켓 계수 등
```

### 4.2 ArrayST 사용 예

캐시를 CACTI로 모델링할 때 `InputParameter`를 설정하고 `ArrayST`를 생성한다.

```cpp
// core.cc - InstFetchU 생성자에서 I-Cache 모델링
interface_ip.cache_sz = XML->sys.core[ithCore].icache.icache_config[0]; // 크기(B)
interface_ip.line_sz = XML->sys.core[ithCore].icache.icache_config[1];  // 라인(B)
interface_ip.assoc = XML->sys.core[ithCore].icache.icache_config[2];    // 결합도
interface_ip.nbanks = XML->sys.core[ithCore].icache.icache_config[3];   // 뱅크 수
interface_ip.is_cache = true;
interface_ip.num_rw_ports = 1;

icache.caches = new ArrayST(&interface_ip, "icache", Core_device, ...);
// 결과: icache.caches->local_result.power.readOp.dynamic (읽기당 동적 에너지)
//       icache.caches->local_result.power.writeOp.dynamic (쓰기당 동적 에너지)
//       icache.caches->local_result.area                   (면적)
```

## 5. 시뮬레이터와의 연동 방식

### 5.1 초기화 경로

```
gpgpu_sim 생성 시:
  gpgpu_sim_wrapper 생성 (xml_filename 전달)
    -> ParseXML::parse(xml_filename)  : XML 설정 파싱
    -> Processor 생성
       -> Core 생성 (CACTI로 각 구조 모델링)
       -> SharedCache (L2) 생성
       -> MemoryController 생성
       -> NoC 생성
       -> computeEnergy(true)  : TDP 계산
       -> computeEnergy(false) : RTP 초기화
```

### 5.2 런타임 전력 계산 경로

```
gpgpu_sim::cycle() (매 사이클)
  |
  v (매 stat_sample_freq 사이클마다)
power_interface.cc::mcpat_cycle()
  |
  +-- 1. 성능 카운터 수집 (power_stat_t에서)
  |      get_total_inst(), get_l1d_read_hits(), get_dram_rd() 등
  |
  +-- 2. wrapper->set_*_power() 호출 (43개 성능 카운터 설정)
  |      각 카운터에 scaling_coefficient 적용하여 ParseXML에 저장
  |
  +-- 3. wrapper->compute()
  |      Processor::compute() -> 각 컴포넌트의 computeEnergy(false)
  |      = 런타임 전력(rt_power) 계산
  |
  +-- 4. wrapper->update_components_power()
  |      구성요소별 전력 분해 + 정적/상수 전력 추가
  |
  +-- 5. wrapper->power_metrics_calculations()
  |      평균/최대/최소 전력 통계 갱신
  |
  +-- 6. wrapper->dump() / print_trace_files()
         결과 출력
```

### 5.3 성능 카운터 매핑

`power_interface.cc::mcpat_cycle()`에서 시뮬레이터의 성능 카운터를 AccelWattch 입력으로 변환하는 주요 매핑이다.

| 시뮬레이터 카운터 | wrapper 함수 | 내부 용도 |
|-------------------|-------------|-----------|
| total_inst, int_inst, fp_inst | set_inst_power() | 명령어 버퍼/스케줄러/파이프라인 |
| regfile_reads, regfile_writes | set_regfile_power() | 레지스터 파일 |
| inst_c_hits, inst_c_misses | set_icache_power() | I-Cache |
| l1d_read_hits/misses, write_hits/misses | set_l1cache_power() | L1D Cache |
| l2_read_hits/misses, write_hits/misses | set_l2cache_power() | L2 Cache |
| dram_rd, dram_wr, dram_pre | set_mem_ctrl_power() | DRAM + MC |
| ialu_acc, fpu_acc, sfu_acc | set_exec_unit_power() | 실행 유닛 |
| icnt_simt_to_mem + icnt_mem_to_simt | set_NoC_power() | 인터커넥트 |
| active_sms, pipeline_duty_cycle | set_idle_core/duty_cycle | 유휴/파이프라인 |

## 6. XML 설정 파라미터 분석

### 6.1 AccelWattch XML 구조 (accelwattch_sass_sim.xml)

```xml
<component id="root" name="root">
  <component id="system" name="system">

    <!-- 동적 전력 활동 계수 (scaling_coefficients) -->
    <param name="TOT_INST" value="10"/>        <!-- 총 명령어 활동 계수 -->
    <param name="FP_INT" value="4.661"/>        <!-- FP/INT 스케줄러 활동 계수 -->
    <param name="IC_H" value="8.593"/>          <!-- I-Cache 히트 활동 계수 -->
    <param name="DC_RH" value="9.835"/>         <!-- L1D 읽기 히트 활동 계수 -->
    <param name="L2_RH" value="1.261"/>         <!-- L2 읽기 히트 활동 계수 -->
    <param name="MEM_RD" value="0.026"/>        <!-- DRAM 읽기 활동 계수 -->
    <param name="NOC_A" value="32.09"/>         <!-- NoC 접근 활동 계수 -->
    <param name="INT_ACC" value="14.988"/>      <!-- 정수 ALU 활동 계수 -->
    <param name="TENSOR_ACC" value="0.815"/>    <!-- 텐서 코어 활동 계수 -->
    <!-- ... 총 43개 -->

    <!-- 상수 및 정적 전력 파라미터 -->
    <param name="constant_power" value="32.325"/>   <!-- 상수 동적 전력 (W) -->
    <param name="idle_core_power" value="0.283"/>    <!-- 유휴 SM당 전력 (W) -->

    <!-- 정적 전력 카테고리별 (쓰레드 발산 선형 모델) -->
    <param name="static_cat1_flane" value="15.290"/> <!-- INT 첫 레인 전력 -->
    <param name="static_cat1_addlane" value="0.586"/><!-- INT 추가 레인 전력 -->
    <!-- ... 13개 카테고리 -->

    <!-- 하드웨어 아키텍처 파라미터 -->
    <param name="core_tech_node" value="23"/>       <!-- 공정 노드 (nm) -->
    <param name="number_of_cores" value="80"/>      <!-- 동적으로 덮어씀 -->
    <param name="number_of_L2s" value="1"/>
    <param name="number_of_NoCs" value="1"/>

    <!-- 코어 설정 -->
    <component id="system.core0" name="core0">
      <param name="clock_rate" value="700"/>        <!-- MHz -->
      <param name="ALU_per_core" value="32"/>       <!-- CUDA 코어 수 -->
      <param name="FPU_per_core" value="32"/>
      <param name="MUL_per_core" value="4"/>        <!-- SFU 수 -->
      <param name="pipeline_depth" value="8,8"/>
      <param name="rf_banks" value="32"/>           <!-- 레지스터 뱅크 수 -->
      <param name="simd_width" value="32"/>         <!-- SIMD 폭 -->
      <param name="collector_units" value="32"/>    <!-- 오퍼랜드 컬렉터 수 -->
      <param name="core_clock_ratio" value="2"/>    <!-- 코어/RF 클럭 비율 -->

      <!-- 캐시 설정 (크기, 라인크기, 결합도, 뱅크, ...) -->
      <param name="icache_config" value="4096,32,4,1,1,3,8,0"/>
      <param name="dcache_config" value="16384,128,4,1,6,3,8,0"/>
      <!-- ... -->
    </component>

    <!-- L2 캐시 설정 -->
    <component id="system.L20" name="L20">
      <param name="L2_config" value="786432,128,8,8,8,23,32,1"/>
      <!-- ... -->
    </component>

    <!-- NoC 설정 -->
    <component id="system.NoC0" name="noc0">
      <param name="type" value="1"/>               <!-- 1=router, 0=bus -->
      <param name="flit_bits" value="256"/>
      <param name="input_ports" value="5"/>
      <param name="output_ports" value="5"/>
      <!-- ... -->
    </component>

    <!-- 메모리 컨트롤러 설정 -->
    <component id="system.mc" name="mc">
      <param name="number_mcs" value="8"/>
      <param name="memory_channels_per_mc" value="1"/>
      <param name="databus_width" value="32"/>
      <param name="peak_transfer_rate" value="3500"/>
      <!-- DRAM 경험적 계수 -->
      <param name="dram_rd_coeff" value="0.000109"/>
      <param name="dram_wr_coeff" value="0.000109"/>
      <param name="dram_pre_coeff" value="0.0000135"/>
      <!-- ... -->
    </component>
  </component>
</component>
```

### 6.2 주요 XML 파라미터 해설

**활동 계수 (scaling_coefficients)**: XML의 상위 `<param name="TOT_INST" value="10"/>` 등의 값이다. 이들은 McPAT/CACTI가 산출한 접근당 에너지에 곱해지는 보정 계수로, 하드웨어 전력 측정값과의 차이를 보정하기 위해 사용된다. 값이 1보다 크면 CACTI 모델이 과소 추정한 것을 보정하고, 1보다 작으면 과대 추정을 보정한다.

**공정 노드 (core_tech_node)**: CACTI의 기술 파라미터 테이블 선택에 사용된다. QV100의 경우 23nm(12nm 실제를 근사)를 사용한다.

## 7. 핵심 함수 라인바이라인 주석

### 7.1 gpgpu_sim_wrapper::update_components_power()

이 함수는 전력 모델의 핵심으로, McPAT/CACTI의 런타임 전력 결과를 개별 구성 요소로 분해한다.

```cpp
void gpgpu_sim_wrapper::update_components_power() {
  // [1] 먼저 전력 계수(coefficient)를 갱신한다
  //     CACTI가 산출한 접근당 에너지 * XML의 scaling_coefficient
  update_coefficients();

  // [2] 전체 프로세서 동적 전력 (CACTI/McPAT 결과)
  proc_power = proc->rt_power.readOp.dynamic;

  // [3] 명령어 버퍼 전력 = (IB read + IB write + 디코더들) / 실행시간
  sample_cmp_pwr[IBP] =
      (proc->cores[0]->ifu->IB->rt_power.readOp.dynamic +
       proc->cores[0]->ifu->IB->rt_power.writeOp.dynamic +
       proc->cores[0]->ifu->ID_misc->rt_power.readOp.dynamic +
       proc->cores[0]->ifu->ID_operand->rt_power.readOp.dynamic +
       proc->cores[0]->ifu->ID_inst->rt_power.readOp.dynamic) /
      (proc->cores[0]->executionTime);

  // [4] I-Cache 전력
  sample_cmp_pwr[ICP] = proc->cores[0]->ifu->icache.rt_power.readOp.dynamic /
                        (proc->cores[0]->executionTime);

  // [5] L1 Data Cache 전력
  sample_cmp_pwr[DCP] = proc->cores[0]->lsu->dcache.rt_power.readOp.dynamic /
                        (proc->cores[0]->executionTime);

  // [6] Texture Cache 전력
  sample_cmp_pwr[TCP] = proc->cores[0]->lsu->tcache.rt_power.readOp.dynamic /
                        (proc->cores[0]->executionTime);

  // [7] Constant Cache 전력
  sample_cmp_pwr[CCP] = proc->cores[0]->lsu->ccache.rt_power.readOp.dynamic /
                        (proc->cores[0]->executionTime);

  // [8] 공유 메모리 전력
  sample_cmp_pwr[SHRDP] =
      proc->cores[0]->lsu->sharedmemory.rt_power.readOp.dynamic /
      (proc->cores[0]->executionTime);

  // [9] 레지스터 파일 전력 (코어/RF 클럭 비율 보정)
  //     RF는 코어보다 느린 클럭으로 동작할 수 있으므로 비율 보정 필요
  sample_cmp_pwr[RFP] =
      (proc->cores[0]->exu->rfu->rt_power.readOp.dynamic /
       (proc->cores[0]->executionTime)) *
      (proc->cores[0]->exu->rf_fu_clockRate / proc->cores[0]->exu->clockRate);

  // [10] FPU와 SFU 전체 전력 (나중에 명령어 유형별로 분배)
  double sample_fp_pwr = (proc->cores[0]->exu->fp_u->rt_power.readOp.dynamic /
                          (proc->cores[0]->executionTime));
  double sample_sfu_pwr = (proc->cores[0]->exu->mul->rt_power.readOp.dynamic /
                           (proc->cores[0]->executionTime));

  // [11] 정수 ALU 전력 (클럭 비율 보정)
  sample_cmp_pwr[INTP] =
      (proc->cores[0]->exu->exeu->rt_power.readOp.dynamic /
       (proc->cores[0]->executionTime)) *
      (proc->cores[0]->exu->rf_fu_clockRate / proc->cores[0]->exu->clockRate);

  // [12] FPU 전력을 FP/DP 접근 비율로 분배
  if (tot_fpu_accesses != 0) {
    sample_cmp_pwr[FPUP] =
        sample_fp_pwr * sample_perf_counters[FP_ACC] / tot_fpu_accesses;
    sample_cmp_pwr[DPUP] =
        sample_fp_pwr * sample_perf_counters[DP_ACC] / tot_fpu_accesses;
  }

  // [13] SFU 전력을 14개 명령어 유형별 접근 비율로 분배
  //      INT_MUL24, INT_MUL32, INT_MUL, INT_DIV,
  //      FP_MUL, FP_DIV, FP_SQRT, FP_LG, FP_SIN, FP_EXP,
  //      DP_MUL, DP_DIV, TENSOR, TEX
  if (tot_sfu_accesses != 0) {
    sample_cmp_pwr[INT_MUL24P] =
        sample_sfu_pwr * sample_perf_counters[INT_MUL24_ACC] / tot_sfu_accesses;
    // ... 나머지 13개 유형에 대해 동일 패턴
  }

  // [14] 스케줄러 전력
  sample_cmp_pwr[SCHEDP] = proc->cores[0]->exu->scheu->rt_power.readOp.dynamic /
                           (proc->cores[0]->executionTime);

  // [15] L2 캐시 전력 (L2가 존재하는 경우만)
  sample_cmp_pwr[L2CP] = (proc->XML->sys.number_of_L2s > 0)
                             ? proc->l2array[0]->rt_power.readOp.dynamic /
                                   (proc->cores[0]->executionTime)
                             : 0;

  // [16] 메모리 컨트롤러 전력 (DRAM 부분 제외)
  sample_cmp_pwr[MCP] = (proc->mc->rt_power.readOp.dynamic -
                         proc->mc->dram->rt_power.readOp.dynamic) /
                        (proc->cores[0]->executionTime);

  // [17] NoC 전력
  sample_cmp_pwr[NOCP] =
      proc->nocs[0]->rt_power.readOp.dynamic / (proc->cores[0]->executionTime);

  // [18] DRAM 전력 (MC에서 분리)
  sample_cmp_pwr[DRAMP] =
      proc->mc->dram->rt_power.readOp.dynamic / (proc->cores[0]->executionTime);

  // [19] 파이프라인 전력 (duty cycle 기반)
  sample_cmp_pwr[PIPEP] =
      proc->cores[0]->Pipeline_energy / (proc->cores[0]->executionTime);

  // [20] 유휴 코어 전력
  sample_cmp_pwr[IDLE_COREP] =
      proc->cores[0]->IdleCoreEnergy / (proc->cores[0]->executionTime);

  // [21] 상수 동적 전력 (XML의 constant_power 값)
  sample_cmp_pwr[CONSTP] = p->sys.scaling_coefficients[constant_power];

  // [22] 정적 전력 (쓰레드 발산 기반 선형 모델)
  sample_cmp_pwr[STATICP] = calculate_static_power();

  // [23] DVFS 스케일링 (활성화된 경우)
  if (g_dvfs_enabled) {
    double voltage_ratio = modeled_chip_voltage / p->sys.modeled_chip_voltage_ref;
    // 정적 전력: V에 선형 비례
    sample_cmp_pwr[IDLE_COREP] *= voltage_ratio;
    sample_cmp_pwr[STATICP] *= voltage_ratio;
    // 동적 전력: V^2에 비례
    for (unsigned i = 0; i < num_pwr_cmps; i++) {
      if ((i != IDLE_COREP) && (i != STATICP)) {
        sample_cmp_pwr[i] *= voltage_ratio * voltage_ratio;
      }
    }
  }

  // [24] 총 전력에 상수/정적 전력 추가
  proc_power += sample_cmp_pwr[CONSTP] + sample_cmp_pwr[STATICP];

  // [25] Sanity check: 구성 요소 합 == 총 전력
  if (!g_dvfs_enabled) {
    double sum_pwr_cmp = 0;
    for (unsigned i = 0; i < num_pwr_cmps; i++) {
      sum_pwr_cmp += sample_cmp_pwr[i];
    }
    assert(sanity_check(sum_pwr_cmp, proc_power));
  }
}
```

### 7.2 gpgpu_sim_wrapper::update_coefficients()

이 함수는 CACTI 기반 접근당 에너지와 XML 활동 계수를 결합하여 유효 전력 계수를 산출한다.

```cpp
void gpgpu_sim_wrapper::update_coefficients() {
  // [1] 스케줄러(FP_INT): instruction window의 read/search/write/selection 에너지
  initpower_coeff[FP_INT] = proc->cores[0]->get_coefficient_fpint_insts();
  effpower_coeff[FP_INT] =
      initpower_coeff[FP_INT] * p->sys.scaling_coefficients[FP_INT];

  // [2] 총 명령어(TOT_INST): IB read/write + 디코더 에너지
  initpower_coeff[TOT_INST] = proc->cores[0]->get_coefficient_tot_insts();
  effpower_coeff[TOT_INST] =
      initpower_coeff[TOT_INST] * p->sys.scaling_coefficients[TOT_INST];

  // [3] 레지스터 파일: IRF read/write + crossbar + arbiter + OPC
  //     코어/RF 클럭 비율로 보정
  initpower_coeff[REG_RD] =
      proc->cores[0]->get_coefficient_regreads_accesses() *
      (proc->cores[0]->exu->rf_fu_clockRate / proc->cores[0]->exu->clockRate);

  // [4] 캐시 계수: CACTI hit/miss 에너지 + coalescing 로직 에너지
  initpower_coeff[DC_RH] = (proc->cores[0]->get_coefficient_dcache_readhits() +
                            proc->get_coefficient_readcoalescing());

  // [5] 실행 유닛 계수 (IALU, FPU, SFU)
  //     FPU/SFU는 접근 비율로 분배
  initpower_coeff[INT_ACC] =
      proc->cores[0]->get_coefficient_ialu_accesses() *
      (proc->cores[0]->exu->rf_fu_clockRate / proc->cores[0]->exu->clockRate);
  if (tot_fpu_accesses != 0) {
    initpower_coeff[FP_ACC] =
        fp_coeff * sample_perf_counters[FP_ACC] / tot_fpu_accesses;
  }

  // [6] 메모리 컨트롤러: frontend buffer + read/write buffer + DRAM + PHY
  initpower_coeff[MEM_RD] = proc->get_coefficient_mem_reads();
  initpower_coeff[MEM_WR] = proc->get_coefficient_mem_writes();
  initpower_coeff[MEM_PRE] = proc->get_coefficient_mem_pre();

  // [7] NoC: router buffer read/write + crossbar + arbiter
  initpower_coeff[NOC_A] = proc->get_coefficient_noc_accesses();

  // [8] 모든 계수를 실행 시간으로 나누어 와트(W) 단위로 변환
  //     에너지(J) / 실행시간(s) = 전력(W)
  for (unsigned i = 0; i < num_perf_counters; i++) {
    initpower_coeff[i] /= (proc->cores[0]->executionTime);
    effpower_coeff[i] /= (proc->cores[0]->executionTime);
  }
}
```

### 7.3 power_interface.cc::mcpat_cycle()

시뮬레이터와 전력 모델의 연결 지점이다.

```cpp
void mcpat_cycle(const gpgpu_sim_config &config,
                 const shader_core_config *shdr_config,
                 class gpgpu_sim_wrapper *wrapper,
                 class power_stat_t *power_stats,
                 unsigned stat_sample_freq,
                 unsigned tot_cycle, unsigned cycle,
                 unsigned tot_inst, unsigned inst,
                 bool dvfs_enabled) {
  static bool mcpat_init = true;
  if (mcpat_init) {           // 첫 사이클에는 전력 데이터 없음
    mcpat_init = false;
    return;
  }

  // stat_sample_freq 사이클마다 전력 계산
  if ((tot_cycle + cycle) % stat_sample_freq == 0) {

    // [1] 명령어 통계 설정
    wrapper->set_inst_power(
        shdr_config->gpgpu_clock_gated_lanes,
        stat_sample_freq, stat_sample_freq,      // tot_cycles, busy_cycles
        power_stats->get_total_inst(0),           // 총 명령어 수
        power_stats->get_total_int_inst(0),       // INT 명령어 수
        power_stats->get_total_fp_inst(0),        // FP 명령어 수
        power_stats->get_l1d_read_accesses(0),    // load 명령어
        power_stats->get_l1d_write_accesses(0),   // store 명령어
        power_stats->get_committed_inst(0));      // 커밋된 명령어

    // [2] 레지스터 파일 통계
    wrapper->set_regfile_power(power_stats->get_regfile_reads(0),
                               power_stats->get_regfile_writes(0),
                               power_stats->get_non_regfile_operands(0));

    // [3] 캐시 통계 (I-cache, L1D, L2, constant, texture, shared)
    wrapper->set_icache_power(power_stats->get_inst_c_hits(0),
                              power_stats->get_inst_c_misses(0));
    wrapper->set_l1cache_power(...);
    wrapper->set_l2cache_power(...);

    // [4] 유휴 코어 수 계산
    float active_sms = (*power_stats->m_active_sms) / stat_sample_freq;
    float num_idle_core = num_cores - active_sms;
    wrapper->set_idle_core_power(num_idle_core);

    // [5] 파이프라인 duty cycle (최대 0.8로 클램프)
    float pipeline_duty_cycle = min(avg_duty_cycle, 0.8f);
    wrapper->set_duty_cycle_power(pipeline_duty_cycle);

    // [6] 메모리 컨트롤러 (DRAM read/write/precharge)
    wrapper->set_mem_ctrl_power(power_stats->get_dram_rd(0),
                                power_stats->get_dram_wr(0),
                                power_stats->get_dram_pre(0));

    // [7] 실행 유닛 세부 접근 (INT, FP, DP, SFU, Tensor, Tex)
    wrapper->set_int_accesses(ialu, imul24, imul32, imul, idiv);
    wrapper->set_fp_accesses(fp, fpmul, fpdiv);
    wrapper->set_dp_accesses(dp, dpmul, dpdiv);
    wrapper->set_trans_accesses(sqrt, log, sin, exp);
    wrapper->set_tensor_accesses(tensor);
    wrapper->set_tex_accesses(tex);

    // [8] NoC 플릿 접근 (양방향 합계)
    wrapper->set_NoC_power(n_icnt_mem_to_simt + n_icnt_simt_to_mem);

    // [9] 전력 계산 실행
    wrapper->compute();                    // McPAT/CACTI 런타임 전력 계산
    wrapper->update_components_power();    // 구성요소별 전력 분해
    wrapper->print_trace_files();          // 트레이스 출력
    power_stats->save_stats();             // 통계 저장
    wrapper->detect_print_steady_state(0, tot_inst + inst);
    wrapper->power_metrics_calculations(); // 평균/최대/최소 갱신
    wrapper->dump();                       // 상세 출력
  }
}
```

## 8. 전력 모델 전체 요약

```
+-------------------------------------------------------------------+
|                     AccelWattch 전력 모델 요약                     |
+-------------------------------------------------------------------+
| 총 전력 = 동적 전력 + 상수 전력 + 정적 전력                       |
|                                                                    |
| 동적 전력 = Sum_i(접근_i x 계수_i x scaling_i) / 실행시간          |
|   - 계수_i: CACTI가 산출한 접근당 에너지 (기술 노드 기반)          |
|   - scaling_i: XML에서 정의한 활동 계수 (하드웨어 보정)            |
|   - 접근_i: 시뮬레이터 성능 카운터                                 |
|                                                                    |
| 상수 전력 = XML의 constant_power 값 (회귀 모델로 결정)             |
|   - 클럭 네트워크, 기타 상시 활동 로직 등                          |
|                                                                    |
| 정적 전력 = (base + (threads-1) x addlane) x active_core_ratio     |
|   - 명령어 유형 조합에 따라 base/addlane 선택                      |
|   - 쓰레드 발산 영향을 선형 모델로 포착                            |
|                                                                    |
| DVFS: 동적 전력 *= V_ratio^2, 정적 전력 *= V_ratio                |
+-------------------------------------------------------------------+
```

## 참고 파일 경로

| 파일 | 경로 |
|------|------|
| Wrapper 헤더 | `src/accelwattch/gpgpu_sim_wrapper.h` |
| Wrapper 구현 | `src/accelwattch/gpgpu_sim_wrapper.cc` |
| Processor 모델 | `src/accelwattch/processor.h`, `processor.cc` |
| Core 모델 | `src/accelwattch/core.h`, `core.cc` |
| SharedCache 모델 | `src/accelwattch/sharedcache.h`, `sharedcache.cc` |
| MemoryController 모델 | `src/accelwattch/memoryctrl.h`, `memoryctrl.cc` |
| Interconnect 모델 | `src/accelwattch/interconnect.h`, `interconnect.cc` |
| NoC 모델 | `src/accelwattch/noc.h`, `noc.cc` |
| Power Interface | `src/gpgpu-sim/power_interface.h`, `power_interface.cc` |
| XML Parser | `src/accelwattch/XML_Parse.h`, `XML_Parse.cc` |
| QV100 XML 설정 | `configs/tested-cfgs/SM7_QV100/accelwattch_sass_sim.xml` |
| CACTI 라이브러리 | `src/accelwattch/cacti/` |
