# drgn 코드 분석 — 타입 시스템과 C 언어 파서
> 소스: osandov/drgn (main), 로컬 `deep/sources/drgn`
> 분석 파일: `libdrgn/type.h`, `libdrgn/type.c`, `libdrgn/language.h`, `libdrgn/language.c`, `libdrgn/language_c.c`, `libdrgn/drgn_internal.h`, `libdrgn/drgn.h`, `libdrgn/c_lexer.h`, `libdrgn/lexer.h`
> 관련: [[drgn-codebase-overview]] · [[03-debuginfo-dwarf]] · [[05-object]]

## 1. 역할

이 서브시스템은 두 가지를 담당한다.

1. **타입 표현 계층** (`type.c`/`type.h`/`drgn_internal.h`) — DWARF에서 읽은 디버그 정보든, 사용자가 문자열로 요청한 타입이든, 모두 단일 `struct drgn_type` 표현으로 정규화한다. 같은 타입을 메모리에서 **한 객체로 유지**(dedup)하여 포인터 비교로 동일성을 판정할 수 있게 한다.
2. **언어 추상화 + C 파서** (`language.c`/`language.h`/`language_c.c`) — `prog.type("struct foo *")` 같은 문자열을 토큰화·파싱해 실제 타입 객체로 바꾸고(역방향: 타입 객체를 사람이 읽는 C 선언으로 포매팅), 객체 연산(캐스트/산술/비교)을 언어 규칙에 맞게 분기한다.

핵심 사상: drgn은 "타입은 언어 중립적 데이터(`drgn_type`), 타입을 다루는 규칙은 언어별 콜백 테이블(`drgn_language`)"로 분리한다. C와 C++은 같은 파서/포매터(`c_family_*`)를 공유하고 플래그로만 갈린다.

## 2. drgn 타입 표현

### 2.1 `enum drgn_type_kind` (drgn.h:323-348)

12종. `__attribute__((__packed__))`로 1바이트. 값은 `DRGN_TYPE_VOID = 1`부터 시작(0은 "미설정"으로 비워둠).

| kind | 의미 | 추가 데이터 |
|------|------|------------|
| `DRGN_TYPE_VOID` | void | 없음(언어당 1개 공유) |
| `DRGN_TYPE_INT` | 정수 | name, size, is_signed, endian |
| `DRGN_TYPE_BOOL` | 불리언 | name, size, endian |
| `DRGN_TYPE_FLOAT` | 부동소수 | name, size, endian |
| `DRGN_TYPE_STRUCT` | 구조체 | tag, size, members[] |
| `DRGN_TYPE_UNION` | 공용체 | tag, size, members[] |
| `DRGN_TYPE_CLASS` | C++ 클래스 | tag, size, members[] |
| `DRGN_TYPE_ENUM` | 열거형 | tag, compatible(int)type, enumerators[] |
| `DRGN_TYPE_TYPEDEF` | 별칭 | name, aliased type |
| `DRGN_TYPE_POINTER` | 포인터 | size, referenced type, endian |
| `DRGN_TYPE_ARRAY` | 배열 | element type, length |
| `DRGN_TYPE_FUNCTION` | 함수 | return type, parameters[], is_variadic |

`drgn_type_kind_spelling[]` (type.c:32-45)이 kind→문자열 매핑을 제공한다.

### 2.2 `struct drgn_type` — 공통 헤더 + union 압축 (drgn_internal.h:35-59)

"가능한 한 작게" 만들기 위해 공통 헤더에 **세 개의 union**을 둬 kind별로 의미를 달리 쓴다. 직접 접근 금지, 게터 함수(`drgn_type_*`)로만 접근한다.

```c
struct drgn_type {
    enum drgn_type_kind     _kind;       // 12종 중 하나
    enum drgn_primitive_type _primitive; // C 기본형이면 그 종류, 아니면 DRGN_NOT_PRIMITIVE_TYPE
    enum drgn_qualifiers    _qualifiers; // ★ "이 타입"이 아니라 "_type이 가리키는 wrapped 타입"의 한정자
    enum drgn_type_flags    _flags;      // IS_COMPLETE/IS_SIGNED/LITTLE_ENDIAN/IS_VARIADIC
    struct drgn_program    *_program;    // 소유 프로그램
    const struct drgn_language *_language;// 이 타입의 언어
    union { const char *_name; const char *_tag; struct drgn_type_parameter *_parameters; };
    union { uint64_t _size; uint64_t _length; struct drgn_type_enumerator *_enumerators; size_t _num_parameters; };
    union { struct drgn_type *_type; struct drgn_type_member *_members; };
};
```

