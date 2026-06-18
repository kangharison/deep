# SCSI子系统文档

> 출처(원문): https://docs.kernel.org/translations/zh_CN/scsi/scsi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [SCSI subsystem documentation](../../../scsi/scsi.html)

翻译:
:   郝栋栋 doubled <[doubled@leap-io-kernel.com](mailto:doubled%40leap-io-kernel.com)>

校译:

# SCSI子系统文档

Linux文档项目（LDP）维护了一份描述Linux内核（lk） 2.4中SCSI
子系统的文档。请参考：
<https://www.tldp.org/HOWTO/SCSI-2.4-HOWTO> 。LDP提供单页和
多页的HTML版本，以及PostScript与PDF格式的文档。

## 在SCSI子系统中使用模块的注意事项

Linux内核中的SCSI支持可以根据终端用户的需求以不同的方式模块
化。为了理解你的选择，我们首先需要定义一些术语。

scsi-core（也被称为“中间层”）包含SCSI支持的核心。没有他你将
无法使用任何其他SCSI驱动程序。SCSI核心支持可以是一个模块（
scsi\_mod.o），也可以编译进内核。如果SCSI核心是一个模块，那么
他必须是第一个被加载的SCSI模块，如果你将卸载该模块，那么他必
须是最后一个被卸载的模块。实际上，modprobe和rmmod命令将确保
SCSI子系统中模块加载与卸载的正确顺序。

一旦SCSI核心存在于内核中（无论是编译进内核还是作为模块加载），
独立的上层驱动和底层驱动可以按照任意顺序加载。磁盘驱动程序
（sd\_mod.o）、光盘驱动程序（sr\_mod.o）、磁带驱动程序 [[1]](#id3)
（st.o）以及SCSI通用驱动程序（sg.o）代表了上层驱动，用于控制
相应的各种设备。例如，你可以加载磁带驱动程序来使用磁带驱动器，
然后在不需要该驱动程序时卸载他（并释放相关内存）。

底层驱动程序用于支持您所运行硬件平台支持的不同主机卡。这些不同
的主机卡通常被称为主机总线适配器（HBAs）。例如，aic7xxx.o驱动
程序被用于控制Adaptec所属的所有最新的SCSI控制器。几乎所有的底
层驱动都可以被编译为模块或直接编译进内核。

[[1](#id2)]

磁带驱动程序有一个变种用于控制OnStream磁带设备。其模块
名称为osst.o 。
