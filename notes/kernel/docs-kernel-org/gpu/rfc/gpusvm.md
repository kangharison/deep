# GPU SVM Section

> 출처(원문): https://docs.kernel.org/gpu/rfc/gpusvm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPU SVM Section

## Agreed upon design principles

* migrate\_to\_ram path
  :   + Rely only on core MM concepts (migration PTEs, page references, and
        page locking).
      + No driver specific locks other than locks for hardware interaction in
        this path. These are not required and generally a bad idea to
        invent driver defined locks to seal core MM races.
      + An example of a driver-specific lock causing issues occurred before
        fixing do\_swap\_page to lock the faulting page. A driver-exclusive lock
        in migrate\_to\_ram produced a stable livelock if enough threads read
        the faulting page.
      + Partial migration is supported (i.e., a subset of pages attempting to
        migrate can actually migrate, with only the faulting page guaranteed
        to migrate).
      + Driver handles mixed migrations via retry loops rather than locking.
* Eviction
  :   + Eviction is defined as migrating data from the GPU back to the
        CPU without a virtual address to free up GPU memory.
      + Only looking at physical memory data structures and locks as opposed to
        looking at virtual memory data structures and locks.
      + No looking at mm/vma structs or relying on those being locked.
      + The rationale for the above two points is that CPU virtual addresses
        can change at any moment, while the physical pages remain stable.
      + GPU page table invalidation, which requires a GPU virtual address, is
        handled via the notifier that has access to the GPU virtual address.
* GPU fault side
  :   + mmap\_read only used around core MM functions which require this lock
        and should strive to take mmap\_read lock only in GPU SVM layer.
      + Big retry loop to handle all races with the mmu notifier under the gpu
        pagetable locks/mmu notifier range lock/whatever we end up calling
        those.
      + Races (especially against concurrent eviction or migrate\_to\_ram)
        should not be handled on the fault side by trying to hold locks;
        rather, they should be handled using retry loops. One possible
        exception is holding a BO’s dma-resv lock during the initial migration
        to VRAM, as this is a well-defined lock that can be taken underneath
        the mmap\_read lock.
      + One possible issue with the above approach is if a driver has a strict
        migration policy requiring GPU access to occur in GPU memory.
        Concurrent CPU access could cause a livelock due to endless retries.
        While no current user (Xe) of GPU SVM has such a policy, it is likely
        to be added in the future. Ideally, this should be resolved on the
        core-MM side rather than through a driver-side lock.
* Physical memory to virtual backpointer
  :   + This does not work, as no pointers from physical memory to virtual
        memory should exist. `mremap()` is an example of the core MM updating
        the virtual address without notifying the driver of address
        change rather the driver only receiving the invalidation notifier.
      + The physical memory backpointer (page->zone\_device\_data) should remain
        stable from allocation to page free. Safely updating this against a
        concurrent user would be very difficult unless the page is free.
* GPU pagetable locking
  :   + Notifier lock only protects range tree, pages valid state for a range
        (rather than seqno due to wider notifiers), pagetable entries, and
        mmu notifier seqno tracking, it is not a global lock to protect
        against races.
      + All races handled with big retry as mentioned above.

## Overview of baseline design

GPU Shared Virtual Memory (GPU SVM) layer for the Direct Rendering Manager (DRM)
is a component of the DRM framework designed to manage shared virtual memory
between the CPU and GPU. It enables efficient data exchange and processing
for GPU-accelerated applications by allowing memory sharing and
synchronization between the CPU’s and GPU’s virtual address spaces.

Key GPU SVM Components:

* Notifiers:
  :   Used for tracking memory intervals and notifying the GPU of changes,
      notifiers are sized based on a GPU SVM initialization parameter, with a
      recommendation of 512M or larger. They maintain a Red-BlacK tree and a
      list of ranges that fall within the notifier interval. Notifiers are
      tracked within a GPU SVM Red-BlacK tree and list and are dynamically
      inserted or removed as ranges within the interval are created or
      destroyed.
* Ranges:
  :   Represent memory ranges mapped in a DRM device and managed by GPU SVM.
      They are sized based on an array of chunk sizes, which is a GPU SVM
      initialization parameter, and the CPU address space. Upon GPU fault,
      the largest aligned chunk that fits within the faulting CPU address
      space is chosen for the range size. Ranges are expected to be
      dynamically allocated on GPU fault and removed on an MMU notifier UNMAP
      event. As mentioned above, ranges are tracked in a notifier’s Red-Black
      tree.
* Operations:
  :   Define the interface for driver-specific GPU SVM operations such as
      range allocation, notifier allocation, and invalidations.
* Device Memory Allocations:
  :   Embedded structure containing enough information for GPU SVM to migrate
      to / from device memory.
