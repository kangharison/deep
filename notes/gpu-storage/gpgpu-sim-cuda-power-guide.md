# GPGPU-Sim 심층 분석 Part 3: CUDA 시뮬레이션, PTX 실행, 전력 모델, 설정

> 소스 위치: `sources/gpgpu-sim_distribution/`
> 관련 소스: `src/cuda-sim/`, `src/accelwattch/`, `src/gpgpu-sim/power_interface.cc`, `libcuda/`

---

## 16. CUDA 기능 시뮬레이션 (cuda-sim)

### 16.1 기능 시뮬레이션 vs 타이밍 시뮬레이션

GPGPU-Sim은 두 가지 시뮬레이션 모드를 제공한다.

```
+-----------------------------+        +-----------------------------+
|   기능적 시뮬레이션 (Func)    |        |   타이밍 시뮬레이션 (Perf)    |
+-----------------------------+        +-----------------------------+
| - 사이클 개념 없음            |        | - 사이클 단위로 진행          |
| - PTX 명령어를 순차 해석      |        | - 파이프라인, 캐시, DRAM 모델  |
| - 정확성(correctness) 검증   |        | - IPC, 지연시간 등 성능 측정   |
| - CTA 하나씩 순차 실행       |        | - 여러 CTA를 SM에 동시 할당   |
| - gpgpu_ptx_sim_mode = 1    |        | - gpgpu_ptx_sim_mode = 0     |
+-----------------------------+        +-----------------------------+
```

모드 설정은 `cuda_sim::g_ptx_sim_mode` 변수로 결정된다. 기능 시뮬레이션은 `functionalCoreSim::execute()` 메서드가 CTA를 하나씩 완료될 때까지 실행한다. 타이밍 시뮬레이션에서는 `shader_core_ctx`가 매 사이클마다 파이프라인 스테이지를 시뮬레이션하면서 동일한 PTX 명령어 실행 함수를 호출한다.

핵심 차이: 타이밍 모드에서도 기능 시뮬레이션 코드(`ptx_exec_inst`)가 동일하게 사용되며, 단지 타이밍 모델이 **언제** 그 명령어를 실행할지를 결정하는 래퍼가 추가된다.

### 16.2 cuda_sim 클래스

`cuda_sim`은 CUDA 기능 시뮬레이션의 전역 상태를 관리하는 중앙 클래스다. `gpgpu_context::func_sim`으로 접근한다.

```
소스: src/cuda-sim/cuda-sim.h (179행~)

class cuda_sim {
    unsigned g_ptx_sim_num_insn;        // 시뮬레이션된 총 PTX 명령어 수
    int g_ptx_kernel_count;             // 실행된 커널 누적 카운트
    int g_ptx_sim_mode;                 // 0=타이밍, 1=기능적 시뮬레이션
    unsigned g_assemble_code_next_pc;   // PTX 어셈블리 시 다음 전역 PC
    unsigned g_ptx_thread_info_uid_next;// 다음 스레드 UID
    std::map<unsigned, function_info*> g_pc_to_finfo;  // PC → 함수 매핑
    std::map<function_info*, rec_pts> g_rpts;           // 재수렴 포인트 캐시

    // opcode별 레이턴시 설정 (문자열)
    char *opcode_latency_int;    // "1,1,19,25,145,32" (ADD,MAX,MUL,MAD,DIV,SHFL)
    char *opcode_latency_fp;     // "1,1,1,1,30"
    char *opcode_latency_dp;     // "8,8,8,8,335"
    char *opcode_latency_sfu;    // "8"
    char *opcode_latency_tensor; // "64"
    // 각 유닛의 initiation interval도 동일 형식
    char *opcode_initiation_int; // "1,1,4,4,32,4"
    ...
    unsigned cdp_latency[5];     // CDP API 오버헤드 사이클
};
```

주요 함수:
- `gpgpu_cuda_ptx_sim_main_func()`: 기능 시뮬레이션 메인 루프, CTA를 순차적으로 실행
- `find_reconvergence_points()`: PDOM 분석으로 재수렴 포인트 계산 (캐시)
- `get_converge_point()`: 특정 분기 PC의 재수렴 PC 반환
- `gpgpu_ptx_sim_memcpy_symbol()`: cudaMemcpyToSymbol/FromSymbol 구현

### 16.3 functionalCoreSim 클래스

기능 시뮬레이션 전용 가상 코어. `core_t`를 상속받아 SIMT 스택과 워프 실행 기능을 재사용한다.

```
소스: src/cuda-sim/cuda-sim.h (85행~)

class functionalCoreSim : public core_t {
    unsigned *m_liveThreadCount;  // 워프별 살아있는 스레드 수
    bool *m_warpAtBarrier;       // 워프별 barrier 대기 상태

    void execute(int inst_count, unsigned ctaid_cp);  // 메인 실행 루프
    void executeWarp(unsigned, bool &allAtBarrier, bool &someOneLive);
    void initializeCTA(unsigned ctaid_cp);             // CTA 스레드 초기화
    void createWarp(unsigned warpId);                  // SIMT 스택 초기화
};
```

실행 흐름:
```
execute()
  ├── initializeCTA()       // CTA의 모든 스레드 생성, PC 설정
  │    └── ptx_sim_init_thread()  // 개별 스레드 초기화
  │         └── createWarp()      // SIMT 스택 launch
  └── while(someOneLive && !allAtBarrier)
       └── executeWarp(warpId)
            ├── SIMT 스택에서 active mask 취득
            ├── ptx_fetch_inst()   // PC에서 명령어 인출
            ├── ptx_exec_inst()    // 명령어 실행
            └── checkExecutionStatusAndUpdate()
                 // 스레드 종료 시 liveThreadCount--
```

### 16.4 ptx_thread_info: 스레드 상태 관리

GPU 스레드 하나의 전체 실행 상태를 모델링하는 핵심 클래스. 실제 GPU에서 각 스레드가 독립적인 레지스터 파일, PC, 콜스택을 갖는 것을 소프트웨어로 구현한다.

```
소스: src/cuda-sim/ptx_sim.h (374행~)

class ptx_thread_info {
    // === 식별 정보 ===
    unsigned m_uid;          // 전역 고유 스레드 ID (모든 커널에 걸쳐 고유)
    kernel_info_t &m_kernel; // 소속 커널 참조
    core_t *m_core;          // 할당된 SM
    gpgpu_t *m_gpu;          // GPU 객체

    // === CUDA 차원 정보 ===
    dim3 m_tid;    // threadIdx (블록 내 스레드 위치)
    dim3 m_ctaid;  // blockIdx  (그리드 내 블록 위치)
    dim3 m_ntid;   // blockDim  (블록 크기)
    dim3 m_nctaid; // gridDim   (그리드 크기)

    // === 하드웨어 매핑 ===
    unsigned m_hw_sid;   // SM ID
    unsigned m_hw_wid;   // 워프 ID (SM 내)
    unsigned m_hw_tid;   // 스레드 ID (SM 내)
    unsigned m_hw_ctaid; // CTA 슬롯 ID (SM 내)

    // === 실행 상태 ===
    unsigned m_PC;   // 현재 프로그램 카운터
    unsigned m_NPC;  // 다음 PC (분기 시 타겟 주소)
    unsigned m_icount;          // 실행한 명령어 수
    bool m_thread_done;         // EXIT 실행 여부
    bool m_branch_taken;        // 마지막 분기 방향
    unsigned m_RPC;             // 재수렴 PC

    // === 레지스터 파일 ===
    // std::list<reg_map_t> m_regs;  (콜스택 프레임별 레지스터 맵)
    // reg_map_t = tr1_hash_map<const symbol*, ptx_reg_t>

    // === 메모리 접근 결과 ===
    addr_t m_last_effective_address;
    memory_space_t m_last_memory_space;
    dram_callback_t m_last_dram_callback;

    // === 메모리 공간 포인터 ===
    memory_space *m_shared_mem;  // CTA shared memory
    memory_space *m_local_mem;   // 스레드 local memory

    // === 콜스택 ===
    std::list<stack_entry> m_callstack;
    unsigned m_local_mem_stack_pointer;
};
```

레지스터 파일은 `reg_map_t`(해시맵: symbol* -> ptx_reg_t)의 리스트로 구현된다. 함수 호출(CALL) 시 새 프레임이 push되고, 리턴(RET) 시 pop된다.