union 메모리 레이아웃을 그림으로:

```
struct drgn_type (공통 헤더, 모든 kind 공유)
┌──────────────┬──────────────┬──────────────┬──────────────┐
│ _kind        │ _primitive   │ _qualifiers  │ _flags       │  ← 메타(각 1바이트, packed)
├──────────────┴──────────────┴──────────────┴──────────────┤
│ _program          _language                               │  ← 소유/언어
├───────────────────────────────────────────────────────────┤
│ union { _name | _tag | _parameters }                      │  ← 이름 슬롯
├───────────────────────────────────────────────────────────┤
│ union { _size | _length | _enumerators | _num_parameters }│  ← 크기 슬롯
├───────────────────────────────────────────────────────────┤
│ union { _type | _members }                                │  ← 참조/구성요소 슬롯
└───────────────────────────────────────────────────────────┘
     INT은 (_name,_size) 사용, POINTER는 (_size,_type), ARRAY는 (_length,_type),
     STRUCT는 (_tag, _members)+별도 _num_members, FUNCTION은 (_parameters,_num_parameters,_type=리턴)
```

`_flags` 비트(drgn_internal.h:23-27): `IS_COMPLETE`(불완전 전방선언 여부), `IS_SIGNED`, `LITTLE_ENDIAN`(엔디안), `IS_VARIADIC`(함수 가변인자).

**`_qualifiers`의 비대칭성이 핵심 트릭**: 헤더의 `_qualifiers`는 *자기 자신*이 아니라 *감싼 타입*(`_type`이 가리키는 referenced/aliased/element/return 타입)의 한정자다. 그래서 `drgn_type_type()`은 `{._type, ._qualifiers}`를 묶어 `drgn_qualified_type`로 반환한다(drgn_internal.h:164-172). 타입 자신의 한정자는 별도 `drgn_qualified_type` 래퍼로 표현한다(§3.3).

### 2.3 확장 구조체 (drgn_internal.h:61-83)

kind에 따라 추가 필드가 필요하면 `drgn_type`을 **첫 멤버로 임베드**한 더 큰 구조체를 malloc한다. 게터는 kind를 보고 안전하게 다운캐스트한다.

- `drgn_extended_type` = `drgn_type` + `_die_addr`(DWARF DIE 주소; struct/union/class/enum/function만, drgn_internal.h:231-256). DWARF 레이어와의 유일한 결합점.
- `drgn_templated_type` = `extended` + 템플릿 파라미터(C++).
- `drgn_compound_type` = `templated` + `_num_members`.
- `drgn_enum_type` = `extended` + `_num_enumerators`.

멤버/열거자/파라미터의 실데이터는 `union drgn_lazy_object`로 들고 있어(drgn.h:3498-3556) **지연 평가**된다 — 멤버 타입은 처음 접근될 때 `drgn_lazy_object_evaluate()`로 풀린다(type.c:194-219).

## 3. 타입 생성·캐싱·중복제거 + qualifiers

### 3.1 두 갈래 저장 전략

| 종류 | 저장소 | 중복제거? |
|------|--------|----------|
| int/bool/float/typedef/pointer/array | `prog->dedupe_types` (해시셋) | **O** (`find_or_create_type`) |
| struct/union/class/enum/function | `prog->created_types` (벡터) | X (항상 새 malloc) |
| void | `prog->void_types[lang->number]` | 언어당 1개 고정 |
| C 기본형 | `prog->primitive_types[]` 캐시 | 프로그램당 1개 |

이유(type.c:329-331): struct/enum/function은 members/enumerators/parameters 같은 가변 데이터를 갖고 DWARF DIE와 1:1 대응하므로 dedup 대상에서 제외한다(dedup 해시/비교 함수가 이 속성들을 무시함).

