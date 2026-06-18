# Ethernet Bridging

> 출처(원문): https://docs.kernel.org/networking/bridge.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ethernet Bridging

## Introduction

The IEEE 802.1Q-2022 (Bridges and Bridged Networks) standard defines the
operation of bridges in computer networks. A bridge, in the context of this
standard, is a device that connects two or more network segments and operates
at the data link layer (Layer 2) of the OSI (Open Systems Interconnection)
model. The purpose of a bridge is to filter and forward frames between
different segments based on the destination MAC (Media Access Control) address.

## Bridge kAPI

Here are some core structures of bridge code. Note that the kAPI is *unstable*,
and can be changed at any time.

struct net\_bridge\_vlan
:   per-vlan entry

**Definition**:

```
struct net_bridge_vlan {
    struct rhash_head               vnode;
    struct rhash_head               tnode;
    u16 vid;
    u16 flags;
    u16 priv_flags;
    u8 state;
    struct pcpu_sw_netstats __percpu *stats;
    union {
        struct net_bridge       *br;
        struct net_bridge_port  *port;
    };
    union {
        refcount_t refcnt;
        struct net_bridge_vlan  *brvlan;
    };
    struct br_tunnel_info           tinfo;
    union {
        struct net_bridge_mcast         br_mcast_ctx;
        struct net_bridge_mcast_port    port_mcast_ctx;
    };
    u16 msti;
    struct list_head                vlist;
    struct rcu_head                 rcu;
};
```

**Members**

`vnode`
:   rhashtable member

`tnode`
:   rhashtable member

`vid`
:   VLAN id

`flags`
:   bridge vlan flags

`priv_flags`
:   private (in-kernel) bridge vlan flags

`state`
:   STP state (e.g. blocking, learning, forwarding)

`stats`
:   per-cpu VLAN statistics

`{unnamed_union}`
:   anonymous

`br`
:   if MASTER flag set, this points to a bridge struct

`port`
:   if MASTER flag unset, this points to a port struct

`{unnamed_union}`
:   anonymous

`refcnt`
:   if MASTER flag set, this is bumped for each port referencing it

`brvlan`
:   if MASTER flag unset, this points to the global per-VLAN context
    for this VLAN entry

`tinfo`
:   bridge tunnel info

`{unnamed_union}`
:   anonymous

`br_mcast_ctx`
:   if MASTER flag set, this is the global vlan multicast context

`port_mcast_ctx`
:   if MASTER flag unset, this is the per-port/vlan multicast
    context

`msti`
:   if MASTER flag set, this holds the VLANs MST instance

`vlist`
:   sorted list of VLAN entries

`rcu`
:   used for entry destruction

**Description**

This structure is shared between the global per-VLAN entries contained in
the bridge rhashtable and the local per-port per-VLAN entries contained in
the port’s rhashtable. The `union entries` should be interpreted depending on
the entry flags that are set.

## Bridge uAPI

Modern Linux bridge uAPI is accessed via Netlink interface. You can find
below files where the bridge and bridge port netlink attributes are defined.

### Bridge netlink attributes

Please *note* that the timer values in the following section are expected
in clock\_t format, which is seconds multiplied by USER\_HZ (generally
defined as 100).

**IFLA\_BR\_FORWARD\_DELAY**
:   The bridge forwarding delay is the time spent in LISTENING state
    (before moving to LEARNING) and in LEARNING state (before moving
    to FORWARDING). Only relevant if STP is enabled.

    The valid values are between (2 \* USER\_HZ) and (30 \* USER\_HZ).
    The default value is (15 \* USER\_HZ).

**IFLA\_BR\_HELLO\_TIME**
:   The time between hello packets sent by the bridge, when it is a root
    bridge or a designated bridge. Only relevant if STP is enabled.

    The valid values are between (1 \* USER\_HZ) and (10 \* USER\_HZ).
    The default value is (2 \* USER\_HZ).

**IFLA\_BR\_MAX\_AGE**
:   The hello packet timeout is the time until another bridge in the
    spanning tree is assumed to be dead, after reception of its last hello
    message. Only relevant if STP is enabled.

    The valid values are between (6 \* USER\_HZ) and (40 \* USER\_HZ).
    The default value is (20 \* USER\_HZ).

**IFLA\_BR\_AGEING\_TIME**
:   Configure the bridge’s FDB entries aging time. It is the time a MAC
    address will be kept in the FDB after a packet has been received from
    that address. After this time has passed, entries are cleaned up.
    Allow values outside the 802.1 standard specification for special cases:

    > * 0 - entry never ages (all permanent)
    > * 1 - entry disappears (no persistence)

    The default value is (300 \* USER\_HZ).

**IFLA\_BR\_STP\_STATE**
:   Turn spanning tree protocol on (*IFLA\_BR\_STP\_STATE* > 0) or off
    (*IFLA\_BR\_STP\_STATE* == 0) for this bridge.

    The default value is 0 (disabled).