```cpp
// 레지스터 쓰기 (src/cuda-sim/instructions.cc, 196행)
void ptx_thread_info::set_reg(const symbol *reg, const ptx_reg_t &value) {
    if (reg->name() == "_") return;  // dummy 레지스터 무시
    m_regs.back()[reg] = value;       // 현재 프레임의 맵에 저장
}

// 레지스터 읽기 - 미초기화 레지스터는 0으로 초기화하고 경고
ptx_reg_t ptx_thread_info::get_reg(const symbol *reg) {
    reg_map_t::iterator it = m_regs.back().find(reg);
    if (it == m_regs.back().end()) {
        ptx_reg_t uninit_reg; uninit_reg.u32 = 0x0;
        set_reg(reg, uninit_reg);  // 0으로 초기화
        // WARNING 출력
    }
    return it->second;
}
```

### 16.5 ptx_reg_t: PTX 레지스터 공용체

PTX의 untyped register 모델을 반영하는 union 타입. 같은 비트를 다양한 타입으로 해석한다.

```
소스: src/cuda-sim/ptx_sim.h (76행~)

union ptx_reg_t {
    signed char s8;        // .s8
    signed short s16;      // .s16
    signed int s32;        // .s32
    signed long long s64;  // .s64
    unsigned char u8;      // .u8
    unsigned short u16;    // .u16
    unsigned int u32;      // .u32  ← 가장 빈번히 사용
    unsigned long long u64;// .u64  ← 포인터/주소용
    half f16;              // .f16 (IEEE 754 반정밀도)
    float f32;             // .f32 (IEEE 754 단정밀도)
    double f64;            // .f64 (IEEE 754 배정밀도)
    struct { unsigned ls; unsigned ms; } bits;  // 64비트를 32비트 쌍으로
    struct { unsigned lowest, low, high, highest; } u128; // 128비트 (v4 연산)
    unsigned pred : 4;     // predicate 비트필드 (4비트)
};
```

### 16.6 ptx_cta_info: CTA 관리

CTA(= CUDA 블록) 내 스레드 추적, barrier 동기화, 종료 검증을 담당한다.

```
소스: src/cuda-sim/ptx_sim.h (190행~), ptx_sim.cc (46행~)

class ptx_cta_info {
    unsigned m_sm_idx;     // SM_ID * max_CTA_per_SM + CTA_slot
    unsigned long long m_uid;  // 전역 고유 CTA ID
    unsigned m_bar_threads;    // barrier 도달 스레드 수

    std::set<ptx_thread_info*> m_threads_in_cta;          // 등록된 스레드
    std::set<ptx_thread_info*> m_threads_that_have_exited; // EXIT 실행 스레드
    std::set<ptx_thread_info*> m_dangling_pointers;        // 삭제된 스레드 포인터
};
```

- `add_thread()`: 스레드 생성 시 CTA에 등록
- `register_thread_exit()`: EXIT 실행 시 종료 등록
- `check_cta_thread_status_and_reset()`: CTA 완료 확인 후 슬롯 재사용 위해 리셋
- `inc_bar_threads()` / `reset_bar_threads()`: `__syncthreads()` barrier 관리

### 16.7 GPU 메모리 복사

`gpgpu_t::memcpy_to_gpu()`는 cudaMemcpy(HostToDevice)의 시뮬레이터 구현이다. PCIe 전송을 시뮬레이션하지 않고 즉시 바이트 단위로 글로벌 메모리에 복사한다.

```cpp
// src/cuda-sim/cuda-sim.cc (568행~)
void gpgpu_t::memcpy_to_gpu(size_t dst_start_addr, const void *src, size_t count) {
    char *src_data = (char *)src;
    for (unsigned n = 0; n < count; n++)
        m_global_mem->write(dst_start_addr + n, 1, src_data + n, NULL, NULL);
    // 성능 모델에도 통지 (캐시 상태 업데이트용)
    gpgpu_ctx->the_gpgpusim->g_the_gpu->perf_memcpy_to_gpu(dst_start_addr, count);
}
```

GPU 메모리 할당(`gpu_malloc`)은 단순 bump allocator로 256바이트 정렬을 보장한다:
```cpp
void *gpgpu_t::gpu_malloc(size_t size) {
    unsigned long long result = m_dev_malloc;
    m_dev_malloc += size;
    if (size % 256) m_dev_malloc += (256 - size % 256);  // 256B 정렬
    return (void *)result;
}
```

### 16.8 워프 실행 흐름: fetch → exec → 메모리 접근 생성

```
ptx_thread_info::ptx_fetch_inst(inst)
  → 현재 PC에서 명령어 인출 → inst에 저장

ptx_thread_info::ptx_exec_inst(warp_inst_t &inst, unsigned lane_id)
  → xxx_impl() 함수 호출 (명령어별 실행)
  → 메모리 접근 시:
     - m_last_effective_address = 유효 주소
     - m_last_memory_space = global/shared/local/...
  → 분기 시: m_branch_taken 설정
  → inst에 메모리 주소, 활성 마스크 기록
```

### 16.9 메모리 공간 모델

CUDA의 generic 주소 공간을 물리적 메모리 공간으로 매핑하는 주소 변환 체계:

```
Generic 주소 공간 레이아웃:
┌─────────────────────────────────────────────┐
│ [0, STATIC_ALLOC_LIMIT)        → global     │  정적 전역 변수
│ [LOCAL_GENERIC_START, ...)     → local      │  스레드별 로컬
│ [SHARED_GENERIC_START, ...)    → shared     │  CTA 공유
│ [GLOBAL_HEAP_START, ...)       → global     │  cudaMalloc 동적 할당
└─────────────────────────────────────────────┘
```

변환 함수 (src/cuda-sim/cuda-sim.cc, 451행~):
- `shared_to_generic(smid, addr)` = `SHARED_GENERIC_START + smid * SHARED_MEM_SIZE_MAX + addr`
- `local_to_generic(smid, hwtid, addr)` = `LOCAL_GENERIC_START + TOTAL_LOCAL_MEM_PER_SM*smid + LOCAL_MEM_SIZE_MAX*hwtid + addr`
- `global_to_generic(addr)` = `addr` (항등 변환)
- `whichspace(addr)`: generic 주소가 어떤 공간에 속하는지 판별

### 16.10 동적 병렬성 (CUDA Dynamic Parallelism, CDP)

커널 내에서 커널을 런치하는 기능. CDP API별 오버헤드 사이클이 설정 가능하다:

```
cdp_latency[5] = {
    7200,    // cudaStreamCreateWithFlags
    8000,    // cudaGetParameterBufferV2 (초기, 워프당)
    100,     // cudaGetParameterBufferV2 (커널당)
    12000,   // cudaLaunchDeviceV2 (초기, 워프당)
    1600     // cudaLaunchDeviceV2 (커널당)
};
```

CDP 지원은 `cuda_device_runtime` 클래스(`src/cuda-sim/cuda_device_runtime.h`)에서 구현되며, `gpgpu_context::device_runtime`으로 접근한다.

### 16.11 내장 변수(Built-in Special Registers)

PTX의 특수 레지스터 값은 `ptx_thread_info::get_builtin()`에서 반환된다:

```cpp
// src/cuda-sim/ptx_sim.cc (254행~)
unsigned ptx_thread_info::get_builtin(int builtin_id, unsigned dim_mod) {
    switch (builtin_id & 0xFFFF) {
        case CLOCK_REG:    return gpu_sim_cycle + gpu_tot_sim_cycle;
        case TID_REG:      // %tid.{x,y,z} → m_tid.x/y/z
        case CTAID_REG:    // %ctaid.{x,y,z} → m_ctaid.x/y/z
        case NTID_REG:     // %ntid.{x,y,z} → m_ntid.x/y/z
        case NCTAID_REG:   // %nctaid.{x,y,z} → m_nctaid.x/y/z
        case LANEID_REG:   return get_hw_tid() % warp_size;
        case WARPSZ_REG:   return m_core->get_warp_size(); // 32
        case GRIDID_REG:   return m_gridid;
        ...
    }
}
```

---

## 17. PTX 명령어 실행

### 17.1 PTX ISA 개요

PTX(Parallel Thread Execution)는 NVIDIA GPU의 **가상 ISA**다. CUDA 코드는 다음 경로로 변환된다:

```
CUDA C/C++ → (nvcc) → PTX → (ptxas JIT) → SASS (GPU 기계어)
```

GPGPU-Sim은 PTX 레벨에서 명령어를 해석/실행한다. PTX는 무한 레지스터(가상 레지스터)를 가정하며, 실제 GPU의 SASS는 고정 크기 레지스터 파일에 할당한다.

### 17.2 명령어 실행 구조: xxx_impl() 패턴

