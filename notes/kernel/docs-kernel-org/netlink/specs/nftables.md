# Familynftablesnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/nftables.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `nftables` netlink specification](#id106)

## [Summary](#id107)

Netfilter nftables configuration over netlink.

## [Operations](#id108)

### [batch-begin](#id109)

Start a batch of operations

attribute-set:
:   [batch-attrs](#nftables-attribute-set-batch-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`genid`]

    **reply**
    :   attributes:
        :   [`genid`]

### [batch-end](#id110)

Finish a batch of operations

attribute-set:
:   [batch-attrs](#nftables-attribute-set-batch-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`genid`]

### [newtable](#id111)

Create a new table.

attribute-set:
:   [table-attrs](#nftables-attribute-set-table-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`name`, `flags`, `userdata`]

### [gettable](#id112)

Get / dump tables.

attribute-set:
:   [table-attrs](#nftables-attribute-set-table-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`name`]

    **reply**
    :   attributes:
        :   [`name`, `use`, `handle`, `flags`, `owner`, `userdata`]

dump:
:   **reply**
    :   attributes:
        :   [`name`, `use`, `handle`, `flags`, `owner`, `userdata`]

### [deltable](#id113)

Delete an existing table.

attribute-set:
:   [table-attrs](#nftables-attribute-set-table-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`name`, `handle`]

### [destroytable](#id114)

Delete an existing table with destroy semantics (ignoring ENOENT
errors).

attribute-set:
:   [table-attrs](#nftables-attribute-set-table-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`name`, `handle`]

### [newchain](#id115)

Create a new chain.

attribute-set:
:   [chain-attrs](#nftables-attribute-set-chain-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `handle`, `policy`, `flags`, `hook`, `name`, `counters`, `userdata`, `type`]

### [getchain](#id116)

Get / dump chains.

attribute-set:
:   [chain-attrs](#nftables-attribute-set-chain-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`]

    **reply**
    :   attributes:
        :   [`table`, `name`, `handle`, `hook`, `policy`, `type`, `flags`, `counters`, `id`, `use`, `userdata`]

dump:
:   **reply**
    :   attributes:
        :   [`table`, `name`, `handle`, `hook`, `policy`, `type`, `flags`, `counters`, `id`, `use`, `userdata`]

### [delchain](#id117)

Delete an existing chain.

attribute-set:
:   [chain-attrs](#nftables-attribute-set-chain-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `handle`, `name`, `hook`]

### [destroychain](#id118)

Delete an existing chain with destroy semantics (ignoring ENOENT
errors).

attribute-set:
:   [chain-attrs](#nftables-attribute-set-chain-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `handle`, `name`, `hook`]

### [newrule](#id119)

Create a new rule.

attribute-set:
:   [rule-attrs](#nftables-attribute-set-rule-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `chain`, `chain-id`, `handle`, `position`, `position-id`, `expressions`, `userdata`, `compat`]

### [getrule](#id120)

Get / dump rules.

attribute-set:
:   [rule-attrs](#nftables-attribute-set-rule-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `chain`, `handle`]

    **reply**
    :   attributes:
        :   [`table`, `chain`, `handle`, `position`, `expressions`, `userdata`]

dump:
:   **request**
    :   attributes:
        :   [`table`, `chain`]

    **reply**
    :   attributes:
        :   [`table`, `chain`, `handle`, `position`, `expressions`, `userdata`]

### [getrule-reset](#id121)

Get / dump rules and reset stateful expressions.

attribute-set:
:   [rule-attrs](#nftables-attribute-set-rule-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `chain`, `handle`]

    **reply**
    :   attributes:
        :   [`table`, `chain`, `handle`, `position`, `expressions`, `userdata`]

dump:
:   **request**
    :   attributes:
        :   [`table`, `chain`, `handle`]

    **reply**
    :   attributes:
        :   [`table`, `chain`, `handle`, `position`, `expressions`, `userdata`]

### [delrule](#id122)

Delete an existing rule.

attribute-set:
:   [rule-attrs](#nftables-attribute-set-rule-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `chain`, `handle`, `id`]

### [destroyrule](#id123)

Delete an existing rule with destroy semantics (ignoring ENOENT errors).

attribute-set:
:   [rule-attrs](#nftables-attribute-set-rule-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `chain`, `handle`, `id`]

### [newset](#id124)

Create a new set.

attribute-set:
:   [set-attrs](#nftables-attribute-set-set-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`, `key-len`, `id`, `key-type`, `flags`, `data-type`, `data-len`, `obj-type`, `timeout`, `gc-interval`, `policy`, `desc`, `userdata`]

### [getset](#id125)

Get / dump sets.

attribute-set:
:   [set-attrs](#nftables-attribute-set-set-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`]

    **reply**
    :   attributes:
        :   [`table`, `name`, `handle`, `flags`, `key-len`, `key-type`, `data-type`, `data-len`, `obj-type`, `gc-interval`, `policy`, `userdata`, `desc`, `expr`, `expressions`]

dump:
:   **request**
    :   attributes:
        :   [`table`]

    **reply**
    :   attributes:
        :   [`table`, `name`, `handle`, `flags`, `key-len`, `key-type`, `data-type`, `data-len`, `obj-type`, `gc-interval`, `policy`, `userdata`, `desc`, `expr`, `expressions`]

### [delset](#id126)

Delete an existing set.

attribute-set:
:   [set-attrs](#nftables-attribute-set-set-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `handle`, `name`]

### [destroyset](#id127)

Delete an existing set with destroy semantics (ignoring ENOENT errors).

attribute-set:
:   [set-attrs](#nftables-attribute-set-set-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `handle`, `name`]

### [newsetelem](#id128)

Create a new set element.

attribute-set:
:   [setelem-list-attrs](#nftables-attribute-set-setelem-list-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `set`, `set-id`, `elements`]

### [getsetelem](#id129)

Get / dump set elements.

attribute-set:
:   [setelem-list-attrs](#nftables-attribute-set-setelem-list-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `set`, `elements`]

    **reply**
    :   attributes:
        :   [`elements`]

dump:
:   **request**
    :   attributes:
        :   [`table`, `set`]

    **reply**
    :   attributes:
        :   [`table`, `set`, `elements`]

### [getsetelem-reset](#id130)

Get / dump set elements and reset stateful expressions.

attribute-set:
:   [setelem-list-attrs](#nftables-attribute-set-setelem-list-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`elements`]

    **reply**
    :   attributes:
        :   [`table`, `set`, `elements`]

dump:
:   **request**
    :   attributes:
        :   [`table`, `set`]

    **reply**
    :   attributes:
        :   [`table`, `set`, `elements`]

### [delsetelem](#id131)

Delete an existing set element.

attribute-set:
:   [setelem-list-attrs](#nftables-attribute-set-setelem-list-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `set`, `elements`]

### [destroysetelem](#id132)

Delete an existing set element with destroy semantics.

attribute-set:
:   [setelem-list-attrs](#nftables-attribute-set-setelem-list-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `set`, `elements`]

### [getgen](#id133)

Get / dump rule-set generation.

attribute-set:
:   [gen-attrs](#nftables-attribute-set-gen-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**

    **reply**
    :   attributes:
        :   [`id`, `proc-pid`, `proc-name`]

dump:
:   **reply**
    :   attributes:
        :   [`id`, `proc-pid`, `proc-name`]

### [newobj](#id134)

Create a new stateful object.

attribute-set:
:   [obj-attrs](#nftables-attribute-set-obj-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`type`, `name`, `data`, `table`, `userdata`]

### [getobj](#id135)

Get / dump stateful objects.

attribute-set:
:   [obj-attrs](#nftables-attribute-set-obj-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`name`, `type`, `table`]

    **reply**
    :   attributes:
        :   [`table`, `name`, `type`, `handle`, `use`, `data`, `userdata`]

dump:
:   **request**
    :   attributes:
        :   [`table`, `type`]

    **reply**
    :   attributes:
        :   [`table`, `name`, `type`, `handle`, `use`, `data`, `userdata`]

### [delobj](#id136)

Delete an existing stateful object.

attribute-set:
:   [obj-attrs](#nftables-attribute-set-obj-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`, `type`, `handle`]

### [destroyobj](#id137)

Delete an existing stateful object with destroy semantics.

attribute-set:
:   [obj-attrs](#nftables-attribute-set-obj-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`, `type`, `handle`]

### [newflowtable](#id138)

Create a new flow table.

attribute-set:
:   [flowtable-attrs](#nftables-attribute-set-flowtable-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`, `hook`, `flags`]

### [getflowtable](#id139)

Get / dump flow tables.

attribute-set:
:   [flowtable-attrs](#nftables-attribute-set-flowtable-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`name`, `table`]

    **reply**
    :   attributes:
        :   [`table`, `name`, `handle`, `use`, `flags`, `hook`]

dump:
:   **reply**
    :   attributes:
        :   [`table`, `name`, `handle`, `use`, `flags`, `hook`]

### [delflowtable](#id140)

Delete an existing flow table.

attribute-set:
:   [flowtable-attrs](#nftables-attribute-set-flowtable-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`, `handle`, `hook`]

### [destroyflowtable](#id141)

Delete an existing flow table with destroy semantics.

attribute-set:
:   [flowtable-attrs](#nftables-attribute-set-flowtable-attrs)

fixed-header:
:   [nfgenmsg](#nftables-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`table`, `name`, `handle`, `hook`]

## [Multicast groups](#id142)

* mgmt

## [Definitions](#id143)

### [nfgenmsg](#id144)

type:
:   struct

members:
:   nfgen-family (`u8`):


    version (`u8`):


    res-id (`u16`):

### [meta-keys](#id145)

type:
:   enum

entries:
:   * `len`
    * `protocol`
    * `priority`
    * `mark`
    * `iif`
    * `oif`
    * `iifname`
    * `oifname`
    * `iftype`
    * `oiftype`
    * `skuid`
    * `skgid`
    * `nftrace`
    * `rtclassid`
    * `secmark`
    * `nfproto`
    * `l4-proto`
    * `bri-iifname`
    * `bri-oifname`
    * `pkttype`
    * `cpu`
    * `iifgroup`
    * `oifgroup`
    * `cgroup`
    * `prandom`
    * `secpath`
    * `iifkind`
    * `oifkind`
    * `bri-iifpvid`
    * `bri-iifvproto`
    * `time-ns`
    * `time-day`
    * `time-hour`
    * `sdif`
    * `sdifname`
    * `bri-broute`

### [bitwise-ops](#id146)

type:
:   enum

entries:
:   mask-xor:
    :   mask-and-xor operation used to implement NOT, AND, OR and XOR boolean operations

    lshift:


    rshift:


    and:


    or:


    xor:

### [cmp-ops](#id147)

type:
:   enum

entries:
:   * `eq`
    * `neq`
    * `lt`
    * `lte`
    * `gt`
    * `gte`

### [object-type](#id148)

type:
:   enum

entries:
:   * `unspec`
    * `counter`
    * `quota`
    * `ct-helper`
    * `limit`
    * `connlimit`
    * `tunnel`
    * `ct-timeout`
    * `secmark`
    * `ct-expect`
    * `synproxy`

### [nat-range-flags](#id149)

type:
:   flags

entries:
:   * `map-ips`
    * `proto-specified`
    * `proto-random`
    * `persistent`
    * `proto-random-fully`
    * `proto-offset`
    * `netmap`

### [table-flags](#id150)

type:
:   flags

entries:
:   * `dormant`
    * `owner`
    * `persist`

### [chain-flags](#id151)

type:
:   flags

entries:
:   * `base`
    * `hw-offload`
    * `binding`

### [set-flags](#id152)

type:
:   flags

entries:
:   * `anonymous`
    * `constant`
    * `interval`
    * `map`
    * `timeout`
    * `eval`
    * `object`
    * `concat`
    * `expr`

### [set-elem-flags](#id153)

type:
:   flags

entries:
:   * `interval-end`
    * `catchall`

### [lookup-flags](#id154)

type:
:   flags

entries:
:   * `invert`

### [ct-keys](#id155)

type:
:   enum

entries:
:   * `state`
    * `direction`
    * `status`
    * `mark`
    * `secmark`
    * `expiration`
    * `helper`
    * `l3protocol`
    * `src`
    * `dst`
    * `protocol`
    * `proto-src`
    * `proto-dst`
    * `labels`
    * `pkts`
    * `bytes`
    * `avgpkt`
    * `zone`
    * `eventmask`
    * `src-ip`
    * `dst-ip`
    * `src-ip6`
    * `dst-ip6`
    * `ct-id`

### [ct-direction](#id156)

type:
:   enum

entries:
:   * `original`
    * `reply`

### [quota-flags](#id157)

type:
:   flags

entries:
:   * `invert`
    * `depleted`

### [verdict-code](#id158)

type:
:   enum

entries:
:   continue:


    break:


    jump:


    goto:


    return:


    drop:


    accept:


    stolen:


    queue:


    repeat:

### [fib-result](#id159)

type:
:   enum

entries:
:   * `oif`
    * `oifname`
    * `addrtype`

### [fib-flags](#id160)

type:
:   flags

entries:
:   * `saddr`
    * `daddr`
    * `mark`
    * `iif`
    * `oif`
    * `present`

### [reject-types](#id161)

type:
:   enum

entries:
:   * `icmp-unreach`
    * `tcp-rst`
    * `icmpx-unreach`

### [reject-inet-code](#id162)

doc:
:   These codes are mapped to real ICMP and ICMPv6 codes.

type:
:   enum

entries:
:   * `icmpx-no-route`
    * `icmpx-port-unreach`
    * `icmpx-host-unreach`
    * `icmpx-admin-prohibited`

### [payload-base](#id163)

type:
:   enum

entries:
:   * `link-layer-header`
    * `network-header`
    * `transport-header`
    * `inner-header`
    * `tun-header`

### [range-ops](#id164)

doc:
:   Range operator

type:
:   enum

entries:
:   * `eq`
    * `neq`

### [registers](#id165)

doc:
:   nf\_tables registers. nf\_tables used to have five registers: a verdict register and four data registers of size 16. The data registers have been changed to 16 registers of size 4. For compatibility reasons, the NFT\_REG\_[1-4] registers still map to areas of size 16, the 4 byte registers are addressed using NFT\_REG32\_00 - NFT\_REG32\_15.

type:
:   enum

entries:
:   reg-verdict:


    reg-1:


    reg-2:


    reg-3:


    reg-4:


    reg32-00:


    reg32-01:


    reg32-02:


    reg32-03:


    reg32-04:


    reg32-05:


    reg32-06:


    reg32-07:


    reg32-08:


    reg32-09:


    reg32-10:


    reg32-11:


    reg32-12:


    reg32-13:


    reg32-14:


    reg32-15:

### [numgen-types](#id166)

type:
:   enum

entries:
:   * `incremental`
    * `random`

### [log-level](#id167)

doc:
:   nf\_tables log levels

type:
:   enum

entries:
:   emerg:
    :   system is unusable

    alert:
    :   action must be taken immediately

    crit:
    :   critical conditions

    err:
    :   error conditions

    warning:
    :   warning conditions

    notice:
    :   normal but significant condition

    info:
    :   informational

    debug:
    :   debug-level messages

    audit:
    :   enabling audit logging

### [log-flags](#id168)

doc:
:   nf\_tables log flags

header:
:   linux/netfilter/nf\_log.h

type:
:   flags

entries:
:   tcpseq:
    :   Log TCP sequence numbers

    tcpopt:
    :   Log TCP options

    ipopt:
    :   Log IP options

    uid:
    :   Log UID owning local socket

    nflog:
    :   Unsupported, don’t reuse

    macdecode:
    :   Decode MAC header

## [Attribute sets](#id169)

### [log-attrs](#id170)

log expression netlink attributes

#### group (`u16`)

doc:
:   netlink group to send messages to

byte-order:
:   big-endian

#### prefix (`string`)

doc:
:   prefix to prepend to log messages

#### snaplen (`u32`)

doc:
:   length of payload to include in netlink message

byte-order:
:   big-endian

#### qthreshold (`u16`)

doc:
:   queue threshold

byte-order:
:   big-endian

#### level (`u32`)

doc:
:   log level

enum:
:   [log-level](#nftables-definition-log-level)

byte-order:
:   big-endian

#### flags (`u32`)

doc:
:   logging flags

enum:
:   [log-flags](#nftables-definition-log-flags)

byte-order:
:   big-endian

### [numgen-attrs](#id171)

nf\_tables number generator expression netlink attributes

#### dreg (`u32`)

doc:
:   destination register

enum:
:   [registers](#nftables-definition-registers)

#### modulus (`u32`)

doc:
:   maximum counter value

byte-order:
:   big-endian

#### type (`u32`)

doc:
:   operation type

byte-order:
:   big-endian

enum:
:   [numgen-types](#nftables-definition-numgen-types)

#### offset (`u32`)

doc:
:   offset to be added to the counter

byte-order:
:   big-endian

### [range-attrs](#id172)

#### sreg (`u32`)

doc:
:   source register of data to compare

byte-order:
:   big-endian

enum:
:   [registers](#nftables-definition-registers)

#### op (`u32`)

doc:
:   cmp operation

byte-order:
:   big-endian

enum:
:   [range-ops](#nftables-definition-range-ops)

#### from-data (`nest`)

doc:
:   data range from

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

#### to-data (`nest`)

doc:
:   data range to

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

### [batch-attrs](#id173)

#### genid (`u32`)

doc:
:   generation ID for this changeset

byte-order:
:   big-endian

### [table-attrs](#id174)

#### name (`string`)

doc:
:   name of the table

#### flags (`u32`)

byte-order:
:   big-endian

doc:
:   bitmask of flags

enum:
:   [table-flags](#nftables-definition-table-flags)

enum-as-flags:
:   True

#### use (`u32`)

byte-order:
:   big-endian

doc:
:   number of chains in this table

#### handle (`u64`)

byte-order:
:   big-endian

doc:
:   numeric handle of the table

#### pad (`pad`)

#### userdata (`binary`)

doc:
:   user data

#### owner (`u32`)

byte-order:
:   big-endian

doc:
:   owner of this table through netlink portID

### [chain-attrs](#id175)

#### table (`string`)

doc:
:   name of the table containing the chain

#### handle (`u64`)

byte-order:
:   big-endian

doc:
:   numeric handle of the chain

#### name (`string`)

doc:
:   name of the chain

#### hook (`nest`)

nested-attributes:
:   [nft-hook-attrs](#nftables-attribute-set-nft-hook-attrs)

doc:
:   hook specification for basechains

#### policy (`u32`)

byte-order:
:   big-endian

doc:
:   numeric policy of the chain

#### use (`u32`)

byte-order:
:   big-endian

doc:
:   number of references to this chain

#### type (`string`)

doc:
:   type name of the chain

#### counters (`nest`)

nested-attributes:
:   [nft-counter-attrs](#nftables-attribute-set-nft-counter-attrs)

doc:
:   counter specification of the chain

#### flags (`u32`)

byte-order:
:   big-endian

doc:
:   chain flags

enum:
:   [chain-flags](#nftables-definition-chain-flags)

enum-as-flags:
:   True

#### id (`u32`)

byte-order:
:   big-endian

doc:
:   uniquely identifies a chain in a transaction

#### userdata (`binary`)

doc:
:   user data

### [counter-attrs](#id176)

#### bytes (`u64`)

byte-order:
:   big-endian

#### packets (`u64`)

byte-order:
:   big-endian

#### pad (`pad`)

### [nft-hook-attrs](#id177)

#### num (`u32`)

byte-order:
:   big-endian

#### priority (`s32`)

byte-order:
:   big-endian

#### dev (`string`)

doc:
:   net device name

#### devs (`nest`)

nested-attributes:
:   [hook-dev-attrs](#nftables-attribute-set-hook-dev-attrs)

doc:
:   list of net devices

### [hook-dev-attrs](#id178)

#### name (`string`)

multi-attr:
:   True

### [nft-counter-attrs](#id179)

#### bytes (`u64`)

byte-order:
:   big-endian

#### packets (`u64`)

byte-order:
:   big-endian

### [rule-attrs](#id180)

#### table (`string`)

doc:
:   name of the table containing the rule

#### chain (`string`)

doc:
:   name of the chain containing the rule

#### handle (`u64`)

byte-order:
:   big-endian

doc:
:   numeric handle of the rule

#### expressions (`nest`)

nested-attributes:
:   [expr-list-attrs](#nftables-attribute-set-expr-list-attrs)

doc:
:   list of expressions

#### compat (`nest`)

nested-attributes:
:   [rule-compat-attrs](#nftables-attribute-set-rule-compat-attrs)

doc:
:   compatibility specifications of the rule

#### position (`u64`)

byte-order:
:   big-endian

doc:
:   numeric handle of the previous rule

#### userdata (`binary`)

doc:
:   user data

#### id (`u32`)

doc:
:   uniquely identifies a rule in a transaction

#### position-id (`u32`)

doc:
:   transaction unique identifier of the previous rule

#### chain-id (`u32`)

doc:
:   add the rule to chain by ID, alternative to chain name

### [expr-list-attrs](#id181)

#### elem (`nest`)

nested-attributes:
:   [expr-attrs](#nftables-attribute-set-expr-attrs)

multi-attr:
:   True

### [expr-attrs](#id182)

#### name (`string`)

doc:
:   name of the expression type

#### data (`sub-message`)

sub-message:
:   [expr-ops](#nftables-sub-message-expr-ops)

selector:
:   name

doc:
:   type specific data

### [rule-compat-attrs](#id183)

#### proto (`u32`)

byte-order:
:   big-endian

doc:
:   numeric value of the handled protocol

#### flags (`u32`)

byte-order:
:   big-endian

doc:
:   bitmask of flags

### [set-attrs](#id184)

#### table (`string`)

doc:
:   table name

#### name (`string`)

doc:
:   set name

#### flags (`u32`)

enum:
:   [set-flags](#nftables-definition-set-flags)

byte-order:
:   big-endian

doc:
:   bitmask of `enum nft_set_flags`

#### key-type (`u32`)

byte-order:
:   big-endian

doc:
:   key data type, informational purpose only

#### key-len (`u32`)

byte-order:
:   big-endian

doc:
:   key data length

#### data-type (`u32`)

byte-order:
:   big-endian

doc:
:   mapping data type

#### data-len (`u32`)

byte-order:
:   big-endian

doc:
:   mapping data length

#### policy (`u32`)

byte-order:
:   big-endian

doc:
:   selection policy

#### desc (`nest`)

nested-attributes:
:   [set-desc-attrs](#nftables-attribute-set-set-desc-attrs)

doc:
:   set description

#### id (`u32`)

doc:
:   uniquely identifies a set in a transaction

#### timeout (`u64`)

doc:
:   default timeout value

#### gc-interval (`u32`)

doc:
:   garbage collection interval

#### userdata (`binary`)

doc:
:   user data

#### pad (`pad`)

#### obj-type (`u32`)

byte-order:
:   big-endian

doc:
:   stateful object type

#### handle (`u64`)

byte-order:
:   big-endian

doc:
:   set handle

#### expr (`nest`)

nested-attributes:
:   [expr-attrs](#nftables-attribute-set-expr-attrs)

doc:
:   set expression

multi-attr:
:   True

#### expressions (`nest`)

nested-attributes:
:   [set-list-attrs](#nftables-attribute-set-set-list-attrs)

doc:
:   list of expressions

#### type (`string`)

doc:
:   set backend type

#### count (`u32`)

byte-order:
:   big-endian

doc:
:   number of set elements

### [set-desc-attrs](#id185)

#### size (`u32`)

byte-order:
:   big-endian

doc:
:   number of elements in set

#### concat (`nest`)

nested-attributes:
:   [set-desc-concat-attrs](#nftables-attribute-set-set-desc-concat-attrs)

doc:
:   description of field concatenation

multi-attr:
:   True

### [set-desc-concat-attrs](#id186)

#### elem (`nest`)

nested-attributes:
:   [set-field-attrs](#nftables-attribute-set-set-field-attrs)

### [set-field-attrs](#id187)

#### len (`u32`)

byte-order:
:   big-endian

### [set-list-attrs](#id188)

#### elem (`nest`)

nested-attributes:
:   [expr-attrs](#nftables-attribute-set-expr-attrs)

multi-attr:
:   True

### [setelem-attrs](#id189)

#### key (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

doc:
:   key value

#### data (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

doc:
:   data value of mapping

#### flags (`binary`)

doc:
:   bitmask of nft\_set\_elem\_flags

#### timeout (`u64`)

doc:
:   timeout value

#### expiration (`u64`)

doc:
:   expiration time

#### userdata (`binary`)

doc:
:   user data

#### expr (`nest`)

nested-attributes:
:   [expr-attrs](#nftables-attribute-set-expr-attrs)

doc:
:   expression

#### objref (`string`)

doc:
:   stateful object reference

#### key-end (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

doc:
:   closing key value

#### expressions (`nest`)

nested-attributes:
:   [expr-list-attrs](#nftables-attribute-set-expr-list-attrs)

doc:
:   list of expressions

### [setelem-list-elem-attrs](#id190)

#### elem (`nest`)

nested-attributes:
:   [setelem-attrs](#nftables-attribute-set-setelem-attrs)

multi-attr:
:   True

### [setelem-list-attrs](#id191)

#### table (`string`)

#### set (`string`)

#### elements (`nest`)

nested-attributes:
:   [setelem-list-elem-attrs](#nftables-attribute-set-setelem-list-elem-attrs)

#### set-id (`u32`)

### [gen-attrs](#id192)

#### id (`u32`)

byte-order:
:   big-endian

doc:
:   ruleset generation id

#### proc-pid (`u32`)

byte-order:
:   big-endian

#### proc-name (`string`)

### [obj-attrs](#id193)

#### table (`string`)

doc:
:   name of the table containing the expression

#### name (`string`)

doc:
:   name of this expression type

#### type (`u32`)

enum:
:   [object-type](#nftables-definition-object-type)

byte-order:
:   big-endian

doc:
:   stateful object type

#### data (`sub-message`)

sub-message:
:   [obj-data](#nftables-sub-message-obj-data)

selector:
:   type

doc:
:   stateful object data

#### use (`u32`)

byte-order:
:   big-endian

doc:
:   number of references to this expression

#### handle (`u64`)

byte-order:
:   big-endian

doc:
:   object handle

#### pad (`pad`)

#### userdata (`binary`)

doc:
:   user data

### [quota-attrs](#id194)

#### bytes (`u64`)

byte-order:
:   big-endian

#### flags (`u32`)

byte-order:
:   big-endian

enum:
:   [quota-flags](#nftables-definition-quota-flags)

#### pad (`pad`)

#### consumed (`u64`)

byte-order:
:   big-endian

### [flowtable-attrs](#id195)

#### table (`string`)

#### name (`string`)

#### hook (`nest`)

nested-attributes:
:   [flowtable-hook-attrs](#nftables-attribute-set-flowtable-hook-attrs)

#### use (`u32`)

byte-order:
:   big-endian

#### handle (`u64`)

byte-order:
:   big-endian

#### pad (`pad`)

#### flags (`u32`)

byte-order:
:   big-endian

### [flowtable-hook-attrs](#id196)

#### num (`u32`)

byte-order:
:   big-endian

#### priority (`u32`)

byte-order:
:   big-endian

#### devs (`nest`)

nested-attributes:
:   [hook-dev-attrs](#nftables-attribute-set-hook-dev-attrs)

### [expr-bitwise-attrs](#id197)

The bitwise expression supports boolean and shift operations. It
implements the boolean operations by performing the following
operation:

```
dreg = (sreg & mask) ^ xor

with these mask and xor values:

op      mask    xor
----    ----    ---
NOT:     1       1
OR:     ~x       x
XOR:     1       x
AND:     x       0
```

#### sreg (`u32`)

byte-order:
:   big-endian

#### dreg (`u32`)

byte-order:
:   big-endian

#### len (`u32`)

byte-order:
:   big-endian

#### mask (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

#### xor (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

#### op (`u32`)

byte-order:
:   big-endian

enum:
:   [bitwise-ops](#nftables-definition-bitwise-ops)

#### data (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

### [expr-cmp-attrs](#id198)

#### sreg (`u32`)

byte-order:
:   big-endian

#### op (`u32`)

byte-order:
:   big-endian

enum:
:   [cmp-ops](#nftables-definition-cmp-ops)

#### data (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

### [data-attrs](#id199)

#### value (`binary`)

#### verdict (`nest`)

nested-attributes:
:   [verdict-attrs](#nftables-attribute-set-verdict-attrs)

### [verdict-attrs](#id200)

#### code (`u32`)

doc:
:   nf\_tables verdict

byte-order:
:   big-endian

enum:
:   [verdict-code](#nftables-definition-verdict-code)

#### chain (`string`)

doc:
:   jump target chain name

#### chain-id (`u32`)

doc:
:   jump target chain ID

byte-order:
:   big-endian

### [expr-counter-attrs](#id201)

#### bytes (`u64`)

byte-order:
:   big-endian

doc:
:   Number of bytes

#### packets (`u64`)

byte-order:
:   big-endian

doc:
:   Number of packets

#### pad (`pad`)

### [expr-fib-attrs](#id202)

#### dreg (`u32`)

byte-order:
:   big-endian

#### result (`u32`)

byte-order:
:   big-endian

enum:
:   [fib-result](#nftables-definition-fib-result)

#### flags (`u32`)

byte-order:
:   big-endian

enum:
:   [fib-flags](#nftables-definition-fib-flags)

### [expr-ct-attrs](#id203)

#### dreg (`u32`)

byte-order:
:   big-endian

#### key (`u32`)

byte-order:
:   big-endian

enum:
:   [ct-keys](#nftables-definition-ct-keys)

#### direction (`u8`)

enum:
:   [ct-direction](#nftables-definition-ct-direction)

#### sreg (`u32`)

byte-order:
:   big-endian

### [expr-flow-offload-attrs](#id204)

#### name (`string`)

doc:
:   Flow offload table name

### [expr-immediate-attrs](#id205)

#### dreg (`u32`)

byte-order:
:   big-endian

#### data (`nest`)

nested-attributes:
:   [data-attrs](#nftables-attribute-set-data-attrs)

### [expr-lookup-attrs](#id206)

#### set (`string`)

doc:
:   Name of set to use

#### set-id (`u32`)

byte-order:
:   big-endian

doc:
:   ID of set to use

#### sreg (`u32`)

byte-order:
:   big-endian

#### dreg (`u32`)

byte-order:
:   big-endian

#### flags (`u32`)

byte-order:
:   big-endian

enum:
:   [lookup-flags](#nftables-definition-lookup-flags)

### [expr-masq-attrs](#id207)

#### flags (`u32`)

byte-order:
:   big-endian

enum:
:   [nat-range-flags](#nftables-definition-nat-range-flags)

enum-as-flags:
:   True

#### reg-proto-min (`u32`)

byte-order:
:   big-endian

enum:
:   [registers](#nftables-definition-registers)

#### reg-proto-max (`u32`)

byte-order:
:   big-endian

enum:
:   [registers](#nftables-definition-registers)

### [expr-meta-attrs](#id208)

#### dreg (`u32`)

byte-order:
:   big-endian

#### key (`u32`)

byte-order:
:   big-endian

enum:
:   [meta-keys](#nftables-definition-meta-keys)

#### sreg (`u32`)

byte-order:
:   big-endian

### [expr-nat-attrs](#id209)

#### type (`u32`)

byte-order:
:   big-endian

#### family (`u32`)

byte-order:
:   big-endian

#### reg-addr-min (`u32`)

byte-order:
:   big-endian

#### reg-addr-max (`u32`)

byte-order:
:   big-endian

#### reg-proto-min (`u32`)

byte-order:
:   big-endian

#### reg-proto-max (`u32`)

byte-order:
:   big-endian

#### flags (`u32`)

byte-order:
:   big-endian

enum:
:   [nat-range-flags](#nftables-definition-nat-range-flags)

enum-as-flags:
:   True

### [expr-payload-attrs](#id210)

nf\_tables payload expression netlink attributes

#### dreg (`u32`)

doc:
:   destination register to load data into

byte-order:
:   big-endian

enum:
:   [registers](#nftables-definition-registers)

#### base (`u32`)

doc:
:   payload base

enum:
:   [payload-base](#nftables-definition-payload-base)

byte-order:
:   big-endian

#### offset (`u32`)

doc:
:   payload offset relative to base

byte-order:
:   big-endian

#### len (`u32`)

doc:
:   payload length

byte-order:
:   big-endian

#### sreg (`u32`)

doc:
:   source register to load data from

byte-order:
:   big-endian

enum:
:   [registers](#nftables-definition-registers)

#### csum-type (`u32`)

doc:
:   checksum type

byte-order:
:   big-endian

#### csum-offset (`u32`)

doc:
:   checksum offset relative to base

byte-order:
:   big-endian

#### csum-flags (`u32`)

doc:
:   checksum flags

byte-order:
:   big-endian

### [expr-reject-attrs](#id211)

#### type (`u32`)

byte-order:
:   big-endian

enum:
:   [reject-types](#nftables-definition-reject-types)

#### icmp-code (`u8`)

### [expr-target-attrs](#id212)

#### name (`string`)

#### rev (`u32`)

byte-order:
:   big-endian

#### info (`binary`)

### [expr-tproxy-attrs](#id213)

#### family (`u32`)

byte-order:
:   big-endian

#### reg-addr (`u32`)

byte-order:
:   big-endian

#### reg-port (`u32`)

byte-order:
:   big-endian

### [expr-objref-attrs](#id214)

#### imm-type (`u32`)

byte-order:
:   big-endian

#### imm-name (`string`)

doc:
:   object name

#### set-sreg (`u32`)

byte-order:
:   big-endian

#### set-name (`string`)

doc:
:   name of object map

#### set-id (`u32`)

byte-order:
:   big-endian

doc:
:   id of object map

### [compat-target-attrs](#id215)

#### name (`string`)

#### rev (`u32`)

byte-order:
:   big-endian

#### info (`binary`)

### [compat-match-attrs](#id216)

#### name (`string`)

#### rev (`u32`)

byte-order:
:   big-endian

#### info (`binary`)

### [compat-attrs](#id217)

#### name (`string`)

#### rev (`u32`)

byte-order:
:   big-endian

#### type (`u32`)

byte-order:
:   big-endian

## [Sub-messages](#id218)

### [expr-ops](#id219)

* **bitwise**
  :   attribute-set:
      :   [expr-bitwise-attrs](#nftables-attribute-set-expr-bitwise-attrs)
* **cmp**
  :   attribute-set:
      :   [expr-cmp-attrs](#nftables-attribute-set-expr-cmp-attrs)
* **counter**
  :   attribute-set:
      :   [expr-counter-attrs](#nftables-attribute-set-expr-counter-attrs)
* **ct**
  :   attribute-set:
      :   [expr-ct-attrs](#nftables-attribute-set-expr-ct-attrs)
* **fib**
  :   attribute-set:
      :   [expr-fib-attrs](#nftables-attribute-set-expr-fib-attrs)
* **flow\_offload**
  :   attribute-set:
      :   [expr-flow-offload-attrs](#nftables-attribute-set-expr-flow-offload-attrs)
* **immediate**
  :   attribute-set:
      :   [expr-immediate-attrs](#nftables-attribute-set-expr-immediate-attrs)
* **log**
  :   attribute-set:
      :   [log-attrs](#nftables-attribute-set-log-attrs)
* **lookup**
  :   attribute-set:
      :   [expr-lookup-attrs](#nftables-attribute-set-expr-lookup-attrs)
* **match**
  :   attribute-set:
      :   [compat-match-attrs](#nftables-attribute-set-compat-match-attrs)
* **meta**
  :   attribute-set:
      :   [expr-meta-attrs](#nftables-attribute-set-expr-meta-attrs)
* **nat**
  :   attribute-set:
      :   [expr-nat-attrs](#nftables-attribute-set-expr-nat-attrs)
* **numgen**
  :   attribute-set:
      :   [numgen-attrs](#nftables-attribute-set-numgen-attrs)
* **objref**
  :   attribute-set:
      :   [expr-objref-attrs](#nftables-attribute-set-expr-objref-attrs)
* **payload**
  :   attribute-set:
      :   [expr-payload-attrs](#nftables-attribute-set-expr-payload-attrs)
* **quota**
  :   attribute-set:
      :   [quota-attrs](#nftables-attribute-set-quota-attrs)
* **range**
  :   attribute-set:
      :   [range-attrs](#nftables-attribute-set-range-attrs)
* **reject**
  :   attribute-set:
      :   [expr-reject-attrs](#nftables-attribute-set-expr-reject-attrs)
* **target**
  :   attribute-set:
      :   [expr-target-attrs](#nftables-attribute-set-expr-target-attrs)
* **tproxy**
  :   attribute-set:
      :   [expr-tproxy-attrs](#nftables-attribute-set-expr-tproxy-attrs)

### [obj-data](#id220)

* **counter**
  :   attribute-set:
      :   [counter-attrs](#nftables-attribute-set-counter-attrs)
* **quota**
  :   attribute-set:
      :   [quota-attrs](#nftables-attribute-set-quota-attrs)
