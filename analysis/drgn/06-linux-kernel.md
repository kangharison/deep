# drgn 코드 분석 — Linux 커널 지원 (vmcoreinfo/kcore/kdump/모듈/kallsyms)

> 소스: osandov/drgn (main) — 로컬 `deep/sources/drgn`
> 관련: [[drgn-codebase-overview]] · [[02-program-memory]] · [[03-debuginfo-dwarf]] · [[07-stack-arch]]
> 분석 대상: `libdrgn/linux_kernel.c`(2762줄), `linux_kernel.h`, `linux_kernel_helpers.c`(1063줄), `kallsyms.c`(714줄), `kdump.c`(365줄), `drgn_program_parse_vmcoreinfo.inc.strswitch`, `linux_kernel_object_find.inc.strswitch`, `no_python.c`
> 보조 인용: `program.c`(코어덤프 셋업), `arch_x86_64.c`(페이지테이블 워킹), `drgn/helpers/linux/kallsyms.py`(파이썬 오케스트레이션)

---

## 1. 역할

이 서브시스템은 drgn이 "Linux 커널 메모리"를 디버깅 대상으로 다룰 수 있게 하는 모든 커널 특화 로직을 담는다. 핵심 임무는 다음 5가지다.

1. **vmcoreinfo 파싱** — 커널이 크래시/덤프 시점에 내보내는 메타데이터(`swapper_pg_dir` 주소, 페이지 크기, KASLR offset, sparsemem 파라미터 등)를 읽어 이후 모든 주소 변환·심볼 복원의 기반값으로 삼는다.
2. **메모리 소스 추상화** — `/proc/kcore`(라이브), ELF `vmcore`, makedumpfile 압축덤프(libkdumpfile)를 동일한 메모리 리더 인터페이스 뒤로 숨긴다.
3. **가상주소 변환** — `swapper_pg_dir`부터 아키텍처별 페이지테이블을 걸어 커널 가상주소 → 물리주소를 해석한다(코어덤프엔 물리주소만 저장되므로 필수).
4. **심볼 복원** — DWARF 심볼테이블이 없는(스트립된) 커널에서 빌트인 kallsyms 테이블을 메모리에서 읽어 심볼을 재구성한다.
5. **모듈 디버그정보 로딩** — 적재된 커널 모듈 목록을 `struct module` 리스트 순회로 찾고, 각 `.ko` 디버그 파일을 파일시스템에서 매칭한다.

이 파일들은 `prog->flags & DRGN_PROGRAM_IS_LINUX_KERNEL`이 설정될 때만 활성화되며, 상위 진입점은 `drgn_program_finish_set_kernel()`(linux_kernel.c:722)이다.

**왜 이렇게 복잡한가** — 핵심 어려움은 두 가지다. (a) 코어덤프엔 보통 물리 메모리만 들어 있고 커널 가상주소 매핑은 들어 있지 않다. 그래서 drgn은 페이지테이블을 스스로 걸어 VA→PA를 풀어야 한다(§4). (b) 어떤 커널 버전·Kconfig·디스트로인지 미리 알 수 없다. drgn은 단 하나의 바이너리로 v4.x부터 최신까지, x86_64/aarch64/ppc64/s390x/riscv 등을 디버깅하려 한다. 그래서 코드 전반이 "여러 가능한 레이아웃을 차례로 시도하고 `DRGN_ERROR_LOOKUP`을 잡아 다음으로 넘어가는" 방어적 분기로 가득하다. 버전 경계는 거의 항상 커널 커밋 해시 주석으로 표기된다.

---

## 2. vmcoreinfo 파싱 (키 표 + 용도)

vmcoreinfo는 `key=value\n` 라인의 텍스트 블롭이다. 파서 `drgn_program_parse_vmcoreinfo()`는 `memchr`로 줄을 끊고 `=`로 키/값을 분리한 뒤 `@memswitch@`(strswitch 코드 생성기)로 키별 분기한다(drgn_program_parse_vmcoreinfo.inc.strswitch:49-218). 결과는 모두 `prog->vmcoreinfo` 구조체(program.h:186-253)에 저장된다.

파일 확장자 `.inc.strswitch`는 빌드 시 strswitch 도구가 `@memswitch (line, equals - line)@ ... @case "KEY"@ ... @endswitch@` 매크로를 효율적인 길이별/문자별 분기로 펼쳐 `.inc`로 생성하고, 이를 `linux_kernel.c:43`이 `#include`한다. 숫자 값은 `parse_vmcoreinfo_u64()`(strswitch:4-19)가 `strtoull`로 파싱하며, 범위 초과는 `DRGN_ERROR_OVERFLOW`, 형식 오류는 `DRGN_ERROR_BAD_DATA`로 보고한다. 키마다 진법이 다르다는 점(주소는 16, 카운트는 0=auto)에 주의.

