# Familyrt-rulenetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/rt-rule.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `rt-rule` netlink specification](#id1)

## [Summary](#id2)

FIB rule management over rtnetlink.

## [Operations](#id3)

### [newrule](#id4)

Add new FIB rule

attribute-set:
:   [fib-rule-attrs](#rt-rule-attribute-set-fib-rule-attrs)

do:
:   **request**
    :   attributes:
        :   [`iifname`, `oifname`, `priority`, `fwmark`, `flow`, `tun-id`, `fwmask`, `table`, `suppress-prefixlen`, `suppress-ifgroup`, `goto`, `l3mdev`, `uid-range`, `protocol`, `ip-proto`, `sport-range`, `dport-range`, `dscp`, `flowlabel`, `flowlabel-mask`, `sport-mask`, `dport-mask`, `dscp-mask`]

### [newrule-ntf](#id5)

Notify a rule creation

value:
:   32

notify:
:   getrule

### [delrule](#id6)

Remove an existing FIB rule

attribute-set:
:   [fib-rule-attrs](#rt-rule-attribute-set-fib-rule-attrs)

do:
:   **request**
    :   attributes:
        :   [`iifname`, `oifname`, `priority`, `fwmark`, `flow`, `tun-id`, `fwmask`, `table`, `suppress-prefixlen`, `suppress-ifgroup`, `goto`, `l3mdev`, `uid-range`, `protocol`, `ip-proto`, `sport-range`, `dport-range`, `dscp`, `flowlabel`, `flowlabel-mask`, `sport-mask`, `dport-mask`, `dscp-mask`]

### [delrule-ntf](#id7)

Notify a rule deletion

value:
:   33

notify:
:   getrule

### [getrule](#id8)

Dump all FIB rules

attribute-set:
:   [fib-rule-attrs](#rt-rule-attribute-set-fib-rule-attrs)

dump:
:   **request**

    **reply**
    :   attributes:
        :   [`iifname`, `oifname`, `priority`, `fwmark`, `flow`, `tun-id`, `fwmask`, `table`, `suppress-prefixlen`, `suppress-ifgroup`, `goto`, `l3mdev`, `uid-range`, `protocol`, `ip-proto`, `sport-range`, `dport-range`, `dscp`, `flowlabel`, `flowlabel-mask`, `sport-mask`, `dport-mask`, `dscp-mask`]

## [Multicast groups](#id9)

* rtnlgrp-ipv4-rule
* rtnlgrp-ipv6-rule

## [Definitions](#id10)

### [rtgenmsg](#id11)

type:
:   struct

members:
:   family (`u8`):

### [fib-rule-hdr](#id12)

type:
:   struct

members:
:   family (`u8`):


    dst-len (`u8`):


    src-len (`u8`):


    tos (`u8`):


    table (`u8`):


    res1 (`pad`):


    res2 (`pad`):


    action (`u8`):


    flags (`u32`):

### [fr-act](#id13)

type:
:   enum

enum-name:
:   None

entries:
:   * `unspec`
    * `to-tbl`
    * `goto`
    * `nop`
    * `res3`
    * `res4`
    * `blackhole`
    * `unreachable`
    * `prohibit`

### [fib-rule-port-range](#id14)

type:
:   struct

members:
:   start (`u16`):


    end (`u16`):

### [fib-rule-uid-range](#id15)

type:
:   struct

members:
:   start (`u32`):


    end (`u32`):

## [Attribute sets](#id16)

### [fib-rule-attrs](#id17)

#### dst (`binary`)

display-hint:
:   ipv4-or-v6

#### src (`binary`)

display-hint:
:   ipv4-or-v6

#### iifname (`string`)

#### goto (`u32`)

#### unused2 (`pad`)

#### priority (`u32`)

#### unused3 (`pad`)

#### unused4 (`pad`)

#### unused5 (`pad`)

#### fwmark (`u32`)

display-hint:
:   hex

#### flow (`u32`)

#### tun-id (`u64`)

#### suppress-ifgroup (`u32`)

#### suppress-prefixlen (`u32`)

display-hint:
:   hex

#### table (`u32`)

#### fwmask (`u32`)

display-hint:
:   hex

#### oifname (`string`)

#### pad (`pad`)

#### l3mdev (`u8`)

#### uid-range (`binary`)

struct:
:   [fib-rule-uid-range](#rt-rule-definition-fib-rule-uid-range)

#### protocol (`u8`)

#### ip-proto (`u8`)

#### sport-range (`binary`)

struct:
:   [fib-rule-port-range](#rt-rule-definition-fib-rule-port-range)

#### dport-range (`binary`)

struct:
:   [fib-rule-port-range](#rt-rule-definition-fib-rule-port-range)

#### dscp (`u8`)

#### flowlabel (`u32`)

byte-order:
:   big-endian

display-hint:
:   hex

#### flowlabel-mask (`u32`)

byte-order:
:   big-endian

display-hint:
:   hex

#### sport-mask (`u16`)

display-hint:
:   hex

#### dport-mask (`u16`)

display-hint:
:   hex

#### dscp-mask (`u8`)

display-hint:
:   hex
