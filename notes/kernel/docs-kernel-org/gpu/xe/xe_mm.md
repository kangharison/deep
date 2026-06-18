# Memory Management

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_mm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Memory Management

## BO management

TTM manages (placement, eviction, etc...) all BOs in Xe.

## BO creation

Create a chunk of memory which can be used by the GPU. Placement rules
(sysmem or vram region) passed in upon creation. TTM handles placement of BO
and can trigger eviction of other BOs to make space for the new BO.

### Kernel BOs

A kernel BO is created as part of driver load (e.g. uC firmware images, GuC
ADS, etc...) or a BO created as part of a user operation which requires
a kernel BO (e.g. engine state, memory for page tables, etc...). These BOs
are typically mapped in the GGTT (any kernel BOs aside memory for page tables
are in the GGTT), are pinned (can’t move or be evicted at runtime), have a
vmap (Xe can access the memory via xe\_map layer) and have contiguous physical
memory.

More details of why kernel BOs are pinned and contiguous below.

### User BOs

A user BO is created via the DRM\_IOCTL\_XE\_GEM\_CREATE IOCTL. Once it is
created the BO can be mmap’d (via DRM\_IOCTL\_XE\_GEM\_MMAP\_OFFSET) for user
access and it can be bound for GPU access (via DRM\_IOCTL\_XE\_VM\_BIND). All
user BOs are evictable and user BOs are never pinned by Xe. The allocation of
the backing store can be deferred from creation time until first use which is
either mmap, bind, or pagefault.

#### Private BOs

A private BO is a user BO created with a valid VM argument passed into the
create IOCTL. If a BO is private it cannot be exported via prime FD and
mappings can only be created for the BO within the VM it is tied to. Lastly,
the BO dma-resv slots / lock point to the VM’s dma-resv slots / lock (all
private BOs to a VM share common dma-resv slots / lock).

#### External BOs

An external BO is a user BO created with a NULL VM argument passed into the
create IOCTL. An external BO can be shared with different UMDs / devices via
prime FD and the BO can be mapped into multiple VMs. An external BO has its
own unique dma-resv slots / lock. An external BO will be in an array of all
VMs which has a mapping of the BO. This allows VMs to lookup and lock all
external BOs mapped in the VM as needed.

#### BO placement

When a user BO is created, a mask of valid placements is passed indicating
which memory regions are considered valid.

The memory region information is available via query uAPI (TODO: add link).

## BO validation

BO validation (ttm\_bo\_validate) refers to ensuring a BO has a valid
placement. If a BO was swapped to temporary storage, a validation call will
trigger a move back to a valid (location where GPU can access BO) placement.
Validation of a BO may evict other BOs to make room for the BO being
validated.

## BO eviction / moving

All eviction (or in other words, moving a BO from one memory location to
another) is routed through TTM with a callback into Xe.

### Runtime eviction

Runtime evictions refers to during normal operations where TTM decides it
needs to move a BO. Typically this is because TTM needs to make room for
another BO and the evicted BO is first BO on LRU list that is not locked.

An example of this is a new BO which can only be placed in VRAM but there is
not space in VRAM. There could be multiple BOs which have sysmem and VRAM
placement rules which currently reside in VRAM, TTM trigger a will move of
one (or multiple) of these BO(s) until there is room in VRAM to place the new
BO. The evicted BO(s) are valid but still need new bindings before the BO
used again (exec or compute mode rebind worker).

Another example would be, TTM can’t find a BO to evict which has another
valid placement. In this case TTM will evict one (or multiple) unlocked BO(s)
to a temporary unreachable (invalid) placement. The evicted BO(s) are invalid
and before next use need to be moved to a valid placement and rebound.

In both cases, moves of these BOs are scheduled behind the fences in the BO’s
dma-resv slots.

WW locking tries to ensures if 2 VMs use 51% of the memory forward progress
is made on both VMs.

Runtime eviction uses per a GT migration engine (TODO: link to migration
engine doc) to do a GPU memcpy from one location to another.

### Rebinds after runtime eviction

When BOs are moved, every mapping (VMA) of the BO needs to rebound before
the BO is used again. Every VMA is added to an evicted list of its VM when
the BO is moved. This is safe because of the VM locking structure (TODO: link
to VM locking doc). On the next use of a VM (exec or compute mode rebind
worker) the evicted VMA list is checked and rebinds are triggered. In the
case of faulting VM, the rebind is done in the page fault handler.

### Suspend / resume eviction of VRAM