| vmcoreinfo 키 | 저장 필드 | 진법 | 용도 |
|---|---|---|---|
| `OSRELEASE` | `osrelease[128]` | str | `uname -r`. vmlinux/모듈 파일 경로 구성, Fedora 판별, kallsyms long-name 휴리스틱(§5). **필수** (없으면 BAD_DATA) |
| `PAGESIZE` | `page_size` | 0(auto) | 페이지 크기. 2의 거듭제곱이어야 하며 `page_shift = ctz(page_size)` 계산(strswitch:207-211) |
| `SYMBOL(swapper_pg_dir)` | `swapper_pg_dir` | 16 | 커널 최상위 페이지테이블 주소. **모든 VA 변환의 출발점**(§4). **필수** |
| `KERNELOFFSET` | `kaslr_offset` | 16 | KASLR로 인한 가상주소 슬라이드. 심볼/타입 주소 재배치 |
| `KERNELOFFPHYS` | `kaslr_offset_phys` (+`have_kaslr_offset_phys`) | 16 | 물리주소 KASLR 슬라이드 |
| `NUMBER(phys_base)` | `phys_base` (+`have_phys_base`) | 0 | x86_64 커널 이미지 물리 베이스. swapper_pg_dir 자기참조 변환 회피에 사용(§4) |
| `NUMBER(pgtable_l5_enabled)` | `pgtable_l5_enabled` | 0 | x86_64 5단계 페이지테이블 여부 → 워킹 레벨 4 vs 5 결정 |
| `NUMBER(kimage_voffset)` | `kimage_voffset` | 0 | AArch64 커널 이미지 가상/물리 오프셋 |
| `NUMBER(KERNELPACMASK)` | `prog->aarch64_insn_pac_mask` | 16 | AArch64 포인터 인증(PAC) 마스크 — 스택 언와인딩 시 리턴주소 PAC 비트 제거 |
| `NUMBER(VA_BITS)` / `NUMBER(TCR_EL1_T1SZ)` | `va_bits` / `tcr_el1_t1sz` | 0 | AArch64 가상주소 비트수/T1SZ |
| `CONFIG_ARM_LPAE` | `arm_lpae` | 'y' | ARM 32비트 LPAE(대형 물리주소 확장) 여부 |
| `LENGTH(mem_section)` | `mem_section_length` | 0 | sparsemem NR_SECTION_ROOTS. `vmemmap`/`MAX_PHYSMEM_BITS` 역산(linux_kernel.c:406,642) |
| `NUMBER(SECTION_SIZE_BITS)` | `section_size_bits` | 0 | sparsemem 섹션 크기 비트(v5.13+에만 존재, 이전은 arch fallback) |
| `NUMBER(MAX_PHYSMEM_BITS)` | `max_physmem_bits` | 0 | 물리주소 최대 비트(v5.9+, 이전은 NR_SECTION_ROOTS로 역산) |
| `BUILD-ID` | `build_id` (+`build_id_len`) | hex | 커널 빌드 ID(v5.9+). 디버그 파일 매칭/검증(§6) |
| `CRASHTIME` | `have_crashtime` | — | 크래시 시각 존재 여부(라이브 vs 덤프 판별 보조) |

추가로 `SYMBOL(kallsyms_names)`, `SYMBOL(kallsyms_num_syms)` 등 kallsyms 관련 심볼도 vmcoreinfo에 들어 있으나, **이들은 C strswitch가 아니라 파이썬 측 `_vmcoreinfo_symbols()`가 정규식으로 raw 블롭에서 추출한다**(kallsyms.py:40-48). C strswitch는 `swapper_pg_dir` 하나만 `SYMBOL(...)` 형태로 처리한다.

원본 블롭 전체는 `prog->vmcoreinfo.raw`/`raw_size`에 보존되어(strswitch:55-56), `prog["VMCOREINFO"]` 객체(linux_kernel.c:357-375)로 파이썬에 노출된다.

**필수 vs 선택**: 파싱 끝의 검증(strswitch:202-217)은 `OSRELEASE`, 2의 거듭제곱 `PAGESIZE`, `swapper_pg_dir` 세 가지만 필수로 요구하고 나머지는 모두 선택이다. 선택 키가 없으면 §4-5의 arch fallback이나 역산 로직이 메운다 — 이는 vmcoreinfo가 커널 버전마다 점진적으로 키를 추가해 온 역사(예: MAX_PHYSMEM_BITS는 v5.9, SECTION_SIZE_BITS는 v5.13, kallsyms 심볼은 v6.0)를 반영한다.

**버그 우회**: `ignore_broken_vmcoreinfo_build_id()`(strswitch:32-47)는 v6.12 build-id 파싱 버그가 백포트된 특정 stable 버전대(예: 6.6.55~62, 5.15.168+)에서 build_id를 강제로 무효화한다 — 깨진 build ID로 잘못된 디버그 파일을 매칭하는 것을 막기 위함.

---

## 3. 메모리 소스별 커널 경로 (kcore/vmcore/kdump 표)

진입점 `drgn_program_set_core_dump_fd_internal()`(program.c:330)이 파일 시그니처를 보고 세 경로로 분기한다.

| 항목 | /proc/kcore (라이브) | ELF vmcore | kdump 압축덤프 |
|---|---|---|---|
| 판별 | ET_CORE + `NT_TASKSTRUCT` 노트 + fs가 `PROC_SUPER_MAGIC`(0x9fa0) → `is_proc_kcore`(program.c:471-484) | ET_CORE + VMCOREINFO 노트, `!is_proc_kcore`(program.c:489) | 파일 헤더 `KDUMP_SIGNATURE`/`makedumpfile`(linux_kernel.h:44-50) → `has_kdump_signature`(program.c:354) |
| vmcoreinfo 출처 | PT_NOTE "VMCOREINFO"(v4.19+); 없으면 `read_vmcoreinfo_fallback()`가 `/sys/kernel/vmcoreinfo`의 물리주소를 읽음(linux_kernel.c:125-168, program.c:647-652) | PT_NOTE "VMCOREINFO"(program.c:450-461,640) | `kdump_vmcoreinfo_raw()` → 파서(kdump.c:228-253) |
| 메모리 읽기 콜백 | `drgn_read_memory_file`(파일 PT_LOAD 세그먼트) + `read_memory_via_pgtable`(미수록 영역 fallback) | 동일, 단 `zerofill=true`(program.c:571) | `drgn_read_kdump` → `kdump_read(KPHYSADDR/KVADDR)`(kdump.c:111-126) |
| 플래그 | `IS_LINUX_KERNEL\|IS_LIVE\|IS_LOCAL`(program.c:653-655); ELF 핸들 닫음(`elf_end`) | `IS_LINUX_KERNEL`만(program.c:659) | `IS_LINUX_KERNEL`(kdump.c:286) |
| 라이브 최적화 | `/sys/kernel/notes`, `/sys/module/$m/{notes,sections}` 직접 읽기(§6) | 불가(코어에서 읽음) | 불가 |
| 라이브러리 의존 | 없음(libelf만) | libelf | **libkdumpfile** (`WITH_LIBKDUMPFILE`; 미빌드 시 에러, linux_kernel.h:55-62) |

