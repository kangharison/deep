# GPGPU-Sim Shader Core 파이프라인 심층 분석

> 소스코드: `src/gpgpu-sim/shader.h`, `shader.cc`, `scoreboard.h/cc`, `stack.h/cc`, `abstract_hardware_model.h/cc`

---

## 1. SM(Streaming Multiprocessor) 아키텍처 개요

### 1.1 전체 구조 ASCII 다이어그램

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        simt_core_cluster (클러스터)                               │
│  ┌────────────────────────────────────────────────────────────────────────────┐  │
│  │                    shader_core_ctx (SM 하나)                                │  │
│  │                                                                            │  │
│  │  ┌──────────────┐     ┌──────────────────────────────────────────┐         │  │
│  │  │   L1I Cache   │────▶│         Fetch Buffer (ifetch_buffer_t)   │         │  │
│  │  │  (read_only)  │     │  m_pc, m_warp_id, m_valid               │         │  │
│  │  └──────────────┘     └────────────────┬─────────────────────────┘         │  │
│  │                                         │ Decode                            │  │
│  │                                         ▼                                   │  │
│  │  ┌──────────────────────────────────────────────────────────────────────┐  │  │
│  │  │              I-Buffer (shd_warp_t::m_ibuffer[2])                     │  │  │
│  │  │              각 워프마다 2개 슬롯의 명령어 버퍼                          │  │  │
│  │  └──────────────────────────────┬───────────────────────────────────────┘  │  │
│  │                                  │ Issue (scheduler_unit::cycle)           │  │
│  │  ┌──────────────────────────────┼───────────────────────────────────────┐  │  │
│  │  │  Warp Schedulers             ▼                                       │  │  │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐              │  │  │
│  │  │  │ Sched #0 │ │ Sched #1 │ │ Sched #2 │ │ Sched #3 │  (최대 N개)  │  │  │
│  │  │  │  (GTO/   │ │  (GTO/   │ │  (GTO/   │ │  (GTO/   │              │  │  │
│  │  │  │  LRR/    │ │  LRR/    │ │  LRR/    │ │  LRR/    │              │  │  │
│  │  │  │  2Level) │ │  2Level) │ │  2Level) │ │  2Level) │              │  │  │
│  │  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘              │  │  │
│  │  │       │             │             │             │                    │  │  │
│  │  │       │    Scoreboard (해저드 체크) │             │                    │  │  │
│  │  └───────┼─────────────┼─────────────┼─────────────┼────────────────────┘  │  │
│  │          ▼             ▼             ▼             ▼                        │  │
│  │  ┌──────────────────────────────────────────────────────────────────────┐  │  │
│  │  │         ID_OC 파이프라인 레지스터 (Dispatch to Operand Collector)     │  │  │
│  │  │  ID_OC_SP | ID_OC_DP | ID_OC_INT | ID_OC_SFU | ID_OC_MEM |        │  │  │
│  │  │  ID_OC_TENSOR_CORE | ID_OC_SPEC_*                                   │  │  │
│  │  └──────────────────────────────┬───────────────────────────────────────┘  │  │
│  │                                  │ Read Operands (opndcoll_rfu_t::step)   │  │
│  │  ┌──────────────────────────────┼───────────────────────────────────────┐  │  │
│  │  │    Operand Collector         ▼                                       │  │  │
│  │  │    ┌─────────────────────────────────────────┐                      │  │  │
│  │  │    │ Collector Units (CU)                     │                      │  │  │
│  │  │    │  레지스터 뱅크에서 오퍼랜드 수집            │                      │  │  │
│  │  │    │  → 뱅크 충돌 시 대기                      │                      │  │  │
│  │  │    └─────────────────────────────────────────┘                      │  │  │
│  │  │    ┌─────────────────────────────────────────┐                      │  │  │
│  │  │    │ Register File Banks                      │                      │  │  │
│  │  │    │  (gpgpu_num_reg_banks 개의 뱅크)          │                      │  │  │
│  │  │    └─────────────────────────────────────────┘                      │  │  │
│  │  └──────────────────────────────┬───────────────────────────────────────┘  │  │
│  │                                  │                                          │  │
│  │  ┌──────────────────────────────┼───────────────────────────────────────┐  │  │
│  │  │  OC_EX 파이프라인 레지스터    ▼                                       │  │  │
│  │  │  OC_EX_SP | OC_EX_DP | OC_EX_INT | OC_EX_SFU | OC_EX_MEM |        │  │  │
│  │  │  OC_EX_TENSOR_CORE | OC_EX_SPEC_*                                   │  │  │
│  │  └──────────────────────────────┬───────────────────────────────────────┘  │  │
│  │                                  │ Execute                                  │  │
│  │  ┌──────────────────────────────┼───────────────────────────────────────┐  │  │
│  │  │  Functional Units            ▼                                       │  │  │
│  │  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐ │  │  │
│  │  │  │  SP  │ │  DP  │ │ INT  │ │ SFU  │ │Tensor│ │ SPEC │ │  LDST  │ │  │  │
│  │  │  │ Unit │ │ Unit │ │ Unit │ │      │ │ Core │ │ Unit │ │  Unit  │ │  │  │
│  │  │  └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘ └──┬─────┘ │  │  │
│  │  │     │        │        │        │        │        │        │        │  │  │
│  │  │     │        │        │  파이프라인 레지스터 (내부)        │        │  │  │
│  │  └─────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┘  │  │
│  │        ▼        ▼        ▼        ▼        ▼        ▼        ▼           │  │
│  │  ┌──────────────────────────────────────────────────────────────────────┐  │  │
│  │  │              EX_WB (Result Bus)                                      │  │  │
│  │  │              m_result_bus[] (MAX_ALU_LATENCY 비트)                    │  │  │
│  │  └──────────────────────────────┬───────────────────────────────────────┘  │  │
│  │                                  │ Writeback                                │  │
│  │                                  ▼                                          │  │
│  │  ┌──────────────────────────────────────────────────────────────────────┐  │  │
│  │  │  Writeback Stage                                                     │  │  │
│  │  │  - Operand Collector writeback (레지스터 파일에 결과 기록)              │  │  │
│  │  │  - Scoreboard::releaseRegisters (레지스터 해제)                       │  │  │
│  │  │  - warp_inst_complete (명령어 완료 처리)                               │  │  │
│  │  └──────────────────────────────────────────────────────────────────────┘  │  │
│  │                                                                            │  │
│  │  ┌───────────────────────────────────────────────────────────────────┐     │  │
│  │  │  SIMT Stacks (simt_stack)                                        │     │  │
│  │  │  워프당 하나, 분기 다이버전스 처리                                   │     │  │
│  │  └───────────────────────────────────────────────────────────────────┘     │  │
│  │                                                                            │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐                                │  │
│  │  │  L1D $   │  │  L1C $   │  │  L1T $   │  (데이터/상수/텍스처 캐시)      │  │
│  │  └──────────┘  └──────────┘  └──────────┘                                │  │
│  │                      │                                                     │  │
│  │              ┌───────▼───────┐                                             │  │
│  │              │ Interconnect  │  (m_icnt → 메모리 서브시스템)                │  │
│  │              └───────────────┘                                             │  │
│  └────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 핵심 파이프라인 스테이지 열거형

```c
// shader.h: pipeline_stage_name_t
enum pipeline_stage_name_t {
    ID_OC_SP = 0,           // Issue → Operand Collect (SP unit)
    ID_OC_DP,               // Issue → Operand Collect (DP unit)
    ID_OC_INT,              // Issue → Operand Collect (INT unit)
    ID_OC_SFU,              // Issue → Operand Collect (SFU)
    ID_OC_MEM,              // Issue → Operand Collect (Memory/LDST)
    OC_EX_SP,               // Operand Collect → Execute (SP)
    OC_EX_DP,               // Operand Collect → Execute (DP)
    OC_EX_INT,              // Operand Collect → Execute (INT)
    OC_EX_SFU,              // Operand Collect → Execute (SFU)
    OC_EX_MEM,              // Operand Collect → Execute (Memory)
    EX_WB,                  // Execute → Writeback (공통)
    ID_OC_TENSOR_CORE,      // Issue → Operand Collect (Tensor Core)
    OC_EX_TENSOR_CORE,      // Operand Collect → Execute (Tensor Core)
    N_PIPELINE_STAGES
};
```

