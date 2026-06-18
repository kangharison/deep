# Familyhandshakenetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/handshake.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `handshake` netlink specification](#id4)

## [Summary](#id5)

Netlink protocol to request a transport layer security handshake.

## [Operations](#id6)

### [ready](#id7)

Notify handlers that a new handshake request is waiting

notify:
:   accept

### [accept](#id8)

Handler retrieves next queued handshake request

attribute-set:
:   [accept](#handshake-attribute-set-accept)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`handler-class`]

    **reply**
    :   attributes:
        :   [`sockfd`, `message-type`, `timeout`, `auth-mode`, `peer-identity`, `certificate`, `peername`, `keyring`]

### [done](#id9)

Handler reports handshake completion

attribute-set:
:   [done](#handshake-attribute-set-done)

do:
:   **request**
    :   attributes:
        :   [`status`, `sockfd`, `remote-auth`]

## [Multicast groups](#id10)

* none
* tlshd

## [Definitions](#id11)

### [max-errno](#id12)

type:
:   const

value:
:   4095

header:
:   linux/err.h

scope:
:   kernel

### [handler-class](#id13)

type:
:   enum

value-start:
:   0

entries:
:   * `none`
    * `tlshd`
    * `max`

### [msg-type](#id14)

type:
:   enum

value-start:
:   0

entries:
:   * `unspec`
    * `clienthello`
    * `serverhello`

### [auth](#id15)

type:
:   enum

value-start:
:   0

entries:
:   * `unspec`
    * `unauth`
    * `psk`
    * `x509`

## [Attribute sets](#id16)

### [x509](#id17)

#### cert (`s32`)

#### privkey (`s32`)

### [accept](#id18)

#### sockfd (`s32`)

#### handler-class (`u32`)

enum:
:   [handler-class](#handshake-definition-handler-class)

#### message-type (`u32`)

enum:
:   [msg-type](#handshake-definition-msg-type)

#### timeout (`u32`)

#### auth-mode (`u32`)

enum:
:   [auth](#handshake-definition-auth)

#### peer-identity (`u32`)

multi-attr:
:   True

#### certificate (`nest`)

nested-attributes:
:   [x509](#handshake-attribute-set-x509)

multi-attr:
:   True

#### peername (`string`)

#### keyring (`u32`)

### [done](#id19)

#### status (`u32`)

#### sockfd (`s32`)

#### remote-auth (`u32`)

multi-attr:
:   True
