# drgn 코드 분석 — Program 객체와 메모리 읽기

> 소스: github.com/osandov/drgn @ main(0.2.0+), 로컬 `deep/sources/drgn`
> 관련: [[drgn-codebase-overview]] · [[03-debuginfo-dwarf]] · [[05-object]] · [[06-linux-kernel]] · [[07-stack-arch]]

## 1. 역할 (이 서브시스템이 drgn에서 하는 일)

drgn의 디버깅 세션 하나는 `struct drgn_program` 인스턴스 하나로 표현된다. Python 쪽 `drgn.Program` 객체가 이 C 구조체를 그대로 감싼 것이다. 이 서브시스템은 두 가지를 책임진다.

1. **중심 허브(hub)**: 메모리 리더, 타입 시스템, 디버그 정보(DWARF), 심볼 finder, 플랫폼(arch/ABI), 기본 언어, 스레드/스택 상태를 한 객체에 묶는다. drgn의 거의 모든 API는 첫 인자로 `struct drgn_program *prog`를 받는다.
2. **메모리 추상화**: "타겟의 주소 X에서 N바이트를 읽어라"라는 단 하나의 인터페이스(`drgn_program_read_memory`)를, 백엔드가 코어 덤프든 라이브 커널이든 라이브 프로세스든 QEMU 게스트든 무관하게 제공한다. 백엔드 차이는 모두 "세그먼트 + read 콜백"으로 흡수된다.

핵심 설계 사상: **메모리 소스가 4가지든 10가지든, 위쪽(타입/오브젝트/스택 언와인더)은 `drgn_program_read_memory` 하나만 호출한다.** 소스별 차이는 `drgn_program_set_*()` 시점에 "어떤 read 콜백을 가진 세그먼트를 등록하는가"로 캡슐화된다.

## 2. struct drgn_program — 중심 허브

정의: `libdrgn/program.h:62-314`. 주석 기준으로 그룹화하면 다음과 같다.

| 그룹 | 핵심 필드 (program.h) | 역할 |
|------|----------------------|------|
| 메모리/코어 | `reader` (L68) | `drgn_memory_reader`. 모든 읽기의 진입점 |
| | `file_segments` (L70) | ELF 코어 덤프 / `/proc/pid/mem`의 파일 세그먼트 배열. `drgn_read_memory_file`의 `arg` |
| | `core` (L72) | ELF 코어 덤프 핸들(`Elf*`). 라이브/kdump에서는 NULL |
| | `core_fd` (L74), `core_path` (L76) | 코어/kdump/`/proc/pid/mem` fd 및 경로 |
| | `pid` (L78) | 라이브 유저스페이스 프로세스 PID |
| | `kdump_ctx` (L80) | libkdumpfile 컨텍스트(diskdump/makedumpfile 압축 덤프용) |
| | `qmp_conn` (L82) | QEMU QMP 소켓 연결 상태 |
| 타입 | `type_finders` (L88), `void_types`, `primitive_types`, `dedupe_types`, `created_types`, `members`/`members_cached` | 타입 캐시 + 중복 제거 + 멤버 조회 캐시 → [[03-debuginfo-dwarf]] |
| 디버그 정보 | `object_finders` (L117), `dbinfo` (L118), `symbol_finders` (L119) | DWARF/심볼 인덱스와 finder 핸들러 체인 |
| 프로그램 정보 | `lang` (L125) | 기본 언어. 커널이면 C로 고정(program.c:100) |
| | `platform` (L126), `has_platform` (L132) | arch/엔디안/워드크기/주소비트. `has_platform=false`면 read조차 거부 |
| | `flags` (L133) | `enum drgn_program_flags` — §6 |
| 스레드/스택 | `thread_set` (L144), `main_thread`, `crashed_thread` | TID→스레드 인덱스. 커널 코어에선 PRSTATUS 노트 인덱스용 → [[07-stack-arch]] |
| union (L160-291) | userspace: `auxv` 캐시 / kernel: `vmcoreinfo`, `direct_mapping_offset`, `pgtable` 캐시들 | **IS_LINUX_KERNEL 여부로 갈리는 상호배타 필드를 union으로 절약** |
| 커널 공통 | `vmemmap` (L297), `pgtable_its[2]` (L299), `address_translation_depth` (L304) | 페이지테이블 워킹용 이터레이터 캐시(init/deinit 단순화를 위해 union 밖) |
| 로깅 | `log_fn`, `log_arg`, `progress_file`, `log_level` (L309-313) | 로그/진행률 출력 |