### 3.2 `find_or_create_type` (type.c:336-359)

스택에 임시 `key`를 채워 호출 → `drgn_dedupe_type_set`에서 검색 → 있으면 기존 포인터 반환, 없으면 malloc 후 삽입. 해시/비교(`drgn_type_dedupe_hash_pair`/`_eq`, type.c:269-327)는 (kind, complete, language, name, size, is_signed, little_endian, referenced type+quals, length)를 키로 한다. 그래서 `int *`를 두 번 만들어도 **포인터가 같다** → 타입 동일성을 O(1) 포인터 비교로 판정.

`drgn_int_type_create` 등(type.c:393-482)은 먼저 `c_parse_specifier_list(name)`로 이름이 C 기본형인지 보고 `_primitive`를 채운 뒤 `find_or_create_type`를 부른다. compound/enum/function은 **builder** 패턴(type.c:524-914): 벡터에 멤버를 모은 뒤 `*_create`가 벡터 소유권을 steal해 `created_types`에 append.

builder 수명 주기(예: 구조체):
```
drgn_compound_type_builder_init(&b, prog, DRGN_TYPE_STRUCT)
  → 반복: drgn_compound_type_builder_add_member(&b, lazy_obj, name, bit_offset)  # 멤버를 벡터에 누적
  → drgn_compound_type_create(&b, tag, size, is_complete, lang, &ret)
        ├ shrink_to_fit + malloc(drgn_compound_type)
        ├ 벡터를 _members/_num_members로 steal (소유권 이전, 복사 없음)
        └ created_types 벡터에 등록 (deinit 때 일괄 free)
  실패 시 drgn_compound_type_builder_deinit(&b)가 lazy_object까지 정리
```
불완전 타입은 멤버가 없고 size=0이어야 한다는 불변식을 `_create`가 검증한다(type.c:573-582).

### 3.3 엔디안 변형 — `drgn_type_with_byte_order` (type.c:1004-1043)

같은 타입을 반대 엔디안으로 재생성하는 헬퍼. int/bool/float/pointer는 `*_create`로 다시 만들고(dedup되므로 비용 작음), typedef/enum은 내부 타입을 재귀로 바꾼 뒤 재조립한다. 다중 엔디안 코어덤프(혼합 아키텍처) 해석에 쓰인다.

### 3.4 qualifiers & `drgn_qualified_type`

`enum drgn_qualifiers`(drgn.h:358-369)는 비트마스크: CONST(1<<0)/VOLATILE(1<<1)/RESTRICT(1<<2)/ATOMIC(1<<3). `struct drgn_qualified_type { drgn_type *type; enum drgn_qualifiers qualifiers; }`(drgn.h:380-385)가 "한정자 붙은 타입"을 표현하는 값 타입이다 — 타입 객체 자체는 dedup되므로 한정자는 객체 밖 래퍼에 둔다(`const int`와 `int`는 같은 `drgn_type` + 다른 qualifiers).

`drgn_qualified_type_unaliased()`(type.c:20-30)는 typedef를 벗기면서 한정자를 누적 OR한다. `drgn_underlying_type()`(type.h:484-492)은 typedef만 재귀적으로 벗긴 raw 타입을 준다.

## 4. 문자열→타입: C 렉서/파서 경로

`prog.type("struct foo *")` 호출 흐름:

```
drgn_program_find_type (type.c:1415)            # 언어 디스패치
  └ lang->find_type = c_family_find_type (language_c.c:2607)
      ├ DRGN_C_FAMILY_LEXER(...)                # 렉서 인스턴스 (c_lexer.h)
      ├ c_parse_specifier_qualifier_list (2187) # "struct foo" → 기반 타입
      │   └ drgn_program_find_type_impl (type.c:1387)  # 등록된 type finder 순회
      │       └ DWARF type finder → drgn_*_type_create  # [[03-debuginfo-dwarf]]
      ├ c_parse_abstract_declarator (2530)      # "*" → c_declarator 연결리스트
      └ c_type_from_declarator (2566)           # 안→밖으로 drgn_pointer_type_create
```

### 4.1 렉서 (language_c.c:1717-1830, `drgn_c_family_lexer_func`)

