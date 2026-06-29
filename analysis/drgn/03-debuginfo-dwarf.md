# drgn 코드 분석 — 디버그 정보 로딩과 DWARF 인덱싱

> 소스: osandov/drgn (GitHub main), 로컬 `deep/sources/drgn`
> 분석 대상: `libdrgn/debug_info.{c,h}`, `libdrgn/dwarf_info.{c,h}`, `libdrgn/elf_file.{c,h}`, `libdrgn/elf_symtab.c`, `libdrgn/debug_info_options.{c,h}`, `libdrgn/linux_kernel.c`, `docs/getting_debugging_symbols.rst`
> 관련: [[drgn-codebase-overview]] · [[02-program-memory]] · [[04-type-language]] · [[06-linux-kernel]]

---

## 1. 역할

이 서브시스템은 drgn이 다루는 모든 "타입 정보의 원천"이다. 프로그램(코어 덤프, `/proc/kcore`, 실행 중 프로세스)에서 읽은 바이트열에 의미를 부여하려면 `struct task_struct`의 필드 오프셋, `int`의 크기, 전역 변수의 주소 같은 메타데이터가 필요하다. 이 메타데이터는 **컴파일러가 생성한 디버그 정보(DWARF)와 ELF 심볼 테이블**에서 온다.

핵심 캐시 구조는 `struct drgn_debug_info`(`debug_info.h:80`)이며, 다음을 담당한다.

- 디버그 정보를 **모듈(module) 단위**로 모델링하고 관리 (`drgn_module_table modules`, `debug_info.h:96`).
- 각 모듈에 맞는 ELF/DWARF 파일을 로컬 경로·build-id·debuginfod에서 **찾아서** 연다.
- DWARF DIE(Debugging Information Entry)를 **이름→DIE 주소** 인덱스로 만들어 빠른 조회를 가능케 한다 (`struct drgn_dwarf_info`, `dwarf_info.h:179`).
- 이 인덱스를 type finder / object finder / symbol finder 형태로 상위 계층(`type.c`/`object.c`/`symbol.c`)에 노출한다 (`debug_info.c:5631`~`5645`).

`debug_info.h:46`의 주석이 범위를 못 박는다: *"caches debugging information (currently DWARF and ORC)"*.

### 전체 파이프라인 (한눈에)

```
  타깃(코어덤프/kcore/프로세스)
        │  주소범위·build-id
        ▼
  [모듈 생성]  drgn_module (kind/name/info)          §3
        │
        ▼
  [파일 탐색]  finder 체인                            §4
   standard ─ 로컬경로 / /usr/lib/debug/.build-id / .gnu_debuglink
   debuginfod ─ build-id로 HTTP 다운로드 (폴백, 커널은 Fedora만)
        │  loaded_file / debug_file (ELF) 채택
        ▼
  [ELF 파싱]   scns[]/scn_data[] 섹션 인덱싱           §2
        │  .debug_info/.debug_abbrev/.symtab/.eh_frame/.orc_*
        ▼
  [DWARF 인덱싱]  abbrev→bytecode, OpenMP 멀티패스      §5
        │  이름 → DIE 주소  (drgn_dwarf_index_die_map)
        ▼
  [finder 노출]  type/object/symbol finder            §6
        │  drgn_type_from_dwarf / drgn_object_from_dwarf
        ▼
   prog.type("...") / prog["..."] / prog.symbol(...)  → type.c/object.c
```

---

## 2. ★ drgn은 DWARF만 쓴다 (BTF 미사용) — 코드 근거

drgn이 타입/심볼 소스로 소비하는 ELF 섹션은 코드 생성기 `libdrgn/build-aux/gen_elf_sections.py:12`~`33`에 **명시적으로 열거**되어 있다. 이 목록이 `enum drgn_section_index`로 컴파일되어 `drgn_elf_file::scns[]`(`elf_file.h:109`)에 인덱싱된다. ELF 파싱 루프(`elf_file.c:139`~`175`)는 섹션 이름을 이 인덱스로 매핑하며, **목록에 없는 섹션은 무시**한다(`index < DRGN_SECTION_INDEX_NUM` 검사, `elf_file.c:174`).

