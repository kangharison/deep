# I915 VM_BIND feature design and use cases

> 출처(원문): https://docs.kernel.org/gpu/rfc/i915_vm_bind.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# I915 VM\_BIND feature design and use cases

## VM\_BIND feature

DRM\_I915\_GEM\_VM\_BIND/UNBIND ioctls allows UMD to bind/unbind GEM buffer
objects (BOs) or sections of a BOs at specified GPU virtual addresses on a
specified address space (VM). These mappings (also referred to as persistent
mappings) will be persistent across multiple GPU submissions (execbuf calls)
issued by the UMD, without user having to provide a list of all required
mappings during each submission (as required by older execbuf mode).

The VM\_BIND/UNBIND calls allow UMDs to request a timeline out fence for
signaling the completion of bind/unbind operation.

VM\_BIND feature is advertised to user via I915\_PARAM\_VM\_BIND\_VERSION.
User has to opt-in for VM\_BIND mode of binding for an address space (VM)
during VM creation time via I915\_VM\_CREATE\_FLAGS\_USE\_VM\_BIND extension.

VM\_BIND/UNBIND ioctl calls executed on different CPU threads concurrently are
not ordered. Furthermore, parts of the VM\_BIND/UNBIND operations can be done
asynchronously, when valid out fence is specified.

VM\_BIND features include:

* Multiple Virtual Address (VA) mappings can map to the same physical pages
  of an object (aliasing).
* VA mapping can map to a partial section of the BO (partial binding).
* Support capture of persistent mappings in the dump upon GPU error.
* Support for userptr gem objects (no special uapi is required for this).

### TLB flush consideration

The i915 driver flushes the TLB for each submission and when an object’s
pages are released. The VM\_BIND/UNBIND operation will not do any additional
TLB flush. Any VM\_BIND mapping added will be in the working set for subsequent
submissions on that VM and will not be in the working set for currently running
batches (which would require additional TLB flushes, which is not supported).

### Execbuf ioctl in VM\_BIND mode

