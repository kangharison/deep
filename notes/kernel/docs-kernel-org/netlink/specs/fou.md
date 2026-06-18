# Familyfounetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/fou.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `fou` netlink specification](#id1)

## [Summary](#id2)

Foo-over-UDP.

## [Operations](#id3)

### [unspec](#id4)

unused

value:
:   0

### [add](#id5)

Add port.

attribute-set:
:   [fou](#fou-attribute-set-fou)

dont-validate:
:   [‘strict’, ‘dump’]

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`port`, `ipproto`, `type`, `remcsum-nopartial`, `local-v4`, `peer-v4`, `local-v6`, `peer-v6`, `peer-port`, `ifindex`]

### [del](#id6)

Delete port.

attribute-set:
:   [fou](#fou-attribute-set-fou)

dont-validate:
:   [‘strict’, ‘dump’]

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`af`, `ifindex`, `port`, `peer-port`, `local-v4`, `peer-v4`, `local-v6`, `peer-v6`]

### [get](#id7)

Get tunnel info.

attribute-set:
:   [fou](#fou-attribute-set-fou)

dont-validate:
:   [‘strict’, ‘dump’]

do:
:   **request**
    :   attributes:
        :   [`af`, `ifindex`, `port`, `peer-port`, `local-v4`, `peer-v4`, `local-v6`, `peer-v6`]

    **reply**
    :   attributes:
        :   [`port`, `ipproto`, `type`, `remcsum-nopartial`, `local-v4`, `peer-v4`, `local-v6`, `peer-v6`, `peer-port`, `ifindex`]

dump:
:   **reply**
    :   attributes:
        :   [`port`, `ipproto`, `type`, `remcsum-nopartial`, `local-v4`, `peer-v4`, `local-v6`, `peer-v6`, `peer-port`, `ifindex`]

## [Definitions](#id8)

### [encap-type](#id9)

type:
:   enum

name-prefix:
:   fou-encap-

enum-name:
:   None

entries:
:   * `unspec`
    * `direct`
    * `gue`

## [Attribute sets](#id10)

### [fou](#id11)

#### unspec (`unused`)

value:
:   0

#### port (`u16`)

byte-order:
:   big-endian

#### af (`u8`)

#### ipproto (`u8`)

#### type (`u8`)

#### remcsum-nopartial (`flag`)

#### local-v4 (`u32`)

#### local-v6 (`binary`)

#### peer-v4 (`u32`)

#### peer-v6 (`binary`)

#### peer-port (`u16`)

byte-order:
:   big-endian

#### ifindex (`s32`)
