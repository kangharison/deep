# Execution Queue

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_exec_queue.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Execution Queue

An Execution queue is an interface for the HW context of execution.
The user creates an execution queue, submits the GPU jobs through those
queues and in the end destroys them.

Execution queues can also be created by XeKMD itself for driver internal
operations like object migration etc.

An execution queue is associated with a specified HW engine or a group of
engines (belonging to the same tile and engine class) and any GPU job
submitted on the queue will be run on one of these engines.

An execution queue is tied to an address space (VM). It holds a reference
of the associated VM and the underlying Logical Ring Context/s (LRC/s)
until the queue is destroyed.

The execution queue sits on top of the submission backend. It opaquely
handles the GuC and Execlist backends whichever the platform uses, and
the ring operations the different engine classes support.

## Multi Queue Group

Multi Queue Group is another mode of execution supported by the compute
and blitter copy command streamers (CCS and BCS, respectively). It is
an enhancement of the existing hardware architecture and leverages the
same submission model. It enables support for efficient, parallel
execution of multiple queues within a single shared context. The multi
queue group functionality is only supported with GuC submission backend.
All the queues of a group must use the same address space (VM).

The DRM\_XE\_EXEC\_QUEUE\_SET\_PROPERTY\_MULTI\_QUEUE execution queue property
supports creating a multi queue group and adding queues to a queue group.

The XE\_EXEC\_QUEUE\_CREATE ioctl call with above property with value field
set to DRM\_XE\_MULTI\_GROUP\_CREATE, will create a new multi queue group with
the queue being created as the primary queue (aka q0) of the group. To add
secondary queues to the group, they need to be created with the above
property with id of the primary queue as the value. The properties of
the primary queue (like priority, time slice) applies to the whole group.
So, these properties can’t be set for secondary queues of a group.

The hardware does not support removing a queue from a multi-queue group.
However, queues can be dynamically added to the group. A group can have
up to 64 queues. To support this, XeKMD holds references to LRCs of the
queues even after the queues are destroyed by the user until the whole
group is destroyed. The secondary queues hold a reference to the primary
queue thus preventing the group from being destroyed when user destroys
the primary queue. Once the primary queue is destroyed, secondary queues
can’t be added to the queue group and new job submissions on existing
secondary queues are not allowed.

The queues of a multi queue group can set their priority within the group
through the DRM\_XE\_EXEC\_QUEUE\_SET\_PROPERTY\_MULTI\_QUEUE\_PRIORITY property.
This multi queue priority can also be set dynamically through the
XE\_EXEC\_QUEUE\_SET\_PROPERTY ioctl. This is the only other property
supported by the secondary queues of a multi queue group, other than
DRM\_XE\_EXEC\_QUEUE\_SET\_PROPERTY\_MULTI\_QUEUE.

When GuC reports an error on any of the queues of a multi queue group,
the queue cleanup mechanism is invoked for all the queues of the group
as hardware cannot make progress on the multi queue context.