### 1.3 실행 유닛 타입

```c
// shader.h
enum exec_unit_type_t {
    NONE = 0,
    SP = 1,           // Single-Precision Float
    SFU = 2,          // Special Function Unit (sin, cos, rsqrt 등)
    MEM = 3,          // Memory (Load/Store)
    DP = 4,           // Double-Precision Float
    INT = 5,          // Integer
    TENSOR = 6,       // Tensor Core
    SPECIALIZED = 7   // 사용자 정의 유닛
};
```

---

## 2. 파이프라인 스테이지 상세 분석

### 2.1 shader_core_ctx::cycle() -- 최상위 사이클 함수

```
shader.cc:3664
```

```c
void shader_core_ctx::cycle() {
    if (!isactive() && get_not_completed() == 0) return;
    // 비활성 코어이고 완료할 스레드가 없으면 즉시 리턴

    m_stats->shader_cycles[m_sid]++;  // 사이클 카운터 증가

    // ★ 역순 실행 (뒤에서부터 실행하여 파이프라인 버블을 방지)
    writeback();          // 5. Writeback: 결과를 레지스터 파일에 기록
    execute();            // 4. Execute: 기능 유닛에서 연산 수행
    read_operands();      // 3. Operand Collection: 레지스터 파일에서 오퍼랜드 읽기
    issue();              // 2. Issue: 워프 스케줄러가 명령어를 디스패치
    for (unsigned int i = 0; i < m_config->inst_fetch_throughput; ++i) {
        decode();         // 1b. Decode: 페치 버퍼 → I-Buffer
        fetch();          // 1a. Fetch: I-Cache에서 명령어 가져오기
    }
}
```

**역순 실행 이유**: 한 사이클 안에서 파이프라인 단계를 뒤에서부터 처리하면, 앞 단계의 결과가 뒤 단계로 "동시에" 전달되지 않고, 한 사이클의 시간적 순서가 올바르게 모델링된다. 즉, writeback이 먼저 EX_WB 레지스터를 비워야 execute가 새 결과를 넣을 수 있다.

### 2.2 Fetch 스테이지

```
shader.cc:924 -- shader_core_ctx::fetch()
```

**동작 흐름**:

```
fetch()
├── [L1I 캐시에서 응답 대기 중인 것이 있는 경우]
│   ├── m_L1I->access_ready() 확인
│   ├── 응답이 있으면 m_inst_fetch_buffer에 명령어 로드
│   └── 해당 워프의 imiss_pending 클리어
│
├── [새로운 페치가 필요한 경우]
│   ├── 라운드 로빈으로 워프 순회 (m_last_warp_fetched + 1부터)
│   ├── 각 워프에 대해:
│   │   ├── hardware_done() 체크 → 완료된 워프 처리 (exit)
│   │   └── functional_done()==false && imiss_pending()==false && ibuffer_empty()
│   │       ├── PC에서 주소 계산 (ppc = pc + PROGRAM_MEM_START)
│   │       ├── 16바이트 요청 생성 (mem_fetch)
│   │       ├── L1I 캐시 access()
│   │       │   ├── HIT: ifetch_buffer에 직접 로드
│   │       │   ├── MISS: imiss_pending 설정, 캐시 미스 큐에 들어감
│   │       │   └── RESERVATION_FAIL: 아무것도 안함 (다음 사이클 재시도)
│   │       └── break (한 사이클에 하나만 페치)
│
└── m_L1I->cycle()  // L1I 캐시 자체의 사이클 진행
```

**핵심 데이터 구조**:
```c
// shader.h:1103 -- 페치 버퍼
struct ifetch_buffer_t {
    bool m_valid;
    address_type m_pc;
    unsigned m_nbytes;
    unsigned m_warp_id;
};
```

### 2.3 Decode 스테이지

```
shader.cc:889 -- shader_core_ctx::decode()
```

```c
void shader_core_ctx::decode() {
    if (m_inst_fetch_buffer.m_valid) {
        // 페치 버퍼에서 PC를 읽어 1~2개의 명령어를 디코딩
        address_type pc = m_inst_fetch_buffer.m_pc;
        const warp_inst_t *pI1 = get_next_inst(m_inst_fetch_buffer.m_warp_id, pc);

        // 첫 번째 명령어를 I-Buffer 슬롯 0에 넣기
        m_warp[m_inst_fetch_buffer.m_warp_id]->ibuffer_fill(0, pI1);
        m_warp[m_inst_fetch_buffer.m_warp_id]->inc_inst_in_pipeline();

        if (pI1) {
            // 통계 카운터 업데이트 (INT/FP 디코드 카운트)
            // 두 번째 명령어 시도
            const warp_inst_t *pI2 = get_next_inst(..., pc + pI1->isize);
            if (pI2) {
                // I-Buffer 슬롯 1에 넣기
                m_warp[...]->ibuffer_fill(1, pI2);
            }
        }
        m_inst_fetch_buffer.m_valid = false;  // 페치 버퍼 소비 완료
    }
}
```

**I-Buffer (Instruction Buffer)**:
- 워프당 2개 슬롯 (`IBUFFER_SIZE = 2`)
- `ibuffer_fill()`: 명령어를 슬롯에 저장
- `ibuffer_next_inst()` / `ibuffer_next_valid()`: 다음 이슈할 명령어 조회
- `ibuffer_step()`: 다음 슬롯으로 이동 (원형 인덱스)
- `ibuffer_flush()`: 컨트롤 해저드 시 전체 플러시

### 2.4 Issue 스테이지

```
shader.cc:1128 -- shader_core_ctx::issue()
```

```c
void shader_core_ctx::issue() {
    // 여러 스케줄러 간의 공정한 라운드 로빈
    unsigned j;
    for (unsigned i = 0; i < schedulers.size(); i++) {
        j = (Issue_Prio + i) % schedulers.size();
        schedulers[j]->cycle();  // ← 각 스케줄러의 핵심 사이클
    }
    Issue_Prio = (Issue_Prio + 1) % schedulers.size();
}
```

여러 워프 스케줄러가 있을 때 `Issue_Prio`를 라운드 로빈으로 돌려 공정성을 보장한다.

#### 2.4.1 scheduler_unit::cycle() -- 워프 스케줄러의 핵심 로직

```
shader.cc:1259 -- scheduler_unit::cycle()
```

이 함수가 GPGPU-Sim에서 가장 중요한 함수 중 하나이다.

