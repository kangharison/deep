# RDMA Driver for the AMD Pensando(R) Ethernet adapter family

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/pensando/ionic_rdma.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# RDMA Driver for the AMD Pensando(R) Ethernet adapter family

AMD Pensando RDMA driver.
Copyright (C) 2018-2025, Advanced Micro Devices, Inc.

## Overview

The ionic\_rdma driver provides Remote Direct Memory Access functionality
for AMD Pensando DSC (Distributed Services Card) devices. This driver
implements RDMA capabilities as an auxiliary driver that operates in
conjunction with the ionic ethernet driver.

The ionic ethernet driver detects RDMA capability during device
initialization and creates auxiliary devices that the ionic\_rdma driver
binds to, establishing the RDMA data path and control interfaces.

## Identifying the Adapter

See [Linux Driver for the Pensando(R) Ethernet adapter family](ionic.html)
for more information on identifying the adapter.

## Enabling the driver

The ionic\_rdma driver depends on the ionic ethernet driver.
See [Linux Driver for the Pensando(R) Ethernet adapter family](ionic.html)
for detailed information on enabling and configuring the ionic driver.

The ionic\_rdma driver is enabled via the standard kernel configuration system,
using the make command:

```
make oldconfig/menuconfig/etc.
```

The driver is located in the menu structure at:

> -> Device Drivers
> :   -> InfiniBand support
>     :   -> AMD Pensando DSC RDMA/RoCE Support

## Support

For general Linux RDMA support, please use the RDMA mailing
list, which is monitored by AMD Pensando personnel:

```
linux-rdma@vger.kernel.org
```