모든 PTX 명령어는 `instructions.cc`에 `xxx_impl()` 함수로 1:1 대응된다. `opcodes.def` 파일에서 X-매크로 패턴으로 매핑을 정의한다:

```
소스: src/cuda-sim/opcodes.def

OP_DEF(ADD_OP,  add_impl,  "add",  1, 1)   // 산술: 덧셈
OP_DEF(MUL_OP,  mul_impl,  "mul",  1, 1)   // 산술: 곱셈
OP_DEF(LD_OP,   ld_impl,   "ld",   1, 5)   // 메모리: 로드
OP_DEF(ST_OP,   st_impl,   "st",   0, 5)   // 메모리: 스토어
OP_DEF(BRA_OP,  bra_impl,  "bra",  0, 3)   // 제어: 분기
OP_DEF(BAR_OP,  bar_impl,  "bar",  1, 3)   // 동기화: barrier
OP_DEF(ATOM_OP, atom_impl, "atom", 1, 3)   // 원자적 연산
OP_DEF(TEX_OP,  tex_impl,  "tex",  1, 6)   // 텍스처 샘플링
...
```

각 필드: `OP_DEF(열거값, 실행함수, 문자열, 목적지여부, 분류코드)`
- 분류코드: 1=ALU, 2=MAD, 3=Control, 4=SFU, 5=Mem, 6=Tex, 7=NOP

### 17.3 주요 명령어 카테고리별 설명

#### 17.3.1 산술 명령어

| opcode | 함수 | 설명 | 분류 |
|--------|------|------|------|
| `add` | `add_impl` | 덧셈 (정수/FP) | ALU |
| `sub` | `sub_impl` | 뺄셈 | ALU |
| `mul` | `mul_impl` | 곱셈 | ALU |
| `mad` | `mad_impl` | 곱하고 더하기 (a*b+c) | MAD |
| `fma` | `fma_impl` | Fused Multiply-Add | MAD |
| `div` | `div_impl` | 나눗셈 | ALU |
| `rem` | `rem_impl` | 나머지 | ALU |
| `abs` | `abs_impl` | 절대값 | ALU |
| `neg` | `neg_impl` | 부호 반전 | ALU |
| `min` | `min_impl` | 최솟값 | ALU |
| `max` | `max_impl` | 최댓값 | ALU |
| `sad` | `sad_impl` | Sum of Absolute Differences | ALU |

#### 17.3.2 논리/비트 명령어

| opcode | 함수 | 설명 |
|--------|------|------|
| `and` | `and_impl` | 비트 AND |
| `or` | `or_impl` | 비트 OR |
| `xor` | `xor_impl` | 비트 XOR |
| `not` | `not_impl` | 비트 NOT |
| `shl` | `shl_impl` | 왼쪽 시프트 |
| `shr` | `shr_impl` | 오른쪽 시프트 |
| `bfe` | `bfe_impl` | Bit Field Extract |
| `bfi` | `bfi_impl` | Bit Field Insert |
| `bfind` | `bfind_impl` | 최상위 set bit 찾기 |
| `brev` | `brev_impl` | 비트 순서 뒤집기 |
| `popc` | `popc_impl` | Population Count (1의 개수) |
| `clz` | `clz_impl` | 선행 0 비트 수 |
| `prmt` | `prmt_impl` | 바이트 퍼뮤테이션 |

#### 17.3.3 비교/선택 명령어

| opcode | 함수 | 설명 |
|--------|------|------|
| `setp` | `setp_impl` | 비교 후 predicate 설정 (p = a < b 등) |
| `set` | `set_impl` | 비교 후 정수 결과 (0 or 0xFFFFFFFF) |
| `selp` | `selp_impl` | Predicate 기반 선택 (d = p ? a : b) |
| `slct` | `slct_impl` | 부호 기반 선택 (d = c>=0 ? a : b) |

#### 17.3.4 메모리 명령어

| opcode | 함수 | 메모리 공간 | 설명 |
|--------|------|-------------|------|
| `ld` | `ld_impl` | global/shared/local/const | 로드 |
| `ldu` | `ldu_impl` | global (읽기 전용 캐시) | Uniform 로드 |
| `st` | `st_impl` | global/shared/local | 스토어 |
| `atom` | `atom_impl` | global/shared | 원자적 연산 |
| `red` | `red_impl` | global/shared | Reduction (결과 무시) |
| `prefetch` | `prefetch_impl` | - | 프리페치 힌트 |

`atom` 명령어가 지원하는 원자적 연산: `add`, `min`, `max`, `inc`, `dec`, `and`, `or`, `xor`, `exch`, `cas` (Compare-And-Swap)

#### 17.3.5 제어 흐름 명령어

| opcode | 함수 | 설명 |
|--------|------|------|
| `bra` | `bra_impl` | 조건/무조건 분기 |
| `call` | `call_impl` | 함수 호출 (직접) |
| `callp` | `callp_impl` | 함수 호출 (간접) |
| `ret` | `ret_impl` | 함수 리턴 |
| `exit` | `exit_impl` | 스레드 종료 |
| `bar` | `bar_impl` | Barrier 동기화 (__syncthreads) |
| `membar` | `membar_impl` | Memory barrier (fence) |
| `break` | `break_impl` | 루프 탈출 |
| `ssy` | `ssy_impl` | Set Synchronization point |

#### 17.3.6 특수 명령어

| opcode | 함수 | 설명 |
|--------|------|------|
| `tex` | `tex_impl` | 텍스처 샘플링 (분류코드 6) |
| `suld` | `suld_impl` | Surface 로드 |
| `sust` | `sust_impl` | Surface 스토어 |
| `shfl` | `shfl_impl` | 워프 셔플 (레인 간 데이터 교환) |
| `vote` | `vote_impl` | 워프 투표 (any/all/uni/ballot) |
| `activemask` | `activemask_impl` | 현재 활성 스레드 마스크 |
| `cos` | `cos_impl` | 코사인 (SFU) |
| `sin` | `sin_impl` | 사인 (SFU) |
| `lg2` | `lg2_impl` | log2 (SFU) |
| `ex2` | `ex2_impl` | 2^x (SFU) |
| `rcp` | `rcp_impl` | 역수 1/x (SFU) |
| `rsqrt` | `rsqrt_impl` | 역제곱근 (SFU) |
| `sqrt` | `sqrt_impl` | 제곱근 (SFU) |

#### 17.3.7 텐서 코어 (WMMA) 명령어

| opcode | 함수 | 설명 |
|--------|------|------|
| `mma` | `mma_impl` | Warp Matrix Multiply-Accumulate |
| `mma_load` | `mma_ld_impl` | 행렬 타일 로드 |
| `mma_store` | `mma_st_impl` | 행렬 타일 스토어 |

WMMA 연산은 워프(32스레드) 단위로 16x16, 32x8, 8x32 크기의 행렬 곱셈을 수행한다. 각 스레드가 행렬의 특정 원소를 담당하며, `thread_group_offset()` 함수가 스레드별 메모리 오프셋을 계산한다:

```cpp
// src/cuda-sim/instructions.cc (86행~)
unsigned thread_group_offset(int thread, unsigned wmma_type,
                             unsigned wmma_layout, unsigned type, int stride) {
    unsigned load_a_row[8] = {0, 128, 0, 128, 64, 192, 64, 192};
    unsigned thread_group = thread / 4;
    unsigned in_tg_index = thread % 4;
    // 타입(LOAD_A/B/C, STORE_D)과 레이아웃(ROW/COL)에 따라 다른 오프셋 패턴
    ...
}
```

### 17.4 operand_info: 피연산자 표현

명령어의 각 피연산자를 표현하는 클래스. 레지스터, 즉시값, 메모리 주소, 벡터, 빌트인 등 모든 유형을 처리한다.

```
소스: src/cuda-sim/ptx_ir.h (407행~)

class operand_info {
    enum operand_type m_type;  // reg_t, vector_t, builtin_t, address_t,
                               // memory_t, float_op_t, int_t, label_t, ...
    union {
        int m_int;
        unsigned m_unsigned;
        float m_float;
        double m_double;
        const symbol *m_symbolic;
        const symbol **m_vector_symbolic;  // 벡터 피연산자 (v2/v4/v8)
    } m_value;

    int m_addr_offset;        // 주소 오프셋 [reg+offset]
    memory_space_t m_addr_space; // 메모리 주소 공간 (PTXPlus용)
    int m_operand_lohi;       // lo/hi 선택 (하위/상위 16비트)
    bool m_operand_neg;       // 부호 반전
    addr_t m_const_mem_offset; // 상수 메모리 오프셋
};
```

