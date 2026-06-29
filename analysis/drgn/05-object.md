# drgn 코드 분석 — Object 시스템 (값/참조/부재)

> 소스: github.com/osandov/drgn @ main(0.2.0+), 로컬 `deep/sources/drgn`
> 관련: [[drgn-codebase-overview]] · [[02-program-memory]] · [[04-type-language]]

## 1. 역할

`struct drgn_object`는 drgn에서 **"프로그램 안의 심볼/값"을 표현하는 단일 자료형**이다. 사용자가 `prog['init_task']`, `task.pid`, `ptr[0]`, `a + b` 같은 표현식을 쓸 때마다 그 결과는 모두 `drgn_object`다. Python 쪽 `drgn.Object` 클래스는 이 C 구조체를 거의 1:1로 감싼 것이다(`_drgn.pyi:2395`).

핵심 설계 사상은 **"가능한 한 메모리를 읽지 않는다"** 이다. 디버깅 대상이 수십 GB 코어 덤프거나 라이브 커널일 때, 변수 하나 짚을 때마다 메모리를 읽으면 비효율적이고 위험하다. 그래서 object는 세 가지 종류(kind)를 가진다.

- **reference**: "타깃 메모리의 주소 X에 이 타입의 값이 있다"는 *약속*만 들고 있다. 실제 바이트는 읽지 않는다. `task.pid` 같은 멤버 접근도 reference면 reference를 유지하므로 메모리를 건드리지 않는다.
- **value**: 메모리에서 떨어져 나온(혹은 계산으로 만들어진) *실제 값*. 산술 연산 결과, 리터럴, `read_()`의 결과가 여기 해당한다.
- **absent**: 값도 주소도 없는 "부재" 상태 (예: 컴파일러가 최적화로 날려버린 변수). `void` 타입 object도 사실상 absent로 출발한다.

object.c는 또한 **언어 독립적인 연산자 구현부**(`drgn_op_*_impl`)를 제공한다. C에서 미정의/구현정의인 동작(부호 있는 오버플로, 우측 시프트, 음수 비트 연산 등)을 drgn은 **2의 보수 모듈러 산술 / 산술 우측 시프트**로 *정의된 동작*으로 못박는다(`object.h:29-33`).

## 2. Object 3종: value / reference / absent (struct drgn_object 표현)

### enum drgn_object_kind (`drgn.h:2377`)

```c
enum drgn_object_kind {
    DRGN_OBJECT_VALUE,      // 상수/임시 계산값
    DRGN_OBJECT_REFERENCE,  // 프로그램 메모리 안에 있음
    DRGN_OBJECT_ABSENT,     // 부재 (예: 최적화로 사라짐)
} __attribute__((__packed__));
```

### enum drgn_object_encoding (`drgn.h:2395`)

`kind`와 별개로, **값이 어떻게 인코딩되는가**를 나타낸다. 어느 union 필드를 쓰는지 결정한다.

| encoding | 값 | 의미 |
|----------|----|------|
| `BUFFER` | 0 | struct/union/class/array — 바이트 버퍼 |
| `SIGNED` / `UNSIGNED` | 1 / 2 | ≤64비트 정수·bool·포인터 (스칼라) |
| `SIGNED_BIG` / `UNSIGNED_BIG` | 3 / 4 | >64비트 정수 (버퍼에 저장) |
| `FLOAT` | 5 | 부동소수점 |
| `NONE` | -1 | void/함수 타입 (값 없음) |
| `INCOMPLETE_BUFFER` | -2 | 불완전 struct/array |
| `INCOMPLETE_INTEGER` | -3 | 불완전 enum |

**음수 encoding = 불완전 타입**이며, `drgn_object_encoding_is_complete()`(`drgn.h:2464`)는 `encoding >= 0`로 판정한다. 불완전 encoding은 reference에만 가능하다(value는 반드시 완전한 타입). 이 매핑을 실제로 계산하는 곳이 `drgn_object_type_impl()`(`object.c:77-180`)로, `drgn_type_kind`별로 위 표를 결정한다.

### union drgn_value (`drgn.h:2471`)

```c
union drgn_value {
    char    *bufp;     // BUFFER(외부할당)/SIGNED_BIG/UNSIGNED_BIG
    char     ibuf[8];  // BUFFER 인라인 (≤8바이트면 별도 malloc 없이 여기)
    int64_t  svalue;   // SIGNED
    uint64_t uvalue;   // UNSIGNED (포인터·bool 포함)
    double   fvalue;   // FLOAT
};
```