**핵심 메모리 와이어링**(program.c:509-589):
- `pgtable_reader`가 가능하면(아키텍처가 페이지테이블 워킹을 지원하면) 먼저 `[0, UINT64_MAX]` 전 범위에 `read_memory_via_pgtable`를 가상 세그먼트로 등록한다(program.c:517). 이는 **fallback**으로, 코어덤프 PT_LOAD에 없는 주소를 페이지테이블로 변환해 읽어준다.
- 그 위에 각 `PT_LOAD` 세그먼트를 가상주소(`p_vaddr`)와, 유효하면 물리주소(`p_paddr`)로 등록한다(program.c:572-589). 나중에 등록된 구체 세그먼트가 우선한다.
- kcore의 `p_paddr`가 0인 구버전(< v4.11)은 direct-mapping fallback으로 물리 세그먼트를 추가하는 3차 패스가 있다(program.c:599-639).

**zerofill 의미**(program.c:541-571): makedumpfile은 제외 영역을 `p_filesz < p_memsz`로 표현한다. 커널 덤프는 "0이라서 제외"인지 "다른 이유로 제외"인지 구분 불가라 항상 0을 반환(`zerofill=true`)하고, kcore는 0 채움 없이 fault(`zerofill=false`)로 처리한다.

**QEMU 게스트 덤프 안내**(program.c:660-667): VMCOREINFO 노트 없이 `QEMU` 노트만 있으면, drgn은 매핑을 풀 방법이 없으므로 친절한 에러를 낸다 — *"-device vmcoreinfo로 QEMU를 실행하고 CONFIG_FW_CFG_SYSFS/CONFIG_KEXEC를 켜고 qemu_fw_cfg 모듈을 올린 뒤 덤프하라"*. 이는 vmcoreinfo가 커널 디버깅의 **필수 전제**임을 잘 보여준다.

**ELF vmcore를 굳이 libkdumpfile로**: `DRGN_USE_LIBKDUMPFILE_FOR_ELF` 환경변수가 켜져 있으면 ELF vmcore도 libkdumpfile로 처리한다(program.c:489-499). 일반적으론 ELF는 drgn 자체 경로가 더 빠르다.

**kdump의 두 리더**: 기본은 libkdumpfile의 `drgn_read_kdump`지만, `DRGN_PREFER_PGTABLE_READER` 환경변수를 켜면 drgn 자체 페이지테이블 리더를 쓴다(kdump.c:128-132,272-278). 덤프 캡처 커널 자신을 디버깅할 때 libkdumpfile이 원본 커널 매핑을 잘못 쓰는 문제를 우회하기 위함(kdump.c:264-271).

**kdump 속성 설정 순서**(kdump.c:148-262): libkdumpfile 속성은 순서가 중요하다 — (1) 아키텍처를 FD보다 먼저 설정해야 헤더의 arch별 노트(PRSTATUS)를 건너뛰지 않아 on-CPU 스택 추적이 정확해지고, (2) vmcoreinfo는 아키텍처 이후에 설정해야 주소 변환 메타데이터가 올바로 재구성된다. drgn이 이미 vmcoreinfo/플랫폼을 갖고 있으면 libkdumpfile에 주입하고, 없으면 `kdump_vmcoreinfo_raw`/`kdump_get_*_attr`로 받아온다.

**kdump 스레드(PRSTATUS) 캐싱**(kdump.c:308-365): `drgn_program_cache_kdump_threads()`가 `cpu.number`만큼 `cpu.N.PRSTATUS` blob을 읽어 `drgn_program_cache_prstatus_entry`로 등록한다. 이 PRSTATUS가 각 CPU의 레지스터 상태이므로 스택 언와인딩의 출발점이 된다([[07-stack-arch]]).

---

## 4. 가상주소 변환: 페이지테이블 워킹 (file:line)

코어덤프엔 보통 **물리 메모리만** 저장되므로, 커널 가상주소를 읽으려면 페이지테이블을 직접 걸어야 한다.

**진입**: 등록된 fallback 세그먼트 콜백 `read_memory_via_pgtable()`(linux_kernel.c:45-52)은 곧장 `linux_helper_read_vm(prog, prog->vmcoreinfo.swapper_pg_dir, addr, buf, count)`를 호출한다. 즉 모든 커널 VA 변환은 vmcoreinfo의 `swapper_pg_dir`에서 출발한다.

**공통 프레임 / 재진입 가드** — `begin_virtual_address_translation()`(linux_kernel_helpers.c:21-68):
- `prog->pgtable_its[]` 배열과 `address_translation_depth`로 **재귀 변환을 차단**한다. 같은 `pgtable`이 스택에 이미 있으면 "recursive address translation; page table may be missing"(line 27-31). 페이지테이블 메모리 자체를 읽으려다 무한 재귀하는 상황 방지.
- `IS_LINUX_KERNEL`인지, `has_platform`인지, `arch->linux_kernel_pgtable_iterator_next`가 구현됐는지 검사 후 아키텍처별 이터레이터를 생성한다(line 47-58). **이것이 arch 후크와의 경계다** — 공통 코드는 "다음 매핑 구간을 달라"고만 요청하고, 실제 PTE 해석은 arch가 한다.

