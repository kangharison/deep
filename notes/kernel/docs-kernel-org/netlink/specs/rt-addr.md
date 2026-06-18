# Familyrt-addrnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/rt-addr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `rt-addr` netlink specification](#id1)

## [Summary](#id2)

Address configuration over rtnetlink.

## [Operations](#id3)

### [newaddr](#id4)

Add new address

attribute-set:
:   [addr-attrs](#rt-addr-attribute-set-addr-attrs)

do:
:   **request**
    :   attributes:
        :   [`address`, `label`, `local`, `cacheinfo`]

### [deladdr](#id5)

Remove address

attribute-set:
:   [addr-attrs](#rt-addr-attribute-set-addr-attrs)

do:
:   **request**
    :   attributes:
        :   [`address`, `local`]

### [getaddr](#id6)

Dump address information.

attribute-set:
:   [addr-attrs](#rt-addr-attribute-set-addr-attrs)

dump:
:   **request**
    :   attributes:
        :   []

    **reply**
    :   attributes:
        :   [`address`, `label`, `local`, `cacheinfo`]

### [getmulticast](#id7)

Get / dump IPv4/IPv6 multicast addresses.

attribute-set:
:   [addr-attrs](#rt-addr-attribute-set-addr-attrs)

fixed-header:
:   [ifaddrmsg](#rt-addr-definition-ifaddrmsg)

do:
:   **request**
    :   attributes:
        :   []

    **reply**
    :   attributes:
        :   [`multicast`, `cacheinfo`]

dump:
:   **request**
    :   attributes:
        :   []

    **reply**
    :   attributes:
        :   [`multicast`, `cacheinfo`]

## [Multicast groups](#id8)

* rtnlgrp-ipv4-ifaddr
* rtnlgrp-ipv6-ifaddr

## [Definitions](#id9)

### [ifaddrmsg](#id10)

type:
:   struct

members:
:   ifa-family (`u8`):


    ifa-prefixlen (`u8`):


    ifa-flags (`u8`):


    ifa-scope (`u8`):


    ifa-index (`u32`):

### [ifa-cacheinfo](#id11)

type:
:   struct

members:
:   ifa-prefered (`u32`):


    ifa-valid (`u32`):


    cstamp (`u32`):


    tstamp (`u32`):

### [ifa-flags](#id12)

type:
:   flags

name-prefix:
:   ifa-f-

enum-name:
:   None

entries:
:   secondary:


    nodad:


    optimistic:


    dadfailed:


    homeaddress:


    deprecated:


    tentative:


    permanent:


    managetempaddr:


    noprefixroute:


    mcautojoin:


    stable-privacy:

## [Attribute sets](#id13)

### [addr-attrs](#id14)

#### address (`binary`)

display-hint:
:   ipv4-or-v6

#### local (`binary`)

display-hint:
:   ipv4-or-v6

#### label (`string`)

#### broadcast (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### anycast (`binary`)

#### cacheinfo (`binary`)

struct:
:   [ifa-cacheinfo](#rt-addr-definition-ifa-cacheinfo)

#### multicast (`binary`)

#### flags (`u32`)

enum:
:   [ifa-flags](#rt-addr-definition-ifa-flags)

enum-as-flags:
:   True

#### rt-priority (`u32`)

#### target-netnsid (`binary`)

#### proto (`u8`)