* Device Memory Operations:
  :   Define the interface for driver-specific device memory operations
      release memory, populate pfns, and copy to / from device memory.

This layer provides interfaces for allocating, mapping, migrating, and
releasing memory ranges between the CPU and GPU. It handles all core memory
management interactions (DMA mapping, HMM, and migration) and provides
driver-specific virtual functions (vfuncs). This infrastructure is sufficient
to build the expected driver components for an SVM implementation as detailed
below.

Expected Driver Components:

* GPU page fault handler:
  :   Used to create ranges and notifiers based on the fault address,
      optionally migrate the range to device memory, and create GPU bindings.
* Garbage collector:
  :   Used to unmap and destroy GPU bindings for ranges. Ranges are expected
      to be added to the garbage collector upon a MMU\_NOTIFY\_UNMAP event in
      notifier callback.
* Notifier callback:
  :   Used to invalidate and DMA unmap GPU bindings for ranges.

GPU SVM handles locking for core MM interactions, i.e., it locks/unlocks the
mmap lock as needed.

GPU SVM introduces a global notifier lock, which safeguards the notifier’s
range RB tree and list, as well as the range’s DMA mappings and sequence
number. GPU SVM manages all necessary locking and unlocking operations,
except for the recheck range’s pages being valid
(drm\_gpusvm\_range\_pages\_valid) when the driver is committing GPU bindings.
This lock corresponds to the `driver->update` lock mentioned in
[Heterogeneous Memory Management (HMM)](../../mm/hmm.html). Future revisions may transition from a GPU SVM
global lock to a per-notifier lock if finer-grained locking is deemed
necessary.

In addition to the locking mentioned above, the driver should implement a
lock to safeguard core GPU SVM function calls that modify state, such as
drm\_gpusvm\_range\_find\_or\_insert and drm\_gpusvm\_range\_remove. This lock is
denoted as ‘driver\_svm\_lock’ in code examples. Finer grained driver side
locking should also be possible for concurrent GPU fault processing within a
single GPU SVM. The ‘driver\_svm\_lock’ can be via drm\_gpusvm\_driver\_set\_lock
to add annotations to GPU SVM.

Partial unmapping of ranges (e.g., 1M out of 2M is unmapped by CPU resulting
in MMU\_NOTIFY\_UNMAP event) presents several challenges, with the main one
being that a subset of the range still has CPU and GPU mappings. If the
backing store for the range is in device memory, a subset of the backing
store has references. One option would be to split the range and device
memory backing store, but the implementation for this would be quite
complicated. Given that partial unmappings are rare and driver-defined range
sizes are relatively small, GPU SVM does not support splitting of ranges.

With no support for range splitting, upon partial unmapping of a range, the
driver is expected to invalidate and destroy the entire range. If the range
has device memory as its backing, the driver is also expected to migrate any
remaining pages back to RAM.

This section provides three examples of how to build the expected driver
components: the GPU page fault handler, the garbage collector, and the
notifier callback.

The generic code provided does not include logic for complex migration
policies, optimized invalidations, fined grained driver locking, or other
potentially required driver locking (e.g., DMA-resv locks).

1. GPU page fault handler

```
int driver_bind_range(struct drm_gpusvm *gpusvm, struct drm_gpusvm_range *range)
{
        int err = 0;

        driver_alloc_and_setup_memory_for_bind(gpusvm, range);

        drm_gpusvm_notifier_lock(gpusvm);
        if (drm_gpusvm_range_pages_valid(range))
                driver_commit_bind(gpusvm, range);
        else
                err = -EAGAIN;
        drm_gpusvm_notifier_unlock(gpusvm);

        return err;
}

int driver_gpu_fault(struct drm_gpusvm *gpusvm, unsigned long fault_addr,
                     unsigned long gpuva_start, unsigned long gpuva_end)
{
        struct drm_gpusvm_ctx ctx = {};
        int err;

        driver_svm_lock();
retry:
        // Always process UNMAPs first so view of GPU SVM ranges is current
        driver_garbage_collector(gpusvm);

        range = drm_gpusvm_range_find_or_insert(gpusvm, fault_addr,
                                                gpuva_start, gpuva_end,
                                                &ctx);
        if (IS_ERR(range)) {
                err = PTR_ERR(range);
                goto unlock;
        }

        if (driver_migration_policy(range)) {
                err = drm_pagemap_populate_mm(driver_choose_drm_pagemap(),
                                              gpuva_start, gpuva_end, gpusvm->mm,
                                              ctx->timeslice_ms);
                if (err)        // CPU mappings may have changed
                        goto retry;
        }

        err = drm_gpusvm_range_get_pages(gpusvm, range, &ctx);
        if (err == -EOPNOTSUPP || err == -EFAULT || err == -EPERM) {    // CPU mappings changed
                if (err == -EOPNOTSUPP)
                        drm_gpusvm_range_evict(gpusvm, range);
                goto retry;
        } else if (err) {
                goto unlock;
        }

        err = driver_bind_range(gpusvm, range);
        if (err == -EAGAIN)     // CPU mappings changed
                goto retry

unlock:
        driver_svm_unlock();
        return err;
}
```

