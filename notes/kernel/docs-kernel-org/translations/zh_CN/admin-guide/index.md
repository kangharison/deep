# Linux 内核用户和管理员指南

> 출처(원문): https://docs.kernel.org/translations/zh_CN/admin-guide/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [The Linux kernel user’s and administrator’s guide](../../../admin-guide/index.html)

Translator:
:   Alex Shi <[alex.shi@linux.alibaba.com](mailto:alex.shi%40linux.alibaba.com)>

# Linux 内核用户和管理员指南

下面是一组随时间添加到内核中的面向用户的文档的集合。到目前为止，还没有一个
整体的顺序或组织 - 这些材料不是一个单一的，连贯的文件！幸运的话，情况会随着
时间的推移而迅速改善。

这个初始部分包含总体信息，包括描述内核的README， 关于内核参数的文档等。

* [Linux内核6.x版本 <http://kernel.org/>](README.html)

Todolist:

* kernel-parameters
* devices
* sysctl/index

本节介绍CPU漏洞及其缓解措施。

Todolist:

* hw-vuln/index

下面的一组文档，针对的是试图跟踪问题和bug的用户。

* [报告问题](reporting-issues.html)
* [报告回归问题](reporting-regressions.html)
* [追踪缺陷](bug-hunting.html)
* [二分（bisect）缺陷](bug-bisect.html)
* [受污染的内核](tainted-kernels.html)
* [解释“No working init found.”启动挂起消息](init.html)

Todolist:

* ramoops
* dynamic-debug-howto
* kdump/index
* perf/index

这是应用程序开发人员感兴趣的章节的开始。可以在这里找到涵盖内核ABI各个
方面的文档。

Todolist:

* sysfs-rules

本手册的其余部分包括各种指南，介绍如何根据您的喜好配置内核的特定行为。

* [引导配置](bootconfig.html)
* [清除 WARN\_ONCE](clearing-warn-once.html)
* [CPU 负载](cpu-load.html)
* [如何通过sysfs将CPU拓扑导出](cputopology.html)
* [Softlockup与hardlockup检测机制(又名:nmi\_watchdog)](lockup-watchdogs.html)
* [Numa策略命中/未命中统计](numastat.html)
* [Unicode（统一码）支持](unicode.html)
* [Linux 魔法系统请求键骇客](sysrq.html)
* [内存管理](mm/index.html)

Todolist:

* acpi/index
* aoe/index
* auxdisplay/index
* bcache
* binderfs
* binfmt-misc
* blockdev/index
* braille-console
* btmrvl
* cgroup-v1/index
* cgroup-v2
* cifs/index
* dell\_rbu
* device-mapper/index
* edid
* efi-stub
* ext4
* nfs/index
* gpio/index
* highuid
* hw\_random
* initrd
* iostats
* java
* jfs
* kernel-per-CPU-kthreads
* laptops/index
* lcd-panel-cgram
* ldm
* LSM/index
* md
* media/index
* module-signing
* mono
* namespaces/index
* parport
* perf-security
* pm/index
* pnp
* rapidio
* ras
* rtc
* serial-console
* svga
* thunderbolt
* ufs
* vga-softcursor
* video-output
* xfs
