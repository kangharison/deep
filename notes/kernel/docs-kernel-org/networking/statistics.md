# Interface statistics

> 출처(원문): https://docs.kernel.org/networking/statistics.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Interface statistics

## Overview

This document is a guide to Linux network interface statistics.

There are three main sources of interface statistics in Linux:

> * standard interface statistics based on
>   [`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64");
> * protocol-specific statistics; and
> * driver-defined statistics available via ethtool.

### Standard interface statistics

There are multiple interfaces to reach the standard statistics.
Most commonly used is the ip command from iproute2:

```
$ ip -s -s link show dev ens4u1u1
6: ens4u1u1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
  link/ether 48:2a:e3:4c:b1:d1 brd ff:ff:ff:ff:ff:ff
  RX: bytes  packets  errors  dropped overrun mcast
  74327665117 69016965 0       0       0       0
  RX errors: length   crc     frame   fifo    missed
             0        0       0       0       0
  TX: bytes  packets  errors  dropped carrier collsns
  21405556176 44608960 0       0       0       0
  TX errors: aborted  fifo   window heartbeat transns
             0        0       0       0       128
  altname enp58s0u1u1
```

Note that -s has been specified twice to see all members of
[`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64").
If -s is specified once the detailed errors won’t be shown.

ip supports JSON formatting via the -j option.

#### Queue statistics

Queue statistics are accessible via the netdev netlink family.

Currently no widely distributed CLI exists to access those statistics.
Kernel development tools (ynl) can be used to experiment with them,
see [Using Netlink protocol specifications](../userspace-api/netlink/intro-specs.html).

### Protocol-specific statistics

Protocol-specific statistics are exposed via relevant interfaces,
the same interfaces as are used to configure them.

#### ethtool

Ethtool exposes common low-level statistics.
All the standard statistics are expected to be maintained
by the device, not the driver (as opposed to driver-defined stats
described in the next section which mix software and hardware stats).
For devices which contain unmanaged
switches (e.g. legacy SR-IOV or multi-host NICs) the events counted
may not pertain exclusively to the packets destined to
the local host interface. In other words the events may
be counted at the network port (MAC/PHY blocks) without separation
for different host side (PCIe) devices. Such ambiguity must not
be present when internal switch is managed by Linux (so called
switchdev mode for NICs).

Standard ethtool statistics can be accessed via the interfaces used
for configuration. For example ethtool interface used
to configure pause frames can report corresponding hardware counters:

```
$ ethtool --include-statistics -a eth0
Pause parameters for eth0:
Autonegotiate:        on
RX:                   on
TX:                   on
Statistics:
  tx_pause_frames: 1
  rx_pause_frames: 1
```

General Ethernet statistics not associated with any particular
functionality are exposed via `ethtool -S $ifc` by specifying
the `--groups` parameter:

```
$ ethtool -S eth0 --groups eth-phy eth-mac eth-ctrl rmon
Stats for eth0:
eth-phy-SymbolErrorDuringCarrier: 0
eth-mac-FramesTransmittedOK: 1
eth-mac-FrameTooLongErrors: 1
eth-ctrl-MACControlFramesTransmitted: 1
eth-ctrl-MACControlFramesReceived: 0
eth-ctrl-UnsupportedOpcodesReceived: 1
rmon-etherStatsUndersizePkts: 1
rmon-etherStatsJabbers: 0
rmon-rx-etherStatsPkts64Octets: 1
rmon-rx-etherStatsPkts65to127Octets: 0
rmon-rx-etherStatsPkts128to255Octets: 0
rmon-tx-etherStatsPkts64Octets: 2
rmon-tx-etherStatsPkts65to127Octets: 3
rmon-tx-etherStatsPkts128to255Octets: 0
```

### Driver-defined statistics

Driver-defined ethtool statistics can be dumped using ethtool -S $ifc, e.g.:

```
$ ethtool -S ens4u1u1
NIC statistics:
   tx_single_collisions: 0
   tx_multi_collisions: 0
```

## uAPIs

### procfs

The historical /proc/net/dev text interface gives access to the list
of interfaces as well as their statistics.

Note that even though this interface is using
[`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64")
internally it combines some of the fields.

### sysfs

Each device directory in sysfs contains a statistics directory (e.g.
/sys/class/net/lo/statistics/) with files corresponding to
members of [`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64").

This simple interface is convenient especially in constrained/embedded
environments without access to tools. However, it’s inefficient when
reading multiple stats as it internally performs a full dump of
[`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64")
and reports only the stat corresponding to the accessed file.

Sysfs files are documented in
[ABI file testing/sysfs-class-net-statistics](../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-class-net-statistics).

### netlink

rtnetlink (NETLINK\_ROUTE) is the preferred method of accessing
[`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64") stats.

Statistics are reported both in the responses to link information
requests (RTM\_GETLINK) and statistic requests (RTM\_GETSTATS,
when IFLA\_STATS\_LINK\_64 bit is set in the .filter\_mask of the request).

#### netdev (netlink)

netdev generic netlink family allows accessing page pool and per queue
statistics.

### ethtool

Ethtool IOCTL interface allows drivers to report implementation
specific statistics. Historically it has also been used to report
statistics for which other APIs did not exist, like per-device-queue
statistics, or standard-based statistics (e.g. RFC 2863).

Statistics and their string identifiers are retrieved separately.
Identifiers via ETHTOOL\_GSTRINGS with string\_set set to ETH\_SS\_STATS,
and values via ETHTOOL\_GSTATS. User space should use ETHTOOL\_GDRVINFO
to retrieve the number of statistics (.n\_stats).

### ethtool-netlink

Ethtool netlink is a replacement for the older IOCTL interface.

Protocol-related statistics can be requested in get commands by setting
the ETHTOOL\_FLAG\_STATS flag in ETHTOOL\_A\_HEADER\_FLAGS. Currently
statistics are supported in the following commands:

> * ETHTOOL\_MSG\_FEC\_GET
> * ETHTOOL\_MSG\_LINKSTATE\_GET
> * ETHTOOL\_MSG\_MM\_GET
> * ETHTOOL\_MSG\_PAUSE\_GET
> * ETHTOOL\_MSG\_TSINFO\_GET

### debugfs

Some drivers expose extra statistics via debugfs.

## struct rtnl\_link\_stats64

struct rtnl\_link\_stats64
:   The main device statistics structure.

**Definition**:

```
struct rtnl_link_stats64 {
    __u64 rx_packets;
    __u64 tx_packets;
    __u64 rx_bytes;
    __u64 tx_bytes;
    __u64 rx_errors;
    __u64 tx_errors;
    __u64 rx_dropped;
    __u64 tx_dropped;
    __u64 multicast;
    __u64 collisions;
    __u64 rx_length_errors;
    __u64 rx_over_errors;
    __u64 rx_crc_errors;
    __u64 rx_frame_errors;
    __u64 rx_fifo_errors;
    __u64 rx_missed_errors;
    __u64 tx_aborted_errors;
    __u64 tx_carrier_errors;
    __u64 tx_fifo_errors;
    __u64 tx_heartbeat_errors;
    __u64 tx_window_errors;
    __u64 rx_compressed;
    __u64 tx_compressed;
    __u64 rx_nohandler;
    __u64 rx_otherhost_dropped;
};
```

**Members**

`rx_packets`
:   Number of good packets received by the interface.
    For hardware interfaces counts all good packets received from the device
    by the host, including packets which host had to drop at various stages
    of processing (even in the driver).

`tx_packets`
:   Number of packets successfully transmitted.
    For hardware interfaces counts packets which host was able to successfully
    hand over to the device, which does not necessarily mean that packets
    had been successfully transmitted out of the device, only that device
    acknowledged it copied them out of host memory.

`rx_bytes`
:   Number of good received bytes, corresponding to **rx\_packets**.

`tx_bytes`
:   Number of good transmitted bytes, corresponding to **tx\_packets**.

`rx_errors`
:   Total number of bad packets received on this network device.
    This counter must include events counted by **rx\_length\_errors**,
    **rx\_crc\_errors**, **rx\_frame\_errors** and other errors not otherwise
    counted.

`tx_errors`
:   Total number of transmit problems.
    This counter must include events counter by **tx\_aborted\_errors**,
    **tx\_carrier\_errors**, **tx\_fifo\_errors**, **tx\_heartbeat\_errors**,
    **tx\_window\_errors** and other errors not otherwise counted.

`rx_dropped`
:   Number of packets received but not processed,
    e.g. due to lack of resources or unsupported protocol.
    For hardware interfaces this counter may include packets discarded
    due to L2 address filtering but should not include packets dropped
    by the device due to buffer exhaustion which are counted separately in
    **rx\_missed\_errors** (since procfs folds those two counters together).

`tx_dropped`
:   Number of packets dropped on their way to transmission,
    e.g. due to lack of resources.

`multicast`
:   Multicast packets received.
    For hardware interfaces this statistic is commonly calculated
    at the device level (unlike **rx\_packets**) and therefore may include
    packets which did not reach the host.

`collisions`
:   Number of collisions during packet transmissions.

`rx_length_errors`
:   Number of packets dropped due to invalid length.
    Part of aggregate “frame” errors in /proc/net/dev.

`rx_over_errors`
:   Receiver FIFO overflow event counter.

`rx_crc_errors`
:   Number of packets received with a CRC error.
    Part of aggregate “frame” errors in /proc/net/dev.

`rx_frame_errors`
:   Receiver frame alignment errors.
    Part of aggregate “frame” errors in /proc/net/dev.

`rx_fifo_errors`
:   Receiver FIFO error counter.

`rx_missed_errors`
:   Count of packets missed by the host.
    Folded into the “drop” counter in /proc/net/dev.

`tx_aborted_errors`
:   Part of aggregate “carrier” errors in /proc/net/dev.
    For IEEE 802.3 devices capable of half-duplex operation this counter
    must be equivalent to:

`tx_carrier_errors`
:   Number of frame transmission errors due to loss
    of carrier during transmission.
    Part of aggregate “carrier” errors in /proc/net/dev.

`tx_fifo_errors`
:   Number of frame transmission errors due to device
    FIFO underrun / underflow. This condition occurs when the device
    begins transmission of a frame but is unable to deliver the
    entire frame to the transmitter in time for transmission.
    Part of aggregate “carrier” errors in /proc/net/dev.

`tx_heartbeat_errors`
:   Number of Heartbeat / SQE Test errors for
    old half-duplex Ethernet.
    Part of aggregate “carrier” errors in /proc/net/dev.

`tx_window_errors`
:   Number of frame transmission errors due
    to late collisions (for Ethernet - after the first 64B of transmission).
    Part of aggregate “carrier” errors in /proc/net/dev.

`rx_compressed`
:   Number of correctly received compressed packets.
    This counters is only meaningful for interfaces which support
    packet compression (e.g. CSLIP, PPP).

`tx_compressed`
:   Number of transmitted compressed packets.
    This counters is only meaningful for interfaces which support
    packet compression (e.g. CSLIP, PPP).

`rx_nohandler`
:   Number of packets received on the interface
    but dropped by the networking stack because the device is
    not designated to receive packets (e.g. backup link in a bond).

`rx_otherhost_dropped`
:   Number of packets dropped due to mismatch
    in destination MAC address.

**Description**

> For IEEE 802.3 devices should count the length of Ethernet Frames
> excluding the FCS.
>
> For IEEE 802.3 devices should count the length of Ethernet Frames
> excluding the FCS.
>
> For IEEE 802.3 devices this counter may be equivalent to:
>
> > * 30.3.1.1.21 aMulticastFramesReceivedOK
>
> For IEEE 802.3 devices this counter should be equivalent to a sum
> of the following attributes:
>
> > * 30.3.1.1.23 aInRangeLengthErrors
> > * 30.3.1.1.24 aOutOfRangeLengthField
> > * 30.3.1.1.25 aFrameTooLongErrors
>
> Historically the count of overflow events. Such events may be
> reported in the receive descriptors or via interrupts, and may
> not correspond one-to-one with dropped packets.
>
> The recommended interpretation for high speed interfaces is -
> number of packets dropped because they did not fit into buffers
> provided by the host, e.g. packets larger than MTU or next buffer
> in the ring was not available for a scatter transfer.
>
> Part of aggregate “frame” errors in /proc/net/dev.
>
> This statistics was historically used interchangeably with
> **rx\_fifo\_errors**.
>
> This statistic corresponds to hardware events and is not commonly used
> on software devices.
>
> For IEEE 802.3 devices this counter must be equivalent to:
>
> > * 30.3.1.1.6 aFrameCheckSequenceErrors
>
> For IEEE 802.3 devices this counter should be equivalent to:
>
> > * 30.3.1.1.7 aAlignmentErrors
>
> Historically the count of overflow events. Those events may be
> reported in the receive descriptors or via interrupts, and may
> not correspond one-to-one with dropped packets.
>
> This statistics was used interchangeably with **rx\_over\_errors**.
> Not recommended for use in drivers for high speed interfaces.
>
> This statistic is used on software devices, e.g. to count software
> packet queue overflow (can) or sequencing errors (GRE).
>
> Counts number of packets dropped by the device due to lack
> of buffer space. This usually indicates that the host interface
> is slower than the network interface, or host is not keeping up
> with the receive packet rate.
>
> This statistic corresponds to hardware events and is not used
> on software devices.
>
> > * 30.3.1.1.11 aFramesAbortedDueToXSColls
>
> High speed interfaces may use this counter as a general device
> discard counter.
>
> For IEEE 802.3 devices this counter must be equivalent to:
>
> > * 30.3.1.1.13 aCarrierSenseErrors
>
> For IEEE 802.3 devices possibly equivalent to:
>
> > * 30.3.2.1.4 aSQETestErrors
>
> For IEEE 802.3 devices this counter must be equivalent to:
>
> > * 30.3.1.1.10 aLateCollisions

## Notes for driver authors

Drivers should report all statistics which have a matching member in
[`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64") exclusively
via .ndo\_get\_stats64. Reporting such standard stats via ethtool
or debugfs will not be accepted.

Drivers must ensure best possible compliance with
[`struct rtnl_link_stats64`](#c.rtnl_link_stats64 "rtnl_link_stats64").
Please note for example that detailed error statistics must be
added into the general rx\_error / tx\_error counters.

The .ndo\_get\_stats64 callback can not sleep because of accesses
via /proc/net/dev. If driver may sleep when retrieving the statistics
from the device it should do so periodically asynchronously and only return
a recent copy from .ndo\_get\_stats64. Ethtool interrupt coalescing interface
allows setting the frequency of refreshing statistics, if needed.

Retrieving ethtool statistics is a multi-syscall process, drivers are advised
to keep the number of statistics constant to avoid race conditions with
user space trying to read them.

Statistics must persist across routine operations like bringing the interface
down and up.

### Kernel-internal data structures

The following structures are internal to the kernel, their members are
translated to netlink attributes when dumped. Drivers must not overwrite
the statistics they don’t report with 0.

* [`ethtool_pause_stats()`](ethtool-netlink.html#c.ethtool_pause_stats "ethtool_pause_stats")
* [`ethtool_fec_stats()`](ethtool-netlink.html#c.ethtool_fec_stats "ethtool_fec_stats")
