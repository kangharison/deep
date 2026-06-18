# Asynchronous VM_BIND

> 출처(원문): https://docs.kernel.org/gpu/drm-vm-bind-async.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Asynchronous VM\_BIND

## Nomenclature:

* `VRAM`: On-device memory. Sometimes referred to as device local memory.
* `gpu_vm`: A virtual GPU address space. Typically per process, but
  can be shared by multiple processes.
* `VM_BIND`: An operation or a list of operations to modify a gpu\_vm using
  an IOCTL. The operations include mapping and unmapping system- or
  VRAM memory.
* `syncobj`: A container that abstracts synchronization objects. The
  synchronization objects can be either generic, like dma-fences or
  driver specific. A syncobj typically indicates the type of the
  underlying synchronization object.
* `in-syncobj`: Argument to a VM\_BIND IOCTL, the VM\_BIND operation waits
  for these before starting.
* `out-syncobj`: Argument to a VM\_BIND\_IOCTL, the VM\_BIND operation
  signals these when the bind operation is complete.
* `dma-fence`: A cross-driver synchronization object. A basic
  understanding of dma-fences is required to digest this
  document. Please refer to the `DMA Fences` section of the
  [dma-buf doc](../driver-api/dma-buf.html).
* `memory fence`: A synchronization object, different from a dma-fence.
  A memory fence uses the value of a specified memory location to determine
  signaled status. A memory fence can be awaited and signaled by both
  the GPU and CPU. Memory fences are sometimes referred to as
  user-fences, userspace-fences or gpu futexes and do not necessarily obey
  the dma-fence rule of signaling within a “reasonable amount of time”.
  The kernel should thus avoid waiting for memory fences with locks held.
* `long-running workload`: A workload that may take more than the
  current stipulated dma-fence maximum signal delay to complete and
  which therefore needs to set the gpu\_vm or the GPU execution context in
  a certain mode that disallows completion dma-fences.
* `exec function`: An exec function is a function that revalidates all
  affected gpu\_vmas, submits a GPU command batch and registers the
  dma\_fence representing the GPU command’s activity with all affected
  dma\_resvs. For completeness, although not covered by this document,
  it’s worth mentioning that an exec function may also be the
  revalidation worker that is used by some drivers in compute /
  long-running mode.
* `bind context`: A context identifier used for the VM\_BIND
  operation. VM\_BIND operations that use the same bind context can be
  assumed, where it matters, to complete in order of submission. No such
  assumptions can be made for VM\_BIND operations using separate bind contexts.
* `UMD`: User-mode driver.
* `KMD`: Kernel-mode driver.

## Synchronous / Asynchronous VM\_BIND operation

### Synchronous VM\_BIND

With Synchronous VM\_BIND, the VM\_BIND operations all complete before the
IOCTL returns. A synchronous VM\_BIND takes neither in-fences nor
out-fences. Synchronous VM\_BIND may block and wait for GPU operations;
for example swap-in or clearing, or even previous binds.

### Asynchronous VM\_BIND

Asynchronous VM\_BIND accepts both in-syncobjs and out-syncobjs. While the
IOCTL may return immediately, the VM\_BIND operations wait for the in-syncobjs
before modifying the GPU page-tables, and signal the out-syncobjs when
the modification is done in the sense that the next exec function that
awaits for the out-syncobjs will see the change. Errors are reported
synchronously.
In low-memory situations the implementation may block, performing the
VM\_BIND synchronously, because there might not be enough memory
immediately available for preparing the asynchronous operation.

If the VM\_BIND IOCTL takes a list or an array of operations as an argument,
the in-syncobjs needs to signal before the first operation starts to
execute, and the out-syncobjs signal after the last operation
completes. Operations in the operation list can be assumed, where it
matters, to complete in order.

Since asynchronous VM\_BIND operations may use dma-fences embedded in
out-syncobjs and internally in KMD to signal bind completion, any
memory fences given as VM\_BIND in-fences need to be awaited
synchronously before the VM\_BIND ioctl returns, since dma-fences,
required to signal in a reasonable amount of time, can never be made
to depend on memory fences that don’t have such a restriction.