During device suspend / resume VRAM may lose power which means the contents
of VRAM’s memory is blown away. Thus BOs present in VRAM at the time of
suspend must be moved to sysmem in order for their contents to be saved.

A simple TTM call (ttm\_resource\_manager\_evict\_all) can move all non-pinned
(user) BOs to sysmem. External BOs that are pinned need to be manually
evicted with a simple loop + xe\_bo\_evict call. It gets a little trickier
with kernel BOs.

Some kernel BOs are used by the GT migration engine to do moves, thus we
can’t move all of the BOs via the GT migration engine. For simplity, use a
TTM memcpy (CPU) to move any kernel (pinned) BO on either suspend or resume.

Some kernel BOs need to be restored to the exact same physical location. TTM
makes this rather easy but the caveat is the memory must be contiguous. Again
for simplity, we enforce that all kernel (pinned) BOs are contiguous and
restored to the same physical location.

Pinned external BOs in VRAM are restored on resume via the GPU.

### Rebinds after suspend / resume

Most kernel BOs have GGTT mappings which must be restored during the resume
process. All user BOs are rebound after validation on their next use.

## Future work

Trim the list of BOs which is saved / restored via TTM memcpy on suspend /
resume. All we really need to save / restore via TTM memcpy is the memory
required for the GuC to load and the memory for the GT migrate engine to
operate.

Do not require kernel BOs to be contiguous in physical memory / restored to
the same physical address on resume. In all likelihood the only memory that
needs to be restored to the same physical address is memory used for page
tables. All of that memory is allocated 1 page at time so the contiguous
requirement isn’t needed. Some work on the vmap code would need to be done if
kernel BOs are not contiguous too.

Make some kernel BO evictable rather than pinned. An example of this would be
engine state, in all likelihood if the dma-slots of these BOs where properly
used rather than pinning we could safely evict + rebind these BOs as needed.

Some kernel BOs do not need to be restored on resume (e.g. GuC ADS as that is
repopulated on resume), add flag to mark such objects as no save / restore.

## GGTT

Xe GGTT implements the support for a Global Virtual Address space that is used
for resources that are accessible to privileged (i.e. kernel-mode) processes,
and not tied to a specific user-level process. For example, the Graphics
micro-Controller (GuC) and Display Engine (if present) utilize this Global
address space.

The Global GTT (GGTT) translates from the Global virtual address to a physical
address that can be accessed by HW. The GGTT is a flat, single-level table.

Xe implements a simplified version of the GGTT specifically managing only a
certain range of it that goes from the Write Once Protected Content Memory (WOPCM)
Layout to a predefined GUC\_GGTT\_TOP. This approach avoids complications related to
the GuC (Graphics Microcontroller) hardware limitations. The GuC address space
is limited on both ends of the GGTT, because the GuC shim HW redirects
accesses to those addresses to other HW areas instead of going through the
GGTT. On the bottom end, the GuC can’t access offsets below the WOPCM size,
while on the top side the limit is fixed at GUC\_GGTT\_TOP. To keep things
simple, instead of checking each object to see if they are accessed by GuC or
not, we just exclude those areas from the allocator. Additionally, to simplify
the driver load, we use the maximum WOPCM size in this logic instead of the
programmed one, so we don’t need to wait until the actual size to be
programmed is determined (which requires FW fetch) before initializing the
GGTT. These simplifications might waste space in the GGTT (about 20-25 MBs
depending on the platform) but we can live with this. Another benefit of this
is the GuC bootrom can’t access anything below the WOPCM max size so anything
the bootrom needs to access (e.g. a RSA key) needs to be placed in the GGTT
above the WOPCM max size. Starting the GGTT allocations above the WOPCM max
give us the correct placement for free.

### GGTT Internal API

struct xe\_ggtt\_node
:   A node in GGTT.

**Definition**:

```
struct xe_ggtt_node {
    struct xe_ggtt *ggtt;
    struct drm_mm_node base;
    struct work_struct delayed_removal_work;
    bool invalidate_on_remove;
};
```

**Members**

`ggtt`
:   Back pointer to xe\_ggtt where this region will be inserted at

`base`
:   A drm\_mm\_node

`delayed_removal_work`
:   The work struct for the delayed removal

`invalidate_on_remove`
:   If it needs invalidation upon removal

**Description**

