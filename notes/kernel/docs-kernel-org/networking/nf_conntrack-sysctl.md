# Netfilter Conntrack Sysfs variables

> 출처(원문): https://docs.kernel.org/networking/nf_conntrack-sysctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Netfilter Conntrack Sysfs variables

## /proc/sys/net/netfilter/nf\_conntrack\_\* Variables:

nf\_conntrack\_acct - BOOLEAN
:   * 0 - disabled (default)
    * not 0 - enabled

    Enable connection tracking flow accounting. 64-bit byte and packet
    counters per flow are added.

nf\_conntrack\_buckets - INTEGER
:   Size of hash table. If not specified as parameter during module
    loading, the default size is calculated by dividing total memory
    by 16384 to determine the number of buckets. The hash table will
    never have fewer than 1024 and never more than 262144 buckets.
    This sysctl is only writeable in the initial net namespace.

nf\_conntrack\_checksum - BOOLEAN
:   * 0 - disabled
    * not 0 - enabled (default)

    Verify checksum of incoming packets. Packets with bad checksums are
    in INVALID state. If this is enabled, such packets will not be
    considered for connection tracking.

nf\_conntrack\_count - INTEGER (read-only)
:   Number of currently allocated flow entries.

nf\_conntrack\_events - BOOLEAN
:   * 0 - disabled
    * 1 - enabled
    * 2 - auto (default)

    If this option is enabled, the connection tracking code will
    provide userspace with connection tracking events via ctnetlink.
    The default allocates the extension if a userspace program is
    listening to ctnetlink events.

nf\_conntrack\_expect\_max - INTEGER
:   Maximum size of expectation table. Default value is
    nf\_conntrack\_buckets / 256. Minimum is 1.

nf\_conntrack\_frag6\_high\_thresh - INTEGER
:   default 262144

    Maximum memory used to reassemble IPv6 fragments. When
    nf\_conntrack\_frag6\_high\_thresh bytes of memory is allocated for this
    purpose, the fragment handler will toss packets until
    nf\_conntrack\_frag6\_low\_thresh is reached.

nf\_conntrack\_frag6\_low\_thresh - INTEGER
:   default 196608

    See nf\_conntrack\_frag6\_low\_thresh

nf\_conntrack\_frag6\_timeout - INTEGER (seconds)
:   default 60

    Time to keep an IPv6 fragment in memory.

nf\_conntrack\_generic\_timeout - INTEGER (seconds)
:   default 600

    Default for generic timeout. This refers to layer 4 unknown/unsupported
    protocols.

nf\_conntrack\_icmp\_timeout - INTEGER (seconds)
:   default 30

    Default for ICMP timeout.

nf\_conntrack\_icmpv6\_timeout - INTEGER (seconds)
:   default 30

    Default for ICMP6 timeout.

nf\_conntrack\_log\_invalid - INTEGER
:   * 0 - disable (default)
    * 1 - log ICMP packets
    * 6 - log TCP packets
    * 17 - log UDP packets
    * 41 - log ICMPv6 packets
    * 136 - log UDPLITE packets
    * 255 - log packets of any protocol

    Log invalid packets of a type specified by value.

nf\_conntrack\_max - INTEGER
:   Maximum number of allowed connection tracking entries. This value is set
    to nf\_conntrack\_buckets by default.
    Note that connection tracking entries are added to the table twice -- once
    for the original direction and once for the reply direction (i.e., with
    the reversed address). This means that with default settings a maxed-out
    table will have a average hash chain length of 2, not 1.

nf\_conntrack\_tcp\_be\_liberal - BOOLEAN
:   * 0 - disabled (default)
    * not 0 - enabled

    Be conservative in what you do, be liberal in what you accept from others.
    If it’s non-zero, we mark only out of window RST segments as INVALID.

nf\_conntrack\_tcp\_ignore\_invalid\_rst - BOOLEAN
:   * 0 - disabled (default)
    * 1 - enabled

    If it’s 1, we don’t mark out of window RST segments as INVALID.

