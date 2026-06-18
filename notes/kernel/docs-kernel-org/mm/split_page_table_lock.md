# Split page table lock

> 출처(원문): https://docs.kernel.org/mm/split_page_table_lock.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Split page table lock

Originally, mm->page\_table\_lock spinlock protected all page tables of the
mm\_struct. But this approach leads to poor page fault scalability of
multi-threaded applications due to high contention on the lock. To improve
scalability, split page table lock was introduced.

With split page table lock we have separate per-table lock to serialize
access to the table. At the moment we use split lock for PTE and PMD
tables. Access to higher level tables protected by mm->page\_table\_lock.

There are helpers to lock/unlock a table and other accessor functions:

> * pte\_offset\_map\_lock()
>   :   maps PTE and takes PTE table lock, returns pointer to PTE with
>       pointer to its PTE table lock, or returns NULL if no PTE table;
> * pte\_offset\_map\_ro\_nolock()
>   :   maps PTE, returns pointer to PTE with pointer to its PTE table
>       lock (not taken), or returns NULL if no PTE table;
> * pte\_offset\_map\_rw\_nolock()
>   :   maps PTE, returns pointer to PTE with pointer to its PTE table
>       lock (not taken) and the value of its pmd entry, or returns NULL
>       if no PTE table;
> * pte\_offset\_map()
>   :   maps PTE, returns pointer to PTE, or returns NULL if no PTE table;
> * pte\_unmap()
>   :   unmaps PTE table;
> * pte\_unmap\_unlock()
>   :   unlocks and unmaps PTE table;
> * pte\_alloc\_map\_lock()
>   :   allocates PTE table if needed and takes its lock, returns pointer to
>       PTE with pointer to its lock, or returns NULL if allocation failed;
> * pmd\_lock()
>   :   takes PMD table lock, returns pointer to taken lock;
> * pmd\_lockptr()
>   :   returns pointer to PMD table lock;

Split page table lock for PTE tables is enabled compile-time if
CONFIG\_SPLIT\_PTLOCK\_CPUS (usually 4) is less or equal to NR\_CPUS.
If split lock is disabled, all tables are guarded by mm->page\_table\_lock.

Split page table lock for PMD tables is enabled, if it’s enabled for PTE
tables and the architecture supports it (see below).

## Hugetlb and split page table lock

Hugetlb can support several page sizes. We use split lock only for PMD
level, but not for PUD.

Hugetlb-specific helpers:

> * huge\_pte\_lock()
>   :   takes pmd split lock for PMD\_SIZE page, mm->page\_table\_lock
>       otherwise;
> * huge\_pte\_lockptr()
>   :   returns pointer to table lock;

## Support of split page table lock by an architecture

There’s no need in special enabling of PTE split page table lock: everything
required is done by `pagetable_pte_ctor()` and `pagetable_dtor()`, which
must be called on PTE table allocation / freeing.

Make sure the architecture doesn’t use slab allocator for page table
allocation: slab uses page->slab\_cache for its pages.
This field shares storage with page->ptl.

PMD split lock only makes sense if you have more than two page table
levels.

PMD split lock enabling requires `pagetable_pmd_ctor()` call on PMD table
allocation and `pagetable_dtor()` on freeing.

Allocation usually happens in `pmd_alloc_one()`, freeing in `pmd_free()` and
`pmd_free_tlb()`, but make sure you cover all PMD table allocation / freeing
paths: i.e X86\_PAE preallocate few PMDs on `pgd_alloc()`.

With everything in place you can set CONFIG\_ARCH\_ENABLE\_SPLIT\_PMD\_PTLOCK.

NOTE: `pagetable_pte_ctor()` and `pagetable_pmd_ctor()` can fail -- it must
be handled properly.

## page->ptl

page->ptl is used to access split page table lock, where ‘page’ is `struct
page` of page containing the table. It shares storage with page->private
(and few other fields in union).

To avoid increasing size of `struct page` and have best performance, we use a
trick:

> * if spinlock\_t fits into long, we use page->ptr as spinlock, so we
>   can avoid indirect access and save a cache line.
> * if size of spinlock\_t is bigger then size of long, we use page->ptl as
>   pointer to spinlock\_t and allocate it dynamically. This allows to use
>   split lock with enabled DEBUG\_SPINLOCK or DEBUG\_LOCK\_ALLOC, but costs
>   one more cache line for indirect access;

The spinlock\_t allocated in `pagetable_pte_ctor()` for PTE table and in
`pagetable_pmd_ctor()` for PMD table.

Please, never access page->ptl directly -- use appropriate helper.