손으로 짠 문자 단위 스캐너. `drgn_lexer`(lexer.h:69-76)는 함수+위치+토큰 스택 구조로, `drgn_lexer_pop/push/peek`로 1토큰 룩어헤드를 제공한다. 토큰 종류는 `c_lexer.h`의 `C_TOKEN_*` enum: 괄호/대괄호/`*`/`.`/`:` 등 구두점, 키워드(VOID..ENUM, 범위 매크로 `MIN/MAX_SPECIFIER_TOKEN`, `MIN/MAX_QUALIFIER_TOKEN`), `C_TOKEN_NUMBER`(10/8/16진), `C_TOKEN_IDENTIFIER`. 식별자는 `identifier_token_kind()`(생성 파일 `c_keywords.inc`, language_c.c:1715 include — 빌드 시 `build-aux/gen_c_keywords_inc_strswitch.py`가 만드는 완전해시 string-switch)로 키워드/일반 식별자를 구분한다. C++ 모드(`cpp` 플래그)에서는 `<...>`를 통째로 `C_TOKEN_TEMPLATE_ARGUMENTS`로 스캔하는 해킹이 있다(1758-1789).

### 4.2 specifier-qualifier-list 파서 (language_c.c:2187-2344)

토큰을 반복적으로 pop하며 세 갈래 처리:
- **한정자 토큰**(const 등) → `qualifier_from_token[]`(1962-1967)로 비트 누적.
- **기본형 specifier**(int/long/...) → 상태기계 `specifier_transition[state][token]`(1969-2082)로 전이. 예: `unsigned`→`SPECIFIER_UNSIGNED`→(`long`)→`SPECIFIER_UNSIGNED_LONG`. 잘못된 조합은 `SPECIFIER_ERROR`. 최종 상태를 `specifier_kind[]`로 `drgn_primitive_type`에 매핑해 `drgn_program_find_primitive_type`로 해결. 이 상태기계가 "long unsigned int" 같은 모든 어순 조합을 흡수한다(`drgn_primitive_type_spellings`, type.c:57-123에 모든 철자 등록).
- **struct/union/class/enum 태그 또는 식별자(typedef)** → 다음 식별자를 읽어 `drgn_program_find_type_impl(prog, kinds, name, ...)` 호출. `kinds`는 비트마스크(`1<<DRGN_TYPE_STRUCT` 등)로 "어떤 종류를 찾을지" 제한한다. 순수 식별자는 C에선 typedef로, C++에선 struct/union/class/enum/typedef 전부 후보(2314-2322). `size_t`/`ptrdiff_t`는 특별 처리(2293-2313).

`drgn_program_find_type_impl`(type.c:1387-1413)은 등록된 **type finder** 핸들러들을 순회한다 — 실제 DWARF 인덱스 조회는 여기서 일어나며, 반환 타입의 program/kind 검증(잘못된 program/kind 반환 시 에러) 후 `drgn_qualified_type`를 돌려준다.

기본형 해소 `drgn_program_find_primitive_type`(type.c:1481-1606)는 (1) program 캐시 확인 → (2) 모든 철자로 DWARF에서 검색 → (3) 없으면 **합성**한다. `long`/`unsigned long`은 워드 크기로(1550-1561), `size_t`/`ptrdiff_t`는 워드 크기와 일치하는 정수 typedef로(`default_size_t_or_ptrdiff_t`, type.c:1439-1479) 만든다. 즉 디버그 정보가 없어도 기본형은 항상 얻을 수 있다.

### 4.3 선언자(declarator) 파싱과 타입 조립 (language_c.c:2346-2605)

`*`/`[]`는 `c_declarator` 연결리스트(2346-2354)로 누적한다. C 선언 문법이 "안에서 밖으로" 읽히므로(`int *a[3]`은 "int를 가리키는 포인터 3개 배열"), `c_type_from_declarator`(2566)는 리스트를 **재귀로 끝까지 내려간 뒤 돌아오며** `drgn_pointer_type_create`/`drgn_array_type_create`로 감싼다. 즉 기반 타입에서 시작해 가장 안쪽 선언자부터 차례로 래핑. 포인터 크기는 `drgn_program_address_size`로 채운다. 함수 포인터 타입은 미구현(2509). 끝에 잔여 토큰이 있으면 "extra tokens" 에러(2652).

