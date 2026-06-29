# drgn 코드 분석 — 스택 언와인딩 · 아키텍처 · QEMU QMP
> 소스: osandov/drgn (main), 로컬 `deep/sources/drgn` — 인용은 `파일:라인`
> 관련: [[drgn-codebase-overview]] · [[02-program-memory]] · [[06-linux-kernel]]

## 1. 역할

이 묶음은 "레지스터 상태에서 출발해 스택 프레임을 한 칸씩 거슬러 올라가는" 언와인딩
엔진과, 그 엔진이 의존하는 **아키텍처 추상화 계층**, 그리고 살아있는 QEMU VM에서 게스트
메모리를 읽어오는 **QMP 메모리 소스**로 구성된다.

- `stack_trace.c` — 언와인딩 오케스트레이터. 초기 레지스터 획득 → CFI로 한 프레임씩 복원 →
  DWARF 스코프와 결합해 인라인 프레임까지 분리. (1665줄)
- `cfi.c/.h` — CFA(Canonical Frame Address)와 각 레지스터의 복원 규칙(`drgn_cfi_rule`)을
  담는 아키텍처 중립 자료구조. DWARF·ORC 둘 다 이 형식으로 번역된다.
- `orc_info.c`/`orc.h` — 리눅스 커널 전용 ORC 언와인더(`.orc_unwind*` 섹션 파싱·정렬·버전 업그레이드).
- `register_state.c/.h` — 한 프레임의 레지스터 값 버퍼 + "알려짐" 비트셋 추상화.
- `arch_x86_64.c` + `arch_x86_64_defs.py` — x86-64 아키텍처 후크 구현과 레지스터 정의(코드생성 입력).
- `platform.c/.h` — `drgn_architecture_info`(후크 테이블)와 `drgn_platform`(arch+flags) 정의.
- `qemu_machine_protocol.c` — 실행 중 QEMU에 QMP 소켓으로 붙어 게스트 메모리를 읽는 메모리 세그먼트 등록.

전체 호출 그래프에서 이 계층은 "프로그램 메모리 읽기"(memory_reader) 위, "객체/타입
시스템" 아래에 놓인다. `prog.stack_trace(tid)` 한 줄이 이 코드 전체를 관통한다.

---

## 2. 스택 언와인딩 큰 그림 (register state → frame 거슬러오르기)

진입점은 세 갈래지만 모두 `drgn_get_stack_trace()`로 수렴한다
(`stack_trace.c:1282`):

- `drgn_object_stack_trace(obj)` — 정수면 TID로, `task_struct*`/`pt_regs*`면 객체로 (`:1392`).
- `drgn_thread_stack_trace(thread)` — 커널이면 thread 객체, 코어덤프면 prstatus 전달 (`:1412`).
- `drgn_program_stack_trace(prog, tid)` — TID 직접 (`:1354`).

핵심 루프 (`drgn_get_stack_trace`, `stack_trace.c:1304-1340`):

```
초기 레지스터 획득 (regs)
for i in 0..1024:                      # 무한 루프 방지용 상한
    drgn_stack_trace_add_frames(regs)  # 현재 PC에 해당하는 DWARF 프레임들 추가
    regs' = drgn_unwind_with_cfi(regs) # CFI로 호출자 레지스터 복원
    if drgn_not_found:                 # CFI 없음 → arch 폴백
        regs' = bad_call_unwind | fallback_unwind(regs)
    if drgn_stop: break                # 스택 바닥 도달
    regs = regs'
```

### 2-1. 초기 레지스터 획득 (`drgn_get_initial_registers`, `:818`)

언와인딩의 출발점은 "0번 프레임의 레지스터"다. 소스에 따라 분기한다.

- **`pt_regs`/`task_struct` 객체** → `arch->pt_regs_get_initial_registers` 또는
  `linux_kernel_get_initial_registers` (`:838`, `:896`).
- **실행 중 커널에서 CPU에 올라간 태스크** → "running task는 언와인드 불가" 에러
  (`:879`). 코어덤프면 `drgn_get_initial_registers_from_kernel_core_dump`로 CPU별
  PRSTATUS를 찾는다(`:714`). kdump면 `crash_notes` per-CPU 변수에서, 그 외(하이퍼바이저
  덤프)면 PID=CPU+1 규약으로 PRSTATUS를 찾는 정교한 분기가 있다.