피연산자 값 읽기는 `ptx_thread_info::get_operand_value()`에서 처리한다. 이 함수는 피연산자 타입에 따라:
- `reg_t`: `get_reg(symbol)` 호출
- `builtin_t`: `get_builtin(id, dim)` 호출
- `literal`: `get_literal_value()` 호출
- `memory_t`: 심볼의 주소 + 오프셋 계산
- `label_t`: 레이블의 PC 주소 반환

### 17.5 타입 변환: cvt 명령어

`cvt`(Convert) 명령어는 11가지 이상의 데이터 타입 간 변환을 처리한다. 소스 타입과 대상 타입의 조합에 따라 적절한 변환 함수가 호출된다.

지원 타입: `s8`, `s16`, `s32`, `s64`, `u8`, `u16`, `u32`, `u64`, `f16`, `f32`, `f64`

### 17.6 명령어 분류와 파이프라인 매핑

`ptx_instruction::set_opcode_and_latency()`에서 각 PTX 명령어를 실행 유닛과 레이턴시에 매핑한다:

```
소스: src/cuda-sim/cuda-sim.cc (870행~)

명령어 분류(op) → 파이프라인:
  ALU_OP  → 정수 ALU (INT unit)
  SP_OP   → SP(단정밀도 FP unit)
  DP_OP   → DP(배정밀도 unit)
  SFU_OP  → SFU(Special Function Unit)
  TENSOR_CORE_OP → Tensor Core

세부 분류(sp_op): set_mul_div_or_other_archop()
  FP_MUL_OP, FP_DIV_OP, FP_SQRT_OP, FP_SIN_OP, FP_EXP_OP, FP_LG_OP
  DP_MUL_OP, DP_DIV_OP, DP___OP
  INT_MUL24_OP, INT_MUL32_OP, INT_MUL_OP, INT_DIV_OP, INT__OP
  TENSOR__OP, TEX__OP
```

기본 레이턴시 설정 (사이클 수):

| 연산 | ADD | MAX | MUL | MAD | DIV | SHFL |
|------|-----|-----|-----|-----|-----|------|
| INT | 1 | 1 | 19 | 25 | 145 | 32 |
| FP32 | 1 | 1 | 1 | 1 | 30 | - |
| FP64 | 8 | 8 | 8 | 8 | 335 | - |
| SFU | 8 | | | | | |
| Tensor | 64 | | | | | |

### 17.7 PTX 로딩 파이프라인

CUDA 바이너리에서 PTX 코드가 시뮬레이터에 로드되는 전체 흐름:

```
CUDA 바이너리 (fatbin)
  │
  ▼  __cudaRegisterFatBinary()
cuobjdump으로 PTX 추출
  │
  ▼  gpgpu_ptx_sim_load_ptx_from_string()
PTX 렉서/파서 (flex/bison) → 심볼 테이블 + 명령어 리스트
  │
  ▼  gpgpu_ptxinfo_load_from_string()
ptxas 호출 → ptxinfo 파싱 (레지스터 수, 공유메모리 크기 등)
  │
  ▼  __cudaRegisterFunction()
호스트 함수 포인터 ↔ PTX 커널 이름 매핑
  │
  ▼  function_info::ptx_assemble()
명령어 메모리 배치 + 분기 레이블 → PC 주소 링킹
  │
  ▼  ptx_instruction::pre_decode()
명령어 분류, 레이턴시, 실행 유닛 결정
```

---

## 18. PTX 파싱 및 IR (Intermediate Representation)

### 18.1 PTX 파싱 파이프라인

```
PTX 소스 텍스트
  │
  ▼  flex 렉서 (ptx.l)
토큰 스트림
  │
  ▼  bison 파서 (ptx.y → ptx.tab.h)
파서 액션: symbol_table + ptx_instruction 리스트 생성
  │
  ▼  function_info 객체
명령어 리스트, 심볼 테이블, 파라미터 정보 보유
```

파서는 `ptx_recognizer` 클래스(`src/cuda-sim/ptx_parser.h`)가 관리하며, `gpgpu_context::ptx_parser`로 접근한다.

### 18.2 function_info: 함수/커널 정보

각 PTX 함수(커널 또는 __device__ 함수)를 표현하는 클래스:

```
function_info 주요 필드:
  m_name            // 함수 이름
  m_instructions    // PTX 명령어 리스트 (파서 출력)
  m_instr_mem       // 어셈블된 명령어 배열 (PC 인덱스)
  m_start_PC        // 함수 시작 PC
  m_n               // 명령어 메모리 크기
  m_symtab          // 함수 스코프 심볼 테이블
  m_assembled       // 어셈블리 완료 여부
  m_basic_blocks    // 기본 블록 배열 (CFG용)
  labels            // 레이블명 → 오프셋 매핑
```

### 18.3 symbol_table: 계층적 심볼 테이블

```
소스: src/cuda-sim/ptx_ir.h (324행~)

class symbol_table {
    std::map<std::string, symbol*> m_symbols;        // 이름 → 심볼
    std::map<std::string, function_info*> m_function_info_lookup; // 함수 조회
    symbol_table *m_parent;  // 부모 스코프 (계층 구조)

    // 메모리 할당기: 각 공간별 다음 할당 주소
    unsigned m_shared_next;  // shared memory 다음 주소
    unsigned m_global_next;  // global memory 다음 주소
    unsigned m_local_next;   // local memory 다음 주소
    unsigned m_const_next;   // constant memory 다음 주소
    unsigned m_tex_next;     // texture memory 다음 주소
    unsigned m_reg_allocator; // 다음 레지스터 번호
};
```

심볼 테이블은 계층 구조를 가진다:
- 전역 심볼 테이블 (`g_global_allfiles_symbol_table`): 모든 PTX 파일의 전역 변수/함수
- 함수별 심볼 테이블: 로컬 변수, 레지스터, 파라미터

### 18.4 basic_block_t와 CFG 구성

```
소스: src/cuda-sim/ptx_ir.h (920행~)

struct basic_block_t {
    unsigned bb_id;
    ptx_instruction *ptx_begin, *ptx_end;  // 블록의 시작/끝 명령어
    std::set<int> predecessor_ids;          // 선행 블록
    std::set<int> successor_ids;            // 후행 블록
    std::set<int> dominator_ids;            // 지배자(dominator) 집합
    std::set<int> postdominator_ids;        // 후위지배자(post-dominator) 집합
    int immediatepostdominator_id;          // 즉시 후위지배자 ID
    int immediatedominator_id;              // 즉시 지배자 ID
    bool is_entry, is_exit;                 // 진입/종료 블록 여부
};
```

기본 블록은 분기가 없는 직선 코드(straight-line code)의 최대 단위다. CFG(Control Flow Graph) 구성 과정:
1. `create_basic_blocks()`: 분기/레이블 기준으로 블록 분할
2. `connect_basic_blocks()`: 분기 타겟으로 에지 연결
3. `find_dominators()` / `find_idominators()`: 지배자 트리 계산
4. `find_postdominators()` / `find_ipostdominators()`: 후위지배자 계산 (Muchnick 알고리즘)

### 18.5 PDOM 트리와 SIMT 스택

후위지배자(Post-Dominator) 분석 결과는 SIMT 스택의 **재수렴 포인트**를 결정하는 데 사용된다:

```
SIMT 실행에서 분기 발산 처리:

if (threadIdx.x < 16) {    // 분기점 PC=100
    A();                    // Taken 경로 PC=104
} else {
    B();                    // Not-taken 경로 PC=120
}
C();                        // 재수렴 포인트(IPDOM) PC=140

SIMT 스택 동작:
1. PC=100에서 분기 발산 감지
2. 스택에 push: (reconverge_PC=140, not_taken_mask, PC=120)
3. taken 경로 실행 (활성 mask = 하위 16스레드)
4. PC=140 도달 → pop → not-taken 경로 실행
5. PC=140 다시 도달 → 모든 스레드 재수렴
```

```
소스: src/cuda-sim/ptx_ir.h (959행~)

struct gpgpu_recon_t {
    address_type source_pc;   // 분기 명령어의 PC
    address_type target_pc;   // 재수렴 포인트(IPDOM)의 PC
    ptx_instruction *source_inst;
    ptx_instruction *target_inst;
};
```

`cuda_sim::find_reconvergence_points()`는 함수별로 PDOM 분석을 수행하고 결과를 `g_rpts` 맵에 캐시한다.

### 18.6 PTX 어셈블리 (명령어 메모리 배치)

