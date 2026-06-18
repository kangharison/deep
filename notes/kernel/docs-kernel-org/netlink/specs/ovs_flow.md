# Familyovs_flownetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/ovs_flow.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `ovs_flow` netlink specification](#id3)

## [Summary](#id4)

OVS flow configuration over generic netlink.

## [Operations](#id5)

### [get](#id6)

Get / dump OVS flow configuration and state

value:
:   3

attribute-set:
:   [flow-attrs](#ovs-flow-attribute-set-flow-attrs)

do:
:   **request**
    :   attributes:
        :   [`key`, `ufid`, `ufid-flags`]

    **reply**
    :   attributes:
        :   [`key`, `ufid`, `mask`, `stats`, `actions`]

dump:
:   **request**
    :   attributes:
        :   [`key`, `ufid`, `ufid-flags`]

    **reply**
    :   attributes:
        :   [`key`, `ufid`, `mask`, `stats`, `actions`]

### [new](#id7)

Create OVS flow configuration in a data path

value:
:   1

attribute-set:
:   [flow-attrs](#ovs-flow-attribute-set-flow-attrs)

do:
:   **request**
    :   attributes:
        :   [`key`, `ufid`, `mask`, `actions`]

## [Multicast groups](#id8)

* ovs\_flow

## [Definitions](#id9)

### [ovs-header](#id10)

type:
:   struct

doc:
:   Header for OVS Generic Netlink messages.

members:
:   dp-ifindex (`u32`):
    :   ifindex of local port for datapath (0 to make a request not specific to a datapath).

### [ovs-flow-stats](#id11)

type:
:   struct

members:
:   n-packets (`u64`):
    :   Number of matched packets.

    n-bytes (`u64`):
    :   Number of matched bytes.

### [ovs-key-ethernet](#id12)

type:
:   struct

members:
:   eth-src (`binary`):


    eth-dst (`binary`):

### [ovs-key-mpls](#id13)

type:
:   struct

members:
:   mpls-lse (`u32`):

### [ovs-key-ipv4](#id14)

type:
:   struct

members:
:   ipv4-src (`u32`):


    ipv4-dst (`u32`):


    ipv4-proto (`u8`):


    ipv4-tos (`u8`):


    ipv4-ttl (`u8`):


    ipv4-frag (`u8`):

### [ovs-key-ipv6](#id15)

type:
:   struct

members:
:   ipv6-src (`binary`):


    ipv6-dst (`binary`):


    ipv6-label (`u32`):


    ipv6-proto (`u8`):


    ipv6-tclass (`u8`):


    ipv6-hlimit (`u8`):


    ipv6-frag (`u8`):

### [ovs-key-ipv6-exthdrs](#id16)

type:
:   struct

members:
:   hdrs (`u16`):

### [ovs-frag-type](#id17)

name-prefix:
:   ovs-frag-type-

enum-name:
:   ovs-frag-type

type:
:   enum

entries:
:   none:
    :   Packet is not a fragment.

    first:
    :   Packet is a fragment with offset 0.

    later:
    :   Packet is a fragment with nonzero offset.

    any:

### [ovs-key-tcp](#id18)

type:
:   struct

members:
:   tcp-src (`u16`):


    tcp-dst (`u16`):

### [ovs-key-udp](#id19)

type:
:   struct

members:
:   udp-src (`u16`):


    udp-dst (`u16`):

### [ovs-key-sctp](#id20)

type:
:   struct

members:
:   sctp-src (`u16`):


    sctp-dst (`u16`):

### [ovs-key-icmp](#id21)

type:
:   struct

members:
:   icmp-type (`u8`):


    icmp-code (`u8`):

### [ovs-key-arp](#id22)

type:
:   struct

members:
:   arp-sip (`u32`):


    arp-tip (`u32`):


    arp-op (`u16`):


    arp-sha (`binary`):


    arp-tha (`binary`):

### [ovs-key-nd](#id23)

type:
:   struct

members:
:   nd-target (`binary`):


    nd-sll (`binary`):


    nd-tll (`binary`):

### [ovs-key-ct-tuple-ipv4](#id24)

type:
:   struct

members:
:   ipv4-src (`u32`):


    ipv4-dst (`u32`):


    src-port (`u16`):


    dst-port (`u16`):


    ipv4-proto (`u8`):

### [ovs-action-push-vlan](#id25)

type:
:   struct

members:
:   vlan-tpid (`u16`):
    :   Tag protocol identifier (TPID) to push.

    vlan-tci (`u16`):
    :   Tag control identifier (TCI) to push.

### [ovs-ufid-flags](#id26)

name-prefix:
:   ovs-ufid-f-

enum-name:
:   None

type:
:   flags

entries:
:   * `omit-key`
    * `omit-mask`
    * `omit-actions`

### [ovs-action-hash](#id27)

type:
:   struct

members:
:   hash-alg (`u32`):
    :   Algorithm used to compute hash prior to recirculation.

    hash-basis (`u32`):
    :   Basis used for computing hash.

### [ovs-hash-alg](#id28)

enum-name:
:   ovs-hash-alg

type:
:   enum

doc:
:   Data path hash algorithm for computing Datapath hash. The algorithm type only specifies the fields in a flow will be used as part of the hash. Each datapath is free to use its own hash algorithm. The hash value will be opaque to the user space daemon.

entries:
:   * `ovs-hash-alg-l4`

### [ovs-action-push-mpls](#id29)

type:
:   struct

members:
:   mpls-lse (`u32`):
    :   MPLS label stack entry to push

    mpls-ethertype (`u32`):
    :   Ethertype to set in the encapsulating ethernet frame. The only values ethertype should ever be given are ETH\_P\_MPLS\_UC and ETH\_P\_MPLS\_MC, indicating MPLS unicast or multicast. Other are rejected.

### [ovs-action-add-mpls](#id30)

type:
:   struct

members:
:   mpls-lse (`u32`):
    :   MPLS label stack entry to push

    mpls-ethertype (`u32`):
    :   Ethertype to set in the encapsulating ethernet frame. The only values ethertype should ever be given are ETH\_P\_MPLS\_UC and ETH\_P\_MPLS\_MC, indicating MPLS unicast or multicast. Other are rejected.

    tun-flags (`u16`):
    :   MPLS tunnel attributes.

### [ct-state-flags](#id31)

enum-name:
:   None

type:
:   flags

name-prefix:
:   ovs-cs-f-

entries:
:   new:
    :   Beginning of a new connection.

    established:
    :   Part of an existing connenction

    related:
    :   Related to an existing connection.

    reply-dir:
    :   Flow is in the reply direction.

    invalid:
    :   Could not track the connection.

    tracked:
    :   Conntrack has occurred.

    src-nat:
    :   Packet’s source address/port was mangled by NAT.

    dst-nat:
    :   Packet’s destination address/port was mangled by NAT.

## [Attribute sets](#id32)

### [flow-attrs](#id33)

#### key (`nest`)

nested-attributes:
:   [key-attrs](#ovs-flow-attribute-set-key-attrs)

doc:
:   Nested attributes specifying the flow key. Always present in notifications. Required for all requests (except dumps).

#### actions (`nest`)

nested-attributes:
:   [action-attrs](#ovs-flow-attribute-set-action-attrs)

doc:
:   Nested attributes specifying the actions to take for packets that match the key. Always present in notifications. Required for OVS\_FLOW\_CMD\_NEW requests, optional for OVS\_FLOW\_CMD\_SET requests. An OVS\_FLOW\_CMD\_SET without OVS\_FLOW\_ATTR\_ACTIONS will not modify the actions. To clear the actions, an OVS\_FLOW\_ATTR\_ACTIONS without any nested attributes must be given.

#### stats (`binary`)

struct:
:   [ovs-flow-stats](#ovs-flow-definition-ovs-flow-stats)

doc:
:   Statistics for this flow. Present in notifications if the stats would be nonzero. Ignored in requests.

#### tcp-flags (`u8`)

doc:
:   An 8-bit value giving the ORed value of all of the TCP flags seen on packets in this flow. Only present in notifications for TCP flows, and only if it would be nonzero. Ignored in requests.

#### used (`u64`)

doc:
:   A 64-bit integer giving the time, in milliseconds on the system monotonic clock, at which a packet was last processed for this flow. Only present in notifications if a packet has been processed for this flow. Ignored in requests.

#### clear (`flag`)

doc:
:   If present in a OVS\_FLOW\_CMD\_SET request, clears the last-used time, accumulated TCP flags, and statistics for this flow. Otherwise ignored in requests. Never present in notifications.

#### mask (`nest`)

nested-attributes:
:   [key-attrs](#ovs-flow-attribute-set-key-attrs)

doc:
:   Nested attributes specifying the mask bits for wildcarded flow match. Mask bit value ‘1’ specifies exact match with corresponding flow key bit, while mask bit value ‘0’ specifies a wildcarded match. Omitting attribute is treated as wildcarding all corresponding fields. Optional for all requests. If not present, all flow key bits are exact match bits.

#### probe (`binary`)

doc:
:   Flow operation is a feature probe, error logging should be suppressed.

#### ufid (`binary`)

doc:
:   A value between 1-16 octets specifying a unique identifier for the flow. Causes the flow to be indexed by this value rather than the value of the OVS\_FLOW\_ATTR\_KEY attribute. Optional for all requests. Present in notifications if the flow was created with this attribute.

display-hint:
:   uuid

#### ufid-flags (`u32`)

enum:
:   [ovs-ufid-flags](#ovs-flow-definition-ovs-ufid-flags)

doc:
:   A 32-bit value of ORed flags that provide alternative semantics for flow installation and retrieval. Optional for all requests.

#### pad (`binary`)

### [key-attrs](#id34)

#### encap (`nest`)

nested-attributes:
:   [key-attrs](#ovs-flow-attribute-set-key-attrs)

#### priority (`u32`)

#### in-port (`u32`)

#### ethernet (`binary`)

struct:
:   [ovs-key-ethernet](#ovs-flow-definition-ovs-key-ethernet)

doc:
:   `struct ovs_key_ethernet`

#### vlan (`u16`)

byte-order:
:   big-endian

#### ethertype (`u16`)

byte-order:
:   big-endian

#### ipv4 (`binary`)

struct:
:   [ovs-key-ipv4](#ovs-flow-definition-ovs-key-ipv4)

#### ipv6 (`binary`)

struct:
:   [ovs-key-ipv6](#ovs-flow-definition-ovs-key-ipv6)

doc:
:   `struct ovs_key_ipv6`

#### tcp (`binary`)

struct:
:   [ovs-key-tcp](#ovs-flow-definition-ovs-key-tcp)

#### udp (`binary`)

struct:
:   [ovs-key-udp](#ovs-flow-definition-ovs-key-udp)

#### icmp (`binary`)

struct:
:   [ovs-key-icmp](#ovs-flow-definition-ovs-key-icmp)

#### icmpv6 (`binary`)

struct:
:   [ovs-key-icmp](#ovs-flow-definition-ovs-key-icmp)

#### arp (`binary`)

struct:
:   [ovs-key-arp](#ovs-flow-definition-ovs-key-arp)

doc:
:   `struct ovs_key_arp`

#### nd (`binary`)

struct:
:   [ovs-key-nd](#ovs-flow-definition-ovs-key-nd)

doc:
:   `struct ovs_key_nd`

#### skb-mark (`u32`)

#### tunnel (`nest`)

nested-attributes:
:   [tunnel-key-attrs](#ovs-flow-attribute-set-tunnel-key-attrs)

#### sctp (`binary`)

struct:
:   [ovs-key-sctp](#ovs-flow-definition-ovs-key-sctp)

#### tcp-flags (`u16`)

byte-order:
:   big-endian

#### dp-hash (`u32`)

doc:
:   Value 0 indicates the hash is not computed by the datapath.

#### recirc-id (`u32`)

#### mpls (`binary`)

struct:
:   [ovs-key-mpls](#ovs-flow-definition-ovs-key-mpls)

#### ct-state (`u32`)

enum:
:   [ct-state-flags](#ovs-flow-definition-ct-state-flags)

enum-as-flags:
:   True

#### ct-zone (`u16`)

doc:
:   connection tracking zone

#### ct-mark (`u32`)

doc:
:   connection tracking mark

#### ct-labels (`binary`)

display-hint:
:   hex

doc:
:   16-octet connection tracking label

#### ct-orig-tuple-ipv4 (`binary`)

struct:
:   [ovs-key-ct-tuple-ipv4](#ovs-flow-definition-ovs-key-ct-tuple-ipv4)

#### ct-orig-tuple-ipv6 (`binary`)

doc:
:   `struct ovs_key_ct_tuple_ipv6`

#### nsh (`nest`)

nested-attributes:
:   [ovs-nsh-key-attrs](#ovs-flow-attribute-set-ovs-nsh-key-attrs)

#### packet-type (`u32`)

byte-order:
:   big-endian

doc:
:   Should not be sent to the kernel

#### nd-extensions (`binary`)

doc:
:   Should not be sent to the kernel

#### tunnel-info (`binary`)

doc:
:   `struct ip_tunnel_info`

#### ipv6-exthdrs (`binary`)

struct:
:   [ovs-key-ipv6-exthdrs](#ovs-flow-definition-ovs-key-ipv6-exthdrs)

doc:
:   `struct ovs_key_ipv6_exthdr`

### [action-attrs](#id35)

#### output (`u32`)

doc:
:   ovs port number in datapath

#### userspace (`nest`)

nested-attributes:
:   [userspace-attrs](#ovs-flow-attribute-set-userspace-attrs)

#### set (`nest`)

nested-attributes:
:   [key-attrs](#ovs-flow-attribute-set-key-attrs)

doc:
:   Replaces the contents of an existing header. The single nested attribute specifies a header to modify and its value.

#### push-vlan (`binary`)

struct:
:   [ovs-action-push-vlan](#ovs-flow-definition-ovs-action-push-vlan)

doc:
:   Push a new outermost 802.1Q or 802.1ad header onto the packet.

#### pop-vlan (`flag`)

doc:
:   Pop the outermost 802.1Q or 802.1ad header from the packet.

#### sample (`nest`)

nested-attributes:
:   [sample-attrs](#ovs-flow-attribute-set-sample-attrs)

doc:
:   Probabilistically executes actions, as specified in the nested attributes.

#### recirc (`u32`)

doc:
:   recirc id

#### hash (`binary`)

struct:
:   [ovs-action-hash](#ovs-flow-definition-ovs-action-hash)

#### push-mpls (`binary`)

struct:
:   [ovs-action-push-mpls](#ovs-flow-definition-ovs-action-push-mpls)

doc:
:   Push a new MPLS label stack entry onto the top of the packets MPLS label stack. Set the ethertype of the encapsulating frame to either ETH\_P\_MPLS\_UC or ETH\_P\_MPLS\_MC to indicate the new packet contents.

#### pop-mpls (`u16`)

byte-order:
:   big-endian

doc:
:   ethertype

#### set-masked (`nest`)

nested-attributes:
:   [key-attrs](#ovs-flow-attribute-set-key-attrs)

doc:
:   Replaces the contents of an existing header. A nested attribute specifies a header to modify, its value, and a mask. For every bit set in the mask, the corresponding bit value is copied from the value to the packet header field, rest of the bits are left unchanged. The non-masked value bits must be passed in as zeroes. Masking is not supported for the OVS\_KEY\_ATTR\_TUNNEL attribute.

#### ct (`nest`)

nested-attributes:
:   [ct-attrs](#ovs-flow-attribute-set-ct-attrs)

doc:
:   Track the connection. Populate the conntrack-related entries in the flow key.

#### trunc (`u32`)

doc:
:   `struct ovs_action_trunc` is a u32 max length

#### push-eth (`binary`)

doc:
:   `struct ovs_action_push_eth`

#### pop-eth (`flag`)

#### ct-clear (`flag`)

#### push-nsh (`nest`)

nested-attributes:
:   [ovs-nsh-key-attrs](#ovs-flow-attribute-set-ovs-nsh-key-attrs)

doc:
:   Push NSH header to the packet.

#### pop-nsh (`flag`)

doc:
:   Pop the outermost NSH header off the packet.

#### meter (`u32`)

doc:
:   Run packet through a meter, which may drop the packet, or modify the packet (e.g., change the DSCP field)

#### clone (`nest`)

nested-attributes:
:   [action-attrs](#ovs-flow-attribute-set-action-attrs)

doc:
:   Make a copy of the packet and execute a list of actions without affecting the original packet and key.

#### check-pkt-len (`nest`)

nested-attributes:
:   [check-pkt-len-attrs](#ovs-flow-attribute-set-check-pkt-len-attrs)

doc:
:   Check the packet length and execute a set of actions if greater than the specified packet length, else execute another set of actions.

#### add-mpls (`binary`)

struct:
:   [ovs-action-add-mpls](#ovs-flow-definition-ovs-action-add-mpls)

doc:
:   Push a new MPLS label stack entry at the start of the packet or at the start of the l3 header depending on the value of l3 tunnel flag in the tun\_flags field of this OVS\_ACTION\_ATTR\_ADD\_MPLS argument.

#### dec-ttl (`nest`)

nested-attributes:
:   [dec-ttl-attrs](#ovs-flow-attribute-set-dec-ttl-attrs)

#### psample (`nest`)

nested-attributes:
:   [psample-attrs](#ovs-flow-attribute-set-psample-attrs)

doc:
:   Sends a packet sample to psample for external observation.

### [tunnel-key-attrs](#id36)

#### id (`u64`)

byte-order:
:   big-endian

value:
:   0

#### ipv4-src (`u32`)

byte-order:
:   big-endian

#### ipv4-dst (`u32`)

byte-order:
:   big-endian

#### tos (`u8`)

#### ttl (`u8`)

#### dont-fragment (`flag`)

#### csum (`flag`)

#### oam (`flag`)

#### geneve-opts (`binary`)

sub-type:
:   u32

#### tp-src (`u16`)

byte-order:
:   big-endian

#### tp-dst (`u16`)

byte-order:
:   big-endian

#### vxlan-opts (`nest`)

nested-attributes:
:   [vxlan-ext-attrs](#ovs-flow-attribute-set-vxlan-ext-attrs)

#### ipv6-src (`binary`)

doc:
:   `struct in6_addr` source IPv6 address

#### ipv6-dst (`binary`)

doc:
:   `struct in6_addr` destination IPv6 address

#### pad (`binary`)

#### erspan-opts (`binary`)

doc:
:   `struct erspan_metadata`

#### ipv4-info-bridge (`flag`)

### [check-pkt-len-attrs](#id37)

#### pkt-len (`u16`)

#### actions-if-greater (`nest`)

nested-attributes:
:   [action-attrs](#ovs-flow-attribute-set-action-attrs)

#### actions-if-less-equal (`nest`)

nested-attributes:
:   [action-attrs](#ovs-flow-attribute-set-action-attrs)

### [sample-attrs](#id38)

#### probability (`u32`)

#### actions (`nest`)

nested-attributes:
:   [action-attrs](#ovs-flow-attribute-set-action-attrs)

### [userspace-attrs](#id39)

#### pid (`u32`)

#### userdata (`binary`)

#### egress-tun-port (`u32`)

#### actions (`flag`)

### [ovs-nsh-key-attrs](#id40)

#### base (`binary`)

#### md1 (`binary`)

#### md2 (`binary`)

### [ct-attrs](#id41)

#### commit (`flag`)

#### zone (`u16`)

#### mark (`binary`)

#### labels (`binary`)

#### helper (`string`)

#### nat (`nest`)

nested-attributes:
:   [nat-attrs](#ovs-flow-attribute-set-nat-attrs)

#### force-commit (`flag`)

#### eventmask (`u32`)

#### timeout (`string`)

### [nat-attrs](#id42)

#### src (`flag`)

#### dst (`flag`)

#### ip-min (`binary`)

#### ip-max (`binary`)

#### proto-min (`u16`)

#### proto-max (`u16`)

#### persistent (`flag`)

#### proto-hash (`flag`)

#### proto-random (`flag`)

### [dec-ttl-attrs](#id43)

#### action (`nest`)

nested-attributes:
:   [action-attrs](#ovs-flow-attribute-set-action-attrs)

### [vxlan-ext-attrs](#id44)

#### gbp (`u32`)

### [psample-attrs](#id45)

#### group (`u32`)

#### cookie (`binary`)