**IFLA\_BR\_PRIORITY**
:   Set this bridge’s spanning tree priority, used during STP root bridge
    election.

    The valid values are between 0 and 65535.

**IFLA\_BR\_VLAN\_FILTERING**
:   Turn VLAN filtering on (*IFLA\_BR\_VLAN\_FILTERING* > 0) or off
    (*IFLA\_BR\_VLAN\_FILTERING* == 0). When disabled, the bridge will not
    consider the VLAN tag when handling packets.

    The default value is 0 (disabled).

**IFLA\_BR\_VLAN\_PROTOCOL**
:   Set the protocol used for VLAN filtering.

    The valid values are 0x8100(802.1Q) or 0x88A8(802.1AD). The default value
    is 0x8100(802.1Q).

**IFLA\_BR\_GROUP\_FWD\_MASK**
:   The group forwarding mask. This is the bitmask that is applied to
    decide whether to forward incoming frames destined to link-local
    addresses (of the form 01:80:C2:00:00:0X).

    The default value is 0, which means the bridge does not forward any
    link-local frames coming on this port.

**IFLA\_BR\_ROOT\_ID**
:   The bridge root id, read only.

**IFLA\_BR\_BRIDGE\_ID**
:   The bridge id, read only.

**IFLA\_BR\_ROOT\_PORT**
:   The bridge root port, read only.

**IFLA\_BR\_ROOT\_PATH\_COST**
:   The bridge root path cost, read only.

**IFLA\_BR\_TOPOLOGY\_CHANGE**
:   The bridge topology change, read only.

**IFLA\_BR\_TOPOLOGY\_CHANGE\_DETECTED**
:   The bridge topology change detected, read only.

**IFLA\_BR\_HELLO\_TIMER**
:   The bridge hello timer, read only.

**IFLA\_BR\_TCN\_TIMER**
:   The bridge tcn timer, read only.

**IFLA\_BR\_TOPOLOGY\_CHANGE\_TIMER**
:   The bridge topology change timer, read only.

**IFLA\_BR\_GC\_TIMER**
:   The bridge gc timer, read only.

**IFLA\_BR\_GROUP\_ADDR**
:   Set the MAC address of the multicast group this bridge uses for STP.
    The address must be a link-local address in standard Ethernet MAC address
    format. It is an address of the form 01:80:C2:00:00:0X, with X in [0, 4..f].

    The default value is 0.

**IFLA\_BR\_FDB\_FLUSH**
:   Flush bridge’s fdb dynamic entries.

**IFLA\_BR\_MCAST\_ROUTER**
:   Set bridge’s multicast router if IGMP snooping is enabled.
    The valid values are:

    > * 0 - disabled.
    > * 1 - automatic (queried).
    > * 2 - permanently enabled.

    The default value is 1.

**IFLA\_BR\_MCAST\_SNOOPING**
:   Turn multicast snooping on (*IFLA\_BR\_MCAST\_SNOOPING* > 0) or off
    (*IFLA\_BR\_MCAST\_SNOOPING* == 0).

    The default value is 1.

**IFLA\_BR\_MCAST\_QUERY\_USE\_IFADDR**
:   If enabled use the bridge’s own IP address as source address for IGMP
    queries (*IFLA\_BR\_MCAST\_QUERY\_USE\_IFADDR* > 0) or the default of 0.0.0.0
    (*IFLA\_BR\_MCAST\_QUERY\_USE\_IFADDR* == 0).

    The default value is 0 (disabled).

**IFLA\_BR\_MCAST\_QUERIER**
:   Enable (*IFLA\_BR\_MULTICAST\_QUERIER* > 0) or disable
    (*IFLA\_BR\_MULTICAST\_QUERIER* == 0) IGMP querier, ie sending of multicast
    queries by the bridge.

    The default value is 0 (disabled).

**IFLA\_BR\_MCAST\_HASH\_ELASTICITY**
:   Set multicast database hash elasticity, It is the maximum chain length in
    the multicast hash table. This attribute is *deprecated* and the value
    is always 16.

**IFLA\_BR\_MCAST\_HASH\_MAX**
:   Set maximum size of the multicast hash table

    The default value is 4096, the value must be a power of 2.

**IFLA\_BR\_MCAST\_LAST\_MEMBER\_CNT**
:   The Last Member Query Count is the number of Group-Specific Queries
    sent before the router assumes there are no local members. The Last
    Member Query Count is also the number of Group-and-Source-Specific
    Queries sent before the router assumes there are no listeners for a
    particular source.

    The default value is 2.

**IFLA\_BR\_MCAST\_STARTUP\_QUERY\_CNT**
:   The Startup Query Count is the number of Queries sent out on startup,
    separated by the Startup Query Interval.

    The default value is 2.

**IFLA\_BR\_MCAST\_LAST\_MEMBER\_INTVL**
:   The Last Member Query Interval is the Max Response Time inserted into
    Group-Specific Queries sent in response to Leave Group messages, and
    is also the amount of time between Group-Specific Query messages.

    The default value is (1 \* USER\_HZ).

