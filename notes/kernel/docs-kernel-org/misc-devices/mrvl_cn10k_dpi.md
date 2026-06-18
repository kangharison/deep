# Marvell CN10K DMA packet interface (DPI) driver

> 출처(원문): https://docs.kernel.org/misc-devices/mrvl_cn10k_dpi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Marvell CN10K DMA packet interface (DPI) driver

## Overview

DPI is a DMA packet interface hardware block in Marvell’s CN10K silicon.
DPI hardware comprises a physical function (PF), its virtual functions,
mailbox logic, and a set of DMA engines & DMA command queues.

DPI PF function is an administrative function which services the mailbox
requests from its VF functions and provisions DMA engine resources to
it’s VF functions.

mrvl\_cn10k\_dpi.ko misc driver loads on DPI PF device and services the
mailbox commands submitted by the VF devices and accordingly initializes
the DMA engines and VF device’s DMA command queues. Also, driver creates
/dev/mrvl-cn10k-dpi node to set DMA engine and PEM (PCIe interface) port
attributes like fifo length, molr, mps & mrrs.

DPI PF driver is just an administrative driver to setup its VF device’s
queues and provisions the hardware resources, it cannot initiate any
DMA operations. Only VF devices are provisioned with DMA capabilities.

## Driver location

drivers/misc/mrvl\_cn10k\_dpi.c

## Driver IOCTLs

`DPI_MPS_MRRS_CFG`
ioctl that sets max payload size & max read request size parameters of
a pem port to which DMA engines are wired.

`DPI_ENGINE_CFG`
ioctl that sets DMA engine’s fifo sizes & max outstanding load request
thresholds.

## User space code example

DPI VF devices are probed and accessed from user space applications using
vfio-pci driver. Below is a sample dpi dma application to demonstrate on
how applications use mailbox and ioctl services from DPI PF kernel driver.

<https://github.com/MarvellEmbeddedProcessors/dpi-sample-app>
