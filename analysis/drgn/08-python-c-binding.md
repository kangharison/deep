# drgn 코드 분석 — C↔Python 바인딩 (_drgn 확장)

> 소스: osandov/drgn (main), 로컬 `deep/sources/drgn`
> 분석 대상: `libdrgn/python/` (CPython 확장 모듈 `_drgn`), `drgn/__init__.py`, `_drgn.pyi`
> 관련: [[drgn-codebase-overview]] · [[02-program-memory]] · [[05-object]] · [[09-python-helpers-cli]]

---

## 1. 역할

`libdrgn/python/`은 순수 C 라이브러리 `libdrgn`(코어 디버거 엔진)을 CPython 확장 모듈 `_drgn`으로 감싸는
**바인딩 계층**이다. 사용자는 파이썬에서 `import drgn` 한 번으로 `Program`, `Object`, `Type`, `Symbol`,
`StackTrace`, `Module` 같은 객체를 다루지만, 그 실체는 모두 C 구조체(`struct drgn_program`,
`struct drgn_object` 등)이고 메서드 호출은 C 함수로 디스패치된다.

이 계층이 책임지는 4가지:

1. **타입 노출**: `struct drgn_program` → 파이썬 `Program` 타입 등으로 `PyTypeObject` 정의/등록.
2. **소유권·생명주기 관리**: C 객체를 파이썬 객체에 박아넣고(embed) refcount/GC로 수명 추적.
3. **에러 변환**: `struct drgn_error *` ↔ 파이썬 예외의 양방향 변환(`error.c`).
4. **인자 변환**: 파이썬 `int`/`str`/`Path`/`enum` ↔ C `uint64_t`/`const char*`/플래그(`util.c` converter들).

특이점: 바인딩이 단방향이 아니라 **양방향**이다. C에서 libdrgn을 standalone으로 쓸 때조차
`drgn_program_create()`(`program.c:387`)는 내부에서 파이썬 인터프리터를 띄우고 파이썬 `Program` 객체를
만들어 그 안의 `struct drgn_program`을 돌려준다. 즉 `struct drgn_program`의 **정규 소유자는 항상 파이썬
`Program` 객체**다.

---

## 2. CPython 확장 구조 (PyTypeObject 등록, main.c)

### 2.1 타입 정의 패턴

각 노출 타입은 `Foo_type`라는 전역 `PyTypeObject`로 정의된다. 공통 형태(`symbol.c:153`,
`memory_search.c:200`, `program.c:2448`):

```c
PyTypeObject Symbol_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_drgn.Symbol",
    .tp_basicsize = sizeof(Symbol),       // 헤더 + 박힌 C 데이터 크기
    .tp_dealloc = (destructor)Symbol_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,       // GC 필요 시 | Py_TPFLAGS_HAVE_GC
    .tp_richcompare = (richcmpfunc)Symbol_richcompare,
    .tp_getset = Symbol_getset,           // 프로퍼티(읽기 전용 attr)
    .tp_new = Symbol_new,                 // 파이썬에서 Symbol(...) 생성
};
```

- **`tp_methods`** (`PyMethodDef[]`): 일반 메서드. 예) `StackFrame_methods`(`stack_trace.c:361`)에
  `locals`, `symbol`, `registers`, `register`. 플래그 `METH_NOARGS`/`METH_O`/`METH_VARARGS|METH_KEYWORDS`로
  C 시그니처를 지정.
- **`tp_getset`** (`PyGetSetDef[]`): 읽기 전용/읽기쓰기 프로퍼티. 예) `Symbol_getset`(`symbol.c:144`)의
  `name`/`address`/`size`/`binding`/`kind`.
- **`tp_members`** (`PyMemberDef[]`): C 구조체 필드를 직접 attr로 노출. `Program_members`(`program.c:2418`)는
  `cache`/`config` 두 dict를 `T_OBJECT_EX`로 노출(파이썬 헬퍼들이 캐시 저장소로 쓰는 임의 dict).
