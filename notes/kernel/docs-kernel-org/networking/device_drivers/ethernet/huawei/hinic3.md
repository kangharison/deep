# Linux kernel driver for Huawei Ethernet Device Driver (hinic3) family

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/huawei/hinic3.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux kernel driver for Huawei Ethernet Device Driver (hinic3) family

## Overview

The hinic3 is a network interface card (NIC) for Data Center. It supports
a range of link-speed devices (10GE, 25GE, 100GE, etc.). The hinic3
devices can have multiple physical forms: LOM (Lan on Motherboard) NIC,
PCIe standard NIC, OCP (Open Compute Project) NIC, etc.

The hinic3 driver supports the following features:
- IPv4/IPv6 TCP/UDP checksum offload
- TSO (TCP Segmentation Offload), LRO (Large Receive Offload)
- RSS (Receive Side Scaling)
- MSI-X interrupt aggregation configuration and interrupt adaptation.
- SR-IOV (Single Root I/O Virtualization).

## Content

* Supported PCI vendor ID/device IDs
* Source Code Structure of Hinic3 Driver
* Management Interface

## Supported PCI vendor ID/device IDs

19e5:0222 - hinic3 PF/PPF
19e5:375F - hinic3 VF

Prime Physical Function (PPF) is responsible for the management of the
whole NIC card. For example, clock synchronization between the NIC and
the host. Any PF may serve as a PPF. The PPF is selected dynamically.

## Source Code Structure of Hinic3 Driver

|  |  |
| --- | --- |
| hinic3\_pci\_id\_tbl.h | Supported device IDs |
| hinic3\_hw\_intf.h | Interface between HW and driver |
| hinic3\_queue\_common.[ch] | Common structures and methods for NIC queues |
| hinic3\_common.[ch] | Encapsulation of memory operations in Linux |
| hinic3\_csr.h | Register definitions in the BAR |
| hinic3\_hwif.[ch] | Interface for BAR |
| hinic3\_eqs.[ch] | Interface for AEQs and CEQs |
| hinic3\_mbox.[ch] | Interface for mailbox |
| hinic3\_mgmt.[ch] | Management interface based on mailbox and AEQ |
| hinic3\_wq.[ch] | Work queue data structures and interface |
| hinic3\_cmdq.[ch] | Command queue is used to post command to HW |
| hinic3\_hwdev.[ch] | HW structures and methods abstractions |
| hinic3\_lld.[ch] | Auxiliary driver adaptation layer |
| hinic3\_hw\_comm.[ch] | Interface for common HW operations |
| hinic3\_mgmt\_interface.h | Interface between firmware and driver |
| hinic3\_hw\_cfg.[ch] | Interface for HW configuration |
| hinic3\_irq.c | Interrupt request |
| hinic3\_netdev\_ops.c | Operations registered to Linux kernel stack |
| hinic3\_nic\_dev.h | NIC structures and methods abstractions |
| hinic3\_main.c | Main Linux kernel driver |
| hinic3\_nic\_cfg.[ch] | NIC service configuration |
| hinic3\_nic\_io.[ch] | Management plane interface for TX and RX |
| hinic3\_rss.[ch] | Interface for Receive Side Scaling (RSS) |
| hinic3\_rx.[ch] | Interface for transmit |
| hinic3\_tx.[ch] | Interface for receive |
| hinic3\_ethtool.c | Interface for ethtool operations (ops) |
| hinic3\_filter.c | Interface for MAC address |

## Management Interface

### Asynchronous Event Queue (AEQ)

AEQ receives high priority events from the HW over a descriptor queue.
Every descriptor is a fixed size of 64 bytes. AEQ can receive solicited or
unsolicited events. Every device, VF or PF, can have up to 4 AEQs.
Every AEQ is associated to a dedicated IRQ. AEQ can receive multiple types
of events, but in practice the hinic3 driver ignores all events except for
2 mailbox related events.

### Mailbox

Mailbox is a communication mechanism between the hinic3 driver and the HW.
Each device has an independent mailbox. Driver can use the mailbox to send
requests to management. Driver receives mailbox messages, such as responses
to requests, over the AEQ (using event HINIC3\_AEQ\_FOR\_MBOX). Due to the
limited size of mailbox data register, mailbox messages are sent
segment-by-segment.

Every device can use its mailbox to post request to firmware. The mailbox
can also be used to post requests and responses between the PF and its VFs.

### Completion Event Queue (CEQ)

The implementation of CEQ is the same as AEQ. It receives completion events
from HW over a fixed size descriptor of 32 bits. Every device can have up
to 32 CEQs. Every CEQ has a dedicated IRQ. CEQ only receives solicited
events that are responses to requests from the driver. CEQ can receive
multiple types of events, but in practice the hinic3 driver ignores all
events except for HINIC3\_CMDQ that represents completion of previously
posted commands on a cmdq.

### Command Queue (cmdq)

Every cmdq has a dedicated work queue on which commands are posted.
Commands on the work queue are fixed size descriptor of size 64 bytes.
Completion of a command will be indicated using ctrl bits in the
descriptor that carried the command. Notification of command completions
will also be provided via event on CEQ. Every device has 4 command queues
that are initialized as a set (called cmdqs), each with its own type.
Hinic3 driver only uses type HINIC3\_CMDQ\_SYNC.

### Work Queues(WQ)

Work queues are logical arrays of fixed size WQEs. The array may be spread
over multiple non-contiguous pages using indirection table. Work queues are
used by I/O queues and command queues.

### Global function ID

Every function, PF or VF, has a unique ordinal identification within the device.
Many management commands (mbox or cmdq) contain this ID so HW can apply the
command effect to the right function.

PF is allowed to post management commands to a subordinate VF by specifying the
VFs ID. A VF must provide its own ID. Anti-spoofing in the HW will cause
command from a VF to fail if it contains the wrong ID.
