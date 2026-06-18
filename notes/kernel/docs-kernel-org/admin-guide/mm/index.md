# Memory Management

> 출처(원문): https://docs.kernel.org/admin-guide/mm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Memory Management

Linux memory management subsystem is responsible, as the name implies,
for managing the memory in the system. This includes implementation of
virtual memory and demand paging, memory allocation both for kernel
internal structures and user space programs, mapping of files into
processes address space and many other cool things.

Linux memory management is a complex system with many configurable
settings. Most of these settings are available via `/proc`
filesystem and can be queried and adjusted using `sysctl`. These APIs
are described in [Documentation for /proc/sys/vm/](../sysctl/vm.html) and in [man 5 proc](http://man7.org/linux/man-pages/man5/proc.5.html).

Linux memory management has its own jargon and if you are not yet
familiar with it, consider reading [Concepts overview](concepts.html).

Here we document in detail how to interact with various mechanisms in
the Linux memory management.

* [Concepts overview](concepts.html)
* [CMA Debugfs Interface](cma_debugfs.html)
* [DAMON: Data Access MONitoring and Access-aware System Operations](damon/index.html)
* [HugeTLB Pages](hugetlbpage.html)
* [Idle Page Tracking](idle_page_tracking.html)
* [Kernel Samepage Merging](ksm.html)
* [Memory Hot(Un)Plug](memory-hotplug.html)
* [Multi-Gen LRU](multigen_lru.html)
* [No-MMU memory mapping support](nommu-mmap.html)
* [NUMA Memory Policy](numa_memory_policy.html)
* [NUMA Memory Performance](numaperf.html)
* [Examining Process Page Tables](pagemap.html)
* [Shrinker Debugfs Interface](shrinker_debugfs.html)
* [Short users guide for the slab allocator](slab.html)
* [Soft-Dirty PTEs](soft-dirty.html)
* [Transparent Hugepage Support](transhuge.html)
* [Userfaultfd](userfaultfd.html)
* [zswap](zswap.html)
* [Kexec Handover Usage](kho.html)