- **유저스페이스 코어덤프** → `drgn_program_find_prstatus(tid)` → `prstatus_get_initial_registers` (`:902`).

### 2-2. PC → DWARF 프레임 분리 (`drgn_stack_trace_add_frames`, `:946`)

복원된 레지스터 하나는 물리적으로는 한 프레임이지만, **인라이닝** 때문에 논리적으로는
여러 프레임일 수 있다. 이 함수가 PC로 DWARF 스코프 배열을 얻어
(`drgn_module_find_dwarf_scopes`, `:963`) 뒤에서부터 훑으며
`DW_TAG_inlined_subroutine`마다 별도 `drgn_stack_frame`을 만들고, 마지막에 실제
`DW_TAG_subprogram` 프레임을 추가한다(`:991-1045`). 인라인 프레임들은 같은 `regs`
포인터를 공유하므로 `drgn_stack_frame_source`가 이를 보고 호출 지점(`DW_AT_call_line`)을
복원한다(`:373`). PC는 `pc - !interrupted`로 보정하는데, 일반 호출 프레임의 반환주소는
"다음 명령"을 가리키므로 1을 빼 호출 명령 위치로 되돌리고, 시그널로 중단된 프레임
(`interrupted`)은 그대로 둔다.

### 2-3. 한 프레임 복원 (`drgn_unwind_with_cfi`, `:1192`)

1. `drgn_module_find_cfi`로 현재 PC의 CFI row를 얻는다(아래 §3).
2. `drgn_unwind_cfa` — CFA 규칙을 평가해 이 프레임의 CFA를 확정(`:1168`). CFA는 보통
   "호출자의 rsp"이며 다른 레지스터 복원의 기준점이다.
3. 각 레지스터에 대해 `drgn_unwind_one_register`로 규칙을 평가해 **호출자(상위 프레임)의
   레지스터 값**을 새 `register_state`에 채운다(`:1229-1243`). 규칙 종류는
   `cfi.h`의 `enum drgn_cfi_rule_kind`: `AT_CFA_PLUS_OFFSET`(스택에 저장됨),
   `REGISTER_PLUS_OFFSET`(같은 값/오프셋), `DWARF_EXPRESSION`(임의 식) 등.
4. 반환주소 레지스터(`ret_addr_regno`, x86-64는 rip)에서 호출자 PC를 세팅(`:1251`).
   이 PC가 다음 반복의 출발점이 되어 "거슬러 올라가기"가 성립한다.
5. 복원된 레지스터가 하나도 없거나(`num_regs==0`) ret_addr 규칙이 UNDEFINED면 `drgn_stop`
   → 스택 바닥으로 간주.

메모리 읽기 실패(`DRGN_ERROR_FAULT`)는 하드 실패가 아니라 "해당 레지스터 모름"으로
강등된다(`drgn_unwind_one_register:1163`). CFI 자체가 없으면 `drgn_not_found`를 반환하고,
호출 루프가 arch 폴백(프레임 포인터/단일 call 되돌리기)으로 넘어간다.

### 2-4. PC 목록만으로 만들기 (`stack_trace_from_pcs`, `:1361`)

`drgn_program_stack_trace_from_pcs`는 언와인딩을 하지 않는다. 외부에서 받은 PC 배열
각각에 대해 `register_state`(레지스터 0개, PC만)를 만들고 `add_frames`로 DWARF 스코프만
붙인다. perf/eBPF가 이미 캡처한 PC를 심볼라이즈할 때 쓴다. 같은 메커니즘을
`drgn_program_source_location`/`addr2line`이 재사용한다(`:1507`, "가짜" source_location_list 캐스팅, `:1423`).

---

## 3. 두 언와인더: DWARF CFI vs 커널 ORC

`drgn_module_find_cfi`(debug_info.c:5736)가 둘을 통합한다. 둘 다 결과를 동일한
`drgn_cfi_row`로 내놓으므로 §2의 복원 로직은 출처를 모른다.