**인라인 최적화**: 값이 ≤8바이트면 `ibuf`에 박아 넣어 malloc을 피한다(`drgn_value_is_inline`, `drgn.h:2516`). `drgn_object_buffer()` 매크로(`drgn.h:2608`)가 인라인/외부를 투명하게 골라준다. 그래서 `drgn_object_deinit()`(`object.c:58`)은 VALUE이고 비인라인 버퍼/빅정수일 때만 `free()`한다.

### struct drgn_object (`drgn.h:2548`)

```c
struct drgn_object {
    struct drgn_type *type;             // 타입 (+ 프로그램·언어를 여기서 끌어온다)
    uint64_t bit_size;                  // 비트 크기 (비트필드면 type 크기보다 작음)
    enum drgn_qualifiers qualifiers;    // const/volatile 등
    enum drgn_object_encoding encoding; // 위 표
    enum drgn_object_kind kind;         // value/reference/absent
    bool is_bit_field;
    bool little_endian;                 // 스칼라일 때만 유효
    uint8_t bit_offset;                 // reference의 바이트내 비트 오프셋(0~7)
    union {
        union drgn_value value;             // VALUE
        uint64_t address;                   // REFERENCE
        enum drgn_absence_reason absence_reason; // ABSENT
    };
};
```

주목할 설계 두 가지.

1. **prog 포인터가 없다.** object는 프로그램을 들고 다니지 않는다. `drgn_object_program(obj)`는 `drgn_type_program(obj->type)`으로 타입에서 역추적한다(`drgn.h:2670`). 그래서 거의 모든 함수가 "두 object가 같은 프로그램인지" 먼저 검사한다.
2. **꼬리 union이 kind에 따라 셋 중 하나를 의미한다.** value면 `value`, reference면 `address`(+`bit_offset`), absent면 `absence_reason`(`drgn.h:2522`: OTHER/OPTIMIZED_OUT/NOT_IMPLEMENTED).

초기화는 `DRGN_OBJECT_INITIALIZER`(`object.c:21`)가 담당: **void 타입 + ABSENT + NONE encoding**으로 시작한다. `drgn_object_init()`(`object.c:35`)이 이 매크로를 쓴다.

### 값 설정자 (setter)

`drgn_object_set_signed/unsigned/float/from_buffer/reference/absent`(`object.c:232~512`). 공통 패턴: ① `drgn_object_type()`으로 타입→`drgn_object_type` 구조체(encoding 등 캐시)를 계산 → ② encoding 검증 → ③ `_internal` 변형 호출. `_internal` 변형(`object.h:159~217`)은 타입 계산이 끝난 상태를 받아 `drgn_object_reinit()`(`object.h:133`)으로 헤더를 갈아끼우고 union을 채운다. 반복 루프에서 타입 계산을 한 번만 하려고 이 분리가 존재한다.

## 3. reference→value 물질화 (read_, 메모리 읽기 트리거)

reference는 주소만 들고 있다. 실제 바이트가 필요한 순간에만 `drgn_program_read_memory`가 호출된다. **이 트리거 지점은 단 하나, `drgn_object_read_reference()`(`object.c:671`)다.**

```
drgn_object_read_reference()  (object.c:671)
  ├─ encoding 불완전 → "cannot read incomplete type" 에러 (object.c:679)
  ├─ BUFFER / *_BIG: drgn_program_read_memory(dst, address, size)  (object.c:702, 731)
  │     └─ bit_offset≠0 이면 read 후 copy_bits로 비트 정렬
  └─ 스칼라(SIGNED/UNSIGNED/FLOAT):
        drgn_program_read_memory(buf, address, read_size)  (object.c:769)
        → drgn_value_deserialize()로 svalue/uvalue/fvalue 채움 (object.c:773)
```

이 함수를 부르는 진입점은 둘이다.

- **`drgn_object_read()`(`object.c:779`)** — Python `read_()`(`_drgn.pyi:2808`). VALUE면 그냥 `drgn_object_copy`, REFERENCE면 `read_reference`로 값을 읽어 **새 VALUE object로 변환**, ABSENT면 `drgn_error_object_absent`. 결과는 더 이상 메모리에 의존하지 않는 스냅샷이다.
- **`drgn_object_read_value()`(`object.c:808`)** — object를 *변형하지 않고* 값 포인터만 돌려준다. VALUE면 `&obj->value`를 그대로(메모리 읽기 0), REFERENCE면 임시 버퍼에 읽어 그 포인터를 반환. 반드시 `drgn_object_deinit_value()`(`object.c:51`)와 짝지어야 한다(임시 버퍼 free). 내부 연산자들이 값만 잠깐 필요할 때 이걸 쓴다.

