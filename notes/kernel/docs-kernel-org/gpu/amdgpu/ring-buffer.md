# Ring Buffer

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/ring-buffer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ring Buffer

To handle communication between user space and kernel space, AMD GPUs use a
ring buffer design to feed the engines (GFX, Compute, SDMA, UVD, VCE, VCN, VPE,
etc.). See the figure below that illustrates how this communication works:

![../../_images/ring_buffers.svg](../../_images/ring_buffers.svg)

Ring buffers in the amdgpu work as a producer-consumer model, where userspace
acts as the producer, constantly filling the ring buffer with GPU commands to
be executed. Meanwhile, the GPU retrieves the information from the ring, parses
it, and distributes the specific set of instructions between the different
amdgpu blocks.

Notice from the diagram that the ring has a Read Pointer (rptr), which
indicates where the engine is currently reading packets from the ring, and a
Write Pointer (wptr), which indicates how many packets software has added to
the ring. When the rptr and wptr are equal, the ring is idle. When software
adds packets to the ring, it updates the wptr, this causes the engine to start
fetching and processing packets. As the engine processes packets, the rptr gets
updates until the rptr catches up to the wptr and they are equal again.

Usually, ring buffers in the driver have a limited size (search for occurrences
of [`amdgpu_ring_init()`](#c.amdgpu_ring_init "amdgpu_ring_init")). One of the reasons for the small ring buffer size is
that CP (Command Processor) is capable of following addresses inserted into the
ring; this is illustrated in the image by the reference to the IB (Indirect
Buffer). The IB gives userspace the possibility to have an area in memory that
CP can read and feed the hardware with extra instructions.

All ASICs pre-GFX11 use what is called a kernel queue, which means
the ring is allocated in kernel space and has some restrictions, such as not
being able to be [preempted directly by the scheduler](gc/mes.html#amdgpu-mes). GFX11
and newer support kernel queues, but also provide a new mechanism named
[user queues](userq.html#amdgpu-userq), where the queue is moved to the user space
and can be mapped and unmapped via the scheduler. In practice, both queues
insert user-space-generated GPU commands from different jobs into the requested
component ring.

## Enforce Isolation

Note

After reading this section, you might want to check the
[Process Isolation](process-isolation.html#amdgpu-process-isolation) page for more details.

Before examining the Enforce Isolation mechanism in the ring buffer context, it
is helpful to briefly discuss how instructions from the ring buffer are
processed in the graphics pipeline. Let’s expand on this topic by checking the
diagram below that illustrates the graphics pipeline:

![../../_images/gfx_pipeline_seq.svg](../../_images/gfx_pipeline_seq.svg)

In terms of executing instructions, the GFX pipeline follows the sequence:
Shader Export (SX), Geometry Engine (GE), Shader Process or Input (SPI), Scan
Converter (SC), Primitive Assembler (PA), and cache manipulation (which may
vary across ASICs). Another common way to describe the pipeline is to use Pixel
Shader (PS), raster, and Vertex Shader (VS) to symbolize the two shader stages.
Now, with this pipeline in mind, let’s assume that Job B causes a hang issue,
but Job C’s instruction might already be executing, leading developers to
incorrectly identify Job C as the problematic one. This problem can be
mitigated on multiple levels; the diagram below illustrates how to minimize
part of this problem:

![../../_images/no_enforce_isolation.svg](../../_images/no_enforce_isolation.svg)

Note from the diagram that there is no guarantee of order or a clear separation
between instructions, which is not a problem most of the time, and is also good
for performance. Furthermore, notice some circles between jobs in the diagram
that represent a **fence wait** used to avoid overlapping work in the ring. At
the end of the fence, a cache flush occurs, ensuring that when the next job
starts, it begins in a clean state and, if issues arise, the developer can
pinpoint the problematic process more precisely.

To increase the level of isolation between jobs, there is the “Enforce
Isolation” method described in the picture below:

![../../_images/enforce_isolation.svg](../../_images/enforce_isolation.svg)

As shown in the diagram, enforcing isolation introduces ordering between
submissions, since the access to GFX/Compute is serialized, think about it as
single process at a time mode for gfx/compute. Notice that this approach has a
significant performance impact, as it allows only one job to submit commands at
a time. However, this option can help pinpoint the job that caused the problem.
Although enforcing isolation improves the situation, it does not fully resolve
the issue of precisely pinpointing bad jobs, since isolation might mask the
problem. In summary, identifying which job caused the issue may not be precise,
but enforcing isolation might help with the debugging.

## Ring Operations

unsigned int amdgpu\_ring\_max\_ibs(enum amdgpu\_ring\_type type)
:   Return max IBs that fit in a single submission.

**Parameters**

`enum amdgpu_ring_type type`
:   ring type for which to return the limit.

int amdgpu\_ring\_alloc(struct amdgpu\_ring \*ring, unsigned int ndw)
:   allocate space on the ring buffer

**Parameters**

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

`unsigned int ndw`
:   number of dwords to allocate in the ring buffer

**Description**

Allocate **ndw** dwords in the ring buffer. The number of dwords should be the
sum of all commands written to the ring.

**Return**

0 on success, otherwise -ENOMEM if it tries to allocate more than the
maximum dword allowed for one submission.

void amdgpu\_ring\_insert\_nop(struct amdgpu\_ring \*ring, uint32\_t count)
:   insert NOP packets

**Parameters**

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

`uint32_t count`
:   the number of NOP packets to insert

**Description**

This is the generic insert\_nop function for rings except SDMA

void amdgpu\_ring\_generic\_pad\_ib(struct amdgpu\_ring \*ring, struct amdgpu\_ib \*ib)
:   pad IB with NOP packets

**Parameters**

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

`struct amdgpu_ib *ib`
:   IB to add NOP packets to

**Description**

This is the generic pad\_ib function for rings except SDMA

void amdgpu\_ring\_commit(struct amdgpu\_ring \*ring)
:   tell the GPU to execute the new commands on the ring buffer

**Parameters**

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

**Description**

Update the wptr (write pointer) to tell the GPU to
execute new commands on the ring buffer (all asics).

void amdgpu\_ring\_undo(struct amdgpu\_ring \*ring)
:   reset the wptr

**Parameters**

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

**Description**

Reset the driver’s copy of the wptr (all asics).

int amdgpu\_ring\_init(struct amdgpu\_device \*adev, struct amdgpu\_ring \*ring, unsigned int max\_dw, struct amdgpu\_irq\_src \*irq\_src, unsigned int irq\_type, unsigned int hw\_prio, atomic\_t \*sched\_score)
:   init driver ring struct.

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

`unsigned int max_dw`
:   maximum number of dw for ring alloc

`struct amdgpu_irq_src *irq_src`
:   interrupt source to use for this ring

`unsigned int irq_type`
:   interrupt type to use for this ring

`unsigned int hw_prio`
:   ring priority (NORMAL/HIGH)

`atomic_t *sched_score`
:   optional score atomic shared with other schedulers

**Description**

Initialize the driver information for the selected ring (all asics).
Returns 0 on success, error on failure.

void amdgpu\_ring\_fini(struct amdgpu\_ring \*ring)
:   tear down the driver ring struct.

**Parameters**

`struct amdgpu_ring *ring`
:   amdgpu\_ring structure holding ring information

**Description**

Tear down the driver information for the selected ring (all asics).

void amdgpu\_ring\_emit\_reg\_write\_reg\_wait\_helper(struct amdgpu\_ring \*ring, uint32\_t reg0, uint32\_t reg1, uint32\_t ref, uint32\_t mask)
:   ring helper

**Parameters**

`struct amdgpu_ring *ring`
:   ring to write to

`uint32_t reg0`
:   register to write

`uint32_t reg1`
:   register to wait on

`uint32_t ref`
:   reference value to write/wait on

`uint32_t mask`
:   mask to wait on

**Description**

Helper for rings that don’t support write and wait in a
single oneshot packet.

bool amdgpu\_ring\_soft\_recovery(struct amdgpu\_ring \*ring, unsigned int vmid, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   try to soft recover a ring lockup

**Parameters**

`struct amdgpu_ring *ring`
:   ring to try the recovery on

`unsigned int vmid`
:   VMID we try to get going again

`struct dma_fence *fence`
:   timedout fence

**Description**

Tries to get a ring proceeding again when it is stuck.

int amdgpu\_ring\_test\_helper(struct amdgpu\_ring \*ring)
:   tests ring and set sched readiness status

**Parameters**

`struct amdgpu_ring *ring`
:   ring to try the recovery on

**Description**

Tests ring and set sched readiness status

Returns 0 on success, error on failure.