2. Garbage Collector

```
void __driver_garbage_collector(struct drm_gpusvm *gpusvm,
                                struct drm_gpusvm_range *range)
{
        assert_driver_svm_locked(gpusvm);

        // Partial unmap, migrate any remaining device memory pages back to RAM
        if (range->flags.partial_unmap)
                drm_gpusvm_range_evict(gpusvm, range);

        driver_unbind_range(range);
        drm_gpusvm_range_remove(gpusvm, range);
}

void driver_garbage_collector(struct drm_gpusvm *gpusvm)
{
        assert_driver_svm_locked(gpusvm);

        for_each_range_in_garbage_collector(gpusvm, range)
                __driver_garbage_collector(gpusvm, range);
}
```

3. Notifier callback

```
void driver_invalidation(struct drm_gpusvm *gpusvm,
                         struct drm_gpusvm_notifier *notifier,
                         const struct mmu_notifier_range *mmu_range)
{
        struct drm_gpusvm_ctx ctx = { .in_notifier = true, };
        struct drm_gpusvm_range *range = NULL;

        driver_invalidate_device_pages(gpusvm, mmu_range->start, mmu_range->end);

        drm_gpusvm_for_each_range(range, notifier, mmu_range->start,
                                  mmu_range->end) {
                drm_gpusvm_range_unmap_pages(gpusvm, range, &ctx);

                if (mmu_range->event != MMU_NOTIFY_UNMAP)
                        continue;

                drm_gpusvm_range_set_unmapped(range, mmu_range);
                driver_garbage_collector_add(gpusvm, range);
        }
}
```

## Overview of drm\_pagemap design

The DRM pagemap layer is intended to augment the dev\_pagemap functionality by
providing a way to populate a `struct mm_struct` virtual range with device
private pages and to provide helpers to abstract device memory allocations,
to migrate memory back and forth between device memory and system RAM and
to handle access (and in the future migration) between devices implementing
a fast interconnect that is not necessarily visible to the rest of the
system.

Typically the DRM pagemap receives requests from one or more DRM GPU SVM
instances to populate `struct mm_struct` virtual ranges with memory, and the
migration is best effort only and may thus fail. The implementation should
also handle device unbinding by blocking (return an -ENODEV) error for new
population requests and after that migrate all device pages to system ram.

Migration granularity typically follows the GPU SVM range requests, but
if there are clashes, due to races or due to the fact that multiple GPU
SVM instances have different views of the ranges used, and because of that
parts of a requested range is already present in the requested device memory,
the implementation has a variety of options. It can fail and it can choose
to populate only the part of the range that isn’t already in device memory,
and it can evict the range to system before trying to migrate. Ideally an
implementation would just try to migrate the missing part of the range and
allocate just enough memory to do so.

When migrating to system memory as a response to a cpu fault or a device
memory eviction request, currently a full device memory allocation is
migrated back to system. Moving forward this might need improvement for
situations where a single page needs bouncing between system memory and
device memory due to, for example, atomic operations.

Key DRM pagemap components:

* Device Memory Allocations:
  :   Embedded structure containing enough information for the drm\_pagemap to
      migrate to / from device memory.
* Device Memory Operations:
  :   Define the interface for driver-specific device memory operations
      release memory, populate pfns, and copy to / from device memory.

## Possible future design features

* Concurrent GPU faults
  :   + CPU faults are concurrent so makes sense to have concurrent GPU
        faults.
      + Should be possible with fined grained locking in the driver GPU
        fault handler.
      + No expected GPU SVM changes required.
* Ranges with mixed system and device pages
  :   + Can be added if required to drm\_gpusvm\_get\_pages fairly easily.
* Multi-GPU support
  :   + Work in progress and patches expected after initially landing on GPU
        SVM.
      + Ideally can be done with little to no changes to GPU SVM.
* Drop ranges in favor of radix tree
  :   + May be desirable for faster notifiers.
* Compound device pages
  :   + Nvidia, AMD, and Intel all have agreed expensive core MM functions in
        migrate device layer are a performance bottleneck, having compound
        device pages should help increase performance by reducing the number
        of these expensive calls.
* Higher order dma mapping for migration
  :   + 4k dma mapping adversely affects migration performance on Intel
        hardware, higher order (2M) dma mapping should help here.
* Build common userptr implementation on top of GPU SVM
* Driver side madvise implementation and migration policies
* Pull in pending dma-mapping API changes from Leon / Nvidia when these land
