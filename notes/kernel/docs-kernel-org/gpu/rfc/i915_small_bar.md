# I915 Small BAR RFC Section

> 출처(원문): https://docs.kernel.org/gpu/rfc/i915_small_bar.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# I915 Small BAR RFC Section

Starting from DG2 we will have resizable BAR support for device local-memory(i.e
I915\_MEMORY\_CLASS\_DEVICE), but in some cases the final BAR size might still be
smaller than the total probed\_size. In such cases, only some subset of
I915\_MEMORY\_CLASS\_DEVICE will be CPU accessible(for example the first 256M),
while the remainder is only accessible via the GPU.

## I915\_GEM\_CREATE\_EXT\_FLAG\_NEEDS\_CPU\_ACCESS flag

New gem\_create\_ext flag to tell the kernel that a BO will require CPU access.
This becomes important when placing an object in I915\_MEMORY\_CLASS\_DEVICE, where
underneath the device has a small BAR, meaning only some portion of it is CPU
accessible. Without this flag the kernel will assume that CPU access is not
required, and prioritize using the non-CPU visible portion of
I915\_MEMORY\_CLASS\_DEVICE.

struct \_\_drm\_i915\_gem\_create\_ext
:   Existing gem\_create behaviour, with added extension support using [`struct i915_user_extension`](../driver-uapi.html#c.i915_user_extension "i915_user_extension").

**Definition**:

```
struct __drm_i915_gem_create_ext {
    __u64 size;
    __u32 handle;
#define I915_GEM_CREATE_EXT_FLAG_NEEDS_CPU_ACCESS (1 << 0);
    __u32 flags;
#define I915_GEM_CREATE_EXT_MEMORY_REGIONS 0;
#define I915_GEM_CREATE_EXT_PROTECTED_CONTENT 1;
    __u64 extensions;
};
```

**Members**

`size`
:   Requested size for the object.

    The (page-aligned) allocated size for the object will be returned.

    Note that for some devices we have might have further minimum
    page-size restrictions (larger than 4K), like for device local-memory.
    However in general the final size here should always reflect any
    rounding up, if for example using the I915\_GEM\_CREATE\_EXT\_MEMORY\_REGIONS
    extension to place the object in device local-memory. The kernel will
    always select the largest minimum page-size for the set of possible
    placements as the value to use when rounding up the **size**.

`handle`
:   Returned handle for the object.

    Object handles are nonzero.

`flags`
:   Optional flags.

    Supported values:

    I915\_GEM\_CREATE\_EXT\_FLAG\_NEEDS\_CPU\_ACCESS - Signal to the kernel that
    the object will need to be accessed via the CPU.

    Only valid when placing objects in I915\_MEMORY\_CLASS\_DEVICE, and only
    strictly required on configurations where some subset of the device
    memory is directly visible/mappable through the CPU (which we also
    call small BAR), like on some DG2+ systems. Note that this is quite
    undesirable, but due to various factors like the client CPU, BIOS etc
    it’s something we can expect to see in the wild. See
    [`__drm_i915_memory_region_info.probed_cpu_visible_size`](#c.__drm_i915_memory_region_info "__drm_i915_memory_region_info") for how to
    determine if this system applies.

    Note that one of the placements MUST be I915\_MEMORY\_CLASS\_SYSTEM, to
    ensure the kernel can always spill the allocation to system memory,
    if the object can’t be allocated in the mappable part of
    I915\_MEMORY\_CLASS\_DEVICE.

    Also note that since the kernel only supports flat-CCS on objects
    that can *only* be placed in I915\_MEMORY\_CLASS\_DEVICE, we therefore
    don’t support I915\_GEM\_CREATE\_EXT\_FLAG\_NEEDS\_CPU\_ACCESS together with
    flat-CCS.

    Without this hint, the kernel will assume that non-mappable
    I915\_MEMORY\_CLASS\_DEVICE is preferred for this object. Note that the
    kernel can still migrate the object to the mappable part, as a last
    resort, if userspace ever CPU faults this object, but this might be
    expensive, and so ideally should be avoided.

    On older kernels which lack the relevant small-bar uAPI support (see
    also [`__drm_i915_memory_region_info.probed_cpu_visible_size`](#c.__drm_i915_memory_region_info "__drm_i915_memory_region_info")),
    usage of the flag will result in an error, but it should NEVER be
    possible to end up with a small BAR configuration, assuming we can
    also successfully load the i915 kernel module. In such cases the
    entire I915\_MEMORY\_CLASS\_DEVICE region will be CPU accessible, and as
    such there are zero restrictions on where the object can be placed.

`extensions`
:   The chain of extensions to apply to this object.

    This will be useful in the future when we need to support several
    different extensions, and we need to apply more than one when
    creating the object. See [`struct i915_user_extension`](../driver-uapi.html#c.i915_user_extension "i915_user_extension").

    If we don’t supply any extensions then we get the same old gem\_create
    behaviour.

    For I915\_GEM\_CREATE\_EXT\_MEMORY\_REGIONS usage see
    [`struct drm_i915_gem_create_ext_memory_regions`](../driver-uapi.html#c.drm_i915_gem_create_ext_memory_regions "drm_i915_gem_create_ext_memory_regions").

    For I915\_GEM\_CREATE\_EXT\_PROTECTED\_CONTENT usage see
    [`struct drm_i915_gem_create_ext_protected_content`](../driver-uapi.html#c.drm_i915_gem_create_ext_protected_content "drm_i915_gem_create_ext_protected_content").

**Description**

Note that new buffer flags should be added here, at least for the stuff that
is immutable. Previously we would have two ioctls, one to create the object
with gem\_create, and another to apply various parameters, however this
creates some ambiguity for the params which are considered immutable. Also in
general we’re phasing out the various SET/GET ioctls.

## probed\_cpu\_visible\_size attribute

New struct\_\_drm\_i915\_memory\_region attribute which returns the total size of the
CPU accessible portion, for the particular region. This should only be
applicable for I915\_MEMORY\_CLASS\_DEVICE. We also report the
unallocated\_cpu\_visible\_size, alongside the unallocated\_size.

Vulkan will need this as part of creating a separate VkMemoryHeap with the
VK\_MEMORY\_PROPERTY\_HOST\_VISIBLE\_BIT set, to represent the CPU visible portion,
where the total size of the heap needs to be known. It also wants to be able to
give a rough estimate of how memory can potentially be allocated.

struct \_\_drm\_i915\_memory\_region\_info
:   Describes one region as known to the driver.

**Definition**:

```
struct __drm_i915_memory_region_info {
    struct drm_i915_gem_memory_class_instance region;
    __u32 rsvd0;
    __u64 probed_size;
    __u64 unallocated_size;
    union {
        __u64 rsvd1[8];
        struct {
            __u64 probed_cpu_visible_size;
            __u64 unallocated_cpu_visible_size;
        };
    };
};
```

**Members**

`region`
:   The class:instance pair encoding

`rsvd0`
:   MBZ

`probed_size`
:   Memory probed by the driver

    Note that it should not be possible to ever encounter a zero value
    here, also note that no current region type will ever return -1 here.
    Although for future region types, this might be a possibility. The
    same applies to the other size fields.

`unallocated_size`
:   Estimate of memory remaining

    Requires CAP\_PERFMON or CAP\_SYS\_ADMIN to get reliable accounting.
    Without this (or if this is an older kernel) the value here will
    always equal the **probed\_size**. Note this is only currently tracked
    for I915\_MEMORY\_CLASS\_DEVICE regions (for other types the value here
    will always equal the **probed\_size**).

`{unnamed_union}`
:   anonymous

`rsvd1`
:   MBZ

`{unnamed_struct}`
:   anonymous

`probed_cpu_visible_size`
:   Memory probed by the driver
    that is CPU accessible.

    This will be always be <= **probed\_size**, and the
    remainder (if there is any) will not be CPU
    accessible.

    On systems without small BAR, the **probed\_size** will
    always equal the **probed\_cpu\_visible\_size**, since all
    of it will be CPU accessible.

    Note this is only tracked for
    I915\_MEMORY\_CLASS\_DEVICE regions (for other types the
    value here will always equal the **probed\_size**).

    Note that if the value returned here is zero, then
    this must be an old kernel which lacks the relevant
    small-bar uAPI support (including
    I915\_GEM\_CREATE\_EXT\_FLAG\_NEEDS\_CPU\_ACCESS), but on
    such systems we should never actually end up with a
    small BAR configuration, assuming we are able to load
    the kernel module. Hence it should be safe to treat
    this the same as when **probed\_cpu\_visible\_size** ==
    **probed\_size**.

`unallocated_cpu_visible_size`
:   Estimate of CPU
    visible memory remaining

    Note this is only tracked for
    I915\_MEMORY\_CLASS\_DEVICE regions (for other types the
    value here will always equal the
    **probed\_cpu\_visible\_size**).

    Requires CAP\_PERFMON or CAP\_SYS\_ADMIN to get reliable
    accounting. Without this the value here will always
    equal the **probed\_cpu\_visible\_size**. Note this is only
    currently tracked for I915\_MEMORY\_CLASS\_DEVICE
    regions (for other types the value here will also
    always equal the **probed\_cpu\_visible\_size**).

    If this is an older kernel the value here will be
    zero, see also **probed\_cpu\_visible\_size**.

**Description**

Note this is using both [`struct drm_i915_query_item`](../driver-uapi.html#c.drm_i915_query_item "drm_i915_query_item") and [`struct drm_i915_query`](../driver-uapi.html#c.drm_i915_query "drm_i915_query").
For this new query we are adding the new query id DRM\_I915\_QUERY\_MEMORY\_REGIONS
at [`drm_i915_query_item.query_id`](../driver-uapi.html#c.drm_i915_query_item "drm_i915_query_item").

## Error Capture restrictions

With error capture we have two new restrictions:

> 1) Error capture is best effort on small BAR systems; if the pages are not
> CPU accessible, at the time of capture, then the kernel is free to skip
> trying to capture them.
>
> 2) On discrete and newer integrated platforms we now reject error capture
> on recoverable contexts. In the future the kernel may want to blit during
> error capture, when for example something is not currently CPU accessible.