## 5. 언어 추상화 (`drgn_language` 훅)

`struct drgn_language`(language.h:95-169)는 **언어별 콜백 테이블**이다. 필드:
- 메타: `name`, `number`(`DRGN_LANGUAGE_C`/`_CPP`), `has_namespaces`.
- 포매팅: `format_type_name`, `format_type`, `format_variable_declaration`, `format_object`.
- 파싱/탐색: `find_type`(문자열→타입), `type_subobject`(`a.b[3]` 같은 designator→하위객체 타입/오프셋).
- 리터럴 생성자: `integer_literal`/`bool_literal`/`float_literal`.
- 연산자: `op_cast`, `op_implicit_convert`, `op_bool`, `op_cmp`, `op_add`...`op_not`. 객체 산술/비교/캐스트의 타입 규칙(정수 승격, 공통 실수 타입 등)을 언어가 책임진다.

`drgn_languages[]`(language.c:10-13)가 number→구현 매핑이고 기본은 C(`drgn_default_language`, language.h:175). C와 C++는 `c_family_*` 함수를 **공유**하고(language_c.c:3882-3944), 차이는 `has_namespaces`, 렉서의 `cpp` 플래그(템플릿/식별자 후보 종류), C++ 키워드 셋뿐이다. 새 언어 추가 절차는 language.h:27-34 주석에 명시.

**언어 훅이 쓰이는 지점**:
- `drgn_format_type*`(type.c:1045-1065) → `drgn_type_language(type)->format_*` 디스패치.
- `drgn_program_find_type`(type.c:1415) → `prog` 언어의 `find_type`.
- `drgn_type_offsetof`/멤버 탐색(type.c:1655-1673) → `lang->type_subobject` = `c_family_type_subobject`(language_c.c:2661). 같은 렉서로 designator를 토큰화해 `.멤버`/`[index]`를 따라가며 비트 오프셋을 누적([[05-object]]의 멤버 접근이 여기로 들어옴).
- 객체 평가(산술/캐스트)는 `lang->op_*`를 호출.

## 6. 타입 포매팅 (`drgn_format_type*`)

타입 객체 → 사람이 읽는 C 선언 문자열. 세 진입점(language_c.c):
- `c_format_type_name`(551) — 선언 형태만(`struct foo *`). 불완전해도 OK.
- `c_format_type`(565) — 완전하면 **정의 전체**(멤버 나열)까지, 불완전하면 이름만.
- `c_format_variable_declaration`(582) — `T name` 형태.

C 선언 문법("declaration follows use")을 구현하려고 **string_callback 체이닝**으로 이름을 안쪽→바깥쪽으로 흘려보낸다. `c_declare_variable`(346)이 kind로 분기:
- 기본형/typedef → `c_declare_basic`(80): `[한정자] 이름 [변수명]`.
- struct/union/class/enum → `c_declare_tagged`(162): `struct tag`(익명이면 `<anonymous>` 또는 본문 전개).
- 포인터 → `c_declare_pointer`(229): referenced 타입을 재귀 선언하되 이름 콜백을 `c_pointer_name`으로 감싸 `*`를 끼워 넣고, referenced가 배열/함수면 **괄호**를 친다(201-203) — `int (*p)[3]`.
- 배열 → `c_array_name`(246)으로 `[N]` 부착, 함수 → `c_declare_function`(284)으로 `ret name(params)`.

정의 전개는 `c_define_compound`/`c_define_enum`/`c_define_typedef`(375-498)가 멤버를 `c_declare_variable(..., indent+1)`로 한 줄씩, 비트필드는 ` : N`까지 출력한다. 즉 포매팅은 파싱의 역연산이며, 같은 kind 분기 구조를 거울처럼 사용한다.

`int (*p)[3]` 포매팅 예시(콜백이 이름을 안→밖으로 운반):
```
c_declare_variable(pointer_to_array_of_int, name="p")
  └ c_declare_pointer → c_declare_variable(array_of_int, name=c_pointer_name["*p"])
       └ c_declare_array → c_declare_variable(int, name=c_array_name["(*p)[3]"])
            └ c_declare_basic → "int (*p)[3]"
  ※ referenced가 ARRAY라서 c_pointer_name이 괄호를 침: "(*p)" → 배열이 "[3]"을 붙임
```