```
scheduler_unit::cycle()
│
├── order_warps()  // 파생 클래스에서 오버라이드하여 워프 우선순위 결정
│   (m_next_cycle_prioritized_warps 리스트를 채움)
│
├── for (우선순위 순서대로 각 워프 순회):
│   │
│   ├── done_exit() 체크 → 건너뛰기
│   │
│   ├── while (!waiting && !ibuffer_empty && checked < max_issue):
│   │   │
│   │   ├── pI = ibuffer_next_inst()  // I-Buffer에서 다음 명령어
│   │   │
│   │   ├── SIMT 스택에서 (pc, rpc) 조회
│   │   │   get_pdom_stack_top_info(warp_id, pI, &pc, &rpc)
│   │   │
│   │   ├── [컨트롤 해저드 체크] pc != pI->pc 이면:
│   │   │   ├── 분기로 인해 PC가 바뀜
│   │   │   ├── warp.set_next_pc(pc)
│   │   │   └── warp.ibuffer_flush()  // I-Buffer 비우기
│   │   │
│   │   ├── [Scoreboard 체크] checkCollision(warp_id, pI):
│   │   │   ├── 충돌 없으면 → ready_inst = true
│   │   │   └── 충돌 있으면 → RAW/WAW 해저드, 이 명령어 건너뛰기
│   │   │
│   │   ├── [디스패치 결정] 명령어 타입에 따라 적절한 파이프라인 레지스터로 보냄:
│   │   │   ├── LOAD/STORE/MEMBAR → m_mem_out (ID_OC_MEM)
│   │   │   ├── SP_OP → m_sp_out (ID_OC_SP)
│   │   │   ├── INT_OP (Volta+) → m_int_out (ID_OC_INT)
│   │   │   ├── DP_OP → m_dp_out (ID_OC_DP) 또는 SFU (Fermi)
│   │   │   ├── SFU_OP → m_sfu_out (ID_OC_SFU)
│   │   │   ├── TENSOR_CORE_OP → m_tensor_core_out (ID_OC_TENSOR_CORE)
│   │   │   └── SPEC_UNIT → m_spec_cores_out[spec_id]
│   │   │
│   │   ├── 파이프라인 레지스터가 비어있는지 확인 (has_free)
│   │   │
│   │   ├── [Dual Issue 체크] gpgpu_dual_issue_diff_exec_units:
│   │   │   └── 이전 이슈와 다른 실행 유닛이어야 함 (Maxwell/Pascal 모델)
│   │   │
│   │   └── issue_warp() 호출 → 실제 이슈 수행
│   │
│   ├── issued > 0이면:
│   │   ├── m_last_supervised_issued 갱신
│   │   ├── single_issue / dual_issue 통계 기록
│   │   └── break (한 사이클에 하나의 워프만 이슈)
│   │
│   └── (다음 워프로)
│
└── [스톨 통계 기록]
    ├── valid_inst 없음 → idle/control hazard (distro[0])
    ├── valid이지만 ready 아님 → RAW hazard (distro[1])
    └── ready이지만 issued 아님 → pipeline stall (distro[2])
```

#### 2.4.2 issue_warp() -- 실제 명령어 이슈

```
shader.cc:1036 -- shader_core_ctx::issue_warp()
```

```c
void shader_core_ctx::issue_warp(register_set &pipe_reg_set,
                                 const warp_inst_t *next_inst,
                                 const active_mask_t &active_mask,
                                 unsigned warp_id, unsigned sch_id) {
    // 1. 파이프라인 레지스터에서 빈 슬롯 찾기
    warp_inst_t **pipe_reg = pipe_reg_set.get_free(sub_core_model, sch_id);

    // 2. I-Buffer에서 명령어 제거
    m_warp[warp_id]->ibuffer_free();

    // 3. 파이프라인 레지스터에 명령어 복사 (정적 + 동적 정보)
    **pipe_reg = *next_inst;
    (*pipe_reg)->issue(active_mask, warp_id, ...);

    // 4. 기능적 실행 (PTX 시뮬레이션)
    func_exec_inst(**pipe_reg);

    // 5. 배리어 처리
    if (next_inst->op == BARRIER_OP) {
        m_barriers.warp_reaches_barrier(...);
    } else if (next_inst->op == MEMORY_BARRIER_OP) {
        m_warp[warp_id]->set_membar();
    }

    // 6. SIMT 스택 업데이트 (분기 다이버전스 처리)
    updateSIMTStack(warp_id, *pipe_reg);

    // 7. Scoreboard에 출력 레지스터 예약
    m_scoreboard->reserveRegisters(*pipe_reg);

    // 8. 다음 PC 설정
    m_warp[warp_id]->set_next_pc(next_inst->pc + next_inst->isize);
}
```

**중요**: `func_exec_inst()`는 이슈 시점에 호출되어 기능적 시뮬레이션(PTX 명령어 실행)을 수행한다. GPGPU-Sim은 기능적 시뮬레이션과 타이밍 시뮬레이션을 분리하는 구조로, 이슈 단계에서 기능적 결과를 확정하고 이후 타이밍만 시뮬레이션한다.

### 2.5 Read Operands (Operand Collection) 스테이지

```
shader.cc:1712 -- shader_core_ctx::read_operands()
```

```c
void shader_core_ctx::read_operands() {
    for (unsigned int i = 0; i < m_config->reg_file_port_throughput; ++i)
        m_operand_collector.step();
}
```

`opndcoll_rfu_t::step()`의 내부:

```c
void step() {
    dispatch_ready_cu();   // 준비된 CU의 명령어를 OC_EX 레지스터로 디스패치
    allocate_reads();      // 레지스터 뱅크 읽기 요청을 아비터에 할당
    for (p : m_in_ports)
        allocate_cu(p);    // ID_OC 레지스터에서 CU로 명령어 할당
    process_banks();       // 뱅크 할당 초기화 (매 사이클 리셋)
}
```

**오퍼랜드 수집기(Operand Collector)** 는 실제 GPU의 레지스터 파일 뱅크 충돌을 모델링한다:
- 레지스터 파일은 여러 뱅크로 나뉘어 있다
- 같은 뱅크에 대한 동시 접근은 뱅크 충돌을 일으킨다
- Collector Unit(CU)은 명령어의 모든 소스 오퍼랜드가 수집될 때까지 대기한다
- 모든 오퍼랜드가 준비되면 OC_EX 레지스터로 디스패치한다

### 2.6 Execute 스테이지

```
shader.cc:1805 -- shader_core_ctx::execute()
```

```c
void shader_core_ctx::execute() {
    // 1. 결과 버스를 1비트 오른쪽 시프트 (시간 진행)
    for (unsigned i = 0; i < num_result_bus; i++) {
        *(m_result_bus[i]) >>= 1;
    }

    // 2. 각 기능 유닛을 사이클 진행
    for (unsigned n = 0; n < m_num_function_units; n++) {
        unsigned multiplier = m_fu[n]->clock_multiplier();
        for (unsigned c = 0; c < multiplier; c++)
            m_fu[n]->cycle();  // 파이프라인 내부 시프트

        // 3. OC_EX 레지스터에서 기능 유닛으로 명령어 투입
        unsigned issue_port = m_issue_port[n];
        register_set &issue_inst = m_pipeline_reg[issue_port];
        warp_inst_t **ready_reg = issue_inst.get_ready(...);

        if (issue_inst.has_ready(...) && m_fu[n]->can_issue(**ready_reg)) {
            bool schedule_wb_now = !m_fu[n]->stallable();
            int resbus = -1;

            if (schedule_wb_now &&
                (resbus = test_res_bus((*ready_reg)->latency)) != -1) {
                // 결과 버스 예약 (ALU 명령어 등)
                m_result_bus[resbus]->set((*ready_reg)->latency);
                m_fu[n]->issue(issue_inst);
            } else if (!schedule_wb_now) {
                // LDST unit은 stallable이므로 결과 버스 예약 없이 이슈
                m_fu[n]->issue(issue_inst);
            }
            // else: 결과 버스 예약 실패 → 스톨
        }
    }
}
```

**결과 버스(Result Bus)**: `m_result_bus`는 `bitset<MAX_ALU_LATENCY>`로, latency만큼의 미래 시점에 결과가 도착함을 표시한다. 매 사이클 `>>= 1`로 시프트하여 시간을 전진시킨다. 비트가 이미 set된 시점에는 다른 명령어가 같은 버스를 사용할 수 없어 구조적 해저드를 모델링한다.

**기능 유닛별 파이프라인**: `pipelined_simd_unit::cycle()`은 내부 파이프라인 레지스터를 시프트하고, 마지막 단계의 명령어를 EX_WB 레지스터로 이동시킨다.

### 2.7 Writeback 스테이지

```
shader.cc:1930 -- shader_core_ctx::writeback()
```