nf\_conntrack\_tcp\_loose - BOOLEAN
:   * 0 - disabled
    * not 0 - enabled (default)

    If it is set to zero, we disable picking up already established
    connections.

nf\_conntrack\_tcp\_max\_retrans - INTEGER
:   default 3

    Maximum number of packets that can be retransmitted without
    received an (acceptable) ACK from the destination. If this number
    is reached, a shorter timer will be started.

nf\_conntrack\_tcp\_timeout\_close - INTEGER (seconds)
:   default 10

nf\_conntrack\_tcp\_timeout\_close\_wait - INTEGER (seconds)
:   default 60

nf\_conntrack\_tcp\_timeout\_established - INTEGER (seconds)
:   default 432000 (5 days)

nf\_conntrack\_tcp\_timeout\_fin\_wait - INTEGER (seconds)
:   default 120

nf\_conntrack\_tcp\_timeout\_last\_ack - INTEGER (seconds)
:   default 30

nf\_conntrack\_tcp\_timeout\_max\_retrans - INTEGER (seconds)
:   default 300

nf\_conntrack\_tcp\_timeout\_syn\_recv - INTEGER (seconds)
:   default 60

nf\_conntrack\_tcp\_timeout\_syn\_sent - INTEGER (seconds)
:   default 120

nf\_conntrack\_tcp\_timeout\_time\_wait - INTEGER (seconds)
:   default 120

nf\_conntrack\_tcp\_timeout\_unacknowledged - INTEGER (seconds)
:   default 300

nf\_conntrack\_timestamp - BOOLEAN
:   * 0 - disabled (default)
    * not 0 - enabled

    Enable connection tracking flow timestamping.

nf\_conntrack\_sctp\_timeout\_closed - INTEGER (seconds)
:   default 10

nf\_conntrack\_sctp\_timeout\_cookie\_wait - INTEGER (seconds)
:   default 3

nf\_conntrack\_sctp\_timeout\_cookie\_echoed - INTEGER (seconds)
:   default 3

nf\_conntrack\_sctp\_timeout\_established - INTEGER (seconds)
:   default 210

    Default is set to (hb\_interval \* path\_max\_retrans + rto\_max)

nf\_conntrack\_sctp\_timeout\_shutdown\_sent - INTEGER (seconds)
:   default 3

nf\_conntrack\_sctp\_timeout\_shutdown\_recd - INTEGER (seconds)
:   default 3

nf\_conntrack\_sctp\_timeout\_shutdown\_ack\_sent - INTEGER (seconds)
:   default 3

nf\_conntrack\_sctp\_timeout\_heartbeat\_sent - INTEGER (seconds)
:   default 30

    This timeout is used to setup conntrack entry on secondary paths.
    Default is set to hb\_interval.

nf\_conntrack\_udp\_timeout - INTEGER (seconds)
:   default 30

nf\_conntrack\_udp\_timeout\_stream - INTEGER (seconds)
:   default 120

    This extended timeout will be used in case there is an UDP stream
    detected.

nf\_conntrack\_gre\_timeout - INTEGER (seconds)
:   default 30

nf\_conntrack\_gre\_timeout\_stream - INTEGER (seconds)
:   default 180

    This extended timeout will be used in case there is an GRE stream
    detected.

nf\_hooks\_lwtunnel - BOOLEAN
:   * 0 - disabled (default)
    * not 0 - enabled

    If this option is enabled, the lightweight tunnel netfilter hooks are
    enabled. This option cannot be disabled once it is enabled.

nf\_flowtable\_tcp\_timeout - INTEGER (seconds)
:   default 30

    Control offload timeout for tcp connections.
    TCP connections may be offloaded from nf conntrack to nf flow table.
    Once aged, the connection is returned to nf conntrack.

nf\_flowtable\_udp\_timeout - INTEGER (seconds)
:   default 30

    Control offload timeout for udp connections.
    UDP connections may be offloaded from nf conntrack to nf flow table.
    Once aged, the connection is returned to nf conntrack.
