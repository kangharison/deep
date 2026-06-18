# Familynetdevnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/netdev.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `netdev` netlink specification](#id12)

## [Summary](#id13)

netdev configuration over generic netlink.

## [Operations](#id14)

### [dev-get](#id15)

Get / dump information about a netdev.

attribute-set:
:   [dev](#netdev-attribute-set-dev)

do:
:   **request**
    :   attributes:
        :   [`ifindex`]

    **reply**
    :   attributes:
        :   [`ifindex`, `xdp-features`, `xdp-zc-max-segs`, `xdp-rx-metadata-features`, `xsk-features`]

dump:
:   **reply**
    :   attributes:
        :   [`ifindex`, `xdp-features`, `xdp-zc-max-segs`, `xdp-rx-metadata-features`, `xsk-features`]

### [dev-add-ntf](#id16)

Notification about device appearing.

notify:
:   dev-get

mcgrp:
:   mgmt

### [dev-del-ntf](#id17)

Notification about device disappearing.

notify:
:   dev-get

mcgrp:
:   mgmt

### [dev-change-ntf](#id18)

Notification about device configuration being changed.

notify:
:   dev-get

mcgrp:
:   mgmt

### [page-pool-get](#id19)

Get / dump information about Page Pools.
Only Page Pools associated by the driver with a net\_device
can be listed. ifindex will not be reported if the net\_device
no longer exists.

attribute-set:
:   [page-pool](#netdev-attribute-set-page-pool)

config-cond:
:   page-pool

do:
:   **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`, `ifindex`, `napi-id`, `inflight`, `inflight-mem`, `detach-time`, `dmabuf`, `io-uring`]

dump:
:   **reply**
    :   attributes:
        :   [`id`, `ifindex`, `napi-id`, `inflight`, `inflight-mem`, `detach-time`, `dmabuf`, `io-uring`]

### [page-pool-add-ntf](#id20)

Notification about page pool appearing.

notify:
:   page-pool-get

mcgrp:
:   page-pool

config-cond:
:   page-pool

### [page-pool-del-ntf](#id21)

Notification about page pool disappearing.

notify:
:   page-pool-get

mcgrp:
:   page-pool

config-cond:
:   page-pool

### [page-pool-change-ntf](#id22)

Notification about page pool configuration being changed.

notify:
:   page-pool-get

mcgrp:
:   page-pool

config-cond:
:   page-pool

### [page-pool-stats-get](#id23)

Get page pool statistics.

attribute-set:
:   [page-pool-stats](#netdev-attribute-set-page-pool-stats)

config-cond:
:   page-pool-stats

do:
:   **request**
    :   attributes:
        :   [`info`]

    **reply**
    :   attributes:
        :   [`info`, `alloc-fast`, `alloc-slow`, `alloc-slow-high-order`, `alloc-empty`, `alloc-refill`, `alloc-waive`, `recycle-cached`, `recycle-cache-full`, `recycle-ring`, `recycle-ring-full`, `recycle-released-refcnt`]

dump:
:   **reply**
    :   attributes:
        :   [`info`, `alloc-fast`, `alloc-slow`, `alloc-slow-high-order`, `alloc-empty`, `alloc-refill`, `alloc-waive`, `recycle-cached`, `recycle-cache-full`, `recycle-ring`, `recycle-ring-full`, `recycle-released-refcnt`]

### [queue-get](#id24)

Get queue information from the kernel. Only configured queues will be reported (as opposed to all available hardware queues).

attribute-set:
:   [queue](#netdev-attribute-set-queue)

do:
:   **request**
    :   attributes:
        :   [`ifindex`, `type`, `id`]

    **reply**
    :   attributes:
        :   [`id`, `type`, `napi-id`, `ifindex`, `dmabuf`, `io-uring`, `xsk`, `lease`]

dump:
:   **request**
    :   attributes:
        :   [`ifindex`]

    **reply**
    :   attributes:
        :   [`id`, `type`, `napi-id`, `ifindex`, `dmabuf`, `io-uring`, `xsk`, `lease`]

### [napi-get](#id25)

Get information about NAPI instances configured on the system.

attribute-set:
:   [napi](#netdev-attribute-set-napi)

do:
:   **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`, `ifindex`, `irq`, `pid`, `defer-hard-irqs`, `gro-flush-timeout`, `irq-suspend-timeout`, `threaded`]

dump:
:   **request**
    :   attributes:
        :   [`ifindex`]

    **reply**
    :   attributes:
        :   [`id`, `ifindex`, `irq`, `pid`, `defer-hard-irqs`, `gro-flush-timeout`, `irq-suspend-timeout`, `threaded`]

### [qstats-get](#id26)

Get / dump fine grained statistics. Which statistics are reported
depends on the device and the driver, and whether the driver stores
software counters per-queue.

attribute-set:
:   [qstats](#netdev-attribute-set-qstats)

dump:
:   **request**
    :   attributes:
        :   [`ifindex`, `scope`]

    **reply**
    :   attributes:
        :   [`ifindex`, `queue-type`, `queue-id`, `rx-packets`, `rx-bytes`, `tx-packets`, `tx-bytes`, `rx-alloc-fail`, `rx-hw-drops`, `rx-hw-drop-overruns`, `rx-csum-complete`, `rx-csum-unnecessary`, `rx-csum-none`, `rx-csum-bad`, `rx-hw-gro-packets`, `rx-hw-gro-bytes`, `rx-hw-gro-wire-packets`, `rx-hw-gro-wire-bytes`, `rx-hw-drop-ratelimits`, `tx-hw-drops`, `tx-hw-drop-errors`, `tx-csum-none`, `tx-needs-csum`, `tx-hw-gso-packets`, `tx-hw-gso-bytes`, `tx-hw-gso-wire-packets`, `tx-hw-gso-wire-bytes`, `tx-hw-drop-ratelimits`, `tx-stop`, `tx-wake`]

### [bind-rx](#id27)

Bind dmabuf to netdev

attribute-set:
:   [dmabuf](#netdev-attribute-set-dmabuf)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`ifindex`, `fd`, `queues`]

    **reply**
    :   attributes:
        :   [`id`]

### [napi-set](#id28)

Set configurable NAPI instance settings.

attribute-set:
:   [napi](#netdev-attribute-set-napi)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`id`, `defer-hard-irqs`, `gro-flush-timeout`, `irq-suspend-timeout`, `threaded`]

### [bind-tx](#id29)

Bind dmabuf to netdev for TX

attribute-set:
:   [dmabuf](#netdev-attribute-set-dmabuf)

do:
:   **request**
    :   attributes:
        :   [`ifindex`, `fd`]

    **reply**
    :   attributes:
        :   [`id`]

### [queue-create](#id30)

Create a new queue for the given netdevice. Whether this operation
is supported depends on the device and the driver.

attribute-set:
:   [queue](#netdev-attribute-set-queue)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`ifindex`, `type`, `lease`]

    **reply**
    :   attributes:
        :   [`id`]

## [Multicast groups](#id31)

* mgmt
* page-pool

## [Definitions](#id32)

### [xdp-act](#id33)

type:
:   flags

entries:
:   basic:
    :   XDP features set supported by all drivers (XDP\_ABORTED, XDP\_DROP, XDP\_PASS, XDP\_TX)

    redirect:
    :   The netdev supports XDP\_REDIRECT

    ndo-xmit:
    :   This feature informs if netdev implements ndo\_xdp\_xmit callback.

    xsk-zerocopy:
    :   This feature informs if netdev supports AF\_XDP in zero copy mode.

    hw-offload:
    :   This feature informs if netdev supports XDP hw offloading.

    rx-sg:
    :   This feature informs if netdev implements non-linear XDP buffer support in the driver napi callback.

    ndo-xmit-sg:
    :   This feature informs if netdev implements non-linear XDP buffer support in ndo\_xdp\_xmit callback.

### [xdp-rx-metadata](#id34)

type:
:   flags

entries:
:   timestamp:
    :   Device is capable of exposing receive HW timestamp via [`bpf_xdp_metadata_rx_timestamp()`](../../networking/xdp-rx-metadata.html#c.bpf_xdp_metadata_rx_timestamp "bpf_xdp_metadata_rx_timestamp").

    hash:
    :   Device is capable of exposing receive packet hash via [`bpf_xdp_metadata_rx_hash()`](../../networking/xdp-rx-metadata.html#c.bpf_xdp_metadata_rx_hash "bpf_xdp_metadata_rx_hash").

    vlan-tag:
    :   Device is capable of exposing receive packet VLAN tag via [`bpf_xdp_metadata_rx_vlan_tag()`](../../networking/xdp-rx-metadata.html#c.bpf_xdp_metadata_rx_vlan_tag "bpf_xdp_metadata_rx_vlan_tag").

### [xsk-flags](#id35)

type:
:   flags

entries:
:   tx-timestamp:
    :   HW timestamping egress packets is supported by the driver.

    tx-checksum:
    :   L3 checksum HW offload is supported by the driver.

    tx-launch-time-fifo:
    :   Launch time HW offload is supported by the driver.

### [queue-type](#id36)

type:
:   enum

entries:
:   * `rx`
    * `tx`

### [qstats-scope](#id37)

type:
:   flags

entries:
:   * `queue`

### [napi-threaded](#id38)

type:
:   enum

entries:
:   * `disabled`
    * `enabled`
    * `busy-poll`

## [Attribute sets](#id39)

### [dev](#id40)

#### ifindex (`u32`)

doc:
:   netdev ifindex

#### pad (`pad`)

#### xdp-features (`u64`)

doc:
:   Bitmask of enabled xdp-features.

enum:
:   [xdp-act](#netdev-definition-xdp-act)

#### xdp-zc-max-segs (`u32`)

doc:
:   max fragment count supported by ZC driver

#### xdp-rx-metadata-features (`u64`)

doc:
:   Bitmask of supported XDP receive metadata features. See [XDP RX Metadata](../../networking/xdp-rx-metadata.html) for more details.

enum:
:   [xdp-rx-metadata](#netdev-definition-xdp-rx-metadata)

#### xsk-features (`u64`)

doc:
:   Bitmask of enabled AF\_XDP features.

enum:
:   [xsk-flags](#netdev-definition-xsk-flags)

### [io-uring-provider-info](#id41)

### [page-pool](#id42)

#### id (`uint`)

doc:
:   Unique ID of a Page Pool instance.

#### ifindex (`u32`)

doc:
:   ifindex of the netdev to which the pool belongs. May not be reported if the page pool was allocated for a netdev which got destroyed already (page pools may outlast their netdevs because they wait for all memory to be returned).

#### napi-id (`uint`)

doc:
:   Id of NAPI using this Page Pool instance.

#### inflight (`uint`)

doc:
:   Number of outstanding references to this page pool (allocated but yet to be freed pages). Allocated pages may be held in socket receive queues, driver receive ring, page pool recycling ring, the page pool cache, etc.

#### inflight-mem (`uint`)

doc:
:   Amount of memory held by inflight pages.

#### detach-time (`uint`)

doc:
:   Seconds in CLOCK\_BOOTTIME of when Page Pool was detached by the driver. Once detached Page Pool can no longer be used to allocate memory. Page Pools wait for all the memory allocated from them to be freed before truly disappearing. “Detached” Page Pools cannot be “re-attached”, they are just waiting to disappear. Attribute is absent if Page Pool has not been detached, and can still be used to allocate new memory.

#### dmabuf (`u32`)

doc:
:   ID of the dmabuf this page-pool is attached to.

#### io-uring (`nest`)

doc:
:   io-uring memory provider information.

nested-attributes:
:   [io-uring-provider-info](#netdev-attribute-set-io-uring-provider-info)

### [page-pool-info](#id43)

#### id

#### ifindex

### [page-pool-stats](#id44)

Page pool statistics, see docs for [`struct page_pool_stats`](../../networking/page_pool.html#c.page_pool_stats "page_pool_stats")
for information about individual statistics.

#### info (`nest`)

doc:
:   Page pool identifying information.

nested-attributes:
:   [page-pool-info](#netdev-attribute-set-page-pool-info)

#### alloc-fast (`uint`)

value:
:   8

#### alloc-slow (`uint`)

#### alloc-slow-high-order (`uint`)

#### alloc-empty (`uint`)

#### alloc-refill (`uint`)

#### alloc-waive (`uint`)

#### recycle-cached (`uint`)

#### recycle-cache-full (`uint`)

#### recycle-ring (`uint`)

#### recycle-ring-full (`uint`)

#### recycle-released-refcnt (`uint`)

### [napi](#id45)

#### ifindex (`u32`)

doc:
:   ifindex of the netdevice to which NAPI instance belongs.

#### id (`u32`)

doc:
:   ID of the NAPI instance.

#### irq (`u32`)

doc:
:   The associated interrupt vector number for the napi

#### pid (`u32`)

doc:
:   PID of the napi thread, if NAPI is configured to operate in threaded mode. If NAPI is not in threaded mode (i.e. uses normal softirq context), the attribute will be absent.

#### defer-hard-irqs (`u32`)

doc:
:   The number of consecutive empty polls before IRQ deferral ends and hardware IRQs are re-enabled.

#### gro-flush-timeout (`uint`)

doc:
:   The timeout, in nanoseconds, of when to trigger the NAPI watchdog timer which schedules NAPI processing. Additionally, a non-zero value will also prevent GRO from flushing recent super-frames at the end of a NAPI cycle. This may add receive latency in exchange for reducing the number of frames processed by the network stack.

#### irq-suspend-timeout (`uint`)

doc:
:   The timeout, in nanoseconds, of how long to suspend irq processing, if event polling finds events

#### threaded (`u32`)

doc:
:   Whether the NAPI is configured to operate in threaded polling mode. If this is set to enabled then the NAPI context operates in threaded polling mode. If this is set to busy-poll, then the threaded polling mode also busy polls.

enum:
:   [napi-threaded](#netdev-definition-napi-threaded)

### [xsk-info](#id46)

### [queue](#id47)

#### id (`u32`)

doc:
:   Queue index; most queue types are indexed like a C array, with indexes starting at 0 and ending at queue count - 1. Queue indexes are scoped to an interface and queue type.

#### ifindex (`u32`)

doc:
:   ifindex of the netdevice to which the queue belongs.

#### type (`u32`)

doc:
:   Queue type as rx, tx. Each queue type defines a separate ID space. XDP TX queues allocated in the kernel are not linked to NAPIs and thus not listed. AF\_XDP queues will have more information set in the xsk attribute.

enum:
:   [queue-type](#netdev-definition-queue-type)

#### napi-id (`u32`)

doc:
:   ID of the NAPI instance which services this queue.

#### dmabuf (`u32`)

doc:
:   ID of the dmabuf attached to this queue, if any.

#### io-uring (`nest`)

doc:
:   io\_uring memory provider information.

nested-attributes:
:   [io-uring-provider-info](#netdev-attribute-set-io-uring-provider-info)

#### xsk (`nest`)

doc:
:   XSK information for this queue, if any.

nested-attributes:
:   [xsk-info](#netdev-attribute-set-xsk-info)

#### lease (`nest`)

doc:
:   A queue from a virtual device can have a lease which refers to another queue from a physical device. This is useful for memory providers and AF\_XDP operations which take an ifindex and queue id to allow applications to bind against virtual devices in containers.

nested-attributes:
:   [lease](#netdev-attribute-set-lease)

### [qstats](#id48)

Get device statistics, scoped to a device or a queue.
These statistics extend (and partially duplicate) statistics available
in [`struct rtnl_link_stats64`](../../networking/statistics.html#c.rtnl_link_stats64 "rtnl_link_stats64").
Value of the scope attribute determines how statistics are
aggregated. When aggregated for the entire device the statistics
represent the total number of events since last explicit reset of
the device (i.e. not a reconfiguration like changing queue count).
When reported per-queue, however, the statistics may not add
up to the total number of events, will only be reported for currently
active objects, and will likely report the number of events since last
reconfiguration.

#### ifindex (`u32`)

doc:
:   ifindex of the netdevice to which stats belong.

#### queue-type (`u32`)

doc:
:   Queue type as rx, tx, for queue-id.

enum:
:   [queue-type](#netdev-definition-queue-type)

#### queue-id (`u32`)

doc:
:   Queue ID, if stats are scoped to a single queue instance.

#### scope (`uint`)

doc:
:   What object type should be used to iterate over the stats.

enum:
:   [qstats-scope](#netdev-definition-qstats-scope)

#### rx-packets (`uint`)

doc:
:   Number of wire packets successfully received and passed to the stack. For drivers supporting XDP, XDP is considered the first layer of the stack, so packets consumed by XDP are still counted here.

value:
:   8

#### rx-bytes (`uint`)

doc:
:   Successfully received bytes, see rx-packets.

#### tx-packets (`uint`)

doc:
:   Number of wire packets successfully sent. Packet is considered to be successfully sent once it is in device memory (usually this means the device has issued a DMA completion for the packet).

#### tx-bytes (`uint`)

doc:
:   Successfully sent bytes, see tx-packets.

#### rx-alloc-fail (`uint`)

doc:
:   Number of times skb or buffer allocation failed on the Rx datapath. Allocation failure may, or may not result in a packet drop, depending on driver implementation and whether system recovers quickly.

#### rx-hw-drops (`uint`)

doc:
:   Number of all packets which entered the device, but never left it, including but not limited to: packets dropped due to lack of buffer space, processing errors, explicit or implicit policies and packet filters.

#### rx-hw-drop-overruns (`uint`)

doc:
:   Number of packets dropped due to transient lack of resources, such as buffer space, host descriptors etc.

#### rx-csum-complete (`uint`)

doc:
:   Number of packets that were marked as CHECKSUM\_COMPLETE.

#### rx-csum-unnecessary (`uint`)

doc:
:   Number of packets that were marked as CHECKSUM\_UNNECESSARY.

#### rx-csum-none (`uint`)

doc:
:   Number of packets that were not checksummed by device.

#### rx-csum-bad (`uint`)

doc:
:   Number of packets with bad checksum. The packets are not discarded, but still delivered to the stack.

#### rx-hw-gro-packets (`uint`)

doc:
:   Number of packets that were coalesced from smaller packets by the device. Counts only packets coalesced with the HW-GRO netdevice feature, LRO-coalesced packets are not counted.

#### rx-hw-gro-bytes (`uint`)

doc:
:   See rx-hw-gro-packets.

#### rx-hw-gro-wire-packets (`uint`)

doc:
:   Number of packets that were coalesced to bigger packetss with the HW-GRO netdevice feature. LRO-coalesced packets are not counted.

#### rx-hw-gro-wire-bytes (`uint`)

doc:
:   See rx-hw-gro-wire-packets.

#### rx-hw-drop-ratelimits (`uint`)

doc:
:   Number of the packets dropped by the device due to the received packets bitrate exceeding the device rate limit.

#### tx-hw-drops (`uint`)

doc:
:   Number of packets that arrived at the device but never left it, encompassing packets dropped for reasons such as processing errors, as well as those affected by explicitly defined policies and packet filtering criteria.

#### tx-hw-drop-errors (`uint`)

doc:
:   Number of packets dropped because they were invalid or malformed.

#### tx-csum-none (`uint`)

doc:
:   Number of packets that did not require the device to calculate the checksum.

#### tx-needs-csum (`uint`)

doc:
:   Number of packets that required the device to calculate the checksum. This counter includes the number of GSO wire packets for which device calculated the L4 checksum.

#### tx-hw-gso-packets (`uint`)

doc:
:   Number of packets that necessitated segmentation into smaller packets by the device.

#### tx-hw-gso-bytes (`uint`)

doc:
:   See tx-hw-gso-packets.

#### tx-hw-gso-wire-packets (`uint`)

doc:
:   Number of wire-sized packets generated by processing tx-hw-gso-packets

#### tx-hw-gso-wire-bytes (`uint`)

doc:
:   See tx-hw-gso-wire-packets.

#### tx-hw-drop-ratelimits (`uint`)

doc:
:   Number of the packets dropped by the device due to the transmit packets bitrate exceeding the device rate limit.

#### tx-stop (`uint`)

doc:
:   Number of times driver paused accepting new tx packets from the stack to this queue, because the queue was full. Note that if BQL is supported and enabled on the device the networking stack will avoid queuing a lot of data at once.

#### tx-wake (`uint`)

doc:
:   Number of times driver re-started accepting send requests to this queue from the stack.

### [queue-id](#id49)

#### id

#### type

### [lease](#id50)

#### ifindex (`u32`)

doc:
:   The netdev ifindex to lease the queue from.

#### queue (`nest`)

doc:
:   The netdev queue to lease from.

nested-attributes:
:   [queue-id](#netdev-attribute-set-queue-id)

#### netns-id (`s32`)

doc:
:   The network namespace id of the netdev.

### [dmabuf](#id51)

#### ifindex (`u32`)

doc:
:   netdev ifindex to bind the dmabuf to.

#### queues (`nest`)

doc:
:   receive queues to bind the dmabuf to.

nested-attributes:
:   [queue-id](#netdev-attribute-set-queue-id)

multi-attr:
:   True

#### fd (`u32`)

doc:
:   dmabuf file descriptor to bind.

#### id (`u32`)

doc:
:   id of the dmabuf binding
