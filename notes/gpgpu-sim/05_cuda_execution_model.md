# GPGPU-Sim CUDA 실행 모델 심층 분석

## 소스코드 위치
- `src/cuda-sim/cuda-sim.h / .cc` - CUDA 시뮬레이션 메인
- `src/cuda-sim/instructions.cc` - PTX 명령어 구현 (~6600줄)
- `src/cuda-sim/ptx_ir.h / .cc` - PTX 중간 표현 (IR)
- `src/cuda-sim/ptx_sim.h / .cc` - PTX 시뮬레이션 코어
- `src/cuda-sim/ptx_loader.h / .cc` - PTX 로더
- `src/cuda-sim/ptx_parser.h / .cc` - PTX 파서
- `src/cuda-sim/memory.h / .cc` - 메모리 공간 시뮬레이션
- `src/cuda-sim/cuda_device_runtime.h / .cc` - CUDA Dynamic Parallelism (CDP) 지원
- `src/cuda-sim/opcodes.def` - opcode 매크로 정의 테이블

---

## 1. CUDA 실행 모델 개요

### 1.1 Grid -> Block -> Warp -> Thread 계층 구조

```
  ┌─────────────────────────────────────────────────────────────────────┐
  │                         CUDA Kernel (Grid)                         │
  │                    kernel_info_t 객체로 표현                        │
  │   gridDim = (Gx, Gy, Gz),  blockDim = (Bx, By, Bz)               │
  │                                                                     │
  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  │
  │  │   CTA (Block)    │  │   CTA (Block)    │  │   CTA (Block)    │  │
  │  │  ctaid=(0,0,0)   │  │  ctaid=(1,0,0)   │  │  ctaid=(2,0,0)   │  │
  │  │  ptx_cta_info    │  │  ptx_cta_info    │  │  ptx_cta_info    │  │
  │  │                  │  │                  │  │                  │  │
  │  │ ┌──────────────┐ │  │ ┌──────────────┐ │  │                  │  │
  │  │ │  Warp 0      │ │  │ │  Warp 0      │ │  │      ...         │  │
  │  │ │ ptx_warp_info│ │  │ │              │ │  │                  │  │
  │  │ │              │ │  │ │              │ │  │                  │  │
  │  │ │ T0 T1 .. T31 │ │  │ │ T0 T1 .. T31│ │  │                  │  │
  │  │ └──────────────┘ │  │ └──────────────┘ │  │                  │  │
  │  │ ┌──────────────┐ │  │ ┌──────────────┐ │  │                  │  │
  │  │ │  Warp 1      │ │  │ │  Warp 1      │ │  │                  │  │
  │  │ │ T32 T33..T63 │ │  │ │              │ │  │                  │  │
  │  │ └──────────────┘ │  │ └──────────────┘ │  │                  │  │
  │  │       ...        │  │       ...        │  │                  │  │
  │  └──────────────────┘  └──────────────────┘  └──────────────────┘  │
  └─────────────────────────────────────────────────────────────────────┘
```

### 1.2 핵심 클래스 관계도

```
  kernel_info_t                    function_info
  ├── gridDim, blockDim            ├── m_instructions (ptx_instruction 리스트)
  ├── entry() → function_info*     ├── m_instr_mem[] (어셈블된 명령어 배열)
  ├── active_threads()             ├── m_symtab → symbol_table
  ├── get_param_memory()           ├── m_start_PC
  └── threads_per_cta()            └── get_instruction(pc)

  ptx_thread_info (각 스레드를 표현)
  ├── m_uid           (스레드 고유 ID)
  ├── m_tid           (dim3: 블록 내 스레드 좌표)
  ├── m_ctaid         (dim3: 그리드 내 블록 좌표)
  ├── m_PC, m_NPC     (현재/다음 프로그램 카운터)
  ├── m_regs          (레지스터 파일 - hash_map<symbol*, ptx_reg_t>)
  ├── m_callstack     (함수 호출 스택)
  ├── m_shared_mem    → memory_space* (공유 메모리)
  ├── m_local_mem     → memory_space* (로컬 메모리)
  ├── m_func_info     → function_info*
  └── m_kernel        → kernel_info_t&

  ptx_cta_info (CTA/Block 상태 관리)
  ├── m_threads_in_cta       (CTA 내 스레드 집합)
  ├── m_threads_that_have_exited
  └── m_bar_threads          (배리어 도달 스레드 수)

  ptx_warp_info (Warp 상태 관리)
  └── m_done_threads         (완료된 스레드 수)

  functionalCoreSim : core_t  (기능적 시뮬레이션 코어)
  ├── m_warpAtBarrier[]      (각 워프의 배리어 상태)
  ├── m_liveThreadCount[]    (각 워프의 살아있는 스레드 수)
  ├── execute()              (모든 워프 실행)
  ├── executeWarp()          (개별 워프 실행)
  └── initializeCTA()        (CTA 초기화)
```

### 1.3 레지스터 표현: ptx_reg_t

`ptx_reg_t`는 PTX 레지스터의 값을 저장하는 공용체(union)이다. 하나의 레지스터가 다양한 타입으로 해석될 수 있도록 설계되었다.

```c++
union ptx_reg_t {
    signed char s8;      signed short s16;
    signed int s32;      signed long long s64;
    unsigned char u8;    unsigned short u16;
    unsigned int u32;    unsigned long long u64;
    half f16;            float f32;
    double f64;
    struct { unsigned ls; unsigned ms; } bits;   // 64비트를 32비트 2개로
    struct { unsigned lowest, low, high, highest; } u128;  // 128비트
    unsigned pred : 4;   // predicate 레지스터 (4비트)
};
```

이 union 덕분에 `data.u32 = a.u32 + b.u32` 같이 타입별로 직접 연산을 수행할 수 있다.

---

## 2. PTX 명령어 처리 파이프라인

### 2.1 전체 파이프라인 흐름

```
  ┌─────────────────┐
  │  CUDA 소스코드   │
  │  (.cu)          │
  └────────┬────────┘
           │ nvcc 컴파일
           v
  ┌─────────────────┐
  │  PTX 텍스트      │   .ptx 파일 또는 바이너리에 임베딩된 PTX
  │  (.ptx)         │
  └────────┬────────┘
           │ (1) PTX 파싱
           v
  ┌─────────────────┐   ptx_lex_init() → ptx__scan_string() → ptx_parse()
  │  PTX 파서        │   flex/bison 기반 파서 (ptx.l / ptx.y)
  │  ptx_recognizer  │   add_instruction(), add_opcode(), add_operand() 등 호출
  └────────┬────────┘
           │ (2) IR 변환
           v
  ┌─────────────────┐   ptx_instruction 객체 리스트 생성
  │  PTX IR          │   symbol_table에 심볼(레지스터, 변수) 등록
  │  ptx_instruction │   function_info에 명령어 리스트 저장
  │  + symbol_table  │
  └────────┬────────┘
           │ (3) 어셈블리 (ptx_assemble)
           v
  ┌─────────────────┐   PC(프로그램 카운터) 할당
  │  명령어 메모리    │   레이블 → 주소 변환
  │  m_instr_mem[]   │   분기 대상 주소 해석
  │  + g_pc_to_finfo │
  └────────┬────────┘
           │ (4) Pre-decode (pre_decode)
           v
  ┌─────────────────┐   opcode/latency 설정
  │  디코딩된 명령어  │   레지스터 입출력 정보 설정
  │  (inst_t 필드)   │   메모리 연산 정보, 캐시 정책 설정
  └────────┬────────┘   reconvergence PC 계산
           │ (5) 실행
           v
  ┌─────────────────┐   ptx_exec_inst() → 각 opcode별 _impl 함수
  │  기능적 실행      │   get_operand_value() / set_operand_value()
  │  (instructions.cc)│  메모리 접근, 분기 등 수행
  └─────────────────┘
```