```c
void shader_core_ctx::writeback() {
    // 파이프라인 듀티 사이클 통계 계산
    m_stats->m_pipeline_duty_cycle[m_sid] = ...;

    // EX_WB 레지스터에서 준비된 명령어를 하나씩 처리
    warp_inst_t **preg = m_pipeline_reg[EX_WB].get_ready();
    warp_inst_t *pipe_reg = (preg == NULL) ? NULL : *preg;

    while (preg and !pipe_reg->empty()) {
        // 1. Operand Collector를 통해 레지스터 파일에 결과 기록
        m_operand_collector.writeback(*pipe_reg);

        // 2. Scoreboard에서 출력 레지스터 해제
        unsigned warp_id = pipe_reg->warp_id();
        m_scoreboard->releaseRegisters(pipe_reg);

        // 3. 파이프라인 카운터 감소
        m_warp[warp_id]->dec_inst_in_pipeline();

        // 4. 명령어 완료 처리 (통계 업데이트)
        warp_inst_complete(*pipe_reg);

        // 5. 파이프라인 레지스터 클리어
        pipe_reg->clear();

        // 다음 준비된 명령어
        preg = m_pipeline_reg[EX_WB].get_ready();
        pipe_reg = (preg == NULL) ? NULL : *preg;
    }
}
```

---

## 3. 워프 스케줄링 정책

### 3.1 스케줄러 종류

```c
// shader.h:366
enum concrete_scheduler {
    CONCRETE_SCHEDULER_LRR = 0,           // Loose Round Robin
    CONCRETE_SCHEDULER_GTO,               // Greedy Then Oldest
    CONCRETE_SCHEDULER_TWO_LEVEL_ACTIVE,  // Two-Level Active
    CONCRETE_SCHEDULER_RRR,               // Round Robin (strict)
    CONCRETE_SCHEDULER_WARP_LIMITING,     // Static Warp Limiting
    CONCRETE_SCHEDULER_OLDEST_FIRST,      // Oldest First
    NUM_CONCRETE_SCHEDULERS
};
```

### 3.2 스케줄러 우선순위 함수

```c
// shader.h:354
enum scheduler_prioritization_type {
    SCHEDULER_PRIORITIZATION_LRR = 0,   // Loose Round Robin
    SCHEDULER_PRIORITIZATION_SRR,       // Strict Round Robin
    SCHEDULER_PRIORITIZATION_GTO,       // Greedy Then Oldest
    SCHEDULER_PRIORITIZATION_GTLRR,     // Greedy Then Loose Round Robin
    SCHEDULER_PRIORITIZATION_GTY,       // Greedy Then Youngest
    SCHEDULER_PRIORITIZATION_OLDEST,    // Oldest First
    SCHEDULER_PRIORITIZATION_YOUNGEST,  // Youngest First
};
```

### 3.3 LRR (Loose Round Robin) 스케줄러

```
shader.cc:1591
```

```c
void lrr_scheduler::order_warps() {
    order_lrr(m_next_cycle_prioritized_warps, m_supervised_warps,
              m_last_supervised_issued, m_supervised_warps.size());
}
```

`order_lrr()` 알고리즘:
1. `m_last_supervised_issued` 다음 위치부터 시작
2. 리스트를 순환하며 모든 워프를 우선순위 리스트에 추가
3. 마지막에 이슈한 워프가 가장 낮은 우선순위를 갖게 됨

```
예시: 워프 [0,1,2,3], 마지막 이슈 = 워프 1
→ 우선순위: [2, 3, 0, 1]
```

LRR에서 "Loose"의 의미: 마지막 이슈된 워프에서 시작하지만, 중간에 워프가 대기/완료 상태여도 건너뛰지 않고 리스트에 포함한다. 이슈 가능 여부는 `cycle()` 내부에서 나중에 판단한다.

### 3.4 GTO (Greedy Then Oldest) 스케줄러

```
shader.cc:1600
```

```c
void gto_scheduler::order_warps() {
    order_by_priority(m_next_cycle_prioritized_warps, m_supervised_warps,
                      m_last_supervised_issued, m_supervised_warps.size(),
                      ORDERING_GREEDY_THEN_PRIORITY_FUNC,
                      scheduler_unit::sort_warps_by_oldest_dynamic_id);
}
```

GTO 동작 원리:
1. **Greedy**: 마지막에 이슈한 워프를 최우선으로 배치 (동일 워프 연속 이슈 선호)
2. **Then Oldest**: 나머지는 `dynamic_warp_id`가 작은 순서(= 오래된 워프)로 정렬

```
예시: 워프 [w0(d=5), w1(d=2), w2(d=8), w3(d=1)], 마지막 이슈 = w2
→ 우선순위: [w2, w3, w1, w0, w2] (w2 우선, 나머지는 dynamic_id 순)
```

GTO가 효과적인 이유: 하나의 워프에서 메모리 접근이 발생하면 그 워프가 스톨될 때까지 계속 이슈하여 메모리 레이턴시를 히딩(hiding)하는 데 유리하다.

### 3.5 Two-Level Active 스케줄러

```
shader.cc:1630
```

두 수준의 워프 관리:
- **Active Pool** (`m_next_cycle_prioritized_warps`): 현재 스케줄 대상 (최대 `m_max_active_warps`개)
- **Pending Pool** (`m_pending_warps`): 대기 중인 워프

```c
void two_level_active_scheduler::order_warps() {
    // 1. 대기 중인 워프를 pending으로 강등
    for (각 active 워프) {
        if (waiting 또는 long operation에 의존) {
            pending으로 이동;
            num_demoted++;
        }
    }

    // 2. pending에서 active로 승격
    while (active 풀에 빈자리가 있으면) {
        pending.front()를 active에 추가;
        num_promoted++;
    }
    assert(num_promoted == num_demoted);  // 항상 같은 수만큼 교환
}
```

**Inner level**: Active Pool 내에서의 우선순위 (보통 LRR)
**Outer level**: Active/Pending 간의 교환 정책 (보통 SRR)

설정 형식: `two_level_active:<max_active>:<inner_priority>:<outer_priority>`

### 3.6 SWL (Static Warp Limiting) 스케줄러

```
shader.cc:1699
```

GTO와 동일하되, 동시에 고려하는 워프 수를 `m_num_warps_to_limit`로 제한한다.

```c
void swl_scheduler::order_warps() {
    order_by_priority(...,
                      MIN(m_num_warps_to_limit, m_supervised_warps.size()),
                      ORDERING_GREEDY_THEN_PRIORITY_FUNC,
                      sort_warps_by_oldest_dynamic_id);
}
```

### 3.7 워프 분배 (Warp-to-Scheduler Assignment)

```
shader.cc:261
```

```c
// 워프를 스케줄러에 균등 분배 (인터리빙)
for (unsigned i = 0; i < m_warp.size(); i++) {
    schedulers[i % gpgpu_num_sched_per_core]->add_supervised_warp_id(i);
}
```

예: 4개 스케줄러, 48개 워프의 경우:
- 스케줄러 0: 워프 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44
- 스케줄러 1: 워프 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45
- 스케줄러 2: 워프 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46
- 스케줄러 3: 워프 3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47

---

## 4. SIMT 스택 동작 원리 (분기 처리)

### 4.1 SIMT 스택 구조

```c
// abstract_hardware_model.h:439
class simt_stack {
protected:
    struct simt_stack_entry {
        address_type m_pc;               // 현재 PC
        unsigned int m_calldepth;        // 함수 호출 깊이
        simt_mask_t m_active_mask;       // 활성 스레드 마스크
        address_type m_recvg_pc;         // 재수렴(reconvergence) PC
        unsigned long long m_branch_div_cycle;  // 분기 발생 사이클
        stack_entry_type m_type;         // NORMAL 또는 CALL
    };

    std::deque<simt_stack_entry> m_stack;  // 스택 (deque 사용)
};
```

### 4.2 Post-Dominator 기반 재수렴 메커니즘

GPGPU-Sim은 PDOM(Post-Dominator) 기반의 SIMT 스택을 사용한다. 분기 명령어에서 워프 내 스레드들이 다른 경로로 분기할 때, SIMT 스택을 이용하여 각 경로를 순차적으로 실행하고 재수렴 지점에서 다시 합친다.