생명주기:
- `drgn_program_init` (program.c:136-152): `memset(0)` 후 `reader`/타입/`dbinfo`/`qmp_conn`/`thread_set` 초기화, `core_fd=-1`, 로그 기본값 설정.
- `drgn_program_create` (no_python.c:19): init를 감싸 힙에 할당(공개 API).
- `drgn_program_deinit` (program.c:154-202): 스레드/페이지테이블 이터레이터/finder/타입/리더 해제, `core`(`elf_end`)·`core_fd`·`kdump_ctx`·`qmp_conn` 정리.

`vmcoreinfo`(program.h:184-254)는 커널 디버깅의 핵심 메타데이터다. `osrelease`, `page_size`, `kaslr_offset`, `swapper_pg_dir`(커널 페이지 글로벌 디렉터리), `va_bits`, `phys_base`, `pgtable_l5_enabled` 등 — 가상↔물리 변환과 심볼 재배치에 전부 쓰인다.

## 3. 메모리 리더 추상화 (세그먼트 모델)

`struct drgn_memory_reader`(memory_reader.h:43-48)는 **두 개의 splay 트리**만 가진다.

```
reader
 ├─ virtual_segments   (가상 주소공간 세그먼트)
 └─ physical_segments  (물리 주소공간 세그먼트)
```

각 세그먼트 `struct drgn_memory_segment`(memory_reader.c:108-123):

```c
struct drgn_memory_segment {
    uint64_t min_address, max_address;   // 현재(절단 후) 주소 범위 [inclusive]
    uint64_t orig_min_address;           // 절단 전 원래 시작 주소
    drgn_memory_read_fn read_fn;         // 이 영역을 읽는 콜백
    void *arg;                           // 콜백에 넘길 컨텍스트
};
```

트리 키는 `min_address`(memory_reader.c:125-133), `search_le`(≤ 검색)로 주소를 포함하는 세그먼트를 O(log n)에 찾는다. `orig_min_address`가 따로 있는 이유: 세그먼트가 겹쳐 등록되어 앞이 잘려도, read 콜백에 넘기는 `offset = address - orig_min_address`(memory_reader.c:368)가 **원래 등록 기준 오프셋**(예: 파일 내 오프셋)을 유지해야 하기 때문이다.

### 등록: `drgn_memory_reader_add_segment` (memory_reader.c:178-342)

`physical` 플래그로 트리를 선택(L186-188)한 뒤, 새 세그먼트가 기존 세그먼트와 겹치면 **잘라내거나(truncate) 훔치거나(steal)** 한다. "나중에 등록한 세그먼트가 우선"(drgn.h:583)이라는 규칙을 트리 일관성을 유지하며 구현한다. 겹침 케이스: ① 새 세그먼트가 기존 안에 완전히 포함 → 꼬리(tail) 분리, ② 같은 시작주소 → 기존을 steal, ③ 앞/뒤 부분 겹침 → `truncate_head`/`truncate_tail`로 경계 조정. 핵심은 **트리 노드 할당이 실패할 수 있으므로, malloc을 모두 끝낸 뒤에야(L325 `insert:`) 트리를 수정**한다는 점.

공개 래퍼 `drgn_program_add_memory_segment` (program.c:204-219): `size==0`이거나 주소가 `address_mask` 초과면 무시, 아니면 `max_address`를 주소 마스크로 클램프해서 리더에 등록.

### 조회/읽기: `drgn_memory_reader_read` (memory_reader.c:344-377)

```c
tree = physical ? &physical_segments : &virtual_segments;   // L351 주소공간 선택
while (count > 0) {
    segment = search_le(tree, &address);                     // 포함 세그먼트 검색
    if (!segment || segment->max_address < address)
        return drgn_error_format_fault(...);                 // 구멍(hole) → FaultError
    n = min(count-1, segment->max_address - address) + 1;    // 세그먼트 경계까지만
    err = segment->read_fn(p, address, n,
                           address - segment->orig_min_address,// offset
                           segment->arg, physical);          // 콜백 위임
    p += n; address += n; count -= n;                        // 다음 세그먼트로 이어읽기
}
```

