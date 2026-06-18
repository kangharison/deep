# Familyrt-routenetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/rt-route.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `rt-route` netlink specification](#id1)

## [Summary](#id2)

Route configuration over rtnetlink.

## [Operations](#id3)

### [getroute](#id4)

Dump route information.

attribute-set:
:   [route-attrs](#rt-route-attribute-set-route-attrs)

do:
:   **request**
    :   attributes:
        :   [`src`, `dst`, `iif`, `oif`, `ip-proto`, `sport`, `dport`, `mark`, `uid`, `flowlabel`]

    **reply**
    :   attributes:
        :   [`dst`, `src`, `iif`, `oif`, `gateway`, `priority`, `prefsrc`, `metrics`, `multipath`, `flow`, `cacheinfo`, `table`, `mark`, `mfc-stats`, `via`, `newdst`, `pref`, `encap-type`, `encap`, `expires`, `pad`, `uid`, `ttl-propagate`, `ip-proto`, `sport`, `dport`, `nh-id`, `flowlabel`]

dump:
:   **request**
    :   attributes:
        :   []

    **reply**
    :   attributes:
        :   [`dst`, `src`, `iif`, `oif`, `gateway`, `priority`, `prefsrc`, `metrics`, `multipath`, `flow`, `cacheinfo`, `table`, `mark`, `mfc-stats`, `via`, `newdst`, `pref`, `encap-type`, `encap`, `expires`, `pad`, `uid`, `ttl-propagate`, `ip-proto`, `sport`, `dport`, `nh-id`, `flowlabel`]

### [newroute](#id5)

Create a new route

attribute-set:
:   [route-attrs](#rt-route-attribute-set-route-attrs)

do:
:   **request**
    :   attributes:
        :   [`dst`, `src`, `iif`, `oif`, `gateway`, `priority`, `prefsrc`, `metrics`, `multipath`, `flow`, `cacheinfo`, `table`, `mark`, `mfc-stats`, `via`, `newdst`, `pref`, `encap-type`, `encap`, `expires`, `pad`, `uid`, `ttl-propagate`, `ip-proto`, `sport`, `dport`, `nh-id`, `flowlabel`]

### [delroute](#id6)

Delete an existing route

attribute-set:
:   [route-attrs](#rt-route-attribute-set-route-attrs)

do:
:   **request**
    :   attributes:
        :   [`dst`, `src`, `iif`, `oif`, `gateway`, `priority`, `prefsrc`, `metrics`, `multipath`, `flow`, `cacheinfo`, `table`, `mark`, `mfc-stats`, `via`, `newdst`, `pref`, `encap-type`, `encap`, `expires`, `pad`, `uid`, `ttl-propagate`, `ip-proto`, `sport`, `dport`, `nh-id`, `flowlabel`]

## [Definitions](#id7)

### [rtm-type](#id8)

name-prefix:
:   rtn-

enum-name:
:   None

type:
:   enum

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

### [rtmsg](#id9)

type:
:   struct

members:
:   rtm-family (`u8`):


    rtm-dst-len (`u8`):


    rtm-src-len (`u8`):


    rtm-tos (`u8`):


    rtm-table (`u8`):


    rtm-protocol (`u8`):


    rtm-scope (`u8`):


    rtm-type (`u8`):


    rtm-flags (`u32`):

### [rta-cacheinfo](#id10)

type:
:   struct

members:
:   rta-clntref (`u32`):


    rta-lastuse (`u32`):


    rta-expires (`u32`):


    rta-error (`u32`):


    rta-used (`u32`):

## [Attribute sets](#id11)

### [route-attrs](#id12)

#### dst (`binary`)

display-hint:
:   ipv4-or-v6

#### src (`binary`)

display-hint:
:   ipv4-or-v6

#### iif (`u32`)

#### oif (`u32`)

#### gateway (`binary`)

display-hint:
:   ipv4-or-v6

#### priority (`u32`)

#### prefsrc (`binary`)

display-hint:
:   ipv4-or-v6

#### metrics (`nest`)

nested-attributes:
:   [metrics](#rt-route-attribute-set-metrics)

#### multipath (`binary`)

#### protoinfo (`binary`)

#### flow (`u32`)

#### cacheinfo (`binary`)

struct:
:   [rta-cacheinfo](#rt-route-definition-rta-cacheinfo)

#### session (`binary`)

#### mp-algo (`binary`)

#### table (`u32`)

#### mark (`u32`)

#### mfc-stats (`binary`)

#### via (`binary`)

#### newdst (`binary`)

#### pref (`u8`)

#### encap-type (`u16`)

#### encap (`binary`)

#### expires (`u32`)

#### pad (`binary`)

#### uid (`u32`)

#### ttl-propagate (`u8`)

#### ip-proto (`u8`)

#### sport (`u16`)

#### dport (`u16`)

#### nh-id (`u32`)

#### flowlabel (`u32`)

byte-order:
:   big-endian

display-hint:
:   hex

### [metrics](#id13)

#### unspec (`unused`)

value:
:   0

#### lock (`u32`)

#### mtu (`u32`)

#### window (`u32`)

#### rtt (`u32`)

#### rttvar (`u32`)

#### ssthresh (`u32`)

#### cwnd (`u32`)

#### advmss (`u32`)

#### reordering (`u32`)

#### hoplimit (`u32`)

#### initcwnd (`u32`)

#### features (`u32`)

#### rto-min (`u32`)

#### initrwnd (`u32`)

#### quickack (`u32`)

#### cc-algo (`string`)

#### fastopen-no-cookie (`u32`)
