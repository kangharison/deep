# Dynamic DMA mapping using the generic device

> 출처(원문): https://docs.kernel.org/core-api/dma-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Dynamic DMA mapping using the generic device

Author:
:   James E.J. Bottomley <[James.Bottomley@HansenPartnership.com](mailto:James.Bottomley%40HansenPartnership.com)>

This document describes the DMA API. For a more gentle introduction
of the API (and actual examples), see [Dynamic DMA mapping Guide](dma-api-howto.html).

This API is split into two pieces. Part I describes the basic API.
Part II describes extensions for supporting non-coherent memory
machines. Unless you know that your driver absolutely has to support
non-coherent platforms (this is usually only legacy platforms) you
should only use the API described in part I.

## Part I - DMA API

To get the DMA API, you must #include <linux/dma-mapping.h>. This
provides dma\_addr\_t and the interfaces described below.

A dma\_addr\_t can hold any valid DMA address for the platform. It can be
given to a device to use as a DMA source or target. A CPU cannot reference
a dma\_addr\_t directly because there may be translation between its physical
address space and the DMA address space.

## Part Ia - Using large DMA-coherent buffers

```
void *
dma_alloc_coherent(struct device *dev, size_t size,
                   dma_addr_t *dma_handle, gfp_t flag)
```

Coherent memory is memory for which a write by either the device or
the processor can immediately be read by the processor or device
without having to worry about caching effects. (You may however need
to make sure to flush the processor’s write buffers before telling
devices to read that memory.)

This routine allocates a region of <size> bytes of coherent memory.

It returns a pointer to the allocated region (in the processor’s virtual
address space) or NULL if the allocation failed.

It also returns a <dma\_handle> which may be cast to an unsigned integer the
same width as the bus and given to the device as the DMA address base of
the region.

Note: coherent memory can be expensive on some platforms, and the
minimum allocation length may be as big as a page, so you should
consolidate your requests for coherent memory as much as possible.
The simplest way to do that is to use the dma\_pool calls (see below).