여러 세그먼트에 걸친 읽기는 자동으로 분할되며, 빈 영역에 닿으면 `FaultError`(drgn의 "잘못된 메모리 접근")로 변환된다. **리더 자체는 최대 주소나 주소 wrap-around 개념이 없다**(memory_reader.h:28-30) — 그건 상위(`drgn_program_read_memory`)가 처리한다.

부가 기능으로 `drgn_program_search_memory_*`(memory_reader.c:699~)가 같은 세그먼트 트리를 순회하며 바이트열/정수/정규식 검색을 제공한다(버퍼링·refill 로직 포함).

## 4. 4가지 메모리 소스 (소스별 등록되는 세그먼트/콜백)

모든 소스는 결국 `drgn_program_add_memory_segment`로 세그먼트를 등록하고, 차이는 **어떤 `read_fn`을 다는가**에 있다.

| 소스 | 설정 함수 | 등록 세그먼트(공간/범위) | read 콜백 | flags |
|------|----------|------------------------|-----------|-------|
| ELF 코어/vmcore | `set_core_dump[_fd]` (program.c:330) | 각 PT_LOAD의 `p_vaddr`(virt) + `p_paddr`(phys) | `drgn_read_memory_file` (pread) | 유저코어=0 / vmcore=LINUX_KERNEL |
| diskdump/kdump | `drgn_program_set_kdump` (kdump.c:134) | [0,2⁶⁴) virt+phys 전체 | `drgn_read_kdump` (libkdumpfile) | LINUX_KERNEL |
| 라이브 커널 | `set_kernel`→`/proc/kcore` (program.c:733) | PT_LOAD + 페이지테이블 폴백 | `drgn_read_memory_file` + `read_memory_via_pgtable` | LINUX_KERNEL\|LIVE\|LOCAL |
| 라이브 프로세스 | `set_pid` (program.c:801) | [0,2⁶⁴) virt 1개 | `drgn_read_memory_file` (`/proc/pid/mem`) | LIVE\|LOCAL |
| QEMU 게스트 | `set_qemu_qmp[_fd]` (qmp.c:842) | [0,2⁶⁴) phys + (선택)virt | `drgn_qmp_read_memory` / `drgn_read_qemu_process_mem` / pgtable | LIVE |

### (a) ELF 코어 덤프 / vmcore — `drgn_program_set_core_dump_fd_internal` (program.c:330-703)

가장 복잡한 경로다. 흐름:
1. `has_kdump_signature`(program.c:308)로 압축 덤프면 `set_kdump`로 위임(L357).
2. libelf로 ELF 헤더 검증(ET_CORE), 플랫폼 자동 감지(`drgn_platform_from_elf`, L381).
3. **1차 패스**(L397-469): PT_LOAD 개수 세고, `p_paddr` 유무 확인, PT_NOTE에서 `NT_TASKSTRUCT`/`NT_ARM_PAC_MASK`/`VMCOREINFO`/`QEMU` 노트 스캔. VMCOREINFO가 있으면 vmcore 또는 `/proc/kcore`로 판단.
4. `NT_TASKSTRUCT` + `fstatfs`가 `PROC_SUPER_MAGIC(0x9fa0)`면 `/proc/kcore`로 확정(L471-487).
5. **페이지테이블 폴백 세그먼트**(L509-522): 커널 코어이고 arch가 `linux_kernel_pgtable_iterator_next`를 지원하면, 가상공간 전체 `[0,UINT64_MAX]`에 `read_memory_via_pgtable` 콜백을 먼저 깐다. → 코어에 안 담긴 가상주소를 페이지테이블 워킹으로 보충.
6. **2차 패스**(L525-591): 각 PT_LOAD를 `drgn_read_memory_file`로 `p_vaddr`(가상)에 등록, `p_paddr` 유효하면 물리에도 등록. `zerofill`/`eio_is_fault`로 "덤프에 없는 바이트" 처리 정책 결정(유저코어는 fault, kdump 계열은 0 채움; L540-571 주석에 근거 상세).
7. **3차 패스**(L599-639): 구버전 `/proc/kcore`(p_paddr=0)는 direct mapping을 역산해 물리 세그먼트 보충.