| 항목 | DWARF CFI (`.eh_frame`/`.debug_frame`) | 커널 ORC (`.orc_unwind*`) |
|------|----------------------------------------|---------------------------|
| 정의 위치 | `dwarf_info.c` (`drgn_module_find_dwarf_cfi`, `:7720`) | `orc_info.c` (`drgn_module_find_orc_cfi`, `:749`) |
| 표현력 | 임의 DWARF 식까지(완전한 튜링 머신급 바이트코드) | SP/BP 기준 + 소수 규칙 (`drgn_orc_to_cfi_x86_64`, arch_x86_64.c:39) |
| 엔트리당 크기 | FDE/CIE 가변, 큼 | 6바이트 고정(sp_offset/bp_offset/flags, `orc.h:40`) |
| 조회 | FDE 이진탐색 | pc_offset 배열 이진탐색(`drgn_orc_pc`, `:743`) |
| 적용 범위 | 유저스페이스 + 디버그파일 있는 커널 | 리눅스 커널 x86-64 전용(`drgn_module_parse_orc`가 arch로 게이트, `:567`) |
| 데이터 출처 | ELF 디버그 섹션 | 디버그파일 섹션 **또는** built-in(살아있는 커널 메모리에서 `__start_orc_*` 심볼로 읽기, `:439`) |
| 버전 | DWARF 표준 안정 | v1~v4까지 포맷 상이 → v4로 정규화(`:643-719`) |

### 왜 커널은 ORC인가

리눅스 커널은 `.eh_frame`을 **명시적으로 비활성화**한다(코드 주석 `orc_info.c:129`).
프레임 포인터(`-fno-omit-frame-pointer`)는 레지스터 한 개와 push/pop 비용을 상시 부과하고,
DWARF CFI는 부팅 시 모두 메모리에 상주시키기엔 너무 크고 파싱이 무겁다. ORC는 objtool이
빌드타임에 생성하는 6바이트 고정 엔트리라 커널에 내장해도 가볍고, 인터럽트/예외/엔트리
어셈블리처럼 컴파일러 CFI가 부정확한 경로도 objtool이 손으로 검증해 정확하다.

### 선택 기준 (`drgn_module_find_cfi`, debug_info.c:5755-5833)

디버그파일이 있으면 ORC·DWARF 둘 다 파싱해두고, **PC별로** 누구를 먼저 쓸지 정한다:

1. `drgn_module_should_prefer_orc_cfi(module, pc)` — 이 PC가 "ORC 우선 구간"이면 ORC 먼저
   시도(`:5783`). 우선 구간은 `ORC_REG_SP_INDIRECT`를 쓰는 엔트리, 즉 커널의
   `call_on_irqstack()` 류 **스택 스위칭** 패턴이다. 인라인 asm으로 rsp를 바꿔치기해
   DWARF CFI가 틀어지므로 ORC로만 풀어야 한다(`orc_info.c:64-80`, `remove_fdes_from_orc`가
   이 구간을 `module->orc.preferred`에 등록).
2. 평시에는 **DWARF 먼저**(`:5790`) → 없으면 eh_frame(`:5805`) → 그래도 없으면 ORC(`:5812`).
3. 디버그파일이 아예 없으면 built-in ORC만 사용(`:5820-5832`).

핵심 최적화: `remove_fdes_from_orc`(`:131`)가 **DWARF FDE로 완전히 덮이는 ORC 엔트리를
버린다**. ORC 대부분은 DWARF와 중복이므로 저장·이진탐색 비용을 줄인다. 환경변수
`DRGN_PREFER_ORC_UNWINDER=1`이면 전 구간을 ORC 우선으로 강제한다(`:136`).

### ORC 파싱 디테일 (`orc_info.c`)

- 섹션 읽기 `drgn_read_orc_sections`(`:288`): `.orc_unwind_ip`(int32 PC 오프셋),
  `.orc_unwind`(엔트리), `.orc_header`(20바이트 버전 해시) 수집.
- 버전 판별: `.orc_header` 해시 매칭(`orc_version_from_header`, `:224`) 우선, 없으면
  커널 osrelease로 추정(`:258`, RHEL 5.14.el9 백포트까지 특수처리).