`format_object`(language_c.c:1699) 경로는 별도로 값까지 렌더링한다(정수/실수/문자열/구조체 초기화자 형태). 타입 이름이 필요한 지점마다 `c_format_type_name_impl`을 재사용한다(예: 포인터 심볼라이즈 1348, 캐스트 표기 1648).

## 7. 타 서브시스템 연결

- **DWARF/디버그정보 ([[03-debuginfo-dwarf]])**: type finder가 DWARF DIE를 읽어 `drgn_*_type_create`/builder로 `drgn_type`을 만든다. struct/enum/function 타입에만 있는 `_die_addr`가 유일한 역참조 고리(정렬 계산 `drgn_dwarf_type_alignment` 등). `find_type_impl`이 finder를 순회하는 진입점.
- **객체 ([[05-object]])**: 모든 `drgn_object`는 `drgn_qualified_type`을 들고 다닌다. 멤버 접근(`obj.member`)은 `type_subobject`/`drgn_type_find_member`(type.c:1675-1761, `prog->members` 해시 캐시)로, 산술/캐스트는 `lang->op_*`로 위임된다. 리터럴은 `integer/bool/float_literal`로 타입이 정해진다.
- **program ([[02-program-memory]])**: 모든 타입은 `prog`가 소유하고 수명도 프로그램과 같다. `dedupe_types`/`created_types`/`void_types`/`primitive_types`/`members` 캐시가 전부 `drgn_program`에 산다(type.c:1322-1336 init, 1338-1385 deinit).
- **Python 바인딩**: `prog.type("...")`, `type.members`, `str(type)`가 각각 `drgn_program_find_type`, 멤버 게터, `drgn_format_type`로 내려온다.

## 8. 파일 맵

| 파일 | 라인수 | 역할 | 핵심 심볼 |
|------|--------|------|-----------|
| `libdrgn/drgn.h` | — | 공개 타입 ABI | `enum drgn_type_kind`(323), `drgn_qualifiers`(358), `drgn_qualified_type`(380), member/enumerator/parameter 구조체(3498-) |
| `libdrgn/drgn_internal.h` | 259 | `struct drgn_type` 내부 표현 + 게터 | `drgn_type`(35), 확장 구조체(61-83), `drgn_type_kind/size/type/...`(85-) |
| `libdrgn/type.h` | 597 | 타입 생성 API + 헬퍼 | `drgn_*_type_create`, builder 구조체, `drgn_underlying_type`(484), `find_type_impl`(583) |
| `libdrgn/type.c` | 1776 | 생성/dedup/캐싱/탐색/포매팅 디스패치 | `find_or_create_type`(336), dedup 해시(269), `drgn_program_find_primitive_type`(1481), `drgn_type_find_member_impl`(1675) |
| `libdrgn/language.h` | 215 | `drgn_language` 콜백 테이블 정의 | `drgn_language`(95), `drgn_language_number`(43) |
| `libdrgn/language.c` | 20 | 언어 레지스트리 | `drgn_languages[]`(10), `drgn_language_name` |
| `libdrgn/language_c.c` | 3944 | C/C++ 렉서·파서·포매터·연산자 | 렉서(1717), specifier 상태기계(1969), `c_parse_specifier_qualifier_list`(2187), 선언자 파서(2357-2605), `c_family_find_type`(2607), `c_family_type_subobject`(2661), 포매터(28-598), `drgn_language_c/cpp`(3882/3914) |
| `libdrgn/c_lexer.h` | 73 | C 토큰 enum + 렉서 구조 | `C_TOKEN_*`, `drgn_c_family_lexer` |
| `libdrgn/lexer.h` | 140 | 범용 렉서 스택 추상화 | `drgn_lexer`, `drgn_lexer_pop/push/peek` |
| `libdrgn/c_keywords.inc` | (생성) | 키워드 완전해시 string-switch | `identifier_token_kind`, `keyword_spelling[]` |
