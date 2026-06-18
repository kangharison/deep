# drm/v3d Broadcom V3D Graphics Driver

> 출처(원문): https://docs.kernel.org/gpu/v3d.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/v3d Broadcom V3D Graphics Driver

This driver supports the Broadcom V3D 3.3 and 4.1 OpenGL ES GPUs.
For V3D 2.x support, see the VC4 driver.

The V3D GPU includes a tiled render (composed of a bin and render
pipelines), the TFU (texture formatting unit), and the CSD (compute
shader dispatch).

## GPU buffer object (BO) management

Compared to VC4 (V3D 2.x), V3D 3.3 introduces an MMU between the
GPU and the bus, allowing us to use shmem objects for our storage
instead of CMA.

Physically contiguous objects may still be imported to V3D, but the
driver doesn’t allocate physically contiguous objects on its own.
Display engines requiring physically contiguous allocations should
look into Mesa’s “renderonly” support (as used by the Mesa pl111
driver) for an example of how to integrate with V3D.

### Address space management

The V3D 3.x hardware (compared to VC4) now includes an MMU. It has
a single level of page tables for the V3D’s 4GB address space to
map to AXI bus addresses, thus it could need up to 4MB of
physically contiguous memory to store the PTEs.

Because the 4MB of contiguous memory for page tables is precious,
and switching between them is expensive, we load all BOs into the
same 4GB address space.

To protect clients from each other, we should use the GMP to
quickly mask out (at 128kb granularity) what pages are available to
each client. This is not yet implemented.

### GPU Scheduling

The shared DRM GPU scheduler is used to coordinate submitting jobs
to the hardware. Each DRM fd (roughly a client process) gets its
own scheduler entity, which will process jobs in order. The GPU
scheduler will schedule the clients with a FIFO scheduling algorithm.

For simplicity, and in order to keep latency low for interactive
jobs when bulk background jobs are queued up, we submit a new job
to the HW only when it has completed the last one, instead of
filling up the CT[01]Q FIFOs with jobs. Similarly, we use
[`drm_sched_job_add_dependency()`](drm-mm.html#c.drm_sched_job_add_dependency "drm_sched_job_add_dependency") to manage the dependency between bin
and render, instead of having the clients submit jobs using the HW’s
semaphores to interlock between them.

## Interrupts

When we take a bin, render, TFU done, or CSD done interrupt, we
need to signal the fence for that job so that the scheduler can
queue up the next one and unblock any waiters.

When we take the binner out of memory interrupt, we need to
allocate some new memory and pass it to the binner so that the
current job can make progress.
