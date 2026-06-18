# Memory Management APIs

> 출처(원문): https://docs.kernel.org/core-api/mm-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Memory Management APIs

## User Space Memory Access

get\_user

`get_user (x, ptr)`

> Get a simple variable from user space.

**Parameters**

`x`
:   Variable to store result.

`ptr`
:   Source address, in user space.

**Context**

User context only. This function may sleep if pagefaults are
enabled.

**Description**

This macro copies a single simple variable from user space to kernel
space. It supports simple types like char and int, but not larger
data types like structures or arrays.

**ptr** must have pointer-to-simple-variable type, and the result of
dereferencing **ptr** must be assignable to **x** without a cast.

**Return**

zero on success, or -EFAULT on error.
On error, the variable **x** is set to zero.

\_\_get\_user

`__get_user (x, ptr)`

> Get a simple variable from user space, with less checking.

**Parameters**

`x`
:   Variable to store result.

`ptr`
:   Source address, in user space.

**Context**

User context only. This function may sleep if pagefaults are
enabled.

**Description**

This macro copies a single simple variable from user space to kernel
space. It supports simple types like char and int, but not larger
data types like structures or arrays.

**ptr** must have pointer-to-simple-variable type, and the result of
dereferencing **ptr** must be assignable to **x** without a cast.

Caller must check the pointer with `access_ok()` before calling this
function.

**Return**

zero on success, or -EFAULT on error.
On error, the variable **x** is set to zero.

put\_user

`put_user (x, ptr)`

> Write a simple value into user space.

**Parameters**

`x`
:   Value to copy to user space.

`ptr`
:   Destination address, in user space.

**Context**

User context only. This function may sleep if pagefaults are
enabled.

**Description**

This macro copies a single simple value from kernel space to user
space. It supports simple types like char and int, but not larger
data types like structures or arrays.

**ptr** must have pointer-to-simple-variable type, and **x** must be assignable
to the result of dereferencing **ptr**.

**Return**

zero on success, or -EFAULT on error.

\_\_put\_user

`__put_user (x, ptr)`

> Write a simple value into user space, with less checking.

**Parameters**

`x`
:   Value to copy to user space.

`ptr`
:   Destination address, in user space.

**Context**

User context only. This function may sleep if pagefaults are
enabled.

**Description**

This macro copies a single simple value from kernel space to user
space. It supports simple types like char and int, but not larger
data types like structures or arrays.

**ptr** must have pointer-to-simple-variable type, and **x** must be assignable
to the result of dereferencing **ptr**.

Caller must check the pointer with `access_ok()` before calling this
function.

**Return**

zero on success, or -EFAULT on error.

unsigned long clear\_user(void \_\_user \*to, unsigned long n)
:   Zero a block of memory in user space.

**Parameters**

`void __user *to`
:   Destination address, in user space.

`unsigned long n`
:   Number of bytes to zero.

**Description**

Zero a block of memory in user space.

**Return**

number of bytes that could not be cleared.
On success, this will be zero.

unsigned long \_\_clear\_user(void \_\_user \*to, unsigned long n)
:   Zero a block of memory in user space, with less checking.

**Parameters**

`void __user *to`
:   Destination address, in user space.

`unsigned long n`
:   Number of bytes to zero.

**Description**

Zero a block of memory in user space. Caller must check
the specified block with `access_ok()` before calling this function.

**Return**

number of bytes that could not be cleared.
On success, this will be zero.

int get\_user\_pages\_fast(unsigned long start, int nr\_pages, unsigned int gup\_flags, struct page \*\*pages)
:   pin user pages in memory

**Parameters**

`unsigned long start`
:   starting user address

`int nr_pages`
:   number of pages from start to pin

`unsigned int gup_flags`
:   flags modifying pin behaviour

`struct page **pages`
:   array that receives pointers to the pages pinned.
    Should be at least nr\_pages long.

**Description**

Attempt to pin user pages in memory without taking mm->mmap\_lock.
If not successful, it will fall back to taking the lock and
calling `get_user_pages()`.

Returns number of pages pinned. This may be fewer than the number requested.
If nr\_pages is 0 or negative, returns 0. If no pages were pinned, returns
-errno.

## Memory Allocation Controls

### Page mobility and placement hints

These flags provide hints about how mobile the page is. Pages with similar
mobility are placed within the same pageblocks to minimise problems due
to external fragmentation.

`__GFP_MOVABLE` (also a zone modifier) indicates that the page can be
moved by page migration during memory compaction or can be reclaimed.

`__GFP_RECLAIMABLE` is used for slab allocations that specify
SLAB\_RECLAIM\_ACCOUNT and whose pages can be freed via shrinkers.

`__GFP_WRITE` indicates the caller intends to dirty the page. Where possible,
these pages will be spread between local zones to avoid all the dirty
pages being in one zone (fair zone allocation policy).

`__GFP_HARDWALL` enforces the cpuset memory allocation policy.

`__GFP_THISNODE` forces the allocation to be satisfied from the requested
node with no fallbacks or placement policy enforcements.

`__GFP_ACCOUNT` causes the allocation to be accounted to kmemcg.

`__GFP_NO_OBJ_EXT` causes slab allocation to have no object extension.
`mark_obj_codetag_empty()` should be called upon freeing for objects allocated
with this flag to indicate that their NULL tags are expected and normal.

### Watermark modifiers -- controls access to emergency reserves

`__GFP_HIGH` indicates that the caller is high-priority and that granting
the request is necessary before the system can make forward progress.
For example creating an IO context to clean pages and requests
from atomic context.

`__GFP_MEMALLOC` allows access to all memory. This should only be used when
the caller guarantees the allocation will allow more memory to be freed
very shortly e.g. process exiting or swapping. Users either should
be the MM or co-ordinating closely with the VM (e.g. swap over NFS).
Users of this flag have to be extremely careful to not deplete the reserve
completely and implement a throttling mechanism which controls the
consumption of the reserve based on the amount of freed memory.
Usage of a pre-allocated pool (e.g. mempool) should be always considered
before using this flag.

`__GFP_NOMEMALLOC` is used to explicitly forbid access to emergency reserves.
This takes precedence over the `__GFP_MEMALLOC` flag if both are set.

### Reclaim modifiers

Please note that all the following flags are only applicable to sleepable
allocations (e.g. `GFP_NOWAIT` and `GFP_ATOMIC` will ignore them).

`__GFP_IO` can start physical IO.

`__GFP_FS` can call down to the low-level FS. Clearing the flag avoids the
allocator recursing into the filesystem which might already be holding
locks.

`__GFP_DIRECT_RECLAIM` indicates that the caller may enter direct reclaim.
This flag can be cleared to avoid unnecessary delays when a fallback
option is available.

`__GFP_KSWAPD_RECLAIM` indicates that the caller wants to wake kswapd when
the low watermark is reached and have it reclaim pages until the high
watermark is reached. A caller may wish to clear this flag when fallback
options are available and the reclaim is likely to disrupt the system. The
canonical example is THP allocation where a fallback is cheap but
reclaim/compaction may cause indirect stalls.

`__GFP_RECLAIM` is shorthand to allow/forbid both direct and kswapd reclaim.

The default allocator behavior depends on the request size. We have a concept
of so-called costly allocations (with order > `PAGE_ALLOC_COSTLY_ORDER`).
!costly allocations are too essential to fail so they are implicitly
non-failing by default (with some exceptions like OOM victims might fail so
the caller still has to check for failures) while costly requests try to be
not disruptive and back off even without invoking the OOM killer.
The following three modifiers might be used to override some of these
implicit rules. Please note that all of them must be used along with
`__GFP_DIRECT_RECLAIM` flag.

`__GFP_NORETRY`: The VM implementation will try only very lightweight
memory direct reclaim to get some memory under memory pressure (thus
it can sleep). It will avoid disruptive actions like OOM killer. The
caller must handle the failure which is quite likely to happen under
heavy memory pressure. The flag is suitable when failure can easily be
handled at small cost, such as reduced throughput.

`__GFP_RETRY_MAYFAIL`: The VM implementation will retry memory reclaim
procedures that have previously failed if there is some indication
that progress has been made elsewhere. It can wait for other
tasks to attempt high-level approaches to freeing memory such as
compaction (which removes fragmentation) and page-out.
There is still a definite limit to the number of retries, but it is
a larger limit than with `__GFP_NORETRY`.
Allocations with this flag may fail, but only when there is
genuinely little unused memory. While these allocations do not
directly trigger the OOM killer, their failure indicates that
the system is likely to need to use the OOM killer soon. The
caller must handle failure, but can reasonably do so by failing
a higher-level request, or completing it only in a much less
efficient manner.
If the allocation does fail, and the caller is in a position to
free some non-essential memory, doing so could benefit the system
as a whole.

`__GFP_NOFAIL`: The VM implementation \_must\_ retry infinitely: the caller
cannot handle allocation failures. The allocation could block
indefinitely but will never return with failure. Testing for
failure is pointless.
It \_must\_ be blockable and used together with \_\_GFP\_DIRECT\_RECLAIM.
It should \_never\_ be used in non-sleepable contexts.
New users should be evaluated carefully (and the flag should be
used only when there is no reasonable failure policy) but it is
definitely preferable to use the flag rather than opencode endless
loop around allocator.
Allocating pages from the buddy with \_\_GFP\_NOFAIL and order > 1 is
not supported. Please consider using `kvmalloc()` instead.

### Useful GFP flag combinations

Useful GFP flag combinations that are commonly used. It is recommended
that subsystems start with one of these combinations and then set/clear
`__GFP_FOO` flags as necessary.

`GFP_ATOMIC` users can not sleep and need the allocation to succeed. A lower
watermark is applied to allow access to “atomic reserves”.
The current implementation doesn’t support NMI, nor contexts that disable
preemption under PREEMPT\_RT. This includes `raw_spin_lock()` and plain
`preempt_disable()` - see “Memory allocation” in
[How realtime kernels differ](real-time/differences.html) for more info.

`GFP_KERNEL` is typical for kernel-internal allocations. The caller requires
`ZONE_NORMAL` or a lower zone for direct access but can direct reclaim.

`GFP_KERNEL_ACCOUNT` is the same as GFP\_KERNEL, except the allocation is
accounted to kmemcg.

`GFP_NOWAIT` is for kernel allocations that should not stall for direct
reclaim, start physical IO or use any filesystem callback. It is very
likely to fail to allocate memory, even for very small allocations.
The same restrictions on calling contexts apply as for `GFP_ATOMIC`.

`GFP_NOIO` will use direct reclaim to discard clean pages or slab pages
that do not require the starting of any physical IO.
Please try to avoid using this flag directly and instead use
memalloc\_noio\_{save,restore} to mark the whole scope which cannot
perform any IO with a short explanation why. All allocation requests
will inherit GFP\_NOIO implicitly.

`GFP_NOFS` will use direct reclaim but will not use any filesystem interfaces.
Please try to avoid using this flag directly and instead use
memalloc\_nofs\_{save,restore} to mark the whole scope which cannot/shouldn’t
recurse into the FS layer with a short explanation why. All allocation
requests will inherit GFP\_NOFS implicitly.

`GFP_USER` is for userspace allocations that also need to be directly
accessibly by the kernel or hardware. It is typically used by hardware
for buffers that are mapped to userspace (e.g. graphics) that hardware
still must DMA to. cpuset limits are enforced for these allocations.

`GFP_DMA` exists for historical reasons and should be avoided where possible.
The flags indicates that the caller requires that the lowest zone be
used (`ZONE_DMA` or 16M on x86-64). Ideally, this would be removed but
it would require careful auditing as some users really require it and
others use the flag to avoid lowmem reserves in `ZONE_DMA` and treat the
lowest zone as a type of emergency reserve.

`GFP_DMA32` is similar to `GFP_DMA` except that the caller requires a 32-bit
address. Note that kmalloc(..., GFP\_DMA32) does not return DMA32 memory
because the DMA32 kmalloc cache array is not implemented.
(Reason: there is no such user in kernel).

`GFP_HIGHUSER` is for userspace allocations that may be mapped to userspace,
do not need to be directly accessible by the kernel but that cannot
move once in use. An example may be a hardware allocation that maps
data directly into userspace but has no addressing limitations.

`GFP_HIGHUSER_MOVABLE` is for userspace allocations that the kernel does not
need direct access to but can use [`kmap()`](../mm/highmem.html#c.kmap "kmap") when access is required. They
are expected to be movable via page reclaim or page migration. Typically,
pages on the LRU would also be allocated with `GFP_HIGHUSER_MOVABLE`.

`GFP_TRANSHUGE` and `GFP_TRANSHUGE_LIGHT` are used for THP allocations. They
are compound allocations that will generally fail quickly if memory is not
available and will not wake kswapd/kcompactd on failure. The \_LIGHT
version does not attempt reclaim/compaction at all and is by default used
in page fault path, while the non-light is used by khugepaged.

## The Slab Cache

SLAB\_HWCACHE\_ALIGN

`SLAB_HWCACHE_ALIGN`

> > Align objects on cache line boundaries.
>
> **Description**
>
> Sufficiently large objects are aligned on cache line boundary. For object
> size smaller than a half of cache line size, the alignment is on the half of
> cache line size. In general, if object size is smaller than 1/2^n of cache
> line size, the alignment is adjusted to 1/2^n.
>
> If explicit alignment is also requested by the respective
> [`struct kmem_cache_args`](#c.kmem_cache_args "kmem_cache_args") field, the greater of both is alignments is applied.

SLAB\_TYPESAFE\_BY\_RCU

`SLAB_TYPESAFE_BY_RCU`

> > **WARNING** READ THIS!
>
> **Description**
>
> This delays freeing the SLAB page by a grace period, it does \_NOT\_
> delay object freeing. This means that if you do [`kmem_cache_free()`](#c.kmem_cache_free "kmem_cache_free")
> that memory location is free to be reused at any time. Thus it may
> be possible to see another object there in the same RCU grace period.
>
> This feature only ensures the memory location backing the object
> stays valid, the trick to using this is relying on an independent
> object validation pass. Something like:
>
> ```
> begin:
>  rcu_read_lock();
>  obj = lockless_lookup(key);
>  if (obj) {
>    if (!try_get_ref(obj)) // might fail for free objects
>      rcu_read_unlock();
>      goto begin;
>
>    if (obj->key != key) { // not the object we expected
>      put_ref(obj);
>      rcu_read_unlock();
>      goto begin;
>    }
>  }
> rcu_read_unlock();
> ```
>
> This is useful if we need to approach a kernel structure obliquely,
> from its address obtained without the usual locking. We can lock
> the structure to stabilize it and check it’s still at the given address,
> only if we can be sure that the memory has not been meanwhile reused
> for some other kind of object (which our subsystem’s lock might corrupt).
>
> rcu\_read\_lock before reading the address, then rcu\_read\_unlock after
> taking the spinlock within the structure expected at that address.
>
> Note that object identity check has to be done *after* acquiring a
> reference, therefore user has to ensure proper ordering for loads.
> Similarly, when initializing objects allocated with SLAB\_TYPESAFE\_BY\_RCU,
> the newly allocated object has to be fully initialized *before* its
> refcount gets initialized and proper ordering for stores is required.
> refcount\_{add|inc}`_not_zero_acquire()` and [`refcount_set_release()`](../driver-api/basics.html#c.refcount_set_release "refcount_set_release") are
> designed with the proper fences required for reference counting objects
> allocated with SLAB\_TYPESAFE\_BY\_RCU.
>
> Note that it is not possible to acquire a lock within a structure
> allocated with SLAB\_TYPESAFE\_BY\_RCU without first acquiring a reference
> as described above. The reason is that SLAB\_TYPESAFE\_BY\_RCU pages
> are not zeroed before being given to the slab, which means that any
> locks must be initialized after each and every `kmem_struct_alloc()`.
> Alternatively, make the ctor passed to [`kmem_cache_create()`](#c.kmem_cache_create "kmem_cache_create") initialize
> the locks at page-allocation time, as is done in `__i915_request_ctor()`,
> `sighand_ctor()`, and `anon_vma_ctor()`. Such a ctor permits readers
> to safely acquire those ctor-initialized locks under [`rcu_read_lock()`](kernel-api.html#c.rcu_read_lock "rcu_read_lock")
> protection.
>
> Note that SLAB\_TYPESAFE\_BY\_RCU was originally named SLAB\_DESTROY\_BY\_RCU.

SLAB\_ACCOUNT

`SLAB_ACCOUNT`

> > Account allocations to memcg.
>
> **Description**
>
> All object allocations from this cache will be memcg accounted, regardless of
> \_\_GFP\_ACCOUNT being or not being passed to individual allocations.

SLAB\_RECLAIM\_ACCOUNT

`SLAB_RECLAIM_ACCOUNT`

> > Objects are reclaimable.
>
> **Description**
>
> Use this flag for caches that have an associated shrinker. As a result, slab
> pages are allocated with \_\_GFP\_RECLAIMABLE, which affects grouping pages by
> mobility, and are accounted in SReclaimable counter in /proc/meminfo

struct kmem\_cache\_args
:   Less common arguments for [`kmem_cache_create()`](#c.kmem_cache_create "kmem_cache_create")

**Definition**:

```
struct kmem_cache_args {
    unsigned int align;
    unsigned int useroffset;
    unsigned int usersize;
    unsigned int freeptr_offset;
    bool use_freeptr_offset;
    void (*ctor)(void *);
    unsigned int sheaf_capacity;
};
```

**Members**

`align`
:   The required alignment for the objects.

    `0` means no specific alignment is requested.

`useroffset`
:   Usercopy region offset.

    `0` is a valid offset, when **usersize** is non-`0`

`usersize`
:   Usercopy region size.

    `0` means no usercopy region is specified.

`freeptr_offset`
:   Custom offset for the free pointer
    in caches with [`SLAB_TYPESAFE_BY_RCU`](#c.SLAB_TYPESAFE_BY_RCU "SLAB_TYPESAFE_BY_RCU") or **ctor**

    By default, [`SLAB_TYPESAFE_BY_RCU`](#c.SLAB_TYPESAFE_BY_RCU "SLAB_TYPESAFE_BY_RCU") and **ctor** caches place the free
    pointer outside of the object. This might cause the object to grow
    in size. Cache creators that have a reason to avoid this can specify
    a custom free pointer offset in their data structure where the free
    pointer will be placed.

    For caches with [`SLAB_TYPESAFE_BY_RCU`](#c.SLAB_TYPESAFE_BY_RCU "SLAB_TYPESAFE_BY_RCU"), the caller must ensure that
    the free pointer does not overlay fields required to guard against
    object recycling (See [`SLAB_TYPESAFE_BY_RCU`](#c.SLAB_TYPESAFE_BY_RCU "SLAB_TYPESAFE_BY_RCU") for details).

    For caches with **ctor**, the caller must ensure that the free pointer
    does not overlay fields initialized by the constructor.

    Currently, only caches with [`SLAB_TYPESAFE_BY_RCU`](#c.SLAB_TYPESAFE_BY_RCU "SLAB_TYPESAFE_BY_RCU") or **ctor**
    may specify **freeptr\_offset**.

    Using `0` as a value for **freeptr\_offset** is valid. If **freeptr\_offset**
    is specified, **use\_freeptr\_offset** must be set `true`.

`use_freeptr_offset`
:   Whether a **freeptr\_offset** is used.

`ctor`
:   A constructor for the objects.

    The constructor is invoked for each object in a newly allocated slab
    page. It is the cache user’s responsibility to free object in the
    same state as after calling the constructor, or deal appropriately
    with any differences between a freshly constructed and a reallocated
    object.

    `NULL` means no constructor.

`sheaf_capacity`
:   Enable sheaves of given capacity for the cache.

    With a non-zero value, allocations from the cache go through caching
    arrays called sheaves. Each cpu has a main sheaf that’s always
    present, and a spare sheaf that may be not present. When both become
    empty, there’s an attempt to replace an empty sheaf with a full sheaf
    from the per-node barn.

    When no full sheaf is available, and gfp flags allow blocking, a
    sheaf is allocated and filled from slab(s) using bulk allocation.
    Otherwise the allocation falls back to the normal operation
    allocating a single object from a slab.

    Analogically when freeing and both percpu sheaves are full, the barn
    may replace it with an empty sheaf, unless it’s over capacity. In
    that case a sheaf is bulk freed to slab pages.

    The sheaves do not enforce NUMA placement of objects, so allocations
    via [`kmem_cache_alloc_node()`](#c.kmem_cache_alloc_node "kmem_cache_alloc_node") with a node specified other than
    NUMA\_NO\_NODE will bypass them.

    Bulk allocation and free operations also try to use the cpu sheaves
    and barn, but fallback to using slab pages directly.

    When slub\_debug is enabled for the cache, the sheaf\_capacity argument
    is ignored.

    `0` means no sheaves will be created.

**Description**

Any uninitialized fields of the structure are interpreted as unused. The
exception is **freeptr\_offset** where `0` is a valid value, so
**use\_freeptr\_offset** must be also set to `true` in order to interpret the field
as used. For **useroffset** `0` is also valid, but only with non-`0`
**usersize**.

When `NULL` args is passed to [`kmem_cache_create()`](#c.kmem_cache_create "kmem_cache_create"), it is equivalent to all
fields unused.

struct kmem\_cache \*kmem\_cache\_create\_usercopy(const char \*name, unsigned int size, unsigned int align, slab\_flags\_t flags, unsigned int useroffset, unsigned int usersize, void (\*ctor)(void\*))
:   Create a kmem cache with a region suitable for copying to userspace.

**Parameters**

`const char *name`
:   A string which is used in /proc/slabinfo to identify this cache.

`unsigned int size`
:   The size of objects to be created in this cache.

`unsigned int align`
:   The required alignment for the objects.

`slab_flags_t flags`
:   SLAB flags

`unsigned int useroffset`
:   Usercopy region offset

`unsigned int usersize`
:   Usercopy region size

`void (*ctor)(void *)`
:   A constructor for the objects, or `NULL`.

**Description**

This is a legacy wrapper, new code should use either `KMEM_CACHE_USERCOPY()`
if whitelisting a single field is sufficient, or [`kmem_cache_create()`](#c.kmem_cache_create "kmem_cache_create") with
the necessary parameters passed via the args parameter (see
[`struct kmem_cache_args`](#c.kmem_cache_args "kmem_cache_args"))

**Return**

a pointer to the cache on success, NULL on failure.

kmem\_cache\_create

`kmem_cache_create (__name, __object_size, __args, ...)`

> Create a kmem cache.

**Parameters**

`__name`
:   A string which is used in /proc/slabinfo to identify this cache.

`__object_size`
:   The size of objects to be created in this cache.

`__args`
:   Optional arguments, see [`struct kmem_cache_args`](#c.kmem_cache_args "kmem_cache_args"). Passing `NULL`
    means defaults will be used for all the arguments.

`...`
:   variable arguments

**Description**

This is currently implemented as a macro using `_Generic()` to call
either the new variant of the function, or a legacy one.

The new variant has 4 parameters:
`kmem_cache_create(name, object_size, args, flags)`

See [`__kmem_cache_create_args()`](#c.__kmem_cache_create_args "__kmem_cache_create_args") which implements this.

The legacy variant has 5 parameters:
`kmem_cache_create(name, object_size, align, flags, ctor)`

The align and ctor parameters map to the respective fields of
[`struct kmem_cache_args`](#c.kmem_cache_args "kmem_cache_args")

**Context**

Cannot be called within a interrupt, but can be interrupted.

**Return**

a pointer to the cache on success, NULL on failure.

void \*kmem\_cache\_alloc(struct kmem\_cache \*cachep, gfp\_t flags)
:   Allocate an object

**Parameters**

`struct kmem_cache *cachep`
:   The cache to allocate from.

`gfp_t flags`
:   See [`kmalloc()`](#c.kmalloc "kmalloc").

**Description**

Allocate an object from this cache.
See `kmem_cache_zalloc()` for a shortcut of adding \_\_GFP\_ZERO to flags.

**Return**

pointer to the new object or `NULL` in case of error

bool kmem\_cache\_charge(void \*objp, gfp\_t gfpflags)
:   memcg charge an already allocated slab memory

**Parameters**

`void *objp`
:   address of the slab object to memcg charge

`gfp_t gfpflags`
:   describe the allocation context

**Description**

kmem\_cache\_charge allows charging a slab object to the current memcg,
primarily in cases where charging at allocation time might not be possible
because the target memcg is not known (i.e. softirq context)

The objp should be pointer returned by the slab allocator functions like
kmalloc (with \_\_GFP\_ACCOUNT in flags) or kmem\_cache\_alloc. The memcg charge
behavior can be controlled through gfpflags parameter, which affects how the
necessary internal metadata can be allocated. Including \_\_GFP\_NOFAIL denotes
that overcharging is requested instead of failure, but is not applied for the
internal metadata allocation.

There are several cases where it will return true even if the charging was
not done:
More specifically:

1. For !CONFIG\_MEMCG or cgroup\_disable=memory systems.
2. Already charged slab objects.
3. For slab objects from KMALLOC\_NORMAL caches - allocated by [`kmalloc()`](#c.kmalloc "kmalloc")
   without \_\_GFP\_ACCOUNT
4. Allocating internal metadata has failed

**Return**

true if charge was successful otherwise false.

void \*kmalloc(size\_t size, gfp\_t flags)
:   allocate kernel memory

**Parameters**

`size_t size`
:   how many bytes of memory are required.

`gfp_t flags`
:   describe the allocation context

**Description**

kmalloc is the normal method of allocating memory
for objects smaller than page size in the kernel.

The allocated object address is aligned to at least ARCH\_KMALLOC\_MINALIGN
bytes. For **size** of power of two bytes, the alignment is also guaranteed
to be at least to the size. For other sizes, the alignment is guaranteed to
be at least the largest power-of-two divisor of **size**.

The **flags** argument may be one of the GFP flags defined at
include/linux/gfp\_types.h and described at
[Documentation/core-api/mm-api.rst](#mm-api-gfp-flags)

The recommended usage of the **flags** is described at
[Documentation/core-api/memory-allocation.rst](memory-allocation.html#memory-allocation)

Below is a brief outline of the most useful GFP flags

`GFP_KERNEL`
:   Allocate normal kernel ram. May sleep.

`GFP_NOWAIT`
:   Allocation will not sleep.

`GFP_ATOMIC`
:   Allocation will not sleep. May use emergency pools.

Also it is possible to set different flags by OR’ing
in one or more of the following additional **flags**:

`__GFP_ZERO`
:   Zero the allocated memory before returning. Also see [`kzalloc()`](#c.kzalloc "kzalloc").

`__GFP_HIGH`
:   This allocation has high priority and may use emergency pools.

`__GFP_NOFAIL`
:   Indicate that this allocation is in no way allowed to fail
    (think twice before using).

`__GFP_NORETRY`
:   If memory is not immediately available,
    then give up at once.

`__GFP_NOWARN`
:   If allocation fails, don’t issue any warnings.

`__GFP_RETRY_MAYFAIL`
:   Try really hard to succeed the allocation but fail
    eventually.

\_\_alloc\_objs

`__alloc_objs (KMALLOC, GFP, TYPE, COUNT)`

> Allocate objects of a given type using

**Parameters**

`KMALLOC`
:   which size-based kmalloc wrapper to allocate with.

`GFP`
:   GFP flags for the allocation.

`TYPE`
:   type to allocate space for.

`COUNT`
:   how many **TYPE** objects to allocate.

**Return**

Newly allocated pointer to (first) **TYPE** of **COUNT**-many
allocated **TYPE** objects, or NULL on failure.

\_\_alloc\_flex

`__alloc_flex (KMALLOC, GFP, TYPE, FAM, COUNT)`

> Allocate an object that has a trailing flexible array

**Parameters**

`KMALLOC`
:   kmalloc wrapper function to use for allocation.

`GFP`
:   GFP flags for the allocation.

`TYPE`
:   type of structure to allocate space for.

`FAM`
:   The name of the flexible array member of **TYPE** structure.

`COUNT`
:   how many **FAM** elements to allocate space for.

**Return**

Newly allocated pointer to **TYPE** with **COUNT**-many trailing
**FAM** elements, or NULL on failure or if **COUNT** cannot be represented
by the member of **TYPE** that counts the **FAM** elements (annotated via
`__counted_by()`).

kmalloc\_obj

`kmalloc_obj (VAR_OR_TYPE, ...)`

> Allocate a single instance of the given type

**Parameters**

`VAR_OR_TYPE`
:   Variable or type to allocate.

`...`
:   variable arguments

**Return**

newly allocated pointer to a **VAR\_OR\_TYPE** on success, or NULL
on failure.

kmalloc\_objs

`kmalloc_objs (VAR_OR_TYPE, COUNT, ...)`

> Allocate an array of the given type

**Parameters**

`VAR_OR_TYPE`
:   Variable or type to allocate an array of.

`COUNT`
:   How many elements in the array.

`...`
:   variable arguments

**Return**

newly allocated pointer to array of **VAR\_OR\_TYPE** on success,
or NULL on failure.

kmalloc\_flex

`kmalloc_flex (VAR_OR_TYPE, FAM, COUNT, ...)`

> Allocate a single instance of the given flexible structure

**Parameters**

`VAR_OR_TYPE`
:   Variable or type to allocate (with its flex array).

`FAM`
:   The name of the flexible array member of the structure.

`COUNT`
:   How many flexible array member elements are desired.

`...`
:   variable arguments

**Return**

newly allocated pointer to **VAR\_OR\_TYPE** on success, NULL on
failure. If **FAM** has been annotated with `__counted_by()`, the allocation
will immediately fail if **COUNT** is larger than what the type of the
struct’s counter variable can represent.

void \*kmalloc\_array(size\_t n, size\_t size, gfp\_t flags)
:   allocate memory for an array.

**Parameters**

`size_t n`
:   number of elements.

`size_t size`
:   element size.

`gfp_t flags`
:   the type of memory to allocate (see kmalloc).

void \*krealloc\_array(void \*p, size\_t new\_n, size\_t new\_size, gfp\_t flags)
:   reallocate memory for an array.

**Parameters**

`void *p`
:   pointer to the memory chunk to reallocate

`size_t new_n`
:   new number of elements to alloc

`size_t new_size`
:   new size of a single member of the array

`gfp_t flags`
:   the type of memory to allocate (see kmalloc)

**Description**

If \_\_GFP\_ZERO logic is requested, callers must ensure that, starting with the
initial memory allocation, every subsequent call to this API for the same
memory allocation is flagged with \_\_GFP\_ZERO. Otherwise, it is possible that
\_\_GFP\_ZERO is not fully honored by this API.

See `krealloc_noprof()` for further details.

In any case, the contents of the object pointed to are preserved up to the
lesser of the new and old sizes.

kcalloc

`kcalloc (n, size, flags)`

> allocate memory for an array. The memory is set to zero.

**Parameters**

`n`
:   number of elements.

`size`
:   element size.

`flags`
:   the type of memory to allocate (see kmalloc).

void \*kzalloc(size\_t size, gfp\_t flags)
:   allocate memory. The memory is set to zero.

**Parameters**

`size_t size`
:   how many bytes of memory are required.

`gfp_t flags`
:   the type of memory to allocate (see kmalloc).

size\_t kmalloc\_size\_roundup(size\_t size)
:   Report allocation bucket size for the given size

**Parameters**

`size_t size`
:   Number of bytes to round up from.

**Description**

This returns the number of bytes that would be available in a [`kmalloc()`](#c.kmalloc "kmalloc")
allocation of **size** bytes. For example, a 126 byte request would be
rounded up to the next sized kmalloc bucket, 128 bytes. (This is strictly
for the general-purpose [`kmalloc()`](#c.kmalloc "kmalloc")-based allocations, and is not for the
pre-sized [`kmem_cache_alloc()`](#c.kmem_cache_alloc "kmem_cache_alloc")-based allocations.)

Use this to [`kmalloc()`](#c.kmalloc "kmalloc") the full bucket size ahead of time instead of using
[`ksize()`](#c.ksize "ksize") to query the size after an allocation.

void \*kmem\_cache\_alloc\_node(struct kmem\_cache \*s, gfp\_t gfpflags, int node)
:   Allocate an object on the specified node

**Parameters**

`struct kmem_cache *s`
:   The cache to allocate from.

`gfp_t gfpflags`
:   See [`kmalloc()`](#c.kmalloc "kmalloc").

`int node`
:   node number of the target node.

**Description**

Identical to kmem\_cache\_alloc but it will allocate memory on the given
node, which can improve the performance for cpu bound structures.

Fallback to other node is possible if \_\_GFP\_THISNODE is not set.

**Return**

pointer to the new object or `NULL` in case of error

void \*kmalloc\_nolock(size\_t size, gfp\_t gfp\_flags, int node)
:   Allocate an object of given size from any context.

**Parameters**

`size_t size`
:   size to allocate

`gfp_t gfp_flags`
:   GFP flags. Only \_\_GFP\_ACCOUNT, \_\_GFP\_ZERO, \_\_GFP\_NO\_OBJ\_EXT
    allowed.

`int node`
:   node number of the target node.

**Return**

pointer to the new object or NULL in case of error.
NULL does not mean EBUSY or EAGAIN. It means ENOMEM.
There is no reason to call it again and expect !NULL.

void kmem\_cache\_free(struct kmem\_cache \*s, void \*x)
:   Deallocate an object

**Parameters**

`struct kmem_cache *s`
:   The cache the allocation was from.

`void *x`
:   The previously allocated object.

**Description**

Free an object which was previously allocated from this
cache.

size\_t ksize(const void \*objp)
:   * Report full size of underlying allocation

**Parameters**

`const void *objp`
:   pointer to the object

**Description**

This should only be used internally to query the true size of allocations.
It is not meant to be a way to discover the usable size of an allocation
after the fact. Instead, use [`kmalloc_size_roundup()`](#c.kmalloc_size_roundup "kmalloc_size_roundup"). Using memory beyond
the originally requested allocation size may trigger KASAN, UBSAN\_BOUNDS,
and/or FORTIFY\_SOURCE.

**Return**

size of the actual memory used by **objp** in bytes

void kfree(const void \*object)
:   free previously allocated memory

**Parameters**

`const void *object`
:   pointer returned by [`kmalloc()`](#c.kmalloc "kmalloc"), [`kmalloc_nolock()`](#c.kmalloc_nolock "kmalloc_nolock"), or [`kmem_cache_alloc()`](#c.kmem_cache_alloc "kmem_cache_alloc")

**Description**

If **object** is NULL, no operation is performed.

void \*krealloc\_node\_align(const void \*p, size\_t new\_size, unsigned long align, gfp\_t flags, int nid)
:   reallocate memory. The contents will remain unchanged.

**Parameters**

`const void *p`
:   object to reallocate memory for.

`size_t new_size`
:   how many bytes of memory are required.

`unsigned long align`
:   desired alignment.

`gfp_t flags`
:   the type of memory to allocate.

`int nid`
:   NUMA node or NUMA\_NO\_NODE

**Description**

If **p** is `NULL`, `krealloc()` behaves exactly like [`kmalloc()`](#c.kmalloc "kmalloc"). If **new\_size**
is 0 and **p** is not a `NULL` pointer, the object pointed to is freed.

Only alignments up to those guaranteed by [`kmalloc()`](#c.kmalloc "kmalloc") will be honored. Please see
[Memory Allocation Guide](memory-allocation.html) for more details.

If \_\_GFP\_ZERO logic is requested, callers must ensure that, starting with the
initial memory allocation, every subsequent call to this API for the same
memory allocation is flagged with \_\_GFP\_ZERO. Otherwise, it is possible that
\_\_GFP\_ZERO is not fully honored by this API.

When `slub_debug_orig_size()` is off, `krealloc()` only knows about the bucket
size of an allocation (but not the exact size it was allocated with) and
hence implements the following semantics for shrinking and growing buffers
with \_\_GFP\_ZERO:

```
        new             bucket
0       size             size
|--------|----------------|
|  keep  |      zero      |
```

Otherwise, the original allocation size ‘orig\_size’ could be used to
precisely clear the requested size, and the new size will also be stored
as the new ‘orig\_size’.

In any case, the contents of the object pointed to are preserved up to the
lesser of the new and old sizes.

**Return**

pointer to the allocated memory or `NULL` in case of error

void \*\_\_kvmalloc\_node(size, b, unsigned long align, gfp\_t flags, int node)
:   attempt to allocate physically contiguous memory, but upon failure, fall back to non-contiguous (vmalloc) allocation.

**Parameters**

`size`
:   size of the request.

`b`
:   which set of kmalloc buckets to allocate from.

`unsigned long align`
:   desired alignment.

`gfp_t flags`
:   gfp mask for the allocation - must be compatible (superset) with GFP\_KERNEL.

`int node`
:   numa node to allocate from

**Description**

Only alignments up to those guaranteed by [`kmalloc()`](#c.kmalloc "kmalloc") will be honored. Please see
[Memory Allocation Guide](memory-allocation.html) for more details.

Uses kmalloc to get the memory but if the allocation fails then falls back
to the vmalloc allocator. Use kvfree for freeing the memory.

GFP\_NOWAIT and GFP\_ATOMIC are supported, the \_\_GFP\_NORETRY modifier is not.
\_\_GFP\_RETRY\_MAYFAIL is supported, and it should be used only if kmalloc is
preferable to the vmalloc fallback, due to visible performance drawbacks.

**Return**

pointer to the allocated memory of `NULL` in case of failure

void kvfree(const void \*addr)
:   Free memory.

**Parameters**

`const void *addr`
:   Pointer to allocated memory.

**Description**

kvfree frees memory allocated by any of [`vmalloc()`](#c.vmalloc "vmalloc"), [`kmalloc()`](#c.kmalloc "kmalloc") or `kvmalloc()`.
It is slightly more efficient to use [`kfree()`](#c.kfree "kfree") or [`vfree()`](#c.vfree "vfree") if you are certain
that you know which one to use.

**Context**

Either preemptible task context or not-NMI interrupt.

void kvfree\_atomic(const void \*addr)
:   Free memory.

**Parameters**

`const void *addr`
:   Pointer to allocated memory.

**Description**

Same as [`kvfree()`](#c.kvfree "kvfree"), but uses `vfree_atomic()` for vmalloc
backed memory. Must not be called from NMI context.

void kvfree\_sensitive(const void \*addr, size\_t len)
:   Free a data object containing sensitive information.

**Parameters**

`const void *addr`
:   address of the data object to be freed.

`size_t len`
:   length of the data object.

**Description**

Use the special [`memzero_explicit()`](kernel-api.html#c.memzero_explicit "memzero_explicit") function to clear the content of a
kvmalloc’ed object containing sensitive data to make sure that the
compiler won’t optimize out the data clearing.

void \*kvrealloc\_node\_align(const void \*p, size\_t size, unsigned long align, gfp\_t flags, int nid)
:   reallocate memory; contents remain unchanged

**Parameters**

`const void *p`
:   object to reallocate memory for

`size_t size`
:   the size to reallocate

`unsigned long align`
:   desired alignment

`gfp_t flags`
:   the flags for the page level allocator

`int nid`
:   NUMA node id

**Description**

If **p** is `NULL`, `kvrealloc()` behaves exactly like `kvmalloc()`. If **size** is 0
and **p** is not a `NULL` pointer, the object pointed to is freed.

Only alignments up to those guaranteed by [`kmalloc()`](#c.kmalloc "kmalloc") will be honored. Please see
[Memory Allocation Guide](memory-allocation.html) for more details.

If \_\_GFP\_ZERO logic is requested, callers must ensure that, starting with the
initial memory allocation, every subsequent call to this API for the same
memory allocation is flagged with \_\_GFP\_ZERO. Otherwise, it is possible that
\_\_GFP\_ZERO is not fully honored by this API.

In any case, the contents of the object pointed to are preserved up to the
lesser of the new and old sizes.

This function must not be called concurrently with itself or [`kvfree()`](#c.kvfree "kvfree") for the
same memory allocation.

**Return**

pointer to the allocated memory or `NULL` in case of error

struct kmem\_cache \*\_\_kmem\_cache\_create\_args(const char \*name, unsigned int object\_size, struct [kmem\_cache\_args](#c.kmem_cache_args "kmem_cache_args") \*args, slab\_flags\_t flags)
:   Create a kmem cache.

**Parameters**

`const char *name`
:   A string which is used in /proc/slabinfo to identify this cache.

`unsigned int object_size`
:   The size of objects to be created in this cache.

`struct kmem_cache_args *args`
:   Additional arguments for the cache creation (see
    [`struct kmem_cache_args`](#c.kmem_cache_args "kmem_cache_args")).

`slab_flags_t flags`
:   See the descriptions of individual flags. The common ones are listed
    in the description below.

**Description**

Not to be called directly, use the [`kmem_cache_create()`](#c.kmem_cache_create "kmem_cache_create") wrapper with the same
parameters.

Commonly used **flags**:

[`SLAB_ACCOUNT`](#c.SLAB_ACCOUNT "SLAB_ACCOUNT") - Account allocations to memcg.

[`SLAB_HWCACHE_ALIGN`](#c.SLAB_HWCACHE_ALIGN "SLAB_HWCACHE_ALIGN") - Align objects on cache line boundaries.

[`SLAB_RECLAIM_ACCOUNT`](#c.SLAB_RECLAIM_ACCOUNT "SLAB_RECLAIM_ACCOUNT") - Objects are reclaimable.

[`SLAB_TYPESAFE_BY_RCU`](#c.SLAB_TYPESAFE_BY_RCU "SLAB_TYPESAFE_BY_RCU") - Slab page (not individual objects) freeing delayed
by a grace period - see the full description before using.

**Context**

Cannot be called within a interrupt, but can be interrupted.

**Return**

a pointer to the cache on success, NULL on failure.

kmem\_buckets \*kmem\_buckets\_create(const char \*name, slab\_flags\_t flags, unsigned int useroffset, unsigned int usersize, void (\*ctor)(void\*))
:   Create a set of caches that handle dynamic sized allocations via `kmem_buckets_alloc()`

**Parameters**

`const char *name`
:   A prefix string which is used in /proc/slabinfo to identify this
    cache. The individual caches with have their sizes as the suffix.

`slab_flags_t flags`
:   SLAB flags (see [`kmem_cache_create()`](#c.kmem_cache_create "kmem_cache_create") for details).

`unsigned int useroffset`
:   Starting offset within an allocation that may be copied
    to/from userspace.

`unsigned int usersize`
:   How many bytes, starting at **useroffset**, may be copied
    to/from userspace.

`void (*ctor)(void *)`
:   A constructor for the objects, run when new allocations are made.

**Description**

Cannot be called within an interrupt, but can be interrupted.

**Return**

a pointer to the cache on success, NULL on failure. When
CONFIG\_SLAB\_BUCKETS is not enabled, ZERO\_SIZE\_PTR is returned, and
subsequent calls to `kmem_buckets_alloc()` will fall back to [`kmalloc()`](#c.kmalloc "kmalloc").
(i.e. callers only need to check for NULL on failure.)

int kmem\_cache\_shrink(struct kmem\_cache \*cachep)
:   Shrink a cache.

**Parameters**

`struct kmem_cache *cachep`
:   The cache to shrink.

**Description**

Releases as many slabs as possible for a cache.
To help debugging, a zero exit status indicates all slabs were released.

**Return**

`0` if all slabs were released, non-zero otherwise

bool kmem\_dump\_obj(void \*object)
:   Print available slab provenance information

**Parameters**

`void *object`
:   slab object for which to find provenance information.

**Description**

This function uses [`pr_cont()`](printk-basics.html#c.pr_cont "pr_cont"), so that the caller is expected to have
printed out whatever preamble is appropriate. The provenance information
depends on the type of object and on how much debugging is enabled.
For a slab-cache object, the fact that it is a slab object is printed,
and, if available, the slab name, return address, and stack trace from
the allocation and last free path of that object.

**Return**

`true` if the pointer is to a not-yet-freed object from
[`kmalloc()`](#c.kmalloc "kmalloc") or [`kmem_cache_alloc()`](#c.kmem_cache_alloc "kmem_cache_alloc"), either `true` or `false` if the pointer
is to an already-freed object, and `false` otherwise.

void kfree\_sensitive(const void \*p)
:   Clear sensitive information in memory before freeing

**Parameters**

`const void *p`
:   object to free memory of

**Description**

The memory of the object **p** points to is zeroed before freed.
If **p** is `NULL`, [`kfree_sensitive()`](#c.kfree_sensitive "kfree_sensitive") does nothing.

**Note**

this function zeroes the whole allocated buffer which can be a good
deal bigger than the requested buffer size passed to [`kmalloc()`](#c.kmalloc "kmalloc"). So be
careful when using this function in performance sensitive code.

void kvfree\_rcu\_barrier(void)
:   Wait until all in-flight `kvfree_rcu()` complete.

**Parameters**

`void`
:   no arguments

**Description**

Note that a single argument of `kvfree_rcu()` call has a slow path that
triggers [`synchronize_rcu()`](kernel-api.html#c.synchronize_rcu "synchronize_rcu") following by freeing a pointer. It is done
before the return from the function. Therefore for any single-argument
call that will result in a [`kfree()`](#c.kfree "kfree") to a cache that is to be destroyed
during module exit, it is developer’s responsibility to ensure that all
such calls have returned before the call to `kmem_cache_destroy()`.

void kvfree\_rcu\_barrier\_on\_cache(struct kmem\_cache \*s)
:   Wait for in-flight `kvfree_rcu()` calls on a specific slab cache.

**Parameters**

`struct kmem_cache *s`
:   slab cache to wait for

**Description**

See the description of [`kvfree_rcu_barrier()`](#c.kvfree_rcu_barrier "kvfree_rcu_barrier") for details.

void kfree\_const(const void \*x)
:   conditionally free memory

**Parameters**

`const void *x`
:   pointer to the memory

**Description**

Function calls kfree only if **x** is not in .rodata section.

## Virtually Contiguous Mappings

void vm\_unmap\_aliases(void)
:   unmap outstanding lazy aliases in the vmap layer

**Parameters**

`void`
:   no arguments

**Description**

The vmap/vmalloc layer lazily flushes kernel virtual mappings primarily
to amortize TLB flushing overheads. What this means is that any page you
have now, may, in a former life, have been mapped into kernel virtual
address by the vmap layer and so there might be some CPUs with TLB entries
still referencing that page (additional to the regular 1:1 kernel mapping).

vm\_unmap\_aliases flushes all such lazy mappings. After it returns, we can
be sure that none of the pages we have control over will have any aliases
from the vmap layer.

void vm\_unmap\_ram(const void \*mem, unsigned int count)
:   unmap linear kernel address space set up by vm\_map\_ram

**Parameters**

`const void *mem`
:   the pointer returned by vm\_map\_ram

`unsigned int count`
:   the count passed to that vm\_map\_ram call (cannot unmap partial)

void \*vm\_map\_ram(struct page \*\*pages, unsigned int count, int node)
:   map pages linearly into kernel virtual address (vmalloc space)

**Parameters**

`struct page **pages`
:   an array of pointers to the pages to be mapped

`unsigned int count`
:   number of pages

`int node`
:   prefer to allocate data structures on this node

**Description**

If you use this function for less than VMAP\_MAX\_ALLOC pages, it could be
faster than vmap so it’s good. But if you mix long-life and short-life
objects with [`vm_map_ram()`](#c.vm_map_ram "vm_map_ram"), it could consume lots of address space through
fragmentation (especially on a 32bit machine). You could see failures in
the end. Please use this function for short-lived objects.

**Return**

a pointer to the address that has been mapped, or `NULL` on failure

void vfree(const void \*addr)
:   Release memory allocated by [`vmalloc()`](#c.vmalloc "vmalloc")

**Parameters**

`const void *addr`
:   Memory base address

**Description**

Free the virtually continuous memory area starting at **addr**, as obtained
from one of the [`vmalloc()`](#c.vmalloc "vmalloc") family of APIs. This will usually also free the
physical memory underlying the virtual allocation, but that memory is
reference counted, so it will not be freed until the last user goes away.

If **addr** is NULL, no operation is performed.

**Context**

May sleep if called *not* from interrupt context.
Must not be called in NMI context (strictly speaking, it could be
if we have CONFIG\_ARCH\_HAVE\_NMI\_SAFE\_CMPXCHG, but making the calling
conventions for [`vfree()`](#c.vfree "vfree") arch-dependent would be a really bad idea).

void vunmap(const void \*addr)
:   release virtual mapping obtained by [`vmap()`](#c.vmap "vmap")

**Parameters**

`const void *addr`
:   memory base address

**Description**

Free the virtually contiguous memory area starting at **addr**,
which was created from the page array passed to [`vmap()`](#c.vmap "vmap").

Must not be called in interrupt context.

void \*vmap(struct page \*\*pages, unsigned int count, unsigned long flags, pgprot\_t prot)
:   map an array of pages into virtually contiguous space

**Parameters**

`struct page **pages`
:   array of page pointers

`unsigned int count`
:   number of pages to map

`unsigned long flags`
:   vm\_area->flags

`pgprot_t prot`
:   page protection for the mapping

**Description**

Maps **count** pages from **pages** into contiguous kernel virtual space.
If **flags** contains `VM_MAP_PUT_PAGES` the ownership of the pages array itself
(which must be kmalloc or vmalloc memory) and one reference per pages in it
are transferred from the caller to [`vmap()`](#c.vmap "vmap"), and will be freed / dropped when
[`vfree()`](#c.vfree "vfree") is called on the return value.

**Return**

the address of the area or `NULL` on failure

void \*vmap\_pfn(unsigned long \*pfns, unsigned int count, pgprot\_t prot)
:   map an array of PFNs into virtually contiguous space

**Parameters**

`unsigned long *pfns`
:   array of PFNs

`unsigned int count`
:   number of pages to map

`pgprot_t prot`
:   page protection for the mapping

**Description**

Maps **count** PFNs from **pfns** into contiguous kernel virtual space and returns
the start address of the mapping.

void \*\_\_vmalloc\_node(unsigned long size, unsigned long align, gfp\_t gfp\_mask, int node, const void \*caller)
:   allocate virtually contiguous memory

**Parameters**

`unsigned long size`
:   allocation size

`unsigned long align`
:   desired alignment

`gfp_t gfp_mask`
:   flags for the page level allocator

`int node`
:   node to use for allocation or NUMA\_NO\_NODE

`const void *caller`
:   caller’s return address

**Description**

Allocate enough pages to cover **size** from the page level allocator with
**gfp\_mask** flags. Map them into contiguous kernel virtual space.

Semantics of **gfp\_mask** (including reclaim/retry modifiers such as
\_\_GFP\_NOFAIL) are the same as in `__vmalloc_node_range_noprof()`.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vmalloc(unsigned long size)
:   allocate virtually contiguous memory

**Parameters**

`unsigned long size`
:   allocation size

**Description**

Allocate enough pages to cover **size** from the page level
allocator and map them into contiguous kernel virtual space.

For tight control over page level allocator and protection flags
use `__vmalloc()` instead.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vmalloc\_huge\_node(unsigned long size, gfp\_t gfp\_mask, int node)
:   allocate virtually contiguous memory, allow huge pages

**Parameters**

`unsigned long size`
:   allocation size

`gfp_t gfp_mask`
:   flags for the page level allocator

`int node`
:   node to use for allocation or NUMA\_NO\_NODE

**Description**

Allocate enough pages to cover **size** from the page level
allocator and map them into contiguous kernel virtual space.
If **size** is greater than or equal to PMD\_SIZE, allow using
huge pages for the memory

**Return**

pointer to the allocated memory or `NULL` on error

void \*vzalloc(unsigned long size)
:   allocate virtually contiguous memory with zero fill

**Parameters**

`unsigned long size`
:   allocation size

**Description**

Allocate enough pages to cover **size** from the page level
allocator and map them into contiguous kernel virtual space.
The memory allocated is set to zero.

For tight control over page level allocator and protection flags
use `__vmalloc()` instead.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vmalloc\_user(unsigned long size)
:   allocate zeroed virtually contiguous memory for userspace

**Parameters**

`unsigned long size`
:   allocation size

**Description**

The resulting memory area is zeroed so it can be mapped to userspace
without leaking data.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vmalloc\_node(unsigned long size, int node)
:   allocate memory on a specific node

**Parameters**

`unsigned long size`
:   allocation size

`int node`
:   numa node

**Description**

Allocate enough pages to cover **size** from the page level
allocator and map them into contiguous kernel virtual space.

For tight control over page level allocator and protection flags
use `__vmalloc()` instead.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vzalloc\_node(unsigned long size, int node)
:   allocate memory on a specific node with zero fill

**Parameters**

`unsigned long size`
:   allocation size

`int node`
:   numa node

**Description**

Allocate enough pages to cover **size** from the page level
allocator and map them into contiguous kernel virtual space.
The memory allocated is set to zero.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vrealloc\_node\_align(const void \*p, size\_t size, unsigned long align, gfp\_t flags, int nid)
:   reallocate virtually contiguous memory; contents remain unchanged

**Parameters**

`const void *p`
:   object to reallocate memory for

`size_t size`
:   the size to reallocate

`unsigned long align`
:   requested alignment

`gfp_t flags`
:   the flags for the page level allocator

`int nid`
:   node number of the target node

**Description**

If **p** is `NULL`, `vrealloc_XXX()` behaves exactly like `vmalloc_XXX()`. If **size**
is 0 and **p** is not a `NULL` pointer, the object pointed to is freed.

If the caller wants the new memory to be on specific node *only*,
\_\_GFP\_THISNODE flag should be set, otherwise the function will try to avoid
reallocation and possibly disregard the specified **nid**.

If \_\_GFP\_ZERO logic is requested, callers must ensure that, starting with the
initial memory allocation, every subsequent call to this API for the same
memory allocation is flagged with \_\_GFP\_ZERO. Otherwise, it is possible that
\_\_GFP\_ZERO is not fully honored by this API.

Requesting an alignment that is bigger than the alignment of the existing
allocation will fail.

In any case, the contents of the object pointed to are preserved up to the
lesser of the new and old sizes.

This function must not be called concurrently with itself or [`vfree()`](#c.vfree "vfree") for the
same memory allocation.

**Return**

pointer to the allocated memory; `NULL` if **size** is zero or in case of
failure

void \*vmalloc\_32(unsigned long size)
:   allocate virtually contiguous memory (32bit addressable)

**Parameters**

`unsigned long size`
:   allocation size

**Description**

Allocate enough 32bit PA addressable pages to cover **size** from the
page level allocator and map them into contiguous kernel virtual space.

**Return**

pointer to the allocated memory or `NULL` on error

void \*vmalloc\_32\_user(unsigned long size)
:   allocate zeroed virtually contiguous 32bit memory

**Parameters**

`unsigned long size`
:   allocation size

**Description**

The resulting memory area is 32bit addressable and zeroed so it can be
mapped to userspace without leaking data.

**Return**

pointer to the allocated memory or `NULL` on error

int remap\_vmalloc\_range(struct vm\_area\_struct \*vma, void \*addr, unsigned long pgoff)
:   map vmalloc pages to userspace

**Parameters**

`struct vm_area_struct *vma`
:   vma to cover (map full range of vma)

`void *addr`
:   vmalloc memory

`unsigned long pgoff`
:   number of pages into addr before first page to map

**Return**

0 for success, -Exxx on failure

**Description**

This function checks that addr is a valid vmalloc’ed area, and
that it is big enough to cover the vma. Will return failure if
that criteria isn’t met.

Similar to [`remap_pfn_range()`](#c.remap_pfn_range "remap_pfn_range") (see mm/memory.c)

## File Mapping and Page Cache

### Filemap

int filemap\_fdatawrite\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t start, loff\_t end)
:   start writeback on mapping dirty pages in range

**Parameters**

`struct address_space *mapping`
:   address space structure to write

`loff_t start`
:   offset in bytes where the range starts

`loff_t end`
:   offset in bytes where the range ends (inclusive)

**Description**

Start writeback against all of a mapping’s dirty pages that lie
within the byte offsets <start, end> inclusive.

This is a data integrity operation that waits upon dirty or in writeback
pages.

**Return**

`0` on success, negative error code otherwise.

int filemap\_flush\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t start, loff\_t end)
:   start writeback on a range

**Parameters**

`struct address_space *mapping`
:   target address\_space

`loff_t start`
:   index to start writeback on

`loff_t end`
:   last (inclusive) index for writeback

**Description**

This is a non-integrity writeback helper, to start writing back folios
for the indicated range.

**Return**

`0` on success, negative error code otherwise.

int filemap\_flush(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   mostly a non-blocking flush

**Parameters**

`struct address_space *mapping`
:   target address\_space

**Description**

This is a mostly non-blocking flush. Not suitable for data-integrity
purposes - I/O may not be started against all dirty pages.

**Return**

`0` on success, negative error code otherwise.

bool filemap\_range\_has\_page(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t start\_byte, loff\_t end\_byte)
:   check if a page exists in range.

**Parameters**

`struct address_space *mapping`
:   address space within which to check

`loff_t start_byte`
:   offset in bytes where the range starts

`loff_t end_byte`
:   offset in bytes where the range ends (inclusive)

**Description**

Find at least one page in the range supplied, usually used to check if
direct writing in this range will trigger a writeback.

**Return**

`true` if at least one page exists in the specified range,
`false` otherwise.

int filemap\_fdatawait\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t start\_byte, loff\_t end\_byte)
:   wait for writeback to complete

**Parameters**

`struct address_space *mapping`
:   address space structure to wait for

`loff_t start_byte`
:   offset in bytes where the range starts

`loff_t end_byte`
:   offset in bytes where the range ends (inclusive)

**Description**

Walk the list of under-writeback pages of the given address space
in the given range and wait for all of them. Check error status of
the address space and return it.

Since the error status of the address space is cleared by this function,
callers are responsible for checking the return value and handling and/or
reporting the error.

**Return**

error status of the address space.

int filemap\_fdatawait\_range\_keep\_errors(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t start\_byte, loff\_t end\_byte)
:   wait for writeback to complete

**Parameters**

`struct address_space *mapping`
:   address space structure to wait for

`loff_t start_byte`
:   offset in bytes where the range starts

`loff_t end_byte`
:   offset in bytes where the range ends (inclusive)

**Description**

Walk the list of under-writeback pages of the given address space in the
given range and wait for all of them. Unlike [`filemap_fdatawait_range()`](#c.filemap_fdatawait_range "filemap_fdatawait_range"),
this function does not clear error status of the address space.

Use this function if callers don’t handle errors themselves. Expected
call sites are system-wide / filesystem-wide data flushers: e.g. sync(2),
fsfreeze(8)

int file\_fdatawait\_range(struct [file](#c.file_fdatawait_range "file") \*file, loff\_t start\_byte, loff\_t end\_byte)
:   wait for writeback to complete

**Parameters**

`struct file *file`
:   file pointing to address space structure to wait for

`loff_t start_byte`
:   offset in bytes where the range starts

`loff_t end_byte`
:   offset in bytes where the range ends (inclusive)

**Description**

Walk the list of under-writeback pages of the address space that file
refers to, in the given range and wait for all of them. Check error
status of the address space vs. the file->f\_wb\_err cursor and return it.

Since the error status of the file is advanced by this function,
callers are responsible for checking the return value and handling and/or
reporting the error.

**Return**

error status of the address space vs. the file->f\_wb\_err cursor.

int filemap\_fdatawait\_keep\_errors(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   wait for writeback without clearing errors

**Parameters**

`struct address_space *mapping`
:   address space structure to wait for

**Description**

Walk the list of under-writeback pages of the given address space
and wait for all of them. Unlike `filemap_fdatawait()`, this function
does not clear error status of the address space.

Use this function if callers don’t handle errors themselves. Expected
call sites are system-wide / filesystem-wide data flushers: e.g. sync(2),
fsfreeze(8)

**Return**

error status of the address space.

int filemap\_write\_and\_wait\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t lstart, loff\_t lend)
:   write out & wait on a file range

**Parameters**

`struct address_space *mapping`
:   the address\_space for the pages

`loff_t lstart`
:   offset in bytes where the range starts

`loff_t lend`
:   offset in bytes where the range ends (inclusive)

**Description**

Write out and wait upon file offsets lstart->lend, inclusive.

Note that **lend** is inclusive (describes the last byte to be written) so
that this function can be used to write to the very end-of-file (end = -1).

**Return**

error status of the address space.

int file\_check\_and\_advance\_wb\_err(struct [file](#c.file_check_and_advance_wb_err "file") \*file)
:   report wb error (if any) that was previously and advance wb\_err to current one

**Parameters**

`struct file *file`
:   [`struct file`](../filesystems/api-summary.html#c.file "file") on which the error is being reported

**Description**

When userland calls fsync (or something like nfsd does the equivalent), we
want to report any writeback errors that occurred since the last fsync (or
since the file was opened if there haven’t been any).

Grab the wb\_err from the mapping. If it matches what we have in the file,
then just quickly return 0. The file is all caught up.

If it doesn’t match, then take the mapping value, set the “seen” flag in
it and try to swap it into place. If it works, or another task beat us
to it with the new value, then update the f\_wb\_err and return the error
portion. The error at this point must be reported via proper channels
(a’la fsync, or NFS COMMIT operation, etc.).

While we handle mapping->wb\_err with atomic operations, the f\_wb\_err
value is protected by the f\_lock since we must ensure that it reflects
the latest value swapped in for this file descriptor.

**Return**

`0` on success, negative error code otherwise.

int file\_write\_and\_wait\_range(struct [file](#c.file_write_and_wait_range "file") \*file, loff\_t lstart, loff\_t lend)
:   write out & wait on a file range

**Parameters**

`struct file *file`
:   file pointing to address\_space with pages

`loff_t lstart`
:   offset in bytes where the range starts

`loff_t lend`
:   offset in bytes where the range ends (inclusive)

**Description**

Write out and wait upon file offsets lstart->lend, inclusive.

Note that **lend** is inclusive (describes the last byte to be written) so
that this function can be used to write to the very end-of-file (end = -1).

After writing out and waiting on the data, we check and advance the
f\_wb\_err cursor to the latest value, and return any errors detected there.

**Return**

`0` on success, negative error code otherwise.

void replace\_page\_cache\_folio(struct [folio](#c.folio "folio") \*old, struct [folio](#c.folio "folio") \*new)
:   replace a pagecache folio with a new one

**Parameters**

`struct folio *old`
:   folio to be replaced

`struct folio *new`
:   folio to replace with

**Description**

This function replaces a folio in the pagecache with a new one. On
success it acquires the pagecache reference for the new folio and
drops it for the old folio. Both the old and new folios must be
locked. This function does not add the new folio to the LRU, the
caller must do that.

The remove + add is atomic. This function cannot fail.

void folio\_unlock(struct [folio](#c.folio_unlock "folio") \*folio)
:   Unlock a locked folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

Unlocks the folio and wakes up any thread sleeping on the page lock.

**Context**

May be called from interrupt or process context. May not be
called from NMI context.

void folio\_end\_read(struct [folio](#c.folio_end_read "folio") \*folio, bool success)
:   End read on a folio.

**Parameters**

`struct folio *folio`
:   The folio.

`bool success`
:   True if all reads completed successfully.

**Description**

When all reads against a folio have completed, filesystems should
call this function to let the pagecache know that no more reads
are outstanding. This will unlock the folio and wake up any thread
sleeping on the lock. The folio will also be marked uptodate if all
reads succeeded.

**Context**

May be called from interrupt or process context. May not be
called from NMI context.

void folio\_end\_private\_2(struct [folio](#c.folio_end_private_2 "folio") \*folio)
:   Clear PG\_private\_2 and wake any waiters.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

Clear the PG\_private\_2 bit on a folio and wake up any sleepers waiting for
it. The folio reference held for PG\_private\_2 being set is released.

This is, for example, used when a netfs folio is being written to a local
disk cache, thereby allowing writes to the cache for the same folio to be
serialised.

void folio\_wait\_private\_2(struct [folio](#c.folio_wait_private_2 "folio") \*folio)
:   Wait for PG\_private\_2 to be cleared on a folio.

**Parameters**

`struct folio *folio`
:   The folio to wait on.

**Description**

Wait for PG\_private\_2 to be cleared on a folio.

int folio\_wait\_private\_2\_killable(struct [folio](#c.folio_wait_private_2_killable "folio") \*folio)
:   Wait for PG\_private\_2 to be cleared on a folio.

**Parameters**

`struct folio *folio`
:   The folio to wait on.

**Description**

Wait for PG\_private\_2 to be cleared on a folio or until a fatal signal is
received by the calling task.

**Return**

* 0 if successful.
* -EINTR if a fatal signal was encountered.

void folio\_end\_writeback\_no\_dropbehind(struct [folio](#c.folio_end_writeback_no_dropbehind "folio") \*folio)
:   End writeback against a folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

The folio must actually be under writeback.
This call is intended for filesystems that need to defer dropbehind.

**Context**

May be called from process or interrupt context.

void folio\_end\_writeback(struct [folio](#c.folio_end_writeback "folio") \*folio)
:   End writeback against a folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

The folio must actually be under writeback.

**Context**

May be called from process or interrupt context.

void \_\_folio\_lock(struct [folio](#c.__folio_lock "folio") \*folio)
:   Get a lock on the folio, assuming we need to sleep to get it.

**Parameters**

`struct folio *folio`
:   The folio to lock

pgoff\_t page\_cache\_next\_miss(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, unsigned long max\_scan)
:   Find the next gap in the page cache.

**Parameters**

`struct address_space *mapping`
:   Mapping.

`pgoff_t index`
:   Index.

`unsigned long max_scan`
:   Maximum range to search.

**Description**

Search the range [index, min(index + max\_scan - 1, ULONG\_MAX)] for the
gap with the lowest index.

This function may be called under the rcu\_read\_lock. However, this will
not atomically search a snapshot of the cache at a single point in time.
For example, if a gap is created at index 5, then subsequently a gap is
created at index 10, page\_cache\_next\_miss covering both indices may
return 10 if called under the rcu\_read\_lock.

**Return**

The index of the gap if found, otherwise an index outside the
range specified (in which case ‘return - index >= max\_scan’ will be true).
In the rare case of index wrap-around, 0 will be returned.

pgoff\_t page\_cache\_prev\_miss(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, unsigned long max\_scan)
:   Find the previous gap in the page cache.

**Parameters**

`struct address_space *mapping`
:   Mapping.

`pgoff_t index`
:   Index.

`unsigned long max_scan`
:   Maximum range to search.

**Description**

Search the range [max(index - max\_scan + 1, 0), index] for the
gap with the highest index.

This function may be called under the rcu\_read\_lock. However, this will
not atomically search a snapshot of the cache at a single point in time.
For example, if a gap is created at index 10, then subsequently a gap is
created at index 5, [`page_cache_prev_miss()`](#c.page_cache_prev_miss "page_cache_prev_miss") covering both indices may
return 5 if called under the rcu\_read\_lock.

**Return**

The index of the gap if found, otherwise an index outside the
range specified (in which case ‘index - return >= max\_scan’ will be true).
In the rare case of wrap-around, ULONG\_MAX will be returned.

struct [folio](#c.folio "folio") \*\_\_filemap\_get\_folio\_mpol(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, [fgf\_t](#c.fgf_t "fgf_t") fgp\_flags, gfp\_t gfp, struct mempolicy \*policy)
:   Find and get a reference to a folio.

**Parameters**

`struct address_space *mapping`
:   The address\_space to search.

`pgoff_t index`
:   The page index.

`fgf_t fgp_flags`
:   `FGP` flags modify how the folio is returned.

`gfp_t gfp`
:   Memory allocation flags to use if `FGP_CREAT` is specified.

`struct mempolicy *policy`
:   NUMA memory allocation policy to follow.

**Description**

Looks up the page cache entry at **mapping** & **index**.

If `FGP_LOCK` or `FGP_CREAT` are specified then the function may sleep even
if the `GFP` flags specified for `FGP_CREAT` are atomic.

If this function returns a folio, it is returned with an increased refcount.

**Return**

The found folio or an [`ERR_PTR()`](kernel-api.html#c.ERR_PTR "ERR_PTR") otherwise.

unsigned filemap\_get\_folios(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t \*start, pgoff\_t end, struct folio\_batch \*fbatch)
:   Get a batch of folios

**Parameters**

`struct address_space *mapping`
:   The address\_space to search

`pgoff_t *start`
:   The starting page index

`pgoff_t end`
:   The final page index (inclusive)

`struct folio_batch *fbatch`
:   The batch to fill.

**Description**

Search for and return a batch of folios in the mapping starting at
index **start** and up to index **end** (inclusive). The folios are returned
in **fbatch** with an elevated reference count.

**Return**

The number of folios which were found.
We also update **start** to index the next folio for the traversal.

unsigned filemap\_get\_folios\_contig(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t \*start, pgoff\_t end, struct folio\_batch \*fbatch)
:   Get a batch of contiguous folios

**Parameters**

`struct address_space *mapping`
:   The address\_space to search

`pgoff_t *start`
:   The starting page index

`pgoff_t end`
:   The final page index (inclusive)

`struct folio_batch *fbatch`
:   The batch to fill

**Description**

[`filemap_get_folios_contig()`](#c.filemap_get_folios_contig "filemap_get_folios_contig") works exactly like [`filemap_get_folios()`](#c.filemap_get_folios "filemap_get_folios"),
except the returned folios are guaranteed to be contiguous. This may
not return all contiguous folios if the batch gets filled up.

**Return**

The number of folios found.
Also update **start** to be positioned for traversal of the next folio.

unsigned filemap\_get\_folios\_tag(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t \*start, pgoff\_t end, xa\_mark\_t tag, struct folio\_batch \*fbatch)
:   Get a batch of folios matching **tag**

**Parameters**

`struct address_space *mapping`
:   The address\_space to search

`pgoff_t *start`
:   The starting page index

`pgoff_t end`
:   The final page index (inclusive)

`xa_mark_t tag`
:   The tag index

`struct folio_batch *fbatch`
:   The batch to fill

**Description**

The first folio may start before **start**; if it does, it will contain
**start**. The final folio may extend beyond **end**; if it does, it will
contain **end**. The folios have ascending indices. There may be gaps
between the folios if there are indices which have no folio in the
page cache. If folios are added to or removed from the page cache
while this is running, they may or may not be found by this call.
Only returns folios that are tagged with **tag**.

**Return**

The number of folios found.
Also update **start** to index the next folio for traversal.

ssize\_t filemap\_read(struct kiocb \*iocb, struct iov\_iter \*iter, ssize\_t already\_read)
:   Read data from the page cache.

**Parameters**

`struct kiocb *iocb`
:   The iocb to read.

`struct iov_iter *iter`
:   Destination for the data.

`ssize_t already_read`
:   Number of bytes already read by the caller.

**Description**

Copies data from the page cache. If the data is not currently present,
uses the readahead and read\_folio address\_space operations to fetch it.

**Return**

Total number of bytes copied, including those already read by
the caller. If an error happens before any bytes are copied, returns
a negative error number.

ssize\_t generic\_file\_read\_iter(struct kiocb \*iocb, struct iov\_iter \*iter)
:   generic filesystem read routine

**Parameters**

`struct kiocb *iocb`
:   kernel I/O control block

`struct iov_iter *iter`
:   destination for the data read

**Description**

This is the “`read_iter()`” routine for all filesystems
that can use the page cache directly.

The IOCB\_NOWAIT flag in iocb->ki\_flags indicates that -EAGAIN shall
be returned when no data can be read without waiting for I/O requests
to complete; it doesn’t prevent readahead.

The IOCB\_NOIO flag in iocb->ki\_flags indicates that no new I/O
requests shall be made for the read or for readahead. When no data
can be read, -EAGAIN shall be returned. When readahead would be
triggered, a partial, possibly empty read shall be returned.

**Return**

* number of bytes copied, even for partial reads
* negative error code (or 0 if IOCB\_NOIO) if nothing was read

ssize\_t filemap\_splice\_read(struct [file](../filesystems/api-summary.html#c.file "file") \*in, loff\_t \*ppos, struct [pipe\_inode\_info](../filesystems/splice.html#c.pipe_inode_info "pipe_inode_info") \*pipe, size\_t len, unsigned int flags)
:   Splice data from a file’s pagecache into a pipe

**Parameters**

`struct file *in`
:   The file to read from

`loff_t *ppos`
:   Pointer to the file position to read from

`struct pipe_inode_info *pipe`
:   The pipe to splice into

`size_t len`
:   The amount to splice

`unsigned int flags`
:   The SPLICE\_F\_\* flags

**Description**

This function gets folios from a file’s pagecache and splices them into the
pipe. Readahead will be called as necessary to fill more folios. This may
be used for blockdevs also.

**Return**

On success, the number of bytes read will be returned and **\*ppos**
will be updated if appropriate; 0 will be returned if there is no more data
to be read; -EAGAIN will be returned if the pipe had no space, and some
other negative error code will be returned on error. A short read may occur
if the pipe has insufficient space, we reach the end of the data or we hit a
hole.

[vm\_fault\_t](#c.vm_fault_t "vm_fault_t") filemap\_fault(struct vm\_fault \*vmf)
:   read in file data for page fault handling

**Parameters**

`struct vm_fault *vmf`
:   `struct vm_fault` containing details of the fault

**Description**

[`filemap_fault()`](#c.filemap_fault "filemap_fault") is invoked via the vma operations vector for a
mapped memory region to read in file data during a page fault.

The goto’s are kind of ugly, but this streamlines the normal case of having
it in the page cache, and handles the special cases reasonably without
having a lot of duplicated code.

vma->vm\_mm->mmap\_lock must be held on entry.

If our return value has VM\_FAULT\_RETRY set, it’s because the mmap\_lock
may be dropped before doing I/O or by `lock_folio_maybe_drop_mmap()`.

If our return value does not have VM\_FAULT\_RETRY set, the mmap\_lock
has not been released.

We never return with VM\_FAULT\_RETRY and a bit from VM\_FAULT\_ERROR set.

**Return**

bitwise-OR of `VM_FAULT_` codes.

struct [folio](#c.folio "folio") \*read\_cache\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, filler\_t filler, struct [file](#c.read_cache_folio "file") \*file)
:   Read into page cache, fill it if needed.

**Parameters**

`struct address_space *mapping`
:   The address\_space to read from.

`pgoff_t index`
:   The index to read.

`filler_t filler`
:   Function to perform the read, or NULL to use aops->`read_folio()`.

`struct file *file`
:   Passed to filler function, may be NULL if not required.

**Description**

Read one page into the page cache. If it succeeds, the folio returned
will contain **index**, but it may not be the first page of the folio.

If the filler function returns an error, it will be returned to the
caller.

**Context**

May sleep. Expects mapping->invalidate\_lock to be held.

**Return**

An uptodate folio on success, [`ERR_PTR()`](kernel-api.html#c.ERR_PTR "ERR_PTR") on failure.

struct [folio](#c.folio "folio") \*mapping\_read\_folio\_gfp(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, gfp\_t gfp)
:   Read into page cache, using specified allocation flags.

**Parameters**

`struct address_space *mapping`
:   The address\_space for the folio.

`pgoff_t index`
:   The index that the allocated folio will contain.

`gfp_t gfp`
:   The page allocator flags to use if allocating.

**Description**

This is the same as “read\_cache\_folio(mapping, index, NULL, NULL)”, but with
any new memory allocations done using the specified allocation flags.

The most likely error from this function is EIO, but ENOMEM is
possible and so is EINTR. If ->read\_folio returns another error,
that will be returned to the caller.

The function expects mapping->invalidate\_lock to be already held.

**Return**

Uptodate folio on success, [`ERR_PTR()`](kernel-api.html#c.ERR_PTR "ERR_PTR") on failure.

struct page \*read\_cache\_page\_gfp(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, gfp\_t gfp)
:   read into page cache, using specified page allocation flags.

**Parameters**

`struct address_space *mapping`
:   the page’s address\_space

`pgoff_t index`
:   the page index

`gfp_t gfp`
:   the page allocator flags to use if allocating

**Description**

This is the same as “read\_mapping\_page(mapping, index, NULL)”, but with
any new page allocations done using the specified allocation flags.

If the page does not get brought uptodate, return -EIO.

The function expects mapping->invalidate\_lock to be already held.

**Return**

up to date page on success, [`ERR_PTR()`](kernel-api.html#c.ERR_PTR "ERR_PTR") on failure.

ssize\_t \_\_generic\_file\_write\_iter(struct kiocb \*iocb, struct iov\_iter \*from)
:   write data to a file

**Parameters**

`struct kiocb *iocb`
:   IO state structure (file, offset, etc.)

`struct iov_iter *from`
:   iov\_iter with data to write

**Description**

This function does all the work needed for actually writing data to a
file. It does all basic checks, removes SUID from the file, updates
modification times and calls proper subroutines depending on whether we
do direct IO or a standard buffered write.

It expects i\_rwsem to be grabbed unless we work on a block device or similar
object which does not need locking at all.

This function does *not* take care of syncing data in case of O\_SYNC write.
A caller has to handle it. This is mainly due to the fact that we want to
avoid syncing under i\_rwsem.

**Return**

* number of bytes written, even for truncated writes
* negative error code if no data has been written at all

ssize\_t generic\_file\_write\_iter(struct kiocb \*iocb, struct iov\_iter \*from)
:   write data to a file

**Parameters**

`struct kiocb *iocb`
:   IO state structure

`struct iov_iter *from`
:   iov\_iter with data to write

**Description**

This is a wrapper around [`__generic_file_write_iter()`](#c.__generic_file_write_iter "__generic_file_write_iter") to be used by most
filesystems. It takes care of syncing the file in case of O\_SYNC file
and acquires i\_rwsem as needed.

**Return**

* negative error code if no data has been written at all of
  [`vfs_fsync_range()`](../filesystems/api-summary.html#c.vfs_fsync_range "vfs_fsync_range") failed for a synchronous write
* number of bytes written, even for truncated writes

bool filemap\_release\_folio(struct [folio](#c.filemap_release_folio "folio") \*folio, gfp\_t gfp)
:   Release fs-specific metadata on a folio.

**Parameters**

`struct folio *folio`
:   The folio which the kernel is trying to free.

`gfp_t gfp`
:   Memory allocation flags (and I/O mode).

**Description**

The address\_space is trying to release any data attached to a folio
(presumably at folio->private).

This will also be called if the private\_2 flag is set on a page,
indicating that the folio has other metadata associated with it.

The **gfp** argument specifies whether I/O may be performed to release
this page (\_\_GFP\_IO), and whether the call may block
(\_\_GFP\_RECLAIM & \_\_GFP\_FS).

**Return**

`true` if the release was successful, otherwise `false`.

int filemap\_invalidate\_inode(struct [inode](#c.filemap_invalidate_inode "inode") \*inode, bool flush, loff\_t start, loff\_t end)
:   Invalidate/forcibly write back a range of an inode’s pagecache

**Parameters**

`struct inode *inode`
:   The inode to flush

`bool flush`
:   Set to write back rather than simply invalidate.

`loff_t start`
:   First byte to in range.

`loff_t end`
:   Last byte in range (inclusive), or LLONG\_MAX for everything from start
    onwards.

**Description**

Invalidate all the folios on an inode that contribute to the specified
range, possibly writing them back first. Whilst the operation is
undertaken, the invalidate lock is held to prevent new folios from being
installed.

### Readahead

Readahead is used to read content into the page cache before it is
explicitly requested by the application. Readahead only ever
attempts to read folios that are not yet in the page cache. If a
folio is present but not up-to-date, readahead will not try to read
it. In that case a simple ->`read_folio()` will be requested.

Readahead is triggered when an application read request (whether a
system call or a page fault) finds that the requested folio is not in
the page cache, or that it is in the page cache and has the
readahead flag set. This flag indicates that the folio was read
as part of a previous readahead request and now that it has been
accessed, it is time for the next readahead.

Each readahead request is partly synchronous read, and partly async
readahead. This is reflected in the [`struct file_ra_state`](../filesystems/api-summary.html#c.file_ra_state "file_ra_state") which
contains ->size being the total number of pages, and ->async\_size
which is the number of pages in the async section. The readahead
flag will be set on the first folio in this async section to trigger
a subsequent readahead. Once a series of sequential reads has been
established, there should be no need for a synchronous component and
all readahead request will be fully asynchronous.

When either of the triggers causes a readahead, three numbers need
to be determined: the start of the region to read, the size of the
region, and the size of the async tail.

The start of the region is simply the first page address at or after
the accessed address, which is not currently populated in the page
cache. This is found with a simple search in the page cache.

The size of the async tail is determined by subtracting the size that
was explicitly requested from the determined request size, unless
this would be less than zero - then zero is used. NOTE THIS
CALCULATION IS WRONG WHEN THE START OF THE REGION IS NOT THE ACCESSED
PAGE. ALSO THIS CALCULATION IS NOT USED CONSISTENTLY.

The size of the region is normally determined from the size of the
previous readahead which loaded the preceding pages. This may be
discovered from the [`struct file_ra_state`](../filesystems/api-summary.html#c.file_ra_state "file_ra_state") for simple sequential reads,
or from examining the state of the page cache when multiple
sequential reads are interleaved. Specifically: where the readahead
was triggered by the readahead flag, the size of the previous
readahead is assumed to be the number of pages from the triggering
page to the start of the new readahead. In these cases, the size of
the previous readahead is scaled, often doubled, for the new
readahead, though see `get_next_ra_size()` for details.

If the size of the previous read cannot be determined, the number of
preceding pages in the page cache is used to estimate the size of
a previous read. This estimate could easily be misled by random
reads being coincidentally adjacent, so it is ignored unless it is
larger than the current request, and it is not scaled up, unless it
is at the start of file.

In general readahead is accelerated at the start of the file, as
reads from there are often sequential. There are other minor
adjustments to the readahead size in various special cases and these
are best discovered by reading the code.

The above calculation, based on the previous readahead size,
determines the size of the readahead, to which any requested read
size may be added.

Readahead requests are sent to the filesystem using the ->`readahead()`
address space operation, for which [`mpage_readahead()`](../filesystems/api-summary.html#c.mpage_readahead "mpage_readahead") is a canonical
implementation. ->`readahead()` should normally initiate reads on all
folios, but may fail to read any or all folios without causing an I/O
error. The page cache reading code will issue a ->`read_folio()` request
for any folio which ->`readahead()` did not read, and only an error
from this will be final.

->`readahead()` will generally call [`readahead_folio()`](#c.readahead_folio "readahead_folio") repeatedly to get
each folio from those prepared for readahead. It may fail to read a
folio by:

* not calling [`readahead_folio()`](#c.readahead_folio "readahead_folio") sufficiently many times, effectively
  ignoring some folios, as might be appropriate if the path to
  storage is congested.
* failing to actually submit a read request for a given folio,
  possibly due to insufficient resources, or
* getting an error during subsequent processing of a request.

In the last two cases, the folio should be unlocked by the filesystem
to indicate that the read attempt has failed. In the first case the
folio will be unlocked by the VFS.

Those folios not in the final `async_size` of the request should be
considered to be important and ->`readahead()` should not fail them due
to congestion or temporary resource unavailability, but should wait
for necessary resources (e.g. memory or indexing information) to
become available. Folios in the final `async_size` may be
considered less urgent and failure to read them is more acceptable.
In this case it is best to use `filemap_remove_folio()` to remove the
folios from the page cache as is automatically done for folios that
were not fetched with [`readahead_folio()`](#c.readahead_folio "readahead_folio"). This will allow a
subsequent synchronous readahead request to try them again. If they
are left in the page cache, then they will be read individually using
->`read_folio()` which may be less efficient.

void page\_cache\_ra\_unbounded(struct [readahead\_control](#c.readahead_control "readahead_control") \*ractl, unsigned long nr\_to\_read, unsigned long lookahead\_size)
:   Start unchecked readahead.

**Parameters**

`struct readahead_control *ractl`
:   Readahead control.

`unsigned long nr_to_read`
:   The number of pages to read.

`unsigned long lookahead_size`
:   Where to start the next readahead.

**Description**

This function is for filesystems to call when they want to start
readahead beyond a file’s stated i\_size. This is almost certainly
not the function you want to call. Use [`page_cache_async_readahead()`](#c.page_cache_async_readahead "page_cache_async_readahead")
or [`page_cache_sync_readahead()`](#c.page_cache_sync_readahead "page_cache_sync_readahead") instead.

**Context**

File is referenced by caller, and ractl->mapping->invalidate\_lock
must be held by the caller at least in shared mode. Mutexes may be held by
caller. May sleep, but will not reenter filesystem to reclaim memory.

void readahead\_expand(struct [readahead\_control](#c.readahead_control "readahead_control") \*ractl, loff\_t new\_start, size\_t new\_len)
:   Expand a readahead request

**Parameters**

`struct readahead_control *ractl`
:   The request to be expanded

`loff_t new_start`
:   The revised start

`size_t new_len`
:   The revised size of the request

**Description**

Attempt to expand a readahead request outwards from the current size to the
specified size by inserting locked pages before and after the current window
to increase the size to the new window. This may involve the insertion of
THPs, in which case the window may get expanded even beyond what was
requested.

The algorithm will stop if it encounters a conflicting page already in the
pagecache and leave a smaller expansion than requested.

The caller must check for this by examining the revised **ractl** object for a
different expansion than was requested.

### Writeback

int balance\_dirty\_pages\_ratelimited\_flags(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, unsigned int flags)
:   Balance dirty memory state.

**Parameters**

`struct address_space *mapping`
:   address\_space which was dirtied.

`unsigned int flags`
:   BDP flags.

**Description**

Processes which are dirtying memory should call in here once for each page
which was newly dirtied. The function will periodically check the system’s
dirty state and will initiate writeback if needed.

See [`balance_dirty_pages_ratelimited()`](#c.balance_dirty_pages_ratelimited "balance_dirty_pages_ratelimited") for details.

**Return**

If **flags** contains BDP\_ASYNC, it may return -EAGAIN to
indicate that memory is out of balance and the caller must wait
for I/O to complete. Otherwise, it will return 0 to indicate
that either memory was already in balance, or it was able to sleep
until the amount of dirty memory returned to balance.

void balance\_dirty\_pages\_ratelimited(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   balance dirty memory state.

**Parameters**

`struct address_space *mapping`
:   address\_space which was dirtied.

**Description**

Processes which are dirtying memory should call in here once for each page
which was newly dirtied. The function will periodically check the system’s
dirty state and will initiate writeback if needed.

Once we’re over the dirty memory limit we decrease the ratelimiting
by a lot, to prevent individual processes from overshooting the limit
by (ratelimit\_pages) each.

void tag\_pages\_for\_writeback(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t start, pgoff\_t end)
:   tag pages to be written by writeback

**Parameters**

`struct address_space *mapping`
:   address space structure to write

`pgoff_t start`
:   starting page index

`pgoff_t end`
:   ending page index (inclusive)

**Description**

This function scans the page range from **start** to **end** (inclusive) and tags
all pages that have DIRTY tag set with a special TOWRITE tag. The caller
can then use the TOWRITE tag to identify pages eligible for writeback.
This mechanism is used to avoid livelocking of writeback by a process
steadily creating new dirty pages in the file (thus it is important for this
function to be quick so that it can tag pages faster than a dirtying process
can create them).

struct [folio](#c.writeback_iter "folio") \*writeback\_iter(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct writeback\_control \*wbc, struct [folio](#c.writeback_iter "folio") \*folio, int \*error)
:   iterate folio of a mapping for writeback

**Parameters**

`struct address_space *mapping`
:   address space structure to write

`struct writeback_control *wbc`
:   writeback context

`struct folio *folio`
:   previously iterated folio (`NULL` to start)

`int *error`
:   in-out pointer for writeback errors (see below)

**Description**

This function returns the next folio for the writeback operation described by
**wbc** on **mapping** and should be called in a while loop in the ->writepages
implementation.

To start the writeback operation, `NULL` is passed in the **folio** argument, and
for every subsequent iteration the folio returned previously should be passed
back in.

If there was an error in the per-folio writeback inside the [`writeback_iter()`](#c.writeback_iter "writeback_iter")
loop, **error** should be set to the error value.

Once the writeback described in **wbc** has finished, this function will return
`NULL` and if there was an error in any iteration restore it to **error**.

**Note**

callers should not manually break out of the loop using break or goto
but must keep calling [`writeback_iter()`](#c.writeback_iter "writeback_iter") until it returns `NULL`.

**Return**

the folio to write or `NULL` if the loop is done.

bool filemap\_dirty\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [folio](#c.filemap_dirty_folio "folio") \*folio)
:   Mark a folio dirty for filesystems which do not use buffer\_heads.

**Parameters**

`struct address_space *mapping`
:   Address space this folio belongs to.

`struct folio *folio`
:   Folio to be marked as dirty.

**Description**

Filesystems which do not use buffer heads should call this function
from their dirty\_folio address space operation. It ignores the
contents of `folio_get_private()`, so if the filesystem marks individual
blocks as dirty, the filesystem should handle that itself.

This is also sometimes used by filesystems which use buffer\_heads when
a single buffer is being dirtied: we want to set the folio dirty in
that case, but not all the buffers. This is a “bottom-up” dirtying,
whereas [`block_dirty_folio()`](../filesystems/buffer.html#c.block_dirty_folio "block_dirty_folio") is a “top-down” dirtying.

The caller must ensure this doesn’t race with truncation. Most will
simply hold the folio lock, but e.g. `zap_pte_range()` calls with the
folio mapped and the pte lock held, which also locks out truncation.

bool folio\_redirty\_for\_writepage(struct writeback\_control \*wbc, struct [folio](#c.folio_redirty_for_writepage "folio") \*folio)
:   Decline to write a dirty folio.

**Parameters**

`struct writeback_control *wbc`
:   The writeback control.

`struct folio *folio`
:   The folio.

**Description**

When a writepage implementation decides that it doesn’t want to write
**folio** for some reason, it should call this function, unlock **folio** and
return 0.

**Return**

True if we redirtied the folio. False if someone else dirtied
it first.

bool folio\_mark\_dirty(struct [folio](#c.folio_mark_dirty "folio") \*folio)
:   Mark a folio as being modified.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

The folio may not be truncated while this function is running.
Holding the folio lock is sufficient to prevent truncation, but some
callers cannot acquire a sleeping lock. These callers instead hold
the page table lock for a page table which contains at least one page
in this folio. Truncation will block on the page table lock as it
unmaps pages before removing the folio from its mapping.

**Return**

True if the folio was newly dirtied, false if it was already dirty.

void folio\_wait\_writeback(struct [folio](#c.folio_wait_writeback "folio") \*folio)
:   Wait for a folio to finish writeback.

**Parameters**

`struct folio *folio`
:   The folio to wait for.

**Description**

If the folio is currently being written back to storage, wait for the
I/O to complete.

**Context**

Sleeps. Must be called in process context and with
no spinlocks held. Caller should hold a reference on the folio.
If the folio is not locked, writeback may start again after writeback
has finished.

int folio\_wait\_writeback\_killable(struct [folio](#c.folio_wait_writeback_killable "folio") \*folio)
:   Wait for a folio to finish writeback.

**Parameters**

`struct folio *folio`
:   The folio to wait for.

**Description**

If the folio is currently being written back to storage, wait for the
I/O to complete or a fatal signal to arrive.

**Context**

Sleeps. Must be called in process context and with
no spinlocks held. Caller should hold a reference on the folio.
If the folio is not locked, writeback may start again after writeback
has finished.

**Return**

0 on success, -EINTR if we get a fatal signal while waiting.

void folio\_wait\_stable(struct [folio](#c.folio_wait_stable "folio") \*folio)
:   wait for writeback to finish, if necessary.

**Parameters**

`struct folio *folio`
:   The folio to wait on.

**Description**

This function determines if the given folio is related to a backing
device that requires folio contents to be held stable during writeback.
If so, then it will wait for any pending writeback to complete.

**Context**

Sleeps. Must be called in process context and with
no spinlocks held. Caller should hold a reference on the folio.
If the folio is not locked, writeback may start again after writeback
has finished.

### Truncate

void folio\_invalidate(struct [folio](#c.folio_invalidate "folio") \*folio, size\_t offset, size\_t length)
:   Invalidate part or all of a folio.

**Parameters**

`struct folio *folio`
:   The folio which is affected.

`size_t offset`
:   start of the range to invalidate

`size_t length`
:   length of the range to invalidate

**Description**

[`folio_invalidate()`](#c.folio_invalidate "folio_invalidate") is called when all or part of the folio has become
invalidated by a truncate operation.

[`folio_invalidate()`](#c.folio_invalidate "folio_invalidate") does not have to release all buffers, but it must
ensure that no dirty buffer is left outside **offset** and that no I/O
is underway against any of the blocks which are outside the truncation
point. Because the caller is about to free (and possibly reuse) those
blocks on-disk.

void truncate\_inode\_pages\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t lstart, uoff\_t lend)
:   truncate range of pages specified by start & end byte offsets

**Parameters**

`struct address_space *mapping`
:   mapping to truncate

`loff_t lstart`
:   offset from which to truncate

`uoff_t lend`
:   offset to which to truncate (inclusive)

**Description**

Truncate the page cache, removing the pages that are between
specified offsets (and zeroing out partial pages
if lstart or lend + 1 is not page aligned).

Truncate takes two passes - the first pass is nonblocking. It will not
block on page locks and it will not block on writeback. The second pass
will wait. This is to prevent as much IO as possible in the affected region.
The first pass will remove most pages, so the search cost of the second pass
is low.

We pass down the cache-hot hint to the page freeing code. Even if the
mapping is large, it is probably the case that the final pages are the most
recently touched, and freeing happens in ascending file offset order.

Note that since ->`invalidate_folio()` accepts range to invalidate
truncate\_inode\_pages\_range is able to handle cases where lend + 1 is not
page aligned properly.

void truncate\_inode\_pages(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t lstart)
:   truncate *all* the pages from an offset

**Parameters**

`struct address_space *mapping`
:   mapping to truncate

`loff_t lstart`
:   offset from which to truncate

**Description**

Called under (and serialised by) inode->i\_rwsem and
mapping->invalidate\_lock.

**Note**

When this function returns, there can be a page in the process of
deletion (inside `__filemap_remove_folio()`) in the specified range. Thus
mapping->nrpages can be non-zero when this function returns even after
truncation of the whole mapping.

void truncate\_inode\_pages\_final(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   truncate *all* pages before inode dies

**Parameters**

`struct address_space *mapping`
:   mapping to truncate

**Description**

Called under (and serialized by) inode->i\_rwsem.

Filesystems have to use this in the .evict\_inode path to inform the
VM that this is the final truncate and the inode is going away.

unsigned long invalidate\_mapping\_pages(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t start, pgoff\_t end)
:   Invalidate all clean, unlocked cache of one inode

**Parameters**

`struct address_space *mapping`
:   the address\_space which holds the cache to invalidate

`pgoff_t start`
:   the offset ‘from’ which to invalidate

`pgoff_t end`
:   the offset ‘to’ which to invalidate (inclusive)

**Description**

This function removes pages that are clean, unmapped and unlocked,
as well as shadow entries. It will not block on IO activity.

If you want to remove all the pages of one inode, regardless of
their use and writeback state, use [`truncate_inode_pages()`](#c.truncate_inode_pages "truncate_inode_pages").

**Return**

The number of indices that had their contents invalidated

int invalidate\_inode\_pages2\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t start, pgoff\_t end)
:   remove range of pages from an address\_space

**Parameters**

`struct address_space *mapping`
:   the address\_space

`pgoff_t start`
:   the page offset ‘from’ which to invalidate

`pgoff_t end`
:   the page offset ‘to’ which to invalidate (inclusive)

**Description**

Any pages which are found to be mapped into pagetables are unmapped prior to
invalidation.

**Return**

-EBUSY if any pages could not be invalidated.

int invalidate\_inode\_pages2(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   remove all pages from an address\_space

**Parameters**

`struct address_space *mapping`
:   the address\_space

**Description**

Any pages which are found to be mapped into pagetables are unmapped prior to
invalidation.

**Return**

-EBUSY if any pages could not be invalidated.

void truncate\_pagecache(struct [inode](#c.truncate_pagecache "inode") \*inode, loff\_t newsize)
:   unmap and remove pagecache that has been truncated

**Parameters**

`struct inode *inode`
:   inode

`loff_t newsize`
:   new file size

**Description**

inode’s new i\_size must already be written before truncate\_pagecache
is called.

This function should typically be called before the filesystem
releases resources associated with the freed range (eg. deallocates
blocks). This way, pagecache will always stay logically coherent
with on-disk format, and the filesystem would not have to deal with
situations such as writepage being called for a page that has already
had its underlying blocks deallocated.

void truncate\_setsize(struct [inode](#c.truncate_setsize "inode") \*inode, loff\_t newsize)
:   update inode and pagecache for a new file size

**Parameters**

`struct inode *inode`
:   inode

`loff_t newsize`
:   new file size

**Description**

truncate\_setsize updates i\_size and performs pagecache truncation (if
necessary) to **newsize**. It will be typically be called from the filesystem’s
setattr function when ATTR\_SIZE is passed in.

Must be called with a lock serializing truncates and writes (generally
i\_rwsem but e.g. xfs uses a different lock) and before all filesystem
specific block truncation has been performed.

void pagecache\_isize\_extended(struct [inode](#c.pagecache_isize_extended "inode") \*inode, loff\_t from, loff\_t to)
:   update pagecache after extension of i\_size

**Parameters**

`struct inode *inode`
:   inode for which i\_size was extended

`loff_t from`
:   original inode size

`loff_t to`
:   new inode size

**Description**

Handle extension of inode size either caused by extending truncate or
by write starting after current i\_size. We mark the page straddling
current i\_size RO so that `page_mkwrite()` is called on the first
write access to the page. The filesystem will update its per-block
information before user writes to the page via mmap after the i\_size
has been changed.

The function must be called after i\_size is updated so that page fault
coming after we unlock the folio will already see the new i\_size.
The function must be called while we still hold i\_rwsem - this not only
makes sure i\_size is stable but also that userspace cannot observe new
i\_size value before we are prepared to store mmap writes at new inode size.

void truncate\_pagecache\_range(struct [inode](#c.truncate_pagecache_range "inode") \*inode, loff\_t lstart, loff\_t lend)
:   unmap and remove pagecache that is hole-punched

**Parameters**

`struct inode *inode`
:   inode

`loff_t lstart`
:   offset of beginning of hole

`loff_t lend`
:   offset of last byte of hole

**Description**

This function should typically be called before the filesystem
releases resources associated with the freed range (eg. deallocates
blocks). This way, pagecache will always stay logically coherent
with on-disk format, and the filesystem would not have to deal with
situations such as writepage being called for a page that has already
had its underlying blocks deallocated.

void filemap\_set\_wb\_err(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, int err)
:   set a writeback error on an address\_space

**Parameters**

`struct address_space *mapping`
:   mapping in which to set writeback error

`int err`
:   error to be set in mapping

**Description**

When writeback fails in some way, we must record that error so that
userspace can be informed when fsync and the like are called. We endeavor
to report errors on any file that was open at the time of the error. Some
internal callers also need to know when writeback errors have occurred.

When a writeback error occurs, most filesystems will want to call
filemap\_set\_wb\_err to record the error in the mapping so that it will be
automatically reported whenever fsync is called on the file.

int filemap\_check\_wb\_err(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, errseq\_t since)
:   has an error occurred since the mark was sampled?

**Parameters**

`struct address_space *mapping`
:   mapping to check for writeback errors

`errseq_t since`
:   previously-sampled errseq\_t

**Description**

Grab the errseq\_t value from the mapping, and see if it has changed “since”
the given value was sampled.

If it has then report the latest error set, otherwise return 0.

errseq\_t filemap\_sample\_wb\_err(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   sample the current errseq\_t to test for later errors

**Parameters**

`struct address_space *mapping`
:   mapping to be sampled

**Description**

Writeback errors are always reported relative to a particular sample point
in the past. This function provides those sample points.

errseq\_t file\_sample\_sb\_err(struct [file](#c.file_sample_sb_err "file") \*file)
:   sample the current errseq\_t to test for later errors

**Parameters**

`struct file *file`
:   file pointer to be sampled

**Description**

Grab the most current superblock-level errseq\_t value for the given
[`struct file`](../filesystems/api-summary.html#c.file "file").

void mapping\_set\_error(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, int error)
:   record a writeback error in the address\_space

**Parameters**

`struct address_space *mapping`
:   the mapping in which an error should be set

`int error`
:   the error to set in the mapping

**Description**

When writeback fails in some way, we must record that error so that
userspace can be informed when fsync and the like are called. We endeavor
to report errors on any file that was open at the time of the error. Some
internal callers also need to know when writeback errors have occurred.

When a writeback error occurs, most filesystems will want to call
mapping\_set\_error to record the error in the mapping so that it can be
reported when the application calls fsync(2).

void mapping\_set\_large\_folios(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping)
:   Indicate the file supports large folios.

**Parameters**

`struct address_space *mapping`
:   The address space of the file.

**Description**

The filesystem should call this function in its inode constructor to
indicate that the VFS can use large folios to cache the contents of
the file.

**Context**

This should not be called while the inode is active as it
is non-atomic.

pgoff\_t mapping\_align\_index(const struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index)
:   Align index for this mapping.

**Parameters**

`const struct address_space *mapping`
:   The address\_space.

`pgoff_t index`
:   The page index.

**Description**

The index of a folio must be naturally aligned. If you are adding a
new folio to the page cache and need to know what index to give it,
call this function.

struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*folio\_flush\_mapping(struct [folio](#c.folio_flush_mapping "folio") \*folio)
:   Find the file mapping this folio belongs to.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

For folios which are in the page cache, return the mapping that this
page belongs to. Anonymous folios return NULL, even if they’re in
the swap cache. Other kinds of folio also return NULL.

This is ONLY used by architecture cache flushing code. If you aren’t
writing cache flushing code, you want either [`folio_mapping()`](#c.folio_mapping "folio_mapping") or
`folio_file_mapping()`.

struct inode \*folio\_inode(struct [folio](#c.folio_inode "folio") \*folio)
:   Get the host inode for this folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

For folios which are in the page cache, return the inode that this folio
belongs to.

Do not call this for folios which aren’t in the page cache.

void folio\_attach\_private(struct [folio](#c.folio_attach_private "folio") \*folio, void \*data)
:   Attach private data to a folio.

**Parameters**

`struct folio *folio`
:   Folio to attach data to.

`void *data`
:   Data to attach to folio.

**Description**

Attaching private data to a folio increments the page’s reference count.
The data must be detached before the folio will be freed.

void \*folio\_change\_private(struct [folio](#c.folio_change_private "folio") \*folio, void \*data)
:   Change private data on a folio.

**Parameters**

`struct folio *folio`
:   Folio to change the data on.

`void *data`
:   Data to set on the folio.

**Description**

Change the private data attached to a folio and return the old
data. The page must previously have had data attached and the data
must be detached before the folio will be freed.

**Return**

Data that was previously attached to the folio.

void \*folio\_detach\_private(struct [folio](#c.folio_detach_private "folio") \*folio)
:   Detach private data from a folio.

**Parameters**

`struct folio *folio`
:   Folio to detach data from.

**Description**

Removes the data that was previously attached to the folio and decrements
the refcount on the page.

**Return**

Data that was attached to the folio.

type fgf\_t
:   Flags for getting folios from the page cache.

**Description**

Most users of the page cache will not need to use these flags;
there are convenience functions such as [`filemap_get_folio()`](#c.filemap_get_folio "filemap_get_folio") and
[`filemap_lock_folio()`](#c.filemap_lock_folio "filemap_lock_folio"). For users which need more control over exactly
what is done with the folios, these flags to `__filemap_get_folio()`
are available.

* `FGP_ACCESSED` - The folio will be marked accessed.
* `FGP_LOCK` - The folio is returned locked.
* `FGP_CREAT` - If no folio is present then a new folio is allocated,
  added to the page cache and the VM’s LRU list. The folio is
  returned locked.
* `FGP_FOR_MMAP` - The caller wants to do its own locking dance if the
  folio is already in cache. If the folio was allocated, unlock it
  before returning so the caller can do the same dance.
* `FGP_WRITE` - The folio will be written to by the caller.
* `FGP_NOFS` - \_\_GFP\_FS will get cleared in gfp.
* `FGP_NOWAIT` - Don’t block on the folio lock.
* `FGP_STABLE` - Wait for the folio to be stable (finished writeback)
* `FGP_DONTCACHE` - Uncached buffered IO
* `FGP_WRITEBEGIN` - The flags to use in a filesystem `write_begin()`
  implementation.

[fgf\_t](#c.fgf_t "fgf_t") fgf\_set\_order(size\_t size)
:   Encode a length in the fgf\_t flags.

**Parameters**

`size_t size`
:   The suggested size of the folio to create.

**Description**

The caller of `__filemap_get_folio()` can use this to suggest a preferred
size for the folio that is created. If there is already a folio at
the index, it will be returned, no matter what its size. If a folio
is freshly created, it may be of a different size than requested
due to alignment constraints, memory pressure, or the presence of
other folios at nearby indices.

struct [folio](#c.folio "folio") \*write\_begin\_get\_folio(const struct kiocb \*iocb, struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, size\_t len)
:   Get folio for write\_begin with flags.

**Parameters**

`const struct kiocb *iocb`
:   The kiocb passed from write\_begin (may be NULL).

`struct address_space *mapping`
:   The address space to search.

`pgoff_t index`
:   The page cache index.

`size_t len`
:   Length of data being written.

**Description**

This is a helper for filesystem `write_begin()` implementations.
It wraps `__filemap_get_folio()`, setting appropriate flags in
the write begin context.

**Return**

A folio or an ERR\_PTR.

struct [folio](#c.folio "folio") \*filemap\_get\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index)
:   Find and get a folio.

**Parameters**

`struct address_space *mapping`
:   The address\_space to search.

`pgoff_t index`
:   The page index.

**Description**

Looks up the page cache entry at **mapping** & **index**. If a folio is
present, it is returned with an increased refcount.

**Return**

A folio or ERR\_PTR(-ENOENT) if there is no folio in the cache for
this index. Will not return a shadow, swap or DAX entry.

struct [folio](#c.folio "folio") \*filemap\_lock\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index)
:   Find and lock a folio.

**Parameters**

`struct address_space *mapping`
:   The address\_space to search.

`pgoff_t index`
:   The page index.

**Description**

Looks up the page cache entry at **mapping** & **index**. If a folio is
present, it is returned locked with an increased refcount.

**Context**

May sleep.

**Return**

A folio or ERR\_PTR(-ENOENT) if there is no folio in the cache for
this index. Will not return a shadow, swap or DAX entry.

struct [folio](#c.folio "folio") \*filemap\_grab\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index)
:   grab a folio from the page cache

**Parameters**

`struct address_space *mapping`
:   The address space to search

`pgoff_t index`
:   The page index

**Description**

Looks up the page cache entry at **mapping** & **index**. If no folio is found,
a new folio is created. The folio is locked, marked as accessed, and
returned.

**Return**

A found or created folio. ERR\_PTR(-ENOMEM) if no folio is found
and failed to create a folio.

struct page \*find\_get\_page(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t offset)
:   find and get a page reference

**Parameters**

`struct address_space *mapping`
:   the address\_space to search

`pgoff_t offset`
:   the page index

**Description**

Looks up the page cache slot at **mapping** & **offset**. If there is a
page cache page, it is returned with an increased refcount.

Otherwise, `NULL` is returned.

struct page \*find\_lock\_page(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index)
:   locate, pin and lock a pagecache page

**Parameters**

`struct address_space *mapping`
:   the address\_space to search

`pgoff_t index`
:   the page index

**Description**

Looks up the page cache entry at **mapping** & **index**. If there is a
page cache page, it is returned locked and with an increased
refcount.

**Context**

May sleep.

**Return**

A `struct page` or `NULL` if there is no page in the cache for this
index.

struct page \*find\_or\_create\_page(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, gfp\_t gfp\_mask)
:   locate or add a pagecache page

**Parameters**

`struct address_space *mapping`
:   the page’s address\_space

`pgoff_t index`
:   the page’s index into the mapping

`gfp_t gfp_mask`
:   page allocation mode

**Description**

Looks up the page cache slot at **mapping** & **offset**. If there is a
page cache page, it is returned locked and with an increased
refcount.

If the page is not present, a new page is allocated using **gfp\_mask**
and added to the page cache and the VM’s LRU list. The page is
returned locked and with an increased refcount.

On memory exhaustion, `NULL` is returned.

[`find_or_create_page()`](#c.find_or_create_page "find_or_create_page") may sleep, even if **gfp\_flags** specifies an
atomic allocation!

struct page \*grab\_cache\_page\_nowait(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index)
:   returns locked page at given index in given cache

**Parameters**

`struct address_space *mapping`
:   target address\_space

`pgoff_t index`
:   the page index

**Description**

Returns locked page at given index in given cache, creating it if
needed, but do not wait if the page is locked or to reclaim memory.
This is intended for speculative data generators, where the data can
be regenerated if the page couldn’t be grabbed. This routine should
be safe to call while holding the lock for another page.

Clear \_\_GFP\_FS when allocating the page to avoid recursion into the fs
and deadlock against the caller’s locked page.

pgoff\_t folio\_next\_index(const struct [folio](#c.folio_next_index "folio") \*folio)
:   Get the index of the next folio.

**Parameters**

`const struct folio *folio`
:   The current folio.

**Return**

The index of the folio which follows this folio in the file.

loff\_t folio\_next\_pos(const struct [folio](#c.folio_next_pos "folio") \*folio)
:   Get the file position of the next folio.

**Parameters**

`const struct folio *folio`
:   The current folio.

**Return**

The position of the folio which follows this folio in the file.

struct page \*folio\_file\_page(struct [folio](#c.folio_file_page "folio") \*folio, pgoff\_t index)
:   The page for a particular index.

**Parameters**

`struct folio *folio`
:   The folio which contains this index.

`pgoff_t index`
:   The index we want to look up.

**Description**

Sometimes after looking up a folio in the page cache, we need to
obtain the specific page for an index (eg a page fault).

**Return**

The page containing the file data for this index.

bool folio\_contains(const struct [folio](#c.folio_contains "folio") \*folio, pgoff\_t index)
:   Does this folio contain this index?

**Parameters**

`const struct folio *folio`
:   The folio.

`pgoff_t index`
:   The page index within the file.

**Context**

The caller should have the folio locked and ensure
e.g., shmem did not move this folio to the swap cache.

**Return**

true or false.

pgoff\_t page\_pgoff(const struct [folio](#c.page_pgoff "folio") \*folio, const struct [page](#c.page_pgoff "page") \*page)
:   Calculate the logical page offset of this page.

**Parameters**

`const struct folio *folio`
:   The folio containing this page.

`const struct page *page`
:   The page which we need the offset of.

**Description**

For file pages, this is the offset from the beginning of the file
in units of PAGE\_SIZE. For anonymous pages, this is the offset from
the beginning of the anon\_vma in units of PAGE\_SIZE. This will
return nonsense for KSM pages.

**Context**

Caller must have a reference on the folio or otherwise
prevent it from being split or freed.

**Return**

The offset in units of PAGE\_SIZE.

loff\_t folio\_pos(const struct [folio](#c.folio_pos "folio") \*folio)
:   Returns the byte position of this folio in its file.

**Parameters**

`const struct folio *folio`
:   The folio.

bool folio\_trylock(struct [folio](#c.folio_trylock "folio") \*folio)
:   Attempt to lock a folio.

**Parameters**

`struct folio *folio`
:   The folio to attempt to lock.

**Description**

Sometimes it is undesirable to wait for a folio to be unlocked (eg
when the locks are being taken in the wrong order, or if making
progress through a batch of folios is more important than processing
them in order). Usually [`folio_lock()`](#c.folio_lock "folio_lock") is the correct function to call.

**Context**

Any context.

**Return**

Whether the lock was successfully acquired.

void folio\_lock(struct [folio](#c.folio_lock "folio") \*folio)
:   Lock this folio.

**Parameters**

`struct folio *folio`
:   The folio to lock.

**Description**

The folio lock protects against many things, probably more than it
should. It is primarily held while a folio is being brought uptodate,
either from its backing file or from swap. It is also held while a
folio is being truncated from its address\_space, so holding the lock
is sufficient to keep folio->mapping stable.

The folio lock is also held while write() is modifying the page to
provide POSIX atomicity guarantees (as long as the write does not
cross a page boundary). Other modifications to the data in the folio
do not hold the folio lock and can race with writes, eg DMA and stores
to mapped pages.

**Context**

May sleep. If you need to acquire the locks of two or
more folios, they must be in order of ascending index, if they are
in the same address\_space. If they are in different address\_spaces,
acquire the lock of the folio which belongs to the address\_space which
has the lowest address in memory first.

void lock\_page(struct [page](#c.lock_page "page") \*page)
:   Lock the folio containing this page.

**Parameters**

`struct page *page`
:   The page to lock.

**Description**

See [`folio_lock()`](#c.folio_lock "folio_lock") for a description of what the lock protects.
This is a legacy function and new code should probably use [`folio_lock()`](#c.folio_lock "folio_lock")
instead.

**Context**

May sleep. Pages in the same folio share a lock, so do not
attempt to lock two pages which share a folio.

int folio\_lock\_killable(struct [folio](#c.folio_lock_killable "folio") \*folio)
:   Lock this folio, interruptible by a fatal signal.

**Parameters**

`struct folio *folio`
:   The folio to lock.

**Description**

Attempts to lock the folio, like [`folio_lock()`](#c.folio_lock "folio_lock"), except that the sleep
to acquire the lock is interruptible by a fatal signal.

**Context**

May sleep; see [`folio_lock()`](#c.folio_lock "folio_lock").

**Return**

0 if the lock was acquired; -EINTR if a fatal signal was received.

bool filemap\_range\_needs\_writeback(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t start\_byte, loff\_t end\_byte)
:   check if range potentially needs writeback

**Parameters**

`struct address_space *mapping`
:   address space within which to check

`loff_t start_byte`
:   offset in bytes where the range starts

`loff_t end_byte`
:   offset in bytes where the range ends (inclusive)

**Description**

Find at least one page in the range supplied, usually used to check if
direct writing in this range will trigger a writeback. Used by O\_DIRECT
read/write with IOCB\_NOWAIT, to see if the caller needs to do
[`filemap_write_and_wait_range()`](#c.filemap_write_and_wait_range "filemap_write_and_wait_range") before proceeding.

**Return**

`true` if the caller should do [`filemap_write_and_wait_range()`](#c.filemap_write_and_wait_range "filemap_write_and_wait_range") before
doing O\_DIRECT to a page in this range, `false` otherwise.

struct readahead\_control
:   Describes a readahead request.

**Definition**:

```
struct readahead_control {
    struct file *file;
    struct address_space *mapping;
    struct file_ra_state *ra;
};
```

**Members**

`file`
:   The file, used primarily by network filesystems for authentication.
    May be NULL if invoked internally by the filesystem.

`mapping`
:   Readahead this filesystem object.

`ra`
:   File readahead state. May be NULL.

**Description**

A readahead request is for consecutive pages. Filesystems which
implement the ->readahead method should call [`readahead_folio()`](#c.readahead_folio "readahead_folio") or
`__readahead_batch()` in a loop and attempt to start reads into each
folio in the request.

Most of the fields in this `struct are` private and should be accessed
by the functions below.

void page\_cache\_sync\_readahead(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [file\_ra\_state](../filesystems/api-summary.html#c.file_ra_state "file_ra_state") \*ra, struct [file](#c.page_cache_sync_readahead "file") \*file, pgoff\_t index, unsigned long req\_count)
:   generic file readahead

**Parameters**

`struct address_space *mapping`
:   address\_space which holds the pagecache and I/O vectors

`struct file_ra_state *ra`
:   file\_ra\_state which holds the readahead state

`struct file *file`
:   Used by the filesystem for authentication.

`pgoff_t index`
:   Index of first page to be read.

`unsigned long req_count`
:   Total number of pages being read by the caller.

**Description**

[`page_cache_sync_readahead()`](#c.page_cache_sync_readahead "page_cache_sync_readahead") should be called when a cache miss happened:
it will submit the read. The readahead logic may decide to piggyback more
pages onto the read request if access patterns suggest it will improve
performance.

void page\_cache\_async\_readahead(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [file\_ra\_state](../filesystems/api-summary.html#c.file_ra_state "file_ra_state") \*ra, struct [file](#c.page_cache_async_readahead "file") \*file, struct [folio](#c.page_cache_async_readahead "folio") \*folio, unsigned long req\_count)
:   file readahead for marked pages

**Parameters**

`struct address_space *mapping`
:   address\_space which holds the pagecache and I/O vectors

`struct file_ra_state *ra`
:   file\_ra\_state which holds the readahead state

`struct file *file`
:   Used by the filesystem for authentication.

`struct folio *folio`
:   The folio which triggered the readahead call.

`unsigned long req_count`
:   Total number of pages being read by the caller.

**Description**

[`page_cache_async_readahead()`](#c.page_cache_async_readahead "page_cache_async_readahead") should be called when a page is used which
is marked as PageReadahead; this is a marker to suggest that the application
has used up enough of the readahead window that we should start pulling in
more pages.

struct [folio](#c.folio "folio") \*readahead\_folio(struct [readahead\_control](#c.readahead_control "readahead_control") \*ractl)
:   Get the next folio to read.

**Parameters**

`struct readahead_control *ractl`
:   The current readahead request.

**Context**

The folio is locked. The caller should unlock the folio once
all I/O to that folio has completed.

**Return**

A pointer to the next folio, or `NULL` if we are done.

loff\_t readahead\_pos(const struct [readahead\_control](#c.readahead_control "readahead_control") \*rac)
:   The byte offset into the file of this readahead request.

**Parameters**

`const struct readahead_control *rac`
:   The readahead request.

size\_t readahead\_length(const struct [readahead\_control](#c.readahead_control "readahead_control") \*rac)
:   The number of bytes in this readahead request.

**Parameters**

`const struct readahead_control *rac`
:   The readahead request.

pgoff\_t readahead\_index(const struct [readahead\_control](#c.readahead_control "readahead_control") \*rac)
:   The index of the first page in this readahead request.

**Parameters**

`const struct readahead_control *rac`
:   The readahead request.

unsigned int readahead\_count(const struct [readahead\_control](#c.readahead_control "readahead_control") \*rac)
:   The number of pages in this readahead request.

**Parameters**

`const struct readahead_control *rac`
:   The readahead request.

size\_t readahead\_batch\_length(const struct [readahead\_control](#c.readahead_control "readahead_control") \*rac)
:   The number of bytes in the current batch.

**Parameters**

`const struct readahead_control *rac`
:   The readahead request.

ssize\_t folio\_mkwrite\_check\_truncate(const struct [folio](#c.folio_mkwrite_check_truncate "folio") \*folio, const struct [inode](#c.folio_mkwrite_check_truncate "inode") \*inode)
:   check if folio was truncated

**Parameters**

`const struct folio *folio`
:   the folio to check

`const struct inode *inode`
:   the inode to check the folio against

**Return**

the number of bytes in the folio up to EOF,
or -EFAULT if the folio was truncated.

unsigned int i\_blocks\_per\_folio(const struct [inode](#c.i_blocks_per_folio "inode") \*inode, const struct [folio](#c.i_blocks_per_folio "folio") \*folio)
:   How many blocks fit in this folio.

**Parameters**

`const struct inode *inode`
:   The inode which contains the blocks.

`const struct folio *folio`
:   The folio.

**Description**

If the block size is larger than the size of this folio, return zero.

**Context**

The caller should hold a refcount on the folio to prevent it
from being split.

**Return**

The number of filesystem blocks covered by this folio.

## Memory pools

void mempool\_exit(struct mempool \*pool)
:   exit a mempool initialized with [`mempool_init()`](#c.mempool_init "mempool_init")

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool which was initialized with
    [`mempool_init()`](#c.mempool_init "mempool_init").

**Description**

Free all reserved elements in **pool** and **pool** itself. This function
only sleeps if the `free_fn()` function sleeps.

May be called on a zeroed but uninitialized mempool (i.e. allocated with
[`kzalloc()`](#c.kzalloc "kzalloc")).

void mempool\_destroy(struct mempool \*pool)
:   deallocate a memory pool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool which was allocated via
    `mempool_create()`.

**Description**

Free all reserved elements in **pool** and **pool** itself. This function
only sleeps if the `free_fn()` function sleeps.

int mempool\_init(struct mempool \*pool, int min\_nr, mempool\_alloc\_t \*alloc\_fn, mempool\_free\_t \*free\_fn, void \*pool\_data)
:   initialize a memory pool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool that should be initialized

`int min_nr`
:   the minimum number of elements guaranteed to be
    allocated for this pool.

`mempool_alloc_t *alloc_fn`
:   user-defined element-allocation function.

`mempool_free_t *free_fn`
:   user-defined element-freeing function.

`void *pool_data`
:   optional private data available to the user-defined functions.

**Description**

Like `mempool_create()`, but initializes the pool in (i.e. embedded in another
structure).

**Return**

`0` on success, negative error code otherwise.

struct mempool \*mempool\_create\_node(int min\_nr, mempool\_alloc\_t \*alloc\_fn, mempool\_free\_t \*free\_fn, void \*pool\_data, gfp\_t gfp\_mask, int node\_id)
:   create a memory pool

**Parameters**

`int min_nr`
:   the minimum number of elements guaranteed to be
    allocated for this pool.

`mempool_alloc_t *alloc_fn`
:   user-defined element-allocation function.

`mempool_free_t *free_fn`
:   user-defined element-freeing function.

`void *pool_data`
:   optional private data available to the user-defined functions.

`gfp_t gfp_mask`
:   memory allocation flags

`int node_id`
:   numa node to allocate on

**Description**

this function creates and allocates a guaranteed size, preallocated
memory pool. The pool can be used from the [`mempool_alloc()`](#c.mempool_alloc "mempool_alloc") and [`mempool_free()`](#c.mempool_free "mempool_free")
functions. This function might sleep. Both the `alloc_fn()` and the `free_fn()`
functions might sleep - as long as the [`mempool_alloc()`](#c.mempool_alloc "mempool_alloc") function is not called
from IRQ contexts.

**Return**

pointer to the created memory pool object or `NULL` on error.

int mempool\_resize(struct mempool \*pool, int new\_min\_nr)
:   resize an existing memory pool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool which was allocated via
    `mempool_create()`.

`int new_min_nr`
:   the new minimum number of elements guaranteed to be
    allocated for this pool.

**Description**

This function shrinks/grows the pool. In the case of growing,
it cannot be guaranteed that the pool will be grown to the new
size immediately, but new [`mempool_free()`](#c.mempool_free "mempool_free") calls will refill it.
This function may sleep.

Note, the caller must guarantee that no mempool\_destroy is called
while this function is running. [`mempool_alloc()`](#c.mempool_alloc "mempool_alloc") & [`mempool_free()`](#c.mempool_free "mempool_free")
might be called (eg. from IRQ contexts) while this function executes.

**Return**

`0` on success, negative error code otherwise.

int mempool\_alloc\_bulk(struct mempool \*pool, void \*\*elems, unsigned int count, unsigned int allocated)
:   allocate multiple elements from a memory pool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool

`void **elems`
:   partially or fully populated elements array

`unsigned int count`
:   number of entries in **elem** that need to be allocated

`unsigned int allocated`
:   number of entries in **elem** already allocated

**Description**

Allocate elements for each slot in **elem** that is non-`NULL`. This is done by
first calling into the alloc\_fn supplied at pool initialization time, and
dipping into the reserved pool when alloc\_fn fails to allocate an element.

On return all **count** elements in **elems** will be populated.

**Return**

Always 0. If it wasn’t for %$#^$ alloc tags, it would return void.

void \*mempool\_alloc(struct mempool \*pool, gfp\_t gfp\_mask)
:   allocate an element from a memory pool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool

`gfp_t gfp_mask`
:   GFP\_\* flags. `__GFP_ZERO` is not supported.

**Description**

Allocate an element from **pool**. This is done by first calling into the
alloc\_fn supplied at pool initialization time, and dipping into the reserved
pool when alloc\_fn fails to allocate an element.

This function only sleeps if the alloc\_fn callback sleeps, or when waiting
for elements to become available in the pool.

**Return**

pointer to the allocated element or `NULL` when failing to allocate
an element. Allocation failure can only happen when **gfp\_mask** does not
include `__GFP_DIRECT_RECLAIM`.

void \*mempool\_alloc\_preallocated(struct mempool \*pool)
:   allocate an element from preallocated elements belonging to a memory pool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool

**Description**

This function is similar to [`mempool_alloc()`](#c.mempool_alloc "mempool_alloc"), but it only attempts allocating
an element from the preallocated elements. It only takes a single spinlock\_t
and immediately returns if no preallocated elements are available.

**Return**

pointer to the allocated element or `NULL` if no elements are
available.

unsigned int mempool\_free\_bulk(struct mempool \*pool, void \*\*elems, unsigned int count)
:   return elements to a mempool

**Parameters**

`struct mempool *pool`
:   pointer to the memory pool

`void **elems`
:   elements to return

`unsigned int count`
:   number of elements to return

**Description**

Returns a number of elements from the start of **elem** to **pool** if **pool** needs
replenishing and sets their slots in **elem** to NULL. Other elements are left
in **elem**.

**Return**

number of elements transferred to **pool**. Elements are always
transferred from the beginning of **elem**, so the return value can be used as
an offset into **elem** for the freeing the remaining elements in the caller.

void mempool\_free(void \*element, struct mempool \*pool)
:   return an element to the pool.

**Parameters**

`void *element`
:   element to return

`struct mempool *pool`
:   pointer to the memory pool

**Description**

Returns **element** to **pool** if it needs replenishing, else frees it using
the free\_fn callback in **pool**.

This function only sleeps if the free\_fn callback sleeps.

## More Memory Management Functions

void zap\_special\_vma\_range(struct vm\_area\_struct \*vma, unsigned long address, unsigned long size)
:   zap all page table entries in a special vma range

**Parameters**

`struct vm_area_struct *vma`
:   the vma covering the range to zap

`unsigned long address`
:   starting address of the range to zap

`unsigned long size`
:   number of bytes to zap

**Description**

This function does nothing when the provided address range is not fully
contained in **vma**, or when the **vma** is not VM\_PFNMAP or VM\_MIXEDMAP.

int vm\_insert\_pages(struct vm\_area\_struct \*vma, unsigned long addr, struct page \*\*pages, unsigned long \*num)
:   insert multiple pages into user vma, batching the pmd lock.

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`unsigned long addr`
:   target start user address of these pages

`struct page **pages`
:   source kernel pages

`unsigned long *num`
:   in: number of pages to map. out: number of pages that were *not*
    mapped. (0 means all pages were successfully mapped).

**Description**

Preferred over [`vm_insert_page()`](#c.vm_insert_page "vm_insert_page") when inserting multiple pages.

In case of error, we may have mapped a subset of the provided
pages. It is the caller’s responsibility to account for this case.

The same restrictions apply as in [`vm_insert_page()`](#c.vm_insert_page "vm_insert_page").

int vm\_insert\_page(struct vm\_area\_struct \*vma, unsigned long addr, struct [page](#c.vm_insert_page "page") \*page)
:   insert single page into user vma

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`unsigned long addr`
:   target user address of this page

`struct page *page`
:   source kernel page

**Description**

This allows drivers to insert individual pages they’ve allocated
into a user vma. The zeropage is supported in some VMAs,
see `vm_mixed_zeropage_allowed()`.

The page has to be a nice clean \_individual\_ kernel allocation.
If you allocate a compound page, you need to have marked it as
such (\_\_GFP\_COMP), or manually just split the page up yourself
(see `split_page()`).

NOTE! Traditionally this was done with “[`remap_pfn_range()`](#c.remap_pfn_range "remap_pfn_range")” which
took an arbitrary page protection parameter. This doesn’t allow
that. Your vma protection will have to be set up correctly, which
means that if you want a shared writable mapping, you’d better
ask for a shared writable mapping!

The page does not need to be reserved.

Usually this function is called from f\_op->mmap() handler
under mm->mmap\_lock write-lock, so it can change vma->vm\_flags.
Caller must set VM\_MIXEDMAP on vma if it wants to call this
function from other places, for example from page-fault handler.

**Return**

`0` on success, negative error code otherwise.

int vm\_map\_pages(struct vm\_area\_struct \*vma, struct page \*\*pages, unsigned long num)
:   maps range of kernel pages starts with non zero offset

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`struct page **pages`
:   pointer to array of source kernel pages

`unsigned long num`
:   number of pages in page array

**Description**

Maps an object consisting of **num** pages, catering for the user’s
requested vm\_pgoff

If we fail to insert any page into the vma, the function will return
immediately leaving any previously inserted pages present. Callers
from the mmap handler may immediately return the error as their caller
will destroy the vma, removing any successfully inserted pages. Other
callers should make their own arrangements for calling `unmap_region()`.

**Context**

Process context. Called by mmap handlers.

**Return**

0 on success and error code otherwise.

int vm\_map\_pages\_zero(struct vm\_area\_struct \*vma, struct page \*\*pages, unsigned long num)
:   map range of kernel pages starts with zero offset

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`struct page **pages`
:   pointer to array of source kernel pages

`unsigned long num`
:   number of pages in page array

**Description**

Similar to [`vm_map_pages()`](#c.vm_map_pages "vm_map_pages"), except that it explicitly sets the offset
to 0. This function is intended for the drivers that did not consider
vm\_pgoff.

**Context**

Process context. Called by mmap handlers.

**Return**

0 on success and error code otherwise.

[vm\_fault\_t](#c.vm_fault_t "vm_fault_t") vmf\_insert\_pfn\_prot(struct vm\_area\_struct \*vma, unsigned long addr, unsigned long pfn, pgprot\_t pgprot)
:   insert single pfn into user vma with specified pgprot

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`unsigned long addr`
:   target user address of this page

`unsigned long pfn`
:   source kernel pfn

`pgprot_t pgprot`
:   pgprot flags for the inserted page

**Description**

This is exactly like [`vmf_insert_pfn()`](#c.vmf_insert_pfn "vmf_insert_pfn"), except that it allows drivers
to override pgprot on a per-page basis.

This only makes sense for IO mappings, and it makes no sense for
COW mappings. In general, using multiple vmas is preferable;
vmf\_insert\_pfn\_prot should only be used if using multiple VMAs is
impractical.

pgprot typically only differs from **vma->vm\_page\_prot** when drivers set
caching- and encryption bits different than those of **vma->vm\_page\_prot**,
because the caching- or encryption mode may not be known at mmap() time.

This is ok as long as **vma->vm\_page\_prot** is not used by the core vm
to set caching and encryption bits for those vmas (except for COW pages).
This is ensured by core vm only modifying these page table entries using
functions that don’t touch caching- or encryption bits, using `pte_modify()`
if needed. (See for example `mprotect()`).

Also when new page-table entries are created, this is only done using the
`fault()` callback, and never using the value of vma->vm\_page\_prot,
except for page-table entries that point to anonymous pages as the result
of COW.

**Context**

Process context. May allocate using `GFP_KERNEL`.

**Return**

vm\_fault\_t value.

[vm\_fault\_t](#c.vm_fault_t "vm_fault_t") vmf\_insert\_pfn(struct vm\_area\_struct \*vma, unsigned long addr, unsigned long pfn)
:   insert single pfn into user vma

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`unsigned long addr`
:   target user address of this page

`unsigned long pfn`
:   source kernel pfn

**Description**

Similar to vm\_insert\_page, this allows drivers to insert individual pages
they’ve allocated into a user vma. Same comments apply.

This function should only be called from a vm\_ops->fault handler, and
in that case the handler should return the result of this function.

vma cannot be a COW mapping.

As this is called only for pages that do not currently exist, we
do not need to flush old virtual caches or the TLB.

**Context**

Process context. May allocate using `GFP_KERNEL`.

**Return**

vm\_fault\_t value.

int remap\_pfn\_range(struct vm\_area\_struct \*vma, unsigned long addr, unsigned long pfn, unsigned long size, pgprot\_t prot)
:   remap kernel memory to userspace

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`unsigned long addr`
:   target page aligned user address to start at

`unsigned long pfn`
:   page frame number of kernel physical memory address

`unsigned long size`
:   size of mapping area

`pgprot_t prot`
:   page protection flags for this mapping

**Note**

this is only safe if the mm semaphore is held when called.

**Return**

`0` on success, negative error code otherwise.

int vm\_iomap\_memory(struct vm\_area\_struct \*vma, phys\_addr\_t start, unsigned long len)
:   remap memory to userspace

**Parameters**

`struct vm_area_struct *vma`
:   user vma to map to

`phys_addr_t start`
:   start of the physical memory to be mapped

`unsigned long len`
:   size of area

**Description**

This is a simplified `io_remap_pfn_range()` for common driver use. The
driver just needs to give us the physical memory range to be mapped,
we’ll figure out the rest from the vma information.

NOTE! Some drivers might want to tweak vma->vm\_page\_prot first to get
whatever write-combining details or similar.

**Return**

`0` on success, negative error code otherwise.

void unmap\_mapping\_pages(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t start, pgoff\_t nr, bool even\_cows)
:   Unmap pages from processes.

**Parameters**

`struct address_space *mapping`
:   The address space containing pages to be unmapped.

`pgoff_t start`
:   Index of first page to be unmapped.

`pgoff_t nr`
:   Number of pages to be unmapped. 0 to unmap to end of file.

`bool even_cows`
:   Whether to unmap even private COWed pages.

**Description**

Unmap the pages in this address space from any userspace process which
has them mmaped. Generally, you want to remove COWed pages as well when
a file is being truncated, but not when invalidating pages from the page
cache.

void unmap\_mapping\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, loff\_t const holebegin, loff\_t const holelen, int even\_cows)
:   unmap the portion of all mmaps in the specified address\_space corresponding to the specified byte range in the underlying file.

**Parameters**

`struct address_space *mapping`
:   the address space containing mmaps to be unmapped.

`loff_t const holebegin`
:   byte in first page to unmap, relative to the start of
    the underlying file. This will be rounded down to a PAGE\_SIZE
    boundary. Note that this is different from [`truncate_pagecache()`](#c.truncate_pagecache "truncate_pagecache"), which
    must keep the partial page. In contrast, we must get rid of
    partial pages.

`loff_t const holelen`
:   size of prospective hole in bytes. This will be rounded
    up to a PAGE\_SIZE boundary. A holelen of zero truncates to the
    end of the file.

`int even_cows`
:   1 when truncating a file, unmap even private COWed pages;
    but 0 when invalidating pagecache, don’t throw away private data.

int follow\_pfnmap\_start(struct follow\_pfnmap\_args \*args)
:   Look up a pfn mapping at a user virtual address

**Parameters**

`struct follow_pfnmap_args *args`
:   Pointer to struct **follow\_pfnmap\_args**

**Description**

The caller needs to setup args->vma and args->address to point to the
virtual address as the target of such lookup. On a successful return,
the results will be put into other output fields.

After the caller finished using the fields, the caller must invoke
another [`follow_pfnmap_end()`](#c.follow_pfnmap_end "follow_pfnmap_end") to proper releases the locks and resources
of such look up request.

During the [`start()`](../networking/ieee802154.html#c.start "start") and `end()` calls, the results in **args** will be valid
as proper locks will be held. After the `end()` is called, all the fields
in **follow\_pfnmap\_args** will be invalid to be further accessed. Further
use of such information after `end()` may require proper synchronizations
by the caller with page table updates, otherwise it can create a
security bug.

If the PTE maps a refcounted page, callers are responsible to protect
against invalidation with MMU notifiers; otherwise access to the PFN at
a later point in time can trigger use-after-free.

Only IO mappings and raw PFN mappings are allowed. The mmap semaphore
should be taken for read, and the mmap semaphore cannot be released
before the `end()` is invoked.

This function must not be used to modify PTE content.

**Return**

zero on success, negative otherwise.

void follow\_pfnmap\_end(struct follow\_pfnmap\_args \*args)
:   End a [`follow_pfnmap_start()`](#c.follow_pfnmap_start "follow_pfnmap_start") process

**Parameters**

`struct follow_pfnmap_args *args`
:   Pointer to struct **follow\_pfnmap\_args**

**Description**

Must be used in pair of [`follow_pfnmap_start()`](#c.follow_pfnmap_start "follow_pfnmap_start"). See the [`start()`](../networking/ieee802154.html#c.start "start") function
above for more information.

int generic\_access\_phys(struct vm\_area\_struct \*vma, unsigned long addr, void \*buf, int len, int write)
:   generic implementation for iomem mmap access

**Parameters**

`struct vm_area_struct *vma`
:   the vma to access

`unsigned long addr`
:   userspace address, not relative offset within **vma**

`void *buf`
:   buffer to read/write

`int len`
:   length of transfer

`int write`
:   set to FOLL\_WRITE when writing, otherwise reading

**Description**

This is a generic implementation for `vm_operations_struct.access` for an
iomem mapping. This callback is used by `access_process_vm()` when the **vma** is
not page based.

int copy\_remote\_vm\_str(struct task\_struct \*tsk, unsigned long addr, void \*buf, int len, unsigned int gup\_flags)
:   copy a string from another process’s address space.

**Parameters**

`struct task_struct *tsk`
:   the task of the target address space

`unsigned long addr`
:   start address to read from

`void *buf`
:   destination buffer

`int len`
:   number of bytes to copy

`unsigned int gup_flags`
:   flags modifying lookup behaviour

**Description**

The caller must hold a reference on **mm**.

**Return**

number of bytes copied from **addr** (source) to **buf** (destination);
not including the trailing NUL. Always guaranteed to leave NUL-terminated
buffer. On any error, return -EFAULT.

unsigned long \_\_get\_pfnblock\_flags\_mask(const struct [page](#c.__get_pfnblock_flags_mask "page") \*page, unsigned long pfn, unsigned long mask)
:   Return the requested group of flags for a pageblock\_nr\_pages block of pages

**Parameters**

`const struct page *page`
:   The page within the block of interest

`unsigned long pfn`
:   The target page frame number

`unsigned long mask`
:   mask of bits that the caller is interested in

**Return**

pageblock\_bits flags

bool get\_pfnblock\_bit(const struct [page](#c.get_pfnblock_bit "page") \*page, unsigned long pfn, enum pageblock\_bits pb\_bit)
:   Check if a standalone bit of a pageblock is set

**Parameters**

`const struct page *page`
:   The page within the block of interest

`unsigned long pfn`
:   The target page frame number

`enum pageblock_bits pb_bit`
:   pageblock bit to check

**Return**

true if the bit is set, otherwise false

enum migratetype get\_pfnblock\_migratetype(const struct [page](#c.get_pfnblock_migratetype "page") \*page, unsigned long pfn)
:   Return the migratetype of a pageblock

**Parameters**

`const struct page *page`
:   The page within the block of interest

`unsigned long pfn`
:   The target page frame number

**Return**

The migratetype of the pageblock

**Description**

Use [`get_pfnblock_migratetype()`](#c.get_pfnblock_migratetype "get_pfnblock_migratetype") if caller already has both **page** and **pfn**
to save a call to `page_to_pfn()`.

void \_\_set\_pfnblock\_flags\_mask(struct [page](#c.__set_pfnblock_flags_mask "page") \*page, unsigned long pfn, unsigned long flags, unsigned long mask)
:   Set the requested group of flags for a pageblock\_nr\_pages block of pages

**Parameters**

`struct page *page`
:   The page within the block of interest

`unsigned long pfn`
:   The target page frame number

`unsigned long flags`
:   The flags to set

`unsigned long mask`
:   mask of bits that the caller is interested in

void set\_pfnblock\_bit(const struct [page](#c.set_pfnblock_bit "page") \*page, unsigned long pfn, enum pageblock\_bits pb\_bit)
:   Set a standalone bit of a pageblock

**Parameters**

`const struct page *page`
:   The page within the block of interest

`unsigned long pfn`
:   The target page frame number

`enum pageblock_bits pb_bit`
:   pageblock bit to set

void clear\_pfnblock\_bit(const struct [page](#c.clear_pfnblock_bit "page") \*page, unsigned long pfn, enum pageblock\_bits pb\_bit)
:   Clear a standalone bit of a pageblock

**Parameters**

`const struct page *page`
:   The page within the block of interest

`unsigned long pfn`
:   The target page frame number

`enum pageblock_bits pb_bit`
:   pageblock bit to clear

void set\_pageblock\_migratetype(struct [page](#c.set_pageblock_migratetype "page") \*page, enum [migratetype](#c.set_pageblock_migratetype "migratetype") migratetype)
:   Set the migratetype of a pageblock

**Parameters**

`struct page *page`
:   The page within the block of interest

`enum migratetype migratetype`
:   migratetype to set

bool \_\_move\_freepages\_block\_isolate(struct [zone](#c.__move_freepages_block_isolate "zone") \*zone, struct [page](#c.__move_freepages_block_isolate "page") \*page, bool isolate)
:   move free pages in block for page isolation

**Parameters**

`struct zone *zone`
:   the zone

`struct page *page`
:   the pageblock page

`bool isolate`
:   to isolate the given pageblock or unisolate it

**Description**

This is similar to `move_freepages_block()`, but handles the special
case encountered in page isolation, where the block of interest
might be part of a larger buddy spanning multiple pageblocks.

Unlike the regular page allocator path, which moves pages while
stealing buddies off the freelist, page isolation is interested in
arbitrary pfn ranges that may have overlapping buddies on both ends.

This function handles that. Straddling buddies are split into
individual pageblocks. Only the block of interest is moved.

Returns `true` if pages could be moved, `false` otherwise.

void \_\_putback\_isolated\_page(struct [page](#c.__putback_isolated_page "page") \*page, unsigned int order, int mt)
:   Return a now-isolated page back where we got it

**Parameters**

`struct page *page`
:   Page that was isolated

`unsigned int order`
:   Order of the isolated page

`int mt`
:   The page’s pageblock’s migratetype

**Description**

This function is meant to return a page pulled from the free lists via
\_\_isolate\_free\_page back to the free lists they were pulled from.

void \_\_free\_pages(struct [page](#c.__free_pages "page") \*page, unsigned int order)
:   Free pages allocated with [`alloc_pages()`](#c.alloc_pages "alloc_pages").

**Parameters**

`struct page *page`
:   The page pointer returned from [`alloc_pages()`](#c.alloc_pages "alloc_pages").

`unsigned int order`
:   The order of the allocation.

**Description**

This function can free multi-page allocations that are not compound
pages. It does not check that the **order** passed in matches that of
the allocation, so it is easy to leak memory. Freeing more memory
than was allocated will probably emit a warning.

If the last reference to this page is speculative, it will be released
by `put_page()` which only frees the first page of a non-compound
allocation. To prevent the remaining pages from being leaked, we free
the subsequent pages here. If you want to use the page’s reference
count to decide when to free the allocation, you should allocate a
compound page, and use `put_page()` instead of [`__free_pages()`](#c.__free_pages "__free_pages").

**Context**

May be called in interrupt context or while holding a normal
spinlock, but not in NMI context or while holding a raw spinlock.

void free\_pages(unsigned long addr, unsigned int order)
:   Free pages allocated with `__get_free_pages()`.

**Parameters**

`unsigned long addr`
:   The virtual address tied to a page returned from `__get_free_pages()`.

`unsigned int order`
:   The order of the allocation.

**Description**

This function behaves the same as [`__free_pages()`](#c.__free_pages "__free_pages"). Use this function
to free pages when you only have a valid virtual address. If you have
the page, call [`__free_pages()`](#c.__free_pages "__free_pages") instead.

void \*alloc\_pages\_exact(size\_t size, gfp\_t gfp\_mask)
:   allocate an exact number physically-contiguous pages.

**Parameters**

`size_t size`
:   the number of bytes to allocate

`gfp_t gfp_mask`
:   GFP flags for the allocation, must not contain \_\_GFP\_COMP

**Description**

This function is similar to [`alloc_pages()`](#c.alloc_pages "alloc_pages"), except that it allocates the
minimum number of pages to satisfy the request. [`alloc_pages()`](#c.alloc_pages "alloc_pages") can only
allocate memory in power-of-two pages.

This function is also limited by MAX\_PAGE\_ORDER.

Memory allocated by this function must be released by [`free_pages_exact()`](#c.free_pages_exact "free_pages_exact").

**Return**

pointer to the allocated area or `NULL` in case of error.

void \*alloc\_pages\_exact\_nid(int nid, size\_t size, gfp\_t gfp\_mask)
:   allocate an exact number of physically-contiguous pages on a node.

**Parameters**

`int nid`
:   the preferred node ID where memory should be allocated

`size_t size`
:   the number of bytes to allocate

`gfp_t gfp_mask`
:   GFP flags for the allocation, must not contain \_\_GFP\_COMP

**Description**

Like [`alloc_pages_exact()`](#c.alloc_pages_exact "alloc_pages_exact"), but try to allocate on node nid first before falling
back.

**Return**

pointer to the allocated area or `NULL` in case of error.

void free\_pages\_exact(void \*virt, size\_t size)
:   release memory allocated via [`alloc_pages_exact()`](#c.alloc_pages_exact "alloc_pages_exact")

**Parameters**

`void *virt`
:   the value returned by alloc\_pages\_exact.

`size_t size`
:   size of allocation, same value as passed to [`alloc_pages_exact()`](#c.alloc_pages_exact "alloc_pages_exact").

**Description**

Release the memory allocated by a previous call to alloc\_pages\_exact.

unsigned long nr\_free\_zone\_pages(int offset)
:   count number of pages beyond high watermark

**Parameters**

`int offset`
:   The zone index of the highest zone

**Description**

[`nr_free_zone_pages()`](#c.nr_free_zone_pages "nr_free_zone_pages") counts the number of pages which are beyond the
high watermark within all zones at or below a given zone index. For each
zone, the number of pages is calculated as:

> nr\_free\_zone\_pages = managed\_pages - high\_pages

**Return**

number of pages beyond high watermark.

unsigned long nr\_free\_buffer\_pages(void)
:   count number of pages beyond high watermark

**Parameters**

`void`
:   no arguments

**Description**

[`nr_free_buffer_pages()`](#c.nr_free_buffer_pages "nr_free_buffer_pages") counts the number of pages which are beyond the high
watermark within ZONE\_DMA and ZONE\_NORMAL.

**Return**

number of pages beyond high watermark within ZONE\_DMA and
ZONE\_NORMAL.

int find\_next\_best\_node(int node, nodemask\_t \*used\_node\_mask)
:   find the next node that should appear in a given node’s fallback list

**Parameters**

`int node`
:   node whose fallback list we’re appending

`nodemask_t *used_node_mask`
:   nodemask\_t of already used nodes

**Description**

We use a number of factors to determine which is the next node that should
appear on a given node’s fallback list. The node should not have appeared
already in **node**’s fallback list, and it should be the next closest node
according to the distance array (which contains arbitrary distance values
from each node to each node in the system), and should also prefer nodes
with no CPUs, since presumably they’ll have very little allocation pressure
on them otherwise.

**Return**

node id of the found node or `NUMA_NO_NODE` if no node is found.

void setup\_per\_zone\_wmarks(void)
:   called when min\_free\_kbytes changes or when memory is hot-{added|removed}

**Parameters**

`void`
:   no arguments

**Description**

Ensures that the watermark[min,low,high] values for each zone are set
correctly with respect to min\_free\_kbytes.

int alloc\_contig\_frozen\_range(unsigned long start, unsigned long end, acr\_flags\_t alloc\_flags, gfp\_t gfp\_mask)
:   * tries to allocate given range of frozen pages

**Parameters**

`unsigned long start`
:   start PFN to allocate

`unsigned long end`
:   one-past-the-last PFN to allocate

`acr_flags_t alloc_flags`
:   allocation information

`gfp_t gfp_mask`
:   GFP mask. Node/zone/placement hints are ignored; only some
    action and reclaim modifiers are supported. Reclaim modifiers
    control allocation behavior during compaction/migration/reclaim.

**Description**

The PFN range does not have to be pageblock aligned. The PFN range must
belong to a single zone.

The first thing this routine does is attempt to MIGRATE\_ISOLATE all
pageblocks in the range. Once isolated, the pageblocks should not
be modified by others.

All frozen pages which PFN is in [start, end) are allocated for the
caller, and they could be freed with [`free_contig_frozen_range()`](#c.free_contig_frozen_range "free_contig_frozen_range"),
`free_frozen_pages()` also could be used to free compound frozen pages
directly.

**Return**

zero on success or negative error code.

int alloc\_contig\_range(unsigned long start, unsigned long end, acr\_flags\_t alloc\_flags, gfp\_t gfp\_mask)
:   * tries to allocate given range of pages

**Parameters**

`unsigned long start`
:   start PFN to allocate

`unsigned long end`
:   one-past-the-last PFN to allocate

`acr_flags_t alloc_flags`
:   allocation information

`gfp_t gfp_mask`
:   GFP mask.

**Description**

This routine is a wrapper around [`alloc_contig_frozen_range()`](#c.alloc_contig_frozen_range "alloc_contig_frozen_range"), it can’t
be used to allocate compound pages, the refcount of each allocated page
will be set to one.

All pages which PFN is in [start, end) are allocated for the caller,
and should be freed with [`free_contig_range()`](#c.free_contig_range "free_contig_range") or by manually calling
`__free_page()` on each allocated page.

**Return**

zero on success or negative error code.

struct page \*alloc\_contig\_frozen\_pages(unsigned long nr\_pages, gfp\_t gfp\_mask, int nid, nodemask\_t \*nodemask)
:   * tries to find and allocate contiguous range of frozen pages

**Parameters**

`unsigned long nr_pages`
:   Number of contiguous pages to allocate

`gfp_t gfp_mask`
:   GFP mask. Node/zone/placement hints limit the search; only some
    action and reclaim modifiers are supported. Reclaim modifiers
    control allocation behavior during compaction/migration/reclaim.

`int nid`
:   Target node

`nodemask_t *nodemask`
:   Mask for other possible nodes

**Description**

This routine is a wrapper around [`alloc_contig_frozen_range()`](#c.alloc_contig_frozen_range "alloc_contig_frozen_range"). It scans over
zones on an applicable zonelist to find a contiguous pfn range which can then
be tried for allocation with [`alloc_contig_frozen_range()`](#c.alloc_contig_frozen_range "alloc_contig_frozen_range"). This routine is
intended for allocation requests which can not be fulfilled with the buddy
allocator.

The allocated memory is always aligned to a page boundary. If nr\_pages is a
power of two, then allocated range is also guaranteed to be aligned to same
nr\_pages (e.g. 1GB request would be aligned to 1GB).

Allocated frozen pages need be freed with [`free_contig_frozen_range()`](#c.free_contig_frozen_range "free_contig_frozen_range"),
or by manually calling `free_frozen_pages()` on each allocated frozen
non-compound page, for compound frozen pages could be freed with
`free_frozen_pages()` directly.

**Return**

pointer to contiguous frozen pages on success, or NULL if not successful.

struct page \*alloc\_contig\_pages(unsigned long nr\_pages, gfp\_t gfp\_mask, int nid, nodemask\_t \*nodemask)
:   * tries to find and allocate contiguous range of pages

**Parameters**

`unsigned long nr_pages`
:   Number of contiguous pages to allocate

`gfp_t gfp_mask`
:   GFP mask.

`int nid`
:   Target node

`nodemask_t *nodemask`
:   Mask for other possible nodes

**Description**

This routine is a wrapper around [`alloc_contig_frozen_pages()`](#c.alloc_contig_frozen_pages "alloc_contig_frozen_pages"), it can’t
be used to allocate compound pages, the refcount of each allocated page
will be set to one.

Allocated pages can be freed with [`free_contig_range()`](#c.free_contig_range "free_contig_range") or by manually
calling `__free_page()` on each allocated page.

**Return**

pointer to contiguous pages on success, or NULL if not successful.

void free\_contig\_frozen\_range(unsigned long pfn, unsigned long nr\_pages)
:   * free the contiguous range of frozen pages

**Parameters**

`unsigned long pfn`
:   start PFN to free

`unsigned long nr_pages`
:   Number of contiguous frozen pages to free

**Description**

This can be used to free the allocated compound/non-compound frozen pages.

void free\_contig\_range(unsigned long pfn, unsigned long nr\_pages)
:   * free the contiguous range of pages

**Parameters**

`unsigned long pfn`
:   start PFN to free

`unsigned long nr_pages`
:   Number of contiguous pages to free

**Description**

This can be only used to free the allocated non-compound pages.

struct page \*alloc\_pages\_nolock(gfp\_t gfp\_flags, int nid, unsigned int order)
:   opportunistic reentrant allocation from any context

**Parameters**

`gfp_t gfp_flags`
:   GFP flags. Only \_\_GFP\_ACCOUNT allowed.

`int nid`
:   node to allocate from

`unsigned int order`
:   allocation order size

**Description**

Allocates pages of a given order from the given node. This is safe to
call from any context (from atomic, NMI, and also reentrant
allocator -> tracepoint -> alloc\_pages\_nolock\_noprof).
Allocation is best effort and to be expected to fail easily so nobody should
rely on the success. Failures are not reported via `warn_alloc()`.
See always fail conditions below.

**Return**

allocated page or NULL on failure. NULL does not mean EBUSY or EAGAIN.
It means ENOMEM. There is no reason to call it again and expect !NULL.

int numa\_nearest\_node(int node, unsigned int state)
:   Find nearest node by state

**Parameters**

`int node`
:   Node id to start the search

`unsigned int state`
:   State to filter the search

**Description**

Lookup the closest node by distance if **nid** is not in state.

**Return**

this **node** if it is in state, otherwise the closest node by distance

int nearest\_node\_nodemask(int node, nodemask\_t \*mask)
:   Find the node in **mask** at the nearest distance from **node**.

**Parameters**

`int node`
:   a valid node ID to start the search from.

`nodemask_t *mask`
:   a pointer to a nodemask representing the allowed nodes.

**Description**

This function iterates over all nodes in **mask** and calculates the
distance from the starting **node**, then it returns the node ID that is
the closest to **node**, or MAX\_NUMNODES if no node is found.

Note that **node** must be a valid node ID usable with `node_distance()`,
providing an invalid node ID (e.g., NUMA\_NO\_NODE) may result in crashes
or unexpected behavior.

bool folio\_can\_map\_prot\_numa(struct [folio](#c.folio_can_map_prot_numa "folio") \*folio, struct vm\_area\_struct \*vma, bool is\_private\_single\_threaded)
:   check whether the folio can map prot numa

**Parameters**

`struct folio *folio`
:   The folio whose mapping considered for being made NUMA hintable

`struct vm_area_struct *vma`
:   The VMA that the folio belongs to.

`bool is_private_single_threaded`
:   Is this a single-threaded private VMA or not

**Description**

This function checks to see if the folio actually indicates that
we need to make the mapping one which causes a NUMA hinting fault,
as there are cases where it’s simply unnecessary, and the folio’s
access time is adjusted for memory tiering if prot numa needed.

**Return**

True if the mapping of the folio needs to be changed, false otherwise.

struct page \*alloc\_pages\_mpol(gfp\_t gfp, unsigned int order, struct mempolicy \*pol, pgoff\_t ilx, int nid)
:   Allocate pages according to NUMA mempolicy.

**Parameters**

`gfp_t gfp`
:   GFP flags.

`unsigned int order`
:   Order of the page allocation.

`struct mempolicy *pol`
:   Pointer to the NUMA mempolicy.

`pgoff_t ilx`
:   Index for interleave mempolicy (also distinguishes [`alloc_pages()`](#c.alloc_pages "alloc_pages")).

`int nid`
:   Preferred node (usually `numa_node_id()` but **mpol** may override it).

**Return**

The page on success or NULL if allocation fails.

struct [folio](#c.folio "folio") \*vma\_alloc\_folio(gfp\_t gfp, int order, struct vm\_area\_struct \*vma, unsigned long addr)
:   Allocate a folio for a VMA.

**Parameters**

`gfp_t gfp`
:   GFP flags.

`int order`
:   Order of the folio.

`struct vm_area_struct *vma`
:   Pointer to VMA.

`unsigned long addr`
:   Virtual address of the allocation. Must be inside **vma**.

**Description**

Allocate a folio for a specific address in **vma**, using the appropriate
NUMA policy. The caller must hold the mmap\_lock of the mm\_struct of the
VMA to prevent it from going away. Should be used for all allocations
for folios that will be mapped into user space, excepting hugetlbfs, and
excepting where direct use of `folio_alloc_mpol()` is more appropriate.

**Return**

The folio on success or NULL if allocation fails.

struct page \*alloc\_pages(gfp\_t gfp, unsigned int order)
:   Allocate pages.

**Parameters**

`gfp_t gfp`
:   GFP flags.

`unsigned int order`
:   Power of two of number of pages to allocate.

**Description**

Allocate 1 << **order** contiguous pages. The physical address of the
first page is naturally aligned (eg an order-3 allocation will be aligned
to a multiple of 8 \* PAGE\_SIZE bytes). The NUMA policy of the current
process is honoured when in process context.

**Context**

Can be called from any context, providing the appropriate GFP
flags are used.

**Return**

The page on success or NULL if allocation fails.

int mpol\_misplaced(struct [folio](#c.mpol_misplaced "folio") \*folio, struct vm\_fault \*vmf, unsigned long addr)
:   check whether current folio node is valid in policy

**Parameters**

`struct folio *folio`
:   folio to be checked

`struct vm_fault *vmf`
:   structure describing the fault

`unsigned long addr`
:   virtual address in **vma** for shared policy lookup and interleave policy

**Description**

Lookup current policy node id for vma,addr and “compare to” folio’s
node id. Policy determination “mimics” `alloc_page_vma()`.
Called from fault path where we know the vma and faulting address.

**Return**

NUMA\_NO\_NODE if the page is in a node that is valid for this
policy, or a suitable node ID to allocate a replacement folio from.

void mpol\_shared\_policy\_init(struct shared\_policy \*sp, struct mempolicy \*mpol)
:   initialize shared policy for inode

**Parameters**

`struct shared_policy *sp`
:   pointer to inode shared policy

`struct mempolicy *mpol`
:   `struct mempolicy` to install

**Description**

Install non-NULL **mpol** in inode’s shared policy rb-tree.
On entry, the current task has a reference on a non-NULL **mpol**.
This must be released on exit.
This is called at `get_inode()` calls and we can use GFP\_KERNEL.

int mpol\_parse\_str(char \*str, struct mempolicy \*\*mpol)
:   parse string to mempolicy, for tmpfs mpol mount option.

**Parameters**

`char *str`
:   string containing mempolicy to parse

`struct mempolicy **mpol`
:   pointer to `struct mempolicy` pointer, returned on success.

**Description**

Format of input:
:   <mode>[=<flags>][:<nodelist>]

**Return**

`0` on success, else `1`

void mpol\_to\_str(char \*buffer, int maxlen, struct mempolicy \*pol)
:   format a mempolicy structure for printing

**Parameters**

`char *buffer`
:   to contain formatted mempolicy string

`int maxlen`
:   length of **buffer**

`struct mempolicy *pol`
:   pointer to mempolicy to be formatted

**Description**

Convert **pol** into a string. If **buffer** is too short, truncate the string.
Recommend a **maxlen** of at least 51 for the longest mode, “weighted
interleave”, plus the longest flag flags, “relative|balancing”, and to
display at least a few node ids.

type softleaf\_t
:   Describes a page table software leaf entry, abstracted from its architecture-specific encoding.

**Description**

Page table leaf entries are those which do not reference any descendent page
tables but rather either reference a data page, are an empty (or ‘none’
entry), or contain a non-present entry.

If referencing another page table or a data page then the page table entry is
pertinent to hardware - that is it tells the hardware how to decode the page
table entry.

Otherwise it is a software-defined leaf page table entry, which this type
describes. See leafops.h and specifically **softleaf\_type** for a list of all
possible kinds of software leaf entry.

A softleaf\_t entry is abstracted from the hardware page table entry, so is
not architecture-specific.

**NOTE**

While we transition from the confusing swp\_entry\_t type used for this
:   purpose, we simply alias this type. This will be removed once the
    transition is complete.

struct folio
:   Represents a contiguous set of bytes.

**Definition**:

```
struct folio {
    memdesc_flags_t flags;
    union {
        struct list_head lru;
        unsigned int mlock_count;
        struct dev_pagemap *pgmap;
    };
    struct address_space *mapping;
    union {
        pgoff_t index;
        unsigned long share;
    };
    union {
        void *private;
        swp_entry_t swap;
    };
    atomic_t _mapcount;
    atomic_t _refcount;
#ifdef CONFIG_MEMCG;
    unsigned long memcg_data;
#elif defined(CONFIG_SLAB_OBJ_EXT);
    unsigned long _unused_slab_obj_exts;
#endif;
#if defined(WANT_PAGE_VIRTUAL);
    void *virtual;
#endif;
#ifdef LAST_CPUPID_NOT_IN_PAGE_FLAGS;
    int _last_cpupid;
#endif;
    atomic_t _large_mapcount;
    atomic_t _nr_pages_mapped;
#ifdef CONFIG_64BIT;
    atomic_t _entire_mapcount;
    atomic_t _pincount;
#endif ;
    mm_id_mapcount_t _mm_id_mapcount[2];
    union {
        mm_id_t _mm_id[2];
        unsigned long _mm_ids;
    };
#ifdef NR_PAGES_IN_LARGE_FOLIO;
    unsigned int _nr_pages;
#endif ;
    struct list_head _deferred_list;
#ifndef CONFIG_64BIT;
    atomic_t _entire_mapcount;
    atomic_t _pincount;
#endif ;
    void *_hugetlb_subpool;
    void *_hugetlb_cgroup;
    void *_hugetlb_cgroup_rsvd;
    void *_hugetlb_hwpoison;
};
```

**Members**

`flags`
:   Identical to the page flags.

`{unnamed_union}`
:   anonymous

`lru`
:   Least Recently Used list; tracks how recently this folio was used.

`mlock_count`
:   Number of times this folio has been pinned by `mlock()`.

`pgmap`
:   Metadata for ZONE\_DEVICE mappings

`mapping`
:   The file this page belongs to, or refers to the anon\_vma for
    anonymous memory.

`{unnamed_union}`
:   anonymous

`index`
:   Offset within the file, in units of pages. For anonymous memory,
    this is the index from the beginning of the mmap.

`share`
:   number of DAX mappings that reference this folio. See
    dax\_associate\_entry.

`{unnamed_union}`
:   anonymous

`private`
:   Filesystem per-folio data (see [`folio_attach_private()`](#c.folio_attach_private "folio_attach_private")).

`swap`
:   Used for swp\_entry\_t if `folio_test_swapcache()`.

`_mapcount`
:   Do not access this member directly. Use [`folio_mapcount()`](#c.folio_mapcount "folio_mapcount") to
    find out how many times this folio is mapped by userspace.

`_refcount`
:   Do not access this member directly. Use [`folio_ref_count()`](#c.folio_ref_count "folio_ref_count")
    to find how many references there are to this folio.

`memcg_data`
:   Memory Control Group data.

`_unused_slab_obj_exts`
:   Placeholder to match obj\_exts in `struct slab`.

`virtual`
:   Virtual address in the kernel direct map.

`_last_cpupid`
:   IDs of last CPU and last process that accessed the folio.

`_large_mapcount`
:   Do not use directly, call [`folio_mapcount()`](#c.folio_mapcount "folio_mapcount").

`_nr_pages_mapped`
:   Do not use outside of rmap and debug code.

`_entire_mapcount`
:   Do not use directly, call `folio_entire_mapcount()`.

`_pincount`
:   Do not use directly, call [`folio_maybe_dma_pinned()`](#c.folio_maybe_dma_pinned "folio_maybe_dma_pinned").

`_mm_id_mapcount`
:   Do not use outside of rmap code.

`{unnamed_union}`
:   anonymous

`_mm_id`
:   Do not use outside of rmap code.

`_mm_ids`
:   Do not use outside of rmap code.

`_nr_pages`
:   Do not use directly, call [`folio_nr_pages()`](#c.folio_nr_pages "folio_nr_pages").

`_deferred_list`
:   Folios to be split under memory pressure.

`_entire_mapcount`
:   Do not use directly, call `folio_entire_mapcount()`.

`_pincount`
:   Do not use directly, call [`folio_maybe_dma_pinned()`](#c.folio_maybe_dma_pinned "folio_maybe_dma_pinned").

`_hugetlb_subpool`
:   Do not use directly, use accessor in hugetlb.h.

`_hugetlb_cgroup`
:   Do not use directly, use accessor in hugetlb\_cgroup.h.

`_hugetlb_cgroup_rsvd`
:   Do not use directly, use accessor in hugetlb\_cgroup.h.

`_hugetlb_hwpoison`
:   Do not use directly, call `raw_hwp_list_head()`.

**Description**

A folio is a physically, virtually and logically contiguous set
of bytes. It is a power-of-two in size, and it is aligned to that
same power-of-two. It is at least as large as `PAGE_SIZE`. If it is
in the page cache, it is at a file offset which is a multiple of that
power-of-two. It may be mapped into userspace at an address which is
at an arbitrary page offset, but its kernel virtual address is aligned
to its size.

struct ptdesc
:   Memory descriptor for page tables.

**Definition**:

```
struct ptdesc {
    memdesc_flags_t pt_flags;
    union {
        struct rcu_head pt_rcu_head;
        struct list_head pt_list;
        struct {
            unsigned long _pt_pad_1;
            pgtable_t pmd_huge_pte;
        };
    };
    unsigned long __page_mapping;
    union {
        pgoff_t pt_index;
        struct mm_struct *pt_mm;
        atomic_t pt_frag_refcount;
#ifdef CONFIG_HUGETLB_PMD_PAGE_TABLE_SHARING;
        atomic_t pt_share_count;
#endif;
    };
    union {
        unsigned long _pt_pad_2;
#if ALLOC_SPLIT_PTLOCKS;
        spinlock_t *ptl;
#else;
        spinlock_t ptl;
#endif;
    };
    unsigned int __page_type;
    atomic_t __page_refcount;
#ifdef CONFIG_MEMCG;
    unsigned long pt_memcg_data;
#endif;
};
```

**Members**

`pt_flags`
:   `enum pt_flags` plus zone/node/section.

`{unnamed_union}`
:   anonymous

`pt_rcu_head`
:   For freeing page table pages.

`pt_list`
:   List of used page tables. Used for s390 gmap shadow pages
    (which are not linked into the user page tables) and x86
    pgds.

`{unnamed_struct}`
:   anonymous

`_pt_pad_1`
:   Padding that aliases with page’s compound head.

`pmd_huge_pte`
:   Protected by ptdesc->ptl, used for THPs.

`__page_mapping`
:   Aliases with page->mapping. Unused for page tables.

`{unnamed_union}`
:   anonymous

`pt_index`
:   Used for s390 gmap.

`pt_mm`
:   Used for x86 pgds.

`pt_frag_refcount`
:   For fragmented page table tracking. Powerpc only.

`pt_share_count`
:   Used for HugeTLB PMD page table share count.

`{unnamed_union}`
:   anonymous

`_pt_pad_2`
:   Padding to ensure proper alignment.

`ptl`
:   Lock for the page table.

`ptl`
:   Lock for the page table.

`__page_type`
:   Same as page->page\_type. Unused for page tables.

`__page_refcount`
:   Same as page refcount.

`pt_memcg_data`
:   Memcg data. Tracked for page tables here.

**Description**

This `struct overlays` `struct page` for now. Do not modify without a good
understanding of the issues.

type vm\_fault\_t
:   Return type for page fault handlers.

**Description**

Page fault handlers return a bitmask of `VM_FAULT` values.

enum vm\_fault\_reason
:   Page fault handlers return a bitmask of these values to tell the core VM what happened when handling the fault. Used to decide whether a process gets delivered SIGBUS or just gets major/minor fault counters bumped up.

**Constants**

`VM_FAULT_OOM`
:   Out Of Memory

`VM_FAULT_SIGBUS`
:   Bad access

`VM_FAULT_MAJOR`
:   Page read from storage

`VM_FAULT_HWPOISON`
:   Hit poisoned small page

`VM_FAULT_HWPOISON_LARGE`
:   Hit poisoned large page. Index encoded
    in upper bits

`VM_FAULT_SIGSEGV`
:   segmentation fault

`VM_FAULT_NOPAGE`
:   ->fault installed the pte, not return page

`VM_FAULT_LOCKED`
:   ->fault locked the returned page

`VM_FAULT_RETRY`
:   ->fault blocked, must retry

`VM_FAULT_FALLBACK`
:   huge page fault failed, fall back to small

`VM_FAULT_DONE_COW`
:   ->fault has fully handled COW

`VM_FAULT_NEEDDSYNC`
:   ->fault did not modify page tables and needs
    `fsync()` to complete (for synchronous page faults
    in DAX)

`VM_FAULT_COMPLETED`
:   ->fault completed, meanwhile mmap lock released

`VM_FAULT_HINDEX_MASK`
:   mask HINDEX value

enum fault\_flag
:   Fault flag definitions.

**Constants**

`FAULT_FLAG_WRITE`
:   Fault was a write fault.

`FAULT_FLAG_MKWRITE`
:   Fault was mkwrite of existing PTE.

`FAULT_FLAG_ALLOW_RETRY`
:   Allow to retry the fault if blocked.

`FAULT_FLAG_RETRY_NOWAIT`
:   Don’t drop mmap\_lock and wait when retrying.

`FAULT_FLAG_KILLABLE`
:   The fault task is in SIGKILL killable region.

`FAULT_FLAG_TRIED`
:   The fault has been tried once.

`FAULT_FLAG_USER`
:   The fault originated in userspace.

`FAULT_FLAG_REMOTE`
:   The fault is not for current task/mm.

`FAULT_FLAG_INSTRUCTION`
:   The fault was during an instruction fetch.

`FAULT_FLAG_INTERRUPTIBLE`
:   The fault can be interrupted by non-fatal signals.

`FAULT_FLAG_UNSHARE`
:   The fault is an unsharing request to break COW in a
    COW mapping, making sure that an exclusive anon page is
    mapped after the fault.

`FAULT_FLAG_ORIG_PTE_VALID`
:   whether the fault has vmf->orig\_pte cached.
    We should only access orig\_pte if this flag set.

`FAULT_FLAG_VMA_LOCK`
:   The fault is handled under VMA lock.

**Description**

About **FAULT\_FLAG\_ALLOW\_RETRY** and **FAULT\_FLAG\_TRIED**: we can specify
whether we would allow page faults to retry by specifying these two
fault flags correctly. Currently there can be three legal combinations:

1. ALLOW\_RETRY and !TRIED: this means the page fault allows retry, and
   :   this is the first try
2. ALLOW\_RETRY and TRIED: this means the page fault allows retry, and
   :   we’ve already tried at least once
3. !ALLOW\_RETRY and !TRIED: this means the page fault does not allow retry

The unlisted combination (!ALLOW\_RETRY && TRIED) is illegal and should never
be used. Note that page faults can be allowed to retry for multiple times,
in which case we’ll have an initial fault with flags (a) then later on
continuous faults with flags (b). We should always try to detect pending
signals before a retry to make sure the continuous page faults can still be
interrupted if necessary.

The combination FAULT\_FLAG\_WRITE|FAULT\_FLAG\_UNSHARE is illegal.
FAULT\_FLAG\_UNSHARE is ignored and treated like an ordinary read fault when
applied to mappings that are not COW mappings.

int folio\_is\_file\_lru(const struct [folio](#c.folio_is_file_lru "folio") \*folio)
:   Should the folio be on a file LRU or anon LRU?

**Parameters**

`const struct folio *folio`
:   The folio to test.

**Description**

We would like to get this info without a page flag, but the state
needs to survive until the folio is last deleted from the LRU, which
could be as far down as \_\_page\_cache\_release.

**Return**

An integer (not a boolean!) used to sort a folio onto the
right LRU list and to account folios correctly.
1 if **folio** is a regular filesystem backed page cache folio
or a lazily freed anonymous folio (e.g. via MADV\_FREE).
0 if **folio** is a normal anonymous folio, a tmpfs folio or otherwise
ram or swap backed folio.

void \_\_folio\_clear\_lru\_flags(struct [folio](#c.__folio_clear_lru_flags "folio") \*folio)
:   Clear page lru flags before releasing a page.

**Parameters**

`struct folio *folio`
:   The folio that was on lru and now has a zero reference.

enum lru\_list folio\_lru\_list(const struct [folio](#c.folio_lru_list "folio") \*folio)
:   Which LRU list should a folio be on?

**Parameters**

`const struct folio *folio`
:   The folio to test.

**Return**

The LRU list a folio should be on, as an index
into the array of LRU lists.

size\_t num\_pages\_contiguous(struct page \*\*pages, size\_t nr\_pages)
:   determine the number of contiguous pages that represent contiguous PFNs

**Parameters**

`struct page **pages`
:   an array of page pointers

`size_t nr_pages`
:   length of the array, at least 1

**Description**

Determine the number of contiguous pages that represent contiguous PFNs
in **pages**, starting from the first page.

In some kernel configs contiguous PFNs will not have contiguous `struct
pages`. In these configurations [`num_pages_contiguous()`](#c.num_pages_contiguous "num_pages_contiguous") will return a num
smaller than ideal number. The caller should continue to check for pfn
contiguity after each call to [`num_pages_contiguous()`](#c.num_pages_contiguous "num_pages_contiguous").

Returns the number of contiguous pages.

page\_folio

`page_folio (p)`

> Converts from page to folio.

**Parameters**

`p`
:   The page.

**Description**

Every page is part of a folio. This function cannot be called on a
NULL pointer.

**Context**

No reference, nor lock is required on **page**. If the caller
does not hold a reference, this call may race with a folio split, so
it should re-check the folio still contains this page after gaining
a reference on the folio.

**Return**

The folio which contains this page.

folio\_page

`folio_page (folio, n)`

> Return a page from a folio.

**Parameters**

`folio`
:   The folio.

`n`
:   The page number to return.

**Description**

**n** is relative to the start of the folio. This function does not
check that the page number lies within **folio**; the caller is presumed
to have a reference to the page.

bool folio\_xor\_flags\_has\_waiters(struct [folio](#c.folio_xor_flags_has_waiters "folio") \*folio, unsigned long mask)
:   Change some folio flags.

**Parameters**

`struct folio *folio`
:   The folio.

`unsigned long mask`
:   Bits set in this word will be changed.

**Description**

This must only be used for flags which are changed with the folio
lock held. For example, it is unsafe to use for PG\_dirty as that
can be set without the folio lock held. It can also only be used
on flags which are in the range 0-6 as some of the implementations
only affect those bits.

**Return**

Whether there are tasks waiting on the folio.

bool folio\_test\_uptodate(const struct [folio](#c.folio_test_uptodate "folio") \*folio)
:   Is this folio up to date?

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

The uptodate flag is set on a folio when every byte in the folio is
at least as new as the corresponding bytes on storage. Anonymous
and CoW folios are always uptodate. If the folio is not uptodate,
some of the bytes in it may be; see the `is_partially_uptodate()`
address\_space operation.

bool folio\_test\_large(const struct [folio](#c.folio_test_large "folio") \*folio)
:   Does this folio contain more than one page?

**Parameters**

`const struct folio *folio`
:   The folio to test.

**Return**

True if the folio is larger than one page.

bool PageHuge(const struct [page](#c.PageHuge "page") \*page)
:   Determine if the page belongs to hugetlbfs

**Parameters**

`const struct page *page`
:   The page to test.

**Context**

Any context.

**Return**

True for hugetlbfs pages, false for anon pages or pages
belonging to other filesystems.

bool page\_has\_movable\_ops(const struct [page](#c.page_has_movable_ops "page") \*page)
:   test for a movable\_ops page

**Parameters**

`const struct page *page`
:   The page to test.

**Description**

Test whether this is a movable\_ops page. Such pages will stay that
way until freed.

Returns true if this is a movable\_ops page, otherwise false.

int folio\_has\_private(const struct [folio](#c.folio_has_private "folio") \*folio)
:   Determine if folio has private stuff

**Parameters**

`const struct folio *folio`
:   The folio to be checked

**Description**

Determine if a folio has private stuff, indicating that release routines
should be invoked upon it.

unsigned long folio\_page\_idx(const struct [folio](#c.folio_page_idx "folio") \*folio, const struct [page](#c.folio_page_idx "page") \*page)
:   Return the number of a page in a folio.

**Parameters**

`const struct folio *folio`
:   The folio.

`const struct page *page`
:   The folio page.

**Description**

This function expects that the page is actually part of the folio.
The returned number is relative to the start of the folio.

type vma\_flag\_t
:   specifies an individual VMA flag by bit number.

**Description**

This value is made type safe by sparse to avoid passing invalid flag values
around.

bool fault\_flag\_allow\_retry\_first(enum [fault\_flag](#c.fault_flag "fault_flag") flags)
:   check ALLOW\_RETRY the first time

**Parameters**

`enum fault_flag flags`
:   Fault flags.

**Description**

This is mostly used for places where we want to try to avoid taking
the mmap\_lock for too long a time when waiting for another condition
to change, in which case we can try to be polite to release the
mmap\_lock in the first round to avoid potential starvation of other
processes that would also want the mmap\_lock.

**Return**

true if the page fault allows retry and this is the first
attempt of the fault handling; false otherwise.

unsigned long vma\_kernel\_pagesize(struct vm\_area\_struct \*vma)
:   Default page size granularity for this VMA.

**Parameters**

`struct vm_area_struct *vma`
:   The user mapping.

**Description**

The kernel page size specifies in which granularity VMA modifications
can be performed. Folios in this VMA will be aligned to, and at least
the size of the number of bytes returned by this function.

The default kernel page size is not affected by Transparent Huge Pages
being in effect.

**Return**

The default page size granularity for this VMA.

unsigned int folio\_order(const struct [folio](#c.folio_order "folio") \*folio)
:   The allocation order of a folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

A folio is composed of 2^order pages. See `get_order()` for the definition
of order.

**Return**

The order of the folio.

void folio\_reset\_order(struct [folio](#c.folio_reset_order "folio") \*folio)
:   Reset the folio order and derived \_nr\_pages

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

Reset the order and derived \_nr\_pages to 0. Must only be used in the
process of splitting large folios.

int folio\_mapcount(const struct [folio](#c.folio_mapcount "folio") \*folio)
:   Number of mappings of this folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

The folio mapcount corresponds to the number of present user page table
entries that reference any part of a folio. Each such present user page
table entry must be paired with exactly on folio reference.

For ordindary folios, each user page table entry (PTE/PMD/PUD/...) counts
exactly once.

For hugetlb folios, each abstracted “hugetlb” user page table entry that
references the entire folio counts exactly once, even when such special
page table entries are comprised of multiple ordinary page table entries.

Will report 0 for pages which cannot be mapped into userspace, such as
slab, page tables and similar.

**Return**

The number of times this folio is mapped.

bool folio\_mapped(const struct [folio](#c.folio_mapped "folio") \*folio)
:   Is this folio mapped into userspace?

**Parameters**

`const struct folio *folio`
:   The folio.

**Return**

True if any page in this folio is referenced by user page tables.

unsigned int thp\_order(struct [page](#c.thp_order "page") \*page)
:   Order of a transparent huge page.

**Parameters**

`struct page *page`
:   Head page of a transparent huge page.

unsigned long thp\_size(struct [page](#c.thp_size "page") \*page)
:   Size of a transparent huge page.

**Parameters**

`struct page *page`
:   Head page of a transparent huge page.

**Return**

Number of bytes in this page.

void folio\_get(struct [folio](#c.folio_get "folio") \*folio)
:   Increment the reference count on a folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Context**

May be called in any context, as long as you know that
you have a refcount on the folio. If you do not already have one,
[`folio_try_get()`](#c.folio_try_get "folio_try_get") may be the right interface for you to use.

void folio\_put(struct [folio](#c.folio_put "folio") \*folio)
:   Decrement the reference count on a folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

If the folio’s reference count reaches zero, the memory will be
released back to the page allocator and may be used by another
allocation immediately. Do not access the memory or the [`struct folio`](#c.folio "folio")
after calling [`folio_put()`](#c.folio_put "folio_put") unless you can be sure that it wasn’t the
last reference.

**Context**

May be called in process or interrupt context, but not in NMI
context. May be called while holding a spinlock.

void folio\_put\_refs(struct [folio](#c.folio_put_refs "folio") \*folio, int refs)
:   Reduce the reference count on a folio.

**Parameters**

`struct folio *folio`
:   The folio.

`int refs`
:   The amount to subtract from the folio’s reference count.

**Description**

If the folio’s reference count reaches zero, the memory will be
released back to the page allocator and may be used by another
allocation immediately. Do not access the memory or the [`struct folio`](#c.folio "folio")
after calling [`folio_put_refs()`](#c.folio_put_refs "folio_put_refs") unless you can be sure that these weren’t
the last references.

**Context**

May be called in process or interrupt context, but not in NMI
context. May be called while holding a spinlock.

void folios\_put(struct folio\_batch \*folios)
:   Decrement the reference count on an array of folios.

**Parameters**

`struct folio_batch *folios`
:   The folios.

**Description**

Like [`folio_put()`](#c.folio_put "folio_put"), but for a batch of folios. This is more efficient
than writing the loop yourself as it will optimise the locks which need
to be taken if the folios are freed. The folios batch is returned
empty and ready to be reused for another batch; there is no need to
reinitialise it.

**Context**

May be called in process or interrupt context, but not in NMI
context. May be called while holding a spinlock.

unsigned long folio\_pfn(const struct [folio](#c.folio_pfn "folio") \*folio)
:   Return the Page Frame Number of a folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

A folio may contain multiple pages. The pages have consecutive
Page Frame Numbers.

**Return**

The Page Frame Number of the first page in the folio.

pte\_t folio\_mk\_pte(const struct [folio](#c.folio_mk_pte "folio") \*folio, pgprot\_t pgprot)
:   Create a PTE for this folio

**Parameters**

`const struct folio *folio`
:   The folio to create a PTE for

`pgprot_t pgprot`
:   The page protection bits to use

**Description**

Create a page table entry for the first page of this folio.
This is suitable for passing to `set_ptes()`.

**Return**

A page table entry suitable for mapping this folio.

pmd\_t folio\_mk\_pmd(const struct [folio](#c.folio_mk_pmd "folio") \*folio, pgprot\_t pgprot)
:   Create a PMD for this folio

**Parameters**

`const struct folio *folio`
:   The folio to create a PMD for

`pgprot_t pgprot`
:   The page protection bits to use

**Description**

Create a page table entry for the first page of this folio.
This is suitable for passing to `set_pmd_at()`.

**Return**

A page table entry suitable for mapping this folio.

pud\_t folio\_mk\_pud(const struct [folio](#c.folio_mk_pud "folio") \*folio, pgprot\_t pgprot)
:   Create a PUD for this folio

**Parameters**

`const struct folio *folio`
:   The folio to create a PUD for

`pgprot_t pgprot`
:   The page protection bits to use

**Description**

Create a page table entry for the first page of this folio.
This is suitable for passing to `set_pud_at()`.

**Return**

A page table entry suitable for mapping this folio.

bool folio\_maybe\_dma\_pinned(struct [folio](#c.folio_maybe_dma_pinned "folio") \*folio)
:   Report if a folio may be pinned for DMA.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

This function checks if a folio has been pinned via a call to
a function in the `pin_user_pages()` family.

For small folios, the return value is partially fuzzy: false is not fuzzy,
because it means “definitely not pinned for DMA”, but true means “probably
pinned for DMA, but possibly a false positive due to having at least
GUP\_PIN\_COUNTING\_BIAS worth of normal folio references”.

False positives are OK, because: a) it’s unlikely for a folio to
get that many refcounts, and b) all the callers of this routine are
expected to be able to deal gracefully with a false positive.

For most large folios, the result will be exactly correct. That’s because
we have more tracking data available: the \_pincount field is used
instead of the GUP\_PIN\_COUNTING\_BIAS scheme.

For more information, please see [pin\_user\_pages() and related calls](pin_user_pages.html).

**Return**

True, if it is likely that the folio has been “dma-pinned”.
False, if the folio is definitely not dma-pinned.

bool is\_zero\_page(const struct [page](#c.is_zero_page "page") \*page)
:   Query if a page is a zero page

**Parameters**

`const struct page *page`
:   The page to query

**Description**

This returns true if **page** is one of the permanent zero pages.

bool is\_zero\_folio(const struct [folio](#c.is_zero_folio "folio") \*folio)
:   Query if a folio is a zero page

**Parameters**

`const struct folio *folio`
:   The folio to query

**Description**

This returns true if **folio** is one of the permanent zero pages.

unsigned long folio\_nr\_pages(const struct [folio](#c.folio_nr_pages "folio") \*folio)
:   The number of pages in the folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Return**

A positive power of two.

struct [folio](#c.folio_next "folio") \*folio\_next(struct [folio](#c.folio_next "folio") \*folio)
:   Move to the next physical folio.

**Parameters**

`struct folio *folio`
:   The folio we’re currently operating on.

**Description**

If you have physically contiguous memory which may span more than
one folio (eg a `struct bio_vec`), use this function to move from one
folio to the next. Do not use it if the memory is only virtually
contiguous as the folios are almost certainly not adjacent to each
other. This is the folio equivalent to writing `page++`.

**Context**

We assume that the folios are refcounted and/or locked at a
higher level and do not adjust the reference counts.

**Return**

The next [`struct folio`](#c.folio "folio").

unsigned int folio\_shift(const struct [folio](#c.folio_shift "folio") \*folio)
:   The size of the memory described by this folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

A folio represents a number of bytes which is a power-of-two in size.
This function tells you which power-of-two the folio is. See also
[`folio_size()`](#c.folio_size "folio_size") and [`folio_order()`](#c.folio_order "folio_order").

**Context**

The caller should have a reference on the folio to prevent
it from being split. It is not necessary for the folio to be locked.

**Return**

The base-2 logarithm of the size of this folio.

size\_t folio\_size(const struct [folio](#c.folio_size "folio") \*folio)
:   The number of bytes in a folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Context**

The caller should have a reference on the folio to prevent
it from being split. It is not necessary for the folio to be locked.

**Return**

The number of bytes in this folio.

bool folio\_maybe\_mapped\_shared(struct [folio](#c.folio_maybe_mapped_shared "folio") \*folio)
:   Whether the folio is mapped into the page tables of more than one MM

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

This function checks if the folio maybe currently mapped into more than one
MM (“maybe mapped shared”), or if the folio is certainly mapped into a single
MM (“mapped exclusively”).

For KSM folios, this function also returns “mapped shared” when a folio is
mapped multiple times into the same MM, because the individual page mappings
are independent.

For small anonymous folios and anonymous hugetlb folios, the return
value will be exactly correct: non-KSM folios can only be mapped at most once
into an MM, and they cannot be partially mapped. KSM folios are
considered shared even if mapped multiple times into the same MM.

For other folios, the result can be fuzzy:
:   1. For partially-mappable large folios (THP), the return value can wrongly
       indicate “mapped shared” (false positive) if a folio was mapped by
       more than two MMs at one point in time.
    2. For pagecache folios (including hugetlb), the return value can wrongly
       indicate “mapped shared” (false positive) when two VMAs in the same MM
       cover the same file range.

Further, this function only considers current page table mappings that
are tracked using the folio mapcount(s).

This function does not consider:
:   1. If the folio might get mapped in the (near) future (e.g., swapcache,
       pagecache, temporary unmapping for migration).
    2. If the folio is mapped differently (VM\_PFNMAP).
    3. If hugetlb page table sharing applies. Callers might want to check
       `hugetlb_pmd_shared()`.

**Return**

Whether the folio is estimated to be mapped into more than one MM.

int folio\_expected\_ref\_count(const struct [folio](#c.folio_expected_ref_count "folio") \*folio)
:   calculate the expected folio refcount

**Parameters**

`const struct folio *folio`
:   the folio

**Description**

Calculate the expected folio refcount, taking references from the pagecache,
swapcache, PG\_private and page table mappings into account. Useful in
combination with [`folio_ref_count()`](#c.folio_ref_count "folio_ref_count") to detect unexpected references (e.g.,
GUP or other temporary references).

Does currently not consider references from the LRU cache. If the folio
was isolated from the LRU (which is the case during migration or split),
the LRU cache does not apply.

Calling this function on an unmapped folio -- ![`folio_mapped()`](#c.folio_mapped "folio_mapped") -- that is
locked will return a stable result.

Calling this function on a mapped folio will not result in a stable result,
because nothing stops additional page table mappings from coming (e.g.,
fork()) or going (e.g., `munmap()`).

Calling this function without the folio lock will also not result in a
stable result: for example, the folio might get dropped from the swapcache
concurrently.

However, even when called without the folio lock or on a mapped folio,
this function can be used to detect unexpected references early (for example,
if it makes sense to even lock the folio and unmap it).

The caller must add any reference (e.g., from [`folio_try_get()`](#c.folio_try_get "folio_try_get")) it might be
holding itself to the result.

**Return**

the expected folio refcount.

void zap\_vma(struct vm\_area\_struct \*vma)
:   zap all page table entries in a vma

**Parameters**

`struct vm_area_struct *vma`
:   The vma to zap.

void \*ptdesc\_address(const struct [ptdesc](#c.ptdesc "ptdesc") \*pt)
:   Virtual address of page table.

**Parameters**

`const struct ptdesc *pt`
:   Page table descriptor.

**Return**

The first byte of the page table described by **pt**.

void ptdesc\_set\_kernel(struct [ptdesc](#c.ptdesc_set_kernel "ptdesc") \*ptdesc)
:   Mark a ptdesc used to map the kernel

**Parameters**

`struct ptdesc *ptdesc`
:   The ptdesc to be marked

**Description**

Kernel page tables often need special handling. Set a flag so that
the handling code knows this ptdesc will not be used for userspace.

void ptdesc\_clear\_kernel(struct [ptdesc](#c.ptdesc_clear_kernel "ptdesc") \*ptdesc)
:   Mark a ptdesc as no longer used to map the kernel

**Parameters**

`struct ptdesc *ptdesc`
:   The ptdesc to be unmarked

**Description**

Use when the ptdesc is no longer used to map the kernel and no longer
needs special handling.

bool ptdesc\_test\_kernel(const struct [ptdesc](#c.ptdesc_test_kernel "ptdesc") \*ptdesc)
:   Check if a ptdesc is used to map the kernel

**Parameters**

`const struct ptdesc *ptdesc`
:   The ptdesc being tested

**Description**

Call to tell if the ptdesc used to map the kernel.

struct [ptdesc](#c.ptdesc "ptdesc") \*pagetable\_alloc(gfp\_t gfp, unsigned int order)
:   Allocate pagetables

**Parameters**

`gfp_t gfp`
:   GFP flags

`unsigned int order`
:   desired pagetable order

**Description**

pagetable\_alloc allocates memory for page tables as well as a page table
descriptor to describe that memory.

**Return**

The ptdesc describing the allocated page tables.

void pagetable\_free(struct [ptdesc](#c.ptdesc "ptdesc") \*pt)
:   Free pagetables

**Parameters**

`struct ptdesc *pt`
:   The page table descriptor

**Description**

pagetable\_free frees the memory of all page tables described by a page
table descriptor and the memory for the descriptor itself.

struct vm\_area\_struct \*vma\_lookup(struct mm\_struct \*mm, unsigned long addr)
:   Find a VMA at a specific address

**Parameters**

`struct mm_struct *mm`
:   The process address space.

`unsigned long addr`
:   The user address.

**Return**

The vm\_area\_struct at the given address, `NULL` otherwise.

void mmap\_action\_remap(struct vm\_area\_desc \*desc, unsigned long start, unsigned long start\_pfn, unsigned long size)
:   helper for mmap\_prepare hook to specify that a pure PFN remap is required.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring remap.

`unsigned long start`
:   The virtual address to start the remap from, must be within the VMA.

`unsigned long start_pfn`
:   The first PFN in the range to remap.

`unsigned long size`
:   The size of the range to remap, in bytes, at most spanning to the end
    of the VMA.

void mmap\_action\_remap\_full(struct vm\_area\_desc \*desc, unsigned long start\_pfn)
:   helper for mmap\_prepare hook to specify that the entirety of a VMA should be PFN remapped.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring remap.

`unsigned long start_pfn`
:   The first PFN in the range to remap.

void mmap\_action\_ioremap(struct vm\_area\_desc \*desc, unsigned long start, unsigned long start\_pfn, unsigned long size)
:   helper for mmap\_prepare hook to specify that a pure PFN I/O remap is required.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring remap.

`unsigned long start`
:   The virtual address to start the remap from, must be within the VMA.

`unsigned long start_pfn`
:   The first PFN in the range to remap.

`unsigned long size`
:   The size of the range to remap, in bytes, at most spanning to the end
    of the VMA.

void mmap\_action\_ioremap\_full(struct vm\_area\_desc \*desc, unsigned long start\_pfn)
:   helper for mmap\_prepare hook to specify that the entirety of a VMA should be PFN I/O remapped.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring remap.

`unsigned long start_pfn`
:   The first PFN in the range to remap.

void mmap\_action\_simple\_ioremap(struct vm\_area\_desc \*desc, phys\_addr\_t start\_phys\_addr, unsigned long size)
:   helper for mmap\_prepare hook to specify that the physical range in [start\_phys\_addr, start\_phys\_addr + size) should be I/O remapped.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring remap.

`phys_addr_t start_phys_addr`
:   Start of the physical memory to be mapped.

`unsigned long size`
:   Size of the area to map.

**NOTE**

Some drivers might want to tweak desc->page\_prot for purposes of
write-combine or similar.

void mmap\_action\_map\_kernel\_pages(struct vm\_area\_desc \*desc, unsigned long start, struct page \*\*pages, unsigned long nr\_pages)
:   helper for mmap\_prepare hook to specify that **num** kernel pages contained in the **pages** array should be mapped to userland starting at virtual address **start**.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring kernel pags to be mapped.

`unsigned long start`
:   The virtual address from which to map them.

`struct page **pages`
:   An array of `struct page` pointers describing the memory to map.

`unsigned long nr_pages`
:   The number of entries in the **pages** aray.

void mmap\_action\_map\_kernel\_pages\_full(struct vm\_area\_desc \*desc, struct page \*\*pages)
:   helper for mmap\_prepare hook to specify that kernel pages contained in the **pages** array should be mapped to userland from **desc->start** to **desc->end**.

**Parameters**

`struct vm_area_desc *desc`
:   The VMA descriptor for the VMA requiring kernel pags to be mapped.

`struct page **pages`
:   An array of `struct page` pointers describing the memory to map.

**Description**

The caller must ensure that **pages** contains sufficient entries to cover the
entire range described by **desc**.

bool range\_is\_subset(unsigned long outer\_start, unsigned long outer\_end, unsigned long inner\_start, unsigned long inner\_end)
:   Is the specified inner range a subset of the outer range?

**Parameters**

`unsigned long outer_start`
:   The start of the outer range.

`unsigned long outer_end`
:   The exclusive end of the outer range.

`unsigned long inner_start`
:   The start of the inner range.

`unsigned long inner_end`
:   The exclusive end of the inner range.

**Return**

`true` if [inner\_start, inner\_end) is a subset of [outer\_start,
outer\_end), otherwise `false`.

bool range\_in\_vma(const struct vm\_area\_struct \*vma, unsigned long start, unsigned long end)
:   is the specified [**start**, **end**) range a subset of the VMA?

**Parameters**

`const struct vm_area_struct *vma`
:   The VMA against which we want to check [**start**, **end**).

`unsigned long start`
:   The start of the range we wish to check.

`unsigned long end`
:   The exclusive end of the range we wish to check.

**Return**

`true` if [**start**, **end**) is a subset of [**vma->vm\_start**,
**vma->vm\_end**), `false` otherwise.

bool range\_in\_vma\_desc(const struct vm\_area\_desc \*desc, unsigned long start, unsigned long end)
:   is the specified [**start**, **end**) range a subset of the VMA described by **desc**, a VMA descriptor?

**Parameters**

`const struct vm_area_desc *desc`
:   The VMA descriptor against which we want to check [**start**, **end**).

`unsigned long start`
:   The start of the range we wish to check.

`unsigned long end`
:   The exclusive end of the range we wish to check.

**Return**

`true` if [**start**, **end**) is a subset of [**desc->start**, **desc->end**),
`false` otherwise.

void clear\_pages(void \*addr, unsigned int npages)
:   clear a page range for kernel-internal use.

**Parameters**

`void *addr`
:   start address

`unsigned int npages`
:   number of pages

**Description**

Use [`clear_user_pages()`](../mm/highmem.html#c.clear_user_pages "clear_user_pages") instead when clearing a page range to be
mapped to user space.

Does absolutely no exception handling.

Note that even though the clearing operation is preemptible, [`clear_pages()`](#c.clear_pages "clear_pages")
does not (and on architectures where it reduces to a few long-running
instructions, might not be able to) call `cond_resched()` to check if
rescheduling is required.

When running under preemptible models this is not a problem. Under
cooperatively scheduled models, however, the caller is expected to
limit **npages** to no more than PROCESS\_PAGES\_NON\_PREEMPT\_BATCH.

int folio\_ref\_count(const struct [folio](#c.folio_ref_count "folio") \*folio)
:   The reference count on this folio.

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

The refcount is usually incremented by calls to [`folio_get()`](#c.folio_get "folio_get") and
decremented by calls to [`folio_put()`](#c.folio_put "folio_put"). Some typical users of the
folio refcount:

* Each reference from a page table
* The page cache
* Filesystem private data
* The LRU list
* Pipes
* Direct IO which references this page in the process address space

**Return**

The number of references to this folio.

bool folio\_try\_get(struct [folio](#c.folio_try_get "folio") \*folio)
:   Attempt to increase the refcount on a folio.

**Parameters**

`struct folio *folio`
:   The folio.

**Description**

If you do not already have a reference to a folio, you can attempt to
get one using this function. It may fail if, for example, the folio
has been freed since you found a pointer to it, or it is frozen for
the purposes of splitting or migration.

**Return**

True if the reference count was successfully incremented.

int is\_highmem(const struct [zone](#c.is_highmem "zone") \*zone)
:   helper function to quickly check if a `struct zone` is a highmem zone or not. This is an attempt to keep references to ZONE\_{DMA/NORMAL/HIGHMEM/etc} in general code to a minimum.

**Parameters**

`const struct zone *zone`
:   pointer to `struct zone` variable

**Return**

1 for a highmem zone, 0 otherwise

for\_each\_online\_pgdat

`for_each_online_pgdat (pgdat)`

> helper macro to iterate over all online nodes

**Parameters**

`pgdat`
:   pointer to a pg\_data\_t variable

for\_each\_zone

`for_each_zone (zone)`

> helper macro to iterate over all memory zones

**Parameters**

`zone`
:   pointer to `struct zone` variable

**Description**

The user only needs to declare the zone variable, for\_each\_zone
fills it in.

struct zoneref \*next\_zones\_zonelist(struct zoneref \*z, enum zone\_type highest\_zoneidx, nodemask\_t \*nodes)
:   Returns the next zone at or below highest\_zoneidx within the allowed nodemask using a cursor within a zonelist as a starting point

**Parameters**

`struct zoneref *z`
:   The cursor used as a starting point for the search

`enum zone_type highest_zoneidx`
:   The zone index of the highest zone to return

`nodemask_t *nodes`
:   An optional nodemask to filter the zonelist with

**Description**

This function returns the next zone at or below a given zone index that is
within the allowed nodemask using a cursor as the starting point for the
search. The zoneref returned is a cursor that represents the current zone
being examined. It should be advanced by one before calling
next\_zones\_zonelist again.

**Return**

the next zone at or below highest\_zoneidx within the allowed
nodemask using a cursor within a zonelist as a starting point

struct zoneref \*first\_zones\_zonelist(struct [zonelist](#c.first_zones_zonelist "zonelist") \*zonelist, enum zone\_type highest\_zoneidx, nodemask\_t \*nodes)
:   Returns the first zone at or below highest\_zoneidx within the allowed nodemask in a zonelist

**Parameters**

`struct zonelist *zonelist`
:   The zonelist to search for a suitable zone

`enum zone_type highest_zoneidx`
:   The zone index of the highest zone to return

`nodemask_t *nodes`
:   An optional nodemask to filter the zonelist with

**Description**

This function returns the first zone at or below a given zone index that is
within the allowed nodemask. The zoneref returned is a cursor that can be
used to iterate the zonelist with next\_zones\_zonelist by advancing it by
one before calling.

When no eligible zone is found, zoneref->zone is NULL (zoneref itself is
never NULL). This may happen either genuinely, or due to concurrent nodemask
update due to cpuset modification.

**Return**

Zoneref pointer for the first suitable zone found

for\_each\_zone\_zonelist\_nodemask

`for_each_zone_zonelist_nodemask (zone, z, zlist, highidx, nodemask)`

> helper macro to iterate over valid zones in a zonelist at or below a given zone index and within a nodemask

**Parameters**

`zone`
:   The current zone in the iterator

`z`
:   The current pointer within zonelist->\_zonerefs being iterated

`zlist`
:   The zonelist being iterated

`highidx`
:   The zone index of the highest zone to return

`nodemask`
:   Nodemask allowed by the allocator

**Description**

This iterator iterates though all zones at or below a given zone index and
within a given nodemask

for\_each\_zone\_zonelist

`for_each_zone_zonelist (zone, z, zlist, highidx)`

> helper macro to iterate over valid zones in a zonelist at or below a given zone index

**Parameters**

`zone`
:   The current zone in the iterator

`z`
:   The current pointer within zonelist->zones being iterated

`zlist`
:   The zonelist being iterated

`highidx`
:   The zone index of the highest zone to return

**Description**

This iterator iterates though all zones at or below a given zone index.

int pfn\_valid(unsigned long pfn)
:   check if there is a valid memory map entry for a PFN

**Parameters**

`unsigned long pfn`
:   the page frame number to check

**Description**

Check if there is a valid memory map entry aka `struct page` for the **pfn**.
Note, that availability of the memory map entry does not imply that
there is actual usable memory at that **pfn**. The `struct page` may
represent a hole or an unusable page frame.

**Return**

1 for PFNs that have memory map entries and 0 otherwise

struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*folio\_mapping(const struct [folio](#c.folio_mapping "folio") \*folio)
:   Find the mapping where this folio is stored.

**Parameters**

`const struct folio *folio`
:   The folio.

**Description**

For folios which are in the page cache, return the mapping that this
page belongs to. Folios in the swap cache return the swap mapping
this page is stored in (which is different from the mapping for the
swap file or swap device where the data is stored).

You can call this for folios which aren’t in the swap cache or page
cache and it will return NULL.

int \_\_anon\_vma\_prepare(struct vm\_area\_struct \*vma)
:   attach an anon\_vma to a memory region

**Parameters**

`struct vm_area_struct *vma`
:   the memory region in question

**Description**

This makes sure the memory mapping described by ‘vma’ has
an ‘anon\_vma’ attached to it, so that we can associate the
anonymous pages mapped into it with that anon\_vma.

The common case will be that we already have one, which
is handled inline by `anon_vma_prepare()`. But if
not we either need to find an adjacent mapping that we
can re-use the anon\_vma from (very common when the only
reason for splitting a vma has been `mprotect()`), or we
allocate a new one.

Anon-vma allocations are very subtle, because we may have
optimistically looked up an anon\_vma in `folio_lock_anon_vma_read()`
and that may actually touch the rwsem even in the newly
allocated vma (it depends on RCU to make sure that the
anon\_vma isn’t actually destroyed).

As a result, we need to do proper anon\_vma locking even
for the new allocation. At the same time, we do not want
to do any locking for the common case of already having
an anon\_vma.

int anon\_vma\_clone(struct vm\_area\_struct \*dst, struct vm\_area\_struct \*src, enum vma\_operation operation)
:   Establishes new anon\_vma\_chain objects in **dst** linking to all of the anon\_vma objects contained within **src** anon\_vma\_chain’s.

**Parameters**

`struct vm_area_struct *dst`
:   The destination VMA with an empty anon\_vma\_chain.

`struct vm_area_struct *src`
:   The source VMA we wish to duplicate.

`enum vma_operation operation`
:   The type of operation which resulted in the clone.

**Description**

This is the heart of the VMA side of the anon\_vma implementation - we invoke
this function whenever we need to set up a new VMA’s anon\_vma state.

This is invoked for:

* VMA Merge, but only when **dst** is unfaulted and **src** is faulted - meaning we
  clone **src** into **dst**.
* VMA split.
* VMA (m)remap.
* Fork of faulted VMA.

In all cases other than fork this is simply a duplication. Fork additionally
adds a new active anon\_vma.

ONLY in the case of fork do we try to ‘reuse’ existing anon\_vma’s in an
anon\_vma hierarchy, reusing anon\_vma’s which have no VMA associated with them
but do have a single child. This is to avoid waste of memory when repeatedly
forking.

**Return**

0 on success, -ENOMEM on failure.

void unlink\_anon\_vmas(struct vm\_area\_struct \*vma)
:   remove all links between a VMA and anon\_vma’s, freeing anon\_vma\_chain objects.

**Parameters**

`struct vm_area_struct *vma`
:   The VMA whose links to anon\_vma objects is to be severed.

**Description**

As part of the process anon\_vma\_chain’s are freed,
anon\_vma->num\_children,num\_active\_vmas is updated as required and, if the
relevant anon\_vma references no further VMAs, its reference count is
decremented.

unsigned long page\_address\_in\_vma(const struct [folio](#c.page_address_in_vma "folio") \*folio, const struct [page](#c.page_address_in_vma "page") \*page, const struct vm\_area\_struct \*vma)
:   The virtual address of a page in this VMA.

**Parameters**

`const struct folio *folio`
:   The folio containing the page.

`const struct page *page`
:   The page within the folio.

`const struct vm_area_struct *vma`
:   The VMA we need to know the address in.

**Description**

Calculates the user virtual address of this page in the specified VMA.
It is the caller’s responsibility to check the page is actually
within the VMA. There may not currently be a PTE pointing at this
page, but if a page fault occurs at this address, this is the page
which will be accessed.

**Context**

Caller should hold a reference to the folio. Caller should
hold a lock (eg the i\_mmap\_lock or the mmap\_lock) which keeps the
VMA from being altered.

**Return**

The virtual address corresponding to this page in the VMA.

int folio\_referenced(struct [folio](#c.folio_referenced "folio") \*folio, int is\_locked, struct mem\_cgroup \*memcg, vm\_flags\_t \*vm\_flags)
:   Test if the folio was referenced.

**Parameters**

`struct folio *folio`
:   The folio to test.

`int is_locked`
:   Caller holds lock on the folio.

`struct mem_cgroup *memcg`
:   target memory cgroup

`vm_flags_t *vm_flags`
:   A combination of all the vma->vm\_flags which referenced the folio.

**Description**

Quick test\_and\_clear\_referenced for all mappings of a folio,

**Return**

The number of mappings which referenced the folio. Return -1 if
the function bailed out due to rmap lock contention.

int mapping\_wrprotect\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t pgoff, unsigned long pfn, unsigned long nr\_pages)
:   Write-protect all mappings in a specified range.

**Parameters**

`struct address_space *mapping`
:   The mapping whose reverse mapping should be traversed.

`pgoff_t pgoff`
:   The page offset at which **pfn** is mapped within **mapping**.

`unsigned long pfn`
:   The PFN of the page mapped in **mapping** at **pgoff**.

`unsigned long nr_pages`
:   The number of physically contiguous base pages spanned.

**Description**

Traverses the reverse mapping, finding all VMAs which contain a shared
mapping of the pages in the specified range in **mapping**, and write-protects
them (that is, updates the page tables to mark the mappings read-only such
that a write protection fault arises when the mappings are written to).

The **pfn** value need not refer to a folio, but rather can reference a kernel
allocation which is mapped into userland. We therefore do not require that
the page maps to a folio with a valid mapping or index field, rather the
caller specifies these in **mapping** and **pgoff**.

**Return**

the number of write-protected PTEs, or an error.

int pfn\_mkclean\_range(unsigned long pfn, unsigned long nr\_pages, pgoff\_t pgoff, struct vm\_area\_struct \*vma)
:   Cleans the PTEs (including PMDs) mapped with range of [**pfn**, **pfn** + **nr\_pages**) at the specific offset (**pgoff**) within the **vma** of shared mappings. And since clean PTEs should also be readonly, write protects them too.

**Parameters**

`unsigned long pfn`
:   start pfn.

`unsigned long nr_pages`
:   number of physically contiguous pages srarting with **pfn**.

`pgoff_t pgoff`
:   page offset that the **pfn** mapped with.

`struct vm_area_struct *vma`
:   vma that **pfn** mapped within.

**Description**

Returns the number of cleaned PTEs (including PMDs).

void folio\_move\_anon\_rmap(struct [folio](#c.folio_move_anon_rmap "folio") \*folio, struct vm\_area\_struct \*vma)
:   move a folio to our anon\_vma

**Parameters**

`struct folio *folio`
:   The folio to move to our anon\_vma

`struct vm_area_struct *vma`
:   The vma the folio belongs to

**Description**

When a folio belongs exclusively to one process after a COW event,
that folio can be moved into the anon\_vma that belongs to just that
process, so the rmap code will not search the parent or sibling processes.

void \_\_folio\_set\_anon(struct [folio](#c.__folio_set_anon "folio") \*folio, struct vm\_area\_struct \*vma, unsigned long address, bool exclusive)
:   set up a new anonymous rmap for a folio

**Parameters**

`struct folio *folio`
:   The folio to set up the new anonymous rmap for.

`struct vm_area_struct *vma`
:   VM area to add the folio to.

`unsigned long address`
:   User virtual address of the mapping

`bool exclusive`
:   Whether the folio is exclusive to the process.

void \_\_page\_check\_anon\_rmap(const struct [folio](#c.__page_check_anon_rmap "folio") \*folio, const struct [page](#c.__page_check_anon_rmap "page") \*page, struct vm\_area\_struct \*vma, unsigned long address)
:   sanity check anonymous rmap addition

**Parameters**

`const struct folio *folio`
:   The folio containing **page**.

`const struct page *page`
:   the page to check the mapping of

`struct vm_area_struct *vma`
:   the vm area in which the mapping is added

`unsigned long address`
:   the user virtual address mapped

void folio\_add\_anon\_rmap\_ptes(struct [folio](#c.folio_add_anon_rmap_ptes "folio") \*folio, struct [page](#c.folio_add_anon_rmap_ptes "page") \*page, int nr\_pages, struct vm\_area\_struct \*vma, unsigned long address, rmap\_t flags)
:   add PTE mappings to a page range of an anon folio

**Parameters**

`struct folio *folio`
:   The folio to add the mappings to

`struct page *page`
:   The first page to add

`int nr_pages`
:   The number of pages which will be mapped

`struct vm_area_struct *vma`
:   The vm area in which the mappings are added

`unsigned long address`
:   The user virtual address of the first page to map

`rmap_t flags`
:   The rmap flags

**Description**

The page range of folio is defined by [first\_page, first\_page + nr\_pages)

The caller needs to hold the page table lock, and the page must be locked in
the anon\_vma case: to serialize mapping,index checking after setting,
and to ensure that an anon folio is not being upgraded racily to a KSM folio
(but KSM folios are never downgraded).

void folio\_add\_anon\_rmap\_pmd(struct [folio](#c.folio_add_anon_rmap_pmd "folio") \*folio, struct [page](#c.folio_add_anon_rmap_pmd "page") \*page, struct vm\_area\_struct \*vma, unsigned long address, rmap\_t flags)
:   add a PMD mapping to a page range of an anon folio

**Parameters**

`struct folio *folio`
:   The folio to add the mapping to

`struct page *page`
:   The first page to add

`struct vm_area_struct *vma`
:   The vm area in which the mapping is added

`unsigned long address`
:   The user virtual address of the first page to map

`rmap_t flags`
:   The rmap flags

**Description**

The page range of folio is defined by [first\_page, first\_page + HPAGE\_PMD\_NR)

The caller needs to hold the page table lock, and the page must be locked in
the anon\_vma case: to serialize mapping,index checking after setting.

void folio\_add\_new\_anon\_rmap(struct [folio](#c.folio_add_new_anon_rmap "folio") \*folio, struct vm\_area\_struct \*vma, unsigned long address, rmap\_t flags)
:   Add mapping to a new anonymous folio.

**Parameters**

`struct folio *folio`
:   The folio to add the mapping to.

`struct vm_area_struct *vma`
:   the vm area in which the mapping is added

`unsigned long address`
:   the user virtual address mapped

`rmap_t flags`
:   The rmap flags

**Description**

Like folio\_add\_anon\_rmap\_\*() but must only be called on *new* folios.
This means the inc-and-test can be bypassed.
The folio doesn’t necessarily need to be locked while it’s exclusive
unless two threads map it concurrently. However, the folio must be
locked if it’s shared.

If the folio is pmd-mappable, it is accounted as a THP.

void folio\_add\_file\_rmap\_ptes(struct [folio](#c.folio_add_file_rmap_ptes "folio") \*folio, struct [page](#c.folio_add_file_rmap_ptes "page") \*page, int nr\_pages, struct vm\_area\_struct \*vma)
:   add PTE mappings to a page range of a folio

**Parameters**

`struct folio *folio`
:   The folio to add the mappings to

`struct page *page`
:   The first page to add

`int nr_pages`
:   The number of pages that will be mapped using PTEs

`struct vm_area_struct *vma`
:   The vm area in which the mappings are added

**Description**

The page range of the folio is defined by [page, page + nr\_pages)

The caller needs to hold the page table lock.

void folio\_add\_file\_rmap\_pmd(struct [folio](#c.folio_add_file_rmap_pmd "folio") \*folio, struct [page](#c.folio_add_file_rmap_pmd "page") \*page, struct vm\_area\_struct \*vma)
:   add a PMD mapping to a page range of a folio

**Parameters**

`struct folio *folio`
:   The folio to add the mapping to

`struct page *page`
:   The first page to add

`struct vm_area_struct *vma`
:   The vm area in which the mapping is added

**Description**

The page range of the folio is defined by [page, page + HPAGE\_PMD\_NR)

The caller needs to hold the page table lock.

void folio\_add\_file\_rmap\_pud(struct [folio](#c.folio_add_file_rmap_pud "folio") \*folio, struct [page](#c.folio_add_file_rmap_pud "page") \*page, struct vm\_area\_struct \*vma)
:   add a PUD mapping to a page range of a folio

**Parameters**

`struct folio *folio`
:   The folio to add the mapping to

`struct page *page`
:   The first page to add

`struct vm_area_struct *vma`
:   The vm area in which the mapping is added

**Description**

The page range of the folio is defined by [page, page + HPAGE\_PUD\_NR)

The caller needs to hold the page table lock.

void folio\_remove\_rmap\_ptes(struct [folio](#c.folio_remove_rmap_ptes "folio") \*folio, struct [page](#c.folio_remove_rmap_ptes "page") \*page, int nr\_pages, struct vm\_area\_struct \*vma)
:   remove PTE mappings from a page range of a folio

**Parameters**

`struct folio *folio`
:   The folio to remove the mappings from

`struct page *page`
:   The first page to remove

`int nr_pages`
:   The number of pages that will be removed from the mapping

`struct vm_area_struct *vma`
:   The vm area from which the mappings are removed

**Description**

The page range of the folio is defined by [page, page + nr\_pages)

The caller needs to hold the page table lock.

void folio\_remove\_rmap\_pmd(struct [folio](#c.folio_remove_rmap_pmd "folio") \*folio, struct [page](#c.folio_remove_rmap_pmd "page") \*page, struct vm\_area\_struct \*vma)
:   remove a PMD mapping from a page range of a folio

**Parameters**

`struct folio *folio`
:   The folio to remove the mapping from

`struct page *page`
:   The first page to remove

`struct vm_area_struct *vma`
:   The vm area from which the mapping is removed

**Description**

The page range of the folio is defined by [page, page + HPAGE\_PMD\_NR)

The caller needs to hold the page table lock.

void folio\_remove\_rmap\_pud(struct [folio](#c.folio_remove_rmap_pud "folio") \*folio, struct [page](#c.folio_remove_rmap_pud "page") \*page, struct vm\_area\_struct \*vma)
:   remove a PUD mapping from a page range of a folio

**Parameters**

`struct folio *folio`
:   The folio to remove the mapping from

`struct page *page`
:   The first page to remove

`struct vm_area_struct *vma`
:   The vm area from which the mapping is removed

**Description**

The page range of the folio is defined by [page, page + HPAGE\_PUD\_NR)

The caller needs to hold the page table lock.

void try\_to\_unmap(struct [folio](#c.try_to_unmap "folio") \*folio, enum ttu\_flags flags)
:   Try to remove all page table mappings to a folio.

**Parameters**

`struct folio *folio`
:   The folio to unmap.

`enum ttu_flags flags`
:   action and flags

**Description**

Tries to remove all the page table entries which are mapping this
folio. It is the caller’s responsibility to check if the folio is
still mapped if needed (use TTU\_SYNC to prevent accounting races).

**Context**

Caller must hold the folio lock.

void try\_to\_migrate(struct [folio](#c.try_to_migrate "folio") \*folio, enum ttu\_flags flags)
:   try to replace all page table mappings with swap entries

**Parameters**

`struct folio *folio`
:   the folio to replace page table entries for

`enum ttu_flags flags`
:   action and flags

**Description**

Tries to remove all the page table entries which are mapping this folio and
replace them with special swap entries. Caller must hold the folio lock.

struct page \*make\_device\_exclusive(struct mm\_struct \*mm, unsigned long addr, void \*owner, struct [folio](#c.folio "folio") \*\*foliop)
:   Mark a page for exclusive use by a device

**Parameters**

`struct mm_struct *mm`
:   mm\_struct of associated target process

`unsigned long addr`
:   the virtual address to mark for exclusive device access

`void *owner`
:   passed to MMU\_NOTIFY\_EXCLUSIVE range notifier to allow filtering

`struct folio **foliop`
:   folio pointer will be stored here on success.

**Description**

This function looks up the page mapped at the given address, grabs a
folio reference, locks the folio and replaces the PTE with special
device-exclusive PFN swap entry, preventing access through the process
page tables. The function will return with the folio locked and referenced.

On fault, the device-exclusive entries are replaced with the original PTE
under folio lock, after calling MMU notifiers.

Only anonymous non-hugetlb folios are supported and the VMA must have
write permissions such that we can fault in the anonymous page writable
in order to mark it exclusive. The caller must hold the mmap\_lock in read
mode.

A driver using this to program access from a device must use a mmu notifier
critical section to hold a device specific lock during programming. Once
programming is complete it should drop the folio lock and reference after
which point CPU access to the page will revoke the exclusive access.

**Notes**

> 1. This function always operates on individual PTEs mapping individual
>    pages. PMD-sized THPs are first remapped to be mapped by PTEs before
>    the conversion happens on a single PTE corresponding to **addr**.
> 2. While concurrent access through the process page tables is prevented,
>    concurrent access through other page references (e.g., earlier GUP
>    invocation) is not handled and not supported.
> 3. device-exclusive entries are considered “clean” and “old” by core-mm.
>    Device drivers must update the folio state when informed by MMU
>    notifiers.

**Return**

pointer to mapped page on success, otherwise a negative error.

void \_\_rmap\_walk\_file(struct [folio](#c.__rmap_walk_file "folio") \*folio, struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t pgoff\_start, unsigned long nr\_pages, struct rmap\_walk\_control \*rwc, bool locked)
:   Traverse the reverse mapping for a file-backed mapping of a page mapped within a specified page cache object at a specified offset.

**Parameters**

`struct folio *folio`
:   Either the folio whose mappings to traverse, or if NULL,
    the callbacks specified in **rwc** will be configured such
    as to be able to look up mappings correctly.

`struct address_space *mapping`
:   The page cache object whose mapping VMAs we intend to
    traverse. If **folio** is non-NULL, this should be equal to
    folio\_mapping(folio).

`pgoff_t pgoff_start`
:   The offset within **mapping** of the page which we are
    looking up. If **folio** is non-NULL, this should be equal
    to folio\_pgoff(folio).

`unsigned long nr_pages`
:   The number of pages mapped by the mapping. If **folio** is
    non-NULL, this should be equal to folio\_nr\_pages(folio).

`struct rmap_walk_control *rwc`
:   The reverse mapping walk control object describing how
    the traversal should proceed.

`bool locked`
:   Is the **mapping** already locked? If not, we acquire the
    lock.

bool isolate\_movable\_ops\_page(struct [page](#c.isolate_movable_ops_page "page") \*page, isolate\_mode\_t mode)
:   isolate a movable\_ops page for migration

**Parameters**

`struct page *page`
:   The page.

`isolate_mode_t mode`
:   The isolation mode.

**Description**

Try to isolate a movable\_ops page for migration. Will fail if the page is
not a movable\_ops page, if the page is already isolated for migration
or if the page was just was released by its owner.

Once isolated, the page cannot get freed until it is either putback
or migrated.

Returns true if isolation succeeded, otherwise false.

void putback\_movable\_ops\_page(struct [page](#c.putback_movable_ops_page "page") \*page)
:   putback an isolated movable\_ops page

**Parameters**

`struct page *page`
:   The isolated page.

**Description**

Putback an isolated movable\_ops page.

After the page was putback, it might get freed instantly.

int migrate\_movable\_ops\_page(struct page \*dst, struct page \*src, enum migrate\_mode mode)
:   migrate an isolated movable\_ops page

**Parameters**

`struct page *dst`
:   The destination page.

`struct page *src`
:   The source page.

`enum migrate_mode mode`
:   The migration mode.

**Description**

Migrate an isolated movable\_ops page.

If the src page was already released by its owner, the src page is
un-isolated (putback) and migration succeeds; the migration core will be the
owner of both pages.

If the src page was not released by its owner and the migration was
successful, the owner of the src page and the dst page are swapped and
the src page is un-isolated.

If migration fails, the ownership stays unmodified and the src page
remains isolated: migration may be retried later or the page can be putback.

TODO: migration core will treat both pages as folios and lock them before
this call to unlock them after this call. Further, the folio refcounts on
src and dst are also released by migration core. These pages will not be
folios in the future, so that must be reworked.

Returns 0 on success, otherwise a negative error code.

int migrate\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [folio](#c.folio "folio") \*dst, struct [folio](#c.folio "folio") \*src, enum migrate\_mode mode)
:   Simple folio migration.

**Parameters**

`struct address_space *mapping`
:   The address\_space containing the folio.

`struct folio *dst`
:   The folio to migrate the data to.

`struct folio *src`
:   The folio containing the current data.

`enum migrate_mode mode`
:   How to migrate the page.

**Description**

Common logic to directly migrate a single LRU folio suitable for
folios that do not have private data.

Folios are locked upon entry and exit.

int buffer\_migrate\_folio(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [folio](#c.folio "folio") \*dst, struct [folio](#c.folio "folio") \*src, enum migrate\_mode mode)
:   Migration function for folios with buffers.

**Parameters**

`struct address_space *mapping`
:   The address space containing **src**.

`struct folio *dst`
:   The folio to migrate to.

`struct folio *src`
:   The folio to migrate from.

`enum migrate_mode mode`
:   How to migrate the folio.

**Description**

This function can only be used if the underlying filesystem guarantees
that no other references to **src** exist. For example attached buffer
heads are accessed only under the folio lock. If your filesystem cannot
provide this guarantee, [`buffer_migrate_folio_norefs()`](#c.buffer_migrate_folio_norefs "buffer_migrate_folio_norefs") may be more
appropriate.

**Return**

0 on success or a negative errno on failure.

int buffer\_migrate\_folio\_norefs(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [folio](#c.folio "folio") \*dst, struct [folio](#c.folio "folio") \*src, enum migrate\_mode mode)
:   Migration function for folios with buffers.

**Parameters**

`struct address_space *mapping`
:   The address space containing **src**.

`struct folio *dst`
:   The folio to migrate to.

`struct folio *src`
:   The folio to migrate from.

`enum migrate_mode mode`
:   How to migrate the folio.

**Description**

Like [`buffer_migrate_folio()`](#c.buffer_migrate_folio "buffer_migrate_folio") except that this variant is more careful
and checks that there are also no buffer head references. This function
is the right one for mappings where buffer heads are directly looked
up and referenced (such as block device mappings).

**Return**

0 on success or a negative errno on failure.

unsigned long do\_mmap(struct [file](#c.do_mmap "file") \*file, unsigned long addr, unsigned long len, unsigned long prot, unsigned long flags, vm\_flags\_t vm\_flags, unsigned long pgoff, unsigned long \*populate, struct list\_head \*uf)
:   Perform a userland memory mapping into the current process address space of length **len** with protection bits **prot**, mmap flags **flags** (from which VMA flags will be inferred), and any additional VMA flags to apply **vm\_flags**. If this is a file-backed mapping then the file is specified in **file** and page offset into the file via **pgoff**.

**Parameters**

`struct file *file`
:   An optional [`struct file`](../filesystems/api-summary.html#c.file "file") pointer describing the file which is to be
    mapped, if a file-backed mapping.

`unsigned long addr`
:   If non-zero, hints at (or if **flags** has MAP\_FIXED set, specifies) the
    address at which to perform this mapping. See mmap (2) for details. Must be
    page-aligned.

`unsigned long len`
:   The length of the mapping. Will be page-aligned and must be at least 1
    page in size.

`unsigned long prot`
:   Protection bits describing access required to the mapping. See mmap
    (2) for details.

`unsigned long flags`
:   Flags specifying how the mapping should be performed, see mmap (2)
    for details.

`vm_flags_t vm_flags`
:   VMA flags which should be set by default, or 0 otherwise.

`unsigned long pgoff`
:   Page offset into the **file** if file-backed, should be 0 otherwise.

`unsigned long *populate`
:   A pointer to a value which will be set to 0 if no population of
    the range is required, or the number of bytes to populate if it is. Must be
    non-NULL. See mmap (2) for details as to under what circumstances population
    of the range occurs.

`struct list_head *uf`
:   An optional pointer to a list head to track userfaultfd unmap events
    should unmapping events arise. If provided, it is up to the caller to manage
    this.

**Description**

This function does not perform security checks on the file and assumes, if
**uf** is non-NULL, the caller has provided a list head to track unmap events
for userfaultfd **uf**.

It also simply indicates whether memory population is required by setting
**populate**, which must be non-NULL, expecting the caller to actually perform
this task itself if appropriate.

This function will invoke architecture-specific (and if provided and
relevant, file system-specific) logic to determine the most appropriate
unmapped area in which to place the mapping if not MAP\_FIXED.

Callers which require userland mmap() behaviour should invoke `vm_mmap()`,
which is also exported for module use.

Those which require this behaviour less security checks, userfaultfd and
populate behaviour, and who handle the mmap write lock themselves, should
call this function.

Note that the returned address may reside within a merged VMA if an
appropriate merge were to take place, so it doesn’t necessarily specify the
start of a VMA, rather only the start of a valid mapped range of length
**len** bytes, rounded down to the nearest page size.

The caller must write-lock current->mm->mmap\_lock.

**Return**

Either an error, or the address at which the requested mapping has
been performed.

struct vm\_area\_struct \*find\_vma\_intersection(struct mm\_struct \*mm, unsigned long start\_addr, unsigned long end\_addr)
:   Look up the first VMA which intersects the interval

**Parameters**

`struct mm_struct *mm`
:   The process address space.

`unsigned long start_addr`
:   The inclusive start user address.

`unsigned long end_addr`
:   The exclusive end user address.

**Return**

The first VMA within the provided range, `NULL` otherwise. Assumes
start\_addr < end\_addr.

struct vm\_area\_struct \*find\_vma(struct mm\_struct \*mm, unsigned long addr)
:   Find the VMA for a given address, or the next VMA.

**Parameters**

`struct mm_struct *mm`
:   The mm\_struct to check

`unsigned long addr`
:   The address

**Return**

The VMA associated with addr, or the next VMA.
May return `NULL` in the case of no VMA at addr or above.

struct vm\_area\_struct \*find\_vma\_prev(struct mm\_struct \*mm, unsigned long addr, struct vm\_area\_struct \*\*pprev)
:   Find the VMA for a given address, or the next vma and set `pprev` to the previous VMA, if any.

**Parameters**

`struct mm_struct *mm`
:   The mm\_struct to check

`unsigned long addr`
:   The address

`struct vm_area_struct **pprev`
:   The pointer to set to the previous VMA

**Description**

Note that RCU lock is missing here since the external `mmap_lock()` is used
instead.

**Return**

The VMA associated with **addr**, or the next vma.
May return `NULL` in the case of no vma at addr or above.

void \_\_ref kmemleak\_alloc(const void \*ptr, size\_t size, int min\_count, gfp\_t gfp)
:   register a newly allocated object

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

`size_t size`
:   size of the object

`int min_count`
:   minimum number of references to this object. If during memory
    scanning a number of references less than **min\_count** is found,
    the object is reported as a memory leak. If **min\_count** is 0,
    the object is never reported as a leak. If **min\_count** is -1,
    the object is ignored (not scanned and not reported as a leak)

`gfp_t gfp`
:   [`kmalloc()`](#c.kmalloc "kmalloc") flags used for kmemleak internal memory allocations

**Description**

This function is called from the kernel allocators when a new object
(memory block) is allocated (kmem\_cache\_alloc, kmalloc etc.).

void \_\_ref kmemleak\_alloc\_percpu(const void \_\_percpu \*ptr, size\_t size, gfp\_t gfp)
:   register a newly allocated \_\_percpu object

**Parameters**

`const void __percpu *ptr`
:   \_\_percpu pointer to beginning of the object

`size_t size`
:   size of the object

`gfp_t gfp`
:   flags used for kmemleak internal memory allocations

**Description**

This function is called from the kernel percpu allocator when a new object
(memory block) is allocated (alloc\_percpu).

void \_\_ref kmemleak\_vmalloc(const struct vm\_struct \*area, size\_t size, gfp\_t gfp)
:   register a newly vmalloc’ed object

**Parameters**

`const struct vm_struct *area`
:   pointer to vm\_struct

`size_t size`
:   size of the object

`gfp_t gfp`
:   `__vmalloc()` flags used for kmemleak internal memory allocations

**Description**

This function is called from the [`vmalloc()`](#c.vmalloc "vmalloc") kernel allocator when a new
object (memory block) is allocated.

void \_\_ref kmemleak\_free(const void \*ptr)
:   unregister a previously registered object

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

**Description**

This function is called from the kernel allocators when an object (memory
block) is freed (kmem\_cache\_free, kfree, vfree etc.).

void \_\_ref kmemleak\_free\_part(const void \*ptr, size\_t size)
:   partially unregister a previously registered object

**Parameters**

`const void *ptr`
:   pointer to the beginning or inside the object. This also
    represents the start of the range to be freed

`size_t size`
:   size to be unregistered

**Description**

This function is called when only a part of a memory block is freed
(usually from the bootmem allocator).

void \_\_ref kmemleak\_free\_percpu(const void \_\_percpu \*ptr)
:   unregister a previously registered \_\_percpu object

**Parameters**

`const void __percpu *ptr`
:   \_\_percpu pointer to beginning of the object

**Description**

This function is called from the kernel percpu allocator when an object
(memory block) is freed (free\_percpu).

void \_\_ref kmemleak\_update\_trace(const void \*ptr)
:   update object allocation stack trace

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

**Description**

Override the object allocation stack trace for cases where the actual
allocation place is not always useful.

void \_\_ref kmemleak\_not\_leak(const void \*ptr)
:   mark an allocated object as false positive

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

**Description**

Calling this function on an object will cause the memory block to no longer
be reported as leak and always be scanned.

void \_\_ref kmemleak\_transient\_leak(const void \*ptr)
:   mark an allocated object as transient false positive

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

**Description**

Calling this function on an object will cause the memory block to not be
reported as a leak temporarily. This may happen, for example, if the object
is part of a singly linked list and the ->next reference to it is changed.

void \_\_ref kmemleak\_ignore\_percpu(const void \_\_percpu \*ptr)
:   similar to kmemleak\_ignore but taking a percpu address argument

**Parameters**

`const void __percpu *ptr`
:   percpu address of the object

void \_\_ref kmemleak\_ignore(const void \*ptr)
:   ignore an allocated object

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

**Description**

Calling this function on an object will cause the memory block to be
ignored (not scanned and not reported as a leak). This is usually done when
it is known that the corresponding block is not a leak and does not contain
any references to other allocated memory blocks.

void \_\_ref kmemleak\_scan\_area(const void \*ptr, size\_t size, gfp\_t gfp)
:   limit the range to be scanned in an allocated object

**Parameters**

`const void *ptr`
:   pointer to beginning or inside the object. This also
    represents the start of the scan area

`size_t size`
:   size of the scan area

`gfp_t gfp`
:   [`kmalloc()`](#c.kmalloc "kmalloc") flags used for kmemleak internal memory allocations

**Description**

This function is used when it is known that only certain parts of an object
contain references to other objects. Kmemleak will only scan these areas
reducing the number false negatives.

void \_\_ref kmemleak\_no\_scan(const void \*ptr)
:   do not scan an allocated object

**Parameters**

`const void *ptr`
:   pointer to beginning of the object

**Description**

This function notifies kmemleak not to scan the given memory block. Useful
in situations where it is known that the given object does not contain any
references to other objects. Kmemleak will not scan such objects reducing
the number of false negatives.

void \_\_ref kmemleak\_alloc\_phys(phys\_addr\_t phys, size\_t size, gfp\_t gfp)
:   similar to kmemleak\_alloc but taking a physical address argument

**Parameters**

`phys_addr_t phys`
:   physical address of the object

`size_t size`
:   size of the object

`gfp_t gfp`
:   [`kmalloc()`](#c.kmalloc "kmalloc") flags used for kmemleak internal memory allocations

void \_\_ref kmemleak\_free\_part\_phys(phys\_addr\_t phys, size\_t size)
:   similar to kmemleak\_free\_part but taking a physical address argument

**Parameters**

`phys_addr_t phys`
:   physical address if the beginning or inside an object. This
    also represents the start of the range to be freed

`size_t size`
:   size to be unregistered

void \_\_ref kmemleak\_ignore\_phys(phys\_addr\_t phys)
:   similar to kmemleak\_ignore but taking a physical address argument

**Parameters**

`phys_addr_t phys`
:   physical address of the object

void \*devm\_memremap\_pages(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct dev\_pagemap \*pgmap)
:   remap and provide memmap backing for the given resource

**Parameters**

`struct device *dev`
:   hosting device for **res**

`struct dev_pagemap *pgmap`
:   pointer to a `struct dev_pagemap`

**Notes**

1/ At a minimum the range and type members of **pgmap** must be initialized
:   by the caller before passing it to this function

2/ The altmap field may optionally be initialized, in which case
:   PGMAP\_ALTMAP\_VALID must be set in pgmap->flags.

3/ The ref field may optionally be provided, in which pgmap->ref must be
:   ‘live’ on entry and will be killed and reaped at
    `devm_memremap_pages_release()` time, or if this routine fails.

4/ range is expected to be a host memory range that could feasibly be
:   treated as a “System RAM” range, i.e. not a device mmio range, but
    this is not enforced.

struct dev\_pagemap \*get\_dev\_pagemap(unsigned long pfn)
:   take a new live reference on the dev\_pagemap for **pfn**

**Parameters**

`unsigned long pfn`
:   page frame number to lookup page\_map

int huge\_pmd\_unshare(struct mmu\_gather \*tlb, struct vm\_area\_struct \*vma, unsigned long addr, pte\_t \*ptep)
:   Unmap a pmd table if it is shared by multiple users

**Parameters**

`struct mmu_gather *tlb`
:   the current mmu\_gather.

`struct vm_area_struct *vma`
:   the vma covering the pmd table.

`unsigned long addr`
:   the address we are trying to unshare.

`pte_t *ptep`
:   pointer into the (pmd) page table.

**Description**

Called with the page table lock held, the i\_mmap\_rwsem held in write mode
and the hugetlb vma lock held in write mode.

**Note**

The caller must call `huge_pmd_unshare_flush()` before dropping the
i\_mmap\_rwsem.

**Return**

1 if it was a shared PMD table and it got unmapped, or 0 if it
was not a shared PMD table.

bool folio\_isolate\_hugetlb(struct [folio](#c.folio_isolate_hugetlb "folio") \*folio, struct list\_head \*list)
:   try to isolate an allocated hugetlb folio

**Parameters**

`struct folio *folio`
:   the folio to isolate

`struct list_head *list`
:   the list to add the folio to on success

**Description**

Isolate an allocated (refcount > 0) hugetlb folio, marking it as
isolated/non-migratable, and moving it from the active list to the
given list.

Isolation will fail if **folio** is not an allocated hugetlb folio, or if
it is already isolated/non-migratable.

On success, an additional folio reference is taken that must be dropped
using [`folio_putback_hugetlb()`](#c.folio_putback_hugetlb "folio_putback_hugetlb") to undo the isolation.

**Return**

True if isolation worked, otherwise False.

void folio\_putback\_hugetlb(struct [folio](#c.folio_putback_hugetlb "folio") \*folio)
:   unisolate a hugetlb folio

**Parameters**

`struct folio *folio`
:   the isolated hugetlb folio

**Description**

Putback/un-isolate the hugetlb folio that was previous isolated using
[`folio_isolate_hugetlb()`](#c.folio_isolate_hugetlb "folio_isolate_hugetlb"): marking it non-isolated/migratable and putting it
back onto the active list.

Will drop the additional folio reference obtained through
[`folio_isolate_hugetlb()`](#c.folio_isolate_hugetlb "folio_isolate_hugetlb").

void folio\_mark\_accessed(struct [folio](#c.folio_mark_accessed "folio") \*folio)
:   Mark a folio as having seen activity.

**Parameters**

`struct folio *folio`
:   The folio to mark.

**Description**

This function will perform one of the following transitions:

* inactive,unreferenced -> inactive,referenced
* inactive,referenced -> active,unreferenced
* active,unreferenced -> active,referenced

When a newly allocated folio is not yet visible, so safe for non-atomic ops,
`__folio_set_referenced()` may be substituted for [`folio_mark_accessed()`](#c.folio_mark_accessed "folio_mark_accessed").

void folio\_add\_lru(struct [folio](#c.folio_add_lru "folio") \*folio)
:   Add a folio to an LRU list.

**Parameters**

`struct folio *folio`
:   The folio to be added to the LRU.

**Description**

Queue the folio for addition to the LRU. The decision on whether
to add the page to the [in]active [file|anon] list is deferred until the
folio\_batch is drained. This gives a chance for the caller of [`folio_add_lru()`](#c.folio_add_lru "folio_add_lru")
have the folio added to the active list using [`folio_mark_accessed()`](#c.folio_mark_accessed "folio_mark_accessed").

void folio\_add\_lru\_vma(struct [folio](#c.folio_add_lru_vma "folio") \*folio, struct vm\_area\_struct \*vma)
:   Add a folio to the appropriate LRU list for this VMA.

**Parameters**

`struct folio *folio`
:   The folio to be added to the LRU.

`struct vm_area_struct *vma`
:   VMA in which the folio is mapped.

**Description**

If the VMA is mlocked, **folio** is added to the unevictable list.
Otherwise, it is treated the same way as [`folio_add_lru()`](#c.folio_add_lru "folio_add_lru").

void deactivate\_file\_folio(struct [folio](#c.deactivate_file_folio "folio") \*folio)
:   Deactivate a file folio.

**Parameters**

`struct folio *folio`
:   Folio to deactivate.

**Description**

This function hints to the VM that **folio** is a good reclaim candidate,
for example if its invalidation fails due to the folio being dirty
or under writeback.

**Context**

Caller holds a reference on the folio.

void folio\_mark\_lazyfree(struct [folio](#c.folio_mark_lazyfree "folio") \*folio)
:   make an anon folio lazyfree

**Parameters**

`struct folio *folio`
:   folio to deactivate

**Description**

[`folio_mark_lazyfree()`](#c.folio_mark_lazyfree "folio_mark_lazyfree") moves **folio** to the inactive file list.
This is done to accelerate the reclaim of **folio**.

void folios\_put\_refs(struct folio\_batch \*folios, unsigned int \*refs)
:   Reduce the reference count on a batch of folios.

**Parameters**

`struct folio_batch *folios`
:   The folios.

`unsigned int *refs`
:   The number of refs to subtract from each folio.

**Description**

Like [`folio_put()`](#c.folio_put "folio_put"), but for a batch of folios. This is more efficient
than writing the loop yourself as it will optimise the locks which need
to be taken if the folios are freed. The folios batch is returned
empty and ready to be reused for another batch; there is no need
to reinitialise it. If **refs** is NULL, we subtract one from each
folio refcount.

**Context**

May be called in process or interrupt context, but not in NMI
context. May be called while holding a spinlock.

void release\_pages(release\_pages\_arg arg, int nr)
:   batched `put_page()`

**Parameters**

`release_pages_arg arg`
:   array of pages to release

`int nr`
:   number of pages

**Description**

Decrement the reference count on all the pages in **arg**. If it
fell to zero, remove the page from the LRU and free it.

Note that the argument can be an array of pages, encoded pages,
or folio pointers. We ignore any encoded bits, and turn any of
them into just a folio that gets free’d.

void folio\_batch\_remove\_exceptionals(struct folio\_batch \*fbatch)
:   Prune non-folios from a batch.

**Parameters**

`struct folio_batch *fbatch`
:   The batch to prune

**Description**

`find_get_entries()` fills a batch with both folios and shadow/swap/DAX
entries. This function prunes all the non-folio entries from **fbatch**
without leaving holes, so that it can be passed on to folio-only batch
operations.

struct cgroup\_subsys\_state \*get\_mem\_cgroup\_css\_from\_folio(struct [folio](#c.get_mem_cgroup_css_from_folio "folio") \*folio)
:   acquire a css of the memcg associated with a folio

**Parameters**

`struct folio *folio`
:   folio of interest

**Description**

If memcg is bound to the default hierarchy, css of the memcg associated
with **folio** is returned. The returned css remains associated with **folio**
until it is released.

If memcg is bound to a traditional hierarchy, the css of root\_mem\_cgroup
is returned.

ino\_t page\_cgroup\_ino(struct [page](#c.page_cgroup_ino "page") \*page)
:   return inode number of the memcg a page is charged to

**Parameters**

`struct page *page`
:   the page

**Description**

Look up the closest online ancestor of the memory cgroup **page** is charged to
and return its inode number or 0 if **page** is not charged to any cgroup. It
is safe to call this function without holding a reference to **page**.

Note, this function is inherently racy, because there is nothing to prevent
the cgroup inode from getting torn down and potentially reallocated a moment
after [`page_cgroup_ino()`](#c.page_cgroup_ino "page_cgroup_ino") returns, so it only should be used by callers that
do not care (such as procfs interfaces).

void mod\_memcg\_state(struct mem\_cgroup \*memcg, enum memcg\_stat\_item idx, int val)
:   update cgroup memory statistics

**Parameters**

`struct mem_cgroup *memcg`
:   the memory cgroup

`enum memcg_stat_item idx`
:   the stat item - can be `enum memcg_stat_item` or `enum node_stat_item`

`int val`
:   delta to add to the counter, can be negative

void mod\_lruvec\_state(struct [lruvec](#c.mod_lruvec_state "lruvec") \*lruvec, enum node\_stat\_item idx, int val)
:   update lruvec memory statistics

**Parameters**

`struct lruvec *lruvec`
:   the lruvec

`enum node_stat_item idx`
:   the stat item

`int val`
:   delta to add to the counter, can be negative

**Description**

The lruvec is the intersection of the NUMA node and a cgroup. This
function updates the all three counters that are affected by a
change of state at this level: per-node, per-cgroup, per-lruvec.

void count\_memcg\_events(struct mem\_cgroup \*memcg, enum vm\_event\_item idx, unsigned long count)
:   account VM events in a cgroup

**Parameters**

`struct mem_cgroup *memcg`
:   the memory cgroup

`enum vm_event_item idx`
:   the event item

`unsigned long count`
:   the number of events that occurred

struct mem\_cgroup \*get\_mem\_cgroup\_from\_mm(struct mm\_struct \*mm)
:   Obtain a reference on given mm\_struct’s memcg.

**Parameters**

`struct mm_struct *mm`
:   mm from which memcg should be extracted. It can be NULL.

**Description**

Obtain a reference on mm->memcg and returns it if successful. If mm
is NULL, then the memcg is chosen as follows:
1) The active memcg, if set.
2) current->mm->memcg, if available
3) root memcg
If mem\_cgroup is disabled, NULL is returned.

struct mem\_cgroup \*get\_mem\_cgroup\_from\_current(void)
:   Obtain a reference on current task’s memcg.

**Parameters**

`void`
:   no arguments

struct mem\_cgroup \*get\_mem\_cgroup\_from\_folio(struct [folio](#c.get_mem_cgroup_from_folio "folio") \*folio)
:   Obtain a reference on a given folio’s memcg.

**Parameters**

`struct folio *folio`
:   folio from which memcg should be extracted.

**Description**

See `folio_memcg()` for folio->objcg/memcg binding rules.

struct mem\_cgroup \*mem\_cgroup\_iter(struct mem\_cgroup \*root, struct mem\_cgroup \*prev, struct mem\_cgroup\_reclaim\_cookie \*reclaim)
:   iterate over memory cgroup hierarchy

**Parameters**

`struct mem_cgroup *root`
:   hierarchy root

`struct mem_cgroup *prev`
:   previously returned memcg, NULL on first invocation

`struct mem_cgroup_reclaim_cookie *reclaim`
:   cookie for shared reclaim walks, NULL for full walks

**Description**

Returns references to children of the hierarchy below **root**, or
**root** itself, or `NULL` after a full round-trip.

Caller must pass the return value in **prev** on subsequent
invocations for reference counting, or use [`mem_cgroup_iter_break()`](#c.mem_cgroup_iter_break "mem_cgroup_iter_break")
to cancel a hierarchy walk before the round-trip is complete.

Reclaimers can specify a node in **reclaim** to divide up the memcgs
in the hierarchy among all concurrent reclaimers operating on the
same node.

void mem\_cgroup\_iter\_break(struct mem\_cgroup \*root, struct mem\_cgroup \*prev)
:   abort a hierarchy walk prematurely

**Parameters**

`struct mem_cgroup *root`
:   hierarchy root

`struct mem_cgroup *prev`
:   last visited hierarchy member as returned by [`mem_cgroup_iter()`](#c.mem_cgroup_iter "mem_cgroup_iter")

void mem\_cgroup\_scan\_tasks(struct mem\_cgroup \*memcg, int (\*fn)(struct task\_struct\*, void\*), void \*arg)
:   iterate over tasks of a memory cgroup hierarchy

**Parameters**

`struct mem_cgroup *memcg`
:   hierarchy root

`int (*fn)(struct task_struct *, void *)`
:   function to call for each task

`void *arg`
:   argument passed to **fn**

**Description**

This function iterates over tasks attached to **memcg** or to any of its
descendants and calls **fn** for each task. If **fn** returns a non-zero
value, the function breaks the iteration loop. Otherwise, it will iterate
over all tasks and return 0.

This function must not be called for the root memory cgroup.

struct lruvec \*folio\_lruvec\_lock(struct [folio](#c.folio_lruvec_lock "folio") \*folio)
:   Lock the lruvec for a folio.

**Parameters**

`struct folio *folio`
:   Pointer to the folio.

**Description**

These functions are safe to use under any of the following conditions:
- folio locked
- folio\_test\_lru false
- folio frozen (refcount of 0)

**Return**

The lruvec this folio is on with its lock held and rcu read lock held.

struct lruvec \*folio\_lruvec\_lock\_irq(struct [folio](#c.folio_lruvec_lock_irq "folio") \*folio)
:   Lock the lruvec for a folio.

**Parameters**

`struct folio *folio`
:   Pointer to the folio.

**Description**

These functions are safe to use under any of the following conditions:
- folio locked
- folio\_test\_lru false
- folio frozen (refcount of 0)

**Return**

The lruvec this folio is on with its lock held and interrupts
disabled and rcu read lock held.

struct lruvec \*folio\_lruvec\_lock\_irqsave(struct [folio](#c.folio_lruvec_lock_irqsave "folio") \*folio, unsigned long \*flags)
:   Lock the lruvec for a folio.

**Parameters**

`struct folio *folio`
:   Pointer to the folio.

`unsigned long *flags`
:   Pointer to irqsave flags.

**Description**

These functions are safe to use under any of the following conditions:
- folio locked
- folio\_test\_lru false
- folio frozen (refcount of 0)

**Return**

The lruvec this folio is on with its lock held and interrupts
disabled and rcu read lock held.

void mem\_cgroup\_update\_lru\_size(struct [lruvec](#c.mem_cgroup_update_lru_size "lruvec") \*lruvec, enum lru\_list lru, int zid, long nr\_pages)
:   account for adding or removing an lru page

**Parameters**

`struct lruvec *lruvec`
:   mem\_cgroup per zone lru vector

`enum lru_list lru`
:   index of lru list the page is sitting on

`int zid`
:   zone id of the accounted pages

`long nr_pages`
:   positive when adding or negative when removing

**Description**

This function must be called under lru\_lock, just before a page is added
to or just after a page is removed from an lru list.

unsigned long mem\_cgroup\_margin(struct mem\_cgroup \*memcg)
:   calculate chargeable space of a memory cgroup

**Parameters**

`struct mem_cgroup *memcg`
:   the memory cgroup

**Description**

Returns the maximum amount of memory **mem** can be charged with, in
pages.

void mem\_cgroup\_print\_oom\_context(struct mem\_cgroup \*memcg, struct task\_struct \*p)
:   Print OOM information relevant to memory controller.

**Parameters**

`struct mem_cgroup *memcg`
:   The memory cgroup that went over limit

`struct task_struct *p`
:   Task that is going to be killed

**NOTE**

**memcg** and **p**’s mem\_cgroup can be different when hierarchy is
enabled

void mem\_cgroup\_print\_oom\_meminfo(struct mem\_cgroup \*memcg)
:   Print OOM memory information relevant to memory controller.

**Parameters**

`struct mem_cgroup *memcg`
:   The memory cgroup that went over limit

struct mem\_cgroup \*mem\_cgroup\_get\_oom\_group(struct task\_struct \*victim, struct mem\_cgroup \*oom\_domain)
:   get a memory cgroup to clean up after OOM

**Parameters**

`struct task_struct *victim`
:   task to be killed by the OOM killer

`struct mem_cgroup *oom_domain`
:   memcg in case of memcg OOM, NULL in case of system-wide OOM

**Description**

Returns a pointer to a memory cgroup, which has to be cleaned up
by killing all belonging OOM-killable tasks.

Caller has to call `mem_cgroup_put()` on the returned non-NULL memcg.

bool consume\_stock(struct mem\_cgroup \*memcg, unsigned int nr\_pages)
:   Try to consume stocked charge on this cpu.

**Parameters**

`struct mem_cgroup *memcg`
:   memcg to consume from.

`unsigned int nr_pages`
:   how many pages to charge.

**Description**

Consume the cached charge if enough nr\_pages are present otherwise return
failure. Also return failure for charge request larger than
MEMCG\_CHARGE\_BATCH or if the local lock is already taken.

returns true if successful, false otherwise.

int \_\_memcg\_kmem\_charge\_page(struct [page](#c.__memcg_kmem_charge_page "page") \*page, gfp\_t gfp, int order)
:   charge a kmem page to the current memory cgroup

**Parameters**

`struct page *page`
:   page to charge

`gfp_t gfp`
:   reclaim mode

`int order`
:   allocation order

**Description**

Returns 0 on success, an error code on failure.

void \_\_memcg\_kmem\_uncharge\_page(struct [page](#c.__memcg_kmem_uncharge_page "page") \*page, int order)
:   uncharge a kmem page

**Parameters**

`struct page *page`
:   page to uncharge

`int order`
:   allocation order

void mem\_cgroup\_wb\_stats(struct bdi\_writeback \*wb, unsigned long \*pfilepages, unsigned long \*pheadroom, unsigned long \*pdirty, unsigned long \*pwriteback)
:   retrieve writeback related stats from its memcg

**Parameters**

`struct bdi_writeback *wb`
:   bdi\_writeback in question

`unsigned long *pfilepages`
:   out parameter for number of file pages

`unsigned long *pheadroom`
:   out parameter for number of allocatable pages according to memcg

`unsigned long *pdirty`
:   out parameter for number of dirty pages

`unsigned long *pwriteback`
:   out parameter for number of pages under writeback

**Description**

Determine the numbers of file, headroom, dirty, and writeback pages in
**wb**’s memcg. File, dirty and writeback are self-explanatory. Headroom
is a bit more involved.

A memcg’s headroom is “min(max, high) - used”. In the hierarchy, the
headroom is calculated as the lowest headroom of itself and the
ancestors. Note that this doesn’t consider the actual amount of
available memory in the system. The caller should further cap
**\*pheadroom** accordingly.

struct mem\_cgroup \*mem\_cgroup\_from\_private\_id(unsigned short id)
:   look up a memcg from a memcg id

**Parameters**

`unsigned short id`
:   the memcg id to look up

**Description**

Caller must hold [`rcu_read_lock()`](kernel-api.html#c.rcu_read_lock "rcu_read_lock").

void mem\_cgroup\_css\_reset(struct cgroup\_subsys\_state \*css)
:   reset the states of a mem\_cgroup

**Parameters**

`struct cgroup_subsys_state *css`
:   the target css

**Description**

Reset the states of the mem\_cgroup associated with **css**. This is
invoked when the userland requests disabling on the default hierarchy
but the memcg is pinned through dependency. The memcg should stop
applying policies and should revert to the vanilla state as it may be
made visible again.

The current implementation only resets the essential configurations.
This needs to be expanded to cover all the visible parts.

void mem\_cgroup\_calculate\_protection(struct mem\_cgroup \*root, struct mem\_cgroup \*memcg)
:   check if memory consumption is in the normal range

**Parameters**

`struct mem_cgroup *root`
:   the top ancestor of the sub-tree being checked

`struct mem_cgroup *memcg`
:   the memory cgroup to check

**Description**

WARNING: This function is not stateless! It can only be used as part
:   of a top-down tree iteration, not for isolated queries.

int mem\_cgroup\_charge\_hugetlb(struct [folio](#c.mem_cgroup_charge_hugetlb "folio") \*folio, gfp\_t gfp)
:   charge the memcg for a hugetlb folio

**Parameters**

`struct folio *folio`
:   folio being charged

`gfp_t gfp`
:   reclaim mode

**Description**

This function is called when allocating a huge page folio, after the page has
already been obtained and charged to the appropriate hugetlb cgroup
controller (if it is enabled).

Returns ENOMEM if the memcg is already full.
Returns 0 if either the charge was successful, or if we skip the charging.

int mem\_cgroup\_swapin\_charge\_folio(struct [folio](#c.mem_cgroup_swapin_charge_folio "folio") \*folio, struct mm\_struct \*mm, gfp\_t gfp, swp\_entry\_t entry)
:   Charge a newly allocated folio for swapin.

**Parameters**

`struct folio *folio`
:   folio to charge.

`struct mm_struct *mm`
:   mm context of the victim

`gfp_t gfp`
:   reclaim mode

`swp_entry_t entry`
:   swap entry for which the folio is allocated

**Description**

This function charges a folio allocated for swapin. Please call this before
adding the folio to the swapcache.

Returns 0 on success. Otherwise, an error code is returned.

void mem\_cgroup\_replace\_folio(struct [folio](#c.folio "folio") \*old, struct [folio](#c.folio "folio") \*new)
:   Charge a folio’s replacement.

**Parameters**

`struct folio *old`
:   Currently circulating folio.

`struct folio *new`
:   Replacement folio.

**Description**

Charge **new** as a replacement folio for **old**. **old** will
be uncharged upon free.

Both folios must be locked, **new->mapping** must be set up.

void mem\_cgroup\_migrate(struct [folio](#c.folio "folio") \*old, struct [folio](#c.folio "folio") \*new)
:   Transfer the memcg data from the old to the new folio.

**Parameters**

`struct folio *old`
:   Currently circulating folio.

`struct folio *new`
:   Replacement folio.

**Description**

Transfer the memcg data from the old folio to the new folio for migration.
The old folio’s data info will be cleared. Note that the memory counters
will remain unchanged throughout the process.

Both folios must be locked, **new->mapping** must be set up.

bool mem\_cgroup\_sk\_charge(const struct [sock](../networking/kapi.html#c.sock "sock") \*sk, unsigned int nr\_pages, gfp\_t gfp\_mask)
:   charge socket memory

**Parameters**

`const struct sock *sk`
:   socket in memcg to charge

`unsigned int nr_pages`
:   number of pages to charge

`gfp_t gfp_mask`
:   reclaim mode

**Description**

Charges **nr\_pages** to **memcg**. Returns `true` if the charge fit within
**memcg**’s configured limit, `false` if it doesn’t.

void mem\_cgroup\_sk\_uncharge(const struct [sock](../networking/kapi.html#c.sock "sock") \*sk, unsigned int nr\_pages)
:   uncharge socket memory

**Parameters**

`const struct sock *sk`
:   socket in memcg to uncharge

`unsigned int nr_pages`
:   number of pages to uncharge

int \_\_mem\_cgroup\_try\_charge\_swap(struct [folio](#c.__mem_cgroup_try_charge_swap "folio") \*folio, swp\_entry\_t entry)
:   try charging swap space for a folio

**Parameters**

`struct folio *folio`
:   folio being added to swap

`swp_entry_t entry`
:   swap entry to charge

**Description**

Try to charge **folio**’s memcg for the swap space at **entry**.

Returns 0 on success, -ENOMEM on failure.

void \_\_mem\_cgroup\_uncharge\_swap(swp\_entry\_t entry, unsigned int nr\_pages)
:   uncharge swap space

**Parameters**

`swp_entry_t entry`
:   swap entry to uncharge

`unsigned int nr_pages`
:   the amount of swap space to uncharge

bool obj\_cgroup\_may\_zswap(struct obj\_cgroup \*objcg)
:   check if this cgroup can zswap

**Parameters**

`struct obj_cgroup *objcg`
:   the object cgroup

**Description**

Check if the hierarchical zswap limit has been reached.

This doesn’t check for specific headroom, and it is not atomic
either. But with zswap, the size of the allocation is only known
once compression has occurred, and this optimistic pre-check avoids
spending cycles on compression when there is already no room left
or zswap is disabled altogether somewhere in the hierarchy.

void obj\_cgroup\_charge\_zswap(struct obj\_cgroup \*objcg, size\_t size)
:   charge compression backend memory

**Parameters**

`struct obj_cgroup *objcg`
:   the object cgroup

`size_t size`
:   size of compressed object

**Description**

This forces the charge after [`obj_cgroup_may_zswap()`](#c.obj_cgroup_may_zswap "obj_cgroup_may_zswap") allowed
compression and storage in zswap for this cgroup to go ahead.

void obj\_cgroup\_uncharge\_zswap(struct obj\_cgroup \*objcg, size\_t size)
:   uncharge compression backend memory

**Parameters**

`struct obj_cgroup *objcg`
:   the object cgroup

`size_t size`
:   size of compressed object

**Description**

Uncharges zswap memory on page in.

bool shmem\_recalc\_inode(struct [inode](#c.shmem_recalc_inode "inode") \*inode, long alloced, long swapped)
:   recalculate the block usage of an inode

**Parameters**

`struct inode *inode`
:   inode to recalc

`long alloced`
:   the change in number of pages allocated to inode

`long swapped`
:   the change in number of pages swapped from inode

**Description**

We have to calculate the free blocks since the mm can drop
undirtied hole pages behind our back.

But normally info->alloced == inode->i\_mapping->nrpages + info->swapped
So mm freed is info->alloced - (inode->i\_mapping->nrpages + info->swapped)

**Return**

true if swapped was incremented from 0, for [`shmem_writeout()`](#c.shmem_writeout "shmem_writeout").

int shmem\_writeout(struct [folio](#c.shmem_writeout "folio") \*folio, struct swap\_iocb \*\*plug, struct list\_head \*folio\_list)
:   Write the folio to swap

**Parameters**

`struct folio *folio`
:   The folio to write

`struct swap_iocb **plug`
:   swap plug

`struct list_head *folio_list`
:   list to put back folios on split

**Description**

Move the folio from the page cache to the swap cache.

int shmem\_get\_folio(struct [inode](#c.shmem_get_folio "inode") \*inode, pgoff\_t index, loff\_t write\_end, struct [folio](#c.folio "folio") \*\*foliop, enum sgp\_type sgp)
:   find, and lock a shmem folio.

**Parameters**

`struct inode *inode`
:   inode to search

`pgoff_t index`
:   the page index.

`loff_t write_end`
:   end of a write, could extend inode size

`struct folio **foliop`
:   pointer to the folio if found

`enum sgp_type sgp`
:   SGP\_\* flags to control behavior

**Description**

Looks up the page cache entry at **inode** & **index**. If a folio is
present, it is returned locked with an increased refcount.

If the caller modifies data in the folio, it must call [`folio_mark_dirty()`](#c.folio_mark_dirty "folio_mark_dirty")
before unlocking the folio to ensure that the folio is not reclaimed.
There is no need to reserve space before calling [`folio_mark_dirty()`](#c.folio_mark_dirty "folio_mark_dirty").

When no folio is found, the behavior depends on **sgp**:
:   * for SGP\_READ, **\*foliop** is `NULL` and 0 is returned
    * for SGP\_NOALLOC, **\*foliop** is `NULL` and -ENOENT is returned
    * for all other flags a new folio is allocated, inserted into the
      page cache and returned locked in **foliop**.

**Context**

May sleep.

**Return**

0 if successful, else a negative error code.

struct [file](../filesystems/api-summary.html#c.file "file") \*shmem\_kernel\_file\_setup(const char \*name, loff\_t size, vma\_flags\_t flags)
:   get an unlinked file living in tmpfs which must be kernel internal. There will be NO LSM permission checks against the underlying inode. So users of this interface must do LSM checks at a higher layer. The users are the big\_key and shm implementations. LSM checks are provided at the key or shm level rather than the inode.

**Parameters**

`const char *name`
:   name for dentry (to be seen in /proc/<pid>/maps)

`loff_t size`
:   size to be set for the file

`vma_flags_t flags`
:   VMA\_NORESERVE\_BIT suppresses pre-accounting of the entire object size

struct [file](../filesystems/api-summary.html#c.file "file") \*shmem\_file\_setup(const char \*name, loff\_t size, vma\_flags\_t flags)
:   get an unlinked file living in tmpfs

**Parameters**

`const char *name`
:   name for dentry (to be seen in /proc/<pid>/maps)

`loff_t size`
:   size to be set for the file

`vma_flags_t flags`
:   VMA\_NORESERVE\_BIT suppresses pre-accounting of the entire object size

struct [file](../filesystems/api-summary.html#c.file "file") \*shmem\_file\_setup\_with\_mnt(struct vfsmount \*mnt, const char \*name, loff\_t size, vma\_flags\_t flags)
:   get an unlinked file living in tmpfs

**Parameters**

`struct vfsmount *mnt`
:   the tmpfs mount where the file will be created

`const char *name`
:   name for dentry (to be seen in /proc/<pid>/maps)

`loff_t size`
:   size to be set for the file

`vma_flags_t flags`
:   VMA\_NORESERVE\_BIT suppresses pre-accounting of the entire object size

int shmem\_zero\_setup(struct vm\_area\_struct \*vma)
:   setup a shared anonymous mapping

**Parameters**

`struct vm_area_struct *vma`
:   the vma to be mmapped is prepared by do\_mmap

**Return**

0 on success, or error

int shmem\_zero\_setup\_desc(struct vm\_area\_desc \*desc)
:   same as shmem\_zero\_setup, but determined by VMA descriptor for convenience.

**Parameters**

`struct vm_area_desc *desc`
:   Describes VMA

**Return**

0 on success, or error

struct [folio](#c.folio "folio") \*shmem\_read\_folio\_gfp(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t index, gfp\_t gfp)
:   read into page cache, using specified page allocation flags.

**Parameters**

`struct address_space *mapping`
:   the folio’s address\_space

`pgoff_t index`
:   the folio index

`gfp_t gfp`
:   the page allocator flags to use if allocating

**Description**

This behaves as a tmpfs “read\_cache\_page\_gfp(mapping, index, gfp)”,
with any new page allocations done using the specified allocation flags.
But [`read_cache_page_gfp()`](#c.read_cache_page_gfp "read_cache_page_gfp") uses the ->`read_folio()` method: which does not
suit tmpfs, since it may have pages in swapcache, and needs to find those
for itself; although drivers/gpu/drm i915 and ttm rely upon this support.

`i915_gem_object_get_pages_gtt()` mixes \_\_GFP\_NORETRY | \_\_GFP\_NOWARN in
with the `mapping_gfp_mask()`, to avoid OOMing the machine unnecessarily.

int migrate\_vma\_split\_folio(struct [folio](#c.migrate_vma_split_folio "folio") \*folio, struct page \*fault\_page)
:   Helper function to split a THP folio

**Parameters**

`struct folio *folio`
:   the folio to split

`struct page *fault_page`
:   `struct page` associated with the fault if any

**Description**

Returns 0 on success

int migrate\_vma\_setup(struct migrate\_vma \*args)
:   prepare to migrate a range of memory

**Parameters**

`struct migrate_vma *args`
:   contains the vma, start, and pfns arrays for the migration

**Return**

negative errno on failures, 0 when 0 or more pages were migrated
without an error.

**Description**

Prepare to migrate a range of memory virtual address range by collecting all
the pages backing each virtual address in the range, saving them inside the
src array. Then lock those pages and unmap them. Once the pages are locked
and unmapped, check whether each page is pinned or not. Pages that aren’t
pinned have the MIGRATE\_PFN\_MIGRATE flag set (by this function) in the
corresponding src array entry. Then restores any pages that are pinned, by
remapping and unlocking those pages.

The caller should then allocate destination memory and copy source memory to
it for all those entries (ie with MIGRATE\_PFN\_VALID and MIGRATE\_PFN\_MIGRATE
flag set). Once these are allocated and copied, the caller must update each
corresponding entry in the dst array with the pfn value of the destination
page and with MIGRATE\_PFN\_VALID. Destination pages must be locked via
[`lock_page()`](#c.lock_page "lock_page").

Note that the caller does not have to migrate all the pages that are marked
with MIGRATE\_PFN\_MIGRATE flag in src array unless this is a migration from
device memory to system memory. If the caller cannot migrate a device page
back to system memory, then it must return VM\_FAULT\_SIGBUS, which has severe
consequences for the userspace process, so it must be avoided if at all
possible.

For empty entries inside CPU page table (`pte_none()` or `pmd_none()` is true) we
do set MIGRATE\_PFN\_MIGRATE flag inside the corresponding source array thus
allowing the caller to allocate device memory for those unbacked virtual
addresses. For this the caller simply has to allocate device memory and
properly set the destination entry like for regular migration. Note that
this can still fail, and thus inside the device driver you must check if the
migration was successful for those entries after calling [`migrate_vma_pages()`](#c.migrate_vma_pages "migrate_vma_pages"),
just like for regular migration.

After that, the callers must call [`migrate_vma_pages()`](#c.migrate_vma_pages "migrate_vma_pages") to go over each entry
in the src array that has the MIGRATE\_PFN\_VALID and MIGRATE\_PFN\_MIGRATE flag
set. If the corresponding entry in dst array has MIGRATE\_PFN\_VALID flag set,
then [`migrate_vma_pages()`](#c.migrate_vma_pages "migrate_vma_pages") to migrate `struct page` information from the source
`struct page` to the destination `struct page`. If it fails to migrate the
`struct page` information, then it clears the MIGRATE\_PFN\_MIGRATE flag in the
src array.

At this point all successfully migrated pages have an entry in the src
array with MIGRATE\_PFN\_VALID and MIGRATE\_PFN\_MIGRATE flag set and the dst
array entry with MIGRATE\_PFN\_VALID flag set.

Once [`migrate_vma_pages()`](#c.migrate_vma_pages "migrate_vma_pages") returns the caller may inspect which pages were
successfully migrated, and which were not. Successfully migrated pages will
have the MIGRATE\_PFN\_MIGRATE flag set for their src array entry.

It is safe to update device page table after [`migrate_vma_pages()`](#c.migrate_vma_pages "migrate_vma_pages") because
both destination and source page are still locked, and the mmap\_lock is held
in read mode (hence no one can unmap the range being migrated).

Once the caller is done cleaning up things and updating its page table (if it
chose to do so, this is not an obligation) it finally calls
[`migrate_vma_finalize()`](#c.migrate_vma_finalize "migrate_vma_finalize") to update the CPU page table to point to new pages
for successfully migrated pages or otherwise restore the CPU page table to
point to the original source pages.

int migrate\_vma\_insert\_huge\_pmd\_page(struct migrate\_vma \*migrate, unsigned long addr, struct [page](#c.migrate_vma_insert_huge_pmd_page "page") \*page, unsigned long \*src, pmd\_t \*pmdp)
:   Insert a huge folio into **migrate->vma->vm\_mm** at **addr**. folio is already allocated as a part of the migration process with large page.

**Parameters**

`struct migrate_vma *migrate`
:   migrate\_vma arguments

`unsigned long addr`
:   address where the folio will be inserted

`struct page *page`
:   page to be inserted at **addr**

`unsigned long *src`
:   src pfn which is being migrated

`pmd_t *pmdp`
:   pointer to the pmd

**Description**

**page** needs to be initialized and setup after it’s allocated. The code bits
here follow closely the code in `__do_huge_pmd_anonymous_page()`. This API does
not support THP zero pages.

void migrate\_device\_pages(unsigned long \*src\_pfns, unsigned long \*dst\_pfns, unsigned long npages)
:   migrate meta-data from src page to dst page

**Parameters**

`unsigned long *src_pfns`
:   src\_pfns returned from [`migrate_device_range()`](#c.migrate_device_range "migrate_device_range")

`unsigned long *dst_pfns`
:   array of pfns allocated by the driver to migrate memory to

`unsigned long npages`
:   number of pages in the range

**Description**

Equivalent to [`migrate_vma_pages()`](#c.migrate_vma_pages "migrate_vma_pages"). This is called to migrate `struct page`
meta-data from source `struct page` to destination.

void migrate\_vma\_pages(struct migrate\_vma \*migrate)
:   migrate meta-data from src page to dst page

**Parameters**

`struct migrate_vma *migrate`
:   migrate `struct containing` all migration information

**Description**

This migrates `struct page` meta-data from source `struct page` to destination
`struct page`. This effectively finishes the migration from source page to the
destination page.

void migrate\_vma\_finalize(struct migrate\_vma \*migrate)
:   restore CPU page table entry

**Parameters**

`struct migrate_vma *migrate`
:   migrate `struct containing` all migration information

**Description**

This replaces the special migration pte entry with either a mapping to the
new page if migration was successful for that page, or to the original page
otherwise.

This also unlocks the pages and puts them back on the lru, or drops the extra
refcount, for device pages.

int migrate\_device\_range(unsigned long \*src\_pfns, unsigned long start, unsigned long npages)
:   migrate device private pfns to normal memory.

**Parameters**

`unsigned long *src_pfns`
:   array large enough to hold migrating source device private pfns.

`unsigned long start`
:   starting pfn in the range to migrate.

`unsigned long npages`
:   number of pages to migrate.

**Description**

[`migrate_vma_setup()`](#c.migrate_vma_setup "migrate_vma_setup") is similar in concept to [`migrate_vma_setup()`](#c.migrate_vma_setup "migrate_vma_setup") except that
instead of looking up pages based on virtual address mappings a range of
device pfns that should be migrated to system memory is used instead.

This is useful when a driver needs to free device memory but doesn’t know the
virtual mappings of every page that may be in device memory. For example this
is often the case when a driver is being unloaded or unbound from a device.

Like [`migrate_vma_setup()`](#c.migrate_vma_setup "migrate_vma_setup") this function will take a reference and lock any
migrating pages that aren’t free before unmapping them. Drivers may then
allocate destination pages and start copying data from the device to CPU
memory before calling [`migrate_device_pages()`](#c.migrate_device_pages "migrate_device_pages").

int migrate\_device\_pfns(unsigned long \*src\_pfns, unsigned long npages)
:   migrate device private pfns to normal memory.

**Parameters**

`unsigned long *src_pfns`
:   pre-populated array of source device private pfns to migrate.

`unsigned long npages`
:   number of pages to migrate.

**Description**

Similar to [`migrate_device_range()`](#c.migrate_device_range "migrate_device_range") but supports non-contiguous pre-populated
array of device pages to migrate.

struct wp\_walk
:   Private struct for pagetable walk callbacks

**Definition**:

```
struct wp_walk {
    struct mmu_notifier_range range;
    unsigned long tlbflush_start;
    unsigned long tlbflush_end;
    unsigned long total;
};
```

**Members**

`range`
:   Range for mmu notifiers

`tlbflush_start`
:   Address of first modified pte

`tlbflush_end`
:   Address of last modified pte + 1

`total`
:   Total number of modified ptes

int wp\_pte(pte\_t \*pte, unsigned long addr, unsigned long end, struct mm\_walk \*walk)
:   Write-protect a pte

**Parameters**

`pte_t *pte`
:   Pointer to the pte

`unsigned long addr`
:   The start of protecting virtual address

`unsigned long end`
:   The end of protecting virtual address

`struct mm_walk *walk`
:   pagetable walk callback argument

**Description**

The function write-protects a pte and records the range in
virtual address space of touched ptes for efficient range TLB flushes.

struct clean\_walk
:   Private struct for the clean\_record\_pte function.

**Definition**:

```
struct clean_walk {
    struct wp_walk base;
    pgoff_t bitmap_pgoff;
    unsigned long *bitmap;
    pgoff_t start;
    pgoff_t end;
};
```

**Members**

`base`
:   [`struct wp_walk`](#c.wp_walk "wp_walk") we derive from

`bitmap_pgoff`
:   Address\_space Page offset of the first bit in **bitmap**

`bitmap`
:   Bitmap with one bit for each page offset in the address\_space range
    covered.

`start`
:   Address\_space page offset of first modified pte relative
    to **bitmap\_pgoff**

`end`
:   Address\_space page offset of last modified pte relative
    to **bitmap\_pgoff**

int clean\_record\_pte(pte\_t \*pte, unsigned long addr, unsigned long end, struct mm\_walk \*walk)
:   Clean a pte and record its address space offset in a bitmap

**Parameters**

`pte_t *pte`
:   Pointer to the pte

`unsigned long addr`
:   The start of virtual address to be clean

`unsigned long end`
:   The end of virtual address to be clean

`struct mm_walk *walk`
:   pagetable walk callback argument

**Description**

The function cleans a pte and records the range in
virtual address space of touched ptes for efficient TLB flushes.
It also records dirty ptes in a bitmap representing page offsets
in the address\_space, as well as the first and last of the bits
touched.

unsigned long wp\_shared\_mapping\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t first\_index, pgoff\_t nr)
:   Write-protect all ptes in an address space range

**Parameters**

`struct address_space *mapping`
:   The address\_space we want to write protect

`pgoff_t first_index`
:   The first page offset in the range

`pgoff_t nr`
:   Number of incremental page offsets to cover

**Note**

This function currently skips transhuge page-table entries, since
it’s intended for dirty-tracking on the PTE level. It will warn on
encountering transhuge write-enabled entries, though, and can easily be
extended to handle them as well.

**Return**

The number of ptes actually write-protected. Note that
already write-protected ptes are not counted.

unsigned long clean\_record\_shared\_mapping\_range(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, pgoff\_t first\_index, pgoff\_t nr, pgoff\_t bitmap\_pgoff, unsigned long \*bitmap, pgoff\_t \*start, pgoff\_t \*end)
:   Clean and record all ptes in an address space range

**Parameters**

`struct address_space *mapping`
:   The address\_space we want to clean

`pgoff_t first_index`
:   The first page offset in the range

`pgoff_t nr`
:   Number of incremental page offsets to cover

`pgoff_t bitmap_pgoff`
:   The page offset of the first bit in **bitmap**

`unsigned long *bitmap`
:   Pointer to a bitmap of at least **nr** bits. The bitmap needs to
    cover the whole range **first\_index**..\*\*first\_index\*\* + **nr**.

`pgoff_t *start`
:   Pointer to number of the first set bit in **bitmap**.
    is modified as new bits are set by the function.

`pgoff_t *end`
:   Pointer to the number of the last set bit in **bitmap**.
    none set. The value is modified as new bits are set by the function.

**Description**

When this function returns there is no guarantee that a CPU has
not already dirtied new ptes. However it will not clean any ptes not
reported in the bitmap. The guarantees are as follows:

* All ptes dirty when the function starts executing will end up recorded
  in the bitmap.
* All ptes dirtied after that will either remain dirty, be recorded in the
  bitmap or both.

If a caller needs to make sure all dirty ptes are picked up and none
additional are added, it first needs to write-protect the address-space
range and make sure new writers are blocked in `page_mkwrite()` or
`pfn_mkwrite()`. And then after a TLB flush following the write-protection
pick up all dirty bits.

This function currently skips transhuge page-table entries, since
it’s intended for dirty-tracking on the PTE level. It will warn on
encountering transhuge dirty entries, though, and can easily be extended
to handle them as well.

**Return**

The number of dirty ptes actually cleaned.

bool pcpu\_addr\_in\_chunk(struct pcpu\_chunk \*chunk, void \*addr)
:   check if the address is served from this chunk

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`void *addr`
:   percpu address

**Return**

True if the address is served from this chunk.

bool pcpu\_check\_block\_hint(struct pcpu\_block\_md \*block, int bits, size\_t align)
:   check against the contig hint

**Parameters**

`struct pcpu_block_md *block`
:   block of interest

`int bits`
:   size of allocation

`size_t align`
:   alignment of area (max PAGE\_SIZE)

**Description**

Check to see if the allocation can fit in the block’s contig hint.
Note, a chunk uses the same hints as a block so this can also check against
the chunk’s contig hint.

void pcpu\_next\_md\_free\_region(struct pcpu\_chunk \*chunk, int \*bit\_off, int \*bits)
:   finds the next hint free area

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int *bit_off`
:   chunk offset

`int *bits`
:   size of free area

**Description**

Helper function for pcpu\_for\_each\_md\_free\_region. It checks
block->contig\_hint and performs aggregation across blocks to find the
next hint. It modifies bit\_off and bits in-place to be consumed in the
loop.

void pcpu\_next\_fit\_region(struct pcpu\_chunk \*chunk, int alloc\_bits, int align, int \*bit\_off, int \*bits)
:   finds fit areas for a given allocation request

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int alloc_bits`
:   size of allocation

`int align`
:   alignment of area (max PAGE\_SIZE)

`int *bit_off`
:   chunk offset

`int *bits`
:   size of free area

**Description**

Finds the next free region that is viable for use with a given size and
alignment. This only returns if there is a valid area to be used for this
allocation. block->first\_free is returned if the allocation request fits
within the block to see if the request can be fulfilled prior to the contig
hint.

void \*pcpu\_mem\_zalloc(size\_t size, gfp\_t gfp)
:   allocate memory

**Parameters**

`size_t size`
:   bytes to allocate

`gfp_t gfp`
:   allocation flags

**Description**

Allocate **size** bytes. If **size** is smaller than PAGE\_SIZE,
[`kzalloc()`](#c.kzalloc "kzalloc") is used; otherwise, the equivalent of [`vzalloc()`](#c.vzalloc "vzalloc") is used.
This is to facilitate passing through whitelisted flags. The
returned memory is always zeroed.

**Return**

Pointer to the allocated area on success, NULL on failure.

void pcpu\_mem\_free(void \*ptr)
:   free memory

**Parameters**

`void *ptr`
:   memory to free

**Description**

Free **ptr**. **ptr** should have been allocated using [`pcpu_mem_zalloc()`](#c.pcpu_mem_zalloc "pcpu_mem_zalloc").

void pcpu\_chunk\_relocate(struct pcpu\_chunk \*chunk, int oslot)
:   put chunk in the appropriate chunk slot

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int oslot`
:   the previous slot it was on

**Description**

This function is called after an allocation or free changed **chunk**.
New slot according to the changed state is determined and **chunk** is
moved to the slot. Note that the reserved chunk is never put on
chunk slots.

**Context**

pcpu\_lock.

void pcpu\_block\_update(struct pcpu\_block\_md \*block, int start, int end)
:   updates a block given a free area

**Parameters**

`struct pcpu_block_md *block`
:   block of interest

`int start`
:   start offset in block

`int end`
:   end offset in block

**Description**

Updates a block given a known free area. The region [start, end) is
expected to be the entirety of the free area within a block. Chooses
the best starting offset if the contig hints are equal.

void pcpu\_chunk\_refresh\_hint(struct pcpu\_chunk \*chunk, bool full\_scan)
:   updates metadata about a chunk

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`bool full_scan`
:   if we should scan from the beginning

**Description**

Iterates over the metadata blocks to find the largest contig area.
A full scan can be avoided on the allocation path as this is triggered
if we broke the contig\_hint. In doing so, the scan\_hint will be before
the contig\_hint or after if the scan\_hint == contig\_hint. This cannot
be prevented on freeing as we want to find the largest area possibly
spanning blocks.

void pcpu\_block\_refresh\_hint(struct pcpu\_chunk \*chunk, int index)

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int index`
:   index of the metadata block

**Description**

Scans over the block beginning at first\_free and updates the block
metadata accordingly.

void pcpu\_block\_update\_hint\_alloc(struct pcpu\_chunk \*chunk, int bit\_off, int bits)
:   update hint on allocation path

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int bit_off`
:   chunk offset

`int bits`
:   size of request

**Description**

Updates metadata for the allocation path. The metadata only has to be
refreshed by a full scan iff the chunk’s contig hint is broken. Block level
scans are required if the block’s contig hint is broken.

void pcpu\_block\_update\_hint\_free(struct pcpu\_chunk \*chunk, int bit\_off, int bits)
:   updates the block hints on the free path

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int bit_off`
:   chunk offset

`int bits`
:   size of request

**Description**

Updates metadata for the allocation path. This avoids a blind block
refresh by making use of the block contig hints. If this fails, it scans
forward and backward to determine the extent of the free area. This is
capped at the boundary of blocks.

A chunk update is triggered if a page becomes free, a block becomes free,
or the free spans across blocks. This tradeoff is to minimize iterating
over the block metadata to update chunk\_md->contig\_hint.
chunk\_md->contig\_hint may be off by up to a page, but it will never be more
than the available space. If the contig hint is contained in one block, it
will be accurate.

bool pcpu\_is\_populated(struct pcpu\_chunk \*chunk, int bit\_off, int bits, int \*next\_off)
:   determines if the region is populated

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int bit_off`
:   chunk offset

`int bits`
:   size of area

`int *next_off`
:   return value for the next offset to start searching

**Description**

For atomic allocations, check if the backing pages are populated.

**Return**

Bool if the backing pages are populated.
next\_index is to skip over unpopulated blocks in pcpu\_find\_block\_fit.

int pcpu\_find\_block\_fit(struct pcpu\_chunk \*chunk, int alloc\_bits, size\_t align, bool pop\_only)
:   finds the block index to start searching

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int alloc_bits`
:   size of request in allocation units

`size_t align`
:   alignment of area (max PAGE\_SIZE bytes)

`bool pop_only`
:   use populated regions only

**Description**

Given a chunk and an allocation spec, find the offset to begin searching
for a free region. This iterates over the bitmap metadata blocks to
find an offset that will be guaranteed to fit the requirements. It is
not quite first fit as if the allocation does not fit in the contig hint
of a block or chunk, it is skipped. This errs on the side of caution
to prevent excess iteration. Poor alignment can cause the allocator to
skip over blocks and chunks that have valid free areas.

**Return**

The offset in the bitmap to begin searching.
-1 if no offset is found.

int pcpu\_alloc\_area(struct pcpu\_chunk \*chunk, int alloc\_bits, size\_t align, int start)
:   allocates an area from a pcpu\_chunk

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int alloc_bits`
:   size of request in allocation units

`size_t align`
:   alignment of area (max PAGE\_SIZE)

`int start`
:   bit\_off to start searching

**Description**

This function takes in a **start** offset to begin searching to fit an
allocation of **alloc\_bits** with alignment **align**. It needs to scan
the allocation map because if it fits within the block’s contig hint,
**start** will be block->first\_free. This is an attempt to fill the
allocation prior to breaking the contig hint. The allocation and
boundary maps are updated accordingly if it confirms a valid
free area.

**Return**

Allocated addr offset in **chunk** on success.
-1 if no matching area is found.

int pcpu\_free\_area(struct pcpu\_chunk \*chunk, int off)
:   frees the corresponding offset

**Parameters**

`struct pcpu_chunk *chunk`
:   chunk of interest

`int off`
:   addr offset into chunk

**Description**

This function determines the size of an allocation to free using
the boundary bitmap and clears the allocation map.

**Return**

Number of freed bytes.

struct pcpu\_chunk \*pcpu\_alloc\_first\_chunk(unsigned long tmp\_addr, int map\_size)
:   creates chunks that serve the first chunk

**Parameters**

`unsigned long tmp_addr`
:   the start of the region served

`int map_size`
:   size of the region served

**Description**

This is responsible for creating the chunks that serve the first chunk. The
base\_addr is page aligned down of **tmp\_addr** while the region end is page
aligned up. Offsets are kept track of to determine the region served. All
this is done to appease the bitmap allocator in avoiding partial blocks.

**Return**

Chunk serving the region at **tmp\_addr** of **map\_size**.

void pcpu\_chunk\_populated(struct pcpu\_chunk \*chunk, int page\_start, int page\_end)
:   post-population bookkeeping

**Parameters**

`struct pcpu_chunk *chunk`
:   pcpu\_chunk which got populated

`int page_start`
:   the start page

`int page_end`
:   the end page

**Description**

Pages in [**page\_start**,\*\*page\_end\*\*) have been populated to **chunk**. Update
the bookkeeping information accordingly. Must be called after each
successful population.

void pcpu\_chunk\_depopulated(struct pcpu\_chunk \*chunk, int page\_start, int page\_end)
:   post-depopulation bookkeeping

**Parameters**

`struct pcpu_chunk *chunk`
:   pcpu\_chunk which got depopulated

`int page_start`
:   the start page

`int page_end`
:   the end page

**Description**

Pages in [**page\_start**,\*\*page\_end\*\*) have been depopulated from **chunk**.
Update the bookkeeping information accordingly. Must be called after
each successful depopulation.

struct pcpu\_chunk \*pcpu\_chunk\_addr\_search(void \*addr)
:   determine chunk containing specified address

**Parameters**

`void *addr`
:   address for which the chunk needs to be determined.

**Description**

This is an internal function that handles all but static allocations.
Static percpu address values should never be passed into the allocator.

**Return**

The address of the found chunk.

void \_\_percpu \*pcpu\_alloc(size\_t size, size\_t align, bool reserved, gfp\_t gfp)
:   the percpu allocator

**Parameters**

`size_t size`
:   size of area to allocate in bytes

`size_t align`
:   alignment of area (max PAGE\_SIZE)

`bool reserved`
:   allocate from the reserved chunk if available

`gfp_t gfp`
:   allocation flags

**Description**

Allocate percpu area of **size** bytes aligned at **align**. If **gfp** doesn’t
contain `GFP_KERNEL`, the allocation is atomic. If **gfp** has \_\_GFP\_NOWARN
then no warning will be triggered on invalid or failed allocation
requests.

**Return**

Percpu pointer to the allocated area on success, NULL on failure.

void pcpu\_balance\_free(bool empty\_only)
:   manage the amount of free chunks

**Parameters**

`bool empty_only`
:   free chunks only if there are no populated pages

**Description**

If empty\_only is `false`, reclaim all fully free chunks regardless of the
number of populated pages. Otherwise, only reclaim chunks that have no
populated pages.

**Context**

pcpu\_lock (can be dropped temporarily)

void pcpu\_balance\_populated(void)
:   manage the amount of populated pages

**Parameters**

`void`
:   no arguments

**Description**

Maintain a certain amount of populated pages to satisfy atomic allocations.
It is possible that this is called when physical memory is scarce causing
OOM killer to be triggered. We should avoid doing so until an actual
allocation causes the failure as it is possible that requests can be
serviced from already backed regions.

**Context**

pcpu\_lock (can be dropped temporarily)

void pcpu\_reclaim\_populated(void)
:   scan over to\_depopulate chunks and free empty pages

**Parameters**

`void`
:   no arguments

**Description**

Scan over chunks in the depopulate list and try to release unused populated
pages back to the system. Depopulated chunks are sidelined to prevent
repopulating these pages unless required. Fully free chunks are reintegrated
and freed accordingly (1 is kept around). If we drop below the empty
populated pages threshold, reintegrate the chunk if it has empty free pages.
Each chunk is scanned in the reverse order to keep populated pages close to
the beginning of the chunk.

**Context**

pcpu\_lock (can be dropped temporarily)

void pcpu\_balance\_workfn(struct work\_struct \*work)
:   manage the amount of free chunks and populated pages

**Parameters**

`struct work_struct *work`
:   unused

**Description**

For each chunk type, manage the number of fully free chunks and the number of
populated pages. An important thing to consider is when pages are freed and
how they contribute to the global counts.

void free\_percpu(void \_\_percpu \*ptr)
:   free percpu area

**Parameters**

`void __percpu *ptr`
:   pointer to area to free

**Description**

Free percpu area **ptr**.

**Context**

Can be called from atomic context.

bool is\_kernel\_percpu\_address(unsigned long addr)
:   test whether address is from static percpu area

**Parameters**

`unsigned long addr`
:   address to test

**Description**

Test whether **addr** belongs to in-kernel static percpu area. Module
static percpu areas are not considered. For those, use
`is_module_percpu_address()`.

**Return**

`true` if **addr** is from in-kernel static percpu area, `false` otherwise.

phys\_addr\_t per\_cpu\_ptr\_to\_phys(void \*addr)
:   convert translated percpu address to physical address

**Parameters**

`void *addr`
:   the address to be converted to physical address

**Description**

Given **addr** which is dereferenceable address obtained via one of
percpu access macros, this function translates it into its physical
address. The caller is responsible for ensuring **addr** stays valid
until this function finishes.

percpu allocator has special setup for the first chunk, which currently
supports either embedding in linear address space or vmalloc mapping,
and, from the second one, the backing allocator (currently either vm or
km) provides translation.

The addr can be translated simply without checking if it falls into the
first chunk. But the current code reflects better how percpu allocator
actually works, and the verification can discover both bugs in percpu
allocator itself and [`per_cpu_ptr_to_phys()`](#c.per_cpu_ptr_to_phys "per_cpu_ptr_to_phys") callers. So we keep current
code.

**Return**

The physical address for **addr**.

struct pcpu\_alloc\_info \*pcpu\_alloc\_alloc\_info(int nr\_groups, int nr\_units)
:   allocate percpu allocation info

**Parameters**

`int nr_groups`
:   the number of groups

`int nr_units`
:   the number of units

**Description**

Allocate ai which is large enough for **nr\_groups** groups containing
**nr\_units** units. The returned ai’s groups[0].cpu\_map points to the
cpu\_map array which is long enough for **nr\_units** and filled with
NR\_CPUS. It’s the caller’s responsibility to initialize cpu\_map
pointer of other groups.

**Return**

Pointer to the allocated pcpu\_alloc\_info on success, NULL on
failure.

void pcpu\_free\_alloc\_info(struct pcpu\_alloc\_info \*ai)
:   free percpu allocation info

**Parameters**

`struct pcpu_alloc_info *ai`
:   pcpu\_alloc\_info to free

**Description**

Free **ai** which was allocated by [`pcpu_alloc_alloc_info()`](#c.pcpu_alloc_alloc_info "pcpu_alloc_alloc_info").

void pcpu\_dump\_alloc\_info(const char \*lvl, const struct pcpu\_alloc\_info \*ai)
:   print out information about pcpu\_alloc\_info

**Parameters**

`const char *lvl`
:   loglevel

`const struct pcpu_alloc_info *ai`
:   allocation info to dump

**Description**

Print out information about **ai** using loglevel **lvl**.

void pcpu\_setup\_first\_chunk(const struct pcpu\_alloc\_info \*ai, void \*base\_addr)
:   initialize the first percpu chunk

**Parameters**

`const struct pcpu_alloc_info *ai`
:   pcpu\_alloc\_info describing how to percpu area is shaped

`void *base_addr`
:   mapped address

**Description**

Initialize the first percpu chunk which contains the kernel static
percpu area. This function is to be called from arch percpu area
setup path.

**ai** contains all information necessary to initialize the first
chunk and prime the dynamic percpu allocator.

**ai->static\_size** is the size of static percpu area.

**ai->reserved\_size**, if non-zero, specifies the amount of bytes to
reserve after the static area in the first chunk. This reserves
the first chunk such that it’s available only through reserved
percpu allocation. This is primarily used to serve module percpu
static areas on architectures where the addressing model has
limited offset range for symbol relocations to guarantee module
percpu symbols fall inside the relocatable range.

**ai->dyn\_size** determines the number of bytes available for dynamic
allocation in the first chunk. The area between **ai->static\_size** +
**ai->reserved\_size** + **ai->dyn\_size** and **ai->unit\_size** is unused.

**ai->unit\_size** specifies unit size and must be aligned to PAGE\_SIZE
and equal to or larger than **ai->static\_size** + **ai->reserved\_size** +
**ai->dyn\_size**.

**ai->atom\_size** is the allocation atom size and used as alignment
for vm areas.

**ai->alloc\_size** is the allocation size and always multiple of
**ai->atom\_size**. This is larger than **ai->atom\_size** if
**ai->unit\_size** is larger than **ai->atom\_size**.

**ai->nr\_groups** and **ai->groups** describe virtual memory layout of
percpu areas. Units which should be colocated are put into the
same group. Dynamic VM areas will be allocated according to these
groupings. If **ai->nr\_groups** is zero, a single group containing
all units is assumed.

The caller should have mapped the first chunk at **base\_addr** and
copied static data to each unit.

The first chunk will always contain a static and a dynamic region.
However, the static region is not managed by any chunk. If the first
chunk also contains a reserved region, it is served by two chunks -
one for the reserved region and one for the dynamic region. They
share the same vm, but use offset regions in the area allocation map.
The chunk serving the dynamic region is circulated in the chunk slots
and available for dynamic allocation like any other chunk.

struct pcpu\_alloc\_info \*pcpu\_build\_alloc\_info(size\_t reserved\_size, size\_t dyn\_size, size\_t atom\_size, pcpu\_fc\_cpu\_distance\_fn\_t cpu\_distance\_fn)
:   build alloc\_info considering distances between CPUs

**Parameters**

`size_t reserved_size`
:   the size of reserved percpu area in bytes

`size_t dyn_size`
:   minimum free size for dynamic allocation in bytes

`size_t atom_size`
:   allocation atom size

`pcpu_fc_cpu_distance_fn_t cpu_distance_fn`
:   callback to determine distance between cpus, optional

**Description**

This function determines grouping of units, their mappings to cpus
and other parameters considering needed percpu size, allocation
atom size and distances between CPUs.

Groups are always multiples of atom size and CPUs which are of
LOCAL\_DISTANCE both ways are grouped together and share space for
units in the same group. The returned configuration is guaranteed
to have CPUs on different nodes on different groups and >=75% usage
of allocated virtual address space.

**Return**

On success, pointer to the new allocation\_info is returned. On
failure, ERR\_PTR value is returned.

int pcpu\_embed\_first\_chunk(size\_t reserved\_size, size\_t dyn\_size, size\_t atom\_size, pcpu\_fc\_cpu\_distance\_fn\_t cpu\_distance\_fn, pcpu\_fc\_cpu\_to\_node\_fn\_t cpu\_to\_nd\_fn)
:   embed the first percpu chunk into bootmem

**Parameters**

`size_t reserved_size`
:   the size of reserved percpu area in bytes

`size_t dyn_size`
:   minimum free size for dynamic allocation in bytes

`size_t atom_size`
:   allocation atom size

`pcpu_fc_cpu_distance_fn_t cpu_distance_fn`
:   callback to determine distance between cpus, optional

`pcpu_fc_cpu_to_node_fn_t cpu_to_nd_fn`
:   callback to convert cpu to it’s node, optional

**Description**

This is a helper to ease setting up embedded first percpu chunk and
can be called where [`pcpu_setup_first_chunk()`](#c.pcpu_setup_first_chunk "pcpu_setup_first_chunk") is expected.

If this function is used to setup the first chunk, it is allocated
by calling pcpu\_fc\_alloc and used as-is without being mapped into
vmalloc area. Allocations are always whole multiples of **atom\_size**
aligned to **atom\_size**.

This enables the first chunk to piggy back on the linear physical
mapping which often uses larger page size. Please note that this
can result in very sparse cpu->unit mapping on NUMA machines thus
requiring large vmalloc address space. Don’t use this allocator if
vmalloc space is not orders of magnitude larger than distances
between node memory addresses (ie. 32bit NUMA machines).

**dyn\_size** specifies the minimum dynamic area size.

If the needed size is smaller than the minimum or specified unit
size, the leftover is returned using pcpu\_fc\_free.

**Return**

0 on success, -errno on failure.

int pcpu\_page\_first\_chunk(size\_t reserved\_size, pcpu\_fc\_cpu\_to\_node\_fn\_t cpu\_to\_nd\_fn)
:   map the first chunk using PAGE\_SIZE pages

**Parameters**

`size_t reserved_size`
:   the size of reserved percpu area in bytes

`pcpu_fc_cpu_to_node_fn_t cpu_to_nd_fn`
:   callback to convert cpu to it’s node, optional

**Description**

This is a helper to ease setting up page-remapped first percpu
chunk and can be called where [`pcpu_setup_first_chunk()`](#c.pcpu_setup_first_chunk "pcpu_setup_first_chunk") is expected.

This is the basic allocator. Static percpu area is allocated
page-by-page into vmalloc area.

**Return**

0 on success, -errno on failure.

long copy\_from\_user\_nofault(void \*dst, const void \_\_user \*src, size\_t size)
:   safely attempt to read from a user-space location

**Parameters**

`void *dst`
:   pointer to the buffer that shall take the data

`const void __user *src`
:   address to read from. This must be a user address.

`size_t size`
:   size of the data chunk

**Description**

Safely read from user address **src** to the buffer at **dst**. If a kernel fault
happens, handle that and return -EFAULT.

long copy\_to\_user\_nofault(void \_\_user \*dst, const void \*src, size\_t size)
:   safely attempt to write to a user-space location

**Parameters**

`void __user *dst`
:   address to write to

`const void *src`
:   pointer to the data that shall be written

`size_t size`
:   size of the data chunk

**Description**

Safely write to address **dst** from the buffer at **src**. If a kernel fault
happens, handle that and return -EFAULT.

long strncpy\_from\_user\_nofault(char \*dst, const void \_\_user \*unsafe\_addr, long count)
:   * Copy a NUL terminated string from unsafe user address.

**Parameters**

`char *dst`
:   Destination address, in kernel space. This buffer must be at
    least **count** bytes long.

`const void __user *unsafe_addr`
:   Unsafe user address.

`long count`
:   Maximum number of bytes to copy, including the trailing NUL.

**Description**

Copies a NUL-terminated string from unsafe user address to kernel buffer.

On success, returns the length of the string INCLUDING the trailing NUL.

If access fails, returns -EFAULT (some data may have been copied
and the trailing NUL added).

If **count** is smaller than the length of the string, copies **count**-1 bytes,
sets the last byte of **dst** buffer to NUL and returns **count**.

long strnlen\_user\_nofault(const void \_\_user \*unsafe\_addr, long count)
:   * Get the size of a user string INCLUDING final NUL.

**Parameters**

`const void __user *unsafe_addr`
:   The string to measure.

`long count`
:   Maximum count (including NUL)

**Description**

Get the size of a NUL-terminated string in user space without pagefault.

Returns the size of the string INCLUDING the terminating NUL.

If the string is too long, returns a number larger than **count**. User
has to check the return value against “> count”.
On exception (or invalid count), returns 0.

Unlike strnlen\_user, this can be used from IRQ handler etc. because
it disables pagefaults.

bool writeback\_throttling\_sane(struct scan\_control \*sc)
:   is the usual dirty throttling mechanism available?

**Parameters**

`struct scan_control *sc`
:   scan\_control in question

**Description**

The normal page dirty throttling mechanism in `balance_dirty_pages()` is
completely broken with the legacy memcg and direct stalling in
`shrink_folio_list()` is used for throttling instead, which lacks all the
niceties such as fairness, adaptive pausing, bandwidth proportional
allocation and configurability.

This function tests whether the vmscan currently in progress can assume
that the normal dirty throttling mechanism is operational.

unsigned long lruvec\_lru\_size(struct [lruvec](#c.lruvec_lru_size "lruvec") \*lruvec, enum lru\_list lru, int zone\_idx)
:   Returns the number of pages on the given LRU list.

**Parameters**

`struct lruvec *lruvec`
:   lru vector

`enum lru_list lru`
:   lru to use

`int zone_idx`
:   zones to consider (use MAX\_NR\_ZONES - 1 for the whole LRU list)

long remove\_mapping(struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, struct [folio](#c.remove_mapping "folio") \*folio)
:   Attempt to remove a folio from its mapping.

**Parameters**

`struct address_space *mapping`
:   The address space.

`struct folio *folio`
:   The folio to remove.

**Description**

If the folio is dirty, under writeback or if someone else has a ref
on it, removal will fail.

**Return**

The number of pages removed from the mapping. 0 if the folio
could not be removed.

**Context**

The caller should have a single refcount on the folio and
hold its lock.

void folio\_putback\_lru(struct [folio](#c.folio_putback_lru "folio") \*folio)
:   Put previously isolated folio onto appropriate LRU list.

**Parameters**

`struct folio *folio`
:   Folio to be returned to an LRU list.

**Description**

Add previously isolated **folio** to appropriate LRU list.
The folio may still be unevictable for other reasons.

**Context**

lru\_lock must not be held, interrupts must be enabled.

bool folio\_isolate\_lru(struct [folio](#c.folio_isolate_lru "folio") \*folio)
:   Try to isolate a folio from its LRU list.

**Parameters**

`struct folio *folio`
:   Folio to isolate from its LRU list.

**Description**

Isolate a **folio** from an LRU list and adjust the vmstat statistic
corresponding to whatever LRU list the folio was on.

The folio will have its LRU flag cleared. If it was found on the
active list, it will have the Active flag set. If it was found on the
unevictable list, it will have the Unevictable flag set. These flags
may need to be cleared by the caller before letting the page go.

1. Must be called with an elevated refcount on the folio. This is a
   fundamental difference from `isolate_lru_folios()` (which is called
   without a stable reference).
2. The lru\_lock must not be held.
3. Interrupts must be enabled.

**Context**

**Return**

true if the folio was removed from an LRU list.
false if the folio was not on an LRU list.

void check\_move\_unevictable\_folios(struct folio\_batch \*fbatch)
:   Move evictable folios to appropriate zone lru list

**Parameters**

`struct folio_batch *fbatch`
:   Batch of lru folios to check.

**Description**

Checks folios for evictability, if an evictable folio is in the unevictable
lru list, moves it to the appropriate evictable lru list. This function
should be only used for lru folios.

void \_\_remove\_pages(unsigned long pfn, unsigned long nr\_pages, struct vmem\_altmap \*altmap)
:   remove sections of pages

**Parameters**

`unsigned long pfn`
:   starting pageframe (must be aligned to start of a section)

`unsigned long nr_pages`
:   number of pages to remove (must be multiple of section size)

`struct vmem_altmap *altmap`
:   alternative device page map or `NULL` if default memmap is used

**Description**

Generic helper function to remove section mappings and sysfs entries
for the section of the memory we are removing. Caller needs to make
sure that pages are marked reserved and zones are adjust properly by
calling `offline_pages()`.

void try\_offline\_node(int nid)

**Parameters**

`int nid`
:   the node ID

**Description**

Offline a node if all memory sections and cpus of the node are removed.

**NOTE**

The caller must call `lock_device_hotplug()` to serialize hotplug
and online/offline operations before this call.

void \_\_remove\_memory(u64 start, u64 size)
:   Remove memory if every memory block is offline

**Parameters**

`u64 start`
:   physical address of the region to remove

`u64 size`
:   size of the region to remove

**NOTE**

The caller must call `lock_device_hotplug()` to serialize hotplug
and online/offline operations before this call, as required by
[`try_offline_node()`](#c.try_offline_node "try_offline_node").

unsigned long mmu\_interval\_read\_begin(struct mmu\_interval\_notifier \*interval\_sub)
:   Begin a read side critical section against a VA range

**Parameters**

`struct mmu_interval_notifier *interval_sub`
:   The interval subscription

**Description**

`mmu_iterval_read_begin()`/`mmu_iterval_read_retry()` implement a
collision-retry scheme similar to seqcount for the VA range under
subscription. If the mm invokes invalidation during the critical section
then `mmu_interval_read_retry()` will return true.

This is useful to obtain shadow PTEs where teardown or setup of the SPTEs
require a blocking context. The critical region formed by this can sleep,
and the required ‘user\_lock’ can also be a sleeping lock.

The caller is required to provide a ‘user\_lock’ to serialize both teardown
and setup.

The return value should be passed to `mmu_interval_read_retry()`.

int mmu\_notifier\_register(struct mmu\_notifier \*subscription, struct mm\_struct \*mm)
:   Register a notifier on a mm

**Parameters**

`struct mmu_notifier *subscription`
:   The notifier to attach

`struct mm_struct *mm`
:   The mm to attach the notifier to

**Description**

Must not hold mmap\_lock nor any other VM related lock when calling
this registration function. Must also ensure mm\_users can’t go down
to zero while this runs to avoid races with mmu\_notifier\_release,
so mm has to be current->mm or the mm should be pinned safely such
as with `get_task_mm()`. If the mm is not current->mm, the mm\_users
pin should be released by calling mmput after mmu\_notifier\_register
returns.

`mmu_notifier_unregister()` or [`mmu_notifier_put()`](#c.mmu_notifier_put "mmu_notifier_put") must be always called to
unregister the notifier.

While the caller has a mmu\_notifier get the subscription->mm pointer will remain
valid, and can be converted to an active mm pointer via `mmget_not_zero()`.

struct mmu\_notifier \*mmu\_notifier\_get\_locked(const struct mmu\_notifier\_ops \*ops, struct mm\_struct \*mm)
:   Return the single `struct mmu_notifier` for the mm & ops

**Parameters**

`const struct mmu_notifier_ops *ops`
:   The operations `struct being` subscribe with

`struct mm_struct *mm`
:   The mm to attach notifiers too

**Description**

This function either allocates a new mmu\_notifier via
ops->`alloc_notifier()`, or returns an already existing notifier on the
list. The value of the ops pointer is used to determine when two notifiers
are the same.

Each call to `mmu_notifier_get()` must be paired with a call to
[`mmu_notifier_put()`](#c.mmu_notifier_put "mmu_notifier_put"). The caller must hold the write side of mm->mmap\_lock.

While the caller has a mmu\_notifier get the mm pointer will remain valid,
and can be converted to an active mm pointer via `mmget_not_zero()`.

void mmu\_notifier\_put(struct mmu\_notifier \*subscription)
:   Release the reference on the notifier

**Parameters**

`struct mmu_notifier *subscription`
:   The notifier to act on

**Description**

This function must be paired with each `mmu_notifier_get()`, it releases the
reference obtained by the get. If this is the last reference then process
to free the notifier will be run asynchronously.

Unlike `mmu_notifier_unregister()` the get/put flow only calls ops->release
when the mm\_struct is destroyed. Instead free\_notifier is always called to
release any resources held by the user.

As ops->release is not guaranteed to be called, the user must ensure that
all sptes are dropped, and no new sptes can be established before
[`mmu_notifier_put()`](#c.mmu_notifier_put "mmu_notifier_put") is called.

This function can be called from the ops->release callback, however the
caller must still ensure it is called pairwise with `mmu_notifier_get()`.

Modules calling this function must call [`mmu_notifier_synchronize()`](#c.mmu_notifier_synchronize "mmu_notifier_synchronize") in
their \_\_exit functions to ensure the async work is completed.

int mmu\_interval\_notifier\_insert(struct mmu\_interval\_notifier \*interval\_sub, struct mm\_struct \*mm, unsigned long start, unsigned long length, const struct mmu\_interval\_notifier\_ops \*ops)
:   Insert an interval notifier

**Parameters**

`struct mmu_interval_notifier *interval_sub`
:   Interval subscription to register

`struct mm_struct *mm`
:   mm\_struct to attach to

`unsigned long start`
:   Starting virtual address to monitor

`unsigned long length`
:   Length of the range to monitor

`const struct mmu_interval_notifier_ops *ops`
:   Interval notifier operations to be called on matching events

**Description**

This function subscribes the interval notifier for notifications from the
mm. Upon return the ops related to mmu\_interval\_notifier will be called
whenever an event that intersects with the given range occurs.

Upon return the range\_notifier may not be present in the interval tree yet.
The caller must use the normal interval notifier read flow via
[`mmu_interval_read_begin()`](#c.mmu_interval_read_begin "mmu_interval_read_begin") to establish SPTEs for this range.

void mmu\_interval\_notifier\_remove(struct mmu\_interval\_notifier \*interval\_sub)
:   Remove a interval notifier

**Parameters**

`struct mmu_interval_notifier *interval_sub`
:   Interval subscription to unregister

**Description**

This function must be paired with [`mmu_interval_notifier_insert()`](#c.mmu_interval_notifier_insert "mmu_interval_notifier_insert"). It cannot
be called from any ops callback.

Once this returns ops callbacks are no longer running on other CPUs and
will not be called in future.

void mmu\_notifier\_synchronize(void)
:   Ensure all mmu\_notifiers are freed

**Parameters**

`void`
:   no arguments

**Description**

This function ensures that all outstanding async SRU work from
[`mmu_notifier_put()`](#c.mmu_notifier_put "mmu_notifier_put") is completed. After it returns any mmu\_notifier\_ops
associated with an unused mmu\_notifier will no longer be called.

Before using the caller must ensure that all of its mmu\_notifiers have been
fully released via [`mmu_notifier_put()`](#c.mmu_notifier_put "mmu_notifier_put").

Modules using the [`mmu_notifier_put()`](#c.mmu_notifier_put "mmu_notifier_put") API should call this in their \_\_exit
function to avoid module unloading races.

void balloon\_page\_insert(struct balloon\_dev\_info \*balloon, struct [page](#c.balloon_page_insert "page") \*page)
:   insert a page into the balloon’s page list and make the page->private assignment accordingly.

**Parameters**

`struct balloon_dev_info *balloon`
:   pointer to balloon device

`struct page *page`
:   page to be assigned as a ‘balloon page’

**Description**

Caller must ensure the balloon\_pages\_lock is held.

void balloon\_page\_finalize(struct [page](#c.balloon_page_finalize "page") \*page)
:   prepare a balloon page that was removed from the balloon list for release to the page allocator

**Parameters**

`struct page *page`
:   page to be released to the page allocator

**Description**

Caller must ensure the balloon\_pages\_lock is held.

size\_t balloon\_page\_list\_enqueue(struct balloon\_dev\_info \*b\_dev\_info, struct list\_head \*pages)
:   inserts a list of pages into the balloon page list.

**Parameters**

`struct balloon_dev_info *b_dev_info`
:   balloon device descriptor where we will insert a new page to

`struct list_head *pages`
:   pages to enqueue - allocated using balloon\_page\_alloc.

**Description**

Driver must call this function to properly enqueue balloon pages before
definitively removing them from the guest system.

**Return**

number of pages that were enqueued.

size\_t balloon\_page\_list\_dequeue(struct balloon\_dev\_info \*b\_dev\_info, struct list\_head \*pages, size\_t n\_req\_pages)
:   removes pages from balloon’s page list and returns a list of the pages.

**Parameters**

`struct balloon_dev_info *b_dev_info`
:   balloon device descriptor where we will grab a page from.

`struct list_head *pages`
:   pointer to the list of pages that would be returned to the caller.

`size_t n_req_pages`
:   number of requested pages.

**Description**

Driver must call this function to properly de-allocate a previous enlisted
balloon pages before definitively releasing it back to the guest system.
This function tries to remove **n\_req\_pages** from the ballooned pages and
return them to the caller in the **pages** list.

Note that this function may fail to dequeue some pages even if the balloon
isn’t empty - since the page list can be temporarily empty due to compaction
of isolated pages.

**Return**

number of pages that were added to the **pages** list.

struct page \*balloon\_page\_alloc(void)
:   allocates a new page for insertion into the balloon page list.

**Parameters**

`void`
:   no arguments

**Description**

Driver must call this function to properly allocate a new balloon page.
Driver must call balloon\_page\_enqueue before definitively removing the page
from the guest system.

**Return**

`struct page` for the allocated page or NULL on allocation failure.

void balloon\_page\_enqueue(struct balloon\_dev\_info \*b\_dev\_info, struct [page](#c.balloon_page_enqueue "page") \*page)
:   inserts a new page into the balloon page list.

**Parameters**

`struct balloon_dev_info *b_dev_info`
:   balloon device descriptor where we will insert a new page

`struct page *page`
:   new page to enqueue - allocated using balloon\_page\_alloc.

**Description**

Drivers must call this function to properly enqueue a new allocated balloon
page before definitively removing the page from the guest system.

Drivers must not enqueue pages while page->lru is still in
use, and must not use page->lru until a page was unqueued again.

struct page \*balloon\_page\_dequeue(struct balloon\_dev\_info \*b\_dev\_info)
:   removes a page from balloon’s page list and returns its address to allow the driver to release the page.

**Parameters**

`struct balloon_dev_info *b_dev_info`
:   balloon device descriptor where we will grab a page from.

**Description**

Driver must call this function to properly dequeue a previously enqueued page
before definitively releasing it back to the guest system.

Caller must perform its own accounting to ensure that this
function is called only if some pages are actually enqueued.

Note that this function may fail to dequeue some pages even if there are
some enqueued pages - since the page list can be temporarily empty due to
the compaction of isolated pages.

TODO: remove the caller accounting requirements, and allow caller to wait
until all pages can be dequeued.

**Return**

`struct page` for the dequeued page, or NULL if no page was dequeued.

[vm\_fault\_t](#c.vm_fault_t "vm_fault_t") vmf\_insert\_pfn\_pmd(struct vm\_fault \*vmf, unsigned long pfn, bool write)
:   insert a pmd size pfn

**Parameters**

`struct vm_fault *vmf`
:   Structure describing the fault

`unsigned long pfn`
:   pfn to insert

`bool write`
:   whether it’s a write fault

**Description**

Insert a pmd size pfn. See [`vmf_insert_pfn()`](#c.vmf_insert_pfn "vmf_insert_pfn") for additional info.

**Return**

vm\_fault\_t value.

[vm\_fault\_t](#c.vm_fault_t "vm_fault_t") vmf\_insert\_pfn\_pud(struct vm\_fault \*vmf, unsigned long pfn, bool write)
:   insert a pud size pfn

**Parameters**

`struct vm_fault *vmf`
:   Structure describing the fault

`unsigned long pfn`
:   pfn to insert

`bool write`
:   whether it’s a write fault

**Description**

Insert a pud size pfn. See [`vmf_insert_pfn()`](#c.vmf_insert_pfn "vmf_insert_pfn") for additional info.

**Return**

vm\_fault\_t value.

[vm\_fault\_t](#c.vm_fault_t "vm_fault_t") vmf\_insert\_folio\_pud(struct vm\_fault \*vmf, struct [folio](#c.vmf_insert_folio_pud "folio") \*folio, bool write)
:   insert a pud size folio mapped by a pud entry

**Parameters**

`struct vm_fault *vmf`
:   Structure describing the fault

`struct folio *folio`
:   folio to insert

`bool write`
:   whether it’s a write fault

**Return**

vm\_fault\_t value.

bool touch\_pmd(struct vm\_area\_struct \*vma, unsigned long addr, pmd\_t \*pmd, bool write)
:   Mark page table pmd entry as accessed and dirty (for write)

**Parameters**

`struct vm_area_struct *vma`
:   The VMA covering **addr**

`unsigned long addr`
:   The virtual address

`pmd_t *pmd`
:   pmd pointer into the page table mapping **addr**

`bool write`
:   Whether it’s a write access

**Return**

whether the pmd entry is changed

bool zap\_huge\_pmd(struct mmu\_gather \*tlb, struct vm\_area\_struct \*vma, pmd\_t \*pmd, unsigned long addr)
:   Zap a huge THP which is of PMD size.

**Parameters**

`struct mmu_gather *tlb`
:   The MMU gather TLB state associated with the operation.

`struct vm_area_struct *vma`
:   The VMA containing the range to zap.

`pmd_t *pmd`
:   A pointer to the leaf PMD entry.

`unsigned long addr`
:   The virtual address for the range to zap.

**Return**

`true` on success, `false` otherwise.

int \_\_split\_unmapped\_folio(struct [folio](#c.__split_unmapped_folio "folio") \*folio, int new\_order, struct page \*split\_at, struct xa\_state \*xas, struct [address\_space](../filesystems/api-summary.html#c.address_space "address_space") \*mapping, enum [split\_type](#c.__split_unmapped_folio "split_type") split\_type)
:   splits an unmapped **folio** to lower order folios in two ways: uniform split or non-uniform split.

**Parameters**

`struct folio *folio`
:   the to-be-split folio

`int new_order`
:   the smallest order of the after split folios (since buddy
    allocator like split generates folios with orders from **folio**’s
    order - 1 to new\_order).

`struct page *split_at`
:   in buddy allocator like split, the folio containing **split\_at**
    will be split until its order becomes **new\_order**.

`struct xa_state *xas`
:   xa\_state pointing to folio->mapping->i\_pages and locked by caller

`struct address_space *mapping`
:   **folio->mapping**

`enum split_type split_type`
:   if the split is uniform or not (buddy allocator like split)

**Description**

1. uniform split: the given **folio** into multiple **new\_order** small folios,
   where all small folios have the same order. This is done when
   split\_type is SPLIT\_TYPE\_UNIFORM.
2. buddy allocator like (non-uniform) split: the given **folio** is split into
   half and one of the half (containing the given page) is split into half
   until the given **folio**’s order becomes **new\_order**. This is done when
   split\_type is SPLIT\_TYPE\_NON\_UNIFORM.

The high level flow for these two methods are:

1. uniform split: **xas** is split with no expectation of failure and a single
   `__split_folio_to_order()` is called to split the **folio** into **new\_order**
   along with stats update.
2. non-uniform split: folio\_order - **new\_order** calls to
   `__split_folio_to_order()` are expected to be made in a for loop to split
   the **folio** to one lower order at a time. The folio containing **split\_at**
   is split in each iteration. **xas** is split into half in each iteration and
   can fail. A failed **xas** split leaves split folios as is without merging
   them back.

After splitting, the caller’s folio reference will be transferred to the
folio containing **split\_at**. The caller needs to unlock and/or free
after-split folios if necessary.

**Return**

0 - successful, <0 - failed (if -ENOMEM is returned, **folio** might be
split but not to **new\_order**, the caller needs to check)

int folio\_check\_splittable(struct [folio](#c.folio_check_splittable "folio") \*folio, unsigned int new\_order, enum [split\_type](#c.folio_check_splittable "split_type") split\_type)
:   check if a folio can be split to a given order

**Parameters**

`struct folio *folio`
:   folio to be split

`unsigned int new_order`
:   the smallest order of the after split folios (since buddy
    allocator like split generates folios with orders from **folio**’s
    order - 1 to new\_order).

`enum split_type split_type`
:   uniform or non-uniform split

**Description**

[`folio_check_splittable()`](#c.folio_check_splittable "folio_check_splittable") checks if **folio** can be split to **new\_order** using
**split\_type** method. The truncated folio check must come first.

**Context**

folio must be locked.

**Return**

0 - **folio** can be split to **new\_order**, otherwise an error number is
returned.

int \_\_folio\_split(struct [folio](#c.__folio_split "folio") \*folio, unsigned int new\_order, struct page \*split\_at, struct page \*lock\_at, struct list\_head \*list, enum [split\_type](#c.__folio_split "split_type") split\_type)
:   split a folio at **split\_at** to a **new\_order** folio

**Parameters**

`struct folio *folio`
:   folio to split

`unsigned int new_order`
:   the order of the new folio

`struct page *split_at`
:   a page within the new folio

`struct page *lock_at`
:   a page within **folio** to be left locked to caller

`struct list_head *list`
:   after-split folios will be put on it if non NULL

`enum split_type split_type`
:   perform uniform split or not (non-uniform split)

**Description**

It calls [`__split_unmapped_folio()`](#c.__split_unmapped_folio "__split_unmapped_folio") to perform uniform and non-uniform split.
It is in charge of checking whether the split is supported or not and
preparing **folio** for [`__split_unmapped_folio()`](#c.__split_unmapped_folio "__split_unmapped_folio").

After splitting, the after-split folio containing **lock\_at** remains locked
and others are unlocked:
1. for uniform split, **lock\_at** points to one of **folio**’s subpages;
2. for buddy allocator like (non-uniform) split, **lock\_at** points to **folio**.

**Return**

0 - successful, <0 - failed (if -ENOMEM is returned, **folio** might be
split but not to **new\_order**, the caller needs to check)

int folio\_split\_unmapped(struct [folio](#c.folio_split_unmapped "folio") \*folio, unsigned int new\_order)
:   split a large anon folio that is already unmapped

**Parameters**

`struct folio *folio`
:   folio to split

`unsigned int new_order`
:   the order of folios after split

**Description**

This function is a helper for splitting folios that have already been
unmapped. The use case is that the device or the CPU can refuse to migrate
THP pages in the middle of migration, due to allocation issues on either
side.

anon\_vma\_lock is not required to be held, `mmap_read_lock()` or
`mmap_write_lock()` should be held. **folio** is expected to be locked by the
caller. device-private and non device-private folios are supported along
with folios that are in the swapcache. **folio** should also be unmapped and
isolated from LRU (if applicable)

Upon return, the folio is not remapped, split folios are not added to LRU,
`free_folio_and_swap_cache()` is not called, and new folios remain locked.

**Return**

0 on success, -EAGAIN if the folio cannot be split (e.g., due to
insufficient reference count or extra pins).

int folio\_split(struct [folio](#c.folio_split "folio") \*folio, unsigned int new\_order, struct page \*split\_at, struct list\_head \*list)
:   split a folio at **split\_at** to a **new\_order** folio

**Parameters**

`struct folio *folio`
:   folio to split

`unsigned int new_order`
:   the order of the new folio

`struct page *split_at`
:   a page within the new folio

`struct list_head *list`
:   after-split folios are added to **list** if not null, otherwise to LRU
    list

**Description**

It has the same prerequisites and returns as
`split_huge_page_to_list_to_order()`.

Split a folio at **split\_at** to a new\_order folio, leave the
remaining subpages of the original folio as large as possible. For example,
in the case of splitting an order-9 folio at its third order-3 subpages to
an order-3 folio, there are 2^(9-3)=64 order-3 subpages in the order-9 folio.
After the split, there will be a group of folios with different orders and
the new folio containing **split\_at** is marked in bracket:
[order-4, {order-3}, order-3, order-5, order-6, order-7, order-8].

After split, folio is left locked for caller.

**Return**

0 - successful, <0 - failed (if -ENOMEM is returned, **folio** might be
split but not to **new\_order**, the caller needs to check)

unsigned int min\_order\_for\_split(struct [folio](#c.min_order_for_split "folio") \*folio)
:   get the minimum order **folio** can be split to

**Parameters**

`struct folio *folio`
:   folio to split

**Description**

[`min_order_for_split()`](#c.min_order_for_split "min_order_for_split") tells the minimum order **folio** can be split to.
If a file-backed folio is truncated, 0 will be returned. Any subsequent
split attempt should get -EBUSY from split checking code.

**Return**

**folio**’s minimum order for split
