# Familyconntracknetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/conntrack.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `conntrack` netlink specification](#id1)

## [Summary](#id2)

Netfilter connection tracking subsystem over nfnetlink

## [Operations](#id3)

### [get](#id4)

get / dump entries

attribute-set:
:   [conntrack-attrs](#conntrack-attribute-set-conntrack-attrs)

fixed-header:
:   [nfgenmsg](#conntrack-definition-nfgenmsg)

do:
:   **request**
    :   attributes:
        :   [`tuple-orig`, `tuple-reply`, `zone`]

    **reply**
    :   attributes:
        :   [`tuple-orig`, `tuple-reply`, `status`, `protoinfo`, `help`, `nat-src`, `nat-dst`, `timeout`, `mark`, `counters-orig`, `counters-reply`, `use`, `id`, `nat-dst`, `tuple-master`, `seq-adj-orig`, `seq-adj-reply`, `zone`, `secctx`, `labels`, `synproxy`]

dump:
:   **request**
    :   attributes:
        :   [`mark`, `filter`, `status`, `zone`]

    **reply**
    :   attributes:
        :   [`tuple-orig`, `tuple-reply`, `status`, `protoinfo`, `help`, `nat-src`, `nat-dst`, `timeout`, `mark`, `counters-orig`, `counters-reply`, `use`, `id`, `nat-dst`, `tuple-master`, `seq-adj-orig`, `seq-adj-reply`, `zone`, `secctx`, `labels`, `synproxy`]

### [get-stats](#id5)

dump pcpu conntrack stats

attribute-set:
:   [conntrack-stats-attrs](#conntrack-attribute-set-conntrack-stats-attrs)

fixed-header:
:   [nfgenmsg](#conntrack-definition-nfgenmsg)

dump:
:   **request**

    **reply**
    :   attributes:
        :   [`searched`, `found`, `insert`, `insert-failed`, `drop`, `early-drop`, `error`, `search-restart`, `clash-resolve`, `chain-toolong`]

## [Definitions](#id6)

### [nfgenmsg](#id7)

type:
:   struct

members:
:   nfgen-family (`u8`):


    version (`u8`):


    res-id (`u16`):

### [nf-ct-tcp-flags-mask](#id8)

type:
:   struct

members:
:   flags (`u8`):


    mask (`u8`):

### [nf-ct-tcp-flags](#id9)

type:
:   flags

entries:
:   * `window-scale`
    * `sack-perm`
    * `close-init`
    * `be-liberal`
    * `unacked`
    * `maxack`
    * `challenge-ack`
    * `simultaneous-open`

### [nf-ct-tcp-state](#id10)

type:
:   enum

entries:
:   * `none`
    * `syn-sent`
    * `syn-recv`
    * `established`
    * `fin-wait`
    * `close-wait`
    * `last-ack`
    * `time-wait`
    * `close`
    * `syn-sent2`
    * `max`
    * `ignore`
    * `retrans`
    * `unack`
    * `timeout-max`

### [nf-ct-sctp-state](#id11)

type:
:   enum

entries:
:   * `none`
    * `cloned`
    * `cookie-wait`
    * `cookie-echoed`
    * `established`
    * `shutdown-sent`
    * `shutdown-received`
    * `shutdown-ack-sent`
    * `shutdown-heartbeat-sent`

### [nf-ct-status](#id12)

type:
:   flags

entries:
:   * `expected`
    * `seen-reply`
    * `assured`
    * `confirmed`
    * `src-nat`
    * `dst-nat`
    * `seq-adj`
    * `src-nat-done`
    * `dst-nat-done`
    * `dying`
    * `fixed-timeout`
    * `template`
    * `nat-clash`
    * `helper`
    * `offload`
    * `hw-offload`

## [Attribute sets](#id13)

### [counter-attrs](#id14)

#### packets (`u64`)

byte-order:
:   big-endian

#### bytes (`u64`)

byte-order:
:   big-endian

#### packets-old (`u32`)

#### bytes-old (`u32`)

#### pad (`pad`)

### [tuple-proto-attrs](#id15)

#### proto-num (`u8`)

doc:
:   l4 protocol number

#### proto-src-port (`u16`)

byte-order:
:   big-endian

doc:
:   l4 source port

#### proto-dst-port (`u16`)

byte-order:
:   big-endian

doc:
:   l4 source port

#### proto-icmp-id (`u16`)

byte-order:
:   big-endian

doc:
:   l4 icmp id

#### proto-icmp-type (`u8`)

#### proto-icmp-code (`u8`)

#### proto-icmpv6-id (`u16`)

byte-order:
:   big-endian

doc:
:   l4 icmp id

#### proto-icmpv6-type (`u8`)

#### proto-icmpv6-code (`u8`)

### [tuple-ip-attrs](#id16)

#### ip-v4-src (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

doc:
:   ipv4 source address

#### ip-v4-dst (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

doc:
:   ipv4 destination address

#### ip-v6-src (`binary`)

byte-order:
:   big-endian

display-hint:
:   ipv6

doc:
:   ipv6 source address

#### ip-v6-dst (`binary`)

byte-order:
:   big-endian

display-hint:
:   ipv6

doc:
:   ipv6 destination address

### [tuple-attrs](#id17)

#### tuple-ip (`nest`)

nested-attributes:
:   [tuple-ip-attrs](#conntrack-attribute-set-tuple-ip-attrs)

doc:
:   conntrack l3 information

#### tuple-proto (`nest`)

nested-attributes:
:   [tuple-proto-attrs](#conntrack-attribute-set-tuple-proto-attrs)

doc:
:   conntrack l4 information

#### tuple-zone (`u16`)

byte-order:
:   big-endian

doc:
:   conntrack zone id

### [protoinfo-tcp-attrs](#id18)

#### tcp-state (`u8`)

enum:
:   [nf-ct-tcp-state](#conntrack-definition-nf-ct-tcp-state)

doc:
:   tcp connection state

#### tcp-wscale-original (`u8`)

doc:
:   window scaling factor in original direction

#### tcp-wscale-reply (`u8`)

doc:
:   window scaling factor in reply direction

#### tcp-flags-original (`binary`)

struct:
:   [nf-ct-tcp-flags-mask](#conntrack-definition-nf-ct-tcp-flags-mask)

#### tcp-flags-reply (`binary`)

struct:
:   [nf-ct-tcp-flags-mask](#conntrack-definition-nf-ct-tcp-flags-mask)

### [protoinfo-dccp-attrs](#id19)

#### dccp-state (`u8`)

doc:
:   dccp connection state

#### dccp-role (`u8`)

#### dccp-handshake-seq (`u64`)

byte-order:
:   big-endian

#### dccp-pad (`pad`)

### [protoinfo-sctp-attrs](#id20)

#### sctp-state (`u8`)

doc:
:   sctp connection state

enum:
:   [nf-ct-sctp-state](#conntrack-definition-nf-ct-sctp-state)

#### vtag-original (`u32`)

byte-order:
:   big-endian

#### vtag-reply (`u32`)

byte-order:
:   big-endian

### [protoinfo-attrs](#id21)

#### protoinfo-tcp (`nest`)

nested-attributes:
:   [protoinfo-tcp-attrs](#conntrack-attribute-set-protoinfo-tcp-attrs)

doc:
:   conntrack tcp state information

#### protoinfo-dccp (`nest`)

nested-attributes:
:   [protoinfo-dccp-attrs](#conntrack-attribute-set-protoinfo-dccp-attrs)

doc:
:   conntrack dccp state information

#### protoinfo-sctp (`nest`)

nested-attributes:
:   [protoinfo-sctp-attrs](#conntrack-attribute-set-protoinfo-sctp-attrs)

doc:
:   conntrack sctp state information

### [help-attrs](#id22)

#### help-name (`string`)

doc:
:   helper name

### [nat-proto-attrs](#id23)

#### nat-port-min (`u16`)

byte-order:
:   big-endian

#### nat-port-max (`u16`)

byte-order:
:   big-endian

### [nat-attrs](#id24)

#### nat-v4-minip (`u32`)

byte-order:
:   big-endian

#### nat-v4-maxip (`u32`)

byte-order:
:   big-endian

#### nat-v6-minip (`binary`)

#### nat-v6-maxip (`binary`)

#### nat-proto (`nest`)

nested-attributes:
:   [nat-proto-attrs](#conntrack-attribute-set-nat-proto-attrs)

### [seqadj-attrs](#id25)

#### correction-pos (`u32`)

byte-order:
:   big-endian

#### offset-before (`u32`)

byte-order:
:   big-endian

#### offset-after (`u32`)

byte-order:
:   big-endian

### [secctx-attrs](#id26)

#### secctx-name (`string`)

### [synproxy-attrs](#id27)

#### isn (`u32`)

byte-order:
:   big-endian

#### its (`u32`)

byte-order:
:   big-endian

#### tsoff (`u32`)

byte-order:
:   big-endian

### [conntrack-attrs](#id28)

#### tuple-orig (`nest`)

nested-attributes:
:   [tuple-attrs](#conntrack-attribute-set-tuple-attrs)

doc:
:   conntrack l3+l4 protocol information, original direction

#### tuple-reply (`nest`)

nested-attributes:
:   [tuple-attrs](#conntrack-attribute-set-tuple-attrs)

doc:
:   conntrack l3+l4 protocol information, reply direction

#### status (`u32`)

byte-order:
:   big-endian

enum:
:   [nf-ct-status](#conntrack-definition-nf-ct-status)

enum-as-flags:
:   True

doc:
:   conntrack flag bits

#### protoinfo (`nest`)

nested-attributes:
:   [protoinfo-attrs](#conntrack-attribute-set-protoinfo-attrs)

#### help (`nest`)

nested-attributes:
:   [help-attrs](#conntrack-attribute-set-help-attrs)

#### nat-src (`nest`)

nested-attributes:
:   [nat-attrs](#conntrack-attribute-set-nat-attrs)

#### timeout (`u32`)

byte-order:
:   big-endian

#### mark (`u32`)

byte-order:
:   big-endian

#### counters-orig (`nest`)

nested-attributes:
:   [counter-attrs](#conntrack-attribute-set-counter-attrs)

#### counters-reply (`nest`)

nested-attributes:
:   [counter-attrs](#conntrack-attribute-set-counter-attrs)

#### use (`u32`)

byte-order:
:   big-endian

#### id (`u32`)

byte-order:
:   big-endian

#### nat-dst (`nest`)

nested-attributes:
:   [nat-attrs](#conntrack-attribute-set-nat-attrs)

#### tuple-master (`nest`)

nested-attributes:
:   [tuple-attrs](#conntrack-attribute-set-tuple-attrs)

#### seq-adj-orig (`nest`)

nested-attributes:
:   [seqadj-attrs](#conntrack-attribute-set-seqadj-attrs)

#### seq-adj-reply (`nest`)

nested-attributes:
:   [seqadj-attrs](#conntrack-attribute-set-seqadj-attrs)

#### secmark (`binary`)

doc:
:   obsolete

#### zone (`u16`)

byte-order:
:   big-endian

doc:
:   conntrack zone id

#### secctx (`nest`)

nested-attributes:
:   [secctx-attrs](#conntrack-attribute-set-secctx-attrs)

#### timestamp (`u64`)

byte-order:
:   big-endian

#### mark-mask (`u32`)

byte-order:
:   big-endian

#### labels (`binary`)

#### labels-mask (`binary`)

#### synproxy (`nest`)

nested-attributes:
:   [synproxy-attrs](#conntrack-attribute-set-synproxy-attrs)

#### filter (`nest`)

nested-attributes:
:   [tuple-attrs](#conntrack-attribute-set-tuple-attrs)

#### status-mask (`u32`)

byte-order:
:   big-endian

enum:
:   [nf-ct-status](#conntrack-definition-nf-ct-status)

enum-as-flags:
:   True

doc:
:   conntrack flag bits to change

#### timestamp-event (`u64`)

byte-order:
:   big-endian

### [conntrack-stats-attrs](#id29)

#### searched (`u32`)

byte-order:
:   big-endian

doc:
:   obsolete

#### found (`u32`)

byte-order:
:   big-endian

#### new (`u32`)

byte-order:
:   big-endian

doc:
:   obsolete

#### invalid (`u32`)

byte-order:
:   big-endian

doc:
:   obsolete

#### ignore (`u32`)

byte-order:
:   big-endian

doc:
:   obsolete

#### delete (`u32`)

byte-order:
:   big-endian

doc:
:   obsolete

#### delete-list (`u32`)

byte-order:
:   big-endian

doc:
:   obsolete

#### insert (`u32`)

byte-order:
:   big-endian

#### insert-failed (`u32`)

byte-order:
:   big-endian

#### drop (`u32`)

byte-order:
:   big-endian

#### early-drop (`u32`)

byte-order:
:   big-endian

#### error (`u32`)

byte-order:
:   big-endian

#### search-restart (`u32`)

byte-order:
:   big-endian

#### clash-resolve (`u32`)

byte-order:
:   big-endian

#### chain-toolong (`u32`)

byte-order:
:   big-endian