스칼라 단축 읽기: `drgn_object_read_signed/unsigned/float`(`object.c:986~1038`)는 encoding을 검증한 뒤 `read_value`로 한 스칼라를 빼낸다. `>64`비트면 `drgn_integer_too_big` 에러. Python `value_()`(`_drgn.pyi:2707`)는 이를 읽어 Python `int/float/bool/dict/list`로 변환한다(`read_()`가 object를 돌려주는 것과 대비).

`drgn_object_read_bytes()`(`object.c:830`)는 object의 값을 **원시 바이트로 직렬화**한다. VALUE면 메모리 읽기 없이 `serialize_bits`/`copy_bits`로 버퍼를 만들고, REFERENCE면 `drgn_program_read_memory`(`object.c:887`)로 읽는다. `to_bytes_()` / `drgn_object_fragment`의 value 경로가 이를 쓴다.

> 핵심: **reference를 만들고, 멤버를 짚고, 포인터 산술을 해도 메모리는 읽히지 않는다.** `read_()`/`value_()`/스칼라 변환/`drgn_object_fragment`의 value 분기처럼 *실제 바이트가 필요한 연산*에서만 `drgn_program_read_memory`(→ [[02-program-memory]])가 불린다.

## 4. 멤버/첨자/역참조/주소 접근 + 비트필드 (accessors.c, file:line)

> **accessors.c에 대한 정정**: `accessors.c`(7줄, `accessors.c:1-8`)는 멤버/첨자 접근 로직이 *아니다*. 이 파일은 `DRGN_ACCESSOR_LINKAGE`를 `LIBDRGN_PUBLIC extern`으로 재정의한 뒤 `drgn_internal.h`를 include 한다(`drgn_internal.h:17`). 평소 `static inline`인 타입 필드 게터들(`drgn_type_kind`, `drgn_type_program` 등, `drgn_internal.h:85+`)을 **공유 라이브러리가 export 하는 외부 심볼로 한 번 실체화(emit)**하는 번역 단위일 뿐이다. 실제 멤버/첨자/역참조/주소 로직은 전부 `object.c`에 있다.

모든 접근의 토대는 **`drgn_object_fragment_internal()`(`object.c:570`)** — "object의 일부(bit_offset부터 bit_size만큼)를 새 타입으로 떼어내기"다. kind에 따라:

- **VALUE**(`object.c:579`): 경계 검사 → 버퍼를 읽어 → `drgn_object_set_from_buffer_internal`로 새 VALUE. (값 안의 값이므로 메모리 읽기 없음)
- **REFERENCE**(`object.c:609`): 새 주소 = `obj->address + (bit_offset >> 3)`, 새 비트오프셋 = `obj->bit_offset + (bit_offset & 7)` → `set_reference_internal`. **여전히 REFERENCE, 메모리 읽기 0.** `>>3`/`&7`을 쓰는 이유는 음의 bit_offset도 음의 무한대 쪽으로 내림하기 위해서다(`/8`,`%8`은 0쪽 절단).
- **ABSENT**: `drgn_error_object_absent`.

이 위에 고수준 접근자들이 얹힌다.

| 연산 | 함수 (object.c) | 동작 |
|------|----------------|------|
| `obj.member` | `drgn_object_member` `:1659` | `drgn_type_find_member`로 멤버의 bit_offset·타입 찾고 → `fragment`. kind 보존 |
| `ptr->member` | `drgn_object_member_dereference` `:1685` | 포인터값을 읽어(주소) → `dereference_offset`로 멤버 위치에 새 reference |
| `obj[i]` | `drgn_object_subscript` `:1593` | `drgn_program_element_info`로 원소 크기 획득 → 포인터(UNSIGNED)면 `dereference_offset`, 배열이면 `fragment`. 오프셋 `i*element.bit_size` |
| `*obj` | `drgn_object_dereference` `drgn.h:3302` | `subscript(obj, 0)`과 동일 |
| `obj[a:b]` | `drgn_object_slice` `:1621` | subscript와 같되 결과 타입을 `array[b-a]`로 생성 |
| `&obj` | `drgn_object_address_of` `:1548` | REFERENCE만 가능. 포인터 타입 만들어 `address`를 UNSIGNED 값으로 |