- 정렬+중복제거: v5.6부터 vmlinux는 이미 정렬돼 있어 필요할 때만 `qsort`(`:618`).
  동일 인접 엔트리(터미네이터)는 병합(`keep_orc_entry`, `:103`).
- 버전 업그레이드(`:654-717`): v1~v3의 flags 비트(type/signal/end)와 레지스터 번호를
  v4 레이아웃으로 리매핑(`reg_map[]`). 이후 게터(`orc.h:82-100`)는 v4만 가정.
- built-in ORC(`copy_builtin_orc_buffers`, `:388`): vmlinux는 `__start_orc_unwind*`
  심볼로, 모듈은 `mod->arch.num_orcs` + 섹션 주소로 게스트 메모리에서 직접 읽는다.

`drgn_orc_to_cfi_x86_64`(arch_x86_64.c:39)가 6바이트 ORC를 `drgn_cfi_row`로 번역한다.
`sp_reg`/`sp_offset`로 CFA 규칙을, `type`(CALL/REGS/REGS_PARTIAL)로 rip·rsp·rbp 등의
복원 규칙을 세운다. `REGS` 타입은 interrupt/exception 진입으로 전체 pt_regs가
스택에 있는 경우라 18개 레지스터를 CFA 오프셋으로 복원한다(`:129-150`).

---

## 4. register_state 추상화 (`register_state.h/.c`)

`drgn_register_state`는 "한 프레임의 레지스터 스냅샷"이다(`register_state.h:49`):

```c
struct drgn_register_state {
    struct drgn_module *module;  // 이 PC가 속한 모듈(캐시). PC 모르면 NULL
    uint32_t regs_size;          // buf 안 레지스터 값 영역 바이트 수
    uint16_t num_regs;           // 할당된 레지스터 개수
    bool interrupted;            // 시그널/인터럽트로 끊긴 프레임인가
    uint64_t _pc;                // 프로그램 카운터 (get_pc로 접근)
    uint64_t _cfa;               // Canonical Frame Address (get_cfa로 접근)
    unsigned char buf[];         // [레지스터 값들][알려짐 비트셋]
};
```

핵심 설계 두 가지:

1. **값 + "알려짐" 비트셋 분리.** `buf` 뒤에 비트셋이 붙어, 비트 0=PC 알려짐, 비트 1=CFA
   알려짐, 비트 2+ = 각 레지스터 알려짐(`register_state.c:73-103`). 언와인딩은 본질적으로
   부분 정보이므로(어떤 레지스터는 복원 못 함) "0인지"와 "모르는지"를 구분해야 한다.
   `drgn_register_state_has_register`가 이 비트를 본다.

2. **레이아웃은 arch가 결정, 접근은 매크로로.** `buf` 안 레지스터 배치는
   `struct drgn_arch_register_layout`(코드생성)로 정해지고,
   `DRGN_REGISTER_OFFSET(rip)`/`DRGN_REGISTER_SIZE(rip)` 매크로가 `offsetof`/`sizeof`로
   풀린다(`register_state.h:100-114`). 그래서 arch 코드는
   `drgn_register_state_set_from_buffer(regs, rip, &frame[1])`처럼 레지스터를 **이름으로**
   다룬다. 내부적으론 `..._impl(regno, offset, size, ...)`가 실제 일을 한다.

값은 항상 LSB 기준으로 복사(`copy_lsbytes`)하며 타깃 엔디안과 호스트 엔디안을 양쪽에서
보정한다(`register_state.c:140-155`). `set_pc`는 주소 마스크를 적용하고
(`drgn_platform_address_mask`) 곧바로 `drgn_module_find_by_address`로 모듈을 캐시한다
(`:114-122`) — 이래서 프레임마다 모듈 재조회가 없다. 생성은
`drgn_register_state_create(last_reg, interrupted)`로 "마지막 레지스터까지 담을 크기"만
할당한다(`register_state.h:128`).

---

## 5. 아키텍처/플랫폼 후크 + `arch_*_defs.py` 코드생성

