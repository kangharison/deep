# GFP masks used from FS/IO context

> 출처(원문): https://docs.kernel.org/core-api/gfp_mask-from-fs-io.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GFP masks used from FS/IO context

Date:
:   May, 2018

Author:
:   Michal Hocko <[mhocko@kernel.org](mailto:mhocko%40kernel.org)>

## Introduction

Code paths in the filesystem and IO stacks must be careful when
allocating memory to prevent recursion deadlocks caused by direct
memory reclaim calling back into the FS or IO paths and blocking on
already held resources (e.g. locks - most commonly those used for the
transaction context).

The traditional way to avoid this deadlock problem is to clear \_\_GFP\_FS
respectively \_\_GFP\_IO (note the latter implies clearing the first as well) in
the gfp mask when calling an allocator. GFP\_NOFS respectively GFP\_NOIO can be
used as shortcut. It turned out though that above approach has led to
abuses when the restricted gfp mask is used “just in case” without a
deeper consideration which leads to problems because an excessive use
of GFP\_NOFS/GFP\_NOIO can lead to memory over-reclaim or other memory
reclaim issues.

## New API

Since 4.12 we do have a generic scope API for both NOFS and NOIO context
`memalloc_nofs_save`, `memalloc_nofs_restore` respectively `memalloc_noio_save`,
`memalloc_noio_restore` which allow to mark a scope to be a critical
section from a filesystem or I/O point of view. Any allocation from that
scope will inherently drop \_\_GFP\_FS respectively \_\_GFP\_IO from the given
mask so no memory allocation can recurse back in the FS/IO.

unsigned int memalloc\_nofs\_save(void)
:   Marks implicit GFP\_NOFS allocation scope.

**Parameters**

`void`
:   no arguments

**Description**

This functions marks the beginning of the GFP\_NOFS allocation scope.
All further allocations will implicitly drop \_\_GFP\_FS flag and so
they are safe for the FS critical section from the allocation recursion
point of view. Use memalloc\_nofs\_restore to end the scope with flags
returned by this function.

**Context**

This function is safe to be used from any context.

**Return**

The saved flags to be passed to memalloc\_nofs\_restore.

void memalloc\_nofs\_restore(unsigned int flags)
:   Ends the implicit GFP\_NOFS scope.

**Parameters**

`unsigned int flags`
:   Flags to restore.

**Description**

Ends the implicit GFP\_NOFS scope started by memalloc\_nofs\_save function.
Always make sure that the given flags is the return value from the
pairing memalloc\_nofs\_save call.

unsigned int memalloc\_noio\_save(void)
:   Marks implicit GFP\_NOIO allocation scope.

**Parameters**

`void`
:   no arguments

**Description**

This functions marks the beginning of the GFP\_NOIO allocation scope.
All further allocations will implicitly drop \_\_GFP\_IO flag and so
they are safe for the IO critical section from the allocation recursion
point of view. Use memalloc\_noio\_restore to end the scope with flags
returned by this function.

**Context**

This function is safe to be used from any context.

**Return**

The saved flags to be passed to memalloc\_noio\_restore.

void memalloc\_noio\_restore(unsigned int flags)
:   Ends the implicit GFP\_NOIO scope.

**Parameters**

`unsigned int flags`
:   Flags to restore.

**Description**

Ends the implicit GFP\_NOIO scope started by memalloc\_noio\_save function.
Always make sure that the given flags is the return value from the
pairing memalloc\_noio\_save call.

FS/IO code then simply calls the appropriate save function before
any critical section with respect to the reclaim is started - e.g.
lock shared with the reclaim context or when a transaction context
nesting would be possible via reclaim. The restore function should be
called when the critical section ends. All that ideally along with an
explanation what is the reclaim context for easier maintenance.

Please note that the proper pairing of save/restore functions
allows nesting so it is safe to call `memalloc_noio_save` or
`memalloc_noio_restore` respectively from an existing NOIO or NOFS
scope.

## What about \_\_vmalloc(GFP\_NOFS)

Since v5.17, and specifically after the [commit 451769ebb7e79](https://git.kernel.org/torvalds/c/451769ebb7e79) (“mm/vmalloc:
alloc GFP\_NO{FS,IO} for vmalloc”), GFP\_NOFS/GFP\_NOIO are now supported in
`[k]vmalloc` by implicitly using scope API.

In earlier kernels `vmalloc` didn’t support GFP\_NOFS semantic because there
were hardcoded GFP\_KERNEL allocations deep inside the allocator. That means
that calling `vmalloc` with GFP\_NOFS/GFP\_NOIO was almost always a bug.

In the ideal world, upper layers should already mark dangerous contexts
and so no special care is required and `vmalloc` should be called without any
problems. Sometimes if the context is not really clear or there are layering
violations then the recommended way around that (on pre-v5.17 kernels) is to
wrap `vmalloc` by the scope API with a comment explaining the problem.