### drgn이 로드/인덱싱하는 섹션 (로드 섹션 표)

| 섹션 | 분류 | 용도 | 코드 근거 |
|------|------|------|-----------|
| `.debug_info`, `.debug_types` | DWARF 인덱스 | DIE 본문 (타입/변수/함수) | `gen_elf_sections.py:12` |
| `.debug_abbrev` | DWARF 인덱스 | DIE 약어(abbrev) 테이블 | `gen_elf_sections.py:15` |
| `.debug_str`, `.debug_str_offsets` | DWARF 인덱스 | 문자열 풀 / 오프셋 | `gen_elf_sections.py:16-17` |
| `.debug_addr` | 캐시 | DWARF5 주소 풀 | `gen_elf_sections.py:21` |
| `.debug_frame`, `.eh_frame` | 캐시 | CFI(스택 언와인드) | `gen_elf_sections.py:22-23` |
| `.debug_loc`, `.debug_loclists` | 캐시 | 위치 표현식 | `gen_elf_sections.py:24-25` |
| `.text`, `.got` | 비캐시 | 주소/리로케이션 기준 | `gen_elf_sections.py:29-30` |
| `.gnu_debuglink`, `.gnu_debugaltlink` | 비캐시 | 분리된 디버그 파일 링크 | `gen_elf_sections.py:31-32` |
| `.symtab` / `.dynsym` | 심볼 | ELF 심볼 테이블 (이름↔주소) | `elf_symtab.c:52-53` |
| `.gnu_debugdata` | 심볼 | LZMA 압축 미니 심볼테이블 | `elf_symtab.c:48-50` |
| `.orc_unwind`, `.orc_unwind_ip` | 언와인드 | 리눅스 커널 ORC 언와인더 | `orc_info.c:315,319` |

`.symtab`/`.dynsym`은 위 인덱스가 아니라 `elf_symtab.c`에서 별도로 섹션 타입(`SHT_SYMTAB`/`SHT_DYNSYM`)으로 직접 스캔한다(`elf_symtab.c:52`). ORC 섹션도 `orc_info.c`에서 이름으로 직접 찾는다(`orc_info.c:314`~`320`). 즉 **타입 = DWARF, 심볼 = ELF symtab, 언와인드 = .eh_frame/.debug_frame/ORC** 세 갈래가 전부다.

### ★ `.BTF`(BPF Type Format)는 타입 소스로 쓰지 않는다

`libdrgn/` 전체에서 `BTF`/`btf`/`.BTF` 문자열을 grep하면 **단 한 건도 나오지 않는다**(C 코드 0건). `gen_elf_sections.py`의 섹션 목록에도 `.BTF`가 없으므로, drgn은 vmlinux의 `.BTF` 섹션을 열어도 그 내용을 타입 정보로 파싱하지 않는다. 실제로 한 ELF 파일이 "DWARF를 가진다"는 판정은 `drgn_elf_file_has_dwarf`(`elf_file.h:186`)가 **`.debug_info`와 `.debug_abbrev` 두 섹션의 존재**만으로 내린다 — BTF는 이 판정에 전혀 관여하지 않는다. drgn에게 "타입이 없다"는 곧 "DWARF가 없다"는 뜻이며, 그래서 `docs/getting_debugging_symbols.rst:37`은 커널 빌드 시 `CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT=y`(DWARF) 설정을 요구한다 — BTF가 아니다.

BTF가 등장하는 유일한 곳은 **Python 헬퍼** `drgn/helpers/linux/bpf.py:36`의 `bpf_btf_for_each()`인데, 이는 타깃 커널 메모리에 있는 `struct btf *` 객체(`btf_idr`)를 **데이터로 순회**할 뿐이다(`bpf.py:44`~`48`). 이때 `struct btf`의 레이아웃조차 DWARF로부터 얻는다(`prog.type("struct btf *")`). 결론: **BTF는 drgn의 디버그 정보 백엔드가 아니라, drgn이 들여다보는 커널 자료구조의 하나일 뿐이다.**