The purpose of an Asynchronous VM\_BIND operation is for user-mode
drivers to be able to pipeline interleaved gpu\_vm modifications and
exec functions. For long-running workloads, such pipelining of a bind
operation is not allowed and any in-fences need to be awaited
synchronously. The reason for this is twofold. First, any memory
fences gated by a long-running workload and used as in-syncobjs for the
VM\_BIND operation will need to be awaited synchronously anyway (see
above). Second, any dma-fences used as in-syncobjs for VM\_BIND
operations for long-running workloads will not allow for pipelining
anyway since long-running workloads don’t allow for dma-fences as
out-syncobjs, so while theoretically possible the use of them is
questionable and should be rejected until there is a valuable use-case.
Note that this is not a limitation imposed by dma-fence rules, but
rather a limitation imposed to keep KMD implementation simple. It does
not affect using dma-fences as dependencies for the long-running
workload itself, which is allowed by dma-fence rules, but rather for
the VM\_BIND operation only.

An asynchronous VM\_BIND operation may take substantial time to
complete and signal the out\_fence. In particular if the operation is
deeply pipelined behind other VM\_BIND operations and workloads
submitted using exec functions. In that case, UMD might want to avoid a
subsequent VM\_BIND operation to be queued behind the first one if
there are no explicit dependencies. In order to circumvent such a queue-up, a
VM\_BIND implementation may allow for VM\_BIND contexts to be
created. For each context, VM\_BIND operations will be guaranteed to
complete in the order they were submitted, but that is not the case
for VM\_BIND operations executing on separate VM\_BIND contexts. Instead
KMD will attempt to execute such VM\_BIND operations in parallel but
leaving no guarantee that they will actually be executed in
parallel. There may be internal implicit dependencies that only KMD knows
about, for example page-table structure changes. A way to attempt
to avoid such internal dependencies is to have different VM\_BIND
contexts use separate regions of a VM.

Also for VM\_BINDS for long-running gpu\_vms the user-mode driver should typically
select memory fences as out-fences since that gives greater flexibility for
the kernel mode driver to inject other operations into the bind /
unbind operations. Like for example inserting breakpoints into batch
buffers. The workload execution can then easily be pipelined behind
the bind completion using the memory out-fence as the signal condition
for a GPU semaphore embedded by UMD in the workload.

There is no difference in the operations supported or in
multi-operation support between asynchronous VM\_BIND and synchronous VM\_BIND.

## Multi-operation VM\_BIND IOCTL error handling and interrupts

The VM\_BIND operations of the IOCTL may error for various reasons, for
example due to lack of resources to complete and due to interrupted
waits.
In these situations UMD should preferably restart the IOCTL after
taking suitable action.
If UMD has over-committed a memory resource, an -ENOSPC error will be
returned, and UMD may then unbind resources that are not used at the
moment and rerun the IOCTL. On -EINTR, UMD should simply rerun the
IOCTL and on -ENOMEM user-space may either attempt to free known
system memory resources or fail. In case of UMD deciding to fail a
bind operation, due to an error return, no additional action is needed
to clean up the failed operation, and the VM is left in the same state
as it was before the failing IOCTL.
Unbind operations are guaranteed not to return any errors due to
resource constraints, but may return errors due to, for example,
invalid arguments or the gpu\_vm being banned.
In the case an unexpected error happens during the asynchronous bind
process, the gpu\_vm will be banned, and attempts to use it after banning
will return -ENOENT.

## Example: The Xe VM\_BIND uAPI

Starting with the VM\_BIND operation struct, the IOCTL call can take
zero, one or many such operations. A zero number means only the
synchronization part of the IOCTL is carried out: an asynchronous
VM\_BIND updates the syncobjects, whereas a sync VM\_BIND waits for the
implicit dependencies to be fulfilled.

