# Familytcnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/tc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `tc` netlink specification](#id170)

## [Summary](#id171)

Netlink raw family for tc qdisc, chain, class and filter configuration over rtnetlink.

## [Operations](#id172)

### [newqdisc](#id173)

Create new tc qdisc.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`kind`, `options`, `rate`, `chain`, `ingress-block`, `egress-block`]

### [delqdisc](#id174)

Delete existing tc qdisc.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**

### [getqdisc](#id175)

Get / dump tc qdisc information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`dump-invisible`]

    **reply**
    :   attributes:
        :   [`kind`, `options`, `stats`, `xstats`, `rate`, `fcnt`, `stats2`, `stab`, `chain`, `ingress-block`, `egress-block`]

dump:
:   **request**
    :   attributes:
        :   [`dump-invisible`]

    **reply**
    :   attributes:
        :   [`kind`, `options`, `stats`, `xstats`, `rate`, `fcnt`, `stats2`, `stab`, `chain`, `ingress-block`, `egress-block`]

### [newtclass](#id176)

Get / dump tc traffic class information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`kind`, `options`, `rate`, `chain`, `ingress-block`, `egress-block`]

### [deltclass](#id177)

Get / dump tc traffic class information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**

### [gettclass](#id178)

Get / dump tc traffic class information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**

    **reply**
    :   attributes:
        :   [`kind`, `options`, `stats`, `xstats`, `rate`, `fcnt`, `stats2`, `stab`, `chain`, `ingress-block`, `egress-block`]

### [newtfilter](#id179)

Get / dump tc filter information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`kind`, `options`, `rate`, `chain`, `ingress-block`, `egress-block`]

### [deltfilter](#id180)

Get / dump tc filter information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`chain`, `kind`]

### [gettfilter](#id181)

Get / dump tc filter information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`chain`, `kind`]

    **reply**
    :   attributes:
        :   [`kind`, `options`, `stats`, `xstats`, `rate`, `fcnt`, `stats2`, `stab`, `chain`, `ingress-block`, `egress-block`]

dump:
:   **request**
    :   attributes:
        :   [`chain`, `dump-flags`]

    **reply**
    :   attributes:
        :   [`kind`, `options`, `stats`, `xstats`, `rate`, `fcnt`, `stats2`, `stab`, `chain`, `ingress-block`, `egress-block`]

### [newchain](#id182)

Get / dump tc chain information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`kind`, `options`, `rate`, `chain`, `ingress-block`, `egress-block`]

### [delchain](#id183)

Get / dump tc chain information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`chain`]

### [getchain](#id184)

Get / dump tc chain information.

