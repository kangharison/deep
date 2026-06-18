# Linux kernel networking driver for Marvell’s Octeon PCI Endpoint NIC

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/marvell/octeon_ep.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux kernel networking driver for Marvell’s Octeon PCI Endpoint NIC

Network driver for Marvell’s Octeon PCI EndPoint NIC.
Copyright (c) 2020 Marvell International Ltd.

## Contents

* [Overview](#overview)
* [Supported Devices](#supported-devices)
* [Interface Control](#interface-control)

## Overview

This driver implements networking functionality of Marvell’s Octeon PCI
EndPoint NIC.

## Supported Devices

Currently, this driver support following devices:
:   * Network controller: Cavium, Inc. Device b100
    * Network controller: Cavium, Inc. Device b200
    * Network controller: Cavium, Inc. Device b400
    * Network controller: Cavium, Inc. Device b900
    * Network controller: Cavium, Inc. Device ba00
    * Network controller: Cavium, Inc. Device bc00
    * Network controller: Cavium, Inc. Device bd00

## Interface Control

Network Interface control like changing mtu, link speed, link down/up are
done by writing command to mailbox command queue, a mailbox interface
implemented through a reserved region in BAR4.
This driver writes the commands into the mailbox and the firmware on the
Octeon device processes them. The firmware also sends unsolicited notifications
to driver for events suchs as link change, through notification queue
implemented as part of mailbox interface.