**포인터 역참조의 진실**(`drgn_object_dereference_offset`, `object.c:647`): 포인터 object의 *값*(주소)을 `drgn_object_read_unsigned`로 읽는다 — 이때 포인터가 reference면 그 포인터 자체를 읽느라 메모리 1회 접근. 그 주소로 **새 reference를 만들 뿐, 가리키는 대상은 읽지 않는다.** 즉 `task.parent.parent.comm`처럼 체이닝해도 각 `->`마다 포인터 한 칸씩만 읽힌다.

**`address_of_`는 역참조의 역연산**: value는 주소가 없으니 "cannot take address of value" 에러(`object.c:1558`), 비트필드/비정렬 비트오프셋도 거부(`object.c:1568`). reference의 `address`를 그대로 포인터 값으로 담는다.

### 비트필드 처리

- `drgn_object_type_impl`(`object.c:108-114`)에서 `bit_field_size`가 0이 아니면 `is_bit_field=true`, `bit_size=bit_field_size`로 설정하고 정수 타입인지 검사(`object.c:169-174`).
- reference의 `bit_offset`(uint8_t, **0~7**)은 바이트 내 잔여 비트다. byte 단위 오프셋은 `address`로 접어 넣는다(`set_reference_internal`, `object.c:453-455`).
- 비정렬(bit_offset≠0)은 **스칼라에만 허용**. 버퍼(struct/array)는 "non-scalar must be byte-aligned" 에러(`object.c:456-473`).
- 읽기 시 `drgn_object_read_reference`가 `obj->bit_offset`을 보고 `copy_bits`/`deserialize_bits`로 비트 정렬(`object.c:745, 773`). 빅엔디안일 때 dst 비트오프셋을 `-bit_size % 8`로 보정(`object.c:688-690`).
- `address_of_`는 비트필드 주소를 취할 수 없어 거부(C와 동일).

## 5. cast/reinterpret/container_of + C 연산자 의미론

### cast — 값 변환 (`drgn_object_cast`, `object.c:1378`)

언어 디스패치(`lang->op_cast` → `c_op_cast`, `language_c.c:3461`). `void`→absent, `bool`→truthiness(`drgn_object_bool`)로 특수 처리하고, 나머지는 **`drgn_op_cast`(`object.c:1861`)**: 포인터면 주소를 꺼내고(`pointer_operand`, `object.c:1831`), 정수/실수면 `drgn_object_convert_*`로 변환·절단. struct/array로의 cast는 금지. **값을 새로 만든다**(원본의 비트를 재해석하지 않음).

### reinterpret — 비트 재해석 (`drgn_object_reinterpret`, `object.c:1408`)

단 한 줄: `drgn_object_fragment(res, obj, type, 0, 0)`. 오프셋 0에서 **같은 바이트를 다른 타입으로** 본다. kind(value/reference)를 보존한다. cast(값 변환)와 정반대 개념 — `(float)1`은 1.0이지만 reinterpret는 정수 1의 비트를 float로 읽는다.

### implicit_convert (`drgn_object_implicit_convert`, `object.c:1392`)

`c_op_implicit_convert`(`language_c.c:3495`). cast보다 엄격 — 타입 호환성을 검사한다(포인터 qualifier 규칙, struct 호환성). 대입/함수 인자/멤버 초기화의 암시적 변환에 쓰인다.

### container_of (`drgn_object_container_of`, `object.c:1737`)

커널 디버깅 필수기. 포인터여야 하고 → `drgn_type_offsetof(member_designator)`로 멤버 오프셋 계산 → 포인터값 읽어 → `address - offset`을 새 포인터 타입의 값으로. "멤버 주소 → 컨테이너 주소"의 역산이라 `member_dereference + address_of`의 반대다.

### C 연산자 의미론 — object.c(언어 독립) + language_c.c(C 규칙)

object.c는 **타입 무관 구현**(`drgn_op_*_impl`)을, language_c.c는 **C의 타입 승격·디스패치**를 담당한다.

