# Kexec Handover Subsystem

> 출처(원문): https://docs.kernel.org/core-api/kho/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kexec Handover Subsystem

## Overview

Kexec HandOver (KHO) is a mechanism that allows Linux to preserve memory
regions, which could contain serialized system states, across kexec.

KHO uses [flattened device tree (FDT)](#kho-fdt) to pass information about
the preserved state from pre-exec kernel to post-kexec kernel and [scratch
memory regions](#kho-scratch) to ensure integrity of the preserved memory.

## KHO FDT

Every KHO kexec carries a KHO specific flattened device tree (FDT) blob that
describes the preserved state. The FDT includes properties describing preserved
memory regions and nodes that hold subsystem specific state.

The preserved memory regions contain either serialized subsystem states, or
in-memory data that shall not be touched across kexec. After KHO, subsystems
can retrieve and restore the preserved state from KHO FDT.

Subsystems participating in KHO can define their own format for state
serialization and preservation.

KHO FDT and structures defined by the subsystems form an ABI between pre-kexec
and post-kexec kernels. This ABI is defined by header files in
`include/linux/kho/abi` directory.

* [Kexec Handover ABI](abi.html)

## Scratch Regions

To boot into kexec, we need to have a physically contiguous memory range that
contains no handed over memory. Kexec then places the target kernel and initrd
into that region. The new kernel exclusively uses this region for memory
allocations before during boot up to the initialization of the page allocator.

We guarantee that we always have such regions through the scratch regions: On
first boot KHO allocates several physically contiguous memory regions. Since
after kexec these regions will be used by early memory allocations, there is a
scratch region per NUMA node plus a scratch region to satisfy allocations
requests that do not require particular NUMA node assignment.
By default, size of the scratch region is calculated based on amount of memory
allocated during boot. The `kho_scratch` kernel command line option may be
used to explicitly define size of the scratch regions.
The scratch regions are declared as CMA when page allocator is initialized so
that their memory can be used during system lifetime. CMA gives us the
guarantee that no handover pages land in that region, because handover pages
must be at a static physical memory location and CMA enforces that only
movable pages can be located inside.

After KHO kexec, we ignore the `kho_scratch` kernel command line option and
instead reuse the exact same region that was originally allocated. This allows
us to recursively execute any amount of KHO kexecs. Because we used this region
for boot memory allocations and as target memory for kexec blobs, some parts
of that memory region may be reserved. These reservations are irrelevant for
the next KHO, because kexec can overwrite even the original kernel.

## Kexec Handover Radix Tree

This is a radix tree implementation for tracking physical memory pages
across kexec transitions. It was developed for the KHO mechanism but is
designed for broader use by any subsystem that needs to preserve pages.

The radix tree is a multi-level tree where leaf nodes are bitmaps
representing individual pages. To allow pages of different sizes (orders)
to be stored efficiently in a single tree, it uses a unique key encoding
scheme. Each key is an unsigned long that combines a page’s physical
address and its order.

Client code is responsible for allocating the root node of the tree,
initializing the mutex lock, and managing its lifecycle. It must use the
tree data structures defined in the KHO ABI,
include/linux/kho/abi/kexec\_handover.h.

## Public API

int kho\_radix\_add\_page(struct kho\_radix\_tree \*tree, unsigned long pfn, unsigned int order)
:   Marks a page as preserved in the radix tree.

**Parameters**

`struct kho_radix_tree *tree`
:   The KHO radix tree.

`unsigned long pfn`
:   The page frame number of the page to preserve.

`unsigned int order`
:   The order of the page.

**Description**

This function traverses the radix tree based on the key derived from **pfn**
and **order**. It sets the corresponding bit in the leaf bitmap to mark the
page for preservation. If intermediate nodes do not exist along the path,
they are allocated and added to the tree.

**Return**

0 on success, or a negative error code on failure.

void kho\_radix\_del\_page(struct kho\_radix\_tree \*tree, unsigned long pfn, unsigned int order)
:   Removes a page’s preservation status from the radix tree.

**Parameters**

`struct kho_radix_tree *tree`
:   The KHO radix tree.

`unsigned long pfn`
:   The page frame number of the page to unpreserve.

`unsigned int order`
:   The order of the page.

**Description**

This function traverses the radix tree and clears the bit corresponding to
the page, effectively removing its “preserved” status. It does not free
the tree’s intermediate nodes, even if they become empty.

int kho\_radix\_walk\_tree(struct kho\_radix\_tree \*tree, kho\_radix\_tree\_walk\_callback\_t cb)
:   Traverses the radix tree and calls a callback for each preserved page.

**Parameters**

`struct kho_radix_tree *tree`
:   A pointer to the KHO radix tree to walk.

`kho_radix_tree_walk_callback_t cb`
:   A callback function of type kho\_radix\_tree\_walk\_callback\_t that will be
    invoked for each preserved page found in the tree. The callback receives
    the physical address and order of the preserved page.

**Description**

This function walks the radix tree, searching from the specified top level
down to the lowest level (level 0). For each preserved page found, it invokes
the provided callback, passing the page’s physical address and order.

**Return**

0 if the walk completed the specified tree, or the non-zero return
value from the callback that stopped the walk.

struct [folio](../mm-api.html#c.folio "folio") \*kho\_restore\_folio(phys\_addr\_t phys)
:   recreates the folio from the preserved memory.

**Parameters**

`phys_addr_t phys`
:   physical address of the folio.

**Return**

pointer to the [`struct folio`](../mm-api.html#c.folio "folio") on success, NULL on failure.

struct page \*kho\_restore\_pages(phys\_addr\_t phys, unsigned long nr\_pages)
:   restore list of contiguous order 0 pages.

**Parameters**

`phys_addr_t phys`
:   physical address of the first page.

`unsigned long nr_pages`
:   number of pages.

**Description**

Restore a contiguous list of order 0 pages that was preserved with
[`kho_preserve_pages()`](#c.kho_preserve_pages "kho_preserve_pages").

**Return**

the first page on success, NULL on failure.

int kho\_add\_subtree(const char \*name, void \*blob, size\_t size)
:   record the physical address of a sub blob in KHO root tree.

**Parameters**

`const char *name`
:   name of the sub tree.

`void *blob`
:   the sub tree blob.

`size_t size`
:   size of the blob in bytes.

**Description**

Creates a new child node named **name** in KHO root FDT and records
the physical address of **blob**. The pages of **blob** must also be preserved
by KHO for the new kernel to retrieve it after kexec.

A debugfs blob entry is also created at
`/sys/kernel/debug/kho/out/sub_fdts/**name**` when kernel is configured with
CONFIG\_KEXEC\_HANDOVER\_DEBUGFS

**Return**

0 on success, error code on failure

int kho\_preserve\_folio(struct [folio](#c.kho_preserve_folio "folio") \*folio)
:   preserve a folio across kexec.

**Parameters**

`struct folio *folio`
:   folio to preserve.

**Description**

Instructs KHO to preserve the whole folio across kexec. The order
will be preserved as well.

**Return**

0 on success, error code on failure

void kho\_unpreserve\_folio(struct [folio](#c.kho_unpreserve_folio "folio") \*folio)
:   unpreserve a folio.

**Parameters**

`struct folio *folio`
:   folio to unpreserve.

**Description**

Instructs KHO to unpreserve a folio that was preserved by
[`kho_preserve_folio()`](#c.kho_preserve_folio "kho_preserve_folio") before. The provided **folio** (pfn and order)
must exactly match a previously preserved folio.

int kho\_preserve\_pages(struct [page](#c.kho_preserve_pages "page") \*page, unsigned long nr\_pages)
:   preserve contiguous pages across kexec

**Parameters**

`struct page *page`
:   first page in the list.

`unsigned long nr_pages`
:   number of pages.

**Description**

Preserve a contiguous list of order 0 pages. Must be restored using
[`kho_restore_pages()`](#c.kho_restore_pages "kho_restore_pages") to ensure the pages are restored properly as order 0.

**Return**

0 on success, error code on failure

void kho\_unpreserve\_pages(struct [page](#c.kho_unpreserve_pages "page") \*page, unsigned long nr\_pages)
:   unpreserve contiguous pages.

**Parameters**

`struct page *page`
:   first page in the list.

`unsigned long nr_pages`
:   number of pages.

**Description**

Instructs KHO to unpreserve **nr\_pages** contiguous pages starting from **page**.
This must be called with the same **page** and **nr\_pages** as the corresponding
[`kho_preserve_pages()`](#c.kho_preserve_pages "kho_preserve_pages") call. Unpreserving arbitrary sub-ranges of larger
preserved blocks is not supported.

int kho\_preserve\_vmalloc(void \*ptr, struct kho\_vmalloc \*preservation)
:   preserve memory allocated with [`vmalloc()`](../mm-api.html#c.vmalloc "vmalloc") across kexec

**Parameters**

`void *ptr`
:   pointer to the area in vmalloc address space

`struct kho_vmalloc *preservation`
:   placeholder for preservation metadata

**Description**

Instructs KHO to preserve the area in vmalloc address space at **ptr**. The
physical pages mapped at **ptr** will be preserved and on successful return
**preservation** will hold the physical address of a structure that describes
the preservation.

**NOTE**

The memory allocated with [`vmalloc_node()`](../mm-api.html#c.vmalloc_node "vmalloc_node") variants cannot be reliably
restored on the same node

**Return**

0 on success, error code on failure

void kho\_unpreserve\_vmalloc(struct kho\_vmalloc \*preservation)
:   unpreserve memory allocated with [`vmalloc()`](../mm-api.html#c.vmalloc "vmalloc")

**Parameters**

`struct kho_vmalloc *preservation`
:   preservation metadata returned by [`kho_preserve_vmalloc()`](#c.kho_preserve_vmalloc "kho_preserve_vmalloc")

**Description**

Instructs KHO to unpreserve the area in vmalloc address space that was
previously preserved with [`kho_preserve_vmalloc()`](#c.kho_preserve_vmalloc "kho_preserve_vmalloc").

void \*kho\_restore\_vmalloc(const struct kho\_vmalloc \*preservation)
:   recreates and populates an area in vmalloc address space from the preserved memory.

**Parameters**

`const struct kho_vmalloc *preservation`
:   preservation metadata.

**Description**

Recreates an area in vmalloc address space and populates it with memory that
was preserved using [`kho_preserve_vmalloc()`](#c.kho_preserve_vmalloc "kho_preserve_vmalloc").

**Return**

pointer to the area in the vmalloc address space, NULL on failure.

void \*kho\_alloc\_preserve(size\_t size)
:   Allocate, zero, and preserve memory.

**Parameters**

`size_t size`
:   The number of bytes to allocate.

**Description**

Allocates a physically contiguous block of zeroed pages that is large
enough to hold **size** bytes. The allocated memory is then registered with
KHO for preservation across a kexec.

**Note**

The actual allocated size will be rounded up to the nearest
power-of-two page boundary.

**return** A virtual pointer to the allocated and preserved memory on success,
or an [`ERR_PTR()`](../kernel-api.html#c.ERR_PTR "ERR_PTR") encoded error on failure.

void kho\_unpreserve\_free(void \*mem)
:   Unpreserve and free memory.

**Parameters**

`void *mem`
:   Pointer to the memory allocated by [`kho_alloc_preserve()`](#c.kho_alloc_preserve "kho_alloc_preserve").

**Description**

Unregisters the memory from KHO preservation and frees the underlying
pages back to the system. This function should be called to clean up
memory allocated with [`kho_alloc_preserve()`](#c.kho_alloc_preserve "kho_alloc_preserve").

void kho\_restore\_free(void \*mem)
:   Restore and free memory after kexec.

**Parameters**

`void *mem`
:   Pointer to the memory (in the new kernel’s address space)
    that was allocated by the old kernel.

**Description**

This function is intended to be called in the new kernel (post-kexec)
to take ownership of and free a memory region that was preserved by the
old kernel using [`kho_alloc_preserve()`](#c.kho_alloc_preserve "kho_alloc_preserve").

It first restores the pages from KHO (using their physical address)
and then frees the pages back to the new kernel’s page allocator.

bool is\_kho\_boot(void)
:   check if current kernel was booted via KHO-enabled kexec

**Parameters**

`void`
:   no arguments

**Description**

This function checks if the current kernel was loaded through a kexec
operation with KHO enabled, by verifying that a valid KHO FDT
was passed.

**Note**

This function returns reliable results only after
`kho_populate()` has been called during early boot. Before that,
it may return false even if KHO data is present.

**Return**

true if booted via KHO-enabled kexec, false otherwise

int kho\_retrieve\_subtree(const char \*name, phys\_addr\_t \*phys, size\_t \*size)
:   retrieve a preserved sub blob by its name.

**Parameters**

`const char *name`
:   the name of the sub blob passed to [`kho_add_subtree()`](#c.kho_add_subtree "kho_add_subtree").

`phys_addr_t *phys`
:   if found, the physical address of the sub blob is stored in **phys**.

`size_t *size`
:   if not NULL and found, the size of the sub blob is stored in **size**.

**Description**

Retrieve a preserved sub blob named **name** and store its physical
address in **phys** and optionally its size in **size**.

**Return**

0 on success, error code on failure

## See Also

* [Kexec Handover Usage](../../admin-guide/mm/kho.html)
