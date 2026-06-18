# Familynlctrlnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/nlctrl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `nlctrl` netlink specification](#id2)

## [Summary](#id3)

genetlink meta-family that exposes information about all genetlink
families registered in the kernel (including itself).

## [Operations](#id4)

### [getfamily](#id5)

Get / dump genetlink families

attribute-set:
:   [ctrl-attrs](#nlctrl-attribute-set-ctrl-attrs)

do:
:   **request**
    :   attributes:
        :   [`family-name`]

    **reply**
    :   attributes:
        :   [`family-id`, `family-name`, `hdrsize`, `maxattr`, `mcast-groups`, `ops`, `version`]

dump:
:   **reply**
    :   attributes:
        :   [`family-id`, `family-name`, `hdrsize`, `maxattr`, `mcast-groups`, `ops`, `version`]

### [getpolicy](#id6)

Get / dump genetlink policies

attribute-set:
:   [ctrl-attrs](#nlctrl-attribute-set-ctrl-attrs)

dump:
:   **request**
    :   attributes:
        :   [`family-name`, `family-id`, `op`]

    **reply**
    :   attributes:
        :   [`family-id`, `op-policy`, `policy`]

## [Definitions](#id7)

### [op-flags](#id8)

type:
:   flags

enum-name:
:   None

entries:
:   * `admin-perm`
    * `cmd-cap-do`
    * `cmd-cap-dump`
    * `cmd-cap-haspol`
    * `uns-admin-perm`

### [attr-type](#id9)

enum-name:
:   netlink-attribute-type

type:
:   enum

entries:
:   * `invalid`
    * `flag`
    * `u8`
    * `u16`
    * `u32`
    * `u64`
    * `s8`
    * `s16`
    * `s32`
    * `s64`
    * `binary`
    * `string`
    * `nul-string`
    * `nested`
    * `nested-array`
    * `bitfield32`
    * `sint`
    * `uint`

## [Attribute sets](#id10)

### [ctrl-attrs](#id11)

#### family-id (`u16`)

#### family-name (`string`)

#### version (`u32`)

#### hdrsize (`u32`)

#### maxattr (`u32`)

#### ops (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [op-attrs](#nlctrl-attribute-set-op-attrs)

#### mcast-groups (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [mcast-group-attrs](#nlctrl-attribute-set-mcast-group-attrs)

#### policy (`nest-type-value`)

type-value:
:   [‘policy-id’, ‘attr-id’]

nested-attributes:
:   [policy-attrs](#nlctrl-attribute-set-policy-attrs)

#### op-policy (`nest-type-value`)

type-value:
:   [‘op-id’]

nested-attributes:
:   [op-policy-attrs](#nlctrl-attribute-set-op-policy-attrs)

#### op (`u32`)

### [mcast-group-attrs](#id12)

#### name (`string`)

#### id (`u32`)

### [op-attrs](#id13)

#### id (`u32`)

#### flags (`u32`)

enum:
:   [op-flags](#nlctrl-definition-op-flags)

enum-as-flags:
:   True

### [policy-attrs](#id14)

#### type (`u32`)

enum:
:   [attr-type](#nlctrl-definition-attr-type)

#### min-value-s (`s64`)

#### max-value-s (`s64`)

#### min-value-u (`u64`)

#### max-value-u (`u64`)

#### min-length (`u32`)

#### max-length (`u32`)

#### policy-idx (`u32`)

#### policy-maxtype (`u32`)

#### bitfield32-mask (`u32`)

#### mask (`u64`)

#### pad (`pad`)

### [op-policy-attrs](#id15)

#### do (`u32`)

#### dump (`u32`)
