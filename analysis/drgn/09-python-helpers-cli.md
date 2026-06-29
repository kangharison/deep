# drgn 코드 분석 — Python 레이어 (helpers · CLI · crash 호환 · kmodify)
> 소스: osandov/drgn (main) — 로컬 `deep/sources/drgn`
> 관련: [[drgn-codebase-overview]] · [[08-python-c-binding]] · [[06-linux-kernel]]

drgn은 "C 확장(`_drgn`)이 메모리/타입/DWARF를 읽는 엔진"이고, 그 위에 얇은 **순수
Python 레이어**가 올라간다. 이 문서는 그 Python 레이어 — 디버깅 헬퍼(`drgn.helpers`),
대화형 CLI/REPL(`drgn.cli`, `drgn.internal.repl`), crash 유틸 호환 명령(`drgn.commands`),
그리고 라이브 커널 패치(`drgn.helpers.experimental.kmodify`) — 를 분석한다.

---

## 1. 역할

C 확장은 "한 줄짜리 원시 연산"만 제공한다: `prog["init_task"]`(전역 변수 읽기),
`prog.type("struct task_struct")`(타입 조회), `Object.member_()`, `container_of()`,
`prog.read()`. 이걸로 커널을 디버깅하려면 매번 `list_head.next`를 따라가고
`container_of`로 구조체를 복원하는 보일러플레이트를 손으로 짜야 한다.

Python 레이어는 이 보일러플레이트를 **재사용 가능한 헬퍼**로 캡슐화한다.
커널 소스의 `for_each_xxx()` / `list_for_each_entry()` 매크로를 Python 제너레이터로
1:1 대응시켜, 커널 개발자가 익숙한 코드를 그대로 Python으로 옮길 수 있게 한다
(`drgn/helpers/linux/__init__.py:16-31`의 docstring이 이 대응 관계를 명시).

추가로 두 가지 "프런트엔드"를 제공한다.
- **drgn CLI/REPL**: `prog`와 모든 헬퍼가 자동 주입된 Python 인터프리터.
- **crash 호환 레이어**: 기존 `crash` 유틸리티 사용자를 위해 `bt`/`ps`/`kmem`/`runq`
  같은 명령을 drgn 위에 재구현 — 익숙한 명령어로 입문하고 점진적으로 Python으로 넘어가게.

---

## 2. 헬퍼 프로그래밍 모델 (`takes_program_or_default`, 암묵 prog 주입)

drgn의 모든 객체는 특정 `Program`(주소 공간)에 묶여 있다. 헬퍼가 커널 메모리를
읽으려면 어떤 `prog`인지 알아야 한다. 그런데 매번 `list_for_each_entry(prog, ...)`로
첫 인자에 `prog`를 넘기는 건 번거롭다. 그래서 drgn은 **"기본 프로그램 인자(default
program)"** 개념과, 그것을 투명하게 주입하는 데코레이터 두 개를 둔다
(`drgn/helpers/common/prog.py`).

### 2.1 `takes_program_or_default` (`prog.py:66`)

원본 함수의 첫 파라미터가 반드시 `prog`여야 한다(`prog.py:87`에서 검사, 아니면
`TypeError`). 데코레이터는 `inspect.signature`로 두 번째 위치 파라미터 이름(`param1`)도
파악한 뒤, 호출 시점에 첫 인자의 타입을 보고 분기한다(`prog.py:110-122`):

```python
if args:
    if isinstance(args[0], Program):      # ① helper(prog, ...) → 그대로 전달
        return f(*args, **kwds)
    elif isinstance(args[0], Object):     # ② helper(obj, ...) → obj.prog_ 주입
        return f(args[0].prog_, *args, **kwds)
elif "prog" in kwds: ...                  # ③ 키워드로 준 경우
...
return f(get_default_prog(), *args, **kwds)  # ④ 아무것도 없으면 기본 prog
```

핵심은 **②**다. drgn `Object`는 자기가 속한 `Program`을 `obj.prog_` 속성으로 안다.
따라서 `slab_object_info(some_obj)`처럼 Object 하나만 넘기면, 데코레이터가
`some_obj.prog_`를 꺼내 `prog`로 자동 채운다. **④**는 CLI에서 `set_default_prog(prog)`로
등록해 둔 전역 기본 프로그램(`get_default_prog()`)을 쓴다 — 그래서 REPL에서
`for_each_task()`처럼 인자 없이도 동작한다.