> 한 줄 요약: drgn의 타입 백엔드는 DWARF뿐이며, BTF는 분석 대상(데이터)이지 타입 소스가 아니다.

---

## 3. 모듈(Module) 모델

drgn은 프로그램을 구성하는 각 바이너리를 `struct drgn_module`(`debug_info.h:201`)로 추상화한다. 모듈은 `(kind, name, info)` 3요소로 유일하게 식별된다(`drgn.h:1470`). 종류는 `enum drgn_module_kind`(`drgn.h:1477`):

| kind | 의미 | 예시 |
|------|------|------|
| `DRGN_MODULE_MAIN` | 메인 모듈 | 유저스페이스 실행파일, 커널의 `vmlinux` (`drgn.h:1479`) |
| `DRGN_MODULE_SHARED_LIBRARY` | 공유 라이브러리(.so) | `libc.so` (`drgn.h:1484`) |
| `DRGN_MODULE_VDSO` | 커널이 매핑한 vDSO | `linux-vdso.so` (`drgn.h:1486`) |
| `DRGN_MODULE_RELOCATABLE` | 리로케이터블 객체 | 리눅스 커널 로더블 모듈(.ko) (`drgn.h:1488`) |
| `DRGN_MODULE_EXTRA` | 추가 디버그 정보 | (`drgn.h:1490`) |

각 모듈은 종류별로 별도의 생성 API를 갖는다(`drgn_module_find_or_create_main`/`_shared_library`/`_vdso`/`_relocatable`, `drgn.h:1526`~`1580`). 모듈은 이름 해시 테이블(`drgn_debug_info::modules`)과 시작 주소 트리(`modules_by_address`, splay tree)에 동시에 등록되어, 이름 조회(`drgn_module_find_by_name`, `debug_info.c:173`)와 주소 조회(`drgn_module_find_by_address`, `debug_info.c:182`)를 모두 지원한다.

### 디버그 정보가 모듈에 붙는 방식

한 모듈은 여러 ELF 파일을 가질 수 있다 (`debug_info.h:248`~`258`):

- `loaded_file` — 메모리에 매핑된 실제 바이너리 (주소/리로케이션 기준).
- `debug_file` — 디버그 심볼이 든 파일 (보통 `-dbg`/`-dbgsym` 별도 파일).
- `supplementary_debug_file` — `.gnu_debugaltlink`가 가리키는 공용 디버그 파일.
- `gnu_debugdata_file` — `.gnu_debugdata` 미니 심볼테이블.
- `split_dwarf_files` — split DWARF(.dwo) 핸들 테이블.

모듈에는 빌드 식별자가 따라붙는다: `build_id`(원시 바이트)와 `build_id_str`(16진 문자열, `debug_info.h:216`~`230`). 이 build-id는 로깅과 **디버그 파일 탐색의 핵심 키**다. 모듈별 DWARF/ORC/심볼 상태도 함께 보관된다: `struct drgn_module_dwarf_info dwarf`(`debug_info.h:261`), `orc`, `elf_symtab`, 그리고 `loaded_file_status`/`debug_file_status`(원함/찾음/없음 상태 머신).

디버그 파일이 채택되면 모듈은 `modules_pending_indexing` 단일 연결 리스트에 push되어(`debug_info.c:1578`) 나중에 DWARF 인덱싱 대상이 된다.

---

## 4. 디버그정보 찾기: 로컬 / build-id / debuginfod (+ 커널·Ubuntu 제약)

디버그 정보 탐색은 **finder(핸들러) 체인**으로 구현된다. `drgn_debug_info_init`이 두 개의 finder를 등록한다(`debug_info.c:5646`~`5669`):

1. `"standard"` finder = `drgn_standard_debug_info_find` (`debug_info.c:5648`) — 로컬 디스크 탐색.
2. `"debuginfod"` finder = `drgn_debuginfod_find` (`debug_info.c:5661`), `DRGN_HANDLER_REGISTER_ENABLE_LAST` 플래그로 **체인 맨 뒤(폴백)**에 배치(`debug_info.c:5668`). debuginfod 라이브러리가 있을 때만 등록된다(`drgn_have_debuginfod()`).