- **프로토콜 슬롯**: `tp_as_number`(`DrgnObject_as_number`, `object.c:1673` — `+ - * / & | ^ << >>`,
  `int()`/`float()`/`bool()`/`index()` 전부 C 객체 산술로 매핑), `tp_as_sequence`/`tp_as_mapping`
  (`StackTrace`는 `len()`/`[]`, `StackFrame`은 `frame["var"]`/`in`), `tp_iter`/`tp_iternext`(이터레이터들).

### 2.2 상속(서브클래싱)

C 레벨에서 `tp_base`로 파이썬 상속 트리를 만든다. `Module`은 추상 베이스이고
`MainModule`/`SharedLibraryModule`/`VdsoModule`/`RelocatableModule`/`ExtraModule`이
`.tp_base = &Module_type`로 파생된다(`module.c:552~629`). 동일 C 구조체(`Module`,
`drgnpy.h:196`)를 공유하되, `Module_wrap()`(`module.c:26`)이 `drgn_module_kind()`로 분기해 적합한
서브타입의 인스턴스를 alloc한다. 예외 타입도 같은 방식: `FaultError_type.tp_base = PyExc_Exception`,
`ObjectNotFoundError_type.tp_base = PyExc_KeyError`로 런타임에 베이스를 꽂는다(`main.c:401~409`).

### 2.3 모듈 초기화 (main.c)

- **모듈 정의**: `drgnmodule`(`main.c:292`)은 `PyModuleDef`. 모듈 레벨 함수 테이블 `drgn_methods`
  (`main.c:219`)에 `sizeof`/`alignof`/`offsetof`/`cast`/`container_of`/`program_from_*` 및 수십 개의
  `_linux_helper_*`가 등록된다.
- **진입점**: `PyInit__drgn()`(`main.c:339`). `PyModule_Create` 후 `add_type()`(`main.c:16`, 사실상
  `PyType_Ready()` + `PyModule_AddObjectRef()`)로 모든 `*_type`을 차례로 등록. 이터레이터처럼 모듈 attr로
  노출할 필요 없는 타입은 `PyType_Ready()`만 호출.
- **enum/Flag 클래스**: `add_module_constants(m)`(`main.c:352`)는 빌드 시 생성된 함수(생성기
  `libdrgn/build-aux/gen_constants.py`)다. `drgn.h`의 C enum(`DRGN_FIND_OBJECT_*`, `DRGN_QUALIFIER_*`,
  `DRGN_TYPE_*`, `DRGN_SYMBOL_BINDING_*` 등)을 정규식으로 긁어 파이썬 `enum.Enum`/`enum.Flag` 서브클래스
  (`FindObjectFlags`, `Qualifiers`, `TypeKind`...)를 만들고, 그 클래스 객체를 C 전역
  `FindObjectFlags_class` 등(`drgnpy.h:333~346`)에 저장한다. 그래서 C 코드는
  `PyObject_CallFunction(SymbolKind_class, "k", ...)`(`symbol.c:120`)로 C 정수를 파이썬 enum 인스턴스로
  되돌린다.
- **예외 생성**: `add_new_exception` 매크로(`main.c:345`)가 `PyErr_NewExceptionWithDoc`으로
  `BadDataError`/`MissingDebugInfoError`/`ObjectAbsentError`/`OutOfBoundsError`/`UnsupportedOperation`
  등을 만든다.
- **부트스트랩 헬퍼**: `drgn_initialize_python()`(`main.c:497`)은 "파이썬이 아직 안 떠 있으면 띄우고,
  GIL 잡고, `_drgn`이 import 안 됐으면 import"를 보장. C standalone 경로에서 `PyImport_AppendInittab`으로
  `_drgn`을 인터프리터에 등록(`main.c:507`)한다.

---

## 3. C객체 ↔ 파이썬객체 래핑 (소유권 / refcount / 생명주기)

### 3.1 임베드(embed) 패턴

파이썬 객체 구조체가 C 구조체를 **값으로 품거나(by value) 포인터로 가리킨다**(`drgnpy.h:158~285`):

```c
typedef struct { PyObject_HEAD struct drgn_object obj; } DrgnObject;      // 값으로 임베드
typedef struct { PyObject_HEAD struct drgn_program prog; ... } Program;   // 값으로 임베드
typedef struct { PyObject_HEAD struct drgn_type *type; ... } DrgnType;    // 포인터(코어가 소유)
typedef struct { PyObject_HEAD struct drgn_stack_trace *trace; } StackTrace; // 포인터(파이썬이 소유)
```