### 5-1. 두 단계 추상화 (`platform.h`)

- `struct drgn_platform` = `{ arch, flags }` (`platform.h:517`). flags는
  64비트 여부·엔디안 등. `drgn_platform_address_size/mask`, `is_little_endian` 같은
  순수함수들이 여기서 파생된다.
- `struct drgn_architecture_info`(`:267`) = arch별 **후크 테이블(vtable)**. 외부에는
  `drgn_platform`만 노출되고, 내부 동작은 거의 다 이 구조체의 함수 포인터로 분기한다.

`drgn_host_platform`(platform.c:23)은 컴파일 `#if`로 호스트 arch를 고른다.
`drgn_platform_from_elf`(`:151`)는 ELF e_machine으로, `qmp_detect_platform`은
QMP 문자열로(§6) arch를 고른다.

### 5-2. `drgn_architecture_info`가 제공하는 후크 (platform.h:267-498)

| 분류 | 후크 | 역할 |
|------|------|------|
| 레지스터 정의 | `registers`, `register_layout`, `dwarf_regno_to_internal`, `register_by_name`, `stack_pointer_regno` | 코드생성으로 채움(`DRGN_ARCHITECTURE_REGISTERS`) |
| CFI 기본값 | `default_dwarf_cfi_row` | psABI 기본 규칙(arch_x86_64.c:25) |
| 언와인딩 폴백 | `fallback_unwind`, `bad_call_unwind`, `demangle_cfi_registers` | CFI 실패 시 프레임포인터/단일 call 되돌리기 |
| 초기 레지스터 | `pt_regs_get_initial_registers`, `prstatus_get_initial_registers`, `linux_kernel_get_initial_registers` | 소스별 0번 프레임 구성 |
| 재배치 | `apply_elf_reloc` | 커널 모듈/디버그섹션 재배치 |
| 페이지테이블 | `linux_kernel_pgtable_iterator_{create,destroy,init,next}` | 가상→물리 주소 변환 |
| 직접매핑/메모리 | `linux_kernel_direct_mapping_offset`, `*_section_size_bits_fallback`, `*_max_physmem_bits_fallback` | SPARSEMEM/직접매핑 상수 |
| 주소 정규화 | `untagged_addr` | CPU 태그 비트 제거(예: AArch64 TBI) |

x86-64 구현(`arch_x86_64.c:709`)이 이 표를 모두 채운다. 페이지테이블 이터레이터
(`linux_kernel_pgtable_iterator_next_x86_64`, `:602`)는 4/5레벨 페이지워크를 캐시
배열로 수행하고, huge page(PSE)·non-canonical 영역·`swapper_pg_dir` 직접해소까지 처리한다.
`fallback_unwind_x86_64`(`:317`)는 rbp 프레임포인터 체인을 따라가되, 커널에서 rbp의
LSB가 1이면 "encoded pt_regs"로 해석한다.

### 5-3. 코드생성 파이프라인 (`arch_*_defs.py` → `arch_*.inc`)

`arch_x86_64_defs.py`는 순수 데이터다: `REGISTERS`(API 노출 이름),
`REGISTER_LAYOUT`(저장 순서+크기+DWARF 번호), `STACK_POINTER_REGISTER`.
저장 순서가 의미를 갖는다 — rip/rsp를 맨 앞에(가장 자주 복원), 그다음 콜리세이브드
레지스터를 **pt_regs와 같은 순서**로 둬서 PRSTATUS/pt_regs를 한 번에 memcpy할 수 있게 한다
(`arch_x86_64_defs.py:37`, arch_x86_64.c:301의 `set_range_from_buffer`).

`build-aux/gen_arch_inc_strswitch.py`가 이 파일을 `runpy`로 읽어
(`gen_arch_inc_strswitch.py:10-56`):
1. `DRGN_REGISTER_NUMBER__rip` 같은 내부 번호 enum,
2. `struct drgn_arch_register_layout`(offsetof/sizeof의 근거),
3. `register_layout[]`/`registers[]` 배열,
4. `register_by_name()` 문자열 스위치,
5. `dwarf_regno_to_internal()` 매핑,
6. 이들을 묶는 `DRGN_ARCHITECTURE_REGISTERS` 매크로
를 생성한다. 결과 `arch_x86_64_defs.inc`를 `arch_x86_64.c:19`가 `#include`하고,
`.arch_info_x86_64 = { ..., DRGN_ARCHITECTURE_REGISTERS, ... }`로 후크 테이블에 끼운다.
이 덕에 §2~§4의 모든 레지스터 접근이 "이름 → offset/size"로 컴파일타임에 풀린다.

