# Ethtool counters

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/mellanox/mlx5/counters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ethtool counters

Copyright:
:   © 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

## Contents

* [Overview](#overview)
* [Groups](#groups)
* [Types](#types)
* [Descriptions](#descriptions)

## Overview

There are several counter groups based on where the counter is being counted. In
addition, each group of counters may have different counter types.

These counter groups are based on which component in a networking setup,
illustrated below, that they describe:

```
                                              ----------------------------------------
                                              |                                      |
----------------------------------------    ---------------------------------------- |
|              Hypervisor              |    |                  VM                  | |
|                                      |    |                                      | |
| -------------------  --------------- |    | -------------------  --------------- | |
| | Ethernet driver |  | RDMA driver | |    | | Ethernet driver |  | RDMA driver | | |
| -------------------  --------------- |    | -------------------  --------------- | |
|           |                 |        |    |           |                 |        | |
|           -------------------        |    |           -------------------        | |
|                   |                  |    |                   |                  |--
----------------------------------------    ----------------------------------------
                    |                                           |
        -------------               -----------------------------
        |                           |
     ------                      ------ ------ ------         ------      ------      ------
-----| PF |----------------------| VF |-| VF |-| VF |-----  --| PF |--- --| PF |--- --| PF |---
|    ------                      ------ ------ ------    |  | ------  | | ------  | | ------  |
|                                                        |  |         | |         | |         |
|                                                        |  |         | |         | |         |
|                                                        |  |         | |         | |         |
| eSwitch                                                |  | eSwitch | | eSwitch | | eSwitch |
----------------------------------------------------------  ----------- ----------- -----------
           -------------------------------------------------------------------------------
           |                                                                             |
           |                                                                             |
           | Uplink (no counters)                                                        |
           -------------------------------------------------------------------------------
                   ---------------------------------------------------------------
                   |                                                             |
                   |                                                             |
                   | MPFS (no counters)                                          |
                   ---------------------------------------------------------------
                                                 |
                                                 |
                                                 | Port
```

## Groups

Ring
:   Software counters populated by the driver stack.

Netdev
:   An aggregation of software ring counters.

vPort counters
:   Traffic counters and drops due to steering or no buffers. May indicate issues
    with NIC. These counters include Ethernet traffic counters (including Raw
    Ethernet) and RDMA/RoCE traffic counters.

Physical port counters
:   Counters that collect statistics about the PFs and VFs. May indicate issues
    with NIC, link, or network. This measuring point holds information on
    standardized counters like IEEE 802.3, RFC2863, RFC 2819, RFC 3635 and
    additional counters like flow control, FEC and more. Physical port counters
    are not exposed to virtual machines.

Priority Port Counters
:   A set of the physical port counters, per priority per port.

## Types

Counters are divided into three types.

Traffic Informative Counters
:   Counters which count traffic. These counters can be used for load estimation
    or for general debug.

Traffic Acceleration Counters
:   Counters which count traffic that was accelerated by Mellanox driver or by
    hardware. The counters are an additional layer to the informative counter set,
    and the same traffic is counted in both informative and acceleration counters.

[1]
([1](#id1),[2](#id2),[3](#id3),[4](#id4),[5](#id5),[6](#id6),[7](#id7),[8](#id8),[9](#id9),[10](#id10),[11](#id11),[12](#id12),[13](#id13),[14](#id14),[15](#id15),[16](#id16),[17](#id20),[18](#id21),[19](#id22),[20](#id23),[21](#id24),[22](#id25),[23](#id27),[24](#id28),[25](#id29),[26](#id30),[27](#id31),[28](#id32),[29](#id33),[30](#id34),[31](#id35),[32](#id36))

Traffic acceleration counter.

Error Counters
:   Increment of these counters might indicate a problem. Each of these counters
    has an explanation and correction action.

Statistic can be fetched via the ip link or ethtool commands. ethtool
provides more detailed information.:

```
ip –s link show <if-name>
ethtool -S <if-name>
```

## Descriptions

XSK, PTP, and QoS counters that are similar to counters defined previously will
not be separately listed. For example, ptp\_tx[i]\_packets will not be
explicitly documented since tx[i]\_packets describes the behavior of both
counters, except ptp\_tx[i]\_packets is only counted when precision time
protocol is used.

### Ring / Netdev Counter

The following counters are available per ring or software port.

These counters provide information on the amount of traffic that was accelerated
by the NIC. The counters are counting the accelerated traffic in addition to the
standard counters which counts it (i.e. accelerated traffic is counted twice).

The counter names in the table below refers to both ring and port counters. The
notation for ring counters includes the [i] index without the braces. The
notation for port counters doesn’t include the [i]. A counter name
rx[i]\_packets will be printed as rx0\_packets for ring 0 and rx\_packets for
the software port.

Ring / Software Port Counter Table

|  |  |  |
| --- | --- | --- |
| Counter | Description | Type |
| rx[i]\_packets | The number of packets received on ring i. | Informative |
| rx[i]\_bytes | The number of bytes received on ring i. | Informative |
| tx[i]\_packets | The number of packets transmitted on ring i. | Informative |
| tx[i]\_bytes | The number of bytes transmitted on ring i. | Informative |
| tx[i]\_recover | The number of times the SQ was recovered. | Error |
| tx[i]\_cqes | Number of CQEs events on SQ issued on ring i. | Informative |
| tx[i]\_cqe\_err | The number of error CQEs encountered on the SQ for ring i. | Error |
| tx[i]\_tso\_packets | The number of TSO packets transmitted on ring i [[1]](#accel). | Acceleration |
| tx[i]\_tso\_bytes | The number of TSO bytes transmitted on ring i [[1]](#accel). | Acceleration |
| tx[i]\_tso\_inner\_packets | The number of TSO packets which are indicated to be carry internal encapsulation transmitted on ring i [[1]](#accel). | Acceleration |
| tx[i]\_tso\_inner\_bytes | The number of TSO bytes which are indicated to be carry internal encapsulation transmitted on ring i [[1]](#accel). | Acceleration |
| rx[i]\_gro\_packets | Number of received packets processed using hardware-accelerated GRO. The number of hardware GRO offloaded packets received on ring i. Only true GRO packets are counted: only packets that are in an SKB with a GRO count > 1. | Acceleration |
| rx[i]\_gro\_bytes | Number of received bytes processed using hardware-accelerated GRO. The number of hardware GRO offloaded bytes received on ring i. Only true GRO packets are counted: only packets that are in an SKB with a GRO count > 1. | Acceleration |
| rx[i]\_gro\_skbs | The number of GRO SKBs constructed from hardware-accelerated GRO. Only SKBs with a GRO count > 1 are counted. | Informative |
| rx[i]\_gro\_large\_hds | Number of receive packets using hardware-accelerated GRO that have large headers that require additional memory to be allocated. | Informative |
| rx[i]\_hds\_nodata\_packets | Number of header only packets in header/data split mode [[1]](#accel). | Informative |
| rx[i]\_hds\_nodata\_bytes | Number of bytes for header only packets in header/data split mode [[1]](#accel). | Informative |
| rx[i]\_hds\_nosplit\_packets | Number of packets that were not split in header/data split mode. A packet will not get split when the hardware does not support its protocol splitting. An example such a protocol is ICMPv4/v6. Currently TCP and UDP with IPv4/IPv6 are supported for header/data split [[1]](#accel). | Informative |
| rx[i]\_hds\_nosplit\_bytes | Number of bytes for packets that were not split in header/data split mode. A packet will not get split when the hardware does not support its protocol splitting. An example such a protocol is ICMPv4/v6. Currently TCP and UDP with IPv4/IPv6 are supported for header/data split [[1]](#accel). | Informative |
| rx[i]\_lro\_packets | The number of LRO packets received on ring i [[1]](#accel). | Acceleration |
| rx[i]\_lro\_bytes | The number of LRO bytes received on ring i [[1]](#accel). | Acceleration |
| rx[i]\_ecn\_mark | The number of received packets where the ECN mark was turned on. | Informative |
| rx\_oversize\_pkts\_buffer | The number of dropped received packets due to length which arrived to RQ and exceed software buffer size allocated by the device for incoming traffic. It might imply that the device MTU is larger than the software buffers size. | Error |
| rx\_oversize\_pkts\_sw\_drop | Number of received packets dropped in software because the CQE data is larger than the MTU size. | Error |
| rx[i]\_csum\_unnecessary | Packets received with a CHECKSUM\_UNNECESSARY on ring i [[1]](#accel). | Acceleration |
| rx[i]\_csum\_unnecessary\_inner | Packets received with inner encapsulation with a CHECKSUM\_UNNECESSARY on ring i [[1]](#accel). | Acceleration |
| rx[i]\_csum\_none | Packets received with a CHECKSUM\_NONE on ring i [[1]](#accel). | Acceleration |
| rx[i]\_csum\_complete | Packets received with a CHECKSUM\_COMPLETE on ring i [[1]](#accel). | Acceleration |
| rx[i]\_csum\_complete\_tail | Number of received packets that had checksum calculation computed, potentially needed padding, and were able to do so with CHECKSUM\_PARTIAL. | Informative |
| rx[i]\_csum\_complete\_tail\_slow | Number of received packets that need padding larger than eight bytes for the checksum. | Informative |
| tx[i]\_csum\_partial | Packets transmitted with a CHECKSUM\_PARTIAL on ring i [[1]](#accel). | Acceleration |
| tx[i]\_csum\_partial\_inner | Packets transmitted with inner encapsulation with a CHECKSUM\_PARTIAL on ring i [[1]](#accel). | Acceleration |
| tx[i]\_csum\_none | Packets transmitted with no hardware checksum acceleration on ring i. | Informative |
| tx[i]\_stopped / tx\_queue\_stopped [[2]](#ring-global) | Events where SQ was full on ring i. If this counter is increased, check the amount of buffers allocated for transmission. | Informative |
| tx[i]\_wake / tx\_queue\_wake [[2]](#ring-global) | Events where SQ was full and has become not full on ring i. | Informative |
| tx[i]\_dropped / tx\_queue\_dropped [[2]](#ring-global) | Packets transmitted that were dropped due to DMA mapping failure on ring i. If this counter is increased, check the amount of buffers allocated for transmission. | Error |
| tx[i]\_nop | The number of nop WQEs (empty WQEs) inserted to the SQ (related to ring i) due to the reach of the end of the cyclic buffer. When reaching near to the end of cyclic buffer the driver may add those empty WQEs to avoid handling a state the a WQE start in the end of the queue and ends in the beginning of the queue. This is a normal condition. | Informative |
| tx[i]\_timestamps | Transmitted packets that were hardware timestamped at the device’s DMA layer. | Informative |
| tx[i]\_added\_vlan\_packets | The number of packets sent where vlan tag insertion was offloaded to the hardware. | Acceleration |
| rx[i]\_removed\_vlan\_packets | The number of packets received where vlan tag stripping was offloaded to the hardware. | Acceleration |
| rx[i]\_wqe\_err | The number of wrong opcodes received on ring i. | Error |
| rx[i]\_mpwqe\_frag | The number of WQEs that failed to allocate compound page and hence fragmented MPWQE’s (Multi Packet WQEs) were used on ring i. If this counter raise, it may suggest that there is no enough memory for large pages, the driver allocated fragmented pages. This is not abnormal condition. | Informative |
| rx[i]\_mpwqe\_filler\_cqes | The number of filler CQEs events that were issued on ring i. | Informative |
| rx[i]\_mpwqe\_filler\_strides | The number of strides consumed by filler CQEs on ring i. | Informative |
| tx[i]\_mpwqe\_blks | The number of send blocks processed from Multi-Packet WQEs (mpwqe). | Informative |
| tx[i]\_mpwqe\_pkts | The number of send packets processed from Multi-Packet WQEs (mpwqe). | Informative |
| rx[i]\_cqe\_compress\_blks | The number of receive blocks with CQE compression on ring i [[1]](#accel). | Acceleration |
| rx[i]\_cqe\_compress\_pkts | The number of receive packets with CQE compression on ring i [[1]](#accel). | Acceleration |
| rx[i]\_arfs\_add | The number of aRFS flow rules added to the device for direct RQ steering on ring i [[1]](#accel). | Acceleration |
| rx[i]\_arfs\_request\_in | Number of flow rules that have been requested to move into ring i for direct RQ steering [[1]](#accel). | Acceleration |
| rx[i]\_arfs\_request\_out | Number of flow rules that have been requested to move out of ring i [[1]](#accel). | Acceleration |
| rx[i]\_arfs\_expired | Number of flow rules that have been expired and removed [[1]](#accel). | Acceleration |
| rx[i]\_arfs\_err | Number of flow rules that failed to be added to the flow table. | Error |
| rx[i]\_recover | The number of times the RQ was recovered. | Error |
| tx[i]\_xmit\_more | The number of packets sent with xmit\_more indication set on the skbuff (no doorbell). | Acceleration |
| ch[i]\_poll | The number of invocations of NAPI poll of channel i. | Informative |
| ch[i]\_arm | The number of times the NAPI poll function completed and armed the completion queues on channel i. | Informative |
| ch[i]\_aff\_change | The number of times the NAPI poll function explicitly stopped execution on a CPU due to a change in affinity, on channel i. | Informative |
| ch[i]\_events | The number of hard interrupt events on the completion queues of channel i. | Informative |
| ch[i]\_eq\_rearm | The number of times the EQ was recovered. | Error |
| ch[i]\_force\_irq | Number of times NAPI is triggered by XSK wakeups by posting a NOP to ICOSQ. | Acceleration |
| rx[i]\_congst\_umr | The number of times an outstanding UMR request is delayed due to congestion, on ring i. | Informative |
| rx\_pp\_alloc\_fast | Number of successful fast path allocations. | Informative |
| rx\_pp\_alloc\_slow | Number of slow path order-0 allocations. | Informative |
| rx\_pp\_alloc\_slow\_high\_order | Number of slow path high order allocations. | Informative |
| rx\_pp\_alloc\_empty | Counter is incremented when ptr ring is empty, so a slow path allocation was forced. | Informative |
| rx\_pp\_alloc\_refill | Counter is incremented when an allocation which triggered a refill of the cache. | Informative |
| rx\_pp\_alloc\_waive | Counter is incremented when pages obtained from the ptr ring that cannot be added to the cache due to a NUMA mismatch. | Informative |
| rx\_pp\_recycle\_cached | Counter is incremented when recycling placed page in the page pool cache. | Informative |
| rx\_pp\_recycle\_cache\_full | Counter is incremented when page pool cache was full. | Informative |
| rx\_pp\_recycle\_ring | Counter is incremented when page placed into the ptr ring. | Informative |
| rx\_pp\_recycle\_ring\_full | Counter is incremented when page released from page pool because the ptr ring was full. | Informative |
| rx\_pp\_recycle\_released\_ref | Counter is incremented when page released (and not recycled) because refcnt > 1. | Informative |
| rx[i]\_xsk\_buff\_alloc\_err | The number of times allocating an skb or XSK buffer failed in the XSK RQ context. | Error |
| rx[i]\_xdp\_tx\_xmit | The number of packets forwarded back to the port due to XDP program XDP\_TX action (bouncing). these packets are not counted by other software counters. These packets are counted by physical port and vPort counters. | Informative |
| rx[i]\_xdp\_tx\_mpwqe | Number of multi-packet WQEs transmitted by the netdev and XDP\_TX-ed by the netdev during the RQ context. | Acceleration |
| rx[i]\_xdp\_tx\_inlnw | Number of WQE data segments transmitted where the data could be inlined in the WQE and then XDP\_TX-ed during the RQ context. | Acceleration |
| rx[i]\_xdp\_tx\_nops | Number of NOP WQEBBs (WQE building blocks) received posted to the XDP SQ. | Acceleration |
| rx[i]\_xdp\_tx\_full | The number of packets that should have been forwarded back to the port due to XDP\_TX action but were dropped due to full tx queue. These packets are not counted by other software counters. These packets are counted by physical port and vPort counters. You may open more rx queues and spread traffic rx over all queues and/or increase rx ring size. | Error |
| rx[i]\_xdp\_tx\_err | The number of times an XDP\_TX error such as frame too long and frame too short occurred on XDP\_TX ring of RX ring. | Error |
| rx[i]\_xdp\_tx\_cqes / rx\_xdp\_tx\_cqe [[2]](#ring-global) | The number of completions received on the CQ of the XDP\_TX ring. | Informative |
| rx[i]\_xdp\_drop | The number of packets dropped due to XDP program XDP\_DROP action. these packets are not counted by other software counters. These packets are counted by physical port and vPort counters. | Informative |
| rx[i]\_xdp\_redirect | The number of times an XDP redirect action was triggered on ring i. | Acceleration |
| tx[i]\_xdp\_xmit | The number of packets redirected to the interface(due to XDP redirect). These packets are not counted by other software counters. These packets are counted by physical port and vPort counters. | Informative |
| tx[i]\_xdp\_full | The number of packets redirected to the interface(due to XDP redirect), but were dropped due to full tx queue. these packets are not counted by other software counters. you may enlarge tx queues. | Informative |
| tx[i]\_xdp\_mpwqe | Number of multi-packet WQEs offloaded onto the NIC that were XDP\_REDIRECT-ed from other netdevs. | Acceleration |
| tx[i]\_xdp\_inlnw | Number of WQE data segments where the data could be inlined in the WQE where the data segments were XDP\_REDIRECT-ed from other netdevs. | Acceleration |
| tx[i]\_xdp\_nops | Number of NOP WQEBBs (WQE building blocks) posted to the SQ that were XDP\_REDIRECT-ed from other netdevs. | Acceleration |
| tx[i]\_xdp\_err | The number of packets redirected to the interface(due to XDP redirect) but were dropped due to error such as frame too long and frame too short. | Error |
| tx[i]\_xdp\_cqes | The number of completions received for packets redirected to the interface(due to XDP redirect) on the CQ. | Informative |
| tx[i]\_xsk\_xmit | The number of packets transmitted using XSK zerocopy functionality. | Acceleration |
| tx[i]\_xsk\_mpwqe | Number of multi-packet WQEs offloaded onto the NIC that were XDP\_REDIRECT-ed from other netdevs. | Acceleration |
| tx[i]\_xsk\_inlnw | Number of WQE data segments where the data could be inlined in the WQE that are transmitted using XSK zerocopy. | Acceleration |
| tx[i]\_xsk\_full | Number of times doorbell is rung in XSK zerocopy mode when SQ is full. | Error |
| tx[i]\_xsk\_err | Number of errors that occurred in XSK zerocopy mode such as if the data size is larger than the MTU size. | Error |
| tx[i]\_xsk\_cqes | Number of CQEs processed in XSK zerocopy mode. | Acceleration |
| tx\_tls\_ctx | Number of TLS TX HW offload contexts added to device for encryption. | Acceleration |
| tx\_tls\_del | Number of TLS TX HW offload contexts removed from device (connection closed). | Acceleration |
| tx\_tls\_pool\_alloc | Number of times a unit of work is successfully allocated in the TLS HW offload pool. | Acceleration |
| tx\_tls\_pool\_free | Number of times a unit of work is freed in the TLS HW offload pool. | Acceleration |
| rx\_tls\_ctx | Number of TLS RX HW offload contexts added to device for decryption. | Acceleration |
| rx\_tls\_del | Number of TLS RX HW offload contexts deleted from device (connection has finished). | Acceleration |
| rx[i]\_tls\_decrypted\_packets | Number of successfully decrypted RX packets which were part of a TLS stream. | Acceleration |
| rx[i]\_tls\_decrypted\_bytes | Number of TLS payload bytes in RX packets which were successfully decrypted. | Acceleration |
| rx[i]\_tls\_resync\_req\_pkt | Number of received TLS packets with a resync request. | Acceleration |
| rx[i]\_tls\_resync\_req\_start | Number of times the TLS async resync request was started. | Acceleration |
| rx[i]\_tls\_resync\_req\_end | Number of times the TLS async resync request properly ended with providing the HW tracked tcp-seq. | Acceleration |
| rx[i]\_tls\_resync\_req\_skip | Number of times the TLS async resync request procedure was started but not properly ended. | Error |
| rx[i]\_tls\_resync\_res\_ok | Number of times the TLS resync response call to the driver was successfully handled. | Acceleration |
| rx[i]\_tls\_resync\_res\_retry | Number of times the TLS resync response call to the driver was reattempted when ICOSQ is full. | Error |
| rx[i]\_tls\_resync\_res\_skip | Number of times the TLS resync response call to the driver was terminated unsuccessfully. | Error |
| rx[i]\_tls\_err | Number of times when CQE TLS offload was problematic. | Error |
| tx[i]\_tls\_encrypted\_packets | The number of send packets that are TLS encrypted by the kernel. | Acceleration |
| tx[i]\_tls\_encrypted\_bytes | The number of send bytes that are TLS encrypted by the kernel. | Acceleration |
| tx[i]\_tls\_ooo | Number of times out of order TLS SQE fragments were handled on ring i. | Acceleration |
| tx[i]\_tls\_dump\_packets | Number of TLS decrypted packets copied over from NIC over DMA. | Acceleration |
| tx[i]\_tls\_dump\_bytes | Number of TLS decrypted bytes copied over from NIC over DMA. | Acceleration |
| tx[i]\_tls\_resync\_bytes | Number of TLS bytes requested to be resynchronized in order to be decrypted. | Acceleration |
| tx[i]\_tls\_skip\_no\_sync\_data | Number of TLS send data that can safely be skipped / do not need to be decrypted. | Acceleration |
| tx[i]\_tls\_drop\_no\_sync\_data | Number of TLS send data that were dropped due to retransmission of TLS data. | Acceleration |
| ptp\_cq[i]\_abort | Number of times a CQE has to be skipped in precision time protocol due to a skew between the port timestamp and CQE timestamp being greater than 128 seconds. | Error |
| ptp\_cq[i]\_abort\_abs\_diff\_ns | Accumulation of time differences between the port timestamp and CQE timestamp when the difference is greater than 128 seconds in precision time protocol. | Error |
| ptp\_cq[i]\_late\_cqe | Number of times a CQE has been delivered on the PTP timestamping CQ when the CQE was not expected since a certain amount of time had elapsed where the device typically ensures not posting the CQE. | Error |
| ptp\_cq[i]\_lost\_cqe | Number of times a CQE is expected to not be delivered on the PTP timestamping CQE by the device due to a time delta elapsing. If such a CQE is somehow delivered, ptp\_cq[i]\_late\_cqe is incremented. | Error |

[2]
([1](#id17),[2](#id18),[3](#id19),[4](#id26))

The corresponding ring and global counters do not share the
same name (i.e. do not follow the common naming scheme).

### vPort Counters

Counters on the NIC port that is connected to a eSwitch.

vPort Counter Table

|  |  |  |
| --- | --- | --- |
| Counter | Description | Type |
| rx\_vport\_unicast\_packets | Unicast packets received, steered to a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| rx\_vport\_unicast\_bytes | Unicast bytes received, steered to a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| tx\_vport\_unicast\_packets | Unicast packets transmitted, steered from a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| tx\_vport\_unicast\_bytes | Unicast bytes transmitted, steered from a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| rx\_vport\_multicast\_packets | Multicast packets received, steered to a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| rx\_vport\_multicast\_bytes | Multicast bytes received, steered to a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| tx\_vport\_multicast\_packets | Multicast packets transmitted, steered from a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| tx\_vport\_multicast\_bytes | Multicast bytes transmitted, steered from a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| rx\_vport\_broadcast\_packets | Broadcast packets received, steered to a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| rx\_vport\_broadcast\_bytes | Broadcast bytes received, steered to a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| tx\_vport\_broadcast\_packets | Broadcast packets transmitted, steered from a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| tx\_vport\_broadcast\_bytes | Broadcast bytes transmitted, steered from a port including Raw Ethernet QP/DPDK traffic, excluding RDMA traffic. | Informative |
| rx\_vport\_rdma\_unicast\_packets | RDMA unicast packets received, steered to a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| rx\_vport\_rdma\_unicast\_bytes | RDMA unicast bytes received, steered to a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| tx\_vport\_rdma\_unicast\_packets | RDMA unicast packets transmitted, steered from a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| tx\_vport\_rdma\_unicast\_bytes | RDMA unicast bytes transmitted, steered from a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| rx\_vport\_rdma\_multicast\_packets | RDMA multicast packets received, steered to a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| rx\_vport\_rdma\_multicast\_bytes | RDMA multicast bytes received, steered to a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| tx\_vport\_rdma\_multicast\_packets | RDMA multicast packets transmitted, steered from a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| tx\_vport\_rdma\_multicast\_bytes | RDMA multicast bytes transmitted, steered from a port (counters counts RoCE/UD/RC traffic) [[1]](#accel). | Acceleration |
| vport\_loopback\_packets | Unicast, multicast and broadcast packets that were loop-back (received and transmitted), IB/Eth [[1]](#accel). | Acceleration |
| vport\_loopback\_bytes | Unicast, multicast and broadcast bytes that were loop-back (received and transmitted), IB/Eth [[1]](#accel). | Acceleration |
| rx\_steer\_missed\_packets | Number of packets that was received by the NIC, however was discarded because it did not match any flow in the NIC flow table. | Error |
| rx\_packets | Representor only: packets received, that were handled by the hypervisor. | Informative |
| rx\_bytes | Representor only: bytes received, that were handled by the hypervisor. | Informative |
| tx\_packets | Representor only: packets transmitted, that were handled by the hypervisor. | Informative |
| tx\_bytes | Representor only: bytes transmitted, that were handled by the hypervisor. | Informative |
| dev\_internal\_queue\_oob | The number of dropped packets due to lack of receive WQEs for an internal device RQ. | Error |

### Physical Port Counters

The physical port counters are the counters on the external port connecting the
adapter to the network. This measuring point holds information on standardized
counters like IEEE 802.3, RFC2863, RFC 2819, RFC 3635 and additional counters
like flow control, FEC and more.

Physical Port Counter Table

|  |  |  |
| --- | --- | --- |
| Counter | Description | Type |
| rx\_packets\_phy | The number of packets received on the physical port. This counter doesn’t include packets that were discarded due to FCS, frame size and similar errors. | Informative |
| tx\_packets\_phy | The number of packets transmitted on the physical port. | Informative |
| rx\_bytes\_phy | The number of bytes received on the physical port, including Ethernet header and FCS. | Informative |
| tx\_bytes\_phy | The number of bytes transmitted on the physical port. | Informative |
| rx\_multicast\_phy | The number of multicast packets received on the physical port. | Informative |
| tx\_multicast\_phy | The number of multicast packets transmitted on the physical port. | Informative |
| rx\_broadcast\_phy | The number of broadcast packets received on the physical port. | Informative |
| tx\_broadcast\_phy | The number of broadcast packets transmitted on the physical port. | Informative |
| rx\_crc\_errors\_phy | The number of dropped received packets due to FCS (Frame Check Sequence) error on the physical port. If this counter is increased in high rate, check the link quality using rx\_symbol\_error\_phy and rx\_corrected\_bits\_phy counters below. | Error |
| rx\_in\_range\_len\_errors\_phy | The number of received packets dropped due to length/type errors on a physical port. | Error |
| rx\_out\_of\_range\_len\_phy | The number of received packets dropped due to length greater than allowed on a physical port. If this counter is increasing, it implies that the peer connected to the adapter has a larger MTU configured. Using same MTU configuration shall resolve this issue. | Error |
| rx\_oversize\_pkts\_phy | The number of dropped received packets due to length which exceed MTU size on a physical port. If this counter is increasing, it implies that the peer connected to the adapter has a larger MTU configured. Using same MTU configuration shall resolve this issue. | Error |
| rx\_symbol\_err\_phy | The number of received packets dropped due to physical coding errors (symbol errors) on a physical port. | Error |
| rx\_mac\_control\_phy | The number of MAC control packets received on the physical port. | Informative |
| tx\_mac\_control\_phy | The number of MAC control packets transmitted on the physical port. | Informative |
| rx\_pause\_ctrl\_phy | The number of link layer pause packets received on a physical port. If this counter is increasing, it implies that the network is congested and cannot absorb the traffic coming from to the adapter. | Informative |
| tx\_pause\_ctrl\_phy | The number of link layer pause packets transmitted on a physical port. If this counter is increasing, it implies that the NIC is congested and cannot absorb the traffic coming from the network. | Informative |
| rx\_unsupported\_op\_phy | The number of MAC control packets received with unsupported opcode on a physical port. | Error |
| rx\_discards\_phy | The number of received packets dropped due to lack of buffers on a physical port. If this counter is increasing, it implies that the adapter is congested and cannot absorb the traffic coming from the network. | Error |
| tx\_discards\_phy | The number of packets which were discarded on transmission, even no errors were detected. the drop might occur due to link in down state, head of line drop, pause from the network, etc. | Error |
| tx\_errors\_phy | The number of transmitted packets dropped due to a length which exceed MTU size on a physical port. | Error |
| rx\_undersize\_pkts\_phy | The number of received packets dropped due to length which is shorter than 64 bytes on a physical port. If this counter is increasing, it implies that the peer connected to the adapter has a non-standard MTU configured or malformed packet had arrived. | Error |
| rx\_fragments\_phy | The number of received packets dropped due to a length which is shorter than 64 bytes and has FCS error on a physical port. If this counter is increasing, it implies that the peer connected to the adapter has a non-standard MTU configured. | Error |
| rx\_jabbers\_phy | The number of received packets d due to a length which is longer than 64 bytes and had FCS error on a physical port. | Error |
| rx\_64\_bytes\_phy | The number of packets received on the physical port with size of 64 bytes. | Informative |
| rx\_65\_to\_127\_bytes\_phy | The number of packets received on the physical port with size of 65 to 127 bytes. | Informative |
| rx\_128\_to\_255\_bytes\_phy | The number of packets received on the physical port with size of 128 to 255 bytes. | Informative |
| rx\_256\_to\_511\_bytes\_phy | The number of packets received on the physical port with size of 256 to 512 bytes. | Informative |
| rx\_512\_to\_1023\_bytes\_phy | The number of packets received on the physical port with size of 512 to 1023 bytes. | Informative |
| rx\_1024\_to\_1518\_bytes\_phy | The number of packets received on the physical port with size of 1024 to 1518 bytes. | Informative |
| rx\_1519\_to\_2047\_bytes\_phy | The number of packets received on the physical port with size of 1519 to 2047 bytes. | Informative |
| rx\_2048\_to\_4095\_bytes\_phy | The number of packets received on the physical port with size of 2048 to 4095 bytes. | Informative |
| rx\_4096\_to\_8191\_bytes\_phy | The number of packets received on the physical port with size of 4096 to 8191 bytes. | Informative |
| rx\_8192\_to\_10239\_bytes\_phy | The number of packets received on the physical port with size of 8192 to 10239 bytes. | Informative |
| link\_down\_events\_phy | The number of times where the link operative state changed to down. In case this counter is increasing it may imply on port flapping. You may need to replace the cable/transceiver. | Error |
| total\_success\_recovery\_phy | The number of total successful recovery events of any type during ports reset cycle. | Error |
| rx\_out\_of\_buffer | Number of times receive queue had no software buffers allocated for the adapter’s incoming traffic. | Error |
| module\_bus\_stuck | The number of times that module’s I2C bus (data or clock) short-wire was detected. You may need to replace the cable/transceiver. | Error |
| module\_high\_temp | The number of times that the module temperature was too high. If this issue persist, you may need to check the ambient temperature or replace the cable/transceiver module. | Error |
| module\_bad\_shorted | The number of times that the module cables were shorted. You may need to replace the cable/transceiver module. | Error |
| module\_unplug | The number of times that module was ejected. | Informative |
| rx\_buffer\_passed\_thres\_phy | The number of events where the port receive buffer was over 85% full. | Informative |
| tx\_pause\_storm\_warning\_events | The number of times the device was sending pauses for a long period of time. | Informative |
| tx\_pause\_storm\_error\_events | The number of times the device was sending pauses for a long period of time, reaching time out and disabling transmission of pause frames. on the period where pause frames were disabled, drop could have been occurred. | Error |
| rx[i]\_buff\_alloc\_err | Failed to allocate a buffer to received packet (or SKB) on ring i. | Error |
| rx\_bits\_phy | This counter provides information on the total amount of traffic that could have been received and can be used as a guideline to measure the ratio of errored traffic in rx\_pcs\_symbol\_err\_phy and rx\_corrected\_bits\_phy. | Informative |
| rx\_pcs\_symbol\_err\_phy | This counter counts the number of symbol errors that wasn’t corrected by FEC correction algorithm or that FEC algorithm was not active on this interface. If this counter is increasing, it implies that the link between the NIC and the network is suffering from high BER, and that traffic is lost. You may need to replace the cable/transceiver. The error rate is the number of rx\_pcs\_symbol\_err\_phy divided by the number of rx\_bits\_phy on a specific time frame. | Error |
| rx\_corrected\_bits\_phy | The number of corrected bits on this port according to active FEC (RS/FC). If this counter is increasing, it implies that the link between the NIC and the network is suffering from high BER. The corrected bit rate is the number of rx\_corrected\_bits\_phy divided by the number of rx\_bits\_phy on a specific time frame. | Error |
| rx\_err\_lane\_[l]\_phy | This counter counts the number of physical raw errors per lane l index. The counter counts errors before FEC corrections. If this counter is increasing, it implies that the link between the NIC and the network is suffering from high BER, and that traffic might be lost. You may need to replace the cable/transceiver. Please check in accordance with rx\_corrected\_bits\_phy. | Error |
| rx\_global\_pause | The number of pause packets received on the physical port. If this counter is increasing, it implies that the network is congested and cannot absorb the traffic coming from the adapter. Note: This counter is only enabled when global pause mode is enabled. | Informative |
| rx\_global\_pause\_duration | The duration of pause received (in microSec) on the physical port. The counter represents the time the port did not send any traffic. If this counter is increasing, it implies that the network is congested and cannot absorb the traffic coming from the adapter. Note: This counter is only enabled when global pause mode is enabled. | Informative |
| tx\_global\_pause | The number of pause packets transmitted on a physical port. If this counter is increasing, it implies that the adapter is congested and cannot absorb the traffic coming from the network. Note: This counter is only enabled when global pause mode is enabled. | Informative |
| tx\_global\_pause\_duration | The duration of pause transmitter (in microSec) on the physical port. Note: This counter is only enabled when global pause mode is enabled. | Informative |
| rx\_global\_pause\_transition | The number of times a transition from Xoff to Xon on the physical port has occurred. Note: This counter is only enabled when global pause mode is enabled. | Informative |
| rx\_if\_down\_packets | The number of received packets that were dropped due to interface down. | Informative |

### Priority Port Counters

The following counters are physical port counters that are counted per L2
priority (0-7).

**Note:** p in the counter name represents the priority.

Priority Port Counter Table

|  |  |  |
| --- | --- | --- |
| Counter | Description | Type |
| rx\_prio[p]\_bytes | The number of bytes received with priority p on the physical port. | Informative |
| rx\_prio[p]\_packets | The number of packets received with priority p on the physical port. | Informative |
| tx\_prio[p]\_bytes | The number of bytes transmitted on priority p on the physical port. | Informative |
| tx\_prio[p]\_packets | The number of packets transmitted on priority p on the physical port. | Informative |
| rx\_prio[p]\_pause | The number of pause packets received with priority p on a physical port. If this counter is increasing, it implies that the network is congested and cannot absorb the traffic coming from the adapter. Note: This counter is available only if PFC was enabled on priority p. | Informative |
| rx\_prio[p]\_pause\_duration | The duration of pause received (in microSec) on priority p on the physical port. The counter represents the time the port did not send any traffic on this priority. If this counter is increasing, it implies that the network is congested and cannot absorb the traffic coming from the adapter. Note: This counter is available only if PFC was enabled on priority p. | Informative |
| rx\_prio[p]\_pause\_transition | The number of times a transition from Xoff to Xon on priority p on the physical port has occurred. Note: This counter is available only if PFC was enabled on priority p. | Informative |
| tx\_prio[p]\_pause | The number of pause packets transmitted on priority p on a physical port. If this counter is increasing, it implies that the adapter is congested and cannot absorb the traffic coming from the network. Note: This counter is available only if PFC was enabled on priority p. | Informative |
| tx\_prio[p]\_pause\_duration | The duration of pause transmitter (in microSec) on priority p on the physical port. Note: This counter is available only if PFC was enabled on priority p. | Informative |
| rx\_prio[p]\_buf\_discard | The number of packets discarded by device due to lack of per host receive buffers. | Informative |
| rx\_prio[p]\_cong\_discard | The number of packets discarded by device due to per host congestion. | Informative |
| rx\_prio[p]\_marked | The number of packets ecn marked by device due to per host congestion. | Informative |
| rx\_prio[p]\_discards | The number of packets discarded by device due to lack of receive buffers. | Informative |

### Device Counters

Device Counter Table

|  |  |  |
| --- | --- | --- |
| Counter | Description | Type |
| rx\_pci\_signal\_integrity | Counts physical layer PCIe signal integrity errors, the number of transitions to recovery due to Framing errors and CRC (dlp and tlp). If this counter is raising, try moving the adapter card to a different slot to rule out a bad PCI slot. Validate that you are running with the latest firmware available and latest server BIOS version. | Error |
| tx\_pci\_signal\_integrity | Counts physical layer PCIe signal integrity errors, the number of transition to recovery initiated by the other side (moving to recovery due to getting TS/EIEOS). If this counter is raising, try moving the adapter card to a different slot to rule out a bad PCI slot. Validate that you are running with the latest firmware available and latest server BIOS version. | Error |
| outbound\_pci\_buffer\_overflow | The number of packets dropped due to pci buffer overflow. If this counter is raising in high rate, it might indicate that the receive traffic rate for a host is larger than the PCIe bus and therefore a congestion occurs. | Informative |
| outbound\_pci\_stalled\_rd | The percentage (in the range 0...100) of time within the last second that the NIC had outbound non-posted reads requests but could not perform the operation due to insufficient posted credits. | Informative |
| outbound\_pci\_stalled\_wr | The percentage (in the range 0...100) of time within the last second that the NIC had outbound posted writes requests but could not perform the operation due to insufficient posted credits. | Informative |
| outbound\_pci\_stalled\_rd\_events | The number of seconds where outbound\_pci\_stalled\_rd was above 30%. | Informative |
| outbound\_pci\_stalled\_wr\_events | The number of seconds where outbound\_pci\_stalled\_wr was above 30%. | Informative |
| dev\_out\_of\_buffer | The number of times the device owned queue had not enough buffers allocated. | Error |
| pci\_bw\_inbound\_high | The number of times the device crossed the high inbound pcie bandwidth threshold. To be compared to pci\_bw\_inbound\_low to check if the device is in a congested state. If pci\_bw\_inbound\_high == pci\_bw\_inbound\_low then the device is not congested. If pci\_bw\_inbound\_high > pci\_bw\_inbound\_low then the device is congested. | Informative |
| pci\_bw\_inbound\_low | The number of times the device crossed the low inbound PCIe bandwidth threshold. To be compared to pci\_bw\_inbound\_high to check if the device is in a congested state. If pci\_bw\_inbound\_high == pci\_bw\_inbound\_low then the device is not congested. If pci\_bw\_inbound\_high > pci\_bw\_inbound\_low then the device is congested. | Informative |
| pci\_bw\_outbound\_high | The number of times the device crossed the high outbound pcie bandwidth threshold. To be compared to pci\_bw\_outbound\_low to check if the device is in a congested state. If pci\_bw\_outbound\_high == pci\_bw\_outbound\_low then the device is not congested. If pci\_bw\_outbound\_high > pci\_bw\_outbound\_low then the device is congested. | Informative |
| pci\_bw\_outbound\_low | The number of times the device crossed the low outbound PCIe bandwidth threshold. To be compared to pci\_bw\_outbound\_high to check if the device is in a congested state. If pci\_bw\_outbound\_high == pci\_bw\_outbound\_low then the device is not congested. If pci\_bw\_outbound\_high > pci\_bw\_outbound\_low then the device is congested. | Informative |
| pci\_bw\_stale\_event | The number of times the device fired a PCIe congestion event but on query there was no change in state. | Informative |