`tp_basicsize = sizeof(Foo)`이므로 C 데이터가 파이썬 객체 메모리 안에 함께 alloc된다.

### 3.2 container_of로 역참조

C 구조체가 값으로 임베드된 경우, C 포인터만 받으면 감싼 파이썬 객체를 `container_of`로 복원할 수 있다.
이게 소유권 추적의 핵심 트릭이다(`drgnpy.h:410`, `:435`, `:467`):

```c
static inline Program *DrgnObject_prog(DrgnObject *obj) {
    return container_of(drgn_object_program(&obj->obj), Program, prog);
}
```

`StackTrace_wrap`/`Module_wrap`/`MemorySearchIterator_wrap` 등 모든 wrap 함수가, 코어가 준
`struct drgn_program *`로부터 `container_of(prog, Program, prog)`를 구해 그 파이썬 `Program`에
`Py_INCREF`한다(`stack_trace.c:13`, `module.c:51`, `memory_search.c:20`).

### 3.3 "프로그램이 모든 것을 떠받친다" 규칙

`Object`/`Type`/`StackTrace`/`Module`/`Thread`/`Symbol`은 모두 자신을 만든 `Program`보다 오래 살 수 없다
(C 데이터가 프로그램의 메모리·DWARF 인덱스를 참조하므로). 따라서 **자식 객체는 생성 시 부모 Program에
`Py_INCREF`하고 dealloc에서 `Py_DECREF`한다**:

- `DrgnObject_alloc`(`drgnpy.h:426`): `drgn_object_init` 후 `Py_INCREF(prog)`.
- `DrgnObject_dealloc`(`object.c:544`): `Py_DECREF(DrgnObject_prog(self))` → `drgn_object_deinit`.
- `DrgnType_wrap`(`type.c:24`)/`DrgnType_dealloc`(`type.c:466`), `Thread`(`thread.c:13/28`),
  `StackTrace`(`stack_trace.c:9/18`) 모두 동일.

`StackFrame`은 한 단계 더: 자기 `Program`이 아니라 부모 `StackTrace`에 `Py_INCREF`하고(`stack_trace.c:71`),
인덱스 `i`만 들고 다닌다(독립 C 객체가 없는 "뷰").

### 3.4 Program이 거꾸로 잡는 객체 — hold set

반대 방향 의존도 있다. 사용자가 등록한 콜백(메모리 read_fn, 타입 finder 등)이나 임의 파이썬 객체는
**Program이 살아 있는 동안 살아 있어야** 한다. 이를 위해 `Program`은 `struct pyobjectp_set objects`라는
해시셋을 갖는다(`drgnpy.h:238`).

- `Program_hold_object()`(`program.c:255`): 셋에 넣고 `Py_INCREF`. `add_memory_segment`의 콜백
  `read_fn`을 여기에 보관(`program.c:499`).
- `Program_dealloc`(`program.c:410`): 셋의 모든 원소에 `Py_DECREF` 후 `drgn_program_deinit`.

### 3.5 순환 참조와 GC

`Program ↔ 콜백`, `Program ↔ Object`는 순환을 만들 수 있어 대부분 타입이 `Py_TPFLAGS_HAVE_GC`를 켜고
`tp_traverse`/`tp_clear`를 구현한다. `Program_traverse`(`program.c:423`)는 hold set + `cache` + `config`를
방문하고, `Program_clear`(`program.c:432`)는 그들을 끊는다. 자식 타입의 `tp_traverse`는 부모 Program을
방문(`Py_VISIT(DrgnObject_prog(self))`, `object.c:1698`)해 GC가 사이클을 인식하게 한다. dealloc 첫 줄이
`PyObject_GC_UnTrack(self)`인 이유.

### 3.6 LIBDRGN_PUBLIC 래퍼 (C → 파이썬 소유 위임)