### 4.1 standard finder: 로컬 + build-id + debuglink

`drgn_module_try_standard_files`(`debug_info.c:2361`)가 모듈 하나에 대해 다음 순서로 시도한다:

1. **보충(supplementary) 파일** 먼저 (`debug_info.c:2376`).
2. **이미 연 파일 재사용** — loadable 디버그 파일을 loaded_file로 돌려쓰기 (`try_reuse`, `debug_info.c:2382`).
3. **확실한 경로** — vDSO는 코어 덤프에서 직접(`drgn_module_try_vdso_in_core`, `2419`), 살아있는 프로세스는 `/proc/$pid/...` 매직 심볼릭 링크(`drgn_module_try_proc_files`, `2423`).
4. **build-id 경로** — build-id를 이미 알면 먼저 시도(`2435`). `drgn_module_try_files_by_build_id`(`debug_info.c:2173`)는 `<debug_dir>/.build-id/<XX>/<나머지>.debug` 규약 경로를 만든다(`debug_info.c:2193`). 기본 `debug_dir`은 `/usr/lib/debug`(`debug_info_options.c:12`).
5. **커널 특수 경로** — 메인 모듈이 커널이면 `drgn_module_try_vmlinux_files`(`2445`), 로더블 모듈이면 `drgn_module_try_linux_kmod_files`(depmod 인덱스/디렉토리 워크, `2450`).
6. **모듈 이름이 경로면** 그 경로 시도(`2462`).
7. build-id를 늦게 얻었으면 다시 build-id 시도(`2473`).
8. **`.gnu_debuglink`** 따라가기(`drgn_module_try_files_by_gnu_debuglink`, `2480`). 기본 탐색 디렉토리는 `$ORIGIN`, `$ORIGIN/.debug`, `""`(`debug_info_options.c:17`).

이 동작들은 `struct drgn_debug_info_options`(`debug_info_options.h:24`)의 불리언/리스트 옵션으로 켜고 끌 수 있다: `directories`, `try_build_id`, `try_module_name`, `debug_link_directories`, `try_debug_link`, `try_procfs`, `try_embedded_vdso`, `try_reuse`, `try_supplementary`, `kernel_directories`, `try_kmod`(`debug_info_options.h:10`~`22`).

### 4.2 debuginfod: build-id로 자동 다운로드

debuginfod는 빌드 ID를 키로 디버그 정보를 HTTP로 내려주는 elfutils 서비스다. drgn은 libdebuginfod를 **dlopen으로 약하게 링크**하여(`DEBUGINFOD_SONAME`, `debug_info.c:108`) 함수 포인터 테이블(`debuginfod_begin`/`_find_debuginfo`/`_find_executable`/`_set_progressfn`, `debug_info.c:90`~`96`)을 채운다. 라이브러리가 없으면 `drgn_have_debuginfod()`가 false가 되어 finder 자체가 등록되지 않는다.

`drgn_debuginfod_find`(`debug_info.c:3119`)는 모듈마다:
- 클라이언트 세션을 lazy 생성(`debuginfod_begin`, `3127`)하고 진행률 콜백을 단다(`3132`).
- 캐시 경로를 `$DEBUGINFOD_CACHE_PATH` → `~/.debuginfod_client_cache` → `$XDG_CACHE_HOME` 순으로 결정(`3142`~`3165`).
- build-id 문자열을 키로 `debuginfod_find_debuginfo`(디버그) / `debuginfod_find_executable`(바이너리)를 호출해 fd를 받는다(`drgn_module_try_file_from_debuginfod`, `debug_info.c:3081`). 받은 fd는 `drgn_module_try_file`로 그대로 모듈에 붙인다(`3096`).
- 필요하면 `.gnu_debugaltlink` 보충 파일도 build-id로 다운로드(`3104`). `Ctrl-C` 취소(`drgn_cancel_debuginfod`)와 진행 바를 지원한다(`3045`~`3052`).

### 4.3 ★ 리눅스 커널의 debuginfod 제약 (Fedora 위주, Ubuntu 막힘)