```cpp
// src/cuda-sim/cuda-sim.cc (327행~)
void function_info::ptx_assemble() {
    // 1. 전역적으로 고유한 PC 할당 (함수 간 겹치지 않음)
    addr_t PC = gpgpu_ctx->func_sim->g_assemble_code_next_pc;
    PC += PC % MAX_INST_SIZE;  // MAX_INST_SIZE(8) 배수로 정렬
    m_start_PC = PC;

    // 2. 명령어 리스트를 순회하며 배치
    for (auto pI : m_instructions) {
        if (pI->is_label()) {
            labels[l->name()] = n;     // 레이블 → 오프셋 매핑
        } else {
            g_pc_to_finfo[PC] = this;  // PC → 함수 매핑
            m_instr_mem[n] = pI;       // 명령어 메모리에 배치
            pI->set_PC(PC);            // 명령어에 PC 설정
            n += pI->inst_size();
            PC += pI->inst_size();
        }
    }

    // 3. 분기 레이블을 PC 주소로 해석 (링킹)
    for (each branch instruction) {
        unsigned index = labels[target.name()];
        unsigned PC = m_instr_mem[index]->get_PC();
        m_symtab->set_label_address(target.get_symbol(), PC);
    }
}
```

---

## 19. GPU 메모리 시뮬레이션

### 19.1 memory_space 클래스 계층

```
소스: src/cuda-sim/memory.h

memory_space (추상 인터페이스)
  │
  └── memory_space_impl<BSIZE> (해시맵 기반 구현)
       │
       ├── memory_space_impl<32>     → local memory
       ├── memory_space_impl<64>     → shared memory
       ├── memory_space_impl<8192>   → 중간 크기
       └── memory_space_impl<16384>  → global/const/tex memory
```

```
memory_space 인터페이스:
┌─────────────────────────────────────────────┐
│ write(addr, length, data, thd, pI)          │ 쓰기 (워치포인트 검사)
│ write_only(index, offset, length, data)     │ 쓰기 (워치포인트 없음)
│ read(addr, length, data)                    │ 읽기
│ print(format, fout)                         │ 내용 출력
│ set_watch(addr, watchpoint)                 │ 워치포인트 설정
└─────────────────────────────────────────────┘
```

### 19.2 해시맵 기반 sparse 메모리 모델

`memory_space_impl<BSIZE>`는 해시맵(`mem_map<addr, mem_storage<BSIZE>>`)으로 구현된다. 접근된 블록만 on-demand로 생성되는 **sparse** 메모리 모델이다.

```
주소 변환:
  addr >> m_log2_block_size = 블록 인덱스 (해시 키)
  addr & (BSIZE - 1)       = 블록 내 오프셋

예: BSIZE=4096 (12비트)
  addr = 0x00001234
  블록 인덱스 = 0x00001234 >> 12 = 0x1
  블록 내 오프셋 = 0x00001234 & 0xFFF = 0x234
```

```
해시맵 구조:
┌────────────┐     ┌──────────────────┐
│ 블록 인덱스  │────→│ mem_storage<BSIZE>│
│ 0x0001     │     │ [BSIZE 바이트]    │
├────────────┤     └──────────────────┘
│ 0x0003     │────→ ...
├────────────┤
│ 0x00FF     │────→ ...
└────────────┘
(접근되지 않은 블록은 존재하지 않음 → 메모리 절약)
```

### 19.3 블록 경계를 넘는 접근 처리

```cpp
// src/cuda-sim/memory.cc (76행~)
void memory_space_impl<BSIZE>::write(mem_addr_t addr, size_t length, ...) {
    mem_addr_t index = addr >> m_log2_block_size;
    if ((addr + length) <= (index + 1) * BSIZE) {
        // Fast path: 단일 블록 내 접근 (대부분의 경우)
        unsigned offset = addr & (BSIZE - 1);
        m_data[index].write(offset, length, data);
    } else {
        // Slow path: 여러 블록에 걸치는 접근
        while (nbytes_remain > 0) {
            // 현재 블록의 끝까지만 쓰고 다음 블록으로 이동
            ...
        }
    }
    // 워치포인트 검사
    for (auto &wp : m_watchpoints) {
        if (주소 범위 겹침) hit_watchpoint(...);
    }
}
```

### 19.4 GPU 메모리 할당과 복사

```
cudaMalloc(devPtr, size)
  └→ gpgpu_t::gpu_malloc(size)
      └→ m_dev_malloc += size (bump allocator)
         └→ 256바이트 정렬

cudaMemcpy(dst, src, count, HostToDevice)
  └→ gpgpu_t::memcpy_to_gpu(dst, src, count)
      └→ 바이트 단위 m_global_mem->write()
         + perf_memcpy_to_gpu() (성능 모델 통지)

cudaMemcpy(dst, src, count, DeviceToHost)
  └→ gpgpu_t::memcpy_from_gpu(dst, src, count)
      └→ 바이트 단위 m_global_mem->read()

cudaMemset(devPtr, value, count)
  └→ gpgpu_t::gpu_memset(dst, c, count)
      └→ 바이트 단위 m_global_mem->write()
```

### 19.5 텍스처/서피스 바인딩 메커니즘

텍스처 바인딩은 3단계로 진행된다:

```
1. gpgpu_ptx_sim_bindNameToTexture(name, texref, dim, readmode, ext)
   → m_NameToTextureRef[name] = {texref}  (이름 → 참조 매핑)
   → m_TextureRefToName[texref] = name     (역방향 매핑)
   → m_NameToAttribute[name] = texAttr     (속성 저장)

2. gpgpu_ptx_sim_bindTextureToArray(texref, array)
   → m_NameToCudaArray[name] = array       (CUDA 배열 바인딩)
   → 2D 타일 크기(Tx, Ty) 계산:
     Tx = f(캐시라인크기), 텍셀크기에 따라 조정
     Ty = 캐시라인크기 / (Tx * 텍셀크기)
   → m_NameToTextureInfo[name] = texInfo   (타일 정보 저장)

3. tex_impl() 실행 시
   → 텍스처 좌표 → 텍셀 주소 변환
   → 텍스처 캐시를 통해 데이터 접근
```

### 19.6 Shared Memory 뱅크 충돌

GPGPU-Sim은 shared memory 접근의 뱅크 충돌을 타이밍 모델에서 계산한다. 주소의 하위 비트가 뱅크 번호를 결정한다:
- 뱅크 수: 32 (Volta 이후)
- 뱅크 폭: 4바이트
- 뱅크 번호 = `(addr / 4) % 32`
- 같은 뱅크에 다른 주소 접근 → 충돌 → 직렬화

---

## 20. AccelWattch 전력 모델

### 20.1 아키텍처 개요

AccelWattch는 GPGPU-Sim과 McPAT을 연결하여 GPU 전력을 추정하는 프레임워크다.

```
┌───────────────────────────────────────────────────┐
│                  GPGPU-Sim                        │
│  ┌──────────────┐   ┌──────────────────────────┐  │
│  │ 타이밍 모델    │──→│ power_stat_t             │  │
│  │ (셰이더코어,   │   │ 성능 카운터 수집:          │  │
│  │  캐시, DRAM)  │   │ - 명령어 수 (FP/INT/MEM) │  │
│  └──────────────┘   │ - 캐시 히트/미스          │  │
│                     │ - DRAM 읽기/쓰기          │  │
│                     │ - 레지스터 접근            │  │
│                     │ - 활성 SM/워프 수          │  │
│                     └──────────┬───────────────┘  │
│                                │                   │
│  ┌─────────────────────────────▼────────────────┐  │
│  │          gpgpu_sim_wrapper                    │  │
│  │  set_inst_power()    → 명령어 카운터          │  │
│  │  set_regfile_power() → RF 접근 카운터         │  │
│  │  set_l1cache_power() → L1 히트/미스           │  │
│  │  set_l2cache_power() → L2 히트/미스           │  │
│  │  set_mem_ctrl_power()→ DRAM 접근              │  │
│  │  set_NoC_power()     → 인터커넥트 트래픽      │  │
│  │          │                                    │  │
│  │          ▼                                    │  │
│  │  compute() → McPAT Processor::compute()       │  │
│  │  update_components_power()                    │  │
│  │  power_metrics_calculations()                 │  │
│  └──────────────────────────────────────────────┘  │
│                                │                   │
│  ┌─────────────────────────────▼────────────────┐  │
│  │            McPAT (Processor)                  │  │
│  │  ParseXML → GPU 아키텍처 파라미터 파싱        │  │
│  │  proc->compute() → 컴포넌트별 에너지 계산     │  │
│  └──────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────┘
```