`drgn_program_create()`(`program.c:387`)는 `Program_new_impl`로 파이썬 객체를 만들고 `&prog->prog`만
반환. `drgn_program_destroy()`(`program.c:402`)는 그 포인터에서 `container_of`로 파이썬 객체를 찾아
`Py_DECREF`. 즉 C API의 create/destroy가 실제로는 파이썬 refcount 조작이다 — libdrgn은 자체 GC가 없고
CPython의 것을 빌린다.

---

## 4. drgn_error → 파이썬 예외 변환 (error.c)

코어 함수는 실패 시 `struct drgn_error *`를 반환한다(NULL이면 성공). 바인딩의 모든 호출 지점은
`if (err) return set_drgn_error(err);` 패턴이다.

### 4.1 C 에러 → 파이썬 예외: `set_drgn_error()` (error.c:265)

`err->_code`(`enum drgn_error_code`, `drgn.h:61`)에 따라 적절한 파이썬 예외를 세팅하고 `drgn_error_destroy`로
C 에러를 해제한 뒤 `NULL`을 반환(파이썬 C-API의 "예외 발생 시 NULL 리턴" 규약). 매핑은 `SIMPLE_DRGN_EXCEPTIONS`
X-매크로(`error.c:85`)로 일괄 정의된다:

| drgn_error_code | 파이썬 예외 | 비고 |
|---|---|---|
| `DRGN_ERROR_INVALID_ARGUMENT` | `ValueError` | |
| `DRGN_ERROR_OVERFLOW` | `OverflowError` | |
| `DRGN_ERROR_RECURSION` | `RecursionError` | |
| `DRGN_ERROR_MISSING_DEBUG_INFO` | `MissingDebugInfoError` | drgn 고유 |
| `DRGN_ERROR_SYNTAX` | `SyntaxError` | |
| `DRGN_ERROR_LOOKUP` | `LookupError` | 심볼/타입/객체 없음 |
| `DRGN_ERROR_TYPE` | `TypeError` | |
| `DRGN_ERROR_ZERO_DIVISION` | `ZeroDivisionError` | |
| `DRGN_ERROR_OUT_OF_BOUNDS` | `OutOfBoundsError` | drgn 고유 |
| `DRGN_ERROR_OBJECT_ABSENT` | `ObjectAbsentError` | drgn 고유 |
| `DRGN_ERROR_NOT_IMPLEMENTED` | `NotImplementedError` | |
| `DRGN_ERROR_UNSUPPORTED_OPERATION` | `UnsupportedOperation`(←`ValueError`) | drgn 고유 |
| `DRGN_ERROR_RUNTIME` | `RuntimeError` | |
| `DRGN_ERROR_BAD_DATA` | `BadDataError` | drgn 고유 |
| `DRGN_ERROR_NO_MEMORY` | `PyErr_NoMemory()` | 특수 |
| `DRGN_ERROR_OS` | `OSError`(errno+filename) | `PyErr_SetFromErrnoWithFilename`(`error.c:286`) |
| `DRGN_ERROR_FAULT` | `FaultError(message, address)` | 잘못된 메모리 접근, 주소 보존(`error.c:288`) |
| 그 외/`DRGN_ERROR_OTHER` | `Exception` | |

**FaultError**가 가장 특별하다. 단순 문자열이 아니라 `message`+`address` 속성을 가진 객체이고
(`FaultError_init`, `error.c:8`), `__str__`은 `"%s: %#x"`로 포맷(`error.c:23`). `set_drgn_error`는
`PyObject_CallFunction(&FaultError_type, "sK", message, address)`로 인스턴스를 직접 생성한다(`error.c:290`).
유효하지 않은 주소를 역참조했을 때 파이썬에서 `except FaultError as e: e.address`로 폴트 주소를 얻는 근거.

`DRGN_ERROR_STOP`은 예외로 변환되지 않고, 이터레이터에서 `drgn_error_catch(&err, DRGN_ERROR_STOP)`로
삼켜 `StopIteration`(NULL 리턴)으로 처리된다(`memory_search.c:53`). 마찬가지로 `DRGN_ERROR_LOOKUP`은
`StackFrame["x"]`에서 `KeyError`로 재해석된다(`stack_trace.c:172`).

### 4.2 파이썬 예외 → C 에러: `drgn_error_from_python()` (error.c:103)