`docs/getting_debugging_symbols.rst:88`~`96`에 따르면, **Fedora의 debuginfod 서버를 제외하면 커널 디버그 심볼 다운로드가 극도로 느려서**(업스트림에서 수정됐으나 아직 미배포) drgn 0.0.31부터는 **커널을 디버깅할 때 Fedora에서만 debuginfod를 쓴다**. Ubuntu/Debian/CentOS/openSUSE 문서 분기는 모두 "drgn will not use it for Linux kernel debugging symbols by default"라고 명시한다(예: Ubuntu `rst:406`).

이 정책은 코드로 강제된다. `drgn_program_finish_set_kernel`(`linux_kernel.c:722`)이 vmcoreinfo의 `osrelease`를 검사해, 커널 릴리스가 정규식 `/.fc[0-9]+(.|$)/`에 맞지 않으면(=Fedora 커널이 아니면) `"debuginfod"` finder를 **비활성화**한다:

```c
// linux_kernel.c:734
if (!is_fedora_kernel(prog->vmcoreinfo.osrelease)
    && drgn_handler_list_disable(&prog->dbinfo.debug_info_finders, "debuginfod"))
    drgn_log_debug(prog, "disabled debuginfod for Linux kernel");
```

즉 Ubuntu 커널(`...-generic`)을 분석하면 osrelease에 `.fcNN`이 없으므로 debuginfod 경로가 꺼지고, 사용자는 `linux-image-$(uname -r)-dbgsym`을 ddebs 저장소에서 수동 설치해야 한다(`rst:435`).

---

## 5. DWARF 파싱·인덱싱 흐름 (file:line)

### 5.1 왜 자체 인덱스인가 (.debug_names 미사용)

이름으로 타입/변수를 찾을 때마다 수십만 개의 DIE를 선형 탐색하면 너무 느리다. DWARF 표준은 `.debug_pubnames`/`.debug_names` 가속 섹션을 정의하지만, `dwarf_info.c:252`~`254`가 분명히 적는다: *"GCC and Clang currently don't emit them by default, so we don't use them."* 그래서 drgn은 **DIE 스캔에 특화된 자체 DWARF 파서로 이름→DIE 인덱스를 직접 구축**한다(`dwarf_info.c:248`~`250`, "bespoke DWARF parser ... highly optimized").

### 5.2 abbrev → 명령어 스트림 컴파일

DWARF abbrev 테이블은 인덱싱에 불필요한 정보까지 담고 있어 크다. drgn은 이를 **압축된 바이트코드(`enum drgn_dwarf_index_abbrev_insn`, `dwarf_info.c:270`)로 번역**한다 — `INSN_SKIP_*`(건너뛸 속성), `INSN_NAME_STRP4`/`STRX`(이름 추출), `INSN_SPECIFICATION_REF*`(정의↔선언 연결), `INSN_DECLARATION_FLAG` 등. 이 스트림은 캐시 친화적이라 타이트한 DIE 파싱 루프를 빠르게 만든다(`dwarf_info.c:261`~`268`). CU별 abbrev은 `read_cu`(`dwarf_info.c:1074`)에서 컴파일되어 `drgn_dwarf_index_cu::abbrev_insns`(`dwarf_info.c:130`)에 저장된다.

### 5.3 컴파일 단위(CU) 모델

각 CU는 `struct drgn_dwarf_index_cu`(`dwarf_info.c:77`)로 표현된다: 소속 파일, `.debug_info` 내 버퍼/길이, DWARF 버전/주소 크기/64비트 여부, abbrev 명령어, `.debug_str_offsets` 포인터 등. 모든 인덱싱된 CU는 `drgn_dwarf_info::index_cus` 벡터(`dwarf_info.h:197`)에 모이고, 버퍼 주소로 정렬된 `index_cu_lookup`(`dwarf_info.h:203`)로 DIE 주소→CU 역추적을 가능케 한다.

### 5.4 멀티스레드(OpenMP) 인덱싱 — `drgn_dwarf_index_update`

