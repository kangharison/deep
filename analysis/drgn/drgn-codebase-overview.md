# drgn 코드베이스 전체 구조 — 종합 개요 (entry point)

> **대상**: drgn (osandov/drgn, GitHub `main`, 0.2.0+) — 프로그래머블 라이브 커널/프로세스 디버거 (Omar Sandoval, Meta)
> **소스**: 로컬 `deep/sources/drgn` (shallow clone, commit `f800e5f`)
> **이 문서**: 8개 서브시스템 상세 분석의 진입점/지도. 각 절은 해당 상세 문서로 연결된다.
> **상세 문서**: [[02-program-memory]] · [[03-debuginfo-dwarf]] · [[04-type-language]] · [[05-object]] · [[06-linux-kernel]] · [[07-stack-arch]] · [[08-python-c-binding]] · [[09-python-helpers-cli]]
> **응용(실전)**: [[drgn-live-blkmq-nvme-pcie-prp]] (이 머신에서 blk-mq/NVMe/PCIe/PRP 라이브 관찰)

---

## 0. 한 문단 요약

drgn은 **"실행 중인 커널/프로세스의 메모리를, 멈추지 않고, 타입을 알고 읽는다"** 를 목표로 한 디버거다. 핵심 아이디어는 **자료구조를 Python 객체처럼 탐색**(`prog["init_task"].comm.string_()`)하게 만드는 것. 이를 위해 세 정보원 — **메모리(어디서 바이트를 읽나)**, **타입(구조체 레이아웃)**, **심볼(이름↔주소)** — 을 `drgn_program` 한 객체에 묶고, 무거운 작업(DWARF 인덱싱, 메모리 읽기, 객체 평가, 스택 언와인딩)은 **C(libdrgn)** 로, 사용자 편의(헬퍼·CLI·REPL)는 **Python** 으로 구현한다. 타입 정보는 **DWARF 전용**이며 **BTF는 쓰지 않는다**(이 한 가지가 실무에서 가장 자주 발목 잡는 사실 — §4 참조).

---

## 1. 3계층 아키텍처

```
┌──────────────────────────────────────────────────────────────────────────┐
│  (C) drgn/  — 순수 Python 레이어  (~32.5k 줄)            [[09-python-helpers-cli]] │
│     helpers/{common,linux,experimental}  CLI(cli.py)  REPL  crash 호환 명령  │
│     "정책·편의·스크립팅". helper(prog)→drgn Object 반환, 체이닝.              │
└───────────────────────────────┬──────────────────────────────────────────┘
                                │  import _drgn  (CPython 확장 경계)
┌───────────────────────────────┴──────────────────────────────────────────┐
│  (B) libdrgn/python/  — C↔Python 바인딩  (~11.5k 줄)     [[08-python-c-binding]] │
│     PyTypeObject(Program/Object/Type/Symbol/StackTrace/Module...)           │
│     소유권/refcount, drgn_error→Python 예외 변환, 핫패스 C 헬퍼            │
└───────────────────────────────┬──────────────────────────────────────────┘
                                │  C API (drgn.h)
┌───────────────────────────────┴──────────────────────────────────────────┐
│  (A) libdrgn/  — C 코어 엔진  (~41.7k 줄)                                    │
│                                                                            │
│   ┌─ Program 허브 [[02-program-memory]] ─ struct drgn_program: 모든 걸 묶음   │
│   │     ├─ Memory reader: 가상/물리 splay 트리, 4 백엔드(core/kcore/pid/qemu)│
│   │     ├─ Debug info  [[03-debuginfo-dwarf]] ─ DWARF 파싱·인덱싱, 모듈, debuginfod│
│   │     ├─ Type system [[04-type-language]] ─ drgn_type 12종, C 렉서/파서      │
│   │     ├─ Object      [[05-object]] ─ value/reference/absent, 멤버·캐스트·산술│
│   │     ├─ Symbols ─ ELF symtab / kallsyms 인덱스                            │
│   │     ├─ Stack/arch  [[07-stack-arch]] ─ DWARF CFI vs ORC, 플랫폼 후크, QMP  │
│   │     └─ Linux 커널  [[06-linux-kernel]] ─ vmcoreinfo, 페이지테이블, 모듈, C헬퍼│
│   └─ 인프라: hash_table, vector, splay/bst, binary_buffer, error, log, 코드젠 │
└────────────────────────────────────────────────────────────────────────────┘
```

