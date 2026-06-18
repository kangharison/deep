# Familyrt-neighnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/rt-neigh.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `rt-neigh` netlink specification](#id3)

## [Summary](#id4)

IP neighbour management over rtnetlink.

## [Operations](#id5)

### [newneigh](#id6)

Add new neighbour entry

fixed-header:
:   [ndmsg](#rt-neigh-definition-ndmsg)

attribute-set:
:   [neighbour-attrs](#rt-neigh-attribute-set-neighbour-attrs)

do:
:   **request**
    :   attributes:
        :   [`dst`, `lladdr`, `probes`, `vlan`, `port`, `vni`, `ifindex`, `master`, `protocol`, `nh-id`, `flags-ext`, `fdb-ext-attrs`]

### [delneigh](#id7)

Remove an existing neighbour entry

fixed-header:
:   [ndmsg](#rt-neigh-definition-ndmsg)

attribute-set:
:   [neighbour-attrs](#rt-neigh-attribute-set-neighbour-attrs)

do:
:   **request**
    :   attributes:
        :   [`dst`, `ifindex`]

### [delneigh-ntf](#id8)

Notify a neighbour deletion

value:
:   29

notify:
:   getneigh

fixed-header:
:   [ndmsg](#rt-neigh-definition-ndmsg)

### [getneigh](#id9)

Get or dump neighbour entries

fixed-header:
:   [ndmsg](#rt-neigh-definition-ndmsg)

attribute-set:
:   [neighbour-attrs](#rt-neigh-attribute-set-neighbour-attrs)

do:
:   **request**
    :   attributes:
        :   [`dst`]

    **reply**
    :   attributes:
        :   [`dst`, `lladdr`, `probes`, `vlan`, `port`, `vni`, `ifindex`, `master`, `protocol`, `nh-id`, `flags-ext`, `fdb-ext-attrs`]

dump:
:   **request**
    :   attributes:
        :   [`ifindex`, `master`]

    **reply**
    :   attributes:
        :   [`dst`, `lladdr`, `probes`, `vlan`, `port`, `vni`, `ifindex`, `master`, `protocol`, `nh-id`, `flags-ext`, `fdb-ext-attrs`]

### [newneigh-ntf](#id10)

Notify a neighbour creation

value:
:   28

notify:
:   getneigh

fixed-header:
:   [ndmsg](#rt-neigh-definition-ndmsg)

### [getneightbl](#id11)

Get or dump neighbour tables

fixed-header:
:   [ndtmsg](#rt-neigh-definition-ndtmsg)

attribute-set:
:   [ndt-attrs](#rt-neigh-attribute-set-ndt-attrs)

dump:
:   **request**

    **reply**
    :   attributes:
        :   [`name`, `thresh1`, `thresh2`, `thresh3`, `config`, `parms`, `stats`, `gc-interval`]

### [setneightbl](#id12)

Set neighbour tables

fixed-header:
:   [ndtmsg](#rt-neigh-definition-ndtmsg)

attribute-set:
:   [ndt-attrs](#rt-neigh-attribute-set-ndt-attrs)

do:
:   **request**
    :   attributes:
        :   [`name`, `thresh1`, `thresh2`, `thresh3`, `parms`, `gc-interval`]

## [Multicast groups](#id13)

* rtnlgrp-neigh

## [Definitions](#id14)

### [ndmsg](#id15)

type:
:   struct

members:
:   ndm-family (`u8`):


    ndm-pad (`pad`):


    ndm-ifindex (`s32`):


    ndm-state (`u16`):


    ndm-flags (`u8`):


    ndm-type (`u8`):

### [ndtmsg](#id16)

type:
:   struct

members:
:   family (`u8`):

### [nud-state](#id17)

type:
:   flags

enum-name:
:   None

entries:
:   * `incomplete`
    * `reachable`
    * `stale`
    * `delay`
    * `probe`
    * `failed`
    * `noarp`
    * `permanent`

### [ntf-flags](#id18)

type:
:   flags

enum-name:
:   None

entries:
:   * `use`
    * `self`
    * `master`
    * `proxy`
    * `ext-learned`
    * `offloaded`
    * `sticky`
    * `router`

### [ntf-ext-flags](#id19)

type:
:   flags

enum-name:
:   None

entries:
:   * `managed`
    * `locked`
    * `ext-validated`

### [rtm-type](#id20)

type:
:   enum

enum-name:
:   None

entries:
:   * `unspec`
    * `unicast`
    * `local`
    * `broadcast`
    * `anycast`
    * `multicast`
    * `blackhole`
    * `unreachable`
    * `prohibit`
    * `throw`
    * `nat`
    * `xresolve`

### [nda-cacheinfo](#id21)

type:
:   struct

members:
:   confirmed (`u32`):


    used (`u32`):


    updated (`u32`):


    refcnt (`u32`):

### [ndt-config](#id22)

type:
:   struct

members:
:   key-len (`u16`):


    entry-size (`u16`):


    entries (`u32`):


    last-flush (`u32`):


    last-rand (`u32`):


    hash-rnd (`u32`):


    hash-mask (`u32`):


    hash-chain-gc (`u32`):


    proxy-qlen (`u32`):

### [ndt-stats](#id23)

type:
:   struct

members:
:   allocs (`u64`):


    destroys (`u64`):


    hash-grows (`u64`):


    res-failed (`u64`):


    lookups (`u64`):


    hits (`u64`):


    rcv-probes-mcast (`u64`):


    rcv-probes-ucast (`u64`):


    periodic-gc-runs (`u64`):


    forced-gc-runs (`u64`):


    table-fulls (`u64`):

## [Attribute sets](#id24)

### [neighbour-attrs](#id25)

#### unspec (`binary`)

value:
:   0

#### dst (`binary`)

display-hint:
:   ipv4-or-v6

#### lladdr (`binary`)

display-hint:
:   mac

#### cacheinfo (`binary`)

struct:
:   [nda-cacheinfo](#rt-neigh-definition-nda-cacheinfo)

#### probes (`u32`)

#### vlan (`u16`)

#### port (`u16`)

#### vni (`u32`)

#### ifindex (`u32`)

#### master (`u32`)

#### link-netnsid (`s32`)

#### src-vni (`u32`)

#### protocol (`u8`)

#### nh-id (`u32`)

#### fdb-ext-attrs (`binary`)

#### flags-ext (`u32`)

enum:
:   [ntf-ext-flags](#rt-neigh-definition-ntf-ext-flags)

#### ndm-state-mask (`u16`)

#### ndm-flags-mask (`u8`)

### [ndt-attrs](#id26)

#### name (`string`)

#### thresh1 (`u32`)

#### thresh2 (`u32`)

#### thresh3 (`u32`)

#### config (`binary`)

struct:
:   [ndt-config](#rt-neigh-definition-ndt-config)

#### parms (`nest`)

nested-attributes:
:   [ndtpa-attrs](#rt-neigh-attribute-set-ndtpa-attrs)

#### stats (`binary`)

struct:
:   [ndt-stats](#rt-neigh-definition-ndt-stats)

#### gc-interval (`u64`)

#### pad (`pad`)

### [ndtpa-attrs](#id27)

#### ifindex (`u32`)

#### refcnt (`u32`)

#### reachable-time (`u64`)

#### base-reachable-time (`u64`)

#### retrans-time (`u64`)

#### gc-staletime (`u64`)

#### delay-probe-time (`u64`)

#### queue-len (`u32`)

#### app-probes (`u32`)

#### ucast-probes (`u32`)

#### mcast-probes (`u32`)

#### anycast-delay (`u64`)

#### proxy-delay (`u64`)

#### proxy-qlen (`u32`)

#### locktime (`u64`)

#### queue-lenbytes (`u32`)

#### mcast-reprobes (`u32`)

#### pad (`pad`)

#### interval-probe-time-ms (`u64`)
