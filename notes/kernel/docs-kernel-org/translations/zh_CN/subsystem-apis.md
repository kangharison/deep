# 内核子系统文档

> 출처(원문): https://docs.kernel.org/translations/zh_CN/subsystem-apis.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Kernel subsystem documentation](../../subsystem-apis.html)

翻译:
:   唐艺舟 Tang Yizhou <[tangyeechou@gmail.com](mailto:tangyeechou%40gmail.com)>

# 内核子系统文档

这些书籍从内核开发者的角度，详细介绍了特定内核子系统
的如何工作。这里的大部分信息直接取自内核源代码，并
根据需要添加了补充材料（或者至少是我们设法添加的 - 可
能 *不是* 所有的材料都有需要）。

## 核心子系统

* [核心API文档](core-api/index.html)
* [Linux驱动实现者的API指南](driver-api/index.html)
* [Linux内存管理文档](mm/index.html)
* [电源管理](power/index.html)
* [Linux调度器](scheduler/index.html)
* [锁](locking/index.html)

TODOList:

* timers/index

## 人机接口

* [Linux 声音子系统文档](sound/index.html)

TODOList:

* input/index
* hid/index
* gpu/index
* fb/index

## 网络接口

* [infiniband](infiniband/index.html)

TODOList:

* networking/index
* netlabel/index
* isdn/index
* mhi/index

## 存储接口

* [Linux Kernel中的文件系统](filesystems/index.html)
* [SCSI子系统](scsi/index.html)

TODOList:

* cdrom/index
* target/index

**Fixme**: 这里还需要更多的分类组织工作。

* [计数](accounting/index.html)
* [Linux CPUFreq - Linux(TM)内核中的CPU频率和电压升降代码](cpu-freq/index.html)
* [工业 I/O](iio/index.html)
* [Linux虚拟化支持](virt/index.html)
* [安全文档](security/index.html)
* [Linux PCI总线子系统](PCI/index.html)
* [Linux PECI 子系统](peci/index.html)

TODOList:

* fpga/index
* i2c/index
* leds/index
* pcmcia/index
* spi/index
* w1/index
* watchdog/index
* hwmon/index
* accel/index
* crypto/index
* bpf/index
* usb/index
* misc-devices/index
* wmi/index
