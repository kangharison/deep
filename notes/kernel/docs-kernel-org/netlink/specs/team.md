# Familyteamnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/team.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `team` netlink specification](#id5)

## [Summary](#id6)

Network team device driver.

## [Operations](#id7)

### [noop](#id8)

No operation

value:
:   0

attribute-set:
:   [team](#team-attribute-set-team)

dont-validate:
:   [‘strict’]

do:
:   **reply**
    :   attributes:
        :   [`team-ifindex`]

### [options-set](#id9)

Set team options

attribute-set:
:   [team](#team-attribute-set-team)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`team-ifindex`, `list-option`]

    **reply**
    :   attributes:
        :   [`team-ifindex`, `list-option`]

### [options-get](#id10)

Get team options info

attribute-set:
:   [team](#team-attribute-set-team)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`team-ifindex`]

    **reply**
    :   attributes:
        :   [`team-ifindex`, `list-option`]

### [port-list-get](#id11)

Get team ports info

attribute-set:
:   [team](#team-attribute-set-team)

dont-validate:
:   [‘strict’]

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`team-ifindex`]

    **reply**
    :   attributes:
        :   [`team-ifindex`, `list-port`]

## [Definitions](#id12)

### [string-max-len](#id13)

type:
:   const

value:
:   32

### [genl-change-event-mc-grp-name](#id14)

type:
:   const

value:
:   change\_event

## [Attribute sets](#id15)

### [team](#id16)

The team nested layout of get/set msg looks like:

```
[TEAM_ATTR_LIST_OPTION]
    [TEAM_ATTR_ITEM_OPTION]
        [TEAM_ATTR_OPTION_*], ...
    [TEAM_ATTR_ITEM_OPTION]
        [TEAM_ATTR_OPTION_*], ...
    ...
[TEAM_ATTR_LIST_PORT]
    [TEAM_ATTR_ITEM_PORT]
        [TEAM_ATTR_PORT_*], ...
    [TEAM_ATTR_ITEM_PORT]
        [TEAM_ATTR_PORT_*], ...
    ...
```

#### unspec (`unused`)

value:
:   0

#### team-ifindex (`u32`)

#### list-option (`nest`)

nested-attributes:
:   [item-option](#team-attribute-set-item-option)

#### list-port (`nest`)

nested-attributes:
:   [item-port](#team-attribute-set-item-port)

### [item-option](#id17)

#### option-unspec (`unused`)

value:
:   0

#### option (`nest`)

nested-attributes:
:   [attr-option](#team-attribute-set-attr-option)

### [attr-option](#id18)

#### unspec (`unused`)

value:
:   0

#### name (`string`)

#### changed (`flag`)

#### type (`u8`)

#### data (`binary`)

#### removed (`flag`)

#### port-ifindex (`u32`)

doc:
:   for per-port options

#### array-index (`u32`)

doc:
:   for array options

### [item-port](#id19)

#### port-unspec (`unused`)

value:
:   0

#### port (`nest`)

nested-attributes:
:   [attr-port](#team-attribute-set-attr-port)

### [attr-port](#id20)

#### unspec (`unused`)

value:
:   0

#### ifindex (`u32`)

#### changed (`flag`)

#### linkup (`flag`)

#### speed (`u32`)

#### duplex (`u8`)

#### removed (`flag`)
