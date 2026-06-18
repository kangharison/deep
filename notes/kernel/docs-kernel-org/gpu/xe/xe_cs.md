# Command submission

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_cs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Command submission

Execs have historically been rather complicated in DRM drivers (at least in
the i915) because a few things:

* Passing in a list BO which are read / written to creating implicit syncs
* Binding at exec time
* Flow controlling the ring at exec time

In Xe we avoid all of this complication by not allowing a BO list to be
passed into an exec, using the dma-buf implicit sync uAPI, have binds as
separate operations, and using the DRM scheduler to flow control the ring.
Let’s deep dive on each of these.

We can get away from a BO list by forcing the user to use in / out fences on
every exec rather than the kernel tracking dependencies of BO (e.g. if the
user knows an exec writes to a BO and reads from the BO in the next exec, it
is the user’s responsibility to pass in / out fence between the two execs).

We do not allow a user to trigger a bind at exec time rather we have a VM
bind IOCTL which uses the same in / out fence interface as exec. In that
sense, a VM bind is basically the same operation as an exec from the user
perspective. e.g. If an exec depends on a VM bind use the in / out fence
interface ([`struct drm_xe_sync`](../driver-uapi.html#c.drm_xe_sync "drm_xe_sync")) to synchronize like syncing between two
dependent execs.

Although a user cannot trigger a bind, we still have to rebind userptrs in
the VM that have been invalidated since the last exec, likewise we also have
to rebind BOs that have been evicted by the kernel. We schedule these rebinds
behind any pending kernel operations on any external BOs in VM or any BOs
private to the VM. This is accomplished by the rebinds waiting on BOs
DMA\_RESV\_USAGE\_KERNEL slot (kernel ops) and kernel ops waiting on all BOs
slots (inflight execs are in the DMA\_RESV\_USAGE\_BOOKKEEP for private BOs and
for external BOs).

Rebinds / dma-resv usage applies to non-compute mode VMs only as for compute
mode VMs we use preempt fences and a rebind worker (TODO: add link).

There is no need to flow control the ring in the exec as we write the ring at
submission time and set the DRM scheduler max job limit SIZE\_OF\_RING /
MAX\_JOB\_SIZE. The DRM scheduler will then hold all jobs until space in the
ring is available.

All of this results in a rather simple exec implementation.

## Flow

```
Parse input arguments
Wait for any async VM bind passed as in-fences to start
<----------------------------------------------------------------------|
Lock global VM lock in read mode                                       |
Pin userptrs (also finds userptr invalidated since last exec)          |
Lock exec (VM dma-resv lock, external BOs dma-resv locks)              |
Validate BOs that have been evicted                                    |
Create job                                                             |
Rebind invalidated userptrs + evicted BOs (non-compute-mode)           |
Add rebind fence dependency to job                                     |
Add job VM dma-resv bookkeeping slot (non-compute mode)                |
Add job to external BOs dma-resv write slots (non-compute mode)        |
Check if any userptrs invalidated since pin ------ Drop locks ---------|
Install in / out fences for job
Submit job
Unlock all
```