### 2.2 PTX 파싱 단계 상세 (ptx_loader.cc)

PTX 로딩은 `gpgpu_ptx_sim_load_ptx_from_string()` 함수에서 시작된다.

```
gpgpu_ptx_sim_load_ptx_from_string(const char *p, unsigned source_num)
  │
  ├── init_parser(buf)           → symbol_table 초기화
  ├── ptx_lex_init(&scanner)     → lexer 초기화
  ├── ptx__scan_string(p, scanner) → PTX 문자열을 lexer에 공급
  ├── ptx_parse(scanner, ptx_parser) → bison 파서 실행
  │     │
  │     ├── PTX 텍스트를 토큰화 (flex)
  │     ├── 문법 규칙에 따라 파싱 (bison)
  │     └── 파서 콜백 호출:
  │           ├── add_function_name()  → 함수 이름 등록
  │           ├── start_function()     → function_info 생성
  │           ├── add_variables()      → 변수/레지스터를 symbol_table에 등록
  │           ├── add_opcode()         → opcode 설정
  │           ├── add_scalar_operand() → 피연산자 추가
  │           ├── add_instruction()    → ptx_instruction 생성, 리스트에 추가
  │           └── end_function()       → 함수 완료
  │
  ├── ptx_lex_destroy(scanner)   → lexer 정리
  └── return symtab              → 심볼 테이블 반환
```

`ptx_recognizer` 클래스 (ptx_parser.h)는 파서의 상태를 보유한다. bison이 생성한 파서가 문법 규칙을 인식할 때마다 `ptx_recognizer`의 멤버 함수를 호출하여 IR을 구축한다.

### 2.3 어셈블리 단계 (function_info::ptx_assemble)

`cuda-sim.cc`의 `function_info::ptx_assemble()` (라인 273)에서 수행된다.

```
function_info::ptx_assemble()
  │
  ├── m_instr_mem = new ptx_instruction*[MAX_INST_SIZE * (num_inst + 1)]
  │       → 명령어 메모리 배열 할당
  │
  ├── PC = g_assemble_code_next_pc  (전역적으로 유니크한 PC 시작)
  │   PC += PC % MAX_INST_SIZE      (8바이트 정렬)
  │   m_start_PC = PC
  │
  ├── 명령어 리스트 순회:
  │   for (각 ptx_instruction):
  │     ├── 레이블이면: labels[name] = offset
  │     └── 아니면:
  │         ├── g_pc_to_finfo[PC] = this   (PC → function_info 매핑)
  │         ├── m_instr_mem[n] = pI         (명령어 메모리에 저장)
  │         ├── s_g_pc_to_insn[PC] = pI     (전역 PC→명령어 매핑)
  │         ├── pI->set_PC(PC)              (명령어에 PC 기록)
  │         └── n += inst_size; PC += inst_size
  │
  └── 분기 명령어 처리:
      for (각 BRA/BREAKADDR/CALLP):
        ├── target.name() → labels에서 인덱스 검색
        ├── PC = m_instr_mem[index]->get_PC()
        └── set_label_address(target, PC)  (심볼에 주소 기록)
```

### 2.4 Pre-decode 단계 (ptx_instruction::pre_decode)

`cuda-sim.cc`의 `pre_decode()` (라인 1071)에서 수행되며, 성능 모델이 사용하는 `inst_t` 구조체의 필드를 채운다.

```
ptx_instruction::pre_decode()
  │
  ├── pc, isize 설정
  ├── in[], out[] 배열 초기화 (레지스터 번호)
  ├── memory_op, data_size 설정 (LD/ST인 경우)
  │
  ├── opcodes.def 기반으로 has_dst 판별
  │
  ├── 캐시 정책 설정:
  │   CA_OPTION → CACHE_ALL      (L1+L2)
  │   CG_OPTION → CACHE_GLOBAL   (L2만)
  │   CS_OPTION → CACHE_STREAMING
  │   CV_OPTION → CACHE_VOLATILE  등
  │
  ├── set_opcode_and_latency() 호출:
  │   └── opcode 타입에 따라 op, latency, initiation_interval 설정
  │       LD_OP → LOAD_OP
  │       ST_OP → STORE_OP
  │       BRA_OP → BRANCH_OP
  │       ADD_OP → latency=int_latency[0], op=INTP_OP (또는 SP_OP/DP_OP)
  │       MUL_OP → latency=int_latency[2]
  │       DIV_OP → SFU_OP, latency=int_latency[4]
  │       ATOM_OP → LOAD_OP
  │       BAR_OP → BARRIER_OP
  │
  ├── 입출력 레지스터 추출 (dst→out[], src→in[], arch_reg)
  ├── predicate 레지스터 추출
  ├── 메모리 피연산자 내 주소 레지스터 추출 (ar1, ar2)
  └── reconvergence_pc = get_converge_point(pc)
```

---

## 3. PTX 명령어 구현 분석

### 3.1 opcodes.def: 명령어 등록 매크로

모든 PTX 명령어는 `opcodes.def`에서 매크로로 정의된다. 이 파일은 X-매크로 패턴을 사용하여 다양한 곳에서 include된다.

```c++
// 형식: OP_DEF(opcode_enum, impl_function, string_name, has_dst, classification)
OP_DEF(ADD_OP,   add_impl,  "add",  1, 1)   // 산술: ALU=1
OP_DEF(MUL_OP,   mul_impl,  "mul",  1, 1)
OP_DEF(MAD_OP,   mad_impl,  "mad",  1, 2)   // MAD=2
OP_DEF(LD_OP,    ld_impl,   "ld",   1, 5)   // 메모리=5
OP_DEF(ST_OP,    st_impl,   "st",   0, 5)   // dst 없음
OP_DEF(BRA_OP,   bra_impl,  "bra",  0, 3)   // 제어흐름=3
OP_DEF(ATOM_OP,  atom_impl, "atom", 1, 3)
OP_DEF(BAR_OP,   bar_impl,  "bar",  1, 3)
OP_DEF(EXIT_OP,  exit_impl, "exit", 1, 3)
OP_DEF(TEX_OP,   tex_impl,  "tex",  1, 6)   // 텍스처=6
OP_DEF(NOP_OP,   nop_impl,  "nop",  0, 7)   // NOP=7

// Warp 동기 명령어: core_t와 warp_inst_t를 받는 별도 형식
OP_W_DEF(MMA_OP,    mma_impl,    "mma",       1, 1)   // Tensor Core
OP_W_DEF(MMA_LD_OP, mma_ld_impl, "mma_load",  1, 5)
OP_W_DEF(MMA_ST_OP, mma_st_impl, "mma_store", 0, 5)
OP_W_DEF(SHFL_OP,   shfl_impl,   "shfl",      1, 10)
```

분류(classification): 1=ALU, 2=MAD, 3=Control, 4=SFU, 5=Memory, 6=Texture, 7=NOP, 10=Other, 11=Video

이 매크로는 두 가지 방식으로 사용된다:

1. **실행 디스패치** (`ptx_exec_inst()` 내 switch-case):
```c++
switch (inst_opcode) {
#define OP_DEF(OP, FUNC, STR, DST, CLASSIFICATION) \
  case OP: FUNC(pI, this); op_classification = CLASSIFICATION; break;
#define OP_W_DEF(OP, FUNC, STR, DST, CLASSIFICATION) \
  case OP: FUNC(pI, get_core(), inst); op_classification = CLASSIFICATION; break;
#include "opcodes.def"
}
```

2. **opcode 문자열 테이블** (`instructions.cc` 상단):
```c++
const char *g_opcode_string[NUM_OPCODES] = {
#define OP_DEF(OP, FUNC, STR, DST, CLASSIFICATION) STR,
#include "opcodes.def"
};
```