The flag parameter allows the caller to specify the `GFP_` flags (see
[`kmalloc()`](mm-api.html#c.kmalloc "kmalloc")) for the allocation (the implementation may ignore flags that affect
the location of the returned memory, like GFP\_DMA).

```
void
dma_free_coherent(struct device *dev, size_t size, void *cpu_addr,
                  dma_addr_t dma_handle)
```

Free a previously allocated region of coherent memory. dev, size and dma\_handle
must all be the same as those passed into `dma_alloc_coherent()`. cpu\_addr must
be the virtual address returned by `dma_alloc_coherent()`.

Note that unlike the sibling allocation call, this routine may only be called
with IRQs enabled.

## Part Ib - Using small DMA-coherent buffers

To get this part of the DMA API, you must #include <linux/dmapool.h>

Many drivers need lots of small DMA-coherent memory regions for DMA
descriptors or I/O buffers. Rather than allocating in units of a page
or more using `dma_alloc_coherent()`, you can use DMA pools. These work
much like a `struct kmem_cache`, except that they use the DMA-coherent allocator,
not `__get_free_pages()`. Also, they understand common hardware constraints
for alignment, like queue heads needing to be aligned on N-byte boundaries.

struct dma\_pool \*dma\_pool\_create\_node(const char \*name, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, size\_t size, size\_t align, size\_t boundary, int node)
:   Creates a pool of coherent DMA memory blocks.

**Parameters**

`const char *name`
:   name of pool, for diagnostics

`struct device *dev`
:   device that will be doing the DMA

`size_t size`
:   size of the blocks in this pool.

`size_t align`
:   alignment requirement for blocks; must be a power of two

`size_t boundary`
:   returned blocks won’t cross this power of two boundary

`int node`
:   optional NUMA node to allocate structs ‘dma\_pool’ and ‘dma\_page’ on

**Context**

not `in_interrupt()`

**Description**

Given one of these pools, [`dma_pool_alloc()`](#c.dma_pool_alloc "dma_pool_alloc")
may be used to allocate memory. Such memory will all have coherent
DMA mappings, accessible by the device and its driver without using
cache flushing primitives. The actual size of blocks allocated may be
larger than requested because of alignment.

If **boundary** is nonzero, objects returned from [`dma_pool_alloc()`](#c.dma_pool_alloc "dma_pool_alloc") won’t
cross that size boundary. This is useful for devices which have
addressing restrictions on individual DMA transfers, such as not crossing
boundaries of 4KBytes.

**Return**

a dma allocation pool with the requested characteristics, or
`NULL` if one can’t be created.

void dma\_pool\_destroy(struct dma\_pool \*pool)
:   destroys a pool of dma memory blocks.

**Parameters**

`struct dma_pool *pool`
:   dma pool that will be destroyed

**Context**

!`in_interrupt()`

**Description**

Caller guarantees that no more memory from the pool is in use,
and that nothing will try to use the pool after this call.

void \*dma\_pool\_alloc(struct dma\_pool \*pool, gfp\_t mem\_flags, dma\_addr\_t \*handle)
:   get a block of coherent memory

**Parameters**

`struct dma_pool *pool`
:   dma pool that will produce the block

`gfp_t mem_flags`
:   GFP\_\* bitmask

`dma_addr_t *handle`
:   pointer to dma address of block

**Return**

the kernel virtual address of a currently unused block,
and reports its dma address through the handle.
If such a memory block can’t be allocated, `NULL` is returned.

void dma\_pool\_free(struct dma\_pool \*pool, void \*vaddr, dma\_addr\_t dma)
:   put block back into dma pool

**Parameters**

`struct dma_pool *pool`
:   the dma pool holding the block

`void *vaddr`
:   virtual address of block

`dma_addr_t dma`
:   dma address of block

**Description**

Caller promises neither device nor driver will again touch this block
unless it is first re-allocated.

struct dma\_pool \*dmam\_pool\_create(const char \*name, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, size\_t size, size\_t align, size\_t allocation)
:   Managed `dma_pool_create()`

**Parameters**

`const char *name`
:   name of pool, for diagnostics

`struct device *dev`
:   device that will be doing the DMA

`size_t size`
:   size of the blocks in this pool.

`size_t align`
:   alignment requirement for blocks; must be a power of two

`size_t allocation`
:   returned blocks won’t cross this boundary (or zero)

**Description**

Managed `dma_pool_create()`. DMA pool created with this function is
automatically destroyed on driver detach.

**Return**

a managed dma allocation pool with the requested
characteristics, or `NULL` if one can’t be created.

void dmam\_pool\_destroy(struct dma\_pool \*pool)
:   Managed [`dma_pool_destroy()`](#c.dma_pool_destroy "dma_pool_destroy")

**Parameters**

`struct dma_pool *pool`
:   dma pool that will be destroyed

**Description**

Managed [`dma_pool_destroy()`](#c.dma_pool_destroy "dma_pool_destroy").

void \*dma\_pool\_zalloc(struct dma\_pool \*pool, gfp\_t mem\_flags, dma\_addr\_t \*handle)
:   Get a zero-initialized block of DMA coherent memory.

**Parameters**

`struct dma_pool *pool`
:   dma pool that will produce the block

`gfp_t mem_flags`
:   GFP\_\* bitmask

`dma_addr_t *handle`
:   pointer to dma address of block

**Description**

Same as [`dma_pool_alloc()`](#c.dma_pool_alloc "dma_pool_alloc"), but the returned memory is zeroed.

## Part Ic - DMA addressing limitations

DMA mask is a bit mask of the addressable region for the device. In other words,
if applying the DMA mask (a bitwise AND operation) to the DMA address of a
memory region does not clear any bits in the address, then the device can
perform DMA to that memory region.

All the below functions which set a DMA mask may fail if the requested mask
cannot be used with the device, or if the device is not capable of doing DMA.

```
int
dma_set_mask_and_coherent(struct device *dev, u64 mask)
```

Updates both streaming and coherent DMA masks.

Returns: 0 if successful and a negative error if not.

```
int
dma_set_mask(struct device *dev, u64 mask)
```

Updates only the streaming DMA mask.

Returns: 0 if successful and a negative error if not.

```
int
dma_set_coherent_mask(struct device *dev, u64 mask)
```

Updates only the coherent DMA mask.

Returns: 0 if successful and a negative error if not.

```
u64
dma_get_required_mask(struct device *dev)
```

This API returns the mask that the platform requires to
operate efficiently. Usually this means the returned mask
is the minimum required to cover all of memory. Examining the
required mask gives drivers with variable descriptor sizes the
opportunity to use smaller descriptors as necessary.

Requesting the required mask does not alter the current mask. If you
wish to take advantage of it, you should issue a `dma_set_mask()`
call to set the mask to the value returned.

```
size_t
dma_max_mapping_size(struct device *dev);
```

Returns the maximum size of a mapping for the device. The size parameter
of the mapping functions like `dma_map_single()`, `dma_map_page()` and
others should not be larger than the returned value.

```
size_t
dma_opt_mapping_size(struct device *dev);
```

Returns the maximum optimal size of a mapping for the device.

Mapping larger buffers may take much longer in certain scenarios. In
addition, for high-rate short-lived streaming mappings, the upfront time
spent on the mapping may account for an appreciable part of the total
request lifetime. As such, if splitting larger requests incurs no
significant performance penalty, then device drivers are advised to
limit total DMA streaming mappings length to the returned value.

```
bool
dma_need_sync(struct device *dev, dma_addr_t dma_addr);
```

Returns %true if dma\_sync\_single\_for\_{device,cpu} calls are required to
transfer memory ownership. Returns %false if those calls can be skipped.

```
unsigned long
dma_get_merge_boundary(struct device *dev);
```

Returns the DMA merge boundary. If the device cannot merge any DMA address
segments, the function returns 0.

## Part Id - Streaming DMA mappings

Streaming DMA allows to map an existing buffer for DMA transfers and then
unmap it when finished. Map functions are not guaranteed to succeed, so the
return value must be checked.

Note

In particular, mapping may fail for memory not addressable by the
device, e.g. if it is not within the DMA mask of the device and/or a
connecting bus bridge. Streaming DMA functions try to overcome such
addressing constraints, either by using an IOMMU (a device which maps
I/O DMA addresses to physical memory addresses), or by copying the
data to/from a bounce buffer if the kernel is configured with a
[SWIOTLB](swiotlb.html). However, these methods are not always
available, and even if they are, they may still fail for a number of
reasons.

In short, a device driver may need to be wary of where buffers are
located in physical memory, especially if the DMA mask is less than 32
bits.

```
dma_addr_t
dma_map_single(struct device *dev, void *cpu_addr, size_t size,
               enum dma_data_direction direction)
```

Maps a piece of processor virtual memory so it can be accessed by the
device and returns the DMA address of the memory.

The DMA API uses a strongly typed enumerator for its direction:

|  |  |
| --- | --- |
| DMA\_NONE | no direction (used for debugging) |
| DMA\_TO\_DEVICE | data is going from the memory to the device |
| DMA\_FROM\_DEVICE | data is coming from the device to the memory |
| DMA\_BIDIRECTIONAL | direction isn’t known |

Note

Contiguous kernel virtual space may not be contiguous as
physical memory. Since this API does not provide any scatter/gather
capability, it will fail if the user tries to map a non-physically
contiguous piece of memory. For this reason, memory to be mapped by
this API should be obtained from sources which guarantee it to be
physically contiguous (like kmalloc).

Warning

Memory coherency operates at a granularity called the cache
line width. In order for memory mapped by this API to operate
correctly, the mapped region must begin exactly on a cache line
boundary and end exactly on one (to prevent two separately mapped
regions from sharing a single cache line). Since the cache line size
may not be known at compile time, the API will not enforce this
requirement. Therefore, it is recommended that driver writers who
don’t take special care to determine the cache line size at run time
only map virtual regions that begin and end on page boundaries (which
are guaranteed also to be cache line boundaries).

DMA\_TO\_DEVICE synchronisation must be done after the last modification
of the memory region by the software and before it is handed off to
the device. Once this primitive is used, memory covered by this
primitive should be treated as read-only by the device. If the device
may write to it at any point, it should be DMA\_BIDIRECTIONAL (see
below).

DMA\_FROM\_DEVICE synchronisation must be done before the driver
accesses data that may be changed by the device. This memory should
be treated as read-only by the driver. If the driver needs to write
to it at any point, it should be DMA\_BIDIRECTIONAL (see below).

DMA\_BIDIRECTIONAL requires special handling: it means that the driver
isn’t sure if the memory was modified before being handed off to the
device and also isn’t sure if the device will also modify it. Thus,
you must always sync bidirectional memory twice: once before the
memory is handed off to the device (to make sure all memory changes
are flushed from the processor) and once before the data may be
accessed after being used by the device (to make sure any processor
cache lines are updated with data that the device may have changed).

```
void
dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
                 enum dma_data_direction direction)
```

Unmaps the region previously mapped. All the parameters passed in
must be identical to those passed to (and returned by) `dma_map_single()`.

```
dma_addr_t
dma_map_page(struct device *dev, struct page *page,
             unsigned long offset, size_t size,
             enum dma_data_direction direction)

void
dma_unmap_page(struct device *dev, dma_addr_t dma_address, size_t size,
               enum dma_data_direction direction)
```

API for mapping and unmapping for pages. All the notes and warnings
for the other mapping APIs apply here. Also, although the <offset>
and <size> parameters are provided to do partial page mapping, it is
recommended that you never use these unless you really know what the
cache width is.

```
dma_addr_t
dma_map_resource(struct device *dev, phys_addr_t phys_addr, size_t size,
                 enum dma_data_direction dir, unsigned long attrs)

void
dma_unmap_resource(struct device *dev, dma_addr_t addr, size_t size,
                   enum dma_data_direction dir, unsigned long attrs)
```

API for mapping and unmapping for MMIO resources. All the notes and
warnings for the other mapping APIs apply here. The API should only be
used to map device MMIO resources, mapping of RAM is not permitted.

```
int
dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
```

In some circumstances `dma_map_single()`, `dma_map_page()` and `dma_map_resource()`
will fail to create a mapping. A driver can check for these errors by testing
the returned DMA address with `dma_mapping_error()`. A non-zero return value
means the mapping could not be created and the driver should take appropriate
action (e.g. reduce current DMA mapping usage or delay and try again later).

```
int
dma_map_sg(struct device *dev, struct scatterlist *sg,
           int nents, enum dma_data_direction direction)
```

Maps a scatter/gather list for DMA. Returns the number of DMA address segments
mapped, which may be smaller than <nents> passed in if several consecutive
sglist entries are merged (e.g. with an IOMMU, or if some adjacent segments
just happen to be physically contiguous).

Please note that the sg cannot be mapped again if it has been mapped once.
The mapping process is allowed to destroy information in the sg.

As with the other mapping interfaces, `dma_map_sg()` can fail. When it
does, 0 is returned and a driver must take appropriate action. It is
critical that the driver do something, in the case of a block driver
aborting the request or even oopsing is better than doing nothing and
corrupting the filesystem.

With scatterlists, you use the resulting mapping like this:

```
int i, count = dma_map_sg(dev, sglist, nents, direction);
struct scatterlist *sg;

for_each_sg(sglist, sg, count, i) {
        hw_address[i] = sg_dma_address(sg);
        hw_len[i] = sg_dma_len(sg);
}
```

where nents is the number of entries in the sglist.

The implementation is free to merge several consecutive sglist entries
into one. The returned number is the actual number of sg entries it
mapped them to. On failure, 0 is returned.

Then you should loop count times (note: this can be less than nents times)
and use `sg_dma_address()` and `sg_dma_len()` macros where you previously
accessed sg->address and sg->length as shown above.

```
void
dma_unmap_sg(struct device *dev, struct scatterlist *sg,
             int nents, enum dma_data_direction direction)
```

Unmap the previously mapped scatter/gather list. All the parameters
must be the same as those and passed in to the scatter/gather mapping
API.

Note: <nents> must be the number you passed in, *not* the number of
DMA address entries returned.

```
void
dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_handle,
                        size_t size,
                        enum dma_data_direction direction)

void
dma_sync_single_for_device(struct device *dev, dma_addr_t dma_handle,
                           size_t size,
                           enum dma_data_direction direction)

void
dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
                    int nents,
                    enum dma_data_direction direction)

void
dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
                       int nents,
                       enum dma_data_direction direction)
```

Synchronise a single contiguous or scatter/gather mapping for the CPU
and device. With the sync\_sg API, all the parameters must be the same
as those passed into the sg mapping API. With the sync\_single API,
you can use dma\_handle and size parameters that aren’t identical to
those passed into the single mapping API to do a partial sync.

Note

You must do this:

* Before reading values that have been written by DMA from the device
  (use the DMA\_FROM\_DEVICE direction)
* After writing values that will be written to the device using DMA
  (use the DMA\_TO\_DEVICE) direction
* before *and* after handing memory to the device if the memory is
  DMA\_BIDIRECTIONAL

See also `dma_map_single()`.

```
dma_addr_t
dma_map_single_attrs(struct device *dev, void *cpu_addr, size_t size,
                     enum dma_data_direction dir,
                     unsigned long attrs)

void
dma_unmap_single_attrs(struct device *dev, dma_addr_t dma_addr,
                       size_t size, enum dma_data_direction dir,
                       unsigned long attrs)

int
dma_map_sg_attrs(struct device *dev, struct scatterlist *sgl,
                 int nents, enum dma_data_direction dir,
                 unsigned long attrs)

void
dma_unmap_sg_attrs(struct device *dev, struct scatterlist *sgl,
                   int nents, enum dma_data_direction dir,
                   unsigned long attrs)
```

The four functions above are just like the counterpart functions
without the \_attrs suffixes, except that they pass an optional
dma\_attrs.

The interpretation of DMA attributes is architecture-specific, and
each attribute should be documented in
[DMA attributes](dma-attributes.html).

If dma\_attrs are 0, the semantics of each of these functions
is identical to those of the corresponding function
without the \_attrs suffix. As a result `dma_map_single_attrs()`
can generally replace `dma_map_single()`, etc.

As an example of the use of the `*_attrs` functions, here’s how
you could pass an attribute DMA\_ATTR\_FOO when mapping memory
for DMA:

```
#include <linux/dma-mapping.h>
/* DMA_ATTR_FOO should be defined in linux/dma-mapping.h and
* documented in Documentation/core-api/dma-attributes.rst */
...

        unsigned long attr;
        attr |= DMA_ATTR_FOO;
        ....
        n = dma_map_sg_attrs(dev, sg, nents, DMA_TO_DEVICE, attr);
        ....
```

Architectures that care about DMA\_ATTR\_FOO would check for its
presence in their implementations of the mapping and unmapping
routines, e.g.::

```
void whizco_dma_map_sg_attrs(struct device *dev, dma_addr_t dma_addr,
                             size_t size, enum dma_data_direction dir,
                             unsigned long attrs)
{
        ....
        if (attrs & DMA_ATTR_FOO)
                /* twizzle the frobnozzle */
        ....
}
```

## Part Ie - IOVA-based DMA mappings

These APIs allow a very efficient mapping when using an IOMMU. They are an
optional path that requires extra code and are only recommended for drivers
where DMA mapping performance, or the space usage for storing the DMA addresses
matter. All the considerations from the previous section apply here as well.

```
bool dma_iova_try_alloc(struct device *dev, struct dma_iova_state *state,
            phys_addr_t phys, size_t size);
```

Is used to try to allocate IOVA space for mapping operation. If it returns
false this API can’t be used for the given device and the normal streaming
DMA mapping API should be used. The `struct dma_iova_state` is allocated
by the driver and must be kept around until unmap time.

```
static inline bool dma_use_iova(struct dma_iova_state *state)
```

Can be used by the driver to check if the IOVA-based API is used after a
call to dma\_iova\_try\_alloc. This can be useful in the unmap path.

```
int dma_iova_link(struct device *dev, struct dma_iova_state *state,
            phys_addr_t phys, size_t offset, size_t size,
            enum dma_data_direction dir, unsigned long attrs);
```

Is used to link ranges to the IOVA previously allocated. The start of all
but the first call to dma\_iova\_link for a given state must be aligned
to the DMA merge boundary returned by `dma_get_merge_boundary())`, and
the size of all but the last range must be aligned to the DMA merge boundary
as well.

```
int dma_iova_sync(struct device *dev, struct dma_iova_state *state,
            size_t offset, size_t size);
```

Must be called to sync the IOMMU page tables for IOVA-range mapped by one or
more calls to `dma_iova_link()`.

For drivers that use a one-shot mapping, all ranges can be unmapped and the
IOVA freed by calling:

```
void dma_iova_destroy(struct device *dev, struct dma_iova_state *state,
             size_t mapped_len, enum dma_data_direction dir,
             unsigned long attrs);
```

Alternatively drivers can dynamically manage the IOVA space by unmapping
and mapping individual regions. In that case

```
void dma_iova_unlink(struct device *dev, struct dma_iova_state *state,
            size_t offset, size_t size, enum dma_data_direction dir,
            unsigned long attrs);
```

is used to unmap a range previously mapped, and

```
void dma_iova_free(struct device *dev, struct dma_iova_state *state);
```

is used to free the IOVA space. All regions must have been unmapped using
`dma_iova_unlink()` before calling `dma_iova_free()`.

## Part II - Non-coherent DMA allocations

These APIs allow to allocate pages that are guaranteed to be DMA addressable
by the passed in device, but which need explicit management of memory ownership
for the kernel vs the device.

If you don’t understand how cache line coherency works between a processor and
an I/O device, you should not be using this part of the API.

```
struct page *
dma_alloc_pages(struct device *dev, size_t size, dma_addr_t *dma_handle,
                enum dma_data_direction dir, gfp_t gfp)
```

This routine allocates a region of <size> bytes of non-coherent memory. It
returns a pointer to first `struct page` for the region, or NULL if the
allocation failed. The resulting `struct page` can be used for everything a
`struct page` is suitable for.

It also returns a <dma\_handle> which may be cast to an unsigned integer the
same width as the bus and given to the device as the DMA address base of
the region.

The dir parameter specified if data is read and/or written by the device,
see `dma_map_single()` for details.

The gfp parameter allows the caller to specify the `GFP_` flags (see
[`kmalloc()`](mm-api.html#c.kmalloc "kmalloc")) for the allocation, but rejects flags used to specify a memory
zone such as GFP\_DMA or GFP\_HIGHMEM.

Before giving the memory to the device, `dma_sync_single_for_device()` needs
to be called, and before reading memory written by the device,
`dma_sync_single_for_cpu()`, just like for streaming DMA mappings that are
reused.

```
void
dma_free_pages(struct device *dev, size_t size, struct page *page,
                dma_addr_t dma_handle, enum dma_data_direction dir)
```

Free a region of memory previously allocated using `dma_alloc_pages()`.
dev, size, dma\_handle and dir must all be the same as those passed into
`dma_alloc_pages()`. page must be the pointer returned by `dma_alloc_pages()`.

```
int
dma_mmap_pages(struct device *dev, struct vm_area_struct *vma,
               size_t size, struct page *page)
```

Map an allocation returned from `dma_alloc_pages()` into a user address space.
dev and size must be the same as those passed into `dma_alloc_pages()`.
page must be the pointer returned by `dma_alloc_pages()`.

```
void *
dma_alloc_noncoherent(struct device *dev, size_t size,
                dma_addr_t *dma_handle, enum dma_data_direction dir,
                gfp_t gfp)
```

This routine is a convenient wrapper around dma\_alloc\_pages that returns the
kernel virtual address for the allocated memory instead of the page structure.

```
void
dma_free_noncoherent(struct device *dev, size_t size, void *cpu_addr,
                dma_addr_t dma_handle, enum dma_data_direction dir)
```

Free a region of memory previously allocated using `dma_alloc_noncoherent()`.
dev, size, dma\_handle and dir must all be the same as those passed into
`dma_alloc_noncoherent()`. cpu\_addr must be the virtual address returned by
`dma_alloc_noncoherent()`.

```
struct sg_table *
dma_alloc_noncontiguous(struct device *dev, size_t size,
                        enum dma_data_direction dir, gfp_t gfp,
                        unsigned long attrs);
```

This routine allocates <size> bytes of non-coherent and possibly non-contiguous
memory. It returns a pointer to `struct sg_table` that describes the allocated
and DMA mapped memory, or NULL if the allocation failed. The resulting memory
can be used for `struct page` mapped into a scatterlist are suitable for.

The return sg\_table is guaranteed to have 1 single DMA mapped segment as
indicated by sgt->nents, but it might have multiple CPU side segments as
indicated by sgt->orig\_nents.

The dir parameter specified if data is read and/or written by the device,
see `dma_map_single()` for details.

The gfp parameter allows the caller to specify the `GFP_` flags (see
[`kmalloc()`](mm-api.html#c.kmalloc "kmalloc")) for the allocation, but rejects flags used to specify a memory
zone such as GFP\_DMA or GFP\_HIGHMEM.

The attrs argument must be either 0 or DMA\_ATTR\_ALLOC\_SINGLE\_PAGES.

Before giving the memory to the device, `dma_sync_sgtable_for_device()` needs
to be called, and before reading memory written by the device,
`dma_sync_sgtable_for_cpu()`, just like for streaming DMA mappings that are
reused.

```
void
dma_free_noncontiguous(struct device *dev, size_t size,
                       struct sg_table *sgt,
                       enum dma_data_direction dir)
```

Free memory previously allocated using `dma_alloc_noncontiguous()`. dev, size,
and dir must all be the same as those passed into `dma_alloc_noncontiguous()`.
sgt must be the pointer returned by `dma_alloc_noncontiguous()`.

```
void *
dma_vmap_noncontiguous(struct device *dev, size_t size,
        struct sg_table *sgt)
```

Return a contiguous kernel mapping for an allocation returned from
`dma_alloc_noncontiguous()`. dev and size must be the same as those passed into
`dma_alloc_noncontiguous()`. sgt must be the pointer returned by
`dma_alloc_noncontiguous()`.

Once a non-contiguous allocation is mapped using this function, the
`flush_kernel_vmap_range()` and `invalidate_kernel_vmap_range()` APIs must be used
to manage the coherency between the kernel mapping, the device and user space
mappings (if any).

```
void
dma_vunmap_noncontiguous(struct device *dev, void *vaddr)
```

Unmap a kernel mapping returned by `dma_vmap_noncontiguous()`. dev must be the
same the one passed into `dma_alloc_noncontiguous()`. vaddr must be the pointer
returned by `dma_vmap_noncontiguous()`.

```
int
dma_mmap_noncontiguous(struct device *dev, struct vm_area_struct *vma,
                       size_t size, struct sg_table *sgt)
```

Map an allocation returned from `dma_alloc_noncontiguous()` into a user address
space. dev and size must be the same as those passed into
`dma_alloc_noncontiguous()`. sgt must be the pointer returned by
`dma_alloc_noncontiguous()`.

```
int
dma_get_cache_alignment(void)
```

Returns the processor cache alignment. This is the absolute minimum
alignment *and* width that you must observe when either mapping
memory or doing partial flushes.

Note

This API may return a number *larger* than the actual cache
line, but it will guarantee that one or more cache lines fit exactly
into the width returned by this call. It will also always be a power
of two for easy alignment.

## Part III - Debug drivers use of the DMA API

The DMA API as described above has some constraints. DMA addresses must be
released with the corresponding function with the same size for example. With
the advent of hardware IOMMUs it becomes more and more important that drivers
do not violate those constraints. In the worst case such a violation can
result in data corruption up to destroyed filesystems.

To debug drivers and find bugs in the usage of the DMA API checking code can
be compiled into the kernel which will tell the developer about those
violations. If your architecture supports it you can select the “Enable
debugging of DMA API usage” option in your kernel configuration. Enabling this
option has a performance impact. Do not enable it in production kernels.

If you boot the resulting kernel will contain code which does some bookkeeping
about what DMA memory was allocated for which device. If this code detects an
error it prints a warning message with some details into your kernel log. An
example warning message may look like this:

```
WARNING: at /data2/repos/linux-2.6-iommu/lib/dma-debug.c:448
        check_unmap+0x203/0x490()
Hardware name:
forcedeth 0000:00:08.0: DMA-API: device driver frees DMA memory with wrong
        function [device address=0x00000000640444be] [size=66 bytes] [mapped as
single] [unmapped as page]
Modules linked in: nfsd exportfs bridge stp llc r8169
Pid: 0, comm: swapper Tainted: G        W  2.6.28-dmatest-09289-g8bb99c0 #1
Call Trace:
<IRQ>  [<ffffffff80240b22>] warn_slowpath+0xf2/0x130
[<ffffffff80647b70>] _spin_unlock+0x10/0x30
[<ffffffff80537e75>] usb_hcd_link_urb_to_ep+0x75/0xc0
[<ffffffff80647c22>] _spin_unlock_irqrestore+0x12/0x40
[<ffffffff8055347f>] ohci_urb_enqueue+0x19f/0x7c0
[<ffffffff80252f96>] queue_work+0x56/0x60
[<ffffffff80237e10>] enqueue_task_fair+0x20/0x50
[<ffffffff80539279>] usb_hcd_submit_urb+0x379/0xbc0
[<ffffffff803b78c3>] cpumask_next_and+0x23/0x40
[<ffffffff80235177>] find_busiest_group+0x207/0x8a0
[<ffffffff8064784f>] _spin_lock_irqsave+0x1f/0x50
[<ffffffff803c7ea3>] check_unmap+0x203/0x490
[<ffffffff803c8259>] debug_dma_unmap_phys+0x49/0x50
[<ffffffff80485f26>] nv_tx_done_optimized+0xc6/0x2c0
[<ffffffff80486c13>] nv_nic_irq_optimized+0x73/0x2b0
[<ffffffff8026df84>] handle_IRQ_event+0x34/0x70
[<ffffffff8026ffe9>] handle_edge_irq+0xc9/0x150
[<ffffffff8020e3ab>] do_IRQ+0xcb/0x1c0
[<ffffffff8020c093>] ret_from_intr+0x0/0xa
<EOI> <4>---[ end trace f6435a98e2a38c0e ]---
```

The driver developer can find the driver and the device including a stacktrace
of the DMA API call which caused this warning.

Per default only the first error will result in a warning message. All other
errors will only silently counted. This limitation exist to prevent the code
from flooding your kernel log. To support debugging a device driver this can
be disabled via debugfs. See the debugfs interface documentation below for
details.

The debugfs directory for the DMA API debugging code is called dma-api/. In
this directory the following files can currently be found:

|  |  |
| --- | --- |
| dma-api/all\_errors | This file contains a numeric value. If this value is not equal to zero the debugging code will print a warning for every error it finds into the kernel log. Be careful with this option, as it can easily flood your logs. |
| dma-api/disabled | This read-only file contains the character ‘Y’ if the debugging code is disabled. This can happen when it runs out of memory or if it was disabled at boot time |
| dma-api/dump | This read-only file contains current DMA mappings. |
| dma-api/error\_count | This file is read-only and shows the total numbers of errors found. |
| dma-api/num\_errors | The number in this file shows how many warnings will be printed to the kernel log before it stops. This number is initialized to one at system boot and be set by writing into this file |
| dma-api/min\_free\_entries | This read-only file can be read to get the minimum number of free dma\_debug\_entries the allocator has ever seen. If this value goes down to zero the code will attempt to increase nr\_total\_entries to compensate. |
| dma-api/num\_free\_entries | The current number of free dma\_debug\_entries in the allocator. |
| dma-api/nr\_total\_entries | The total number of dma\_debug\_entries in the allocator, both free and used. |
| dma-api/driver\_filter | You can write a name of a driver into this file to limit the debug output to requests from that particular driver. Write an empty string to that file to disable the filter and see all errors again. |

If you have this code compiled into your kernel it will be enabled by default.
If you want to boot without the bookkeeping anyway you can provide
‘dma\_debug=off’ as a boot parameter. This will disable DMA API debugging.
Notice that you can not enable it again at runtime. You have to reboot to do
so.

If you want to see debug messages only for a special device driver you can
specify the dma\_debug\_driver=<drivername> parameter. This will enable the
driver filter at boot time. The debug code will only print errors for that
driver afterwards. This filter can be disabled or changed later using debugfs.

When the code disables itself at runtime this is most likely because it ran
out of dma\_debug\_entries and was unable to allocate more on-demand. 65536
entries are preallocated at boot - if this is too low for you boot with
‘dma\_debug\_entries=<your\_desired\_number>’ to overwrite the default. Note
that the code allocates entries in batches, so the exact number of
preallocated entries may be greater than the actual number requested. The
code will print to the kernel log each time it has dynamically allocated
as many entries as were initially preallocated. This is to indicate that a
larger preallocation size may be appropriate, or if it happens continually
that a driver may be leaking mappings.

```
void
debug_dma_mapping_error(struct device *dev, dma_addr_t dma_addr);
```

dma-debug interface `debug_dma_mapping_error()` to debug drivers that fail
to check DMA mapping errors on addresses returned by `dma_map_single()` and
`dma_map_page()` interfaces. This interface clears a flag set by
`debug_dma_map_phys()` to indicate that `dma_mapping_error()` has been called by
the driver. When driver does unmap, `debug_dma_unmap()` checks the flag and if
this flag is still set, prints warning message that includes call trace that
leads up to the unmap. This interface can be called from `dma_mapping_error()`
routines to enable DMA mapping error check debugging.

### Functions and structures

struct scatterlist \*sg\_next(struct scatterlist \*sg)
:   return the next scatterlist entry in a list

**Parameters**

`struct scatterlist *sg`
:   The current sg entry

**Description**

> Usually the next entry will be **sg** + 1, but if this sg element is part
> of a chained scatterlist, it could jump to the start of a new
> scatterlist array.

void sg\_assign\_page(struct scatterlist \*sg, struct [page](#c.sg_assign_page "page") \*page)
:   Assign a given page to an SG entry

**Parameters**

`struct scatterlist *sg`
:   SG entry

`struct page *page`
:   The page

**Description**

> Assign page to sg entry. Also see [`sg_set_page()`](#c.sg_set_page "sg_set_page"), the most commonly used
> variant.

void sg\_set\_page(struct scatterlist \*sg, struct [page](#c.sg_set_page "page") \*page, unsigned int len, unsigned int offset)
:   Set sg entry to point at given page

**Parameters**

`struct scatterlist *sg`
:   SG entry

`struct page *page`
:   The page

`unsigned int len`
:   Length of data

`unsigned int offset`
:   Offset into page

**Description**

> Use this function to set an sg entry pointing at a page, never assign
> the page directly. We encode sg table information in the lower bits
> of the page pointer. See `sg_page()` for looking up the page belonging
> to an sg entry.

void sg\_set\_folio(struct scatterlist \*sg, struct [folio](#c.sg_set_folio "folio") \*folio, size\_t len, size\_t offset)
:   Set sg entry to point at given folio

**Parameters**

`struct scatterlist *sg`
:   SG entry

`struct folio *folio`
:   The folio

`size_t len`
:   Length of data

`size_t offset`
:   Offset into folio

**Description**

> Use this function to set an sg entry pointing at a folio, never assign
> the folio directly. We encode sg table information in the lower bits
> of the folio pointer. See `sg_page()` for looking up the page belonging
> to an sg entry.

void sg\_set\_buf(struct scatterlist \*sg, const void \*buf, unsigned int buflen)
:   Set sg entry to point at given data

**Parameters**

`struct scatterlist *sg`
:   SG entry

`const void *buf`
:   Data

`unsigned int buflen`
:   Data length

void sg\_chain(struct scatterlist \*prv, unsigned int prv\_nents, struct scatterlist \*sgl)
:   Chain two sglists together

**Parameters**

`struct scatterlist *prv`
:   First scatterlist

`unsigned int prv_nents`
:   Number of entries in prv

`struct scatterlist *sgl`
:   Second scatterlist

**Description**

> Links **prv** and **sgl** together, to form a longer scatterlist.

void sg\_mark\_end(struct scatterlist \*sg)
:   Mark the end of the scatterlist

**Parameters**

`struct scatterlist *sg`
:   SG entryScatterlist

**Description**

> Marks the passed in sg entry as the termination point for the sg
> table. A call to [`sg_next()`](#c.sg_next "sg_next") on this entry will return NULL.

void sg\_unmark\_end(struct scatterlist \*sg)
:   Undo setting the end of the scatterlist

**Parameters**

`struct scatterlist *sg`
:   SG entryScatterlist

**Description**

> Removes the termination marker from the given entry of the scatterlist.

bool sg\_dma\_is\_bus\_address(struct scatterlist \*sg)
:   Return whether a given segment was marked as a bus address

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> Returns true if [`sg_dma_mark_bus_address()`](#c.sg_dma_mark_bus_address "sg_dma_mark_bus_address") has been called on
> this segment.

void sg\_dma\_mark\_bus\_address(struct scatterlist \*sg)
:   Mark the scatterlist entry as a bus address

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> Marks the passed in sg entry to indicate that the dma\_address is
> a bus address and doesn’t need to be unmapped. This should only be
> used by `dma_map_sg()` implementations to mark bus addresses
> so they can be properly cleaned up in `dma_unmap_sg()`.

void sg\_dma\_unmark\_bus\_address(struct scatterlist \*sg)
:   Unmark the scatterlist entry as a bus address

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> Clears the bus address mark.

bool sg\_dma\_is\_swiotlb(struct scatterlist \*sg)
:   Return whether the scatterlist was marked for SWIOTLB bouncing

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> Returns true if the scatterlist was marked for SWIOTLB bouncing. Not all
> elements may have been bounced, so the caller would have to check
> individual SG entries with `swiotlb_find_pool()`.

void sg\_dma\_mark\_swiotlb(struct scatterlist \*sg)
:   Mark the scatterlist for SWIOTLB bouncing

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> Marks a a scatterlist for SWIOTLB bounce. Not all SG entries may be
> bounced.

dma\_addr\_t sg\_phys(struct scatterlist \*sg)
:   Return physical address of an sg entry

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> This calls `page_to_phys()` on the page in this sg entry, and adds the
> sg offset. The caller must know that it is legal to call `page_to_phys()`
> on the sg page.

void \*sg\_virt(struct scatterlist \*sg)
:   Return virtual address of an sg entry

**Parameters**

`struct scatterlist *sg`
:   SG entry

**Description**

> This calls [`page_address()`](../mm/highmem.html#c.page_address "page_address") on the page in this sg entry, and adds the
> sg offset. The caller must know that the sg page has a valid virtual
> mapping.

void sg\_init\_marker(struct scatterlist \*sgl, unsigned int nents)
:   Initialize markers in sg table

**Parameters**

`struct scatterlist *sgl`
:   The SG table

`unsigned int nents`
:   Number of entries in table

int sg\_alloc\_table\_from\_pages(struct sg\_table \*sgt, struct page \*\*pages, unsigned int n\_pages, unsigned int offset, unsigned long size, gfp\_t gfp\_mask)
:   Allocate and initialize an sg table from an array of pages

**Parameters**

`struct sg_table *sgt`
:   The sg table header to use

`struct page **pages`
:   Pointer to an array of page pointers

`unsigned int n_pages`
:   Number of pages in the pages array

`unsigned int offset`
:   Offset from start of the first page to the start of a buffer

`unsigned long size`
:   Number of valid bytes in the buffer (after offset)

`gfp_t gfp_mask`
:   GFP allocation mask

**Description**

> Allocate and initialize an sg table from a list of pages. Contiguous
> ranges of the pages are squashed into a single scatterlist node. A user
> may provide an offset at a start and a size of valid data in a buffer
> specified by the page array. The returned sg table is released by
> sg\_free\_table.

**Return**

0 on success, negative error on failure

struct page \*sg\_page\_iter\_page(struct sg\_page\_iter \*piter)
:   get the current page held by the page iterator

**Parameters**

`struct sg_page_iter *piter`
:   page iterator holding the page

dma\_addr\_t sg\_page\_iter\_dma\_address(struct sg\_dma\_page\_iter \*dma\_iter)
:   get the dma address of the current page held by the page iterator.

**Parameters**

`struct sg_dma_page_iter *dma_iter`
:   page iterator holding the page

for\_each\_sg\_page

`for_each_sg_page (sglist, piter, nents, pgoffset)`

> iterate over the pages of the given sg list

**Parameters**

`sglist`
:   sglist to iterate over

`piter`
:   page iterator to hold current page, sg, sg\_pgoffset

`nents`
:   maximum number of sg entries to iterate over

`pgoffset`
:   starting page offset (in pages)

**Description**

Callers may use [`sg_page_iter_page()`](#c.sg_page_iter_page "sg_page_iter_page") to get each page pointer.
In each loop it operates on PAGE\_SIZE unit.

for\_each\_sg\_dma\_page

`for_each_sg_dma_page (sglist, dma_iter, dma_nents, pgoffset)`

> iterate over the pages of the given sg list

**Parameters**

`sglist`
:   sglist to iterate over

`dma_iter`
:   DMA page iterator to hold current page

`dma_nents`
:   maximum number of sg entries to iterate over, this is the value
    returned from dma\_map\_sg

`pgoffset`
:   starting page offset (in pages)

**Description**

Callers may use [`sg_page_iter_dma_address()`](#c.sg_page_iter_dma_address "sg_page_iter_dma_address") to get each page’s DMA address.
In each loop it operates on PAGE\_SIZE unit.

for\_each\_sgtable\_page

`for_each_sgtable_page (sgt, piter, pgoffset)`

> iterate over all pages in the sg\_table object

**Parameters**

`sgt`
:   sg\_table object to iterate over

`piter`
:   page iterator to hold current page

`pgoffset`
:   starting page offset (in pages)

**Description**

Iterates over the all memory pages in the buffer described by
a scatterlist stored in the given sg\_table object.
See also [`for_each_sg_page()`](#c.for_each_sg_page "for_each_sg_page"). In each loop it operates on PAGE\_SIZE unit.

for\_each\_sgtable\_dma\_page

`for_each_sgtable_dma_page (sgt, dma_iter, pgoffset)`

> iterate over the DMA mapped sg\_table object

**Parameters**

`sgt`
:   sg\_table object to iterate over

`dma_iter`
:   DMA page iterator to hold current page

`pgoffset`
:   starting page offset (in pages)

**Description**

Iterates over the all DMA mapped pages in the buffer described by
a scatterlist stored in the given sg\_table object.
See also [`for_each_sg_dma_page()`](#c.for_each_sg_dma_page "for_each_sg_dma_page"). In each loop it operates on PAGE\_SIZE
unit.

int sg\_nents(struct scatterlist \*sg)
:   return total count of entries in scatterlist

**Parameters**

`struct scatterlist *sg`
:   The scatterlist

**Description**

Allows to know how many entries are in sg, taking into account
chaining as well

int sg\_nents\_for\_len(struct scatterlist \*sg, u64 len)
:   return total count of entries in scatterlist needed to satisfy the supplied length

**Parameters**

`struct scatterlist *sg`
:   The scatterlist

`u64 len`
:   The total required length

**Description**

Determines the number of entries in sg that are required to meet
the supplied length, taking into account chaining as well

**Return**

the number of sg entries needed, negative error on failure

int sg\_nents\_for\_dma(struct scatterlist \*sgl, unsigned int sglen, size\_t len)
:   return the count of DMA-capable entries in scatterlist

**Parameters**

`struct scatterlist *sgl`
:   The scatterlist

`unsigned int sglen`
:   The current number of entries

`size_t len`
:   The maximum length of DMA-capable block

**Description**

Determines the number of entries in **sgl** which would be permitted in
DMA-capable transfer if list had been split accordingly, taking into
account chaining as well.

**Return**

the number of sgl entries needed

struct scatterlist \*sg\_last(struct scatterlist \*sgl, unsigned int nents)
:   return the last scatterlist entry in a list

**Parameters**

`struct scatterlist *sgl`
:   First entry in the scatterlist

`unsigned int nents`
:   Number of entries in the scatterlist

**Description**

> Should only be used casually, it (currently) scans the entire list
> to get the last entry.
>
> Note that the **sgl** pointer passed in need not be the first one,
> the important bit is that **nents** denotes the number of entries that
> exist from **sgl**.

void sg\_init\_table(struct scatterlist \*sgl, unsigned int nents)
:   Initialize SG table

**Parameters**

`struct scatterlist *sgl`
:   The SG table

`unsigned int nents`
:   Number of entries in table

**Notes**

> If this is part of a chained sg table, [`sg_mark_end()`](#c.sg_mark_end "sg_mark_end") should be
> used only on the last table part.

void sg\_init\_one(struct scatterlist \*sg, const void \*buf, unsigned int buflen)
:   Initialize a single entry sg list

**Parameters**

`struct scatterlist *sg`
:   SG entry

`const void *buf`
:   Virtual address for IO

`unsigned int buflen`
:   IO length

void \_\_sg\_free\_table(struct sg\_table \*table, unsigned int max\_ents, unsigned int nents\_first\_chunk, sg\_free\_fn \*free\_fn, unsigned int num\_ents)
:   Free a previously mapped sg table

**Parameters**

`struct sg_table *table`
:   The sg table header to use

`unsigned int max_ents`
:   The maximum number of entries per single scatterlist

`unsigned int nents_first_chunk`
:   Number of entries int the (preallocated) first
    scatterlist chunk, 0 means no such preallocated first chunk

`sg_free_fn *free_fn`
:   Free function

`unsigned int num_ents`
:   Number of entries in the table

**Description**

> Free an sg table previously allocated and setup with
> [`__sg_alloc_table()`](#c.__sg_alloc_table "__sg_alloc_table"). The **max\_ents** value must be identical to
> that previously used with [`__sg_alloc_table()`](#c.__sg_alloc_table "__sg_alloc_table").

void sg\_free\_append\_table(struct sg\_append\_table \*table)
:   Free a previously allocated append sg table.

**Parameters**

`struct sg_append_table *table`
:   The mapped sg append table header

void sg\_free\_table(struct sg\_table \*table)
:   Free a previously allocated sg table

**Parameters**

`struct sg_table *table`
:   The mapped sg table header

int \_\_sg\_alloc\_table(struct sg\_table \*table, unsigned int nents, unsigned int max\_ents, struct scatterlist \*first\_chunk, unsigned int nents\_first\_chunk, gfp\_t gfp\_mask, sg\_alloc\_fn \*alloc\_fn)
:   Allocate and initialize an sg table with given allocator

**Parameters**

`struct sg_table *table`
:   The sg table header to use

`unsigned int nents`
:   Number of entries in sg list

`unsigned int max_ents`
:   The maximum number of entries the allocator returns per call

`struct scatterlist *first_chunk`
:   first SGL if preallocated (may be `NULL`)

`unsigned int nents_first_chunk`
:   Number of entries in the (preallocated) first
    scatterlist chunk, 0 means no such preallocated chunk provided by user

`gfp_t gfp_mask`
:   GFP allocation mask

`sg_alloc_fn *alloc_fn`
:   Allocator to use

**Description**

> This function returns a **table** **nents** long. The allocator is
> defined to return scatterlist chunks of maximum size **max\_ents**.
> Thus if **nents** is bigger than **max\_ents**, the scatterlists will be
> chained in units of **max\_ents**.

**Notes**

> If this function returns non-0 (eg failure), the caller must call
> [`__sg_free_table()`](#c.__sg_free_table "__sg_free_table") to cleanup any leftover allocations.

int sg\_alloc\_table(struct sg\_table \*table, unsigned int nents, gfp\_t gfp\_mask)
:   Allocate and initialize an sg table

**Parameters**

`struct sg_table *table`
:   The sg table header to use

`unsigned int nents`
:   Number of entries in sg list

`gfp_t gfp_mask`
:   GFP allocation mask

**Description**

> Allocate and initialize an sg table. If **nents** is larger than
> SG\_MAX\_SINGLE\_ALLOC a chained sg table will be setup.

int sg\_alloc\_append\_table\_from\_pages(struct sg\_append\_table \*sgt\_append, struct page \*\*pages, unsigned int n\_pages, unsigned int offset, unsigned long size, unsigned int max\_segment, unsigned int left\_pages, gfp\_t gfp\_mask)
:   Allocate and initialize an append sg table from an array of pages

**Parameters**

`struct sg_append_table *sgt_append`
:   The sg append table to use

`struct page **pages`
:   Pointer to an array of page pointers

`unsigned int n_pages`
:   Number of pages in the pages array

`unsigned int offset`
:   Offset from start of the first page to the start of a buffer

`unsigned long size`
:   Number of valid bytes in the buffer (after offset)

`unsigned int max_segment`
:   Maximum size of a scatterlist element in bytes

`unsigned int left_pages`
:   Left pages caller have to set after this call

`gfp_t gfp_mask`
:   GFP allocation mask

**Description**

> In the first call it allocate and initialize an sg table from a list of
> pages, else reuse the scatterlist from sgt\_append. Contiguous ranges of
> the pages are squashed into a single scatterlist entry up to the maximum
> size specified in **max\_segment**. A user may provide an offset at a start
> and a size of valid data in a buffer specified by the page array. The
> returned sg table is released by sg\_free\_append\_table

**Return**

0 on success, negative error on failure

**Notes**

> If this function returns non-0 (eg failure), the caller must call
> [`sg_free_append_table()`](#c.sg_free_append_table "sg_free_append_table") to cleanup any leftover allocations.
>
> In the fist call, sgt\_append must by initialized.

int sg\_alloc\_table\_from\_pages\_segment(struct sg\_table \*sgt, struct page \*\*pages, unsigned int n\_pages, unsigned int offset, unsigned long size, unsigned int max\_segment, gfp\_t gfp\_mask)
:   Allocate and initialize an sg table from an array of pages and given maximum segment.

**Parameters**

`struct sg_table *sgt`
:   The sg table header to use

`struct page **pages`
:   Pointer to an array of page pointers

`unsigned int n_pages`
:   Number of pages in the pages array

`unsigned int offset`
:   Offset from start of the first page to the start of a buffer

`unsigned long size`
:   Number of valid bytes in the buffer (after offset)

`unsigned int max_segment`
:   Maximum size of a scatterlist element in bytes

`gfp_t gfp_mask`
:   GFP allocation mask

**Description**

> Allocate and initialize an sg table from a list of pages. Contiguous
> ranges of the pages are squashed into a single scatterlist node up to the
> maximum size specified in **max\_segment**. A user may provide an offset at a
> start and a size of valid data in a buffer specified by the page array.
>
> The returned sg table is released by sg\_free\_table.

**Return**

0 on success, negative error on failure

struct scatterlist \*sgl\_alloc\_order(unsigned long long length, unsigned int order, bool chainable, gfp\_t gfp, unsigned int \*nent\_p)
:   allocate a scatterlist and its pages

**Parameters**

`unsigned long long length`
:   Length in bytes of the scatterlist. Must be at least one

`unsigned int order`
:   Second argument for [`alloc_pages()`](mm-api.html#c.alloc_pages "alloc_pages")

`bool chainable`
:   Whether or not to allocate an extra element in the scatterlist
    for scatterlist chaining purposes

`gfp_t gfp`
:   Memory allocation flags

`unsigned int *nent_p`
:   [out] Number of entries in the scatterlist that have pages

**Return**

A pointer to an initialized scatterlist or `NULL` upon failure.

struct scatterlist \*sgl\_alloc(unsigned long long length, gfp\_t gfp, unsigned int \*nent\_p)
:   allocate a scatterlist and its pages

**Parameters**

`unsigned long long length`
:   Length in bytes of the scatterlist

`gfp_t gfp`
:   Memory allocation flags

`unsigned int *nent_p`
:   [out] Number of entries in the scatterlist

**Return**

A pointer to an initialized scatterlist or `NULL` upon failure.

void sgl\_free\_n\_order(struct scatterlist \*sgl, int nents, int order)
:   free a scatterlist and its pages

**Parameters**

`struct scatterlist *sgl`
:   Scatterlist with one or more elements

`int nents`
:   Maximum number of elements to free

`int order`
:   Second argument for [`__free_pages()`](mm-api.html#c.__free_pages "__free_pages")

**Notes**

* If several scatterlists have been chained and each chain element is
  freed separately then it’s essential to set nents correctly to avoid that a
  page would get freed twice.
* All pages in a chained scatterlist can be freed at once by setting **nents**
  to a high number.

void sgl\_free\_order(struct scatterlist \*sgl, int order)
:   free a scatterlist and its pages

**Parameters**

`struct scatterlist *sgl`
:   Scatterlist with one or more elements

`int order`
:   Second argument for [`__free_pages()`](mm-api.html#c.__free_pages "__free_pages")

void sgl\_free(struct scatterlist \*sgl)
:   free a scatterlist and its pages

**Parameters**

`struct scatterlist *sgl`
:   Scatterlist with one or more elements

void sg\_miter\_start(struct sg\_mapping\_iter \*miter, struct scatterlist \*sgl, unsigned int nents, unsigned int flags)
:   start mapping iteration over a sg list

**Parameters**

`struct sg_mapping_iter *miter`
:   sg mapping iter to be started

`struct scatterlist *sgl`
:   sg list to iterate over

`unsigned int nents`
:   number of sg entries

`unsigned int flags`
:   sg iterator flags

**Description**

> Starts mapping iterator **miter**.

**Context**

Don’t care.

bool sg\_miter\_skip(struct sg\_mapping\_iter \*miter, off\_t offset)
:   reposition mapping iterator

**Parameters**

`struct sg_mapping_iter *miter`
:   sg mapping iter to be skipped

`off_t offset`
:   number of bytes to plus the current location

**Description**

> Sets the offset of **miter** to its current location plus **offset** bytes.
> If mapping iterator **miter** has been proceeded by [`sg_miter_next()`](#c.sg_miter_next "sg_miter_next"), this
> stops **miter**.

**Context**

Don’t care.

**Return**

true if **miter** contains the valid mapping. false if end of sg
list is reached.

bool sg\_miter\_next(struct sg\_mapping\_iter \*miter)
:   proceed mapping iterator to the next mapping

**Parameters**

`struct sg_mapping_iter *miter`
:   sg mapping iter to proceed

**Description**

> Proceeds **miter** to the next mapping. **miter** should have been started
> using [`sg_miter_start()`](#c.sg_miter_start "sg_miter_start"). On successful return, **miter->page**,
> **miter->addr** and **miter->length** point to the current mapping.

**Context**

May sleep if !SG\_MITER\_ATOMIC && !SG\_MITER\_LOCAL.

**Return**

true if **miter** contains the next mapping. false if end of sg
list is reached.

void sg\_miter\_stop(struct sg\_mapping\_iter \*miter)
:   stop mapping iteration

**Parameters**

`struct sg_mapping_iter *miter`
:   sg mapping iter to be stopped

**Description**

> Stops mapping iterator **miter**. **miter** should have been started
> using [`sg_miter_start()`](#c.sg_miter_start "sg_miter_start"). A stopped iteration can be resumed by
> calling [`sg_miter_next()`](#c.sg_miter_next "sg_miter_next") on it. This is useful when resources (kmap)
> need to be released during iteration.

**Context**

Don’t care otherwise.

size\_t sg\_copy\_buffer(struct scatterlist \*sgl, unsigned int nents, void \*buf, size\_t buflen, off\_t skip, bool to\_buffer)
:   Copy data between a linear buffer and an SG list

**Parameters**

`struct scatterlist *sgl`
:   The SG list

`unsigned int nents`
:   Number of SG entries

`void *buf`
:   Where to copy from

`size_t buflen`
:   The number of bytes to copy

`off_t skip`
:   Number of bytes to skip before copying

`bool to_buffer`
:   transfer direction (true == from an sg list to a
    buffer, false == from a buffer to an sg list)

**Description**

Returns the number of copied bytes.

size\_t sg\_copy\_from\_buffer(struct scatterlist \*sgl, unsigned int nents, const void \*buf, size\_t buflen)
:   Copy from a linear buffer to an SG list

**Parameters**

`struct scatterlist *sgl`
:   The SG list

`unsigned int nents`
:   Number of SG entries

`const void *buf`
:   Where to copy from

`size_t buflen`
:   The number of bytes to copy

**Description**

Returns the number of copied bytes.

size\_t sg\_copy\_to\_buffer(struct scatterlist \*sgl, unsigned int nents, void \*buf, size\_t buflen)
:   Copy from an SG list to a linear buffer

**Parameters**

`struct scatterlist *sgl`
:   The SG list

`unsigned int nents`
:   Number of SG entries

`void *buf`
:   Where to copy to

`size_t buflen`
:   The number of bytes to copy

**Description**

Returns the number of copied bytes.

size\_t sg\_pcopy\_from\_buffer(struct scatterlist \*sgl, unsigned int nents, const void \*buf, size\_t buflen, off\_t skip)
:   Copy from a linear buffer to an SG list

**Parameters**

`struct scatterlist *sgl`
:   The SG list

`unsigned int nents`
:   Number of SG entries

`const void *buf`
:   Where to copy from

`size_t buflen`
:   The number of bytes to copy

`off_t skip`
:   Number of bytes to skip before copying

**Description**

Returns the number of copied bytes.

size\_t sg\_pcopy\_to\_buffer(struct scatterlist \*sgl, unsigned int nents, void \*buf, size\_t buflen, off\_t skip)
:   Copy from an SG list to a linear buffer

**Parameters**

`struct scatterlist *sgl`
:   The SG list

`unsigned int nents`
:   Number of SG entries

`void *buf`
:   Where to copy to

`size_t buflen`
:   The number of bytes to copy

`off_t skip`
:   Number of bytes to skip before copying

**Description**

Returns the number of copied bytes.

size\_t sg\_zero\_buffer(struct scatterlist \*sgl, unsigned int nents, size\_t buflen, off\_t skip)
:   Zero-out a part of a SG list

**Parameters**

`struct scatterlist *sgl`
:   The SG list

`unsigned int nents`
:   Number of SG entries

`size_t buflen`
:   The number of bytes to zero out

`off_t skip`
:   Number of bytes to skip before zeroing

**Description**

Returns the number of bytes zeroed.

ssize\_t extract\_iter\_to\_sg(struct iov\_iter \*iter, size\_t maxsize, struct sg\_table \*sgtable, unsigned int sg\_max, iov\_iter\_extraction\_t extraction\_flags)
:   Extract pages from an iterator and add to an sglist

**Parameters**

`struct iov_iter *iter`
:   The iterator to extract from

`size_t maxsize`
:   The amount of iterator to copy

`struct sg_table *sgtable`
:   The scatterlist table to fill in

`unsigned int sg_max`
:   Maximum number of elements in **sgtable** that may be filled

`iov_iter_extraction_t extraction_flags`
:   Flags to qualify the request

**Description**

Extract the page fragments from the given amount of the source iterator and
add them to a scatterlist that refers to all of those bits, to a maximum
addition of **sg\_max** elements.

The pages referred to by UBUF- and IOVEC-type iterators are extracted and
pinned; BVEC-, KVEC-, FOLIOQ- and XARRAY-type are extracted but aren’t
pinned; DISCARD-type is not supported.

No end mark is placed on the scatterlist; that’s left to the caller.

**extraction\_flags** can have ITER\_ALLOW\_P2PDMA set to request peer-to-peer DMA
be allowed on the pages extracted.

If successful, **sgtable->nents** is updated to include the number of elements
added and the number of bytes added is returned. **sgtable->orig\_nents** is
left unaltered.

The `iov_iter_extract_mode()` function should be used to query how cleanup
should be performed.
