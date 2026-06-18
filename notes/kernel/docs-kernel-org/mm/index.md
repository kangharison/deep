# Memory Management Documentation

> 출처(원문): https://docs.kernel.org/mm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Memory Management Documentation

This is a guide to understanding the memory management subsystem
of Linux. If you are looking for advice on simply allocating memory,
see the [Memory Allocation Guide](../core-api/memory-allocation.html#memory-allocation). For controlling and tuning guides,
see the [admin guide](../admin-guide/mm/index.html).

* [Physical Memory](physical_memory.html)
* [Page Tables](page_tables.html)
* [Process Addresses](process_addrs.html)
* [Boot Memory](bootmem.html)
* [Page Allocation](page_allocation.html)
* [Virtually Contiguous Memory Allocation](vmalloc.html)
* [Slab Allocation](slab.html)
* [High Memory Handling](highmem.html)
* [Page Reclaim](page_reclaim.html)
* [Swap](swap.html)
* [Swap Table](swap-table.html)
* [Page Cache](page_cache.html)
* [Shared Memory Filesystem](shmfs.html)
* [Out Of Memory Handling](oom.html)

## Unsorted Documentation

This is a collection of unsorted documents about the Linux memory management
(MM) subsystem internals with different level of details ranging from notes and
mailing list responses for elaborating descriptions of data structures and
algorithms. It should all be integrated nicely into the above structured
documentation, or deleted if it has served its purpose.

* [Active MM](active_mm.html)
* [MEMORY ALLOCATION PROFILING](allocation-profiling.html)
* [Architecture Page Table Helpers](arch_pgtable_helpers.html)
* [Memory Balancing](balance.html)
* [DAMON: Data Access MONitoring and Access-aware System Operations](damon/index.html)
* [Free Page Reporting](free_page_reporting.html)
* [Heterogeneous Memory Management (HMM)](hmm.html)
* [hwpoison](hwpoison.html)
* [Hugetlbfs Reservation](hugetlbfs_reserv.html)
* [Kernel Samepage Merging](ksm.html)
* [Physical Memory Model](memory-model.html)
* [Memfd Preservation via LUO](memfd_preservation.html)
* [When do you need to notify inside page table lock ?](mmu_notifier.html)
* [Multi-Gen LRU](multigen_lru.html)
* [What is NUMA?](numa.html)
* [Overcommit Accounting](overcommit-accounting.html)
* [Page migration](page_migration.html)
* [Page fragments](page_frags.html)
* [page owner: Tracking about who allocated each page](page_owner.html)
* [Page Table Check](page_table_check.html)
* [remap\_file\_pages() system call](remap_file_pages.html)
* [Split page table lock](split_page_table_lock.html)
* [Transparent Hugepage Support](transhuge.html)
* [Unevictable LRU Infrastructure](unevictable-lru.html)
* [Virtually Mapped Kernel Stack Support](vmalloced-kernel-stacks.html)
* [A vmemmap diet for HugeTLB and Device DAX](vmemmap_dedup.html)
* [zsmalloc](zsmalloc.html)