### 3.2 산술 명령어: ADD

```c++
// instructions.cc:990
void add_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    ptx_reg_t src1_data, src2_data, data;
    int overflow = 0, carry = 0;

    // 1단계: 피연산자 정보 추출
    const operand_info &dst  = pI->dst();
    const operand_info &src1 = pI->src1();
    const operand_info &src2 = pI->src2();

    // 2단계: 타입 정보와 피연산자 값 읽기
    unsigned i_type = pI->get_type();
    src1_data = thread->get_operand_value(src1, dst, i_type, thread, 1);
    src2_data = thread->get_operand_value(src2, dst, i_type, thread, 1);

    // 3단계: 라운딩 모드 설정 (RN/RZ)
    unsigned rounding_mode = pI->rounding_mode();
    int orig_rm = fegetround();
    switch (rounding_mode) { /* RN, RZ 처리 */ }

    // 4단계: 타입별 덧셈 수행
    switch (i_type) {
        case S8_TYPE:   // 부호 있는 8비트
            data.s64 = (src1_data.s64 & 0xFF) + (src2_data.s64 & 0xFF);
            // 오버플로우/캐리 검사
            break;
        case S32_TYPE:  // 부호 있는 32비트
            data.s64 = (src1_data.s64 & 0xFFFFFFFF) + (src2_data.s64 & 0xFFFFFFFF);
            break;
        case U32_TYPE:  // 부호 없는 32비트
            data.u64 = (src1_data.u64 & 0xFFFFFFFF) + (src2_data.u64 & 0xFFFFFFFF);
            carry = (data.u64 & 0x100000000) >> 32;
            break;
        case F32_TYPE:  // 단정밀도 부동소수점
            data.f32 = src1_data.f32 + src2_data.f32;
            break;
        case F64_TYPE:  // 배정밀도 부동소수점
            data.f64 = src1_data.f64 + src2_data.f64;
            break;
    }

    // 5단계: 결과를 대상 레지스터에 기록
    fesetround(orig_rm);
    thread->set_operand_value(dst, data, i_type, thread, pI, overflow, carry);
}
```

핵심 포인트:
- 모든 산술 명령어는 동일한 패턴을 따른다: 피연산자 읽기 → 타입별 연산 → 결과 기록
- `get_operand_value()`는 레지스터, 즉시값, 주소 등 다양한 피연산자 타입을 처리한다
- 오버플로우와 캐리 비트는 정수 연산에서만 계산된다
- 호스트 CPU의 `fesetround()`를 이용하여 부동소수점 라운딩 모드를 시뮬레이션한다

### 3.3 곱셈 명령어: MUL

```c++
// instructions.cc:4290
void mul_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    unsigned i_type = pI->get_type();
    ptx_reg_t a = thread->get_operand_value(src1, dst, i_type, thread, 1);
    ptx_reg_t b = thread->get_operand_value(src2, dst, i_type, thread, 1);

    switch (i_type) {
        case S32_TYPE:
            t.s64 = ((long long)a.s32) * ((long long)b.s32);
            if (pI->is_wide())      d.s64 = t.s64;       // mul.wide: 전체 64비트 결과
            else if (pI->is_hi())   d.s32 = (t.s64>>32);  // mul.hi: 상위 32비트
            else if (pI->is_lo())   d.s32 = t.s32;        // mul.lo: 하위 32비트
            break;
        case F32_TYPE:
            d.f32 = a.f32 * b.f32;
            if (pI->saturation_mode()) { /* 0~1 클램핑 */ }
            break;
    }
    thread->set_operand_value(dst, d, i_type, thread, pI);
}
```

PTX의 `mul.wide`, `mul.hi`, `mul.lo` 변형을 모두 지원한다. wide는 2배 폭 결과를 생성하고, hi/lo는 각각 상위/하위 절반만 취한다.

### 3.4 메모리 명령어: LD (Load)

```c++
// instructions.cc:3373
void ld_exec(const ptx_instruction *pI, ptx_thread_info *thread) {
    const operand_info &dst  = pI->dst();
    const operand_info &src1 = pI->src1();
    unsigned type = pI->get_type();

    // 1단계: 주소 레지스터에서 메모리 주소 추출
    ptx_reg_t src1_data = thread->get_operand_value(src1, dst, type, thread, 1);
    addr_t addr = src1_data.u32;

    // 2단계: 메모리 공간 결정 및 주소 변환
    memory_space_t space = pI->get_space();
    memory_space *mem = NULL;
    decode_space(space, thread, src1, mem, addr);
    //   → global_space: thread->get_global_memory()
    //   → shared_space: thread->m_shared_mem
    //   → local_space:  thread->m_local_mem + stack_pointer offset
    //   → generic_space: whichspace(addr)로 동적 판별

    // 3단계: 데이터 크기 결정
    size_t size;
    int t;
    type_info_key::type_decode(type, size, t);

    // 4단계: 메모리 읽기
    if (!vector_spec) {
        // 스칼라 로드
        mem->read(addr, size/8, &data.s64);
        if (type == S16_TYPE || type == S32_TYPE)
            sign_extend(data, size, dst);    // 부호 확장
        thread->set_operand_value(dst, data, type, thread, pI);
    } else {
        // 벡터 로드 (v2, v3, v4)
        mem->read(addr,            size/8, &data1.s64);
        mem->read(addr + size/8,   size/8, &data2.s64);
        // ... V3, V4에 대해 추가 읽기
        thread->set_vector_operand_values(dst, data1, data2, data3, data4);
    }

    // 5단계: 성능 모델을 위한 메타데이터 기록
    thread->m_last_effective_address = addr;
    thread->m_last_memory_space = space;
}
```

### 3.5 메모리 명령어: ST (Store)

```c++
// instructions.cc:5791
void st_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    const operand_info &dst  = pI->dst();    // 주소 피연산자
    const operand_info &src1 = pI->src1();   // 데이터 피연산자
    unsigned type = pI->get_type();

    // 주소 계산
    ptx_reg_t addr_reg = thread->get_operand_value(dst, dst, type, thread, 1);
    addr_t addr = addr_reg.u32;
    memory_space_t space = pI->get_space();
    memory_space *mem = NULL;
    decode_space(space, thread, dst, mem, addr);

    size_t size; int t;
    type_info_key::type_decode(type, size, t);

    if (!vector_spec) {
        data = thread->get_operand_value(src1, dst, type, thread, 1);
        mem->write(addr, size/8, &data.s64, thread, pI);
    } else {
        // V2, V3, V4 벡터 스토어
        ptx_reg_t *ptx_regs = new ptx_reg_t[4];
        thread->get_vector_operand_values(src1, ptx_regs, 4);
        for (int i = 0; i < 4; i++)
            mem->write(addr + i*size/8, size/8, &ptx_regs[i].s64, thread, pI);
    }
    thread->m_last_effective_address = addr;
    thread->m_last_memory_space = space;
}
```

### 3.6 원자적 메모리 명령어: ATOM

```c++
// instructions.cc:1470
void atom_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    // atom.space.operation.type d, a, b[, c]
    memory_space_t space = pI->get_space();

    // 유효 주소 계산
    const operand_info &src1 = pI->src1();
    unsigned i_type = pI->get_type();
    ptx_reg_t src1_data = thread->get_operand_value(src1, src1, i_type, thread, 1);
    addr_t effective_address = src1_data.u64;

    // generic space → global/shared 변환
    if (space == undefined_space) {
        if (whichspace(effective_address) == global_space) {
            effective_address = generic_to_global(effective_address);
            space = global_space;
        } else if (whichspace(effective_address) == shared_space) {
            unsigned smid = thread->get_hw_sid();
            effective_address = generic_to_shared(smid, effective_address);
            space = shared_space;
        }
    }

    // 메타데이터만 기록 (실제 연산은 콜백으로 지연 실행)
    thread->m_last_effective_address = effective_address;
    thread->m_last_memory_space = space;
    thread->m_last_dram_callback.function = atom_callback;
    thread->m_last_dram_callback.instruction = pI;
}
```

