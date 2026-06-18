# Linux驱动实现者的API指南

> 출처(원문): https://docs.kernel.org/translations/zh_CN/driver-api/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Driver implementer’s API guide](../../../driver-api/index.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# Linux驱动实现者的API指南

内核提供了各种各样的接口来支持设备驱动的开发。这份文档只是对其中一些接口进行了
一定程度的整理——希望随着时间的推移，它能变得更好！可用的小节可以在下面看到。

目录列表

* [通用型输入/输出（GPIO）](gpio/index.html)
  + [核心](gpio/index.html#id1)
  + [ACPI支持](gpio/index.html#acpi)
  + [设备树支持](gpio/index.html#id2)
  + [设备管理支持](gpio/index.html#id3)
  + [sysfs帮助（函数）](gpio/index.html#sysfs)
* [对内存映射地址的I/O写入排序](io_ordering.html)
* [PHY 通用框架](phy/index.html)
  + [PHY子系统](phy/phy.html)

Todolist:

* driver-model/index
* basics
* infrastructure
* ioctl
* early-userspace/index
* pm/index
* clk
* device-io
* dma-buf
* device\_link
* component
* message-based
* infiniband
* aperture
* frame-buffer
* regulator
* reset
* iio/index
* input
* usb/index
* firewire
* pci/index
* cxl/index
* spi
* i2c
* ipmb
* ipmi
* i3c/index
* interconnect
* devfreq
* hsi
* edac
* scsi
* libata
* target
* mailbox
* mtdnand
* miscellaneous
* mei/index
* mtd/index
* mmc/index
* nvdimm/index
* w1
* rapidio/index
* s390-drivers
* vme
* 80211/index
* uio-howto
* firmware/index
* pin-control
* md/index
* media/index
* misc\_devices
* nfc/index
* dmaengine/index
* slimbus
* soundwire/index
* thermal/index
* fpga/index
* acpi/index
* auxiliary\_bus
* backlight/lp855x-driver.rst
* connector
* console
* dcdbas
* eisa
* isa
* isapnp
* io-mapping
* generic-counter
* memory-devices/index
* men-chameleon-bus
* ntb
* nvmem
* parport-lowlevel
* pps
* ptp
* pwm
* pldmfw/index
* rfkill
* serial/index
* sm501
* surface\_aggregator/index
* switchtec
* sync\_file
* tty/index
* vfio-mediated-device
* vfio
* vfio-pci-device-specific-driver-acceptance
* xilinx/index
* xillybus
* zorro
* hte/index
