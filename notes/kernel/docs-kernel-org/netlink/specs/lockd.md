# Familylockdnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/lockd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `lockd` netlink specification](#id1)

## [Summary](#id2)

lockd configuration over generic netlink

## [Operations](#id3)

### [server-set](#id4)

set the lockd server parameters

attribute-set:
:   [server](#lockd-attribute-set-server)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`gracetime`, `tcp-port`, `udp-port`]

### [server-get](#id5)

get the lockd server parameters

attribute-set:
:   [server](#lockd-attribute-set-server)

do:
:   **reply**
    :   attributes:
        :   [`gracetime`, `tcp-port`, `udp-port`]

## [Attribute sets](#id6)

### [server](#id7)

#### gracetime (`u32`)

#### tcp-port (`u16`)

#### udp-port (`u16`)