| 계층 | 디렉토리 | 규모 | 역할 | 상세 |
|------|---------|------|------|------|
| A. C 코어 | `libdrgn/*.c` | 41.7k 줄 | 디버깅 엔진 전체 | §3 |
| B. 바인딩 | `libdrgn/python/*.c` | 11.5k 줄 | libdrgn → `_drgn` CPython 확장 | [[08-python-c-binding]] |
| C. Python | `drgn/**.py` | 32.5k 줄 | helpers·CLI·명령 | [[09-python-helpers-cli]] |

---

## 2. 중심 객체: `struct drgn_program`

drgn의 모든 것은 `drgn_program`(`libdrgn/program.h:62`) 한 객체에 매달린다. 거의 모든 C API의 첫 인자이고, Python `Program` 객체가 이걸 값으로 임베드해 **소유**한다([[08-python-c-binding]]).

| Program이 묶는 것 | 내용 | 상세 문서 |
|------------------|------|----------|
| 메모리 리더 | 가상/물리 주소공간 splay 트리(세그먼트별 read 콜백) | [[02-program-memory]] |
| 디버그 정보 | `dbinfo` + finder 체인(DWARF/ELF/debuginfod) | [[03-debuginfo-dwarf]] |
| 타입 캐시 | `dedupe_types`/`created_types`/`primitive_types`/`members` | [[04-type-language]] |
| 심볼 finder | ELF symtab, kallsyms | [[03-debuginfo-dwarf]] · [[06-linux-kernel]] |
| 플랫폼 | arch(x86_64/aarch64/...), 레지스터·CFI·페이지테이블 후크 | [[07-stack-arch]] |
| 언어 | 기본 C (포매팅·연산자·파서 콜백 테이블) | [[04-type-language]] |
| 스레드/플래그 | `ProgramFlags`(IS_LINUX_KERNEL/IS_LIVE/IS_LOCAL) | [[02-program-memory]] |

`ProgramFlags` 조합이 "이 타깃이 무엇인가"(라이브 커널 / 커널 덤프 / 라이브 프로세스 / 코어덤프)를 정하고, 언어 기본값·검색 경로·페이지테이블 변환 사용 여부를 가른다.

---

## 3. 데이터 흐름 한 바퀴 — `prog["init_task"].comm.string_()`

drgn의 한 줄이 8개 서브시스템을 어떻게 관통하는지가 곧 전체 구조다.

```
prog["init_task"]
  │  ① 심볼 조회: "init_task" → 주소         (kallsyms 또는 ELF symtab)   [[06-linux-kernel]]/[[03-debuginfo-dwarf]]
  │  ② 타입 조회: init_task의 DWARF DIE → struct task_struct 타입 물질화  [[03-debuginfo-dwarf]]→[[04-type-language]]
  │  ③ Object 생성: REFERENCE 객체 (주소만 보유, 아직 메모리 안 읽음)     [[05-object]]
  ▼
.comm
  │  ④ 멤버 접근: task_struct 안 comm의 bit_offset 계산 → 새 REFERENCE     [[05-object]] + [[04-type-language]](멤버 오프셋)
  ▼
.string_()
  │  ⑤ 물질화: reference 주소에서 실제 바이트 읽기 요청                    [[05-object]] drgn_object_read_reference
  │  ⑥ 메모리 읽기: 가상주소 → 물리 변환(페이지테이블 워킹) → 바이트       [[02-program-memory]]→[[06-linux-kernel]] read_memory_via_pgtable
  │  ⑦ 백엔드: /proc/kcore(라이브) pread / vmcore / kdump / QMP            [[02-program-memory]]
  ▼
b"swapper/0"
```