### 4.3 simt_stack::update() 상세 분석

```
abstract_hardware_model.cc:1046
```

```
update(thread_done, next_pc, recvg_pc, next_inst_op, next_inst_size, next_inst_pc)
│
├── 현재 스택 최상위 정보 읽기:
│   top_active_mask, top_recvg_pc, top_pc, top_type
│
├── [다이버전스 경로 수집]
│   각 활성 스레드의 next_pc를 분석하여 divergent_paths 맵 생성
│   (같은 next_pc를 가진 스레드끼리 그룹화)
│   → num_divergent_paths ≤ 2 (분기는 최대 2갈래)
│
├── [특수 경우: CALL 명령어]
│   └── STACK_ENTRY_TYPE_CALL 엔트리를 스택에 push
│
├── [특수 경우: RET 명령어 + 최상위가 CALL 엔트리]
│   └── 스택 pop하고 리턴 PC 설정
│
├── [일반 분기 처리]
│   for (각 다이버전트 경로):
│       ├── not_taken_pc 경로를 먼저 처리 (안정적 순서 보장)
│       │
│       ├── tmp_next_pc == top_recvg_pc 이면:
│       │   └── 자동 재수렴, 새 엔트리 불필요 → continue
│       │
│       ├── [다이버전스 발생 (num_divergent_paths > 1)]
│       │   ├── 현재 최상위 엔트리를 재수렴 엔트리로 변환
│       │   │   m_stack.back().m_pc = recvg_pc
│       │   └── 새 빈 엔트리 push
│       │
│       └── 현재 최상위에 경로 정보 설정:
│           m_stack.back().m_pc = tmp_next_pc
│           m_stack.back().m_active_mask = tmp_active_mask
│           m_stack.back().m_recvg_pc = new_recvg_pc (또는 top_recvg_pc)
│           → 새 빈 엔트리 push
│
└── m_stack.pop_back()  // 마지막 빈 엔트리 제거
```

### 4.4 분기 다이버전스 예시

```
                        분기 전 스택
                    ┌──────────────────────────┐
                    │ PC=100, mask=11111111     │  ← top
                    │ recvg_pc = (없음)         │
                    └──────────────────────────┘

  if (threadIdx.x < 4)  // PC=100에서 분기
  → 스레드 0~3: taken (PC=200)
  → 스레드 4~7: not-taken (PC=150)
  → recvg_pc = 300 (컴파일러가 계산)

                        분기 후 스택
                    ┌──────────────────────────┐
                    │ PC=200, mask=00001111     │  ← top (taken 경로)
                    │ recvg_pc = 300            │
                    ├──────────────────────────┤
                    │ PC=150, mask=11110000     │  (not-taken 경로)
                    │ recvg_pc = 300            │
                    ├──────────────────────────┤
                    │ PC=300, mask=11111111     │  (재수렴 지점)
                    │ recvg_pc = (없음)         │
                    └──────────────────────────┘

  → taken 경로 먼저 실행 (mask=00001111)
  → taken 경로가 PC=300에 도달하면 pop
  → not-taken 경로 실행 (mask=11110000)
  → not-taken 경로가 PC=300에 도달하면 pop
  → 재수렴 엔트리에서 모든 스레드 활성 (mask=11111111)
```

---

## 5. Scoreboard 메커니즘 (데이터 해저드 감지)

### 5.1 Scoreboard 클래스 구조

```c
// scoreboard.h:40
class Scoreboard {
public:
    Scoreboard(unsigned sid, unsigned n_warps, class gpgpu_t *gpu);

    void reserveRegisters(const warp_inst_t *inst);    // 이슈 시 호출
    void releaseRegisters(const warp_inst_t *inst);    // Writeback 시 호출
    void releaseRegister(unsigned wid, unsigned regnum);

    bool checkCollision(unsigned wid, const inst_t *inst) const;  // 이슈 전 체크
    bool pendingWrites(unsigned wid) const;  // 미완료 쓰기 존재 여부
    const bool islongop(unsigned warp_id, unsigned regnum);  // 장기 연산 여부

private:
    std::vector<std::set<unsigned>> reg_table;     // [워프ID] → {예약된 레지스터 번호들}
    std::vector<std::set<unsigned>> longopregs;    // [워프ID] → {장기 연산 레지스터들}
};
```

### 5.2 레지스터 예약 (reserveRegisters)

```c
// scoreboard.cc:83
void Scoreboard::reserveRegisters(const warp_inst_t* inst) {
    // 모든 출력 레지스터를 예약
    for (unsigned r = 0; r < MAX_OUTPUT_VALUES; r++) {
        if (inst->out[r] > 0) {
            reserveRegister(inst->warp_id(), inst->out[r]);
        }
    }

    // 장기 연산(글로벌/로컬/텍스처 메모리 로드)이면 longopregs에도 기록
    if (inst->is_load() && (inst->space == global/local/param/tex)) {
        for (unsigned r = 0; r < MAX_OUTPUT_VALUES; r++) {
            if (inst->out[r] > 0) {
                longopregs[inst->warp_id()].insert(inst->out[r]);
            }
        }
    }
}
```

### 5.3 충돌 검사 (checkCollision)

```c
// scoreboard.cc:128
bool Scoreboard::checkCollision(unsigned wid, const inst_t* inst) const {
    // 명령어의 모든 입력 + 출력 + 술어 레지스터를 수집
    std::set<int> inst_regs;
    for (출력 레지스터) inst_regs.insert(inst->out[i]);   // WAW 체크
    for (입력 레지스터) inst_regs.insert(inst->in[i]);    // RAW 체크
    if (inst->pred > 0) inst_regs.insert(inst->pred);     // 술어 레지스터
    if (inst->ar1 > 0)  inst_regs.insert(inst->ar1);      // 주소 레지스터
    if (inst->ar2 > 0)  inst_regs.insert(inst->ar2);

    // 예약된 레지스터와 교집합이 있으면 충돌
    for (각 inst_regs의 레지스터) {
        if (reg_table[wid]에 존재) return true;  // 해저드!
    }
    return false;  // 충돌 없음
}
```

### 5.4 해저드 타입 분석

GPGPU-Sim의 Scoreboard는 다음 해저드를 감지한다:

| 해저드 | 설명 | 감지 방법 |
|--------|------|-----------|
| **RAW** (Read After Write) | 이전 명령어가 쓸 레지스터를 읽으려 함 | 입력 레지스터가 reg_table에 있음 |
| **WAW** (Write After Write) | 이전 명령어가 쓸 레지스터에 다시 쓰려 함 | 출력 레지스터가 reg_table에 있음 |
| **WAR** (Write After Read) | 감지하지 않음 | In-order issue이므로 WAR은 발생하지 않음 |

### 5.5 Scoreboard 연동 흐름도

```
Issue 시:
  scheduler_unit::cycle()
    → checkCollision(warp_id, inst)  [충돌 체크]
    → 충돌 없으면 issue_warp()
      → m_scoreboard->reserveRegisters(inst)  [출력 레지스터 예약]

Writeback 시 (ALU 계열):
  shader_core_ctx::writeback()
    → m_scoreboard->releaseRegisters(pipe_reg)  [레지스터 해제]

Writeback 시 (메모리 계열):
  ldst_unit::writeback()
    → m_scoreboard->releaseRegister(wid, regnum)  [개별 해제]
    (메모리 응답이 올 때마다 하나씩 해제)
```

---

## 6. 주요 클래스 계층 구조

### 6.1 클래스 상속도