This `struct is` allocated with xe\_ggtt\_insert\_node(,\_transform) or xe\_ggtt\_insert\_bo(,\_at).
It will be deallocated using [`xe_ggtt_node_remove()`](#c.xe_ggtt_node_remove "xe_ggtt_node_remove").

struct xe\_ggtt\_pt\_ops
:   GGTT Page table operations Which can vary from platform to platform.

**Definition**:

```
struct xe_ggtt_pt_ops {
    u64 (*pte_encode_flags)(struct xe_bo *bo, u16 pat_index);
    xe_ggtt_set_pte_fn ggtt_set_pte;
    u64 (*ggtt_get_pte)(struct xe_ggtt *ggtt, u64 addr);
};
```

**Members**

`pte_encode_flags`
:   Encode PTE flags for a given BO

`ggtt_set_pte`
:   Directly write into GGTT’s PTE

`ggtt_get_pte`
:   Directly read from GGTT’s PTE

struct xe\_ggtt
:   Main GGTT struct

**Definition**:

```
struct xe_ggtt {
    struct xe_tile *tile;
    u64 start;
    u64 size;
#define XE_GGTT_FLAGS_64K BIT(0);
    unsigned int flags;
    struct xe_bo *scratch;
    struct mutex lock;
    u64 __iomem *gsm;
    const struct xe_ggtt_pt_ops *pt_ops;
    struct drm_mm mm;
    unsigned int access_count;
    struct workqueue_struct *wq;
};
```

**Members**

`tile`
:   Back pointer to tile where this GGTT belongs

`start`
:   Start offset of GGTT

`size`
:   Total usable size of this GGTT

`flags`
:   Flags for this GGTT
    Acceptable flags:
    - `XE_GGTT_FLAGS_64K` - if PTE size is 64K. Otherwise, regular is 4K.
    - `XE_GGTT_FLAGS_ONLINE` - is GGTT online, protected by ggtt->lock

    > after init

`scratch`
:   Internal object allocation used as a scratch page

`lock`
:   Mutex lock to protect GGTT data

`gsm`
:   The iomem pointer to the actual location of the translation
    table located in the GSM for easy PTE manipulation

`pt_ops`
:   Page Table operations per platform

`mm`
:   The memory manager used to manage individual GGTT allocations

`access_count`
:   counts GGTT writes

`wq`
:   Dedicated unordered work queue to process node removals

**Description**

In general, each tile can contains its own Global Graphics Translation Table
(GGTT) instance.

u64 xe\_ggtt\_start(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt)
:   Get starting offset of GGTT.

**Parameters**

`struct xe_ggtt *ggtt`
:   [`xe_ggtt`](#c.xe_ggtt "xe_ggtt")

**Return**

Starting offset for this [`xe_ggtt`](#c.xe_ggtt "xe_ggtt").

u64 xe\_ggtt\_size(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt)
:   Get size of GGTT.

**Parameters**

`struct xe_ggtt *ggtt`
:   [`xe_ggtt`](#c.xe_ggtt "xe_ggtt")

**Return**

Total usable size of this [`xe_ggtt`](#c.xe_ggtt "xe_ggtt").

struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*xe\_ggtt\_alloc(struct xe\_tile \*tile)
:   Allocate a GGTT for a given `xe_tile`

**Parameters**

`struct xe_tile *tile`
:   `xe_tile`

**Description**

Allocates a [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") for a given tile.

**Return**

[`xe_ggtt`](#c.xe_ggtt "xe_ggtt") on success, or NULL when out of memory.

int xe\_ggtt\_init\_early(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt)
:   Early GGTT initialization

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") to be initialized

**Description**

It allows to create new mappings usable by the GuC.
Mappings are not usable by the HW engines, as it doesn’t have scratch nor
initial clear done to it yet. That will happen in the regular, non-early
GGTT initialization.

**Return**

0 on success or a negative error code on failure.

void xe\_ggtt\_node\_remove(struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node, bool invalidate)
:   Remove a [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") from the GGTT

**Parameters**

`struct xe_ggtt_node *node`
:   the [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") to be removed

`bool invalidate`
:   if node needs invalidation upon removal

int xe\_ggtt\_init(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt)
:   Regular non-early GGTT initialization

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") to be initialized

**Return**

0 on success or a negative error code on failure.

void xe\_ggtt\_shift\_nodes(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, u64 new\_start)
:   Shift GGTT nodes to adjust for a change in usable address range.

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") `struct instance`

`u64 new_start`
:   new location of area provisioned for current VF

**Description**

Ensure that all struct [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") are moved to the **new\_start** base address
by changing the base offset of the GGTT.

This function may be called multiple times during recovery, but if
**new\_start** is unchanged from the current base, it’s a noop.

**new\_start** should be a value between `xe_wopcm_size()` and #GUC\_GGTT\_TOP.

struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*xe\_ggtt\_insert\_node(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, u32 size, u32 align)
:   Insert a [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") into the GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") into which the node should be inserted.

`u32 size`
:   size of the node

`u32 align`
:   alignment constrain of the node

**Return**

[`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") on success or a ERR\_PTR on failure.

size\_t xe\_ggtt\_node\_pt\_size(const struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node)
:   Get the size of page table entries needed to map a GGTT node.

**Parameters**

`const struct xe_ggtt_node *node`
:   the [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node")

**Return**

GGTT node page table entries size in bytes.

void xe\_ggtt\_map\_bo(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node, struct xe\_bo \*bo, u64 pte)
:   Map the BO into GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") where node will be mapped

`struct xe_ggtt_node *node`
:   the [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") where this BO is mapped

`struct xe_bo *bo`
:   the `xe_bo` to be mapped

`u64 pte`
:   The pte flags to append.

void xe\_ggtt\_map\_bo\_unlocked(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct xe\_bo \*bo)
:   Restore a mapping of a BO into GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") where node will be mapped

`struct xe_bo *bo`
:   the `xe_bo` to be mapped

**Description**

This is used to restore a GGTT mapping after suspend.

struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*xe\_ggtt\_insert\_node\_transform(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct xe\_bo \*bo, u64 pte\_flags, u64 size, u32 align, xe\_ggtt\_transform\_cb transform, void \*arg)
:   Insert a newly allocated [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") into the GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") where the node will inserted/reserved.

`struct xe_bo *bo`
:   The bo to be transformed

`u64 pte_flags`
:   The extra GGTT flags to add to mapping.

`u64 size`
:   size of the node

`u32 align`
:   required alignment for node

`xe_ggtt_transform_cb transform`
:   transformation function that will populate the GGTT node, or NULL for linear mapping.

`void *arg`
:   Extra argument to pass to the transformation function.

**Description**

This function allows inserting a GGTT node with a custom transformation function.
This is useful for display to allow inserting rotated framebuffers to GGTT.

**Return**

A pointer to `xe_ggtt_node` `struct on` success. An ERR\_PTR otherwise.

int xe\_ggtt\_insert\_bo\_at(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct xe\_bo \*bo, u64 start, u64 end, struct [drm\_exec](../drm-mm.html#c.drm_exec "drm_exec") \*exec)
:   Insert BO at a specific GGTT space

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") where bo will be inserted

`struct xe_bo *bo`
:   the `xe_bo` to be inserted

`u64 start`
:   address where it will be inserted

`u64 end`
:   end of the range where it will be inserted

`struct drm_exec *exec`
:   The drm\_exec transaction to use for exhaustive eviction.

**Return**

0 on success or a negative error code on failure.

int xe\_ggtt\_insert\_bo(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct xe\_bo \*bo, struct [drm\_exec](../drm-mm.html#c.drm_exec "drm_exec") \*exec)
:   Insert BO into GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") where bo will be inserted

`struct xe_bo *bo`
:   the `xe_bo` to be inserted

`struct drm_exec *exec`
:   The drm\_exec transaction to use for exhaustive eviction.

**Return**

0 on success or a negative error code on failure.

void xe\_ggtt\_remove\_bo(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct xe\_bo \*bo)
:   Remove a BO from the GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") where node will be removed

`struct xe_bo *bo`
:   the `xe_bo` to be removed

u64 xe\_ggtt\_largest\_hole(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, u64 alignment, u64 \*spare)
:   Largest GGTT hole

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") that will be inspected

`u64 alignment`
:   minimum alignment

`u64 *spare`
:   If not NULL: in: desired memory size to be spared / out: Adjusted possible spare

**Return**

size of the largest continuous GGTT region

void xe\_ggtt\_assign(const struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node, u16 vfid)
:   assign a GGTT region to the VF

**Parameters**

`const struct xe_ggtt_node *node`
:   the [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") to update

`u16 vfid`
:   the VF identifier

**Description**

This function is used by the PF driver to assign a GGTT region to the VF.
In addition to PTE’s VFID bits 11:2 also PRESENT bit 0 is set as on some
platforms VFs can’t modify that either.

int xe\_ggtt\_node\_save(struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node, void \*dst, size\_t size, u16 vfid)
:   Save a [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") to a buffer.

**Parameters**

`struct xe_ggtt_node *node`
:   the [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") to be saved

`void *dst`
:   destination buffer

`size_t size`
:   destination buffer size in bytes

`u16 vfid`
:   VF identifier

**Return**

0 on success or a negative error code on failure.

int xe\_ggtt\_node\_load(struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node, const void \*src, size\_t size, u16 vfid)
:   Load a [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") from a buffer.

**Parameters**

`struct xe_ggtt_node *node`
:   the [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node") to be loaded

`const void *src`
:   source buffer

`size_t size`
:   source buffer size in bytes

`u16 vfid`
:   VF identifier

**Return**

0 on success or a negative error code on failure.

int xe\_ggtt\_dump(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct [drm\_printer](../drm-internals.html#c.drm_printer "drm_printer") \*p)
:   Dump GGTT for debug

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") to be dumped

`struct drm_printer *p`
:   the `drm_mm_printer` helper handle to be used to dump the information

**Return**

0 on success or a negative error code on failure.

u64 xe\_ggtt\_print\_holes(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, u64 alignment, struct [drm\_printer](../drm-internals.html#c.drm_printer "drm_printer") \*p)
:   Print holes

**Parameters**

`struct xe_ggtt *ggtt`
:   the [`xe_ggtt`](#c.xe_ggtt "xe_ggtt") to be inspected

`u64 alignment`
:   min alignment

`struct drm_printer *p`
:   the [`drm_printer`](../drm-internals.html#c.drm_printer "drm_printer")

**Description**

Print GGTT ranges that are available and return total size available.

**Return**

Total available size.

u64 xe\_ggtt\_encode\_pte\_flags(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, struct xe\_bo \*bo, u16 pat\_index)
:   Get PTE encoding flags for BO

**Parameters**

`struct xe_ggtt *ggtt`
:   [`xe_ggtt`](#c.xe_ggtt "xe_ggtt")

`struct xe_bo *bo`
:   `xe_bo`

`u16 pat_index`
:   The pat\_index for the PTE.

**Description**

This function returns the pte\_flags for a given BO, without address.
It’s used for DPT to fill a GGTT mapped BO with a linear lookup table.

u64 xe\_ggtt\_read\_pte(struct [xe\_ggtt](#c.xe_ggtt "xe_ggtt") \*ggtt, u64 offset)
:   Read a PTE from the GGTT

**Parameters**

`struct xe_ggtt *ggtt`
:   [`xe_ggtt`](#c.xe_ggtt "xe_ggtt")

`u64 offset`
:   the offset for which the mapping should be read.

**Description**

Used by testcases, and by display reading out an inherited bios FB.

u64 xe\_ggtt\_node\_addr(const struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node)
:   Get **node** offset in GGTT.

**Parameters**

`const struct xe_ggtt_node *node`
:   [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node")

**Description**

Get the GGTT offset for allocated node.

u64 xe\_ggtt\_node\_size(const struct [xe\_ggtt\_node](#c.xe_ggtt_node "xe_ggtt_node") \*node)
:   Get **node** allocation size.

**Parameters**

`const struct xe_ggtt_node *node`
:   [`xe_ggtt_node`](#c.xe_ggtt_node "xe_ggtt_node")

**Description**

Get the allocated node’s size.

## Pagetable building

Below we use the term “page-table” for both page-directories, containing
pointers to lower level page-directories or page-tables, and level 0
page-tables that contain only page-table-entries pointing to memory pages.

When inserting an address range in an already existing page-table tree
there will typically be a set of page-tables that are shared with other
address ranges, and a set that are private to this address range.
The set of shared page-tables can be at most two per level,
and those can’t be updated immediately because the entries of those
page-tables may still be in use by the gpu for other mappings. Therefore
when inserting entries into those, we instead stage those insertions by
adding insertion data into `struct xe_vm_pgtable_update` structures. This
data, (subtrees for the cpu and page-table-entries for the gpu) is then
added in a separate commit step. CPU-data is committed while still under the
vm lock, the object lock and for userptr, the notifier lock in read mode.
The GPU async data is committed either by the GPU or CPU after fulfilling
relevant dependencies.
For non-shared page-tables (and, in fact, for shared ones that aren’t
existing at the time of staging), we add the data in-place without the
special update structures. This private part of the page-table tree will
remain disconnected from the vm page-table tree until data is committed to
the shared page tables of the vm tree in the commit phase.
