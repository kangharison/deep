# Boot time memory management

> 출처(원문): https://docs.kernel.org/core-api/boot-time-mm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Boot time memory management

Early system initialization cannot use “normal” memory management
simply because it is not set up yet. But there is still need to
allocate memory for various data structures, for instance for the
physical page allocator.

A specialized allocator called `memblock` performs the
boot time memory management. The architecture specific initialization
must set it up in `setup_arch()` and tear it down in
`mem_init()` functions.

Once the early memory management is available it offers a variety of
functions and macros for memory allocations. The allocation request
may be directed to the first (and probably the only) node or to a
particular node in a NUMA system. There are API variants that panic
when an allocation fails and those that don’t.

Memblock also offers a variety of APIs that control its own behaviour.

## Memblock Overview

Memblock is a method of managing memory regions during the early
boot period when the usual kernel memory allocators are not up and
running.

Memblock views the system memory as collections of contiguous
regions. There are several types of these collections:

* `memory` - describes the physical memory available to the
  kernel; this may differ from the actual physical memory installed
  in the system, for instance when the memory is restricted with
  `mem=` command line parameter
* `reserved` - describes the regions that were allocated
* `physmem` - describes the actual physical memory available during
  boot regardless of the possible restrictions and memory hot(un)plug;
  the `physmem` type is only available on some architectures.

