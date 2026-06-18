# Linux Kernel Driver for Huawei Intelligent NIC(HiNIC) family

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/huawei/hinic.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Kernel Driver for Huawei Intelligent NIC(HiNIC) family

## Overview:

HiNIC is a network interface card for the Data Center Area.

The driver supports a range of link-speed devices (10GbE, 25GbE, 40GbE, etc.).
The driver supports also a negotiated and extendable feature set.

Some HiNIC devices support SR-IOV. This driver is used for Physical Function
(PF).

HiNIC devices support MSI-X interrupt vector for each Tx/Rx queue and
adaptive interrupt moderation.

HiNIC devices support also various offload features such as checksum offload,
TCP Transmit Segmentation Offload(TSO), Receive-Side Scaling(RSS) and
LRO(Large Receive Offload).

## Supported PCI vendor ID/device IDs:

19e5:1822 - HiNIC PF

## Driver Architecture and Source Code:

hinic\_dev - Implement a Logical Network device that is independent from
specific HW details about HW data structure formats.

hinic\_hwdev - Implement the HW details of the device and include the components
for accessing the PCI NIC.

## hinic\_hwdev contains the following components:

## HW Interface:

The interface for accessing the pci device (DMA memory and PCI BARs).
(hinic\_hw\_if.c, hinic\_hw\_if.h)

Configuration Status Registers Area that describes the HW Registers on the
configuration and status BAR0. (hinic\_hw\_csr.h)

## MGMT components:

Asynchronous Event Queues(AEQs) - The event queues for receiving messages from
the MGMT modules on the cards. (hinic\_hw\_eqs.c, hinic\_hw\_eqs.h)

Application Programmable Interface commands(API CMD) - Interface for sending
MGMT commands to the card. (hinic\_hw\_api\_cmd.c, hinic\_hw\_api\_cmd.h)

Management (MGMT) - the PF to MGMT channel that uses API CMD for sending MGMT
commands to the card and receives notifications from the MGMT modules on the
card by AEQs. Also set the addresses of the IO CMDQs in HW.
(hinic\_hw\_mgmt.c, hinic\_hw\_mgmt.h)

## IO components:

Completion Event Queues(CEQs) - The completion Event Queues that describe IO
tasks that are finished. (hinic\_hw\_eqs.c, hinic\_hw\_eqs.h)

Work Queues(WQ) - Contain the memory and operations for use by CMD queues and
the Queue Pairs. The WQ is a Memory Block in a Page. The Block contains
pointers to Memory Areas that are the Memory for the Work Queue Elements(WQEs).
(hinic\_hw\_wq.c, hinic\_hw\_wq.h)

Command Queues(CMDQ) - The queues for sending commands for IO management and is
used to set the QPs addresses in HW. The commands completion events are
accumulated on the CEQ that is configured to receive the CMDQ completion events.
(hinic\_hw\_cmdq.c, hinic\_hw\_cmdq.h)

Queue Pairs(QPs) - The HW Receive and Send queues for Receiving and Transmitting
Data. (hinic\_hw\_qp.c, hinic\_hw\_qp.h, hinic\_hw\_qp\_ctxt.h)

IO - de/constructs all the IO components. (hinic\_hw\_io.c, hinic\_hw\_io.h)

## HW device:

HW device - de/constructs the HW Interface, the MGMT components on the
initialization of the driver and the IO components on the case of Interface
UP/DOWN Events. (hinic\_hw\_dev.c, hinic\_hw\_dev.h)

## hinic\_dev contains the following components:

PCI ID table - Contains the supported PCI Vendor/Device IDs.
(hinic\_pci\_tbl.h)

Port Commands - Send commands to the HW device for port management
(MAC, Vlan, MTU, ...). (hinic\_port.c, hinic\_port.h)

Tx Queues - Logical Tx Queues that use the HW Send Queues for transmit.
The Logical Tx queue is not dependent on the format of the HW Send Queue.
(hinic\_tx.c, hinic\_tx.h)

Rx Queues - Logical Rx Queues that use the HW Receive Queues for receive.
The Logical Rx queue is not dependent on the format of the HW Receive Queue.
(hinic\_rx.c, hinic\_rx.h)

hinic\_dev - de/constructs the Logical Tx and Rx Queues.
(hinic\_main.c, hinic\_dev.h)

## Miscellaneous

Common functions that are used by HW and Logical Device.
(hinic\_common.c, hinic\_common.h)

## Support

If an issue is identified with the released source code on the supported kernel
with a supported adapter, email the specific information related to the issue to
[aviad.krawczyk@huawei.com](mailto:aviad.krawczyk%40huawei.com).