**IFLA\_BR\_MCAST\_MEMBERSHIP\_INTVL**
:   The interval after which the bridge will leave a group, if no membership
    reports for this group are received.

    The default value is (260 \* USER\_HZ).

**IFLA\_BR\_MCAST\_QUERIER\_INTVL**
:   The interval between queries sent by other routers. if no queries are
    seen after this delay has passed, the bridge will start to send its own
    queries (as if *IFLA\_BR\_MCAST\_QUERIER\_INTVL* was enabled).

    The default value is (255 \* USER\_HZ).

**IFLA\_BR\_MCAST\_QUERY\_INTVL**
:   The Query Interval is the interval between General Queries sent by
    the Querier.

    The default value is (125 \* USER\_HZ). The minimum value is (1 \* USER\_HZ).

**IFLA\_BR\_MCAST\_QUERY\_RESPONSE\_INTVL**
:   The Max Response Time used to calculate the Max Resp Code inserted
    into the periodic General Queries.

    The default value is (10 \* USER\_HZ).

**IFLA\_BR\_MCAST\_STARTUP\_QUERY\_INTVL**
:   The interval between queries in the startup phase.

    The default value is (125 \* USER\_HZ) / 4. The minimum value is (1 \* USER\_HZ).

**IFLA\_BR\_NF\_CALL\_IPTABLES**
:   Enable (*NF\_CALL\_IPTABLES* > 0) or disable (*NF\_CALL\_IPTABLES* == 0)
    iptables hooks on the bridge.

    The default value is 0 (disabled).

**IFLA\_BR\_NF\_CALL\_IP6TABLES**
:   Enable (*NF\_CALL\_IP6TABLES* > 0) or disable (*NF\_CALL\_IP6TABLES* == 0)
    ip6tables hooks on the bridge.

    The default value is 0 (disabled).

**IFLA\_BR\_NF\_CALL\_ARPTABLES**
:   Enable (*NF\_CALL\_ARPTABLES* > 0) or disable (*NF\_CALL\_ARPTABLES* == 0)
    arptables hooks on the bridge.

    The default value is 0 (disabled).

**IFLA\_BR\_VLAN\_DEFAULT\_PVID**
:   VLAN ID applied to untagged and priority-tagged incoming packets.

    The default value is 1. Setting to the special value 0 makes all ports of
    this bridge not have a PVID by default, which means that they will
    not accept VLAN-untagged traffic.

**IFLA\_BR\_PAD**
:   Bridge attribute padding type for netlink message.

**IFLA\_BR\_VLAN\_STATS\_ENABLED**
:   Enable (*IFLA\_BR\_VLAN\_STATS\_ENABLED* == 1) or disable
    (*IFLA\_BR\_VLAN\_STATS\_ENABLED* == 0) per-VLAN stats accounting.

    The default value is 0 (disabled).

**IFLA\_BR\_MCAST\_STATS\_ENABLED**
:   Enable (*IFLA\_BR\_MCAST\_STATS\_ENABLED* > 0) or disable
    (*IFLA\_BR\_MCAST\_STATS\_ENABLED* == 0) multicast (IGMP/MLD) stats
    accounting.

    The default value is 0 (disabled).

**IFLA\_BR\_MCAST\_IGMP\_VERSION**
:   Set the IGMP version.

    The valid values are 2 and 3. The default value is 2.

**IFLA\_BR\_MCAST\_MLD\_VERSION**
:   Set the MLD version.

    The valid values are 1 and 2. The default value is 1.

**IFLA\_BR\_VLAN\_STATS\_PER\_PORT**
:   Enable (*IFLA\_BR\_VLAN\_STATS\_PER\_PORT* == 1) or disable
    (*IFLA\_BR\_VLAN\_STATS\_PER\_PORT* == 0) per-VLAN per-port stats accounting.
    Can be changed only when there are no port VLANs configured.

    The default value is 0 (disabled).

**IFLA\_BR\_MULTI\_BOOLOPT**
:   The multi\_boolopt is used to control new boolean options to avoid adding
    new netlink attributes. You can look at `enum br_boolopt_id` for those
    options.

**IFLA\_BR\_MCAST\_QUERIER\_STATE**
:   Bridge mcast querier states, read only.

**IFLA\_BR\_FDB\_N\_LEARNED**
:   The number of dynamically learned FDB entries for the current bridge,
    read only.

**IFLA\_BR\_FDB\_MAX\_LEARNED**
:   Set the number of max dynamically learned FDB entries for the current
    bridge.

**IFLA\_BR\_STP\_MODE**
:   Set the STP mode for the bridge, which controls how the bridge
    selects between userspace and kernel STP. The valid values are
    documented below in the `BR_STP_MODE_*` constants.

### Bridge port netlink attributes