---

## 6. QEMU QMP 메모리 소스 (`qemu_machine_protocol.c`)

drgn이 **실행 중인 QEMU VM**에 붙어 게스트 물리/가상 메모리를 코어덤프처럼 읽게 해주는
메모리 소스다. 진입점 `drgn_program_set_qemu_qmp(prog, address)`(`:842`)는 주소 형식으로
Unix 소켓/TCP를 구분해 연결하고 `..._fd`(`:865`)로 넘어간다.

설정 시퀀스(`drgn_program_set_qemu_qmp_fd`, `:865-943`):

1. **QMP 핸드셰이크** — greeting 수신 후 `qmp_capabilities`로 커맨드 모드 진입(`qmp_negotiate`, `:209`).
   메시지는 줄 단위 JSON, async `event`는 건너뛴다(`qmp_recv_msg`, `:56-104`).
2. **플랫폼 감지** — `query-target`의 `arch` 문자열로 `drgn_architecture_info` 선택
   (`qmp_detect_platform`, `:227`). 코어덤프 대신 살아있는 VM이라 ELF 헤더가 없으니 이 경로가 필요.
3. **기본 메모리 세그먼트 등록** — `[0, UINT64_MAX]`를 `drgn_qmp_read_memory`로 등록(`:896`).
   이게 모든 물리 메모리 읽기의 폴백이다. `IS_LIVE` 플래그를 켠다.
4. **VMCOREINFO 획득**(Unix 소켓일 때) — QMP엔 직접 읽는 명령이 없어,
   `dump-guest-memory`로 길이 1짜리 초소형 코어덤프를 memfd로 받아 그 안의 VMCOREINFO
   노트를 파싱한다(`qmp_read_vmcoreinfo`, `:727`). fd는 SCM_RIGHTS로 전달(`qmp_send_fd`, `:163`).
   파싱되면 `IS_LINUX_KERNEL`을 켜고 페이지테이블 기반 가상주소 변환 세그먼트도 추가(`:699`).

### 두 가지 메모리 읽기 경로

- **느린 경로: QEMU monitor `xp`** (`drgn_qmp_read_memory`, `:325`).
  `human-monitor-command`로 `xp /Nxb 0xADDR`을 실행해 16진 덤프 문자열을 받아
  `parse_qemu_xp`(`:273`)로 바이트 복원. 한 번에 최대 1024바이트. "Cannot access
  memory"는 fault 에러로 번역한다. 항상 동작하지만 명령/파싱 왕복이 비싸다.
- **빠른 경로: `process_vm_readv` 직접 읽기** (`drgn_read_qemu_process_mem`, `:571`).
  QEMU가 로컬 프로세스(SO_PEERCRED로 PID 확인, `qmp_get_peer_pid`, `:371`)면,
  `info mtree -f`로 게스트 RAM 영역 목록을 얻고(`qmp_get_ram_ranges`, `:394`),
  각 GPA를 `gpa2hva`로 호스트 가상주소에 매핑(`qmp_gpa2hva`, `:492`)한 뒤
  그 hva 범위를 `process_vm_readv`로 직접 읽는 세그먼트로 등록한다
  (`qmp_setup_process_mem`, `:608`). 이러면 GPA→HVA가 한 번 정해진 뒤로는 QMP 왕복 없이
  커널 syscall 한 번으로 읽어 훨씬 빠르다. 실패하면 조용히 xp 폴백.

### 메모리 리더로서의 위치