핵심 함수 `drgn_dwarf_index_update`(`dwarf_info.c:2078`)는 `modules_pending_indexing` 리스트를 벡터로 모은 뒤(`2090`~`2098`) `drgn_init_num_threads()`로 스레드 수를 정하고 `#pragma omp parallel num_threads(drgn_num_threads)`(`dwarf_info.c:2127`) 단일 병렬 영역에서 **여러 패스**를 수행한다. 스레드 0은 dbinfo의 구조를 직접 쓰고, 나머지 스레드는 자기만의 per-thread 구조에 쌓았다가 master 스레드가 merge한다(`2100`~`2120`).

병렬 패스(모두 `#pragma omp for schedule(dynamic)`):

1. **CU 열거** — 각 모듈 `debug_file`의 `.debug_info`/`.debug_types`에서 CU를 읽어 per-thread `cus`/`partial_units`에 적재(`drgn_dwarf_index_read_file`, `2151`). 이어 master가 dbinfo로 merge하고 `index_cu_lookup` 정렬 테이블을 갱신(`2167`~`2244`).
2. **abbrev 읽기** — 새 CU마다 `read_cu`로 abbrev을 명령어 스트림으로 컴파일(`2247`~`2256`).
3. **1차 패스** `index_cu_first_pass`(`2299`) — `DW_AT_specification`/`DW_AT_abstract_origin`/`DW_AT_import` 관계를 모아 `specifications` 맵(선언 DIE↔정의 DIE 연결, `dwarf_info.h:195`)을 만든다. per-thread 맵은 `2317`에서 merge.
4. **2차 패스** `index_cu_second_pass`(`2353`) — 실제로 DIE를 이름→주소로 색인. 인덱싱 대상 태그는 `DRGN_DWARF_INDEX_TAGS`(`dwarf_info.h:86`): struct/class/union/namespace/enum/typedef/enumerator/subprogram/variable/base_type. `index_die`(`dwarf_info.c:1506`)가 태그별 해시 맵 `drgn_dwarf_index_die_map`에 `(name → DIE 주소 리스트)`를 채운다. `base_type`은 전역에 하나뿐이라 별도 `base_types` 맵(`dwarf_info.h:189`)에 저장. per-thread 맵은 `2360`~`2377`에서 merge.

2차 패스에는 인덱스의 정확성을 좌우하는 몇 가지 특수 처리가 있다(`dwarf_info.c:1820`~`1875`, `dwarf_info.h:128`~`145`):
- **enumerator**: 열거자(enumerator)의 이름 항목은 그 자신이 아니라 **부모 `enumeration_type` DIE 주소**를 가리키게 저장한다(`die_addr = depth1_addr`, `1833`). 그래서 `MY_ENUM_VALUE`로 조회하면 그 값을 담은 enum 타입을 바로 얻는다.
- **선언(declaration) DIE**: `DW_AT_declaration` 플래그만 있고 본문이 없는 DIE는 보통 인덱스에서 제외한다. 단, 자식이 있는 class/struct/union 선언은 그 안에 중첩 정의를 담을 수 있으므로 **namespace처럼** 취급해 색인한다(`1837`~`1844`). 또 선언은 `drgn_dwarf_find_definition`으로 진짜 정의 DIE로 치환된다(`1845`, specifications 맵 활용).
- **partial/imported units**: `DW_TAG_partial_unit`(split DWARF/공용 정의)은 별도로 모아 인덱스 끝에 두고 직접 색인 대상에서 제외하며(`2164`~`2166`), `INSN_DIE_TAG_imported_unit`(`dwarf_info.c:330`)를 만나면 그 CU를 펼쳐 내용을 끌어온다.

에러는 OpenMP 병렬 영역을 break할 수 없으므로 `#pragma omp critical(drgn_dwarf_info_update_index_error)`로 첫 에러만 보존하고, 남은 루프를 no-op으로 만드는 트릭(`new_cus_size`를 옛 값으로 되돌림, `2211`)을 쓴다. 한 번 실패하면 `global.saved_err`(`dwarf_info.h:157`)에 박제되어 이후 호출은 같은 에러를 즉시 복제 반환한다(`dwarf_info.c:2084`).

### 5.5 네임스페이스 지연 인덱싱