```
struct drm_xe_vm_bind_op {
     /**
      * @obj: GEM object to operate on, MBZ for MAP_USERPTR, MBZ for UNMAP
      */
     __u32 obj;

     /** @pad: MBZ */
     __u32 pad;

     union {
             /**
              * @obj_offset: Offset into the object for MAP.
              */
             __u64 obj_offset;

             /** @userptr: user virtual address for MAP_USERPTR */
             __u64 userptr;
     };

     /**
      * @range: Number of bytes from the object to bind to addr, MBZ for UNMAP_ALL
      */
     __u64 range;

     /** @addr: Address to operate on, MBZ for UNMAP_ALL */
     __u64 addr;

     /**
      * @tile_mask: Mask for which tiles to create binds for, 0 == All tiles,
      * only applies to creating new VMAs
      */
     __u64 tile_mask;

    /* Map (parts of) an object into the GPU virtual address range.
 #define XE_VM_BIND_OP_MAP           0x0
     /* Unmap a GPU virtual address range */
 #define XE_VM_BIND_OP_UNMAP         0x1
     /*
      * Map a CPU virtual address range into a GPU virtual
      * address range.
      */
 #define XE_VM_BIND_OP_MAP_USERPTR   0x2
     /* Unmap a gem object from the VM. */
 #define XE_VM_BIND_OP_UNMAP_ALL     0x3
     /*
      * Make the backing memory of an address range resident if
      * possible. Note that this doesn't pin backing memory.
      */
 #define XE_VM_BIND_OP_PREFETCH      0x4

     /* Make the GPU map readonly. */
 #define XE_VM_BIND_FLAG_READONLY    (0x1 << 16)
     /*
      * Valid on a faulting VM only, do the MAP operation immediately rather
      * than deferring the MAP to the page fault handler.
      */
 #define XE_VM_BIND_FLAG_IMMEDIATE   (0x1 << 17)
     /*
      * When the NULL flag is set, the page tables are setup with a special
      * bit which indicates writes are dropped and all reads return zero.  In
      * the future, the NULL flags will only be valid for XE_VM_BIND_OP_MAP
      * operations, the BO handle MBZ, and the BO offset MBZ. This flag is
      * intended to implement VK sparse bindings.
      */
 #define XE_VM_BIND_FLAG_NULL        (0x1 << 18)
     /** @op: Operation to perform (lower 16 bits) and flags (upper 16 bits) */
     __u32 op;

     /** @mem_region: Memory region to prefetch VMA to, instance not a mask */
     __u32 region;

     /** @reserved: Reserved */
     __u64 reserved[2];
};
```

The VM\_BIND IOCTL argument itself, looks like follows. Note that for
synchronous VM\_BIND, the num\_syncs and syncs fields must be zero. Here
the `exec_queue_id` field is the VM\_BIND context discussed previously
that is used to facilitate out-of-order VM\_BINDs.

```
struct drm_xe_vm_bind {
    /** @extensions: Pointer to the first extension struct, if any */
    __u64 extensions;

    /** @vm_id: The ID of the VM to bind to */
    __u32 vm_id;

    /**
     * @exec_queue_id: exec_queue_id, must be of class DRM_XE_ENGINE_CLASS_VM_BIND
     * and exec queue must have same vm_id. If zero, the default VM bind engine
     * is used.
     */
    __u32 exec_queue_id;

    /** @num_binds: number of binds in this IOCTL */
    __u32 num_binds;

    /* If set, perform an async VM_BIND, if clear a sync VM_BIND */
#define XE_VM_BIND_IOCTL_FLAG_ASYNC (0x1 << 0)

    /** @flag: Flags controlling all operations in this ioctl. */
    __u32 flags;

    union {
            /** @bind: used if num_binds == 1 */
            struct drm_xe_vm_bind_op bind;

            /**
             * @vector_of_binds: userptr to array of struct
             * drm_xe_vm_bind_op if num_binds > 1
             */
            __u64 vector_of_binds;
    };

    /** @num_syncs: amount of syncs to wait for or to signal on completion. */
    __u32 num_syncs;

    /** @pad2: MBZ */
    __u32 pad2;

    /** @syncs: pointer to struct drm_xe_sync array */
    __u64 syncs;

    /** @reserved: Reserved */
    __u64 reserved[2];
};
```