결과적으로 한 헬퍼가 세 가지 호출 형태를 모두 받는다:
`helper(prog)` · `helper(obj)` · `helper()`. (`prog.py:42-49`의 `TakesProgramOrDefault`
Protocol이 이 오버로드를 타입으로 표현.)

### 2.2 `takes_object_or_program_or_default` (`prog.py:130`)

원본 시그니처가 `(prog, obj: Optional[Object], ...)`인 헬퍼용. `prog`만, 또는
`obj`만, 또는 둘 다 생략을 받는다. 예: `find_task(prog, pid)` / `find_task(ns_obj, pid)` /
`find_task(pid)`. Object를 받으면 그것이 `prog` 자리(`namespace` 등 컨텍스트)인지
판단해 `obj.prog_`로 prog를 채우고 `obj`를 두 번째 파라미터로 전달한다
(`prog.py:234-245`). 위치 인자에 기본값이 있으면 모호성 때문에 금지(`prog.py:201-204`).

### 2.3 왜 헬퍼는 drgn `Object`를 반환하나

헬퍼는 Python `int`/`str`이 아니라 **drgn `Object`** 를 반환하는 게 기본이다.
이유:
- `Object`는 *타입 정보 + 주소 + 지연 평가*를 모두 담는다. `list_first_entry()`가
  반환한 `task_struct *`는 단순 정수가 아니라, 이어서 `.comm`/`.pid`로 멤버 접근,
  `container_of`, `format_()` 출력이 가능한 살아있는 핸들이다.
- 값을 미리 다 읽지 않고 **lazy**하게 다룬다. 순회 중 실제로 건드린 필드만 메모리를
  읽으므로 대용량 커널 자료구조 순회가 효율적이다.
- 모든 `Object`가 `prog_`를 들고 다니므로 §2.1의 암묵 주입이 체인처럼 이어진다
  (한 헬퍼의 출력 Object를 다음 헬퍼에 그대로 입력).

(`drgn/helpers/__init__.py:15`의 `ValidationError`: `validate_*` 류 "검증자" 헬퍼가
자료구조 불변식 위반 시 던지는 전용 예외. 일반 헬퍼는 자료구조가 유효하다고 가정.)

---

## 3. common / linux / experimental 구분 + linux 헬퍼 범주 표

| 패키지 | 위치 | 대상 | 안정성 |
|--------|------|------|--------|
| `helpers.common` | `drgn/helpers/common/` | **프로그램 무관** — 유저스페이스/커널/코어덤프 공통 | 안정 |
| `helpers.linux` | `drgn/helpers/linux/` | **리눅스 커널** 전용 자료구조·서브시스템 | 안정 |
| `helpers.experimental` | `drgn/helpers/experimental/` | 불안정·위험(라이브 변경 등) | 실험적 |

- **common** (`drgn/helpers/common/`): `prog`(§2 데코레이터), `format`(ASCII/문자열
  이스케이프), `type`(타입 멤버/enum 헬퍼), `memory`(`identify_address`,
  `print_annotated_memory` — 주소가 심볼/슬랩/페이지/태스크 중 무엇인지 식별), `stack`
  (스택 주석). `common/__init__.py:25-30`은 서브모듈 `__all__`을 모아 패키지 톱레벨로
  재노출.
- **linux**: 커널 자료구조마다 모듈 하나. `linux/__init__.py:38-43`이 동일하게 톱레벨 재노출.
- **experimental**: 현재 `kmodify` 하나(§7).

### linux 헬퍼 범주 한눈 표 (대표 모듈)