전역 네임스페이스(`drgn_dwarf_info::global`, `dwarf_info.h:181`)는 로딩 즉시 인덱싱되지만, C++ 네임스페이스나 클래스 내부 정의는 **처음 접근될 때** 인덱싱된다(`dwarf_info.c:256`~`258`). 각 네임스페이스는 자식 네임스페이스 테이블과 태그별 DIE 맵을 갖는 `struct drgn_namespace_dwarf_index`(`dwarf_info.h:117`)로 트리를 이룬다. 공개 진입점 `drgn_dwarf_info_update_index`(`dwarf_info.c:2531`)는 사실상 `index_namespace(&global)`이다.

---

## 6. 상위 계층(type / object / symbol)으로의 노출

DWARF 인덱스는 세 개의 finder 콜백으로 추상화되어 상위 계층과의 경계를 형성한다(`debug_info.c:5631`~`5645`):

| finder | 이름 | 콜백 | 역할 |
|--------|------|------|------|
| type finder | `"dwarf"` | `drgn_debug_info_find_type` (`dwarf_info.c:6678`) | 이름→`struct drgn_qualified_type` |
| object finder | `"dwarf"` | `drgn_debug_info_find_object` (`dwarf_info.c:6731`) | 이름→`struct drgn_object` (변수/함수/상수) |
| symbol finder | `"elf"` | `elf_symbols_search` (`debug_info.c:5588`) | 이름/주소→ELF 심볼 |

**타입 조회 경로** (`drgn_debug_info_find_type`, `dwarf_info.c:6678`): 요청된 `kinds` 비트마스크를 DWARF 태그 배열로 변환(`6688`~`6701`) → `find_enclosing_namespace`로 네임스페이스 한정(`6704`) → `drgn_dwarf_index_iterator`로 해당 이름의 DIE를 순회(`6709`) → 파일명이 맞으면 `drgn_type_from_dwarf`(DIE → drgn 타입 객체로 변환)를 호출(`6717`). `drgn_type_from_dwarf`/`drgn_object_from_dwarf`(`dwarf_info.h:307`)가 DWARF DIE를 `type.c`/`object.c`가 다루는 in-memory 표현으로 물질화한다. 파싱된 타입은 `drgn_dwarf_info::types` 캐시(DIE 주소→`drgn_dwarf_type`, `dwarf_info.h:211`)에 보관되어 재방문을 피한다.

**객체 조회 경로** (`drgn_debug_info_find_object`, `dwarf_info.c:6731`): `flags`(상수/함수/변수)를 태그 배열로 바꿔(`6748`~`6753`) 인덱스를 순회하고, 찾은 DIE가 `DW_TAG_enumeration_type`이면 `drgn_object_from_dwarf_enumerator`로 열거자 상수를 만들고(`6765`~`6768`, §5.4의 enumerator→부모 트릭과 짝), 그 외에는 `drgn_object_from_dwarf`로 변수/함수 객체를 만든다(`6770`). 둘 다 없으면 `&drgn_not_found`를 돌려 다음 finder로 넘긴다.

**심볼 조회**는 `elf_symbols_search`(`debug_info.c:5588`)가 주소→모듈을 찾아 모듈별 `drgn_module_elf_symbols_search`(`elf_symtab.c:667`)에 위임한다. 심볼 소스 우선순위(`elf_symtab.c:237`~`243`): ① loaded/debug 파일의 `.symtab`(full) → ② `.dynsym` + `.gnu_debugdata`의 미니 symtab → ③ `.dynsym`만. full symtab 발견 여부는 `have_full_symtab`(`debug_info.h:281`)로 추적한다. 심볼 finder가 DWARF finder와 분리되어 있으므로, **DWARF가 없어도 `.symtab`만으로 주소↔이름 변환(스택 트레이스의 함수명 등)은 동작**한다 — 이것이 "타입은 없지만 심볼은 있는" 상태가 가능한 이유다.

상위에서 `drgn_load_debug_info`가 끝나면 곧바로 `drgn_dwarf_info_update_index`를 eager하게 호출해 인덱스를 갱신한다(`debug_info.c:5506`~`5508`).

---

## 7. 타 서브시스템 연결