```
core_t (abstract_hardware_model.h)
└── shader_core_ctx (shader.h:2059)
    └── exec_shader_core_ctx (shader.h:2580)
        (execution-driven 모드용)

simt_core_cluster (shader.h:2612)
└── exec_simt_core_cluster (shader.h:2676)
    └── sst_simt_core_cluster (shader.h:2694)

scheduler_unit (shader.h:376)
├── lrr_scheduler
├── rrr_scheduler
├── gto_scheduler
├── oldest_scheduler
├── two_level_active_scheduler
└── swl_scheduler

simd_function_unit (shader.h:1121)
└── pipelined_simd_unit (shader.h:1153)
    ├── sp_unit
    ├── dp_unit
    ├── int_unit
    ├── sfu
    ├── tensor_core
    ├── specialized_unit
    └── ldst_unit (shader.h:1345)
```

### 6.2 shader_core_ctx 클래스

SM 하나를 나타내는 핵심 클래스이다.

```c
class shader_core_ctx : public core_t {
    // === 파이프라인 구성 요소 ===
    read_only_cache *m_L1I;                    // 명령어 캐시
    ifetch_buffer_t m_inst_fetch_buffer;        // 페치 버퍼
    std::vector<shd_warp_t *> m_warp;           // 워프 배열 (I-Buffer 포함)
    std::vector<register_set> m_pipeline_reg;   // 파이프라인 레지스터들
    Scoreboard *m_scoreboard;                   // 데이터 해저드 감지
    opndcoll_rfu_t m_operand_collector;         // 오퍼랜드 수집기
    std::vector<scheduler_unit *> schedulers;    // 워프 스케줄러들
    std::vector<simd_function_unit *> m_fu;     // 기능 유닛들
    ldst_unit *m_ldst_unit;                     // 로드/스토어 유닛
    std::vector<std::bitset<MAX_ALU_LATENCY> *> m_result_bus;  // 결과 버스

    // === 상태 관리 ===
    unsigned m_sid;                             // Shader ID
    unsigned m_n_active_cta;                    // 활성 CTA 수
    unsigned m_not_completed;                   // 미완료 스레드 수
    int m_active_warps;                         // 활성 워프 수
    barrier_set_t m_barriers;                   // 배리어 관리
    simt_stack **m_simt_stack;                  // SIMT 스택 (core_t에서 상속)

    // === 핵심 파이프라인 함수 ===
    void cycle();           // 매 사이클 호출
    void fetch();           // 명령어 페치
    void decode();          // 디코드 및 I-Buffer 채우기
    void issue();           // 스케줄러 사이클
    void read_operands();   // 오퍼랜드 수집
    void execute();         // 기능 유닛 실행
    void writeback();       // 결과 기록
};
```

### 6.3 simt_core_cluster 클래스

여러 SM을 하나의 클러스터로 묶는다.

```c
class simt_core_cluster {
    shader_core_ctx **m_core;           // 소속 코어 배열
    unsigned m_cluster_id;
    std::list<mem_fetch *> m_response_fifo;  // 메모리 응답 FIFO

    void core_cycle();   // 각 코어의 cycle() 호출
    void icnt_cycle();   // 인터커넥트 사이클 (메모리 응답 처리)
};
```

### 6.4 shd_warp_t 클래스

개별 워프의 상태를 관리한다.

```c
class shd_warp_t {
    // I-Buffer
    static const unsigned IBUFFER_SIZE = 2;
    ibuffer_entry m_ibuffer[IBUFFER_SIZE];
    unsigned m_next;  // 다음 이슈할 슬롯 인덱스

    // 워프 식별
    unsigned m_warp_id;              // 정적 워프 슬롯 번호
    unsigned m_dynamic_warp_id;      // 동적 워프 번호 (고유)
    unsigned m_cta_id;               // 소속 CTA

    // 워프 상태
    address_type m_next_pc;          // 다음 실행할 PC
    unsigned n_completed;            // 완료된 스레드 수
    std::bitset<MAX_WARP_SIZE> m_active_threads;  // 활성 스레드 마스크
    bool m_imiss_pending;            // I-Cache 미스 대기 중
    bool m_membar;                   // 메모리 배리어 대기
    bool m_done_exit;                // 종료 처리 완료
    unsigned m_stores_outstanding;   // 미완료 스토어 수
    unsigned m_inst_in_pipeline;     // 파이프라인 내 명령어 수
};
```

**워프 상태 판단 함수들**:
- `functional_done()`: 모든 스레드가 완료됨
- `hardware_done()`: functional_done이고 파이프라인에 명령어가 없음
- `waiting()`: 배리어, membar, 미완료 atomic, cdp_latency 등으로 대기 중
- `ibuffer_empty()`: I-Buffer에 유효한 명령어 없음

---

## 7. 핵심 함수 라인 바이 라인 주석

### 7.1 shader_core_ctx::cycle() [shader.cc:3664]

```c
void shader_core_ctx::cycle() {
    // 비활성 코어이고 완료 대기 스레드도 없으면 아무 것도 하지 않음
    // isactive()는 m_n_active_cta > 0이면 true
    if (!isactive() && get_not_completed() == 0) return;

    // 이 SM의 사이클 카운터 증가
    m_stats->shader_cycles[m_sid]++;

    // ★ 파이프라인을 역순으로 실행 (뒤→앞)
    // 이렇게 하면 같은 사이클에 앞 스테이지의 결과가 뒤 스테이지로
    // 바로 전달되지 않아 정확한 파이프라인 동작을 모델링함
    writeback();       // 5단계: EX_WB → 레지스터 파일, Scoreboard 해제
    execute();         // 4단계: OC_EX → 기능유닛 실행 → EX_WB
    read_operands();   // 3단계: ID_OC → Operand Collector → OC_EX
    issue();           // 2단계: I-Buffer → Scoreboard 체크 → ID_OC
    // 페치+디코드는 inst_fetch_throughput만큼 반복 (보통 1)
    for (unsigned int i = 0; i < m_config->inst_fetch_throughput; ++i) {
        decode();      // 1b: Fetch Buffer → I-Buffer (2슬롯)
        fetch();       // 1a: I-Cache → Fetch Buffer
    }
}
```

### 7.2 shader_core_ctx::fetch() [shader.cc:924]

```c
void shader_core_ctx::fetch() {
    if (!m_inst_fetch_buffer.m_valid) {
        // 페치 버퍼가 비어있을 때만 새 명령어를 가져옴

        if (m_L1I->access_ready()) {
            // ── 경우 1: L1I 캐시에서 이전 미스 응답이 도착 ──
            mem_fetch *mf = m_L1I->next_access();
            m_warp[mf->get_wid()]->clear_imiss_pending();
            // 페치 버퍼에 로드
            m_inst_fetch_buffer = ifetch_buffer_t(
                m_warp[mf->get_wid()]->get_pc(),  // 워프의 현재 PC
                mf->get_access_size(),             // 바이트 수
                mf->get_wid());                    // 워프 ID
            m_inst_fetch_buffer.m_valid = true;
            m_warp[mf->get_wid()]->set_last_fetch(m_gpu->gpu_sim_cycle);
            delete mf;
        } else {
            // ── 경우 2: 새 워프에서 페치 시도 ──
            // 라운드 로빈으로 활성 워프 순회
            for (unsigned i = 0; i < m_config->max_warps_per_shader; i++) {
                unsigned warp_id = (m_last_warp_fetched + 1 + i)
                                   % m_config->max_warps_per_shader;

                // 완료된 워프의 exit 처리
                if (m_warp[warp_id]->hardware_done() &&
                    !m_scoreboard->pendingWrites(warp_id) &&
                    !m_warp[warp_id]->done_exit()) {
                    // 각 스레드의 CTA 종료 등록
                    // m_not_completed 감소, m_active_warps 감소
                }

                // 페치 가능한 워프 찾기
                if (!m_warp[warp_id]->functional_done() &&  // 아직 실행 중
                    !m_warp[warp_id]->imiss_pending() &&    // I-miss 대기 아님
                    m_warp[warp_id]->ibuffer_empty()) {     // I-Buffer 비어있음

                    address_type pc = m_warp[warp_id]->get_pc();
                    address_type ppc = pc + PROGRAM_MEM_START;
                    unsigned nbytes = 16;  // 16바이트 페치

                    // L1I 캐시 접근
                    status = m_L1I->access(ppc, mf, ...);
                    if (status == MISS) {
                        m_warp[warp_id]->set_imiss_pending();
                        // 다음 사이클에 캐시 응답이 올 때까지 대기
                    } else if (status == HIT) {
                        m_inst_fetch_buffer = ifetch_buffer_t(pc, nbytes, warp_id);
                        // 바로 페치 버퍼에 로드
                    }
                    break;  // 한 사이클에 하나의 워프만 페치
                }
            }
        }
    }
    m_L1I->cycle();  // L1I 캐시 자체의 사이클 진행 (미스 큐 처리 등)
}
```