### 20.2 전력 컴포넌트 분해

AccelWattch는 GPU를 33개 전력 컴포넌트로 분해한다:

```
소스: src/accelwattch/gpgpu_sim_wrapper.cc (41행~)

enum pwr_cmp_t {
    IBP = 0,       // Instruction Buffer Power
    ICP,           // I-Cache Power
    DCP,           // D-Cache (L1D) Power
    TCP,           // Texture Cache Power
    CCP,           // Constant Cache Power
    SHRDP,         // Shared Memory Power
    RFP,           // Register File Power
    INTP,          // Integer ALU Power
    FPUP,          // FPU Power
    DPUP,          // Double Precision Unit Power
    INT_MUL24P,    // INT 24-bit Multiply
    INT_MUL32P,    // INT 32-bit Multiply
    INT_MULP,      // INT General Multiply
    INT_DIVP,      // INT Division
    FP_MULP,       // FP Multiply
    FP_DIVP,       // FP Division
    FP_SQRTP,      // FP Square Root
    FP_LGP,        // FP Log2
    FP_SINP,       // FP Sin/Cos
    FP_EXP,        // FP Exp2
    DP_MULP,       // DP Multiply
    DP_DIVP,       // DP Division
    TENSORP,       // Tensor Core Power
    TEXP,          // Texture Unit Power
    SCHEDP,        // Scheduler Power
    L2CP,          // L2 Cache Power
    MCP,           // Memory Controller Power
    NOCP,          // Network on Chip Power
    DRAMP,         // DRAM Power
    PIPEP,         // Pipeline Power
    IDLE_COREP,    // Idle Core Power (Clock Gating)
    CONSTP,        // Constant Dynamic Power
    STATICP,       // Static (Leakage) Power
    NUM_COMPONENTS_MODELLED  // = 33
};
```

### 20.3 전력 계산 흐름

매 `stat_sample_freq` 사이클마다 `mcpat_cycle()` 함수가 호출된다:

```
소스: src/gpgpu-sim/power_interface.cc (65행~)

mcpat_cycle() 호출 (stat_sample_freq 사이클마다)
  │
  ├── wrapper->set_inst_power(...)       // 명령어 통계
  │     (clk_gated_lanes, cycles, tot_inst, int_inst, fp_inst,
  │      load_inst, store_inst, committed_inst)
  │
  ├── wrapper->set_regfile_power(reads, writes, ops)
  ├── wrapper->set_icache_power(hits, misses)
  ├── wrapper->set_ccache_power(accesses, 0)
  ├── wrapper->set_tcache_power(hits, misses)
  ├── wrapper->set_shrd_mem_power(accesses)
  ├── wrapper->set_l1cache_power(rd_hits, rd_miss, wr_hits, wr_miss)
  ├── wrapper->set_l2cache_power(rd_hits, rd_miss, wr_hits, wr_miss)
  │
  ├── wrapper->set_num_cores(num_cores)
  ├── wrapper->set_idle_core_power(num_idle_core)
  ├── wrapper->set_duty_cycle_power(pipeline_duty_cycle)  // ≤ 0.8 상한
  │
  ├── wrapper->set_mem_ctrl_power(dram_rd, dram_wr, dram_pre)
  │
  ├── wrapper->set_int_accesses(ialu, imul24, imul32, imul, idiv)
  ├── wrapper->set_fp_accesses(fpu, fpmul, fpdiv)
  ├── wrapper->set_dp_accesses(dpu, dpmul, dpdiv)
  ├── wrapper->set_trans_accesses(sqrt, log, sin, exp)
  ├── wrapper->set_tensor_accesses(tensor)
  ├── wrapper->set_tex_accesses(tex)
  ├── wrapper->set_exec_unit_power(fpu, ialu, sfu)
  │
  ├── wrapper->set_avg_active_threads(active_threads)
  ├── wrapper->set_active_lanes_power(sp_lanes, sfu_lanes)  // ≤ 32 상한
  ├── wrapper->set_NoC_power(simt_to_mem + mem_to_simt)
  │
  ├── wrapper->compute()           // McPAT 전력 계산 실행
  ├── wrapper->update_components_power()  // 컴포넌트별 전력 분해
  └── wrapper->power_metrics_calculations()  // avg/max/min 통계 갱신
```

### 20.4 동적 전력 vs 정적(누설) 전력

```
총 전력 = 동적 전력 + 정적 전력 + 상수 동적 전력

동적 전력 (Dynamic Power):
  P_dynamic = α × C × V² × f
  - α: 활동률 (activity factor, 성능 카운터에서 계산)
  - C: 게이트 용량 (McPAT/CACTI에서 추정)
  - V: 공급 전압
  - f: 동작 주파수

정적 전력 (Static/Leakage Power):
  P_static = V × I_leak
  - 트랜지스터 누설 전류 (온도, 공정에 의존)
  - McPAT에서 기술 노드별 모델링

상수 동적 전력 (Constant Dynamic Power):
  - 클럭 분배 네트워크 등 활동률과 무관하게 항상 소비
  - const_dynamic_power 변수
```

### 20.5 DVFS: 전압/주파수 스케일링

```
소스: gpgpu_sim_wrapper.h (127행)

void set_model_voltage(double model_voltage);

DVFS 전력 스케일링:
  동적 전력 ∝ V²  (전압 제곱에 비례)
  정적 전력 ∝ V    (전압에 선형 비례)

modeled_chip_voltage 변수가 스케일링 팩터로 사용
```

### 20.6 전력 스케일링 계수 (PowerscalingCoefficients)

명령어 종류별로 다른 전력 소비를 모델링하기 위한 계수:

```
소스: src/accelwattch/gpgpu_sim_wrapper.h (73행~)

struct PowerscalingCoefficients {
    double int_coeff;          // 정수 연산
    double int_mul_coeff;      // 정수 곱셈
    double int_mul24_coeff;    // 24비트 정수 곱셈
    double int_mul32_coeff;    // 32비트 정수 곱셈
    double int_div_coeff;      // 정수 나눗셈
    double fp_coeff;           // 단정밀도 FP
    double dp_coeff;           // 배정밀도 FP
    double fp_mul_coeff, fp_div_coeff;
    double dp_mul_coeff, dp_div_coeff;
    double sqrt_coeff, log_coeff, sin_coeff, exp_coeff;  // SFU 연산
    double tensor_coeff;       // 텐서 코어
    double tex_coeff;          // 텍스처 유닛
};
```

### 20.7 power_stat_t: 성능 카운터 수집

`power_stat_t` 구조체(`src/gpgpu-sim/power_stat.h`)는 전력 모델에 필요한 성능 카운터를 수집한다. 각 카운터는 `[CURRENT_STAT_IDX]`와 `[PREV_STAT_IDX]` 두 개의 슬롯을 유지하여 구간별 차분(delta)을 계산한다.

```
소스: src/gpgpu-sim/power_stat.h (55행~)

struct shader_core_power_stats_pod {
    float *m_pipeline_duty_cycle[2];    // 파이프라인 활성 비율
    unsigned *m_num_decoded_insn[2];    // 디코드된 총 명령어 수
    unsigned *m_num_FPdecoded_insn[2];  // FP 명령어 수
    unsigned *m_num_INTdecoded_insn[2]; // INT 명령어 수
    unsigned *m_num_storequeued_insn[2];
    unsigned *m_num_loadqueued_insn[2];
    unsigned *m_num_tex_inst[2];        // 텍스처 명령어 수

    // 실행 유닛별 접근 카운터 (double: 워프 내 활성 레인 비율 반영)
    double *m_num_ialu_acesses[2];      // IALU
    double *m_num_fp_acesses[2];        // FPU
    double *m_num_dp_acesses[2];        // DPU
    double *m_num_sp_acesses[2];        // SP 유닛
    double *m_num_sfu_acesses[2];       // SFU
    double *m_num_tensor_core_acesses[2]; // 텐서 코어
    double *m_num_tex_acesses[2];       // 텍스처 유닛

    // 세부 연산별 카운터
    double *m_num_imul_acesses[2];      // 정수 곱셈
    double *m_num_imul24_acesses[2];    // 24비트 곱셈
    double *m_num_imul32_acesses[2];    // 32비트 곱셈
    double *m_num_idiv_acesses[2];      // 정수 나눗셈
    double *m_num_fpmul_acesses[2];     // FP 곱셈
    double *m_num_fpdiv_acesses[2];     // FP 나눗셈
    double *m_num_dpmul_acesses[2];     // DP 곱셈
    double *m_num_dpdiv_acesses[2];     // DP 나눗셈
    double *m_num_sqrt_acesses[2];      // sqrt
    double *m_num_log_acesses[2];       // log
    double *m_num_sin_acesses[2];       // sin
    double *m_num_exp_acesses[2];       // exp

    // 캐시/메모리 통계
    double *m_num_regfile_reads[2];     // RF 읽기
    double *m_num_regfile_writes[2];    // RF 쓰기
    ...
};
```