원자적 연산의 핵심 특징:
- `atom_impl()`은 주소 계산과 콜백 등록만 수행한다
- 실제 read-modify-write 연산은 `atom_callback()`에서 수행된다
- 성능 모델에서 메모리 접근이 완료된 후 콜백이 호출되어 타이밍을 정확히 시뮬레이션한다
- `inst.add_callback(lane_id, atom_callback, pI, thread, true)` 형태로 등록된다

### 3.7 제어흐름 명령어: BRA (Branch)

```c++
// instructions.cc:1765
void bra_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    const operand_info &target = pI->dst();
    ptx_reg_t target_pc = thread->get_operand_value(target, target, U32_TYPE, thread, 1);

    thread->m_branch_taken = true;   // 분기 발생 플래그
    thread->set_npc(target_pc);      // 다음 PC를 타겟 주소로 설정
}
```

분기 명령어 자체는 단순히 NPC를 변경한다. 분기 분산(divergence)과 수렴(convergence)은 SIMT 스택에서 처리된다 (후술).

### 3.8 EXIT 명령어

```c++
// instructions.cc:3247
void exit_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    thread->set_done();       // m_thread_done = true, m_cycle_done 기록
    thread->exitCore();       // core_t::warp_exit() 호출 (성능 모델)
    thread->registerExit();   // ptx_cta_info에 종료 등록
}
```

### 3.9 BAR (배리어) 명령어

```c++
// instructions.cc:1514
void bar_impl(const ptx_instruction *pIin, ptx_thread_info *thread) {
    unsigned bar_op = pI->barrier_op();
    unsigned ctaid = thread->get_cta_uid();

    switch (bar_op) {
        case SYNC_OPTION:    // bar.sync: 모든 스레드 대기
            pI->set_bar_id(op0_data.u32);
            break;
        case ARRIVE_OPTION:  // bar.arrive: 도달 알림만
            pI->set_bar_id(op0_data.u32);
            pI->set_bar_count(op1_data.u32);
            break;
        case RED_OPTION:     // bar.red: 리덕션 수행
            switch (red_op) {
                case ATOMIC_POPC: thread->popc_reduction(...); break;
                case ATOMIC_AND:  thread->and_reduction(...);  break;
                case ATOMIC_OR:   thread->or_reduction(...);   break;
            }
            break;
    }
    thread->m_last_dram_callback.function = bar_callback;
    thread->m_last_dram_callback.instruction = pIin;
}
```

---

## 4. 메모리 공간 시뮬레이션

### 4.1 메모리 공간 아키텍처

```
  ┌────────────────────────────────────────────────────────────────┐
  │                    Generic Address Space (PTX 2.0+)            │
  │                                                                │
  │  주소 범위에 따라 동적으로 메모리 공간 결정:                      │
  │  ┌──────────────────────────────────────────────────────┐      │
  │  │ addr >= GLOBAL_HEAP_START  →  Global Memory          │      │
  │  │ addr < STATIC_ALLOC_LIMIT  →  Global Memory          │      │
  │  │ addr >= SHARED_GENERIC_START → Shared Memory         │      │
  │  │ 그 외                       → Local Memory           │      │
  │  └──────────────────────────────────────────────────────┘      │
  └────────────────────────────────────────────────────────────────┘

  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
  │ Global Mem   │  │ Shared Mem   │  │ Local Mem    │
  │ (m_global_mem│  │ (m_shared_mem│  │ (m_local_mem │
  │  gpgpu_t)    │  │  per CTA)    │  │  per Thread) │
  │              │  │              │  │              │
  │ 모든 스레드  │  │ 같은 CTA 내  │  │ 스레드 전용  │
  │ 공유         │  │ 스레드만     │  │ (스택 포함)  │
  │              │  │ 공유         │  │              │
  │ 16*1024 블록 │  │ 16*1024 블록 │  │ 32B 블록     │
  └──────────────┘  └──────────────┘  └──────────────┘

  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
  │ Const Mem    │  │ Texture Mem  │  │ Param Mem    │
  │ (const_space)│  │ (tex_space)  │  │ (param_space)│
  │              │  │              │  │              │
  │ global_mem에 │  │ tex_memory에 │  │ 커널 인자를  │
  │ 매핑됨       │  │ 별도 저장    │  │ 저장         │
  └──────────────┘  └──────────────┘  └──────────────┘
```

### 4.2 주소 공간 변환 함수

```c++
// Generic → Specific 변환
addr_t generic_to_shared(unsigned smid, addr_t addr) {
    return addr - (SHARED_GENERIC_START + smid * SHARED_MEM_SIZE_MAX);
}
addr_t generic_to_local(unsigned smid, unsigned hwtid, addr_t addr) {
    return addr - (LOCAL_GENERIC_START + TOTAL_LOCAL_MEM_PER_SM*smid
                   + LOCAL_MEM_SIZE_MAX*hwtid);
}
addr_t generic_to_global(addr_t addr) { return addr; }

// Specific → Generic 변환 (역방향)
addr_t shared_to_generic(unsigned smid, addr_t addr) {
    return SHARED_GENERIC_START + smid * SHARED_MEM_SIZE_MAX + addr;
}
addr_t local_to_generic(unsigned smid, unsigned hwtid, addr_t addr) {
    return LOCAL_GENERIC_START + TOTAL_LOCAL_MEM_PER_SM*smid
           + LOCAL_MEM_SIZE_MAX*hwtid + addr;
}

// 주소가 어떤 공간에 속하는지 판별
memory_space_t whichspace(addr_t addr) {
    if (addr >= GLOBAL_HEAP_START || addr < STATIC_ALLOC_LIMIT) return global_space;
    else if (addr >= SHARED_GENERIC_START) return shared_space;
    else return local_space;
}
```

### 4.3 decode_space(): 메모리 공간 디코딩

LD/ST 명령어의 핵심 헬퍼 함수이다. 명령어의 메모리 공간 지정자에 따라 실제 `memory_space*` 포인터와 변환된 주소를 반환한다.

```c++
void decode_space(memory_space_t &space, ptx_thread_info *thread,
                  const operand_info &op, memory_space *&mem, addr_t &addr) {
    switch (space.get_type()) {
        case global_space:
            mem = thread->get_global_memory();  // gpgpu_t::m_global_mem
            break;
        case local_space:
        case param_space_local:
            mem = thread->m_local_mem;
            addr += thread->get_local_mem_stack_pointer();  // 스택 오프셋 추가
            break;
        case shared_space:
            mem = thread->m_shared_mem;         // CTA별 공유 메모리
            break;
        case param_space_kernel:
            mem = thread->get_param_memory();   // kernel_info_t::param_memory
            break;
        case const_space:
            mem = thread->get_global_memory();  // 글로벌 메모리에 매핑
            break;
        case tex_space:
            mem = thread->get_tex_memory();
            break;
        case generic_space:       // PTX 2.0+ 제네릭 주소
            space = whichspace(addr);    // 주소 범위로 판별
            switch (space.get_type()) {
                case global_space:
                    mem = thread->get_global_memory();
                    addr = generic_to_global(addr);
                    break;
                case local_space:
                    mem = thread->m_local_mem;
                    addr = generic_to_local(smid, hwtid, addr);
                    break;
                case shared_space:
                    mem = thread->m_shared_mem;
                    addr = generic_to_shared(smid, addr);
                    break;
            }
            break;
    }
}
```

### 4.4 memory_space_impl: 메모리 저장소 구현

