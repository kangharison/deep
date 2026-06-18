# DAX Driver Operation

> 출처(원문): https://docs.kernel.org/driver-api/cxl/linux/dax-driver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DAX Driver Operation

The Direct Access Device driver was originally designed to provide a
memory-like access mechanism to memory-like block-devices. It was
extended to support CXL Memory Devices, which provide user-configured
memory devices.

The CXL subsystem depends on the DAX subsystem to either:

* Generate a file-like interface to userland via `/dev/daxN.Y`, or
* Engage the memory-hotplug interface to add CXL memory to page allocator.

The DAX subsystem exposes this ability through the cxl\_dax\_region driver.
A dax\_region provides the translation between a CXL memory\_region and
a DAX Device.

## DAX Device

A DAX Device is a file-like interface exposed in `/dev/daxN.Y`. A
memory region exposed via dax device can be accessed via userland software
via the `mmap()` system-call. The result is direct mappings to the
CXL capacity in the task’s page tables.

Users wishing to manually handle allocation of CXL memory should use this
interface.

## kmem conversion

The `dax_kmem` driver converts a DAX Device into a series of hotplug
memory blocks managed by `kernel/memory-hotplug.c`. This capacity
will be exposed to the kernel page allocator in the user-selected memory
zone.

The `memmap_on_memory` setting (both global and DAX device local)
dictates where the kernell will allocate the `struct folio` descriptors
for this memory will come from. If `memmap_on_memory` is set, memory
hotplug will set aside a portion of the memory block capacity to allocate
folios. If unset, the memory is allocated via a normal `GFP_KERNEL`
allocation - and as a result will most likely land on the local NUM node of the
CPU executing the hotplug operation.
