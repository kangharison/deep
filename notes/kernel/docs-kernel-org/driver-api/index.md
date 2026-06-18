# Driver implementer’s API guide

> 출처(원문): https://docs.kernel.org/driver-api/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Driver implementer’s API guide

The kernel offers a wide variety of interfaces to support the development
of device drivers. This document is an only somewhat organized collection
of some of those interfaces — it will hopefully get better over time! The
available subsections can be seen below.

## General information for driver authors

This section contains documentation that should, at some point or other, be
of interest to most developers working on device drivers.

* [Driver Basics](basics.html)
* [Driver Model](driver-model/index.html)
* [Device links](device_link.html)
* [Device drivers infrastructure](infrastructure.html)
* [ioctl based interfaces](ioctl.html)
* [CPU and Device Power Management](pm/index.html)

## Useful support libraries

This section contains documentation that should, at some point or other, be
of interest to most developers working on device drivers.

* [Early Userspace](early-userspace/index.html)
* [Kernel Connector](connector.html)
* [Bus-Independent Device Accesses](device-io.html)
* [Device Frequency Scaling](devfreq.html)
* [Buffer Sharing and Synchronization (dma-buf)](dma-buf.html)
* [Component Helper for Aggregate Drivers](component.html)
* [The io\_mapping functions](io-mapping.html)
* [Ordering I/O writes to memory-mapped addresses](io_ordering.html)
* [The Userspace I/O HOWTO](uio-howto.html)
* [VFIO Mediated devices](vfio-mediated-device.html)
* [VFIO - “Virtual Function I/O”](vfio.html)
* [Acceptance criteria for vfio-pci device specific driver variants](vfio-pci-device-specific-driver-acceptance.html)

## Bus-level documentation

* [Auxiliary Bus](auxiliary_bus.html)
* [Compute Express Link](cxl/index.html)
* [EISA bus support](eisa.html)
* [Firewire (IEEE 1394) driver Interface Guide](firewire.html)
* [I3C subsystem](i3c/index.html)
* [ISA Drivers](isa.html)
* [MEN Chameleon Bus](men-chameleon-bus.html)
* [The Linux PCI driver implementer’s API guide](pci/index.html)
* [The Linux RapidIO Subsystem](rapidio/index.html)
* [Linux kernel SLIMbus support](slimbus.html)
* [Linux USB API](usb/index.html)
* [Virtio](virtio/index.html)
* [VME Device Drivers](vme.html)
* [W1: Dallas’ 1-wire bus](w1.html)
* [Xillybus driver for generic FPGA interface](xillybus.html)

## Subsystem-specific APIs

* [Linux 802.11 Driver Developer’s Guide](80211/index.html)
* [ACPI Support](acpi/index.html)
* [Kernel driver lp855x](backlight/lp855x-driver.html)
* [The Common Clk Framework](clk.html)
* [Confidential Computing](coco/index.html)
* [Console Drivers](console.html)
* [Crypto Drivers](crypto/index.html)
* [DMAEngine documentation](dmaengine/index.html)
* [The Linux kernel dpll subsystem](dpll.html)
* [Error Detection And Correction (EDAC) Devices](edac.html)
* [Extcon Device Subsystem](extcon.html)
* [Linux Firmware API](firmware/index.html)
* [FPGA Subsystem](fpga/index.html)
* [Frame Buffer Library](frame-buffer.html)
* [Managing Ownership of the Framebuffer Aperture](aperture.html)
* [Generic Counter Interface](generic-counter.html)
* [Generic Radix Page Table](generic_pt.html)
* [General Purpose Input/Output (GPIO)](gpio/index.html)
* [High Speed Synchronous Serial Interface (HSI)](hsi.html)
* [The Linux Hardware Timestamping Engine (HTE)](hte/index.html)
* [Recoverable Hardware Error Tracking in vmcoreinfo](hw-recoverable-errors.html)
* [I2C and SMBus Subsystem](i2c.html)
* [Industrial I/O](iio/index.html)
* [InfiniBand and Remote DMA (RDMA) Interfaces](infiniband.html)
* [Input Subsystem](input.html)
* [Generic System Interconnect Subsystem](interconnect.html)
* [IPMB Driver for a Satellite MC](ipmb.html)
* [The Linux IPMI Driver](ipmi.html)
* [libATA Developer’s Guide](libata.html)
* [The Common Mailbox Framework](mailbox.html)
* [RAID](md/index.html)
* [Media subsystem kernel internal API](media/index.html)
* [Intel(R) Management Engine Interface (Intel(R) MEI)](mei/index.html)
* [Memory Controller drivers](memory-devices/index.html)
* [Message-based devices](message-based.html)
* [Miscellaneous Devices](misc_devices.html)
* [Parallel Port Devices](miscellaneous.html)
* [16x50 UART Driver](miscellaneous.html#x50-uart-driver)
* [Pulse-Width Modulation (PWM)](miscellaneous.html#pulse-width-modulation-pwm)
* [MMC/SD/SDIO card support](mmc/index.html)
* [Memory Technology Device (MTD)](mtd/index.html)
* [MTD NAND Driver Programming Interface](mtdnand.html)
* [Near Field Communication](nfc/index.html)
* [NTB Drivers](ntb.html)
* [Non-Volatile Memory Device (NVDIMM)](nvdimm/index.html)
* [NVMEM Subsystem](nvmem.html)
* [PARPORT interface documentation](parport-lowlevel.html)
* [Generic PHY Framework](phy/index.html)
* [PINCTRL (PIN CONTROL) subsystem](pin-control.html)
* [PLDM Firmware Flash Update Library](pldmfw/index.html)
* [PPS - Pulse Per Second](pps.html)
* [PTP hardware clock infrastructure for Linux](ptp.html)
* [Pulse Width Modulation (PWM) interface](pwm.html)
* [Power Sequencing API](pwrseq.html)
* [Voltage and current regulator API](regulator.html)
* [Reset controller API](reset.html)
* [rfkill - RF kill switch support](rfkill.html)
* [Writing s390 channel device drivers](s390-drivers.html)
* [SCSI Interfaces Guide](scsi.html)
* [Support for Serial devices](serial/index.html)
* [SM501 Driver](sm501.html)
* [SoundWire Documentation](soundwire/index.html)
* [Serial Peripheral Interface (SPI)](spi.html)
* [Surface System Aggregator Module (SSAM)](surface_aggregator/index.html)
* [Linux Switchtec Support](switchtec.html)
* [Sync File API Guide](sync_file.html)
* [target and iSCSI Interfaces Guide](target.html)
* [TEE (Trusted Execution Environment) driver API](tee.html)
* [Thermal](thermal/index.html)
* [TTY](tty/index.html)
* [WBRF - Wifi Band RFI Mitigations](wbrf.html)
* [WMI Driver API](wmi.html)
* [Xilinx FPGA](xilinx/index.html)
* [Writing Device Drivers for Zorro Devices](zorro.html)
