# DRM Memory Management

> 출처(원문): https://docs.kernel.org/gpu/drm-mm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DRM Memory Management

Modern Linux systems require large amount of graphics memory to store
frame buffers, textures, vertices and other graphics-related data. Given
the very dynamic nature of many of that data, managing graphics memory
efficiently is thus crucial for the graphics stack and plays a central
role in the DRM infrastructure.

The DRM core includes two memory managers, namely Translation Table Manager
(TTM) and Graphics Execution Manager (GEM). TTM was the first DRM memory
manager to be developed and tried to be a one-size-fits-them all
solution. It provides a single userspace API to accommodate the need of
all hardware, supporting both Unified Memory Architecture (UMA) devices
and devices with dedicated video RAM (i.e. most discrete video cards).
This resulted in a large, complex piece of code that turned out to be
hard to use for driver development.

GEM started as an Intel-sponsored project in reaction to TTM’s
complexity. Its design philosophy is completely different: instead of
providing a solution to every graphics memory-related problems, GEM
identified common code between drivers and created a support library to
share it. GEM has simpler initialization and execution requirements than
TTM, but has no video RAM management capabilities and is thus limited to
UMA devices.

## The Translation Table Manager (TTM)

TTM is a memory manager for accelerator devices with dedicated memory.

The basic idea is that resources are grouped together in buffer objects of
certain size and TTM handles lifetime, movement and CPU mappings of those
objects.

TODO: Add more design background and information here.

enum ttm\_caching
:   CPU caching and BUS snooping behavior.

**Constants**

`ttm_uncached`
:   Most defensive option for device mappings,
    don’t even allow write combining.

`ttm_write_combined`
:   Don’t cache read accesses, but allow at least
    writes to be combined.

`ttm_cached`
:   Fully cached like normal system memory, requires that
    devices snoop the CPU cache on accesses.

### TTM device object reference

struct ttm\_global
:   Buffer object driver global data.

**Definition**:

```
struct ttm_global {
    struct page *dummy_read_page;
    struct list_head device_list;
    atomic_t bo_count;
};
```

**Members**

`dummy_read_page`
:   Pointer to a dummy page used for mapping requests
    of unpopulated pages. Constant after init.

`device_list`
:   List of buffer object devices. Protected by
    ttm\_global\_mutex.

`bo_count`
:   Number of buffer objects allocated by devices.

struct ttm\_device
:   Buffer object driver device-specific data.

**Definition**:

```
struct ttm_device {
    struct list_head device_list;
    unsigned int alloc_flags;
    const struct ttm_device_funcs *funcs;
    struct ttm_resource_manager sysman;
    struct ttm_resource_manager *man_drv[TTM_NUM_MEM_TYPES];
    struct drm_vma_offset_manager *vma_manager;
    struct ttm_pool pool;
    spinlock_t lru_lock;
    struct list_head unevictable;
    struct address_space *dev_mapping;
    struct workqueue_struct *wq;
};
```

**Members**

`device_list`
:   Our entry in the global device list.
    Constant after bo device init

`alloc_flags`
:   TTM\_ALLOCATION\_\* flags.

`funcs`
:   Function table for the device.
    Constant after bo device init

`sysman`
:   Resource manager for the system domain.
    Access via ttm\_manager\_type.

`man_drv`
:   An array of resource\_managers, one per resource type.

`vma_manager`
:   Address space manager for finding BOs to mmap.

`pool`
:   page pool for the device.

`lru_lock`
:   Protection for the per manager LRU and ddestroy lists.

`unevictable`
:   Buffer objects which are pinned or swapped and as such
    not on an LRU list.

`dev_mapping`
:   A pointer to the [`struct address_space`](../filesystems/api-summary.html#c.address_space "address_space") for invalidating
    CPU mappings on buffer move. Protected by load/unload sync.

`wq`
:   Work queue structure for the delayed delete workqueue.

int ttm\_device\_prepare\_hibernation(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev)
:   move GTT BOs to shmem for hibernation.

**Parameters**

`struct ttm_device *bdev`
:   A pointer to a [`struct ttm_device`](#c.ttm_device "ttm_device") to prepare hibernation for.

**Return**

0 on success, negative number on failure.

int ttm\_device\_init(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev, const struct ttm\_device\_funcs \*funcs, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct drm\_vma\_offset\_manager \*vma\_manager, unsigned int alloc\_flags)

**Parameters**

`struct ttm_device *bdev`
:   A pointer to a [`struct ttm_device`](#c.ttm_device "ttm_device") to initialize.

`const struct ttm_device_funcs *funcs`
:   Function table for the device.

`struct device *dev`
:   The core kernel device pointer for DMA mappings and allocations.

`struct address_space *mapping`
:   The address space to use for this bo.

`struct drm_vma_offset_manager *vma_manager`
:   A pointer to a vma manager.

`unsigned int alloc_flags`
:   TTM\_ALLOCATION\_\* flags.

**Description**

Initializes a [`struct ttm_device`](#c.ttm_device "ttm_device"):

**Return**

!0: Failure.

### TTM resource placement reference

struct ttm\_place

**Definition**:

```
struct ttm_place {
    unsigned fpfn;
    unsigned lpfn;
    uint32_t mem_type;
    uint32_t flags;
};
```

**Members**

`fpfn`
:   first valid page frame number to put the object

`lpfn`
:   last valid page frame number to put the object

`mem_type`
:   One of TTM\_PL\_\* where the resource should be allocated from.

`flags`
:   memory domain and caching flags for the object

**Description**

Structure indicating a possible place to put an object.

struct ttm\_placement

**Definition**:

```
struct ttm_placement {
    unsigned num_placement;
    const struct ttm_place  *placement;
};
```

**Members**

`num_placement`
:   number of preferred placements

`placement`
:   preferred placements

**Description**

Structure indicating the placement you request for an object.

### TTM resource object reference

TTM\_NUM\_MOVE\_FENCES

`TTM_NUM_MOVE_FENCES`

> > How many entities can be used for evictions
>
> **Description**
>
> Pipelined evictions can be spread on multiple entities. This
> is the max number of entities that can be used by the driver
> for that purpose.

enum ttm\_lru\_item\_type
:   enumerate ttm\_lru\_item subclasses

**Constants**

`TTM_LRU_RESOURCE`
:   The resource subclass

`TTM_LRU_HITCH`
:   The iterator hitch subclass

struct ttm\_lru\_item
:   The TTM lru list node base class

**Definition**:

```
struct ttm_lru_item {
    struct list_head link;
    enum ttm_lru_item_type type;
};
```

**Members**

`link`
:   The list link

`type`
:   The subclass type

void ttm\_lru\_item\_init(struct [ttm\_lru\_item](#c.ttm_lru_item "ttm_lru_item") \*item, enum [ttm\_lru\_item\_type](#c.ttm_lru_item_type "ttm_lru_item_type") type)
:   initialize a [`struct ttm_lru_item`](#c.ttm_lru_item "ttm_lru_item")

**Parameters**

`struct ttm_lru_item *item`
:   The item to initialize

`enum ttm_lru_item_type type`
:   The subclass type

struct ttm\_resource\_manager

**Definition**:

```
struct ttm_resource_manager {
    bool use_type;
    bool use_tt;
    struct ttm_device *bdev;
    uint64_t size;
    const struct ttm_resource_manager_func *func;
    spinlock_t eviction_lock;
    struct dma_fence *eviction_fences[TTM_NUM_MOVE_FENCES];
    struct list_head lru[TTM_MAX_BO_PRIORITY];
    uint64_t usage;
    struct dmem_cgroup_region *cg;
};
```

**Members**

`use_type`
:   The memory type is enabled.

`use_tt`
:   If a TT object should be used for the backing store.

`bdev`
:   ttm device this manager belongs to

`size`
:   Size of the managed region.

`func`
:   structure pointer implementing the range manager. See above

`eviction_lock`
:   lock for eviction fences

`eviction_fences`
:   The fences of the last pipelined move operation.

`lru`
:   The lru list for this memory type.

`usage`
:   How much of the resources are used, protected by the
    bdev->lru\_lock.

`cg`
:   `dmem_cgroup_region` used for memory accounting, if not NULL.

**Description**

This structure is used to identify and manage memory types for a device.

struct ttm\_bus\_placement

**Definition**:

```
struct ttm_bus_placement {
    void *addr;
    phys_addr_t offset;
    bool is_iomem;
    enum ttm_caching        caching;
};
```

**Members**

`addr`
:   mapped virtual address

`offset`
:   physical addr

`is_iomem`
:   is this io memory ?

`caching`
:   See [`enum ttm_caching`](#c.ttm_caching "ttm_caching")

**Description**

Structure indicating the bus placement of an object.

struct ttm\_resource

**Definition**:

```
struct ttm_resource {
    unsigned long start;
    size_t size;
    uint32_t mem_type;
    uint32_t placement;
    struct ttm_bus_placement bus;
    struct ttm_buffer_object *bo;
    struct dmem_cgroup_pool_state *css;
    struct ttm_lru_item lru;
};
```

**Members**

`start`
:   Start of the allocation.

`size`
:   Actual size of resource in bytes.

`mem_type`
:   Resource type of the allocation.

`placement`
:   Placement flags.

`bus`
:   Placement on io bus accessible to the CPU

`bo`
:   weak reference to the BO, protected by ttm\_device::lru\_lock

`css`
:   cgroup state this resource is charged to

`lru`
:   Least recently used list, see [`ttm_resource_manager.lru`](#c.ttm_resource_manager "ttm_resource_manager")

**Description**

Structure indicating the placement and space resources used by a
buffer object.

struct [ttm\_resource](#c.ttm_resource "ttm_resource") \*ttm\_lru\_item\_to\_res(struct [ttm\_lru\_item](#c.ttm_lru_item "ttm_lru_item") \*item)
:   Downcast a [`struct ttm_lru_item`](#c.ttm_lru_item "ttm_lru_item") to a [`struct ttm_resource`](#c.ttm_resource "ttm_resource")

**Parameters**

`struct ttm_lru_item *item`
:   The [`struct ttm_lru_item`](#c.ttm_lru_item "ttm_lru_item") to downcast

**Return**

Pointer to the embedding [`struct ttm_resource`](#c.ttm_resource "ttm_resource")

struct ttm\_lru\_bulk\_move\_pos

**Definition**:

```
struct ttm_lru_bulk_move_pos {
    struct ttm_resource *first;
    struct ttm_resource *last;
};
```

**Members**

`first`
:   first res in the bulk move range

`last`
:   last res in the bulk move range

**Description**

Range of resources for a lru bulk move.

struct ttm\_lru\_bulk\_move

**Definition**:

```
struct ttm_lru_bulk_move {
    struct ttm_lru_bulk_move_pos pos[TTM_NUM_MEM_TYPES][TTM_MAX_BO_PRIORITY];
    struct list_head cursor_list;
};
```

**Members**

`pos`
:   first/last lru entry for resources in the each domain/priority

`cursor_list`
:   The list of cursors currently traversing any of
    the sublists of **pos**. Protected by the ttm device’s lru\_lock.

**Description**

Container for the current bulk move state. Should be used with
[`ttm_lru_bulk_move_init()`](#c.ttm_lru_bulk_move_init "ttm_lru_bulk_move_init") and `ttm_bo_set_bulk_move()`.
All BOs in a bulk\_move structure need to share the same reservation object to
ensure that the bulk as a whole is locked for eviction even if only one BO of
the bulk is evicted.

struct ttm\_resource\_cursor

**Definition**:

```
struct ttm_resource_cursor {
    struct ttm_resource_manager *man;
    struct ttm_lru_item hitch;
    struct list_head bulk_link;
    struct ttm_lru_bulk_move *bulk;
    unsigned int mem_type;
    unsigned int priority;
};
```

**Members**

`man`
:   The resource manager currently being iterated over

`hitch`
:   A hitch list node inserted before the next resource
    to iterate over.

`bulk_link`
:   A list link for the list of cursors traversing the
    bulk sublist of **bulk**. Protected by the ttm device’s lru\_lock.

`bulk`
:   Pointer to [`struct ttm_lru_bulk_move`](#c.ttm_lru_bulk_move "ttm_lru_bulk_move") whose subrange **hitch** is
    inserted to. NULL if none. Never dereference this pointer since
    the [`struct ttm_lru_bulk_move`](#c.ttm_lru_bulk_move "ttm_lru_bulk_move") object pointed to might have been
    freed. The pointer is only for comparison.

`mem_type`
:   The memory type of the LRU list being traversed.
    This field is valid iff **bulk** != NULL.

`priority`
:   the current priority

**Description**

Cursor to iterate over the resources in a manager.

struct ttm\_kmap\_iter\_iomap
:   Specialization for a `struct io_mapping` + `struct sg_table` backed [`struct ttm_resource`](#c.ttm_resource "ttm_resource").

**Definition**:

```
struct ttm_kmap_iter_iomap {
    struct ttm_kmap_iter base;
    struct io_mapping *iomap;
    struct sg_table *st;
    resource_size_t start;
    struct {
        struct scatterlist *sg;
        pgoff_t i;
        pgoff_t end;
        pgoff_t offs;
    } cache;
};
```

**Members**

`base`
:   Embedded `struct ttm_kmap_iter` providing the usage interface.

`iomap`
:   `struct io_mapping` representing the underlying linear io\_memory.

`st`
:   sg\_table into **iomap**, representing the memory of the [`struct ttm_resource`](#c.ttm_resource "ttm_resource").

`start`
:   Offset that needs to be subtracted from **st** to make
    sg\_dma\_address(st->sgl) - **start** == 0 for **iomap** start.

`cache`
:   Scatterlist traversal cache for fast lookups.

`cache.sg`
:   Pointer to the currently cached scatterlist segment.

`cache.i`
:   First index of **sg**. PAGE\_SIZE granularity.

`cache.end`
:   Last index + 1 of **sg**. PAGE\_SIZE granularity.

`cache.offs`
:   First offset into **iomap** of **sg**. PAGE\_SIZE granularity.

struct ttm\_kmap\_iter\_linear\_io
:   Iterator specialization for linear io

**Definition**:

```
struct ttm_kmap_iter_linear_io {
    struct ttm_kmap_iter base;
    struct iosys_map dmap;
    bool needs_unmap;
};
```

**Members**

`base`
:   The base iterator

`dmap`
:   Points to the starting address of the region

`needs_unmap`
:   Whether we need to unmap on fini

void ttm\_resource\_manager\_set\_used(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man, bool used)

**Parameters**

`struct ttm_resource_manager *man`
:   A memory manager object.

`bool used`
:   usage state to set.

**Description**

Set the manager in use flag. If disabled the manager is no longer
used for object placement.

bool ttm\_resource\_manager\_used(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man)

**Parameters**

`struct ttm_resource_manager *man`
:   Manager to get used state for

**Description**

Get the in use flag for a manager.

**Return**

true is used, false if not.

void ttm\_resource\_manager\_cleanup(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man)

**Parameters**

`struct ttm_resource_manager *man`
:   A memory manager object.

**Description**

Cleanup the move fences from the memory manager object.

ttm\_resource\_manager\_for\_each\_res

`ttm_resource_manager_for_each_res (cursor, res)`

> iterate over all resources

**Parameters**

`cursor`
:   [`struct ttm_resource_cursor`](#c.ttm_resource_cursor "ttm_resource_cursor") for the current position

`res`
:   the current resource

**Description**

Iterate over all the evictable resources in a resource manager.

void ttm\_lru\_bulk\_move\_init(struct [ttm\_lru\_bulk\_move](#c.ttm_lru_bulk_move "ttm_lru_bulk_move") \*bulk)
:   initialize a bulk move structure

**Parameters**

`struct ttm_lru_bulk_move *bulk`
:   the structure to init

**Description**

For now just memset the structure to zero.

void ttm\_lru\_bulk\_move\_fini(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev, struct [ttm\_lru\_bulk\_move](#c.ttm_lru_bulk_move "ttm_lru_bulk_move") \*bulk)
:   finalize a bulk move structure

**Parameters**

`struct ttm_device *bdev`
:   The [`struct ttm_device`](#c.ttm_device "ttm_device")

`struct ttm_lru_bulk_move *bulk`
:   the structure to finalize

**Description**

Sanity checks that bulk moves don’t have any
resources left and hence no cursors attached.

void ttm\_lru\_bulk\_move\_tail(struct [ttm\_lru\_bulk\_move](#c.ttm_lru_bulk_move "ttm_lru_bulk_move") \*bulk)
:   bulk move range of resources to the LRU tail.

**Parameters**

`struct ttm_lru_bulk_move *bulk`
:   bulk move structure

**Description**

Bulk move BOs to the LRU tail, only valid to use when driver makes sure that
resource order never changes. Should be called with [`ttm_device.lru_lock`](#c.ttm_device "ttm_device") held.

void ttm\_resource\_init(struct ttm\_buffer\_object \*bo, const struct [ttm\_place](#c.ttm_place "ttm_place") \*place, struct [ttm\_resource](#c.ttm_resource "ttm_resource") \*res)
:   resource object constructor

**Parameters**

`struct ttm_buffer_object *bo`
:   buffer object this resource is allocated for

`const struct ttm_place *place`
:   placement of the resource

`struct ttm_resource *res`
:   the resource object to initialize

**Description**

Initialize a new resource object. Counterpart of [`ttm_resource_fini()`](#c.ttm_resource_fini "ttm_resource_fini").

void ttm\_resource\_fini(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man, struct [ttm\_resource](#c.ttm_resource "ttm_resource") \*res)
:   resource destructor

**Parameters**

`struct ttm_resource_manager *man`
:   the resource manager this resource belongs to

`struct ttm_resource *res`
:   the resource to clean up

**Description**

Should be used by resource manager backends to clean up the TTM resource
objects before freeing the underlying structure. Makes sure the resource is
removed from the LRU before destruction.
Counterpart of [`ttm_resource_init()`](#c.ttm_resource_init "ttm_resource_init").

void ttm\_resource\_manager\_init(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man, struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev, uint64\_t size)

**Parameters**

`struct ttm_resource_manager *man`
:   memory manager object to init

`struct ttm_device *bdev`
:   ttm device this manager belongs to

`uint64_t size`
:   size of managed resources in arbitrary units

**Description**

Initialize core parts of a manager object.

uint64\_t ttm\_resource\_manager\_usage(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man)

**Parameters**

`struct ttm_resource_manager *man`
:   A memory manager object.

**Description**

Return how many resources are currently used.

void ttm\_resource\_manager\_debug(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p)

**Parameters**

`struct ttm_resource_manager *man`
:   manager type to dump.

`struct drm_printer *p`
:   printer to use for debug.

struct ttm\_kmap\_iter \*ttm\_kmap\_iter\_iomap\_init(struct [ttm\_kmap\_iter\_iomap](#c.ttm_kmap_iter_iomap "ttm_kmap_iter_iomap") \*iter\_io, struct io\_mapping \*iomap, struct sg\_table \*st, resource\_size\_t start)
:   Initialize a [`struct ttm_kmap_iter_iomap`](#c.ttm_kmap_iter_iomap "ttm_kmap_iter_iomap")

**Parameters**

`struct ttm_kmap_iter_iomap *iter_io`
:   The [`struct ttm_kmap_iter_iomap`](#c.ttm_kmap_iter_iomap "ttm_kmap_iter_iomap") to initialize.

`struct io_mapping *iomap`
:   The `struct io_mapping` representing the underlying linear io\_memory.

`struct sg_table *st`
:   sg\_table into **iomap**, representing the memory of the [`struct
    ttm_resource`](#c.ttm_resource "ttm_resource").

`resource_size_t start`
:   Offset that needs to be subtracted from **st** to make
    sg\_dma\_address(st->sgl) - **start** == 0 for **iomap** start.

**Return**

Pointer to the embedded `struct ttm_kmap_iter`.

void ttm\_resource\_manager\_create\_debugfs(struct [ttm\_resource\_manager](#c.ttm_resource_manager "ttm_resource_manager") \*man, struct dentry \*parent, const char \*name)
:   Create debugfs entry for specified resource manager.

**Parameters**

`struct ttm_resource_manager *man`
:   The TTM resource manager for which the debugfs stats file to be created

`struct dentry *parent`
:   debugfs directory in which the file will reside

`const char *name`
:   The filename to create.

**Description**

This function sets up a debugfs file that can be used to look
at debug statistics of the specified ttm\_resource\_manager.

### TTM TT object reference

struct ttm\_tt
:   This is a structure holding the pages, caching- and aperture binding status for a buffer object that isn’t backed by fixed (VRAM / AGP) memory.

**Definition**:

```
struct ttm_tt {
    struct page **pages;
#define TTM_TT_FLAG_SWAPPED             BIT(0);
#define TTM_TT_FLAG_ZERO_ALLOC          BIT(1);
#define TTM_TT_FLAG_EXTERNAL            BIT(2);
#define TTM_TT_FLAG_EXTERNAL_MAPPABLE   BIT(3);
#define TTM_TT_FLAG_DECRYPTED           BIT(4);
#define TTM_TT_FLAG_BACKED_UP           BIT(5);
#define TTM_TT_FLAG_PRIV_POPULATED      BIT(6);
    uint32_t page_flags;
    uint32_t num_pages;
    struct sg_table *sg;
    dma_addr_t *dma_address;
    struct file *swap_storage;
    struct file *backup;
    enum ttm_caching caching;
    struct ttm_pool_tt_restore *restore;
};
```

**Members**

`pages`
:   Array of pages backing the data.

`page_flags`
:   The page flags.

    Supported values:

    TTM\_TT\_FLAG\_SWAPPED: Set by TTM when the pages have been unpopulated
    and swapped out by TTM. Calling [`ttm_tt_populate()`](#c.ttm_tt_populate "ttm_tt_populate") will then swap the
    pages back in, and unset the flag. Drivers should in general never
    need to touch this.

    TTM\_TT\_FLAG\_ZERO\_ALLOC: Set if the pages will be zeroed on
    allocation.

    TTM\_TT\_FLAG\_EXTERNAL: Set if the underlying pages were allocated
    externally, like with dma-buf or userptr. This effectively disables
    TTM swapping out such pages. Also important is to prevent TTM from
    ever directly mapping these pages.

    Note that `enum ttm_bo_type`.ttm\_bo\_type\_sg objects will always enable
    this flag.

    TTM\_TT\_FLAG\_EXTERNAL\_MAPPABLE: Same behaviour as
    TTM\_TT\_FLAG\_EXTERNAL, but with the reduced restriction that it is
    still valid to use TTM to map the pages directly. This is useful when
    implementing a ttm\_tt backend which still allocates driver owned
    pages underneath(say with shmem).

    Note that since this also implies TTM\_TT\_FLAG\_EXTERNAL, the usage
    here should always be:

    > page\_flags = TTM\_TT\_FLAG\_EXTERNAL |
    > :   TTM\_TT\_FLAG\_EXTERNAL\_MAPPABLE;

    TTM\_TT\_FLAG\_DECRYPTED: The mapped ttm pages should be marked as
    not encrypted. The framework will try to match what the dma layer
    is doing, but note that it is a little fragile because ttm page
    fault handling abuses the DMA api a bit and dma\_map\_attrs can’t be
    used to assure pgprot always matches.

    TTM\_TT\_FLAG\_BACKED\_UP: TTM internal only. This is set if the
    [`struct ttm_tt`](#c.ttm_tt "ttm_tt") has been (possibly partially) backed up.

    TTM\_TT\_FLAG\_PRIV\_POPULATED: TTM internal only. DO NOT USE. This is
    set by TTM after [`ttm_tt_populate()`](#c.ttm_tt_populate "ttm_tt_populate") has successfully returned, and is
    then unset when TTM calls [`ttm_tt_unpopulate()`](#c.ttm_tt_unpopulate "ttm_tt_unpopulate").

`num_pages`
:   Number of pages in the page array.

`sg`
:   for SG objects via dma-buf.

`dma_address`
:   The DMA (bus) addresses of the pages.

`swap_storage`
:   Pointer to shmem [`struct file`](../filesystems/api-summary.html#c.file "file") for swap storage.

`backup`
:   Pointer to backup struct for backed up tts.
    Could be unified with **swap\_storage**. Meanwhile, the driver’s
    [`ttm_tt_create()`](#c.ttm_tt_create "ttm_tt_create") callback is responsible for assigning
    this field.

`caching`
:   The current caching state of the pages, see [`enum
    ttm_caching`](#c.ttm_caching "ttm_caching").

`restore`
:   Partial restoration from backup state. TTM private

struct ttm\_kmap\_iter\_tt
:   Specialization of a mappig iterator for a tt.

**Definition**:

```
struct ttm_kmap_iter_tt {
    struct ttm_kmap_iter base;
    struct ttm_tt *tt;
    pgprot_t prot;
};
```

**Members**

`base`
:   Embedded `struct ttm_kmap_iter` providing the usage interface

`tt`
:   Cached [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

`prot`
:   Cached page protection for mapping.

bool ttm\_tt\_is\_swapped(const struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt)
:   Whether the ttm\_tt is swapped out or backed up

**Parameters**

`const struct ttm_tt *tt`
:   The [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

**Return**

true if swapped or backed up, false otherwise.

bool ttm\_tt\_is\_backed\_up(const struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt)
:   Whether the ttm\_tt backed up

**Parameters**

`const struct ttm_tt *tt`
:   The [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

**Return**

true if swapped or backed up, false otherwise.

void ttm\_tt\_clear\_backed\_up(struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt)
:   Clear the ttm\_tt backed-up status

**Parameters**

`struct ttm_tt *tt`
:   The [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

**Description**

Drivers can use this functionto clear the backed-up status,
for example before destroying or re-validating a purged tt.

int ttm\_tt\_create(struct ttm\_buffer\_object \*bo, bool zero\_alloc)

**Parameters**

`struct ttm_buffer_object *bo`
:   pointer to a `struct ttm_buffer_object`

`bool zero_alloc`
:   true if allocated pages needs to be zeroed

**Description**

Make sure we have a TTM structure allocated for the given BO.
No pages are actually allocated.

int ttm\_tt\_init(struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm, struct ttm\_buffer\_object \*bo, uint32\_t page\_flags, enum [ttm\_caching](#c.ttm_caching "ttm_caching") caching, unsigned long extra\_pages)

**Parameters**

`struct ttm_tt *ttm`
:   The [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

`struct ttm_buffer_object *bo`
:   The buffer object we create the ttm for.

`uint32_t page_flags`
:   Page flags as identified by TTM\_TT\_FLAG\_XX flags.

`enum ttm_caching caching`
:   the desired caching state of the pages

`unsigned long extra_pages`
:   Extra pages needed for the driver.

**Description**

Create a [`struct ttm_tt`](#c.ttm_tt "ttm_tt") to back data with system memory pages.
No pages are actually allocated.

**Return**

NULL: Out of memory.

void ttm\_tt\_fini(struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm)

**Parameters**

`struct ttm_tt *ttm`
:   the ttm\_tt structure.

**Description**

Free memory of ttm\_tt structure

void ttm\_tt\_destroy(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev, struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm)

**Parameters**

`struct ttm_device *bdev`
:   the ttm\_device this object belongs to

`struct ttm_tt *ttm`
:   The [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

**Description**

Unbind, unpopulate and destroy common [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

int ttm\_tt\_swapin(struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm)

**Parameters**

`struct ttm_tt *ttm`
:   The [`struct ttm_tt`](#c.ttm_tt "ttm_tt").

**Description**

Swap in a previously swap out ttm\_tt.

int ttm\_tt\_populate(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev, struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm, struct ttm\_operation\_ctx \*ctx)
:   allocate pages for a ttm

**Parameters**

`struct ttm_device *bdev`
:   the ttm\_device this object belongs to

`struct ttm_tt *ttm`
:   Pointer to the ttm\_tt structure

`struct ttm_operation_ctx *ctx`
:   operation context for populating the tt object.

**Description**

Calls the driver method to allocate pages for a ttm

void ttm\_tt\_unpopulate(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev, struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm)
:   free pages from a ttm

**Parameters**

`struct ttm_device *bdev`
:   the ttm\_device this object belongs to

`struct ttm_tt *ttm`
:   Pointer to the ttm\_tt structure

**Description**

Calls the driver method to free all pages from a ttm

void ttm\_tt\_mark\_for\_clear(struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm)
:   Mark pages for clearing on populate.

**Parameters**

`struct ttm_tt *ttm`
:   Pointer to the ttm\_tt structure

**Description**

Marks pages for clearing so that the next time the page vector is
populated, the pages will be cleared.

struct ttm\_backup\_flags
:   Flags to govern backup behaviour.

**Definition**:

```
struct ttm_backup_flags {
    u32 purge : 1;
    u32 writeback : 1;
};
```

**Members**

`purge`
:   Free pages without backing up. Bypass pools.

`writeback`
:   Attempt to copy contents directly to swap space, even
    if that means blocking on writes to external memory.

struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*ttm\_agp\_tt\_create(struct ttm\_buffer\_object \*bo, struct agp\_bridge\_data \*bridge, uint32\_t page\_flags)

**Parameters**

`struct ttm_buffer_object *bo`
:   Buffer object we allocate the ttm for.

`struct agp_bridge_data *bridge`
:   The agp bridge this device is sitting on.

`uint32_t page_flags`
:   Page flags as identified by TTM\_TT\_FLAG\_XX flags.

**Description**

Create a TTM backend that uses the indicated AGP bridge as an aperture
for TT memory. This function uses the linux agpgart interface to
bind and unbind memory backing a ttm\_tt.

struct ttm\_kmap\_iter \*ttm\_kmap\_iter\_tt\_init(struct [ttm\_kmap\_iter\_tt](#c.ttm_kmap_iter_tt "ttm_kmap_iter_tt") \*iter\_tt, struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt)
:   Initialize a [`struct ttm_kmap_iter_tt`](#c.ttm_kmap_iter_tt "ttm_kmap_iter_tt")

**Parameters**

`struct ttm_kmap_iter_tt *iter_tt`
:   The [`struct ttm_kmap_iter_tt`](#c.ttm_kmap_iter_tt "ttm_kmap_iter_tt") to initialize.

`struct ttm_tt *tt`
:   Struct ttm\_tt holding page pointers of the [`struct ttm_resource`](#c.ttm_resource "ttm_resource").

**Return**

Pointer to the embedded `struct ttm_kmap_iter`.

int ttm\_tt\_setup\_backup(struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt)
:   Allocate and assign a backup structure for a ttm\_tt

**Parameters**

`struct ttm_tt *tt`
:   The ttm\_tt for wich to allocate and assign a backup structure.

**Description**

Assign a backup structure to be used for tt backup. This should
typically be done at bo creation, to avoid allocations at shrinking
time.

**Return**

0 on success, negative error code on failure.

### TTM page pool reference

struct ttm\_pool\_type
:   Pool for a certain memory type

**Definition**:

```
struct ttm_pool_type {
    struct ttm_pool *pool;
    unsigned int order;
    enum ttm_caching caching;
    struct list_head shrinker_list;
    struct list_lru pages;
};
```

**Members**

`pool`
:   the pool we belong to, might be NULL for the global ones

`order`
:   the allocation order our pages have

`caching`
:   the caching type our pages have

`shrinker_list`
:   our place on the global shrinker list

`pages`
:   the lru\_list of pages in the pool

struct ttm\_pool
:   Pool for all caching and orders

**Definition**:

```
struct ttm_pool {
    struct device *dev;
    int nid;
    unsigned int alloc_flags;
    struct {
        struct ttm_pool_type orders[NR_PAGE_ORDERS];
    } caching[TTM_NUM_CACHING_TYPES];
};
```

**Members**

`dev`
:   the device we allocate pages for

`nid`
:   which numa node to use

`alloc_flags`
:   TTM\_ALLOCATION\_POOL\_\* flags

`caching`
:   pools for each caching/order

int ttm\_pool\_alloc(struct [ttm\_pool](#c.ttm_pool "ttm_pool") \*pool, struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt, struct ttm\_operation\_ctx \*ctx)
:   Fill a ttm\_tt object

**Parameters**

`struct ttm_pool *pool`
:   ttm\_pool to use

`struct ttm_tt *tt`
:   ttm\_tt object to fill

`struct ttm_operation_ctx *ctx`
:   operation context

**Description**

Fill the ttm\_tt object with pages and also make sure to DMA map them when
necessary.

**Return**

0 on successe, negative error code otherwise.

void ttm\_pool\_free(struct [ttm\_pool](#c.ttm_pool "ttm_pool") \*pool, struct [ttm\_tt](#c.ttm_tt "ttm_tt") \*tt)
:   Free the backing pages from a ttm\_tt object

**Parameters**

`struct ttm_pool *pool`
:   Pool to give pages back to.

`struct ttm_tt *tt`
:   ttm\_tt object to unpopulate

**Description**

Give the packing pages back to a pool or free them

void ttm\_pool\_init(struct [ttm\_pool](#c.ttm_pool "ttm_pool") \*pool, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int nid, unsigned int alloc\_flags)
:   Initialize a pool

**Parameters**

`struct ttm_pool *pool`
:   the pool to initialize

`struct device *dev`
:   device for DMA allocations and mappings

`int nid`
:   NUMA node to use for allocations

`unsigned int alloc_flags`
:   TTM\_ALLOCATION\_POOL\_\* flags

**Description**

Initialize the pool and its pool types.

void ttm\_pool\_fini(struct [ttm\_pool](#c.ttm_pool "ttm_pool") \*pool)
:   Cleanup a pool

**Parameters**

`struct ttm_pool *pool`
:   the pool to clean up

**Description**

Free all pages in the pool and unregister the types from the global
shrinker.

int ttm\_pool\_debugfs(struct [ttm\_pool](#c.ttm_pool "ttm_pool") \*pool, struct seq\_file \*m)
:   Debugfs dump function for a pool

**Parameters**

`struct ttm_pool *pool`
:   the pool to dump the information for

`struct seq_file *m`
:   seq\_file to dump to

**Description**

Make a debugfs dump with the per pool and global information.

## The Graphics Execution Manager (GEM)

The GEM design approach has resulted in a memory manager that doesn’t
provide full coverage of all (or even all common) use cases in its
userspace or kernel API. GEM exposes a set of standard memory-related
operations to userspace and a set of helper functions to drivers, and
let drivers implement hardware-specific operations with their own
private API.

The GEM userspace API is described in the [GEM - the Graphics Execution
Manager](http://lwn.net/Articles/283798/) article on LWN. While
slightly outdated, the document provides a good overview of the GEM API
principles. Buffer allocation and read and write operations, described
as part of the common GEM API, are currently implemented using
driver-specific ioctls.

GEM is data-agnostic. It manages abstract buffer objects without knowing
what individual buffers contain. APIs that require knowledge of buffer
contents or purpose, such as buffer allocation or synchronization
primitives, are thus outside of the scope of GEM and must be implemented
using driver-specific ioctls.

On a fundamental level, GEM involves several operations:

* Memory allocation and freeing
* Command execution
* Aperture management at command execution time

Buffer object allocation is relatively straightforward and largely
provided by Linux’s shmem layer, which provides memory to back each
object.

Device-specific operations, such as command execution, pinning, buffer
read & write, mapping, and domain ownership transfers are left to
driver-specific ioctls.

### GEM Initialization

Drivers that use GEM must set the DRIVER\_GEM bit in the struct
[`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver") driver\_features
field. The DRM core will then automatically initialize the GEM core
before calling the load operation. Behind the scene, this will create a
DRM Memory Manager object which provides an address space pool for
object allocation.

In a KMS configuration, drivers need to allocate and initialize a
command ring buffer following core GEM initialization if required by the
hardware. UMA devices usually have what is called a “stolen” memory
region, which provides space for the initial framebuffer and large,
contiguous memory regions required by the device. This space is
typically not managed by GEM, and must be initialized separately into
its own DRM MM object.

### GEM Objects Creation

GEM splits creation of GEM objects and allocation of the memory that
backs them in two distinct operations.

GEM objects are represented by an instance of struct [`struct
drm_gem_object`](#c.drm_gem_object "drm_gem_object"). Drivers usually need to
extend GEM objects with private information and thus create a
driver-specific GEM object structure type that embeds an instance of
struct [`struct drm_gem_object`](#c.drm_gem_object "drm_gem_object").

To create a GEM object, a driver allocates memory for an instance of its
specific GEM object type and initializes the embedded struct
[`struct drm_gem_object`](#c.drm_gem_object "drm_gem_object") with a call
to [`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init"). The function takes a pointer
to the DRM device, a pointer to the GEM object and the buffer object
size in bytes.

GEM uses shmem to allocate anonymous pageable memory.
[`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init") will create an shmfs file of the
requested size and store it into the struct [`struct
drm_gem_object`](#c.drm_gem_object "drm_gem_object") filp field. The memory is
used as either main storage for the object when the graphics hardware
uses system memory directly or as a backing store otherwise. Drivers
can call [`drm_gem_huge_mnt_create()`](#c.drm_gem_huge_mnt_create "drm_gem_huge_mnt_create") to create, mount and use a huge
shmem mountpoint instead of the default one (‘shm\_mnt’). For builds
with CONFIG\_TRANSPARENT\_HUGEPAGE enabled, further calls to
[`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init") will let shmem allocate huge pages when
possible.

Drivers are responsible for the actual physical pages allocation by
calling `shmem_read_mapping_page_gfp()` for each page.
Note that they can decide to allocate pages when initializing the GEM
object, or to delay allocation until the memory is needed (for instance
when a page fault occurs as a result of a userspace memory access or
when the driver needs to start a DMA transfer involving the memory).

Anonymous pageable memory allocation is not always desired, for instance
when the hardware requires physically contiguous system memory as is
often the case in embedded devices. Drivers can create GEM objects with
no shmfs backing (called private GEM objects) by initializing them with a call
to [`drm_gem_private_object_init()`](#c.drm_gem_private_object_init "drm_gem_private_object_init") instead of [`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init"). Storage for
private GEM objects must be managed by drivers.

### GEM Objects Lifetime

All GEM objects are reference-counted by the GEM core. References can be
acquired and release by calling [`drm_gem_object_get()`](#c.drm_gem_object_get "drm_gem_object_get") and [`drm_gem_object_put()`](#c.drm_gem_object_put "drm_gem_object_put")
respectively.

When the last reference to a GEM object is released the GEM core calls
the `struct drm_gem_object_funcs` free
operation. That operation is mandatory for GEM-enabled drivers and must
free the GEM object and all associated resources.

void (\*free) ([`struct drm_gem_object`](#c.drm_gem_object "drm_gem_object") \*obj); Drivers are
responsible for freeing all GEM object resources. This includes the
resources created by the GEM core, which need to be released with
[`drm_gem_object_release()`](#c.drm_gem_object_release "drm_gem_object_release").

### GEM Objects Naming

Communication between userspace and the kernel refers to GEM objects
using local handles, global names or, more recently, file descriptors.
All of those are 32-bit integer values; the usual Linux kernel limits
apply to the file descriptors.

GEM handles are local to a DRM file. Applications get a handle to a GEM
object through a driver-specific ioctl, and can use that handle to refer
to the GEM object in other standard or driver-specific ioctls. Closing a
DRM file handle frees all its GEM handles and dereferences the
associated GEM objects.

To create a handle for a GEM object drivers call [`drm_gem_handle_create()`](#c.drm_gem_handle_create "drm_gem_handle_create"). The
function takes a pointer to the DRM file and the GEM object and returns a
locally unique handle. When the handle is no longer needed drivers delete it
with a call to [`drm_gem_handle_delete()`](#c.drm_gem_handle_delete "drm_gem_handle_delete"). Finally the GEM object associated with a
handle can be retrieved by a call to [`drm_gem_object_lookup()`](#c.drm_gem_object_lookup "drm_gem_object_lookup").

Handles don’t take ownership of GEM objects, they only take a reference
to the object that will be dropped when the handle is destroyed. To
avoid leaking GEM objects, drivers must make sure they drop the
reference(s) they own (such as the initial reference taken at object
creation time) as appropriate, without any special consideration for the
handle. For example, in the particular case of combined GEM object and
handle creation in the implementation of the dumb\_create operation,
drivers must drop the initial reference to the GEM object before
returning the handle.

GEM names are similar in purpose to handles but are not local to DRM
files. They can be passed between processes to reference a GEM object
globally. Names can’t be used directly to refer to objects in the DRM
API, applications must convert handles to names and names to handles
using the DRM\_IOCTL\_GEM\_FLINK and DRM\_IOCTL\_GEM\_OPEN ioctls
respectively. The conversion is handled by the DRM core without any
driver-specific support.

GEM also supports buffer sharing with dma-buf file descriptors through
PRIME. GEM-based drivers must use the provided helpers functions to
implement the exporting and importing correctly. See ?. Since sharing
file descriptors is inherently more secure than the easily guessable and
global GEM names it is the preferred buffer sharing mechanism. Sharing
buffers through GEM names is only supported for legacy userspace.
Furthermore PRIME also allows cross-device buffer sharing since it is
based on dma-bufs.

### GEM Objects Mapping

Because mapping operations are fairly heavyweight GEM favours
read/write-like access to buffers, implemented through driver-specific
ioctls, over mapping buffers to userspace. However, when random access
to the buffer is needed (to perform software rendering for instance),
direct access to the object can be more efficient.

The mmap system call can’t be used directly to map GEM objects, as they
don’t have their own file handle. Two alternative methods currently
co-exist to map GEM objects to userspace. The first method uses a
driver-specific ioctl to perform the mapping operation, calling
[`do_mmap()`](../core-api/mm-api.html#c.do_mmap "do_mmap") under the hood. This is often considered
dubious, seems to be discouraged for new GEM-enabled drivers, and will
thus not be described here.

The second method uses the mmap system call on the DRM file handle. void
\*mmap(void \*addr, size\_t length, int prot, int flags, int fd, off\_t
offset); DRM identifies the GEM object to be mapped by a fake offset
passed through the mmap offset argument. Prior to being mapped, a GEM
object must thus be associated with a fake offset. To do so, drivers
must call [`drm_gem_create_mmap_offset()`](#c.drm_gem_create_mmap_offset "drm_gem_create_mmap_offset") on the object.

Once allocated, the fake offset value must be passed to the application
in a driver-specific way and can then be used as the mmap offset
argument.

The GEM core provides a helper method [`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap") to
handle object mapping. The method can be set directly as the mmap file
operation handler. It will look up the GEM object based on the offset
value and set the VMA operations to the [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver") gem\_vm\_ops field. Note that [`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap") doesn’t map memory to
userspace, but relies on the driver-provided fault handler to map pages
individually.

To use [`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap"), drivers must fill the struct [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver") gem\_vm\_ops field with a pointer to VM operations.

The VM operations is a `struct vm_operations_struct`
made up of several fields, the more interesting ones being:

```
struct vm_operations_struct {
        void (*open)(struct vm_area_struct * area);
        void (*close)(struct vm_area_struct * area);
        vm_fault_t (*fault)(struct vm_fault *vmf);
};
```

The open and close operations must update the GEM object reference
count. Drivers can use the [`drm_gem_vm_open()`](#c.drm_gem_vm_open "drm_gem_vm_open") and [`drm_gem_vm_close()`](#c.drm_gem_vm_close "drm_gem_vm_close") helper
functions directly as open and close handlers.

The fault operation handler is responsible for mapping pages to
userspace when a page fault occurs. Depending on the memory allocation
scheme, drivers can allocate pages at fault time, or can decide to
allocate memory for the GEM object at the time the object is created.

Drivers that want to map the GEM object upfront instead of handling page
faults can implement their own mmap file operation handler.

In order to reduce page table overhead, if the internal shmem mountpoint
“shm\_mnt” is configured to use transparent huge pages (for builds with
CONFIG\_TRANSPARENT\_HUGEPAGE enabled) and if the shmem backing store
managed to allocate a huge page for a faulty address, the fault handler
will first attempt to insert that huge page into the VMA before falling
back to individual page insertion. mmap() user address alignment for GEM
objects is handled by providing a custom get\_unmapped\_area file
operation which forwards to the shmem backing store. For most drivers,
which don’t create a huge mountpoint by default or through a module
parameter, transparent huge pages can be enabled by either setting the
“transparent\_hugepage\_shmem” kernel parameter or the
“/sys/kernel/mm/transparent\_hugepage/shmem\_enabled” sysfs knob.

For platforms without MMU the GEM core provides a helper method
[`drm_gem_dma_get_unmapped_area()`](#c.drm_gem_dma_get_unmapped_area "drm_gem_dma_get_unmapped_area"). The mmap() routines will call this to get a
proposed address for the mapping.

To use [`drm_gem_dma_get_unmapped_area()`](#c.drm_gem_dma_get_unmapped_area "drm_gem_dma_get_unmapped_area"), drivers must fill the struct
`struct file_operations` get\_unmapped\_area field with
a pointer on [`drm_gem_dma_get_unmapped_area()`](#c.drm_gem_dma_get_unmapped_area "drm_gem_dma_get_unmapped_area").

More detailed information about get\_unmapped\_area can be found in
[No-MMU memory mapping support](../admin-guide/mm/nommu-mmap.html)

### Memory Coherency

When mapped to the device or used in a command buffer, backing pages for
an object are flushed to memory and marked write combined so as to be
coherent with the GPU. Likewise, if the CPU accesses an object after the
GPU has finished rendering to the object, then the object must be made
coherent with the CPU’s view of memory, usually involving GPU cache
flushing of various kinds. This core CPU<->GPU coherency management is
provided by a device-specific ioctl, which evaluates an object’s current
domain and performs any necessary flushing or synchronization to put the
object into the desired coherency domain (note that the object may be
busy, i.e. an active render target; in that case, setting the domain
blocks the client and waits for rendering to complete before performing
any necessary flushing operations).

### Command Execution

Perhaps the most important GEM function for GPU devices is providing a
command execution interface to clients. Client programs construct
command buffers containing references to previously allocated memory
objects, and then submit them to GEM. At that point, GEM takes care to
bind all the objects into the GTT, execute the buffer, and provide
necessary synchronization between clients accessing the same buffers.
This often involves evicting some objects from the GTT and re-binding
others (a fairly expensive operation), and providing relocation support
which hides fixed GTT offsets from clients. Clients must take care not
to submit command buffers that reference more objects than can fit in
the GTT; otherwise, GEM will reject them and no rendering will occur.
Similarly, if several objects in the buffer require fence registers to
be allocated for correct rendering (e.g. 2D blits on pre-965 chips),
care must be taken not to require more fence registers than are
available to the client. Such resource management should be abstracted
from the client in libdrm.

### GEM Function Reference

enum drm\_gem\_object\_status
:   bitmask of object state for fdinfo reporting

**Constants**

`DRM_GEM_OBJECT_RESIDENT`
:   object is resident in memory (ie. not unpinned)

`DRM_GEM_OBJECT_PURGEABLE`
:   object marked as purgeable by userspace

`DRM_GEM_OBJECT_ACTIVE`
:   object is currently used by an active submission

**Description**

Bitmask of status used for fdinfo memory stats, see [`drm_gem_object_funcs.status`](#c.drm_gem_object_funcs "drm_gem_object_funcs")
and [`drm_show_fdinfo()`](drm-internals.html#c.drm_show_fdinfo "drm_show_fdinfo"). Note that an object can report DRM\_GEM\_OBJECT\_PURGEABLE
and be active or not resident, in which case [`drm_show_fdinfo()`](drm-internals.html#c.drm_show_fdinfo "drm_show_fdinfo") will not
account for it as purgeable. So drivers do not need to check if the buffer
is idle and resident to return this bit, i.e. userspace can mark a buffer as
purgeable even while it is still busy on the GPU. It will not get reported in
the puregeable stats until it becomes idle. The status gem object func does
not need to consider this.

struct drm\_gem\_object\_funcs
:   GEM object functions

**Definition**:

```
struct drm_gem_object_funcs {
    void (*free)(struct drm_gem_object *obj);
    int (*open)(struct drm_gem_object *obj, struct drm_file *file);
    void (*close)(struct drm_gem_object *obj, struct drm_file *file);
    void (*print_info)(struct drm_printer *p, unsigned int indent, const struct drm_gem_object *obj);
    struct dma_buf *(*export)(struct drm_gem_object *obj, int flags);
    int (*pin)(struct drm_gem_object *obj);
    void (*unpin)(struct drm_gem_object *obj);
    struct sg_table *(*get_sg_table)(struct drm_gem_object *obj);
    int (*vmap)(struct drm_gem_object *obj, struct iosys_map *map);
    void (*vunmap)(struct drm_gem_object *obj, struct iosys_map *map);
    int (*mmap)(struct drm_gem_object *obj, struct vm_area_struct *vma);
    int (*evict)(struct drm_gem_object *obj);
    enum drm_gem_object_status (*status)(struct drm_gem_object *obj);
    size_t (*rss)(struct drm_gem_object *obj);
    const struct vm_operations_struct *vm_ops;
};
```

**Members**

`free`
:   Deconstructor for drm\_gem\_objects.

    This callback is mandatory.

`open`
:   Called upon GEM handle creation.

    This callback is optional.

`close`
:   Called upon GEM handle release.

    This callback is optional.

`print_info`
:   If driver subclasses struct [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"), it can implement this
    optional hook for printing additional driver specific info.

    [`drm_printf_indent()`](drm-internals.html#c.drm_printf_indent "drm_printf_indent") should be used in the callback passing it the
    indent argument.

    This callback is called from `drm_gem_print_info()`.

    This callback is optional.

`export`
:   Export backing buffer as a [`dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf").
    If this is not set [`drm_gem_prime_export()`](#c.drm_gem_prime_export "drm_gem_prime_export") is used.

    This callback is optional.

`pin`
:   Pin backing buffer in memory, such that dma-buf importers can
    access it. Used by the [`drm_gem_map_attach()`](#c.drm_gem_map_attach "drm_gem_map_attach") helper.

    This callback is optional.

`unpin`
:   Unpin backing buffer. Used by the [`drm_gem_map_detach()`](#c.drm_gem_map_detach "drm_gem_map_detach") helper.

    This callback is optional.

`get_sg_table`
:   Returns a Scatter-Gather table representation of the buffer.
    Used when exporting a buffer by the [`drm_gem_map_dma_buf()`](#c.drm_gem_map_dma_buf "drm_gem_map_dma_buf") helper.
    Releasing is done by calling `dma_unmap_sg_attrs()` and [`sg_free_table()`](../core-api/dma-api.html#c.sg_free_table "sg_free_table")
    in `drm_gem_unmap_buf()`, therefore these helpers and this callback
    here cannot be used for sg tables pointing at driver private memory
    ranges.

    See also [`drm_prime_pages_to_sg()`](#c.drm_prime_pages_to_sg "drm_prime_pages_to_sg").

`vmap`
:   Returns a virtual address for the buffer. Used by the
    [`drm_gem_dmabuf_vmap()`](#c.drm_gem_dmabuf_vmap "drm_gem_dmabuf_vmap") helper. Called with a held GEM reservation
    lock.

    This callback is optional.

`vunmap`
:   Releases the address previously returned by **vmap**. Used by the
    [`drm_gem_dmabuf_vunmap()`](#c.drm_gem_dmabuf_vunmap "drm_gem_dmabuf_vunmap") helper. Called with a held GEM reservation
    lock.

    This callback is optional.

`mmap`
:   Handle mmap() of the gem object, setup vma accordingly.

    This callback is optional.

    The callback is used by both [`drm_gem_mmap_obj()`](#c.drm_gem_mmap_obj "drm_gem_mmap_obj") and
    [`drm_gem_prime_mmap()`](#c.drm_gem_prime_mmap "drm_gem_prime_mmap"). When **mmap** is present **vm\_ops** is not
    used, the **mmap** callback must set vma->vm\_ops instead.

`evict`
:   Evicts gem object out from memory. Used by the `drm_gem_object_evict()`
    helper. Returns 0 on success, -errno otherwise. Called with a held
    GEM reservation lock.

    This callback is optional.

`status`
:   The optional status callback can return additional object state
    which determines which stats the object is counted against. The
    callback is called under table\_lock. Racing against object status
    change is “harmless”, and the callback can expect to not race
    against object destruction.

    Called by [`drm_show_memory_stats()`](drm-internals.html#c.drm_show_memory_stats "drm_show_memory_stats").

`rss`
:   Return resident size of the object in physical memory.

    Called by [`drm_show_memory_stats()`](drm-internals.html#c.drm_show_memory_stats "drm_show_memory_stats").

`vm_ops`
:   Virtual memory operations used with mmap.

    This is optional but necessary for mmap support.

struct drm\_gem\_lru
:   A simple LRU helper

**Definition**:

```
struct drm_gem_lru {
    long count;
    struct list_head list;
};
```

**Members**

`count`
:   The total number of backing pages of the GEM objects in
    this LRU.

`list`
:   The LRU list.

**Description**

A helper for tracking GEM objects in a given state, to aid in
driver’s shrinker implementation. Tracks the count of pages
for lockless `shrinker.count_objects`, and provides
[`drm_gem_lru_scan`](#c.drm_gem_lru_scan "drm_gem_lru_scan") for driver’s `shrinker.scan_objects`
implementation.

Any access to this kind of object must be done with
drm\_device::gem\_lru\_mutex held.

struct drm\_gem\_object
:   GEM buffer object

**Definition**:

```
struct drm_gem_object {
    struct kref refcount;
    unsigned handle_count;
    struct drm_device *dev;
    struct file *filp;
    struct drm_vma_offset_node vma_node;
    size_t size;
    int name;
    struct dma_buf *dma_buf;
    struct dma_buf_attachment *import_attach;
    struct dma_resv *resv;
    struct dma_resv _resv;
    struct {
        struct list_head list;
        struct mutex lock;
    } gpuva;
    const struct drm_gem_object_funcs *funcs;
    struct list_head lru_node;
    struct drm_gem_lru *lru;
};
```

**Members**

`refcount`
:   Reference count of this object

    Please use [`drm_gem_object_get()`](#c.drm_gem_object_get "drm_gem_object_get") to acquire and `drm_gem_object_put_locked()`
    or [`drm_gem_object_put()`](#c.drm_gem_object_put "drm_gem_object_put") to release a reference to a GEM
    buffer object.

`handle_count`
:   This is the GEM file\_priv handle count of this object.

    Each handle also holds a reference. Note that when the handle\_count
    drops to 0 any global names (e.g. the id in the flink namespace) will
    be cleared.

    Protected by [`drm_device.object_name_lock`](drm-internals.html#c.drm_device "drm_device").

`dev`
:   DRM dev this object belongs to.

`filp`
:   SHMEM file node used as backing storage for swappable buffer objects.
    GEM also supports driver private objects with driver-specific backing
    storage (contiguous DMA memory, special reserved blocks). In this
    case **filp** is NULL.

`vma_node`
:   Mapping info for this object to support mmap. Drivers are supposed to
    allocate the mmap offset using [`drm_gem_create_mmap_offset()`](#c.drm_gem_create_mmap_offset "drm_gem_create_mmap_offset"). The
    offset itself can be retrieved using [`drm_vma_node_offset_addr()`](#c.drm_vma_node_offset_addr "drm_vma_node_offset_addr").

    Memory mapping itself is handled by [`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap"), which also checks
    that userspace is allowed to access the object.

`size`
:   Size of the object, in bytes. Immutable over the object’s
    lifetime.

`name`
:   Global name for this object, starts at 1. 0 means unnamed.
    Access is covered by [`drm_device.object_name_lock`](drm-internals.html#c.drm_device "drm_device"). This is used by
    the GEM\_FLINK and GEM\_OPEN ioctls.

`dma_buf`
:   dma-buf associated with this GEM object.

    Pointer to the dma-buf associated with this gem object (either
    through importing or exporting). We break the resulting reference
    loop when the last gem handle for this object is released.

    Protected by [`drm_device.object_name_lock`](drm-internals.html#c.drm_device "drm_device").

`import_attach`
:   dma-buf attachment backing this object.

    Any foreign dma\_buf imported as a gem object has this set to the
    attachment point for the device. This is invariant over the lifetime
    of a gem object.

    The [`drm_gem_object_funcs.free`](#c.drm_gem_object_funcs "drm_gem_object_funcs") callback is responsible for
    cleaning up the dma\_buf attachment and references acquired at import
    time.

    Note that the drm gem/prime core does not depend upon drivers setting
    this field any more. So for drivers where this doesn’t make sense
    (e.g. virtual devices or a displaylink behind an usb bus) they can
    simply leave it as NULL.

`resv`
:   Pointer to reservation object associated with the this GEM object.

    Normally (**resv** == &\*\*\_resv\*\*) except for imported GEM objects.

`_resv`
:   A reservation object for this GEM object.

    This is unused for imported GEM objects.

`gpuva`
:   Fields used by GPUVM to manage mappings pointing to this GEM object.

    When DRM\_GPUVM\_IMMEDIATE\_MODE is set, this list is protected by the
    mutex. Otherwise, the list is protected by the GEMs [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") lock.

    Note that all entries in this list must agree on whether
    DRM\_GPUVM\_IMMEDIATE\_MODE is set.

`gpuva.list`
:   list of GPUVM mappings attached to this GEM object.

    Drivers should lock list accesses with either the GEMs
    [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") lock ([`drm_gem_object.resv`](#c.drm_gem_object "drm_gem_object")) or the
    [`drm_gem_object.gpuva`](#c.drm_gem_object "drm_gem_object").lock mutex.

`gpuva.lock`
:   lock protecting access to [`drm_gem_object.gpuva`](#c.drm_gem_object "drm_gem_object").list
    when DRM\_GPUVM\_IMMEDIATE\_MODE is used.

    Only used when DRM\_GPUVM\_IMMEDIATE\_MODE is set. It should be
    safe to take this mutex during the fence signalling path, so
    do not allocate memory while holding this lock. Otherwise,
    the [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") lock should be used.

`funcs`
:   Optional GEM object functions. If this is set, it will be used instead of the
    corresponding [`drm_driver`](drm-internals.html#c.drm_driver "drm_driver") GEM callbacks.

    New drivers should use this.

`lru_node`
:   List node in a [`drm_gem_lru`](#c.drm_gem_lru "drm_gem_lru").

`lru`
:   The current LRU list that the GEM object is on.

    Access to this field must be done with drm\_device::gem\_lru\_mutex
    held.

**Description**

This structure defines the generic parts for GEM buffer objects, which are
mostly around handling mmap and userspace handles.

Buffer objects are often abbreviated to BO.

DRM\_GEM\_FOPS

`DRM_GEM_FOPS`

> > Default drm GEM file operations
>
> **Description**
>
> This macro provides a shorthand for setting the GEM file ops in the
> `file_operations` structure. If all you need are the default ops, use
> DEFINE\_DRM\_GEM\_FOPS instead.

DEFINE\_DRM\_GEM\_FOPS

`DEFINE_DRM_GEM_FOPS (name)`

> macro to generate file operations for GEM drivers

**Parameters**

`name`
:   name for the generated structure

**Description**

This macro autogenerates a suitable `struct file_operations` for GEM based
drivers, which can be assigned to [`drm_driver.fops`](drm-internals.html#c.drm_driver "drm_driver"). Note that this structure
cannot be shared between drivers, because it contains a reference to the
current module using THIS\_MODULE.

Note that the declaration is already marked as static - if you need a
non-static version of this you’re probably doing it wrong and will break the
THIS\_MODULE reference by accident.

struct vfsmount \*drm\_gem\_get\_huge\_mnt(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Get the huge tmpfs mountpoint used by a DRM device

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This function gets the huge tmpfs mountpoint used by DRM device **dev**. A huge
tmpfs mountpoint is used instead of shm\_mnt after a successful call to
[`drm_gem_huge_mnt_create()`](#c.drm_gem_huge_mnt_create "drm_gem_huge_mnt_create") when CONFIG\_TRANSPARENT\_HUGEPAGE is enabled.

**Return**

The huge tmpfs mountpoint in use, NULL otherwise.

void drm\_gem\_object\_get(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   acquire a GEM buffer object reference

**Parameters**

`struct drm_gem_object *obj`
:   GEM buffer object

**Description**

This function acquires an additional reference to **obj**. It is illegal to
call this without already holding a reference. No locks required.

void drm\_gem\_object\_put(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   drop a GEM buffer object reference

**Parameters**

`struct drm_gem_object *obj`
:   GEM buffer object

**Description**

This releases a reference to **obj**.

bool drm\_gem\_object\_is\_shared\_for\_memory\_stats(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   helper for shared memory stats

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

**Description**

This helper should only be used for fdinfo shared memory stats to determine
if a GEM object is shared.

bool drm\_gem\_is\_imported(const struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   Tests if GEM object’s buffer has been imported

**Parameters**

`const struct drm_gem_object *obj`
:   the GEM object

**Return**

True if the GEM object’s buffer has been imported, false otherwise

void drm\_gem\_gpuva\_init(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   initialize the gpuva list of a GEM object

**Parameters**

`struct drm_gem_object *obj`
:   the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

**Description**

This initializes the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")’s [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") list.

Calling this function is only necessary for drivers intending to support the
[`drm_driver_feature`](drm-internals.html#c.drm_driver_feature "drm_driver_feature") DRIVER\_GEM\_GPUVA.

See also `drm_gem_gpuva_set_lock()`.

drm\_gem\_for\_each\_gpuvm\_bo

`drm_gem_for_each_gpuvm_bo (entry__, obj__)`

> iterator to walk over a list of [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")

**Parameters**

`entry__`
:   [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") structure to assign to in each iteration step

`obj__`
:   the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to walk are associated with

**Description**

This iterator walks over all [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") structures associated with the
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object").

drm\_gem\_for\_each\_gpuvm\_bo\_safe

`drm_gem_for_each_gpuvm_bo_safe (entry__, next__, obj__)`

> iterator to safely walk over a list of [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")

**Parameters**

`entry__`
:   `drm_gpuvm_bostructure` to assign to in each iteration step

`next__`
:   `next` [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to store the next step

`obj__`
:   the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to walk are associated with

**Description**

This iterator walks over all [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") structures associated with the
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object"). It is implemented with [`list_for_each_entry_safe()`](../core-api/list.html#c.list_for_each_entry_safe "list_for_each_entry_safe"), hence
it is save against removal of elements.

int drm\_gem\_huge\_mnt\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const char \*value)
:   Create, mount and use a huge tmpfs mountpoint

**Parameters**

`struct drm_device *dev`
:   DRM device that will use the huge tmpfs mountpoint

`const char *value`
:   huge tmpfs mount option value

**Description**

This function creates and mounts a dedicated huge tmpfs mountpoint for the
lifetime of the DRM device **dev** which is used at GEM object initialization
with [`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init").

The most common option for **value** is “within\_size” which only allocates huge
pages if the page will be fully within the GEM object size. “always”,
“advise” and “never” are supported too but the latter would just create a
mountpoint similar to the default one (shm\_mnt). See shmemfs and
Transparent Hugepage for more information.

**Return**

0 on success or a negative error code on failure.

int drm\_gem\_object\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, size\_t size)
:   initialize an allocated shmem-backed GEM object

**Parameters**

`struct drm_device *dev`
:   drm\_device the object should be initialized for

`struct drm_gem_object *obj`
:   drm\_gem\_object to initialize

`size_t size`
:   object size

**Description**

Initialize an already allocated GEM object of the specified size with
shmfs backing store. A huge mountpoint can be used by calling
[`drm_gem_huge_mnt_create()`](#c.drm_gem_huge_mnt_create "drm_gem_huge_mnt_create") beforehand.

void drm\_gem\_private\_object\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, size\_t size)
:   initialize an allocated private GEM object

**Parameters**

`struct drm_device *dev`
:   drm\_device the object should be initialized for

`struct drm_gem_object *obj`
:   drm\_gem\_object to initialize

`size_t size`
:   object size

**Description**

Initialize an already allocated GEM object of the specified size with
no GEM provided backing store. Instead the caller is responsible for
backing the object and handling it.

void drm\_gem\_private\_object\_fini(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   Finalize a failed drm\_gem\_object

**Parameters**

`struct drm_gem_object *obj`
:   drm\_gem\_object

**Description**

Uninitialize an already allocated GEM object when it initialized failed

int drm\_gem\_handle\_delete(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*filp, u32 handle)
:   deletes the given file-private handle

**Parameters**

`struct drm_file *filp`
:   drm file-private structure to use for the handle look up

`u32 handle`
:   userspace handle to delete

**Description**

Removes the GEM handle from the **filp** lookup table which has been added with
[`drm_gem_handle_create()`](#c.drm_gem_handle_create "drm_gem_handle_create"). If this is the last handle also cleans up linked
resources like GEM names.

int drm\_gem\_dumb\_map\_offset(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 handle, u64 \*offset)
:   return the fake mmap offset for a gem object

**Parameters**

`struct drm_file *file`
:   drm file-private structure containing the gem object

`struct drm_device *dev`
:   corresponding drm\_device

`u32 handle`
:   gem object handle

`u64 *offset`
:   return location for the fake mmap offset

**Description**

This implements the [`drm_driver.dumb_map_offset`](drm-internals.html#c.drm_driver "drm_driver") kms driver callback for
drivers which use gem to manage their backing storage.

**Return**

0 on success or a negative error code on failure.

int drm\_gem\_handle\_create(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, u32 \*handlep)
:   create a gem handle for an object

**Parameters**

`struct drm_file *file_priv`
:   drm file-private structure to register the handle for

`struct drm_gem_object *obj`
:   object to register

`u32 *handlep`
:   pointer to return the created handle to the caller

**Description**

Create a handle for this object. This adds a handle reference to the object,
which includes a regular reference count. Callers will likely want to
dereference the object afterwards.

Since this publishes **obj** to userspace it must be fully set up by this point,
drivers must call this last in their buffer object creation callbacks.

void drm\_gem\_free\_mmap\_offset(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   release a fake mmap offset for an object

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

**Description**

This routine frees fake offsets allocated by [`drm_gem_create_mmap_offset()`](#c.drm_gem_create_mmap_offset "drm_gem_create_mmap_offset").

Note that [`drm_gem_object_release()`](#c.drm_gem_object_release "drm_gem_object_release") already calls this function, so drivers
don’t have to take care of releasing the mmap offset themselves when freeing
the GEM object.

int drm\_gem\_create\_mmap\_offset\_size(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, size\_t size)
:   create a fake mmap offset for an object

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

`size_t size`
:   the virtual size

**Description**

GEM memory mapping works by handing back to userspace a fake mmap offset
it can use in a subsequent mmap(2) call. The DRM core code then looks
up the object based on the offset and sets up the various memory mapping
structures.

This routine allocates and attaches a fake offset for **obj**, in cases where
the virtual size differs from the physical size (ie. [`drm_gem_object.size`](#c.drm_gem_object "drm_gem_object")).
Otherwise just use [`drm_gem_create_mmap_offset()`](#c.drm_gem_create_mmap_offset "drm_gem_create_mmap_offset").

This function is idempotent and handles an already allocated mmap offset
transparently. Drivers do not need to check for this case.

int drm\_gem\_create\_mmap\_offset(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   create a fake mmap offset for an object

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

**Description**

GEM memory mapping works by handing back to userspace a fake mmap offset
it can use in a subsequent mmap(2) call. The DRM core code then looks
up the object based on the offset and sets up the various memory mapping
structures.

This routine allocates and attaches a fake offset for **obj**.

Drivers can call [`drm_gem_free_mmap_offset()`](#c.drm_gem_free_mmap_offset "drm_gem_free_mmap_offset") before freeing **obj** to release
the fake offset again.

struct page \*\*drm\_gem\_get\_pages(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   helper to allocate backing pages for a GEM object from shmem

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

**Description**

This reads the page-array of the shmem-backing storage of the given gem
object. An array of pages is returned. If a page is not allocated or
swapped-out, this will allocate/swap-in the required pages. Note that the
whole object is covered by the page-array and pinned in memory.

Use [`drm_gem_put_pages()`](#c.drm_gem_put_pages "drm_gem_put_pages") to release the array and unpin all pages.

This uses the GFP-mask set on the shmem-mapping (see `mapping_set_gfp_mask()`).
If you require other GFP-masks, you have to do those allocations yourself.

Note that you are not allowed to change gfp-zones during runtime. That is,
`shmem_read_mapping_page_gfp()` must be called with the same gfp\_zone(gfp) as
set during initialization. If you have special zone constraints, set them
after [`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init") via `mapping_set_gfp_mask()`. shmem-core takes care
to keep pages in the required zone during swap-in.

This function is only valid on objects initialized with
[`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init"), but not for those initialized with
[`drm_gem_private_object_init()`](#c.drm_gem_private_object_init "drm_gem_private_object_init") only.

void drm\_gem\_put\_pages(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, struct page \*\*pages, bool dirty, bool accessed)
:   helper to free backing pages for a GEM object

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

`struct page **pages`
:   pages to free

`bool dirty`
:   if true, pages will be marked as dirty

`bool accessed`
:   if true, the pages will be marked as accessed

int drm\_gem\_objects\_lookup(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*filp, void \_\_user \*bo\_handles, int count, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*\*\*objs\_out)
:   look up GEM objects from an array of handles

**Parameters**

`struct drm_file *filp`
:   DRM file private date

`void __user *bo_handles`
:   user pointer to array of userspace handle

`int count`
:   size of handle array

`struct drm_gem_object ***objs_out`
:   returned pointer to array of drm\_gem\_object pointers

**Description**

Takes an array of userspace handles and returns a newly allocated array of
GEM objects.

After a successful lookup GEM objects need to be released using
[`drm_gem_object_put()`](#c.drm_gem_object_put "drm_gem_object_put") and the array returned in **objs\_out** must be freed using
[`kvfree()`](../core-api/mm-api.html#c.kvfree "kvfree").

For a single handle lookup, use [`drm_gem_object_lookup()`](#c.drm_gem_object_lookup "drm_gem_object_lookup").

**Return**

Zero on success or a negative error code.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_object\_lookup(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*filp, u32 handle)
:   look up a GEM object from its handle

**Parameters**

`struct drm_file *filp`
:   DRM file private date

`u32 handle`
:   userspace handle

**Description**

If looking up an array of handles, use [`drm_gem_objects_lookup()`](#c.drm_gem_objects_lookup "drm_gem_objects_lookup").

**Return**

A reference to the object named by the handle if such exists on **filp**, NULL
otherwise.

long drm\_gem\_dma\_resv\_wait(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*filep, u32 handle, bool wait\_all, unsigned long timeout)
:   Wait on GEM object’s reservation’s objects shared and/or exclusive fences.

**Parameters**

`struct drm_file *filep`
:   DRM file private date

`u32 handle`
:   userspace handle

`bool wait_all`
:   if true, wait on all fences, else wait on just exclusive fence

`unsigned long timeout`
:   timeout value in jiffies or zero to return immediately

**Return**

Returns -ERESTARTSYS if interrupted, 0 if the wait timed out, or
greater than 0 on success.

void drm\_gem\_object\_release(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   release GEM buffer object resources

**Parameters**

`struct drm_gem_object *obj`
:   GEM buffer object

**Description**

This releases any structures and resources used by **obj** and is the inverse of
[`drm_gem_object_init()`](#c.drm_gem_object_init "drm_gem_object_init").

void drm\_gem\_object\_free(struct [kref](#c.drm_gem_object_free "kref") \*kref)
:   free a GEM object

**Parameters**

`struct kref *kref`
:   kref of the object to free

**Description**

Called after the last reference to the object has been lost.

Frees the object

void drm\_gem\_vm\_open(struct vm\_area\_struct \*vma)
:   vma->ops->open implementation for GEM

**Parameters**

`struct vm_area_struct *vma`
:   VM area structure

**Description**

This function implements the #vm\_operations\_struct open() callback for GEM
drivers. This must be used together with [`drm_gem_vm_close()`](#c.drm_gem_vm_close "drm_gem_vm_close").

void drm\_gem\_vm\_close(struct vm\_area\_struct \*vma)
:   vma->ops->close implementation for GEM

**Parameters**

`struct vm_area_struct *vma`
:   VM area structure

**Description**

This function implements the #vm\_operations\_struct close() callback for GEM
drivers. This must be used together with [`drm_gem_vm_open()`](#c.drm_gem_vm_open "drm_gem_vm_open").

int drm\_gem\_mmap\_obj(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, unsigned long obj\_size, struct vm\_area\_struct \*vma)
:   memory map a GEM object

**Parameters**

`struct drm_gem_object *obj`
:   the GEM object to map

`unsigned long obj_size`
:   the object size to be mapped, in bytes

`struct vm_area_struct *vma`
:   VMA for the area to be mapped

**Description**

Set up the VMA to prepare mapping of the GEM object using the GEM object’s
vm\_ops. Depending on their requirements, GEM objects can either
provide a fault handler in their vm\_ops (in which case any accesses to
the object will be trapped, to perform migration, GTT binding, surface
register allocation, or performance monitoring), or mmap the buffer memory
synchronously after calling drm\_gem\_mmap\_obj.

This function is mainly intended to implement the DMABUF mmap operation, when
the GEM object is not looked up based on its fake offset. To implement the
DRM mmap operation, drivers should use the [`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap") function.

[`drm_gem_mmap_obj()`](#c.drm_gem_mmap_obj "drm_gem_mmap_obj") assumes the user is granted access to the buffer while
[`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap") prevents unprivileged users from mapping random objects. So
callers must verify access restrictions before calling this helper.

Return 0 or success or -EINVAL if the object size is smaller than the VMA
size, or if no vm\_ops are provided.

unsigned long drm\_gem\_get\_unmapped\_area(struct [file](../filesystems/api-summary.html#c.file "file") \*filp, unsigned long uaddr, unsigned long len, unsigned long pgoff, unsigned long flags)
:   get memory mapping region routine for GEM objects

**Parameters**

`struct file *filp`
:   DRM file pointer

`unsigned long uaddr`
:   User address hint

`unsigned long len`
:   Mapping length

`unsigned long pgoff`
:   Offset (in pages)

`unsigned long flags`
:   Mapping flags

**Description**

If a driver supports GEM object mapping, before ending up in [`drm_gem_mmap()`](#c.drm_gem_mmap "drm_gem_mmap"),
mmap calls on the DRM file descriptor will first try to find a free linear
address space large enough for a mapping. Since GEM objects are backed by
shmem buffers, this should preferably be handled by the shmem virtual memory
filesystem which can appropriately align addresses to huge page sizes when
needed.

Look up the GEM object based on the offset passed in (vma->vm\_pgoff will
contain the fake offset we created) and call `shmem_get_unmapped_area()` with
the right file pointer.

If a GEM object is not available at the given offset or if the caller is not
granted access to it, fall back to `mm_get_unmapped_area()`.

int drm\_gem\_mmap(struct [file](../filesystems/api-summary.html#c.file "file") \*filp, struct vm\_area\_struct \*vma)
:   memory map routine for GEM objects

**Parameters**

`struct file *filp`
:   DRM file pointer

`struct vm_area_struct *vma`
:   VMA for the area to be mapped

**Description**

If a driver supports GEM object mapping, mmap calls on the DRM file
descriptor will end up here.

Look up the GEM object based on the offset passed in (vma->vm\_pgoff will
contain the fake offset we created) and map it with a call to
[`drm_gem_mmap_obj()`](#c.drm_gem_mmap_obj "drm_gem_mmap_obj").

If the caller is not granted access to the buffer object, the mmap will fail
with EACCES. Please see the vma manager for more information.

int drm\_gem\_lock\_reservations(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*\*objs, int count, struct ww\_acquire\_ctx \*acquire\_ctx)
:   Sets up the ww context and acquires the lock on an array of GEM objects.

**Parameters**

`struct drm_gem_object **objs`
:   drm\_gem\_objects to lock

`int count`
:   Number of objects in **objs**

`struct ww_acquire_ctx *acquire_ctx`
:   `struct ww_acquire_ctx` that will be initialized as
    part of tracking this set of locked reservations.

**Description**

Once you’ve locked your reservations, you’ll want to set up space
for your shared fences (if applicable), submit your job, then
`drm_gem_unlock_reservations()`.

void drm\_gem\_lru\_init(struct [drm\_gem\_lru](#c.drm_gem_lru "drm_gem_lru") \*lru)
:   initialize a LRU

**Parameters**

`struct drm_gem_lru *lru`
:   The LRU to initialize

void drm\_gem\_lru\_remove(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   remove object from whatever LRU it is in

**Parameters**

`struct drm_gem_object *obj`
:   The GEM object to remove from current LRU

**Description**

If the object is currently in any LRU, remove it.

void drm\_gem\_lru\_move\_tail\_locked(struct [drm\_gem\_lru](#c.drm_gem_lru "drm_gem_lru") \*lru, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   move the object to the tail of the LRU

**Parameters**

`struct drm_gem_lru *lru`
:   The LRU to move the object into.

`struct drm_gem_object *obj`
:   The GEM object to move into this LRU

**Description**

Like [`drm_gem_lru_move_tail`](#c.drm_gem_lru_move_tail "drm_gem_lru_move_tail") but lru lock must be held

void drm\_gem\_lru\_move\_tail(struct [drm\_gem\_lru](#c.drm_gem_lru "drm_gem_lru") \*lru, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   move the object to the tail of the LRU

**Parameters**

`struct drm_gem_lru *lru`
:   The LRU to move the object into.

`struct drm_gem_object *obj`
:   The GEM object to move into this LRU

**Description**

If the object is already in this LRU it will be moved to the
tail. Otherwise it will be removed from whichever other LRU
it is in (if any) and moved into this LRU.

unsigned long drm\_gem\_lru\_scan(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_gem\_lru](#c.drm_gem_lru "drm_gem_lru") \*lru, unsigned int nr\_to\_scan, unsigned long \*remaining, bool (\*shrink)(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, struct ww\_acquire\_ctx \*ticket), struct ww\_acquire\_ctx \*ticket)
:   helper to implement shrinker.scan\_objects

**Parameters**

`struct drm_device *dev`
:   DRM device the LRU belongs to

`struct drm_gem_lru *lru`
:   The LRU to scan

`unsigned int nr_to_scan`
:   The number of pages to try to reclaim

`unsigned long *remaining`
:   The number of pages left to reclaim, should be initialized by caller

`bool (*shrink)(struct drm_gem_object *obj, struct ww_acquire_ctx *ticket)`
:   Callback to try to shrink/reclaim the object.

`struct ww_acquire_ctx *ticket`
:   Optional ww\_acquire\_ctx context to use for locking

**Description**

If the shrink callback succeeds, it is expected that the driver
move the object out of this LRU.

If the LRU possibly contain active buffers, it is the responsibility
of the shrink callback to check for this (ie. [`dma_resv_test_signaled()`](../driver-api/dma-buf.html#c.dma_resv_test_signaled "dma_resv_test_signaled"))
or if necessary block until the buffer becomes idle.

int drm\_gem\_evict\_locked(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   helper to evict backing pages for a GEM object

**Parameters**

`struct drm_gem_object *obj`
:   obj in question

### GEM DMA Helper Functions Reference

The DRM GEM/DMA helpers are a means to provide buffer objects that are
presented to the device as a contiguous chunk of memory. This is useful
for devices that do not support scatter-gather DMA (either directly or
by using an intimately attached IOMMU).

For devices that access the memory bus through an (external) IOMMU then
the buffer objects are allocated using a traditional page-based
allocator and may be scattered through physical memory. However they
are contiguous in the IOVA space so appear contiguous to devices using
them.

For other devices then the helpers rely on CMA to provide buffer
objects that are physically contiguous in memory.

For GEM callback helpers in struct [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") functions, see likewise
named functions with an \_object\_ infix (e.g., `drm_gem_dma_object_vmap()` wraps
[`drm_gem_dma_vmap()`](#c.drm_gem_dma_vmap "drm_gem_dma_vmap")). These helpers perform the necessary type conversion.

struct drm\_gem\_dma\_object
:   GEM object backed by DMA memory allocations

**Definition**:

```
struct drm_gem_dma_object {
    struct drm_gem_object base;
    dma_addr_t dma_addr;
    struct sg_table *sgt;
    void *vaddr;
    bool map_noncoherent;
};
```

**Members**

`base`
:   base GEM object

`dma_addr`
:   DMA address of the backing memory

`sgt`
:   scatter/gather table for imported PRIME buffers. The table can have
    more than one entry but they are guaranteed to have contiguous
    DMA addresses.

`vaddr`
:   kernel virtual address of the backing memory

`map_noncoherent`
:   if true, the GEM object is backed by non-coherent memory

void drm\_gem\_dma\_object\_free(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   GEM object function for [`drm_gem_dma_free()`](#c.drm_gem_dma_free "drm_gem_dma_free")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object to free

**Description**

This function wraps `drm_gem_dma_free_object()`. Drivers that employ the DMA helpers
should use it as their [`drm_gem_object_funcs.free`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

void drm\_gem\_dma\_object\_print\_info(struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, unsigned int indent, const struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   Print [`drm_gem_dma_object`](#c.drm_gem_dma_object "drm_gem_dma_object") info for debugfs

**Parameters**

`struct drm_printer *p`
:   DRM printer

`unsigned int indent`
:   Tab indentation level

`const struct drm_gem_object *obj`
:   GEM object

**Description**

This function wraps [`drm_gem_dma_print_info()`](#c.drm_gem_dma_print_info "drm_gem_dma_print_info"). Drivers that employ the DMA helpers
should use this function as their [`drm_gem_object_funcs.print_info`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

struct sg\_table \*drm\_gem\_dma\_object\_get\_sg\_table(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   GEM object function for [`drm_gem_dma_get_sg_table()`](#c.drm_gem_dma_get_sg_table "drm_gem_dma_get_sg_table")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

**Description**

This function wraps [`drm_gem_dma_get_sg_table()`](#c.drm_gem_dma_get_sg_table "drm_gem_dma_get_sg_table"). Drivers that employ the DMA helpers should
use it as their [`drm_gem_object_funcs.get_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

**Return**

A pointer to the scatter/gather table of pinned pages or NULL on failure.

int drm\_gem\_dma\_object\_mmap(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, struct vm\_area\_struct \*vma)
:   GEM object function for [`drm_gem_dma_mmap()`](#c.drm_gem_dma_mmap "drm_gem_dma_mmap")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

`struct vm_area_struct *vma`
:   VMA for the area to be mapped

**Description**

This function wraps [`drm_gem_dma_mmap()`](#c.drm_gem_dma_mmap "drm_gem_dma_mmap"). Drivers that employ the dma helpers should
use it as their [`drm_gem_object_funcs.mmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

**Return**

0 on success or a negative error code on failure.

DRM\_GEM\_DMA\_DRIVER\_OPS\_WITH\_DUMB\_CREATE

`DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE (dumb_create_func)`

> DMA GEM driver operations

**Parameters**

`dumb_create_func`
:   callback function for .dumb\_create

**Description**

This macro provides a shortcut for setting the default GEM operations in the
[`drm_driver`](drm-internals.html#c.drm_driver "drm_driver") structure.

This macro is a variant of DRM\_GEM\_DMA\_DRIVER\_OPS for drivers that
override the default implementation of `struct rm_driver`.dumb\_create. Use
DRM\_GEM\_DMA\_DRIVER\_OPS if possible. Drivers that require a virtual address
on imported buffers should use
[`DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE()`](#c.DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE "DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE") instead.

DRM\_GEM\_DMA\_DRIVER\_OPS

`DRM_GEM_DMA_DRIVER_OPS`

> > DMA GEM driver operations
>
> **Description**
>
> This macro provides a shortcut for setting the default GEM operations in the
> [`drm_driver`](drm-internals.html#c.drm_driver "drm_driver") structure.
>
> Drivers that come with their own implementation of
> [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create should use
> [`DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE()`](#c.DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE "DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE") instead. Use
> DRM\_GEM\_DMA\_DRIVER\_OPS if possible. Drivers that require a virtual address
> on imported buffers should use DRM\_GEM\_DMA\_DRIVER\_OPS\_VMAP instead.

DRM\_GEM\_DMA\_DRIVER\_OPS\_VMAP\_WITH\_DUMB\_CREATE

`DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE (dumb_create_func)`

> DMA GEM driver operations ensuring a virtual address on the buffer

**Parameters**

`dumb_create_func`
:   callback function for .dumb\_create

**Description**

This macro provides a shortcut for setting the default GEM operations in the
[`drm_driver`](drm-internals.html#c.drm_driver "drm_driver") structure for drivers that need the virtual address also on
imported buffers.

This macro is a variant of DRM\_GEM\_DMA\_DRIVER\_OPS\_VMAP for drivers that
override the default implementation of [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create. Use
DRM\_GEM\_DMA\_DRIVER\_OPS\_VMAP if possible. Drivers that do not require a
virtual address on imported buffers should use
[`DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE()`](#c.DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE "DRM_GEM_DMA_DRIVER_OPS_WITH_DUMB_CREATE") instead.

DRM\_GEM\_DMA\_DRIVER\_OPS\_VMAP

`DRM_GEM_DMA_DRIVER_OPS_VMAP`

> > DMA GEM driver operations ensuring a virtual address on the buffer
>
> **Description**
>
> This macro provides a shortcut for setting the default GEM operations in the
> [`drm_driver`](drm-internals.html#c.drm_driver "drm_driver") structure for drivers that need the virtual address also on
> imported buffers.
>
> Drivers that come with their own implementation of
> [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create should use
> [`DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE()`](#c.DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE "DRM_GEM_DMA_DRIVER_OPS_VMAP_WITH_DUMB_CREATE") instead. Use
> DRM\_GEM\_DMA\_DRIVER\_OPS\_VMAP if possible. Drivers that do not require a
> virtual address on imported buffers should use DRM\_GEM\_DMA\_DRIVER\_OPS
> instead.

DEFINE\_DRM\_GEM\_DMA\_FOPS

`DEFINE_DRM_GEM_DMA_FOPS (name)`

> macro to generate file operations for DMA drivers

**Parameters**

`name`
:   name for the generated structure

**Description**

This macro autogenerates a suitable `struct file_operations` for DMA based
drivers, which can be assigned to [`drm_driver.fops`](drm-internals.html#c.drm_driver "drm_driver"). Note that this structure
cannot be shared between drivers, because it contains a reference to the
current module using THIS\_MODULE.

Note that the declaration is already marked as static - if you need a
non-static version of this you’re probably doing it wrong and will break the
THIS\_MODULE reference by accident.

struct [drm\_gem\_dma\_object](#c.drm_gem_dma_object "drm_gem_dma_object") \*drm\_gem\_dma\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, size\_t size)
:   allocate an object with the given size

**Parameters**

`struct drm_device *drm`
:   DRM device

`size_t size`
:   size of the object to allocate

**Description**

This function creates a DMA GEM object and allocates memory as backing store.
The allocated memory will occupy a contiguous chunk of bus address space.

For devices that are directly connected to the memory bus then the allocated
memory will be physically contiguous. For devices that access through an
IOMMU, then the allocated memory is not expected to be physically contiguous
because having contiguous IOVAs is sufficient to meet a devices DMA
requirements.

**Return**

A [`struct drm_gem_dma_object`](#c.drm_gem_dma_object "drm_gem_dma_object") \* on success or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded negative
error code on failure.

void drm\_gem\_dma\_free(struct [drm\_gem\_dma\_object](#c.drm_gem_dma_object "drm_gem_dma_object") \*dma\_obj)
:   free resources associated with a DMA GEM object

**Parameters**

`struct drm_gem_dma_object *dma_obj`
:   DMA GEM object to free

**Description**

This function frees the backing memory of the DMA GEM object, cleans up the
GEM object state and frees the memory used to store the object itself.
If the buffer is imported and the virtual address is set, it is released.

int drm\_gem\_dma\_dumb\_create\_internal(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct [drm\_mode\_create\_dumb](drm-uapi.html#c.drm_mode_create_dumb "drm_mode_create_dumb") \*args)
:   create a dumb buffer object

**Parameters**

`struct drm_file *file_priv`
:   DRM file-private structure to create the dumb buffer for

`struct drm_device *drm`
:   DRM device

`struct drm_mode_create_dumb *args`
:   IOCTL data

**Description**

This aligns the pitch and size arguments to the minimum required. This is
an internal helper that can be wrapped by a driver to account for hardware
with more specific alignment requirements. It should not be used directly
as their [`drm_driver.dumb_create`](drm-internals.html#c.drm_driver "drm_driver") callback.

**Return**

0 on success or a negative error code on failure.

int drm\_gem\_dma\_dumb\_create(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct [drm\_mode\_create\_dumb](drm-uapi.html#c.drm_mode_create_dumb "drm_mode_create_dumb") \*args)
:   create a dumb buffer object

**Parameters**

`struct drm_file *file_priv`
:   DRM file-private structure to create the dumb buffer for

`struct drm_device *drm`
:   DRM device

`struct drm_mode_create_dumb *args`
:   IOCTL data

**Description**

This function computes the pitch of the dumb buffer and rounds it up to an
integer number of bytes per pixel. Drivers for hardware that doesn’t have
any additional restrictions on the pitch can directly use this function as
their [`drm_driver.dumb_create`](drm-internals.html#c.drm_driver "drm_driver") callback.

For hardware with additional restrictions, drivers can adjust the fields
set up by userspace and pass the IOCTL data along to the
[`drm_gem_dma_dumb_create_internal()`](#c.drm_gem_dma_dumb_create_internal "drm_gem_dma_dumb_create_internal") function.

**Return**

0 on success or a negative error code on failure.

unsigned long drm\_gem\_dma\_get\_unmapped\_area(struct [file](../filesystems/api-summary.html#c.file "file") \*filp, unsigned long addr, unsigned long len, unsigned long pgoff, unsigned long flags)
:   propose address for mapping in noMMU cases

**Parameters**

`struct file *filp`
:   file object

`unsigned long addr`
:   memory address

`unsigned long len`
:   buffer size

`unsigned long pgoff`
:   page offset

`unsigned long flags`
:   memory flags

**Description**

This function is used in noMMU platforms to propose address mapping
for a given buffer.
It’s intended to be used as a direct handler for the struct
`file_operations.get_unmapped_area` operation.

**Return**

mapping address on success or a negative error code on failure.

void drm\_gem\_dma\_print\_info(const struct [drm\_gem\_dma\_object](#c.drm_gem_dma_object "drm_gem_dma_object") \*dma\_obj, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, unsigned int indent)
:   Print [`drm_gem_dma_object`](#c.drm_gem_dma_object "drm_gem_dma_object") info for debugfs

**Parameters**

`const struct drm_gem_dma_object *dma_obj`
:   DMA GEM object

`struct drm_printer *p`
:   DRM printer

`unsigned int indent`
:   Tab indentation level

**Description**

This function prints dma\_addr and vaddr for use in e.g. debugfs output.

struct sg\_table \*drm\_gem\_dma\_get\_sg\_table(struct [drm\_gem\_dma\_object](#c.drm_gem_dma_object "drm_gem_dma_object") \*dma\_obj)
:   provide a scatter/gather table of pinned pages for a DMA GEM object

**Parameters**

`struct drm_gem_dma_object *dma_obj`
:   DMA GEM object

**Description**

This function exports a scatter/gather table by calling the standard
DMA mapping API.

**Return**

A pointer to the scatter/gather table of pinned pages or NULL on failure.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_dma\_prime\_import\_sg\_table(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct sg\_table \*sgt)
:   produce a DMA GEM object from another driver’s scatter/gather table of pinned pages

**Parameters**

`struct drm_device *dev`
:   device to import into

`struct dma_buf_attachment *attach`
:   DMA-BUF attachment

`struct sg_table *sgt`
:   scatter/gather table of pinned pages

**Description**

This function imports a scatter/gather table exported via DMA-BUF by
another driver. Imported buffers must be physically contiguous in memory
(i.e. the scatter/gather table must contain a single entry). Drivers that
use the DMA helpers should set this as their
[`drm_driver.gem_prime_import_sg_table`](drm-internals.html#c.drm_driver "drm_driver") callback.

**Return**

A pointer to a newly created GEM object or an ERR\_PTR-encoded negative
error code on failure.

int drm\_gem\_dma\_vmap(struct [drm\_gem\_dma\_object](#c.drm_gem_dma_object "drm_gem_dma_object") \*dma\_obj, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   map a DMA GEM object into the kernel’s virtual address space

**Parameters**

`struct drm_gem_dma_object *dma_obj`
:   DMA GEM object

`struct iosys_map *map`
:   Returns the kernel virtual address of the DMA GEM object’s backing
    store.

**Description**

This function maps a buffer into the kernel’s virtual address space.
Since the DMA buffers are already mapped into the kernel virtual address
space this simply returns the cached virtual address.

**Return**

0 on success, or a negative error code otherwise.

int drm\_gem\_dma\_mmap(struct [drm\_gem\_dma\_object](#c.drm_gem_dma_object "drm_gem_dma_object") \*dma\_obj, struct vm\_area\_struct \*vma)
:   memory-map an exported DMA GEM object

**Parameters**

`struct drm_gem_dma_object *dma_obj`
:   DMA GEM object

`struct vm_area_struct *vma`
:   VMA for the area to be mapped

**Description**

This function maps a buffer into a userspace process’s address space.
In addition to the usual GEM VMA setup it immediately faults in the entire
object instead of using on-demand faulting.

**Return**

0 on success or a negative error code on failure.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_dma\_prime\_import\_sg\_table\_vmap(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct sg\_table \*sgt)
:   PRIME import another driver’s scatter/gather table and get the virtual address of the buffer

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct dma_buf_attachment *attach`
:   DMA-BUF attachment

`struct sg_table *sgt`
:   Scatter/gather table of pinned pages

**Description**

This function imports a scatter/gather table using
[`drm_gem_dma_prime_import_sg_table()`](#c.drm_gem_dma_prime_import_sg_table "drm_gem_dma_prime_import_sg_table") and uses [`dma_buf_vmap()`](../driver-api/dma-buf.html#c.dma_buf_vmap "dma_buf_vmap") to get the kernel
virtual address. This ensures that a DMA GEM object always has its virtual
address set. This address is released when the object is freed.

This function can be used as the [`drm_driver.gem_prime_import_sg_table`](drm-internals.html#c.drm_driver "drm_driver")
callback. The [`DRM_GEM_DMA_DRIVER_OPS_VMAP`](#c.DRM_GEM_DMA_DRIVER_OPS_VMAP "DRM_GEM_DMA_DRIVER_OPS_VMAP") macro provides a shortcut to set
the necessary DRM driver operations.

**Return**

A pointer to a newly created GEM object or an ERR\_PTR-encoded negative
error code on failure.

### GEM SHMEM Helper Function Reference

This library provides helpers for GEM objects backed by shmem buffers
allocated using anonymous pageable memory.

Functions that operate on the GEM object receive struct [`drm_gem_shmem_object`](#c.drm_gem_shmem_object "drm_gem_shmem_object").
For GEM callback helpers in struct [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") functions, see likewise
named functions with an \_object\_ infix (e.g., `drm_gem_shmem_object_vmap()` wraps
`drm_gem_shmem_vmap()`). These helpers perform the necessary type conversion.

struct drm\_gem\_shmem\_object
:   GEM object backed by shmem

**Definition**:

```
struct drm_gem_shmem_object {
    struct drm_gem_object base;
    struct page **pages;
    refcount_t pages_use_count;
    refcount_t pages_pin_count;
    int madv;
    struct list_head madv_list;
    struct sg_table *sgt;
    void *vaddr;
    refcount_t vmap_use_count;
    bool pages_mark_dirty_on_put : 1;
    bool pages_mark_accessed_on_put : 1;
    bool map_wc : 1;
};
```

**Members**

`base`
:   Base GEM object

`pages`
:   Page table

`pages_use_count`
:   Reference count on the pages table.
    The pages are put when the count reaches zero.

`pages_pin_count`
:   Reference count on the pinned pages table.

    Pages are hard-pinned and reside in memory if count
    greater than zero. Otherwise, when count is zero, the pages are
    allowed to be evicted and purged by memory shrinker.

`madv`
:   State for madvise

    0 is active/inuse.
    A negative value is the object is purged.
    Positive values are driver specific and not used by the helpers.

`madv_list`
:   List entry for madvise tracking

    Typically used by drivers to track purgeable objects

`sgt`
:   Scatter/gather table for imported PRIME buffers

`vaddr`
:   Kernel virtual address of the backing memory

`vmap_use_count`
:   Reference count on the virtual address.
    The address are un-mapped when the count reaches zero.

`pages_mark_dirty_on_put`
:   Mark pages as dirty when they are put.

`pages_mark_accessed_on_put`
:   Mark pages as accessed when they are put.

`map_wc`
:   map object write-combined (instead of using shmem defaults).

void drm\_gem\_shmem\_object\_free(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   GEM object function for [`drm_gem_shmem_free()`](#c.drm_gem_shmem_free "drm_gem_shmem_free")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object to free

**Description**

This function wraps [`drm_gem_shmem_free()`](#c.drm_gem_shmem_free "drm_gem_shmem_free"). Drivers that employ the shmem helpers
should use it as their [`drm_gem_object_funcs.free`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

void drm\_gem\_shmem\_object\_print\_info(struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, unsigned int indent, const struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   Print [`drm_gem_shmem_object`](#c.drm_gem_shmem_object "drm_gem_shmem_object") info for debugfs

**Parameters**

`struct drm_printer *p`
:   DRM printer

`unsigned int indent`
:   Tab indentation level

`const struct drm_gem_object *obj`
:   GEM object

**Description**

This function wraps [`drm_gem_shmem_print_info()`](#c.drm_gem_shmem_print_info "drm_gem_shmem_print_info"). Drivers that employ the shmem helpers should
use this function as their [`drm_gem_object_funcs.print_info`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

int drm\_gem\_shmem\_object\_pin(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   GEM object function for [`drm_gem_shmem_pin()`](#c.drm_gem_shmem_pin "drm_gem_shmem_pin")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

**Description**

This function wraps [`drm_gem_shmem_pin()`](#c.drm_gem_shmem_pin "drm_gem_shmem_pin"). Drivers that employ the shmem helpers should
use it as their [`drm_gem_object_funcs.pin`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

void drm\_gem\_shmem\_object\_unpin(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   GEM object function for [`drm_gem_shmem_unpin()`](#c.drm_gem_shmem_unpin "drm_gem_shmem_unpin")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

**Description**

This function wraps [`drm_gem_shmem_unpin()`](#c.drm_gem_shmem_unpin "drm_gem_shmem_unpin"). Drivers that employ the shmem helpers should
use it as their [`drm_gem_object_funcs.unpin`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

struct sg\_table \*drm\_gem\_shmem\_object\_get\_sg\_table(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   GEM object function for [`drm_gem_shmem_get_sg_table()`](#c.drm_gem_shmem_get_sg_table "drm_gem_shmem_get_sg_table")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

**Description**

This function wraps [`drm_gem_shmem_get_sg_table()`](#c.drm_gem_shmem_get_sg_table "drm_gem_shmem_get_sg_table"). Drivers that employ the shmem helpers should
use it as their [`drm_gem_object_funcs.get_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

**Return**

A pointer to the scatter/gather table of pinned pages or error pointer on failure.

int drm\_gem\_shmem\_object\_mmap(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, struct vm\_area\_struct \*vma)
:   GEM object function for [`drm_gem_shmem_mmap()`](#c.drm_gem_shmem_mmap "drm_gem_shmem_mmap")

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

`struct vm_area_struct *vma`
:   VMA for the area to be mapped

**Description**

This function wraps [`drm_gem_shmem_mmap()`](#c.drm_gem_shmem_mmap "drm_gem_shmem_mmap"). Drivers that employ the shmem helpers should
use it as their [`drm_gem_object_funcs.mmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs") handler.

**Return**

0 on success or a negative error code on failure.

DRM\_GEM\_SHMEM\_DRIVER\_OPS

`DRM_GEM_SHMEM_DRIVER_OPS`

> > Default shmem GEM operations
>
> **Description**
>
> This macro provides a shortcut for setting the shmem GEM operations
> in the [`drm_driver`](drm-internals.html#c.drm_driver "drm_driver") structure. Drivers that do not require an s/g table
> for imported buffers should use this.

int drm\_gem\_shmem\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem, size\_t size)
:   Initialize an allocated object.

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object to initialize

`size_t size`
:   Buffer size in bytes

**Description**

This function initializes an allocated shmem GEM object.

**Return**

0 on success, or a negative error code on failure.

struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*drm\_gem\_shmem\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, size\_t size)
:   Allocate an object with the given size

**Parameters**

`struct drm_device *dev`
:   DRM device

`size_t size`
:   Size of the object to allocate

**Description**

This function creates a shmem GEM object.

**Return**

A [`struct drm_gem_shmem_object`](#c.drm_gem_shmem_object "drm_gem_shmem_object") \* on success or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded negative
error code on failure.

void drm\_gem\_shmem\_release(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem)
:   Release resources associated with a shmem GEM object.

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object

**Description**

This function cleans up the GEM object state, but does not free the memory used to store the
object itself. This function is meant to be a dedicated helper for the Rust GEM bindings.

void drm\_gem\_shmem\_free(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem)
:   Free resources associated with a shmem GEM object

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object to free

**Description**

This function cleans up the GEM object state and frees the memory used to
store the object itself.

int drm\_gem\_shmem\_pin(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem)
:   Pin backing pages for a shmem GEM object

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object

**Description**

This function makes sure the backing pages are pinned in memory while the
buffer is exported.

**Return**

0 on success or a negative error code on failure.

void drm\_gem\_shmem\_unpin(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem)
:   Unpin backing pages for a shmem GEM object

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object

**Description**

This function removes the requirement that the backing pages are pinned in
memory.

int drm\_gem\_shmem\_dumb\_create(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_mode\_create\_dumb](drm-uapi.html#c.drm_mode_create_dumb "drm_mode_create_dumb") \*args)
:   Create a dumb shmem buffer object

**Parameters**

`struct drm_file *file`
:   DRM file structure to create the dumb buffer for

`struct drm_device *dev`
:   DRM device

`struct drm_mode_create_dumb *args`
:   IOCTL data

**Description**

This function computes the pitch of the dumb buffer and rounds it up to an
integer number of bytes per pixel. Drivers for hardware that doesn’t have
any additional restrictions on the pitch can directly use this function as
their [`drm_driver.dumb_create`](drm-internals.html#c.drm_driver "drm_driver") callback.

For hardware with additional restrictions, drivers can adjust the fields
set up by userspace before calling into this function.

**Return**

0 on success or a negative error code on failure.

int drm\_gem\_shmem\_mmap(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem, struct vm\_area\_struct \*vma)
:   Memory-map a shmem GEM object

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object

`struct vm_area_struct *vma`
:   VMA for the area to be mapped

**Description**

This function implements an augmented version of the GEM DRM file mmap
operation for shmem objects.

**Return**

0 on success or a negative error code on failure.

void drm\_gem\_shmem\_print\_info(const struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, unsigned int indent)
:   Print [`drm_gem_shmem_object`](#c.drm_gem_shmem_object "drm_gem_shmem_object") info for debugfs

**Parameters**

`const struct drm_gem_shmem_object *shmem`
:   shmem GEM object

`struct drm_printer *p`
:   DRM printer

`unsigned int indent`
:   Tab indentation level

struct sg\_table \*drm\_gem\_shmem\_get\_sg\_table(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem)
:   Provide a scatter/gather table of pinned pages for a shmem GEM object

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object

**Description**

This function exports a scatter/gather table suitable for PRIME usage by
calling the standard DMA mapping API.

Drivers who need to acquire an scatter/gather table for objects need to call
[`drm_gem_shmem_get_pages_sgt()`](#c.drm_gem_shmem_get_pages_sgt "drm_gem_shmem_get_pages_sgt") instead.

**Return**

A pointer to the scatter/gather table of pinned pages or error pointer on failure.

struct sg\_table \*drm\_gem\_shmem\_get\_pages\_sgt(struct [drm\_gem\_shmem\_object](#c.drm_gem_shmem_object "drm_gem_shmem_object") \*shmem)
:   Pin pages, dma map them, and return a scatter/gather table for a shmem GEM object.

**Parameters**

`struct drm_gem_shmem_object *shmem`
:   shmem GEM object

**Description**

This function returns a scatter/gather table suitable for driver usage. If
the sg table doesn’t exist, the pages are pinned, dma-mapped, and a sg
table created.

This is the main function for drivers to get at backing storage, and it hides
and difference between dma-buf imported and natively allocated objects.
[`drm_gem_shmem_get_sg_table()`](#c.drm_gem_shmem_get_sg_table "drm_gem_shmem_get_sg_table") should not be directly called by drivers.

**Return**

A pointer to the scatter/gather table of pinned pages or errno on failure.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_shmem\_prime\_import\_sg\_table(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct sg\_table \*sgt)
:   Produce a shmem GEM object from another driver’s scatter/gather table of pinned pages

**Parameters**

`struct drm_device *dev`
:   Device to import into

`struct dma_buf_attachment *attach`
:   DMA-BUF attachment

`struct sg_table *sgt`
:   Scatter/gather table of pinned pages

**Description**

This function imports a scatter/gather table exported via DMA-BUF by
another driver. Drivers that use the shmem helpers should set this as their
[`drm_driver.gem_prime_import_sg_table`](drm-internals.html#c.drm_driver "drm_driver") callback.

**Return**

A pointer to a newly created GEM object or an ERR\_PTR-encoded negative
error code on failure.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_shmem\_prime\_import\_no\_map(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf](#c.drm_gem_shmem_prime_import_no_map "dma_buf") \*dma\_buf)
:   Import dmabuf without mapping its sg\_table

**Parameters**

`struct drm_device *dev`
:   Device to import into

`struct dma_buf *dma_buf`
:   dma-buf object to import

**Description**

Drivers that use the shmem helpers but also wants to import dmabuf without
mapping its sg\_table can use this as their [`drm_driver.gem_prime_import`](drm-internals.html#c.drm_driver "drm_driver")
implementation.

### GEM VRAM Helper Functions Reference

This library provides [`struct drm_gem_vram_object`](#c.drm_gem_vram_object "drm_gem_vram_object") (GEM VRAM), a GEM
buffer object that is backed by video RAM (VRAM). It can be used for
framebuffer devices with dedicated memory.

The data structure [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm") and its helpers implement a memory
manager for simple framebuffer devices with dedicated video memory. GEM
VRAM buffer objects are either placed in the video memory or remain evicted
to system memory.

With the GEM interface userspace applications create, manage and destroy
graphics buffers, such as an on-screen framebuffer. GEM does not provide
an implementation of these interfaces. It’s up to the DRM driver to
provide an implementation that suits the hardware. If the hardware device
contains dedicated video memory, the DRM driver can use the VRAM helper
library. Each active buffer object is stored in video RAM. Active
buffer are used for drawing the current frame, typically something like
the frame’s scanout buffer or the cursor image. If there’s no more space
left in VRAM, inactive GEM objects can be moved to system memory.

To initialize the VRAM helper library call [`drmm_vram_helper_init()`](#c.drmm_vram_helper_init "drmm_vram_helper_init").
The function allocates and initializes an instance of [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm")
in [`struct drm_device`](drm-internals.html#c.drm_device "drm_device").vram\_mm . Use [`DRM_GEM_VRAM_DRIVER`](#c.DRM_GEM_VRAM_DRIVER "DRM_GEM_VRAM_DRIVER") to initialize
[`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver") and [`DEFINE_DRM_GEM_FOPS`](#c.DEFINE_DRM_GEM_FOPS "DEFINE_DRM_GEM_FOPS") to define
`struct file_operations`; as illustrated below.

```
DEFINE_DRM_GEM_FOPS(fops);
struct drm_driver drv = {
        .driver_feature = DRM_ ... ,
        .fops = &fops,
        DRM_GEM_VRAM_DRIVER
};

int init_drm_driver()
{
        struct drm_device *dev;
        uint64_t vram_base;
        unsigned long vram_size;
        int ret;

        // setup device, vram base and size
        // ...

        ret = drmm_vram_helper_init(dev, vram_base, vram_size);
        if (ret)
                return ret;
        return 0;
}
```

This creates an instance of [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm"), exports DRM userspace
interfaces for GEM buffer management and initializes file operations to
allow for accessing created GEM buffers. With this setup, the DRM driver
manages an area of video RAM with VRAM MM and provides GEM VRAM objects
to userspace.

You don’t have to clean up the instance of VRAM MM.
[`drmm_vram_helper_init()`](#c.drmm_vram_helper_init "drmm_vram_helper_init") is a managed interface that installs a
clean-up handler to run during the DRM device’s release.

A buffer object that is pinned in video RAM has a fixed address within that
memory region. Call [`drm_gem_vram_offset()`](#c.drm_gem_vram_offset "drm_gem_vram_offset") to retrieve this value. Typically
it’s used to program the hardware’s scanout engine for framebuffers, set
the cursor overlay’s image for a mouse cursor, or use it as input to the
hardware’s drawing engine.

To access a buffer object’s memory from the DRM driver, call
[`drm_gem_vram_vmap()`](#c.drm_gem_vram_vmap "drm_gem_vram_vmap"). It maps the buffer into kernel address
space and returns the memory address. Use [`drm_gem_vram_vunmap()`](#c.drm_gem_vram_vunmap "drm_gem_vram_vunmap") to
release the mapping.

struct drm\_gem\_vram\_object
:   GEM object backed by VRAM

**Definition**:

```
struct drm_gem_vram_object {
    struct ttm_buffer_object bo;
    struct iosys_map map;
    unsigned int vmap_use_count;
    struct ttm_placement placement;
    struct ttm_place placements[2];
};
```

**Members**

`bo`
:   TTM buffer object

`map`
:   Mapping information for **bo**

`vmap_use_count`
:   Reference count on the virtual address.
    The address are un-mapped when the count reaches zero.

`placement`
:   TTM placement information. Supported placements are `TTM_PL_VRAM`
    and `TTM_PL_SYSTEM`

`placements`
:   TTM placement information.

**Description**

The type [`struct drm_gem_vram_object`](#c.drm_gem_vram_object "drm_gem_vram_object") represents a GEM object that is
backed by VRAM. It can be used for simple framebuffer devices with
dedicated memory. The buffer object can be evicted to system memory if
video memory becomes scarce.

GEM VRAM objects perform reference counting for pin and mapping
operations. So a buffer object that has been pinned N times with
`drm_gem_vram_pin()` must be unpinned N times with
`drm_gem_vram_unpin()`. The same applies to pairs of
`drm_gem_vram_kmap()` and `drm_gem_vram_kunmap()`, as well as pairs of
[`drm_gem_vram_vmap()`](#c.drm_gem_vram_vmap "drm_gem_vram_vmap") and [`drm_gem_vram_vunmap()`](#c.drm_gem_vram_vunmap "drm_gem_vram_vunmap").

struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*drm\_gem\_vram\_of\_bo(struct ttm\_buffer\_object \*bo)
:   Returns the container of type [`struct drm_gem_vram_object`](#c.drm_gem_vram_object "drm_gem_vram_object") for field bo.

**Parameters**

`struct ttm_buffer_object *bo`
:   the VRAM buffer object

**Return**

The containing GEM VRAM object

struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*drm\_gem\_vram\_of\_gem(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*gem)
:   Returns the container of type [`struct drm_gem_vram_object`](#c.drm_gem_vram_object "drm_gem_vram_object") for field gem.

**Parameters**

`struct drm_gem_object *gem`
:   the GEM object

**Return**

The containing GEM VRAM object

DRM\_GEM\_VRAM\_PLANE\_HELPER\_FUNCS

`DRM_GEM_VRAM_PLANE_HELPER_FUNCS`

> > Initializes [`struct drm_plane_helper_funcs`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") for VRAM handling
>
> **Description**
>
> Drivers may use GEM BOs as VRAM helpers for the framebuffer memory. This
> macro initializes [`struct drm_plane_helper_funcs`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") to use the respective helper
> functions.

DRM\_GEM\_VRAM\_DRIVER

`DRM_GEM_VRAM_DRIVER`

> > default callback functions for [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver")
>
> **Description**
>
> Drivers that use VRAM MM and GEM VRAM can use this macro to initialize
> [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver") with default functions.

struct drm\_vram\_mm
:   An instance of VRAM MM

**Definition**:

```
struct drm_vram_mm {
    uint64_t vram_base;
    size_t vram_size;
    struct ttm_device bdev;
};
```

**Members**

`vram_base`
:   Base address of the managed video memory

`vram_size`
:   Size of the managed video memory in bytes

`bdev`
:   The TTM BO device.

**Description**

The fields [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm").vram\_base and
[`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm").vrm\_size are managed by VRAM MM, but are
available for public read access. Use the field
[`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm").bdev to access the TTM BO device.

struct [drm\_vram\_mm](#c.drm_vram_mm "drm_vram_mm") \*drm\_vram\_mm\_of\_bdev(struct [ttm\_device](#c.ttm_device "ttm_device") \*bdev)
:   Returns the container of type [`struct ttm_device`](#c.ttm_device "ttm_device") for field bdev.

**Parameters**

`struct ttm_device *bdev`
:   the TTM BO device

**Return**

The containing instance of [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm")

struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*drm\_gem\_vram\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, size\_t size, unsigned long pg\_align)
:   Creates a VRAM-backed GEM object

**Parameters**

`struct drm_device *dev`
:   the DRM device

`size_t size`
:   the buffer size in bytes

`unsigned long pg_align`
:   the buffer’s alignment in multiples of the page size

**Description**

GEM objects are allocated by calling [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").gem\_create\_object,
if set. Otherwise [`kzalloc()`](../core-api/mm-api.html#c.kzalloc "kzalloc") will be used. Drivers can set their own GEM
object functions in [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").gem\_create\_object. If no functions
are set, the new GEM object will use the default functions from GEM VRAM
helpers.

**Return**

A new instance of [`struct drm_gem_vram_object`](#c.drm_gem_vram_object "drm_gem_vram_object") on success, or
an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded error code otherwise.

void drm\_gem\_vram\_put(struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*gbo)
:   Releases a reference to a VRAM-backed GEM object

**Parameters**

`struct drm_gem_vram_object *gbo`
:   the GEM VRAM object

**Description**

See `ttm_bo_fini()` for more information.

s64 drm\_gem\_vram\_offset(struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*gbo)
:   Returns a GEM VRAM object’s offset in video memory

**Parameters**

`struct drm_gem_vram_object *gbo`
:   the GEM VRAM object

**Description**

This function returns the buffer object’s offset in the device’s video
memory. The buffer object has to be pinned to `TTM_PL_VRAM`.

**Return**

The buffer object’s offset in video memory on success, or
a negative errno code otherwise.

int drm\_gem\_vram\_vmap(struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*gbo, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   Pins and maps a GEM VRAM object into kernel address space

**Parameters**

`struct drm_gem_vram_object *gbo`
:   The GEM VRAM object to map

`struct iosys_map *map`
:   Returns the kernel virtual address of the VRAM GEM object’s backing
    store.

**Description**

The vmap function pins a GEM VRAM object to its current location, either
system or video memory, and maps its buffer into kernel address space.
As pinned object cannot be relocated, you should avoid pinning objects
permanently. Call [`drm_gem_vram_vunmap()`](#c.drm_gem_vram_vunmap "drm_gem_vram_vunmap") with the returned address to
unmap and unpin the GEM VRAM object.

**Return**

0 on success, or a negative error code otherwise.

void drm\_gem\_vram\_vunmap(struct [drm\_gem\_vram\_object](#c.drm_gem_vram_object "drm_gem_vram_object") \*gbo, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   Unmaps and unpins a GEM VRAM object

**Parameters**

`struct drm_gem_vram_object *gbo`
:   The GEM VRAM object to unmap

`struct iosys_map *map`
:   Kernel virtual address where the VRAM GEM object was mapped

**Description**

A call to [`drm_gem_vram_vunmap()`](#c.drm_gem_vram_vunmap "drm_gem_vram_vunmap") unmaps and unpins a GEM VRAM buffer. See
the documentation for [`drm_gem_vram_vmap()`](#c.drm_gem_vram_vmap "drm_gem_vram_vmap") for more information.

int drm\_gem\_vram\_fill\_create\_dumb(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, unsigned long pg\_align, unsigned long pitch\_align, struct [drm\_mode\_create\_dumb](drm-uapi.html#c.drm_mode_create_dumb "drm_mode_create_dumb") \*args)
:   Helper for implementing [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create

**Parameters**

`struct drm_file *file`
:   the DRM file

`struct drm_device *dev`
:   the DRM device

`unsigned long pg_align`
:   the buffer’s alignment in multiples of the page size

`unsigned long pitch_align`
:   the scanline’s alignment in powers of 2

`struct drm_mode_create_dumb *args`
:   the arguments as provided to
    [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create

**Description**

This helper function fills [`struct drm_mode_create_dumb`](drm-uapi.html#c.drm_mode_create_dumb "drm_mode_create_dumb"), which is used
by [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create. Implementations of this interface
should forwards their arguments to this helper, plus the driver-specific
parameters.

**Return**

0 on success, or
a negative error code otherwise.

int drm\_gem\_vram\_driver\_dumb\_create(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_mode\_create\_dumb](drm-uapi.html#c.drm_mode_create_dumb "drm_mode_create_dumb") \*args)
:   Implements [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create

**Parameters**

`struct drm_file *file`
:   the DRM file

`struct drm_device *dev`
:   the DRM device

`struct drm_mode_create_dumb *args`
:   the arguments as provided to
    [`struct drm_driver`](drm-internals.html#c.drm_driver "drm_driver").dumb\_create

**Description**

This function requires the driver to use **drm\_device.vram\_mm** for its
instance of VRAM MM.

**Return**

0 on success, or
a negative error code otherwise.

int drm\_gem\_vram\_plane\_helper\_prepare\_fb(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*new\_state)
:   Implements [`struct drm_plane_helper_funcs`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs").prepare\_fb

**Parameters**

`struct drm_plane *plane`
:   a DRM plane

`struct drm_plane_state *new_state`
:   the plane’s new state

**Description**

During plane updates, this function sets the plane’s fence and
pins the GEM VRAM objects of the plane’s new framebuffer to VRAM.
Call [`drm_gem_vram_plane_helper_cleanup_fb()`](#c.drm_gem_vram_plane_helper_cleanup_fb "drm_gem_vram_plane_helper_cleanup_fb") to unpin them.

**Return**

0 on success, or
a negative errno code otherwise.

void drm\_gem\_vram\_plane\_helper\_cleanup\_fb(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*old\_state)
:   Implements [`struct drm_plane_helper_funcs`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs").cleanup\_fb

**Parameters**

`struct drm_plane *plane`
:   a DRM plane

`struct drm_plane_state *old_state`
:   the plane’s old state

**Description**

During plane updates, this function unpins the GEM VRAM
objects of the plane’s old framebuffer from VRAM. Complements
[`drm_gem_vram_plane_helper_prepare_fb()`](#c.drm_gem_vram_plane_helper_prepare_fb "drm_gem_vram_plane_helper_prepare_fb").

void drm\_vram\_mm\_debugfs\_init(struct [drm\_minor](drm-internals.html#c.drm_minor "drm_minor") \*minor)
:   Register VRAM MM debugfs file.

**Parameters**

`struct drm_minor *minor`
:   drm minor device.

int drmm\_vram\_helper\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, uint64\_t vram\_base, size\_t vram\_size)
:   Initializes a device’s instance of [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm")

**Parameters**

`struct drm_device *dev`
:   the DRM device

`uint64_t vram_base`
:   the base address of the video memory

`size_t vram_size`
:   the size of the video memory in bytes

**Description**

Creates a new instance of [`struct drm_vram_mm`](#c.drm_vram_mm "drm_vram_mm") and stores it in
struct [`drm_device.vram_mm`](drm-internals.html#c.drm_device "drm_device"). The instance is auto-managed and cleaned
up as part of device cleanup. Calling this function multiple times
will generate an error message.

**Return**

0 on success, or a negative errno code otherwise.

enum [drm\_mode\_status](drm-kms.html#c.drm_mode_status "drm_mode_status") drm\_vram\_helper\_mode\_valid(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   Tests if a display mode’s framebuffer fits into the available video memory.

**Parameters**

`struct drm_device *dev`
:   the DRM device

`const struct drm_display_mode *mode`
:   the mode to test

**Description**

This function tests if enough video memory is available for using the
specified display mode. Atomic modesetting requires importing the
designated framebuffer into video memory before evicting the active
one. Hence, any framebuffer may consume at most half of the available
VRAM. Display modes that require a larger framebuffer can not be used,
even if the CRTC does support them. Each framebuffer is assumed to
have 32-bit color depth.

**Note**

The function can only test if the display mode is supported in
general. If there are too many framebuffers pinned to video memory,
a display mode may still not be usable in practice. The color depth of
32-bit fits all current use case. A more flexible test can be added
when necessary.

**Return**

MODE\_OK if the display mode is supported, or an error code of type
[`enum drm_mode_status`](drm-kms.html#c.drm_mode_status "drm_mode_status") otherwise.

### GEM TTM Helper Functions Reference

This library provides helper functions for gem objects backed by
ttm.

void drm\_gem\_ttm\_print\_info(struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, unsigned int indent, const struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*gem)
:   Print `ttm_buffer_object` info for debugfs

**Parameters**

`struct drm_printer *p`
:   DRM printer

`unsigned int indent`
:   Tab indentation level

`const struct drm_gem_object *gem`
:   GEM object

**Description**

This function can be used as [`drm_gem_object_funcs.print_info`](#c.drm_gem_object_funcs "drm_gem_object_funcs")
callback.

int drm\_gem\_ttm\_vmap(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*gem, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   vmap `ttm_buffer_object`

**Parameters**

`struct drm_gem_object *gem`
:   GEM object.

`struct iosys_map *map`
:   [out] returns the dma-buf mapping.

**Description**

Maps a GEM object with `ttm_bo_vmap()`. This function can be used as
[`drm_gem_object_funcs.vmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs") callback.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_gem\_ttm\_vunmap(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*gem, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   vunmap `ttm_buffer_object`

**Parameters**

`struct drm_gem_object *gem`
:   GEM object.

`struct iosys_map *map`
:   dma-buf mapping.

**Description**

Unmaps a GEM object with `ttm_bo_vunmap()`. This function can be used as
[`drm_gem_object_funcs.vmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs") callback.

int drm\_gem\_ttm\_mmap(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*gem, struct vm\_area\_struct \*vma)
:   mmap `ttm_buffer_object`

**Parameters**

`struct drm_gem_object *gem`
:   GEM object.

`struct vm_area_struct *vma`
:   vm area.

**Description**

This function can be used as [`drm_gem_object_funcs.mmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs")
callback.

int drm\_gem\_ttm\_dumb\_map\_offset(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, uint32\_t handle, uint64\_t \*offset)
:   Implements struct [`drm_driver.dumb_map_offset`](drm-internals.html#c.drm_driver "drm_driver")

**Parameters**

`struct drm_file *file`
:   DRM file pointer.

`struct drm_device *dev`
:   DRM device.

`uint32_t handle`
:   GEM handle

`uint64_t *offset`
:   Returns the mapping’s memory offset on success

**Description**

Provides an implementation of struct [`drm_driver.dumb_map_offset`](drm-internals.html#c.drm_driver "drm_driver") for
TTM-based GEM drivers. TTM allocates the offset internally and
[`drm_gem_ttm_dumb_map_offset()`](#c.drm_gem_ttm_dumb_map_offset "drm_gem_ttm_dumb_map_offset") returns it for dumb-buffer implementations.

See struct [`drm_driver.dumb_map_offset`](drm-internals.html#c.drm_driver "drm_driver").

**Return**

0 on success, or a negative errno code otherwise.

## VMA Offset Manager

The vma-manager is responsible to map arbitrary driver-dependent memory
regions into the linear user address-space. It provides offsets to the
caller which can then be used on the address\_space of the drm-device. It
takes care to not overlap regions, size them appropriately and to not
confuse mm-core by inconsistent fake vm\_pgoff fields.
Drivers shouldn’t use this for object placement in VMEM. This manager should
only be used to manage mappings into linear user-space VMs.

We use drm\_mm as backend to manage object allocations. But it is highly
optimized for alloc/free calls, not lookups. Hence, we use an rb-tree to
speed up offset lookups.

You must not use multiple offset managers on a single address\_space.
Otherwise, mm-core will be unable to tear down memory mappings as the VM will
no longer be linear.

This offset manager works on page-based addresses. That is, every argument
and return code (with the exception of [`drm_vma_node_offset_addr()`](#c.drm_vma_node_offset_addr "drm_vma_node_offset_addr")) is given
in number of pages, not number of bytes. That means, object sizes and offsets
must always be page-aligned (as usual).
If you want to get a valid byte-based user-space address for a given offset,
please see [`drm_vma_node_offset_addr()`](#c.drm_vma_node_offset_addr "drm_vma_node_offset_addr").

Additionally to offset management, the vma offset manager also handles access
management. For every open-file context that is allowed to access a given
node, you must call [`drm_vma_node_allow()`](#c.drm_vma_node_allow "drm_vma_node_allow"). Otherwise, an mmap() call on this
open-file with the offset of the node will fail with -EACCES. To revoke
access again, use [`drm_vma_node_revoke()`](#c.drm_vma_node_revoke "drm_vma_node_revoke"). However, the caller is responsible
for destroying already existing mappings, if required.

struct drm\_vma\_offset\_node \*drm\_vma\_offset\_exact\_lookup\_locked(struct drm\_vma\_offset\_manager \*mgr, unsigned long start, unsigned long pages)
:   Look up node by exact address

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

`unsigned long start`
:   Start address (page-based, not byte-based)

`unsigned long pages`
:   Size of object (page-based)

**Description**

Same as [`drm_vma_offset_lookup_locked()`](#c.drm_vma_offset_lookup_locked "drm_vma_offset_lookup_locked") but does not allow any offset into the node.
It only returns the exact object with the given start address.

**Return**

Node at exact start address **start**.

void drm\_vma\_offset\_lock\_lookup(struct drm\_vma\_offset\_manager \*mgr)
:   Lock lookup for extended private use

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

**Description**

Lock VMA manager for extended lookups. Only locked VMA function calls
are allowed while holding this lock. All other contexts are blocked from VMA
until the lock is released via [`drm_vma_offset_unlock_lookup()`](#c.drm_vma_offset_unlock_lookup "drm_vma_offset_unlock_lookup").

Use this if you need to take a reference to the objects returned by
[`drm_vma_offset_lookup_locked()`](#c.drm_vma_offset_lookup_locked "drm_vma_offset_lookup_locked") before releasing this lock again.

This lock must not be used for anything else than extended lookups. You must
not call any other VMA helpers while holding this lock.

**Note**

You’re in atomic-context while holding this lock!

void drm\_vma\_offset\_unlock\_lookup(struct drm\_vma\_offset\_manager \*mgr)
:   Unlock lookup for extended private use

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

**Description**

Release lookup-lock. See [`drm_vma_offset_lock_lookup()`](#c.drm_vma_offset_lock_lookup "drm_vma_offset_lock_lookup") for more information.

void drm\_vma\_node\_reset(struct drm\_vma\_offset\_node \*node)
:   Initialize or reset node object

**Parameters**

`struct drm_vma_offset_node *node`
:   Node to initialize or reset

**Description**

Reset a node to its initial state. This must be called before using it with
any VMA offset manager.

This must not be called on an already allocated node, or you will leak
memory.

unsigned long drm\_vma\_node\_start(const struct drm\_vma\_offset\_node \*node)
:   Return start address for page-based addressing

**Parameters**

`const struct drm_vma_offset_node *node`
:   Node to inspect

**Description**

Return the start address of the given node. This can be used as offset into
the linear VM space that is provided by the VMA offset manager. Note that
this can only be used for page-based addressing. If you need a proper offset
for user-space mappings, you must apply “<< PAGE\_SHIFT” or use the
[`drm_vma_node_offset_addr()`](#c.drm_vma_node_offset_addr "drm_vma_node_offset_addr") helper instead.

**Return**

Start address of **node** for page-based addressing. 0 if the node does not
have an offset allocated.

unsigned long drm\_vma\_node\_size(struct drm\_vma\_offset\_node \*node)
:   Return size (page-based)

**Parameters**

`struct drm_vma_offset_node *node`
:   Node to inspect

**Description**

Return the size as number of pages for the given node. This is the same size
that was passed to [`drm_vma_offset_add()`](#c.drm_vma_offset_add "drm_vma_offset_add"). If no offset is allocated for the
node, this is 0.

**Return**

Size of **node** as number of pages. 0 if the node does not have an offset
allocated.

\_\_u64 drm\_vma\_node\_offset\_addr(struct drm\_vma\_offset\_node \*node)
:   Return sanitized offset for user-space mmaps

**Parameters**

`struct drm_vma_offset_node *node`
:   Linked offset node

**Description**

Same as [`drm_vma_node_start()`](#c.drm_vma_node_start "drm_vma_node_start") but returns the address as a valid offset that
can be used for user-space mappings during mmap().
This must not be called on unlinked nodes.

**Return**

Offset of **node** for byte-based addressing. 0 if the node does not have an
object allocated.

void drm\_vma\_node\_unmap(struct drm\_vma\_offset\_node \*node, struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*file\_mapping)
:   Unmap offset node

**Parameters**

`struct drm_vma_offset_node *node`
:   Offset node

`struct address_space *file_mapping`
:   Address space to unmap **node** from

**Description**

Unmap all userspace mappings for a given offset node. The mappings must be
associated with the **file\_mapping** address-space. If no offset exists
nothing is done.

This call is unlocked. The caller must guarantee that [`drm_vma_offset_remove()`](#c.drm_vma_offset_remove "drm_vma_offset_remove")
is not called on this node concurrently.

int drm\_vma\_node\_verify\_access(struct drm\_vma\_offset\_node \*node, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*tag)
:   Access verification helper for TTM

**Parameters**

`struct drm_vma_offset_node *node`
:   Offset node

`struct drm_file *tag`
:   Tag of file to check

**Description**

This checks whether **tag** is granted access to **node**. It is the same as
[`drm_vma_node_is_allowed()`](#c.drm_vma_node_is_allowed "drm_vma_node_is_allowed") but suitable as drop-in helper for TTM
`verify_access()` callbacks.

**Return**

0 if access is granted, -EACCES otherwise.

void drm\_vma\_offset\_manager\_init(struct drm\_vma\_offset\_manager \*mgr, unsigned long page\_offset, unsigned long size)
:   Initialize new offset-manager

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

`unsigned long page_offset`
:   Offset of available memory area (page-based)

`unsigned long size`
:   Size of available address space range (page-based)

**Description**

Initialize a new offset-manager. The offset and area size available for the
manager are given as **page\_offset** and **size**. Both are interpreted as
page-numbers, not bytes.

Adding/removing nodes from the manager is locked internally and protected
against concurrent access. However, node allocation and destruction is left
for the caller. While calling into the vma-manager, a given node must
always be guaranteed to be referenced.

void drm\_vma\_offset\_manager\_destroy(struct drm\_vma\_offset\_manager \*mgr)
:   Destroy offset manager

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

**Description**

Destroy an object manager which was previously created via
[`drm_vma_offset_manager_init()`](#c.drm_vma_offset_manager_init "drm_vma_offset_manager_init"). The caller must remove all allocated nodes
before destroying the manager. Otherwise, drm\_mm will refuse to free the
requested resources.

The manager must not be accessed after this function is called.

struct drm\_vma\_offset\_node \*drm\_vma\_offset\_lookup\_locked(struct drm\_vma\_offset\_manager \*mgr, unsigned long start, unsigned long pages)
:   Find node in offset space

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

`unsigned long start`
:   Start address for object (page-based)

`unsigned long pages`
:   Size of object (page-based)

**Description**

Find a node given a start address and object size. This returns the \_best\_
match for the given node. That is, **start** may point somewhere into a valid
region and the given node will be returned, as long as the node spans the
whole requested area (given the size in number of pages as **pages**).

Note that before lookup the vma offset manager lookup lock must be acquired
with [`drm_vma_offset_lock_lookup()`](#c.drm_vma_offset_lock_lookup "drm_vma_offset_lock_lookup"). See there for an example. This can then be
used to implement weakly referenced lookups using [`kref_get_unless_zero()`](../core-api/kref.html#c.kref_get_unless_zero "kref_get_unless_zero").

**Example**

```
drm_vma_offset_lock_lookup(mgr);
node = drm_vma_offset_lookup_locked(mgr);
if (node)
    kref_get_unless_zero(container_of(node, sth, entr));
drm_vma_offset_unlock_lookup(mgr);
```

**Return**

Returns NULL if no suitable node can be found. Otherwise, the best match
is returned. It’s the caller’s responsibility to make sure the node doesn’t
get destroyed before the caller can access it.

int drm\_vma\_offset\_add(struct drm\_vma\_offset\_manager \*mgr, struct drm\_vma\_offset\_node \*node, unsigned long pages)
:   Add offset node to manager

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

`struct drm_vma_offset_node *node`
:   Node to be added

`unsigned long pages`
:   Allocation size visible to user-space (in number of pages)

**Description**

Add a node to the offset-manager. If the node was already added, this does
nothing and return 0. **pages** is the size of the object given in number of
pages.
After this call succeeds, you can access the offset of the node until it
is removed again.

If this call fails, it is safe to retry the operation or call
[`drm_vma_offset_remove()`](#c.drm_vma_offset_remove "drm_vma_offset_remove"), anyway. However, no cleanup is required in that
case.

**pages** is not required to be the same size as the underlying memory object
that you want to map. It only limits the size that user-space can map into
their address space.

**Return**

0 on success, negative error code on failure.

void drm\_vma\_offset\_remove(struct drm\_vma\_offset\_manager \*mgr, struct drm\_vma\_offset\_node \*node)
:   Remove offset node from manager

**Parameters**

`struct drm_vma_offset_manager *mgr`
:   Manager object

`struct drm_vma_offset_node *node`
:   Node to be removed

**Description**

Remove a node from the offset manager. If the node wasn’t added before, this
does nothing. After this call returns, the offset and size will be 0 until a
new offset is allocated via [`drm_vma_offset_add()`](#c.drm_vma_offset_add "drm_vma_offset_add") again. Helper functions like
[`drm_vma_node_start()`](#c.drm_vma_node_start "drm_vma_node_start") and [`drm_vma_node_offset_addr()`](#c.drm_vma_node_offset_addr "drm_vma_node_offset_addr") will return 0 if no
offset is allocated.

int drm\_vma\_node\_allow(struct drm\_vma\_offset\_node \*node, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*tag)
:   Add open-file to list of allowed users

**Parameters**

`struct drm_vma_offset_node *node`
:   Node to modify

`struct drm_file *tag`
:   Tag of file to remove

**Description**

Add **tag** to the list of allowed open-files for this node. If **tag** is
already on this list, the ref-count is incremented.

The list of allowed-users is preserved across [`drm_vma_offset_add()`](#c.drm_vma_offset_add "drm_vma_offset_add") and
[`drm_vma_offset_remove()`](#c.drm_vma_offset_remove "drm_vma_offset_remove") calls. You may even call it if the node is currently
not added to any offset-manager.

You must remove all open-files the same number of times as you added them
before destroying the node. Otherwise, you will leak memory.

This is locked against concurrent access internally.

**Return**

0 on success, negative error code on internal failure (out-of-mem)

int drm\_vma\_node\_allow\_once(struct drm\_vma\_offset\_node \*node, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*tag)
:   Add open-file to list of allowed users

**Parameters**

`struct drm_vma_offset_node *node`
:   Node to modify

`struct drm_file *tag`
:   Tag of file to remove

**Description**

Add **tag** to the list of allowed open-files for this node.

The list of allowed-users is preserved across [`drm_vma_offset_add()`](#c.drm_vma_offset_add "drm_vma_offset_add") and
[`drm_vma_offset_remove()`](#c.drm_vma_offset_remove "drm_vma_offset_remove") calls. You may even call it if the node is currently
not added to any offset-manager.

This is not ref-counted unlike [`drm_vma_node_allow()`](#c.drm_vma_node_allow "drm_vma_node_allow") hence [`drm_vma_node_revoke()`](#c.drm_vma_node_revoke "drm_vma_node_revoke")
should only be called once after this.

This is locked against concurrent access internally.

**Return**

0 on success, negative error code on internal failure (out-of-mem)

void drm\_vma\_node\_revoke(struct drm\_vma\_offset\_node \*node, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*tag)
:   Remove open-file from list of allowed users

**Parameters**

`struct drm_vma_offset_node *node`
:   Node to modify

`struct drm_file *tag`
:   Tag of file to remove

**Description**

Decrement the ref-count of **tag** in the list of allowed open-files on **node**.
If the ref-count drops to zero, remove **tag** from the list. You must call
this once for every [`drm_vma_node_allow()`](#c.drm_vma_node_allow "drm_vma_node_allow") on **tag**.

This is locked against concurrent access internally.

If **tag** is not on the list, nothing is done.

bool drm\_vma\_node\_is\_allowed(struct drm\_vma\_offset\_node \*node, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*tag)
:   Check whether an open-file is granted access

**Parameters**

`struct drm_vma_offset_node *node`
:   Node to check

`struct drm_file *tag`
:   Tag of file to remove

**Description**

Search the list in **node** whether **tag** is currently on the list of allowed
open-files (see [`drm_vma_node_allow()`](#c.drm_vma_node_allow "drm_vma_node_allow")).

This is locked against concurrent access internally.

**Return**

true if **filp** is on the list

## PRIME Buffer Sharing

PRIME is the cross device buffer sharing framework in drm, originally
created for the OPTIMUS range of multi-gpu platforms. To userspace PRIME
buffers are dma-buf based file descriptors.

### Overview and Lifetime Rules

Similar to GEM global names, PRIME file descriptors are also used to share
buffer objects across processes. They offer additional security: as file
descriptors must be explicitly sent over UNIX domain sockets to be shared
between applications, they can’t be guessed like the globally unique GEM
names.

Drivers that support the PRIME API implement the drm\_gem\_object\_funcs.export
and [`drm_driver.gem_prime_import`](drm-internals.html#c.drm_driver "drm_driver") hooks. [`dma_buf_ops`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementations for
drivers are all individually exported for drivers which need to overwrite
or reimplement some of them.

#### Reference Counting for GEM Drivers

On the export the [`dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf") holds a reference to the exported buffer object,
usually a [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"). It takes this reference in the PRIME\_HANDLE\_TO\_FD
IOCTL, when it first calls [`drm_gem_object_funcs.export`](#c.drm_gem_object_funcs "drm_gem_object_funcs")
and stores the exporting GEM object in the [`dma_buf.priv`](../driver-api/dma-buf.html#c.dma_buf "dma_buf") field. This
reference needs to be released when the final reference to the [`dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf")
itself is dropped and its [`dma_buf_ops.release`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") function is called. For
GEM-based drivers, the [`dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf") should be exported using
[`drm_gem_dmabuf_export()`](#c.drm_gem_dmabuf_export "drm_gem_dmabuf_export") and then released by [`drm_gem_dmabuf_release()`](#c.drm_gem_dmabuf_release "drm_gem_dmabuf_release").

Thus the chain of references always flows in one direction, avoiding loops:
importing GEM object -> dma-buf -> exported GEM bo. A further complication
are the lookup caches for import and export. These are required to guarantee
that any given object will always have only one unique userspace handle. This
is required to allow userspace to detect duplicated imports, since some GEM
drivers do fail command submissions if a given buffer object is listed more
than once. These import and export caches in [`drm_prime_file_private`](#c.drm_prime_file_private "drm_prime_file_private") only
retain a weak reference, which is cleaned up when the corresponding object is
released.

Self-importing: If userspace is using PRIME as a replacement for flink then
it will get a fd->handle request for a GEM object that it created. Drivers
should detect this situation and return back the underlying object from the
dma-buf private. For GEM based drivers this is handled in
[`drm_gem_prime_import()`](#c.drm_gem_prime_import "drm_gem_prime_import") already.

### PRIME Helper Functions

Drivers can implement [`drm_gem_object_funcs.export`](#c.drm_gem_object_funcs "drm_gem_object_funcs") and
[`drm_driver.gem_prime_import`](drm-internals.html#c.drm_driver "drm_driver") in terms of simpler APIs by using the helper
functions [`drm_gem_prime_export()`](#c.drm_gem_prime_export "drm_gem_prime_export") and [`drm_gem_prime_import()`](#c.drm_gem_prime_import "drm_gem_prime_import"). These functions
implement dma-buf support in terms of some lower-level helpers, which are
again exported for drivers to use individually:

#### Exporting buffers

Optional pinning of buffers is handled at dma-buf attach and detach time in
[`drm_gem_map_attach()`](#c.drm_gem_map_attach "drm_gem_map_attach") and [`drm_gem_map_detach()`](#c.drm_gem_map_detach "drm_gem_map_detach"). Backing storage itself is
handled by [`drm_gem_map_dma_buf()`](#c.drm_gem_map_dma_buf "drm_gem_map_dma_buf") and [`drm_gem_unmap_dma_buf()`](#c.drm_gem_unmap_dma_buf "drm_gem_unmap_dma_buf"), which relies on
[`drm_gem_object_funcs.get_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs"). If [`drm_gem_object_funcs.get_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs") is
unimplemented, exports into another device are rejected.

For kernel-internal access there’s [`drm_gem_dmabuf_vmap()`](#c.drm_gem_dmabuf_vmap "drm_gem_dmabuf_vmap") and
[`drm_gem_dmabuf_vunmap()`](#c.drm_gem_dmabuf_vunmap "drm_gem_dmabuf_vunmap"). Userspace mmap support is provided by
[`drm_gem_dmabuf_mmap()`](#c.drm_gem_dmabuf_mmap "drm_gem_dmabuf_mmap").

Note that these export helpers can only be used if the underlying backing
storage is fully coherent and either permanently pinned, or it is safe to pin
it indefinitely.

FIXME: The underlying helper functions are named rather inconsistently.

#### Importing buffers

Importing dma-bufs using [`drm_gem_prime_import()`](#c.drm_gem_prime_import "drm_gem_prime_import") relies on
[`drm_driver.gem_prime_import_sg_table`](drm-internals.html#c.drm_driver "drm_driver").

Note that similarly to the export helpers this permanently pins the
underlying backing storage. Which is ok for scanout, but is not the best
option for sharing lots of buffers for rendering.

### PRIME Function References

struct drm\_prime\_file\_private
:   per-file tracking for PRIME

**Definition**:

```
struct drm_prime_file_private {
};
```

**Members**

**Description**

This just contains the internal [`struct dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf") and handle caches for each
[`struct drm_file`](drm-internals.html#c.drm_file "drm_file") used by the PRIME core code.

struct [dma\_buf](../driver-api/dma-buf.html#c.dma_buf "dma_buf") \*drm\_gem\_dmabuf\_export(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf\_export\_info](../driver-api/dma-buf.html#c.dma_buf_export_info "dma_buf_export_info") \*exp\_info)
:   [`dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf") export implementation for GEM

**Parameters**

`struct drm_device *dev`
:   parent device for the exported dmabuf

`struct dma_buf_export_info *exp_info`
:   the export information used by [`dma_buf_export()`](../driver-api/dma-buf.html#c.dma_buf_export "dma_buf_export")

**Description**

This wraps [`dma_buf_export()`](../driver-api/dma-buf.html#c.dma_buf_export "dma_buf_export") for use by generic GEM drivers that are using
[`drm_gem_dmabuf_release()`](#c.drm_gem_dmabuf_release "drm_gem_dmabuf_release"). In addition to calling [`dma_buf_export()`](../driver-api/dma-buf.html#c.dma_buf_export "dma_buf_export"), we take
a reference to the [`drm_device`](drm-internals.html#c.drm_device "drm_device") and the exported [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") (stored in
[`dma_buf_export_info.priv`](../driver-api/dma-buf.html#c.dma_buf_export_info "dma_buf_export_info")) which is released by [`drm_gem_dmabuf_release()`](#c.drm_gem_dmabuf_release "drm_gem_dmabuf_release").

Returns the new dmabuf.

void drm\_gem\_dmabuf\_release(struct [dma\_buf](#c.drm_gem_dmabuf_release "dma_buf") \*dma\_buf)
:   [`dma_buf`](#c.drm_gem_dmabuf_release "dma_buf") release implementation for GEM

**Parameters**

`struct dma_buf *dma_buf`
:   buffer to be released

**Description**

Generic release function for dma\_bufs exported as PRIME buffers. GEM drivers
must use this in their [`dma_buf_ops`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") structure as the release callback.
[`drm_gem_dmabuf_release()`](#c.drm_gem_dmabuf_release "drm_gem_dmabuf_release") should be used in conjunction with
[`drm_gem_dmabuf_export()`](#c.drm_gem_dmabuf_export "drm_gem_dmabuf_export").

int drm\_gem\_prime\_fd\_to\_handle(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, int prime\_fd, uint32\_t \*handle)
:   PRIME import function for GEM drivers

**Parameters**

`struct drm_device *dev`
:   drm\_device to import into

`struct drm_file *file_priv`
:   drm file-private structure

`int prime_fd`
:   fd id of the dma-buf which should be imported

`uint32_t *handle`
:   pointer to storage for the handle of the imported buffer object

**Description**

This is the PRIME import function which must be used mandatorily by GEM
drivers to ensure correct lifetime management of the underlying GEM object.
The actual importing of GEM object from the dma-buf is done through the
[`drm_driver.gem_prime_import`](drm-internals.html#c.drm_driver "drm_driver") driver callback.

Returns 0 on success or a negative error code on failure.

struct [dma\_buf](../driver-api/dma-buf.html#c.dma_buf "dma_buf") \*drm\_gem\_prime\_handle\_to\_dmabuf(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t handle, uint32\_t flags)
:   PRIME export function for GEM drivers

**Parameters**

`struct drm_device *dev`
:   dev to export the buffer from

`struct drm_file *file_priv`
:   drm file-private structure

`uint32_t handle`
:   buffer handle to export

`uint32_t flags`
:   flags like DRM\_CLOEXEC

**Description**

This is the PRIME export function which must be used mandatorily by GEM
drivers to ensure correct lifetime management of the underlying GEM object.
The actual exporting from GEM object to a dma-buf is done through the
[`drm_gem_object_funcs.export`](#c.drm_gem_object_funcs "drm_gem_object_funcs") callback.

Unlike [`drm_gem_prime_handle_to_fd()`](#c.drm_gem_prime_handle_to_fd "drm_gem_prime_handle_to_fd"), it returns the [`struct dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf") it
has created, without attaching it to any file descriptors. The difference
between those two is similar to that between [`anon_inode_getfile()`](../filesystems/api-summary.html#c.anon_inode_getfile "anon_inode_getfile") and
[`anon_inode_getfd()`](../filesystems/api-summary.html#c.anon_inode_getfd "anon_inode_getfd"); insertion into descriptor table is something you
can not revert if any cleanup is needed, so the descriptor-returning
variants should only be used when you are past the last failure exit
and the only thing left is passing the new file descriptor to userland.
When all you need is the object itself or when you need to do something
else that might fail, use that one instead.

int drm\_gem\_prime\_handle\_to\_fd(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t handle, uint32\_t flags, int \*prime\_fd)
:   PRIME export function for GEM drivers

**Parameters**

`struct drm_device *dev`
:   dev to export the buffer from

`struct drm_file *file_priv`
:   drm file-private structure

`uint32_t handle`
:   buffer handle to export

`uint32_t flags`
:   flags like DRM\_CLOEXEC

`int *prime_fd`
:   pointer to storage for the fd id of the create dma-buf

**Description**

This is the PRIME export function which must be used mandatorily by GEM
drivers to ensure correct lifetime management of the underlying GEM object.
The actual exporting from GEM object to a dma-buf is done through the
[`drm_gem_object_funcs.export`](#c.drm_gem_object_funcs "drm_gem_object_funcs") callback.

int drm\_gem\_map\_attach(struct [dma\_buf](#c.drm_gem_map_attach "dma_buf") \*dma\_buf, struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   dma\_buf attach implementation for GEM

**Parameters**

`struct dma_buf *dma_buf`
:   buffer to attach device to

`struct dma_buf_attachment *attach`
:   buffer attachment data

**Description**

Calls [`drm_gem_object_funcs.pin`](#c.drm_gem_object_funcs "drm_gem_object_funcs") for device specific handling. This can be
used as the [`dma_buf_ops.attach`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") callback. Must be used together with
[`drm_gem_map_detach()`](#c.drm_gem_map_detach "drm_gem_map_detach").

Returns 0 on success, negative error code on failure.

void drm\_gem\_map\_detach(struct [dma\_buf](#c.drm_gem_map_detach "dma_buf") \*dma\_buf, struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   dma\_buf detach implementation for GEM

**Parameters**

`struct dma_buf *dma_buf`
:   buffer to detach from

`struct dma_buf_attachment *attach`
:   attachment to be detached

**Description**

Calls [`drm_gem_object_funcs.pin`](#c.drm_gem_object_funcs "drm_gem_object_funcs") for device specific handling. Cleans up
[`dma_buf_attachment`](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") from [`drm_gem_map_attach()`](#c.drm_gem_map_attach "drm_gem_map_attach"). This can be used as the
[`dma_buf_ops.detach`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") callback.

struct sg\_table \*drm\_gem\_map\_dma\_buf(struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, enum dma\_data\_direction dir)
:   map\_dma\_buf implementation for GEM

**Parameters**

`struct dma_buf_attachment *attach`
:   attachment whose scatterlist is to be returned

`enum dma_data_direction dir`
:   direction of DMA transfer

**Description**

Calls [`drm_gem_object_funcs.get_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs") and then maps the scatterlist. This
can be used as the [`dma_buf_ops.map_dma_buf`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") callback. Should be used together
with [`drm_gem_unmap_dma_buf()`](#c.drm_gem_unmap_dma_buf "drm_gem_unmap_dma_buf").

**Return**

sg\_table containing the scatterlist to be returned; returns ERR\_PTR
on error. May return -EINTR if it is interrupted by a signal.

void drm\_gem\_unmap\_dma\_buf(struct [dma\_buf\_attachment](../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct sg\_table \*sgt, enum dma\_data\_direction dir)
:   unmap\_dma\_buf implementation for GEM

**Parameters**

`struct dma_buf_attachment *attach`
:   attachment to unmap buffer from

`struct sg_table *sgt`
:   scatterlist info of the buffer to unmap

`enum dma_data_direction dir`
:   direction of DMA transfer

**Description**

This can be used as the [`dma_buf_ops.unmap_dma_buf`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") callback.

int drm\_gem\_dmabuf\_vmap(struct [dma\_buf](#c.drm_gem_dmabuf_vmap "dma_buf") \*dma\_buf, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   dma\_buf vmap implementation for GEM

**Parameters**

`struct dma_buf *dma_buf`
:   buffer to be mapped

`struct iosys_map *map`
:   the virtual address of the buffer

**Description**

Sets up a kernel virtual mapping. This can be used as the [`dma_buf_ops.vmap`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops")
callback. Calls into [`drm_gem_object_funcs.vmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs") for device specific handling.
The kernel virtual address is returned in map.

Returns 0 on success or a negative errno code otherwise.

void drm\_gem\_dmabuf\_vunmap(struct [dma\_buf](#c.drm_gem_dmabuf_vunmap "dma_buf") \*dma\_buf, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   dma\_buf vunmap implementation for GEM

**Parameters**

`struct dma_buf *dma_buf`
:   buffer to be unmapped

`struct iosys_map *map`
:   the virtual address of the buffer

**Description**

Releases a kernel virtual mapping. This can be used as the
[`dma_buf_ops.vunmap`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") callback. Calls into [`drm_gem_object_funcs.vunmap`](#c.drm_gem_object_funcs "drm_gem_object_funcs") for device specific handling.

int drm\_gem\_prime\_mmap(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, struct vm\_area\_struct \*vma)
:   PRIME mmap function for GEM drivers

**Parameters**

`struct drm_gem_object *obj`
:   GEM object

`struct vm_area_struct *vma`
:   Virtual address range

**Description**

This function sets up a userspace mapping for PRIME exported buffers using
the same codepath that is used for regular GEM buffer mapping on the DRM fd.
The fake GEM offset is added to vma->vm\_pgoff and [`drm_driver->fops`](drm-internals.html#c.drm_driver "drm_driver")->mmap is
called to set up the mapping.

int drm\_gem\_dmabuf\_mmap(struct [dma\_buf](#c.drm_gem_dmabuf_mmap "dma_buf") \*dma\_buf, struct vm\_area\_struct \*vma)
:   dma\_buf mmap implementation for GEM

**Parameters**

`struct dma_buf *dma_buf`
:   buffer to be mapped

`struct vm_area_struct *vma`
:   virtual address range

**Description**

Provides memory mapping for the buffer. This can be used as the
[`dma_buf_ops.mmap`](../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") callback. It just forwards to [`drm_gem_prime_mmap()`](#c.drm_gem_prime_mmap "drm_gem_prime_mmap").

Returns 0 on success or a negative error code on failure.

struct sg\_table \*drm\_prime\_pages\_to\_sg(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct page \*\*pages, unsigned int nr\_pages)
:   converts a page array into an sg list

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct page **pages`
:   pointer to the array of page pointers to convert

`unsigned int nr_pages`
:   length of the page vector

**Description**

This helper creates an sg table object from a set of pages
the driver is responsible for mapping the pages into the
importers address space for use with dma\_buf itself.

This is useful for implementing [`drm_gem_object_funcs.get_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs").

unsigned long drm\_prime\_get\_contiguous\_size(struct sg\_table \*sgt)
:   returns the contiguous size of the buffer

**Parameters**

`struct sg_table *sgt`
:   sg\_table describing the buffer to check

**Description**

This helper calculates the contiguous size in the DMA address space
of the buffer described by the provided sg\_table.

This is useful for implementing
[`drm_gem_object_funcs.gem_prime_import_sg_table`](#c.drm_gem_object_funcs "drm_gem_object_funcs").

struct [dma\_buf](../driver-api/dma-buf.html#c.dma_buf "dma_buf") \*drm\_gem\_prime\_export(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, int flags)
:   helper library implementation of the export callback

**Parameters**

`struct drm_gem_object *obj`
:   GEM object to export

`int flags`
:   flags like DRM\_CLOEXEC and DRM\_RDWR

**Description**

This is the implementation of the [`drm_gem_object_funcs.export`](#c.drm_gem_object_funcs "drm_gem_object_funcs") functions for GEM drivers
using the PRIME helpers. It is used as the default in
[`drm_gem_prime_handle_to_fd()`](#c.drm_gem_prime_handle_to_fd "drm_gem_prime_handle_to_fd").

bool drm\_gem\_is\_prime\_exported\_dma\_buf(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf](#c.drm_gem_is_prime_exported_dma_buf "dma_buf") \*dma\_buf)
:   checks if the DMA-BUF was exported from a GEM object belonging to **dev**.

**Parameters**

`struct drm_device *dev`
:   drm\_device to check against

`struct dma_buf *dma_buf`
:   dma-buf object to import

**Return**

true if the DMA-BUF was exported from a GEM object belonging
to **dev**, false otherwise.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_prime\_import\_dev(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf](#c.drm_gem_prime_import_dev "dma_buf") \*dma\_buf, struct [device](../driver-api/infrastructure.html#c.device "device") \*attach\_dev)
:   core implementation of the import callback

**Parameters**

`struct drm_device *dev`
:   drm\_device to import into

`struct dma_buf *dma_buf`
:   dma-buf object to import

`struct device *attach_dev`
:   [`struct device`](../driver-api/infrastructure.html#c.device "device") to dma\_buf attach

**Description**

This is the core of [`drm_gem_prime_import()`](#c.drm_gem_prime_import "drm_gem_prime_import"). It’s designed to be called by
drivers who want to use a different device structure than [`drm_device.dev`](drm-internals.html#c.drm_device "drm_device") for
attaching via dma\_buf. This function calls
[`drm_driver.gem_prime_import_sg_table`](drm-internals.html#c.drm_driver "drm_driver") internally.

Drivers must arrange to call [`drm_prime_gem_destroy()`](#c.drm_prime_gem_destroy "drm_prime_gem_destroy") from their
[`drm_gem_object_funcs.free`](#c.drm_gem_object_funcs "drm_gem_object_funcs") hook when using this function.

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gem\_prime\_import(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf](#c.drm_gem_prime_import "dma_buf") \*dma\_buf)
:   helper library implementation of the import callback

**Parameters**

`struct drm_device *dev`
:   drm\_device to import into

`struct dma_buf *dma_buf`
:   dma-buf object to import

**Description**

This is the implementation of the gem\_prime\_import functions for GEM drivers
using the PRIME helpers. Drivers can use this as their
[`drm_driver.gem_prime_import`](drm-internals.html#c.drm_driver "drm_driver") implementation. It is used as the default
implementation in [`drm_gem_prime_fd_to_handle()`](#c.drm_gem_prime_fd_to_handle "drm_gem_prime_fd_to_handle").

Drivers must arrange to call [`drm_prime_gem_destroy()`](#c.drm_prime_gem_destroy "drm_prime_gem_destroy") from their
[`drm_gem_object_funcs.free`](#c.drm_gem_object_funcs "drm_gem_object_funcs") hook when using this function.

int drm\_prime\_sg\_to\_page\_array(struct sg\_table \*sgt, struct page \*\*pages, int max\_entries)
:   convert an sg table into a page array

**Parameters**

`struct sg_table *sgt`
:   scatter-gather table to convert

`struct page **pages`
:   array of page pointers to store the pages in

`int max_entries`
:   size of the passed-in array

**Description**

Exports an sg table into an array of pages.

This function is deprecated and strongly discouraged to be used.
The page array is only useful for page faults and those can corrupt fields
in the `struct page` if they are not handled by the exporting driver.

int drm\_prime\_sg\_to\_dma\_addr\_array(struct sg\_table \*sgt, dma\_addr\_t \*addrs, int max\_entries)
:   convert an sg table into a dma addr array

**Parameters**

`struct sg_table *sgt`
:   scatter-gather table to convert

`dma_addr_t *addrs`
:   array to store the dma bus address of each page

`int max_entries`
:   size of both the passed-in arrays

**Description**

Exports an sg table into an array of addresses.

Drivers should use this in their [`drm_driver.gem_prime_import_sg_table`](drm-internals.html#c.drm_driver "drm_driver")
implementation.

void drm\_prime\_gem\_destroy(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, struct sg\_table \*sg)
:   helper to clean up a PRIME-imported GEM object

**Parameters**

`struct drm_gem_object *obj`
:   GEM object which was created from a dma-buf

`struct sg_table *sg`
:   the sg-table which was pinned at import time

**Description**

This is the cleanup functions which GEM drivers need to call when they use
[`drm_gem_prime_import()`](#c.drm_gem_prime_import "drm_gem_prime_import") or [`drm_gem_prime_import_dev()`](#c.drm_gem_prime_import_dev "drm_gem_prime_import_dev") to import dma-bufs.

## DRM MM Range Allocator

### Overview

drm\_mm provides a simple range allocator. The drivers are free to use the
resource allocator from the linux core if it suits them, the upside of drm\_mm
is that it’s in the DRM core. Which means that it’s easier to extend for
some of the crazier special purpose needs of gpus.

The main data `struct is` [`drm_mm`](#c.drm_mm "drm_mm"), allocations are tracked in [`drm_mm_node`](#c.drm_mm_node "drm_mm_node").
Drivers are free to embed either of them into their own suitable
datastructures. drm\_mm itself will not do any memory allocations of its own,
so if drivers choose not to embed nodes they need to still allocate them
themselves.

The range allocator also supports reservation of preallocated blocks. This is
useful for taking over initial mode setting configurations from the firmware,
where an object needs to be created which exactly matches the firmware’s
scanout target. As long as the range is still free it can be inserted anytime
after the allocator is initialized, which helps with avoiding looped
dependencies in the driver load sequence.

drm\_mm maintains a stack of most recently freed holes, which of all
simplistic datastructures seems to be a fairly decent approach to clustering
allocations and avoiding too much fragmentation. This means free space
searches are O(num\_holes). Given that all the fancy features drm\_mm supports
something better would be fairly complex and since gfx thrashing is a fairly
steep cliff not a real concern. Removing a node again is O(1).

drm\_mm supports a few features: Alignment and range restrictions can be
supplied. Furthermore every [`drm_mm_node`](#c.drm_mm_node "drm_mm_node") has a color value (which is just an
opaque unsigned long) which in conjunction with a driver callback can be used
to implement sophisticated placement restrictions. The i915 DRM driver uses
this to implement guard pages between incompatible caching domains in the
graphics TT.

Two behaviors are supported for searching and allocating: bottom-up and
top-down. The default is bottom-up. Top-down allocation can be used if the
memory area has different restrictions, or just to reduce fragmentation.

Finally iteration helpers to walk all nodes and all holes are provided as are
some basic allocator dumpers for debugging.

Note that this range allocator is not thread-safe, drivers need to protect
modifications with their own locking. The idea behind this is that for a full
memory manager additional data needs to be protected anyway, hence internal
locking would be fully redundant.

### LRU Scan/Eviction Support

Very often GPUs need to have continuous allocations for a given object. When
evicting objects to make space for a new one it is therefore not most
efficient when we simply start to select all objects from the tail of an LRU
until there’s a suitable hole: Especially for big objects or nodes that
otherwise have special allocation constraints there’s a good chance we evict
lots of (smaller) objects unnecessarily.

The DRM range allocator supports this use-case through the scanning
interfaces. First a scan operation needs to be initialized with
[`drm_mm_scan_init()`](#c.drm_mm_scan_init "drm_mm_scan_init") or [`drm_mm_scan_init_with_range()`](#c.drm_mm_scan_init_with_range "drm_mm_scan_init_with_range"). The driver adds
objects to the roster, probably by walking an LRU list, but this can be
freely implemented. Eviction candidates are added using
[`drm_mm_scan_add_block()`](#c.drm_mm_scan_add_block "drm_mm_scan_add_block") until a suitable hole is found or there are no
further evictable objects. Eviction roster metadata is tracked in [`struct
drm_mm_scan`](#c.drm_mm_scan "drm_mm_scan").

The driver must walk through all objects again in exactly the reverse
order to restore the allocator state. Note that while the allocator is used
in the scan mode no other operation is allowed.

Finally the driver evicts all objects selected ([`drm_mm_scan_remove_block()`](#c.drm_mm_scan_remove_block "drm_mm_scan_remove_block")
reported true) in the scan, and any overlapping nodes after color adjustment
([`drm_mm_scan_color_evict()`](#c.drm_mm_scan_color_evict "drm_mm_scan_color_evict")). Adding and removing an object is O(1), and
since freeing a node is also O(1) the overall complexity is
O(scanned\_objects). So like the free stack which needs to be walked before a
scan operation even begins this is linear in the number of objects. It
doesn’t seem to hurt too badly.

### DRM MM Range Allocator Function References

enum drm\_mm\_insert\_mode
:   control search and allocation behaviour

**Constants**

`DRM_MM_INSERT_BEST`
:   Search for the smallest hole (within the search range) that fits
    the desired node.

    Allocates the node from the bottom of the found hole.

`DRM_MM_INSERT_LOW`
:   Search for the lowest hole (address closest to 0, within the search
    range) that fits the desired node.

    Allocates the node from the bottom of the found hole.

`DRM_MM_INSERT_HIGH`
:   Search for the highest hole (address closest to U64\_MAX, within the
    search range) that fits the desired node.

    Allocates the node from the *top* of the found hole. The specified
    alignment for the node is applied to the base of the node
    ([`drm_mm_node.start`](#c.drm_mm_node "drm_mm_node")).

`DRM_MM_INSERT_EVICT`
:   Search for the most recently evicted hole (within the search range)
    that fits the desired node. This is appropriate for use immediately
    after performing an eviction scan (see [`drm_mm_scan_init()`](#c.drm_mm_scan_init "drm_mm_scan_init")) and
    removing the selected nodes to form a hole.

    Allocates the node from the bottom of the found hole.

`DRM_MM_INSERT_ONCE`
:   Only check the first hole for suitablity and report -ENOSPC
    immediately otherwise, rather than check every hole until a
    suitable one is found. Can only be used in conjunction with another
    search method such as DRM\_MM\_INSERT\_HIGH or DRM\_MM\_INSERT\_LOW.

`DRM_MM_INSERT_HIGHEST`
:   Only check the highest hole (the hole with the largest address) and
    insert the node at the top of the hole or report -ENOSPC if
    unsuitable.

    Does not search all holes.

`DRM_MM_INSERT_LOWEST`
:   Only check the lowest hole (the hole with the smallest address) and
    insert the node at the bottom of the hole or report -ENOSPC if
    unsuitable.

    Does not search all holes.

**Description**

The [`struct drm_mm`](#c.drm_mm "drm_mm") range manager supports finding a suitable modes using
a number of search trees. These trees are oranised by size, by address and
in most recent eviction order. This allows the user to find either the
smallest hole to reuse, the lowest or highest address to reuse, or simply
reuse the most recent eviction that fits. When allocating the [`drm_mm_node`](#c.drm_mm_node "drm_mm_node")
from within the hole, the [`drm_mm_insert_mode`](#c.drm_mm_insert_mode "drm_mm_insert_mode") also dictate whether to
allocate the lowest matching address or the highest.

struct drm\_mm\_node
:   allocated block in the DRM allocator

**Definition**:

```
struct drm_mm_node {
    unsigned long color;
    u64 start;
    u64 size;
};
```

**Members**

`color`
:   Opaque driver-private tag.

`start`
:   Start address of the allocated block.

`size`
:   Size of the allocated block.

**Description**

This represents an allocated block in a [`drm_mm`](#c.drm_mm "drm_mm") allocator. Except for
pre-reserved nodes inserted using [`drm_mm_reserve_node()`](#c.drm_mm_reserve_node "drm_mm_reserve_node") the structure is
entirely opaque and should only be accessed through the provided funcions.
Since allocation of these nodes is entirely handled by the driver they can be
embedded.

struct drm\_mm
:   DRM allocator

**Definition**:

```
struct drm_mm {
    void (*color_adjust)(const struct drm_mm_node *node, unsigned long color, u64 *start, u64 *end);
};
```

**Members**

`color_adjust`
:   Optional driver callback to further apply restrictions on a hole. The
    node argument points at the node containing the hole from which the
    block would be allocated (see [`drm_mm_hole_follows()`](#c.drm_mm_hole_follows "drm_mm_hole_follows") and friends). The
    other arguments are the size of the block to be allocated. The driver
    can adjust the start and end as needed to e.g. insert guard pages.

**Description**

DRM range allocator with a few special functions and features geared towards
managing GPU memory. Except for the **color\_adjust** callback the structure is
entirely opaque and should only be accessed through the provided functions
and macros. This structure can be embedded into larger driver structures.

struct drm\_mm\_scan
:   DRM allocator eviction roaster data

**Definition**:

```
struct drm_mm_scan {
};
```

**Members**

**Description**

This structure tracks data needed for the eviction roaster set up using
[`drm_mm_scan_init()`](#c.drm_mm_scan_init "drm_mm_scan_init"), and used with [`drm_mm_scan_add_block()`](#c.drm_mm_scan_add_block "drm_mm_scan_add_block") and
[`drm_mm_scan_remove_block()`](#c.drm_mm_scan_remove_block "drm_mm_scan_remove_block"). The structure is entirely opaque and should only
be accessed through the provided functions and macros. It is meant to be
allocated temporarily by the driver on the stack.

bool drm\_mm\_node\_allocated(const struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node)
:   checks whether a node is allocated

**Parameters**

`const struct drm_mm_node *node`
:   drm\_mm\_node to check

**Description**

Drivers are required to clear a node prior to using it with the
drm\_mm range manager.

Drivers should use this helper for proper encapsulation of drm\_mm
internals.

**Return**

True if the **node** is allocated.

bool drm\_mm\_initialized(const struct [drm\_mm](#c.drm_mm "drm_mm") \*mm)
:   checks whether an allocator is initialized

**Parameters**

`const struct drm_mm *mm`
:   drm\_mm to check

**Description**

Drivers should clear the [`struct drm_mm`](#c.drm_mm "drm_mm") prior to initialisation if they
want to use this function.

Drivers should use this helper for proper encapsulation of drm\_mm
internals.

**Return**

True if the **mm** is initialized.

bool drm\_mm\_hole\_follows(const struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node)
:   checks whether a hole follows this node

**Parameters**

`const struct drm_mm_node *node`
:   drm\_mm\_node to check

**Description**

Holes are embedded into the drm\_mm using the tail of a drm\_mm\_node.
If you wish to know whether a hole follows this particular node,
query this function. See also [`drm_mm_hole_node_start()`](#c.drm_mm_hole_node_start "drm_mm_hole_node_start") and
[`drm_mm_hole_node_end()`](#c.drm_mm_hole_node_end "drm_mm_hole_node_end").

**Return**

True if a hole follows the **node**.

u64 drm\_mm\_hole\_node\_start(const struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*hole\_node)
:   computes the start of the hole following **node**

**Parameters**

`const struct drm_mm_node *hole_node`
:   drm\_mm\_node which implicitly tracks the following hole

**Description**

This is useful for driver-specific debug dumpers. Otherwise drivers should
not inspect holes themselves. Drivers must check first whether a hole indeed
follows by looking at [`drm_mm_hole_follows()`](#c.drm_mm_hole_follows "drm_mm_hole_follows")

**Return**

Start of the subsequent hole.

u64 drm\_mm\_hole\_node\_end(const struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*hole\_node)
:   computes the end of the hole following **node**

**Parameters**

`const struct drm_mm_node *hole_node`
:   drm\_mm\_node which implicitly tracks the following hole

**Description**

This is useful for driver-specific debug dumpers. Otherwise drivers should
not inspect holes themselves. Drivers must check first whether a hole indeed
follows by looking at [`drm_mm_hole_follows()`](#c.drm_mm_hole_follows "drm_mm_hole_follows").

**Return**

End of the subsequent hole.

drm\_mm\_nodes

`drm_mm_nodes (mm)`

> list of nodes under the drm\_mm range manager

**Parameters**

`mm`
:   the [`struct drm_mm`](#c.drm_mm "drm_mm") range manager

**Description**

As the drm\_mm range manager hides its node\_list deep with its
structure, extracting it looks painful and repetitive. This is
not expected to be used outside of the [`drm_mm_for_each_node()`](#c.drm_mm_for_each_node "drm_mm_for_each_node")
macros and similar internal functions.

**Return**

The node list, may be empty.

drm\_mm\_for\_each\_node

`drm_mm_for_each_node (entry, mm)`

> iterator to walk over all allocated nodes

**Parameters**

`entry`
:   [`struct drm_mm_node`](#c.drm_mm_node "drm_mm_node") to assign to in each iteration step

`mm`
:   [`drm_mm`](#c.drm_mm "drm_mm") allocator to walk

**Description**

This iterator walks over all nodes in the range allocator. It is implemented
with [`list_for_each()`](../core-api/list.html#c.list_for_each "list_for_each"), so not save against removal of elements.

drm\_mm\_for\_each\_node\_safe

`drm_mm_for_each_node_safe (entry, next, mm)`

> iterator to walk over all allocated nodes

**Parameters**

`entry`
:   [`struct drm_mm_node`](#c.drm_mm_node "drm_mm_node") to assign to in each iteration step

`next`
:   [`struct drm_mm_node`](#c.drm_mm_node "drm_mm_node") to store the next step

`mm`
:   [`drm_mm`](#c.drm_mm "drm_mm") allocator to walk

**Description**

This iterator walks over all nodes in the range allocator. It is implemented
with [`list_for_each_safe()`](../core-api/list.html#c.list_for_each_safe "list_for_each_safe"), so save against removal of elements.

drm\_mm\_for\_each\_hole

`drm_mm_for_each_hole (pos, mm, hole_start, hole_end)`

> iterator to walk over all holes

**Parameters**

`pos`
:   [`drm_mm_node`](#c.drm_mm_node "drm_mm_node") used internally to track progress

`mm`
:   [`drm_mm`](#c.drm_mm "drm_mm") allocator to walk

`hole_start`
:   ulong variable to assign the hole start to on each iteration

`hole_end`
:   ulong variable to assign the hole end to on each iteration

**Description**

This iterator walks over all holes in the range allocator. It is implemented
with [`list_for_each()`](../core-api/list.html#c.list_for_each "list_for_each"), so not save against removal of elements. **entry** is used
internally and will not reflect a real drm\_mm\_node for the very first hole.
Hence users of this iterator may not access it.

Implementation Note:
We need to inline list\_for\_each\_entry in order to be able to set hole\_start
and hole\_end on each iteration while keeping the macro sane.

int drm\_mm\_insert\_node\_generic(struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node, u64 size, u64 alignment, unsigned long color, enum [drm\_mm\_insert\_mode](#c.drm_mm_insert_mode "drm_mm_insert_mode") mode)
:   search for space and insert **node**

**Parameters**

`struct drm_mm *mm`
:   drm\_mm to allocate from

`struct drm_mm_node *node`
:   preallocate node to insert

`u64 size`
:   size of the allocation

`u64 alignment`
:   alignment of the allocation

`unsigned long color`
:   opaque tag value to use for this node

`enum drm_mm_insert_mode mode`
:   fine-tune the allocation search and placement

**Description**

This is a simplified version of [`drm_mm_insert_node_in_range()`](#c.drm_mm_insert_node_in_range "drm_mm_insert_node_in_range") with no
range restrictions applied.

The preallocated node must be cleared to 0.

**Return**

0 on success, -ENOSPC if there’s no suitable hole.

int drm\_mm\_insert\_node(struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node, u64 size)
:   search for space and insert **node**

**Parameters**

`struct drm_mm *mm`
:   drm\_mm to allocate from

`struct drm_mm_node *node`
:   preallocate node to insert

`u64 size`
:   size of the allocation

**Description**

This is a simplified version of [`drm_mm_insert_node_generic()`](#c.drm_mm_insert_node_generic "drm_mm_insert_node_generic") with **color** set
to 0.

The preallocated node must be cleared to 0.

**Return**

0 on success, -ENOSPC if there’s no suitable hole.

bool drm\_mm\_clean(const struct [drm\_mm](#c.drm_mm "drm_mm") \*mm)
:   checks whether an allocator is clean

**Parameters**

`const struct drm_mm *mm`
:   drm\_mm allocator to check

**Return**

True if the allocator is completely free, false if there’s still a node
allocated in it.

drm\_mm\_for\_each\_node\_in\_range

`drm_mm_for_each_node_in_range (node__, mm__, start__, end__)`

> iterator to walk over a range of allocated nodes

**Parameters**

`node__`
:   drm\_mm\_node structure to assign to in each iteration step

`mm__`
:   drm\_mm allocator to walk

`start__`
:   starting offset, the first node will overlap this

`end__`
:   ending offset, the last node will start before this (but may overlap)

**Description**

This iterator walks over all nodes in the range allocator that lie
between **start** and **end**. It is implemented similarly to [`list_for_each()`](../core-api/list.html#c.list_for_each "list_for_each"),
but using the internal interval tree to accelerate the search for the
starting node, and so not safe against removal of elements. It assumes
that **end** is within (or is the upper limit of) the drm\_mm allocator.
If [**start**, **end**] are beyond the range of the drm\_mm, the iterator may walk
over the special \_unallocated\_ [`drm_mm.head_node`](#c.drm_mm "drm_mm"), and may even continue
indefinitely.

void drm\_mm\_scan\_init(struct [drm\_mm\_scan](#c.drm_mm_scan "drm_mm_scan") \*scan, struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, u64 size, u64 alignment, unsigned long color, enum [drm\_mm\_insert\_mode](#c.drm_mm_insert_mode "drm_mm_insert_mode") mode)
:   initialize lru scanning

**Parameters**

`struct drm_mm_scan *scan`
:   scan state

`struct drm_mm *mm`
:   drm\_mm to scan

`u64 size`
:   size of the allocation

`u64 alignment`
:   alignment of the allocation

`unsigned long color`
:   opaque tag value to use for the allocation

`enum drm_mm_insert_mode mode`
:   fine-tune the allocation search and placement

**Description**

This is a simplified version of [`drm_mm_scan_init_with_range()`](#c.drm_mm_scan_init_with_range "drm_mm_scan_init_with_range") with no range
restrictions applied.

This simply sets up the scanning routines with the parameters for the desired
hole.

Warning:
As long as the scan list is non-empty, no other operations than
adding/removing nodes to/from the scan list are allowed.

int drm\_mm\_reserve\_node(struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node)
:   insert an pre-initialized node

**Parameters**

`struct drm_mm *mm`
:   drm\_mm allocator to insert **node** into

`struct drm_mm_node *node`
:   drm\_mm\_node to insert

**Description**

This functions inserts an already set-up [`drm_mm_node`](#c.drm_mm_node "drm_mm_node") into the allocator,
meaning that start, size and color must be set by the caller. All other
fields must be cleared to 0. This is useful to initialize the allocator with
preallocated objects which must be set-up before the range allocator can be
set-up, e.g. when taking over a firmware framebuffer.

**Return**

0 on success, -ENOSPC if there’s no hole where **node** is.

int drm\_mm\_insert\_node\_in\_range(struct [drm\_mm](#c.drm_mm "drm_mm") \*const mm, struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*const node, u64 size, u64 alignment, unsigned long color, u64 range\_start, u64 range\_end, enum [drm\_mm\_insert\_mode](#c.drm_mm_insert_mode "drm_mm_insert_mode") mode)
:   ranged search for space and insert **node**

**Parameters**

`struct drm_mm * const mm`
:   drm\_mm to allocate from

`struct drm_mm_node * const node`
:   preallocate node to insert

`u64 size`
:   size of the allocation

`u64 alignment`
:   alignment of the allocation

`unsigned long color`
:   opaque tag value to use for this node

`u64 range_start`
:   start of the allowed range for this node

`u64 range_end`
:   end of the allowed range for this node

`enum drm_mm_insert_mode mode`
:   fine-tune the allocation search and placement

**Description**

The preallocated **node** must be cleared to 0.

**Return**

0 on success, -ENOSPC if there’s no suitable hole.

void drm\_mm\_remove\_node(struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node)
:   Remove a memory node from the allocator.

**Parameters**

`struct drm_mm_node *node`
:   drm\_mm\_node to remove

**Description**

This just removes a node from its drm\_mm allocator. The node does not need to
be cleared again before it can be re-inserted into this or any other drm\_mm
allocator. It is a bug to call this function on a unallocated node.

void drm\_mm\_scan\_init\_with\_range(struct [drm\_mm\_scan](#c.drm_mm_scan "drm_mm_scan") \*scan, struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, u64 size, u64 alignment, unsigned long color, u64 start, u64 end, enum [drm\_mm\_insert\_mode](#c.drm_mm_insert_mode "drm_mm_insert_mode") mode)
:   initialize range-restricted lru scanning

**Parameters**

`struct drm_mm_scan *scan`
:   scan state

`struct drm_mm *mm`
:   drm\_mm to scan

`u64 size`
:   size of the allocation

`u64 alignment`
:   alignment of the allocation

`unsigned long color`
:   opaque tag value to use for the allocation

`u64 start`
:   start of the allowed range for the allocation

`u64 end`
:   end of the allowed range for the allocation

`enum drm_mm_insert_mode mode`
:   fine-tune the allocation search and placement

**Description**

This simply sets up the scanning routines with the parameters for the desired
hole.

Warning:
As long as the scan list is non-empty, no other operations than
adding/removing nodes to/from the scan list are allowed.

bool drm\_mm\_scan\_add\_block(struct [drm\_mm\_scan](#c.drm_mm_scan "drm_mm_scan") \*scan, struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node)
:   add a node to the scan list

**Parameters**

`struct drm_mm_scan *scan`
:   the active drm\_mm scanner

`struct drm_mm_node *node`
:   drm\_mm\_node to add

**Description**

Add a node to the scan list that might be freed to make space for the desired
hole.

**Return**

True if a hole has been found, false otherwise.

bool drm\_mm\_scan\_remove\_block(struct [drm\_mm\_scan](#c.drm_mm_scan "drm_mm_scan") \*scan, struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*node)
:   remove a node from the scan list

**Parameters**

`struct drm_mm_scan *scan`
:   the active drm\_mm scanner

`struct drm_mm_node *node`
:   drm\_mm\_node to remove

**Description**

Nodes **must** be removed in exactly the reverse order from the scan list as
they have been added (e.g. using [`list_add()`](../core-api/list.html#c.list_add "list_add") as they are added and then
[`list_for_each()`](../core-api/list.html#c.list_for_each "list_for_each") over that eviction list to remove), otherwise the internal
state of the memory manager will be corrupted.

When the scan list is empty, the selected memory nodes can be freed. An
immediately following `drm_mm_insert_node_in_range_generic()` or one of the
simpler versions of that function with !DRM\_MM\_SEARCH\_BEST will then return
the just freed block (because it’s at the top of the free\_stack list).

**Return**

True if this block should be evicted, false otherwise. Will always
return false when no hole has been found.

struct [drm\_mm\_node](#c.drm_mm_node "drm_mm_node") \*drm\_mm\_scan\_color\_evict(struct [drm\_mm\_scan](#c.drm_mm_scan "drm_mm_scan") \*scan)
:   evict overlapping nodes on either side of hole

**Parameters**

`struct drm_mm_scan *scan`
:   drm\_mm scan with target hole

**Description**

After completing an eviction scan and removing the selected nodes, we may
need to remove a few more nodes from either side of the target hole if
mm.color\_adjust is being used.

**Return**

A node to evict, or NULL if there are no overlapping nodes.

void drm\_mm\_init(struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, u64 start, u64 size)
:   initialize a drm-mm allocator

**Parameters**

`struct drm_mm *mm`
:   the drm\_mm structure to initialize

`u64 start`
:   start of the range managed by **mm**

`u64 size`
:   end of the range managed by **mm**

**Description**

Note that **mm** must be cleared to 0 before calling this function.

void drm\_mm\_takedown(struct [drm\_mm](#c.drm_mm "drm_mm") \*mm)
:   clean up a drm\_mm allocator

**Parameters**

`struct drm_mm *mm`
:   drm\_mm allocator to clean up

**Description**

Note that it is a bug to call this function on an allocator which is not
clean.

void drm\_mm\_print(const struct [drm\_mm](#c.drm_mm "drm_mm") \*mm, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p)
:   print allocator state

**Parameters**

`const struct drm_mm *mm`
:   drm\_mm allocator to print

`struct drm_printer *p`
:   DRM printer to use

## DRM GPUVM

### Overview

The DRM GPU VA Manager, represented by [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") keeps track of a
GPU’s virtual address (VA) space and manages the corresponding virtual
mappings represented by [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") objects. It also keeps track of the
mapping’s backing [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") buffers.

[`drm_gem_object`](#c.drm_gem_object "drm_gem_object") buffers maintain a list of [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") objects representing
all existing GPU VA mappings using this [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") as backing buffer.

GPU VAs can be flagged as sparse, such that drivers may use GPU VAs to also
keep track of sparse PTEs in order to support Vulkan ‘Sparse Resources’.

The GPU VA manager internally uses a rb-tree to manage the
[`drm_gpuva`](#c.drm_gpuva "drm_gpuva") mappings within a GPU’s virtual address space.

The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") structure contains a special [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") representing the
portion of VA space reserved by the kernel. This node is initialized together
with the GPU VA manager instance and removed when the GPU VA manager is
destroyed.

In a typical application drivers would embed [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and
[`struct drm_gpuva`](#c.drm_gpuva "drm_gpuva") within their own driver specific structures, there won’t be
any memory allocations of its own nor memory allocations of [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")
entries.

The data structures needed to store `drm_gpuvas` within the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") are
contained within [`struct drm_gpuva`](#c.drm_gpuva "drm_gpuva") already. Hence, for inserting [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")
entries from within dma-fence signalling critical sections it is enough to
pre-allocate the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structures.

`drm_gem_objects` which are private to a single VM can share a common
[`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") in order to improve locking efficiency (e.g. with [`drm_exec`](#c.drm_exec "drm_exec")).
For this purpose drivers must pass a [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") to [`drm_gpuvm_init()`](#c.drm_gpuvm_init "drm_gpuvm_init"), in
the following called ‘resv object’, which serves as the container of the
GPUVM’s shared [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv"). This resv object can be a driver specific
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object"), such as the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") containing the root page table,
but it can also be a ‘dummy’ object, which can be allocated with
[`drm_gpuvm_resv_object_alloc()`](#c.drm_gpuvm_resv_object_alloc "drm_gpuvm_resv_object_alloc").

In order to connect a [`struct drm_gpuva`](#c.drm_gpuva "drm_gpuva") to its backing [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") each
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object") maintains a list of [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") structures, and each
[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") contains a list of [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structures.

A [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") is an abstraction that represents a combination of a
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and a [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"). Every such combination should be unique.
This is ensured by the API through `drm_gpuvm_bo_obtain()` and
[`drm_gpuvm_bo_obtain_prealloc()`](#c.drm_gpuvm_bo_obtain_prealloc "drm_gpuvm_bo_obtain_prealloc") which first look into the corresponding
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object") list of `drm_gpuvm_bos` for an existing instance of this
particular combination. If not present, a new instance is created and linked
to the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object").

[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") structures, since unique for a given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm"), are also used
as entry for the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s lists of external and evicted objects. Those
lists are maintained in order to accelerate locking of dma-resv locks and
validation of evicted objects bound in a [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm"). For instance, all
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object")’s [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") of a given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") can be locked by calling
[`drm_gpuvm_exec_lock()`](#c.drm_gpuvm_exec_lock "drm_gpuvm_exec_lock"). Once locked drivers can call [`drm_gpuvm_validate()`](#c.drm_gpuvm_validate "drm_gpuvm_validate") in
order to validate all evicted `drm_gem_objects`. It is also possible to lock
additional `drm_gem_objects` by providing the corresponding parameters to
[`drm_gpuvm_exec_lock()`](#c.drm_gpuvm_exec_lock "drm_gpuvm_exec_lock") as well as open code the [`drm_exec`](#c.drm_exec "drm_exec") loop while making
use of helper functions such as [`drm_gpuvm_prepare_range()`](#c.drm_gpuvm_prepare_range "drm_gpuvm_prepare_range") or
[`drm_gpuvm_prepare_objects()`](#c.drm_gpuvm_prepare_objects "drm_gpuvm_prepare_objects").

Every bound [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") is treated as external object when its [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")
structure is different than the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s common [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") structure.

### Split and Merge

Besides its capability to manage and represent a GPU VA space, the
GPU VA manager also provides functions to let the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") calculate a
sequence of operations to satisfy a given map or unmap request.

Therefore the DRM GPU VA manager provides an algorithm implementing splitting
and merging of existing GPU VA mappings with the ones that are requested to
be mapped or unmapped. This feature is required by the Vulkan API to
implement Vulkan ‘Sparse Memory Bindings’ - drivers UAPIs often refer to this
as VM BIND.

Drivers can call [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") to receive a sequence of callbacks
containing map, unmap and remap operations for a given newly requested
mapping. The sequence of callbacks represents the set of operations to
execute in order to integrate the new mapping cleanly into the current state
of the GPU VA space.

Depending on how the new GPU VA mapping intersects with the existing mappings
of the GPU VA space the [`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") callbacks contain an arbitrary amount
of unmap operations, a maximum of two remap operations and a single map
operation. The caller might receive no callback at all if no operation is
required, e.g. if the requested mapping already exists in the exact same way.

The single map operation represents the original map operation requested by
the caller.

[`drm_gpuva_op_unmap`](#c.drm_gpuva_op_unmap "drm_gpuva_op_unmap") contains a ‘keep’ field, which indicates whether the
[`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to unmap is physically contiguous with the original mapping
request. Optionally, if ‘keep’ is set, drivers may keep the actual page table
entries for this [`drm_gpuva`](#c.drm_gpuva "drm_gpuva"), adding the missing page table entries only and
update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of things accordingly.

Drivers may do the same optimization, namely delta page table updates, also
for remap operations. This is possible since [`drm_gpuva_op_remap`](#c.drm_gpuva_op_remap "drm_gpuva_op_remap") consists of
one unmap operation and one or two map operations, such that drivers can
derive the page table update delta accordingly.

Note that there can’t be more than two existing mappings to split up, one at
the beginning and one at the end of the new mapping, hence there is a
maximum of two remap operations.

Analogous to [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") [`drm_gpuvm_sm_unmap()`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") uses [`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") to
call back into the driver in order to unmap a range of GPU VA space. The
logic behind this function is way simpler though: For all existing mappings
enclosed by the given range unmap operations are created. For mappings which
are only partially located within the given range, remap operations are
created such that those mappings are split up and re-mapped partially.

As an alternative to [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") and [`drm_gpuvm_sm_unmap()`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap"),
[`drm_gpuvm_sm_map_ops_create()`](#c.drm_gpuvm_sm_map_ops_create "drm_gpuvm_sm_map_ops_create") and [`drm_gpuvm_sm_unmap_ops_create()`](#c.drm_gpuvm_sm_unmap_ops_create "drm_gpuvm_sm_unmap_ops_create") can be used
to directly obtain an instance of [`struct drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") containing a list of
[`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op"), which can be iterated with [`drm_gpuva_for_each_op()`](#c.drm_gpuva_for_each_op "drm_gpuva_for_each_op"). This list
contains the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") analogous to the callbacks one would receive when
calling [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") or [`drm_gpuvm_sm_unmap()`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap"). While this way requires
more memory (to allocate the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops")), it provides drivers a way to
iterate the [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") multiple times, e.g. once in a context where memory
allocations are possible (e.g. to allocate GPU page tables) and once in the
dma-fence signalling critical path.

To update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space [`drm_gpuva_insert()`](#c.drm_gpuva_insert "drm_gpuva_insert") and
[`drm_gpuva_remove()`](#c.drm_gpuva_remove "drm_gpuva_remove") may be used. These functions can safely be used from
[`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") callbacks originating from [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") or
[`drm_gpuvm_sm_unmap()`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap"). However, it might be more convenient to use the
provided helper functions [`drm_gpuva_map()`](#c.drm_gpuva_map "drm_gpuva_map"), [`drm_gpuva_remap()`](#c.drm_gpuva_remap "drm_gpuva_remap") and
[`drm_gpuva_unmap()`](#c.drm_gpuva_unmap "drm_gpuva_unmap") instead.

The following diagram depicts the basic relationships of existing GPU VA
mappings, a newly requested mapping and the resulting mappings as implemented
by [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") - it doesn’t cover any arbitrary combinations of these.

1. Requested mapping is identical. Replace it, but indicate the backing PTEs
   could be kept.

   ```
        0     a     1
   old: |-----------| (bo_offset=n)

        0     a     1
   req: |-----------| (bo_offset=n)

        0     a     1
   new: |-----------| (bo_offset=n)
   ```
2. Requested mapping is identical, except for the BO offset, hence replace
   the mapping.

   ```
        0     a     1
   old: |-----------| (bo_offset=n)

        0     a     1
   req: |-----------| (bo_offset=m)

        0     a     1
   new: |-----------| (bo_offset=m)
   ```
3. Requested mapping is identical, except for the backing BO, hence replace
   the mapping.

   ```
        0     a     1
   old: |-----------| (bo_offset=n)

        0     b     1
   req: |-----------| (bo_offset=n)

        0     b     1
   new: |-----------| (bo_offset=n)
   ```
4. Existent mapping is a left aligned subset of the requested one, hence
   replace the existing one.

   ```
        0  a  1
   old: |-----|       (bo_offset=n)

        0     a     2
   req: |-----------| (bo_offset=n)

        0     a     2
   new: |-----------| (bo_offset=n)
   ```

   Note

   We expect to see the same result for a request with a different BO
   and/or non-contiguous BO offset.
5. Requested mapping’s range is a left aligned subset of the existing one,
   but backed by a different BO. Hence, map the requested mapping and split
   the existing one adjusting its BO offset.

   ```
        0     a     2
   old: |-----------| (bo_offset=n)

        0  b  1
   req: |-----|       (bo_offset=n)

        0  b  1  a' 2
   new: |-----|-----| (b.bo_offset=n, a.bo_offset=n+1)
   ```

   Note

   We expect to see the same result for a request with a different BO
   and/or non-contiguous BO offset.
6. Existent mapping is a superset of the requested mapping. Split it up, but
   indicate that the backing PTEs could be kept.

   ```
        0     a     2
   old: |-----------| (bo_offset=n)

        0  a  1
   req: |-----|       (bo_offset=n)

        0  a  1  a' 2
   new: |-----|-----| (a.bo_offset=n, a'.bo_offset=n+1)
   ```
7. Requested mapping’s range is a right aligned subset of the existing one,
   but backed by a different BO. Hence, map the requested mapping and split
   the existing one, without adjusting the BO offset.

   ```
        0     a     2
   old: |-----------| (bo_offset=n)

              1  b  2
   req:       |-----| (bo_offset=m)

        0  a  1  b  2
   new: |-----|-----| (a.bo_offset=n,b.bo_offset=m)
   ```
8. Existent mapping is a superset of the requested mapping. Split it up, but
   indicate that the backing PTEs could be kept.

   ```
         0     a     2
   old: |-----------| (bo_offset=n)

              1  a  2
   req:       |-----| (bo_offset=n+1)

        0  a' 1  a  2
   new: |-----|-----| (a'.bo_offset=n, a.bo_offset=n+1)
   ```
9. Existent mapping is overlapped at the end by the requested mapping backed
   by a different BO. Hence, map the requested mapping and split up the
   existing one, without adjusting the BO offset.

   ```
        0     a     2
   old: |-----------|       (bo_offset=n)

              1     b     3
   req:       |-----------| (bo_offset=m)

        0  a  1     b     3
   new: |-----|-----------| (a.bo_offset=n,b.bo_offset=m)
   ```
10. Existent mapping is overlapped by the requested mapping, both having the
    same backing BO with a contiguous offset. Indicate the backing PTEs of
    the old mapping could be kept.

    ```
         0     a     2
    old: |-----------|       (bo_offset=n)

               1     a     3
    req:       |-----------| (bo_offset=n+1)

         0  a' 1     a     3
    new: |-----|-----------| (a'.bo_offset=n, a.bo_offset=n+1)
    ```
11. Requested mapping’s range is a centered subset of the existing one
    having a different backing BO. Hence, map the requested mapping and split
    up the existing one in two mappings, adjusting the BO offset of the right
    one accordingly.

    ```
         0        a        3
    old: |-----------------| (bo_offset=n)

               1  b  2
    req:       |-----|       (bo_offset=m)

         0  a  1  b  2  a' 3
    new: |-----|-----|-----| (a.bo_offset=n,b.bo_offset=m,a'.bo_offset=n+2)
    ```
12. Requested mapping is a contiguous subset of the existing one. Split it
    up, but indicate that the backing PTEs could be kept.

    ```
         0        a        3
    old: |-----------------| (bo_offset=n)

               1  a  2
    req:       |-----|       (bo_offset=n+1)

         0  a' 1  a  2 a'' 3
    old: |-----|-----|-----| (a'.bo_offset=n, a.bo_offset=n+1, a''.bo_offset=n+2)
    ```
13. Existent mapping is a right aligned subset of the requested one, hence
    replace the existing one.

    ```
               1  a  2
    old:       |-----| (bo_offset=n+1)

         0     a     2
    req: |-----------| (bo_offset=n)

         0     a     2
    new: |-----------| (bo_offset=n)
    ```

    Note

    We expect to see the same result for a request with a different bo
    and/or non-contiguous bo\_offset.
14. Existent mapping is a centered subset of the requested one, hence
    replace the existing one.

    ```
               1  a  2
    old:       |-----| (bo_offset=n+1)

         0        a       3
    req: |----------------| (bo_offset=n)

         0        a       3
    new: |----------------| (bo_offset=n)
    ```

    Note

    We expect to see the same result for a request with a different bo
    and/or non-contiguous bo\_offset.
15. Existent mappings is overlapped at the beginning by the requested mapping
    backed by a different BO. Hence, map the requested mapping and split up
    the existing one, adjusting its BO offset accordingly.

    ```
               1     a     3
    old:       |-----------| (bo_offset=n)

         0     b     2
    req: |-----------|       (bo_offset=m)

         0     b     2  a' 3
    new: |-----------|-----| (b.bo_offset=m,a.bo_offset=n+2)
    ```

### Locking

In terms of managing [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") entries DRM GPUVM does not take care of
locking itself, it is the drivers responsibility to take care about locking.
Drivers might want to protect the following operations: inserting, removing
and iterating [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") objects as well as generating all kinds of
operations, such as split / merge or prefetch.

DRM GPUVM also does not take care of the locking of the backing
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object") buffers GPU VA lists and [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") abstractions by
itself; drivers are responsible to enforce mutual exclusion using either the
GEMs dma\_resv lock or the GEMs gpuva.lock mutex.

However, DRM GPUVM contains lockdep checks to ensure callers of its API hold
the corresponding lock whenever the `drm_gem_objects` GPU VA list is accessed
by functions such as [`drm_gpuva_link()`](#c.drm_gpuva_link "drm_gpuva_link") or [`drm_gpuva_unlink()`](#c.drm_gpuva_unlink "drm_gpuva_unlink"), but also
`drm_gpuvm_bo_obtain()` and [`drm_gpuvm_bo_put()`](#c.drm_gpuvm_bo_put "drm_gpuvm_bo_put").

The latter is required since on creation and destruction of a [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")
the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") is attached / removed from the `drm_gem_objects` gpuva list.
Subsequent calls to `drm_gpuvm_bo_obtain()` for the same [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object") must be able to observe previous creations and destructions
of `drm_gpuvm_bos` in order to keep instances unique.

The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s lists for keeping track of external and evicted objects are
protected against concurrent insertion / removal and iteration internally.

However, drivers still need ensure to protect concurrent calls to functions
iterating those lists, namely [`drm_gpuvm_prepare_objects()`](#c.drm_gpuvm_prepare_objects "drm_gpuvm_prepare_objects") and
[`drm_gpuvm_validate()`](#c.drm_gpuvm_validate "drm_gpuvm_validate").

Alternatively, drivers can set the `DRM_GPUVM_RESV_PROTECTED` flag to indicate
that the corresponding [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") locks are held in order to protect the
lists. If `DRM_GPUVM_RESV_PROTECTED` is set, internal locking is disabled and
the corresponding lockdep checks are enabled. This is an optimization for
drivers which are capable of taking the corresponding [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") locks and
hence do not require internal locking.

### Examples

This section gives two examples on how to let the DRM GPUVA Manager generate
[`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") in order to satisfy a given map or unmap request and how to
make use of them.

The below code is strictly limited to illustrate the generic usage pattern.
To maintain simplicity, it doesn’t make use of any abstractions for common
code, different (asynchronous) stages with fence signalling critical paths,
any other helpers or error handling in terms of freeing memory and dropping
previously taken locks.

1. Obtain a list of [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to create a new mapping:

   ```
   // Allocates a new &drm_gpuva.
   struct drm_gpuva * driver_gpuva_alloc(void);

   // Typically drivers would embed the &drm_gpuvm and &drm_gpuva
   // structure in individual driver structures and lock the dma-resv with
   // drm_exec or similar helpers.
   int driver_mapping_create(struct drm_gpuvm *gpuvm,
                             u64 addr, u64 range,
                             struct drm_gem_object *obj, u64 offset)
   {
           struct drm_gpuvm_map_req map_req = {
                   .map.va.addr = addr,
                   .map.va.range = range,
                   .map.gem.obj = obj,
                   .map.gem.offset = offset,
              };
           struct drm_gpuva_ops *ops;
           struct drm_gpuva_op *op
           struct drm_gpuvm_bo *vm_bo;

           driver_lock_va_space();
           ops = drm_gpuvm_sm_map_ops_create(gpuvm, &map_req);
           if (IS_ERR(ops))
                   return PTR_ERR(ops);

           vm_bo = drm_gpuvm_bo_obtain(gpuvm, obj);
           if (IS_ERR(vm_bo))
                   return PTR_ERR(vm_bo);

           drm_gpuva_for_each_op(op, ops) {
                   struct drm_gpuva *va;

                   switch (op->op) {
                   case DRM_GPUVA_OP_MAP:
                           va = driver_gpuva_alloc();
                           if (!va)
                                   ; // unwind previous VA space updates,
                                     // free memory and unlock

                           driver_vm_map();
                           drm_gpuva_map(gpuvm, va, &op->map);
                           drm_gpuva_link(va, vm_bo);

                           break;
                   case DRM_GPUVA_OP_REMAP: {
                           struct drm_gpuva *prev = NULL, *next = NULL;

                           va = op->remap.unmap->va;

                           if (op->remap.prev) {
                                   prev = driver_gpuva_alloc();
                                   if (!prev)
                                           ; // unwind previous VA space
                                             // updates, free memory and
                                             // unlock
                           }

                           if (op->remap.next) {
                                   next = driver_gpuva_alloc();
                                   if (!next)
                                           ; // unwind previous VA space
                                             // updates, free memory and
                                             // unlock
                           }

                           driver_vm_remap();
                           drm_gpuva_remap(prev, next, &op->remap);

                           if (prev)
                                   drm_gpuva_link(prev, va->vm_bo);
                           if (next)
                                   drm_gpuva_link(next, va->vm_bo);
                           drm_gpuva_unlink(va);

                           break;
                   }
                   case DRM_GPUVA_OP_UNMAP:
                           va = op->unmap->va;

                           driver_vm_unmap();
                           drm_gpuva_unlink(va);
                           drm_gpuva_unmap(&op->unmap);

                           break;
                   default:
                           break;
                   }
           }
           drm_gpuvm_bo_put(vm_bo);
           driver_unlock_va_space();

           return 0;
   }
   ```
2. Receive a callback for each [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to create a new mapping:

   ```
   struct driver_context {
           struct drm_gpuvm *gpuvm;
           struct drm_gpuvm_bo *vm_bo;
           struct drm_gpuva *new_va;
           struct drm_gpuva *prev_va;
           struct drm_gpuva *next_va;
   };

   // ops to pass to drm_gpuvm_init()
   static const struct drm_gpuvm_ops driver_gpuvm_ops = {
           .sm_step_map = driver_gpuva_map,
           .sm_step_remap = driver_gpuva_remap,
           .sm_step_unmap = driver_gpuva_unmap,
   };

   // Typically drivers would embed the &drm_gpuvm and &drm_gpuva
   // structure in individual driver structures and lock the dma-resv with
   // drm_exec or similar helpers.
   int driver_mapping_create(struct drm_gpuvm *gpuvm,
                             u64 addr, u64 range,
                             struct drm_gem_object *obj, u64 offset)
   {
           struct driver_context ctx;
           struct drm_gpuvm_bo *vm_bo;
           struct drm_gpuva_ops *ops;
           struct drm_gpuva_op *op;
           int ret = 0;

           ctx.gpuvm = gpuvm;

           ctx.new_va = kzalloc(sizeof(*ctx.new_va), GFP_KERNEL);
           ctx.prev_va = kzalloc(sizeof(*ctx.prev_va), GFP_KERNEL);
           ctx.next_va = kzalloc(sizeof(*ctx.next_va), GFP_KERNEL);
           ctx.vm_bo = drm_gpuvm_bo_create(gpuvm, obj);
           if (!ctx.new_va || !ctx.prev_va || !ctx.next_va || !vm_bo) {
                   ret = -ENOMEM;
                   goto out;
           }

           // Typically protected with a driver specific GEM gpuva lock
           // used in the fence signaling path for drm_gpuva_link() and
           // drm_gpuva_unlink(), hence pre-allocate.
           ctx.vm_bo = drm_gpuvm_bo_obtain_prealloc(ctx.vm_bo);

           driver_lock_va_space();
           ret = drm_gpuvm_sm_map(gpuvm, &ctx, addr, range, obj, offset);
           driver_unlock_va_space();

   out:
           drm_gpuvm_bo_put(ctx.vm_bo);
           kfree(ctx.new_va);
           kfree(ctx.prev_va);
           kfree(ctx.next_va);
           return ret;
   }

   int driver_gpuva_map(struct drm_gpuva_op *op, void *__ctx)
   {
           struct driver_context *ctx = __ctx;

           drm_gpuva_map(ctx->vm, ctx->new_va, &op->map);

           drm_gpuva_link(ctx->new_va, ctx->vm_bo);

           // prevent the new GPUVA from being freed in
           // driver_mapping_create()
           ctx->new_va = NULL;

           return 0;
   }

   int driver_gpuva_remap(struct drm_gpuva_op *op, void *__ctx)
   {
           struct driver_context *ctx = __ctx;
           struct drm_gpuva *va = op->remap.unmap->va;

           drm_gpuva_remap(ctx->prev_va, ctx->next_va, &op->remap);

           if (op->remap.prev) {
                   drm_gpuva_link(ctx->prev_va, va->vm_bo);
                   ctx->prev_va = NULL;
           }

           if (op->remap.next) {
                   drm_gpuva_link(ctx->next_va, va->vm_bo);
                   ctx->next_va = NULL;
           }

           drm_gpuva_unlink(va);
           kfree(va);

           return 0;
   }

   int driver_gpuva_unmap(struct drm_gpuva_op *op, void *__ctx)
   {
           drm_gpuva_unlink(op->unmap.va);
           drm_gpuva_unmap(&op->unmap);
           kfree(op->unmap.va);

           return 0;
   }
   ```

### DRM GPUVM Function References

enum drm\_gpuva\_flags
:   flags for [`struct drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Constants**

`DRM_GPUVA_INVALIDATED`
:   Flag indicating that the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")’s backing GEM is invalidated.

`DRM_GPUVA_SPARSE`
:   Flag indicating that the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") is a sparse mapping.

`DRM_GPUVA_USERBITS`
:   user defined bits

struct drm\_gpuva
:   structure to track a GPU VA mapping

**Definition**:

```
struct drm_gpuva {
    struct drm_gpuvm *vm;
    struct drm_gpuvm_bo *vm_bo;
    enum drm_gpuva_flags flags;
    struct {
        u64 addr;
        u64 range;
    } va;
    struct {
        u64 offset;
        struct drm_gem_object *obj;
        struct list_head entry;
    } gem;
    struct {
        struct rb_node node;
        struct list_head entry;
        u64 __subtree_last;
    } rb;
};
```

**Members**

`vm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") this object is associated with

`vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") abstraction for the mapped
    [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

`flags`
:   the [`drm_gpuva_flags`](#c.drm_gpuva_flags "drm_gpuva_flags") for this mapping

`va`
:   structure containing the address and range of the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

`va.addr`
:   the start address

`gem`
:   structure containing the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") and its offset

`gem.offset`
:   the offset within the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

`gem.obj`
:   the mapped [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

`gem.entry`
:   the `list_head` to attach this object to a [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")

`rb`
:   structure containing data to store `drm_gpuvas` in a rb-tree

`rb.node`
:   the rb-tree node

`rb.entry`
:   The `list_head` to additionally connect `drm_gpuvas`
    in the same order they appear in the interval tree. This is
    useful to keep iterating `drm_gpuvas` from a start node found
    through the rb-tree while doing modifications on the rb-tree
    itself.

`rb.__subtree_last`
:   needed by the interval tree, holding last-in-subtree

**Description**

This structure represents a GPU VA mapping and is associated with a
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm").

Typically, this structure is embedded in bigger driver structures.

void drm\_gpuva\_invalidate(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va, bool invalidate)
:   sets whether the backing GEM of this [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") is invalidated

**Parameters**

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to set the invalidate flag for

`bool invalidate`
:   indicates whether the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") is invalidated

bool drm\_gpuva\_invalidated(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va)
:   indicates whether the backing BO of this [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") is invalidated

**Parameters**

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to check

**Return**

`true` if the GPU VA is invalidated, `false` otherwise

enum drm\_gpuvm\_flags
:   flags for [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Constants**

`DRM_GPUVM_RESV_PROTECTED`
:   GPUVM is protected externally by the
    GPUVM’s [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") lock

`DRM_GPUVM_IMMEDIATE_MODE`
:   use the locking scheme for GEMs designed
    for modifying the GPUVM during the fence signalling path

    When set, gpuva.lock is used to protect gpuva.list in all GEM
    objects associated with this GPUVM. Otherwise, the GEMs dma-resv is
    used.

`DRM_GPUVM_USERBITS`
:   user defined bits

struct drm\_gpuvm
:   DRM GPU VA Manager

**Definition**:

```
struct drm_gpuvm {
    const char *name;
    enum drm_gpuvm_flags flags;
    struct drm_device *drm;
    u64 mm_start;
    u64 mm_range;
    struct {
        struct rb_root_cached tree;
        struct list_head list;
    } rb;
    struct kref kref;
    struct drm_gpuva kernel_alloc_node;
    const struct drm_gpuvm_ops *ops;
    struct drm_gem_object *r_obj;
    struct {
        struct list_head list;
        struct list_head *local_list;
        spinlock_t lock;
    } extobj;
    struct {
        struct list_head list;
        struct list_head *local_list;
        spinlock_t lock;
    } evict;
    struct llist_head bo_defer;
};
```

**Members**

`name`
:   the name of the DRM GPU VA space

`flags`
:   the [`drm_gpuvm_flags`](#c.drm_gpuvm_flags "drm_gpuvm_flags") of this GPUVM

`drm`
:   the [`drm_device`](drm-internals.html#c.drm_device "drm_device") this VM lives in

`mm_start`
:   start of the VA space

`mm_range`
:   length of the VA space

`rb`
:   structures to track [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") entries

`rb.tree`
:   the rb-tree to track GPU VA mappings

`rb.list`
:   the `list_head` to track GPU VA mappings

`kref`
:   reference count of this object

`kernel_alloc_node`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") representing the address space cutout reserved for
    the kernel

`ops`
:   [`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") providing the split/merge steps to drivers

`r_obj`
:   Resv GEM object; representing the GPUVM’s common [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv").

`extobj`
:   structure holding the extobj list

`extobj.list`
:   `list_head` storing `drm_gpuvm_bos` serving as
    external object

`extobj.local_list`
:   pointer to the local list temporarily
    storing entries from the external object list

`extobj.lock`
:   spinlock to protect the extobj list

`evict`
:   structure holding the evict list and evict list lock

`evict.list`
:   `list_head` storing `drm_gpuvm_bos` currently
    being evicted

`evict.local_list`
:   pointer to the local list temporarily
    storing entries from the evicted object list

`evict.lock`
:   spinlock to protect the evict list

`bo_defer`
:   structure holding vm\_bos that need to be destroyed

**Description**

The DRM GPU VA Manager keeps track of a GPU’s virtual address space by using
`maple_tree` structures. Typically, this structure is embedded in bigger
driver structures.

Drivers can pass addresses and ranges in an arbitrary unit, e.g. bytes or
pages.

There should be one manager instance per GPU virtual address space.

struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*drm\_gpuvm\_get(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm)
:   acquire a [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") reference

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to acquire the reference of

**Description**

This function acquires an additional reference to **gpuvm**. It is illegal to
call this without already holding a reference. No locks required.

**Return**

the [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") pointer

bool drm\_gpuvm\_resv\_protected(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm)
:   indicates whether `DRM_GPUVM_RESV_PROTECTED` is set

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Return**

true if `DRM_GPUVM_RESV_PROTECTED` is set, false otherwise.

bool drm\_gpuvm\_immediate\_mode(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm)
:   indicates whether `DRM_GPUVM_IMMEDIATE_MODE` is set

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Return**

true if `DRM_GPUVM_IMMEDIATE_MODE` is set, false otherwise.

drm\_gpuvm\_resv

`drm_gpuvm_resv (gpuvm__)`

> returns the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")

**Parameters**

`gpuvm__`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Return**

a pointer to the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s shared [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")

drm\_gpuvm\_resv\_obj

`drm_gpuvm_resv_obj (gpuvm__)`

> returns the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") holding the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")

**Parameters**

`gpuvm__`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Return**

a pointer to the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") holding the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s shared
[`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")

bool drm\_gpuvm\_is\_extobj(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   indicates whether the given [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") is an external object

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to check

`struct drm_gem_object *obj`
:   the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") to check

**Return**

true if the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") differs from the
`drm_gpuvms` [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv"), false otherwise

drm\_gpuvm\_for\_each\_va\_range

`drm_gpuvm_for_each_va_range (va__, gpuvm__, start__, end__)`

> iterate over a range of `drm_gpuvas`

**Parameters**

`va__`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structure to assign to in each iteration step

`gpuvm__`
:   [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to walk over

`start__`
:   starting offset, the first gpuva will overlap this

`end__`
:   ending offset, the last gpuva will start before this (but may
    overlap)

**Description**

This iterator walks over all `drm_gpuvas` in the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") that lie
between **start\_\_** and **end\_\_**. It is implemented similarly to [`list_for_each()`](../core-api/list.html#c.list_for_each "list_for_each"),
but is using the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s internal interval tree to accelerate
the search for the starting [`drm_gpuva`](#c.drm_gpuva "drm_gpuva"), and hence isn’t safe against removal
of elements. It assumes that **end\_\_** is within (or is the upper limit of) the
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm"). This iterator does not skip over the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s
**kernel\_alloc\_node**.

drm\_gpuvm\_for\_each\_va\_range\_safe

`drm_gpuvm_for_each_va_range_safe (va__, next__, gpuvm__, start__, end__)`

> safely iterate over a range of `drm_gpuvas`

**Parameters**

`va__`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to assign to in each iteration step

`next__`
:   another [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to use as temporary storage

`gpuvm__`
:   [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to walk over

`start__`
:   starting offset, the first gpuva will overlap this

`end__`
:   ending offset, the last gpuva will start before this (but may
    overlap)

**Description**

This iterator walks over all `drm_gpuvas` in the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") that lie
between **start\_\_** and **end\_\_**. It is implemented similarly to
[`list_for_each_safe()`](../core-api/list.html#c.list_for_each_safe "list_for_each_safe"), but is using the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s internal interval
tree to accelerate the search for the starting [`drm_gpuva`](#c.drm_gpuva "drm_gpuva"), and hence is safe
against removal of elements. It assumes that **end\_\_** is within (or is the
upper limit of) the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm"). This iterator does not skip over the
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s **kernel\_alloc\_node**.

drm\_gpuvm\_for\_each\_va

`drm_gpuvm_for_each_va (va__, gpuvm__)`

> iterate over all `drm_gpuvas`

**Parameters**

`va__`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to assign to in each iteration step

`gpuvm__`
:   [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to walk over

**Description**

This iterator walks over all [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structures associated with the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm").

drm\_gpuvm\_for\_each\_va\_safe

`drm_gpuvm_for_each_va_safe (va__, next__, gpuvm__)`

> safely iterate over all `drm_gpuvas`

**Parameters**

`va__`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to assign to in each iteration step

`next__`
:   another [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to use as temporary storage

`gpuvm__`
:   [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to walk over

**Description**

This iterator walks over all [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structures associated with the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm"). It is implemented with [`list_for_each_entry_safe()`](../core-api/list.html#c.list_for_each_entry_safe "list_for_each_entry_safe"), and
hence safe against the removal of elements.

struct drm\_gpuvm\_exec
:   [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") abstraction of [`drm_exec`](#c.drm_exec "drm_exec")

**Definition**:

```
struct drm_gpuvm_exec {
    struct drm_exec exec;
    u32 flags;
    struct drm_gpuvm *vm;
    unsigned int num_fences;
    struct {
        int (*fn)(struct drm_gpuvm_exec *vm_exec);
        void *priv;
    } extra;
};
```

**Members**

`exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") structure

`flags`
:   the flags for the [`struct drm_exec`](#c.drm_exec "drm_exec")

`vm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to lock its DMA reservations

`num_fences`
:   the number of fences to reserve for the [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") of the
    locked `drm_gem_objects`

`extra`
:   Callback and corresponding private data for the driver to
    lock arbitrary additional `drm_gem_objects`.

`extra.fn`
:   The driver callback to lock additional
    `drm_gem_objects`.

`extra.priv`
:   driver private data for the **fn** callback

**Description**

This structure should be created on the stack as [`drm_exec`](#c.drm_exec "drm_exec") should be.

Optionally, **extra** can be set in order to lock additional `drm_gem_objects`.

void drm\_gpuvm\_exec\_unlock(struct [drm\_gpuvm\_exec](#c.drm_gpuvm_exec "drm_gpuvm_exec") \*vm\_exec)
:   lock all dma-resv of all assoiciated BOs

**Parameters**

`struct drm_gpuvm_exec *vm_exec`
:   the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") wrapper

**Description**

Releases all dma-resv locks of all `drm_gem_objects` previously acquired
through [`drm_gpuvm_exec_lock()`](#c.drm_gpuvm_exec_lock "drm_gpuvm_exec_lock") or its variants.

**Return**

0 on success, negative error code on failure.

void drm\_gpuvm\_exec\_resv\_add\_fence(struct [drm\_gpuvm\_exec](#c.drm_gpuvm_exec "drm_gpuvm_exec") \*vm\_exec, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, enum [dma\_resv\_usage](../driver-api/dma-buf.html#c.dma_resv_usage "dma_resv_usage") private\_usage, enum [dma\_resv\_usage](../driver-api/dma-buf.html#c.dma_resv_usage "dma_resv_usage") extobj\_usage)
:   add fence to private and all extobj

**Parameters**

`struct drm_gpuvm_exec *vm_exec`
:   the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") wrapper

`struct dma_fence *fence`
:   fence to add

`enum dma_resv_usage private_usage`
:   private dma-resv usage

`enum dma_resv_usage extobj_usage`
:   extobj dma-resv usage

**Description**

See [`drm_gpuvm_resv_add_fence()`](#c.drm_gpuvm_resv_add_fence "drm_gpuvm_resv_add_fence").

int drm\_gpuvm\_exec\_validate(struct [drm\_gpuvm\_exec](#c.drm_gpuvm_exec "drm_gpuvm_exec") \*vm\_exec)
:   validate all BOs marked as evicted

**Parameters**

`struct drm_gpuvm_exec *vm_exec`
:   the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") wrapper

**Description**

See [`drm_gpuvm_validate()`](#c.drm_gpuvm_validate "drm_gpuvm_validate").

**Return**

0 on success, negative error code on failure.

struct drm\_gpuvm\_bo
:   structure representing a [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") combination

**Definition**:

```
struct drm_gpuvm_bo {
    struct drm_gpuvm *vm;
    struct drm_gem_object *obj;
    bool evicted;
    struct kref kref;
    struct {
        struct list_head gpuva;
        struct {
            struct list_head gem;
            struct list_head extobj;
            struct list_head evict;
            struct llist_node bo_defer;
        } entry;
    } list;
};
```

**Members**

`vm`
:   The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") the **obj** is mapped in. This is a reference
    counted pointer.

`obj`
:   The [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") being mapped in **vm**. This is a reference
    counted pointer.

`evicted`
:   Indicates whether the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") is evicted; field
    protected by the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")’s dma-resv lock.

`kref`
:   The reference count for this [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo").

`list`
:   Structure containing all `list_heads`.

`list.gpuva`
:   The list of linked `drm_gpuvas`.

    It is safe to access entries from this list as long as the
    GEM’s gpuva lock is held. See also [`struct drm_gem_object`](#c.drm_gem_object "drm_gem_object").

`list.entry`
:   Structure containing all `list_heads` serving as
    entry.

`list.entry.gem`
:   List entry to attach to the
    `drm_gem_objects` gpuva list.

`list.entry.evict`
:   List entry to attach to the
    `drm_gpuvms` evict list.

`list.entry.bo_defer`
:   List entry to attach to
    the `drm_gpuvms` bo\_defer list.

**Description**

This structure is an abstraction representing a [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object") combination. It serves as an indirection to accelerate
iterating all `drm_gpuvas` within a [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") backed by the same
[`drm_gem_object`](#c.drm_gem_object "drm_gem_object").

Furthermore it is used cache evicted GEM objects for a certain GPU-VM to
accelerate validation.

Typically, drivers want to create an instance of a [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") once
a GEM object is mapped first in a GPU-VM and release the instance once the
last mapping of the GEM object in this GPU-VM is unmapped.

struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*drm\_gpuvm\_bo\_get(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo)
:   acquire a [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") reference

**Parameters**

`struct drm_gpuvm_bo *vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to acquire the reference of

**Description**

This function acquires an additional reference to **vm\_bo**. It is illegal to
call this without already holding a reference. No locks required.

**Return**

the `struct vm_bo` pointer

void drm\_gpuvm\_bo\_gem\_evict(struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, bool evict)
:   add/remove all [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")’s in the list to/from the `drm_gpuvms` evicted list

**Parameters**

`struct drm_gem_object *obj`
:   the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

`bool evict`
:   indicates whether **obj** is evicted

**Description**

See [`drm_gpuvm_bo_evict()`](#c.drm_gpuvm_bo_evict "drm_gpuvm_bo_evict").

drm\_gpuvm\_bo\_for\_each\_va

`drm_gpuvm_bo_for_each_va (va__, vm_bo__)`

> iterator to walk over a list of [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`va__`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structure to assign to in each iteration step

`vm_bo__`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to walk are associated with

**Description**

This iterator walks over all [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structures associated with the
[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo").

The caller must hold the GEM’s gpuva lock.

drm\_gpuvm\_bo\_for\_each\_va\_safe

`drm_gpuvm_bo_for_each_va_safe (va__, next__, vm_bo__)`

> iterator to safely walk over a list of [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`va__`
:   [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structure to assign to in each iteration step

`next__`
:   `next` [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to store the next step

`vm_bo__`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to walk are associated with

**Description**

This iterator walks over all [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structures associated with the
[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo"). It is implemented with [`list_for_each_entry_safe()`](../core-api/list.html#c.list_for_each_entry_safe "list_for_each_entry_safe"), hence
it is save against removal of elements.

The caller must hold the GEM’s gpuva lock.

enum drm\_gpuva\_op\_type
:   GPU VA operation type

**Constants**

`DRM_GPUVA_OP_MAP`
:   the map op type

`DRM_GPUVA_OP_REMAP`
:   the remap op type

`DRM_GPUVA_OP_UNMAP`
:   the unmap op type

`DRM_GPUVA_OP_PREFETCH`
:   the prefetch op type

`DRM_GPUVA_OP_DRIVER`
:   the driver defined op type

**Description**

Operations to alter the GPU VA mappings tracked by the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm").

struct drm\_gpuva\_op\_map
:   GPU VA map operation

**Definition**:

```
struct drm_gpuva_op_map {
    struct {
        u64 addr;
        u64 range;
    } va;
    struct {
        u64 offset;
        struct drm_gem_object *obj;
    } gem;
};
```

**Members**

`va`
:   structure containing address and range of a map
    operation

`va.addr`
:   the base address of the new mapping

`va.range`
:   the range of the new mapping

`gem`
:   structure containing the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") and its offset

`gem.offset`
:   the offset within the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

`gem.obj`
:   the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") to map

**Description**

This structure represents a single map operation generated by the
DRM GPU VA manager.

struct drm\_gpuva\_op\_unmap
:   GPU VA unmap operation

**Definition**:

```
struct drm_gpuva_op_unmap {
    struct drm_gpuva *va;
    bool keep;
};
```

**Members**

`va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to unmap

`keep`
:   Indicates whether this [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") is physically contiguous with the
    original mapping request.

    Optionally, if `keep` is set, drivers may keep the actual page table
    mappings for this [`drm_gpuva`](#c.drm_gpuva "drm_gpuva"), adding the missing page table entries
    only and update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") accordingly.

**Description**

This structure represents a single unmap operation generated by the
DRM GPU VA manager.

struct drm\_gpuva\_op\_remap
:   GPU VA remap operation

**Definition**:

```
struct drm_gpuva_op_remap {
    struct drm_gpuva_op_map *prev;
    struct drm_gpuva_op_map *next;
    struct drm_gpuva_op_unmap *unmap;
};
```

**Members**

`prev`
:   the preceding part of a split mapping

`next`
:   the subsequent part of a split mapping

`unmap`
:   the unmap operation for the original existing mapping

**Description**

This represents a single remap operation generated by the DRM GPU VA manager.

A remap operation is generated when an existing GPU VA mmapping is split up
by inserting a new GPU VA mapping or by partially unmapping existent
mapping(s), hence it consists of a maximum of two map and one unmap
operation.

The **unmap** operation takes care of removing the original existing mapping.
**prev** is used to remap the preceding part, **next** the subsequent part.

If either a new mapping’s start address is aligned with the start address
of the old mapping or the new mapping’s end address is aligned with the
end address of the old mapping, either **prev** or **next** is NULL.

Note, the reason for a dedicated remap operation, rather than arbitrary
unmap and map operations, is to give drivers the chance of extracting driver
specific data for creating the new mappings from the unmap operations’s
[`drm_gpuva`](#c.drm_gpuva "drm_gpuva") structure which typically is embedded in larger driver specific
structures.

struct drm\_gpuva\_op\_prefetch
:   GPU VA prefetch operation

**Definition**:

```
struct drm_gpuva_op_prefetch {
    struct drm_gpuva *va;
};
```

**Members**

`va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to prefetch

**Description**

This structure represents a single prefetch operation generated by the
DRM GPU VA manager.

struct drm\_gpuva\_op
:   GPU VA operation

**Definition**:

```
struct drm_gpuva_op {
    struct list_head entry;
    enum drm_gpuva_op_type op;
    union {
        struct drm_gpuva_op_map map;
        struct drm_gpuva_op_remap remap;
        struct drm_gpuva_op_unmap unmap;
        struct drm_gpuva_op_prefetch prefetch;
    };
};
```

**Members**

`entry`
:   The `list_head` used to distribute instances of this `struct within`
    [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops").

`op`
:   the type of the operation

`{unnamed_union}`
:   anonymous

`map`
:   the map operation

`remap`
:   the remap operation

`unmap`
:   the unmap operation

`prefetch`
:   the prefetch operation

**Description**

This structure represents a single generic operation.

The particular type of the operation is defined by **op**.

struct drm\_gpuva\_ops
:   wraps a list of [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op")

**Definition**:

```
struct drm_gpuva_ops {
    struct list_head list;
};
```

**Members**

`list`
:   the `list_head`

drm\_gpuva\_for\_each\_op

`drm_gpuva_for_each_op (op, ops)`

> iterator to walk over [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops")

**Parameters**

`op`
:   [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to assign in each iteration step

`ops`
:   [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to walk

**Description**

This iterator walks over all ops within a given list of operations.

drm\_gpuva\_for\_each\_op\_safe

`drm_gpuva_for_each_op_safe (op, next, ops)`

> iterator to safely walk over [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops")

**Parameters**

`op`
:   [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to assign in each iteration step

`next`
:   `next` [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to store the next step

`ops`
:   [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to walk

**Description**

This iterator walks over all ops within a given list of operations. It is
implemented with [`list_for_each_safe()`](../core-api/list.html#c.list_for_each_safe "list_for_each_safe"), so save against removal of elements.

drm\_gpuva\_for\_each\_op\_from\_reverse

`drm_gpuva_for_each_op_from_reverse (op, ops)`

> iterate backwards from the given point

**Parameters**

`op`
:   [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to assign in each iteration step

`ops`
:   [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to walk

**Description**

This iterator walks over all ops within a given list of operations beginning
from the given operation in reverse order.

drm\_gpuva\_for\_each\_op\_reverse

`drm_gpuva_for_each_op_reverse (op, ops)`

> iterator to walk over [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") in reverse

**Parameters**

`op`
:   [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") to assign in each iteration step

`ops`
:   [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to walk

**Description**

This iterator walks over all ops within a given list of operations in reverse

drm\_gpuva\_first\_op

`drm_gpuva_first_op (ops)`

> returns the first [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") from [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops")

**Parameters**

`ops`
:   the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to get the fist [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") from

drm\_gpuva\_last\_op

`drm_gpuva_last_op (ops)`

> returns the last [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") from [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops")

**Parameters**

`ops`
:   the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to get the last [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") from

drm\_gpuva\_prev\_op

`drm_gpuva_prev_op (op)`

> previous [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") in the list

**Parameters**

`op`
:   the current [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op")

drm\_gpuva\_next\_op

`drm_gpuva_next_op (op)`

> next [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") in the list

**Parameters**

`op`
:   the current [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op")

struct drm\_gpuvm\_map\_req
:   arguments passed to drm\_gpuvm\_sm\_map[\_ops\_create]()

**Definition**:

```
struct drm_gpuvm_map_req {
    struct drm_gpuva_op_map map;
};
```

**Members**

`map`
:   [`struct drm_gpuva_op_map`](#c.drm_gpuva_op_map "drm_gpuva_op_map")

struct drm\_gpuvm\_ops
:   callbacks for split/merge steps

**Definition**:

```
struct drm_gpuvm_ops {
    void (*vm_free)(struct drm_gpuvm *gpuvm);
    struct drm_gpuva_op *(*op_alloc)(void);
    void (*op_free)(struct drm_gpuva_op *op);
    struct drm_gpuvm_bo *(*vm_bo_alloc)(void);
    void (*vm_bo_free)(struct drm_gpuvm_bo *vm_bo);
    int (*vm_bo_validate)(struct drm_gpuvm_bo *vm_bo, struct drm_exec *exec);
    int (*sm_step_map)(struct drm_gpuva_op *op, void *priv);
    int (*sm_step_remap)(struct drm_gpuva_op *op, void *priv);
    int (*sm_step_unmap)(struct drm_gpuva_op *op, void *priv);
};
```

**Members**

`vm_free`
:   called when the last reference of a [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") is
    dropped

    This callback is mandatory.

`op_alloc`
:   called when the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") allocates
    a [`struct drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op")

    Some drivers may want to embed [`struct drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") into driver
    specific structures. By implementing this callback drivers can
    allocate memory accordingly.

    This callback is optional.

`op_free`
:   called when the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") frees a
    [`struct drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op")

    Some drivers may want to embed [`struct drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") into driver
    specific structures. By implementing this callback drivers can
    free the previously allocated memory accordingly.

    This callback is optional.

`vm_bo_alloc`
:   called when the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") allocates
    a [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")

    Some drivers may want to embed [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") into driver
    specific structures. By implementing this callback drivers can
    allocate memory accordingly.

    This callback is optional.

`vm_bo_free`
:   called when the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") frees a
    [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")

    Some drivers may want to embed [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") into driver
    specific structures. By implementing this callback drivers can
    free the previously allocated memory accordingly.

    This callback is optional.

`vm_bo_validate`
:   called from [`drm_gpuvm_validate()`](#c.drm_gpuvm_validate "drm_gpuvm_validate")

    Drivers receive this callback for every evicted [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") being
    mapped in the corresponding [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm").

    Typically, drivers would call their driver specific variant of
    `ttm_bo_validate()` from within this callback.

`sm_step_map`
:   called from [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") to finally insert the
    mapping once all previous steps were completed

    The `priv` pointer matches the one the driver passed to
    [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") or [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap"), respectively.

    Can be NULL if [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") is used.

`sm_step_remap`
:   called from [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") and
    [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") to split up an existent mapping

    This callback is called when existent mapping needs to be split up.
    This is the case when either a newly requested mapping overlaps or
    is enclosed by an existent mapping or a partial unmap of an existent
    mapping is requested.

    The `priv` pointer matches the one the driver passed to
    [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") or [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap"), respectively.

    Can be NULL if neither [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") nor [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") is
    used.

`sm_step_unmap`
:   called from [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") and
    [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") to unmap an existing mapping

    This callback is called when existing mapping needs to be unmapped.
    This is the case when either a newly requested mapping encloses an
    existing mapping or an unmap of an existing mapping is requested.

    The `priv` pointer matches the one the driver passed to
    [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") or [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap"), respectively.

    Can be NULL if neither [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") nor [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") is
    used.

**Description**

This structure defines the callbacks used by [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") and
[`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") to provide the split/merge steps for map and unmap
operations to drivers.

void drm\_gpuva\_op\_remap\_to\_unmap\_range(const struct [drm\_gpuva\_op\_remap](#c.drm_gpuva_op_remap "drm_gpuva_op_remap") \*op, u64 \*start\_addr, u64 \*range)
:   Helper to get the start and range of the unmap stage of a remap op.

**Parameters**

`const struct drm_gpuva_op_remap *op`
:   Remap op.

`u64 *start_addr`
:   Output pointer for the start of the required unmap.

`u64 *range`
:   Output pointer for the length of the required unmap.

**Description**

The given start address and range will be set such that they represent the
range of the address space that was previously covered by the mapping being
re-mapped, but is now empty.

bool drm\_gpuvm\_range\_valid(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 addr, u64 range)
:   checks whether the given range is valid for the given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the GPUVM to check the range for

`u64 addr`
:   the base address

`u64 range`
:   the range starting from the base address

**Description**

Checks whether the range is within the GPUVM’s managed boundaries.

**Return**

true for a valid range, false otherwise

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_gpuvm\_resv\_object\_alloc(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm)
:   allocate a dummy [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

**Parameters**

`struct drm_device *drm`
:   the drivers [`drm_device`](drm-internals.html#c.drm_device "drm_device")

**Description**

Allocates a dummy [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") which can be passed to [`drm_gpuvm_init()`](#c.drm_gpuvm_init "drm_gpuvm_init") in
order to serve as root GEM object providing the `drm_resv` shared across
`drm_gem_objects` local to a single GPUVM.

**Return**

the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") on success, NULL on failure

void drm\_gpuvm\_init(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, const char \*name, enum [drm\_gpuvm\_flags](#c.drm_gpuvm_flags "drm_gpuvm_flags") flags, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*r\_obj, u64 start\_offset, u64 range, u64 reserve\_offset, u64 reserve\_range, const struct [drm\_gpuvm\_ops](#c.drm_gpuvm_ops "drm_gpuvm_ops") \*ops)
:   initialize a [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   pointer to the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to initialize

`const char *name`
:   the name of the GPU VA space

`enum drm_gpuvm_flags flags`
:   the [`drm_gpuvm_flags`](#c.drm_gpuvm_flags "drm_gpuvm_flags") for this GPUVM

`struct drm_device *drm`
:   the [`drm_device`](drm-internals.html#c.drm_device "drm_device") this VM resides in

`struct drm_gem_object *r_obj`
:   the resv [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") providing the GPUVM’s common [`dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")

`u64 start_offset`
:   the start offset of the GPU VA space

`u64 range`
:   the size of the GPU VA space

`u64 reserve_offset`
:   the start of the kernel reserved GPU VA area

`u64 reserve_range`
:   the size of the kernel reserved GPU VA area

`const struct drm_gpuvm_ops *ops`
:   [`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") called on [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") / [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap")

**Description**

The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") must be initialized with this function before use.

Note that **gpuvm** must be cleared to 0 before calling this function. The given
`name` is expected to be managed by the surrounding driver structures.

void drm\_gpuvm\_put(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm)
:   drop a [`struct drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") reference

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to release the reference of

**Description**

This releases a reference to **gpuvm**.

This function may be called from atomic context.

int drm\_gpuvm\_prepare\_vm(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, unsigned int num\_fences)
:   prepare the GPUVMs common dma-resv

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") context

`unsigned int num_fences`
:   the amount of `dma_fences` to reserve

**Description**

Calls [`drm_exec_prepare_obj()`](#c.drm_exec_prepare_obj "drm_exec_prepare_obj") for the GPUVMs dummy [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"); if
**num\_fences** is zero [`drm_exec_lock_obj()`](#c.drm_exec_lock_obj "drm_exec_lock_obj") is called instead.

Using this function directly, it is the drivers responsibility to call
[`drm_exec_init()`](#c.drm_exec_init "drm_exec_init") and [`drm_exec_fini()`](#c.drm_exec_fini "drm_exec_fini") accordingly.

**Return**

0 on success, negative error code on failure.

int drm\_gpuvm\_prepare\_objects(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, unsigned int num\_fences)
:   prepare all associated BOs

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") locking context

`unsigned int num_fences`
:   the amount of `dma_fences` to reserve

**Description**

Calls [`drm_exec_prepare_obj()`](#c.drm_exec_prepare_obj "drm_exec_prepare_obj") for all `drm_gem_objects` the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") contains mappings of; if **num\_fences** is zero [`drm_exec_lock_obj()`](#c.drm_exec_lock_obj "drm_exec_lock_obj")
is called instead.

Using this function directly, it is the drivers responsibility to call
[`drm_exec_init()`](#c.drm_exec_init "drm_exec_init") and [`drm_exec_fini()`](#c.drm_exec_fini "drm_exec_fini") accordingly.

**Note**

This function is safe against concurrent insertion and removal of
external objects, however it is not safe against concurrent usage itself.

Drivers need to make sure to protect this case with either an outer VM lock
or by calling [`drm_gpuvm_prepare_vm()`](#c.drm_gpuvm_prepare_vm "drm_gpuvm_prepare_vm") before this function within the
[`drm_exec_until_all_locked()`](#c.drm_exec_until_all_locked "drm_exec_until_all_locked") loop, such that the GPUVM’s dma-resv lock ensures
mutual exclusion.

**Return**

0 on success, negative error code on failure.

int drm\_gpuvm\_prepare\_range(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, u64 addr, u64 range, unsigned int num\_fences)
:   prepare all BOs mapped within a given range

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") locking context

`u64 addr`
:   the start address within the VA space

`u64 range`
:   the range to iterate within the VA space

`unsigned int num_fences`
:   the amount of `dma_fences` to reserve

**Description**

Calls [`drm_exec_prepare_obj()`](#c.drm_exec_prepare_obj "drm_exec_prepare_obj") for all `drm_gem_objects` mapped between **addr**
and **addr** + **range**; if **num\_fences** is zero [`drm_exec_lock_obj()`](#c.drm_exec_lock_obj "drm_exec_lock_obj") is called
instead.

**Return**

0 on success, negative error code on failure.

int drm\_gpuvm\_exec\_lock(struct [drm\_gpuvm\_exec](#c.drm_gpuvm_exec "drm_gpuvm_exec") \*vm\_exec)
:   lock all dma-resv of all associated BOs

**Parameters**

`struct drm_gpuvm_exec *vm_exec`
:   the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") wrapper

**Description**

Acquires all dma-resv locks of all `drm_gem_objects` the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") contains mappings of.

Additionally, when calling this function with [`struct drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec")::extra
being set the driver receives the given **fn** callback to lock additional
dma-resv in the context of the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") instance. Typically, drivers
would call [`drm_exec_prepare_obj()`](#c.drm_exec_prepare_obj "drm_exec_prepare_obj") from within this callback.

**Return**

0 on success, negative error code on failure.

int drm\_gpuvm\_exec\_lock\_array(struct [drm\_gpuvm\_exec](#c.drm_gpuvm_exec "drm_gpuvm_exec") \*vm\_exec, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*\*objs, unsigned int num\_objs)
:   lock all dma-resv of all associated BOs

**Parameters**

`struct drm_gpuvm_exec *vm_exec`
:   the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") wrapper

`struct drm_gem_object **objs`
:   additional `drm_gem_objects` to lock

`unsigned int num_objs`
:   the number of additional `drm_gem_objects` to lock

**Description**

Acquires all dma-resv locks of all `drm_gem_objects` the given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")
contains mappings of, plus the ones given through **objs**.

**Return**

0 on success, negative error code on failure.

int drm\_gpuvm\_exec\_lock\_range(struct [drm\_gpuvm\_exec](#c.drm_gpuvm_exec "drm_gpuvm_exec") \*vm\_exec, u64 addr, u64 range)
:   prepare all BOs mapped within a given range

**Parameters**

`struct drm_gpuvm_exec *vm_exec`
:   the [`drm_gpuvm_exec`](#c.drm_gpuvm_exec "drm_gpuvm_exec") wrapper

`u64 addr`
:   the start address within the VA space

`u64 range`
:   the range to iterate within the VA space

**Description**

Acquires all dma-resv locks of all `drm_gem_objects` mapped between **addr** and
**addr** + **range**.

**Return**

0 on success, negative error code on failure.

int drm\_gpuvm\_validate(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec)
:   validate all BOs marked as evicted

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to validate evicted BOs

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") instance used for locking the GPUVM

**Description**

Calls the [`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops")::vm\_bo\_validate callback for all evicted buffer
objects being mapped in the given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm").

**Return**

0 on success, negative error code on failure.

void drm\_gpuvm\_resv\_add\_fence(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, enum [dma\_resv\_usage](../driver-api/dma-buf.html#c.dma_resv_usage "dma_resv_usage") private\_usage, enum [dma\_resv\_usage](../driver-api/dma-buf.html#c.dma_resv_usage "dma_resv_usage") extobj\_usage)
:   add fence to private and all extobj dma-resv

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to add a fence to

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") locking context

`struct dma_fence *fence`
:   fence to add

`enum dma_resv_usage private_usage`
:   private dma-resv usage

`enum dma_resv_usage extobj_usage`
:   extobj dma-resv usage

struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*drm\_gpuvm\_bo\_create(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   create a new instance of [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") the **obj** is mapped in.

`struct drm_gem_object *obj`
:   The [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") being mapped in the **gpuvm**.

**Description**

If provided by the driver, this function uses the [`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops")
`vm_bo_alloc()` callback to allocate.

**Return**

a pointer to the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") on success, NULL on failure

bool drm\_gpuvm\_bo\_put(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo)
:   drop a [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") reference

**Parameters**

`struct drm_gpuvm_bo *vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to release the reference of

**Description**

This releases a reference to **vm\_bo**.

If the reference count drops to zero, the `gpuvm_bo` is destroyed, which
includes removing it from the GEMs gpuva list. Hence, if a call to this
function can potentially let the reference count drop to zero the caller must
hold the lock that the GEM uses for its gpuva list (either the GEM’s
dma-resv or gpuva.lock mutex).

This function may only be called from non-atomic context.

**Return**

true if vm\_bo was destroyed, false otherwise.

bool drm\_gpuvm\_bo\_put\_deferred(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo)
:   drop a [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") reference with deferred cleanup

**Parameters**

`struct drm_gpuvm_bo *vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to release the reference of

**Description**

This releases a reference to **vm\_bo**.

This might take and release the GEMs GPUVA lock. You should call
[`drm_gpuvm_bo_deferred_cleanup()`](#c.drm_gpuvm_bo_deferred_cleanup "drm_gpuvm_bo_deferred_cleanup") later to complete the cleanup process.

**Return**

true if vm\_bo is being destroyed, false otherwise.

void drm\_gpuvm\_bo\_deferred\_cleanup(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm)
:   clean up BOs in the deferred list deferred cleanup

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the VM to clean up

**Description**

Cleans up [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") instances in the deferred cleanup list.

struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*drm\_gpuvm\_bo\_find(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   find the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") for the given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") the **obj** is mapped in.

`struct drm_gem_object *obj`
:   The [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") being mapped in the **gpuvm**.

**Description**

Find the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") representing the combination of the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"). If found, increases the reference
count of the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") accordingly.

**Return**

a pointer to the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") on success, NULL on failure

struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*drm\_gpuvm\_bo\_obtain\_locked(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   obtains an instance of the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") for the given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   The [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") the **obj** is mapped in.

`struct drm_gem_object *obj`
:   The [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") being mapped in the **gpuvm**.

**Description**

Find the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") representing the combination of the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"). If found, increases the reference
count of the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") accordingly. If not found, allocates a new
[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo").

Requires the lock for the GEMs gpuva list.

A new [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") is added to the GEMs gpuva list.

**Return**

a pointer to the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") on success, an ERR\_PTR on failure

struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*drm\_gpuvm\_bo\_obtain\_prealloc(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*\_\_vm\_bo)
:   obtains an instance of the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") for the given [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object")

**Parameters**

`struct drm_gpuvm_bo *__vm_bo`
:   A pre-allocated [`struct drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo").

**Description**

Find the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") representing the combination of the given
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") and [`drm_gem_object`](#c.drm_gem_object "drm_gem_object"). If found, increases the reference
count of the found [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") accordingly, while the **\_\_vm\_bo** reference
count is decreased. If not found **\_\_vm\_bo** is returned without further
increase of the reference count.

The provided **\_\_vm\_bo** must not already be in the gpuva, evict, or extobj
lists prior to calling this method.

A new [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") is added to the GEMs gpuva list.

**Return**

a pointer to the found [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") or **\_\_vm\_bo** if no existing
[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") was found

void drm\_gpuvm\_bo\_extobj\_add(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo)
:   adds the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to its [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s extobj list

**Parameters**

`struct drm_gpuvm_bo *vm_bo`
:   The [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to add to its [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s the extobj list.

**Description**

Adds the given **vm\_bo** to its [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s extobj list if not on the list
already and if the corresponding [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") is an external object,
actually.

void drm\_gpuvm\_bo\_evict(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo, bool evict)
:   add / remove a [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to / from the `drm_gpuvms` evicted list

**Parameters**

`struct drm_gpuvm_bo *vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to add or remove

`bool evict`
:   indicates whether the object is evicted

**Description**

Adds a [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to or removes it from the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s evicted list.

int drm\_gpuva\_insert(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va)
:   insert a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to insert the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") in

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to insert

**Description**

Insert a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") with a given address and range into a
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm").

It is safe to use this function using the safe versions of iterating the GPU
VA space, such as [`drm_gpuvm_for_each_va_safe()`](#c.drm_gpuvm_for_each_va_safe "drm_gpuvm_for_each_va_safe") and
[`drm_gpuvm_for_each_va_range_safe()`](#c.drm_gpuvm_for_each_va_range_safe "drm_gpuvm_for_each_va_range_safe").

**Return**

0 on success, negative error code on failure.

void drm\_gpuva\_remove(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va)
:   remove a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to remove

**Description**

This removes the given `va` from the underlying tree.

It is safe to use this function using the safe versions of iterating the GPU
VA space, such as [`drm_gpuvm_for_each_va_safe()`](#c.drm_gpuvm_for_each_va_safe "drm_gpuvm_for_each_va_safe") and
[`drm_gpuvm_for_each_va_range_safe()`](#c.drm_gpuvm_for_each_va_range_safe "drm_gpuvm_for_each_va_range_safe").

void drm\_gpuva\_link(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va, struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo)
:   link a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to link

`struct drm_gpuvm_bo *vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to add the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to

**Description**

This adds the given `va` to the GPU VA list of the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") and the
[`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") to the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") it is associated with.

For every [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") entry added to the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") an additional
reference of the latter is taken.

This function expects the caller to protect the GEM’s GPUVA list against
concurrent access using either the GEM’s dma-resv or gpuva.lock mutex.

void drm\_gpuva\_unlink(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va)
:   unlink a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to unlink

**Description**

This removes the given `va` from the GPU VA list of the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") it is
associated with.

This removes the given `va` from the GPU VA list of the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") and
the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") from the [`drm_gem_object`](#c.drm_gem_object "drm_gem_object") it is associated with in case
this call unlinks the last [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") from the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo").

For every [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") entry removed from the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") a reference of
the latter is dropped.

This function expects the caller to protect the GEM’s GPUVA list against
concurrent access using either the GEM’s dma-resv or gpuva.lock mutex.

void drm\_gpuva\_unlink\_defer(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va)
:   unlink a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") with deferred vm\_bo cleanup

**Parameters**

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to unlink

**Description**

Similar to [`drm_gpuva_unlink()`](#c.drm_gpuva_unlink "drm_gpuva_unlink"), but uses [`drm_gpuvm_bo_put_deferred()`](#c.drm_gpuvm_bo_put_deferred "drm_gpuvm_bo_put_deferred") and takes
the lock for the caller.

struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*drm\_gpuva\_find\_first(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 addr, u64 range)
:   find the first [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") in the given range

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to search in

`u64 addr`
:   the `drm_gpuvas` address

`u64 range`
:   the `drm_gpuvas` range

**Return**

the first [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") within the given range

struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*drm\_gpuva\_find(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 addr, u64 range)
:   find a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to search in

`u64 addr`
:   the `drm_gpuvas` address

`u64 range`
:   the `drm_gpuvas` range

**Return**

the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") at a given `addr` and with a given `range`

struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*drm\_gpuva\_find\_prev(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 start)
:   find the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") before the given address

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to search in

`u64 start`
:   the given GPU VA’s start address

**Description**

Find the adjacent [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") before the GPU VA with given [`start`](../networking/ieee802154.html#c.start "start") address.

Note that if there is any free space between the GPU VA mappings no mapping
is returned.

**Return**

a pointer to the found [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") or NULL if none was found

struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*drm\_gpuva\_find\_next(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 end)
:   find the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") after the given address

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to search in

`u64 end`
:   the given GPU VA’s end address

**Description**

Find the adjacent [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") after the GPU VA with given `end` address.

Note that if there is any free space between the GPU VA mappings no mapping
is returned.

**Return**

a pointer to the found [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") or NULL if none was found

bool drm\_gpuvm\_interval\_empty(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 addr, u64 range)
:   indicate whether a given interval of the VA space is empty

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") to check the range for

`u64 addr`
:   the start address of the range

`u64 range`
:   the range of the interval

**Return**

true if the interval is empty, false otherwise

void drm\_gpuva\_map(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*va, const struct [drm\_gpuva\_op\_map](#c.drm_gpuva_op_map "drm_gpuva_op_map") \*op)
:   helper to insert a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") according to a [`drm_gpuva_op_map`](#c.drm_gpuva_op_map "drm_gpuva_op_map")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")

`struct drm_gpuva *va`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to insert

`const struct drm_gpuva_op_map *op`
:   the [`drm_gpuva_op_map`](#c.drm_gpuva_op_map "drm_gpuva_op_map") to initialize **va** with

**Description**

Initializes the **va** from the **op** and inserts it into the given **gpuvm**.

void drm\_gpuva\_remap(struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*prev, struct [drm\_gpuva](#c.drm_gpuva "drm_gpuva") \*next, const struct [drm\_gpuva\_op\_remap](#c.drm_gpuva_op_remap "drm_gpuva_op_remap") \*op)
:   helper to remap a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") according to a [`drm_gpuva_op_remap`](#c.drm_gpuva_op_remap "drm_gpuva_op_remap")

**Parameters**

`struct drm_gpuva *prev`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to remap when keeping the start of a mapping

`struct drm_gpuva *next`
:   the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to remap when keeping the end of a mapping

`const struct drm_gpuva_op_remap *op`
:   the [`drm_gpuva_op_remap`](#c.drm_gpuva_op_remap "drm_gpuva_op_remap") to initialize **prev** and **next** with

**Description**

Removes the currently mapped [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") and remaps it using **prev** and/or
**next**.

void drm\_gpuva\_unmap(const struct [drm\_gpuva\_op\_unmap](#c.drm_gpuva_op_unmap "drm_gpuva_op_unmap") \*op)
:   helper to remove a [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") according to a [`drm_gpuva_op_unmap`](#c.drm_gpuva_op_unmap "drm_gpuva_op_unmap")

**Parameters**

`const struct drm_gpuva_op_unmap *op`
:   the [`drm_gpuva_op_unmap`](#c.drm_gpuva_op_unmap "drm_gpuva_op_unmap") specifying the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") to remove

**Description**

Removes the [`drm_gpuva`](#c.drm_gpuva "drm_gpuva") associated with the [`drm_gpuva_op_unmap`](#c.drm_gpuva_op_unmap "drm_gpuva_op_unmap").

int drm\_gpuvm\_sm\_map(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, void \*priv, const struct [drm\_gpuvm\_map\_req](#c.drm_gpuvm_map_req "drm_gpuvm_map_req") \*req)
:   calls the [`drm_gpuva_op`](#c.drm_gpuva_op "drm_gpuva_op") split/merge steps

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`void *priv`
:   pointer to a driver private data structure

`const struct drm_gpuvm_map_req *req`
:   ptr to [`struct drm_gpuvm_map_req`](#c.drm_gpuvm_map_req "drm_gpuvm_map_req")

**Description**

This function iterates the given range of the GPU VA space. It utilizes the
[`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") to call back into the driver providing the split and merge
steps.

Drivers may use these callbacks to update the GPU VA space right away within
the callback. In case the driver decides to copy and store the operations for
later processing neither this function nor [`drm_gpuvm_sm_unmap`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap") is allowed to
be called before the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space was
updated with the previous set of operations. To update the
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space [`drm_gpuva_insert()`](#c.drm_gpuva_insert "drm_gpuva_insert"),
`drm_gpuva_destroy_locked()` and/or `drm_gpuva_destroy_unlocked()` should be
used.

A sequence of callbacks can contain map, unmap and remap operations, but
the sequence of callbacks might also be empty if no operation is required,
e.g. if the requested mapping already exists in the exact same way.

There can be an arbitrary amount of unmap operations, a maximum of two remap
operations and a single map operation. The latter one represents the original
map operation requested by the caller.

**Return**

0 on success or a negative error code

int drm\_gpuvm\_sm\_unmap(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, void \*priv, u64 req\_addr, u64 req\_range)
:   calls the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to split on unmap

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`void *priv`
:   pointer to a driver private data structure

`u64 req_addr`
:   the start address of the range to unmap

`u64 req_range`
:   the range of the mappings to unmap

**Description**

This function iterates the given range of the GPU VA space. It utilizes the
[`drm_gpuvm_ops`](#c.drm_gpuvm_ops "drm_gpuvm_ops") to call back into the driver providing the operations to
unmap and, if required, split existing mappings.

Drivers may use these callbacks to update the GPU VA space right away within
the callback. In case the driver decides to copy and store the operations for
later processing neither this function nor [`drm_gpuvm_sm_map`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map") is allowed to be
called before the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space was updated
with the previous set of operations. To update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view
of the GPU VA space [`drm_gpuva_insert()`](#c.drm_gpuva_insert "drm_gpuva_insert"), `drm_gpuva_destroy_locked()` and/or
`drm_gpuva_destroy_unlocked()` should be used.

A sequence of callbacks can contain unmap and remap operations, depending on
whether there are actual overlapping mappings to split.

There can be an arbitrary amount of unmap operations and a maximum of two
remap operations.

**Return**

0 on success or a negative error code

int drm\_gpuvm\_sm\_map\_exec\_lock(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, unsigned int num\_fences, struct [drm\_gpuvm\_map\_req](#c.drm_gpuvm_map_req "drm_gpuvm_map_req") \*req)
:   locks the objects touched by a [`drm_gpuvm_sm_map()`](#c.drm_gpuvm_sm_map "drm_gpuvm_sm_map")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") locking context

`unsigned int num_fences`
:   for newly mapped objects, the # of fences to reserve

`struct drm_gpuvm_map_req *req`
:   ptr to drm\_gpuvm\_map\_req struct

**Description**

This function locks ([`drm_exec_lock_obj()`](#c.drm_exec_lock_obj "drm_exec_lock_obj")) objects that will be unmapped/
remapped, and locks+prepares (`drm_exec_prepare_object()`) objects that
will be newly mapped.

The expected usage is:

```
vm_bind {
    struct drm_exec exec;

    // IGNORE_DUPLICATES is required, INTERRUPTIBLE_WAIT is recommended:
    drm_exec_init(&exec, IGNORE_DUPLICATES | INTERRUPTIBLE_WAIT, 0);

    drm_exec_until_all_locked (&exec) {
        for_each_vm_bind_operation {
            switch (op->op) {
            case DRIVER_OP_UNMAP:
                ret = drm_gpuvm_sm_unmap_exec_lock(gpuvm, &exec, op->addr, op->range);
                break;
            case DRIVER_OP_MAP:
                ret = drm_gpuvm_sm_map_exec_lock(gpuvm, &exec, num_fences, &req);
                break;
            }

            drm_exec_retry_on_contention(&exec);
            if (ret)
                return ret;
        }
    }
}
```

This enables all locking to be performed before the driver begins modifying
the VM. This is safe to do in the case of overlapping DRIVER\_VM\_BIND\_OPs,
where an earlier op can alter the sequence of steps generated for a later
op, because the later altered step will involve the same GEM object(s)
already seen in the earlier locking step. For example:

1. An earlier driver DRIVER\_OP\_UNMAP op removes the need for a
   DRM\_GPUVA\_OP\_REMAP/UNMAP step. This is safe because we’ve already
   locked the GEM object in the earlier DRIVER\_OP\_UNMAP op.
2. An earlier DRIVER\_OP\_MAP op overlaps with a later DRIVER\_OP\_MAP/UNMAP
   op, introducing a DRM\_GPUVA\_OP\_REMAP/UNMAP that wouldn’t have been
   required without the earlier DRIVER\_OP\_MAP. This is safe because we’ve
   already locked the GEM object in the earlier DRIVER\_OP\_MAP step.

**Return**

0 on success or a negative error code

int drm\_gpuvm\_sm\_unmap\_exec\_lock(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, u64 req\_addr, u64 req\_range)
:   locks the objects touched by [`drm_gpuvm_sm_unmap()`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`struct drm_exec *exec`
:   the [`drm_exec`](#c.drm_exec "drm_exec") locking context

`u64 req_addr`
:   the start address of the range to unmap

`u64 req_range`
:   the range of the mappings to unmap

**Description**

This function locks ([`drm_exec_lock_obj()`](#c.drm_exec_lock_obj "drm_exec_lock_obj")) objects that will be unmapped/
remapped by [`drm_gpuvm_sm_unmap()`](#c.drm_gpuvm_sm_unmap "drm_gpuvm_sm_unmap").

See [`drm_gpuvm_sm_map_exec_lock()`](#c.drm_gpuvm_sm_map_exec_lock "drm_gpuvm_sm_map_exec_lock") for expected usage.

**Return**

0 on success or a negative error code

struct [drm\_gpuva\_ops](#c.drm_gpuva_ops "drm_gpuva_ops") \*drm\_gpuvm\_sm\_map\_ops\_create(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, const struct [drm\_gpuvm\_map\_req](#c.drm_gpuvm_map_req "drm_gpuvm_map_req") \*req)
:   creates the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to split and merge

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`const struct drm_gpuvm_map_req *req`
:   map request arguments

**Description**

This function creates a list of operations to perform splitting and merging
of existing mapping(s) with the newly requested one.

The list can be iterated with [`drm_gpuva_for_each_op`](#c.drm_gpuva_for_each_op "drm_gpuva_for_each_op") and must be processed
in the given order. It can contain map, unmap and remap operations, but it
also can be empty if no operation is required, e.g. if the requested mapping
already exists in the exact same way.

There can be an arbitrary amount of unmap operations, a maximum of two remap
operations and a single map operation. The latter one represents the original
map operation requested by the caller.

Note that before calling this function again with another mapping request it
is necessary to update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space. The
previously obtained operations must be either processed or abandoned. To
update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space [`drm_gpuva_insert()`](#c.drm_gpuva_insert "drm_gpuva_insert"),
`drm_gpuva_destroy_locked()` and/or `drm_gpuva_destroy_unlocked()` should be
used.

After the caller finished processing the returned [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops"), they must
be freed with [`drm_gpuva_ops_free`](#c.drm_gpuva_ops_free "drm_gpuva_ops_free").

**Return**

a pointer to the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") on success, an ERR\_PTR on failure

struct [drm\_gpuva\_ops](#c.drm_gpuva_ops "drm_gpuva_ops") \*drm\_gpuvm\_madvise\_ops\_create(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, const struct [drm\_gpuvm\_map\_req](#c.drm_gpuvm_map_req "drm_gpuvm_map_req") \*req)
:   creates the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to split

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`const struct drm_gpuvm_map_req *req`
:   map request arguments

**Description**

This function creates a list of operations to perform splitting
of existent mapping(s) at start or end, based on the request map.

The list can be iterated with [`drm_gpuva_for_each_op`](#c.drm_gpuva_for_each_op "drm_gpuva_for_each_op") and must be processed
in the given order. It can contain map and remap operations, but it
also can be empty if no operation is required, e.g. if the requested mapping
already exists is the exact same way.

There will be no unmap operations, a maximum of two remap operations and two
map operations. The two map operations correspond to: one from start to the
end of drm\_gpuvaX, and another from the start of drm\_gpuvaY to end.

Note that before calling this function again with another mapping request it
is necessary to update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space. The
previously obtained operations must be either processed or abandoned. To
update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space [`drm_gpuva_insert()`](#c.drm_gpuva_insert "drm_gpuva_insert"),
`drm_gpuva_destroy_locked()` and/or `drm_gpuva_destroy_unlocked()` should be
used.

After the caller finished processing the returned [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops"), they must
be freed with [`drm_gpuva_ops_free`](#c.drm_gpuva_ops_free "drm_gpuva_ops_free").

**Return**

a pointer to the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") on success, an ERR\_PTR on failure

struct [drm\_gpuva\_ops](#c.drm_gpuva_ops "drm_gpuva_ops") \*drm\_gpuvm\_sm\_unmap\_ops\_create(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 req\_addr, u64 req\_range)
:   creates the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to split on unmap

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`u64 req_addr`
:   the start address of the range to unmap

`u64 req_range`
:   the range of the mappings to unmap

**Description**

This function creates a list of operations to perform unmapping and, if
required, splitting of the mappings overlapping the unmap range.

The list can be iterated with [`drm_gpuva_for_each_op`](#c.drm_gpuva_for_each_op "drm_gpuva_for_each_op") and must be processed
in the given order. It can contain unmap and remap operations, depending on
whether there are actual overlapping mappings to split.

There can be an arbitrary amount of unmap operations and a maximum of two
remap operations.

Note that before calling this function again with another range to unmap it
is necessary to update the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space. The
previously obtained operations must be processed or abandoned. To update the
[`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm")’s view of the GPU VA space [`drm_gpuva_insert()`](#c.drm_gpuva_insert "drm_gpuva_insert"),
`drm_gpuva_destroy_locked()` and/or `drm_gpuva_destroy_unlocked()` should be
used.

After the caller finished processing the returned [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops"), they must
be freed with [`drm_gpuva_ops_free`](#c.drm_gpuva_ops_free "drm_gpuva_ops_free").

**Return**

a pointer to the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") on success, an ERR\_PTR on failure

struct [drm\_gpuva\_ops](#c.drm_gpuva_ops "drm_gpuva_ops") \*drm\_gpuvm\_prefetch\_ops\_create(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, u64 addr, u64 range)
:   creates the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to prefetch

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

`u64 addr`
:   the start address of the range to prefetch

`u64 range`
:   the range of the mappings to prefetch

**Description**

This function creates a list of operations to perform prefetching.

The list can be iterated with [`drm_gpuva_for_each_op`](#c.drm_gpuva_for_each_op "drm_gpuva_for_each_op") and must be processed
in the given order. It can contain prefetch operations.

There can be an arbitrary amount of prefetch operations.

After the caller finished processing the returned [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops"), they must
be freed with [`drm_gpuva_ops_free`](#c.drm_gpuva_ops_free "drm_gpuva_ops_free").

**Return**

a pointer to the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") on success, an ERR\_PTR on failure

struct [drm\_gpuva\_ops](#c.drm_gpuva_ops "drm_gpuva_ops") \*drm\_gpuvm\_bo\_unmap\_ops\_create(struct [drm\_gpuvm\_bo](#c.drm_gpuvm_bo "drm_gpuvm_bo") \*vm\_bo)
:   creates the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to unmap a GEM

**Parameters**

`struct drm_gpuvm_bo *vm_bo`
:   the [`drm_gpuvm_bo`](#c.drm_gpuvm_bo "drm_gpuvm_bo") abstraction

**Description**

This function creates a list of operations to perform unmapping for every
GPUVA attached to a GEM.

The list can be iterated with [`drm_gpuva_for_each_op`](#c.drm_gpuva_for_each_op "drm_gpuva_for_each_op") and consists out of an
arbitrary amount of unmap operations.

After the caller finished processing the returned [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops"), they must
be freed with [`drm_gpuva_ops_free`](#c.drm_gpuva_ops_free "drm_gpuva_ops_free").

This function expects the caller to protect the GEM’s GPUVA list against
concurrent access using either the GEM’s dma-resv or gpuva.lock mutex.

**Return**

a pointer to the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") on success, an ERR\_PTR on failure

void drm\_gpuva\_ops\_free(struct [drm\_gpuvm](#c.drm_gpuvm "drm_gpuvm") \*gpuvm, struct [drm\_gpuva\_ops](#c.drm_gpuva_ops "drm_gpuva_ops") \*ops)
:   free the given [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops")

**Parameters**

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](#c.drm_gpuvm "drm_gpuvm") the ops were created for

`struct drm_gpuva_ops *ops`
:   the [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") to free

**Description**

Frees the given [`drm_gpuva_ops`](#c.drm_gpuva_ops "drm_gpuva_ops") structure including all the ops associated
with it.

## DRM Buddy Allocator

### Buddy Allocator Function References (GPU buddy)

int gpu\_buddy\_init(struct gpu\_buddy \*mm, u64 size, u64 chunk\_size)
:   init memory manager

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager to initialize

`u64 size`
:   size in bytes to manage

`u64 chunk_size`
:   minimum page size in bytes for our allocations

**Description**

Initializes the memory manager and its resources.

**Return**

0 on success, error code on failure.

void gpu\_buddy\_fini(struct gpu\_buddy \*mm)
:   tear down the memory manager

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager to free

**Description**

Cleanup memory manager resources and the freetree

void gpu\_buddy\_reset\_clear(struct gpu\_buddy \*mm, bool is\_clear)
:   reset blocks clear state

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager

`bool is_clear`
:   blocks clear state

**Description**

Reset the clear state based on **is\_clear** value for each block
in the freetree.

void gpu\_buddy\_free\_block(struct gpu\_buddy \*mm, struct gpu\_buddy\_block \*block)
:   free a block

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager

`struct gpu_buddy_block *block`
:   block to be freed

void gpu\_buddy\_free\_list(struct gpu\_buddy \*mm, struct list\_head \*objects, unsigned int flags)
:   free blocks

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager

`struct list_head *objects`
:   input list head to free blocks

`unsigned int flags`
:   optional flags like GPU\_BUDDY\_CLEARED

int gpu\_buddy\_block\_trim(struct gpu\_buddy \*mm, u64 \*start, u64 new\_size, struct list\_head \*blocks)
:   free unused pages

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager

`u64 *start`
:   start address to begin the trimming.

`u64 new_size`
:   original size requested

`struct list_head *blocks`
:   Input and output list of allocated blocks.
    MUST contain single block as input to be trimmed.
    On success will contain the newly allocated blocks
    making up the **new\_size**. Blocks always appear in
    ascending order

**Description**

For contiguous allocation, we round up the size to the nearest
power of two value, drivers consume *actual* size, so remaining
portions are unused and can be optionally freed with this function

**Return**

0 on success, error code on failure.

int gpu\_buddy\_alloc\_blocks(struct gpu\_buddy \*mm, u64 start, u64 end, u64 size, u64 min\_block\_size, struct list\_head \*blocks, unsigned long flags)
:   allocate power-of-two blocks

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager to allocate from

`u64 start`
:   start of the allowed range for this block

`u64 end`
:   end of the allowed range for this block

`u64 size`
:   size of the allocation in bytes

`u64 min_block_size`
:   alignment of the allocation

`struct list_head *blocks`
:   output list head to add allocated blocks

`unsigned long flags`
:   GPU\_BUDDY\_\*\_ALLOCATION flags

**Description**

`alloc_range_bias()` called on range limitations, which traverses
the tree and returns the desired block.

`alloc_from_freetree()` called when *no* range restrictions
are enforced, which picks the block from the freetree.

**Return**

0 on success, error code on failure.

void gpu\_buddy\_block\_print(struct gpu\_buddy \*mm, struct gpu\_buddy\_block \*block)
:   print block information

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager

`struct gpu_buddy_block *block`
:   GPU buddy block

void gpu\_buddy\_print(struct gpu\_buddy \*mm)
:   print allocator state

**Parameters**

`struct gpu_buddy *mm`
:   GPU buddy manager

### DRM Buddy Specific Logging Function References

void drm\_buddy\_block\_print(struct gpu\_buddy \*mm, struct gpu\_buddy\_block \*block, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p)
:   print block information

**Parameters**

`struct gpu_buddy *mm`
:   DRM buddy manager

`struct gpu_buddy_block *block`
:   DRM buddy block

`struct drm_printer *p`
:   DRM printer to use

void drm\_buddy\_print(struct gpu\_buddy \*mm, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p)
:   print allocator state

**Parameters**

`struct gpu_buddy *mm`
:   DRM buddy manager

`struct drm_printer *p`
:   DRM printer to use

## DRM Cache Handling and Fast WC memcpy()

void drm\_clflush\_pages(struct page \*pages[], unsigned long num\_pages)
:   Flush dcache lines of a set of pages.

**Parameters**

`struct page *pages[]`
:   List of pages to be flushed.

`unsigned long num_pages`
:   Number of pages in the array.

**Description**

Flush every data cache line entry that points to an address belonging
to a page in the array.

void drm\_clflush\_sg(struct sg\_table \*st)
:   Flush dcache lines pointing to a scather-gather.

**Parameters**

`struct sg_table *st`
:   `struct sg_table`.

**Description**

Flush every data cache line entry that points to an address in the
sg.

void drm\_clflush\_virt\_range(void \*addr, unsigned long length)
:   Flush dcache lines of a region

**Parameters**

`void *addr`
:   Initial kernel memory address.

`unsigned long length`
:   Region size.

**Description**

Flush every data cache line entry that points to an address in the
region requested.

void drm\_memcpy\_from\_wc(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, unsigned long len)
:   Perform the fastest available memcpy from a source that may be WC.

**Parameters**

`struct iosys_map *dst`
:   The destination pointer

`const struct iosys_map *src`
:   The source pointer

`unsigned long len`
:   The size of the area o transfer in bytes

**Description**

Tries an arch optimized memcpy for prefetching reading out of a WC region,
and if no such beast is available, falls back to a normal memcpy.

## DRM Sync Objects

DRM synchronisation objects (syncobj, see struct [`drm_syncobj`](#c.drm_syncobj "drm_syncobj")) provide a
container for a synchronization primitive which can be used by userspace
to explicitly synchronize GPU commands, can be shared between userspace
processes, and can be shared between different DRM drivers.
Their primary use-case is to implement Vulkan fences and semaphores.
The syncobj userspace API provides ioctls for several operations:

> * Creation and destruction of syncobjs
> * Import and export of syncobjs to/from a syncobj file descriptor
> * Import and export a syncobj’s underlying fence to/from a sync file
> * Reset a syncobj (set its fence to NULL)
> * Signal a syncobj (set a trivially signaled fence)
> * Wait for a syncobj’s fence to appear and be signaled

The syncobj userspace API also provides operations to manipulate a syncobj
in terms of a timeline of struct [`dma_fence_chain`](../driver-api/dma-buf.html#c.dma_fence_chain "dma_fence_chain") rather than a single
struct [`dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence"), through the following operations:

> * Signal a given point on the timeline
> * Wait for a given point to appear and/or be signaled
> * Import and export from/to a given point of a timeline

At it’s core, a syncobj is simply a wrapper around a pointer to a struct
[`dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence") which may be NULL.
When a syncobj is first created, its pointer is either NULL or a pointer
to an already signaled fence depending on whether the
`DRM_SYNCOBJ_CREATE_SIGNALED` flag is passed to
`DRM_IOCTL_SYNCOBJ_CREATE`.

If the syncobj is considered as a binary (its state is either signaled or
unsignaled) primitive, when GPU work is enqueued in a DRM driver to signal
the syncobj, the syncobj’s fence is replaced with a fence which will be
signaled by the completion of that work.
If the syncobj is considered as a timeline primitive, when GPU work is
enqueued in a DRM driver to signal the a given point of the syncobj, a new
struct [`dma_fence_chain`](../driver-api/dma-buf.html#c.dma_fence_chain "dma_fence_chain") pointing to the DRM driver’s fence and also
pointing to the previous fence that was in the syncobj. The new struct
[`dma_fence_chain`](../driver-api/dma-buf.html#c.dma_fence_chain "dma_fence_chain") fence replace the syncobj’s fence and will be signaled by
completion of the DRM driver’s work and also any work associated with the
fence previously in the syncobj.

When GPU work which waits on a syncobj is enqueued in a DRM driver, at the
time the work is enqueued, it waits on the syncobj’s fence before
submitting the work to hardware. That fence is either :

> * The syncobj’s current fence if the syncobj is considered as a binary
>   primitive.
> * The struct [`dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence") associated with a given point if the syncobj is
>   considered as a timeline primitive.

If the syncobj’s fence is NULL or not present in the syncobj’s timeline,
the enqueue operation is expected to fail.

With binary syncobj, all manipulation of the syncobjs’s fence happens in
terms of the current fence at the time the ioctl is called by userspace
regardless of whether that operation is an immediate host-side operation
(signal or reset) or or an operation which is enqueued in some driver
queue. `DRM_IOCTL_SYNCOBJ_RESET` and `DRM_IOCTL_SYNCOBJ_SIGNAL` can be used
to manipulate a syncobj from the host by resetting its pointer to NULL or
setting its pointer to a fence which is already signaled.

With a timeline syncobj, all manipulation of the synobj’s fence happens in
terms of a u64 value referring to point in the timeline. See
[`dma_fence_chain_find_seqno()`](../driver-api/dma-buf.html#c.dma_fence_chain_find_seqno "dma_fence_chain_find_seqno") to see how a given point is found in the
timeline.

Note that applications should be careful to always use timeline set of
ioctl() when dealing with syncobj considered as timeline. Using a binary
set of ioctl() with a syncobj considered as timeline could result incorrect
synchronization. The use of binary syncobj is supported through the
timeline set of ioctl() by using a point value of 0, this will reproduce
the behavior of the binary set of ioctl() (for example replace the
syncobj’s fence when signaling).

### Host-side wait on syncobjs

`DRM_IOCTL_SYNCOBJ_WAIT` takes an array of syncobj handles and does a
host-side wait on all of the syncobj fences simultaneously.
If `DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL` is set, the wait ioctl will wait on
all of the syncobj fences to be signaled before it returns.
Otherwise, it returns once at least one syncobj fence has been signaled
and the index of a signaled fence is written back to the client.

Unlike the enqueued GPU work dependencies which fail if they see a NULL
fence in a syncobj, if `DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT` is set,
the host-side wait will first wait for the syncobj to receive a non-NULL
fence and then wait on that fence.
If `DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT` is not set and any one of the
syncobjs in the array has a NULL fence, -EINVAL will be returned.
Assuming the syncobj starts off with a NULL fence, this allows a client
to do a host wait in one thread (or process) which waits on GPU work
submitted in another thread (or process) without having to manually
synchronize between the two.
This requirement is inherited from the Vulkan fence API.

If `DRM_SYNCOBJ_WAIT_FLAGS_WAIT_DEADLINE` is set, the ioctl will also set
a fence deadline hint on the backing fences before waiting, to provide the
fence signaler with an appropriate sense of urgency. The deadline is
specified as an absolute `CLOCK_MONOTONIC` value in units of ns.

Similarly, `DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT` takes an array of syncobj
handles as well as an array of u64 points and does a host-side wait on all
of syncobj fences at the given points simultaneously.

`DRM_IOCTL_SYNCOBJ_TIMELINE_WAIT` also adds the ability to wait for a given
fence to materialize on the timeline without waiting for the fence to be
signaled by using the `DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE` flag. This
requirement is inherited from the wait-before-signal behavior required by
the Vulkan timeline semaphore API.

Alternatively, `DRM_IOCTL_SYNCOBJ_EVENTFD` can be used to wait without
blocking: an eventfd will be signaled when the syncobj is. This is useful to
integrate the wait in an event loop.

### Import/export of syncobjs

`DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE` and `DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD`
provide two mechanisms for import/export of syncobjs.

The first lets the client import or export an entire syncobj to a file
descriptor.
These fd’s are opaque and have no other use case, except passing the
syncobj between processes.
All exported file descriptors and any syncobj handles created as a
result of importing those file descriptors own a reference to the
same underlying struct [`drm_syncobj`](#c.drm_syncobj "drm_syncobj") and the syncobj can be used
persistently across all the processes with which it is shared.
The syncobj is freed only once the last reference is dropped.
Unlike dma-buf, importing a syncobj creates a new handle (with its own
reference) for every import instead of de-duplicating.
The primary use-case of this persistent import/export is for shared
Vulkan fences and semaphores.

The second import/export mechanism, which is indicated by
`DRM_SYNCOBJ_FD_TO_HANDLE_FLAGS_IMPORT_SYNC_FILE` or
`DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE` lets the client
import/export the syncobj’s current fence from/to a [`sync_file`](../driver-api/dma-buf.html#c.sync_file "sync_file").
When a syncobj is exported to a sync file, that sync file wraps the
sycnobj’s fence at the time of export and any later signal or reset
operations on the syncobj will not affect the exported sync file.
When a sync file is imported into a syncobj, the syncobj’s fence is set
to the fence wrapped by that sync file.
Because sync files are immutable, resetting or signaling the syncobj
will not affect any sync files whose fences have been imported into the
syncobj.

### Import/export of timeline points in timeline syncobjs

`DRM_IOCTL_SYNCOBJ_TRANSFER` provides a mechanism to transfer a struct
[`dma_fence_chain`](../driver-api/dma-buf.html#c.dma_fence_chain "dma_fence_chain") of a syncobj at a given u64 point to another u64 point
into another syncobj.

Note that if you want to transfer a struct [`dma_fence_chain`](../driver-api/dma-buf.html#c.dma_fence_chain "dma_fence_chain") from a given
point on a timeline syncobj from/into a binary syncobj, you can use the
point 0 to mean take/replace the fence in the syncobj.

struct drm\_syncobj
:   sync object.

**Definition**:

```
struct drm_syncobj {
    struct kref refcount;
    struct dma_fence *fence;
    struct list_head cb_list;
    struct list_head ev_fd_list;
    spinlock_t lock;
    struct file *file;
};
```

**Members**

`refcount`
:   Reference count of this object.

`fence`
:   NULL or a pointer to the fence bound to this object.

    This field should not be used directly. Use [`drm_syncobj_fence_get()`](#c.drm_syncobj_fence_get "drm_syncobj_fence_get")
    and [`drm_syncobj_replace_fence()`](#c.drm_syncobj_replace_fence "drm_syncobj_replace_fence") instead.

`cb_list`
:   List of callbacks to call when the `fence` gets replaced.

`ev_fd_list`
:   List of registered eventfd.

`lock`
:   Protects `cb_list` and `ev_fd_list`, and write-locks `fence`.

`file`
:   A file backing for this syncobj.

**Description**

This structure defines a generic sync object which wraps a [`dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence").

void drm\_syncobj\_get(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*obj)
:   acquire a syncobj reference

**Parameters**

`struct drm_syncobj *obj`
:   sync object

**Description**

This acquires an additional reference to **obj**. It is illegal to call this
without already holding a reference. No locks required.

void drm\_syncobj\_put(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*obj)
:   release a reference to a sync object.

**Parameters**

`struct drm_syncobj *obj`
:   sync object.

struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*drm\_syncobj\_fence\_get(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*syncobj)
:   get a reference to a fence in a sync object

**Parameters**

`struct drm_syncobj *syncobj`
:   sync object.

**Description**

This acquires additional reference to [`drm_syncobj.fence`](#c.drm_syncobj "drm_syncobj") contained in **obj**,
if not NULL. It is illegal to call this without already holding a reference.
No locks required.

**Return**

Either the fence of **obj** or NULL if there’s none.

struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*drm\_syncobj\_find(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_private, u32 handle)
:   lookup and reference a sync object.

**Parameters**

`struct drm_file *file_private`
:   drm file private pointer

`u32 handle`
:   sync object handle to lookup.

**Description**

Returns a reference to the syncobj pointed to by handle or NULL. The
reference must be released by calling [`drm_syncobj_put()`](#c.drm_syncobj_put "drm_syncobj_put").

void drm\_syncobj\_add\_point(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*syncobj, struct [dma\_fence\_chain](../driver-api/dma-buf.html#c.dma_fence_chain "dma_fence_chain") \*chain, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, uint64\_t point)
:   add new timeline point to the syncobj

**Parameters**

`struct drm_syncobj *syncobj`
:   sync object to add timeline point do

`struct dma_fence_chain *chain`
:   chain node to use to add the point

`struct dma_fence *fence`
:   fence to encapsulate in the chain node

`uint64_t point`
:   sequence number to use for the point

**Description**

Add the chain node as new timeline point to the syncobj.

void drm\_syncobj\_replace\_fence(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*syncobj, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   replace fence in a sync object.

**Parameters**

`struct drm_syncobj *syncobj`
:   Sync object to replace fence in

`struct dma_fence *fence`
:   fence to install in sync file.

**Description**

This replaces the fence on a sync object.

int drm\_syncobj\_find\_fence(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_private, u32 handle, u64 point, u64 flags, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*\*fence)
:   lookup and reference the fence in a sync object

**Parameters**

`struct drm_file *file_private`
:   drm file private pointer

`u32 handle`
:   sync object handle to lookup.

`u64 point`
:   timeline point

`u64 flags`
:   DRM\_SYNCOBJ\_WAIT\_FLAGS\_WAIT\_FOR\_SUBMIT or not

`struct dma_fence **fence`
:   out parameter for the fence

**Description**

This is just a convenience function that combines [`drm_syncobj_find()`](#c.drm_syncobj_find "drm_syncobj_find") and
[`drm_syncobj_fence_get()`](#c.drm_syncobj_fence_get "drm_syncobj_fence_get").

Returns 0 on success or a negative error value on failure. On success **fence**
contains a reference to the fence, which must be released by calling
[`dma_fence_put()`](../driver-api/dma-buf.html#c.dma_fence_put "dma_fence_put").

void drm\_syncobj\_free(struct [kref](#c.drm_syncobj_free "kref") \*kref)
:   free a sync object.

**Parameters**

`struct kref *kref`
:   kref to free.

**Description**

Only to be called from kref\_put in drm\_syncobj\_put.

int drm\_syncobj\_create(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*\*out\_syncobj, uint32\_t flags, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   create a new syncobj

**Parameters**

`struct drm_syncobj **out_syncobj`
:   returned syncobj

`uint32_t flags`
:   DRM\_SYNCOBJ\_\* flags

`struct dma_fence *fence`
:   if non-NULL, the syncobj will represent this fence

**Description**

This is the first function to create a sync object. After creating, drivers
probably want to make it available to userspace, either through
[`drm_syncobj_get_handle()`](#c.drm_syncobj_get_handle "drm_syncobj_get_handle") or [`drm_syncobj_get_fd()`](#c.drm_syncobj_get_fd "drm_syncobj_get_fd").

Returns 0 on success or a negative error value on failure.

int drm\_syncobj\_get\_handle(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_private, struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*syncobj, u32 \*handle)
:   get a handle from a syncobj

**Parameters**

`struct drm_file *file_private`
:   drm file private pointer

`struct drm_syncobj *syncobj`
:   Sync object to export

`u32 *handle`
:   out parameter with the new handle

**Description**

Exports a sync object created with [`drm_syncobj_create()`](#c.drm_syncobj_create "drm_syncobj_create") as a handle on
**file\_private** to userspace.

Returns 0 on success or a negative error value on failure.

int drm\_syncobj\_get\_fd(struct [drm\_syncobj](#c.drm_syncobj "drm_syncobj") \*syncobj, int \*p\_fd)
:   get a file descriptor from a syncobj

**Parameters**

`struct drm_syncobj *syncobj`
:   Sync object to export

`int *p_fd`
:   out parameter with the new file descriptor

**Description**

Exports a sync object created with [`drm_syncobj_create()`](#c.drm_syncobj_create "drm_syncobj_create") as a file descriptor.

Returns 0 on success or a negative error value on failure.

signed long drm\_timeout\_abs\_to\_jiffies(int64\_t timeout\_nsec)
:   calculate jiffies timeout from absolute value

**Parameters**

`int64_t timeout_nsec`
:   timeout nsec component in ns, 0 for poll

**Description**

Calculate the timeout in jiffies from an absolute time in sec/nsec.

## DRM Execution context

This component mainly abstracts the retry loop necessary for locking
multiple GEM objects while preparing hardware operations (e.g. command
submissions, page table updates etc..).

If a contention is detected while locking a GEM object the cleanup procedure
unlocks all previously locked GEM objects and locks the contended one first
before locking any further objects.

After an object is locked fences slots can optionally be reserved on the
dma\_resv object inside the GEM object.

A typical usage pattern should look like this:

```
struct drm_gem_object *obj;
struct drm_exec exec;
unsigned long index;
int ret;

drm_exec_init(&exec, DRM_EXEC_INTERRUPTIBLE_WAIT);
drm_exec_until_all_locked(&exec) {
        ret = drm_exec_prepare_obj(&exec, boA, 1);
        drm_exec_retry_on_contention(&exec);
        if (ret)
                goto error;

        ret = drm_exec_prepare_obj(&exec, boB, 1);
        drm_exec_retry_on_contention(&exec);
        if (ret)
                goto error;
}

drm_exec_for_each_locked_object(&exec, index, obj) {
        dma_resv_add_fence(obj->resv, fence, DMA_RESV_USAGE_READ);
        ...
}
drm_exec_fini(&exec);
```

See `struct dma_exec` for more details.

struct drm\_exec
:   Execution context

**Definition**:

```
struct drm_exec {
    u32 flags;
    struct ww_acquire_ctx   ticket;
    unsigned int            num_objects;
    unsigned int            max_objects;
    struct drm_gem_object   **objects;
    struct drm_gem_object   *contended;
    struct drm_gem_object *prelocked;
};
```

**Members**

`flags`
:   Flags to control locking behavior

`ticket`
:   WW ticket used for acquiring locks

`num_objects`
:   number of objects locked

`max_objects`
:   maximum objects in array

`objects`
:   array of the locked objects

`contended`
:   contended GEM object we backed off for

`prelocked`
:   already locked GEM object due to contention

struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*drm\_exec\_obj(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, unsigned long index)
:   Return the object for a give drm\_exec index

**Parameters**

`struct drm_exec *exec`
:   Pointer to the drm\_exec context

`unsigned long index`
:   The index.

**Return**

Pointer to the locked object corresponding to **index** if
index is within the number of locked objects. NULL otherwise.

drm\_exec\_for\_each\_locked\_object

`drm_exec_for_each_locked_object (exec, index, obj)`

> iterate over all the locked objects

**Parameters**

`exec`
:   drm\_exec object

`index`
:   unsigned long index for the iteration

`obj`
:   the current GEM object

**Description**

Iterate over all the locked GEM objects inside the drm\_exec object.

drm\_exec\_for\_each\_locked\_object\_reverse

`drm_exec_for_each_locked_object_reverse (exec, index, obj)`

> iterate over all the locked objects in reverse locking order

**Parameters**

`exec`
:   drm\_exec object

`index`
:   unsigned long index for the iteration

`obj`
:   the current GEM object

**Description**

Iterate over all the locked GEM objects inside the drm\_exec object in
reverse locking order. Note that **index** may go below zero and wrap,
but that will be caught by [`drm_exec_obj()`](#c.drm_exec_obj "drm_exec_obj"), returning a NULL object.

drm\_exec\_until\_all\_locked

`drm_exec_until_all_locked (exec)`

> loop until all GEM objects are locked

**Parameters**

`exec`
:   drm\_exec object

**Description**

Core functionality of the drm\_exec object. Loops until all GEM objects are
locked and no more contention exists. At the beginning of the loop it is
guaranteed that no GEM object is locked.

Since labels can’t be defined local to the loops body we use a jump pointer
to make sure that the retry is only used from within the loops body.

drm\_exec\_retry\_on\_contention

`drm_exec_retry_on_contention (exec)`

> restart the loop to grap all locks

**Parameters**

`exec`
:   drm\_exec object

**Description**

Control flow helper to continue when a contention was detected and we need to
clean up and re-start the loop to prepare all GEM objects.

bool drm\_exec\_is\_contended(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec)
:   check for contention

**Parameters**

`struct drm_exec *exec`
:   drm\_exec object

**Description**

Returns true if the drm\_exec object has run into some contention while
locking a GEM object and needs to clean up.

void drm\_exec\_init(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, u32 flags, unsigned nr)
:   initialize a drm\_exec object

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object to initialize

`u32 flags`
:   controls locking behavior, see DRM\_EXEC\_\* defines

`unsigned nr`
:   the initial # of objects

**Description**

Initialize the object and make sure that we can track locked objects.

If nr is non-zero then it is used as the initial objects table size.
In either case, the table will grow (be re-allocated) on demand.

void drm\_exec\_fini(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec)
:   finalize a drm\_exec object

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object to finalize

**Description**

Unlock all locked objects, drop the references to objects and free all memory
used for tracking the state.

bool drm\_exec\_cleanup(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec)
:   cleanup when contention is detected

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object to cleanup

**Description**

Cleanup the current state and return true if we should stay inside the retry
loop, false if there wasn’t any contention detected and we can keep the
objects locked.

int drm\_exec\_lock\_obj(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   lock a GEM object for use

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object with the state

`struct drm_gem_object *obj`
:   the GEM object to lock

**Description**

Lock a GEM object for use and grab a reference to it.

**Return**

-EDEADLK if a contention is detected, -EALREADY when object is
already locked (can be suppressed by setting the DRM\_EXEC\_IGNORE\_DUPLICATES
flag), -ENOMEM when memory allocation failed and zero for success.

void drm\_exec\_unlock\_obj(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj)
:   unlock a GEM object in this exec context

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object with the state

`struct drm_gem_object *obj`
:   the GEM object to unlock

**Description**

Unlock the GEM object and remove it from the collection of locked objects.
Should only be used to unlock the most recently locked objects. It’s not time
efficient to unlock objects locked long ago.

int drm\_exec\_prepare\_obj(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, unsigned int num\_fences)
:   prepare a GEM object for use

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object with the state

`struct drm_gem_object *obj`
:   the GEM object to prepare

`unsigned int num_fences`
:   how many fences to reserve

**Description**

Prepare a GEM object for use by locking it and reserving fence slots.

**Return**

-EDEADLK if a contention is detected, -EALREADY when object is
already locked, -ENOMEM when memory allocation failed and zero for success.

int drm\_exec\_prepare\_array(struct [drm\_exec](#c.drm_exec "drm_exec") \*exec, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*\*objects, unsigned int num\_objects, unsigned int num\_fences)
:   helper to prepare an array of objects

**Parameters**

`struct drm_exec *exec`
:   the drm\_exec object with the state

`struct drm_gem_object **objects`
:   array of GEM object to prepare

`unsigned int num_objects`
:   number of GEM objects in the array

`unsigned int num_fences`
:   number of fences to reserve on each GEM object

**Description**

Prepares all GEM objects in an array, aborts on first error.
Reserves **num\_fences** on each GEM object after locking it.

**Return**

-EDEADLOCK on contention, -EALREADY when object is already locked,
-ENOMEM when memory allocation failed and zero for success.

## GPU Scheduler

### Overview

The GPU scheduler provides entities which allow userspace to push jobs
into software queues which are then scheduled on a hardware run queue.
The software queues have a priority among them. The scheduler selects the entities
from the run queue using a FIFO. The scheduler provides dependency handling
features among jobs. The driver is supposed to provide callback functions for
backend operations to the scheduler like submitting a job to hardware run queue,
returning the dependencies of a job etc.

The organisation of the scheduler is the following:

1. Each hw run queue has one scheduler
2. Each scheduler has multiple run queues with different priorities
   (e.g., HIGH\_HW,HIGH\_SW, KERNEL, NORMAL)
3. Each scheduler run queue has a queue of entities to schedule
4. Entities themselves maintain a queue of jobs that will be scheduled on
   the hardware.

The jobs in an entity are always scheduled in the order in which they were pushed.

Note that once a job was taken from the entities queue and pushed to the
hardware, i.e. the pending queue, the entity must not be referenced anymore
through the jobs entity pointer.

### Flow Control

The DRM GPU scheduler provides a flow control mechanism to regulate the rate
in which the jobs fetched from scheduler entities are executed.

In this context the [`drm_gpu_scheduler`](#c.drm_gpu_scheduler "drm_gpu_scheduler") keeps track of a driver specified
credit limit representing the capacity of this scheduler and a credit count;
every [`drm_sched_job`](#c.drm_sched_job "drm_sched_job") carries a driver specified number of credits.

Once a job is executed (but not yet finished), the job’s credits contribute
to the scheduler’s credit count until the job is finished. If by executing
one more job the scheduler’s credit count would exceed the scheduler’s
credit limit, the job won’t be executed. Instead, the scheduler will wait
until the credit count has decreased enough to not overflow its credit limit.
This implies waiting for previously executed jobs.

### Scheduler Function References

DRM\_SCHED\_FENCE\_DONT\_PIPELINE

`DRM_SCHED_FENCE_DONT_PIPELINE`

> > Prevent dependency pipelining
>
> **Description**
>
> Setting this flag on a scheduler fence prevents pipelining of jobs depending
> on this fence. In other words we always insert a full CPU round trip before
> dependent jobs are pushed to the hw queue.

DRM\_SCHED\_FENCE\_FLAG\_HAS\_DEADLINE\_BIT

`DRM_SCHED_FENCE_FLAG_HAS_DEADLINE_BIT`

> > A fence deadline hint has been set
>
> **Description**
>
> Because we could have a deadline hint can be set before the backing hw
> fence is created, we need to keep track of whether a deadline has already
> been set.

struct drm\_sched\_entity
:   A wrapper around a job queue (typically attached to the DRM file\_priv).

**Definition**:

```
struct drm_sched_entity {
    struct list_head                list;
    spinlock_t lock;
    struct drm_sched_rq             *rq;
    struct drm_gpu_scheduler        **sched_list;
    unsigned int                    num_sched_list;
    enum drm_sched_priority         priority;
    struct spsc_queue               job_queue;
    atomic_t fence_seq;
    uint64_t fence_context;
    struct dma_fence                *dependency;
    struct dma_fence_cb             cb;
    atomic_t *guilty;
    struct dma_fence *last_scheduled;
    struct task_struct              *last_user;
    bool stopped;
    struct completion               entity_idle;
    ktime_t oldest_job_waiting;
    struct rb_node                  rb_tree_node;
};
```

**Members**

`list`
:   Used to append this `struct to` the list of entities in the runqueue
    **rq** under [`drm_sched_rq.entities`](#c.drm_sched_rq "drm_sched_rq").

    Protected by [`drm_sched_rq.lock`](#c.drm_sched_rq "drm_sched_rq") of **rq**.

`lock`
:   Lock protecting the run-queue (**rq**) to which this entity belongs,
    **priority** and the list of schedulers (**sched\_list**, **num\_sched\_list**).

`rq`
:   Runqueue on which this entity is currently scheduled.

    FIXME: Locking is very unclear for this. Writers are protected by
    **lock**, but readers are generally lockless and seem to just race with
    not even a READ\_ONCE.

`sched_list`
:   A list of schedulers ([`struct drm_gpu_scheduler`](#c.drm_gpu_scheduler "drm_gpu_scheduler")). Jobs from this entity can
    be scheduled on any scheduler on this list.

    This can be modified by calling [`drm_sched_entity_modify_sched()`](#c.drm_sched_entity_modify_sched "drm_sched_entity_modify_sched").
    Locking is entirely up to the driver, see the above function for more
    details.

    This will be set to NULL if `num_sched_list` equals 1 and **rq** has been
    set already.

    FIXME: This means priority changes through
    [`drm_sched_entity_set_priority()`](#c.drm_sched_entity_set_priority "drm_sched_entity_set_priority") will be lost henceforth in this case.

`num_sched_list`
:   Number of drm\_gpu\_schedulers in the **sched\_list**.

`priority`
:   Priority of the entity. This can be modified by calling
    [`drm_sched_entity_set_priority()`](#c.drm_sched_entity_set_priority "drm_sched_entity_set_priority"). Protected by **lock**.

`job_queue`
:   the list of jobs of this entity.

`fence_seq`
:   A linearly increasing seqno incremented with each new
    [`drm_sched_fence`](#c.drm_sched_fence "drm_sched_fence") which is part of the entity.

    FIXME: Callers of [`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm") need to ensure correct locking,
    this doesn’t need to be atomic.

`fence_context`
:   A unique context for all the fences which belong to this entity. The
    [`drm_sched_fence.scheduled`](#c.drm_sched_fence "drm_sched_fence") uses the fence\_context but
    [`drm_sched_fence.finished`](#c.drm_sched_fence "drm_sched_fence") uses fence\_context + 1.

`dependency`
:   The dependency fence of the job which is on the top of the job queue.

`cb`
:   Callback for the dependency fence above.

`guilty`
:   Points to entities’ guilty.

`last_scheduled`
:   Points to the finished fence of the last scheduled job. Only written
    by `drm_sched_entity_pop_job()`. Can be accessed locklessly from
    [`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm") if the queue is empty.

`last_user`
:   last group leader pushing a job into the entity.

`stopped`
:   Marks the enity as removed from rq and destined for
    termination. This is set by calling [`drm_sched_entity_flush()`](#c.drm_sched_entity_flush "drm_sched_entity_flush") and by
    [`drm_sched_fini()`](#c.drm_sched_fini "drm_sched_fini").

`entity_idle`
:   Signals when entity is not in use, used to sequence entity cleanup in
    [`drm_sched_entity_fini()`](#c.drm_sched_entity_fini "drm_sched_entity_fini").

`oldest_job_waiting`
:   Marks earliest job waiting in SW queue

`rb_tree_node`
:   The node used to insert this entity into time based priority queue

**Description**

Entities will emit jobs in order to their corresponding hardware
ring, and the scheduler will alternate between entities based on
scheduling policy.

struct drm\_sched\_rq
:   queue of entities to be scheduled.

**Definition**:

```
struct drm_sched_rq {
    struct drm_gpu_scheduler        *sched;
    spinlock_t lock;
    struct drm_sched_entity         *current_entity;
    struct list_head                entities;
    struct rb_root_cached           rb_tree_root;
};
```

**Members**

`sched`
:   the scheduler to which this rq belongs to.

`lock`
:   protects **entities**, **rb\_tree\_root** and **current\_entity**.

`current_entity`
:   the entity which is to be scheduled.

`entities`
:   list of the entities to be scheduled.

`rb_tree_root`
:   root of time based priority queue of entities for FIFO scheduling

**Description**

Run queue is a set of entities scheduling command submissions for
one specific ring. It implements the scheduling policy that selects
the next entity to emit commands from.

struct drm\_sched\_fence
:   fences corresponding to the scheduling of a job.

**Definition**:

```
struct drm_sched_fence {
    struct dma_fence                scheduled;
    struct dma_fence                finished;
    ktime_t deadline;
    struct dma_fence                *parent;
    struct drm_gpu_scheduler        *sched;
    spinlock_t lock;
    void *owner;
    uint64_t drm_client_id;
};
```

**Members**

`scheduled`
:   this fence is what will be signaled by the scheduler
    when the job is scheduled.

`finished`
:   this fence is what will be signaled by the scheduler
    when the job is completed.

    When setting up an out fence for the job, you should use
    this, since it’s available immediately upon
    [`drm_sched_job_init()`](#c.drm_sched_job_init "drm_sched_job_init"), and the fence returned by the driver
    from `run_job()` won’t be created until the dependencies have
    resolved.

`deadline`
:   deadline set on [`drm_sched_fence.finished`](#c.drm_sched_fence "drm_sched_fence") which
    potentially needs to be propagated to [`drm_sched_fence.parent`](#c.drm_sched_fence "drm_sched_fence")

`parent`
:   the fence returned by [`drm_sched_backend_ops.run_job`](#c.drm_sched_backend_ops "drm_sched_backend_ops")
    when scheduling the job on hardware. We signal the
    [`drm_sched_fence.finished`](#c.drm_sched_fence "drm_sched_fence") fence once parent is signalled.

`sched`
:   the scheduler instance to which the job having this `struct
    belongs` to.

`lock`
:   the lock used by the scheduled and the finished fences.

`owner`
:   job owner for debugging

`drm_client_id`
:   The client\_id of the drm\_file which owns the job.

struct drm\_sched\_job
:   A job to be run by an entity.

**Definition**:

```
struct drm_sched_job {
    ktime_t submit_ts;
    struct drm_gpu_scheduler        *sched;
    struct drm_sched_fence          *s_fence;
    struct drm_sched_entity         *entity;
    enum drm_sched_priority         s_priority;
    u32 credits;
    unsigned int                    last_dependency;
    atomic_t karma;
    struct spsc_node                queue_node;
    struct list_head                list;
    union {
        struct dma_fence_cb     finish_cb;
        struct work_struct      work;
    };
    struct dma_fence_cb             cb;
    struct xarray                   dependencies;
};
```

**Members**

`submit_ts`
:   When the job was pushed into the entity queue.

`sched`
:   The scheduler this job is or will be scheduled on. Gets set by
    [`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm"). Valid until drm\_sched\_backend\_ops.`free_job()`
    has finished.

`s_fence`
:   contains the fences for the scheduling of job.

`entity`
:   the entity to which this job belongs.

`s_priority`
:   the priority of the job.

`credits`
:   the number of credits this job contributes to the scheduler

`last_dependency`
:   tracks **dependencies** as they signal

`karma`
:   increment on every hang caused by this job. If this exceeds the hang
    limit of the scheduler then the job is marked guilty and will not
    be scheduled further.

`queue_node`
:   used to append this `struct to` the queue of jobs in an entity.

`list`
:   a job participates in a “pending” and “done” lists.

`{unnamed_union}`
:   anonymous

`finish_cb`
:   the callback for the finished fence.

`work`
:   Helper to reschedule job kill to different context.

`cb`
:   the callback for the parent fence in s\_fence.

`dependencies`
:   Contains the dependencies as [`struct dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence") for this job, see
    [`drm_sched_job_add_dependency()`](#c.drm_sched_job_add_dependency "drm_sched_job_add_dependency") and
    [`drm_sched_job_add_implicit_dependencies()`](#c.drm_sched_job_add_implicit_dependencies "drm_sched_job_add_implicit_dependencies").

**Description**

A job is created by the driver using [`drm_sched_job_init()`](#c.drm_sched_job_init "drm_sched_job_init"), and
should call [`drm_sched_entity_push_job()`](#c.drm_sched_entity_push_job "drm_sched_entity_push_job") once it wants the scheduler
to schedule the job.

enum drm\_gpu\_sched\_stat
:   the scheduler’s status

**Constants**

`DRM_GPU_SCHED_STAT_NONE`
:   Reserved. Do not use.

`DRM_GPU_SCHED_STAT_RESET`
:   The GPU hung and successfully reset.

`DRM_GPU_SCHED_STAT_ENODEV`
:   Error: Device is not available anymore.

`DRM_GPU_SCHED_STAT_NO_HANG`
:   Contrary to scheduler’s assumption, the GPU
    did not hang and is still running.

struct drm\_sched\_backend\_ops
:   Define the backend operations called by the scheduler

**Definition**:

```
struct drm_sched_backend_ops {
    struct dma_fence *(*prepare_job)(struct drm_sched_job *sched_job, struct drm_sched_entity *s_entity);
    struct dma_fence *(*run_job)(struct drm_sched_job *sched_job);
    enum drm_gpu_sched_stat (*timedout_job)(struct drm_sched_job *sched_job);
    void (*free_job)(struct drm_sched_job *sched_job);
    void (*cancel_job)(struct drm_sched_job *sched_job);
};
```

**Members**

`prepare_job`
:   Called when the scheduler is considering scheduling this job next, to
    get another [`struct dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence") for this job to block on. Once it
    returns NULL, `run_job()` may be called.

    Can be NULL if no additional preparation to the dependencies are
    necessary. Skipped when jobs are killed instead of run.

`run_job`
:   Called to execute the job once all of the dependencies
    have been resolved.

    **sched\_job**: the job to run

    The deprecated [`drm_sched_resubmit_jobs()`](#c.drm_sched_resubmit_jobs "drm_sched_resubmit_jobs") (called by [`struct
    drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").timedout\_job) can invoke this again with the
    same parameters. Using this is discouraged because it violates
    dma\_fence rules, notably [`dma_fence_init()`](../driver-api/dma-buf.html#c.dma_fence_init "dma_fence_init") has to be called on
    already initialized fences for a second time. Moreover, this is
    dangerous because attempts to allocate memory might deadlock with
    memory management code waiting for the reset to complete.

    TODO: Document what drivers should do / use instead.

    This method is called in a workqueue context - either from the
    submit\_wq the driver passed through [`drm_sched_init()`](#c.drm_sched_init "drm_sched_init"), or, if the
    driver passed NULL, a separate, ordered workqueue the scheduler
    allocated.

    Note that the scheduler expects to ‘inherit’ its own reference to
    this fence from the callback. It does not invoke an extra
    [`dma_fence_get()`](../driver-api/dma-buf.html#c.dma_fence_get "dma_fence_get") on it. Consequently, this callback must take a
    reference for the scheduler, and additional ones for the driver’s
    respective needs.

    Return:
    \* On success: dma\_fence the driver must signal once the hardware has
    completed the job (“hardware fence”).
    \* On failure: NULL or an ERR\_PTR.

`timedout_job`
:   Called when a job has taken too long to execute,
    to trigger GPU recovery.

    **sched\_job**: The job that has timed out

    Drivers typically issue a reset to recover from GPU hangs.
    This procedure looks very different depending on whether a firmware
    or a hardware scheduler is being used.

    For a FIRMWARE SCHEDULER, each ring has one scheduler, and each
    scheduler has one entity. Hence, the steps taken typically look as
    follows:

    1. Stop the scheduler using [`drm_sched_stop()`](#c.drm_sched_stop "drm_sched_stop"). This will pause the
       scheduler workqueues and cancel the timeout work, guaranteeing
       that nothing is queued while the ring is being removed.
    2. Remove the ring. The firmware will make sure that the
       corresponding parts of the hardware are resetted, and that other
       rings are not impacted.
    3. Kill the entity and the associated scheduler.

    For a HARDWARE SCHEDULER, a scheduler instance schedules jobs from
    one or more entities to one ring. This implies that all entities
    associated with the affected scheduler cannot be torn down, because
    this would effectively also affect innocent userspace processes which
    did not submit faulty jobs (for example).

    Consequently, the procedure to recover with a hardware scheduler
    should look like this:

    1. Stop all schedulers impacted by the reset using [`drm_sched_stop()`](#c.drm_sched_stop "drm_sched_stop").
    2. Kill the entity the faulty job stems from.
    3. Issue a GPU reset on all faulty rings (driver-specific).
    4. Re-submit jobs on all schedulers impacted by re-submitting them to
       the entities which are still alive.
    5. Restart all schedulers that were stopped in step #1 using
       [`drm_sched_start()`](#c.drm_sched_start "drm_sched_start").

    Note that some GPUs have distinct hardware queues but need to reset
    the GPU globally, which requires extra synchronization between the
    timeout handlers of different schedulers. One way to achieve this
    synchronization is to create an ordered workqueue (using
    [`alloc_ordered_workqueue()`](../core-api/workqueue.html#c.alloc_ordered_workqueue "alloc_ordered_workqueue")) at the driver level, and pass this queue
    as [`drm_sched_init()`](#c.drm_sched_init "drm_sched_init")’s **timeout\_wq** parameter. This will guarantee
    that timeout handlers are executed sequentially.

    Return: The scheduler’s status, defined by [`enum drm_gpu_sched_stat`](#c.drm_gpu_sched_stat "drm_gpu_sched_stat")

`free_job`
:   Called once the job’s finished fence has been signaled
    and it’s time to clean it up.

`cancel_job`
:   Used by the scheduler to guarantee remaining jobs’ fences
    get signaled in [`drm_sched_fini()`](#c.drm_sched_fini "drm_sched_fini").

    Used by the scheduler to cancel all jobs that have not been executed
    with [`struct drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").run\_job by the time
    [`drm_sched_fini()`](#c.drm_sched_fini "drm_sched_fini") gets invoked.

    Drivers need to signal the passed job’s hardware fence with an
    appropriate error code (e.g., -ECANCELED) in this callback. They
    must not free the job.

    The scheduler will only call this callback once it stopped calling
    all other callbacks forever, with the exception of [`struct
    drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").free\_job.

**Description**

These functions should be implemented in the driver side.

struct drm\_gpu\_scheduler
:   scheduler instance-specific data

**Definition**:

```
struct drm_gpu_scheduler {
    const struct drm_sched_backend_ops      *ops;
    u32 credit_limit;
    atomic_t credit_count;
    long timeout;
    const char                      *name;
    u32 num_rqs;
    struct drm_sched_rq             **sched_rq;
    wait_queue_head_t job_scheduled;
    atomic64_t job_id_count;
    struct workqueue_struct         *submit_wq;
    struct workqueue_struct         *timeout_wq;
    struct work_struct              work_run_job;
    struct work_struct              work_free_job;
    struct delayed_work             work_tdr;
    struct list_head                pending_list;
    spinlock_t job_list_lock;
    int hang_limit;
    atomic_t *score;
    atomic_t _score;
    bool ready;
    bool free_guilty;
    bool pause_submit;
    bool own_submit_wq;
    struct device                   *dev;
};
```

**Members**

`ops`
:   backend operations provided by the driver.

`credit_limit`
:   the credit limit of this scheduler

`credit_count`
:   the current credit count of this scheduler

`timeout`
:   the time after which a job is removed from the scheduler.

`name`
:   name of the ring for which this scheduler is being used.

`num_rqs`
:   Number of run-queues. This is at most DRM\_SCHED\_PRIORITY\_COUNT,
    as there’s usually one run-queue per priority, but could be less.

`sched_rq`
:   An allocated array of run-queues of size **num\_rqs**;

`job_scheduled`
:   once [`drm_sched_entity_flush()`](#c.drm_sched_entity_flush "drm_sched_entity_flush") is called the scheduler
    waits on this wait queue until all the scheduled jobs are
    finished.

`job_id_count`
:   used to assign unique id to the each job.

`submit_wq`
:   workqueue used to queue **work\_run\_job** and **work\_free\_job**

`timeout_wq`
:   workqueue used to queue **work\_tdr**

`work_run_job`
:   work which calls run\_job op of each scheduler.

`work_free_job`
:   work which calls free\_job op of each scheduler.

`work_tdr`
:   schedules a delayed call to **drm\_sched\_job\_timedout** after the
    timeout interval is over.

`pending_list`
:   the list of jobs which are currently in the job queue.

`job_list_lock`
:   lock to protect the pending\_list.

`hang_limit`
:   once the hangs by a job crosses this limit then it is marked
    guilty and it will no longer be considered for scheduling.

`score`
:   score to help loadbalancer pick a idle sched

`_score`
:   score used when the driver doesn’t provide one

`ready`
:   marks if the underlying HW is ready to work

`free_guilty`
:   A hit to time out handler to free the guilty job.

`pause_submit`
:   pause queuing of **work\_run\_job** on **submit\_wq**

`own_submit_wq`
:   scheduler owns allocation of **submit\_wq**

`dev`
:   system [`struct device`](../driver-api/infrastructure.html#c.device "device")

**Description**

One scheduler is implemented for each hardware ring.

struct drm\_sched\_init\_args
:   parameters for initializing a DRM GPU scheduler

**Definition**:

```
struct drm_sched_init_args {
    const struct drm_sched_backend_ops *ops;
    struct workqueue_struct *submit_wq;
    struct workqueue_struct *timeout_wq;
    u32 num_rqs;
    u32 credit_limit;
    unsigned int hang_limit;
    long timeout;
    atomic_t *score;
    const char *name;
    struct device *dev;
};
```

**Members**

`ops`
:   backend operations provided by the driver

`submit_wq`
:   workqueue to use for submission. If NULL, an ordered wq is
    allocated and used.

`timeout_wq`
:   workqueue to use for timeout work. If NULL, the system\_wq is used.

`num_rqs`
:   Number of run-queues. This may be at most DRM\_SCHED\_PRIORITY\_COUNT,
    as there’s usually one run-queue per priority, but may be less.

`credit_limit`
:   the number of credits this scheduler can hold from all jobs

`hang_limit`
:   number of times to allow a job to hang before dropping it.
    This mechanism is DEPRECATED. Set it to 0.

`timeout`
:   timeout value in jiffies for submitted jobs.

`score`
:   score atomic shared with other schedulers. May be NULL.

`name`
:   name (typically the driver’s name). Used for debugging

`dev`
:   associated device. Used for debugging

struct drm\_sched\_pending\_job\_iter
:   DRM scheduler pending job iterator state

**Definition**:

```
struct drm_sched_pending_job_iter {
    struct drm_gpu_scheduler *sched;
};
```

**Members**

`sched`
:   DRM scheduler associated with pending job iterator

drm\_sched\_for\_each\_pending\_job

`drm_sched_for_each_pending_job (__job, __sched, __entity)`

> Iterator for each pending job in scheduler

**Parameters**

`__job`
:   Current pending job being iterated over

`__sched`
:   DRM scheduler to iterate over pending jobs

`__entity`
:   DRM scheduler entity to filter jobs, NULL indicates no filter

**Description**

Iterator for each pending job in scheduler, filtering on an entity, and
enforcing scheduler is fully stopped

void drm\_sched\_tdr\_queue\_imm(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   * immediately start job timeout handler

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler for which the timeout handling should be started.

**Description**

Start timeout handling immediately for the named scheduler.

void drm\_sched\_fault(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   immediately start timeout handler

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler where the timeout handling should be started.

**Description**

Start timeout handling immediately when the driver detects a hardware fault.

unsigned long drm\_sched\_suspend\_timeout(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   Suspend scheduler job timeout

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance for which to suspend the timeout

**Description**

Suspend the delayed work timeout for the scheduler. This is done by
modifying the delayed work timeout to an arbitrary large value,
MAX\_SCHEDULE\_TIMEOUT in this case.

Returns the timeout remaining

void drm\_sched\_resume\_timeout(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched, unsigned long remaining)
:   Resume scheduler job timeout

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance for which to resume the timeout

`unsigned long remaining`
:   remaining timeout

**Description**

Resume the delayed work timeout for the scheduler.

void drm\_sched\_stop(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched, struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*bad)
:   stop the scheduler

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

`struct drm_sched_job *bad`
:   job which caused the time out

**Description**

Stop the scheduler and also removes and frees all completed jobs.

**Note**

bad job will not be freed as it might be used later and so it’s
callers responsibility to release it manually if it’s not part of the
pending list any more.

This function is typically used for reset recovery (see the docu of
drm\_sched\_backend\_ops.`timedout_job()` for details). Do not call it for
scheduler teardown, i.e., before calling [`drm_sched_fini()`](#c.drm_sched_fini "drm_sched_fini").

As it’s only used for reset recovery, drivers must not call this function
in their [`struct drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").timedout\_job callback when they
skip a reset using [`enum drm_gpu_sched_stat`](#c.drm_gpu_sched_stat "drm_gpu_sched_stat").DRM\_GPU\_SCHED\_STAT\_NO\_HANG.

void drm\_sched\_start(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched, int errno)
:   recover jobs after a reset

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

`int errno`
:   error to set on the pending fences

**Description**

This function is typically used for reset recovery (see the docu of
drm\_sched\_backend\_ops.`timedout_job()` for details). Do not call it for
scheduler startup. The scheduler itself is fully operational after
[`drm_sched_init()`](#c.drm_sched_init "drm_sched_init") succeeded.

As it’s only used for reset recovery, drivers must not call this function
in their [`struct drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").timedout\_job callback when they
skip a reset using [`enum drm_gpu_sched_stat`](#c.drm_gpu_sched_stat "drm_gpu_sched_stat").DRM\_GPU\_SCHED\_STAT\_NO\_HANG.

void drm\_sched\_resubmit\_jobs(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   Deprecated, don’t use in new code!

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

**Description**

Re-submitting jobs was a concept AMD came up as cheap way to implement
recovery after a job timeout.

This turned out to be not working very well. First of all there are many
problem with the dma\_fence implementation and requirements. Either the
implementation is risking deadlocks with core memory management or violating
documented implementation details of the dma\_fence object.

Drivers can still save and restore their state for recovery operations, but
we shouldn’t make this a general scheduler feature around the dma\_fence
interface. The suggested driver-side replacement is to use
[`drm_sched_for_each_pending_job()`](#c.drm_sched_for_each_pending_job "drm_sched_for_each_pending_job") after stopping the scheduler and implement
their own recovery operations.

int drm\_sched\_job\_init(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job, struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity, u32 credits, void \*owner, uint64\_t drm\_client\_id)
:   init a scheduler job

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to init

`struct drm_sched_entity *entity`
:   scheduler entity to use

`u32 credits`
:   the number of credits this job contributes to the schedulers
    credit limit

`void *owner`
:   job owner for debugging

`uint64_t drm_client_id`
:   [`struct drm_file`](drm-internals.html#c.drm_file "drm_file").client\_id of the owner (used by trace
    events)

**Description**

Refer to [`drm_sched_entity_push_job()`](#c.drm_sched_entity_push_job "drm_sched_entity_push_job") documentation
for locking considerations.

Drivers must make sure [`drm_sched_job_cleanup()`](#c.drm_sched_job_cleanup "drm_sched_job_cleanup") if this function returns
successfully, even when **job** is aborted before [`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm") is called.

Note that this function does not assign a valid value to each `struct member`
of [`struct drm_sched_job`](#c.drm_sched_job "drm_sched_job"). Take a look at that struct’s documentation to see
who sets which `struct member` with what lifetime.

WARNING: amdgpu abuses `drm_sched.ready` to signal when the hardware
has died, which can mean that there’s no valid runqueue for a **entity**.
This function returns -ENOENT in this case (which probably should be -EIO as
a more meanigful return value).

Returns 0 for success, negative error code otherwise.

void drm\_sched\_job\_arm(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job)
:   arm a scheduler job for execution

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to arm

**Description**

This arms a scheduler job for execution. Specifically it initializes the
[`drm_sched_job.s_fence`](#c.drm_sched_job "drm_sched_job") of **job**, so that it can be attached to [`struct dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv")
or other places that need to track the completion of this job. It also
initializes sequence numbers, which are fundamental for fence ordering.

Refer to [`drm_sched_entity_push_job()`](#c.drm_sched_entity_push_job "drm_sched_entity_push_job") documentation for locking
considerations.

Once this function was called, you *must* submit **job** with
[`drm_sched_entity_push_job()`](#c.drm_sched_entity_push_job "drm_sched_entity_push_job").

This can only be called if [`drm_sched_job_init()`](#c.drm_sched_job_init "drm_sched_job_init") succeeded.

int drm\_sched\_job\_add\_dependency(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   adds the fence as a job dependency

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to add the dependencies to

`struct dma_fence *fence`
:   the dma\_fence to add to the list of dependencies.

**Description**

Note that **fence** is consumed in both the success and error cases.

**Return**

0 on success, or an error on failing to expand the array.

int drm\_sched\_job\_add\_syncobj\_dependency(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, u32 handle, u32 point)
:   adds a syncobj’s fence as a job dependency

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to add the dependencies to

`struct drm_file *file`
:   drm file private pointer

`u32 handle`
:   syncobj handle to lookup

`u32 point`
:   timeline point

**Description**

This adds the fence matching the given syncobj to **job**.

**Return**

0 on success, or an error on failing to expand the array.

int drm\_sched\_job\_add\_resv\_dependencies(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job, struct [dma\_resv](../driver-api/dma-buf.html#c.dma_resv "dma_resv") \*resv, enum [dma\_resv\_usage](../driver-api/dma-buf.html#c.dma_resv_usage "dma_resv_usage") usage)
:   add all fences from the resv to the job

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to add the dependencies to

`struct dma_resv *resv`
:   the dma\_resv object to get the fences from

`enum dma_resv_usage usage`
:   the dma\_resv\_usage to use to filter the fences

**Description**

This adds all fences matching the given usage from **resv** to **job**.
Must be called with the **resv** lock held.

**Return**

0 on success, or an error on failing to expand the array.

int drm\_sched\_job\_add\_implicit\_dependencies(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job, struct [drm\_gem\_object](#c.drm_gem_object "drm_gem_object") \*obj, bool write)
:   adds implicit dependencies as job dependencies

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to add the dependencies to

`struct drm_gem_object *obj`
:   the gem object to add new dependencies from.

`bool write`
:   whether the job might write the object (so we need to depend on
    shared fences in the reservation object).

**Description**

This should be called after [`drm_gem_lock_reservations()`](#c.drm_gem_lock_reservations "drm_gem_lock_reservations") on your array of
GEM objects used in the job but before updating the reservations with your
own fences.

**Return**

0 on success, or an error on failing to expand the array.

bool drm\_sched\_job\_has\_dependency(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job, struct [dma\_fence](../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   check whether fence is the job’s dependency

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to check

`struct dma_fence *fence`
:   fence to look for

**Return**

True if **fence** is found within the job’s dependencies, or otherwise false.

void drm\_sched\_job\_cleanup(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job)
:   clean up scheduler job resources

**Parameters**

`struct drm_sched_job *job`
:   scheduler job to clean up

**Description**

Cleans up the resources allocated with [`drm_sched_job_init()`](#c.drm_sched_job_init "drm_sched_job_init").

Drivers should call this from their error unwind code if **job** is aborted
before [`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm") is called.

[`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm") is a point of no return since it initializes the fences
and their sequence number etc. Once that function has been called, you *must*
submit it with [`drm_sched_entity_push_job()`](#c.drm_sched_entity_push_job "drm_sched_entity_push_job") and cannot simply abort it by
calling [`drm_sched_job_cleanup()`](#c.drm_sched_job_cleanup "drm_sched_job_cleanup").

This function should be called in the [`drm_sched_backend_ops.free_job`](#c.drm_sched_backend_ops "drm_sched_backend_ops") callback.

struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*drm\_sched\_pick\_best(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*\*sched\_list, unsigned int num\_sched\_list)
:   Get a drm sched from a sched\_list with the least load

**Parameters**

`struct drm_gpu_scheduler **sched_list`
:   list of drm\_gpu\_schedulers

`unsigned int num_sched_list`
:   number of drm\_gpu\_schedulers in the sched\_list

**Description**

Returns pointer of the sched with the least load or NULL if none of the
drm\_gpu\_schedulers are ready

int drm\_sched\_init(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched, const struct [drm\_sched\_init\_args](#c.drm_sched_init_args "drm_sched_init_args") \*args)
:   Init a gpu scheduler instance

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

`const struct drm_sched_init_args *args`
:   scheduler initialization arguments

**Description**

Return 0 on success, otherwise error code.

void drm\_sched\_fini(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   Destroy a gpu scheduler

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

**Description**

Tears down and cleans up the scheduler.

This stops submission of new jobs to the hardware through [`struct
drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").run\_job. If [`struct drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").cancel\_job
is implemented, all jobs will be canceled through it and afterwards cleaned
up through [`struct drm_sched_backend_ops`](#c.drm_sched_backend_ops "drm_sched_backend_ops").free\_job. If cancel\_job is not
implemented, memory could leak.

void drm\_sched\_increase\_karma(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*bad)
:   Update sched\_entity guilty flag

**Parameters**

`struct drm_sched_job *bad`
:   The job guilty of time out

**Description**

Increment on every hang caused by the ‘bad’ job. If this exceeds the hang
limit of the scheduler then the respective sched entity is marked guilty and
jobs from it will not be scheduled further

bool drm\_sched\_wqueue\_ready(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   Is the scheduler ready for submission

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

**Description**

Returns true if submission is ready

void drm\_sched\_wqueue\_stop(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   stop scheduler submission

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

**Description**

Stops the scheduler from pulling new jobs from entities. It also stops
freeing jobs automatically through drm\_sched\_backend\_ops.`free_job()`.

void drm\_sched\_wqueue\_start(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   start scheduler submission

**Parameters**

`struct drm_gpu_scheduler *sched`
:   scheduler instance

**Description**

Restarts the scheduler after [`drm_sched_wqueue_stop()`](#c.drm_sched_wqueue_stop "drm_sched_wqueue_stop") has stopped it.

This function is not necessary for ‘conventional’ startup. The scheduler is
fully operational after [`drm_sched_init()`](#c.drm_sched_init "drm_sched_init") succeeded.

bool drm\_sched\_is\_stopped(struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*sched)
:   Checks whether drm\_sched is stopped

**Parameters**

`struct drm_gpu_scheduler *sched`
:   DRM scheduler

**Return**

true if sched is stopped, false otherwise

bool drm\_sched\_job\_is\_signaled(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*job)
:   DRM scheduler job is signaled

**Parameters**

`struct drm_sched_job *job`
:   DRM scheduler job

**Description**

Determine if DRM scheduler job is signaled. DRM scheduler should be stopped
to obtain a stable snapshot of state. Both parent fence (hardware fence) and
finished fence (software fence) are checked to determine signaling state.

**Return**

true if job is signaled, false otherwise

int drm\_sched\_entity\_init(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity, enum drm\_sched\_priority priority, struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*\*sched\_list, unsigned int num\_sched\_list, atomic\_t \*guilty)
:   Init a context entity used by scheduler when submit to HW ring.

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity to init

`enum drm_sched_priority priority`
:   priority of the entity

`struct drm_gpu_scheduler **sched_list`
:   the list of drm scheds on which jobs from this
    entity can be submitted

`unsigned int num_sched_list`
:   number of drm sched in sched\_list

`atomic_t *guilty`
:   atomic\_t set to 1 when a job on this queue
    is found to be guilty causing a timeout

**Description**

Note that the `sched_list` must have at least one element to schedule the entity.

For changing **priority** later on at runtime see
[`drm_sched_entity_set_priority()`](#c.drm_sched_entity_set_priority "drm_sched_entity_set_priority"). For changing the set of schedulers
**sched\_list** at runtime see [`drm_sched_entity_modify_sched()`](#c.drm_sched_entity_modify_sched "drm_sched_entity_modify_sched").

An entity is cleaned up by calling [`drm_sched_entity_fini()`](#c.drm_sched_entity_fini "drm_sched_entity_fini"). See also
[`drm_sched_entity_destroy()`](#c.drm_sched_entity_destroy "drm_sched_entity_destroy").

Returns 0 on success or a negative error code on failure.

void drm\_sched\_entity\_modify\_sched(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity, struct [drm\_gpu\_scheduler](#c.drm_gpu_scheduler "drm_gpu_scheduler") \*\*sched\_list, unsigned int num\_sched\_list)
:   Modify sched of an entity

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity to init

`struct drm_gpu_scheduler **sched_list`
:   the list of new drm scheds which will replace
    existing entity->sched\_list

`unsigned int num_sched_list`
:   number of drm sched in sched\_list

**Description**

Note that this must be called under the same common lock for **entity** as
[`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm") and [`drm_sched_entity_push_job()`](#c.drm_sched_entity_push_job "drm_sched_entity_push_job"), or the driver needs to
guarantee through some other means that this is never called while new jobs
can be pushed to **entity**.

int drm\_sched\_entity\_error(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity)
:   return error of last scheduled job

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity to check

**Description**

Opportunistically return the error of the last scheduled job. Result can
change any time when new jobs are pushed to the hw.

long drm\_sched\_entity\_flush(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity, long timeout)
:   Flush a context entity

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity

`long timeout`
:   time to wait in for Q to become empty in jiffies.

**Description**

Splitting [`drm_sched_entity_fini()`](#c.drm_sched_entity_fini "drm_sched_entity_fini") into two functions, The first one does the
waiting, removes the entity from the runqueue and returns an error when the
process was killed.

Returns the remaining time in jiffies left from the input timeout

void drm\_sched\_entity\_fini(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity)
:   Destroy a context entity

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity

**Description**

Cleanups up **entity** which has been initialized by [`drm_sched_entity_init()`](#c.drm_sched_entity_init "drm_sched_entity_init").

If there are potentially job still in flight or getting newly queued
[`drm_sched_entity_flush()`](#c.drm_sched_entity_flush "drm_sched_entity_flush") must be called first. This function then goes over
the entity and signals all jobs with an error code if the process was killed.

void drm\_sched\_entity\_destroy(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity)
:   Destroy a context entity

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity

**Description**

Calls [`drm_sched_entity_flush()`](#c.drm_sched_entity_flush "drm_sched_entity_flush") and [`drm_sched_entity_fini()`](#c.drm_sched_entity_fini "drm_sched_entity_fini") as a
convenience wrapper.

void drm\_sched\_entity\_set\_priority(struct [drm\_sched\_entity](#c.drm_sched_entity "drm_sched_entity") \*entity, enum drm\_sched\_priority priority)
:   Sets priority of the entity

**Parameters**

`struct drm_sched_entity *entity`
:   scheduler entity

`enum drm_sched_priority priority`
:   scheduler priority

**Description**

Update the priority of runqueues used for the entity.

void drm\_sched\_entity\_push\_job(struct [drm\_sched\_job](#c.drm_sched_job "drm_sched_job") \*sched\_job)
:   Submit a job to the entity’s job queue

**Parameters**

`struct drm_sched_job *sched_job`
:   job to submit

**Note**

To guarantee that the order of insertion to queue matches the job’s
fence sequence number this function should be called with [`drm_sched_job_arm()`](#c.drm_sched_job_arm "drm_sched_job_arm")
under common lock for the [`struct drm_sched_entity`](#c.drm_sched_entity "drm_sched_entity") that was set up for
**sched\_job** in [`drm_sched_job_init()`](#c.drm_sched_job_init "drm_sched_job_init").