파이썬 콜백(예: 사용자가 등록한 `read_fn`, 타입 finder)이 예외를 던지면, 그걸 C 코어로 다시 전달해야 한다.
`drgn_error_from_python()`은 현재 발생 예외(`PyErr_GetRaisedException`)를 떼어내 `_code = DRGN_ERROR_PYTHON`,
`_python_exc = exc`인 특수 에러를 만든다(예외 객체를 보존). `py_memory_read_fn`(`program.c:443`)/
`py_debug_info_find_fn`(`program.c:509`)이 콜백 실패 시 이걸 반환.

### 4.3 왕복(round-trip) 보존

이 설계의 묘미: 파이썬 콜백에서 던진 예외가 C 코어를 거쳐 다시 파이썬으로 나올 때 **원본 예외 객체 그대로**
복원된다. `set_drgn_error`(`error.c:267`)는 `err->_python_exc`가 있으면 매핑 테이블을 건너뛰고
`PyErr_SetRaisedException(err->_python_exc)`로 원본을 그대로 재발생시킨다. 트레이스백·타입이 보존됨.

`drgn_error_resolve()`(`error.c:225`)는 그 반대로, 보존된 파이썬 예외를 (GIL 밖 C 컨텍스트에서 메시지가
필요할 때) 코드/메시지로 "구체화"한다 — `FaultError`면 address 추출(`error.c:208`), `OSError`면 errno/path
추출(`error.c:160`).

### 4.4 시그널(Ctrl-C)과 GIL

장시간 C 작업 동안 GIL을 놓기 위해 `drgn_begin_blocking`/`drgn_end_blocking`(`program.c:314/326`)이
`PyEval_ReleaseThread`/`RestoreThread`를 감싼다. `drgn_blocking_check_signals`(`error.c:313`)는 블로킹 중
GIL을 잠깐 잡아 `PyErr_CheckSignals()`를 호출 — `KeyboardInterrupt`를 `drgn_error_from_python()`으로 변환해
코어 루프를 중단시킨다.

---

## 5. 성능 경계: 왜 이건 C에 있나 (helpers / memory_search / symbol_index)

파이썬↔C 경계를 넘는 건 비싸다(인자 변환, refcount, GIL). drgn은 **타이트 루프와 자주 호출되는 헬퍼를
C쪽에 두어** 경계 횡단 횟수를 줄인다.

### 5.1 helpers.c — Linux 커널 핫패스 헬퍼

`per_cpu_ptr`, `idr_find`, `xa_load`, `find_task`, `task_cpu`, `cpu_curr`, `idle_task` 등(`helpers.c`)은
커널 디버깅에서 수만 번 불릴 수 있고, 매번 여러 멤버 역참조 + 메모리 read를 한다. 파이썬으로 짜면
역참조마다 `Object` 임시 객체가 생겨 GC 압박이 크다. C 버전(`linux_helper_*`)은 임시 `struct drgn_object`만
쓰고 결과 하나만 `DrgnObject`로 wrap한다. 바인딩 함수는 인자 파싱 + 코어 호출 + 결과 wrap의 얇은 껍데기
(`drgnpy_linux_helper_per_cpu_ptr`, `helpers.c:74`). `read_vm`(`helpers.c:23`)은 가상주소 → 페이지테이블
워크 → 물리 read를 C에서 끝내고 `bytes` 한 번만 돌려준다.

### 5.2 object.c — 값 변환과 산술

`DrgnObject_value_impl`(`object.c:641`)은 구조체/배열을 **재귀적으로** 파이썬 dict/list로 변환한다. 큰
구조체를 파이썬에서 멤버별로 풀면 멤버마다 경계 횡단이지만, C 재귀는 한 번의 호출로 전체 트리를 만든다.
`tp_as_number`(`object.c:1673`)의 모든 산술 연산자도 C 객체 산술(`drgn_object_add` 등)로 직접 매핑돼,
포인터 연산/타입 전파를 코어가 처리한다.

### 5.3 memory_search.c — 대용량 메모리 스캔