```c++
template <unsigned BSIZE>   // BSIZE = 블록 크기 (32, 64, 8192, 16*1024)
class memory_space_impl : public memory_space {
    typedef mem_map<mem_addr_t, mem_storage<BSIZE>> map_t;
    map_t m_data;                    // 해시맵 기반 희소 메모리
    unsigned m_log2_block_size;      // 블록 크기의 log2

    void write(mem_addr_t addr, size_t length, const void *data, ...) {
        mem_addr_t index = addr >> m_log2_block_size;   // 페이지 인덱스
        if ((addr + length) <= (index+1) * BSIZE) {
            // Fast path: 블록 내 접근
            unsigned offset = addr & (BSIZE - 1);
            m_data[index].write(offset, length, data);
        } else {
            // Slow path: 블록 경계 걸침 → 분할 접근
            while (남은 바이트 > 0) { /* 페이지 단위로 나누어 기록 */ }
        }
        // 워치포인트 검사
        for (watchpoint : m_watchpoints) { /* 디버깅 지원 */ }
    }
};
```

GPGPU-Sim에서 사용하는 메모리 블록 크기:
- `memory_space_impl<32>` : Local 메모리 (스레드별, 작은 블록)
- `memory_space_impl<64>` : 일반 목적
- `memory_space_impl<8192>` : 중간 크기
- `memory_space_impl<16*1024>` : Global/Shared 메모리 (큰 블록, 16KB)

핵심 설계: 해시맵 기반 **희소 메모리(sparse memory)**를 사용하여 실제 접근된 주소만 메모리를 할당한다. GPU의 수 GB 주소 공간 전체를 시뮬레이션하면서도 실제 사용된 양만큼만 호스트 메모리를 소비한다.

---

## 5. CUDA 커널 실행 흐름

### 5.1 커널 실행 전체 흐름

```
  cudaLaunchKernel() [libcuda/cuda_runtime_api.cc]
      │
      ├── kernel_info_t 객체 생성
      │     ├── gridDim, blockDim 설정
      │     ├── function_info* entry 연결
      │     └── param_memory 할당, 커널 인자 복사
      │
      ├── function_info::ptx_assemble()   [최초 1회만]
      │     └── 명령어에 PC 할당, 레이블 해석
      │
      ├── function_info::finalize(param_mem)
      │     └── 커널 인자를 param 메모리 공간에 복사
      │
      └── stream_manager에 커널 제출
            │
            v
  ┌─────────────────────────────────────────────────┐
  │           gpgpu_sim::cycle()                     │
  │  (성능 시뮬레이션 모드에서 매 사이클 호출)         │
  │                                                   │
  │  shader_core_ctx::issue_block2core()              │
  │    ├── kernel_info_t에서 다음 CTA ID 가져오기     │
  │    └── ptx_sim_init_thread() 호출                 │
  │          → CTA의 모든 스레드 생성/초기화           │
  └─────────────────────────────────────────────────┘
```

### 5.2 ptx_sim_init_thread(): 스레드 초기화 상세

이 함수는 `cuda-sim.cc:2056`에 정의되어 있으며, 커널의 CTA를 SM에 할당하고 스레드를 초기화하는 핵심 함수이다.

```
ptx_sim_init_thread(kernel, &thread_info, sid, tid, threads_left, ...)
  │
  ├── 기존 스레드가 완료된 경우: delete & NULL 설정
  │
  ├── active_threads가 비어있지 않으면:
  │     └── 기존 CTA의 다음 스레드 반환
  │
  ├── 새 CTA 시작이 필요한 경우:
  │     ├── shared_mem = new memory_space_impl<16*1024>("shared_N", 4)
  │     ├── cta_info = new ptx_cta_info(sm_idx, ctx)
  │     │
  │     └── while (kernel.more_threads_in_cta()):
  │           ├── thd = new ptx_thread_info(kernel)
  │           ├── warp_info 할당 (워프별 공유)
  │           ├── local_mem = new memory_space_impl<32>("local_N_M", 32)
  │           │
  │           ├── thd->set_info(kernel.entry())  → m_PC = start_PC
  │           ├── thd->set_nctaid(gridDim)
  │           ├── thd->set_ntid(blockDim)
  │           ├── thd->set_ctaid(ctaid3d)
  │           ├── thd->set_tid(tid3d)
  │           │
  │           ├── thd->m_shared_mem = shared_mem  (CTA 내 모든 스레드 공유)
  │           ├── thd->m_cta_info = cta_info
  │           ├── thd->m_local_mem = local_mem    (스레드별 고유)
  │           │
  │           └── active_threads.push_back(thd)
  │
  └── kernel.increment_cta_id()
```

핵심 메모리 할당 규칙:
- **Shared Memory**: CTA당 1개, CTA 내 모든 스레드가 `m_shared_mem` 포인터를 공유
- **Local Memory**: 스레드당 1개, 각 스레드가 고유한 `m_local_mem`을 가짐
- **Global Memory**: GPU 전체에서 1개, `gpgpu_t::m_global_mem`으로 접근

### 5.3 기능적 시뮬레이션 모드 (functionalCoreSim)

GPGPU-Sim은 두 가지 시뮬레이션 모드를 제공한다:
1. **성능 모델**: `shader_core_ctx`가 파이프라인을 사이클 단위로 시뮬레이션
2. **기능적 시뮬레이션**: `functionalCoreSim`이 빠르게 실행 (타이밍 없이)

```
functionalCoreSim::execute(inst_count, ctaid_cp)
  │
  ├── initializeCTA(ctaid_cp)
  │     ├── ptx_sim_init_thread()로 모든 스레드 초기화
  │     └── createWarp(): SIMT 스택 초기화, 초기 마스크 설정
  │
  └── while (true):
        ├── for 각 warp i:
        │     executeWarp(i, allAtBarrier, someOneLive)
        │       │
        │       ├── warp_inst_t inst = getExecuteWarp(i)
        │       │     └── SIMT 스택에서 active mask & PC 가져오기
        │       │         활성 스레드에 대해 ptx_fetch_inst() 호출
        │       │
        │       ├── execute_warp_inst_t(inst, i)
        │       │     └── 워프 내 각 활성 스레드에 대해
        │       │         thread->ptx_exec_inst(inst, lane_id) 호출
        │       │
        │       ├── inst.isatomic() → inst.do_atomic(true)
        │       ├── BARRIER_OP → m_warpAtBarrier[i] = true
        │       └── updateSIMTStack(i, &inst)
        │             └── 분기 분산/수렴 처리
        │
        ├── !someOneLive → break (모든 스레드 완료)
        └── allAtBarrier → 모든 배리어 해제
```

---

## 6. Warp 실행 모델

### 6.1 SIMT 스택과 분기 분산

GPGPU-Sim은 PDOM(Post-Dominator) 기반 SIMT 스택을 사용하여 워프 내 분기 분산과 수렴을 관리한다.

```
  SIMT 스택 구조 (각 워프마다 1개):

  ┌─────────────────────────────────────────┐
  │  Entry N: { PC, RPC(재수렴PC), ActiveMask }  ← Top
  │  Entry N-1: { PC, RPC, ActiveMask }
  │  ...
  │  Entry 0: { start_PC, -1, 0xFFFFFFFF }  ← Bottom
  └─────────────────────────────────────────┘

  분기 발생 시:
  ┌──────────────────────────────────────────────────┐
  │  if (condition) {     // 일부 스레드 taken        │
  │      A;               // taken 경로               │
  │  } else {                                        │
  │      B;               // not-taken 경로           │
  │  }                                               │
  │  C;                   // 수렴 지점 (post-dominator)│
  └──────────────────────────────────────────────────┘

  SIMT 스택 변화:

  분기 전:                     분기 후:
  ┌────────────────┐           ┌────────────────────────┐
  │ PC=if, RPC=-1  │           │ PC=B, RPC=C, mask=0110 │ ← not-taken
  │ mask=1111      │           │ PC=A, RPC=C, mask=1001 │ ← taken
  └────────────────┘           │ PC=if, RPC=-1,mask=1111│
                               └────────────────────────┘

  A 실행 후:                   수렴:
  ┌────────────────────────┐   ┌────────────────┐
  │ PC=B, RPC=C, mask=0110 │   │ PC=C, RPC=-1   │
  │ PC=if, RPC=-1,mask=1111│   │ mask=1111       │
  └────────────────────────┘   └────────────────┘
```