- **정수 승격**(`c_integer_promotions`, `language_c.c:2909`): enum→호환 정수, `char`/`short`→`int`, 비트필드 폭 유지(GCC 방식). 단항 연산·시프트의 lhs에 적용.
- **일반 산술 변환**(`c_common_real_type`, `language_c.c:3039`): 이항 연산에서 두 피연산자의 공통 결과 타입을 정한다(부호·rank 규칙). `c_op_add`/`c_op_cmp` 등이 이걸로 결과 타입을 만들고 `drgn_op_*_impl`에 넘긴다.
- **정의된 동작 보장**(`object.h:29-33`): C가 UB로 두는 것을 drgn은 못박는다.
  - 부호 산술: **모듈러**. `BINARY_OP_SIGNED_2C`(`object.c:1935`)가 unsigned로 연산 후 signed로 되돌려 오버플로 UB를 피한다.
  - 부호 비트연산(`& | ^ ~`): **2의 보수 표현** 위에서.
  - 좌측 시프트(`object.c:2347`): shift ≥ bit_size면 0. 음수 시프트량은 에러(`shift_operand`, `object.c:2314`).
  - 우측 시프트(`object.c:2400`): 부호 있으면 **산술 시프트**(음수는 -1로 채움), 부호 없으면 논리. shift ≥ bit_size면 0/-1.
  - 나눗셈/나머지: 0으로 나누면 `drgn_zero_division` 에러; 0쪽 절단; 나머지는 피제수 부호.
- **비교**(`drgn_op_cmp_impl`, `object.c:1984`): `CMP` 매크로가 -1/0/1 반환. 포인터끼리는 주소로 비교(`drgn_op_cmp_pointers`, `object.c:2029`).
- **포인터 산술**: `drgn_op_add_to_pointer`(`object.c:2076`)는 `ptr ± index*referenced_size`; `drgn_op_sub_pointers`(`object.c:2134`)는 `(lhs-rhs)/referenced_size`를 `ptrdiff_t`로. `c_op_add`/`c_op_sub`(`language_c.c:3701, 3742`)가 포인터/정수 조합을 보고 디스패치한다.
- 한쪽 피연산자만 object면 다른 쪽은 언어의 리터럴 규칙으로 object화(`integer_literal`, `c_integer_literal` `language_c.c:2796` — int/long/long long 중 표현 가능한 최소 타입 선택). 이항 연산 래퍼는 `BINARY_OP` 매크로(`object.c:1495`)가 생성한다.

## 6. lazy object

### union drgn_lazy_object (`drgn.h:3431`)

```c
union drgn_lazy_object {
    struct drgn_object obj;        // 이미 평가됨
    struct {                       // 아직 평가 안 됨 (thunk)
        struct drgn_type *dummy_type;  // 항상 NULL (obj.type과 같은 오프셋)
        struct drgn_program *prog;
        drgn_object_thunk_fn *fn;      // 평가 콜백
        void *arg;                     // fn에 넘길 인자
    } thunk;
};
```

`obj.type`과 `thunk.dummy_type`이 **같은 오프셋**(`lazy_object.c:8`의 static_assert로 보장)이라, `obj.type == NULL`이면 "미평가"로 판정한다(`drgn_lazy_object_is_evaluated`, `lazy_object.h:51`). evaluated object의 type은 절대 NULL이 아니므로 충돌이 없다.

### 왜 필요한가

프로그램의 **object·type 그래프는 깊고 자주 순환**한다(`lazy_object.h:24-26`). 예: `struct task_struct`의 멤버 `parent`는 또 `struct task_struct *`다. 구조체를 만들 때 모든 멤버의 타입·값을 즉시 평가하면 무한 재귀거나 거대한 작업이 된다. 그래서 **멤버/파라미터/템플릿 인자를 thunk(콜백+인자)로 보관하고, 실제로 접근될 때 한 번 평가**한다.

### 쓰이는 곳

- 구조체/공용체/클래스 **멤버**: `drgn_type_member.object`(`drgn.h:3505`)
- 함수 파라미터 **기본 인자**: `drgn_type_parameter.default_argument`(`drgn.h:3538`)
- **템플릿 파라미터**: `drgn_type_template_parameter.argument`(`drgn.h:3551`)

### 생명주기

- **평가**(`drgn_lazy_object_evaluate`, `lazy_object.c:12`): 미평가면 `prog/fn/arg`를 꺼내 `drgn_object_init` 후 `fn(&obj, arg)` 호출. 실패하면 **thunk로 되돌려 재시도 가능**하게 둔다. 성공하면 평가 결과가 영구 캐시된다.
- **해제**(`drgn_lazy_object_deinit`, `lazy_object.c:30`): 평가됐으면 `obj`를 deinit, 아니면 `fn(NULL, arg)` 호출 — **res==NULL은 "arg만 free하라"는 thunk의 약속**이다.
- **프로그램 검사**(`drgn_lazy_object_check_prog`, `lazy_object.c:38`).

### 생성 (DWARF)

