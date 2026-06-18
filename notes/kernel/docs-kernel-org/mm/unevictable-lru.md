# Unevictable LRU Infrastructure

> 출처(원문): https://docs.kernel.org/mm/unevictable-lru.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Unevictable LRU Infrastructure

## [Introduction](#id1)

This document describes the Linux memory manager’s “Unevictable LRU”
infrastructure and the use of this to manage several types of “unevictable”
folios.

The document attempts to provide the overall rationale behind this mechanism
and the rationale for some of the design decisions that drove the
implementation. The latter design rationale is discussed in the context of an
implementation description. Admittedly, one can obtain the implementation
details - the “what does it do?” - by reading the code. One hopes that the
descriptions below add value by provide the answer to “why does it do that?”.

## [The Unevictable LRU](#id2)

The Unevictable LRU facility adds an additional LRU list to track unevictable
folios and to hide these folios from vmscan. This mechanism is based on a patch
by Larry Woodman of Red Hat to address several scalability problems with folio
reclaim in Linux. The problems have been observed at customer sites on large
memory x86\_64 systems.

To illustrate this with an example, a non-NUMA x86\_64 platform with 128GB of
main memory will have over 32 million 4k pages in a single node. When a large
fraction of these pages are not evictable for any reason [see below], vmscan
will spend a lot of time scanning the LRU lists looking for the small fraction
of pages that are evictable. This can result in a situation where all CPUs are
spending 100% of their time in vmscan for hours or days on end, with the system
completely unresponsive.

The unevictable list addresses the following classes of unevictable pages:

> * Those owned by ramfs.
> * Those owned by tmpfs with the noswap mount option.
> * Those mapped into SHM\_LOCK’d shared memory regions.
> * Those mapped into VM\_LOCKED [`mlock()`ed] VMAs.

The infrastructure may also be able to handle other conditions that make pages
unevictable, either by definition or by circumstance, in the future.

### [The Unevictable LRU Folio List](#id3)

The Unevictable LRU folio list is a lie. It was never an LRU-ordered
list, but a companion to the LRU-ordered anonymous and file, active and
inactive folio lists; and now it is not even a folio list. But following
familiar convention, here in this document and in the source, we often
imagine it as a fifth LRU folio list.

The Unevictable LRU infrastructure consists of an additional, per-node, LRU list
called the “unevictable” list and an associated folio flag, PG\_unevictable, to
indicate that the folio is being managed on the unevictable list.

The PG\_unevictable flag is analogous to, and mutually exclusive with, the
PG\_active flag in that it indicates on which LRU list a folio resides when
PG\_lru is set.

The Unevictable LRU infrastructure maintains unevictable folios as if they were
on an additional LRU list for a few reasons:

> 1. We get to “treat unevictable folios just like we treat other folios in the
>    system - which means we get to use the same code to manipulate them, the
>    same code to isolate them (for migrate, etc.), the same code to keep track
>    of the statistics, etc...” [Rik van Riel]
> 2. We want to be able to migrate unevictable folios between nodes for memory
>    defragmentation, workload management and memory hotplug. The Linux kernel
>    can only migrate folios that it can successfully isolate from the LRU
>    lists (or “Movable” folios: outside of consideration here). If we were to
>    maintain folios elsewhere than on an LRU-like list, where they can be
>    detected by [`folio_isolate_lru()`](../core-api/mm-api.html#c.folio_isolate_lru "folio_isolate_lru"), we would prevent their migration.

The unevictable list does not differentiate between file-backed and
anonymous, swap-backed folios. This differentiation is only important
while the folios are, in fact, evictable.

The unevictable list benefits from the “arrayification” of the per-node LRU
lists and statistics originally proposed and posted by Christoph Lameter.

### [Memory Control Group Interaction](#id4)

The unevictable LRU facility interacts with the memory control group [aka
memory controller; see [Memory Resource Controller](../admin-guide/cgroup-v1/memory.html)] by
extending the lru\_list enum.

The memory controller data structure automatically gets a per-node unevictable
list as a result of the “arrayification” of the per-node LRU lists (one per
lru\_list `enum element`). The memory controller tracks the movement of pages to
and from the unevictable list.

When a memory control group comes under memory pressure, the controller will
not attempt to reclaim pages on the unevictable list. This has a couple of
effects:

> 1. Because the pages are “hidden” from reclaim on the unevictable list, the
>    reclaim process can be more efficient, dealing only with pages that have a
>    chance of being reclaimed.
> 2. On the other hand, if too many of the pages charged to the control group
>    are unevictable, the evictable portion of the working set of the tasks in
>    the control group may not fit into the available memory. This can cause
>    the control group to thrash or to OOM-kill tasks.

### [Marking Address Spaces Unevictable](#id5)

For facilities such as ramfs none of the pages attached to the address space
may be evicted. To prevent eviction of any such pages, the AS\_UNEVICTABLE
address space flag is provided, and this can be manipulated by a filesystem
using a number of wrapper functions:

> * `void mapping_set_unevictable(struct address_space *mapping);`
>
>   > Mark the address space as being completely unevictable.
> * `void mapping_clear_unevictable(struct address_space *mapping);`
>
>   > Mark the address space as being evictable.
> * `int mapping_unevictable(struct address_space *mapping);`
>
>   > Query the address space, and return true if it is completely
>   > unevictable.

These are currently used in three places in the kernel:

> 1. By ramfs to mark the address spaces of its inodes when they are created,
>    and this mark remains for the life of the inode.
> 2. By SYSV SHM to mark SHM\_LOCK’d address spaces until SHM\_UNLOCK is called.
>    Note that SHM\_LOCK is not required to page in the locked pages if they’re
>    swapped out; the application must touch the pages manually if it wants to
>    ensure they’re in memory.
> 3. By the i915 driver to mark pinned address space until it’s unpinned. The
>    amount of unevictable memory marked by i915 driver is roughly the bounded
>    object size in debugfs/dri/0/i915\_gem\_objects.

### [Detecting Unevictable Pages](#id6)

The function `folio_evictable()` in mm/internal.h determines whether a folio is
evictable or not using the query function outlined above [see section
[Marking address spaces unevictable](#mark-addr-space-unevict)]
to check the AS\_UNEVICTABLE flag.

For address spaces that are so marked after being populated (as SHM regions
might be), the lock action (e.g. SHM\_LOCK) can be lazy, and need not populate
the page tables for the region as does, for example, `mlock()`, nor need it make
any special effort to push any pages in the SHM\_LOCK’d area to the unevictable
list. Instead, vmscan will do this if and when it encounters the folios during
a reclamation scan.

On an unlock action (such as SHM\_UNLOCK), the unlocker (e.g. `shmctl()`) must scan
the pages in the region and “rescue” them from the unevictable list if no other
condition is keeping them unevictable. If an unevictable region is destroyed,
the pages are also “rescued” from the unevictable list in the process of
freeing them.

`folio_evictable()` also checks for mlocked folios by calling
`folio_test_mlocked()`, which is set when a folio is faulted into a
VM\_LOCKED VMA, or found in a VMA being VM\_LOCKED.

### [Vmscan’s Handling of Unevictable Folios](#id7)

If unevictable folios are culled in the fault path, or moved to the unevictable
list at `mlock()` or mmap() time, vmscan will not encounter the folios until they
have become evictable again (via `munlock()` for example) and have been “rescued”
from the unevictable list. However, there may be situations where we decide,
for the sake of expediency, to leave an unevictable folio on one of the regular
active/inactive LRU lists for vmscan to deal with. vmscan checks for such
folios in all of the shrink\_{active|inactive|folio}`_list()` functions and will
“cull” such folios that it encounters: that is, it diverts those folios to the
unevictable list for the memory cgroup and node being scanned.

There may be situations where a folio is mapped into a VM\_LOCKED VMA,
but the folio does not have the mlocked flag set. Such folios will make
it all the way to `shrink_active_list()` or `shrink_folio_list()` where they
will be detected when vmscan walks the reverse map in [`folio_referenced()`](../core-api/mm-api.html#c.folio_referenced "folio_referenced")
or [`try_to_unmap()`](../core-api/mm-api.html#c.try_to_unmap "try_to_unmap"). The folio is culled to the unevictable list when it
is released by the shrinker.

To “cull” an unevictable folio, vmscan simply puts the folio back on
the LRU list using [`folio_putback_lru()`](../core-api/mm-api.html#c.folio_putback_lru "folio_putback_lru") - the inverse operation to
[`folio_isolate_lru()`](../core-api/mm-api.html#c.folio_isolate_lru "folio_isolate_lru") - after dropping the folio lock. Because the
condition which makes the folio unevictable may change once the folio
is unlocked, `__pagevec_lru_add_fn()` will recheck the unevictable state
of a folio before placing it on the unevictable list.

## [MLOCKED Pages](#id8)

The unevictable folio list is also useful for `mlock()`, in addition to ramfs and
SYSV SHM. Note that `mlock()` is only available in CONFIG\_MMU=y situations; in
NOMMU situations, all mappings are effectively mlocked.

### [History](#id9)

The “Unevictable mlocked Pages” infrastructure is based on work originally
posted by Nick Piggin in an RFC patch entitled “mm: mlocked pages off LRU”.
Nick posted his patch as an alternative to a patch posted by Christoph Lameter
to achieve the same objective: hiding mlocked pages from vmscan.

In Nick’s patch, he used one of the `struct page` LRU list link fields as a count
of VM\_LOCKED VMAs that map the page (Rik van Riel had the same idea three years
earlier). But this use of the link field for a count prevented the management
of the pages on an LRU list, and thus mlocked pages were not migratable as
[`folio_isolate_lru()`](../core-api/mm-api.html#c.folio_isolate_lru "folio_isolate_lru") could not detect them, and the LRU list link field was not
available to the migration subsystem.

Nick resolved this by putting mlocked pages back on the LRU list before
attempting to isolate them, thus abandoning the count of VM\_LOCKED VMAs. When
Nick’s patch was integrated with the Unevictable LRU work, the count was
replaced by walking the reverse map when munlocking, to determine whether any
other VM\_LOCKED VMAs still mapped the page.

However, walking the reverse map for each page when munlocking was ugly and
inefficient, and could lead to catastrophic contention on a file’s rmap lock,
when many processes which had it mlocked were trying to exit. In 5.18, the
idea of keeping mlock\_count in Unevictable LRU list link field was revived and
put to work, without preventing the migration of mlocked pages. This is why
the “Unevictable LRU list” cannot be a linked list of pages now; but there was
no use for that linked list anyway - though its size is maintained for meminfo.

### [Basic Management](#id10)

mlocked pages - pages mapped into a VM\_LOCKED VMA - are a class of unevictable
pages. When such a page has been “noticed” by the memory management subsystem,
the folio is marked with the PG\_mlocked flag. This can be manipulated using
`folio_set_mlocked()` and `folio_clear_mlocked()` functions.

A PG\_mlocked page will be placed on the unevictable list when it is added to
the LRU. Such pages can be “noticed” by memory management in several places:

> 1. in the `mlock()`/`mlock2()`/`mlockall()` system call handlers;
> 2. in the mmap() system call handler when mmapping a region with the
>    MAP\_LOCKED flag;
> 3. mmapping a region in a task that has called `mlockall()` with the MCL\_FUTURE
>    flag;
> 4. in the fault path and when a VM\_LOCKED stack segment is expanded; or
> 5. as mentioned above, in vmscan:`shrink_folio_list()` when attempting to
>    reclaim a page in a VM\_LOCKED VMA by [`folio_referenced()`](../core-api/mm-api.html#c.folio_referenced "folio_referenced") or [`try_to_unmap()`](../core-api/mm-api.html#c.try_to_unmap "try_to_unmap").

mlocked pages become unlocked and rescued from the unevictable list when:

> 1. mapped in a range unlocked via the `munlock()`/`munlockall()` system calls;
> 2. `munmap()`’d out of the last VM\_LOCKED VMA that maps the page, including
>    unmapping at task exit;
> 3. when the page is truncated from the last VM\_LOCKED VMA of an mmapped file;
>    or
> 4. before a page is COW’d in a VM\_LOCKED VMA.

### [mlock()/mlock2()/mlockall() System Call Handling](#id11)

`mlock()`, `mlock2()` and `mlockall()` system call handlers proceed to `mlock_fixup()`
for each VMA in the range specified by the call. In the case of `mlockall()`,
this is the entire active address space of the task. Note that `mlock_fixup()`
is used for both mlocking and munlocking a range of memory. A call to `mlock()`
an already VM\_LOCKED VMA, or to `munlock()` a VMA that is not VM\_LOCKED, is
treated as a no-op and `mlock_fixup()` simply returns.

If the VMA passes some filtering as described in “Filtering Special VMAs”
below, `mlock_fixup()` will attempt to merge the VMA with its neighbors or split
off a subset of the VMA if the range does not cover the entire VMA. Any pages
already present in the VMA are then marked as mlocked by `mlock_folio()` via
`mlock_pte_range()` via `walk_page_range()` via `mlock_vma_pages_range()`.

Before returning from the system call, `do_mlock()` or `mlockall()` will call
`__mm_populate()` to fault in the remaining pages via `get_user_pages()` and to
mark those pages as mlocked as they are faulted.

Note that the VMA being mlocked might be mapped with PROT\_NONE. In this case,
`get_user_pages()` will be unable to fault in the pages. That’s okay. If pages
do end up getting faulted into this VM\_LOCKED VMA, they will be handled in the
fault path - which is also how `mlock2()`’s MLOCK\_ONFAULT areas are handled.

For each PTE (or PMD) being faulted into a VMA, the page add rmap function
calls `mlock_vma_folio()`, which calls `mlock_folio()` when the VMA is VM\_LOCKED
(unless it is a PTE mapping of a part of a transparent huge page). Or when
it is a newly allocated anonymous page, [`folio_add_lru_vma()`](../core-api/mm-api.html#c.folio_add_lru_vma "folio_add_lru_vma") calls
`mlock_new_folio()` instead: similar to `mlock_folio()`, but can make better
judgments, since this page is held exclusively and known not to be on LRU yet.

`mlock_folio()` sets PG\_mlocked immediately, then places the page on the CPU’s
mlock folio batch, to batch up the rest of the work to be done under lru\_lock by
`__mlock_folio()`. `__mlock_folio()` sets PG\_unevictable, initializes mlock\_count
and moves the page to unevictable state (“the unevictable LRU”, but with
mlock\_count in place of LRU threading). Or if the page was already PG\_lru
and PG\_unevictable and PG\_mlocked, it simply increments the mlock\_count.

But in practice that may not work ideally: the page may not yet be on an LRU, or
it may have been temporarily isolated from LRU. In such cases the mlock\_count
field cannot be touched, but will be set to 0 later when `__munlock_folio()`
returns the page to “LRU”. Races prohibit mlock\_count from being set to 1 then:
rather than risk stranding a page indefinitely as unevictable, always err with
mlock\_count on the low side, so that when munlocked the page will be rescued to
an evictable LRU, then perhaps be mlocked again later if vmscan finds it in a
VM\_LOCKED VMA.

### [Filtering Special VMAs](#id12)

`mlock_fixup()` filters several classes of “special” VMAs:

1. VMAs with VM\_IO or VM\_PFNMAP set are skipped entirely. The pages behind
   these mappings are inherently pinned, so we don’t need to mark them as
   mlocked. In any case, most of the pages have no `struct page` in which to so
   mark the page. Because of this, `get_user_pages()` will fail for these VMAs,
   so there is no sense in attempting to visit them.
2. VMAs mapping hugetlbfs page are already effectively pinned into memory. We
   neither need nor want to `mlock()` these pages. But `__mm_populate()` includes
   hugetlbfs ranges, allocating the huge pages and populating the PTEs.
3. VMAs with VM\_DONTEXPAND are generally userspace mappings of kernel pages,
   such as the VDSO page, relay channel pages, etc. These pages are inherently
   unevictable and are not managed on the LRU lists. `__mm_populate()` includes
   these ranges, populating the PTEs if not already populated.
4. VMAs with VM\_MIXEDMAP set are not marked VM\_LOCKED, but `__mm_populate()`
   includes these ranges, populating the PTEs if not already populated.

Note that for all of these special VMAs, `mlock_fixup()` does not set the
VM\_LOCKED flag. Therefore, we won’t have to deal with them later during
`munlock()`, `munmap()` or task exit. Neither does `mlock_fixup()` account these
VMAs against the task’s “locked\_vm”.

### [munlock()/munlockall() System Call Handling](#id13)

The `munlock()` and `munlockall()` system calls are handled by the same
`mlock_fixup()` function as `mlock()`, `mlock2()` and `mlockall()` system calls are.
If called to munlock an already munlocked VMA, `mlock_fixup()` simply returns.
Because of the VMA filtering discussed above, VM\_LOCKED will not be set in
any “special” VMAs. So, those VMAs will be ignored for munlock.

If the VMA is VM\_LOCKED, `mlock_fixup()` again attempts to merge or split off the
specified range. All pages in the VMA are then munlocked by `munlock_folio()` via
`mlock_pte_range()` via `walk_page_range()` via `mlock_vma_pages_range()` - the same
function used when mlocking a VMA range, with new flags for the VMA indicating
that it is `munlock()` being performed.

`munlock_folio()` uses the mlock pagevec to batch up work to be done
under lru\_lock by `__munlock_folio()`. `__munlock_folio()` decrements the
folio’s mlock\_count, and when that reaches 0 it clears the mlocked flag
and clears the unevictable flag, moving the folio from unevictable state
to the inactive LRU.

But in practice that may not work ideally: the folio may not yet have reached
“the unevictable LRU”, or it may have been temporarily isolated from it. In
those cases its mlock\_count field is unusable and must be assumed to be 0: so
that the folio will be rescued to an evictable LRU, then perhaps be mlocked
again later if vmscan finds it in a VM\_LOCKED VMA.

### [Migrating MLOCKED Pages](#id14)

A page that is being migrated has been isolated from the LRU lists and is held
locked across unmapping of the page, updating the page’s address space entry
and copying the contents and state, until the page table entry has been
replaced with an entry that refers to the new page. Linux supports migration
of mlocked pages and other unevictable pages. PG\_mlocked is cleared from the
the old page when it is unmapped from the last VM\_LOCKED VMA, and set when the
new page is mapped in place of migration entry in a VM\_LOCKED VMA. If the page
was unevictable because mlocked, PG\_unevictable follows PG\_mlocked; but if the
page was unevictable for other reasons, PG\_unevictable is copied explicitly.

Note that page migration can race with mlocking or munlocking of the same page.
There is mostly no problem since page migration requires unmapping all PTEs of
the old page (including munlock where VM\_LOCKED), then mapping in the new page
(including mlock where VM\_LOCKED). The page table locks provide sufficient
synchronization.

However, since `mlock_vma_pages_range()` starts by setting VM\_LOCKED on a VMA,
before mlocking any pages already present, if one of those pages were migrated
before `mlock_pte_range()` reached it, it would get counted twice in mlock\_count.
To prevent that, `mlock_vma_pages_range()` temporarily marks the VMA as VM\_IO,
so that `mlock_vma_folio()` will skip it.

To complete page migration, we place the old and new pages back onto the LRU
afterwards. The “unneeded” page - old page on success, new page on failure -
is freed when the reference count held by the migration process is released.

### [Compacting MLOCKED Pages](#id15)

The memory map can be scanned for compactable regions and the default behavior
is to let unevictable pages be moved. /proc/sys/vm/compact\_unevictable\_allowed
controls this behavior (see [Documentation for /proc/sys/vm/](../admin-guide/sysctl/vm.html)). The work
of compaction is mostly handled by the page migration code and the same work
flow as described in Migrating MLOCKED Pages will apply.

### [MLOCKING Transparent Huge Pages](#id16)

A transparent huge page is represented by a single entry on an LRU list.
Therefore, we can only make unevictable an entire compound page, not
individual subpages.

If a user tries to `mlock()` part of a huge page, and no user `mlock()`s the
whole of the huge page, we want the rest of the page to be reclaimable.

We cannot just split the page on partial `mlock()` as `split_huge_page()` can
fail and a new intermittent failure mode for the syscall is undesirable.

We handle this by keeping PTE-mlocked huge pages on evictable LRU lists:
the PMD on the border of a VM\_LOCKED VMA will be split into a PTE table.

This way the huge page is accessible for vmscan. Under memory pressure the
page will be split, subpages which belong to VM\_LOCKED VMAs will be moved
to the unevictable LRU and the rest can be reclaimed.

/proc/meminfo’s Unevictable and Mlocked amounts do not include those parts
of a transparent huge page which are mapped only by PTEs in VM\_LOCKED VMAs.

### [mmap(MAP\_LOCKED) System Call Handling](#id17)

In addition to the `mlock()`, `mlock2()` and `mlockall()` system calls, an application
can request that a region of memory be mlocked by supplying the MAP\_LOCKED flag
to the mmap() call. There is one important and subtle difference here, though.
mmap() + `mlock()` will fail if the range cannot be faulted in (e.g. because
mm\_populate fails) and returns with ENOMEM while mmap(MAP\_LOCKED) will not fail.
The mmapped area will still have properties of the locked area - pages will not
get swapped out - but major page faults to fault memory in might still happen.

Furthermore, any mmap() call or `brk()` call that expands the heap by a task
that has previously called `mlockall()` with the MCL\_FUTURE flag will result
in the newly mapped memory being mlocked. Before the unevictable/mlock
changes, the kernel simply called `make_pages_present()` to allocate pages
and populate the page table.

To mlock a range of memory under the unevictable/mlock infrastructure,
the mmap() handler and task address space expansion functions call
`populate_vma_page_range()` specifying the vma and the address range to mlock.

### [munmap()/exit()/exec() System Call Handling](#id18)

When unmapping an mlocked region of memory, whether by an explicit call to
`munmap()` or via an internal unmap from `exit()` or `exec()` processing, we must
munlock the pages if we’re removing the last VM\_LOCKED VMA that maps the pages.
Before the unevictable/mlock changes, mlocking did not mark the pages in any
way, so unmapping them required no processing.

For each PTE (or PMD) being unmapped from a VMA, folio\_remove\_rmap\_\*() calls
`munlock_vma_folio()`, which calls `munlock_folio()` when the VMA is VM\_LOCKED
(unless it was a PTE mapping of a part of a transparent huge page).

`munlock_folio()` uses the mlock pagevec to batch up work to be done
under lru\_lock by `__munlock_folio()`. `__munlock_folio()` decrements the
folio’s mlock\_count, and when that reaches 0 it clears the mlocked flag
and clears the unevictable flag, moving the folio from unevictable state
to the inactive LRU.

But in practice that may not work ideally: the folio may not yet have reached
“the unevictable LRU”, or it may have been temporarily isolated from it. In
those cases its mlock\_count field is unusable and must be assumed to be 0: so
that the folio will be rescued to an evictable LRU, then perhaps be mlocked
again later if vmscan finds it in a VM\_LOCKED VMA.

### [Truncating MLOCKED Pages](#id19)

File truncation or hole punching forcibly unmaps the deleted pages from
userspace; truncation even unmaps and deletes any private anonymous pages
which had been Copied-On-Write from the file pages now being truncated.

Mlocked pages can be munlocked and deleted in this way: like with `munmap()`,
for each PTE (or PMD) being unmapped from a VMA, folio\_remove\_rmap\_\*() calls
`munlock_vma_folio()`, which calls `munlock_folio()` when the VMA is VM\_LOCKED
(unless it was a PTE mapping of a part of a transparent huge page).

However, if there is a racing `munlock()`, since `mlock_vma_pages_range()` starts
munlocking by clearing VM\_LOCKED from a VMA, before munlocking all the pages
present, if one of those pages were unmapped by truncation or hole punch before
`mlock_pte_range()` reached it, it would not be recognized as mlocked by this VMA,
and would not be counted out of mlock\_count. In this rare case, a page may
still appear as PG\_mlocked after it has been fully unmapped: and it is left to
[`release_pages()`](../core-api/mm-api.html#c.release_pages "release_pages") (or `__page_cache_release()`) to clear it and update statistics
before freeing (this event is counted in /proc/vmstat unevictable\_pgs\_cleared,
which is usually 0).

### [Page Reclaim in shrink\_\*\_list()](#id20)

vmscan’s `shrink_active_list()` culls any obviously unevictable pages -
i.e. !page\_evictable(page) pages - diverting those to the unevictable list.
However, `shrink_active_list()` only sees unevictable pages that made it onto the
active/inactive LRU lists. Note that these pages do not have PG\_unevictable
set - otherwise they would be on the unevictable list and `shrink_active_list()`
would never see them.

Some examples of these unevictable pages on the LRU lists are:

> 1. ramfs pages that have been placed on the LRU lists when first allocated.
> 2. SHM\_LOCK’d shared memory pages. shmctl(SHM\_LOCK) does not attempt to
>    allocate or fault in the pages in the shared memory region. This happens
>    when an application accesses the page the first time after SHM\_LOCK’ing
>    the segment.
> 3. pages still mapped into VM\_LOCKED VMAs, which should be marked mlocked,
>    but events left mlock\_count too low, so they were munlocked too early.

vmscan’s `shrink_inactive_list()` and `shrink_folio_list()` also divert obviously
unevictable pages found on the inactive lists to the appropriate memory cgroup
and node unevictable list.

rmap’s `folio_referenced_one()`, called via vmscan’s `shrink_active_list()` or
`shrink_folio_list()`, and rmap’s `try_to_unmap_one()` called via `shrink_folio_list()`,
check for (3) pages still mapped into VM\_LOCKED VMAs, and call `mlock_vma_folio()`
to correct them. Such pages are culled to the unevictable list when released
by the shrinker.