> 핵심 통찰: **reference 객체는 메모리를 읽지 않는다.** `init_task`를 잡고 `.comm`까지 멤버를 타고 들어가도 바이트는 안 읽힌다. `string_()`/`read_()`/스칼라 변환 같은 **물질화 시점에만** `drgn_program_read_memory`가 호출된다([[05-object]] §3). 그래서 거대한 구조체를 가볍게 탐색할 수 있다.

---

## 4. ★ 가장 중요한 횡단 사실들 (오해 방지)

### 4.1 drgn은 DWARF만 쓴다 — **BTF는 타입 소스가 아니다**
[[03-debuginfo-dwarf]] §2에서 코드로 3중 확정:
- 인덱싱 대상 ELF 섹션 목록(`libdrgn/build-aux/gen_elf_sections.py`)에 `.BTF` 없음 — `.debug_info/.debug_abbrev/...`(DWARF) + `.symtab`(ELF 심볼) + `.eh_frame/.orc_unwind`(언와인드)만.
- `libdrgn/` C 코드에 `BTF` grep 0건.
- BTF가 등장하는 유일한 곳은 `drgn/helpers/linux/bpf.py` — 타깃 커널의 `struct btf`를 **데이터로 순회**하는 헬퍼일 뿐, drgn 자신의 타입 소스가 아니다(그 레이아웃조차 DWARF로 안다).

→ **함의**: `/sys/kernel/btf/vmlinux`가 있어도 drgn은 쓰지 못한다. **DWARF(vmlinux 디버그심볼)가 반드시 필요**하다. Ubuntu는 vmlinux를 stripped로 배포하므로 `linux-image-$(uname -r)-dbgsym`(ddebs 저장소)을 설치해야 한다.

### 4.2 debuginfod는 비-Fedora 커널엔 비활성
[[06-linux-kernel]] §8 / [[03-debuginfo-dwarf]] §4: `is_fedora_kernel()`(`linux_kernel.c:706-720`)이 OSRELEASE에 `.fcNN.` 패턴이 없으면 커널 디버그정보용 **debuginfod finder를 끈다**(`linux_kernel.c:737`). Ubuntu 서버가 커널 DWARF를 잘 안 주기 때문. → **Ubuntu/Debian에선 debuginfod 자동 다운로드가 안 되고, dbgsym 수동 설치가 사실상 유일.**

### 4.3 메모리 리더 = 가상/물리 두 트리 + 4 백엔드
[[02-program-memory]]: 백엔드(core dump·/proc/kcore·/proc/pid/mem·QEMU QMP)의 차이는 "어떤 read 콜백을 단 세그먼트를 등록하나"로 캡슐화된다. `physical=True`(예: [[drgn-live-blkmq-nvme-pcie-prp]]의 PRP 페이로드 읽기)는 read 경로에 분기가 없고 **물리 트리를 고를 뿐**, 가상→물리 변환은 가상 catch-all 세그먼트에 깔린 페이지테이블 워킹 콜백이 lazy하게 수행한다.

### 4.4 언와인더가 둘 — DWARF CFI vs 커널 ORC
[[07-stack-arch]]: 사용자 공간은 `.eh_frame/.debug_frame`(DWARF CFI), **리눅스 커널은 ORC**(`.orc_unwind*`, objtool 생성, 6바이트 고정 엔트리). 둘 다 동일한 `drgn_cfi_row`로 번역돼 복원 로직은 출처를 모른다. 커널이 `.eh_frame`을 끄고 ORC를 쓰는 이유까지 [[07-stack-arch]] §3에 정리.

---

## 5. 메모리 소스(백엔드) 한눈 표

| 백엔드 | 진입 API | 메모리 경로 | 타입소스 필요 | 비고 |
|--------|---------|------------|--------------|------|
| 라이브 커널 | `program_from_kernel()` / `-k` | `/proc/kcore` pread | DWARF(vmlinux dbgsym) | root 필요, 비침습 |
| 커널 코어덤프 | `program_from_core_dump()` / `-c` | ELF vmcore | DWARF | crash가 만든 덤프 |
| kdump 압축덤프 | `program_from_core_dump()` | libkdumpfile `kdump_read` | DWARF | makedumpfile 압축본 |
| 라이브 프로세스 | `program_from_pid()` / `-p` | `/proc/pid/mem` | DWARF(바이너리+라이브러리) | userspace |
| QEMU VM | `--qemu` | QMP `gpa2hva`+`process_vm_readv` | DWARF | 게스트 메모리 |