---

## 21. CUDA Runtime API 인터셉트

### 21.1 LD_PRELOAD 기반 라이브러리 교체 메커니즘

GPGPU-Sim은 CUDA Runtime API를 인터셉트하여 실제 GPU 대신 시뮬레이터를 사용한다. `libcuda/cuda_runtime_api.cc`가 `libcudart.so`로 컴파일되어 LD_PRELOAD 또는 직접 링크로 실제 NVIDIA 라이브러리를 대체한다.

```
실제 GPU 실행:
  CUDA App → libcudart.so (NVIDIA) → GPU Driver → GPU Hardware

GPGPU-Sim 시뮬레이션:
  CUDA App → libcudart.so (GPGPU-Sim) → gpgpu_sim → 소프트웨어 시뮬레이션
```

### 21.2 gpgpu_context: 전역 시뮬레이션 컨텍스트

```
소스: libcuda/gpgpu_context.h

class gpgpu_context {
    // 서브시스템 객체 포인터
    cuda_runtime_api *api;         // CUDA API 인터셉션
    ptxinfo_data *ptxinfo;         // ptxas 출력 파서
    ptx_recognizer *ptx_parser;    // PTX 파서
    GPGPUsim_ctx *the_gpgpusim;    // GPU 시뮬레이터 코어
    cuda_sim *func_sim;            // 기능적 시뮬레이션
    cuda_device_runtime *device_runtime; // CDP 지원
    ptx_stats *stats;              // 통계 수집

    // 전역 상태
    symbol_table *g_global_allfiles_symbol_table;
    std::vector<ptx_instruction*> s_g_pc_to_insn;  // PC → 명령어 매핑
    unsigned kernel_info_m_next_uid;                // 커널 UID 카운터
};

// 싱글턴 접근
gpgpu_context *GPGPU_Context() {
    static gpgpu_context *gpgpu_ctx = NULL;
    if (!gpgpu_ctx) gpgpu_ctx = new gpgpu_context();
    return gpgpu_ctx;
}
```

### 21.3 주요 API 인터셉트

#### cudaMalloc → gpu_malloc

```
cudaMalloc(&devPtr, size)
  └→ gpgpu_t::gpu_malloc(size)
      └→ bump allocator (m_dev_malloc += size, 256B 정렬)
      └→ devPtr = 할당된 시뮬레이션 주소
```

#### cudaMemcpy → memcpy (즉시 복사)

```
cudaMemcpy(dst, src, count, kind)
  ├── HostToDevice:   gpgpu_t::memcpy_to_gpu(dst, src, count)
  │                   바이트 단위 global_mem->write()
  ├── DeviceToHost:   gpgpu_t::memcpy_from_gpu(dst, src, count)
  │                   바이트 단위 global_mem->read()
  └── DeviceToDevice: gpgpu_t::memcpy_gpu_to_gpu(dst, src, count)

주의: PCIe 전송 지연은 시뮬레이션하지 않음 (즉시 복사)
```

#### cudaLaunchKernel → stream_manager::push

```
cudaLaunchKernel(func, gridDim, blockDim, args, sharedMem, stream)
  │
  ├── 1. function_info 조회 (호스트 함수 포인터 → PTX 커널)
  ├── 2. kernel_info_t 생성 (그리드/블록 차원, 인자, 공유메모리)
  ├── 3. stream_operation 생성 (KERNEL_LAUNCH 타입)
  └── 4. stream_manager->push(stream_operation)
         → 시뮬레이션 스레드가 이를 감지하고 커널 실행 시작
```

#### cudaDeviceSynchronize → 시뮬레이션 완료 대기

```
cudaDeviceSynchronize()
  └→ gpgpu_context::synchronize()
      └→ 모든 스트림의 모든 작업이 완료될 때까지 블로킹 대기
         (시뮬레이션 스레드가 GPU 사이클을 계속 진행)
```

### 21.4 커널 등록 흐름

```
__cudaRegisterFatBinary(fatCubin)
  └→ fat binary에서 PTX 코드 추출 (cuobjdump 사용)
     → gpgpu_ptx_sim_load_ptx_from_string()
        → PTX 파서 실행 → 심볼 테이블 + 명령어 리스트 생성
     → gpgpu_ptxinfo_load_from_string()
        → ptxas 실행 → 리소스 정보 (레지스터 수, smem 등) 파싱

__cudaRegisterFunction(fatCubinHandle, hostFun, deviceFun, ...)
  └→ 호스트 함수 포인터를 PTX 커널 이름에 매핑
     → 이후 cudaLaunchKernel 시 이 매핑으로 PTX 커널을 찾음

cudaConfigureCall(gridDim, blockDim, sharedMem, stream)
  └→ 실행 설정(그리드/블록 차원)을 내부 스택에 push

cudaSetupArgument(arg, size, offset)
  └→ 커널 인자를 내부 버퍼에 복사

cudaLaunch(hostFun)
  └→ 스택에서 설정 pop → kernel_info_t 생성 → 스트림에 push
```

### 21.5 시뮬레이터 초기화 (GPGPUSim_Init)

```cpp
// libcuda/cuda_runtime_api.cc (246행~)
struct _cuda_device_id *gpgpu_context::GPGPUSim_Init() {
    if (!the_device) {
        gpgpu_sim *the_gpu = gpgpu_ptx_sim_init_perf();
        // gpgpusim.config 파일 읽기 → gpgpu_sim 인스턴스 생성

        cudaDeviceProp *prop = (cudaDeviceProp *)calloc(sizeof(cudaDeviceProp), 1);
        prop->name = "GPGPU-Sim_v버전";
        prop->major = compute_capability_major;  // 예: 7
        prop->minor = compute_capability_minor;  // 예: 0
        prop->totalGlobalMem = 0x80000000;       // 2GB
        prop->maxThreadsPerBlock = 1024;         // (CC >= 2.0)
        prop->warpSize = 32;
        prop->multiProcessorCount = num_shader();
        prop->sharedMemPerBlock = shared_mem_per_block();
        prop->regsPerBlock = num_registers_per_block();
        ...
    }
    start_sim_thread(1);  // 시뮬레이션 스레드 시작
}
```

---

## 22. 설정 및 튜닝

### 22.1 gpgpusim.config 주요 파라미터

Volta V100 (`configs/tested-cfgs/SM7_QV100/gpgpusim.config`) 기준:

#### 기본 아키텍처

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_n_clusters` | 80 | SM(클러스터) 수 |
| `-gpgpu_n_cores_per_cluster` | 1 | 클러스터당 코어 수 |
| `-gpgpu_n_mem` | 32 | 메모리 컨트롤러 수 |
| `-gpgpu_n_sub_partition_per_mchannel` | 2 | 채널당 서브파티션 수 |

#### SM 구성

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_shader_core_pipeline` | 2048:32 | SM당 최대 스레드:워프 크기 |
| `-gpgpu_shader_registers` | 65536 | SM당 총 레지스터 수 |
| `-gpgpu_shader_cta` | 32 | SM당 최대 CTA 수 |
| `-gpgpu_occupancy_sm_number` | 70 | 점유율 계산 기준 SM 번호 |

#### 실행 유닛

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_num_sp_units` | 4 | SP(단정밀도) 유닛 수 |
| `-gpgpu_num_dp_units` | 4 | DP(배정밀도) 유닛 수 |
| `-gpgpu_num_int_units` | 4 | INT(정수) 유닛 수 |
| `-gpgpu_num_sfu_units` | 4 | SFU 유닛 수 |
| `-gpgpu_num_tensor_core_units` | 4 | 텐서 코어 유닛 수 |
| `-gpgpu_tensor_core_avail` | 1 | 텐서 코어 활성화 |
| `-gpgpu_pipeline_widths` | 4,4,4,4,4,4,4,4,4,4,8,4,4 | 13개 파이프라인 스테이지 폭 |

파이프라인 폭 순서:
```
ID_OC_SP, ID_OC_DP, ID_OC_INT, ID_OC_SFU, ID_OC_MEM,
OC_EX_SP, OC_EX_DP, OC_EX_INT, OC_EX_SFU, OC_EX_MEM,
EX_WB, ID_OC_TENSOR_CORE, OC_EX_TENSOR_CORE
```

#### 클럭 도메인

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_clock_domains` | 1132.0:1132.0:1132.0:850.0 | Core:Interconnect:L2:DRAM (MHz) |

