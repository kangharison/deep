# 内存管理

> 출처(원문): https://docs.kernel.org/translations/zh_CN/admin-guide/mm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Memory Management](../../../../admin-guide/mm/index.html)

翻译:
:   徐鑫 xu xin <[xu.xin16@zte.com.cn](mailto:xu.xin16%40zte.com.cn)>

# 内存管理

Linux内存管理子系统，顾名思义，是负责系统中的内存管理。它包括了虚拟内存与请求
分页的实现，内核内部结构和用户空间程序的内存分配、将文件映射到进程地址空间以
及许多其他很酷的事情。

Linux内存管理是一个具有许多可配置设置的复杂系统, 且这些设置中的大多数都可以通
过 `/proc` 文件系统获得，并且可以使用 `sysctl` 进行查询和调整。这些API接
口被描述在Documentation/admin-guide/sysctl/vm.rst文件和 [man 5 proc](http://man7.org/linux/man-pages/man5/proc.5.html) 中。

Linux内存管理有它自己的术语，如果你还不熟悉它，请考虑阅读下面参考：
[Concepts overview](../../../../admin-guide/mm/concepts.html).

在此目录下，我们详细描述了如何与Linux内存管理中的各种机制交互。

* [监测数据访问](damon/index.html)
* [内核同页合并](ksm.html)

Todolist:
\* concepts
\* cma\_debugfs
\* hugetlbpage
\* idle\_page\_tracking
\* memory-hotplug
\* nommu-mmap
\* numa\_memory\_policy
\* numaperf
\* pagemap
\* soft-dirty
\* swap\_numa
\* transhuge
\* userfaultfd
\* zswap