`Object` 단위로 파이썬에서 메모리를 뒤지면 GB 단위 코어덤프에서 치명적이다. `MemorySearchIterator`
(`memory_search.c`)는 코어의 `drgn_memory_search_iterator_next`(C에서 청크 단위 스캔)를 `tp_iternext`로 감싼
이터레이터다. 매칭 1건당 한 번만 파이썬으로 나온다. 결과 타입별로 4종(주소만 / `WithBytes` / `WithStr` /
`WithInt`)을 두어 변환 비용도 최소화. `WithInt`(`memory_search.c:114`)는 플랫폼 바이트오더(`bswap`)까지
C에서 처리해 정수로 돌려준다.

### 5.4 symbol_index.c — 심볼 조회 인덱스

`SymbolIndex`(`symbol_index.c`)는 심볼 배열을 코어의 해시 인덱스로 들고 있다가, `__call__`(`tp_call`,
`symbol_index.c:13`)로 이름/주소 기준 O(1) 조회를 한다. 커널 `kallsyms`처럼 수십만 심볼을 파이썬 dict로
다루면 메모리·조회 비용이 크므로 C 인덱스가 유리. `helpers.c`의 `load_proc_kallsyms`/`load_builtin_kallsyms`
가 `/proc/kallsyms` 또는 vmlinux 내장 테이블을 파싱해 곧장 `SymbolIndex`를 만든다(`helpers.c:313/335`).
이 인덱스는 `prog.register_symbol_finder()`에 콜백으로 꽂혀 심볼 해석 백엔드가 된다.

요약하면 경계 설계 원칙: **반복(iteration)·재귀·바이트 단위 작업은 C, 정책·조합·스크립팅은 파이썬**.

---

## 6. drgn/__init__.py 재노출

`_drgn`은 내부 확장 모듈이고, 사용자 향 공개 패키지는 `drgn`이다. `drgn/__init__.py`가 둘을 잇는다.

- **C 타입/함수 재노출**: `from _drgn import (...)`(`__init__.py:49~115`)로 `Program`, `Object`, `Type`,
  `Symbol`, `StackTrace`, `Module`(+서브타입), `FaultError`, enum 클래스들, `sizeof`/`cast`/`container_of`/
  `program_from_*` 등을 그대로 끌어와 `drgn` 네임스페이스에 노출. `__all__`(`__init__.py:131`)이 공개 표면을
  확정한다.
- **빌드 플래그**: `_elfutils_version`, `_have_debuginfod`, `_with_lzma`, `_with_pcre2` 등 내부 플래그도
  재노출(`__init__.py:119`) — `main.c:417~478`에서 컴파일 옵션에 따라 모듈 상수로 추가된 것들.
- **순수 파이썬 추가분**: C로 노출되지 않은 편의 함수는 `__init__.py`에서 파이썬으로 구현. `execscript`
  (`__init__.py:223`)는 스크립트를 호출자 전역에서 실행하는 CLI 편의 함수. 그 외 `stack_trace`,
  `search_memory*`, `source_location` 등도 파이썬 래퍼(`__all__`에 포함, `__init__.py:196~205`)로
  `_drgn`의 저수준 API 위에 사용자 친화 시그니처를 입힌다.
- **타입 스텁**: `_drgn.pyi`(약 4,200줄)는 C 확장에는 없는 정적 타입 정보를 제공하는 스텁. 런타임에는
  영향이 없고 IDE/mypy/`pydoc`용 API 계약서다. `main.c`의 `add_type_aliases`(`main.c:303`)가
  `IntegerLike`/`Path`/`MemorySearchIterator` 같은 타이핑 별칭을 런타임에도 채워 스텁과 일치시킨다.

---

## 7. 타 서브시스템 연결

- **코어 엔진** (`libdrgn/*.c`): 바인딩이 감싸는 실체. `program.c`↔`../program.[ch]`,
  `object.c`↔`../object.[ch]`, `type.c`↔`../type.[ch]`. 자세한 동작은 [[02-program-memory]],
  [[05-object]] 참조.
- **공통 헤더** `drgnpy.h`: 모든 파이썬 객체 구조체 정의, `*_type`/`*_class` extern 선언, wrap/alloc
  인라인, converter 프로토타입을 한곳에 모은 바인딩 ABI. 모든 `python/*.c`가 포함.