| 범주 | 모듈 | 다루는 커널 자료구조 / 대표 헬퍼 |
|------|------|----------------------------------|
| 자료구조 — 리스트 | `list.py`, `llist.py`, `plist.py`, `list_nulls.py` | `struct list_head`/`hlist_head` 순회, `list_for_each_entry` |
| 자료구조 — 트리/맵 | `rbtree.py`, `xarray.py`, `radixtree.py`, `idr.py`, `mapletree.py` | 레드블랙트리, XArray, IDR(정수→포인터), 메이플트리 |
| 자료구조 — 비트/마스크 | `bitops.py`, `bitmap.py`, `cpumask.py`, `nodemask.py`, `sbitmap.py` | CPU 마스크 순회 `for_each_online_cpu` |
| 메모리(MM) | `mm.py`(2066줄), `mmzone.py`, `hugetlb.py`, `swap.py`, `vmstat.py` | `struct page`/pfn↔가상주소 변환, 존(zone), 페이지 워크 |
| 슬랩 할당자 | `slab.py` | `for_each_slab_cache`, `slab_object_info`(주소→슬랩 객체) |
| 퍼CPU | `percpu.py` | `per_cpu_ptr` 등 퍼CPU 변수 접근 |
| 프로세스/PID | `pid.py`, `sched.py`, `kthread.py` | `find_task`, `for_each_task`, `task_state_to_char` |
| 시그널/대기 | `signal.py`, `wait.py`, `swait.py`, `completion.py` | 시그널, wait queue 순회 |
| 파일시스템(VFS) | `fs.py`, `sysfs.py`, `kernfs.py` | 마운트/dentry/inode, `path_lookup` |
| 블록 | `block.py` | `struct gendisk`/파티션(`block_device`) 순회 |
| 네트워크 | `net.py`, `tcp.py`, `tc.py` | netdev, 소켓, TCP 상태 |
| 디바이스/버스 | `device.py`, `pci.py`, `module.py`, `irq.py` | `struct device`, PCI, 모듈, IRQ |
| 시간/타이머 | `timer.py`, `timekeeping.py` | 타이머 휠, hrtimer |
| IPC/cgroup | `ipc.py`, `cgroup.py` | SysV IPC, cgroup 계층 |
| 동기화/락 | `locking.py` | 락 소유자 분석 |
| BPF | `bpf.py` | BPF 맵/프로그램 |
| 로그/부트 | `printk.py`, `boot.py`, `panic.py`, `kconfig.py`, `kallsyms.py` | dmesg(`get_dmesg`), vmcoreinfo, kallsyms |
| 스택뎁 | `stack.py`, `stackdepot.py` | 커널 스택 추적 |

---

## 4. 대표 헬퍼 구현 뜯어보기 (file:line)

### 4.1 `list_for_each_entry` — `container_of` + next 포인터 순회

리눅스의 이중 연결 리스트는 구조체 *안에* `struct list_head`를 박아두는 침투형이다.
헬퍼는 두 단계로 분해된다.

`list_for_each` (`list.py:144-156`) — 순수 노드 순회:
```python
head = head.read_()          # head를 한 번만 읽어 캐시 (반복 메모리 접근 절감)
pos = head.next.read_()       # 첫 노드 = head->next
while pos != head:            # 원형 리스트라 head로 돌아오면 종료
    yield pos                 # struct list_head * 를 내보냄
    pos = pos.next.read_()    # 다음 노드로 전진
```

`list_for_each_entry` (`list.py:172-185`) — 노드를 "포함 구조체"로 복원:
```python
type = head.prog_.type(type)             # "struct task_struct" → drgn Type
for pos in list_for_each(head):          # 위 노드 순회를 재사용
    yield container_of(pos, type, member)  # list_head 주소 - 멤버 오프셋 = 구조체 주소
```

`container_of`(C 확장 제공)는 멤버 오프셋만큼 포인터를 빼서 바깥 구조체 포인터를
만든다 — 커널 `container_of` 매크로의 drgn 판본. `list_first_entry`(`list.py:84`),
`list_next_entry`(`list.py:130`)도 같은 `container_of` 패턴.

추가로 `validate_list_for_each` (`list.py:229-264`)는 **Brent의 사이클 탐지
알고리즘**으로 손상된 리스트(끊긴 prev 포인터, 순환)를 잡아내며 순회 —
크래시 덤프의 깨진 자료구조 디버깅용. 위반 시 `ValidationError`.

### 4.2 `for_each_task` — `init_pid_ns`에서 시작하는 전체 태스크 순회

`pid.py:91-113`. `@takes_object_or_program_or_default`라서 `for_each_task()` /
`for_each_task(prog)` / `for_each_task(ns)` 모두 가능.