**본체** — `linux_helper_read_vm()`(linux_kernel_helpers.c:154-210):
- `next = arch->linux_kernel_pgtable_iterator_next`를 반복 호출해 `(start_virt_addr, start_phys_addr)` 매핑 구간을 받는다(line 168-174).
- `start_phys_addr == UINT64_MAX`면 미매핑 → fault("address is not mapped")(line 177-181).
- `phys_addr = start_phys + (virt - start_virt)`로 변환하고, **연속된 물리 구간을 합쳐서**(line 186-199) 한 번의 `drgn_program_read_memory(..., physical=true)`로 읽어 효율화한다.

**아키텍처 후크 (x86_64)** — `linux_kernel_pgtable_iterator_next_x86_64()`(arch_x86_64.c:602-689):
- 상태 `pgtable_iterator_x86_64`는 5레벨 캐시 `table[5][512]`와 `index[5]`를 가진다(arch_x86_64.c:571-575) — 테이블당 4096바이트를 통째로 읽어 캐시.
- 레벨 수는 `pgtable_l5_enabled ? 5 : 4`(line 619). 비정규(non-canonical) 주소 구간은 즉시 미매핑 처리(line 621-632).
- 각 레벨 엔트리에서 `PRESENT`(0x1)·`PSE`(0x80, huge page)·최하위 레벨을 검사해 매핑 구간을 산출한다(line 661-672). PSE면 그 레벨에서 대형 페이지로 종료.
- **자기참조 회피**(line 644-652): 최상위 테이블이 `swapper_pg_dir`이고 `have_phys_base`면 `table = pgtable + phys_base - START_KERNEL_MAP`로 **직접 물리주소를 계산**한다(VA 변환 재귀 방지). `phys_base`는 v4.10+ vmcoreinfo가 제공.
- 페이지테이블 메모리 읽기는 첫 레벨만 가상(`table_physical=false`), 이후는 물리주소로 읽는다(line 644-684).

**direct map fallback** — `linux_helper_direct_mapping_offset()`(linux_kernel_helpers.c:70-152):
- 페이지테이블 워킹이 비싸므로, direct map(물리주소를 선형 매핑한 영역) 오프셋을 한 번 구해 캐시하면 단순 가감산으로 물리주소를 얻을 수 있다.
- x86_64는 arch 후크 `linux_kernel_direct_mapping_offset_x86_64`가 `page_offset_base` 변수를 읽어 즉답(arch_x86_64.c:558-568). 실패 시 공통 fallback이 작동.
- 공통 fallback(line 94-148): direct map에 있는 임의 변수 `saved_command_line`의 가상주소를 페이지테이블로 한 번 변환해 `offset = start_virt - start_phys`를 구한다. 결과는 `direct_mapping_offset_cached`에 저장.

`linux_helper_follow_phys()`(linux_kernel_helpers.c:212-239)는 변환 후 첫 매핑만 보고 단일 VA→PA를 반환하는 경량 버전이다.

**재귀 한도 / 동시성**: `prog->pgtable_its[]`는 고정 크기 배열이고 `address_translation_depth`가 인덱스다(linux_kernel_helpers.c:33-36). 깊이가 배열 크기를 넘으면 "address translation recursion limit hit" fault. 한 변환 안에서 페이지테이블 메모리를 읽으려다 또 변환이 필요할 수 있으므로(예: 페이지테이블이 vmalloc 영역에 있는 드문 경우) 이 중첩을 깊이로 제한한다. 이터레이터는 `prog`당 한 벌이라 **단일 스레드 가정**이며, drgn은 프로그램 객체를 스레드 간 공유하지 않는 전제로 동작한다.

**arch 후크 인터페이스**: 각 아키텍처는 `linux_kernel_pgtable_iterator_{create,destroy,init,next}`와, 선택적으로 `direct_mapping_offset`/`live_direct_mapping_fallback`/`section_size_bits_fallback`/`max_physmem_bits_fallback`을 `arch->...`로 제공한다(arch_x86_64.c:726-735의 ops 테이블). 공통 코드는 이 함수 포인터만 호출하므로, 새 아키텍처 지원은 이 후크 구현으로 환원된다.

---

## 5. kallsyms 심볼 복원 (스트립 커널 대응)

DWARF 심볼테이블이 없는 커널에서도, 커널은 자체 심볼테이블 `kallsyms`를 **메모리에 빌트인**으로 들고 있다(`CONFIG_KALLSYMS`). drgn은 이를 메모리에서 읽어 복원한다(kallsyms.c).

**위치 정보 출처**: `struct kallsyms_locations`(kallsyms.h:20-30)의 8개 주소(`kallsyms_names`, `kallsyms_token_table`, `kallsyms_token_index`, `kallsyms_num_syms`, `kallsyms_offsets`, `kallsyms_relative_base`, `kallsyms_addresses`, `_stext`)는 vmcoreinfo의 `SYMBOL(...)` 라인에서 온다(v6.0+ 커널). 파이썬 `_load_builtin_kallsyms()`가 `_vmcoreinfo_symbols()`로 추출해 C 함수 `drgn_load_builtin_kallsyms()`에 넘긴다(kallsyms.py:51-66, python/helpers.c:335-364). vmcoreinfo에 없으면(구커널/CAP_SYSLOG 가능 시) `/proc/kallsyms` 텍스트 파싱으로 대체(kallsyms.py:84-88).