**IFLA\_BRPORT\_STATE**
:   The operation state of the port. Here are the valid values.

    > * 0 - port is in STP *DISABLED* state. Make this port completely
    >   inactive for STP. This is also called BPDU filter and could be used
    >   to disable STP on an untrusted port, like a leaf virtual device.
    >   The traffic forwarding is also stopped on this port.
    > * 1 - port is in STP *LISTENING* state. Only valid if STP is enabled
    >   on the bridge. In this state the port listens for STP BPDUs and
    >   drops all other traffic frames.
    > * 2 - port is in STP *LEARNING* state. Only valid if STP is enabled on
    >   the bridge. In this state the port will accept traffic only for the
    >   purpose of updating MAC address tables.
    > * 3 - port is in STP *FORWARDING* state. Port is fully active.
    > * 4 - port is in STP *BLOCKING* state. Only valid if STP is enabled on
    >   the bridge. This state is used during the STP election process.
    >   In this state, port will only process STP BPDUs.

**IFLA\_BRPORT\_PRIORITY**
:   The STP port priority. The valid values are between 0 and 255.

**IFLA\_BRPORT\_COST**
:   The STP path cost of the port. The valid values are between 1 and 65535.

**IFLA\_BRPORT\_MODE**
:   Set the bridge port mode. See *BRIDGE\_MODE\_HAIRPIN* for more details.

**IFLA\_BRPORT\_GUARD**
:   Controls whether STP BPDUs will be processed by the bridge port. By
    default, the flag is turned off to allow BPDU processing. Turning this
    flag on will disable the bridge port if a STP BPDU packet is received.

    If the bridge has Spanning Tree enabled, hostile devices on the network
    may send BPDU on a port and cause network failure. Setting *guard on*
    will detect and stop this by disabling the port. The port will be
    restarted if the link is brought down, or removed and reattached.

**IFLA\_BRPORT\_PROTECT**
:   Controls whether a given port is allowed to become a root port or not.
    Only used when STP is enabled on the bridge. By default the flag is off.

    This feature is also called root port guard. If BPDU is received from a
    leaf (edge) port, it should not be elected as root port. This could
    be used if using STP on a bridge and the downstream bridges are not fully
    trusted; this prevents a hostile guest from rerouting traffic.

**IFLA\_BRPORT\_FAST\_LEAVE**
:   This flag allows the bridge to immediately stop multicast traffic
    forwarding on a port that receives an IGMP Leave message. It is only used
    when IGMP snooping is enabled on the bridge. By default the flag is off.

**IFLA\_BRPORT\_LEARNING**
:   Controls whether a given port will learn *source* MAC addresses from
    received traffic or not. Also controls whether dynamic FDB entries
    (which can also be added by software) will be refreshed by incoming
    traffic. By default this flag is on.

**IFLA\_BRPORT\_UNICAST\_FLOOD**
:   Controls whether unicast traffic for which there is no FDB entry will
    be flooded towards this port. By default this flag is on.

**IFLA\_BRPORT\_PROXYARP**
:   Enable proxy ARP on this port.

**IFLA\_BRPORT\_LEARNING\_SYNC**
:   Controls whether a given port will sync MAC addresses learned on device
    port to bridge FDB.

**IFLA\_BRPORT\_PROXYARP\_WIFI**
:   Enable proxy ARP on this port which meets extended requirements by
    IEEE 802.11 and Hotspot 2.0 specifications.

**IFLA\_BRPORT\_ROOT\_ID**

**IFLA\_BRPORT\_BRIDGE\_ID**

**IFLA\_BRPORT\_DESIGNATED\_PORT**

**IFLA\_BRPORT\_DESIGNATED\_COST**

**IFLA\_BRPORT\_ID**

**IFLA\_BRPORT\_NO**

**IFLA\_BRPORT\_TOPOLOGY\_CHANGE\_ACK**

**IFLA\_BRPORT\_CONFIG\_PENDING**

**IFLA\_BRPORT\_MESSAGE\_AGE\_TIMER**

**IFLA\_BRPORT\_FORWARD\_DELAY\_TIMER**

**IFLA\_BRPORT\_HOLD\_TIMER**

**IFLA\_BRPORT\_FLUSH**
:   Flush bridge ports’ fdb dynamic entries.

**IFLA\_BRPORT\_MULTICAST\_ROUTER**
:   Configure the port’s multicast router presence. A port with
    a multicast router will receive all multicast traffic.
    The valid values are:

    > * 0 disable multicast routers on this port
    > * 1 let the system detect the presence of routers (default)
    > * 2 permanently enable multicast traffic forwarding on this port
    > * 3 enable multicast routers temporarily on this port, not depending
    >   :   on incoming queries.

**IFLA\_BRPORT\_PAD**

**IFLA\_BRPORT\_MCAST\_FLOOD**
:   Controls whether a given port will flood multicast traffic for which
    there is no MDB entry. By default this flag is on.

