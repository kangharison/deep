# Linux内存管理文档

> 출처(원문): https://docs.kernel.org/translations/zh_CN/mm/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Note

此文件的目的是为让中文读者更容易阅读和理解，而不是作为一个分支。 因此，
如果您对此文件有任何意见或更新，请先尝试更新原始英文文件。
如果您发现本文档与原始文件有任何不同或者有翻译问题，请发建议或者补丁给
该文件的译者，或者请求中文文档维护者和审阅者的帮助。

Original:
:   [Memory Management Documentation](../../../mm/index.html)

翻译:
:   司延腾 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

校译:

# Linux内存管理文档

这是一份关于了解Linux的内存管理子系统的指南。如果你正在寻找关于简单分配内存的
建议，请参阅内存分配指南
([内存分配指南](../core-api/memory-allocation.html))。
关于控制和调整的指南，请看管理指南
([内存管理](../admin-guide/mm/index.html))。

* [高内存处理](highmem.html)

该处剩余文档待原始文档有内容后翻译。

## 遗留文档

这是一个关于Linux内存管理（MM）子系统内部的旧文档的集合，其中有不同层次的细节，
包括注释和邮件列表的回复，用于阐述数据结构和算法的描述。它应该被很好地整合到上述
结构化的文档中，如果它已经完成了它的使命，可以删除。

* [Active MM](active_mm.html)
* [内存平衡](balance.html)
* [DAMON:数据访问监视器](damon/index.html)
* [空闲页报告](free_page_reporting.html)
* [内核同页合并](ksm.html)
* [异构内存管理 (HMM)](hmm.html)
* [hwpoison](hwpoison.html)
* [Hugetlbfs 预留](hugetlbfs_reserv.html)
* [物理内存模型](memory-model.html)
* [什么时候需要页表锁内通知？](mmu_notifier.html)
* [何为非统一内存访问(NUMA)？](numa.html)
* [超量使用审计](overcommit-accounting.html)
* [页面片段](page_frags.html)
* [页面迁移](page_migration.html)
* [page owner: 跟踪谁分配的每个页面](page_owner.html)
* [页表检查](page_table_check.html)
* [页表](page_tables.html)
* [物理内存](physical_memory.html)
* [remap\_file\_pages()系统调用](remap_file_pages.html)
* [分页表锁（split page table lock）](split_page_table_lock.html)
* [支持虚拟映射的内核栈](vmalloced-kernel-stacks.html)
* [zsmalloc](zsmalloc.html)

TODOLIST:
\* arch\_pgtable\_helpers
\* free\_page\_reporting
\* hugetlbfs\_reserv
\* slub
\* transhuge
\* unevictable-lru