**이름 압축 해제**: vmlinux kallsyms는 토큰 테이블 압축을 쓴다(kallsyms.c:20-45). `token_table`(연결된 토큰 문자열)과 `token_index[256]`(각 토큰 시작 오프셋)으로, 이름 각 바이트를 토큰으로 펼친다. 첫 디코드 바이트는 심볼 종류(nm 문자). `kallsyms_copy_tables()`(kallsyms.c:115-231)가 세 테이블을 호스트 메모리로 복사하고, `kallsyms_expand_symbol()`(line 250-287)이 한 심볼을 펼친다.

**long-name 휴리스틱**: "big" 심볼(2바이트 길이) 지원 여부는 vmcoreinfo로 구별 불가라, 커널 ≥ 6.1을 기준으로 추정한다(`guess_long_names()`, kallsyms.c:90-107). `DRGN_KALLSYMS_LONG` 환경변수로 강제 가능.

**주소 인코딩 4종 자동 판별** — `kallsyms_load_addresses()`(kallsyms.c:427-550):

| 인코딩 | 시기 | 방식 |
|---|---|---|
| Absolute | ~v6.11 | `kallsyms_addresses[]`에 절대주소 직접 저장 |
| Base relative | v4.6~ | `relative_base + kallsyms_offsets[i]` |
| Absolute percpu | v4.6~v6.15 | 음수=상대오프셋, 양수=절대(percpu 저주소 대응) |
| Place relative | v7.0~ | `&kallsyms_offsets[i] + offsets[i]` (저장 위치 상대) |

판별은 **`_stext`의 정답 주소(vmcoreinfo가 제공)를 세 방식으로 계산해 일치하는 것을 채택**한다(line 519-547). `_stext`의 인덱스는 압축 해제하며 이름을 비교해 찾는다(`search_for_string()`, kallsyms.c:293-315). 이 분기야말로 버전·Kconfig 차이를 런타임에 흡수하는 drgn의 핵심 트릭이다. `kallsyms_addresses[]`가 vmcoreinfo에 직접 있으면(절대 인코딩) 32/64비트·바이트오더만 맞춰 그대로 읽는다(line 447-476).

**바이트오더 처리**: 덤프 호스트와 분석 호스트의 엔디안이 다를 수 있으므로 `drgn_program_bswap()`로 swap 여부를 판단해 모든 다바이트 값(token_index, addresses)을 보정한다(kallsyms.c:128-150). `token_index[i]`가 `token_table_len`을 벗어나면 손상으로 보고 거부한다(line 173-179) — 신뢰할 수 없는 덤프 데이터에 대한 방어.

**심볼 종류 매핑**: `symbol_from_kallsyms()`(kallsyms.c:317-355)가 nm 종류 문자(`t`=함수, `b/c/d/g/r/v`=객체, 대문자=global, `u`=unique, `v/w`=weak)를 drgn 바인딩/종류로 변환한다. 심볼 길이는 다음 심볼 시작주소로 추정하되 `MAX_SYMBOL_LENGTH`(0x10000)로 캡(percpu~본문 사이 큰 간격이 거대 심볼로 보이는 문제 방지, kallsyms.c:56-69).

**라이브 보조**: `proc_kallsyms_symbol_addr()`(linux_kernel.c:54-117)는 `/proc/kallsyms`에서 단일 심볼 주소를 찾는다(arch가 `page_offset_base` 등을 구할 때 사용). `kptr_restrict`로 주소가 0이고 비-root면 친절한 에러를 낸다(line 113-115).

**전체 텍스트 파서**: `drgn_load_proc_kallsyms()`(kallsyms.c:616-714)는 `/proc/kallsyms` 전체를 줄 단위로 파싱한다. `modules` 인자로 모듈 심볼 포함 여부를 정하고, 모듈 경계가 바뀌면 직전 심볼 크기 계산을 멈춘다(모듈 kallsyms는 주소가 단조 증가하지 않을 수 있어 오버플로 방어, line 674-684). 모듈 kallsyms는 파이썬 측 `module_kallsyms()`/`load_module_kallsyms()`(kallsyms.py:164-)가 `mod->kallsyms`(또는 init 단계 `mod_kallsyms`)를 직접 읽어 처리한다 — 모듈별 심볼 복원 경로다.

---

## 6. 커널 모듈 디버그정보 로딩

**적재 모듈 목록 찾기** — `linux_kernel_loaded_module_iterator`(linux_kernel.c:1627-2588):
- 먼저 vmlinux를 `yield_vmlinux()`로 한 번 내보낸다(line 1648-1710). 빌드 ID는 vmcoreinfo(v5.9+) → 라이브 `/sys/kernel/notes`(pre-5.9) 순으로 획득.
- 그다음 전역 `modules` 리스트(`struct list_head`)를 순회한다(line 2488-2572). `struct module` 타입과 `modules` 변수가 **디버그정보에 있어야** 시작 가능 — 없으면 main_module 디버그 파일을 먼저 로드 시도, 그래도 없으면 "can't find loaded modules without kernel debug info" 경고(line 2504-2547).
- 사이클/손상 방어로 `MAX_MODULE_LIST_ITERATIONS`(10000) 한도(line 1623-1625, 2445-2452). 각 모듈은 `container_of(node, struct module, list)`로 복원하고, kcore 성능을 위해 `struct module` 전체(<2KB)를 한 번에 읽는다(line 2462-2466).

**모듈 주소 범위 3버전** — `kernel_module_address()`(linux_kernel.c:1712-1776):

| 버전 enum | 시기 | 베이스 주소 | 크기 |
|---|---|---|---|
| `MODULE_MEMORY` | v6.4+ | `module->mem[MOD_TEXT].base` | `mem[type].size` 타입별(불연속 범위들) |
| `MODULE_LAYOUT` | v4.5~v6.4 | `module->core_layout.base` | `core_layout.size`(연속) |
| `IN_MODULE` | ~v4.5 | `module->module_core` | `module->core_size`(연속) |

