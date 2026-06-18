# Multi-Queue Block IO Queueing Mechanism (blk-mq)

> 출처(원문): https://docs.kernel.org/block/blk-mq.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Multi-Queue Block IO Queueing Mechanism (blk-mq)

The Multi-Queue Block IO Queueing Mechanism is an API to enable fast storage
devices to achieve a huge number of input/output operations per second (IOPS)
through queueing and submitting IO requests to block devices simultaneously,
benefiting from the parallelism offered by modern storage devices.

## Introduction

### Background

Magnetic hard disks have been the de facto standard from the beginning of the
development of the kernel. The Block IO subsystem aimed to achieve the best
performance possible for those devices with a high penalty when doing random
access, and the bottleneck was the mechanical moving parts, a lot slower than
any layer on the storage stack. One example of such optimization technique
involves ordering read/write requests according to the current position of the
hard disk head.

However, with the development of Solid State Drives and Non-Volatile Memories
without mechanical parts nor random access penalty and capable of performing
high parallel access, the bottleneck of the stack had moved from the storage
device to the operating system. In order to take advantage of the parallelism
in those devices’ design, the multi-queue mechanism was introduced.

The former design had a single queue to store block IO requests with a single
lock. That did not scale well in SMP systems due to dirty data in cache and the
bottleneck of having a single lock for multiple processors. This setup also
suffered with congestion when different processes (or the same process, moving
to different CPUs) wanted to perform block IO. Instead of this, the blk-mq API
spawns multiple queues with individual entry points local to the CPU, removing
the need for a lock. A deeper explanation on how this works is covered in the
following section ([Operation](#operation)).

### Operation

When the userspace performs IO to a block device (reading or writing a file,
for instance), blk-mq takes action: it will store and manage IO requests to
the block device, acting as middleware between the userspace (and a file
system, if present) and the block device driver.

blk-mq has two group of queues: software staging queues and hardware dispatch
queues. When the request arrives at the block layer, it will try the shortest
path possible: send it directly to the hardware queue. However, there are two
cases that it might not do that: if there’s an IO scheduler attached at the
layer or if we want to try to merge requests. In both cases, requests will be
sent to the software queue.

Then, after the requests are processed by software queues, they will be placed
at the hardware queue, a second stage queue where the hardware has direct access
to process those requests. However, if the hardware does not have enough
resources to accept more requests, blk-mq will place requests on a temporary
queue, to be sent in the future, when the hardware is able.

#### Software staging queues

The block IO subsystem adds requests in the software staging queues
(represented by `struct blk_mq_ctx`) in case that they weren’t sent
directly to the driver. A request is one or more BIOs. They arrived at the
block layer through the data structure `struct bio`. The block layer
will then build a new structure from it, the `struct request` that will
be used to communicate with the device driver. Each queue has its own lock and
the number of queues is defined by a per-CPU or per-node basis.

The staging queue can be used to merge requests for adjacent sectors. For
instance, requests for sector 3-6, 6-7, 7-9 can become one request for 3-9.
Even if random access to SSDs and NVMs have the same time of response compared
to sequential access, grouped requests for sequential access decreases the
number of individual requests. This technique of merging requests is called
plugging.

Along with that, the requests can be reordered to ensure fairness of system
resources (e.g. to ensure that no application suffers from starvation) and/or to
improve IO performance, by an IO scheduler.

##### IO Schedulers

There are several schedulers implemented by the block layer, each one following
a heuristic to improve the IO performance. They are “pluggable” (as in plug
and play), in the sense of they can be selected at run time using sysfs. You
can read more about Linux’s IO schedulers [here](https://www.kernel.org/doc/html/latest/block/index.html). The scheduling
happens only between requests in the same queue, so it is not possible to merge
requests from different queues, otherwise there would be cache trashing and a
need to have a lock for each queue. After the scheduling, the requests are
eligible to be sent to the hardware. One of the possible schedulers to be
selected is the NONE scheduler, the most straightforward one. It will just
place requests on whatever software queue the process is running on, without
any reordering. When the device starts processing requests in the hardware
queue (a.k.a. run the hardware queue), the software queues mapped to that
hardware queue will be drained in sequence according to their mapping.

#### Hardware dispatch queues

The hardware queue (represented by [`struct blk_mq_hw_ctx`](#c.blk_mq_hw_ctx "blk_mq_hw_ctx")) is a `struct
used` by device drivers to map the device submission queues (or device DMA ring
buffer), and are the last step of the block layer submission code before the
low level device driver taking ownership of the request. To run this queue, the
block layer removes requests from the associated software queues and tries to
dispatch to the hardware.

If it’s not possible to send the requests directly to hardware, they will be
added to a linked list (`hctx->dispatch`) of requests. Then,
next time the block layer runs a queue, it will send the requests laying at the
`dispatch` list first, to ensure a fairness dispatch with those
requests that were ready to be sent first. The number of hardware queues
depends on the number of hardware contexts supported by the hardware and its
device driver, but it will not be more than the number of cores of the system.
There is no reordering at this stage, and each software queue has a set of
hardware queues to send requests for.

Note

Neither the block layer nor the device protocols guarantee
the order of completion of requests. This must be handled by
higher layers, like the filesystem.

#### Tag-based completion

In order to indicate which request has been completed, every request is
identified by an integer, ranging from 0 to the dispatch queue size. This tag
is generated by the block layer and later reused by the device driver, removing
the need to create a redundant identifier. When a request is completed in the
driver, the tag is sent back to the block layer to notify it of the finalization.
This removes the need to do a linear search to find out which IO has been
completed.

### Further reading

* [Linux Block IO: Introducing Multi-queue SSD Access on Multi-core Systems](http://kernel.dk/blk-mq.pdf)
* [NOOP scheduler](https://en.wikipedia.org/wiki/Noop_scheduler)
* [Null block device driver](https://www.kernel.org/doc/html/latest/block/null_blk.html)

## Source code documentation

enum blk\_eh\_timer\_return
:   How the timeout handler should proceed

**Constants**

`BLK_EH_DONE`
:   The block driver completed the command or will complete it at
    a later time.

`BLK_EH_RESET_TIMER`
:   Reset the request timer and continue waiting for the
    request to complete.

struct blk\_mq\_hw\_ctx
:   State for a hardware queue facing the hardware block device

**Definition**:

```
struct blk_mq_hw_ctx {
    struct {
        spinlock_t lock;
        struct list_head        dispatch;
        unsigned long           state;
    };
    struct delayed_work     run_work;
    cpumask_var_t cpumask;
    int next_cpu;
    int next_cpu_batch;
    unsigned long           flags;
    void *sched_data;
    struct request_queue    *queue;
    struct blk_flush_queue  *fq;
    void *driver_data;
    struct sbitmap          ctx_map;
    struct blk_mq_ctx       *dispatch_from;
    unsigned int            dispatch_busy;
    unsigned short          type;
    unsigned short          nr_ctx;
    struct blk_mq_ctx       **ctxs;
    spinlock_t dispatch_wait_lock;
    wait_queue_entry_t dispatch_wait;
    atomic_t wait_index;
    struct blk_mq_tags      *tags;
    struct blk_mq_tags      *sched_tags;
    unsigned int            numa_node;
    unsigned int            queue_num;
    atomic_t nr_active;
    struct hlist_node       cpuhp_online;
    struct hlist_node       cpuhp_dead;
    struct kobject          kobj;
#ifdef CONFIG_BLK_DEBUG_FS;
    struct dentry           *debugfs_dir;
    struct dentry           *sched_debugfs_dir;
#endif;
    struct list_head        hctx_list;
};
```

**Members**

`{unnamed_struct}`
:   anonymous

`lock`
:   Protects the dispatch list.

`dispatch`
:   Used for requests that are ready to be
    dispatched to the hardware but for some reason (e.g. lack of
    resources) could not be sent to the hardware. As soon as the
    driver can send new requests, requests at this list will
    be sent first for a fairer dispatch.

`state`
:   BLK\_MQ\_S\_\* flags. Defines the state of the hw
    queue (active, scheduled to restart, stopped).

`run_work`
:   Used for scheduling a hardware queue run at a later time.

`cpumask`
:   Map of available CPUs where this hctx can run.

`next_cpu`
:   Used by `blk_mq_hctx_next_cpu()` for round-robin CPU
    selection from **cpumask**.

`next_cpu_batch`
:   Counter of how many works left in the batch before
    changing to the next CPU.

`flags`
:   BLK\_MQ\_F\_\* flags. Defines the behaviour of the queue.

`sched_data`
:   Pointer owned by the IO scheduler attached to a request
    queue. It’s up to the IO scheduler how to use this pointer.

`queue`
:   Pointer to the request queue that owns this hardware context.

`fq`
:   Queue of requests that need to perform a flush operation.

`driver_data`
:   Pointer to data owned by the block driver that created
    this hctx

`ctx_map`
:   Bitmap for each software queue. If bit is on, there is a
    pending request in that software queue.

`dispatch_from`
:   Software queue to be used when no scheduler was
    selected.

`dispatch_busy`
:   Number used by `blk_mq_update_dispatch_busy()` to
    decide if the hw\_queue is busy using Exponential Weighted Moving
    Average algorithm.

`type`
:   HCTX\_TYPE\_\* flags. Type of hardware queue.

`nr_ctx`
:   Number of software queues.

`ctxs`
:   Array of software queues.

`dispatch_wait_lock`
:   Lock for dispatch\_wait queue.

`dispatch_wait`
:   Waitqueue to put requests when there is no tag
    available at the moment, to wait for another try in the future.

`wait_index`
:   Index of next available dispatch\_wait queue to insert
    requests.

`tags`
:   Tags owned by the block driver. A tag at this set is only
    assigned when a request is dispatched from a hardware queue.

`sched_tags`
:   Tags owned by I/O scheduler. If there is an I/O
    scheduler associated with a request queue, a tag is assigned when
    that request is allocated. Else, this member is not used.

`numa_node`
:   NUMA node the storage adapter has been connected to.

`queue_num`
:   Index of this hardware queue.

`nr_active`
:   Number of active requests. Only used when a tag set is
    shared across request queues.

`cpuhp_online`
:   List to store request if CPU is going to die

`cpuhp_dead`
:   List to store request if some CPU die.

`kobj`
:   Kernel object for sysfs.

`debugfs_dir`
:   debugfs directory for this hardware queue. Named
    as cpu<cpu\_number>.

`sched_debugfs_dir`
:   debugfs directory for the scheduler.

`hctx_list`
:   if this hctx is not in use, this is an entry in
    q->unused\_hctx\_list.

struct blk\_mq\_queue\_map
:   Map software queues to hardware queues

**Definition**:

```
struct blk_mq_queue_map {
    unsigned int *mq_map;
    unsigned int nr_queues;
    unsigned int queue_offset;
};
```

**Members**

`mq_map`
:   CPU ID to hardware queue index map. This is an array
    with nr\_cpu\_ids elements. Each element has a value in the range
    [**queue\_offset**, **queue\_offset** + **nr\_queues**).

`nr_queues`
:   Number of hardware queues to map CPU IDs onto.

`queue_offset`
:   First hardware queue to map onto. Used by the PCIe NVMe
    driver to map each hardware queue type ([`enum hctx_type`](#c.hctx_type "hctx_type")) onto a distinct
    set of hardware queues.

enum hctx\_type
:   Type of hardware queue

**Constants**

`HCTX_TYPE_DEFAULT`
:   All I/O not otherwise accounted for.

`HCTX_TYPE_READ`
:   Just for READ I/O.

`HCTX_TYPE_POLL`
:   Polled I/O of any kind.

`HCTX_MAX_TYPES`
:   Number of types of hctx.

struct blk\_mq\_tag\_set
:   tag set that can be shared between request queues

**Definition**:

```
struct blk_mq_tag_set {
    const struct blk_mq_ops *ops;
    struct blk_mq_queue_map map[HCTX_MAX_TYPES];
    unsigned int            nr_maps;
    unsigned int            nr_hw_queues;
    unsigned int            queue_depth;
    unsigned int            reserved_tags;
    unsigned int            cmd_size;
    int numa_node;
    unsigned int            timeout;
    unsigned int            flags;
    void *driver_data;
    struct blk_mq_tags      **tags;
    struct blk_mq_tags      *shared_tags;
    struct mutex            tag_list_lock;
    struct list_head        tag_list;
    struct srcu_struct      *srcu;
    struct srcu_struct      tags_srcu;
    struct rw_semaphore     update_nr_hwq_lock;
};
```

**Members**

`ops`
:   Pointers to functions that implement block driver behavior.

`map`
:   One or more ctx -> hctx mappings. One map exists for each
    hardware queue type ([`enum hctx_type`](#c.hctx_type "hctx_type")) that the driver wishes
    to support. There are no restrictions on maps being of the
    same size, and it’s perfectly legal to share maps between
    types.

`nr_maps`
:   Number of elements in the **map** array. A number in the range
    [1, HCTX\_MAX\_TYPES].

`nr_hw_queues`
:   Number of hardware queues supported by the block driver that
    owns this data structure.

`queue_depth`
:   Number of tags per hardware queue, reserved tags included.

`reserved_tags`
:   Number of tags to set aside for BLK\_MQ\_REQ\_RESERVED tag
    allocations.

`cmd_size`
:   Number of additional bytes to allocate per request. The block
    driver owns these additional bytes.

`numa_node`
:   NUMA node the storage adapter has been connected to.

`timeout`
:   Request processing timeout in jiffies.

`flags`
:   Zero or more BLK\_MQ\_F\_\* flags.

`driver_data`
:   Pointer to data owned by the block driver that created this
    tag set.

`tags`
:   Tag sets. One tag set per hardware queue. Has **nr\_hw\_queues**
    elements.

`shared_tags`
:   Shared set of tags. Has **nr\_hw\_queues** elements. If set,
    shared by all **tags**.

`tag_list_lock`
:   Serializes tag\_list accesses.

`tag_list`
:   List of the request queues that use this tag set. See also
    request\_queue.tag\_set\_list.

`srcu`
:   Use as lock when type of the request queue is blocking
    (BLK\_MQ\_F\_BLOCKING).

`tags_srcu`
:   SRCU used to defer freeing of tags page\_list to prevent
    use-after-free when iterating tags.

`update_nr_hwq_lock`
:   Synchronize updating nr\_hw\_queues with add/del disk &
    switching elevator.

struct blk\_mq\_queue\_data
:   Data about a request inserted in a queue

**Definition**:

```
struct blk_mq_queue_data {
    struct request *rq;
    bool last;
};
```

**Members**

`rq`
:   Request pointer.

`last`
:   If it is the last request in the queue.

struct blk\_mq\_ops
:   Callback functions that implements block driver behaviour.

**Definition**:

```
struct blk_mq_ops {
    blk_status_t (*queue_rq)(struct blk_mq_hw_ctx *, const struct blk_mq_queue_data *);
    void (*commit_rqs)(struct blk_mq_hw_ctx *);
    void (*queue_rqs)(struct rq_list *rqlist);
    int (*get_budget)(struct request_queue *);
    void (*put_budget)(struct request_queue *, int);
    void (*set_rq_budget_token)(struct request *, int);
    int (*get_rq_budget_token)(struct request *);
    enum blk_eh_timer_return (*timeout)(struct request *);
    int (*poll)(struct blk_mq_hw_ctx *, struct io_comp_batch *);
    void (*complete)(struct request *);
    int (*init_hctx)(struct blk_mq_hw_ctx *, void *, unsigned int);
    void (*exit_hctx)(struct blk_mq_hw_ctx *, unsigned int);
    int (*init_request)(struct blk_mq_tag_set *set, struct request *, unsigned int, unsigned int);
    void (*exit_request)(struct blk_mq_tag_set *set, struct request *, unsigned int);
    void (*cleanup_rq)(struct request *);
    bool (*busy)(struct request_queue *);
    void (*map_queues)(struct blk_mq_tag_set *set);
#ifdef CONFIG_BLK_DEBUG_FS;
    void (*show_rq)(struct seq_file *m, struct request *rq);
#endif;
};
```

**Members**

`queue_rq`
:   Queue a new request from block IO.

`commit_rqs`
:   If a driver uses bd->last to judge when to submit
    requests to hardware, it must define this function. In case of errors
    that make us stop issuing further requests, this hook serves the
    purpose of kicking the hardware (which the last request otherwise
    would have done).

`queue_rqs`
:   Queue a list of new requests. Driver is guaranteed
    that each request belongs to the same queue. If the driver doesn’t
    empty the **rqlist** completely, then the rest will be queued
    individually by the block layer upon return.

`get_budget`
:   Reserve budget before queue request, once .queue\_rq is
    run, it is driver’s responsibility to release the
    reserved budget. Also we have to handle failure case
    of .get\_budget for avoiding I/O deadlock.

`put_budget`
:   Release the reserved budget.

`set_rq_budget_token`
:   store rq’s budget token

`get_rq_budget_token`
:   retrieve rq’s budget token

`timeout`
:   Called on request timeout.

`poll`
:   Called to poll for completion of a specific tag.

`complete`
:   Mark the request as complete.

`init_hctx`
:   Called when the block layer side of a hardware queue has
    been set up, allowing the driver to allocate/init matching
    structures.

`exit_hctx`
:   Ditto for exit/teardown.

`init_request`
:   Called for every command allocated by the block layer
    to allow the driver to set up driver specific data.

    Tag greater than or equal to queue\_depth is for setting up
    flush request.

`exit_request`
:   Ditto for exit/teardown.

`cleanup_rq`
:   Called before freeing one request which isn’t completed
    yet, and usually for freeing the driver private data.

`busy`
:   If set, returns whether or not this queue currently is busy.

`map_queues`
:   This allows drivers specify their own queue mapping by
    overriding the setup-time function that builds the mq\_map.

`show_rq`
:   Used by the debugfs implementation to show driver-specific
    information about a request.

enum mq\_rq\_state blk\_mq\_rq\_state(struct request \*rq)
:   read the current MQ\_RQ\_\* state of a request

**Parameters**

`struct request *rq`
:   target request.

bool blk\_mq\_add\_to\_batch(struct request \*req, struct io\_comp\_batch \*iob, bool is\_error, void (\*complete)(struct io\_comp\_batch\*))
:   add a request to the completion batch

**Parameters**

`struct request *req`
:   The request to add to batch

`struct io_comp_batch *iob`
:   The batch to add the request

`bool is_error`
:   Specify true if the request failed with an error

`void (*complete)(struct io_comp_batch *)`
:   The completaion handler for the request

**Description**

Batched completions only work when there is no I/O error and no special
->end\_io handler.

**Return**

true when the request was added to the batch, otherwise false

struct request \*blk\_mq\_rq\_from\_pdu(void \*pdu)
:   cast a PDU to a request

**Parameters**

`void *pdu`
:   the PDU (Protocol Data Unit) to be casted

**Return**

request

**Description**

Driver command data is immediately after the request. So subtract request
size to get back to the original request.

void \*blk\_mq\_rq\_to\_pdu(struct request \*rq)
:   cast a request to a PDU

**Parameters**

`struct request *rq`
:   the request to be casted

**Return**

pointer to the PDU

**Description**

Driver command data is immediately after the request. So add request to get
the PDU.

unsigned int blk\_rq\_nr\_bvec(struct request \*rq)
:   return number of bvecs in a request

**Parameters**

`struct request *rq`
:   request to calculate bvecs for

**Description**

Returns the number of bvecs.

void blk\_mq\_wait\_quiesce\_done(struct [blk\_mq\_tag\_set](#c.blk_mq_tag_set "blk_mq_tag_set") \*set)
:   wait until in-progress quiesce is done

**Parameters**

`struct blk_mq_tag_set *set`
:   tag\_set to wait on

**Note**

it is driver’s responsibility for making sure that quiesce has
been started on or more of the request\_queues of the tag\_set. This
function only waits for the quiesce on those request\_queues that had
the quiesce flag set using blk\_mq\_quiesce\_queue\_nowait.

void blk\_mq\_quiesce\_queue(struct request\_queue \*q)
:   wait until all ongoing dispatches have finished

**Parameters**

`struct request_queue *q`
:   request queue.

**Note**

this function does not prevent that the `struct request` `end_io()`
callback function is invoked. Once this function is returned, we make
sure no dispatch can happen until the queue is unquiesced via
`blk_mq_unquiesce_queue()`.

bool blk\_update\_request(struct request \*req, blk\_status\_t error, unsigned int nr\_bytes)
:   Complete multiple bytes without completing the request

**Parameters**

`struct request *req`
:   the request being processed

`blk_status_t error`
:   block status code

`unsigned int nr_bytes`
:   number of bytes to complete for **req**

**Description**

> Ends I/O on a number of bytes attached to **req**, but doesn’t complete
> the request structure even if **req** doesn’t have leftover.
> If **req** has leftover, sets it up for the next range of segments.
>
> Passing the result of `blk_rq_bytes()` as **nr\_bytes** guarantees
> `false` return from this function.

**Note**

> The RQF\_SPECIAL\_PAYLOAD flag is ignored on purpose in this function
> except in the consistency check at the end of this function.

**Return**

`false` - this request doesn’t have any more data
`true` - this request has more data

void blk\_mq\_complete\_request(struct request \*rq)
:   end I/O on a request

**Parameters**

`struct request *rq`
:   the request being processed

**Description**

> Complete a request by scheduling the ->complete\_rq operation.

void blk\_mq\_start\_request(struct request \*rq)
:   Start processing a request

**Parameters**

`struct request *rq`
:   Pointer to request to be started

**Description**

Function used by device drivers to notify the block layer that a request
is going to be processed now, so blk layer can do proper initializations
such as starting the timeout timer.

void blk\_execute\_rq\_nowait(struct request \*rq, bool at\_head)
:   insert a request to I/O scheduler for execution

**Parameters**

`struct request *rq`
:   request to insert

`bool at_head`
:   insert request at head or tail of queue

**Description**

> Insert a fully prepared request at the back of the I/O scheduler queue
> for execution. Don’t wait for completion.

**Note**

> This function will invoke **done** directly if the queue is dead.

blk\_status\_t blk\_execute\_rq(struct request \*rq, bool at\_head)
:   insert a request into queue for execution

**Parameters**

`struct request *rq`
:   request to insert

`bool at_head`
:   insert request at head or tail of queue

**Description**

> Insert a fully prepared request at the back of the I/O scheduler queue
> for execution and wait for completion.

**Return**

The blk\_status\_t result provided to `blk_mq_end_request()`.

void blk\_mq\_delay\_run\_hw\_queue(struct [blk\_mq\_hw\_ctx](#c.blk_mq_hw_ctx "blk_mq_hw_ctx") \*hctx, unsigned long msecs)
:   Run a hardware queue asynchronously.

**Parameters**

`struct blk_mq_hw_ctx *hctx`
:   Pointer to the hardware queue to run.

`unsigned long msecs`
:   Milliseconds of delay to wait before running the queue.

**Description**

Run a hardware queue asynchronously with a delay of **msecs**.

void blk\_mq\_run\_hw\_queue(struct [blk\_mq\_hw\_ctx](#c.blk_mq_hw_ctx "blk_mq_hw_ctx") \*hctx, bool async)
:   Start to run a hardware queue.

**Parameters**

`struct blk_mq_hw_ctx *hctx`
:   Pointer to the hardware queue to run.

`bool async`
:   If we want to run the queue asynchronously.

**Description**

Check if the request queue is not in a quiesced state and if there are
pending requests to be sent. If this is true, run the queue to send requests
to hardware.

void blk\_mq\_run\_hw\_queues(struct request\_queue \*q, bool async)
:   Run all hardware queues in a request queue.

**Parameters**

`struct request_queue *q`
:   Pointer to the request queue to run.

`bool async`
:   If we want to run the queue asynchronously.

void blk\_mq\_delay\_run\_hw\_queues(struct request\_queue \*q, unsigned long msecs)
:   Run all hardware queues asynchronously.

**Parameters**

`struct request_queue *q`
:   Pointer to the request queue to run.

`unsigned long msecs`
:   Milliseconds of delay to wait before running the queues.

void blk\_mq\_request\_bypass\_insert(struct request \*rq, blk\_insert\_t flags)
:   Insert a request at dispatch list.

**Parameters**

`struct request *rq`
:   Pointer to request to be inserted.

`blk_insert_t flags`
:   BLK\_MQ\_INSERT\_\*

**Description**

Should only be used carefully, when the caller knows we want to
bypass a potential IO scheduler on the target device.

void blk\_mq\_try\_issue\_directly(struct [blk\_mq\_hw\_ctx](#c.blk_mq_hw_ctx "blk_mq_hw_ctx") \*hctx, struct request \*rq)
:   Try to send a request directly to device driver.

**Parameters**

`struct blk_mq_hw_ctx *hctx`
:   Pointer of the associated hardware queue.

`struct request *rq`
:   Pointer to request to be sent.

**Description**

If the device has enough resources to accept a new request now, send the
request directly to device driver. Else, insert at hctx->dispatch queue, so
we can try send it another time in the future. Requests inserted at this
queue have higher priority.

void blk\_mq\_submit\_bio(struct [bio](#c.blk_mq_submit_bio "bio") \*bio)
:   Create and send a request to block device.

**Parameters**

`struct bio *bio`
:   Bio pointer.

**Description**

Builds up a request structure from **q** and **bio** and send to the device. The
request may not be queued directly to hardware if:
\* This request can be merged with another one
\* We want to place request at plug queue for possible future merging
\* There is an IO scheduler active at this queue

It will not queue the request if there is an error with the bio, or at the
request creation.

blk\_status\_t blk\_insert\_cloned\_request(struct request \*rq)
:   Helper for stacking drivers to submit a request

**Parameters**

`struct request *rq`
:   the request being queued

void blk\_rq\_unprep\_clone(struct request \*rq)
:   Helper function to free all bios in a cloned request

**Parameters**

`struct request *rq`
:   the clone request to be cleaned up

**Description**

> Free all bios in **rq** for a cloned request.

int blk\_rq\_prep\_clone(struct request \*rq, struct request \*rq\_src, struct bio\_set \*bs, gfp\_t gfp\_mask, int (\*bio\_ctr)(struct bio\*, struct bio\*, void\*), void \*data)
:   Helper function to setup clone request

**Parameters**

`struct request *rq`
:   the request to be setup

`struct request *rq_src`
:   original request to be cloned

`struct bio_set *bs`
:   bio\_set that bios for clone are allocated from

`gfp_t gfp_mask`
:   memory allocation mask for bio

`int (*bio_ctr)(struct bio *, struct bio *, void *)`
:   setup function to be called for each clone bio.
    Returns `0` for success, non `0` for failure.

`void *data`
:   private data to be passed to **bio\_ctr**

**Description**

> Clones bios in **rq\_src** to **rq**, and copies attributes of **rq\_src** to **rq**.
> Also, pages which the original bios are pointing to are not copied
> and the cloned bios just point same pages.
> So cloned bios must be completed before original bios, which means
> the caller must complete **rq** before **rq\_src**.

void blk\_mq\_destroy\_queue(struct request\_queue \*q)
:   shutdown a request queue

**Parameters**

`struct request_queue *q`
:   request queue to shutdown

**Description**

This shuts down a request queue allocated by `blk_mq_alloc_queue()`. All future
requests will be failed with -ENODEV. The caller is responsible for dropping
the reference from `blk_mq_alloc_queue()` by calling [`blk_put_queue()`](../core-api/kernel-api.html#c.blk_put_queue "blk_put_queue").

**Context**

can sleep