### 6.2 활성 마스크 처리

`cuda-sim.h`에서 정의된 상수들:

```c++
#define RECONVERGE_RETURN_PC ((address_type)-2)
#define NO_BRANCH_DIVERGENCE ((address_type)-1)
```

`ptx_exec_inst()`에서의 활성 마스크 처리:

```c++
void ptx_thread_info::ptx_exec_inst(warp_inst_t &inst, unsigned lane_id) {
    // 1. predicate 평가
    if (pI->has_pred()) {
        ptx_reg_t pred_value = get_operand_value(pred, ...);
        if (pI->get_pred_mod() == -1) {
            skip = (pred_value.pred & 0x0001) ^ pI->get_pred_neg();
        } else {
            skip = !pred_lookup(pI->get_pred_mod(), pred_value.pred);
        }
    }

    if (skip) {
        inst.set_not_active(lane_id);  // 이 레인을 비활성으로 표시
    } else {
        // 명령어 실행
        switch (inst_opcode) {
            // opcodes.def에서 확장된 case문들
        }
    }
}
```

### 6.3 분기 수렴 메커니즘

`createWarp()` (cuda-sim.cc:2631)에서 SIMT 스택이 초기화된다:

```c++
void functionalCoreSim::createWarp(unsigned warpId) {
    simt_mask_t initialMask;
    initialMask.set();  // 모든 비트 1로 설정

    // 스레드가 없는 레인은 마스크에서 제거
    for (int i = warpId*m_warp_size; i < (warpId+1)*m_warp_size; i++) {
        if (m_thread[i] == NULL)
            initialMask.reset(i - warpId*m_warp_size);
    }

    // SIMT 스택에 초기 엔트리 푸시 (시작 PC, 초기 마스크)
    m_simt_stack[warpId]->launch(
        m_thread[warpId * m_warp_size]->get_pc(),
        initialMask
    );
}
```

`executeWarp()` (cuda-sim.cc:2731)에서 매 사이클 SIMT 스택이 업데이트된다:

```c++
void functionalCoreSim::executeWarp(unsigned i, bool &allAtBarrier, bool &someOneLive) {
    if (!m_warpAtBarrier[i] && m_liveThreadCount[i] != 0) {
        warp_inst_t inst = getExecuteWarp(i);
        //   → SIMT 스택 top에서 PC, active mask를 가져옴
        //   → 활성 스레드에 대해 ptx_fetch_inst() 호출
        //   → inst.active_mask 설정

        execute_warp_inst_t(inst, i);
        //   → 각 활성 레인에 대해 ptx_exec_inst(inst, lane_id) 호출
        //   → 레인별로 NPC가 달라질 수 있음 (분기 분산)

        if (inst.isatomic())
            inst.do_atomic(true);

        if (inst.op == BARRIER_OP || inst.op == MEMORY_BARRIER_OP)
            m_warpAtBarrier[i] = true;

        updateSIMTStack(i, &inst);
        //   → 각 스레드의 NPC를 검사
        //   → 분기 분산이면: SIMT 스택에 분기 경로 푸시
        //   → 수렴 지점 도달이면: 스택 팝
    }
}
```

---

## 7. 핵심 함수 라인바이라인 분석

### 7.1 ptx_thread_info::ptx_exec_inst() (cuda-sim.cc:1787~2041)

이 함수는 단일 스레드가 하나의 PTX 명령어를 실행하는 핵심 함수이다.

```c++
void ptx_thread_info::ptx_exec_inst(warp_inst_t &inst, unsigned lane_id) {
    bool skip = false;
    int op_classification = 0;

    // ── 1단계: PC 가져오기 ──
    addr_t pc = next_instr();
    //   m_icount++;          (실행 명령어 카운트 증가)
    //   m_branch_taken = false;  (분기 플래그 초기화)
    //   return m_PC;

    assert(pc == inst.pc);
    //   타이밍 모델과 기능 모델의 PC가 일치하는지 검증

    // ── 2단계: 명령어 fetch ──
    const ptx_instruction *pI = m_func_info->get_instruction(pc);
    //   m_instr_mem[] 배열에서 PC 위치의 명령어 포인터 반환

    // ── 3단계: NPC 설정 (기본값: 순차 실행) ──
    set_npc(pc + pI->inst_size());
    //   분기가 아니면 다음 명령어 주소

    try {
        clearRPC();
        //   m_RPC = -1, m_RPC_updated = false, m_last_was_call = false

        m_last_set_operand_value.u64 = 0;

        // ── 4단계: 종료 검사 ──
        if (is_done()) { assert(0); }

        // ── 5단계: 디버그 추적 설정 ──
        if (g_debug_execution >= 6) {
            clear_modifiedregs();
            enable_debug_trace();
        }

        // ── 6단계: Predicate 평가 ──
        if (pI->has_pred()) {
            const operand_info &pred = pI->get_pred();
            ptx_reg_t pred_value = get_operand_value(pred, pred, PRED_TYPE, this, 0);

            if (pI->get_pred_mod() == -1) {
                // PTXPlus 방식: zero flag 반전
                skip = (pred_value.pred & 0x0001) ^ pI->get_pred_neg();
            } else {
                // PTX 방식: pred modifier (eq, ne, lt, gt 등)
                skip = !pred_lookup(pI->get_pred_mod(), pred_value.pred & 0x000F);
            }
        }

        int inst_opcode = pI->get_opcode();

        // ── 7단계: 스킵 또는 실행 ──
        if (skip) {
            inst.set_not_active(lane_id);
            //   이 레인의 결과가 무효임을 표시
        } else {
            // VOTE/ACTIVEMASK는 워프 전체 active mask 필요 → inst 복사
            if (pI->get_opcode() == VOTE_OP || pI->get_opcode() == ACTIVEMASK_OP) {
                pJ = new ptx_instruction(*pI);
                *((warp_inst_t *)pJ) = inst;  // active mask 정보 복사
                pI = pJ;
            }

            // Tensor Core 명령어: 전체 워프 활성 필요
            if (inst_opcode == MMA_OP || inst_opcode == MMA_LD_OP || ...)
                assert(inst.active_count() == MAX_WARP_SIZE);

            // ── 8단계: 명령어 디스패치 (opcodes.def 기반) ──
            // Tensor Core 명령어는 lane_id==0에서만 실행 (워프 동기)
            if (!tensorcore_op(inst_opcode) ||
                (tensorcore_op(inst_opcode) && lane_id == 0)) {
                switch (inst_opcode) {
                    // OP_DEF 매크로가 확장되어 각 opcode별 case 생성:
                    case ADD_OP: add_impl(pI, this); op_classification = 1; break;
                    case LD_OP:  ld_impl(pI, this);  op_classification = 5; break;
                    case BRA_OP: bra_impl(pI, this); op_classification = 3; break;
                    // ... 100개+ opcode
                    // OP_W_DEF: warp 단위 명령어
                    case MMA_OP: mma_impl(pI, get_core(), inst); break;
                    case SHFL_OP: shfl_impl(pI, get_core(), inst); break;
                }
            }

            // EXIT 옵션이 있으면 exit_impl 추가 실행
            if (pI->is_exit()) exit_impl(pI, this);
        }

        // ── 9단계: 성능 모델을 위한 메타데이터 수집 ──
        if (pI->has_memory_read() || pI->has_memory_write()) {
            insn_memaddr = last_eaddr();     // 유효 주소
            insn_space = last_space();       // 메모리 공간
            insn_data_size = datatype2size(pI->get_type());
            insn_memory_op = memory_load 또는 memory_store;
        }

        // ATOM: 콜백 등록 (실제 원자적 연산은 지연 실행)
        if (pI->get_opcode() == ATOM_OP) {
            inst.add_callback(lane_id, last_callback().function,
                              last_callback().instruction, this, true);
        }

        // ── 10단계: PC 업데이트 ──
        update_pc();
        //   m_PC = m_NPC; (NPC가 분기에 의해 변경되었을 수 있음)

        g_ptx_sim_num_insn++;  // 전역 실행 명령어 카운터

        // ── 11단계: 결과를 warp_inst_t에 기록 ──
        if (!skip) {
            inst.space = insn_space;
            inst.set_addr(lane_id, insn_memaddr);
            inst.data_size = insn_data_size;
        }

    } catch (int x) {
        // 실행 에러 처리
        printf("ERROR (%d) executing instruction (%s:%u)\n", ...);
        abort();
    }
}
```