thunk는 DWARF 파서가 만든다. 멤버는 `drgn_lazy_object_init_thunk(&member_object, prog, drgn_dwarf_member_thunk_fn, thunk_arg)`(`dwarf_info.c:5744`), 함수 인자는 `dwarf_info.c:5856`, 기본 인자는 `dwarf_info.c:6391`. thunk_arg에 DIE 위치를 저장해 두었다가, 접근 시 콜백이 그 DIE를 파싱해 타입/값을 채운다(→ [[03-debuginfo-dwarf]]). 사용자가 `drgn_member_type()`/`drgn_member_object()`를 호출하면 내부에서 lazy object가 평가된다.

## 7. 타 서브시스템 연결

- **Program ([[02-program-memory]])**: 메모리 물질화의 종착지 `drgn_program_read_memory`. 그 외 `drgn_program_element_info`(원소 크기·타입), `drgn_program_address_mask/size`(주소 절단·포인터 폭), `drgn_pointer_type_create`(address_of/container_of의 결과 포인터). object에 prog 필드가 없어 전부 `drgn_object_program()`(타입 역추적)으로 prog를 얻는다.
- **Type ([[04-type-language]])**: object의 encoding/bit_size는 `drgn_object_type_impl`이 `drgn_type`에서 계산한다. 멤버 접근은 `drgn_type_find_member`, container_of는 `drgn_type_offsetof`, subscript는 `element_info`에 의존. `underlying_type`(typedef 해제)이 encoding 결정의 기준.
- **Language (language_c.c)**: 모든 연산자/cast/리터럴은 `lang->op_*` 가상 디스패치를 거친다. object.c는 **언어 독립 산술**(`drgn_op_*_impl`)만 제공하고, C 특유의 승격·암시 변환·포인터 규칙은 language_c.c가 결정한다. 같은 C object는 항상 C 의미론을 따른다.
- **DWARF ([[03-debuginfo-dwarf]])**: 변수의 주소를 담은 **reference object**와 멤버/인자의 **lazy thunk**를 만든다. drgn의 모든 reference는 사실상 DWARF 위치 정보에서 출발한다.
- **Python 바인딩 (`_drgn`)**: `drgn.Object`는 `struct drgn_object`를 감싼다. `__getattr__`→`member`, `__getitem__`→`subscript`/`slice`, 산술/비교 연산자→`drgn_object_*`, `read_()`→`drgn_object_read`, `value_()`→읽고 Python 타입 변환, `address_of_()`→`address_of`, `member_()`→`member`(이름 충돌 회피용).

## 8. 파일 맵

| 파일 | 줄수 | 역할 |
|------|------|------|
| `libdrgn/drgn.h:2376-2671, 3431-3556` | — | `enum drgn_object_kind/encoding/absence_reason`, `union drgn_value`, `struct drgn_object`, `union drgn_lazy_object`와 멤버/파라미터 구조체. 공개 API 선언 |
| `libdrgn/object.h` | 437 | 내부 헬퍼: `drgn_object_type`(encoding 캐시), `_internal` setter들, `drgn_object_reinit`, 연산자 impl 타입(`drgn_binary_op_impl` 등) 선언 |
| `libdrgn/object.c` | 2556 | **핵심**: setter, copy, `fragment`(접근의 토대), `read`/`read_value`/`read_reference`(메모리 물질화), 멤버/첨자/역참조/주소/slice/container_of, cast/reinterpret, 모든 `drgn_op_*_impl`(2의 보수 산술/시프트/비교/포인터 산술) |
| `libdrgn/accessors.c` | 7 | (멤버 접근 아님) 타입 필드 게터 inline들을 외부 심볼로 실체화하는 번역 단위 |
| `libdrgn/lazy_object.h` | 85 | lazy object 인터페이스: `init_thunk`, `is_evaluated`, `evaluate`, `deinit`, `check_prog` |
| `libdrgn/lazy_object.c` | 49 | thunk 평가/해제 구현, layout static_assert |
| `libdrgn/language_c.c:2796-3940` | — | C 의미론: 정수 승격, 일반 산술 변환, `c_op_cast/cmp/add/...`, C 언어 op 테이블(`.op_cast = c_op_cast` 등) |
| `libdrgn/dwarf_info.c:5744, 5856, 6391` | — | 멤버/인자/기본인자의 lazy thunk 생성 지점 |
| `_drgn.pyi:2395-2840` | — | Python `class Object`: `__getattr__`/`__getitem__`/연산자/`read_`/`value_`/`member_`/`address_of_` 시그니처와 의미 |
