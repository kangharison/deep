# Core API Documentation

> 출처(원문): https://docs.kernel.org/core-api/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Core API Documentation

This is the beginning of a manual for core kernel APIs. The conversion
(and writing!) of documents for this manual is much appreciated!

## Core utilities

This section has general and “core core” documentation. The first is a
massive grab-bag of kerneldoc info left over from the docbook days; it
should really be broken up someday when somebody finds the energy to do
it.

* [The Linux Kernel API](kernel-api.html)
* [Workqueue](workqueue.html)
* [General notification mechanism](watch_queue.html)
* [Message logging with printk](printk-basics.html)
* [How to get printk format specifiers right](printk-formats.html)
* [Printk Index](printk-index.html)
* [Symbol Namespaces](symbol-namespaces.html)
* [Assembler Annotations](asm-annotations.html)
* [Real-time preemption](real-time/index.html)
* [Housekeeping](housekeeping.html)

## Data structures and low-level utilities

Library functionality that is used throughout the kernel.

* [Everything you never wanted to know about kobjects, ksets, and ktypes](kobject.html)
* [Adding reference counters (krefs) to kernel objects](kref.html)
* [Scope-based Cleanup Helpers](cleanup.html)
* [Generic Associative Array Implementation](assoc_array.html)
* [Folio Queue](folio_queue.html)
* [XArray](xarray.html)
* [Maple Tree](maple_tree.html)
* [ID Allocation](idr.html)
* [Circular Buffers](circular-buffers.html)
* [Red-black Trees (rbtree) in Linux](rbtree.html)
* [Generic radix trees/sparse arrays](generic-radix-tree.html)
* [Generic bitfield packing and unpacking functions](packing.html)
* [this\_cpu operations](this_cpu_ops.html)
* [ktime accessors](timekeeping.html)
* [The errseq\_t datatype](errseq.html)
* [Atomic types](wrappers/atomic_t.html)
* [Atomic bitops](wrappers/atomic_bitops.html)
* [Floating-point API](floating-point.html)
* [Union-Find in Linux](union_find.html)
* [Min Heap API](min_heap.html)
* [Generic parser](parser.html)
* [Linked Lists in Linux](list.html)

## Low level entry and exit

* [Entry/exit handling for exceptions, interrupts, syscalls and KVM](entry.html)

## Concurrency primitives

How Linux keeps everything from happening at the same time. See
[Locking](../locking/index.html) for more related documentation.

* [refcount\_t API compared to atomic\_t](refcount-vs-atomic.html)
* [IRQs](irq/index.html)
* [Semantics and Behavior of Local Atomic Operations](local_ops.html)
* [The padata parallel execution mechanism](padata.html)
* [RCU Handbook](../RCU/index.html)
* [Linux kernel memory barriers](wrappers/memory-barriers.html)

## Low-level hardware management

Cache management, managing CPU hotplug, etc.

* [Cache and TLB Flushing Under Linux](cachetlb.html)
* [CPU hotplug in the Kernel](cpu_hotplug.html)
* [Memory hotplug](memory-hotplug.html)
* [Linux generic IRQ handling](genericirq.html)
* [Memory Protection Keys](protection-keys.html)

## Memory management

How to allocate and use memory in the kernel. Note that there is a lot
more memory-management documentation in [Memory Management Documentation](../mm/index.html).

* [Memory Allocation Guide](memory-allocation.html)
* [Unaligned Memory Accesses](unaligned-memory-access.html)
* [Dynamic DMA mapping using the generic device](dma-api.html)
* [Dynamic DMA mapping Guide](dma-api-howto.html)
* [DMA attributes](dma-attributes.html)
* [DMA with ISA and LPC devices](dma-isa-lpc.html)
* [DMA and swiotlb](swiotlb.html)
* [Memory Management APIs](mm-api.html)
* [Cgroup Kernel APIs](cgroup.html)
* [The genalloc/genpool subsystem](genalloc.html)
* [pin\_user\_pages() and related calls](pin_user_pages.html)
* [Boot time memory management](boot-time-mm.html)
* [GFP masks used from FS/IO context](gfp_mask-from-fs-io.html)
* [Kexec Handover Subsystem](kho/index.html)

## Interfaces for kernel debugging

* [The object-lifetime debugging infrastructure](debug-objects.html)
* [The Linux Kernel Tracepoint API](tracepoint.html)
* [Using physical DMA provided by OHCI-1394 FireWire controllers for debugging](debugging-via-ohci1394.html)

## Everything else

Documents that don’t fit elsewhere or which have yet to be categorized.

* [Reed-Solomon Library Programming Interface](librs.html)
* [Live Update Orchestrator](liveupdate.html)
* [Netlink notes for kernel developers](netlink.html)