A VM in VM\_BIND mode will not support older execbuf mode of binding.
The execbuf ioctl handling in VM\_BIND mode differs significantly from the
older execbuf2 ioctl (See [`struct drm_i915_gem_execbuffer2`](../driver-uapi.html#c.drm_i915_gem_execbuffer2 "drm_i915_gem_execbuffer2")).
Hence, a new execbuf3 ioctl has been added to support VM\_BIND mode. (See
[`struct drm_i915_gem_execbuffer3`](#c.drm_i915_gem_execbuffer3 "drm_i915_gem_execbuffer3")). The execbuf3 ioctl will not accept any
execlist. Hence, no support for implicit sync. It is expected that the below
work will be able to support requirements of object dependency setting in all
use cases:

“dma-buf: Add an API for exporting sync files”
(<https://lwn.net/Articles/859290/>)

The new execbuf3 ioctl only works in VM\_BIND mode and the VM\_BIND mode only
works with execbuf3 ioctl for submission. All BOs mapped on that VM (through
VM\_BIND call) at the time of execbuf3 call are deemed required for that
submission.

The execbuf3 ioctl directly specifies the batch addresses instead of as
object handles as in execbuf2 ioctl. The execbuf3 ioctl will also not
support many of the older features like in/out/submit fences, fence array,
default gem context and many more (See [`struct drm_i915_gem_execbuffer3`](#c.drm_i915_gem_execbuffer3 "drm_i915_gem_execbuffer3")).

In VM\_BIND mode, VA allocation is completely managed by the user instead of
the i915 driver. Hence all VA assignment, eviction are not applicable in
VM\_BIND mode. Also, for determining object activeness, VM\_BIND mode will not
be using the i915\_vma active reference tracking. It will instead use dma-resv
object for that (See [VM\_BIND dma\_resv usage](#vm-bind-dma-resv-usage)).

So, a lot of existing code supporting execbuf2 ioctl, like relocations, VA
evictions, vma lookup table, implicit sync, vma active reference tracking etc.,
are not applicable for execbuf3 ioctl. Hence, all execbuf3 specific handling
should be in a separate file and only functionalities common to these ioctls
can be the shared code where possible.

### VM\_PRIVATE objects

By default, BOs can be mapped on multiple VMs and can also be dma-buf
exported. Hence these BOs are referred to as Shared BOs.
During each execbuf submission, the request fence must be added to the
dma-resv fence list of all shared BOs mapped on the VM.

VM\_BIND feature introduces an optimization where user can create BO which
is private to a specified VM via I915\_GEM\_CREATE\_EXT\_VM\_PRIVATE flag during
BO creation. Unlike Shared BOs, these VM private BOs can only be mapped on
the VM they are private to and can’t be dma-buf exported.
All private BOs of a VM share the dma-resv object. Hence during each execbuf
submission, they need only one dma-resv fence list updated. Thus, the fast
path (where required mappings are already bound) submission latency is O(1)
w.r.t the number of VM private BOs.

### VM\_BIND locking hierarchy

The locking design here supports the older (execlist based) execbuf mode, the
newer VM\_BIND mode, the VM\_BIND mode with GPU page faults and possible future
system allocator support (See [Shared Virtual Memory (SVM) support](#shared-virtual-memory-svm-support)).
The older execbuf mode and the newer VM\_BIND mode without page faults manages
residency of backing storage using dma\_fence. The VM\_BIND mode with page faults
and the system allocator support do not use any dma\_fence at all.

VM\_BIND locking order is as below.

1. Lock-A: A vm\_bind mutex will protect vm\_bind lists. This lock is taken in
   vm\_bind/vm\_unbind ioctl calls, in the execbuf path and while releasing the
   mapping.

   In future, when GPU page faults are supported, we can potentially use a
   rwsem instead, so that multiple page fault handlers can take the read side
   lock to lookup the mapping and hence can run in parallel.
   The older execbuf mode of binding do not need this lock.
2. Lock-B: The object’s dma-resv lock will protect i915\_vma state and needs to
   be held while binding/unbinding a vma in the async worker and while updating
   dma-resv fence list of an object. Note that private BOs of a VM will all
   share a dma-resv object.

   The future system allocator support will use the HMM prescribed locking
   instead.
3. Lock-C: Spinlock/s to protect some of the VM’s lists like the list of
   invalidated vmas (due to eviction and userptr invalidation) etc.

When GPU page faults are supported, the execbuf path do not take any of these
locks. There we will simply smash the new batch buffer address into the ring and
then tell the scheduler run that. The lock taking only happens from the page
fault handler, where we take lock-A in read mode, whichever lock-B we need to
find the backing storage (dma\_resv lock for gem objects, and hmm/core mm for
system allocator) and some additional locks (lock-D) for taking care of page
table races. Page fault mode should not need to ever manipulate the vm lists,
so won’t ever need lock-C.

### VM\_BIND LRU handling

We need to ensure VM\_BIND mapped objects are properly LRU tagged to avoid
performance degradation. We will also need support for bulk LRU movement of
VM\_BIND objects to avoid additional latencies in execbuf path.

The page table pages are similar to VM\_BIND mapped objects (See
[Evictable page table allocations](#evictable-page-table-allocations)) and are maintained per VM and needs to
be pinned in memory when VM is made active (ie., upon an execbuf call with
that VM). So, bulk LRU movement of page table pages is also needed.

### VM\_BIND dma\_resv usage

Fences needs to be added to all VM\_BIND mapped objects. During each execbuf
submission, they are added with DMA\_RESV\_USAGE\_BOOKKEEP usage to prevent
over sync (See [`enum dma_resv_usage`](../../driver-api/dma-buf.html#c.dma_resv_usage "dma_resv_usage")). One can override it with either
DMA\_RESV\_USAGE\_READ or DMA\_RESV\_USAGE\_WRITE usage during explicit object
dependency setting.

Note that DRM\_I915\_GEM\_WAIT and DRM\_I915\_GEM\_BUSY ioctls do not check for
DMA\_RESV\_USAGE\_BOOKKEEP usage and hence should not be used for end of batch
check. Instead, the execbuf3 out fence should be used for end of batch check
(See [`struct drm_i915_gem_execbuffer3`](#c.drm_i915_gem_execbuffer3 "drm_i915_gem_execbuffer3")).

Also, in VM\_BIND mode, use dma-resv apis for determining object activeness
(See [`dma_resv_test_signaled()`](../../driver-api/dma-buf.html#c.dma_resv_test_signaled "dma_resv_test_signaled") and [`dma_resv_wait_timeout()`](../../driver-api/dma-buf.html#c.dma_resv_wait_timeout "dma_resv_wait_timeout")) and do not use the
older i915\_vma active reference tracking which is deprecated. This should be
easier to get it working with the current TTM backend.

### Mesa use case

VM\_BIND can potentially reduce the CPU overhead in Mesa (both Vulkan and Iris),
hence improving performance of CPU-bound applications. It also allows us to
implement Vulkan’s Sparse Resources. With increasing GPU hardware performance,
reducing CPU overhead becomes more impactful.

## Other VM\_BIND use cases

### Long running Compute contexts

Usage of dma-fence expects that they complete in reasonable amount of time.
Compute on the other hand can be long running. Hence it is appropriate for
compute to use user/memory fence (See [User/Memory Fence](#user-memory-fence)) and dma-fence usage
must be limited to in-kernel consumption only.

Where GPU page faults are not available, kernel driver upon buffer invalidation
will initiate a suspend (preemption) of long running context, finish the
invalidation, revalidate the BO and then resume the compute context. This is
done by having a per-context preempt fence which is enabled when someone tries
to wait on it and triggers the context preemption.

#### User/Memory Fence

User/Memory fence is a <address, value> pair. To signal the user fence, the
specified value will be written at the specified virtual address and wakeup the
waiting process. User fence can be signaled either by the GPU or kernel async
worker (like upon bind completion). User can wait on a user fence with a new
user fence wait ioctl.

Here is some prior work on this:
<https://patchwork.freedesktop.org/patch/349417/>

#### Low Latency Submission

Allows compute UMD to directly submit GPU jobs instead of through execbuf
ioctl. This is made possible by VM\_BIND is not being synchronized against
execbuf. VM\_BIND allows bind/unbind of mappings required for the directly
submitted jobs.

### Debugger

With debug event interface user space process (debugger) is able to keep track
of and act upon resources created by another process (debugged) and attached
to GPU via vm\_bind interface.

### GPU page faults

GPU page faults when supported (in future), will only be supported in the
VM\_BIND mode. While both the older execbuf mode and the newer VM\_BIND mode of
binding will require using dma-fence to ensure residency, the GPU page faults
mode when supported, will not use any dma-fence as residency is purely managed
by installing and removing/invalidating page table entries.

### Page level hints settings

VM\_BIND allows any hints setting per mapping instead of per BO. Possible hints
include placement and atomicity. Sub-BO level placement hint will be even more
relevant with upcoming GPU on-demand page fault support.

### Page level Cache/CLOS settings

VM\_BIND allows cache/CLOS settings per mapping instead of per BO.

### Evictable page table allocations

Make pagetable allocations evictable and manage them similar to VM\_BIND
mapped objects. Page table pages are similar to persistent mappings of a
VM (difference here are that the page table pages will not have an i915\_vma
structure and after swapping pages back in, parent page link needs to be
updated).

### Shared Virtual Memory (SVM) support

VM\_BIND interface can be used to map system memory directly (without gem BO
abstraction) using the HMM interface. SVM is only supported with GPU page
faults enabled.

## VM\_BIND UAPI

**I915\_PARAM\_VM\_BIND\_VERSION**

VM\_BIND feature version supported.
See [`typedef drm_i915_getparam_t`](../driver-uapi.html#c.drm_i915_getparam_t "drm_i915_getparam_t") param.

Specifies the VM\_BIND feature version supported.
The following versions of VM\_BIND have been defined:

0: No VM\_BIND support.

1: In VM\_UNBIND calls, the UMD must specify the exact mappings created
:   previously with VM\_BIND, the ioctl will not support unbinding multiple
    mappings or splitting them. Similarly, VM\_BIND calls will not replace
    any existing mappings.

2: The restrictions on unbinding partial or multiple mappings is
:   lifted, Similarly, binding will replace any mappings in the given range.

See [`struct drm_i915_gem_vm_bind`](#c.drm_i915_gem_vm_bind "drm_i915_gem_vm_bind") and [`struct drm_i915_gem_vm_unbind`](#c.drm_i915_gem_vm_unbind "drm_i915_gem_vm_unbind").

**I915\_VM\_CREATE\_FLAGS\_USE\_VM\_BIND**

Flag to opt-in for VM\_BIND mode of binding during VM creation.
See [`struct drm_i915_gem_vm_control`](../driver-uapi.html#c.drm_i915_gem_vm_control "drm_i915_gem_vm_control") flags.

The older execbuf2 ioctl will not support VM\_BIND mode of operation.
For VM\_BIND mode, we have new execbuf3 ioctl which will not accept any
execlist (See [`struct drm_i915_gem_execbuffer3`](#c.drm_i915_gem_execbuffer3 "drm_i915_gem_execbuffer3") for more details).

struct drm\_i915\_gem\_timeline\_fence
:   An input or output timeline fence.

**Definition**:

```
struct drm_i915_gem_timeline_fence {
    __u32 handle;
    __u32 flags;
#define I915_TIMELINE_FENCE_WAIT            (1 << 0);
#define I915_TIMELINE_FENCE_SIGNAL          (1 << 1);
#define __I915_TIMELINE_FENCE_UNKNOWN_FLAGS (-(I915_TIMELINE_FENCE_SIGNAL << 1));
    __u64 value;
};
```

**Members**

`handle`
:   User’s handle for a drm\_syncobj to wait on or signal.

`flags`
:   Supported flags are:

    I915\_TIMELINE\_FENCE\_WAIT:
    Wait for the input fence before the operation.

    I915\_TIMELINE\_FENCE\_SIGNAL:
    Return operation completion fence as output.

`value`
:   A point in the timeline.
    Value must be 0 for a binary drm\_syncobj. A Value of 0 for a
    timeline drm\_syncobj is invalid as it turns a drm\_syncobj into a
    binary one.

**Description**

The operation will wait for input fence to signal.

The returned output fence will be signaled after the completion of the
operation.

struct drm\_i915\_gem\_vm\_bind
:   VA to object mapping to bind.

**Definition**:

```
struct drm_i915_gem_vm_bind {
    __u32 vm_id;
    __u32 handle;
    __u64 start;
    __u64 offset;
    __u64 length;
    __u64 flags;
#define I915_GEM_VM_BIND_CAPTURE        (1 << 0);
    struct drm_i915_gem_timeline_fence fence;
    __u64 extensions;
};
```

**Members**

`vm_id`
:   VM (address space) id to bind

`handle`
:   Object handle

`start`
:   Virtual Address start to bind

`offset`
:   Offset in object to bind

`length`
:   Length of mapping to bind

`flags`
:   Supported flags are:

    I915\_GEM\_VM\_BIND\_CAPTURE:
    Capture this mapping in the dump upon GPU error.

    Note that **fence** carries its own flags.

`fence`
:   Timeline fence for bind completion signaling.

    Timeline fence is of format [`struct drm_i915_gem_timeline_fence`](#c.drm_i915_gem_timeline_fence "drm_i915_gem_timeline_fence").

    It is an out fence, hence using I915\_TIMELINE\_FENCE\_WAIT flag
    is invalid, and an error will be returned.

    If I915\_TIMELINE\_FENCE\_SIGNAL flag is not set, then out fence
    is not requested and binding is completed synchronously.

`extensions`
:   Zero-terminated chain of extensions.

    For future extensions. See [`struct i915_user_extension`](../driver-uapi.html#c.i915_user_extension "i915_user_extension").

**Description**

This structure is passed to VM\_BIND ioctl and specifies the mapping of GPU
virtual address (VA) range to the section of an object that should be bound
in the device page table of the specified address space (VM).
The VA range specified must be unique (ie., not currently bound) and can
be mapped to whole object or a section of the object (partial binding).
Multiple VA mappings can be created to the same section of the object
(aliasing).

The **start**, **offset** and **length** must be 4K page aligned. However the DG2 has
64K page size for device local memory and has compact page table. On that
platform, for binding device local-memory objects, the **start**, **offset** and
**length** must be 64K aligned. Also, UMDs should not mix the local memory 64K
page and the system memory 4K page bindings in the same 2M range.

Error code -EINVAL will be returned if **start**, **offset** and **length** are not
properly aligned. In version 1 (See I915\_PARAM\_VM\_BIND\_VERSION), error code
-ENOSPC will be returned if the VA range specified can’t be reserved.

VM\_BIND/UNBIND ioctl calls executed on different CPU threads concurrently
are not ordered. Furthermore, parts of the VM\_BIND operation can be done
asynchronously, if valid **fence** is specified.

struct drm\_i915\_gem\_vm\_unbind
:   VA to object mapping to unbind.

**Definition**:

```
struct drm_i915_gem_vm_unbind {
    __u32 vm_id;
    __u32 rsvd;
    __u64 start;
    __u64 length;
    __u64 flags;
    struct drm_i915_gem_timeline_fence fence;
    __u64 extensions;
};
```

**Members**

`vm_id`
:   VM (address space) id to bind

`rsvd`
:   Reserved, MBZ

`start`
:   Virtual Address start to unbind

`length`
:   Length of mapping to unbind

`flags`
:   Currently reserved, MBZ.

    Note that **fence** carries its own flags.

`fence`
:   Timeline fence for unbind completion signaling.

    Timeline fence is of format [`struct drm_i915_gem_timeline_fence`](#c.drm_i915_gem_timeline_fence "drm_i915_gem_timeline_fence").

    It is an out fence, hence using I915\_TIMELINE\_FENCE\_WAIT flag
    is invalid, and an error will be returned.

    If I915\_TIMELINE\_FENCE\_SIGNAL flag is not set, then out fence
    is not requested and unbinding is completed synchronously.

`extensions`
:   Zero-terminated chain of extensions.

    For future extensions. See [`struct i915_user_extension`](../driver-uapi.html#c.i915_user_extension "i915_user_extension").

**Description**

This structure is passed to VM\_UNBIND ioctl and specifies the GPU virtual
address (VA) range that should be unbound from the device page table of the
specified address space (VM). VM\_UNBIND will force unbind the specified
range from device page table without waiting for any GPU job to complete.
It is UMDs responsibility to ensure the mapping is no longer in use before
calling VM\_UNBIND.

If the specified mapping is not found, the ioctl will simply return without
any error.

VM\_BIND/UNBIND ioctl calls executed on different CPU threads concurrently
are not ordered. Furthermore, parts of the VM\_UNBIND operation can be done
asynchronously, if valid **fence** is specified.

struct drm\_i915\_gem\_execbuffer3
:   Structure for DRM\_I915\_GEM\_EXECBUFFER3 ioctl.

**Definition**:

```
struct drm_i915_gem_execbuffer3 {
    __u32 ctx_id;
    __u32 engine_idx;
    __u64 batch_address;
    __u64 flags;
    __u32 rsvd1;
    __u32 fence_count;
    __u64 timeline_fences;
    __u64 rsvd2;
    __u64 extensions;
};
```

**Members**

`ctx_id`
:   Context id

    Only contexts with user engine map are allowed.

`engine_idx`
:   Engine index

    An index in the user engine map of the context specified by **ctx\_id**.

`batch_address`
:   Batch gpu virtual address/es.

    For normal submission, it is the gpu virtual address of the batch
    buffer. For parallel submission, it is a pointer to an array of
    batch buffer gpu virtual addresses with array size equal to the
    number of (parallel) engines involved in that submission (See
    [`struct i915_context_engines_parallel_submit`](../driver-uapi.html#c.i915_context_engines_parallel_submit "i915_context_engines_parallel_submit")).

`flags`
:   Currently reserved, MBZ

`rsvd1`
:   Reserved, MBZ

`fence_count`
:   Number of fences in **timeline\_fences** array.

`timeline_fences`
:   Pointer to an array of timeline fences.

    Timeline fences are of format [`struct drm_i915_gem_timeline_fence`](#c.drm_i915_gem_timeline_fence "drm_i915_gem_timeline_fence").

`rsvd2`
:   Reserved, MBZ

`extensions`
:   Zero-terminated chain of extensions.

    For future extensions. See [`struct i915_user_extension`](../driver-uapi.html#c.i915_user_extension "i915_user_extension").

**Description**

DRM\_I915\_GEM\_EXECBUFFER3 ioctl only works in VM\_BIND mode and VM\_BIND mode
only works with this ioctl for submission.
See I915\_VM\_CREATE\_FLAGS\_USE\_VM\_BIND.

struct drm\_i915\_gem\_create\_ext\_vm\_private
:   Extension to make the object private to the specified VM.

**Definition**:

```
struct drm_i915_gem_create_ext_vm_private {
#define I915_GEM_CREATE_EXT_VM_PRIVATE          2;
    struct i915_user_extension base;
    __u32 vm_id;
};
```

**Members**

`base`
:   Extension link. See [`struct i915_user_extension`](../driver-uapi.html#c.i915_user_extension "i915_user_extension").

`vm_id`
:   Id of the VM to which the object is private

**Description**

See [`struct drm_i915_gem_create_ext`](../driver-uapi.html#c.drm_i915_gem_create_ext "drm_i915_gem_create_ext").