```python
PIDTYPE_PID = prog["PIDTYPE_PID"].value_()         # enum 값을 커널에서 직접 읽음
for pid in for_each_pid(prog if ns is None else ns): # ① 모든 struct pid 순회
    task = pid_task(pid, PIDTYPE_PID)               # ② pid → task_struct (C 헬퍼)
    if task:
        yield task
```

①의 `for_each_pid` (`pid.py:51-73`)는 네임스페이스의 `init_pid_ns`(없으면
`prog["init_pid_ns"]`)에서 출발한다. 최신 커널은 PID를 **IDR**로 관리하므로
`idr_for_each(ns.idr)`로 순회(`pid.py:63-65`); 구버전 커널은 `pid_hash`
해시테이블을 `hlist_for_each_entry`로 순회하는 폴백 경로를 둔다(`pid.py:66-73`).
②의 `pid_task`/`find_task`는 C 확장(`_drgn._linux_helper_*`)을 직접 임포트
(`pid.py:14-18`) — 성능이 중요한 핫패스라 C로 구현.

`for_each_task_in_group` (`pid.py:116-135`)은 `task.signal.thread_head` 리스트를
`list_for_each_entry`로 돌아 한 스레드 그룹(유저 관점의 "프로세스의 모든 스레드")을 낸다 —
§4.1 헬퍼가 §4.2에서 재사용되는 합성 패턴.

---

## 5. CLI/REPL: prog 생성 + 네임스페이스 자동 주입

진입점은 `drgn/cli.py:_main()` (`cli.py:456`). argparse로 옵션을 파싱한 뒤
`prog = drgn.Program(...)`를 만들고(`cli.py:678`), **프로그램 소스**에 따라 한 번만 바인딩:

| 옵션 | 동작 | cli.py |
|------|------|--------|
| `-c/--core PATH` | `prog.set_core_dump(path)` — 코어덤프/vmcore | `:680-681` |
| `-p/--pid PID` | `prog.set_pid(pid)` — 라이브 유저 프로세스(ptrace) | `:682-688` |
| `--qemu ADDR` | `prog.set_qemu_qmp(addr)` — QEMU 게스트(QMP) | `:689-690` |
| `-k/--kernel`(기본) | `_set_kernel_with_sudo_fallback` → `prog.set_kernel()` | `:691-692` |

기본값은 **라이브 커널**이다. `set_kernel()`이 권한 오류면 `/proc/kcore`를 sudo로 다시
열어 코어덤프로 붙는다(`cli.py:165-174`). `-s`/`--symbols` 등으로 디버그심볼을 로드한 뒤
(`_load_debugging_symbols`, `cli.py:364`), 대화형이면 `run_interactive(prog)`,
스크립트면 `runpy.run_path(..., init_globals={"prog": prog})`(`cli.py:701-722`).

### 5.1 네임스페이스 자동 주입 — `default_globals` (`cli.py:138-162`)

REPL/스크립트에 넘길 전역 dict를 만든다:
1. `prog` 자신 + `drgn` 모듈.
2. `_DRGN_GLOBALS`(`cli.py:32-52`)의 핵심 심볼 — `Object`, `NULL`, `cast`,
   `container_of`, `sizeof`, `offsetof`, `stack_trace`, `search_memory*` 등 —
   을 `drgn`에서 꺼내 톱레벨로.
3. `drgn.helpers.common`의 모든 `__all__`을 주입(`cli.py:155-157`).
4. **커널이면** `drgn.helpers.linux`의 모든 `__all__`도 주입(`cli.py:158-161`,
   `prog.flags & IS_LINUX_KERNEL` 검사). 그래서 커널 디버깅 시에만
   `for_each_task`/`list_for_each_entry`가 이름만으로 바로 보인다.

`run_interactive` (`cli.py:816`)는 이 globals로 REPL을 띄우면서 (a)
`set_default_prog(prog)`로 §2의 기본 프로그램을 등록(`cli.py:877` — 그래서 `helper()`가
prog 없이 동작), (b) `sys.displayhook`을 `_displayhook`(`cli.py:264`)으로 바꿔
`Object`는 `format_()`로 예쁘게, `StackTrace`/`Type` 등은 `str()`로 출력,
(c) readline 히스토리/탭완성을 설정(`_setup_readline`, `cli.py:773`).

### 5.2 REPL과 `%` 명령 — `drgn/internal/repl.py`