### 7.3 shader_core_ctx::issue() [shader.cc:1128]

```c
void shader_core_ctx::issue() {
    unsigned j;
    // 각 스케줄러를 순환하며 실행
    // Issue_Prio는 공정성을 위해 매 사이클 1씩 증가
    for (unsigned i = 0; i < schedulers.size(); i++) {
        j = (Issue_Prio + i) % schedulers.size();
        schedulers[j]->cycle();
        // cycle() 내에서:
        //   1. order_warps() 호출 → 워프 우선순위 결정
        //   2. 우선순위 순서대로 워프 순회
        //   3. Scoreboard 체크 → 해저드 없으면 issue_warp() 호출
        //   4. 이슈 성공 시 break
    }
    Issue_Prio = (Issue_Prio + 1) % schedulers.size();
}
```

### 7.4 shader_core_ctx::execute() [shader.cc:1805]

```c
void shader_core_ctx::execute() {
    // 1. 결과 버스 시간 진행 (1비트 오른쪽 시프트)
    // 각 결과 버스는 MAX_ALU_LATENCY 비트의 bitset
    // bit 0이 "이번 사이클에 도착"을 의미
    for (unsigned i = 0; i < num_result_bus; i++) {
        *(m_result_bus[i]) >>= 1;
    }

    // 2. 각 기능 유닛 처리
    for (unsigned n = 0; n < m_num_function_units; n++) {
        // clock_multiplier: SFU 등은 코어 클럭보다 빠르게 동작할 수 있음
        unsigned multiplier = m_fu[n]->clock_multiplier();
        for (unsigned c = 0; c < multiplier; c++)
            m_fu[n]->cycle();  // 파이프라인 레지스터 시프트

        m_fu[n]->active_lanes_in_pipeline();  // 파워 모델용 통계

        // OC_EX 레지스터에서 이 기능 유닛으로 이슈할 준비가 된 명령어 확인
        unsigned issue_port = m_issue_port[n];
        register_set &issue_inst = m_pipeline_reg[issue_port];
        warp_inst_t **ready_reg = issue_inst.get_ready(...);

        if (issue_inst.has_ready(...) && m_fu[n]->can_issue(**ready_reg)) {
            bool schedule_wb_now = !m_fu[n]->stallable();
            // stallable = true인 것은 ldst_unit뿐
            // ALU 유닛들은 결정적이므로 이슈 시점에 결과 버스 예약

            int resbus = -1;
            if (schedule_wb_now &&
                (resbus = test_res_bus((*ready_reg)->latency)) != -1) {
                // 결과 버스의 latency 위치에 비트 설정
                m_result_bus[resbus]->set((*ready_reg)->latency);
                m_fu[n]->issue(issue_inst);
                // OC_EX 레지스터에서 기능 유닛의 dispatch 레지스터로 이동
            } else if (!schedule_wb_now) {
                // ldst_unit: 결과 도착 시간이 비결정적이므로 예약 없이 이슈
                m_fu[n]->issue(issue_inst);
            }
            // else: 결과 버스 충돌 → 구조적 해저드, 이번 사이클 이슈 불가
        }
    }
}
```

### 7.5 shader_core_ctx::writeback() [shader.cc:1930]

```c
void shader_core_ctx::writeback() {
    // 파이프라인 듀티 사이클 계산 (이번 사이클의 IPC)
    unsigned max_committed = m_config->warp_size * m_config->pipe_widths[EX_WB];
    m_stats->m_pipeline_duty_cycle[m_sid] =
        (float)(현재_커밋_수 - 이전_커밋_수) / max_committed;
    // 이전 카운터 갱신
    m_stats->m_last_num_sim_insn[m_sid] = m_stats->m_num_sim_insn[m_sid];

    // EX_WB 레지스터에서 모든 준비된 명령어를 처리
    warp_inst_t **preg = m_pipeline_reg[EX_WB].get_ready();
    warp_inst_t *pipe_reg = (preg == NULL) ? NULL : *preg;

    while (preg and !pipe_reg->empty()) {
        // (a) 오퍼랜드 수집기를 통해 레지스터 파일에 결과 기록
        m_operand_collector.writeback(*pipe_reg);

        // (b) Scoreboard에서 출력 레지스터 해제
        //     → 이 레지스터에 RAW 의존하던 워프가 이제 이슈 가능해짐
        unsigned warp_id = pipe_reg->warp_id();
        m_scoreboard->releaseRegisters(pipe_reg);

        // (c) 파이프라인 내 명령어 카운터 감소
        m_warp[warp_id]->dec_inst_in_pipeline();

        // (d) 명령어 완료 통계 (IPC 카운트, committed 카운트 등)
        warp_inst_complete(*pipe_reg);

        // (e) 시뮬레이션 사이클 추적 갱신
        m_gpu->gpu_sim_insn_last_update_sid = m_sid;
        m_gpu->gpu_sim_insn_last_update = m_gpu->gpu_sim_cycle;

        // (f) 파이프라인 레지스터 클리어
        pipe_reg->clear();

        // 다음 준비된 명령어
        preg = m_pipeline_reg[EX_WB].get_ready();
        pipe_reg = (preg == NULL) ? NULL : *preg;
    }
}
```

### 7.6 scheduler_unit::cycle() [shader.cc:1259] -- 완전 주석