### 7.2 LD 명령어 실행 흐름 (instructions.cc:3373)

```c++
void ld_exec(const ptx_instruction *pI, ptx_thread_info *thread) {
    // ── 피연산자 추출 ──
    const operand_info &dst  = pI->dst();     // 대상 레지스터
    const operand_info &src1 = pI->src1();    // 주소 레지스터/즉시값
    unsigned type = pI->get_type();           // 데이터 타입 (U32, F32 등)

    // ── 주소 계산 ──
    ptx_reg_t src1_data = thread->get_operand_value(src1, dst, type, thread, 1);
    //   get_operand_value()는:
    //   - 레지스터면: m_regs 해시맵에서 값 읽기
    //   - 즉시값이면: 즉시값 반환
    //   - 주소면: 심볼 주소 + 오프셋 계산
    //   - 빌트인이면: %tid.x, %ctaid.x 등 반환

    addr_t addr = src1_data.u32;

    // ── 메모리 공간 결정 ──
    memory_space_t space = pI->get_space();
    //   ld.global → global_space
    //   ld.shared → shared_space
    //   ld.local  → local_space
    //   ld.param  → param_space
    //   ld (generic) → generic_space

    memory_space *mem = NULL;
    decode_space(space, thread, src1, mem, addr);
    //   space와 addr이 필요에 따라 변환됨
    //   mem 포인터가 적절한 memory_space 객체를 가리킴

    // ── 타입 크기 결정 ──
    size_t size;  // 비트 단위
    int t;
    type_info_key::type_decode(type, size, t);
    //   U32_TYPE → size=32
    //   F64_TYPE → size=64
    //   U8_TYPE  → size=8

    // ── 메모리 읽기 실행 ──
    data.u64 = 0;  // 초기화
    if (!vector_spec) {
        mem->read(addr, size/8, &data.s64);
        //   memory_space_impl::read():
        //   index = addr >> m_log2_block_size  (페이지 인덱스)
        //   m_data[index].read(offset, nbytes, ...)
        //   존재하지 않는 페이지는 0 반환

        if (type == S16_TYPE || type == S32_TYPE)
            sign_extend(data, size, dst);    // 부호 확장

        thread->set_operand_value(dst, data, type, thread, pI);
        //   대상 레지스터에 값 기록: m_regs[symbol] = data
    } else {
        // 벡터 로드 (ld.v2, ld.v4)
        mem->read(addr,          size/8, &data1.s64);
        mem->read(addr + size/8, size/8, &data2.s64);
        // ... 추가 요소
        thread->set_vector_operand_values(dst, data1, data2, data3, data4);
    }

    // ── 성능 모델 메타데이터 ──
    thread->m_last_effective_address = addr;
    //   ptx_exec_inst()에서 이 값을 읽어 inst.set_addr() 호출
    thread->m_last_memory_space = space;
    //   성능 모델이 캐시/메모리 시뮬레이션에 사용
}
```

### 7.3 ADD 명령어 실행 흐름 (instructions.cc:990)

```c++
void add_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    ptx_reg_t src1_data, src2_data, data;
    int overflow = 0, carry = 0;

    // ── 피연산자 정보 추출 ──
    const operand_info &dst  = pI->dst();
    const operand_info &src1 = pI->src1();
    const operand_info &src2 = pI->src2();
    //   ptx_instruction은 m_operands 벡터를 보유
    //   dst()  = m_operands[0]
    //   src1() = m_operands[1]
    //   src2() = m_operands[2]

    // ── 타입 정보 ──
    unsigned i_type = pI->get_type();
    //   add.s32 → S32_TYPE
    //   add.f32 → F32_TYPE

    // ── 피연산자 값 읽기 ──
    src1_data = thread->get_operand_value(src1, dst, i_type, thread, 1);
    src2_data = thread->get_operand_value(src2, dst, i_type, thread, 1);
    //   마지막 인자 1 = dereference flag (주소가 아닌 값 읽기)

    // ── 라운딩 모드 ──
    unsigned rounding_mode = pI->rounding_mode();
    int orig_rm = fegetround();
    // RZ_OPTION → fesetround(FE_TOWARDZERO)  (0 방향 절삭)
    // RN_OPTION → 기본 (가장 가까운 짝수)

    // ── 타입별 덧셈 수행 ──
    switch (i_type) {
        case S32_TYPE:
            // 32비트를 64비트로 확장하여 연산 → 오버플로우/캐리 검출
            data.s64 = (src1_data.s64 & 0x0FFFFFFFF) + (src2_data.s64 & 0x0FFFFFFFF);
            // 부호가 같은데 결과 부호가 다르면 overflow
            if (같은_부호 && 결과_부호_다름) overflow = 1;
            carry = (data.u64 & 0x100000000) >> 32;
            break;

        case U32_TYPE:
            data.u64 = (src1_data.u64 & 0xFFFFFFFF) + (src2_data.u64 & 0xFFFFFFFF);
            carry = (data.u64 & 0x100000000) >> 32;
            break;

        case F32_TYPE:
            data.f32 = src1_data.f32 + src2_data.f32;
            //   호스트 CPU의 FPU를 직접 사용하여 연산
            break;

        case F64_TYPE:
            data.f64 = src1_data.f64 + src2_data.f64;
            break;
    }

    // ── 라운딩 복원 & 결과 기록 ──
    fesetround(orig_rm);
    thread->set_operand_value(dst, data, i_type, thread, pI, overflow, carry);
    //   set_operand_value()는:
    //   1. m_regs[dst_symbol] = data
    //   2. m_debug_trace_regs_modified에 기록 (디버그용)
    //   3. overflow/carry 플래그를 CC 레지스터에 기록 (있으면)
}
```

### 7.4 BRA 명령어 실행 흐름 (instructions.cc:1765)

```c++
void bra_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    // ── 분기 대상 주소 가져오기 ──
    const operand_info &target = pI->dst();
    //   PTX에서 bra의 대상은 dst로 인코딩됨
    //   예: bra $L__BB0_2 → target = label "$L__BB0_2"

    ptx_reg_t target_pc = thread->get_operand_value(target, target, U32_TYPE, thread, 1);
    //   레이블의 경우: 어셈블리 단계에서 설정된 PC 값 반환
    //   ptx_assemble()에서: labels["$L__BB0_2"] = index
    //                       m_instr_mem[index]->get_PC() → target_pc

    // ── 분기 실행 ──
    thread->m_branch_taken = true;
    //   ptx_exec_inst()의 next_instr()에서 이미 false로 초기화됨
    //   true로 설정하면 이 스레드가 분기를 탔음을 표시

    thread->set_npc(target_pc);
    //   m_NPC = target_pc.u32
    //   ptx_exec_inst() 끝에서 update_pc() → m_PC = m_NPC

    // ── 분기 분산 처리는 SIMT 스택에서 수행 ──
    //   executeWarp() 이후 updateSIMTStack()에서:
    //   각 스레드의 NPC를 비교
    //   NPC가 다른 스레드 그룹 → SIMT 스택에 분기 경로 푸시
    //   reconvergence PC도 함께 푸시
}
```