**IFLA\_BRPORT\_MCAST\_TO\_UCAST**
:   Controls whether a given port will replicate packets using unicast
    instead of multicast. By default this flag is off.

    This is done by copying the packet per host and changing the multicast
    destination MAC to a unicast one accordingly.

    *mcast\_to\_unicast* works on top of the multicast snooping feature of the
    bridge. Which means unicast copies are only delivered to hosts which
    are interested in unicast and signaled this via IGMP/MLD reports previously.

    This feature is intended for interface types which have a more reliable
    and/or efficient way to deliver unicast packets than broadcast ones
    (e.g. WiFi).

    However, it should only be enabled on interfaces where no IGMPv2/MLDv1
    report suppression takes place. IGMP/MLD report suppression issue is
    usually overcome by the network daemon (supplicant) enabling AP isolation
    and by that separating all STAs.

    Delivery of STA-to-STA IP multicast is made possible again by enabling
    and utilizing the bridge hairpin mode, which considers the incoming port
    as a potential outgoing port, too (see *BRIDGE\_MODE\_HAIRPIN* option).
    Hairpin mode is performed after multicast snooping, therefore leading
    to only deliver reports to STAs running a multicast router.

**IFLA\_BRPORT\_VLAN\_TUNNEL**
:   Controls whether vlan to tunnel mapping is enabled on the port.
    By default this flag is off.

**IFLA\_BRPORT\_BCAST\_FLOOD**
:   Controls flooding of broadcast traffic on the given port. By default
    this flag is on.

**IFLA\_BRPORT\_GROUP\_FWD\_MASK**
:   Set the group forward mask. This is a bitmask that is applied to
    decide whether to forward incoming frames destined to link-local
    addresses. The addresses of the form are 01:80:C2:00:00:0X (defaults
    to 0, which means the bridge does not forward any link-local frames
    coming on this port).

**IFLA\_BRPORT\_NEIGH\_SUPPRESS**
:   Controls whether neighbor discovery (arp and nd) proxy and suppression
    is enabled on the port. By default this flag is off.

**IFLA\_BRPORT\_ISOLATED**
:   Controls whether a given port will be isolated, which means it will be
    able to communicate with non-isolated ports only. By default this
    flag is off.

**IFLA\_BRPORT\_BACKUP\_PORT**
:   Set a backup port. If the port loses carrier all traffic will be
    redirected to the configured backup port. Set the value to 0 to disable
    it.

**IFLA\_BRPORT\_MRP\_RING\_OPEN**

**IFLA\_BRPORT\_MRP\_IN\_OPEN**

**IFLA\_BRPORT\_MCAST\_EHT\_HOSTS\_LIMIT**
:   The number of per-port EHT hosts limit. The default value is 512.
    Setting to 0 is not allowed.

**IFLA\_BRPORT\_MCAST\_EHT\_HOSTS\_CNT**
:   The current number of tracked hosts, read only.

**IFLA\_BRPORT\_LOCKED**
:   Controls whether a port will be locked, meaning that hosts behind the
    port will not be able to communicate through the port unless an FDB
    entry with the unit’s MAC address is in the FDB. The common use case is
    that hosts are allowed access through authentication with the IEEE 802.1X
    protocol or based on whitelists. By default this flag is off.

    Please note that secure 802.1X deployments should always use the
    *BR\_BOOLOPT\_NO\_LL\_LEARN* flag, to not permit the bridge to populate its
    FDB based on link-local (EAPOL) traffic received on the port.

**IFLA\_BRPORT\_MAB**
:   Controls whether a port will use MAC Authentication Bypass (MAB), a
    technique through which select MAC addresses may be allowed on a locked
    port, without using 802.1X authentication. Packets with an unknown source
    MAC address generates a “locked” FDB entry on the incoming bridge port.
    The common use case is for user space to react to these bridge FDB
    notifications and optionally replace the locked FDB entry with a normal
    one, allowing traffic to pass for whitelisted MAC addresses.

    Setting this flag also requires *IFLA\_BRPORT\_LOCKED* and
    *IFLA\_BRPORT\_LEARNING*. *IFLA\_BRPORT\_LOCKED* ensures that unauthorized
    data packets are dropped, and *IFLA\_BRPORT\_LEARNING* allows the dynamic
    FDB entries installed by user space (as replacements for the locked FDB
    entries) to be refreshed and/or aged out.

**IFLA\_BRPORT\_MCAST\_N\_GROUPS**

**IFLA\_BRPORT\_MCAST\_MAX\_GROUPS**
:   Sets the maximum number of MDB entries that can be registered for a
    given port. Attempts to register more MDB entries at the port than this
    limit allows will be rejected, whether they are done through netlink
    (e.g. the bridge tool), or IGMP or MLD membership reports. Setting a
    limit of 0 disables the limit. The default value is 0.

**IFLA\_BRPORT\_NEIGH\_VLAN\_SUPPRESS**
:   Controls whether neighbor discovery (arp and nd) proxy and suppression is
    enabled for a given port. By default this flag is off.

    Note that this option only takes effect when *IFLA\_BRPORT\_NEIGH\_SUPPRESS*
    is enabled for a given port.