`repl.py`는 Python 3.13의 새 `_pyrepl`(색상/멀티라인)을 감지해 쓰고, 없으면 표준 `code`
모듈로 폴백(`repl.py:19-54`). 핵심은 `_InteractiveConsoleWithCommand.runsource`
(`repl.py:61-80`): 입력 첫 글자가 **`%`** 면 Python으로 평가하지 않고
`run_command(prog, source[1:], globals=self.locals)`로 §6 명령 시스템에 넘긴다.
즉 한 REPL 안에서 `>>> for_each_task()`(Python)와 `>>> %ps`(crash식 명령)가 공존한다.

---

## 6. crash 유틸 호환 명령 레이어 (`drgn/commands/_crash`)

### 6.1 의도

`crash`는 커널 코어덤프 분석의 사실상 표준 도구다. 그 사용자가 drgn으로 넘어올 때
`bt`/`ps`/`kmem` 같은 익숙한 명령을 그대로 쓸 수 있게, drgn 헬퍼 위에 **crash 명령을
재구현**한다. `%crash`로 진입하면 `crash`처럼 보이는 프롬프트가 뜨고, `drgn` 명령으로
언제든 Python REPL로 빠져나갈 수 있다 — 점진적 학습 경로.

### 6.2 명령 시스템 구조 (`drgn/commands/__init__.py`, 1518줄)

- **`CommandNamespace`** (`commands/__init__.py:261`): 이름→`Command` 레지스트리.
  `run()`(`:319`)이 셸식 토큰 파싱 → 명령 조회 → 인자 파싱 → 리다이렉션/파이프/페이저
  설정(`_redirect_and_pipe`, `:90`) → 실행을 담당. drgn은 자체적으로 `< > >> |` 리다이렉션과
  외부 셸 파이프를 흉내 낸다(`_SHELL_TOKEN_REGEX`, `:68`).
- **`DEFAULT_COMMAND_NAMESPACE`** (`:379`): `%`로 직접 부르는 일반 명령.
- **데코레이터**: `@command`(`:678`, argparse 기반 옵션 파싱) / `@custom_command`
  (`:995`, 자체 파서) / `@raw_command`(가공 안 한 인자 문자열). 함수명 `_cmd_xxx`에서
  접두사를 떼 명령 이름을 만든다(`:739`). `enabled` 콜백으로 "커널일 때만" 같은 조건부 노출.
- **`_builtin`**: `sh`(셸 실행), `py`(Python 문 + 리다이렉션), `source`(drgn 스크립트),
  그리고 `crash`(아래)가 여기 등록. `commands/linux.py`의 `linux_kernel_*_command`는
  `enabled`에 `IS_LINUX_KERNEL` 검사를 끼워 커널 전용 명령을 만든다.

### 6.3 crash 네임스페이스 (`_crash/`)

- **별도 네임스페이스** `CRASH_COMMAND_NAMESPACE`(`_crash/common.py:256`, `_CrashCommandNamespace`).
  `crash_command = functools.partial(command, namespace=CRASH_COMMAND_NAMESPACE)`
  (`common.py:260`)로 crash 명령을 이 네임스페이스에 격리 등록 — 일반 `%` 네임스페이스를
  오염시키지 않는다. `pid_or_task`/`addr_or_sym` 같은 crash 특유의 인자 타입을 argparse에
  등록(`common.py:210-213`).
- **`%crash` 진입점** (`_crash/__init__.py:73-132`, `linux_kernel_raw_command`로 등록):
  인자가 있으면 단발 실행, 없으면 `%crash>` 프롬프트 루프를 돌며 줄마다
  `CRASH_COMMAND_NAMESPACE.run()` 호출. 전용 히스토리 파일과 탭완성(`_CrashCompleter`)도 둠.
- **`_resolve` 오버라이드** (`common.py:216-253`): crash 문법 특수 케이스 처리 —
  `!cmd`는 셸로, `*`/타입 이름은 `struct` 출력 명령(`*`)으로 라우팅, 그 외엔 페이저 연결.
