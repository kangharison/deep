# Familytcp_metricsnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/tcp_metrics.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `tcp_metrics` netlink specification](#id1)

## [Summary](#id2)

Management interface for TCP metrics.

## [Operations](#id3)

### [get](#id4)

Retrieve metrics.

attribute-set:
:   [tcp-metrics](#tcp-metrics-attribute-set-tcp-metrics)

dont-validate:
:   [‘strict’, ‘dump’]

do:
:   **request**
    :   attributes:
        :   [`addr-ipv4`, `addr-ipv6`, `saddr-ipv4`, `saddr-ipv6`]

    **reply**
    :   attributes:
        :   [`addr-ipv4`, `addr-ipv6`, `saddr-ipv4`, `saddr-ipv6`, `age`, `vals`, `fopen-mss`, `fopen-syn-drops`, `fopen-syn-drop-ts`, `fopen-cookie`]

dump:
:   **reply**
    :   attributes:
        :   [`addr-ipv4`, `addr-ipv6`, `saddr-ipv4`, `saddr-ipv6`, `age`, `vals`, `fopen-mss`, `fopen-syn-drops`, `fopen-syn-drop-ts`, `fopen-cookie`]

### [del](#id5)

Delete metrics.

attribute-set:
:   [tcp-metrics](#tcp-metrics-attribute-set-tcp-metrics)

dont-validate:
:   [‘strict’, ‘dump’]

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`addr-ipv4`, `addr-ipv6`, `saddr-ipv4`, `saddr-ipv6`]

## [Definitions](#id6)

### [tcp-fastopen-cookie-max](#id7)

type:
:   const

value:
:   16

## [Attribute sets](#id8)

### [tcp-metrics](#id9)

#### addr-ipv4 (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### addr-ipv6 (`binary`)

byte-order:
:   big-endian

display-hint:
:   ipv6

#### age (`u64`)

#### tw-tsval (`u32`)

doc:
:   unused

#### tw-ts-stamp (`s32`)

doc:
:   unused

#### vals (`nest`)

nested-attributes:
:   [metrics](#tcp-metrics-attribute-set-metrics)

#### fopen-mss (`u16`)

#### fopen-syn-drops (`u16`)

#### fopen-syn-drop-ts (`u64`)

#### fopen-cookie (`binary`)

#### saddr-ipv4 (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### saddr-ipv6 (`binary`)

byte-order:
:   big-endian

display-hint:
:   ipv6

#### pad (`pad`)

### [metrics](#id10)

Attributes with metrics. Note that the values here do not match
the TCP\_METRIC\_\* defines in the kernel, because kernel defines
are off-by one (e.g. rtt is defined as enum 0, while netlink carries
attribute type 1).

#### rtt (`u32`)

doc:
:   Round Trip Time (RTT), in msecs with 3 bits fractional (left-shift by 3 to get the msec value).

#### rttvar (`u32`)

doc:
:   Round Trip Time VARiance (RTT), in msecs with 2 bits fractional (left-shift by 2 to get the msec value).

#### ssthresh (`u32`)

doc:
:   Slow Start THRESHold.

#### cwnd (`u32`)

doc:
:   Congestion Window.

#### reodering (`u32`)

doc:
:   Reodering metric.

#### rtt-us (`u32`)

doc:
:   Round Trip Time (RTT), in usecs, with 3 bits fractional (left-shift by 3 to get the msec value).

#### rttvar-us (`u32`)

doc:
:   Round Trip Time (RTT), in usecs, with 2 bits fractional (left-shift by 3 to get the msec value).