### 7.5 MUL 명령어 실행 흐름 (instructions.cc:4290)

```c++
void mul_impl(const ptx_instruction *pI, ptx_thread_info *thread) {
    unsigned i_type = pI->get_type();
    ptx_reg_t a = thread->get_operand_value(src1, dst, i_type, thread, 1);
    ptx_reg_t b = thread->get_operand_value(src2, dst, i_type, thread, 1);

    switch (i_type) {
        case S32_TYPE:
            // 32x32 → 64비트 전체 결과 계산
            t.s64 = ((long long)a.s32) * ((long long)b.s32);

            // PTX mul 변형에 따라 결과 선택:
            if (pI->is_wide())
                d.s64 = t.s64;         // mul.wide.s32: 전체 64비트
            else if (pI->is_hi())
                d.s32 = (t.s64 >> 32); // mul.hi.s32: 상위 32비트
            else if (pI->is_lo())
                d.s32 = t.s32;         // mul.lo.s32: 하위 32비트 (기본)
            break;

        case U32_TYPE:
            t.u64 = ((unsigned long long)a.u32) * ((unsigned long long)b.u32);
            // 동일한 wide/hi/lo 패턴
            break;

        case F32_TYPE:
            d.f32 = a.f32 * b.f32;
            if (pI->saturation_mode()) {
                if (d.f32 < 0) d.f32 = 0;
                else if (d.f32 > 1.0f) d.f32 = 1.0f;
            }
            break;

        case F64_TYPE:
            d.f64 = a.f64 * b.f64;
            break;
    }

    thread->set_operand_value(dst, d, i_type, thread, pI);
}
```

---

## 8. CUDA Device Runtime (CDP) 지원

GPGPU-Sim은 CUDA Dynamic Parallelism (CDP)을 지원하여 GPU 커널 내에서 새로운 커널을 런칭할 수 있다.

### 8.1 핵심 클래스

```c++
class cuda_device_runtime {
    std::map<void*, device_launch_config_t> g_cuda_device_launch_param_map;
    //   파라미터 버퍼 → 런치 설정 매핑
    std::list<device_launch_operation_t> g_cuda_device_launch_op;
    //   런치 대기 중인 자식 커널 큐
    unsigned g_kernel_launch_latency;
    unsigned g_TB_launch_latency;

    void gpgpusim_cuda_getParameterBufferV2(...);
    //   자식 커널의 파라미터 버퍼 할당
    void gpgpusim_cuda_launchDeviceV2(...);
    //   자식 커널 런칭 (kernel_info_t 생성, 스트림에 제출)
    void launch_all_device_kernels();
    //   대기 중인 모든 자식 커널을 실제로 런칭
};
```

### 8.2 CDP 실행 흐름

```
GPU 커널에서 cudaLaunchDevice() 호출
  │
  ├── cudaGetParameterBufferV2() 인터셉트
  │     └── gpgpusim_cuda_getParameterBufferV2():
  │           ├── 자식 커널의 function_info* 추출
  │           ├── grid_dim, block_dim 추출
  │           ├── 파라미터 버퍼를 GPU 글로벌 메모리에 할당
  │           └── device_launch_config_t에 설정 저장
  │
  ├── 파라미터 복사 (커널 코드에서 수행)
  │
  └── cudaLaunchDeviceV2() 인터셉트
        └── gpgpusim_cuda_launchDeviceV2():
              ├── 새 kernel_info_t 생성
              ├── device_launch_operation_t에 등록
              └── launch_one_device_kernel()로 즉시 또는 지연 런칭
```

---

## 9. Opcode 레이턴시 설정

`set_opcode_and_latency()`에서 각 명령어의 파이프라인 레이턴시와 initiation interval을 설정한다.

```
  명령어 유형       기본 레이턴시    Initiation Interval    파이프라인
  ──────────────   ──────────────  ────────────────────   ──────────
  ADD/SUB (int)    1 cycle         1 cycle                INTP_OP
  ADD/SUB (fp32)   1 cycle         1 cycle                SP_OP
  ADD/SUB (fp64)   8 cycles        8 cycles               DP_OP
  MUL (int)        19 cycles       4 cycles               INTP_OP
  MUL (fp32)       1 cycle         1 cycle                SP_OP
  MAD (int)        25 cycles       4 cycles               INTP_OP
  MAD (fp32)       1 cycle         1 cycle                SP_OP
  DIV (int)        145 cycles      32 cycles              SFU_OP
  DIV (fp32)       30 cycles       5 cycles               SFU_OP
  DIV (fp64)       335 cycles      130 cycles             SFU_OP
  SQRT/SIN/COS     8 cycles        8 cycles               SFU_OP
  MMA (tensor)     64 cycles       64 cycles              TENSOR_CORE_OP
  LD/ST            -               -                      LOAD_OP/STORE_OP
  BRA              -               -                      BRANCH_OP
  BAR              -               -                      BARRIER_OP
  SHFL             32 cycles       4 cycles               -
```

이 값들은 설정 파일에서 `-ptx_opcode_latency_int`, `-ptx_opcode_latency_fp` 등의 옵션으로 변경 가능하다.

---

## 10. 요약: 핵심 설계 특징

### 10.1 기능적 시뮬레이션 vs 성능 시뮬레이션 분리

GPGPU-Sim의 CUDA 실행 모델은 **기능(functional)**과 **타이밍(timing)**을 명확히 분리한다.

- **기능적 실행** (`ptx_exec_inst()`, `instructions.cc`): PTX 명령어의 정확한 시맨틱스를 시뮬레이션한다. 레지스터 읽기/쓰기, 메모리 접근, 분기 처리를 수행한다.
- **타이밍 모델** (`shader_core_ctx`): 파이프라인 스테이지, 캐시 계층, 메모리 인터커넥트 등의 지연시간을 사이클 단위로 시뮬레이션한다.

기능적 실행의 결과(유효 주소, 메모리 공간, 분기 방향)는 `warp_inst_t`를 통해 타이밍 모델에 전달된다.

### 10.2 X-매크로 패턴의 활용

`opcodes.def`의 X-매크로 패턴은 코드 중복을 줄이고 새 명령어 추가를 용이하게 한다. 한 번의 정의로 실행 디스패치, 문자열 변환, pre-decode 등 여러 곳에서 자동으로 코드가 생성된다.

### 10.3 호스트 CPU의 FPU 활용

부동소수점 연산 시뮬레이션에서 호스트 CPU의 FPU를 직접 사용한다 (`data.f32 = a.f32 + b.f32`). 이는 시뮬레이션 속도를 높이지만, 호스트와 실제 GPU의 부동소수점 동작이 다를 수 있는 한계가 있다. 라운딩 모드는 `fesetround()`로 일부 제어한다.

### 10.4 SIMT 스택 기반 분기 수렴

PDOM(Post-Dominator) 분석을 기반으로 한 SIMT 스택은 NVIDIA GPU의 실제 하드웨어 동작을 근사한다. 분기 분산 시 SIMT 스택에 두 경로를 푸시하고, 재수렴 지점(post-dominator)에서 마스크를 합쳐 수렴한다.