`MOD_TEXT` 상수값은 캐시한다(`prog->mod_text_cached`). 범위는 `kernel_module_set_address_ranges()`(line 1780-1853)가 등록 — MODULE_MEMORY는 size>0인 메모리 타입마다 별도 range를 등록한다.

**빌드 ID 획득** — `kernel_module_set_build_id()`(linux_kernel.c:1921-2048):
- 라이브(`use_sys_module`)면 `/sys/module/$m/notes/` 디렉토리의 노트 파일을 읽음(line 1855-1919).
- 덤프면 `module->notes_attrs`를 순회 — v6.14+는 `grp.bin_attrs`(NULL 종단 배열), 이전은 `attrs[notes]`(개수 명시) 두 레이아웃을 모두 처리(line 1939-1976). 각 attr의 `private`(주소)/`size`로 메모리를 읽어 GNU note에서 build-id 추출.

**섹션 주소** — `kernel_module_set_section_addresses()`(linux_kernel.c:2050-2281): 라이브는 `/sys/module/$m/sections/`의 각 파일에서 주소를 fscanf(line 2050-2102), 덤프는 `sect_attrs`에서 읽는다. 재배치 가능 ELF의 섹션 배치를 실제 적재주소에 맞춘다.

**라이브 fast-path 토글**: 라이브(`IS_LIVE|IS_LOCAL`)에서는 build-id/섹션 주소를 코어가 아닌 `/sys/module/...`에서 읽는 빠른 경로를 쓴다. `DRGN_USE_SYS_MODULE` 환경변수로 끌 수 있고(테스트/권한 문제 시), `/sys/module/$m/sections` 권한 오류가 나면 자동으로 코어 읽기로 폴백한다(linux_kernel.c:2344-2362). 마찬가지로 vmlinux build-id도 라이브면 `/sys/kernel/notes`를 먼저 본다(§6).

**`.ko` 디버그 파일 매칭** — `drgn_module_try_linux_kmod_files()`(linux_kernel.c:1499-1573), 두 전략:
1. **depmod**: `/lib/modules/$(uname -r)/modules.dep.bin`(radix tree)을 mmap·파싱해 모듈명→경로를 O(이름길이)로 조회(`depmod_index_find()`, line 851-941). 매직 `0xb007f457`, 버전 `0x00020001` 검증(line 790-811). 노드 오프셋의 상위 비트가 자식/값/접두사 존재를 인코딩한다 — `INDEX_NODE_PREFIX`(0x80000000)면 공통 접두사 문자열, `INDEX_NODE_CHILDS`(0x20000000)면 first~last 범위 자식 포인터 배열, `INDEX_NODE_VALUES`(0x40000000)면 값(경로) 보유(line 856-916). `.ko`/`.ko.gz`/`.ko.xz`/`.ko.zst` 압축 확장자를 한 겹 벗겨 `.debug` 후보까지 시도한다(line 1225-1244). depmod 포맷이 바뀌면 libkmod 대신 자체 파서를 재평가한다는 주석이 있다(line 745-756).
2. **walk**: `/lib/modules/$(uname -r)` 트리를 직접 DFS 순회하며 모든 `.ko`를 수집한다(`drgn_kmod_walk*`, line 1283-1497). `drgn_kmod_walk_inode_set`으로 (dev,ino) 중복 디렉토리를 제거(심볼릭 링크 순환 방지, line 961-991)하고, 발견한 경로를 모듈명별 벡터(`drgn_kmod_walk_module_map`)에 적재한다. 한 모듈에 매칭 후보가 떨어지면 walk를 재개해 다음 후보를 찾는 지연(lazy) 방식이다(line 1544-1567). `DRGN_KMOD_SEARCH_{NONE,DEPMOD,WALK,DEPMOD_AND_WALK}` 모드로 선택(line 1506-1524).

**vmlinux 파일 매칭** — `drgn_module_try_vmlinux_files()`(linux_kernel.c:1038-1102): `/boot/vmlinux-$osrelease`, `/lib/modules/$osrelease/{build/vmlinux,vmlinux}`, 디스트로별 디버그 디렉토리(Debian `/boot/vmlinux-%s`, Fedora `/lib/modules/%s/vmlinux`, SUSE `.vmlinux.debug`)를 차례로 시도(line 1008-1015).

---

## 7. C 레벨 내장 헬퍼 (linux_kernel_helpers.c)

성능이 중요한(타이트 루프에서 수만 번 호출되는) 헬퍼는 파이썬 대신 C로 구현된다. 프로토타입은 helpers.h:24-92.

