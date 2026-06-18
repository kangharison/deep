# 分页表锁（split page table lock）

> 출처(원문): https://docs.kernel.org/translations/zh_CN/mm/split_page_table_lock.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 分页表锁（split page table lock）

最初，mm->page\_table\_lock spinlock保护了mm\_struct的所有页表。但是这种方
法导致了多线程应用程序的缺页异常可扩展性差，因为对锁的争夺很激烈。为了提高可扩
展性，我们引入了分页表锁。

有了分页表锁，我们就有了单独的每张表锁来顺序化对表的访问。目前，我们对PTE和
PMD表使用分页锁。对高层表的访问由mm->page\_table\_lock保护。

有一些辅助工具来锁定/解锁一个表和其他访问器函数:

> * pte\_offset\_map\_lock()
>   :   映射pte并获取PTE表锁，返回所取锁的指针；
> * pte\_unmap\_unlock()
>   :   解锁和解映射PTE表；
> * pte\_alloc\_map\_lock()
>   :   如果需要的话，分配PTE表并获取锁，如果分配失败，返回已获取的锁的指针
>       或NULL；
> * pte\_lockptr()
>   :   返回指向PTE表锁的指针；
> * pmd\_lock()
>   :   取得PMD表锁，返回所取锁的指针。
> * pmd\_lockptr()
>   :   返回指向PMD表锁的指针；

如果CONFIG\_SPLIT\_PTLOCK\_CPUS（通常为4）小于或等于NR\_CPUS，则在编译
时启用PTE表的分页表锁。如果分页锁被禁用，所有的表都由mm->page\_table\_lock
来保护。

如果PMD表启用了分页锁，并且架构支持它，那么PMD表的分页锁就会被启用（见
下文）。

## Hugetlb 和分页表锁

Hugetlb可以支持多种页面大小。我们只对PMD级别使用分页锁，但不对PUD使用。

Hugetlb特定的辅助函数:

> * huge\_pte\_lock()
>   :   对PMD\_SIZE页面采取pmd分割锁，否则mm->page\_table\_lock；
> * huge\_pte\_lockptr()
>   :   返回指向表锁的指针。

## 架构对分页表锁的支持

没有必要特别启用PTE分页表锁：所有需要的东西都由`pagetable_pte_ctor()`
和`pagetable_pte_dtor()`完成，它们必须在PTE表分配/释放时被调用。

确保架构不使用slab分配器来分配页表：slab使用page->slab\_cache来分配其页
面。这个区域与page->ptl共享存储。

PMD分页锁只有在你有两个以上的页表级别时才有意义。

启用PMD分页锁需要在PMD表分配时调用`pagetable_pmd_ctor()`，在释放时调
用`pagetable_pmd_dtor()`。

分配通常发生在`pmd_alloc_one()`中，释放发生在`pmd_free()`和`pmd_free_tlb()`
中，但要确保覆盖所有的PMD表分配/释放路径：即X86\_PAE在`pgd_alloc()`中预先
分配一些PMD。

一切就绪后，你可以设置CONFIG\_ARCH\_ENABLE\_SPLIT\_PMD\_PTLOCK。

注意：`pagetable_pte_ctor()`和`pagetable_pmd_ctor()`可能失败--必
须正确处理。

## page->ptl

page->ptl用于访问分割页表锁，其中’page’是包含该表的页面`struct page`。它
与page->private（以及union中的其他几个字段）共享存储。

为了避免增加`struct page`的大小并获得最佳性能，我们使用了一个技巧:

> * 如果spinlock\_t适合于long，我们使用page->ptr作为spinlock，这样我们
>   就可以避免间接访问并节省一个缓存行。
> * 如果spinlock\_t的大小大于long的大小，我们使用page->ptl作为spinlock\_t
>   的指针并动态分配它。这允许在启用DEBUG\_SPINLOCK或DEBUG\_LOCK\_ALLOC的
>   情况下使用分页锁，但由于间接访问而多花了一个缓存行。

PTE表的spinlock\_t分配在`pagetable_pte_ctor()`中，PMD表的spinlock\_t
分配在`pagetable_pmd_ctor()`中。

请不要直接访问page->ptl - -使用适当的辅助函数。