**IFLA\_BRPORT\_BACKUP\_NHID**
:   The FDB nexthop object ID to attach to packets being redirected to a
    backup port that has VLAN tunnel mapping enabled (via the
    *IFLA\_BRPORT\_VLAN\_TUNNEL* option). Setting a value of 0 (default) has
    the effect of not attaching any ID.

### Bridge sysfs

The sysfs interface is deprecated and should not be extended if new
options are added.

## STP

The STP (Spanning Tree Protocol) implementation in the Linux bridge driver
is a critical feature that helps prevent loops and broadcast storms in
Ethernet networks by identifying and disabling redundant links. In a Linux
bridge context, STP is crucial for network stability and availability.

STP is a Layer 2 protocol that operates at the Data Link Layer of the OSI
model. It was originally developed as IEEE 802.1D and has since evolved into
multiple versions, including Rapid Spanning Tree Protocol (RSTP) and
[Multiple Spanning Tree Protocol (MSTP)](https://lore.kernel.org/netdev/20220316150857.2442916-1-tobias@waldekranz.com/).

The 802.1D-2004 removed the original Spanning Tree Protocol, instead
incorporating the Rapid Spanning Tree Protocol (RSTP). By 2014, all the
functionality defined by IEEE 802.1D has been incorporated into either
IEEE 802.1Q (Bridges and Bridged Networks) or IEEE 802.1AC (MAC Service
Definition). 802.1D has been officially withdrawn in 2022.

### Bridge Ports and STP States

In the context of STP, bridge ports can be in one of the following states:
:   * Blocking: The port is disabled for data traffic and only listens for
      BPDUs (Bridge Protocol Data Units) from other devices to determine the
      network topology.
    * Listening: The port begins to participate in the STP process and listens
      for BPDUs.
    * Learning: The port continues to listen for BPDUs and begins to learn MAC
      addresses from incoming frames but does not forward data frames.
    * Forwarding: The port is fully operational and forwards both BPDUs and
      data frames.
    * Disabled: The port is administratively disabled and does not participate
      in the STP process. The data frames forwarding are also disabled.

### Root Bridge and Convergence

In the context of networking and Ethernet bridging in Linux, the root bridge
is a designated switch in a bridged network that serves as a reference point
for the spanning tree algorithm to create a loop-free topology.

Here’s how the STP works and root bridge is chosen:
:   1. Bridge Priority: Each bridge running a spanning tree protocol, has a
       configurable Bridge Priority value. The lower the value, the higher the
       priority. By default, the Bridge Priority is set to a standard value
       (e.g., 32768).
    2. Bridge ID: The Bridge ID is composed of two components: Bridge Priority
       and the MAC address of the bridge. It uniquely identifies each bridge
       in the network. The Bridge ID is used to compare the priorities of
       different bridges.
    3. Bridge Election: When the network starts, all bridges initially assume
       that they are the root bridge. They start advertising Bridge Protocol
       Data Units (BPDU) to their neighbors, containing their Bridge ID and
       other information.
    4. BPDU Comparison: Bridges exchange BPDUs to determine the root bridge.
       Each bridge examines the received BPDUs, including the Bridge Priority
       and Bridge ID, to determine if it should adjust its own priorities.
       The bridge with the lowest Bridge ID will become the root bridge.
    5. Root Bridge Announcement: Once the root bridge is determined, it sends
       BPDUs with information about the root bridge to all other bridges in the
       network. This information is used by other bridges to calculate the
       shortest path to the root bridge and, in doing so, create a loop-free
       topology.
    6. Forwarding Ports: After the root bridge is selected and the spanning tree
       topology is established, each bridge determines which of its ports should
       be in the forwarding state (used for data traffic) and which should be in
       the blocking state (used to prevent loops). The root bridge’s ports are
       all in the forwarding state. while other bridges have some ports in the
       blocking state to avoid loops.
    7. Root Ports: After the root bridge is selected and the spanning tree
       topology is established, each non-root bridge processes incoming
       BPDUs and determines which of its ports provides the shortest path to the
       root bridge based on the information in the received BPDUs. This port is
       designated as the root port. And it is in the Forwarding state, allowing
       it to actively forward network traffic.
    8. Designated ports: A designated port is the port through which the non-root
       bridge will forward traffic towards the designated segment. Designated ports
       are placed in the Forwarding state. All other ports on the non-root
       bridge that are not designated for specific segments are placed in the
       Blocking state to prevent network loops.

STP ensures network convergence by calculating the shortest path and disabling
redundant links. When network topology changes occur (e.g., a link failure),
STP recalculates the network topology to restore connectivity while avoiding loops.

Proper configuration of STP parameters, such as the bridge priority, can
influence network performance, path selection and which bridge becomes the
Root Bridge.

### User space STP helper

The user space STP helper *bridge-stp* is a program to control whether to use
user mode spanning tree. The `/sbin/bridge-stp <bridge> <start|stop>` is
called by the kernel when STP is enabled/disabled on a bridge
(via `brctl stp <bridge> <on|off>` or `ip link set <bridge> type bridge
stp_state <0|1>`). The kernel enables user\_stp mode if that command returns
0, or enables kernel\_stp mode if that command returns any other value.

### STP mode selection

The `IFLA_BR_STP_MODE` bridge attribute allows explicit control over how
STP operates when enabled, bypassing the `/sbin/bridge-stp` helper
entirely for the `user` and `kernel` modes.

**BR\_STP\_MODE\_AUTO**
:   Default. The kernel invokes the `/sbin/bridge-stp` helper to hand
    the bridge to a userspace STP daemon (e.g. mstpd). Only attempted in
    the initial network namespace; in other namespaces this falls back to
    kernel STP.

**BR\_STP\_MODE\_USER**
:   Directly enable userspace STP (`BR_USER_STP`) without invoking the
    `/sbin/bridge-stp` helper. Works in any network namespace.
    Userspace is responsible for ensuring an STP daemon manages the
    bridge.

**BR\_STP\_MODE\_KERNEL**
:   Directly enable kernel STP (`BR_KERNEL_STP`) without invoking the
    helper.

The mode controls how the bridge selects between userspace and kernel
STP when STP is enabled via `IFLA_BR_STP_STATE`. It can only be
changed while STP is disabled (`IFLA_BR_STP_STATE` == 0), returns
`-EBUSY` otherwise. The default value is `BR_STP_MODE_AUTO`.

The default mode is `BR_STP_MODE_AUTO`, which preserves the traditional
behavior of invoking the `/sbin/bridge-stp` helper. The `user` and
`kernel` modes are particularly useful in network namespace environments
where the helper mechanism is not available, as `call_usermodehelper()`
is restricted to the initial network namespace.

Example:

```
ip link set dev br0 type bridge stp_mode user stp_state 1
```

The mode can only be changed while STP is disabled.

## VLAN

A LAN (Local Area Network) is a network that covers a small geographic area,
typically within a single building or a campus. LANs are used to connect
computers, servers, printers, and other networked devices within a localized
area. LANs can be wired (using Ethernet cables) or wireless (using Wi-Fi).

A VLAN (Virtual Local Area Network) is a logical segmentation of a physical
network into multiple isolated broadcast domains. VLANs are used to divide
a single physical LAN into multiple virtual LANs, allowing different groups of
devices to communicate as if they were on separate physical networks.

Typically there are two VLAN implementations, IEEE 802.1Q and IEEE 802.1ad
(also known as QinQ). IEEE 802.1Q is a standard for VLAN tagging in Ethernet
networks. It allows network administrators to create logical VLANs on a
physical network and tag Ethernet frames with VLAN information, which is
called *VLAN-tagged frames*. IEEE 802.1ad, commonly known as QinQ or Double
VLAN, is an extension of the IEEE 802.1Q standard. QinQ allows for the
stacking of multiple VLAN tags within a single Ethernet frame. The Linux
bridge supports both the IEEE 802.1Q and [802.1AD](https://lore.kernel.org/netdev/1402401565-15423-1-git-send-email-makita.toshiaki@lab.ntt.co.jp/)
protocol for VLAN tagging.

[VLAN filtering](https://lore.kernel.org/netdev/1360792820-14116-1-git-send-email-vyasevic@redhat.com/)
on a bridge is disabled by default. After enabling VLAN filtering on a bridge,
it will start forwarding frames to appropriate destinations based on their
destination MAC address and VLAN tag (both must match).

## Multicast

The Linux bridge driver has multicast support allowing it to process Internet
Group Management Protocol (IGMP) or Multicast Listener Discovery (MLD)
messages, and to efficiently forward multicast data packets. The bridge
driver supports IGMPv2/IGMPv3 and MLDv1/MLDv2.

### Multicast snooping

Multicast snooping is a networking technology that allows network switches
to intelligently manage multicast traffic within a local area network (LAN).

The switch maintains a multicast group table, which records the association
between multicast group addresses and the ports where hosts have joined these
groups. The group table is dynamically updated based on the IGMP/MLD messages
received. With the multicast group information gathered through snooping, the
switch optimizes the forwarding of multicast traffic. Instead of blindly
broadcasting the multicast traffic to all ports, it sends the multicast
traffic based on the destination MAC address only to ports which have
subscribed the respective destination multicast group.

When created, the Linux bridge devices have multicast snooping enabled by
default. It maintains a Multicast forwarding database (MDB) which keeps track
of port and group relationships.

### IGMPv3/MLDv2 EHT support

The Linux bridge supports IGMPv3/MLDv2 EHT (Explicit Host Tracking), which
was added by [474ddb37fa3a (“net: bridge: multicast: add EHT allow/block handling”)](https://lore.kernel.org/netdev/20210120145203.1109140-1-razor@blackwall.org/)

The explicit host tracking enables the device to keep track of each
individual host that is joined to a particular group or channel. The main
benefit of the explicit host tracking in IGMP is to allow minimal leave
latencies when a host leaves a multicast group or channel.

The length of time between a host wanting to leave and a device stopping
traffic forwarding is called the IGMP leave latency. A device configured
with IGMPv3 or MLDv2 and explicit tracking can immediately stop forwarding
traffic if the last host to request to receive traffic from the device
indicates that it no longer wants to receive traffic. The leave latency
is thus bound only by the packet transmission latencies in the multiaccess
network and the processing time in the device.

### Other multicast features

The Linux bridge also supports [per-VLAN multicast snooping](https://lore.kernel.org/netdev/20210719170637.435541-1-razor@blackwall.org/),
which is disabled by default but can be enabled. And [Multicast Router Discovery](https://lore.kernel.org/netdev/20190121062628.2710-1-linus.luessing@c0d3.blue/),
which help identify the location of multicast routers.

## Switchdev

Linux Bridge Switchdev is a feature in the Linux kernel that extends the
capabilities of the traditional Linux bridge to work more efficiently with
hardware switches that support switchdev. With Linux Bridge Switchdev, certain
networking functions like forwarding, filtering, and learning of Ethernet
frames can be offloaded to a hardware switch. This offloading reduces the
burden on the Linux kernel and CPU, leading to improved network performance
and lower latency.

To use Linux Bridge Switchdev, you need hardware switches that support the
switchdev interface. This means that the switch hardware needs to have the
necessary drivers and functionality to work in conjunction with the Linux
kernel.

Please see the [Ethernet switch device driver model (switchdev)](switchdev.html#switchdev) document for more details.

## Netfilter

The bridge netfilter module is a legacy feature that allows to filter bridged
packets with iptables and ip6tables. Its use is discouraged. Users should
consider using nftables for packet filtering.

The older ebtables tool is more feature-limited compared to nftables, but
just like nftables it doesn’t need this module either to function.

The br\_netfilter module intercepts packets entering the bridge, performs
minimal sanity tests on ipv4 and ipv6 packets and then pretends that
these packets are being routed, not bridged. br\_netfilter then calls
the ip and ipv6 netfilter hooks from the bridge layer, i.e. ip(6)tables
rulesets will also see these packets.

br\_netfilter is also the reason for the iptables *physdev* match:
This match is the only way to reliably tell routed and bridged packets
apart in an iptables ruleset.

Note that ebtables and nftables will work fine without the br\_netfilter module.
iptables/ip6tables/arptables do not work for bridged traffic because they
plug in the routing stack. nftables rules in ip/ip6/inet/arp families won’t
see traffic that is forwarded by a bridge either, but that’s very much how it
should be.

Historically the feature set of ebtables was very limited (it still is),
this module was added to pretend packets are routed and invoke the ipv4/ipv6
netfilter hooks from the bridge so users had access to the more feature-rich
iptables matching capabilities (including conntrack). nftables doesn’t have
this limitation, pretty much all features work regardless of the protocol family.

So, br\_netfilter is only needed if users, for some reason, need to use
ip(6)tables to filter packets forwarded by the bridge, or NAT bridged
traffic. For pure link layer filtering, this module isn’t needed.

## Other Features

The Linux bridge also supports [IEEE 802.11 Proxy ARP](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=958501163ddd6ea22a98f94fa0e7ce6d4734e5c4),
[Media Redundancy Protocol (MRP)](https://lore.kernel.org/netdev/20200426132208.3232-1-horatiu.vultur@microchip.com/),
[Media Redundancy Protocol (MRP) LC mode](https://lore.kernel.org/r/20201124082525.273820-1-horatiu.vultur@microchip.com),
[IEEE 802.1X port authentication](https://lore.kernel.org/netdev/20220218155148.2329797-1-schultz.hans+netdev@gmail.com/),
and [MAC Authentication Bypass (MAB)](https://lore.kernel.org/netdev/20221101193922.2125323-2-idosch@nvidia.com/).

## FAQ

### What does a bridge do?

A bridge transparently forwards traffic between multiple network interfaces.
In plain English this means that a bridge connects two or more physical
Ethernet networks, to form one larger (logical) Ethernet network.

### Is it L3 protocol independent?

Yes. The bridge sees all frames, but it *uses* only L2 headers/information.
As such, the bridging functionality is protocol independent, and there should
be no trouble forwarding IPX, NetBEUI, IP, IPv6, etc.

## Contact Info

The code is currently maintained by Roopa Prabhu <[roopa@nvidia.com](mailto:roopa%40nvidia.com)> and
Nikolay Aleksandrov <[razor@blackwall.org](mailto:razor%40blackwall.org)>. Bridge bugs and enhancements
are discussed on the linux-netdev mailing list [netdev@vger.kernel.org](mailto:netdev%40vger.kernel.org) and
[bridge@lists.linux.dev](mailto:bridge%40lists.linux.dev).

The list is open to anyone interested: <http://vger.kernel.org/vger-lists.html#netdev>

## External Links

The old Documentation for Linux bridging is on:
<https://wiki.linuxfoundation.org/networking/bridge>