```c
void scheduler_unit::cycle() {
    bool valid_inst = false;   // I-Buffer에 유효한 명령어가 있는 워프 존재?
    bool ready_inst = false;   // 유효 명령어 중 Scoreboard 통과한 것 있나?
    bool issued_inst = false;  // 실제 이슈에 성공했나?

    // ① 워프 우선순위 결정 (파생 클래스에서 오버라이드)
    order_warps();
    // → m_next_cycle_prioritized_warps 리스트가 채워짐

    // ② 우선순위 순서대로 워프 순회
    for (iter = m_next_cycle_prioritized_warps.begin(); ...; iter++) {

        if ((*iter) == NULL || (*iter)->done_exit()) continue;

        unsigned warp_id = (*iter)->get_warp_id();
        unsigned checked = 0;
        unsigned issued = 0;
        exec_unit_type_t previous_issued_inst_exec_type = NONE;
        unsigned max_issue = m_shader->m_config->gpgpu_max_insn_issue_per_warp;
        bool diff_exec_units = m_shader->m_config->gpgpu_dual_issue_diff_exec_units;

        // ③ 하나의 워프에서 최대 max_issue개 명령어 이슈 시도
        while (!warp.waiting() && !warp.ibuffer_empty()
               && (checked < max_issue) && (checked <= issued)
               && (issued < max_issue)) {

            const warp_inst_t *pI = warp.ibuffer_next_inst();

            // CDP(Cooperative Dynamic Parallelism) 지연 처리
            if (pI && pI->m_is_cdp && warp.m_cdp_latency > 0) {
                warp.m_cdp_latency--;
                break;
            }

            bool valid = warp.ibuffer_next_valid();

            // SIMT 스택에서 현재 PC와 재수렴 PC 조회
            unsigned pc, rpc;
            m_shader->get_pdom_stack_top_info(warp_id, pI, &pc, &rpc);

            if (pI) {
                if (pc != pI->pc) {
                    // ④ 컨트롤 해저드: SIMT 스택의 PC와 I-Buffer의 PC가 불일치
                    // → 분기가 발생하여 I-Buffer가 무효화됨
                    warp.set_next_pc(pc);
                    warp.ibuffer_flush();
                } else {
                    valid_inst = true;

                    // ⑤ Scoreboard 체크
                    if (!m_scoreboard->checkCollision(warp_id, pI)) {
                        ready_inst = true;

                        // SIMT 스택에서 활성 마스크 가져오기
                        const active_mask_t &active_mask =
                            m_shader->get_active_mask(warp_id, pI);

                        // ⑥ 명령어 타입별 파이프라인 레지스터에 디스패치
                        if (LOAD/STORE/MEMBAR/TENSOR_LOAD/TENSOR_STORE) {
                            if (m_mem_out->has_free() && !diff_exec 충돌) {
                                m_shader->issue_warp(*m_mem_out, pI, ...);
                                issued++; issued_inst = true;
                                previous_issued_inst_exec_type = MEM;
                            }
                        } else {
                            // SP/INT/DP/SFU/TENSOR/SPEC 중 적절한 유닛 선택
                            // (코드에서 여러 if-else로 분기)
                            // ...
                        }
                    } else {
                        // Scoreboard 충돌 → 이 명령어 이슈 불가
                    }
                }
            }

            if (warp_inst_issued) {
                do_on_warp_issued(warp_id, issued, iter);
                // → ibuffer_step()과 통계 기록
            }
            checked++;
        }

        if (issued) {
            // 이슈 성공 → m_last_supervised_issued 갱신
            m_last_supervised_issued = 해당 supervised_iter;
            m_num_issued_last_cycle = issued;
            if (issued == 1) m_stats->single_issue_nums[m_id]++;
            else if (issued > 1) m_stats->dual_issue_nums[m_id]++;
            break;  // ★ 한 사이클에 하나의 워프에서만 이슈
        }
    }

    // ⑦ 스톨 통계
    if (!valid_inst)   m_stats->shader_cycle_distro[0]++;  // Idle/Control Hazard
    else if (!ready_inst) m_stats->shader_cycle_distro[1]++;  // Scoreboard Stall
    else if (!issued_inst) m_stats->shader_cycle_distro[2]++;  // Pipeline Stall
}
```

---

## 8. 주요 설정 파라미터

| 파라미터 | 설명 | 기본값 (예) |
|----------|------|-------------|
| `n_thread_per_shader` | SM당 최대 스레드 수 | 2048 |
| `warp_size` | 워프 크기 | 32 |
| `max_warps_per_shader` | SM당 최대 워프 수 (= n_threads/warp_size) | 64 |
| `gpgpu_num_sched_per_core` | SM당 워프 스케줄러 수 | 4 |
| `gpgpu_max_insn_issue_per_warp` | 워프당 사이클당 최대 이슈 수 | 2 |
| `gpgpu_dual_issue_diff_exec_units` | 듀얼 이슈 시 다른 실행 유닛 강제 | true |
| `gpgpu_num_sp_units` | SP 유닛 수 | 4 |
| `gpgpu_num_dp_units` | DP 유닛 수 | 0 또는 4 |
| `gpgpu_num_int_units` | INT 유닛 수 | 0 또는 4 |
| `gpgpu_num_sfu_units` | SFU 유닛 수 | 4 |
| `gpgpu_num_tensor_core_units` | 텐서 코어 수 | 0 또는 4 |
| `gpgpu_num_reg_banks` | 레지스터 파일 뱅크 수 | 16 |
| `gpgpu_scheduler_string` | 스케줄러 종류 문자열 | "gto" |
| `inst_fetch_throughput` | 사이클당 페치+디코드 반복 수 | 1 |
| `reg_file_port_throughput` | 사이클당 오퍼랜드 수집기 step 반복 수 | 1 |

---

## 9. 파이프라인 전체 흐름 요약

```
┌─────────────────────────────────────────────────────────────────┐
│                    한 사이클의 실행 순서                           │
│  (역순 실행으로 파이프라인 타이밍 정확성 보장)                       │
│                                                                 │
│  1. writeback()                                                 │
│     EX_WB 레지스터 → 레지스터 파일 기록                           │
│     Scoreboard 해제 → 의존 워프 이슈 허용                        │
│                                                                 │
│  2. execute()                                                   │
│     결과 버스 >>= 1 (시간 진행)                                  │
│     각 FU의 내부 파이프라인 시프트                                 │
│     OC_EX → FU dispatch 레지스터 (can_issue 체크)               │
│     결과 버스 예약 (ALU) 또는 비예약 (LDST)                      │
│                                                                 │
│  3. read_operands()                                             │
│     opndcoll_rfu_t::step():                                     │
│       dispatch_ready_cu() → OC_EX로                             │
│       allocate_reads()    → 뱅크 아비트레이션                     │
│       allocate_cu()       → ID_OC에서 CU로                      │
│       process_banks()     → 뱅크 할당 리셋                       │
│                                                                 │
│  4. issue()                                                     │
│     각 scheduler_unit::cycle():                                 │
│       order_warps() → 우선순위 결정                               │
│       워프 순회 → Scoreboard 체크                                │
│       issue_warp() → ID_OC 레지스터에 명령어 투입                 │
│       func_exec_inst() → PTX 기능 시뮬레이션                     │
│       updateSIMTStack() → 분기 처리                              │
│       scoreboard->reserveRegisters() → 출력 레지스터 잠금         │
│                                                                 │
│  5. decode() + fetch()                                          │
│     fetch(): L1I 캐시 접근 → ifetch_buffer                      │
│     decode(): ifetch_buffer → I-Buffer (2슬롯)                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 10. LDST Unit 상세

ldst_unit은 다른 기능 유닛과 달리 stallable하며, 자체적으로 writeback을 수행한다.

### 10.1 ldst_unit::cycle() [shader.cc:2834]

```
ldst_unit::cycle()
├── writeback()  // LDST 자체 writeback (공유/글로벌 메모리 응답 처리)
│
├── 내부 파이프라인 레지스터 시프트
│
├── [메모리 응답 처리] (m_response_fifo)
│   ├── TEXTURE_ACC_R → L1T 캐시 fill
│   ├── CONST_ACC_R → L1C 캐시 fill
│   ├── WRITE_ACK → store_ack() 호출
│   └── GLOBAL_ACC_R → L1D fill 또는 bypass
│
├── L1T/L1C/L1D 캐시 cycle()
│
└── [디스패치 레지스터 처리] (m_dispatch_reg)
    ├── shared_cycle()  → 공유 메모리 뱅크 충돌 처리
    ├── constant_cycle() → 상수 캐시 접근
    ├── texture_cycle() → 텍스처 캐시 접근
    └── memory_cycle()  → 글로벌/로컬 메모리 접근
        ├── bypassL1D → 인터커넥트로 직접 전송
        └── L1D 접근 → process_memory_access_queue_l1cache()
```

---

## 11. Sub-Core 모델

`sub_core_model` 옵션이 활성화되면, SM 내부를 여러 서브코어로 분할한다.

- 각 스케줄러는 자신만의 ID_OC/OC_EX 레지스터 슬롯을 사용한다
- 오퍼랜드 수집기의 CU도 서브코어별로 할당된다
- 레지스터 뱅크도 서브코어별로 파티셔닝된다

이는 Volta 이후 아키텍처의 서브코어 구조를 모델링하기 위한 것이다.

```c
// register_bank 함수: 서브코어 모델에서 뱅크 번호 계산
unsigned register_bank(int regnum, int wid, unsigned num_banks,
                       bool sub_core_model, unsigned banks_per_sched,
                       unsigned sched_id) {
    if (sub_core_model) {
        // 서브코어별로 뱅크 범위를 분리
        return (regnum % banks_per_sched) + (sched_id * banks_per_sched);
    } else {
        return regnum % num_banks;
    }
}
```