### (b) 라이브 커널 — `drgn_program_set_kernel` (program.c:733-737)

`set_core_dump(prog, "/proc/kcore")` 한 줄. 즉 라이브 커널은 "특수한 ELF 코어"로 취급되어 (a) 경로를 그대로 탄다. `/proc/kcore` 감지 후 `IS_LINUX_KERNEL|IS_LIVE|IS_LOCAL`를 세우고 `core`를 닫는다(L653-657).

### (c) 라이브 프로세스 — `drgn_program_set_pid` (program.c:801-852)

`/proc/<pid>/mem`을 열고(L814), 플랫폼을 호스트로 고정(L819), **가상공간 전체 `[0,UINT64_MAX]` 단일 세그먼트**를 `drgn_read_memory_file`로 등록(L831). `file_segments[0]`은 `file_offset=0, file_size=UINT64_MAX, eio_is_fault=true`(L826-830) — 즉 파일 오프셋이 곧 가상주소이고, `pread`가 EIO를 내면 `FaultError`로 변환. `IS_LIVE|IS_LOCAL` 설정.

### (d) QEMU QMP — `drgn_program_set_qemu_qmp[_fd]` (qemu_machine_protocol.c:842/865)

`json-c` 빌드일 때만 동작(아니면 program.c:854-869이 NOT_IMPLEMENTED 반환). QMP 소켓 연결→`qmp_negotiate`→`qmp_detect_platform` 후:
- **물리공간 전체** `[0,UINT64_MAX]`에 `drgn_qmp_read_memory`(QEMU `xp` 모니터 명령) 등록(L896).
- UNIX 소켓이면 피어 PID를 얻어 `gpa2hva`로 게스트 물리→호스트 가상 매핑을 만들고, `process_vm_readv` 기반 `drgn_read_qemu_process_mem`(qmp.c:571-598)으로 RAM 영역을 더 빠른 직접 읽기로 덮어쓴다(L920-927).
- VMCOREINFO를 `dump-guest-memory` 핵으로 얻으면(parse_vmcoreinfo_from_dump, L676) `IS_LINUX_KERNEL`을 세우고 pgtable 가상 리더를 추가(L699-704). `IS_LIVE` 설정.

### read 콜백 4종 요약

| 콜백 | 위치 | 동작 |
|------|------|------|
| `drgn_read_memory_file` | memory_reader.c:379-421 | `pread`로 fd에서 읽음. `file_size` 초과분은 `zerofill`이면 0, 아니면 fault. EIO는 `eio_is_fault`에 따라 fault/OS에러 |
| `read_memory_via_pgtable` | linux_kernel.c:45-52 | `linux_helper_read_vm(prog, swapper_pg_dir, addr, ...)` — **소프트웨어 페이지테이블 워킹** |
| `drgn_read_kdump` | kdump.c:111-126 | `kdump_read(ctx, physical?KDUMP_KPHYSADDR:KDUMP_KVADDR, ...)` |
| `drgn_qmp_read_memory` / `drgn_read_qemu_process_mem` | qmp.c | QEMU `xp` 명령 / `process_vm_readv(pid, hva)` |

## 5. 읽기 경로: drgn_program_read_memory 와 주소 변환

`drgn_program_read_memory` (program.c:1877-1900):

```c
err = drgn_program_address_mask(prog, &address_mask);   // 플랫폼 주소비트 마스크
err = drgn_program_untagged_addr(prog, &address);       // ① 태그 비트 제거 + arch 후크
while (count > 0) {
    n = min(count-1, address_mask - address) + 1;        // ② 주소공간 끝까지만 1회분
    err = drgn_memory_reader_read(&prog->reader, p, address, n, physical); // ③ 위임
    p += n; address = 0; count -= n;                     // wrap 시 0부터 이어읽기
}
```