QMP는 `drgn_program_add_memory_segment`로 등록되는 **여러 메모리 소스 중 하나**다
([[02-program-memory]]의 `memory_reader`). 우선순위: process_vm_readv 세그먼트(있으면) →
pgtable 가상주소 변환 세그먼트(커널) → 전역 xp 세그먼트. §2의 언와인더가
`drgn_program_read_memory`를 부르면 이 스택이 투명하게 게스트 메모리를 돌려준다.
즉 QMP는 "살아있는 VM을 코어덤프처럼 보이게" 만드는 어댑터다.

---

## 7. 타 서브시스템 연결

- **memory_reader** ([[02-program-memory]]) — 언와인딩의 모든 메모리 접근
  (`drgn_program_read_memory`)과 QMP 세그먼트가 여기로 모인다. fault 강등 정책이 언와인딩
  견고성의 핵심.
- **debug_info / DWARF** — `drgn_module_find_cfi`·`find_dwarf_scopes`가 ELF 디버그섹션을
  파싱(dwarf_info.c). CFI row와 인라인 스코프 모두 여기서 온다.
- **linux_kernel 헬퍼** ([[06-linux-kernel]]) — `linux_helper_find_task`,
  `task_on_cpu`, `per_cpu_ptr`, `direct_mapping_offset` 등이 커널 초기 레지스터·CPU
  매핑에 쓰인다. ORC built-in 읽기·페이지테이블 변환도 커널 전용.
- **object/type 시스템** — `drgn_stack_frame_find_object`(`stack_trace.c:527`)가
  복원된 레지스터 + DWARF 위치식으로 지역변수 객체를 만든다. 언와인딩이 단순 PC 나열이
  아니라 "프레임별 변수 검사"까지 가능한 이유.
- **symbol** — 프레임 포맷팅(`drgn_format_stack_trace`)이 PC를 심볼+오프셋으로 변환.

---

## 8. 파일 맵

| 파일 | 라인 | 핵심 내용 |
|------|------|-----------|
| `libdrgn/stack_trace.c` | 1665 | 언와인딩 루프(`:1282`), 프레임 분리(`:946`), CFI 적용(`:1086`,`:1192`), 초기 레지스터(`:818`), 포맷팅/지역변수 |
| `libdrgn/stack_trace.h` | 52 | `drgn_stack_frame`/`drgn_stack_trace` 구조체 |
| `libdrgn/cfi.h` | 232 | `drgn_cfi_rule_kind`(규칙 8종), `drgn_cfi_rule`, `drgn_cfi_row` |
| `libdrgn/cfi.c` | 107 | CFI row 복사/예약/get·set (정적→힙 승격) |
| `libdrgn/orc_info.c` | 768 | ORC 섹션 파싱·정렬·FDE중복제거(`:131`)·버전업그레이드(`:643`)·조회(`:749`) |
| `libdrgn/orc.h` | 103 | `drgn_orc_entry`(6B), 타입/레지스터 enum, flags 게터 |
| `libdrgn/register_state.c` | 156 | 레지스터 버퍼+비트셋 구현, PC/CFA get·set |
| `libdrgn/register_state.h` | 356 | `drgn_register_state` 구조체, 이름기반 접근 매크로 |
| `libdrgn/arch_x86_64.c` | 740 | x86-64 후크 전부: ORC→CFI(`:39`), 폴백(`:317`), pt_regs/prstatus, 페이지워크(`:602`) |
| `libdrgn/arch_x86_64_defs.py` | 70 | 레지스터 이름·레이아웃·DWARF번호(코드생성 입력) |
| `libdrgn/platform.c` | 250 | `drgn_host_platform`, ELF/arch에서 플랫폼 생성, reloc add 함수 |
| `libdrgn/platform.h` | 573 | `drgn_architecture_info` 후크 테이블, `drgn_platform`, pgtable_iterator |
| `libdrgn/qemu_machine_protocol.c` | 957 | QMP 핸드셰이크·플랫폼감지·xp/process_vm_readv 메모리·VMCOREINFO |
| `libdrgn/debug_info.c` | `:5736` | `drgn_module_find_cfi` — ORC vs DWARF 선택 디스패치 |
| `libdrgn/build-aux/gen_arch_inc_strswitch.py` | — | `arch_*_defs.py` → `arch_*.inc` 코드생성 |