상세: [[02-program-memory]] §4, [[06-linux-kernel]] §3, [[07-stack-arch]] §6.

---

## 6. 디렉토리 지도 (어디에 뭐가 있나)

```
drgn/
├── libdrgn/                     ← (A) C 코어 엔진
│   ├── program.c/.h             Program 허브               [[02-program-memory]]
│   ├── memory_reader.c, io.c    메모리 읽기 추상화          [[02-program-memory]]
│   ├── debug_info.c, dwarf_info.c, elf_file.c, elf_symtab.c  디버그정보/DWARF  [[03-debuginfo-dwarf]]
│   ├── type.c, language_c.c, language.c     타입 + C 파서   [[04-type-language]]
│   ├── object.c, accessors.c, lazy_object.c  Object 시스템  [[05-object]]
│   ├── linux_kernel*.c, kallsyms.c, kdump.c  커널 지원      [[06-linux-kernel]]
│   ├── stack_trace.c, cfi.c, orc_info.c, register_state.c   언와인딩  [[07-stack-arch]]
│   ├── arch_*.c (+ *_defs.py), platform.c   아키텍처        [[07-stack-arch]]
│   ├── qemu_machine_protocol.c  QEMU QMP                    [[07-stack-arch]]
│   ├── symbol.c                 심볼 인덱스
│   ├── {hash_table,vector,splay_tree,binary_buffer,...}     인프라 자료구조
│   ├── build-aux/*.py, *.strswitch, *_defs.py               코드 생성
│   └── python/                  ← (B) C↔Python 바인딩       [[08-python-c-binding]]
│       └── {program,object,type,module,main,...}.c
├── drgn/                        ← (C) 순수 Python           [[09-python-helpers-cli]]
│   ├── __init__.py, cli.py, internal/repl.py
│   ├── helpers/{common,linux,experimental}/
│   └── commands/{_crash,_builtin}/
├── _drgn.pyi                    공개 API 타입스텁(141KB) — API 계약서
├── docs/                        공식 RST 문서
├── tests/, vmtest/              테스트(실제 커널 부팅 테스트 포함)
└── contrib/                     예제 스크립트
```

---

## 7. 읽는 순서 (학습 동선 추천)

1. **이 문서(개요)** → 큰 그림.
2. [[02-program-memory]] — 모든 게 매달리는 Program 허브부터.
3. [[05-object]] + [[04-type-language]] — 사용자가 실제로 만지는 객체/타입(드gn을 "쓰는" 감각).
4. [[03-debuginfo-dwarf]] — 타입이 어디서 오나(DWARF/디buginfod/모듈).
5. [[06-linux-kernel]] — 커널 학습자의 본진(vmcoreinfo/페이지테이블/모듈/kallsyms).
6. [[07-stack-arch]] — 백트레이스와 arch 추상화.
7. [[08-python-c-binding]] → [[09-python-helpers-cli]] — C가 어떻게 Python이 되고, 헬퍼/CLI가 어떻게 얹히나.
8. 실전: [[drgn-live-blkmq-nvme-pcie-prp]] — 이 머신에서 직접 blk-mq/NVMe/PCIe/PRP 관찰.

---

## 8. 한 줄 결론

drgn = **(DWARF 타입) × (다중 백엔드 메모리 리더) × (lazy Object 모델)** 을 `drgn_program` 허브로 묶고, 무거운 건 C로·편한 건 Python으로 가른 디버거. 커널 학습 관점에선 [[06-linux-kernel]](vmcoreinfo·페이지테이블·모듈)과 [[09-python-helpers-cli]](linux 헬퍼·crash 호환·kmodify)가 가장 실전적이며, 가장 흔한 함정은 **"BTF로는 안 되고 DWARF(dbgsym)가 필요하다"**(§4.1) 는 점이다.