① `drgn_program_untagged_addr`(program.h:433-444): `address &= address_mask`로 상위 미사용 비트를 자르고, arch별 `untagged_addr` 후크가 있으면(예: AArch64 top-byte-ignore / PAC) 추가 정리. 즉 **태그/인증 비트 제거가 read의 첫 단계**다.

**가상→물리 변환은 read_memory 안에 분기가 없다.** 이것이 drgn 설계의 핵심 우아함이다:
- `physical=true` → `drgn_memory_reader_read`가 `physical_segments` 트리를 직접 사용(memory_reader.c:351). 변환 없음.
- `physical=false` → `virtual_segments` 트리 사용. 만약 그 가상 세그먼트의 `read_fn`이 `read_memory_via_pgtable`라면, **변환은 그 콜백 안에서 lazy하게** 일어난다. `read_memory_via_pgtable`(linux_kernel.c:45)이 `linux_helper_read_vm`(helpers.h:27)을 호출 → `swapper_pg_dir`(커널 페이지 글로벌 디렉터리)부터 페이지테이블을 소프트웨어로 워킹해 물리주소를 구하고, 그 물리주소를 다시 물리 세그먼트에서 읽는다.

따라서 "페이지테이블 워킹 후크가 어디서 호출되나"의 답은: **`drgn_program_set_*()`가 가상공간에 등록한 세그먼트의 read 콜백으로서 호출된다.** 코어 덤프에 가상 매핑(PT_LOAD virt 세그먼트)이 이미 있으면 그게 우선이고, 빠진 가상주소만 그 아래 깔린 `[0,2⁶⁴) → read_memory_via_pgtable` catch-all 세그먼트가 받아 워킹한다(등록은 program.c:517, kdump.c:273, qmp.c:700에서 `DRGN_PREFER_PGTABLE_READER` 등 조건부).

페이지테이블 이터레이터 상태는 `prog->pgtable_its[2]`, `address_translation_depth`(program.h:299-304)에 캐시되어 반복 워킹 비용을 줄인다.

## 6. ProgramFlags 분기

정의: `enum drgn_program_flags`(drgn.h:533-540), Python `ProgramFlags`(_drgn.pyi:1521).

| 플래그 | 값 | 의미 |
|--------|-----|------|
| `IS_LINUX_KERNEL` | 1<<0 | 타겟이 리눅스 커널 |
| `IS_LIVE` | 1<<1 | 실행 중(코어 덤프 아님) |
| `IS_LOCAL` | 1<<2 | 로컬 머신에서 실행 중 |

플래그 조합으로 타겟 종류를 식별(program.h):
- **유저스페이스 프로세스** = `LIVE|LOCAL` (LINUX_KERNEL 없음) — `drgn_program_is_userspace_process`(program.h:367-374). `/proc/pid/auxv` 직접 읽기 가능 등.
- **유저스페이스 코어** = LINUX_KERNEL·LIVE 둘 다 없고 `core!=NULL` — `drgn_program_is_userspace_core`(program.h:377-383). auxv는 `NT_AUXV` 노트에서 파싱.
- **vmcore(ELF 커널 덤프)** = `LINUX_KERNEL`만.
- **라이브 커널(/proc/kcore)** = `LINUX_KERNEL|LIVE|LOCAL`.
- **QEMU 게스트** = `LIVE`(로컬 아님, 변환은 게스트 커널 기준).

플래그가 동작을 가르는 지점:
- **언어 기본값**: `IS_LINUX_KERNEL`이면 기본 언어를 무조건 C로(program.c:100-103). 아니면 `main()`에서 추론.
- **union 필드 선택**: `IS_LINUX_KERNEL` 여부로 `auxv`(유저) vs `vmcoreinfo`/페이지테이블 캐시(커널)가 갈린다(program.h:160-291).
- **메모리 검색 핵**: `drgn_memory_search_iterator_replace_linux_kernel_address_range`(memory_reader.c:1424-1461)는 `IS_LINUX_KERNEL`일 때 direct mapping 영역 검색을 물리 검색으로 바꿔 주소 변환 비용을 우회(결과는 가상주소로 반환).
- **deinit 분기**: 유저코어면 `main_thread`/`crashed_thread`가 `thread_set` 안에 있어 별도 해제 안 함(program.c:157-165).
- **auxv 캐시 경로**: 유저 프로세스면 `/proc/pid/auxv` 파일, 유저 코어면 `NT_AUXV` 노트(program.c:884-901).