| 헬퍼 | 역할 | 핵심 |
|---|---|---|
| `linux_helper_read_vm` / `follow_phys` | VA→PA 변환·읽기 | §4 |
| `linux_helper_direct_mapping_offset` | direct map 오프셋 | §4 |
| `linux_helper_per_cpu_ptr`(line 241-274) | percpu 포인터 해석 | `__per_cpu_offset[cpu]`를 베이스에 더함. 없으면 !SMP로 원본 복사 |
| `linux_helper_cpu_curr`/`idle_task`(line 276-307) | CPU의 현재/idle task | `runqueues` percpu의 `curr`/`idle` 멤버 |
| `linux_helper_task_thread_info`(line 309-334) | task→thread_info | `CONFIG_THREAD_INFO_IN_TASK` y/n 두 경우 |
| `linux_helper_task_cpu`(line 336-387) | task의 CPU 번호 | `task->cpu` vs `thread_info->cpu` 버전 분기(v5.16/v4.9 커밋 차이 상세 주석) |
| `linux_helper_task_on_cpu`(line 389-412) | 실행 중 여부 | `task->on_cpu`, !SMP면 `cpu_curr(0)` 비교 |
| `linux_helper_xa_load`(line 414-618) | XArray/radix tree 조회 | xarray vs 구 radix_tree 양쪽, sibling 엔트리 처리 |
| `linux_helper_idr_find`(line 625-655) | IDR 조회 | `idr_base` 보정 후 `xa_load` |
| `linux_helper_find_pid`(line 815-850) | PID→struct pid | v4.15+ `ns->idr`, 이전은 `pid_hash` 전수 탐색(line 665-813) |
| `linux_helper_pid_task`(line 852-915) | pid→task_struct | `pid_links[]` vs `pids[].node` container_of |
| `linux_helper_find_task`(line 917-939) | pid→task 종합 | find_pid + pid_task |
| `linux_helper_task_iterator_*`(line 941-1063) | 전체 task 순회 | `init_task.tasks` 리스트 + `signal->thread_head` 스레드 순회 |

이들은 공통적으로 **여러 커널 버전/Kconfig를 `drgn_error_catch(DRGN_ERROR_LOOKUP)`로 흡수**하는 패턴을 쓴다(멤버가 없으면 다음 레이아웃 시도). 예컨대 `linux_helper_task_cpu()`(line 336-387)는 `task->cpu`를 먼저 시도하고 없으면 `thread_info->cpu`로, 그것도 없으면 `!SMP`로 0을 반환한다 — 주석에 v5.16/v4.9 커밋과 CentOS Stream 백포트 예외까지 명시되어 있다.

**task 이터레이터 구조**(linux_kernel_helpers.c:941-1063): 바깥 루프는 `init_task.tasks` 리스트(프로세스), 안쪽 루프는 각 프로세스의 `signal->thread_head` 리스트(스레드)를 순회하는 이중 루프다. `tasks_head`/`thread_head`로 리스트 끝(헤드 복귀)을 감지한다. 모든 task를 도는 동작은 매우 빈번하므로 C 구현의 이득이 크다.

**XArray 헬퍼의 복잡성**(line 414-618): `linux_helper_xa_load()`는 현대 XArray와 구버전 radix_tree를 동시에 지원하며, 내부 노드 플래그(`internal_flag` 1 vs 2)·sibling 엔트리(중복 슬롯)·노드 shift를 따라 다단계로 내려간다. PID→task, 페이지 캐시 등에서 쓰이는 핵심 자료구조라 C로 구현되었다.

참고로 `linux_cpu_present_mask()`(linux_kernel.c:2590-2683)는 C 내부 전용으로 두고 파이썬 헬퍼로 노출하지 않는다(이득이 적어서). 이 함수는 `__cpu_present_mask`/`cpu_present_mask`의 비트맵을 32/64비트·엔디안 보정하며 읽어, `drgn_program_is_irq_regs`가 CPU별 `__irq_regs`를 모으는 데 쓴다.

---

## 8. ★ debuginfod 커널 제약 (is_fedora_kernel)

`drgn_program_finish_set_kernel()`(linux_kernel.c:722-743)은 커널 셋업 마지막에 **비-Fedora 커널이면 debuginfod 디버그정보 파인더를 비활성화**한다:

```c
// linux_kernel.c:737-740
if (!is_fedora_kernel(prog->vmcoreinfo.osrelease)
    && drgn_handler_list_disable(&prog->dbinfo.debug_info_finders, "debuginfod"))
    drgn_log_debug(prog, "disabled debuginfod for Linux kernel");
```

이유는 바로 위 주석(line 734-736)에 명시: *"작성 시점 기준 오직 Fedora의 debuginfod 서버만 빠른 커널 다운로드를 제공한다. 다른 곳은 고통스럽게 느려서 비활성화한다."*

`is_fedora_kernel()`(linux_kernel.c:706-720)은 `osrelease`(=`uname -r`)에 정규식 `/\.fc[0-9]+(\.|$)/` 패턴(예: `6.8.0-1.fc40.x86_64`의 `.fc40.`)이 있는지 검사한다. `.fc` 뒤에 1자리 이상 숫자가 오고, 그 뒤가 `.` 또는 문자열 끝이면 Fedora로 판정한다. 즉 **vmcoreinfo의 OSRELEASE 문자열만으로 디스트로를 추정**해 debuginfod 사용 여부를 가른다. Fedora 커널이면 debuginfod로 vmlinux/모듈 디버그정보를 자동 다운로드하지만, Ubuntu/Debian/SUSE 등은 §6의 로컬 파일 경로(`/usr/lib/debug/...`, `/boot/vmlinux-...`)에 의존해야 한다.

비활성화는 `drgn_handler_list_disable(&prog->dbinfo.debug_info_finders, "debuginfod")`로 수행된다 — 디버그정보 파인더는 핸들러 리스트로 관리되며, 이 호출은 "debuginfod" 핸들러를 비활성 표시한다(존재해서 실제로 끄면 true 반환 → 디버그 로그). 이는 코드를 지우는 게 아니라 **런타임에 OSRELEASE에 따라 파인더 체인을 조정**하는 설계로, 사용자 커널이 어디 출신이냐에 따라 디버그정보 획득 경로가 달라짐을 의미한다. (이 제약은 성능상 결정이며, 다른 debuginfod 서버가 빨라지면 풀릴 수 있다고 주석이 단서를 단다.)

---

## 9. 타 서브시스템 연결