- **재구현된 crash 명령들**(함수명 `_crash_cmd_*` → 명령 이름):
  `bt`(스택), `ps`/`task`(프로세스), `runq`(런큐), `kmem`/`vm`(메모리/슬랩),
  `mod`(모듈), `sym`/`whatis`/`struct`/`union`/`p`(심볼·타입·값), `log`(dmesg),
  `irq`/`timer`/`dev`/`files`/`mount`/`net`/`sig`/`ipcs`/`bpf`/`swap`,
  `rd`(메모리 읽기)/`search`/`eval`/`ascii`/`list`/`tree`/`foreach`/`vtop`/`ptov` 등
  (`_crash/` 디렉토리 각 `_xxx.py`). 각 명령은 내부적으로 §3~4의 helpers를 호출해
  결과를 crash식 텍스트로 포맷한다. `crash` 명령 도움말은 `help`/`_help_overview`
  (`__init__.py:176-258`)가 출력.

---

## 7. kmodify: 라이브 커널 패치 (개념) — `drgn/helpers/experimental/kmodify.py`

drgn은 본래 **읽기 전용** 디버거다. kmodify는 그 금기를 깨고 실행 중인 커널의 메모리를
쓰거나 임의 커널 함수를 호출한다. 공개 API(`kmodify.py:69`): `write_memory`,
`write_object`, `call_function`, `call_functions`, `pass_pointer`.

### 7.1 메커니즘: "임시 커널 모듈을 만들어 로드"

drgn은 ptrace로 커널에 코드를 주입할 수 없다(커널엔 그런 수단이 없음). 대신
**커널이 이미 제공하는 합법적 코드 주입 경로 = 모듈 로딩(`finit_module`)** 을 악용한다.
`_Kmodify` (`kmodify.py:852`)의 흐름:

1. **제약 검사**(`kmodify.py:854-874`): 라이브(`IS_LIVE`)·로컬(`IS_LOCAL`)·리눅스
   커널이어야 하고, 현재 **x86-64만** 지원. (모듈도 `CONFIG_MODULES=y`,
   `CONFIG_MODULE_SIG_FORCE=n` 필요 — `kmodify.py:8-13`.)
2. **기계어 생성**(`_CodeGen_x86_64`, `kmodify.py:455`): "함수 A를 이 인자들로 호출하고
   반환값을 검사하라" 같은 고수준 명령(`_Call`, `_Return`, `_AtomicSetBit` 등 NamedTuple)을
   받아 x86-64 바이트코드 + 재배치(relocation)를 직접 어셈블한다. 프레임 설정, 인자
   레지스터 적재, `call`, 반환값 저장까지 손으로 인코딩.
3. **합성 ELF `.ko` 조립**(`insert`, `kmodify.py:878-1012`): 생성한 코드를 `.init.text`로,
   데이터를 `.data`로 넣고, `.gnu.linkonce.this_module`(`struct module` 인스턴스)·
   `.modinfo`(license=GPL, vermagic은 `prog["vermagic"]`에서 그대로 복사 — 버전 검사 통과)
   섹션을 갖춘 진짜 커널 모듈 ELF를 메모리에서 만든다. `init_module` 심볼이 생성 코드를 가리킨다.
4. **로드**(`kmodify.py:1002-1023`): ELF를 `memfd_create`로 익명 fd에 쓴 뒤
   `finit_module(2)` 시스템콜로 로드. 커널이 모듈을 적재하면서 **`init_module`(=우리 코드)을
   커널 컨텍스트에서 실행**한다.
5. **즉시 언로드 트릭**: 생성 코드는 마지막에 `-EINPROGRESS`(또는 `-EFAULT`)를 반환한다
   (예: `write_memory`의 `_Return(-errno.EINPROGRESS)`, `kmodify.py:1111`).
   init이 음수를 반환하면 커널은 "모듈 초기화 실패"로 보고 모듈을 **자동으로 떼어낸다** —
   덕분에 코드만 한 번 실행되고 모듈은 잔류하지 않는다. 반환 코드(`ret`)로 성공/실패를
   판별(`kmodify.py:1142-1148`).

### 7.2 대표 동작

- **`write_memory`** (`kmodify.py:1039`): 생성 코드가 커널의
  `copy_from_kernel_nofault`로 대상 주소가 유효한지 먼저 확인하고(잘못된 주소면
  `-EFAULT` 반환 → `FaultError`로 변환), `copy_to_kernel_nofault`로 실제 바이트를
  복사한다(`kmodify.py:1089-1111`). 커널 이름이 버전마다 다른 걸 폴백 목록으로 처리
  (`_COPY_TO_FROM_KERNEL_NOFAULT_NAMES`, `kmodify.py:1026`).