`drgn_program_check_initialized`(program.c:299-307): `core_fd!=-1`이거나 리더에 세그먼트가 있으면 "이미 초기화됨" 에러. → **메모리 소스는 한 번만 설정 가능**(set_core_dump/set_pid/set_kernel/set_qemu_qmp 상호 배타).

## 7. 타 서브시스템 연결

- **플랫폼/아키텍처([[07-stack-arch]])**: `prog->platform.arch`가 `untagged_addr`, `linux_kernel_pgtable_iterator_next/destroy`, `linux_kernel_live_direct_mapping_fallback` 등 arch별 후크를 제공. 메모리 읽기와 주소 변환의 arch 의존부가 전부 여기로 위임된다. `has_platform=false`면 read·search·바이트스왑 전부 거부.
- **DWARF/디버그 정보([[03-debuginfo-dwarf]])**: `prog->dbinfo`, `*_finders`. `drgn_program_init_*`는 메모리 설정 직후 `drgn_program_load_debug_info`(program.c:1764 등)를 호출해 심볼/타입 인덱싱까지 끝낸다.
- **리눅스 커널 헬퍼([[06-linux-kernel]])**: `read_memory_via_pgtable`→`linux_helper_read_vm`, `linux_helper_direct_mapping_offset`. vmcoreinfo의 `swapper_pg_dir`/`kaslr_offset`/`direct_mapping_offset`이 변환의 입력.
- **오브젝트([[05-object]])**: `drgn_object_read_bytes`로 값을 읽어 `drgn_search_memory_for_object`(memory_reader.c:1091)가 메모리 검색에 사용. 상위 오브젝트 평가는 결국 `drgn_program_read_memory`로 귀결.
- **스레드/스택([[07-stack-arch]])**: `thread_set`, PRSTATUS 노트(`drgn_program_find_prstatus`, program.h:460), `aarch64_insn_pac_mask`가 스택 언와인딩 레지스터 복원에 쓰임.
- **저수준 I/O(io.c)**: `read_all`/`pread_all`(부분 읽기 방지 래퍼)과 `fd_canonical_path`(`/proc/self/fd`로 코어 경로 정규화, program.c:348에서 사용).

## 8. 파일 맵 (담당 파일별 1줄 요약)

| 파일 | 요약 |
|------|------|
| `libdrgn/program.h` | `struct drgn_program` 정의(허브 필드 전체)와 init/deinit, 플래그 판별 inline 함수들 |
| `libdrgn/program.c` | Program 생명주기, 4개 메모리 소스 설정 함수, `read_memory`/`read_u*`/`read_c_string`, vmcoreinfo·스레드 캐시 |
| `libdrgn/memory_reader.h` | 리더 인터페이스: 가상/물리 세그먼트 트리, `add_segment`/`read`, 파일 세그먼트 구조체 |
| `libdrgn/memory_reader.c` | 세그먼트 splay 트리 구현(겹침 절단/steal), `drgn_read_memory_file`, 메모리 검색(memmem/정수/PCRE2) |
| `libdrgn/io.c` / `io.h` | `read_all`/`write_all`/`pread_all` 부분 I/O 방지 래퍼, `fd_canonical_path` |
| `libdrgn/drgn.h` | 공개 API: `enum drgn_program_flags`, `drgn_memory_read_fn` typedef, `add_memory_segment`/`read_memory` 시그니처 |
| (보조) `linux_kernel.c:45` | `read_memory_via_pgtable` — 가상→물리 변환을 수행하는 read 콜백 |
| (보조) `kdump.c:111/134` | `drgn_read_kdump` 콜백 및 libkdumpfile 기반 `set_kdump` |
| (보조) `qemu_machine_protocol.c` | QMP `xp`/`process_vm_readv` 읽기 콜백 및 `set_qemu_qmp` |
| (보조) `_drgn.pyi:428-579` | Python `Program.read/read_u*/add_memory_segment` 시그니처와 FaultError 계약 |