- **program / memory** ([[02-program-memory]]) — 모듈 주소 범위와 build-id는 코어 덤프/`/proc/$pid/maps`에서 온다. 인덱싱된 타입으로 메모리 바이트를 해석하는 게 최종 목표다.
- **type / object 언어 계층** ([[04-type-language]]) — §6의 finder가 경계. DWARF DIE → `drgn_type`/`drgn_object` 변환이 `dwarf_info.c`의 `drgn_type_from_dwarf`에서 일어나고, 그 결과를 `type.c`/`object.c`/`language_c.c`가 소비한다.
- **linux kernel** ([[06-linux-kernel]]) — `linux_kernel.c`가 vmcoreinfo로 vmlinux/모듈 경로와 Fedora-debuginfod 정책(§4.3)을 정한다. ORC 언와인더(`orc_info.c`)는 커널 전용 스택 언와인드 소스.
- **stack unwinding / CFI** — `.eh_frame`/`.debug_frame`(`dwarf_info.c`의 CFI)과 ORC가 `cfi.c`/`stack_trace.c`로 연결된다(`debug_info.h:414` `drgn_module_find_cfi`).

---

## 8. 파일 맵

| 파일 | 줄 수 | 역할 |
|------|-------|------|
| `libdrgn/debug_info.h` | 430 | `drgn_debug_info`/`drgn_module` 모델, finder·옵션 구조 정의 |
| `libdrgn/debug_info.c` | 5836 | 모듈 관리, 파일 탐색(로컬/build-id/debuginfod), finder 등록, 심볼 검색 진입 |
| `libdrgn/dwarf_info.h` | 336 | DWARF 인덱스 자료구조(CU/네임스페이스 인덱스/태그), 공개 API 선언 |
| `libdrgn/dwarf_info.c` | 7797 | abbrev→명령어 컴파일, OpenMP 멀티패스 인덱싱, type/object finder, CFI |
| `libdrgn/elf_file.{c,h}` | 868/285 | ELF 파일 래퍼, 섹션→인덱스 매핑, libdw 핸들, 리로케이션 |
| `libdrgn/elf_symtab.{c,h}` | 706 | `.symtab`/`.dynsym`/`.gnu_debugdata` 심볼 테이블 파싱·검색 |
| `libdrgn/debug_info_options.{c,h}` | 327/40 | 디버그 정보 탐색 옵션(디렉토리/build-id/debuglink/kmod) |
| `libdrgn/orc_info.c` | — | 리눅스 커널 ORC 언와인더(`.orc_unwind*`) |
| `libdrgn/linux_kernel.c` | — | 커널 vmlinux/모듈 경로, Fedora-only debuginfod 정책(`is_fedora_kernel`) |
| `libdrgn/build-aux/gen_elf_sections.py` | 159 | "중요 ELF 섹션" 목록의 single source of truth (BTF 없음) |
| `docs/getting_debugging_symbols.rst` | 596 | 사용자 관점: 배포판별 심볼 설치·debuginfod 제약(커널/Ubuntu) |

---

### 핵심 결론

1. **drgn의 타입 정보 원천은 DWARF가 전부다.** ELF `.symtab`(심볼)과 `.eh_frame`/`.debug_frame`/ORC(언와인드)를 보조로 쓰지만, 타입·구조체 레이아웃은 오직 DWARF에서 온다.
2. **★ `.BTF`는 타입 소스가 아니다.** `libdrgn/`에 BTF 파서가 0줄이며, `gen_elf_sections.py`의 로드 섹션 목록에도 없다. BTF는 `drgn/helpers/linux/bpf.py`에서 커널 자료구조(데이터)로만 순회된다.
3. **debuginfod는 dlopen 기반 폴백 finder**이며, 리눅스 커널에 대해서는 `is_fedora_kernel()` 검사로 **Fedora에서만 활성**(Ubuntu 등은 코드에서 비활성화).
4. **인덱싱은 자체 구축**이다(`.debug_names` 미사용). abbrev을 바이트코드로 컴파일하고 OpenMP로 CU 열거→abbrev→specification→DIE 색인의 멀티패스를 병렬 수행한다.