- **`call_function`** (`kmodify.py:1580`): 임의 커널 함수를 인자와 함께 호출하고 반환값을
  `Object`로 돌려준다. 예: `call_function("wake_up_process", task)`,
  `call_function("__kmalloc_noprof", 13, GFP_KERNEL)`. Python 값/Object 인자를 C ABI에
  맞게 승격(`_default_argument_promotions`)하고, 구조체는 `pass_pointer`로 포인터 전달 —
  함수가 값을 바꾸면 호출 후 `pass_pointer.object`에 되읽어 반영(`kmodify.py:1629-1639`).
  가변 인자(`_printk`)도 지원.

> 경고(소스 docstring): "극도로 위험". 잘못 쓰면 커널을 즉시 패닉시킬 수 있고, 쓰기도
> 비원자적이라 부분 기록 상태가 노출될 수 있다.

---

## 8. 타 서브시스템 연결

- **C 확장(`_drgn`) ↔ Python 헬퍼**: 헬퍼는 `prog[...]`/`prog.type()`/`Object` 같은
  C 확장 원시 연산 위에서 동작. 핫패스(`pid_task`, `find_task`,
  `_linux_helper_find_pid`)는 C로 내려가 `_drgn`에서 직접 임포트(`pid.py:14`).
  자세한 바인딩은 [[08-python-c-binding]].
- **DWARF/디버그심볼**: 모든 헬퍼는 `prog.type("struct ...")`로 멤버 오프셋을 얻으므로
  커널 디버그 정보(또는 BTF)가 전제. `kallsyms`/`vmcoreinfo` 폴백은 [[03-debuginfo-dwarf]],
  심볼 로딩은 `cli.py:_load_debugging_symbols`.
- **리눅스 커널 자료구조**: §3·§4 헬퍼가 다루는 `task_struct`/`list_head`/`slab`/`page` 등의
  실제 의미는 [[06-linux-kernel]].
- **메모리/주소 공간**: 헬퍼가 읽는 모든 주소는 `Program`의 메모리 리더를 거친다
  ([[02-program-memory]]). kmodify는 그 모델을 깨고 쓰기까지 한다.

---

## 9. 파일 맵

| 파일 | 줄수 | 역할 |
|------|------|------|
| `drgn/__init__.py` | 514 | C 확장 심볼 공개 재노출 + 모듈 docstring |
| `drgn/cli.py` | 888 | `_main` 진입점, argparse 옵션, `prog` 생성, `default_globals`(네임스페이스 주입), `run_interactive`, `_displayhook` |
| `drgn/internal/repl.py` | 80 | pyrepl/code 어댑터, `%` 명령 디스패치(`runsource`) |
| `drgn/helpers/__init__.py` | 30 | `ValidationError` 정의 |
| `drgn/helpers/common/prog.py` | 255 | **`takes_program_or_default` / `takes_object_or_program_or_default`** (암묵 prog 주입) |
| `drgn/helpers/common/{memory,format,type,stack}.py` | — | 프로그램 무관 헬퍼(`identify_address` 등) |
| `drgn/helpers/linux/list.py` | 333 | `list_for_each_entry` 등 — `container_of`+next 순회 |
| `drgn/helpers/linux/pid.py` | 136 | `for_each_task`/`find_task` — IDR/pid_hash 순회 |
| `drgn/helpers/linux/{mm,slab,block,sched,fs,...}.py` | — | 커널 서브시스템별 헬퍼(§3 표) |
| `drgn/helpers/experimental/kmodify.py` | 1838 | 라이브 커널 쓰기/함수 호출(임시 모듈 로드) |
| `drgn/commands/__init__.py` | 1518 | `CommandNamespace`, `@command`/`@custom_command`, `run_command`, 셸 리다이렉션 |
| `drgn/commands/linux.py` | 58 | 커널 전용 명령 데코레이터 |
| `drgn/commands/_builtin/__init__.py` | — | `sh`/`py`/`source` 내장 명령 |
| `drgn/commands/_crash/` | 22개 파일 | crash 호환 명령(`bt`/`ps`/`kmem`/`runq`/`mod`/…) + `%crash` 진입점·네임스페이스 |