#### Compute Capability

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_compute_capability_major` | 7 | CC 주 버전 |
| `-gpgpu_compute_capability_minor` | 0 | CC 부 버전 |
| `-gpgpu_ptx_force_max_capability` | 70 | PTX 강제 CC |

#### 시뮬레이션 모드

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_ptx_sim_mode` | 0 | 0=타이밍, 1=기능적 |
| `-gpgpu_ptx_instruction_classification` | 0 | 명령어 분류 통계 수집 |
| `-gpgpu_simd_model` | 1 | SIMT 실행 모델 |

#### 디바이스 한계

| 파라미터 | 값 | 설명 |
|---------|-----|------|
| `-gpgpu_stack_size_limit` | 1024 | 스레드별 스택 크기 (바이트) |
| `-gpgpu_heap_size_limit` | 8388608 | 디바이스 힙 크기 (8MB) |
| `-gpgpu_max_concurrent_kernel` | 128 | 최대 동시 커널 수 |
| `-gpgpu_kernel_launch_latency` | 5000 | 커널 런치 오버헤드 (사이클) |

### 22.2 인터커넥트 설정

`config_volta_islip.icnt` 파일이 NoC 토폴로지를 정의한다. 일반적으로 butterfly나 mesh 토폴로지를 사용하며, ISLIP 중재 알고리즘으로 패킷 라우팅을 결정한다.

### 22.3 아키텍처별 프리셋

제공되는 테스트 완료 설정:

| 디렉토리 | GPU | 아키텍처 | SM 수 |
|----------|-----|---------|--------|
| `SM2_GTX480` | GTX 480 | Fermi (sm_20) | 15 |
| `SM3_KEPLER_TITAN` | TITAN | Kepler (sm_35) | 14 |
| `SM7_TITANV` | TITAN V | Volta (sm_70) | 80 |
| `SM7_QV100` | Quadro V100 | Volta (sm_70) | 80 |
| `SM7_GV100` | Tesla V100 | Volta (sm_70) | 80 |
| `SM75_RTX2060` | RTX 2060 | Turing (sm_75) | 30 |

deprecated 설정: `SM6_P100`, `SM6_TITANX`, `SM6_GTX1080`

---

## 23. 시뮬레이션 출력 및 통계 해석

### 23.1 주요 통계 항목

시뮬레이션 완료 시 출력되는 핵심 통계:

| 통계 항목 | 설명 | 이상적 값 |
|-----------|------|-----------|
| `gpu_sim_cycle` | 현재 커널의 시뮬레이션 사이클 수 | 낮을수록 좋음 |
| `gpu_tot_sim_cycle` | 전체 누적 사이클 수 | - |
| `gpu_ipc` | 사이클당 명령어 수 (IPC) | 높을수록 좋음 |
| `gpu_occupancy` | SM 점유율 (%) | 높을수록 좋음 (보통) |

#### 캐시 통계

| 통계 항목 | 설명 |
|-----------|------|
| `L1D_total_cache_accesses` | L1D 캐시 총 접근 수 |
| `L1D_total_cache_misses` | L1D 캐시 총 미스 수 |
| `L1D_total_cache_miss_rate` | L1D 미스율 (낮을수록 좋음) |
| `L2_total_cache_accesses` | L2 캐시 총 접근 수 |
| `L2_total_cache_misses` | L2 캐시 총 미스 수 |
| `L2_total_cache_miss_rate` | L2 미스율 |

#### DRAM 통계

| 통계 항목 | 설명 |
|-----------|------|
| `total_dram_reads` | DRAM 총 읽기 수 |
| `total_dram_writes` | DRAM 총 쓰기 수 |
| `dram_utilization` | DRAM 대역폭 활용률 |

### 23.2 워프 스톨 원인 분석

시뮬레이터는 워프가 실행되지 못하는 원인별 사이클 수를 추적한다:

```
워프 스톨 원인 (stall reasons):
┌──────────────────────┬────────────────────────────────────┐
│ Scoreboard           │ RAW 의존성 (이전 명령어 결과 대기)    │
│ Memory               │ 메모리 접근 결과 대기 (캐시 미스)     │
│ Barrier              │ __syncthreads() 대기               │
│ Pipeline Busy        │ 실행 유닛 충돌 (구조적 해저드)        │
│ Idle                 │ 모든 워프 종료/비활성                │
│ Not Selected         │ 스케줄러가 다른 워프 선택             │
└──────────────────────┴────────────────────────────────────┘
```

### 23.3 성능 병목 식별 방법

```
성능 병목 판단 흐름:

1. gpu_ipc가 낮은가?
   ├── YES → 워프 스톨 원인 확인
   │    ├── Memory 스톨 지배적 → 메모리 바운드
   │    │    ├── L1D miss rate 높음 → 데이터 지역성 문제
   │    │    ├── L2 miss rate 높음 → 워킹셋이 L2보다 큼
   │    │    └── DRAM util 높음 → 대역폭 포화
   │    ├── Scoreboard 스톨 지배적 → 계산 의존성 병목
   │    │    └── 레이턴시가 긴 명령어(DIV, SFU) 연쇄 사용
   │    ├── Barrier 스톨 지배적 → 동기화 오버헤드
   │    │    └── CTA 내 워프 간 작업량 불균형
   │    └── Pipeline Busy → 실행 유닛 부족
   │         └── 특정 유닛(SP/SFU/DP) 사용 집중
   └── NO → 충분히 효율적 또는 점유율 문제

2. 점유율(occupancy) 확인:
   ├── 레지스터 사용량 과다 → -maxrregcount 옵션
   ├── 공유 메모리 과다 사용 → 사용량 줄이기
   └── CTA당 스레드 수 비최적 → blockDim 조정
```

### 23.4 전력 관련 출력

전력 시뮬레이션 활성화 시 추가 출력:

```
gpgpu_simulation_power_report.log:
  - 커널별 평균/최대/최소 전력
  - 컴포넌트별 전력 분해 (IBP, ICP, DCP, ..., STATICP)

gpgpu_simulation_power_trace.log.gz:
  - 시간별 전력 트레이스 (stat_sample_freq 사이클 단위)

gpgpu_simulation_metric_trace.log.gz:
  - 시간별 성능 카운터 트레이스
```

---

## 소스 파일 인덱스

| 파일 | 역할 |
|------|------|
| `src/cuda-sim/cuda-sim.h` | CUDA 기능 시뮬레이션 헤더 (cuda_sim, functionalCoreSim) |
| `src/cuda-sim/cuda-sim.cc` | 어셈블리, 메모리 관리, 주소 변환, 명령어 분류 |
| `src/cuda-sim/ptx_sim.h` | PTX 스레드/CTA/워프 정보, ptx_reg_t |
| `src/cuda-sim/ptx_sim.cc` | 스레드 생성/소멸, 내장 변수, 콜스택 |
| `src/cuda-sim/ptx_ir.h` | PTX IR (symbol, operand_info, ptx_instruction, basic_block_t) |
| `src/cuda-sim/ptx_ir.cc` | IR 구현 (타입 디코딩, CFG, PDOM 분석) |
| `src/cuda-sim/instructions.cc` | PTX 명령어별 xxx_impl() 실행 함수 |
| `src/cuda-sim/opcodes.h` | opcode 열거형, 특수 레지스터, WMMA 타입 |
| `src/cuda-sim/opcodes.def` | 오피코드 정의 (X-매크로 패턴) |
| `src/cuda-sim/memory.h` | 메모리 공간 추상 인터페이스, 해시맵 구현 |
| `src/cuda-sim/memory.cc` | 메모리 읽기/쓰기, 워치포인트 |
| `src/accelwattch/gpgpu_sim_wrapper.h` | AccelWattch 전력 래퍼 (McPAT 인터페이스) |
| `src/accelwattch/gpgpu_sim_wrapper.cc` | 전력 컴포넌트 정의, McPAT 초기화 |
| `src/gpgpu-sim/power_interface.cc` | 성능 카운터 → 전력 모델 전달 |
| `src/gpgpu-sim/power_stat.h` | 성능 카운터 수집 (셰이더 코어별) |
| `libcuda/cuda_runtime_api.cc` | CUDA API 인터셉션 구현 |
| `libcuda/gpgpu_context.h` | 전역 시뮬레이터 컨텍스트 (싱글턴) |
