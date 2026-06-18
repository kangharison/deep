# Firewire (IEEE 1394) driver Interface Guide

> 출처(원문): https://docs.kernel.org/driver-api/firewire.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Firewire (IEEE 1394) driver Interface Guide

## Introduction and Overview

The Linux FireWire subsystem adds some interfaces into the Linux system to
:   use/maintain+any resource on IEEE 1394 bus.

The main purpose of these interfaces is to access address space on each node
on IEEE 1394 bus by ISO/IEC 13213 (IEEE 1212) procedure, and to control
isochronous resources on the bus by IEEE 1394 procedure.

Two types of interfaces are added, according to consumers of the interface. A
set of userspace interfaces is available via firewire character devices. A set
of kernel interfaces is available via exported symbols in firewire-core module.

## Firewire char device data structures

```
What:           /dev/fw[0-9]+
Date:           May 2007
KernelVersion:  2.6.22
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                The character device files /dev/fw* are the interface between
                firewire-core and IEEE 1394 device drivers implemented in
                userspace.  The ioctl(2)- and read(2)-based ABI is defined and
                documented in <linux/firewire-cdev.h>.

                This ABI offers most of the features which firewire-core also
                exposes to kernelspace IEEE 1394 drivers.

                Each /dev/fw* is associated with one IEEE 1394 node, which can
                be remote or local nodes.  Operations on a /dev/fw* file have
                different scope:

                  - The 1394 node which is associated with the file:

                          - Asynchronous request transmission
                          - Get the Configuration ROM
                          - Query node ID
                          - Query maximum speed of the path between this node
                            and local node

                  - The 1394 bus (i.e. "card") to which the node is attached to:

                          - Isochronous stream transmission and reception
                          - Asynchronous stream transmission and reception
                          - Asynchronous broadcast request transmission
                          - PHY packet transmission and reception
                          - Allocate, reallocate, deallocate isochronous
                            resources (channels, bandwidth) at the bus's IRM
                          - Query node IDs of local node, root node, IRM, bus
                            manager
                          - Query cycle time
                          - Bus reset initiation, bus reset event reception

                  - All 1394 buses:

                          - Allocation of IEEE 1212 address ranges on the local
                            link layers, reception of inbound requests to such
                            an address range, asynchronous response transmission
                            to inbound requests
                          - Addition of descriptors or directories to the local
                            nodes' Configuration ROM

                Due to the different scope of operations and in order to let
                userland implement different access permission models, some
                operations are restricted to /dev/fw* files that are associated
                with a local node:

                          - Addition of descriptors or directories to the local
                            nodes' Configuration ROM
                          - PHY packet transmission and reception

                A /dev/fw* file remains associated with one particular node
                during its entire life time.  Bus topology changes, and hence
                node ID changes, are tracked by firewire-core.  ABI users do not
                need to be aware of topology.

                The following file operations are supported:

                open(2)
                    Currently the only useful flags are O_RDWR.

                ioctl(2)
                    Initiate various actions.  Some take immediate effect, others
                    are performed asynchronously while or after the ioctl returns.
                    See the inline documentation in <linux/firewire-cdev.h> for
                    descriptions of all ioctls.

                poll(2), select(2), epoll_wait(2) etc.
                    Watch for events to become available to be read.

                read(2)
                    Receive various events.  There are solicited events like
                    outbound asynchronous transaction completion or isochronous
                    buffer completion, and unsolicited events such as bus resets,
                    request reception, or PHY packet reception.  Always use a read
                    buffer which is large enough to receive the largest event that
                    could ever arrive.  See <linux/firewire-cdev.h> for descriptions
                    of all event types and for which ioctls affect reception of
                    events.

                mmap(2)
                    Allocate a DMA buffer for isochronous reception or transmission
                    and map it into the process address space.  The arguments should
                    be used as follows:  addr = NULL, length = the desired buffer
                    size, i.e. number of packets times size of largest packet,
                    prot = at least PROT_READ for reception and at least PROT_WRITE
                    for transmission, flags = MAP_SHARED, fd = the handle to the
                    /dev/fw*, offset = 0.

                Isochronous reception works in packet-per-buffer fashion except
                for multichannel reception which works in buffer-fill mode.

                munmap(2)
                    Unmap the isochronous I/O buffer from the process address space.

                close(2)
                    Besides stopping and freeing I/O contexts that were associated
                    with the file descriptor, back out any changes to the local
                    nodes' Configuration ROM.  Deallocate isochronous channels and
                    bandwidth at the IRM that were marked for kernel-assisted
                    re- and deallocation.

Users:          libraw1394;
                libdc1394;
                libhinawa;
                tools like linux-firewire-utils, fwhack, ...
```

struct fw\_cdev\_event\_common
:   Common part of all fw\_cdev\_event\_\* types

**Definition**:

```
struct fw_cdev_event_common {
    __u64 closure;
    __u32 type;
};
```

**Members**

`closure`
:   For arbitrary use by userspace

`type`
:   Discriminates the fw\_cdev\_event\_\* types

**Description**

This `struct may` be used to access generic members of all fw\_cdev\_event\_\*
types regardless of the specific type.

Data passed in the **closure** field for a request will be returned in the
corresponding event. It is big enough to hold a pointer on all platforms.
The ioctl used to set **closure** depends on the **type** of event.

struct fw\_cdev\_event\_bus\_reset
:   Sent when a bus reset occurred

**Definition**:

```
struct fw_cdev_event_bus_reset {
    __u64 closure;
    __u32 type;
    __u32 node_id;
    __u32 local_node_id;
    __u32 bm_node_id;
    __u32 irm_node_id;
    __u32 root_node_id;
    __u32 generation;
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_GET_INFO` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_BUS_RESET`

`node_id`
:   New node ID of this node

`local_node_id`
:   Node ID of the local node, i.e. of the controller

`bm_node_id`
:   Node ID of the bus manager

`irm_node_id`
:   Node ID of the iso resource manager

`root_node_id`
:   Node ID of the root node

`generation`
:   New bus generation

**Description**

This event is sent when the bus the device belongs to goes through a bus
reset. It provides information about the new bus configuration, such as
new node ID for this device, new root ID, and others.

If **bm\_node\_id** is 0xffff right after bus reset it can be reread by an
`FW_CDEV_IOC_GET_INFO` ioctl after bus manager selection was finished.
Kernels with ABI version < 4 do not set **bm\_node\_id**.

struct fw\_cdev\_event\_response
:   Sent when a response packet was received

**Definition**:

```
struct fw_cdev_event_response {
    __u64 closure;
    __u32 type;
    __u32 rcode;
    __u32 length;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_SEND_REQUEST`
    or `FW_CDEV_IOC_SEND_BROADCAST_REQUEST`
    or `FW_CDEV_IOC_SEND_STREAM_PACKET` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_RESPONSE`

`rcode`
:   Response code returned by the remote node

`length`
:   Data length, i.e. the response’s payload size in bytes

`data`
:   Payload data, if any

**Description**

This event is sent instead of [`fw_cdev_event_response`](#c.fw_cdev_event_response "fw_cdev_event_response") if the kernel or the client implements
ABI version <= 5. It has the lack of time stamp field comparing to [`fw_cdev_event_response2`](#c.fw_cdev_event_response2 "fw_cdev_event_response2").

struct fw\_cdev\_event\_response2
:   Sent when a response packet was received

**Definition**:

```
struct fw_cdev_event_response2 {
    __u64 closure;
    __u32 type;
    __u32 rcode;
    __u32 length;
    __u32 request_tstamp;
    __u32 response_tstamp;
    __u32 padding;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_SEND_REQUEST`
    or `FW_CDEV_IOC_SEND_BROADCAST_REQUEST`
    or `FW_CDEV_IOC_SEND_STREAM_PACKET` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_RESPONSE`

`rcode`
:   Response code returned by the remote node

`length`
:   Data length, i.e. the response’s payload size in bytes

`request_tstamp`
:   The time stamp of isochronous cycle at which the request was sent.

`response_tstamp`
:   The time stamp of isochronous cycle at which the response was sent.

`padding`
:   Padding to keep the size of structure as multiples of 8 in various architectures
    since 4 byte alignment is used for 8 byte of object type in System V ABI for i386
    architecture.

`data`
:   Payload data, if any

**Description**

This event is sent when the stack receives a response to an outgoing request
sent by `FW_CDEV_IOC_SEND_REQUEST` ioctl. The payload data for responses
carrying data (read and lock responses) follows immediately and can be
accessed through the **data** field.

The event is also generated after conclusions of transactions that do not
involve response packets. This includes unified write transactions,
broadcast write transactions, and transmission of asynchronous stream
packets. **rcode** indicates success or failure of such transmissions.

The value of **request\_tstamp** expresses the isochronous cycle at which the request was sent to
initiate the transaction. The value of **response\_tstamp** expresses the isochronous cycle at which
the response arrived to complete the transaction. Each value is unsigned 16 bit integer
containing three low order bits of second field and all 13 bits of cycle field in format of
CYCLE\_TIMER register.

struct fw\_cdev\_event\_request
:   Old version of [`fw_cdev_event_request2`](#c.fw_cdev_event_request2 "fw_cdev_event_request2")

**Definition**:

```
struct fw_cdev_event_request {
    __u64 closure;
    __u32 type;
    __u32 tcode;
    __u64 offset;
    __u32 handle;
    __u32 length;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_ALLOCATE` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_REQUEST`

`tcode`
:   Transaction code of the incoming request

`offset`
:   The offset into the 48-bit per-node address space

`handle`
:   Reference to the kernel-side pending request

`length`
:   Data length, i.e. the request’s payload size in bytes

`data`
:   Incoming data, if any

**Description**

This event is sent instead of [`fw_cdev_event_request2`](#c.fw_cdev_event_request2 "fw_cdev_event_request2") if the kernel or
the client implements ABI version <= 3. [`fw_cdev_event_request`](#c.fw_cdev_event_request "fw_cdev_event_request") lacks
essential information; use [`fw_cdev_event_request2`](#c.fw_cdev_event_request2 "fw_cdev_event_request2") instead.

struct fw\_cdev\_event\_request2
:   Sent on incoming request to an address region

**Definition**:

```
struct fw_cdev_event_request2 {
    __u64 closure;
    __u32 type;
    __u32 tcode;
    __u64 offset;
    __u32 source_node_id;
    __u32 destination_node_id;
    __u32 card;
    __u32 generation;
    __u32 handle;
    __u32 length;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_ALLOCATE` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_REQUEST2`

`tcode`
:   Transaction code of the incoming request

`offset`
:   The offset into the 48-bit per-node address space

`source_node_id`
:   Sender node ID

`destination_node_id`
:   Destination node ID

`card`
:   The index of the card from which the request came

`generation`
:   Bus generation in which the request is valid

`handle`
:   Reference to the kernel-side pending request

`length`
:   Data length, i.e. the request’s payload size in bytes

`data`
:   Incoming data, if any

**Description**

This event is sent instead of [`fw_cdev_event_request3`](#c.fw_cdev_event_request3 "fw_cdev_event_request3") if the kernel or the client implements
ABI version <= 5. It has the lack of time stamp field comparing to [`fw_cdev_event_request3`](#c.fw_cdev_event_request3 "fw_cdev_event_request3").

struct fw\_cdev\_event\_request3
:   Sent on incoming request to an address region

**Definition**:

```
struct fw_cdev_event_request3 {
    __u64 closure;
    __u32 type;
    __u32 tcode;
    __u64 offset;
    __u32 source_node_id;
    __u32 destination_node_id;
    __u32 card;
    __u32 generation;
    __u32 handle;
    __u32 length;
    __u32 tstamp;
    __u32 padding;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_ALLOCATE` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_REQUEST2`

`tcode`
:   Transaction code of the incoming request

`offset`
:   The offset into the 48-bit per-node address space

`source_node_id`
:   Sender node ID

`destination_node_id`
:   Destination node ID

`card`
:   The index of the card from which the request came

`generation`
:   Bus generation in which the request is valid

`handle`
:   Reference to the kernel-side pending request

`length`
:   Data length, i.e. the request’s payload size in bytes

`tstamp`
:   The time stamp of isochronous cycle at which the request arrived.

`padding`
:   Padding to keep the size of structure as multiples of 8 in various architectures
    since 4 byte alignment is used for 8 byte of object type in System V ABI for i386
    architecture.

`data`
:   Incoming data, if any

**Description**

This event is sent when the stack receives an incoming request to an address
region registered using the `FW_CDEV_IOC_ALLOCATE` ioctl. The request is
guaranteed to be completely contained in the specified region. Userspace is
responsible for sending the response by `FW_CDEV_IOC_SEND_RESPONSE` ioctl,
using the same **handle**.

The payload data for requests carrying data (write and lock requests)
follows immediately and can be accessed through the **data** field.

Unlike [`fw_cdev_event_request`](#c.fw_cdev_event_request "fw_cdev_event_request"), **tcode** of lock requests is one of the
firewire-core specific `TCODE_LOCK_MASK_SWAP`...``TCODE\_LOCK\_VENDOR\_DEPENDENT``,
i.e. encodes the extended transaction code.

**card** may differ from [`fw_cdev_get_info.card`](#c.fw_cdev_get_info "fw_cdev_get_info") because requests are received
from all cards of the Linux host. **source\_node\_id**, **destination\_node\_id**, and
**generation** pertain to that card. Destination node ID and bus generation may
therefore differ from the corresponding fields of the last
[`fw_cdev_event_bus_reset`](#c.fw_cdev_event_bus_reset "fw_cdev_event_bus_reset").

**destination\_node\_id** may also differ from the current node ID because of a
non-local bus ID part or in case of a broadcast write request. Note, a
client must call an `FW_CDEV_IOC_SEND_RESPONSE` ioctl even in case of a
broadcast write request; the kernel will then release the kernel-side pending
request but will not actually send a response packet.

In case of a write request to FCP\_REQUEST or FCP\_RESPONSE, the kernel already
sent a write response immediately after the request was received; in this
case the client must still call an `FW_CDEV_IOC_SEND_RESPONSE` ioctl to
release the kernel-side pending request, though another response won’t be
sent.

If the client subsequently needs to initiate requests to the sender node of
an [`fw_cdev_event_request3`](#c.fw_cdev_event_request3 "fw_cdev_event_request3"), it needs to use a device file with matching
card index, node ID, and generation for outbound requests.

**tstamp** is isochronous cycle at which the request arrived. It is 16 bit integer value and the
higher 3 bits expresses three low order bits of second field in the format of CYCLE\_TIME
register and the rest 13 bits expresses cycle field.

struct fw\_cdev\_event\_iso\_interrupt
:   Sent when an iso packet was completed

**Definition**:

```
struct fw_cdev_event_iso_interrupt {
    __u64 closure;
    __u32 type;
    __u32 cycle;
    __u32 header_length;
    __u32 header[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common");
    set by `FW_CDEV_CREATE_ISO_CONTEXT` ioctl

`type`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); always `FW_CDEV_EVENT_ISO_INTERRUPT`

`cycle`
:   Cycle counter of the last completed packet

`header_length`
:   Total length of following headers, in bytes

`header`
:   Stripped headers, if any

**Description**

This event is sent when the controller has completed an [`fw_cdev_iso_packet`](#c.fw_cdev_iso_packet "fw_cdev_iso_packet")
with the `FW_CDEV_ISO_INTERRUPT` bit set, when explicitly requested with
`FW_CDEV_IOC_FLUSH_ISO`, or when there have been so many completed packets
without the interrupt bit set that the kernel’s internal buffer for **header**
is about to overflow. (In the last case, ABI versions < 5 drop header data
up to the next interrupt packet.)

Isochronous transmit events (context type `FW_CDEV_ISO_CONTEXT_TRANSMIT`):

In version 3 and some implementations of version 2 of the ABI, `header_length`
is a multiple of 4 and `header` contains timestamps of all packets up until
the interrupt packet. The format of the timestamps is as described below for
isochronous reception. In version 1 of the ABI, `header_length` was 0.

Isochronous receive events (context type `FW_CDEV_ISO_CONTEXT_RECEIVE`):

The headers stripped of all packets up until and including the interrupt
packet are returned in the **header** field. The amount of header data per
packet is as specified at iso context creation by
[`fw_cdev_create_iso_context.header_size`](#c.fw_cdev_create_iso_context "fw_cdev_create_iso_context").

Hence, \_interrupt.header\_length / \_context.header\_size is the number of
packets received in this interrupt event. The client can now iterate
through the mmap()’ed DMA buffer according to this number of packets and
to the buffer sizes as the client specified in [`fw_cdev_queue_iso`](#c.fw_cdev_queue_iso "fw_cdev_queue_iso").

Since version 2 of this ABI, the portion for each packet in \_interrupt.header
consists of the 1394 isochronous packet header, followed by a timestamp
quadlet if [`fw_cdev_create_iso_context.header_size`](#c.fw_cdev_create_iso_context "fw_cdev_create_iso_context") > 4, followed by quadlets
from the packet payload if [`fw_cdev_create_iso_context.header_size`](#c.fw_cdev_create_iso_context "fw_cdev_create_iso_context") > 8.

Format of 1394 iso packet header: 16 bits data\_length, 2 bits tag, 6 bits
channel, 4 bits tcode, 4 bits sy, in big endian byte order.
data\_length is the actual received size of the packet without the four
1394 iso packet header bytes.

Format of timestamp: 16 bits invalid, 3 bits cycleSeconds, 13 bits
cycleCount, in big endian byte order.

In version 1 of the ABI, no timestamp quadlet was inserted; instead, payload
data followed directly after the 1394 is header if header\_size > 4.
Behaviour of ver. 1 of this ABI is no longer available since ABI ver. 2.

struct fw\_cdev\_event\_iso\_interrupt\_mc
:   An iso buffer chunk was completed

**Definition**:

```
struct fw_cdev_event_iso_interrupt_mc {
    __u64 closure;
    __u32 type;
    __u32 completed;
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common");
    set by `FW_CDEV_CREATE_ISO_CONTEXT` ioctl

`type`
:   `FW_CDEV_EVENT_ISO_INTERRUPT_MULTICHANNEL`

`completed`
:   Offset into the receive buffer; data before this offset is valid

**Description**

This event is sent in multichannel contexts (context type
`FW_CDEV_ISO_CONTEXT_RECEIVE_MULTICHANNEL`) for [`fw_cdev_iso_packet`](#c.fw_cdev_iso_packet "fw_cdev_iso_packet") buffer
chunks that have been completely filled and that have the
`FW_CDEV_ISO_INTERRUPT` bit set, or when explicitly requested with
`FW_CDEV_IOC_FLUSH_ISO`.

The buffer is continuously filled with the following data, per packet:
:   * the 1394 iso packet header as described at [`fw_cdev_event_iso_interrupt`](#c.fw_cdev_event_iso_interrupt "fw_cdev_event_iso_interrupt"),
      but in little endian byte order,
    * packet payload (as many bytes as specified in the data\_length field of
      the 1394 iso packet header) in big endian byte order,
    * 0...3 padding bytes as needed to align the following trailer quadlet,
    * trailer quadlet, containing the reception timestamp as described at
      [`fw_cdev_event_iso_interrupt`](#c.fw_cdev_event_iso_interrupt "fw_cdev_event_iso_interrupt"), but in little endian byte order.

Hence the per-packet size is data\_length (rounded up to a multiple of 4) + 8.
When processing the data, stop before a packet that would cross the
**completed** offset.

A packet near the end of a buffer chunk will typically spill over into the
next queued buffer chunk. It is the responsibility of the client to check
for this condition, assemble a broken-up packet from its parts, and not to
re-queue any buffer chunks in which as yet unread packet parts reside.

struct fw\_cdev\_event\_iso\_resource
:   Iso resources were allocated or freed

**Definition**:

```
struct fw_cdev_event_iso_resource {
    __u64 closure;
    __u32 type;
    __u32 handle;
    __s32 channel;
    __s32 bandwidth;
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common");
    set by``FW\_CDEV\_IOC\_(DE)ALLOCATE\_ISO\_RESOURCE(\_ONCE)`` ioctl

`type`
:   `FW_CDEV_EVENT_ISO_RESOURCE_ALLOCATED` or
    `FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED`

`handle`
:   Reference by which an allocated resource can be deallocated

`channel`
:   Isochronous channel which was (de)allocated, if any

`bandwidth`
:   Bandwidth allocation units which were (de)allocated, if any

**Description**

An `FW_CDEV_EVENT_ISO_RESOURCE_ALLOCATED` event is sent after an isochronous
resource was allocated at the IRM. The client has to check **channel** and
**bandwidth** for whether the allocation actually succeeded.

An `FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED` event is sent after an isochronous
resource was deallocated at the IRM. It is also sent when automatic
reallocation after a bus reset failed.

**channel** is <0 if no channel was (de)allocated or if reallocation failed.
**bandwidth** is 0 if no bandwidth was (de)allocated or if reallocation failed.

struct fw\_cdev\_event\_phy\_packet
:   A PHY packet was transmitted or received

**Definition**:

```
struct fw_cdev_event_phy_packet {
    __u64 closure;
    __u32 type;
    __u32 rcode;
    __u32 length;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_SEND_PHY_PACKET`
    or `FW_CDEV_IOC_RECEIVE_PHY_PACKETS` ioctl

`type`
:   `FW_CDEV_EVENT_PHY_PACKET_SENT` or %...\_RECEIVED

`rcode`
:   `RCODE_`..., indicates success or failure of transmission

`length`
:   Data length in bytes

`data`
:   Incoming data for `FW_CDEV_IOC_RECEIVE_PHY_PACKETS`. For `FW_CDEV_IOC_SEND_PHY_PACKET`
    the field has the same data in the request, thus the length of 8 bytes.

**Description**

This event is sent instead of [`fw_cdev_event_phy_packet2`](#c.fw_cdev_event_phy_packet2 "fw_cdev_event_phy_packet2") if the kernel or
the client implements ABI version <= 5. It has the lack of time stamp field comparing to
[`fw_cdev_event_phy_packet2`](#c.fw_cdev_event_phy_packet2 "fw_cdev_event_phy_packet2").

struct fw\_cdev\_event\_phy\_packet2
:   A PHY packet was transmitted or received with time stamp.

**Definition**:

```
struct fw_cdev_event_phy_packet2 {
    __u64 closure;
    __u32 type;
    __u32 rcode;
    __u32 length;
    __u32 tstamp;
    __u32 data[];
};
```

**Members**

`closure`
:   See [`fw_cdev_event_common`](#c.fw_cdev_event_common "fw_cdev_event_common"); set by `FW_CDEV_IOC_SEND_PHY_PACKET`
    or `FW_CDEV_IOC_RECEIVE_PHY_PACKETS` ioctl

`type`
:   `FW_CDEV_EVENT_PHY_PACKET_SENT2` or `FW_CDEV_EVENT_PHY_PACKET_RECEIVED2`

`rcode`
:   `RCODE_`..., indicates success or failure of transmission

`length`
:   Data length in bytes

`tstamp`
:   For `FW_CDEV_EVENT_PHY_PACKET_RECEIVED2`, the time stamp of isochronous cycle at
    which the packet arrived. For `FW_CDEV_EVENT_PHY_PACKET_SENT2` and non-ping packet,
    the time stamp of isochronous cycle at which the packet was sent. For ping packet,
    the tick count for round-trip time measured by 1394 OHCI controller.

`data`
:   Incoming data

**Description**

> The time stamp of isochronous cycle at which either the response was sent for
> `FW_CDEV_EVENT_PHY_PACKET_SENT2` or the request arrived for
> `FW_CDEV_EVENT_PHY_PACKET_RECEIVED2`.

If **type** is `FW_CDEV_EVENT_PHY_PACKET_SENT2`, **length** is 8 and **data** consists of the two PHY
packet quadlets to be sent, in host byte order,

If **type** is `FW_CDEV_EVENT_PHY_PACKET_RECEIVED2`, **length** is 8 and **data** consists of the two PHY
packet quadlets, in host byte order.

For `FW_CDEV_EVENT_PHY_PACKET_RECEIVED2`, the **tstamp** is the isochronous cycle at which the
packet arrived. It is 16 bit integer value and the higher 3 bits expresses three low order bits
of second field and the rest 13 bits expresses cycle field in the format of CYCLE\_TIME register.

For `FW_CDEV_EVENT_PHY_PACKET_SENT2`, the **tstamp** has different meanings whether to sent the
packet for ping or not. If it’s not for ping, the **tstamp** is the isochronous cycle at which the
packet was sent, and use the same format as the case of `FW_CDEV_EVENT_PHY_PACKET_SENT2`. If it’s
for ping, the **tstamp** is for round-trip time measured by 1394 OHCI controller with 42.195 MHz
resolution.

union fw\_cdev\_event
:   Convenience `union of` fw\_cdev\_event\_\* types

**Definition**:

```
union fw_cdev_event {
    struct fw_cdev_event_common             common;
    struct fw_cdev_event_bus_reset          bus_reset;
    struct fw_cdev_event_response           response;
    struct fw_cdev_event_request            request;
    struct fw_cdev_event_request2           request2;
    struct fw_cdev_event_iso_interrupt      iso_interrupt;
    struct fw_cdev_event_iso_interrupt_mc   iso_interrupt_mc;
    struct fw_cdev_event_iso_resource       iso_resource;
    struct fw_cdev_event_phy_packet         phy_packet;
    struct fw_cdev_event_request3           request3;
    struct fw_cdev_event_response2          response2;
    struct fw_cdev_event_phy_packet2        phy_packet2;
};
```

**Members**

`common`
:   Valid for all types

`bus_reset`
:   Valid if **common.type** == `FW_CDEV_EVENT_BUS_RESET`

`response`
:   Valid if **common.type** == `FW_CDEV_EVENT_RESPONSE`

`request`
:   Valid if **common.type** == `FW_CDEV_EVENT_REQUEST`

`request2`
:   Valid if **common.type** == `FW_CDEV_EVENT_REQUEST2`

`iso_interrupt`
:   Valid if **common.type** == `FW_CDEV_EVENT_ISO_INTERRUPT`

`iso_interrupt_mc`
:   Valid if **common.type** ==
    `FW_CDEV_EVENT_ISO_INTERRUPT_MULTICHANNEL`

`iso_resource`
:   Valid if **common.type** ==
    `FW_CDEV_EVENT_ISO_RESOURCE_ALLOCATED` or
    `FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED`

`phy_packet`
:   Valid if **common.type** ==
    `FW_CDEV_EVENT_PHY_PACKET_SENT` or
    `FW_CDEV_EVENT_PHY_PACKET_RECEIVED`

`request3`
:   Valid if **common.type** == `FW_CDEV_EVENT_REQUEST3`

`response2`
:   Valid if **common.type** == `FW_CDEV_EVENT_RESPONSE2`

`phy_packet2`
:   Valid if **common.type** == `FW_CDEV_EVENT_PHY_PACKET_SENT2` or
    `FW_CDEV_EVENT_PHY_PACKET_RECEIVED2`

**Description**

Convenience union for userspace use. Events could be read(2) into an
appropriately aligned char buffer and then cast to this union for further
processing. Note that for a request, response or iso\_interrupt event,
the data[] or header[] may make the size of the full event larger than
sizeof([`union fw_cdev_event`](#c.fw_cdev_event "fw_cdev_event")). Also note that if you attempt to read(2)
an event into a buffer that is not large enough for it, the data that does
not fit will be discarded so that the next read(2) will return a new event.

struct fw\_cdev\_get\_info
:   General purpose information ioctl

**Definition**:

```
struct fw_cdev_get_info {
    __u32 version;
    __u32 rom_length;
    __u64 rom;
    __u64 bus_reset;
    __u64 bus_reset_closure;
    __u32 card;
};
```

**Members**

`version`
:   The version field is just a running serial number. Both an
    input parameter (ABI version implemented by the client) and
    output parameter (ABI version implemented by the kernel).
    A client shall fill in the ABI **version** for which the client
    was implemented. This is necessary for forward compatibility.

`rom_length`
:   If **rom** is non-zero, up to **rom\_length** bytes of Configuration
    ROM will be copied into that user space address. In either
    case, **rom\_length** is updated with the actual length of the
    Configuration ROM.

`rom`
:   If non-zero, address of a buffer to be filled by a copy of the
    device’s Configuration ROM

`bus_reset`
:   If non-zero, address of a buffer to be filled by a
    [`struct fw_cdev_event_bus_reset`](#c.fw_cdev_event_bus_reset "fw_cdev_event_bus_reset") with the current state
    of the bus. This does not cause a bus reset to happen.

`bus_reset_closure`
:   Value of `closure` in this and subsequent bus reset events

`card`
:   The index of the card this device belongs to

**Description**

The `FW_CDEV_IOC_GET_INFO` ioctl is usually the very first one which a client
performs right after it opened a /dev/fw\* file.

As a side effect, reception of `FW_CDEV_EVENT_BUS_RESET` events to be read(2)
is started by this ioctl.

struct fw\_cdev\_send\_request
:   Send an asynchronous request packet

**Definition**:

```
struct fw_cdev_send_request {
    __u32 tcode;
    __u32 length;
    __u64 offset;
    __u64 closure;
    __u64 data;
    __u32 generation;
};
```

**Members**

`tcode`
:   Transaction code of the request

`length`
:   Length of outgoing payload, in bytes

`offset`
:   48-bit offset at destination node

`closure`
:   Passed back to userspace in the response event

`data`
:   Userspace pointer to payload

`generation`
:   The bus generation where packet is valid

**Description**

Send a request to the device. This ioctl implements all outgoing requests. Both quadlet and
block request specify the payload as a pointer to the data in the **data** field. Once the
transaction completes, the kernel writes either [`fw_cdev_event_response`](#c.fw_cdev_event_response "fw_cdev_event_response") event or
[`fw_cdev_event_response`](#c.fw_cdev_event_response "fw_cdev_event_response") event back. The **closure** field is passed back to user space in the
response event.

struct fw\_cdev\_send\_response
:   Send an asynchronous response packet

**Definition**:

```
struct fw_cdev_send_response {
    __u32 rcode;
    __u32 length;
    __u64 data;
    __u32 handle;
};
```

**Members**

`rcode`
:   Response code as determined by the userspace handler

`length`
:   Length of outgoing payload, in bytes

`data`
:   Userspace pointer to payload

`handle`
:   The handle from the [`fw_cdev_event_request`](#c.fw_cdev_event_request "fw_cdev_event_request")

**Description**

Send a response to an incoming request. By setting up an address range using
the `FW_CDEV_IOC_ALLOCATE` ioctl, userspace can listen for incoming requests. An
incoming request will generate an `FW_CDEV_EVENT_REQUEST`, and userspace must
send a reply using this ioctl. The event has a handle to the kernel-side
pending transaction, which should be used with this ioctl.

struct fw\_cdev\_allocate
:   Allocate a CSR in an address range

**Definition**:

```
struct fw_cdev_allocate {
    __u64 offset;
    __u64 closure;
    __u32 length;
    __u32 handle;
    __u64 region_end;
};
```

**Members**

`offset`
:   Start offset of the address range

`closure`
:   To be passed back to userspace in request events

`length`
:   Length of the CSR, in bytes

`handle`
:   Handle to the allocation, written by the kernel

`region_end`
:   First address above the address range (added in ABI v4, 2.6.36)

**Description**

Allocate an address range in the 48-bit address space on the local node
(the controller). This allows userspace to listen for requests with an
offset within that address range. Every time when the kernel receives a
request within the range, an [`fw_cdev_event_request2`](#c.fw_cdev_event_request2 "fw_cdev_event_request2") event will be emitted.
(If the kernel or the client implements ABI version <= 3, an
[`fw_cdev_event_request`](#c.fw_cdev_event_request "fw_cdev_event_request") will be generated instead.)

The **closure** field is passed back to userspace in these request events.
The **handle** field is an out parameter, returning a handle to the allocated
range to be used for later deallocation of the range.

The address range is allocated on all local nodes. The address allocation
is exclusive except for the FCP command and response registers. If an
exclusive address region is already in use, the ioctl fails with errno set
to `EBUSY`.

If kernel and client implement ABI version >= 4, the kernel looks up a free
spot of size **length** inside [**offset**..\*\*region\_end\*\*) and, if found, writes
the start address of the new CSR back in **offset**. I.e. **offset** is an
in and out parameter. If this automatic placement of a CSR in a bigger
address range is not desired, the client simply needs to set **region\_end**
= **offset** + **length**.

If the kernel or the client implements ABI version <= 3, **region\_end** is
ignored and effectively assumed to be **offset** + **length**.

**region\_end** is only present in a kernel header >= 2.6.36. If necessary,
this can for example be tested by #ifdef FW\_CDEV\_EVENT\_REQUEST2.

struct fw\_cdev\_deallocate
:   Free a CSR address range or isochronous resource

**Definition**:

```
struct fw_cdev_deallocate {
    __u32 handle;
};
```

**Members**

`handle`
:   Handle to the address range or iso resource, as returned by the
    kernel when the range or resource was allocated

struct fw\_cdev\_initiate\_bus\_reset
:   Initiate a bus reset

**Definition**:

```
struct fw_cdev_initiate_bus_reset {
    __u32 type;
};
```

**Members**

`type`
:   `FW_CDEV_SHORT_RESET` or `FW_CDEV_LONG_RESET`

**Description**

Initiate a bus reset for the bus this device is on. The bus reset can be
either the original (long) bus reset or the arbitrated (short) bus reset
introduced in 1394a-2000.

The ioctl returns immediately. A subsequent [`fw_cdev_event_bus_reset`](#c.fw_cdev_event_bus_reset "fw_cdev_event_bus_reset")
indicates when the reset actually happened. Since ABI v4, this may be
considerably later than the ioctl because the kernel ensures a grace period
between subsequent bus resets as per IEEE 1394 bus management specification.

struct fw\_cdev\_add\_descriptor
:   Add contents to the local node’s config ROM

**Definition**:

```
struct fw_cdev_add_descriptor {
    __u32 immediate;
    __u32 key;
    __u64 data;
    __u32 length;
    __u32 handle;
};
```

**Members**

`immediate`
:   If non-zero, immediate key to insert before pointer

`key`
:   Upper 8 bits of root directory pointer

`data`
:   Userspace pointer to contents of descriptor block

`length`
:   Length of descriptor block data, in quadlets

`handle`
:   Handle to the descriptor, written by the kernel

**Description**

Add a descriptor block and optionally a preceding immediate key to the local
node’s Configuration ROM.

The **key** field specifies the upper 8 bits of the descriptor root directory
pointer and the **data** and **length** fields specify the contents. The **key**
should be of the form 0xXX000000. The offset part of the root directory entry
will be filled in by the kernel.

If not 0, the **immediate** field specifies an immediate key which will be
inserted before the root directory pointer.

**immediate**, **key**, and **data** array elements are CPU-endian quadlets.

If successful, the kernel adds the descriptor and writes back a **handle** to
the kernel-side object to be used for later removal of the descriptor block
and immediate key. The kernel will also generate a bus reset to signal the
change of the Configuration ROM to other nodes.

This ioctl affects the Configuration ROMs of all local nodes.
The ioctl only succeeds on device files which represent a local node.

struct fw\_cdev\_remove\_descriptor
:   Remove contents from the Configuration ROM

**Definition**:

```
struct fw_cdev_remove_descriptor {
    __u32 handle;
};
```

**Members**

`handle`
:   Handle to the descriptor, as returned by the kernel when the
    descriptor was added

**Description**

Remove a descriptor block and accompanying immediate key from the local
nodes’ Configuration ROMs. The kernel will also generate a bus reset to
signal the change of the Configuration ROM to other nodes.

struct fw\_cdev\_create\_iso\_context
:   Create a context for isochronous I/O

**Definition**:

```
struct fw_cdev_create_iso_context {
    __u32 type;
    __u32 header_size;
    __u32 channel;
    __u32 speed;
    __u64 closure;
    __u32 handle;
};
```

**Members**

`type`
:   `FW_CDEV_ISO_CONTEXT_TRANSMIT` or `FW_CDEV_ISO_CONTEXT_RECEIVE` or
    `FW_CDEV_ISO_CONTEXT_RECEIVE_MULTICHANNEL`

`header_size`
:   Header size to strip in single-channel reception

`channel`
:   Channel to bind to in single-channel reception or transmission

`speed`
:   Transmission speed

`closure`
:   To be returned in [`fw_cdev_event_iso_interrupt`](#c.fw_cdev_event_iso_interrupt "fw_cdev_event_iso_interrupt") or
    `fw_cdev_event_iso_interrupt_multichannel`

`handle`
:   Handle to context, written back by kernel

**Description**

Prior to sending or receiving isochronous I/O, a context must be created.
The context records information about the transmit or receive configuration
and typically maps to an underlying hardware resource. A context is set up
for either sending or receiving. It is bound to a specific isochronous
**channel**.

In case of multichannel reception, **header\_size** and **channel** are ignored
and the channels are selected by `FW_CDEV_IOC_SET_ISO_CHANNELS`.

For `FW_CDEV_ISO_CONTEXT_RECEIVE` contexts, **header\_size** must be at least 4
and must be a multiple of 4. It is ignored in other context types.

**speed** is ignored in receive context types.

If a context was successfully created, the kernel writes back a handle to the
context, which must be passed in for subsequent operations on that context.

Limitations:
No more than one iso context can be created per fd.
The total number of contexts that all userspace and kernelspace drivers can
create on a card at a time is a hardware limit, typically 4 or 8 contexts per
direction, and of them at most one multichannel receive context.

struct fw\_cdev\_set\_iso\_channels
:   Select channels in multichannel reception

**Definition**:

```
struct fw_cdev_set_iso_channels {
    __u64 channels;
    __u32 handle;
};
```

**Members**

`channels`
:   Bitmask of channels to listen to

`handle`
:   Handle of the mutichannel receive context

**Description**

**channels** is the bitwise or of 1ULL << n for each channel n to listen to.

The ioctl fails with errno `EBUSY` if there is already another receive context
on a channel in **channels**. In that case, the bitmask of all unoccupied
channels is returned in **channels**.

struct fw\_cdev\_iso\_packet
:   Isochronous packet

**Definition**:

```
struct fw_cdev_iso_packet {
    __u32 control;
    __u32 header[];
};
```

**Members**

`control`
:   Contains the header length (8 uppermost bits),
    the sy field (4 bits), the tag field (2 bits), a sync flag
    or a skip flag (1 bit), an interrupt flag (1 bit), and the
    payload length (16 lowermost bits)

`header`
:   Header and payload in case of a transmit context.

**Description**

[`struct fw_cdev_iso_packet`](#c.fw_cdev_iso_packet "fw_cdev_iso_packet") is used to describe isochronous packet queues.
Use the FW\_CDEV\_ISO\_\* macros to fill in **control**.
The **header** array is empty in case of receive contexts.

Context type `FW_CDEV_ISO_CONTEXT_TRANSMIT`:

**control.HEADER\_LENGTH** must be a multiple of 4. It specifies the numbers of
bytes in **header** that will be prepended to the packet’s payload. These bytes
are copied into the kernel and will not be accessed after the ioctl has
returned.

The **control.SY** and TAG fields are copied to the iso packet header. These
fields are specified by IEEE 1394a and IEC 61883-1.

The **control.SKIP** flag specifies that no packet is to be sent in a frame.
When using this, all other fields except **control.INTERRUPT** must be zero.

When a packet with the **control.INTERRUPT** flag set has been completed, an
[`fw_cdev_event_iso_interrupt`](#c.fw_cdev_event_iso_interrupt "fw_cdev_event_iso_interrupt") event will be sent.

Context type `FW_CDEV_ISO_CONTEXT_RECEIVE`:

**control.HEADER\_LENGTH** must be a multiple of the context’s header\_size.
If the HEADER\_LENGTH is larger than the context’s header\_size, multiple
packets are queued for this entry.

The **control.SY** and TAG fields are ignored.

If the **control.SYNC** flag is set, the context drops all packets until a
packet with a sy field is received which matches [`fw_cdev_start_iso.sync`](#c.fw_cdev_start_iso "fw_cdev_start_iso").

**control.PAYLOAD\_LENGTH** defines how many payload bytes can be received for
one packet (in addition to payload quadlets that have been defined as headers
and are stripped and returned in the [`fw_cdev_event_iso_interrupt`](#c.fw_cdev_event_iso_interrupt "fw_cdev_event_iso_interrupt") structure).
If more bytes are received, the additional bytes are dropped. If less bytes
are received, the remaining bytes in this part of the payload buffer will not
be written to, not even by the next packet. I.e., packets received in
consecutive frames will not necessarily be consecutive in memory. If an
entry has queued multiple packets, the PAYLOAD\_LENGTH is divided equally
among them.

When a packet with the **control.INTERRUPT** flag set has been completed, an
[`fw_cdev_event_iso_interrupt`](#c.fw_cdev_event_iso_interrupt "fw_cdev_event_iso_interrupt") event will be sent. An entry that has queued
multiple receive packets is completed when its last packet is completed.

Context type `FW_CDEV_ISO_CONTEXT_RECEIVE_MULTICHANNEL`:

Here, [`fw_cdev_iso_packet`](#c.fw_cdev_iso_packet "fw_cdev_iso_packet") would be more aptly named \_iso\_buffer\_chunk since
it specifies a chunk of the mmap()’ed buffer, while the number and alignment
of packets to be placed into the buffer chunk is not known beforehand.

**control.PAYLOAD\_LENGTH** is the size of the buffer chunk and specifies room
for header, payload, padding, and trailer bytes of one or more packets.
It must be a multiple of 4.

**control.HEADER\_LENGTH**, TAG and SY are ignored. SYNC is treated as described
for single-channel reception.

When a buffer chunk with the **control.INTERRUPT** flag set has been filled
entirely, an [`fw_cdev_event_iso_interrupt_mc`](#c.fw_cdev_event_iso_interrupt_mc "fw_cdev_event_iso_interrupt_mc") event will be sent.

struct fw\_cdev\_queue\_iso
:   Queue isochronous packets for I/O

**Definition**:

```
struct fw_cdev_queue_iso {
    __u64 packets;
    __u64 data;
    __u32 size;
    __u32 handle;
};
```

**Members**

`packets`
:   Userspace pointer to an array of [`fw_cdev_iso_packet`](#c.fw_cdev_iso_packet "fw_cdev_iso_packet")

`data`
:   Pointer into mmap()’ed payload buffer

`size`
:   Size of the **packets** array, in bytes

`handle`
:   Isochronous context handle

**Description**

Queue a number of isochronous packets for reception or transmission.
This ioctl takes a pointer to an array of [`fw_cdev_iso_packet`](#c.fw_cdev_iso_packet "fw_cdev_iso_packet") structs,
which describe how to transmit from or receive into a contiguous region
of a mmap()’ed payload buffer. As part of transmit packet descriptors,
a series of headers can be supplied, which will be prepended to the
payload during DMA.

The kernel may or may not queue all packets, but will write back updated
values of the **packets**, **data** and **size** fields, so the ioctl can be
resubmitted easily.

In case of a multichannel receive context, **data** must be quadlet-aligned
relative to the buffer start.

struct fw\_cdev\_start\_iso
:   Start an isochronous transmission or reception

**Definition**:

```
struct fw_cdev_start_iso {
    __s32 cycle;
    __u32 sync;
    __u32 tags;
    __u32 handle;
};
```

**Members**

`cycle`
:   Cycle in which to start I/O. If **cycle** is greater than or
    equal to 0, the I/O will start on that cycle.

`sync`
:   Determines the value to wait for receive packets that have
    the `FW_CDEV_ISO_SYNC` bit set

`tags`
:   Tag filter bit mask. Only valid for isochronous reception.
    Determines the tag values for which packets will be accepted.
    Use FW\_CDEV\_ISO\_CONTEXT\_MATCH\_\* macros to set **tags**.

`handle`
:   Isochronous context handle within which to transmit or receive

struct fw\_cdev\_stop\_iso
:   Stop an isochronous transmission or reception

**Definition**:

```
struct fw_cdev_stop_iso {
    __u32 handle;
};
```

**Members**

`handle`
:   Handle of isochronous context to stop

struct fw\_cdev\_flush\_iso
:   flush completed iso packets

**Definition**:

```
struct fw_cdev_flush_iso {
    __u32 handle;
};
```

**Members**

`handle`
:   handle of isochronous context to flush

**Description**

For `FW_CDEV_ISO_CONTEXT_TRANSMIT` or `FW_CDEV_ISO_CONTEXT_RECEIVE` contexts,
report any completed packets.

For `FW_CDEV_ISO_CONTEXT_RECEIVE_MULTICHANNEL` contexts, report the current
offset in the receive buffer, if it has changed; this is typically in the
middle of some buffer chunk.

Any `FW_CDEV_EVENT_ISO_INTERRUPT` or `FW_CDEV_EVENT_ISO_INTERRUPT_MULTICHANNEL`
events generated by this ioctl are sent synchronously, i.e., are available
for reading from the file descriptor when this ioctl returns.

struct fw\_cdev\_get\_cycle\_timer
:   read cycle timer register

**Definition**:

```
struct fw_cdev_get_cycle_timer {
    __u64 local_time;
    __u32 cycle_timer;
};
```

**Members**

`local_time`
:   system time, in microseconds since the Epoch

`cycle_timer`
:   Cycle Time register contents

**Description**

Same as `FW_CDEV_IOC_GET_CYCLE_TIMER2`, but fixed to use `CLOCK_REALTIME`
and only with microseconds resolution.

In version 1 and 2 of the ABI, this ioctl returned unreliable (non-
monotonic) **cycle\_timer** values on certain controllers.

struct fw\_cdev\_get\_cycle\_timer2
:   read cycle timer register

**Definition**:

```
struct fw_cdev_get_cycle_timer2 {
    __s64 tv_sec;
    __s32 tv_nsec;
    __s32 clk_id;
    __u32 cycle_timer;
};
```

**Members**

`tv_sec`
:   system time, seconds

`tv_nsec`
:   system time, sub-seconds part in nanoseconds

`clk_id`
:   input parameter, clock from which to get the system time

`cycle_timer`
:   Cycle Time register contents

**Description**

The `FW_CDEV_IOC_GET_CYCLE_TIMER2` ioctl reads the isochronous cycle timer
and also the system clock. This allows to correlate reception time of
isochronous packets with system time.

**clk\_id** lets you choose a clock like with POSIX’ clock\_gettime function.
Supported **clk\_id** values are POSIX’ `CLOCK_REALTIME` and `CLOCK_MONOTONIC`
and Linux’ `CLOCK_MONOTONIC_RAW`.

**cycle\_timer** consists of 7 bits cycleSeconds, 13 bits cycleCount, and
12 bits cycleOffset, in host byte order. Cf. the Cycle Time register
per IEEE 1394 or Isochronous Cycle Timer register per OHCI-1394.

struct fw\_cdev\_allocate\_iso\_resource
:   (De)allocate a channel or bandwidth

**Definition**:

```
struct fw_cdev_allocate_iso_resource {
    __u64 closure;
    __u64 channels;
    __u32 bandwidth;
    __u32 handle;
};
```

**Members**

`closure`
:   Passed back to userspace in corresponding iso resource events

`channels`
:   Isochronous channels of which one is to be (de)allocated

`bandwidth`
:   Isochronous bandwidth units to be (de)allocated

`handle`
:   Handle to the allocation, written by the kernel (only valid in
    case of `FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE` ioctls)

**Description**

The `FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE` ioctl initiates allocation of an
isochronous channel and/or of isochronous bandwidth at the isochronous
resource manager (IRM). Only one of the channels specified in **channels** is
allocated. An `FW_CDEV_EVENT_ISO_RESOURCE_ALLOCATED` is sent after
communication with the IRM, indicating success or failure in the event data.
The kernel will automatically reallocate the resources after bus resets.
Should a reallocation fail, an `FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED` event
will be sent. The kernel will also automatically deallocate the resources
when the file descriptor is closed.

The `FW_CDEV_IOC_DEALLOCATE_ISO_RESOURCE` ioctl can be used to initiate
deallocation of resources which were allocated as described above.
An `FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED` event concludes this operation.

The `FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE_ONCE` ioctl is a variant of allocation
without automatic re- or deallocation.
An `FW_CDEV_EVENT_ISO_RESOURCE_ALLOCATED` event concludes this operation,
indicating success or failure in its data.

The `FW_CDEV_IOC_DEALLOCATE_ISO_RESOURCE_ONCE` ioctl works like
`FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE_ONCE` except that resources are freed
instead of allocated.
An `FW_CDEV_EVENT_ISO_RESOURCE_DEALLOCATED` event concludes this operation.

To summarize, `FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE` allocates iso resources
for the lifetime of the fd or **handle**.
In contrast, `FW_CDEV_IOC_ALLOCATE_ISO_RESOURCE_ONCE` allocates iso resources
for the duration of a bus generation.

**channels** is a host-endian bitfield with the least significant bit
representing channel 0 and the most significant bit representing channel 63:
1ULL << c for each channel c that is a candidate for (de)allocation.

**bandwidth** is expressed in bandwidth allocation units, i.e. the time to send
one quadlet of data (payload or header data) at speed S1600.

struct fw\_cdev\_send\_stream\_packet
:   send an asynchronous stream packet

**Definition**:

```
struct fw_cdev_send_stream_packet {
    __u32 length;
    __u32 tag;
    __u32 channel;
    __u32 sy;
    __u64 closure;
    __u64 data;
    __u32 generation;
    __u32 speed;
};
```

**Members**

`length`
:   Length of outgoing payload, in bytes

`tag`
:   Data format tag

`channel`
:   Isochronous channel to transmit to

`sy`
:   Synchronization code

`closure`
:   Passed back to userspace in the response event

`data`
:   Userspace pointer to payload

`generation`
:   The bus generation where packet is valid

`speed`
:   Speed to transmit at

**Description**

The `FW_CDEV_IOC_SEND_STREAM_PACKET` ioctl sends an asynchronous stream packet to every device
which is listening to the specified channel. The kernel writes either [`fw_cdev_event_response`](#c.fw_cdev_event_response "fw_cdev_event_response")
event or [`fw_cdev_event_response2`](#c.fw_cdev_event_response2 "fw_cdev_event_response2") event which indicates success or failure of the transmission.

struct fw\_cdev\_send\_phy\_packet
:   send a PHY packet

**Definition**:

```
struct fw_cdev_send_phy_packet {
    __u64 closure;
    __u32 data[2];
    __u32 generation;
};
```

**Members**

`closure`
:   Passed back to userspace in the PHY-packet-sent event

`data`
:   First and second quadlet of the PHY packet

`generation`
:   The bus generation where packet is valid

**Description**

The `FW_CDEV_IOC_SEND_PHY_PACKET` ioctl sends a PHY packet to all nodes on the same card as this
device. After transmission, either `FW_CDEV_EVENT_PHY_PACKET_SENT` event or
`FW_CDEV_EVENT_PHY_PACKET_SENT` event is generated.

The payload **data**[] shall be specified in host byte order. Usually,
**data**[1] needs to be the bitwise inverse of **data**[0]. VersaPHY packets
are an exception to this rule.

The ioctl is only permitted on device files which represent a local node.

struct fw\_cdev\_receive\_phy\_packets
:   start reception of PHY packets

**Definition**:

```
struct fw_cdev_receive_phy_packets {
    __u64 closure;
};
```

**Members**

`closure`
:   Passed back to userspace in phy packet events

**Description**

This ioctl activates issuing of either `FW_CDEV_EVENT_PHY_PACKET_RECEIVED` or
`FW_CDEV_EVENT_PHY_PACKET_RECEIVED2` due to incoming PHY packets from any node on the same bus
as the device.

The ioctl is only permitted on device files which represent a local node.

## Firewire device probing and sysfs interfaces

```
What:           /sys/bus/firewire/devices/fw[0-9]+/
Date:           May 2007
KernelVersion:  2.6.22
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                IEEE 1394 node device attributes.
                Read-only.  Mutable during the node device's lifetime.
                See IEEE 1212 for semantic definitions.

                config_rom
                        Contents of the Configuration ROM register.
                        Binary attribute; an array of host-endian u32.

                guid
                        The node's EUI-64 in the bus information block of
                        Configuration ROM.
                        Hexadecimal string representation of an u64.


What:           /sys/bus/firewire/devices/fw[0-9]+/units
Date:           June 2009
KernelVersion:  2.6.31
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                IEEE 1394 node device attribute.
                Read-only.  Mutable during the node device's lifetime.
                See IEEE 1212 for semantic definitions.

                units
                        Summary of all units present in an IEEE 1394 node.
                        Contains space-separated tuples of specifier_id and
                        version of each unit present in the node.  Specifier_id
                        and version are hexadecimal string representations of
                        u24 of the respective unit directory entries.
                        Specifier_id and version within each tuple are separated
                        by a colon.

Users:          udev rules to set ownership and access permissions or ACLs of
                /dev/fw[0-9]+ character device files


What:           /sys/bus/firewire/devices/fw[0-9]+/is_local
Date:           July 2012
KernelVersion:  3.6
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                IEEE 1394 node device attribute.
                Read-only and immutable.
Values:         1: The sysfs entry represents a local node (a controller card).

                0: The sysfs entry represents a remote node.


What:           /sys/bus/firewire/devices/fw[0-9]+[.][0-9]+/
Date:           May 2007
KernelVersion:  2.6.22
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                IEEE 1394 unit device attributes.
                Read-only.  Immutable during the unit device's lifetime.
                See IEEE 1212 for semantic definitions.

                modalias
                        Same as MODALIAS in the uevent at device creation.

                rom_index
                        Offset of the unit directory within the parent device's
                        (node device's) Configuration ROM, in quadlets.
                        Decimal string representation.


What:           /sys/bus/firewire/devices/*/
Date:           May 2007
KernelVersion:  2.6.22
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                Attributes common to IEEE 1394 node devices and unit devices.
                Read-only.  Mutable during the node device's lifetime.
                Immutable during the unit device's lifetime.
                See IEEE 1212 for semantic definitions.

                These attributes are only created if the root directory of an
                IEEE 1394 node or the unit directory of an IEEE 1394 unit
                actually contains according entries.

                hardware_version
                        Hexadecimal string representation of an u24.

                hardware_version_name
                        Contents of a respective textual descriptor leaf.

                model
                        Hexadecimal string representation of an u24.

                model_name
                        Contents of a respective textual descriptor leaf.

                specifier_id
                        Hexadecimal string representation of an u24.
                        Mandatory in unit directories according to IEEE 1212.

                vendor
                        Hexadecimal string representation of an u24.
                        Mandatory in the root directory according to IEEE 1212.

                vendor_name
                        Contents of a respective textual descriptor leaf.

                version
                        Hexadecimal string representation of an u24.
                        Mandatory in unit directories according to IEEE 1212.


What:           /sys/bus/firewire/drivers/sbp2/fw*/host*/target*/*:*:*:*/ieee1394_id
                formerly
                /sys/bus/ieee1394/drivers/sbp2/fw*/host*/target*/*:*:*:*/ieee1394_id
Date:           Feb 2004
KernelVersion:  2.6.4
Contact:        linux1394-devel@lists.sourceforge.net
Description:
                SCSI target port identifier and logical unit identifier of a
                logical unit of an SBP-2 target.  The identifiers are specified
                in SAM-2...SAM-4 annex A.  They are persistent and world-wide
                unique properties the SBP-2 attached target.

                Read-only attribute, immutable during the target's lifetime.
                Format, as exposed by firewire-sbp2 since 2.6.22, May 2007:
                Colon-separated hexadecimal string representations of

                        u64 EUI-64 : u24 directory_ID : u16 LUN

                without 0x prefixes, without whitespace.  The former sbp2 driver
                (removed in 2.6.37 after being superseded by firewire-sbp2) used
                a somewhat shorter format which was not as close to SAM.

Users:          udev rules to create /dev/disk/by-id/ symlinks
```

int fw\_csr\_string(const u32 \*directory, int key, char \*buf, size\_t size)
:   reads a string from the configuration ROM

**Parameters**

`const u32 *directory`
:   e.g. root directory or unit directory

`int key`
:   the key of the preceding directory entry

`char *buf`
:   where to put the string

`size_t size`
:   size of **buf**, in bytes

**Description**

The string is taken from a minimal ASCII text descriptor leaf just after the entry with the
**key**. The string is zero-terminated. An overlong string is silently truncated such that it
and the zero byte fit into **size**.

Returns strlen(buf) or a negative error code.

## Firewire core transaction interfaces

void \_\_fw\_send\_request(struct fw\_card \*card, struct fw\_transaction \*t, int tcode, int destination\_id, int generation, int speed, unsigned long long offset, void \*payload, size\_t length, union fw\_transaction\_callback callback, bool with\_tstamp, void \*callback\_data)
:   submit a request packet for transmission to generate callback for response subaction with or without time stamp.

**Parameters**

`struct fw_card *card`
:   interface to send the request at

`struct fw_transaction *t`
:   transaction instance to which the request belongs

`int tcode`
:   transaction code

`int destination_id`
:   destination node ID, consisting of bus\_ID and phy\_ID

`int generation`
:   bus generation in which request and response are valid

`int speed`
:   transmission speed

`unsigned long long offset`
:   48bit wide offset into destination’s address space

`void *payload`
:   data payload for the request subaction

`size_t length`
:   length of the payload, in bytes

`union fw_transaction_callback callback`
:   `union of` two functions whether to receive time stamp or not for response
    subaction.

`bool with_tstamp`
:   Whether to receive time stamp or not for response subaction.

`void *callback_data`
:   data to be passed to the transaction completion callback

**Description**

Submit a request packet into the asynchronous request transmission queue.
Can be called from atomic context. If you prefer a blocking API, use
[`fw_run_transaction()`](#c.fw_run_transaction "fw_run_transaction") in a context that can sleep.

In case of lock requests, specify one of the firewire-core specific `TCODE_`
constants instead of `TCODE_LOCK_REQUEST` in **tcode**.

Make sure that the value in **destination\_id** is not older than the one in
**generation**. Otherwise the request is in danger to be sent to a wrong node.

In case of asynchronous stream packets i.e. `TCODE_STREAM_DATA`, the caller
needs to synthesize **destination\_id** with `fw_stream_packet_destination_id()`.
It will contain tag, channel, and sy data instead of a node ID then.

The payload buffer at **data** is going to be DMA-mapped except in case of
**length** <= 8 or of local (loopback) requests. Hence make sure that the
buffer complies with the restrictions of the streaming DMA mapping API.
**payload** must not be freed before the **callback** is called.

In case of request types without payload, **data** is NULL and **length** is 0.

After the transaction is completed successfully or unsuccessfully, the
**callback** will be called. Among its parameters is the response code which
is either one of the rcodes per IEEE 1394 or, in case of internal errors,
the firewire-core specific `RCODE_SEND_ERROR`. The other firewire-core
specific rcodes (`RCODE_CANCELLED`, `RCODE_BUSY`, `RCODE_GENERATION`,
`RCODE_NO_ACK`) denote transaction timeout, busy responder, stale request
generation, or missing ACK respectively.

Note some timing corner cases: `fw_send_request()` may complete much earlier
than when the request packet actually hits the wire. On the other hand,
transaction completion and hence execution of **callback** may happen even
before `fw_send_request()` returns.

int fw\_run\_transaction(struct fw\_card \*card, int tcode, int destination\_id, int generation, int speed, unsigned long long offset, void \*payload, size\_t length)
:   send request and sleep until transaction is completed

**Parameters**

`struct fw_card *card`
:   card interface for this request

`int tcode`
:   transaction code

`int destination_id`
:   destination node ID, consisting of bus\_ID and phy\_ID

`int generation`
:   bus generation in which request and response are valid

`int speed`
:   transmission speed

`unsigned long long offset`
:   48bit wide offset into destination’s address space

`void *payload`
:   data payload for the request subaction

`size_t length`
:   length of the payload, in bytes

**Description**

Returns the RCODE. See `fw_send_request()` for parameter documentation.
Unlike `fw_send_request()`, **data** points to the payload of the request or/and
to the payload of the response. DMA mapping restrictions apply to outbound
request payloads of >= 8 bytes but not to inbound response payloads.

int fw\_core\_add\_address\_handler(struct fw\_address\_handler \*handler, const struct fw\_address\_region \*region)
:   register for incoming requests

**Parameters**

`struct fw_address_handler *handler`
:   callback

`const struct fw_address_region *region`
:   region in the IEEE 1212 node space address range

**Description**

region->start, ->end, and handler->length have to be quadlet-aligned.

When a request is received that falls within the specified address range, the specified callback
is invoked. The parameters passed to the callback give the details of the particular request.
The callback is invoked in the workqueue context in most cases. However, if the request is
initiated by the local node, the callback is invoked in the initiator’s context.

To be called in process context.
Return value: 0 on success, non-zero otherwise.

The start offset of the handler’s address region is determined by
[`fw_core_add_address_handler()`](#c.fw_core_add_address_handler "fw_core_add_address_handler") and is returned in handler->offset.

Address allocations are exclusive, except for the FCP registers.

void fw\_core\_remove\_address\_handler(struct fw\_address\_handler \*handler)
:   unregister an address handler

**Parameters**

`struct fw_address_handler *handler`
:   callback

**Description**

To be called in process context.

When [`fw_core_remove_address_handler()`](#c.fw_core_remove_address_handler "fw_core_remove_address_handler") returns, **handler->`callback()`** is
guaranteed to not run on any CPU anymore.

void fw\_send\_response(struct fw\_card \*card, struct fw\_request \*request, int rcode)
:   * send response packet for asynchronous transaction.

**Parameters**

`struct fw_card *card`
:   interface to send the response at.

`struct fw_request *request`
:   firewire request data for the transaction.

`int rcode`
:   response code to send.

**Description**

Submit a response packet into the asynchronous response transmission queue. The **request**
is going to be released when the transmission successfully finishes later.

int fw\_get\_request\_speed(struct fw\_request \*request)
:   returns speed at which the **request** was received

**Parameters**

`struct fw_request *request`
:   firewire request data

u32 fw\_request\_get\_timestamp(const struct fw\_request \*request)
:   Get timestamp of the request.

**Parameters**

`const struct fw_request *request`
:   The opaque pointer to request structure.

**Description**

Get timestamp when 1394 OHCI controller receives the asynchronous request subaction. The
timestamp consists of the low order 3 bits of second field and the full 13 bits of count
field of isochronous cycle time register.

**Return**

timestamp of the request.

const char \*fw\_rcode\_string(int rcode)
:   convert a firewire result code to an error description

**Parameters**

`int rcode`
:   the result code

## Firewire Isochronous I/O interfaces

void fw\_iso\_context\_schedule\_flush\_completions(struct fw\_iso\_context \*ctx)
:   schedule work item to process isochronous context.

**Parameters**

`struct fw_iso_context *ctx`
:   the isochronous context

**Description**

Schedule a work item on workqueue to process the isochronous context. The registered callback
function is called by the worker when a queued packet buffer with the interrupt flag is
completed, either after transmission in the IT context or after being filled in the IR context.
The callback function is also called when the header buffer in the context becomes full, If it
is required to process the context in the current context, [`fw_iso_context_flush_completions()`](#c.fw_iso_context_flush_completions "fw_iso_context_flush_completions") is
available instead.

**Context**

Any context.

int fw\_iso\_context\_flush\_completions(struct fw\_iso\_context \*ctx)
:   process isochronous context in current process context.

**Parameters**

`struct fw_iso_context *ctx`
:   the isochronous context

**Description**

Process the isochronous context in the current process context. The registered callback function
is called when a queued packet buffer with the interrupt flag is completed, either after
transmission in the IT context or after being filled in the IR context. Additionally, the
callback function is also called for the packet buffer completed at last. Furthermore, the
callback function is called as well when the header buffer in the context becomes full. If it is
required to process the context asynchronously, [`fw_iso_context_schedule_flush_completions()`](#c.fw_iso_context_schedule_flush_completions "fw_iso_context_schedule_flush_completions") is
available instead.

**Context**

Process context. May sleep due to [`disable_work_sync()`](../core-api/workqueue.html#c.disable_work_sync "disable_work_sync").

void fw\_iso\_resource\_manage(struct fw\_card \*card, int generation, u64 channels\_mask, int \*channel, int \*bandwidth, bool allocate)
:   Allocate or deallocate a channel and/or bandwidth

**Parameters**

`struct fw_card *card`
:   card interface for this action

`int generation`
:   bus generation

`u64 channels_mask`
:   bitmask for channel allocation

`int *channel`
:   pointer for returning channel allocation result

`int *bandwidth`
:   pointer for returning bandwidth allocation result

`bool allocate`
:   whether to allocate (true) or deallocate (false)

**Description**

In parameters: card, generation, channels\_mask, bandwidth, allocate
Out parameters: channel, bandwidth

This function blocks (sleeps) during communication with the IRM.

Allocates or deallocates at most one channel out of channels\_mask.
channels\_mask is a bitfield with MSB for channel 63 and LSB for channel 0.
(Note, the IRM’s CHANNELS\_AVAILABLE is a big-endian bitfield with MSB for
channel 0 and LSB for channel 63.)
Allocates or deallocates as many bandwidth allocation units as specified.

Returns channel < 0 if no channel was allocated or deallocated.
Returns bandwidth = 0 if no bandwidth was allocated or deallocated.

If generation is stale, deallocations succeed but allocations fail with
channel = -EAGAIN.

If channel allocation fails, no bandwidth will be allocated either.
If bandwidth allocation fails, no channel will be allocated either.
But deallocations of channel and bandwidth are tried independently
of each other’s success.