attribute-set:
:   [attrs](#tc-attribute-set-attrs)

fixed-header:
:   [tcmsg](#tc-definition-tcmsg)

do:
:   **request**
    :   attributes:
        :   [`chain`]

    **reply**
    :   attributes:
        :   [`kind`, `options`, `stats`, `xstats`, `rate`, `fcnt`, `stats2`, `stab`, `chain`, `ingress-block`, `egress-block`]

## [Multicast groups](#id185)

* rtnlgrp-tc

## [Definitions](#id186)

### [tcmsg](#id187)

type:
:   struct

header:
:   linux/rtnetlink.h

members:
:   family (`u8`):


    ifindex (`s32`):


    handle (`u32`):


    parent (`u32`):


    info (`u32`):

### [cls-flags](#id188)

enum-name:
:   None

type:
:   flags

entries:
:   * `skip-hw`
    * `skip-sw`
    * `in-hw`
    * `not-in-nw`
    * `verbose`

### [flower-key-ctrl-flags](#id189)

name-prefix:
:   tca-flower-key-flags-

enum-name:
:   None

type:
:   flags

entries:
:   * `frag`
    * `firstfrag`
    * `tuncsum`
    * `tundf`
    * `tunoam`
    * `tuncrit`

### [dualpi2-drop-overload](#id190)

type:
:   enum

entries:
:   * `overflow`
    * `drop`

### [dualpi2-drop-early](#id191)

type:
:   enum

entries:
:   * `drop-dequeue`
    * `drop-enqueue`

### [dualpi2-ecn-mask](#id192)

type:
:   enum

value-start:
:   1

entries:
:   * `l4s-ect`
    * `cla-ect`
    * `any-ect`

### [dualpi2-split-gso](#id193)

type:
:   enum

entries:
:   * `no-split-gso`
    * `split-gso`

### [tc-stats](#id194)

type:
:   struct

members:
:   bytes (`u64`):
    :   Number of enqueued bytes

    packets (`u32`):
    :   Number of enqueued packets

    drops (`u32`):
    :   Packets dropped because of lack of resources

    overlimits (`u32`):
    :   Number of throttle events when this flow goes out of allocated bandwidth

    bps (`u32`):
    :   Current flow byte rate

    pps (`u32`):
    :   Current flow packet rate

    qlen (`u32`):


    backlog (`u32`):

### [tc-cbs-qopt](#id195)

type:
:   struct

members:
:   offload (`u8`):


    hicredit (`s32`):


    locredit (`s32`):


    idleslope (`s32`):


    sendslope (`s32`):

### [tc-etf-qopt](#id196)

type:
:   struct

members:
:   delta (`s32`):


    clockid (`s32`):


    flags (`s32`):

### [tc-fifo-qopt](#id197)

type:
:   struct

members:
:   limit (`u32`):
    :   Queue length; bytes for bfifo, packets for pfifo

### [tc-htb-opt](#id198)

type:
:   struct

members:
:   rate (`binary`):


    ceil (`binary`):


    buffer (`u32`):


    cbuffer (`u32`):


    quantum (`u32`):


    level (`u32`):


    prio (`u32`):

### [tc-htb-glob](#id199)

type:
:   struct

members:
:   version (`u32`):


    rate2quantum (`u32`):
    :   bps->quantum divisor

    defcls (`u32`):
    :   Default class number

    debug (`u32`):
    :   Debug flags

    direct-pkts (`u32`):
    :   Count of non shaped packets

### [tc-gred-qopt](#id200)

type:
:   struct

members:
:   limit (`u32`):
    :   HARD maximal queue length in bytes

    qth-min (`u32`):
    :   Min average length threshold in bytes

    qth-max (`u32`):
    :   Max average length threshold in bytes

    DP (`u32`):
    :   Up to 2^32 DPs

    backlog (`u32`):


    qave (`u32`):


    forced (`u32`):


    early (`u32`):


    other (`u32`):


    pdrop (`u32`):


    Wlog (`u8`):
    :   log(W)

    Plog (`u8`):
    :   log(P\_max / (qth-max - qth-min))

    Scell-log (`u8`):
    :   cell size for idle damping

    prio (`u8`):
    :   Priority of this VQ

    packets (`u32`):


    bytesin (`u32`):

### [tc-gred-sopt](#id201)

type:
:   struct

members:
:   DPs (`u32`):


    def-DP (`u32`):


    grio (`u8`):


    flags (`u8`):

### [tc-hfsc-qopt](#id202)

type:
:   struct

members:
:   defcls (`u16`):

### [tc-mqprio-qopt](#id203)

type:
:   struct

members:
:   num-tc (`u8`):


    prio-tc-map (`binary`):


    hw (`u8`):


    count (`binary`):


    offset (`binary`):

### [tc-multiq-qopt](#id204)

type:
:   struct

members:
:   bands (`u16`):
    :   Number of bands

    max-bands (`u16`):
    :   Maximum number of queues

### [tc-netem-qopt](#id205)

type:
:   struct

members:
:   latency (`u32`):
    :   Added delay in microseconds

    limit (`u32`):
    :   Fifo limit in packets

    loss (`u32`):
    :   Random packet loss (0=none, ~0=100%)

    gap (`u32`):
    :   Re-ordering gap (0 for none)

    duplicate (`u32`):
    :   Random packet duplication (0=none, ~0=100%)

    jitter (`u32`):
    :   Random jitter latency in microseconds

### [tc-netem-gimodel](#id206)

doc:
:   State transition probabilities for 4 state model

type:
:   struct

members:
:   p13 (`u32`):


    p31 (`u32`):


    p32 (`u32`):


    p14 (`u32`):


    p23 (`u32`):

### [tc-netem-gemodel](#id207)

doc:
:   Gilbert-Elliot models

type:
:   struct

members:
:   p (`u32`):


    r (`u32`):


    h (`u32`):


    k1 (`u32`):

### [tc-netem-corr](#id208)

type:
:   struct

members:
:   delay-corr (`u32`):
    :   Delay correlation

    loss-corr (`u32`):
    :   Packet loss correlation

    dup-corr (`u32`):
    :   Duplicate correlation

### [tc-netem-reorder](#id209)

type:
:   struct

members:
:   probability (`u32`):


    correlation (`u32`):

### [tc-netem-corrupt](#id210)

type:
:   struct

members:
:   probability (`u32`):


    correlation (`u32`):

### [tc-netem-rate](#id211)

type:
:   struct

members:
:   rate (`u32`):


    packet-overhead (`s32`):


    cell-size (`u32`):


    cell-overhead (`s32`):

### [tc-netem-slot](#id212)

type:
:   struct

members:
:   min-delay (`s64`):


    max-delay (`s64`):


    max-packets (`s32`):


    max-bytes (`s32`):


    dist-delay (`s64`):


    dist-jitter (`s64`):

### [tc-plug-qopt](#id213)

type:
:   struct

members:
:   action (`s32`):


    limit (`u32`):

### [tc-prio-qopt](#id214)

type:
:   struct

members:
:   bands (`u32`):
    :   Number of bands

    priomap (`binary`):
    :   Map of logical priority -> PRIO band

### [tc-red-qopt](#id215)

type:
:   struct

members:
:   limit (`u32`):
    :   Hard queue length in packets

    qth-min (`u32`):
    :   Min average threshold in packets

    qth-max (`u32`):
    :   Max average threshold in packets

    Wlog (`u8`):
    :   log(W)

    Plog (`u8`):
    :   log(P\_max / (qth-max - qth-min))

    Scell-log (`u8`):
    :   Cell size for idle damping

    flags (`u8`):

### [tc-sfb-qopt](#id216)

type:
:   struct

members:
:   rehash-interval (`u32`):


    warmup-time (`u32`):


    max (`u32`):


    bin-size (`u32`):


    increment (`u32`):


    decrement (`u32`):


    limit (`u32`):


    penalty-rate (`u32`):


    penalty-burst (`u32`):

### [tc-sfq-qopt](#id217)

type:
:   struct

members:
:   quantum (`u32`):
    :   Bytes per round allocated to flow

    perturb-period (`s32`):
    :   Period of hash perturbation

    limit (`u32`):
    :   Maximal packets in queue

    divisor (`u32`):
    :   Hash divisor

    flows (`u32`):
    :   Maximal number of flows

### [tc-sfqred-stats](#id218)

type:
:   struct

members:
:   prob-drop (`u32`):
    :   Early drops, below max threshold

    forced-drop (`u32`):
    :   Early drops, after max threshold

    prob-mark (`u32`):
    :   Marked packets, below max threshold

    forced-mark (`u32`):
    :   Marked packets, after max threshold

    prob-mark-head (`u32`):
    :   Marked packets, below max threshold

    forced-mark-head (`u32`):
    :   Marked packets, after max threshold

### [tc-sfq-qopt-v1](#id219)

type:
:   struct

members:
:   v0 (`binary`):


    depth (`u32`):
    :   Maximum number of packets per flow

    headdrop (`u32`):


    limit (`u32`):
    :   HARD maximal flow queue length in bytes

    qth-min (`u32`):
    :   Min average length threshold in bytes

    qth-max (`u32`):
    :   Max average length threshold in bytes

    Wlog (`u8`):
    :   log(W)

    Plog (`u8`):
    :   log(P\_max / (qth-max - qth-min))

    Scell-log (`u8`):
    :   Cell size for idle damping

    flags (`u8`):


    max-P (`u32`):
    :   probability, high resolution

    stats (`binary`):

### [tc-ratespec](#id220)

type:
:   struct

header:
:   linux/pkt\_sched.h

members:
:   cell-log (`u8`):


    linklayer (`u8`):


    overhead (`u8`):


    cell-align (`u8`):


    mpu (`u8`):


    rate (`u32`):

### [tc-tbf-qopt](#id221)

type:
:   struct

members:
:   rate (`binary`):


    peakrate (`binary`):


    limit (`u32`):


    buffer (`u32`):


    mtu (`u32`):

### [tc-sizespec](#id222)

type:
:   struct

members:
:   cell-log (`u8`):


    size-log (`u8`):


    cell-align (`s16`):


    overhead (`s32`):


    linklayer (`u32`):


    mpu (`u32`):


    mtu (`u32`):


    tsize (`u32`):

### [gnet-estimator](#id223)

type:
:   struct

members:
:   interval (`s8`):
    :   Sampling period

    ewma-log (`u8`):
    :   The `log()` of measurement window weight

### [tc-choke-xstats](#id224)

type:
:   struct

members:
:   early (`u32`):
    :   Early drops

    pdrop (`u32`):
    :   Drops due to queue limits

    other (`u32`):
    :   Drops due to `drop()` calls

    marked (`u32`):
    :   Marked packets

    matched (`u32`):
    :   Drops due to flow match

### [tc-codel-xstats](#id225)

type:
:   struct

members:
:   maxpacket (`u32`):
    :   Largest packet we’ve seen so far

    count (`u32`):
    :   How many drops we’ve done since the last time we entered dropping state

    lastcount (`u32`):
    :   Count at entry to dropping state

    ldelay (`u32`):
    :   in-queue delay seen by most recently dequeued packet

    drop-next (`s32`):
    :   Time to drop next packet

    drop-overlimit (`u32`):
    :   Number of times max qdisc packet limit was hit

    ecn-mark (`u32`):
    :   Number of packets we’ve ECN marked instead of dropped

    dropping (`u32`):
    :   Are we in a dropping state?

    ce-mark (`u32`):
    :   Number of CE marked packets because of ce-threshold

### [tc-fq-codel-xstats](#id226)

type:
:   struct

members:
:   type (`u32`):


    maxpacket (`u32`):
    :   Largest packet we’ve seen so far

    drop-overlimit (`u32`):
    :   Number of times max qdisc packet limit was hit

    ecn-mark (`u32`):
    :   Number of packets we ECN marked instead of being dropped

    new-flow-count (`u32`):
    :   Number of times packets created a new flow

    new-flows-len (`u32`):
    :   Count of flows in new list

    old-flows-len (`u32`):
    :   Count of flows in old list

    ce-mark (`u32`):
    :   Packets above ce-threshold

    memory-usage (`u32`):
    :   Memory usage in bytes

    drop-overmemory (`u32`):

### [tc-dualpi2-xstats](#id227)

type:
:   struct

members:
:   prob (`u32`):
    :   Current base PI probability

    delay-c (`u32`):
    :   Current C-queue delay in microseconds

    delay-l (`u32`):
    :   Current L-queue delay in microseconds

    pkts-in-c (`u32`):
    :   Number of packets enqueued in the C-queue

    pkts-in-l (`u32`):
    :   Number of packets enqueued in the L-queue

    maxq (`u32`):
    :   Maximum number of packets seen by the DualPI2

    ecn-mark (`u32`):
    :   All packets marked with ECN

    step-mark (`u32`):
    :   Only packets marked with ECN due to L-queue step AQM

    credit (`s32`):
    :   Current credit value for WRR

    memory-used (`u32`):
    :   Memory used in bytes by the DualPI2

    max-memory-used (`u32`):
    :   Maximum memory used in bytes by the DualPI2

    memory-limit (`u32`):
    :   Memory limit in bytes

### [tc-fq-pie-xstats](#id228)

type:
:   struct

members:
:   packets-in (`u32`):
    :   Total number of packets enqueued

    dropped (`u32`):
    :   Packets dropped due to fq\_pie\_action

    overlimit (`u32`):
    :   Dropped due to lack of space in queue

    overmemory (`u32`):
    :   Dropped due to lack of memory in queue

    ecn-mark (`u32`):
    :   Packets marked with ECN

    new-flow-count (`u32`):
    :   Count of new flows created by packets

    new-flows-len (`u32`):
    :   Count of flows in new list

    old-flows-len (`u32`):
    :   Count of flows in old list

    memory-usage (`u32`):
    :   Total memory across all queues

### [tc-fq-qd-stats](#id229)

type:
:   struct

members:
:   gc-flows (`u64`):


    highprio-packets (`u64`):
    :   obsolete

    tcp-retrans (`u64`):
    :   obsolete

    throttled (`u64`):


    flows-plimit (`u64`):


    pkts-too-long (`u64`):


    allocation-errors (`u64`):


    time-next-delayed-flow (`s64`):


    flows (`u32`):


    inactive-flows (`u32`):


    throttled-flows (`u32`):


    unthrottle-latency-ns (`u32`):


    ce-mark (`u64`):
    :   Packets above ce-threshold

    horizon-drops (`u64`):


    horizon-caps (`u64`):


    fastpath-packets (`u64`):


    band-drops (`binary`):


    band-pkt-count (`binary`):

### [tc-hhf-xstats](#id230)

type:
:   struct

members:
:   drop-overlimit (`u32`):
    :   Number of times max qdisc packet limit was hit

    hh-overlimit (`u32`):
    :   Number of times max heavy-hitters was hit

    hh-tot-count (`u32`):
    :   Number of captured heavy-hitters so far

    hh-cur-count (`u32`):
    :   Number of current heavy-hitters

### [tc-pie-xstats](#id231)

type:
:   struct

members:
:   prob (`u64`):
    :   Current probability

    delay (`u32`):
    :   Current delay in ms

    avg-dq-rate (`u32`):
    :   Current average dq rate in bits/pie-time

    dq-rate-estimating (`u32`):
    :   Is avg-dq-rate being calculated?

    packets-in (`u32`):
    :   Total number of packets enqueued

    dropped (`u32`):
    :   Packets dropped due to pie action

    overlimit (`u32`):
    :   Dropped due to lack of space in queue

    maxq (`u32`):
    :   Maximum queue size

    ecn-mark (`u32`):
    :   Packets marked with ECN

### [tc-red-xstats](#id232)

type:
:   struct

members:
:   early (`u32`):
    :   Early drops

    pdrop (`u32`):
    :   Drops due to queue limits

    other (`u32`):
    :   Drops due to `drop()` calls

    marked (`u32`):
    :   Marked packets

### [tc-sfb-xstats](#id233)

type:
:   struct

members:
:   earlydrop (`u32`):


    penaltydrop (`u32`):


    bucketdrop (`u32`):


    queuedrop (`u32`):


    childdrop (`u32`):
    :   drops in child qdisc

    marked (`u32`):


    maxqlen (`u32`):


    maxprob (`u32`):


    avgprob (`u32`):

### [tc-sfq-xstats](#id234)

type:
:   struct

members:
:   allot (`s32`):

### [gnet-stats-basic](#id235)

type:
:   struct

members:
:   bytes (`u64`):


    packets (`u32`):

### [gnet-stats-rate-est](#id236)

type:
:   struct

members:
:   bps (`u32`):


    pps (`u32`):

### [gnet-stats-rate-est64](#id237)

type:
:   struct

members:
:   bps (`u64`):


    pps (`u64`):

### [gnet-stats-queue](#id238)

type:
:   struct

members:
:   qlen (`u32`):


    backlog (`u32`):


    drops (`u32`):


    requeues (`u32`):


    overlimits (`u32`):

### [tc-u32-key](#id239)

type:
:   struct

members:
:   mask (`u32`):


    val (`u32`):


    off (`s32`):


    offmask (`s32`):

### [tc-u32-mark](#id240)

type:
:   struct

members:
:   val (`u32`):


    mask (`u32`):


    success (`u32`):

### [tc-u32-sel](#id241)

type:
:   struct

members:
:   flags (`u8`):


    offshift (`u8`):


    nkeys (`u8`):


    offmask (`u16`):


    off (`u16`):


    offoff (`s16`):


    hoff (`s16`):


    hmask (`u32`):


    keys (`binary`):

### [tc-u32-pcnt](#id242)

type:
:   struct

members:
:   rcnt (`u64`):


    rhit (`u64`):


    kcnts (`u64`):

### [tcf-t](#id243)

type:
:   struct

members:
:   install (`u64`):


    lastuse (`u64`):


    expires (`u64`):


    firstuse (`u64`):

### [tc-gact](#id244)

type:
:   struct

members:
:   index (`u32`):


    capab (`u32`):


    action (`s32`):


    refcnt (`s32`):


    bindcnt (`s32`):

### [tc-gact-p](#id245)

type:
:   struct

members:
:   ptype (`u16`):


    pval (`u16`):


    paction (`s32`):

### [tcf-ematch-tree-hdr](#id246)

type:
:   struct

members:
:   nmatches (`u16`):


    progid (`u16`):

### [tc-basic-pcnt](#id247)

type:
:   struct

members:
:   rcnt (`u64`):


    rhit (`u64`):

### [tc-matchall-pcnt](#id248)

type:
:   struct

members:
:   rhit (`u64`):

### [tc-mpls](#id249)

type:
:   struct

members:
:   index (`u32`):


    capab (`u32`):


    action (`s32`):


    refcnt (`s32`):


    bindcnt (`s32`):


    m-action (`s32`):

### [tc-police](#id250)

type:
:   struct

members:
:   index (`u32`):


    action (`s32`):


    limit (`u32`):


    burst (`u32`):


    mtu (`u32`):


    rate (`binary`):


    peakrate (`binary`):


    refcnt (`s32`):


    bindcnt (`s32`):


    capab (`u32`):

### [tc-pedit-sel](#id251)

type:
:   struct

members:
:   index (`u32`):


    capab (`u32`):


    action (`s32`):


    refcnt (`s32`):


    bindcnt (`s32`):


    nkeys (`u8`):


    flags (`u8`):


    keys (`binary`):

### [tc-pedit-key](#id252)

type:
:   struct

members:
:   mask (`u32`):


    val (`u32`):


    off (`u32`):


    at (`u32`):


    offmask (`u32`):


    shift (`u32`):

### [tc-vlan](#id253)

type:
:   struct

members:
:   index (`u32`):


    capab (`u32`):


    action (`s32`):


    refcnt (`s32`):


    bindcnt (`s32`):


    v-action (`s32`):

## [Attribute sets](#id254)

### [attrs](#id255)

#### kind (`string`)

#### options (`sub-message`)

sub-message:
:   [options-msg](#tc-sub-message-options-msg)

selector:
:   kind

#### stats (`binary`)

struct:
:   [tc-stats](#tc-definition-tc-stats)

#### xstats (`sub-message`)

sub-message:
:   [tca-stats-app-msg](#tc-sub-message-tca-stats-app-msg)

selector:
:   kind

#### rate (`binary`)

struct:
:   [gnet-estimator](#tc-definition-gnet-estimator)

#### fcnt (`u32`)

#### stats2 (`nest`)

nested-attributes:
:   [tca-stats-attrs](#tc-attribute-set-tca-stats-attrs)

#### stab (`nest`)

nested-attributes:
:   [tca-stab-attrs](#tc-attribute-set-tca-stab-attrs)

#### pad (`pad`)

#### dump-invisible (`flag`)

#### chain (`u32`)

#### hw-offload (`u8`)

#### ingress-block (`u32`)

#### egress-block (`u32`)

#### dump-flags (`bitfield32`)

#### ext-warn-msg (`string`)

### [act-attrs](#id256)

#### kind (`string`)

#### options (`sub-message`)

sub-message:
:   [act-options-msg](#tc-sub-message-act-options-msg)

selector:
:   kind

#### index (`u32`)

#### stats (`nest`)

nested-attributes:
:   [tca-stats-attrs](#tc-attribute-set-tca-stats-attrs)

#### pad (`pad`)

#### cookie (`binary`)

#### flags (`bitfield32`)

#### hw-stats (`bitfield32`)

#### used-hw-stats (`bitfield32`)

#### in-hw-count (`u32`)

### [act-bpf-attrs](#id257)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### ops-len (`u16`)

#### ops (`binary`)

#### fd (`u32`)

#### name (`string`)

#### pad (`pad`)

#### tag (`binary`)

#### id (`binary`)

### [act-connmark-attrs](#id258)

#### parms (`binary`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### pad (`pad`)

### [act-csum-attrs](#id259)

#### parms (`binary`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### pad (`pad`)

### [act-ct-attrs](#id260)

#### parms (`binary`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### action (`u16`)

#### zone (`u16`)

#### mark (`u32`)

#### mark-mask (`u32`)

#### labels (`binary`)

#### labels-mask (`binary`)

#### nat-ipv4-min (`u32`)

byte-order:
:   big-endian

#### nat-ipv4-max (`u32`)

byte-order:
:   big-endian

#### nat-ipv6-min (`binary`)

#### nat-ipv6-max (`binary`)

#### nat-port-min (`u16`)

byte-order:
:   big-endian

#### nat-port-max (`u16`)

byte-order:
:   big-endian

#### pad (`pad`)

#### helper-name (`string`)

#### helper-family (`u8`)

#### helper-proto (`u8`)

### [act-ctinfo-attrs](#id261)

#### pad (`pad`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### act (`binary`)

#### zone (`u16`)

#### parms-dscp-mask (`u32`)

#### parms-dscp-statemask (`u32`)

#### parms-cpmark-mask (`u32`)

#### stats-dscp-set (`u64`)

#### stats-dscp-error (`u64`)

#### stats-cpmark-set (`u64`)

### [act-gate-attrs](#id262)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### pad (`pad`)

#### priority (`s32`)

#### entry-list (`binary`)

#### base-time (`u64`)

#### cycle-time (`u64`)

#### cycle-time-ext (`u64`)

#### flags (`u32`)

#### clockid (`s32`)

### [act-ife-attrs](#id263)

#### parms (`binary`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### dmac (`binary`)

#### smac (`binary`)

#### type (`u16`)

#### metalst (`binary`)

#### pad (`pad`)

### [act-mirred-attrs](#id264)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### pad (`pad`)

#### blockid (`binary`)

### [act-mpls-attrs](#id265)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

struct:
:   [tc-mpls](#tc-definition-tc-mpls)

#### pad (`pad`)

#### proto (`u16`)

byte-order:
:   big-endian

#### label (`u32`)

#### tc (`u8`)

#### ttl (`u8`)

#### bos (`u8`)

### [act-nat-attrs](#id266)

#### parms (`binary`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### pad (`pad`)

### [act-pedit-attrs](#id267)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

struct:
:   [tc-pedit-sel](#tc-definition-tc-pedit-sel)

#### pad (`pad`)

#### parms-ex (`binary`)

#### keys-ex (`binary`)

#### key-ex (`binary`)

### [act-simple-attrs](#id268)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### data (`binary`)

#### pad (`pad`)

### [act-skbedit-attrs](#id269)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### priority (`u32`)

#### queue-mapping (`u16`)

#### mark (`u32`)

#### pad (`pad`)

#### ptype (`u16`)

#### mask (`u32`)

#### flags (`u64`)

#### queue-mapping-max (`u16`)

### [act-skbmod-attrs](#id270)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### dmac (`binary`)

#### smac (`binary`)

#### etype (`binary`)

#### pad (`pad`)

### [act-tunnel-key-attrs](#id271)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

#### enc-ipv4-src (`u32`)

byte-order:
:   big-endian

#### enc-ipv4-dst (`u32`)

byte-order:
:   big-endian

#### enc-ipv6-src (`binary`)

#### enc-ipv6-dst (`binary`)

#### enc-key-id (`u64`)

byte-order:
:   big-endian

#### pad (`pad`)

#### enc-dst-port (`u16`)

byte-order:
:   big-endian

#### no-csum (`u8`)

#### enc-opts (`binary`)

#### enc-tos (`u8`)

#### enc-ttl (`u8`)

#### no-frag (`flag`)

### [act-vlan-attrs](#id272)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

struct:
:   [tc-vlan](#tc-definition-tc-vlan)

#### push-vlan-id (`u16`)

#### push-vlan-protocol (`u16`)

#### pad (`pad`)

#### push-vlan-priority (`u8`)

#### push-eth-dst (`binary`)

#### push-eth-src (`binary`)

### [basic-attrs](#id273)

#### classid (`u32`)

#### ematches (`nest`)

nested-attributes:
:   [ematch-attrs](#tc-attribute-set-ematch-attrs)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### pcnt (`binary`)

struct:
:   [tc-basic-pcnt](#tc-definition-tc-basic-pcnt)

#### pad (`pad`)

### [bpf-attrs](#id274)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### classid (`u32`)

#### ops-len (`u16`)

#### ops (`binary`)

#### fd (`u32`)

#### name (`string`)

#### flags (`u32`)

#### flags-gen (`u32`)

#### tag (`binary`)

#### id (`u32`)

### [cake-attrs](#id275)

#### pad (`pad`)

#### base-rate64 (`u64`)

#### diffserv-mode (`u32`)

#### atm (`u32`)

#### flow-mode (`u32`)

#### overhead (`u32`)

#### rtt (`u32`)

#### target (`u32`)

#### autorate (`u32`)

#### memory (`u32`)

#### nat (`u32`)

#### raw (`u32`)

#### wash (`u32`)

#### mpu (`u32`)

#### ingress (`u32`)

#### ack-filter (`u32`)

#### split-gso (`u32`)

#### fwmark (`u32`)

### [cake-stats-attrs](#id276)

#### pad (`pad`)

#### capacity-estimate64 (`u64`)

#### memory-limit (`u32`)

#### memory-used (`u32`)

#### avg-netoff (`u32`)

#### min-netlen (`u32`)

#### max-netlen (`u32`)

#### min-adjlen (`u32`)

#### max-adjlen (`u32`)

#### tin-stats (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [cake-tin-stats-attrs](#tc-attribute-set-cake-tin-stats-attrs)

#### deficit (`s32`)

#### cobalt-count (`u32`)

#### dropping (`u32`)

#### drop-next-us (`s32`)

#### p-drop (`u32`)

#### blue-timer-us (`s32`)

#### active-queues (`u32`)

### [cake-tin-stats-attrs](#id277)

#### pad (`pad`)

#### sent-packets (`u32`)

#### sent-bytes64 (`u64`)

#### dropped-packets (`u32`)

#### dropped-bytes64 (`u64`)

#### acks-dropped-packets (`u32`)

#### acks-dropped-bytes64 (`u64`)

#### ecn-marked-packets (`u32`)

#### ecn-marked-bytes64 (`u64`)

#### backlog-packets (`u32`)

#### backlog-bytes (`u32`)

#### threshold-rate64 (`u64`)

#### target-us (`u32`)

#### interval-us (`u32`)

#### way-indirect-hits (`u32`)

#### way-misses (`u32`)

#### way-collisions (`u32`)

#### peak-delay-us (`u32`)

#### avg-delay-us (`u32`)

#### base-delay-us (`u32`)

#### sparse-flows (`u32`)

#### bulk-flows (`u32`)

#### unresponsive-flows (`u32`)

#### max-skblen (`u32`)

#### flow-quantum (`u32`)

### [cbs-attrs](#id278)

#### parms (`binary`)

struct:
:   [tc-cbs-qopt](#tc-definition-tc-cbs-qopt)

### [cgroup-attrs](#id279)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### ematches (`binary`)

### [choke-attrs](#id280)

#### parms (`binary`)

struct:
:   [tc-red-qopt](#tc-definition-tc-red-qopt)

#### stab (`binary`)

#### max-p (`u32`)

### [codel-attrs](#id281)

#### target (`u32`)

#### limit (`u32`)

#### interval (`u32`)

#### ecn (`u32`)

#### ce-threshold (`u32`)

### [drr-attrs](#id282)

#### quantum (`u32`)

### [dualpi2-attrs](#id283)

#### limit (`u32`)

doc:
:   Limit of total number of packets in queue

#### memory-limit (`u32`)

doc:
:   Memory limit of total number of packets in queue

#### target (`u32`)

doc:
:   Classic target delay in microseconds

#### tupdate (`u32`)

doc:
:   Drop probability update interval time in microseconds

#### alpha (`u32`)

doc:
:   Integral gain factor in Hz for PI controller

#### beta (`u32`)

doc:
:   Proportional gain factor in Hz for PI controller

#### step-thresh-pkts (`u32`)

doc:
:   L4S step marking threshold in packets

#### step-thresh-us (`u32`)

doc:
:   L4S Step marking threshold in microseconds

#### min-qlen-step (`u32`)

doc:
:   Packets enqueued to the L-queue can apply the step threshold when the queue length of L-queue is larger than this value. (0 is recommended)

#### coupling (`u8`)

doc:
:   Probability coupling factor between Classic and L4S (2 is recommended)

#### drop-overload (`u8`)

doc:
:   Control the overload strategy (drop to preserve latency or let the queue overflow)

enum:
:   [dualpi2-drop-overload](#tc-definition-dualpi2-drop-overload)

#### drop-early (`u8`)

doc:
:   Decide where the Classic packets are PI-based dropped or marked

enum:
:   [dualpi2-drop-early](#tc-definition-dualpi2-drop-early)

#### c-protection (`u8`)

doc:
:   Classic WRR weight in percentage (from 0 to 100)

#### ecn-mask (`u8`)

doc:
:   Configure the L-queue ECN classifier

enum:
:   [dualpi2-ecn-mask](#tc-definition-dualpi2-ecn-mask)

#### split-gso (`u8`)

doc:
:   Split aggregated skb or not

enum:
:   [dualpi2-split-gso](#tc-definition-dualpi2-split-gso)

### [ematch-attrs](#id284)

#### tree-hdr (`binary`)

struct:
:   [tcf-ematch-tree-hdr](#tc-definition-tcf-ematch-tree-hdr)

#### tree-list (`binary`)

### [flow-attrs](#id285)

#### keys (`u32`)

#### mode (`u32`)

#### baseclass (`u32`)

#### rshift (`u32`)

#### addend (`u32`)

#### mask (`u32`)

#### xor (`u32`)

#### divisor (`u32`)

#### act (`binary`)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### ematches (`binary`)

#### perturb (`u32`)

### [flower-attrs](#id286)

#### classid (`u32`)

#### indev (`string`)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### key-eth-dst (`binary`)

display-hint:
:   mac

#### key-eth-dst-mask (`binary`)

display-hint:
:   mac

#### key-eth-src (`binary`)

display-hint:
:   mac

#### key-eth-src-mask (`binary`)

display-hint:
:   mac

#### key-eth-type (`u16`)

byte-order:
:   big-endian

#### key-ip-proto (`u8`)

#### key-ipv4-src (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-ipv4-src-mask (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-ipv4-dst (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-ipv4-dst-mask (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-ipv6-src (`binary`)

display-hint:
:   ipv6

#### key-ipv6-src-mask (`binary`)

display-hint:
:   ipv6

#### key-ipv6-dst (`binary`)

display-hint:
:   ipv6

#### key-ipv6-dst-mask (`binary`)

display-hint:
:   ipv6

#### key-tcp-src (`u16`)

byte-order:
:   big-endian

#### key-tcp-dst (`u16`)

byte-order:
:   big-endian

#### key-udp-src (`u16`)

byte-order:
:   big-endian

#### key-udp-dst (`u16`)

byte-order:
:   big-endian

#### flags (`u32`)

enum:
:   [cls-flags](#tc-definition-cls-flags)

enum-as-flags:
:   True

#### key-vlan-id (`u16`)

byte-order:
:   big-endian

#### key-vlan-prio (`u8`)

#### key-vlan-eth-type (`u16`)

byte-order:
:   big-endian

#### key-enc-key-id (`u32`)

byte-order:
:   big-endian

#### key-enc-ipv4-src (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-enc-ipv4-src-mask (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-enc-ipv4-dst (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-enc-ipv4-dst-mask (`u32`)

byte-order:
:   big-endian

display-hint:
:   ipv4

#### key-enc-ipv6-src (`binary`)

display-hint:
:   ipv6

#### key-enc-ipv6-src-mask (`binary`)

display-hint:
:   ipv6

#### key-enc-ipv6-dst (`binary`)

display-hint:
:   ipv6

#### key-enc-ipv6-dst-mask (`binary`)

display-hint:
:   ipv6

#### key-tcp-src-mask (`u16`)

byte-order:
:   big-endian

#### key-tcp-dst-mask (`u16`)

byte-order:
:   big-endian

#### key-udp-src-mask (`u16`)

byte-order:
:   big-endian

#### key-udp-dst-mask (`u16`)

byte-order:
:   big-endian

#### key-sctp-src-mask (`u16`)

byte-order:
:   big-endian

#### key-sctp-dst-mask (`u16`)

byte-order:
:   big-endian

#### key-sctp-src (`u16`)

byte-order:
:   big-endian

#### key-sctp-dst (`u16`)

byte-order:
:   big-endian

#### key-enc-udp-src-port (`u16`)

byte-order:
:   big-endian

#### key-enc-udp-src-port-mask (`u16`)

byte-order:
:   big-endian

#### key-enc-udp-dst-port (`u16`)

byte-order:
:   big-endian

#### key-enc-udp-dst-port-mask (`u16`)

byte-order:
:   big-endian

#### key-flags (`u32`)

byte-order:
:   big-endian

enum:
:   [flower-key-ctrl-flags](#tc-definition-flower-key-ctrl-flags)

enum-as-flags:
:   True

#### key-flags-mask (`u32`)

byte-order:
:   big-endian

enum:
:   [flower-key-ctrl-flags](#tc-definition-flower-key-ctrl-flags)

enum-as-flags:
:   True

#### key-icmpv4-code (`u8`)

#### key-icmpv4-code-mask (`u8`)

#### key-icmpv4-type (`u8`)

#### key-icmpv4-type-mask (`u8`)

#### key-icmpv6-code (`u8`)

#### key-icmpv6-code-mask (`u8`)

#### key-icmpv6-type (`u8`)

#### key-icmpv6-type-mask (`u8`)

#### key-arp-sip (`u32`)

byte-order:
:   big-endian

#### key-arp-sip-mask (`u32`)

byte-order:
:   big-endian

#### key-arp-tip (`u32`)

byte-order:
:   big-endian

#### key-arp-tip-mask (`u32`)

byte-order:
:   big-endian

#### key-arp-op (`u8`)

#### key-arp-op-mask (`u8`)

#### key-arp-sha (`binary`)

display-hint:
:   mac

#### key-arp-sha-mask (`binary`)

display-hint:
:   mac

#### key-arp-tha (`binary`)

display-hint:
:   mac

#### key-arp-tha-mask (`binary`)

display-hint:
:   mac

#### key-mpls-ttl (`u8`)

#### key-mpls-bos (`u8`)

#### key-mpls-tc (`u8`)

#### key-mpls-label (`u32`)

byte-order:
:   big-endian

#### key-tcp-flags (`u16`)

byte-order:
:   big-endian

#### key-tcp-flags-mask (`u16`)

byte-order:
:   big-endian

#### key-ip-tos (`u8`)

#### key-ip-tos-mask (`u8`)

#### key-ip-ttl (`u8`)

#### key-ip-ttl-mask (`u8`)

#### key-cvlan-id (`u16`)

byte-order:
:   big-endian

#### key-cvlan-prio (`u8`)

#### key-cvlan-eth-type (`u16`)

byte-order:
:   big-endian

#### key-enc-ip-tos (`u8`)

#### key-enc-ip-tos-mask (`u8`)

#### key-enc-ip-ttl (`u8`)

#### key-enc-ip-ttl-mask (`u8`)

#### key-enc-opts (`nest`)

nested-attributes:
:   [flower-key-enc-opts-attrs](#tc-attribute-set-flower-key-enc-opts-attrs)

#### key-enc-opts-mask (`nest`)

nested-attributes:
:   [flower-key-enc-opts-attrs](#tc-attribute-set-flower-key-enc-opts-attrs)

#### in-hw-count (`u32`)

#### key-port-src-min (`u16`)

byte-order:
:   big-endian

#### key-port-src-max (`u16`)

byte-order:
:   big-endian

#### key-port-dst-min (`u16`)

byte-order:
:   big-endian

#### key-port-dst-max (`u16`)

byte-order:
:   big-endian

#### key-ct-state (`u16`)

#### key-ct-state-mask (`u16`)

#### key-ct-zone (`u16`)

#### key-ct-zone-mask (`u16`)

#### key-ct-mark (`u32`)

#### key-ct-mark-mask (`u32`)

#### key-ct-labels (`binary`)

#### key-ct-labels-mask (`binary`)

#### key-mpls-opts (`nest`)

nested-attributes:
:   [flower-key-mpls-opt-attrs](#tc-attribute-set-flower-key-mpls-opt-attrs)

#### key-hash (`u32`)

#### key-hash-mask (`u32`)

#### key-num-of-vlans (`u8`)

#### key-pppoe-sid (`u16`)

byte-order:
:   big-endian

#### key-ppp-proto (`u16`)

byte-order:
:   big-endian

#### key-l2tpv3-sid (`u32`)

byte-order:
:   big-endian

#### l2-miss (`u8`)

#### key-cfm (`nest`)

nested-attributes:
:   [flower-key-cfm-attrs](#tc-attribute-set-flower-key-cfm-attrs)

#### key-spi (`u32`)

byte-order:
:   big-endian

#### key-spi-mask (`u32`)

byte-order:
:   big-endian

#### key-enc-flags (`u32`)

byte-order:
:   big-endian

enum:
:   [flower-key-ctrl-flags](#tc-definition-flower-key-ctrl-flags)

enum-as-flags:
:   True

#### key-enc-flags-mask (`u32`)

byte-order:
:   big-endian

enum:
:   [flower-key-ctrl-flags](#tc-definition-flower-key-ctrl-flags)

enum-as-flags:
:   True

### [flower-key-enc-opts-attrs](#id287)

#### geneve (`nest`)

nested-attributes:
:   [flower-key-enc-opt-geneve-attrs](#tc-attribute-set-flower-key-enc-opt-geneve-attrs)

#### vxlan (`nest`)

nested-attributes:
:   [flower-key-enc-opt-vxlan-attrs](#tc-attribute-set-flower-key-enc-opt-vxlan-attrs)

#### erspan (`nest`)

nested-attributes:
:   [flower-key-enc-opt-erspan-attrs](#tc-attribute-set-flower-key-enc-opt-erspan-attrs)

#### gtp (`nest`)

nested-attributes:
:   [flower-key-enc-opt-gtp-attrs](#tc-attribute-set-flower-key-enc-opt-gtp-attrs)

### [flower-key-enc-opt-geneve-attrs](#id288)

#### class (`u16`)

#### type (`u8`)

#### data (`binary`)

### [flower-key-enc-opt-vxlan-attrs](#id289)

#### gbp (`u32`)

### [flower-key-enc-opt-erspan-attrs](#id290)

#### ver (`u8`)

#### index (`u32`)

#### dir (`u8`)

#### hwid (`u8`)

### [flower-key-enc-opt-gtp-attrs](#id291)

#### pdu-type (`u8`)

#### qfi (`u8`)

### [flower-key-mpls-opt-attrs](#id292)

#### lse-depth (`u8`)

#### lse-ttl (`u8`)

#### lse-bos (`u8`)

#### lse-tc (`u8`)

#### lse-label (`u32`)

### [flower-key-cfm-attrs](#id293)

#### md-level (`u8`)

#### opcode (`u8`)

### [fw-attrs](#id294)

#### classid (`u32`)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### indev (`string`)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### mask (`u32`)

### [gred-attrs](#id295)

#### parms (`binary`)

#### stab (`binary`)

sub-type:
:   u8

#### dps (`binary`)

struct:
:   [tc-gred-sopt](#tc-definition-tc-gred-sopt)

#### max-p (`binary`)

sub-type:
:   u32

#### limit (`u32`)

#### vq-list (`nest`)

nested-attributes:
:   [tca-gred-vq-list-attrs](#tc-attribute-set-tca-gred-vq-list-attrs)

### [tca-gred-vq-list-attrs](#id296)

#### entry (`nest`)

nested-attributes:
:   [tca-gred-vq-entry-attrs](#tc-attribute-set-tca-gred-vq-entry-attrs)

multi-attr:
:   True

### [tca-gred-vq-entry-attrs](#id297)

#### pad (`pad`)

#### dp (`u32`)

#### stat-bytes (`u64`)

#### stat-packets (`u32`)

#### stat-backlog (`u32`)

#### stat-prob-drop (`u32`)

#### stat-prob-mark (`u32`)

#### stat-forced-drop (`u32`)

#### stat-forced-mark (`u32`)

#### stat-pdrop (`u32`)

#### stat-other (`u32`)

#### flags (`u32`)

### [hfsc-attrs](#id298)

#### rsc (`binary`)

#### fsc (`binary`)

#### usc (`binary`)

### [hhf-attrs](#id299)

#### backlog-limit (`u32`)

#### quantum (`u32`)

#### hh-flows-limit (`u32`)

#### reset-timeout (`u32`)

#### admit-bytes (`u32`)

#### evict-timeout (`u32`)

#### non-hh-weight (`u32`)

### [htb-attrs](#id300)

#### parms (`binary`)

struct:
:   [tc-htb-opt](#tc-definition-tc-htb-opt)

#### init (`binary`)

struct:
:   [tc-htb-glob](#tc-definition-tc-htb-glob)

#### ctab (`binary`)

#### rtab (`binary`)

#### direct-qlen (`u32`)

#### rate64 (`u64`)

#### ceil64 (`u64`)

#### pad (`pad`)

#### offload (`flag`)

### [matchall-attrs](#id301)

#### classid (`u32`)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### flags (`u32`)

#### pcnt (`binary`)

struct:
:   [tc-matchall-pcnt](#tc-definition-tc-matchall-pcnt)

#### pad (`pad`)

### [etf-attrs](#id302)

#### parms (`binary`)

struct:
:   [tc-etf-qopt](#tc-definition-tc-etf-qopt)

### [ets-attrs](#id303)

#### nbands (`u8`)

#### nstrict (`u8`)

#### quanta (`nest`)

nested-attributes:
:   [ets-attrs](#tc-attribute-set-ets-attrs)

#### quanta-band (`u32`)

multi-attr:
:   True

#### priomap (`nest`)

nested-attributes:
:   [ets-attrs](#tc-attribute-set-ets-attrs)

#### priomap-band (`u8`)

multi-attr:
:   True

### [fq-attrs](#id304)

#### plimit (`u32`)

doc:
:   Limit of total number of packets in queue

#### flow-plimit (`u32`)

doc:
:   Limit of packets per flow

#### quantum (`u32`)

doc:
:   RR quantum

#### initial-quantum (`u32`)

doc:
:   RR quantum for new flow

#### rate-enable (`u32`)

doc:
:   Enable / disable rate limiting

#### flow-default-rate (`u32`)

doc:
:   Obsolete, do not use

#### flow-max-rate (`u32`)

doc:
:   Per flow max rate

#### buckets-log (`u32`)

doc:
:   log2(number of buckets)

#### flow-refill-delay (`u32`)

doc:
:   Flow credit refill delay in usec

#### orphan-mask (`u32`)

doc:
:   Mask applied to orphaned skb hashes

#### low-rate-threshold (`u32`)

doc:
:   Per packet delay under this rate

#### ce-threshold (`u32`)

doc:
:   DCTCP-like CE marking threshold

#### timer-slack (`u32`)

#### horizon (`u32`)

doc:
:   Time horizon in usec

#### horizon-drop (`u8`)

doc:
:   Drop packets beyond horizon, or cap their EDT

#### priomap (`binary`)

struct:
:   [tc-prio-qopt](#tc-definition-tc-prio-qopt)

#### weights (`binary`)

sub-type:
:   s32

doc:
:   Weights for each band

### [fq-codel-attrs](#id305)

#### target (`u32`)

#### limit (`u32`)

#### interval (`u32`)

#### ecn (`u32`)

#### flows (`u32`)

#### quantum (`u32`)

#### ce-threshold (`u32`)

#### drop-batch-size (`u32`)

#### memory-limit (`u32`)

#### ce-threshold-selector (`u8`)

#### ce-threshold-mask (`u8`)

### [fq-pie-attrs](#id306)

#### limit (`u32`)

#### flows (`u32`)

#### target (`u32`)

#### tupdate (`u32`)

#### alpha (`u32`)

#### beta (`u32`)

#### quantum (`u32`)

#### memory-limit (`u32`)

#### ecn-prob (`u32`)

#### ecn (`u32`)

#### bytemode (`u32`)

#### dq-rate-estimator (`u32`)

### [netem-attrs](#id307)

#### corr (`binary`)

struct:
:   [tc-netem-corr](#tc-definition-tc-netem-corr)

#### delay-dist (`binary`)

sub-type:
:   s16

#### reorder (`binary`)

struct:
:   [tc-netem-reorder](#tc-definition-tc-netem-reorder)

#### corrupt (`binary`)

struct:
:   [tc-netem-corrupt](#tc-definition-tc-netem-corrupt)

#### loss (`nest`)

nested-attributes:
:   [netem-loss-attrs](#tc-attribute-set-netem-loss-attrs)

#### rate (`binary`)

struct:
:   [tc-netem-rate](#tc-definition-tc-netem-rate)

#### ecn (`u32`)

#### rate64 (`u64`)

#### pad (`u32`)

#### latency64 (`s64`)

#### jitter64 (`s64`)

#### slot (`binary`)

struct:
:   [tc-netem-slot](#tc-definition-tc-netem-slot)

#### slot-dist (`binary`)

sub-type:
:   s16

#### prng-seed (`u64`)

### [netem-loss-attrs](#id308)

#### gi (`binary`)

doc:
:   General Intuitive - 4 state model

struct:
:   [tc-netem-gimodel](#tc-definition-tc-netem-gimodel)

#### ge (`binary`)

doc:
:   Gilbert Elliot models

struct:
:   [tc-netem-gemodel](#tc-definition-tc-netem-gemodel)

### [pie-attrs](#id309)

#### target (`u32`)

#### limit (`u32`)

#### tupdate (`u32`)

#### alpha (`u32`)

#### beta (`u32`)

#### ecn (`u32`)

#### bytemode (`u32`)

#### dq-rate-estimator (`u32`)

### [police-attrs](#id310)

#### tbf (`binary`)

struct:
:   [tc-police](#tc-definition-tc-police)

#### rate (`binary`)

#### peakrate (`binary`)

#### avrate (`u32`)

#### result (`u32`)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### pad (`pad`)

#### rate64 (`u64`)

#### peakrate64 (`u64`)

#### pktrate64 (`u64`)

#### pktburst64 (`u64`)

### [qfq-attrs](#id311)

#### weight (`u32`)

#### lmax (`u32`)

### [red-attrs](#id312)

#### parms (`binary`)

struct:
:   [tc-red-qopt](#tc-definition-tc-red-qopt)

#### stab (`binary`)

#### max-p (`u32`)

#### flags (`bitfield32`)

#### early-drop-block (`u32`)

#### mark-block (`u32`)

### [route-attrs](#id313)

#### classid (`u32`)

#### to (`u32`)

#### from (`u32`)

#### iif (`u32`)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

### [taprio-attrs](#id314)

#### priomap (`binary`)

struct:
:   [tc-mqprio-qopt](#tc-definition-tc-mqprio-qopt)

#### sched-entry-list (`nest`)

nested-attributes:
:   [taprio-sched-entry-list](#tc-attribute-set-taprio-sched-entry-list)

#### sched-base-time (`s64`)

#### sched-single-entry (`nest`)

nested-attributes:
:   [taprio-sched-entry](#tc-attribute-set-taprio-sched-entry)

#### sched-clockid (`s32`)

#### pad (`pad`)

#### admin-sched (`binary`)

#### sched-cycle-time (`s64`)

#### sched-cycle-time-extension (`s64`)

#### flags (`u32`)

#### txtime-delay (`u32`)

#### tc-entry (`nest`)

nested-attributes:
:   [taprio-tc-entry-attrs](#tc-attribute-set-taprio-tc-entry-attrs)

### [taprio-sched-entry-list](#id315)

#### entry (`nest`)

nested-attributes:
:   [taprio-sched-entry](#tc-attribute-set-taprio-sched-entry)

multi-attr:
:   True

### [taprio-sched-entry](#id316)

#### index (`u32`)

#### cmd (`u8`)

#### gate-mask (`u32`)

#### interval (`u32`)

### [taprio-tc-entry-attrs](#id317)

#### index (`u32`)

#### max-sdu (`u32`)

#### fp (`u32`)

### [tbf-attrs](#id318)

#### parms (`binary`)

struct:
:   [tc-tbf-qopt](#tc-definition-tc-tbf-qopt)

#### rtab (`binary`)

#### ptab (`binary`)

#### rate64 (`u64`)

#### prate64 (`u64`)

#### burst (`u32`)

#### pburst (`u32`)

#### pad (`pad`)

### [act-sample-attrs](#id319)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

struct:
:   [tc-gact](#tc-definition-tc-gact)

#### rate (`u32`)

#### trunc-size (`u32`)

#### psample-group (`u32`)

#### pad (`pad`)

### [act-gact-attrs](#id320)

#### tm (`binary`)

struct:
:   [tcf-t](#tc-definition-tcf-t)

#### parms (`binary`)

struct:
:   [tc-gact](#tc-definition-tc-gact)

#### prob (`binary`)

struct:
:   [tc-gact-p](#tc-definition-tc-gact-p)

#### pad (`pad`)

### [tca-stab-attrs](#id321)

#### base (`binary`)

struct:
:   [tc-sizespec](#tc-definition-tc-sizespec)

#### data (`binary`)

### [tca-stats-attrs](#id322)

#### basic (`binary`)

struct:
:   [gnet-stats-basic](#tc-definition-gnet-stats-basic)

#### rate-est (`binary`)

struct:
:   [gnet-stats-rate-est](#tc-definition-gnet-stats-rate-est)

#### queue (`binary`)

struct:
:   [gnet-stats-queue](#tc-definition-gnet-stats-queue)

#### app (`sub-message`)

sub-message:
:   [tca-stats-app-msg](#tc-sub-message-tca-stats-app-msg)

selector:
:   kind

#### rate-est64 (`binary`)

struct:
:   [gnet-stats-rate-est64](#tc-definition-gnet-stats-rate-est64)

#### pad (`pad`)

#### basic-hw (`binary`)

struct:
:   [gnet-stats-basic](#tc-definition-gnet-stats-basic)

#### pkt64 (`u64`)

### [u32-attrs](#id323)

#### classid (`u32`)

#### hash (`u32`)

#### link (`u32`)

#### divisor (`u32`)

#### sel (`binary`)

struct:
:   [tc-u32-sel](#tc-definition-tc-u32-sel)

#### police (`nest`)

nested-attributes:
:   [police-attrs](#tc-attribute-set-police-attrs)

#### act (`indexed-array`)

sub-type:
:   nest

nested-attributes:
:   [act-attrs](#tc-attribute-set-act-attrs)

#### indev (`string`)

#### pcnt (`binary`)

struct:
:   [tc-u32-pcnt](#tc-definition-tc-u32-pcnt)

#### mark (`binary`)

struct:
:   [tc-u32-mark](#tc-definition-tc-u32-mark)

#### flags (`u32`)

#### pad (`pad`)

## [Sub-messages](#id324)

### [options-msg](#id325)

* **basic**
  :   attribute-set:
      :   [basic-attrs](#tc-attribute-set-basic-attrs)
* **bpf**
  :   attribute-set:
      :   [bpf-attrs](#tc-attribute-set-bpf-attrs)
* **bfifo**
  :   fixed-header:
      :   [tc-fifo-qopt](#tc-definition-tc-fifo-qopt)
* **cake**
  :   attribute-set:
      :   [cake-attrs](#tc-attribute-set-cake-attrs)
* **cbs**
  :   attribute-set:
      :   [cbs-attrs](#tc-attribute-set-cbs-attrs)
* **cgroup**
  :   attribute-set:
      :   [cgroup-attrs](#tc-attribute-set-cgroup-attrs)
* **choke**
  :   attribute-set:
      :   [choke-attrs](#tc-attribute-set-choke-attrs)
* **clsact**
* **codel**
  :   attribute-set:
      :   [codel-attrs](#tc-attribute-set-codel-attrs)
* **drr**
  :   attribute-set:
      :   [drr-attrs](#tc-attribute-set-drr-attrs)
* **dualpi2**
  :   attribute-set:
      :   [dualpi2-attrs](#tc-attribute-set-dualpi2-attrs)
* **etf**
  :   attribute-set:
      :   [etf-attrs](#tc-attribute-set-etf-attrs)
* **ets**
  :   attribute-set:
      :   [ets-attrs](#tc-attribute-set-ets-attrs)
* **flow**
  :   attribute-set:
      :   [flow-attrs](#tc-attribute-set-flow-attrs)
* **flower**
  :   attribute-set:
      :   [flower-attrs](#tc-attribute-set-flower-attrs)
* **fq**
  :   attribute-set:
      :   [fq-attrs](#tc-attribute-set-fq-attrs)
* **fq\_codel**
  :   attribute-set:
      :   [fq-codel-attrs](#tc-attribute-set-fq-codel-attrs)
* **fq\_pie**
  :   attribute-set:
      :   [fq-pie-attrs](#tc-attribute-set-fq-pie-attrs)
* **fw**
  :   attribute-set:
      :   [fw-attrs](#tc-attribute-set-fw-attrs)
* **gred**
  :   attribute-set:
      :   [gred-attrs](#tc-attribute-set-gred-attrs)
* **hfsc**
  :   fixed-header:
      :   [tc-hfsc-qopt](#tc-definition-tc-hfsc-qopt)
* **hhf**
  :   attribute-set:
      :   [hhf-attrs](#tc-attribute-set-hhf-attrs)
* **htb**
  :   attribute-set:
      :   [htb-attrs](#tc-attribute-set-htb-attrs)
* **ingress**
* **matchall**
  :   attribute-set:
      :   [matchall-attrs](#tc-attribute-set-matchall-attrs)
* **mq**
* **mqprio**
  :   fixed-header:
      :   [tc-mqprio-qopt](#tc-definition-tc-mqprio-qopt)
* **multiq**
  :   fixed-header:
      :   [tc-multiq-qopt](#tc-definition-tc-multiq-qopt)
* **netem**
  :   fixed-header:
      :   [tc-netem-qopt](#tc-definition-tc-netem-qopt)

      attribute-set:
      :   [netem-attrs](#tc-attribute-set-netem-attrs)
* **pfifo**
  :   fixed-header:
      :   [tc-fifo-qopt](#tc-definition-tc-fifo-qopt)
* **pfifo\_fast**
  :   fixed-header:
      :   [tc-prio-qopt](#tc-definition-tc-prio-qopt)
* **pfifo\_head\_drop**
  :   fixed-header:
      :   [tc-fifo-qopt](#tc-definition-tc-fifo-qopt)
* **pie**
  :   attribute-set:
      :   [pie-attrs](#tc-attribute-set-pie-attrs)
* **plug**
  :   fixed-header:
      :   [tc-plug-qopt](#tc-definition-tc-plug-qopt)
* **prio**
  :   fixed-header:
      :   [tc-prio-qopt](#tc-definition-tc-prio-qopt)
* **qfq**
  :   attribute-set:
      :   [qfq-attrs](#tc-attribute-set-qfq-attrs)
* **red**
  :   attribute-set:
      :   [red-attrs](#tc-attribute-set-red-attrs)
* **route**
  :   attribute-set:
      :   [route-attrs](#tc-attribute-set-route-attrs)
* **sfb**
  :   fixed-header:
      :   [tc-sfb-qopt](#tc-definition-tc-sfb-qopt)
* **sfq**
  :   fixed-header:
      :   [tc-sfq-qopt-v1](#tc-definition-tc-sfq-qopt-v1)
* **taprio**
  :   attribute-set:
      :   [taprio-attrs](#tc-attribute-set-taprio-attrs)
* **tbf**
  :   attribute-set:
      :   [tbf-attrs](#tc-attribute-set-tbf-attrs)
* **u32**
  :   attribute-set:
      :   [u32-attrs](#tc-attribute-set-u32-attrs)

### [act-options-msg](#id326)

* **bpf**
  :   attribute-set:
      :   [act-bpf-attrs](#tc-attribute-set-act-bpf-attrs)
* **connmark**
  :   attribute-set:
      :   [act-connmark-attrs](#tc-attribute-set-act-connmark-attrs)
* **csum**
  :   attribute-set:
      :   [act-csum-attrs](#tc-attribute-set-act-csum-attrs)
* **ct**
  :   attribute-set:
      :   [act-ct-attrs](#tc-attribute-set-act-ct-attrs)
* **ctinfo**
  :   attribute-set:
      :   [act-ctinfo-attrs](#tc-attribute-set-act-ctinfo-attrs)
* **gact**
  :   attribute-set:
      :   [act-gact-attrs](#tc-attribute-set-act-gact-attrs)
* **gate**
  :   attribute-set:
      :   [act-gate-attrs](#tc-attribute-set-act-gate-attrs)
* **ife**
  :   attribute-set:
      :   [act-ife-attrs](#tc-attribute-set-act-ife-attrs)
* **mirred**
  :   attribute-set:
      :   [act-mirred-attrs](#tc-attribute-set-act-mirred-attrs)
* **mpls**
  :   attribute-set:
      :   [act-mpls-attrs](#tc-attribute-set-act-mpls-attrs)
* **nat**
  :   attribute-set:
      :   [act-nat-attrs](#tc-attribute-set-act-nat-attrs)
* **pedit**
  :   attribute-set:
      :   [act-pedit-attrs](#tc-attribute-set-act-pedit-attrs)
* **police**
  :   attribute-set:
      :   [police-attrs](#tc-attribute-set-police-attrs)
* **sample**
  :   attribute-set:
      :   [act-sample-attrs](#tc-attribute-set-act-sample-attrs)
* **simple**
  :   attribute-set:
      :   [act-simple-attrs](#tc-attribute-set-act-simple-attrs)
* **skbedit**
  :   attribute-set:
      :   [act-skbedit-attrs](#tc-attribute-set-act-skbedit-attrs)
* **skbmod**
  :   attribute-set:
      :   [act-skbmod-attrs](#tc-attribute-set-act-skbmod-attrs)
* **tunnel\_key**
  :   attribute-set:
      :   [act-tunnel-key-attrs](#tc-attribute-set-act-tunnel-key-attrs)
* **vlan**
  :   attribute-set:
      :   [act-vlan-attrs](#tc-attribute-set-act-vlan-attrs)

### [tca-stats-app-msg](#id327)

* **cake**
  :   attribute-set:
      :   [cake-stats-attrs](#tc-attribute-set-cake-stats-attrs)
* **choke**
  :   fixed-header:
      :   [tc-choke-xstats](#tc-definition-tc-choke-xstats)
* **codel**
  :   fixed-header:
      :   [tc-codel-xstats](#tc-definition-tc-codel-xstats)
* **dualpi2**
  :   fixed-header:
      :   [tc-dualpi2-xstats](#tc-definition-tc-dualpi2-xstats)
* **fq**
  :   fixed-header:
      :   [tc-fq-qd-stats](#tc-definition-tc-fq-qd-stats)
* **fq\_codel**
  :   fixed-header:
      :   [tc-fq-codel-xstats](#tc-definition-tc-fq-codel-xstats)
* **fq\_pie**
  :   fixed-header:
      :   [tc-fq-pie-xstats](#tc-definition-tc-fq-pie-xstats)
* **hhf**
  :   fixed-header:
      :   [tc-hhf-xstats](#tc-definition-tc-hhf-xstats)
* **pie**
  :   fixed-header:
      :   [tc-pie-xstats](#tc-definition-tc-pie-xstats)
* **red**
  :   fixed-header:
      :   [tc-red-xstats](#tc-definition-tc-red-xstats)
* **sfb**
  :   fixed-header:
      :   [tc-sfb-xstats](#tc-definition-tc-sfb-xstats)
* **sfq**
  :   fixed-header:
      :   [tc-sfq-xstats](#tc-definition-tc-sfq-xstats)