- **[[02-program-memory]]**: 메모리 리더에 세그먼트 콜백을 등록하는 쪽이 이 서브시스템이다. `read_memory_via_pgtable`/`drgn_read_kdump`/`drgn_read_memory_file`이 `drgn_program_add_memory_segment`로 꽂힌다(program.c:517-589, kdump.c:272-282). `drgn_program_read_memory(physical=true/false)`가 최종 소비자.
- **[[03-debuginfo-dwarf]]**: `drgn_module_try_vmlinux_files`/`try_linux_kmod_files`가 찾은 ELF 파일이 DWARF 파서로 들어간다. kallsyms는 DWARF가 없을 때의 심볼 인덱스 대체 공급원(`drgn_symbol_index`).
- **[[07-stack-arch]]**: vmcoreinfo의 `aarch64_insn_pac_mask`, `phys_base`, `pgtable_l5_enabled` 등 arch 필드와 페이지테이블 이터레이터(arch_x86_64.c 등)가 arch 추상화 경계다. `drgn_program_is_irq_regs()`(linux_kernel.c:2687)는 percpu `__irq_regs`로 IRQ 스택 프레임을 식별해 언와인딩을 돕는다.
- **object/type 시스템**: `linux_kernel_object_find()`(linux_kernel_object_find.inc.strswitch)가 `PAGE_SIZE`, `THREAD_SIZE`, `UTS_RELEASE`, `vmemmap`, `MAX_PHYSMEM_BITS` 등 디버그정보에 없는 커널 상수/심볼을 vmcoreinfo·휴리스틱으로 합성해 객체 파인더로 등록한다(linux_kernel.c:725-728).
- **파이썬 바인딩 없는 빌드**: `no_python.c`는 libdrgn을 C 라이브러리로만 쓸 때의 스텁(`drgn_program_create/destroy`, 플러그인·blocking no-op)을 제공한다(no_python.c:18-55). 커널 로직 자체는 파이썬과 독립적인 C로 구현되어 이 스텁만으로도 동작 가능. 단, kallsyms 오케스트레이션(`_vmcoreinfo_symbols` 정규식 추출, 모듈 kallsyms)과 일부 고수준 헬퍼는 파이썬 측(`drgn/helpers/linux/*.py`)에 있어, 풀 기능은 파이썬 빌드를 전제로 한다.

**데이터 흐름 한눈에**: 코어덤프/kcore → (program.c) vmcoreinfo 노트 파싱 → `prog->vmcoreinfo` 채움 → 메모리 세그먼트 등록(pgtable fallback + PT_LOAD) → `finish_set_kernel`(객체 파인더 등록 + debuginfod 토글) → 이후 사용자 쿼리 시 VA는 `read_memory_via_pgtable`로 변환, 심볼은 DWARF 또는 kallsyms로, 모듈은 `modules` 리스트 순회로 해소된다.

---

## 10. 파일 맵

| 파일:라인 | 내용 |
|---|---|
| `linux_kernel.c:45-52` | `read_memory_via_pgtable` — VA 변환 fallback 진입 |
| `linux_kernel.c:54-117` | `proc_kallsyms_symbol_addr` — 라이브 단일 심볼 조회 |
| `linux_kernel.c:125-168` | `read_vmcoreinfo_fallback` — `/sys/kernel/vmcoreinfo`(pre-4.19) |
| `linux_kernel.c:380-491` | vmemmap 주소 산출(section_mem_map 기반) |
| `linux_kernel.c:706-743` | `is_fedora_kernel` + `drgn_program_finish_set_kernel`(★debuginfod 제약) |
| `linux_kernel.c:790-941` | depmod(modules.dep.bin) radix tree 파서 |
| `linux_kernel.c:1038-1102` | vmlinux 파일 경로 탐색 |
| `linux_kernel.c:1283-1573` | 모듈 `.ko` 탐색(depmod/walk) |
| `linux_kernel.c:1627-2588` | 적재 모듈 이터레이터(vmlinux→modules 리스트) |
| `linux_kernel.c:1712-1853` | 모듈 주소 범위 3버전 |
| `linux_kernel.c:1921-2281` | 모듈 build-id/섹션 주소 |
| `linux_kernel.c:2687-2762` | `drgn_program_is_irq_regs` |
| `linux_kernel_helpers.c:16-239` | VA 변환 프레임 + read_vm/follow_phys/direct_mapping_offset |
| `linux_kernel_helpers.c:241-1063` | per_cpu/task/xarray/idr/pid C 헬퍼들 |
| `kallsyms.c:115-613` | 빌트인 kallsyms 복원(테이블 복사/주소 인코딩 판별/심볼 빌드) |
| `kallsyms.c:616-714` | `/proc/kallsyms` 텍스트 파서 |
| `kdump.c:111-306` | libkdumpfile 연동(플랫폼/vmcoreinfo/메모리 읽기) |
| `kdump.c:308-365` | PRSTATUS(스레드) 캐시 |
| `drgn_program_parse_vmcoreinfo.inc.strswitch:49-218` | vmcoreinfo 키별 파서 |
| `linux_kernel_object_find.inc.strswitch:4-68` | 커널 합성 상수/객체 파인더 |
| `program.c:330-699` | 코어덤프 셋업(kcore/vmcore/kdump 분기, 세그먼트 등록) |
| `arch_x86_64.c:571-689` | x86_64 페이지테이블 이터레이터 |
| `no_python.c:18-55` | 파이썬 없는 빌드 스텁 |
| `kallsyms.h:20-44` | `kallsyms_locations` 구조체 + 로더 프로토타입 |
| `linux_kernel.h:12-62` | 커널 지원 공개 API + kdump 시그니처/스텁 |
| `drgn/helpers/linux/kallsyms.py:40-88` | vmcoreinfo SYMBOL 추출 + kallsyms 로더 오케스트레이션(파이썬) |