Refer [Multi Queue Group GuC interface](#multi-queue-group-guc-interface) for multi queue group GuC
interface.

## Multi Queue Group GuC interface

The multi queue group coordination between KMD and GuC is through a software
construct called Context Group Page (CGP). The CGP is a KMD managed 4KB page
allocated in the global GTT.

CGP format:

|  |  |  |
| --- | --- | --- |
| DWORD | Name | Description |
| 0 | Version | Bits [15:8]=Major ver, [7:0]=Minor ver |
| 1..15 | RESERVED | MBZ |
| 16 | KMD\_QUEUE\_UPDATE\_MASK\_DW0 | KMD queue mask for queues 31..0 |
| 17 | KMD\_QUEUE\_UPDATE\_MASK\_DW1 | KMD queue mask for queues 63..32 |
| 18..31 | RESERVED | MBZ |
| 32 | Q0CD\_DW0 | Queue 0 context LRC descriptor lower DWORD |
| 33 | Q0ContextIndex | Context ID for Queue 0 |
| 34 | Q1CD\_DW0 | Queue 1 context LRC descriptor lower DWORD |
| 35 | Q1ContextIndex | Context ID for Queue 1 |
| ... | ... | ... |
| 158 | Q63CD\_DW0 | Queue 63 context LRC descriptor lower DWORD |
| 159 | Q63ContextIndex | Context ID for Queue 63 |
| 160..1024 | RESERVED | MBZ |

While registering Q0 with GuC, CGP is updated with Q0 entry and GuC is notified
through XE\_GUC\_ACTION\_REGISTER\_CONTEXT\_MULTI\_QUEUE H2G message which specifies
the CGP address. When the secondary queues are added to the group, the CGP is
updated with entry for that queue and GuC is notified through the H2G interface
XE\_GUC\_ACTION\_MULTI\_QUEUE\_CONTEXT\_CGP\_SYNC. GuC responds to these H2G messages
with a XE\_GUC\_ACTION\_NOTIFY\_MULTIQ\_CONTEXT\_CGP\_SYNC\_DONE G2H message. GuC also
sends a XE\_GUC\_ACTION\_NOTIFY\_MULTI\_QUEUE\_CGP\_CONTEXT\_ERROR notification for any
error in the CGP. Only one of these CGP update messages can be outstanding
(waiting for GuC response) at any time. The bits in KMD\_QUEUE\_UPDATE\_MASK\_DW\*
fields indicate which queue entry is being updated in the CGP.

The primary queue (Q0) represents the multi queue group context in GuC and
submission on any queue of the group must be through Q0 GuC interface only.

As it is not required to register secondary queues with GuC, the secondary queue
context ids in the CGP are populated with Q0 context id.

## Internal API

enum xe\_multi\_queue\_priority
:   Multi Queue priority values

**Constants**

`XE_MULTI_QUEUE_PRIORITY_LOW`
:   Priority low

`XE_MULTI_QUEUE_PRIORITY_NORMAL`
:   Priority normal

`XE_MULTI_QUEUE_PRIORITY_HIGH`
:   Priority high

**Description**

The priority values of the queues within the multi queue group.

struct xe\_exec\_queue\_group
:   Execution multi queue group

**Definition**:

```
struct xe_exec_queue_group {
    struct xe_exec_queue *primary;
    struct xe_bo *cgp_bo;
    struct xarray xa;
    struct list_head list;
    struct mutex list_lock;
    bool sync_pending;
    bool banned;
    bool stopped;
};
```

**Members**

`primary`
:   Primary queue of this group

`cgp_bo`
:   BO for the Context Group Page

`xa`
:   xarray to store LRCs

`list`
:   List of all secondary queues in the group

`list_lock`
:   Secondary queue list lock

`sync_pending`
:   CGP\_SYNC\_DONE g2h response pending

`banned`
:   Group banned

`stopped`
:   Group is stopped, protected by list\_lock

**Description**

Contains multi queue group information.

struct xe\_exec\_queue
:   Execution queue

**Definition**:

```
struct xe_exec_queue {
    struct xe_file *xef;
    struct xe_gt *gt;
    struct xe_hw_engine *hwe;
    struct kref refcount;
    struct xe_vm *vm;
    struct xe_vm *user_vm;
    enum xe_engine_class class;
    u32 logical_mask;
    char name[MAX_FENCE_NAME_LEN];
    u16 width;
    u16 msix_vec;
    struct xe_hw_fence_irq *fence_irq;
    struct dma_fence *last_fence;
#define EXEC_QUEUE_FLAG_KERNEL                  BIT(0);
#define EXEC_QUEUE_FLAG_PERMANENT               BIT(1);
#define EXEC_QUEUE_FLAG_VM                      BIT(2);
#define EXEC_QUEUE_FLAG_BIND_ENGINE_CHILD       BIT(3);
#define EXEC_QUEUE_FLAG_HIGH_PRIORITY           BIT(4);
#define EXEC_QUEUE_FLAG_LOW_LATENCY             BIT(5);
#define EXEC_QUEUE_FLAG_MIGRATE                 BIT(6);
#define EXEC_QUEUE_FLAG_DISABLE_STATE_CACHE_PERF_FIX    BIT(7);
    unsigned long flags;
    union {
        struct list_head multi_gt_list;
        struct list_head multi_gt_link;
    };
    union {
        struct xe_execlist_exec_queue *execlist;
        struct xe_guc_exec_queue *guc;
    };
    struct {
        struct xe_exec_queue_group *group;
        struct list_head link;
        enum xe_multi_queue_priority priority;
        spinlock_t lock;
        u8 pos;
        u8 valid:1;
        u8 is_primary:1;
    } multi_queue;
    struct {
        u32 timeslice_us;
        u32 preempt_timeout_us;
        u32 job_timeout_ms;
        enum xe_exec_queue_priority priority;
    } sched_props;
    struct {
        struct dma_fence *pfence;
        u64 context;
        u32 seqno;
        struct list_head link;
    } lr;
#define XE_EXEC_QUEUE_TLB_INVAL_PRIMARY_GT      0;
#define XE_EXEC_QUEUE_TLB_INVAL_MEDIA_GT        1;
#define XE_EXEC_QUEUE_TLB_INVAL_COUNT           (XE_EXEC_QUEUE_TLB_INVAL_MEDIA_GT  + 1);
    struct {
        struct xe_dep_scheduler *dep_scheduler;
        struct dma_fence *last_fence;
    } tlb_inval[XE_EXEC_QUEUE_TLB_INVAL_COUNT];
    struct list_head vm_exec_queue_link;
    struct {
        u8 type;
        struct list_head link;
    } pxp;
    struct drm_syncobj *ufence_syncobj;
    u64 ufence_timeline_value;
    void *replay_state;
    const struct xe_exec_queue_ops *ops;
    const struct xe_ring_ops *ring_ops;
    struct drm_sched_entity *entity;
#define XE_MAX_JOB_COUNT_PER_EXEC_QUEUE 1000;
    atomic_t job_cnt;
    u64 tlb_flush_seqno;
    struct list_head hw_engine_group_link;
    spinlock_t lrc_lookup_lock;
    struct xe_lrc *lrc[];
};
```

**Members**

`xef`
:   Back pointer to xe file if this is user created exec queue

`gt`
:   GT structure this exec queue can submit to

`hwe`
:   A hardware of the same class. May (physical engine) or may not
    (virtual engine) be where jobs actual engine up running. Should never
    really be used for submissions.

`refcount`
:   ref count of this exec queue

`vm`
:   VM (address space) for this exec queue

`user_vm`
:   User VM (address space) for this exec queue (bind queues
    only)

`class`
:   class of this exec queue

`logical_mask`
:   logical mask of where job submitted to exec queue can run

`name`
:   name of this exec queue

`width`
:   width (number BB submitted per exec) of this exec queue

`msix_vec`
:   MSI-X vector (for platforms that support it)

`fence_irq`
:   fence IRQ used to signal job completion

`last_fence`
:   last fence for tlb invalidation, protected by
    vm->lock in write mode

`flags`
:   flags for this exec queue, should statically setup aside from ban
    bit

`{unnamed_union}`
:   anonymous

`multi_gt_list`
:   list head for VM bind engines if multi-GT

`multi_gt_link`
:   link for VM bind engines if multi-GT

`{unnamed_union}`
:   anonymous

`execlist`
:   execlist backend specific state for exec queue

`guc`
:   GuC backend specific state for exec queue

`multi_queue`
:   Multi queue information

`multi_queue.group`
:   Queue group information

`multi_queue.link`
:   Link into group’s secondary queues list

`multi_queue.priority`
:   Queue priority within the multi-queue group.
    It is protected by **multi\_queue.lock**.

`multi_queue.lock`
:   Lock for protecting certain members

`multi_queue.pos`
:   Position of queue within the multi-queue group

`multi_queue.valid`
:   Queue belongs to a multi queue group

`multi_queue.is_primary`
:   Is primary queue (Q0) of the group

`sched_props`
:   scheduling properties

`sched_props.timeslice_us`
:   timeslice period in micro-seconds

`sched_props.preempt_timeout_us`
:   preemption timeout in micro-seconds

`sched_props.job_timeout_ms`
:   job timeout in milliseconds

`sched_props.priority`
:   priority of this exec queue

`lr`
:   long-running exec queue state

`lr.pfence`
:   preemption fence

`lr.context`
:   preemption fence context

`lr.seqno`
:   preemption fence seqno

`lr.link`
:   link into VM’s list of exec queues

`tlb_inval`
:   TLB invalidations exec queue state

`tlb_inval.dep_scheduler`
:   The TLB invalidation
    dependency scheduler

`vm_exec_queue_link`
:   Link to track exec queue within a VM’s list of exec queues.

`pxp`
:   PXP info tracking

`pxp.type`
:   PXP session type used by this queue

`pxp.link`
:   link into the list of PXP exec queues

`ufence_syncobj`
:   User fence syncobj

`ufence_timeline_value`
:   User fence timeline value

`replay_state`
:   GPU hang replay state

`ops`
:   submission backend exec queue operations

`ring_ops`
:   ring operations for this exec queue

`entity`
:   DRM sched entity for this exec queue (1 to 1 relationship)

`job_cnt`
:   number of drm jobs in this exec queue

`tlb_flush_seqno`
:   The seqno of the last rebind tlb flush performed
    Protected by **vm**’s resv. Unused if **vm** == NULL.

`hw_engine_group_link`
:   link into exec queues in the same hw engine group

`lrc_lookup_lock`
:   Lock for protecting lrc array access. Only used when
    running in parallel to queue creation is possible.

`lrc`
:   logical ring context for this exec queue

**Description**

Contains all state necessary for submissions. Can either be a user object or
a kernel object.

struct xe\_exec\_queue\_ops
:   Submission backend exec queue operations

**Definition**:

```
struct xe_exec_queue_ops {
    int (*init)(struct xe_exec_queue *q);
    void (*kill)(struct xe_exec_queue *q);
    void (*fini)(struct xe_exec_queue *q);
    void (*destroy)(struct xe_exec_queue *q);
    int (*set_priority)(struct xe_exec_queue *q, enum xe_exec_queue_priority priority);
    int (*set_timeslice)(struct xe_exec_queue *q, u32 timeslice_us);
    int (*set_preempt_timeout)(struct xe_exec_queue *q, u32 preempt_timeout_us);
    int (*set_multi_queue_priority)(struct xe_exec_queue *q, enum xe_multi_queue_priority priority);
    int (*suspend)(struct xe_exec_queue *q);
    int (*suspend_wait)(struct xe_exec_queue *q);
    void (*resume)(struct xe_exec_queue *q);
    bool (*reset_status)(struct xe_exec_queue *q);
    bool (*active)(struct xe_exec_queue *q);
};
```

**Members**

`init`
:   Initialize exec queue for submission backend

`kill`
:   Kill inflight submissions for backend

`fini`
:   Undoes the `init()` for submission backend

`destroy`
:   Destroy exec queue for submission backend. The backend
    function must call [`xe_exec_queue_fini()`](#c.xe_exec_queue_fini "xe_exec_queue_fini") (which will in turn call the
    `fini()` backend function) to ensure the queue is properly cleaned up.

`set_priority`
:   Set priority for exec queue

`set_timeslice`
:   Set timeslice for exec queue

`set_preempt_timeout`
:   Set preemption timeout for exec queue

`set_multi_queue_priority`
:   Set multi queue priority

`suspend`
:   Suspend exec queue from executing, allowed to be called
    multiple times in a row before resume with the caveat that
    suspend\_wait returns before calling suspend again.

`suspend_wait`
:   Wait for an exec queue to suspend executing, should be
    call after suspend. In dma-fencing path thus must return within a
    reasonable amount of time. -ETIME return shall indicate an error
    waiting for suspend resulting in associated VM getting killed.
    -EAGAIN return indicates the wait should be tried again, if the wait
    is within a work item, the work item should be requeued as deadlock
    avoidance mechanism.

`resume`
:   Resume exec queue execution, exec queue must be in a suspended
    state and dma fence returned from most recent suspend call must be
    signalled when this function is called.

`reset_status`
:   check exec queue reset status

`active`
:   check exec queue is active

bool xe\_exec\_queue\_is\_multi\_queue(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Whether an exec\_queue is part of a queue group.

**Parameters**

`struct xe_exec_queue *q`
:   The exec\_queue

**Return**

True if the exec\_queue is part of a queue group, false otherwise.

bool xe\_exec\_queue\_is\_multi\_queue\_primary(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Whether an exec\_queue is primary queue of a multi queue group.

**Parameters**

`struct xe_exec_queue *q`
:   The exec\_queue

**Return**

True if **q** is primary queue of a queue group, false otherwise.

bool xe\_exec\_queue\_is\_multi\_queue\_secondary(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Whether an exec\_queue is secondary queue of a multi queue group.

**Parameters**

`struct xe_exec_queue *q`
:   The exec\_queue

**Return**

True if **q** is secondary queue of a queue group, false otherwise.

struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*xe\_exec\_queue\_multi\_queue\_primary(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Get multi queue group’s primary queue

**Parameters**

`struct xe_exec_queue *q`
:   The exec\_queue

**Description**

If **q** belongs to a multi queue group, then the primary queue of the group will
be returned. Otherwise, **q** will be returned.

struct xe\_lrc \*xe\_exec\_queue\_get\_lrc(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, u16 idx)
:   Get the LRC from exec queue.

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue instance.

`u16 idx`
:   Index within multi-LRC array.

**Description**

Retrieves LRC of given index for the exec queue under lock
and takes reference.

**Return**

Pointer to LRC on success, error on failure, NULL on
lookup failure.

struct xe\_lrc \*xe\_exec\_queue\_lrc(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Get the LRC from exec queue.

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue instance.

**Description**

Retrieves the primary LRC for the exec queue. Note that this function
returns only the first LRC instance, even when multiple parallel LRCs
are configured. This function does not increment reference count,
so the reference can be just forgotten after use.

**Return**

Pointer to LRC on success, error on failure

struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*xe\_exec\_queue\_create(struct xe\_device \*xe, struct xe\_vm \*vm, u32 logical\_mask, u16 width, struct xe\_hw\_engine \*hwe, u32 flags, u64 extensions)
:   Create an exec queue

**Parameters**

`struct xe_device *xe`
:   Xe device

`struct xe_vm *vm`
:   VM for the exec queue

`u32 logical_mask`
:   Logical mask of HW engines

`u16 width`
:   Width of the exec queue (number of LRCs)

`struct xe_hw_engine *hwe`
:   Hardware engine

`u32 flags`
:   Exec queue creation flags

`u64 extensions`
:   Extensions for exec queue creation

**Description**

Create an exec queue (allocate and initialize) with the specified parameters

**Return**

Pointer to the created exec queue on success, ERR\_PTR on failure

struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*xe\_exec\_queue\_create\_class(struct xe\_device \*xe, struct xe\_gt \*gt, struct xe\_vm \*vm, enum xe\_engine\_class class, u32 flags, u64 extensions)
:   Create an exec queue for a specific engine class

**Parameters**

`struct xe_device *xe`
:   Xe device

`struct xe_gt *gt`
:   GT for the exec queue

`struct xe_vm *vm`
:   VM for the exec queue

`enum xe_engine_class class`
:   Engine class

`u32 flags`
:   Exec queue creation flags

`u64 extensions`
:   Extensions for exec queue creation

**Description**

Create an exec queue for the specified engine class.

**Return**

Pointer to the created exec queue on success, ERR\_PTR on failure

struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*xe\_exec\_queue\_create\_bind(struct xe\_device \*xe, struct xe\_tile \*tile, struct xe\_vm \*user\_vm, u32 flags, u64 extensions)
:   Create bind exec queue.

**Parameters**

`struct xe_device *xe`
:   Xe device.

`struct xe_tile *tile`
:   tile which bind exec queue belongs to.

`struct xe_vm *user_vm`
:   The user VM which this exec queue belongs to

`u32 flags`
:   exec queue creation flags

`u64 extensions`
:   exec queue creation extensions

**Description**

Normalize bind exec queue creation. Bind exec queue is tied to migration VM
for access to physical memory required for page table programming. On a
faulting devices the reserved copy engine instance must be used to avoid
deadlocking (user binds cannot get stuck behind faults as kernel binds which
resolve faults depend on user binds). On non-faulting devices any copy engine
can be used.

Returns exec queue on success, ERR\_PTR on failure

void xe\_exec\_queue\_destroy(struct kref \*ref)
:   Destroy an exec queue

**Parameters**

`struct kref *ref`
:   Reference count of the exec queue

**Description**

Called when the last reference to the exec queue is dropped.
Cleans up all resources associated with the exec queue.
This function should not be called directly; use `xe_exec_queue_put()` instead.

void xe\_exec\_queue\_fini(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Finalize an exec queue

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

**Description**

Finalizes the exec queue by updating run ticks, releasing LRC references,
and freeing the queue structure. This is called after the queue has been
destroyed and all references have been dropped.

void xe\_exec\_queue\_assign\_name(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, u32 instance)
:   Assign a name to an exec queue

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`u32 instance`
:   Instance number for the engine

**Description**

Assigns a human-readable name to the exec queue based on its engine class
and instance number (e.g., “rcs0”, “vcs1”, “bcs2”).

struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*xe\_exec\_queue\_lookup(struct xe\_file \*xef, u32 id)
:   Look up an exec queue by ID

**Parameters**

`struct xe_file *xef`
:   Xe file private data

`u32 id`
:   Exec queue ID

**Description**

Looks up an exec queue by its ID and increments its reference count.

**Return**

Pointer to the exec queue if found, NULL otherwise

enum xe\_exec\_queue\_priority xe\_exec\_queue\_device\_get\_max\_priority(struct xe\_device \*xe)
:   Get maximum priority for an exec queues

**Parameters**

`struct xe_device *xe`
:   Xe device

**Description**

Returns the maximum priority level that can be assigned to an exec queues.

**Return**

Maximum priority level (HIGH if CAP\_SYS\_NICE, NORMAL otherwise)

int xe\_exec\_queue\_set\_property\_ioctl(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](../drm-internals.html#c.drm_file "drm_file") \*file)
:   Set a property on an exec queue

**Parameters**

`struct drm_device *dev`
:   DRM device

`void *data`
:   IOCTL data

`struct drm_file *file`
:   DRM file

**Description**

Allows setting properties on an existing exec queue. Currently only
supports setting multi-queue priority.

**Return**

0 on success, negative error code on failure

int xe\_exec\_queue\_create\_ioctl(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](../drm-internals.html#c.drm_file "drm_file") \*file)
:   Create an exec queue via IOCTL

**Parameters**

`struct drm_device *dev`
:   DRM device

`void *data`
:   IOCTL data

`struct drm_file *file`
:   DRM file

**Description**

Creates a new exec queue based on user-provided parameters. Supports
creating VM bind queues, regular exec queues, multi-lrc exec queues
and multi-queue groups.

**Return**

0 on success with exec\_queue\_id filled in, negative error code on failure

int xe\_exec\_queue\_get\_property\_ioctl(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](../drm-internals.html#c.drm_file "drm_file") \*file)
:   Get a property from an exec queue

**Parameters**

`struct drm_device *dev`
:   DRM device

`void *data`
:   IOCTL data

`struct drm_file *file`
:   DRM file

**Description**

Retrieves property values from an existing exec queue. Currently supports
getting the ban/reset status.

**Return**

0 on success with value filled in, negative error code on failure

bool xe\_exec\_queue\_is\_lr(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Whether an exec\_queue is long-running

**Parameters**

`struct xe_exec_queue *q`
:   The exec\_queue

**Return**

True if the exec\_queue is long-running, false otherwise.

bool xe\_exec\_queue\_is\_idle(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Whether an exec\_queue is idle.

**Parameters**

`struct xe_exec_queue *q`
:   The exec\_queue

**Description**

FIXME: Need to determine what to use as the short-lived
timeline lock for the exec\_queues, so that the return value
of this function becomes more than just an advisory
snapshot in time. The timeline lock must protect the
seqno from racing submissions on the same exec\_queue.
Typically vm->resv, but user-created timeline locks use the migrate vm
and never grabs the migrate vm->resv so we have a race there.

**Return**

True if the exec\_queue is idle, false otherwise.

void xe\_exec\_queue\_update\_run\_ticks(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Update run time in ticks for this exec queue from hw

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

**Description**

Update the timestamp saved by HW for this exec queue and save run ticks
calculated by using the delta from last update.

void xe\_exec\_queue\_kill(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   permanently stop all execution from an exec queue

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

**Description**

This function permanently stops all activity on an exec queue. If the queue
is actively executing on the HW, it will be kicked off the engine; any
pending jobs are discarded and all future submissions are rejected.
This function is safe to call multiple times.

int xe\_exec\_queue\_destroy\_ioctl(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](../drm-internals.html#c.drm_file "drm_file") \*file)
:   Destroy an exec queue via IOCTL

**Parameters**

`struct drm_device *dev`
:   DRM device

`void *data`
:   IOCTL data

`struct drm_file *file`
:   DRM file

**Description**

Destroys an existing exec queue and releases its reference.

**Return**

0 on success, negative error code on failure

void xe\_exec\_queue\_last\_fence\_put(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm)
:   Drop ref to last fence

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind or exec for

void xe\_exec\_queue\_last\_fence\_put\_unlocked(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q)
:   Drop ref to last fence unlocked

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

**Description**

Only safe to be called from [`xe_exec_queue_destroy()`](#c.xe_exec_queue_destroy "xe_exec_queue_destroy").

struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*xe\_exec\_queue\_last\_fence\_get(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm)
:   Get last fence

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind or exec for

**Description**

Get last fence, takes a ref

**Return**

last fence if not signaled, dma fence stub if signaled

struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*xe\_exec\_queue\_last\_fence\_get\_for\_resume(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm)
:   Get last fence

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind or exec for

**Description**

Get last fence, takes a ref. Only safe to be called in the context of
resuming the hw engine group’s long-running exec queue, when the group
semaphore is held.

**Return**

last fence if not signaled, dma fence stub if signaled

void xe\_exec\_queue\_last\_fence\_set(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   Set last fence

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind or exec for

`struct dma_fence *fence`
:   The fence

**Description**

Set the last fence for the engine. Increases reference count for fence, when
closing engine xe\_exec\_queue\_last\_fence\_put should be called.

void xe\_exec\_queue\_tlb\_inval\_last\_fence\_put(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm, unsigned int type)
:   Drop ref to last TLB invalidation fence

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind for

`unsigned int type`
:   Either primary or media GT

void xe\_exec\_queue\_tlb\_inval\_last\_fence\_put\_unlocked(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, unsigned int type)
:   Drop ref to last TLB invalidation fence unlocked

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`unsigned int type`
:   Either primary or media GT

**Description**

Only safe to be called from [`xe_exec_queue_destroy()`](#c.xe_exec_queue_destroy "xe_exec_queue_destroy").

struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*xe\_exec\_queue\_tlb\_inval\_last\_fence\_get(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm, unsigned int type)
:   Get last fence for TLB invalidation

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind for

`unsigned int type`
:   Either primary or media GT

**Description**

Get last fence, takes a ref

**Return**

last fence if not signaled, dma fence stub if signaled

void xe\_exec\_queue\_tlb\_inval\_last\_fence\_set(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, struct xe\_vm \*vm, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, unsigned int type)
:   Set last fence for TLB invalidation

**Parameters**

`struct xe_exec_queue *q`
:   The exec queue

`struct xe_vm *vm`
:   The VM the engine does a bind for

`struct dma_fence *fence`
:   The fence

`unsigned int type`
:   Either primary or media GT

**Description**

Set the last fence for the tlb invalidation type on the queue. Increases
reference count for fence, when closing queue
xe\_exec\_queue\_tlb\_inval\_last\_fence\_put should be called.

int xe\_exec\_queue\_contexts\_hwsp\_rebase(struct [xe\_exec\_queue](#c.xe_exec_queue "xe_exec_queue") \*q, void \*scratch)
:   Re-compute GGTT references within all LRCs of a queue.

**Parameters**

`struct xe_exec_queue *q`
:   the [`xe_exec_queue`](#c.xe_exec_queue "xe_exec_queue") `struct instance` containing target LRCs

`void *scratch`
:   scratch buffer to be used as temporary storage

**Return**

zero on success, negative error code on failure