- **에러 인프라** `../error.[ch]` + `error.c`: `struct drgn_error`에 `_python_exc` 필드를 둬 파이썬 예외를
  C 에러로 운반하는 다리. `drgn_error_resolve`/`set_drgn_error`가 양 끝.
- **인자 변환 유틸** `util.c`: `index_converter`/`path_converter`/`enum_converter`/`u64_converter`가
  `PyArg_ParseTupleAndKeywords`의 `O&` 훅으로 쓰여 파이썬 값을 C 타입으로 검증·변환. 거의 모든 메서드가 의존.
- **헬퍼/CLI 파이썬 계층**: `drgn/helpers/`, `drgn/cli.py`는 이 바인딩이 노출한 객체 위에서 동작.
  [[09-python-helpers-cli]] 참조.
- **로깅 브리지**: `program.c`의 `drgnpy_log_fn`(`:27`)이 코어 로그를 파이썬 `logging` 모듈로 포워딩하고,
  `logger._cache`를 몽키패치해 로그 레벨 동기화(`:196`)까지 한다.

---

## 8. 파일 맵

| 파일 | 줄수 | 역할 |
|---|---|---|
| `python/main.c` | 524 | 모듈 정의/진입점 `PyInit__drgn`, 타입 등록, 예외 생성, 모듈 함수(`sizeof`/`cast`/...), `drgn_initialize_python` 부트스트랩 |
| `python/drgnpy.h` | 577 | 바인딩 공통 헤더: 파이썬 객체 구조체, `*_type`/`*_class` extern, wrap/alloc 인라인, converter 선언 |
| `python/program.c` | 2519 | `Program` 타입(가장 큰 표면): 생성/소유권/hold set/GC, 메모리 콜백, 타입·객체·심볼·스택·모듈 API, 로깅 브리지, blocking/GIL |
| `python/object.c` | 1883 | `Object` 타입: 임베드 `struct drgn_object`, 값 변환(재귀), 전체 산술 연산자(`tp_as_number`), 비교/인덱싱 |
| `python/type.c` | 2275 | `Type` 타입 + `TypeMember`/`Parameter`/`Enumerator`, `attr_cache`, `Program_*_type` 팩토리, lazy object |
| `python/error.c` | 323 | **에러 변환 허브**: `set_drgn_error`(C→Py), `drgn_error_from_python`(Py→C), `FaultError`/`ObjectNotFoundError`, 시그널 |
| `python/module.c` | 701 | `Module` 베이스 + 5개 서브타입, `Module_wrap` kind 분기, 모듈/섹션 이터레이터 |
| `python/stack_trace.c` | 415 | `StackTrace`(시퀀스) + `StackFrame`(뷰, 매핑): locals/symbol/registers/source |
| `python/symbol.c` | 165 | `Symbol` 타입: name_obj 참조 보존 래핑, getset 프로퍼티, 동등 비교 |
| `python/symbol_index.c` | 123 | `SymbolIndex`: 심볼 해시 인덱스, `tp_call`로 이름/주소 조회, builder 기반 생성 |
| `python/memory_search.c` | 246 | `MemorySearchIterator` 4종(주소/bytes/str/int), 코어 청크 스캔을 이터레이터로 |
| `python/thread.c` | 151 | `Thread` + `ThreadIterator`: tid/object/name/stack_trace |
| `python/helpers.c` | 364 | Linux 커널 핫패스 헬퍼 바인딩(`per_cpu_ptr`/`idr_find`/`find_task`/`read_vm`/kallsyms 로더 등) |
| `python/util.c` | 360 | 인자 converter(`index`/`path`/`enum`/`u64`), repr 빌더, `PyLong_As*` 폴리필 |
| `drgn/__init__.py` | 514 | `_drgn` 재노출(`__all__`), `execscript` 등 파이썬 편의 함수 |
| `_drgn.pyi` | 4231 | 공개 API 타입 스텁(런타임 무관, IDE/mypy/pydoc용 계약) |
| `libdrgn/build-aux/gen_constants.py` | — | 빌드 시 `add_module_constants` 생성: C enum → 파이썬 Enum/Flag 클래스 |