Each region is represented by [`struct memblock_region`](#c.memblock_region "memblock_region") that
defines the region extents, its attributes and NUMA node id on NUMA
systems. Every memory type is described by the [`struct memblock_type`](#c.memblock_type "memblock_type")
which contains an array of memory regions along with
the allocator metadata. The “memory” and “reserved” types are nicely
wrapped with [`struct memblock`](#c.memblock "memblock"). This structure is statically
initialized at build time. The region arrays are initially sized to
`INIT_MEMBLOCK_MEMORY_REGIONS` for “memory” and
`INIT_MEMBLOCK_RESERVED_REGIONS` for “reserved”. The region array
for “physmem” is initially sized to `INIT_PHYSMEM_REGIONS`.
The `memblock_allow_resize()` enables automatic resizing of the region
arrays during addition of new regions. This feature should be used
with care so that memory allocated for the region array will not
overlap with areas that should be reserved, for example initrd.

The early architecture setup should tell memblock what the physical
memory layout is by using [`memblock_add()`](#c.memblock_add "memblock_add") or [`memblock_add_node()`](#c.memblock_add_node "memblock_add_node")
functions. The first function does not assign the region to a NUMA
node and it is appropriate for UMA systems. Yet, it is possible to
use it on NUMA systems as well and assign the region to a NUMA node
later in the setup process using [`memblock_set_node()`](#c.memblock_set_node "memblock_set_node"). The
[`memblock_add_node()`](#c.memblock_add_node "memblock_add_node") performs such an assignment directly.

Once memblock is setup the memory can be allocated using one of the
API variants:

* memblock\_phys\_alloc\*() - these functions return the **physical**
  address of the allocated memory
* memblock\_alloc\*() - these functions return the **virtual** address
  of the allocated memory.

Note, that both API variants use implicit assumptions about allowed
memory ranges and the fallback methods. Consult the documentation
of [`memblock_alloc_internal()`](#c.memblock_alloc_internal "memblock_alloc_internal") and [`memblock_alloc_range_nid()`](#c.memblock_alloc_range_nid "memblock_alloc_range_nid")
functions for more elaborate description.

As the system boot progresses, the architecture specific `mem_init()`
function frees all the memory to the buddy page allocator.

Unless an architecture enables `CONFIG_ARCH_KEEP_MEMBLOCK`, the
memblock data structures (except “physmem”) will be discarded after the
system initialization completes.

## Functions and structures

Here is the description of memblock data structures, functions and
macros. Some of them are actually internal, but since they are
documented it would be silly to omit them. Besides, reading the
descriptions for the internal functions can help to understand what
really happens under the hood.

enum memblock\_flags
:   definition of memory region attributes

**Constants**

`MEMBLOCK_NONE`
:   no special request

`MEMBLOCK_HOTPLUG`
:   memory region indicated in the firmware-provided memory
    map during early boot as hot(un)pluggable system RAM (e.g., memory range
    that might get hotunplugged later). With “movable\_node” set on the kernel
    commandline, try keeping this memory region hotunpluggable. Does not apply
    to memblocks added (“hotplugged”) after early boot.

`MEMBLOCK_MIRROR`
:   mirrored region

`MEMBLOCK_NOMAP`
:   don’t add to kernel direct mapping and treat as
    reserved in the memory map; refer to [`memblock_mark_nomap()`](#c.memblock_mark_nomap "memblock_mark_nomap") description
    for further details

`MEMBLOCK_DRIVER_MANAGED`
:   memory region that is always detected and added
    via a driver, and never indicated in the firmware-provided memory map as
    system RAM. This corresponds to IORESOURCE\_SYSRAM\_DRIVER\_MANAGED in the
    kernel resource tree.

`MEMBLOCK_RSRV_NOINIT`
:   reserved memory region for which `struct pages` are not
    fully initialized. Users of this flag are responsible to properly initialize
    `struct pages` of this region

`MEMBLOCK_RSRV_KERN`
:   memory region that is reserved for kernel use,
    either explictitly with `memblock_reserve_kern()` or via memblock
    allocation APIs. All memblock allocations set this flag.

`MEMBLOCK_KHO_SCRATCH`
:   memory region that kexec can pass to the next
    kernel in handover mode. During early boot, we do not know about all
    memory reservations yet, so we get scratch memory from the previous
    kernel that we know is good to use. It is the only memory that
    allocations may happen from in this phase.

struct memblock\_region
:   represents a memory region

**Definition**:

```
struct memblock_region {
    phys_addr_t base;
    phys_addr_t size;
    enum memblock_flags flags;
#ifdef CONFIG_NUMA;
    int nid;
#endif;
};
```

**Members**

`base`
:   base address of the region

`size`
:   size of the region

`flags`
:   memory region attributes

`nid`
:   NUMA node id

struct memblock\_type
:   collection of memory regions of certain type

**Definition**:

```
struct memblock_type {
    unsigned long cnt;
    unsigned long max;
    phys_addr_t total_size;
    struct memblock_region *regions;
    char *name;
};
```

**Members**

`cnt`
:   number of regions

`max`
:   size of the allocated array

`total_size`
:   size of all regions

`regions`
:   array of regions

`name`
:   the memory type symbolic name

struct memblock
:   memblock allocator metadata

**Definition**:

```
struct memblock {
    bool bottom_up;
    phys_addr_t current_limit;
    struct memblock_type memory;
    struct memblock_type reserved;
};
```

**Members**

`bottom_up`
:   is bottom up direction?

`current_limit`
:   physical address of the current allocation limit

`memory`
:   usable memory regions

`reserved`
:   reserved memory regions

for\_each\_physmem\_range

`for_each_physmem_range (i, type, p_start, p_end)`

> iterate through physmem areas not included in type.

**Parameters**

`i`
:   u64 used as loop variable

`type`
:   ptr to memblock\_type which excludes from the iteration, can be `NULL`

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

\_\_for\_each\_mem\_range

`__for_each_mem_range (i, type_a, type_b, nid, flags, p_start, p_end, p_nid)`

> iterate through memblock areas from type\_a and not included in type\_b. Or just type\_a if type\_b is NULL.

**Parameters**

`i`
:   u64 used as loop variable

`type_a`
:   ptr to memblock\_type to iterate

`type_b`
:   ptr to memblock\_type which excludes from the iteration

`nid`
:   node selector, `NUMA_NO_NODE` for all nodes

`flags`
:   pick from blocks based on memory attributes

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

`p_nid`
:   ptr to int for nid of the range, can be `NULL`

\_\_for\_each\_mem\_range\_rev

`__for_each_mem_range_rev (i, type_a, type_b, nid, flags, p_start, p_end, p_nid)`

> reverse iterate through memblock areas from type\_a and not included in type\_b. Or just type\_a if type\_b is NULL.

**Parameters**

`i`
:   u64 used as loop variable

`type_a`
:   ptr to memblock\_type to iterate

`type_b`
:   ptr to memblock\_type which excludes from the iteration

`nid`
:   node selector, `NUMA_NO_NODE` for all nodes

`flags`
:   pick from blocks based on memory attributes

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

`p_nid`
:   ptr to int for nid of the range, can be `NULL`

for\_each\_mem\_range

`for_each_mem_range (i, p_start, p_end)`

> iterate through memory areas.

**Parameters**

`i`
:   u64 used as loop variable

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

for\_each\_mem\_range\_rev

`for_each_mem_range_rev (i, p_start, p_end)`

> reverse iterate through memblock areas from type\_a and not included in type\_b. Or just type\_a if type\_b is NULL.

**Parameters**

`i`
:   u64 used as loop variable

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

for\_each\_reserved\_mem\_range

`for_each_reserved_mem_range (i, p_start, p_end)`

> iterate over all reserved memblock areas

**Parameters**

`i`
:   u64 used as loop variable

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

**Description**

Walks over reserved areas of memblock. Available as soon as memblock
is initialized.

for\_each\_mem\_pfn\_range

`for_each_mem_pfn_range (i, nid, p_start, p_end, p_nid)`

> early memory pfn range iterator

**Parameters**

`i`
:   an integer used as loop variable

`nid`
:   node selector, `MAX_NUMNODES` for all nodes

`p_start`
:   ptr to ulong for start pfn of the range, can be `NULL`

`p_end`
:   ptr to ulong for end pfn of the range, can be `NULL`

`p_nid`
:   ptr to int for nid of the range, can be `NULL`

**Description**

Walks over configured memory ranges.

for\_each\_free\_mem\_range

`for_each_free_mem_range (i, nid, flags, p_start, p_end, p_nid)`

> iterate through free memblock areas

**Parameters**

`i`
:   u64 used as loop variable

`nid`
:   node selector, `NUMA_NO_NODE` for all nodes

`flags`
:   pick from blocks based on memory attributes

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

`p_nid`
:   ptr to int for nid of the range, can be `NULL`

**Description**

Walks over free (memory && !reserved) areas of memblock. Available as
soon as memblock is initialized.

for\_each\_free\_mem\_range\_reverse

`for_each_free_mem_range_reverse (i, nid, flags, p_start, p_end, p_nid)`

> rev-iterate through free memblock areas

**Parameters**

`i`
:   u64 used as loop variable

`nid`
:   node selector, `NUMA_NO_NODE` for all nodes

`flags`
:   pick from blocks based on memory attributes

`p_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`p_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

`p_nid`
:   ptr to int for nid of the range, can be `NULL`

**Description**

Walks over free (memory && !reserved) areas of memblock in reverse
order. Available as soon as memblock is initialized.

void memblock\_set\_current\_limit(phys\_addr\_t limit)
:   Set the current allocation limit to allow limiting allocations to what is currently accessible during boot

**Parameters**

`phys_addr_t limit`
:   New limit value (physical address)

unsigned long memblock\_region\_memory\_base\_pfn(const struct [memblock\_region](#c.memblock_region "memblock_region") \*reg)
:   get the lowest pfn of the memory region

**Parameters**

`const struct memblock_region *reg`
:   memblock\_region structure

**Return**

the lowest pfn intersecting with the memory region

unsigned long memblock\_region\_memory\_end\_pfn(const struct [memblock\_region](#c.memblock_region "memblock_region") \*reg)
:   get the end pfn of the memory region

**Parameters**

`const struct memblock_region *reg`
:   memblock\_region structure

**Return**

the end\_pfn of the reserved region

unsigned long memblock\_region\_reserved\_base\_pfn(const struct [memblock\_region](#c.memblock_region "memblock_region") \*reg)
:   get the lowest pfn of the reserved region

**Parameters**

`const struct memblock_region *reg`
:   memblock\_region structure

**Return**

the lowest pfn intersecting with the reserved region

unsigned long memblock\_region\_reserved\_end\_pfn(const struct [memblock\_region](#c.memblock_region "memblock_region") \*reg)
:   get the end pfn of the reserved region

**Parameters**

`const struct memblock_region *reg`
:   memblock\_region structure

**Return**

the end\_pfn of the reserved region

for\_each\_mem\_region

`for_each_mem_region (region)`

> iterate over memory regions

**Parameters**

`region`
:   loop variable

for\_each\_reserved\_mem\_region

`for_each_reserved_mem_region (region)`

> itereate over reserved memory regions

**Parameters**

`region`
:   loop variable

phys\_addr\_t \_\_init\_memblock \_\_memblock\_find\_range\_bottom\_up(phys\_addr\_t start, phys\_addr\_t end, phys\_addr\_t size, phys\_addr\_t align, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags)
:   find free area utility in bottom-up

**Parameters**

`phys_addr_t start`
:   start of candidate range

`phys_addr_t end`
:   end of candidate range, can be `MEMBLOCK_ALLOC_ANYWHERE` or
    `MEMBLOCK_ALLOC_ACCESSIBLE`

`phys_addr_t size`
:   size of free area to find

`phys_addr_t align`
:   alignment of free area to find

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

`enum memblock_flags flags`
:   pick from blocks based on memory attributes

**Description**

Utility called from [`memblock_find_in_range_node()`](#c.memblock_find_in_range_node "memblock_find_in_range_node"), find free area bottom-up.

**Return**

Found address on success, 0 on failure.

phys\_addr\_t \_\_init\_memblock \_\_memblock\_find\_range\_top\_down(phys\_addr\_t start, phys\_addr\_t end, phys\_addr\_t size, phys\_addr\_t align, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags)
:   find free area utility, in top-down

**Parameters**

`phys_addr_t start`
:   start of candidate range

`phys_addr_t end`
:   end of candidate range, can be `MEMBLOCK_ALLOC_ANYWHERE` or
    `MEMBLOCK_ALLOC_ACCESSIBLE`

`phys_addr_t size`
:   size of free area to find

`phys_addr_t align`
:   alignment of free area to find

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

`enum memblock_flags flags`
:   pick from blocks based on memory attributes

**Description**

Utility called from [`memblock_find_in_range_node()`](#c.memblock_find_in_range_node "memblock_find_in_range_node"), find free area top-down.

**Return**

Found address on success, 0 on failure.

phys\_addr\_t \_\_init\_memblock memblock\_find\_in\_range\_node(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t start, phys\_addr\_t end, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags)
:   find free area in given range and node

**Parameters**

`phys_addr_t size`
:   size of free area to find

`phys_addr_t align`
:   alignment of free area to find

`phys_addr_t start`
:   start of candidate range

`phys_addr_t end`
:   end of candidate range, can be `MEMBLOCK_ALLOC_ANYWHERE` or
    `MEMBLOCK_ALLOC_ACCESSIBLE`

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

`enum memblock_flags flags`
:   pick from blocks based on memory attributes

**Description**

Find **size** free area aligned to **align** in the specified range and node.

**Return**

Found address on success, 0 on failure.

phys\_addr\_t \_\_init\_memblock memblock\_find\_in\_range(phys\_addr\_t start, phys\_addr\_t end, phys\_addr\_t size, phys\_addr\_t align)
:   find free area in given range

**Parameters**

`phys_addr_t start`
:   start of candidate range

`phys_addr_t end`
:   end of candidate range, can be `MEMBLOCK_ALLOC_ANYWHERE` or
    `MEMBLOCK_ALLOC_ACCESSIBLE`

`phys_addr_t size`
:   size of free area to find

`phys_addr_t align`
:   alignment of free area to find

**Description**

Find **size** free area aligned to **align** in the specified range.

**Return**

Found address on success, 0 on failure.

void memblock\_discard(void)
:   discard memory and reserved arrays if they were allocated

**Parameters**

`void`
:   no arguments

int \_\_init\_memblock memblock\_double\_array(struct [memblock\_type](#c.memblock_type "memblock_type") \*type, phys\_addr\_t new\_area\_start, phys\_addr\_t new\_area\_size)
:   double the size of the memblock regions array

**Parameters**

`struct memblock_type *type`
:   memblock type of the regions array being doubled

`phys_addr_t new_area_start`
:   starting address of memory range to avoid overlap with

`phys_addr_t new_area_size`
:   size of memory range to avoid overlap with

**Description**

Double the size of the **type** regions array. If memblock is being used to
allocate memory for a new reserved regions array and there is a previously
allocated memory range [**new\_area\_start**, **new\_area\_start** + **new\_area\_size**]
waiting to be reserved, ensure the memory used by the new array does
not overlap.

**Return**

0 on success, -1 on failure.

void \_\_init\_memblock memblock\_merge\_regions(struct [memblock\_type](#c.memblock_type "memblock_type") \*type, unsigned long start\_rgn, unsigned long end\_rgn)
:   merge neighboring compatible regions

**Parameters**

`struct memblock_type *type`
:   memblock type to scan

`unsigned long start_rgn`
:   start scanning from (**start\_rgn** - 1)

`unsigned long end_rgn`
:   end scanning at (**end\_rgn** - 1)
    Scan **type** and merge neighboring compatible regions in [**start\_rgn** - 1, **end\_rgn**)

void \_\_init\_memblock memblock\_insert\_region(struct [memblock\_type](#c.memblock_type "memblock_type") \*type, int idx, phys\_addr\_t base, phys\_addr\_t size, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags)
:   insert new memblock region

**Parameters**

`struct memblock_type *type`
:   memblock type to insert into

`int idx`
:   index for the insertion point

`phys_addr_t base`
:   base address of the new region

`phys_addr_t size`
:   size of the new region

`int nid`
:   node id of the new region

`enum memblock_flags flags`
:   flags of the new region

**Description**

Insert new memblock region [**base**, **base** + **size**) into **type** at **idx**.
**type** must already have extra room to accommodate the new region.

int \_\_init\_memblock memblock\_add\_range(struct [memblock\_type](#c.memblock_type "memblock_type") \*type, phys\_addr\_t base, phys\_addr\_t size, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags)
:   add new memblock region

**Parameters**

`struct memblock_type *type`
:   memblock type to add new region into

`phys_addr_t base`
:   base address of the new region

`phys_addr_t size`
:   size of the new region

`int nid`
:   nid of the new region

`enum memblock_flags flags`
:   flags of the new region

**Description**

Add new memblock region [**base**, **base** + **size**) into **type**. The new region
is allowed to overlap with existing ones - overlaps don’t affect already
existing regions. **type** is guaranteed to be minimal (all neighbouring
compatible regions are merged) after the addition.

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_add\_node(phys\_addr\_t base, phys\_addr\_t size, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags)
:   add new memblock region within a NUMA node

**Parameters**

`phys_addr_t base`
:   base address of the new region

`phys_addr_t size`
:   size of the new region

`int nid`
:   nid of the new region

`enum memblock_flags flags`
:   flags of the new region

**Description**

Add new memblock region [**base**, **base** + **size**) to the “memory”
type. See [`memblock_add_range()`](#c.memblock_add_range "memblock_add_range") description for mode details

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_add(phys\_addr\_t base, phys\_addr\_t size)
:   add new memblock region

**Parameters**

`phys_addr_t base`
:   base address of the new region

`phys_addr_t size`
:   size of the new region

**Description**

Add new memblock region [**base**, **base** + **size**) to the “memory”
type. See [`memblock_add_range()`](#c.memblock_add_range "memblock_add_range") description for mode details

**Return**

0 on success, -errno on failure.

bool \_\_init\_memblock memblock\_validate\_numa\_coverage(unsigned long threshold\_bytes)
:   check if amount of memory with no node ID assigned is less than a threshold

**Parameters**

`unsigned long threshold_bytes`
:   maximal memory size that can have unassigned node
    ID (in bytes).

**Description**

A buggy firmware may report memory that does not belong to any node.
Check if amount of such memory is below **threshold\_bytes**.

**Return**

true on success, false on failure.

int \_\_init\_memblock memblock\_isolate\_range(struct [memblock\_type](#c.memblock_type "memblock_type") \*type, phys\_addr\_t base, phys\_addr\_t size, int \*start\_rgn, int \*end\_rgn)
:   isolate given range into disjoint memblocks

**Parameters**

`struct memblock_type *type`
:   memblock type to isolate range for

`phys_addr_t base`
:   base of range to isolate

`phys_addr_t size`
:   size of range to isolate

`int *start_rgn`
:   out parameter for the start of isolated region

`int *end_rgn`
:   out parameter for the end of isolated region

**Description**

Walk **type** and ensure that regions don’t cross the boundaries defined by
[**base**, **base** + **size**). Crossing regions are split at the boundaries,
which may create at most two more regions. The index of the first
region inside the range is returned in **\*start\_rgn** and the index of the
first region after the range is returned in **\*end\_rgn**.

**Return**

0 on success, -errno on failure.

void \_\_init\_memblock memblock\_free(void \*ptr, size\_t size)
:   free boot memory allocation

**Parameters**

`void *ptr`
:   starting address of the boot memory allocation

`size_t size`
:   size of the boot memory block in bytes

**Description**

Free boot memory block previously allocated by `memblock_alloc_xx()` API.
If called after the buddy allocator is available, the memory is released to
the buddy allocator.

int \_\_init\_memblock memblock\_phys\_free(phys\_addr\_t base, phys\_addr\_t size)
:   free boot memory block

**Parameters**

`phys_addr_t base`
:   phys starting address of the boot memory block

`phys_addr_t size`
:   size of the boot memory block in bytes

**Description**

Free boot memory block previously allocated by `memblock_phys_alloc_xx()` API.
If called after the buddy allocator is available, the memory is released to
the buddy allocator.

int \_\_init\_memblock memblock\_setclr\_flag(struct [memblock\_type](#c.memblock_type "memblock_type") \*type, phys\_addr\_t base, phys\_addr\_t size, int set, int flag)
:   set or clear flag for a memory region

**Parameters**

`struct memblock_type *type`
:   memblock type to set/clear flag for

`phys_addr_t base`
:   base address of the region

`phys_addr_t size`
:   size of the region

`int set`
:   set or clear the flag

`int flag`
:   the flag to update

**Description**

This function isolates region [**base**, **base** + **size**), and sets/clears flag

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_mark\_hotplug(phys\_addr\_t base, phys\_addr\_t size)
:   Mark hotpluggable memory with flag MEMBLOCK\_HOTPLUG.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_clear\_hotplug(phys\_addr\_t base, phys\_addr\_t size)
:   Clear flag MEMBLOCK\_HOTPLUG for a specified region.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_mark\_mirror(phys\_addr\_t base, phys\_addr\_t size)
:   Mark mirrored memory with flag MEMBLOCK\_MIRROR.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_mark\_nomap(phys\_addr\_t base, phys\_addr\_t size)
:   Mark a memory region with flag MEMBLOCK\_NOMAP.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Description**

The memory regions marked with `MEMBLOCK_NOMAP` will not be added to the
direct mapping of the physical memory. These regions will still be
covered by the memory map. The `struct page` representing NOMAP memory
frames in the memory map will be `PageReserved()`

**Note**

if the memory being marked `MEMBLOCK_NOMAP` was allocated from
memblock, the caller must inform kmemleak to ignore that memory

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_clear\_nomap(phys\_addr\_t base, phys\_addr\_t size)
:   Clear flag MEMBLOCK\_NOMAP for a specified region.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_reserved\_mark\_noinit(phys\_addr\_t base, phys\_addr\_t size)
:   Mark a reserved memory region with flag MEMBLOCK\_RSRV\_NOINIT

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Description**

The `struct pages` for the reserved regions marked `MEMBLOCK_RSRV_NOINIT` will
not be fully initialized to allow the caller optimize their initialization.

When `CONFIG_DEFERRED_STRUCT_PAGE_INIT` is enabled, setting this flag
completely bypasses the initialization of `struct pages` for such region.

When `CONFIG_DEFERRED_STRUCT_PAGE_INIT` is disabled, `struct pages` in this
region will be initialized with default values but won’t be marked as
reserved.

**Return**

0 on success, -errno on failure.

int \_\_init\_memblock memblock\_reserved\_mark\_kern(phys\_addr\_t base, phys\_addr\_t size)
:   Mark a reserved memory region with flag MEMBLOCK\_RSRV\_KERN

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Return**

0 on success, -errno on failure.

int memblock\_mark\_kho\_scratch(phys\_addr\_t base, phys\_addr\_t size)
:   Mark a memory region as MEMBLOCK\_KHO\_SCRATCH.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Description**

Only memory regions marked with `MEMBLOCK_KHO_SCRATCH` will be considered
for allocations during early boot with kexec handover.

**Return**

0 on success, -errno on failure.

int memblock\_clear\_kho\_scratch(phys\_addr\_t base, phys\_addr\_t size)
:   Clear MEMBLOCK\_KHO\_SCRATCH flag for a specified region.

**Parameters**

`phys_addr_t base`
:   the base phys addr of the region

`phys_addr_t size`
:   the size of the region

**Return**

0 on success, -errno on failure.

void \_\_next\_mem\_range(u64 \*idx, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags, struct [memblock\_type](#c.memblock_type "memblock_type") \*type\_a, struct [memblock\_type](#c.memblock_type "memblock_type") \*type\_b, phys\_addr\_t \*out\_start, phys\_addr\_t \*out\_end, int \*out\_nid)
:   next function for [`for_each_free_mem_range()`](#c.for_each_free_mem_range "for_each_free_mem_range") etc.

**Parameters**

`u64 *idx`
:   pointer to u64 loop variable

`int nid`
:   node selector, `NUMA_NO_NODE` for all nodes

`enum memblock_flags flags`
:   pick from blocks based on memory attributes

`struct memblock_type *type_a`
:   pointer to memblock\_type from where the range is taken

`struct memblock_type *type_b`
:   pointer to memblock\_type which excludes memory from being taken

`phys_addr_t *out_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`phys_addr_t *out_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

`int *out_nid`
:   ptr to int for nid of the range, can be `NULL`

**Description**

Find the first area from **\*idx** which matches **nid**, fill the out
parameters, and update **\*idx** for the next iteration. The lower 32bit of
**\*idx** contains index into type\_a and the upper 32bit indexes the
areas before each region in type\_b. For example, if type\_b regions
look like the following,

> 0:[0-16), 1:[32-48), 2:[128-130)

The upper 32bit indexes the following regions.

> 0:[0-0), 1:[16-32), 2:[48-128), 3:[130-MAX)

As both region arrays are sorted, the function advances the two indices
in lockstep and returns each intersection.

void \_\_init\_memblock \_\_next\_mem\_range\_rev(u64 \*idx, int nid, enum [memblock\_flags](#c.memblock_flags "memblock_flags") flags, struct [memblock\_type](#c.memblock_type "memblock_type") \*type\_a, struct [memblock\_type](#c.memblock_type "memblock_type") \*type\_b, phys\_addr\_t \*out\_start, phys\_addr\_t \*out\_end, int \*out\_nid)
:   generic next function for for\_each\_\*`_range_rev()`

**Parameters**

`u64 *idx`
:   pointer to u64 loop variable

`int nid`
:   node selector, `NUMA_NO_NODE` for all nodes

`enum memblock_flags flags`
:   pick from blocks based on memory attributes

`struct memblock_type *type_a`
:   pointer to memblock\_type from where the range is taken

`struct memblock_type *type_b`
:   pointer to memblock\_type which excludes memory from being taken

`phys_addr_t *out_start`
:   ptr to phys\_addr\_t for start address of the range, can be `NULL`

`phys_addr_t *out_end`
:   ptr to phys\_addr\_t for end address of the range, can be `NULL`

`int *out_nid`
:   ptr to int for nid of the range, can be `NULL`

**Description**

Finds the next range from type\_a which is not marked as unsuitable
in type\_b.

Reverse of [`__next_mem_range()`](#c.__next_mem_range "__next_mem_range").

int \_\_init\_memblock memblock\_set\_node(phys\_addr\_t base, phys\_addr\_t size, struct [memblock\_type](#c.memblock_type "memblock_type") \*type, int nid)
:   set node ID on memblock regions

**Parameters**

`phys_addr_t base`
:   base of area to set node ID for

`phys_addr_t size`
:   size of area to set node ID for

`struct memblock_type *type`
:   memblock type to set node ID for

`int nid`
:   node ID to set

**Description**

Set the nid of memblock **type** regions in [**base**, **base** + **size**) to **nid**.
Regions which cross the area boundaries are split as necessary.

**Return**

0 on success, -errno on failure.

phys\_addr\_t memblock\_alloc\_range\_nid(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t start, phys\_addr\_t end, int nid, bool exact\_nid)
:   allocate boot memory block

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`phys_addr_t start`
:   the lower bound of the memory region to allocate (phys address)

`phys_addr_t end`
:   the upper bound of the memory region to allocate (phys address)

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

`bool exact_nid`
:   control the allocation fall back to other nodes

**Description**

The allocation is performed from memory region limited by
memblock.current\_limit if **end** == `MEMBLOCK_ALLOC_ACCESSIBLE`.

If the specified node can not hold the requested memory and **exact\_nid**
is false, the allocation falls back to any node in the system.

For systems with memory mirroring, the allocation is attempted first
from the regions with mirroring enabled and then retried from any
memory region.

In addition, function using kmemleak\_alloc\_phys for allocated boot
memory block, it is never reported as leaks.

**Return**

Physical address of allocated memory block on success, `0` on failure.

phys\_addr\_t memblock\_phys\_alloc\_range(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t start, phys\_addr\_t end)
:   allocate a memory block inside specified range

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`phys_addr_t start`
:   the lower bound of the memory region to allocate (physical address)

`phys_addr_t end`
:   the upper bound of the memory region to allocate (physical address)

**Description**

Allocate **size** bytes in the between **start** and **end**.

**Return**

physical address of the allocated memory block on success,
`0` on failure.

phys\_addr\_t memblock\_phys\_alloc\_try\_nid(phys\_addr\_t size, phys\_addr\_t align, int nid)
:   allocate a memory block from specified NUMA node

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

**Description**

Allocates memory block from the specified NUMA node. If the node
has no available memory, attempts to allocated from any node in the
system.

**Return**

physical address of the allocated memory block on success,
`0` on failure.

void \*memblock\_alloc\_internal(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t min\_addr, phys\_addr\_t max\_addr, int nid, bool exact\_nid)
:   allocate boot memory block

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`phys_addr_t min_addr`
:   the lower bound of the memory region to allocate (phys address)

`phys_addr_t max_addr`
:   the upper bound of the memory region to allocate (phys address)

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

`bool exact_nid`
:   control the allocation fall back to other nodes

**Description**

Allocates memory block using [`memblock_alloc_range_nid()`](#c.memblock_alloc_range_nid "memblock_alloc_range_nid") and
converts the returned physical address to virtual.

The **min\_addr** limit is dropped if it can not be satisfied and the allocation
will fall back to memory below **min\_addr**. Other constraints, such
as node and mirrored memory will be handled again in
[`memblock_alloc_range_nid()`](#c.memblock_alloc_range_nid "memblock_alloc_range_nid").

**Return**

Virtual address of allocated memory block on success, NULL on failure.

void \*memblock\_alloc\_exact\_nid\_raw(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t min\_addr, phys\_addr\_t max\_addr, int nid)
:   allocate boot memory block on the exact node without zeroing memory

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`phys_addr_t min_addr`
:   the lower bound of the memory region from where the allocation
    is preferred (phys address)

`phys_addr_t max_addr`
:   the upper bound of the memory region from where the allocation
    is preferred (phys address), or `MEMBLOCK_ALLOC_ACCESSIBLE` to
    allocate only from memory limited by memblock.current\_limit value

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

**Description**

Public function, provides additional debug information (including caller
info), if enabled. Does not zero allocated memory.

**Return**

Virtual address of allocated memory block on success, NULL on failure.

void \*memblock\_alloc\_try\_nid\_raw(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t min\_addr, phys\_addr\_t max\_addr, int nid)
:   allocate boot memory block without zeroing memory and without panicking

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`phys_addr_t min_addr`
:   the lower bound of the memory region from where the allocation
    is preferred (phys address)

`phys_addr_t max_addr`
:   the upper bound of the memory region from where the allocation
    is preferred (phys address), or `MEMBLOCK_ALLOC_ACCESSIBLE` to
    allocate only from memory limited by memblock.current\_limit value

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

**Description**

Public function, provides additional debug information (including caller
info), if enabled. Does not zero allocated memory, does not panic if request
cannot be satisfied.

**Return**

Virtual address of allocated memory block on success, NULL on failure.

void \*memblock\_alloc\_try\_nid(phys\_addr\_t size, phys\_addr\_t align, phys\_addr\_t min\_addr, phys\_addr\_t max\_addr, int nid)
:   allocate boot memory block

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`phys_addr_t min_addr`
:   the lower bound of the memory region from where the allocation
    is preferred (phys address)

`phys_addr_t max_addr`
:   the upper bound of the memory region from where the allocation
    is preferred (phys address), or `MEMBLOCK_ALLOC_ACCESSIBLE` to
    allocate only from memory limited by memblock.current\_limit value

`int nid`
:   nid of the free area to find, `NUMA_NO_NODE` for any node

**Description**

Public function, provides additional debug information (including caller
info), if enabled. This function zeroes the allocated memory.

**Return**

Virtual address of allocated memory block on success, NULL on failure.

void \*\_\_memblock\_alloc\_or\_panic(phys\_addr\_t size, phys\_addr\_t align, const char \*func)
:   Try to allocate memory and panic on failure

**Parameters**

`phys_addr_t size`
:   size of memory block to be allocated in bytes

`phys_addr_t align`
:   alignment of the region and block’s size

`const char *func`
:   caller func name

**Description**

This function attempts to allocate memory using memblock\_alloc,
and in case of failure, it calls panic with the formatted message.
This function should not be used directly, please use the macro memblock\_alloc\_or\_panic.

unsigned long memblock\_estimated\_nr\_free\_pages(void)
:   return estimated number of free pages from memblock point of view

**Parameters**

`void`
:   no arguments

**Description**

During bootup, subsystems might need a rough estimate of the number of free
pages in the whole system, before precise numbers are available from the
buddy. Especially with CONFIG\_DEFERRED\_STRUCT\_PAGE\_INIT, the numbers
obtained from the buddy might be very imprecise during bootup.

**Return**

An estimated number of free pages from memblock point of view.

bool \_\_init\_memblock memblock\_is\_region\_memory(phys\_addr\_t base, phys\_addr\_t size)
:   check if a region is a subset of memory

**Parameters**

`phys_addr_t base`
:   base of region to check

`phys_addr_t size`
:   size of region to check

**Description**

Check if the region [**base**, **base** + **size**) is a subset of a memory block.

**Return**

0 if false, non-zero if true

bool \_\_init\_memblock memblock\_is\_region\_reserved(phys\_addr\_t base, phys\_addr\_t size)
:   check if a region intersects reserved memory

**Parameters**

`phys_addr_t base`
:   base of region to check

`phys_addr_t size`
:   size of region to check

**Description**

Check if the region [**base**, **base** + **size**) intersects a reserved
memory block.

**Return**

True if they intersect, false if not.

void memblock\_free\_all(void)
:   release free pages to the buddy allocator

**Parameters**

`void`
:   no arguments

int reserve\_mem\_find\_by\_name(const char \*name, phys\_addr\_t \*start, phys\_addr\_t \*size)
:   Find reserved memory region with a given name

**Parameters**

`const char *name`
:   The name that is attached to a reserved memory region

`phys_addr_t *start`
:   If found, holds the start address

`phys_addr_t *size`
:   If found, holds the size of the address.

**Description**

**start** and **size** are only updated if **name** is found.

**Return**

1 if found or 0 if not found.

int reserve\_mem\_release\_by\_name(const char \*name)
:   Release reserved memory region with a given name

**Parameters**

`const char *name`
:   The name that is attached to a reserved memory region

**Description**

Forcibly release the pages in the reserved memory region so that those memory
can be used as free memory. After released the reserved region size becomes 0.

**Return**

1 if released or 0 if not found.
