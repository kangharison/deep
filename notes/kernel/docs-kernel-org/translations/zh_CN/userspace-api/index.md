# Linux 内核用户空间API指南

> 출처(원문): https://docs.kernel.org/translations/zh_CN/userspace-api/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [The Linux kernel user-space API guide](../../../userspace-api/index.html)

翻译:
:   李睿 Rui Li <[me@lirui.org](mailto:me%40lirui.org)>

# Linux 内核用户空间API指南

尽管许多用户空间API的文档被记录在别处（特别是在 [man-pages](https://www.kernel.org/doc/man-pages/) 项目中），
在代码树中仍然可以找到有关用户空间的部分信息。这个手册意在成为这些信息
聚集的地方。

目录

* [无新权限标志](no_new_privs.html)
* [Seccomp BPF (基于过滤器的安全计算)](seccomp_filter.html)
  + [介绍](seccomp_filter.html#id1)
  + [这不是什么](seccomp_filter.html#id2)
  + [用法](seccomp_filter.html#id3)
  + [返回值](seccomp_filter.html#id4)
  + [隐患](seccomp_filter.html#id5)
  + [例子](seccomp_filter.html#id6)
  + [用户空间通知](seccomp_filter.html#id7)
  + [Sysctls](seccomp_filter.html#sysctls)
  + [添加架构支持](seccomp_filter.html#id8)
  + [注意事项](seccomp_filter.html#id9)
* [OpenCAPI （开放相干加速器处理器接口）](accelerators/ocxl.html)
  + [高层视角](accelerators/ocxl.html#id1)
  + [设备发现](accelerators/ocxl.html#id2)
  + [MMIO](accelerators/ocxl.html#mmio)
  + [AFU中断](accelerators/ocxl.html#afu)
  + [字符设备](accelerators/ocxl.html#id3)
  + [Sysfs类](accelerators/ocxl.html#sysfs)
  + [用户API](accelerators/ocxl.html#api)
* [eBPF 用户空间API](ebpf/index.html)
  + [eBPF Syscall](ebpf/syscall.html)
* [平台配置文件选择（如 /sys/firmware/acpi/platform\_profile）](sysfs-platform_profile.html)
* [futex2](futex2.html)
  + [用户API](futex2.html#api)

TODOList:

* landlock
* unshare
* spec\_ctrl
* ioctl/index
* iommu
* media/index
* netlink/index
* vduse
