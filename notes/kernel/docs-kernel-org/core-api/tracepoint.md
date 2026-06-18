# The Linux Kernel Tracepoint API

> 출처(원문): https://docs.kernel.org/core-api/tracepoint.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux Kernel Tracepoint API

Author:
:   Jason Baron

Author:
:   William Cohen

## Introduction

Tracepoints are static probe points that are located in strategic points
throughout the kernel. ‘Probes’ register/unregister with tracepoints via
a callback mechanism. The ‘probes’ are strictly typed functions that are
passed a unique set of parameters defined by each tracepoint.

From this simple callback mechanism, ‘probes’ can be used to profile,
debug, and understand kernel behavior. There are a number of tools that
provide a framework for using ‘probes’. These tools include Systemtap,
ftrace, and LTTng.

Tracepoints are defined in a number of header files via various macros.
Thus, the purpose of this document is to provide a clear accounting of
the available tracepoints. The intention is to understand not only what
tracepoints are available but also to understand where future
tracepoints might be added.

The API presented has functions of the form:
`trace_tracepointname(function parameters)`. These are the tracepoints
callbacks that are found throughout the code. Registering and
unregistering probes with these callback sites is covered in the
`Documentation/trace/*` directory.

## IRQ

void trace\_irq\_handler\_entry(int irq, struct [irqaction](genericirq.html#c.irqaction "irqaction") \*action)
:   called immediately before the irq action handler

**Parameters**

`int irq`
:   irq number

`struct irqaction *action`
:   pointer to [`struct irqaction`](genericirq.html#c.irqaction "irqaction")

**Description**

The [`struct irqaction`](genericirq.html#c.irqaction "irqaction") pointed to by **action** contains various
information about the handler, including the device name,
**action->name**, and the device id, **action->dev\_id**. When used in
conjunction with the irq\_handler\_exit tracepoint, we can figure
out irq handler latencies.

void trace\_irq\_handler\_exit(int irq, struct [irqaction](genericirq.html#c.irqaction "irqaction") \*action, int ret)
:   called immediately after the irq action handler returns

**Parameters**

`int irq`
:   irq number

`struct irqaction *action`
:   pointer to [`struct irqaction`](genericirq.html#c.irqaction "irqaction")

`int ret`
:   return value

**Description**

If the **ret** value is set to IRQ\_HANDLED, then we know that the corresponding
**action->handler** successfully handled this irq. Otherwise, the irq might be
a shared irq line, or the irq was not handled successfully. Can be used in
conjunction with the irq\_handler\_entry to understand irq handler latencies.

void trace\_softirq\_entry(unsigned int vec\_nr)
:   called immediately before the softirq handler

**Parameters**

`unsigned int vec_nr`
:   softirq vector number

**Description**

When used in combination with the softirq\_exit tracepoint
we can determine the softirq handler routine.

void trace\_softirq\_exit(unsigned int vec\_nr)
:   called immediately after the softirq handler returns

**Parameters**

`unsigned int vec_nr`
:   softirq vector number

**Description**

When used in combination with the softirq\_entry tracepoint
we can determine the softirq handler routine.

void trace\_softirq\_raise(unsigned int vec\_nr)
:   called immediately when a softirq is raised

**Parameters**

`unsigned int vec_nr`
:   softirq vector number

**Description**

When used in combination with the softirq\_entry tracepoint
we can determine the softirq raise to run latency.

void trace\_tasklet\_entry(struct tasklet\_struct \*t, void \*func)
:   called immediately before the tasklet is run

**Parameters**

`struct tasklet_struct *t`
:   tasklet pointer

`void *func`
:   tasklet callback or function being run

**Description**

Used to find individual tasklet execution time

void trace\_tasklet\_exit(struct tasklet\_struct \*t, void \*func)
:   called immediately after the tasklet is run

**Parameters**

`struct tasklet_struct *t`
:   tasklet pointer

`void *func`
:   tasklet callback or function being run

**Description**

Used to find individual tasklet execution time

## SIGNAL

void trace\_signal\_generate(int sig, struct kernel\_siginfo \*info, struct task\_struct \*task, int group, int result)
:   called when a signal is generated

**Parameters**

`int sig`
:   signal number

`struct kernel_siginfo *info`
:   pointer to `struct siginfo`

`struct task_struct *task`
:   pointer to `struct task_struct`

`int group`
:   shared or private

`int result`
:   TRACE\_SIGNAL\_\*

**Description**

Current process sends a ‘sig’ signal to ‘task’ process with
‘info’ siginfo. If ‘info’ is SEND\_SIG\_NOINFO or SEND\_SIG\_PRIV,
‘info’ is not a pointer and you can’t access its field. Instead,
SEND\_SIG\_NOINFO means that si\_code is SI\_USER, and SEND\_SIG\_PRIV
means that si\_code is SI\_KERNEL.

void trace\_signal\_deliver(int sig, struct kernel\_siginfo \*info, struct k\_sigaction \*ka)
:   called when a signal is delivered

**Parameters**

`int sig`
:   signal number

`struct kernel_siginfo *info`
:   pointer to `struct siginfo`

`struct k_sigaction *ka`
:   pointer to `struct k_sigaction`

**Description**

A ‘sig’ signal is delivered to current process with ‘info’ siginfo,
and it will be handled by ‘ka’. ka->sa.sa\_handler can be SIG\_IGN or
SIG\_DFL.
Note that some signals reported by signal\_generate tracepoint can be
lost, ignored or modified (by debugger) before hitting this tracepoint.
This means, this can show which signals are actually delivered, but
matching generated signals and delivered signals may not be correct.

## Block IO

void trace\_block\_touch\_buffer(struct buffer\_head \*bh)
:   mark a buffer accessed

**Parameters**

`struct buffer_head *bh`
:   buffer\_head being touched

**Description**

Called from `touch_buffer()`.

void trace\_block\_dirty\_buffer(struct buffer\_head \*bh)
:   mark a buffer dirty

**Parameters**

`struct buffer_head *bh`
:   buffer\_head being dirtied

**Description**

Called from [`mark_buffer_dirty()`](../filesystems/buffer.html#c.mark_buffer_dirty "mark_buffer_dirty").

void trace\_block\_rq\_requeue(struct request \*rq)
:   place block IO request back on a queue

**Parameters**

`struct request *rq`
:   block IO operation request

**Description**

The block operation request **rq** is being placed back into queue
**q**. For some reason the request was not completed and needs to be
put back in the queue.

void trace\_block\_rq\_complete(struct request \*rq, blk\_status\_t error, unsigned int nr\_bytes)
:   block IO operation completed by device driver

**Parameters**

`struct request *rq`
:   block operations request

`blk_status_t error`
:   status code

`unsigned int nr_bytes`
:   number of completed bytes

**Description**

The block\_rq\_complete tracepoint event indicates that some portion
of operation request has been completed by the device driver. If
the **rq->bio** is `NULL`, then there is absolutely no additional work to
do for the request. If **rq->bio** is non-NULL then there is
additional work required to complete the request.

void trace\_block\_rq\_error(struct request \*rq, blk\_status\_t error, unsigned int nr\_bytes)
:   block IO operation error reported by device driver

**Parameters**

`struct request *rq`
:   block operations request

`blk_status_t error`
:   status code

`unsigned int nr_bytes`
:   number of completed bytes

**Description**

The block\_rq\_error tracepoint event indicates that some portion
of operation request has failed as reported by the device driver.

void trace\_block\_rq\_insert(struct request \*rq)
:   insert block operation request into queue

**Parameters**

`struct request *rq`
:   block IO operation request

**Description**

Called immediately before block operation request **rq** is inserted
into queue **q**. The fields in the operation request **rq** `struct can`
be examined to determine which device and sectors the pending
operation would access.

void trace\_block\_rq\_issue(struct request \*rq)
:   issue pending block IO request operation to device driver

**Parameters**

`struct request *rq`
:   block IO operation request

**Description**

Called when block operation request **rq** from queue **q** is sent to a
device driver for processing.

void trace\_block\_rq\_merge(struct request \*rq)
:   merge request with another one in the elevator

**Parameters**

`struct request *rq`
:   block IO operation request

**Description**

Called when block operation request **rq** from queue **q** is merged to another
request queued in the elevator.

void trace\_block\_io\_start(struct request \*rq)
:   insert a request for execution

**Parameters**

`struct request *rq`
:   block IO operation request

**Description**

Called when block operation request **rq** is queued for execution

void trace\_block\_io\_done(struct request \*rq)
:   block IO operation request completed

**Parameters**

`struct request *rq`
:   block IO operation request

**Description**

Called when block operation request **rq** is completed

void trace\_block\_bio\_complete(struct request\_queue \*q, struct [bio](#c.trace_block_bio_complete "bio") \*bio)
:   completed all work on the block operation

**Parameters**

`struct request_queue *q`
:   queue holding the block operation

`struct bio *bio`
:   block operation completed

**Description**

This tracepoint indicates there is no further work to do on this
block IO operation **bio**.

void trace\_block\_bio\_backmerge(struct [bio](#c.trace_block_bio_backmerge "bio") \*bio)
:   merging block operation to the end of an existing operation

**Parameters**

`struct bio *bio`
:   new block operation to merge

**Description**

Merging block request **bio** to the end of an existing block request.

void trace\_block\_bio\_frontmerge(struct [bio](#c.trace_block_bio_frontmerge "bio") \*bio)
:   merging block operation to the beginning of an existing operation

**Parameters**

`struct bio *bio`
:   new block operation to merge

**Description**

Merging block IO operation **bio** to the beginning of an existing block request.

void trace\_block\_bio\_queue(struct [bio](#c.trace_block_bio_queue "bio") \*bio)
:   putting new block IO operation in queue

**Parameters**

`struct bio *bio`
:   new block operation

**Description**

About to place the block IO operation **bio** into queue **q**.

void trace\_block\_getrq(struct [bio](#c.trace_block_getrq "bio") \*bio)
:   get a free request entry in queue for block IO operations

**Parameters**

`struct bio *bio`
:   pending block IO operation (can be `NULL`)

**Description**

A request `struct has` been allocated to handle the block IO operation **bio**.

void trace\_blk\_zone\_append\_update\_request\_bio(struct request \*rq)
:   update bio sector after zone append

**Parameters**

`struct request *rq`
:   the completed request that sets the bio sector

**Description**

Update the bio’s bi\_sector after a zone append command has been completed.

void trace\_block\_plug(struct request\_queue \*q)
:   keep operations requests in request queue

**Parameters**

`struct request_queue *q`
:   request queue to plug

**Description**

Plug the request queue **q**. Do not allow block operation requests
to be sent to the device driver. Instead, accumulate requests in
the queue to improve throughput performance of the block device.

void trace\_block\_unplug(struct request\_queue \*q, unsigned int depth, bool explicit)
:   release of operations requests in request queue

**Parameters**

`struct request_queue *q`
:   request queue to unplug

`unsigned int depth`
:   number of requests just added to the queue

`bool explicit`
:   whether this was an explicit unplug, or one from `schedule()`

**Description**

Unplug request queue **q** because device driver is scheduled to work
on elements in the request queue.

void trace\_block\_split(struct [bio](#c.trace_block_split "bio") \*bio, unsigned int new\_sector)
:   split a single bio `struct into` two bio structs

**Parameters**

`struct bio *bio`
:   block operation being split

`unsigned int new_sector`
:   The starting sector for the new bio

**Description**

The bio request **bio** needs to be split into two bio requests. The newly
created **bio** request starts at **new\_sector**. This split may be required due to
hardware limitations such as operation crossing device boundaries in a RAID
system.

void trace\_block\_bio\_remap(struct [bio](#c.trace_block_bio_remap "bio") \*bio, dev\_t dev, sector\_t from)
:   map request for a logical device to the raw device

**Parameters**

`struct bio *bio`
:   revised operation

`dev_t dev`
:   original device for the operation

`sector_t from`
:   original sector for the operation

**Description**

An operation for a logical device has been mapped to the
raw block device.

void trace\_block\_rq\_remap(struct request \*rq, dev\_t dev, sector\_t from)
:   map request for a block operation request

**Parameters**

`struct request *rq`
:   block IO operation request

`dev_t dev`
:   device for the operation

`sector_t from`
:   original sector for the operation

**Description**

The block operation request **rq** in **q** has been remapped. The block
operation request **rq** holds the current information and **from** hold
the original sector.

void trace\_blkdev\_zone\_mgmt(struct [bio](#c.trace_blkdev_zone_mgmt "bio") \*bio, sector\_t nr\_sectors)
:   Execute a zone management operation on a range of zones

**Parameters**

`struct bio *bio`
:   The block IO operation sent down to the device

`sector_t nr_sectors`
:   The number of sectors affected by this operation

**Description**

Execute a zone management operation on a specified range of zones. This
range is encoded in `nr_sectors`, which has to be a multiple of the zone
size.

## Workqueue

void trace\_workqueue\_queue\_work(int req\_cpu, struct pool\_workqueue \*pwq, struct work\_struct \*work)
:   called when a work gets queued

**Parameters**

`int req_cpu`
:   the requested cpu

`struct pool_workqueue *pwq`
:   pointer to `struct pool_workqueue`

`struct work_struct *work`
:   pointer to `struct work_struct`

**Description**

This event occurs when a work is queued immediately or once a
delayed work is actually queued on a workqueue (ie: once the delay
has been reached).

void trace\_workqueue\_activate\_work(struct work\_struct \*work)
:   called when a work gets activated

**Parameters**

`struct work_struct *work`
:   pointer to `struct work_struct`

**Description**

This event occurs when a queued work is put on the active queue,
which happens immediately after queueing unless **max\_active** limit
is reached.

void trace\_workqueue\_execute\_start(struct work\_struct \*work)
:   called immediately before the workqueue callback

**Parameters**

`struct work_struct *work`
:   pointer to `struct work_struct`

**Description**

Allows to track workqueue execution.

void trace\_workqueue\_execute\_end(struct work\_struct \*work, work\_func\_t function)
:   called immediately after the workqueue callback

**Parameters**

`struct work_struct *work`
:   pointer to `struct work_struct`

`work_func_t function`
:   pointer to worker function

**Description**

Allows to track workqueue execution.
