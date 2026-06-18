# Core Driver Infrastructure

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/driver-core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Core Driver Infrastructure

## GPU Hardware Structure

Each ASIC is a collection of hardware blocks. We refer to them as
“IPs” (Intellectual Property blocks). Each IP encapsulates certain
functionality. IPs are versioned and can also be mixed and matched.
E.g., you might have two different ASICs that both have System DMA (SDMA) 5.x IPs.
The driver is arranged by IPs. There are driver components to handle
the initialization and operation of each IP. There are also a bunch
of smaller IPs that don’t really need much if any driver interaction.
Those end up getting lumped into the common stuff in the soc files.
The soc files (e.g., vi.c, soc15.c nv.c) contain code for aspects of
the SoC itself rather than specific IPs. E.g., things like GPU resets
and register access functions are SoC dependent.

An APU contains more than just CPU and GPU, it also contains all of
the platform stuff (audio, usb, gpio, etc.). Also, a lot of
components are shared between the CPU, platform, and the GPU (e.g.,
SMU, PSP, etc.). Specific components (CPU, GPU, etc.) usually have
their interface to interact with those common components. For things
like S0i3 there is a ton of coordination required across all the
components, but that is probably a bit beyond the scope of this
section.

With respect to the GPU, we have the following major IPs:

GMC (Graphics Memory Controller)
:   This was a dedicated IP on older pre-vega chips, but has since
    become somewhat decentralized on vega and newer chips. They now
    have dedicated memory hubs for specific IPs or groups of IPs. We
    still treat it as a single component in the driver however since
    the programming model is still pretty similar. This is how the
    different IPs on the GPU get the memory (VRAM or system memory).
    It also provides the support for per process GPU virtual address
    spaces.

IH (Interrupt Handler)
:   This is the interrupt controller on the GPU. All of the IPs feed
    their interrupts into this IP and it aggregates them into a set of
    ring buffers that the driver can parse to handle interrupts from
    different IPs.

PSP (Platform Security Processor)
:   This handles security policy for the SoC and executes trusted
    applications, and validates and loads firmwares for other blocks.

SMU (System Management Unit)
:   This is the power management microcontroller. It manages the entire
    SoC. The driver interacts with it to control power management
    features like clocks, voltages, power rails, etc.

DCN (Display Controller Next)
:   This is the display controller. It handles the display hardware.
    It is described in more details in [Display Core](display/index.html#amdgpu-display-core).

SDMA (System DMA)
:   This is a multi-purpose DMA engine. The kernel driver uses it for
    various things including paging and GPU page table updates. It’s also
    exposed to userspace for use by user mode drivers (OpenGL, Vulkan,
    etc.)

GC (Graphics and Compute)
:   This is the graphics and compute engine, i.e., the block that
    encompasses the 3D pipeline and shader blocks. This is by far the
    largest block on the GPU. The 3D pipeline has tons of sub-blocks. In
    addition to that, it also contains the CP microcontrollers (ME, PFP, CE,
    MEC) and the RLC microcontroller. It’s exposed to userspace for user mode
    drivers (OpenGL, Vulkan, OpenCL, etc.). More details in [Graphics (GFX)
    and Compute](gc/index.html#amdgpu-gc).

VCN (Video Core Next)
:   This is the multi-media engine. It handles video and image encode and
    decode. It’s exposed to userspace for user mode drivers (VA-API,
    OpenMAX, etc.)

It is important to note that these blocks can interact with each other. The
picture below illustrates some of the components and their interconnection:

![../../_images/amd_overview_block.svg](../../_images/amd_overview_block.svg)

In the diagram, memory-related blocks are shown in green. Notice that specific
IPs have a green square that represents a small hardware block named ‘hub’,
which is responsible for interfacing with memory. All memory hubs are connected
in the UMCs, which in turn are connected to memory blocks. As a note,
pre-vega devices have a dedicated block for the Graphic Memory Controller
(GMC), which was replaced by UMC and hubs in new architectures. In the driver
code, you can identify this component by looking for the suffix hub, for
example: gfxhub, dchub, mmhub, vmhub, etc. Keep in mind that the component’s
interaction with the memory block may vary across architectures. For example,
on Navi and newer, GC and SDMA are both attached to GCHUB; on pre-Navi, SDMA
goes through MMHUB; VCN, JPEG, and VPE go through MMHUB; DCN goes through
DCHUB.

There is some protection for certain memory elements, and the PSP plays an
essential role in this area. When a specific firmware is loaded into memory,
the PSP takes steps to ensure it has a valid signature. It also stores firmware
images in a protected memory area named Trusted Memory Area (TMR), so the OS or
driver can’t corrupt them at runtime. Another use of PSP is to support Trusted
Applications (TA), which are basically small applications that run on the
trusted processor and handles a trusted operation (e.g., HDCP). PSP is also
used for encrypted memory for content protection via Trusted Memory Zone (TMZ).

Another critical IP is the SMU. It handles reset distribution, as well as
clock, thermal, and power management for all IPs on the SoC. SMU also helps to
balance performance and power consumption.

## GFX, Compute, and SDMA Overall Behavior

Note

For simplicity, whenever the term block is used in this section, it
means GFX, Compute, and SDMA.

GFX, Compute and SDMA share a similar form of operation that can be abstracted
to facilitate understanding of the behavior of these blocks. See the figure
below illustrating the common components of these blocks:

![../../_images/pipe_and_queue_abstraction.svg](../../_images/pipe_and_queue_abstraction.svg)

In the central part of this figure, you can see two hardware elements, one called
**Pipes** and another called **Queues**; it is important to highlight that Queues
must be associated with a Pipe and vice-versa. Every specific hardware IP may have
a different number of Pipes and, in turn, a different number of Queues; for
example, GFX 11 has two Pipes and two Queues per Pipe for the GFX front end.

Pipe is the hardware that processes the instructions available in the Queues;
in other words, it is a thread executing the operations inserted in the Queue.
One crucial characteristic of Pipes is that they can only execute one Queue at
a time; no matter if the hardware has multiple Queues in the Pipe, it only runs
one Queue per Pipe.

Pipes have the mechanics of swapping between queues at the hardware level.
Nonetheless, they only make use of Queues that are considered mapped. Pipes can
switch between queues based on any of the following inputs:

1. Command Stream;
2. Packet by Packet;
3. Other hardware requests the change (e.g., MES).

Queues within Pipes are defined by the Hardware Queue Descriptors (HQD).
Associated with the HQD concept, we have the Memory Queue Descriptor (MQD),
which is responsible for storing information about the state of each of the
available Queues in the memory. The state of a Queue contains information such
as the GPU virtual address of the queue itself, save areas, doorbell, etc. The
MQD also stores the HQD registers, which are vital for activating or
deactivating a given Queue. The scheduling firmware (e.g., MES) is responsible
for loading HQDs from MQDs and vice versa.

The Queue-switching process can also happen with the firmware requesting the
preemption or unmapping of a Queue. The firmware waits for the HQD\_ACTIVE bit
to change to low before saving the state into the MQD. To make a different
Queue become active, the firmware copies the MQD state into the HQD registers
and loads any additional state. Finally, it sets the HQD\_ACTIVE bit to high to
indicate that the queue is active. The Pipe will then execute work from active
Queues.

## Driver Structure

In general, the driver has a list of all of the IPs on a particular
SoC and for things like init/fini/suspend/resume, more or less just
walks the list and handles each IP.

Some useful constructs:

KIQ (Kernel Interface Queue)
:   This is a control queue used by the kernel driver to manage other gfx
    and compute queues on the GFX/compute engine. You can use it to
    map/unmap additional queues, etc. This is replaced by MES on
    GFX 11 and newer hardware.

IB (Indirect Buffer)
:   A command buffer for a particular engine. Rather than writing
    commands directly to the queue, you can write the commands into a
    piece of memory and then put a pointer to the memory into the queue.
    The hardware will then follow the pointer and execute the commands in
    the memory, then returning to the rest of the commands in the ring.

## Memory Domains

`AMDGPU_GEM_DOMAIN_CPU` System memory that is not GPU accessible.
Memory in this pool could be swapped out to disk if there is pressure.

`AMDGPU_GEM_DOMAIN_GTT` GPU accessible system memory, mapped into the
GPU’s virtual address space via gart. Gart memory linearizes non-contiguous
pages of system memory, allows GPU access system memory in a linearized
fashion.

`AMDGPU_GEM_DOMAIN_VRAM` Local video memory. For APUs, it is memory
carved out by the BIOS.

`AMDGPU_GEM_DOMAIN_GDS` Global on-chip data storage used to share data
across shader threads.

`AMDGPU_GEM_DOMAIN_GWS` Global wave sync, used to synchronize the
execution of all the waves on a device.

`AMDGPU_GEM_DOMAIN_OA` Ordered append, used by 3D or Compute engines
for appending data.

`AMDGPU_GEM_DOMAIN_DOORBELL` Doorbell. It is an MMIO region for
signalling user mode queues.

## Buffer Objects

This defines the interfaces to operate on an `amdgpu_bo` buffer object which
represents memory used by driver (VRAM, system memory, etc.). The driver
provides DRM/GEM APIs to userspace. DRM/GEM APIs then use these interfaces
to create/destroy/set buffer object which are then managed by the kernel TTM
memory manager.
The interfaces are also used internally by kernel clients, including gfx,
uvd, etc. for kernel managed allocations used by the GPU.

bool amdgpu\_bo\_is\_amdgpu\_bo(struct ttm\_buffer\_object \*bo)
:   check if the buffer object is an `amdgpu_bo`

**Parameters**

`struct ttm_buffer_object *bo`
:   buffer object to be checked

**Description**

Uses destroy function associated with the object to determine if this is
an `amdgpu_bo`.

**Return**

true if the object belongs to `amdgpu_bo`, false if not.

void amdgpu\_bo\_placement\_from\_domain(struct amdgpu\_bo \*abo, u32 domain)
:   set buffer’s placement

**Parameters**

`struct amdgpu_bo *abo`
:   `amdgpu_bo` buffer object whose placement is to be set

`u32 domain`
:   requested domain

**Description**

Sets buffer’s placement according to requested domain and the buffer’s
flags.

int amdgpu\_bo\_create\_reserved(struct amdgpu\_device \*adev, unsigned long size, int align, u32 domain, struct amdgpu\_bo \*\*bo\_ptr, u64 \*gpu\_addr, void \*\*cpu\_addr)
:   create reserved BO for kernel use

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`unsigned long size`
:   size for the new BO

`int align`
:   alignment for the new BO

`u32 domain`
:   where to place it

`struct amdgpu_bo **bo_ptr`
:   used to initialize BOs in structures

`u64 *gpu_addr`
:   GPU addr of the pinned BO

`void **cpu_addr`
:   optional CPU address mapping

**Description**

Allocates and pins a BO for kernel internal use, and returns it still
reserved.

**Note**

For bo\_ptr new BO is only created if bo\_ptr points to NULL.

**Return**

0 on success, negative error code otherwise.

int amdgpu\_bo\_create\_kernel(struct amdgpu\_device \*adev, unsigned long size, int align, u32 domain, struct amdgpu\_bo \*\*bo\_ptr, u64 \*gpu\_addr, void \*\*cpu\_addr)
:   create BO for kernel use

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`unsigned long size`
:   size for the new BO

`int align`
:   alignment for the new BO

`u32 domain`
:   where to place it

`struct amdgpu_bo **bo_ptr`
:   used to initialize BOs in structures

`u64 *gpu_addr`
:   GPU addr of the pinned BO

`void **cpu_addr`
:   optional CPU address mapping

**Description**

Allocates and pins a BO for kernel internal use.

This function is exported to allow the V4L2 isp device
external to drm device to create and access the kernel BO.

**Note**

For bo\_ptr new BO is only created if bo\_ptr points to NULL.

**Return**

0 on success, negative error code otherwise.

int amdgpu\_bo\_create\_isp\_user(struct amdgpu\_device \*adev, struct [dma\_buf](#c.amdgpu_bo_create_isp_user "dma_buf") \*dma\_buf, u32 domain, struct amdgpu\_bo \*\*bo, u64 \*gpu\_addr)
:   create user BO for isp

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`struct dma_buf *dma_buf`
:   DMABUF handle for isp buffer

`u32 domain`
:   where to place it

`struct amdgpu_bo **bo`
:   used to initialize BOs in structures

`u64 *gpu_addr`
:   GPU addr of the pinned BO

**Description**

Imports isp DMABUF to allocate and pin a user BO for isp internal use. It does
GART alloc to generate gpu\_addr for BO to make it accessible through the
GART aperture for ISP HW.

This function is exported to allow the V4L2 isp device external to drm device
to create and access the isp user BO.

**Return**

0 on success, negative error code otherwise.

int amdgpu\_bo\_create\_kernel\_at(struct amdgpu\_device \*adev, uint64\_t offset, uint64\_t size, struct amdgpu\_bo \*\*bo\_ptr, void \*\*cpu\_addr)
:   create BO for kernel use at specific location

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`uint64_t offset`
:   offset of the BO

`uint64_t size`
:   size of the BO

`struct amdgpu_bo **bo_ptr`
:   used to initialize BOs in structures

`void **cpu_addr`
:   optional CPU address mapping

**Description**

Creates a kernel BO at a specific offset in VRAM.

**Return**

0 on success, negative error code otherwise.

void amdgpu\_bo\_free\_kernel(struct amdgpu\_bo \*\*bo, u64 \*gpu\_addr, void \*\*cpu\_addr)
:   free BO for kernel use

**Parameters**

`struct amdgpu_bo **bo`
:   amdgpu BO to free

`u64 *gpu_addr`
:   pointer to where the BO’s GPU memory space address was stored

`void **cpu_addr`
:   pointer to where the BO’s CPU memory space address was stored

**Description**

unmaps and unpin a BO for kernel internal use.

This function is exported to allow the V4L2 isp device
external to drm device to free the kernel BO.

void amdgpu\_bo\_free\_isp\_user(struct amdgpu\_bo \*bo)
:   free BO for isp use

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu isp user BO to free

**Description**

unpin and unref BO for isp internal use.

This function is exported to allow the V4L2 isp device
external to drm device to free the isp user BO.

int amdgpu\_bo\_create(struct amdgpu\_device \*adev, struct amdgpu\_bo\_param \*bp, struct amdgpu\_bo \*\*bo\_ptr)
:   create an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`struct amdgpu_bo_param *bp`
:   parameters to be used for the buffer object

`struct amdgpu_bo **bo_ptr`
:   pointer to the buffer object pointer

**Description**

Creates an `amdgpu_bo` buffer object.

**Return**

0 for success or a negative error code on failure.

int amdgpu\_bo\_create\_user(struct amdgpu\_device \*adev, struct amdgpu\_bo\_param \*bp, struct amdgpu\_bo\_user \*\*ubo\_ptr)
:   create an `amdgpu_bo_user` buffer object

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`struct amdgpu_bo_param *bp`
:   parameters to be used for the buffer object

`struct amdgpu_bo_user **ubo_ptr`
:   pointer to the buffer object pointer

**Description**

Create a BO to be used by user application;

**Return**

0 for success or a negative error code on failure.

int amdgpu\_bo\_create\_vm(struct amdgpu\_device \*adev, struct amdgpu\_bo\_param \*bp, struct amdgpu\_bo\_vm \*\*vmbo\_ptr)
:   create an `amdgpu_bo_vm` buffer object

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`struct amdgpu_bo_param *bp`
:   parameters to be used for the buffer object

`struct amdgpu_bo_vm **vmbo_ptr`
:   pointer to the buffer object pointer

**Description**

Create a BO to be for GPUVM.

**Return**

0 for success or a negative error code on failure.

int amdgpu\_bo\_kmap(struct amdgpu\_bo \*bo, void \*\*ptr)
:   map an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object to be mapped

`void **ptr`
:   kernel virtual address to be returned

**Description**

Calls `ttm_bo_kmap()` to set up the kernel virtual mapping; calls
[`amdgpu_bo_kptr()`](#c.amdgpu_bo_kptr "amdgpu_bo_kptr") to get the kernel virtual address.

**Return**

0 for success or a negative error code on failure.

void \*amdgpu\_bo\_kptr(struct amdgpu\_bo \*bo)
:   returns a kernel virtual address of the buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object

**Description**

Calls `ttm_kmap_obj_virtual()` to get the kernel virtual address

**Return**

the virtual address of a buffer object area.

void amdgpu\_bo\_kunmap(struct amdgpu\_bo \*bo)
:   unmap an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object to be unmapped

**Description**

Unmaps a kernel map set up by [`amdgpu_bo_kmap()`](#c.amdgpu_bo_kmap "amdgpu_bo_kmap").

struct amdgpu\_bo \*amdgpu\_bo\_ref(struct amdgpu\_bo \*bo)
:   reference an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object

**Description**

References the contained `ttm_buffer_object`.

**Return**

a refcounted pointer to the `amdgpu_bo` buffer object.

void amdgpu\_bo\_unref(struct amdgpu\_bo \*\*bo)
:   unreference an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_bo **bo`
:   `amdgpu_bo` buffer object

**Description**

Unreferences the contained `ttm_buffer_object` and clear the pointer

int amdgpu\_bo\_pin(struct amdgpu\_bo \*bo, u32 domain)
:   pin an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object to be pinned

`u32 domain`
:   domain to be pinned to

**Description**

Pins the buffer object according to requested domain. If the memory is
unbound gart memory, binds the pages into gart table. Adjusts pin\_count and
pin\_size accordingly.

Pinning means to lock pages in memory along with keeping them at a fixed
offset. It is required when a buffer can not be moved, for example, when
a display buffer is being scanned out.

**Return**

0 for success or a negative error code on failure.

void amdgpu\_bo\_unpin(struct amdgpu\_bo \*bo)
:   unpin an `amdgpu_bo` buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object to be unpinned

**Description**

Decreases the pin\_count, and clears the flags if pin\_count reaches 0.
Changes placement and pin size accordingly.

**Return**

0 for success or a negative error code on failure.

int amdgpu\_bo\_init(struct amdgpu\_device \*adev)
:   initialize memory manager

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

**Description**

Calls `amdgpu_ttm_init()` to initialize amdgpu memory manager.

**Return**

0 for success or a negative error code on failure.

void amdgpu\_bo\_fini(struct amdgpu\_device \*adev)
:   tear down memory manager

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

**Description**

Reverses [`amdgpu_bo_init()`](#c.amdgpu_bo_init "amdgpu_bo_init") to tear down memory manager.

int amdgpu\_bo\_set\_tiling\_flags(struct amdgpu\_bo \*bo, u64 tiling\_flags)
:   set tiling flags

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object

`u64 tiling_flags`
:   new flags

**Description**

Sets buffer object’s tiling flags with the new one. Used by GEM ioctl or
kernel driver to set the tiling flags on a buffer.

**Return**

0 for success or a negative error code on failure.

void amdgpu\_bo\_get\_tiling\_flags(struct amdgpu\_bo \*bo, u64 \*tiling\_flags)
:   get tiling flags

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object

`u64 *tiling_flags`
:   returned flags

**Description**

Gets buffer object’s tiling flags. Used by GEM ioctl or kernel driver to
set the tiling flags on a buffer.

int amdgpu\_bo\_set\_metadata(struct amdgpu\_bo \*bo, void \*metadata, u32 metadata\_size, uint64\_t flags)
:   set metadata

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object

`void *metadata`
:   new metadata

`u32 metadata_size`
:   size of the new metadata

`uint64_t flags`
:   flags of the new metadata

**Description**

Sets buffer object’s metadata, its size and flags.
Used via GEM ioctl.

**Return**

0 for success or a negative error code on failure.

int amdgpu\_bo\_get\_metadata(struct amdgpu\_bo \*bo, void \*buffer, size\_t buffer\_size, uint32\_t \*metadata\_size, uint64\_t \*flags)
:   get metadata

**Parameters**

`struct amdgpu_bo *bo`
:   `amdgpu_bo` buffer object

`void *buffer`
:   returned metadata

`size_t buffer_size`
:   size of the buffer

`uint32_t *metadata_size`
:   size of the returned metadata

`uint64_t *flags`
:   flags of the returned metadata

**Description**

Gets buffer object’s metadata, its size and flags. buffer\_size shall not be
less than metadata\_size.
Used via GEM ioctl.

**Return**

0 for success or a negative error code on failure.

void amdgpu\_bo\_move\_notify(struct ttm\_buffer\_object \*bo, bool evict, struct [ttm\_resource](../drm-mm.html#c.ttm_resource "ttm_resource") \*new\_mem)
:   notification about a memory move

**Parameters**

`struct ttm_buffer_object *bo`
:   pointer to a buffer object

`bool evict`
:   if this move is evicting the buffer from the graphics address space

`struct ttm_resource *new_mem`
:   new resource for backing the BO

**Description**

Marks the corresponding `amdgpu_bo` buffer object as invalid, also performs
bookkeeping.
TTM driver callback which is called when ttm moves a buffer.

void amdgpu\_bo\_release\_notify(struct ttm\_buffer\_object \*bo)
:   notification about a BO being released

**Parameters**

`struct ttm_buffer_object *bo`
:   pointer to a buffer object

**Description**

Wipes VRAM buffers whose contents should not be leaked before the
memory is released.

[vm\_fault\_t](../../core-api/mm-api.html#c.vm_fault_t "vm_fault_t") amdgpu\_bo\_fault\_reserve\_notify(struct ttm\_buffer\_object \*bo)
:   notification about a memory fault

**Parameters**

`struct ttm_buffer_object *bo`
:   pointer to a buffer object

**Description**

Notifies the driver we are taking a fault on this BO and have reserved it,
also performs bookkeeping.
TTM driver callback for dealing with vm faults.

**Return**

0 for success or a negative error code on failure.

void amdgpu\_bo\_fence(struct amdgpu\_bo \*bo, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, bool shared)
:   add fence to buffer object

**Parameters**

`struct amdgpu_bo *bo`
:   buffer object in question

`struct dma_fence *fence`
:   fence to add

`bool shared`
:   true if fence should be added shared

int amdgpu\_bo\_sync\_wait\_resv(struct amdgpu\_device \*adev, struct [dma\_resv](../../driver-api/dma-buf.html#c.dma_resv "dma_resv") \*resv, enum amdgpu\_sync\_mode sync\_mode, void \*owner, bool intr)
:   Wait for BO reservation fences

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct dma_resv *resv`
:   reservation object to sync to

`enum amdgpu_sync_mode sync_mode`
:   synchronization mode

`void *owner`
:   fence owner

`bool intr`
:   Whether the wait is interruptible

**Description**

Extract the fences from the reservation object and waits for them to finish.

**Return**

0 on success, errno otherwise.

int amdgpu\_bo\_sync\_wait(struct amdgpu\_bo \*bo, void \*owner, bool intr)
:   Wrapper for amdgpu\_bo\_sync\_wait\_resv

**Parameters**

`struct amdgpu_bo *bo`
:   buffer object to wait for

`void *owner`
:   fence owner

`bool intr`
:   Whether the wait is interruptible

**Description**

Wrapper to wait for fences in a BO.

**Return**

0 on success, errno otherwise.

u64 amdgpu\_bo\_gpu\_offset(struct amdgpu\_bo \*bo)
:   return GPU offset of bo

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu object for which we query the offset

**Note**

object should either be pinned or reserved when calling this
function, it might be useful to add check for this for debugging.

**Return**

current GPU offset of the object.

u64 amdgpu\_bo\_fb\_aper\_addr(struct amdgpu\_bo \*bo)
:   return FB aperture GPU offset of the VRAM bo

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu VRAM buffer object for which we query the offset

**Return**

current FB aperture GPU offset of the object.

u64 amdgpu\_bo\_gpu\_offset\_no\_check(struct amdgpu\_bo \*bo)
:   return GPU offset of bo

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu object for which we query the offset

**Return**

current GPU offset of the object without raising warnings.

uint32\_t amdgpu\_bo\_mem\_stats\_placement(struct amdgpu\_bo \*bo)
:   bo placement for memory accounting

**Parameters**

`struct amdgpu_bo *bo`
:   the buffer object we should look at

**Description**

BO can have multiple preferred placements, to avoid double counting we want
to file it under a single placement for memory stats.
Luckily, if we take the highest set bit in preferred\_domains the result is
quite sensible.

**Return**

Which of the placements should the BO be accounted under.

uint32\_t amdgpu\_bo\_get\_preferred\_domain(struct amdgpu\_device \*adev, uint32\_t domain)
:   get preferred domain

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device object

`uint32_t domain`
:   allowed [memory domains](#amdgpu-memory-domains)

**Return**

Which of the allowed domains is preferred for allocating the BO.

u64 amdgpu\_bo\_print\_info(int id, struct amdgpu\_bo \*bo, struct seq\_file \*m)
:   print BO info in debugfs file

**Parameters**

`int id`
:   Index or Id of the BO

`struct amdgpu_bo *bo`
:   Requested BO for printing info

`struct seq_file *m`
:   debugfs file

**Description**

Print BO information in debugfs file

**Return**

Size of the BO in bytes.

## PRIME Buffer Sharing

The following callback implementations are used for [sharing GEM buffer
objects between different devices via PRIME](../drm-mm.html#prime-buffer-sharing).

struct amdgpu\_device \*dma\_buf\_attach\_adev(struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   Helper to get adev of an attachment

**Parameters**

`struct dma_buf_attachment *attach`
:   attachment

**Return**

A `struct amdgpu_device` \* if the attaching device is an amdgpu device or
partition, NULL otherwise.

int amdgpu\_dma\_buf\_attach(struct [dma\_buf](../../driver-api/dma-buf.html#c.dma_buf "dma_buf") \*dmabuf, struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   [`dma_buf_ops.attach`](../../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementation

**Parameters**

`struct dma_buf *dmabuf`
:   DMA-buf where we attach to

`struct dma_buf_attachment *attach`
:   attachment to add

**Description**

Add the attachment as user to the exported DMA-buf.

int amdgpu\_dma\_buf\_pin(struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   [`dma_buf_ops.pin`](../../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementation

**Parameters**

`struct dma_buf_attachment *attach`
:   attachment to pin down

**Description**

Pin the BO which is backing the DMA-buf so that it can’t move any more.

void amdgpu\_dma\_buf\_unpin(struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   [`dma_buf_ops.unpin`](../../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementation

**Parameters**

`struct dma_buf_attachment *attach`
:   attachment to unpin

**Description**

Unpin a previously pinned BO to make it movable again.

struct sg\_table \*amdgpu\_dma\_buf\_map(struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, enum dma\_data\_direction dir)
:   [`dma_buf_ops.map_dma_buf`](../../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementation

**Parameters**

`struct dma_buf_attachment *attach`
:   DMA-buf attachment

`enum dma_data_direction dir`
:   DMA direction

**Description**

Makes sure that the shared DMA buffer can be accessed by the target device.
For now, simply pins it to the GTT domain, where it should be accessible by
all DMA devices.

**Return**

sg\_table filled with the DMA addresses to use or ERR\_PRT with negative error
code.

void amdgpu\_dma\_buf\_unmap(struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct sg\_table \*sgt, enum dma\_data\_direction dir)
:   [`dma_buf_ops.unmap_dma_buf`](../../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementation

**Parameters**

`struct dma_buf_attachment *attach`
:   DMA-buf attachment

`struct sg_table *sgt`
:   sg\_table to unmap

`enum dma_data_direction dir`
:   DMA direction

**Description**

This is called when a shared DMA buffer no longer needs to be accessible by
another device. For now, simply unpins the buffer from GTT.

int amdgpu\_dma\_buf\_begin\_cpu\_access(struct [dma\_buf](#c.amdgpu_dma_buf_begin_cpu_access "dma_buf") \*dma\_buf, enum dma\_data\_direction direction)
:   [`dma_buf_ops.begin_cpu_access`](../../driver-api/dma-buf.html#c.dma_buf_ops "dma_buf_ops") implementation

**Parameters**

`struct dma_buf *dma_buf`
:   Shared DMA buffer

`enum dma_data_direction direction`
:   Direction of DMA transfer

**Description**

This is called before CPU access to the shared DMA buffer’s memory. If it’s
a read access, the buffer is moved to the GTT domain if possible, for optimal
CPU read performance.

**Return**

0 on success or a negative error code on failure.

struct [dma\_buf](../../driver-api/dma-buf.html#c.dma_buf "dma_buf") \*amdgpu\_gem\_prime\_export(struct [drm\_gem\_object](../drm-mm.html#c.drm_gem_object "drm_gem_object") \*gobj, int flags)
:   [`drm_driver.gem_prime_export`](../drm-internals.html#c.drm_driver "drm_driver") implementation

**Parameters**

`struct drm_gem_object *gobj`
:   GEM BO

`int flags`
:   Flags such as DRM\_CLOEXEC and DRM\_RDWR.

**Description**

The main work is done by the [`drm_gem_prime_export`](../drm-mm.html#c.drm_gem_prime_export "drm_gem_prime_export") helper.

**Return**

Shared DMA buffer representing the GEM BO from the given device.

struct [drm\_gem\_object](../drm-mm.html#c.drm_gem_object "drm_gem_object") \*amdgpu\_dma\_buf\_create\_obj(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf](#c.amdgpu_dma_buf_create_obj "dma_buf") \*dma\_buf)
:   create BO for DMA-buf import

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct dma_buf *dma_buf`
:   DMA-buf

**Description**

Creates an empty SG BO for DMA-buf import.

**Return**

A new GEM BO of the given DRM device, representing the memory
described by the given DMA-buf attachment and scatter/gather table.

void amdgpu\_dma\_buf\_move\_notify(struct [dma\_buf\_attachment](../../driver-api/dma-buf.html#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   [`attach.invalidate_mappings`](#c.amdgpu_dma_buf_move_notify "attach") implementation

**Parameters**

`struct dma_buf_attachment *attach`
:   the DMA-buf attachment

**Description**

Invalidate the DMA-buf attachment, making sure that the we re-create the
mapping before the next use.

struct [drm\_gem\_object](../drm-mm.html#c.drm_gem_object "drm_gem_object") \*amdgpu\_gem\_prime\_import(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, struct [dma\_buf](#c.amdgpu_gem_prime_import "dma_buf") \*dma\_buf)
:   [`drm_driver.gem_prime_import`](../drm-internals.html#c.drm_driver "drm_driver") implementation

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct dma_buf *dma_buf`
:   Shared DMA buffer

**Description**

Import a dma\_buf into a the driver and potentially create a new GEM object.

**Return**

GEM BO representing the shared DMA buffer for the given device.

bool amdgpu\_dmabuf\_is\_xgmi\_accessible(struct amdgpu\_device \*adev, struct amdgpu\_bo \*bo)
:   Check if xgmi available for P2P transfer

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer of the importer

`struct amdgpu_bo *bo`
:   amdgpu buffer object

**Return**

True if dmabuf accessible over xgmi, false otherwise.

## MMU Notifier

For coherent userptr handling registers an MMU notifier to inform the driver
about updates on the page tables of a process.

When somebody tries to invalidate the page tables we block the update until
all operations on the pages in question are completed, then those pages are
marked as accessed and also dirty if it wasn’t a read only access.

New command submissions using the userptrs in question are delayed until all
page table invalidation are completed and we once more see a coherent process
address space.

bool amdgpu\_hmm\_invalidate\_gfx(struct mmu\_interval\_notifier \*mni, const struct mmu\_notifier\_range \*range, unsigned long cur\_seq)
:   callback to notify about mm change

**Parameters**

`struct mmu_interval_notifier *mni`
:   the range (mm) is about to update

`const struct mmu_notifier_range *range`
:   details on the invalidation

`unsigned long cur_seq`
:   Value to pass to `mmu_interval_set_seq()`

**Description**

Block for operations on BOs to finish and mark pages as accessed and
potentially dirty.

bool amdgpu\_hmm\_invalidate\_hsa(struct mmu\_interval\_notifier \*mni, const struct mmu\_notifier\_range \*range, unsigned long cur\_seq)
:   callback to notify about mm change

**Parameters**

`struct mmu_interval_notifier *mni`
:   the range (mm) is about to update

`const struct mmu_notifier_range *range`
:   details on the invalidation

`unsigned long cur_seq`
:   Value to pass to `mmu_interval_set_seq()`

**Description**

We temporarily evict the BO attached to this range. This necessitates
evicting all user-mode queues of the process.

int amdgpu\_hmm\_register(struct amdgpu\_bo \*bo, unsigned long addr)
:   register a BO for notifier updates

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu buffer object

`unsigned long addr`
:   userptr addr we should monitor

**Description**

Registers a mmu\_notifier for the given BO at the specified address.
Returns 0 on success, -ERRNO if anything goes wrong.

void amdgpu\_hmm\_unregister(struct amdgpu\_bo \*bo)
:   unregister a BO for notifier updates

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu buffer object

**Description**

Remove any registration of mmu notifier updates from the buffer object.

bool amdgpu\_hmm\_range\_valid(struct amdgpu\_hmm\_range \*range)
:   check if an HMM range is still valid

**Parameters**

`struct amdgpu_hmm_range *range`
:   pointer to the `struct amdgpu_hmm_range` to validate

**Description**

Determines whether the given HMM range **range** is still valid by
checking for invalidations via the MMU notifier sequence. This is
typically used to verify that the range has not been invalidated
by concurrent address space updates before it is accessed.

**Return**

* true if **range** is valid and can be used safely
* false if **range** is NULL or has been invalidated

struct amdgpu\_hmm\_range \*amdgpu\_hmm\_range\_alloc(struct amdgpu\_bo \*bo)
:   allocate and initialize an AMDGPU HMM range

**Parameters**

`struct amdgpu_bo *bo`
:   optional buffer object to associate with this HMM range

**Description**

Allocates memory for amdgpu\_hmm\_range and associates it with the **bo** passed.
The reference count of the **bo** is incremented.

**Return**

Pointer to a newly allocated `struct amdgpu_hmm_range` on success,
or NULL if memory allocation fails.

void amdgpu\_hmm\_range\_free(struct amdgpu\_hmm\_range \*range)
:   release an AMDGPU HMM range

**Parameters**

`struct amdgpu_hmm_range *range`
:   pointer to the range object to free

**Description**

Releases all resources held by **range**, including the associated
hmm\_pfns and the dropping reference of associated bo if any.

**Return**

void

## AMDGPU Virtual Memory

GPUVM is the MMU functionality provided on the GPU.
GPUVM is similar to the legacy GART on older asics, however
rather than there being a single global GART table
for the entire GPU, there can be multiple GPUVM page tables active
at any given time. The GPUVM page tables can contain a mix
VRAM pages and system pages (both memory and MMIO) and system pages
can be mapped as snooped (cached system pages) or unsnooped
(uncached system pages).

Each active GPUVM has an ID associated with it and there is a page table
linked with each VMID. When executing a command buffer,
the kernel tells the engine what VMID to use for that command
buffer. VMIDs are allocated dynamically as commands are submitted.
The userspace drivers maintain their own address space and the kernel
sets up their pages tables accordingly when they submit their
command buffers and a VMID is assigned.
The hardware supports up to 16 active GPUVMs at any given time.

Each GPUVM is represented by a 1-2 or 1-5 level page table, depending
on the ASIC family. GPUVM supports RWX attributes on each page as well
as other features such as encryption and caching attributes.

VMID 0 is special. It is the GPUVM used for the kernel driver. In
addition to an aperture managed by a page table, VMID 0 also has
several other apertures. There is an aperture for direct access to VRAM
and there is a legacy AGP aperture which just forwards accesses directly
to the matching system physical addresses (or IOVAs when an IOMMU is
present). These apertures provide direct access to these memories without
incurring the overhead of a page table. VMID 0 is used by the kernel
driver for tasks like memory management.

GPU clients (i.e., engines on the GPU) use GPUVM VMIDs to access memory.
For user applications, each application can have their own unique GPUVM
address space. The application manages the address space and the kernel
driver manages the GPUVM page tables for each process. If an GPU client
accesses an invalid page, it will generate a GPU page fault, similar to
accessing an invalid page on a CPU.

struct amdgpu\_prt\_cb
:   Helper to disable partial resident texture feature from a fence callback

**Definition**:

```
struct amdgpu_prt_cb {
    struct amdgpu_device *adev;
    struct dma_fence_cb cb;
};
```

**Members**

`adev`
:   amdgpu device

`cb`
:   callback

struct amdgpu\_vm\_tlb\_seq\_struct
:   Helper to increment the TLB flush sequence

**Definition**:

```
struct amdgpu_vm_tlb_seq_struct {
    struct amdgpu_vm *vm;
    struct dma_fence_cb cb;
};
```

**Members**

`vm`
:   pointer to the amdgpu\_vm structure to set the fence sequence on

`cb`
:   callback

void amdgpu\_vm\_assert\_locked(struct amdgpu\_vm \*vm)
:   check if VM is correctly locked

**Parameters**

`struct amdgpu_vm *vm`
:   the VM which schould be tested

**Description**

Asserts that the VM root PD is locked.

bool amdgpu\_vm\_is\_bo\_always\_valid(struct amdgpu\_vm \*vm, struct amdgpu\_bo \*bo)
:   check if the BO is VM always valid

**Parameters**

`struct amdgpu_vm *vm`
:   VM to test against.

`struct amdgpu_bo *bo`
:   BO to be tested.

**Description**

Returns true if the BO shares the dma\_resv object with the root PD and is
always guaranteed to be valid inside the VM.

void amdgpu\_vm\_bo\_evicted(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is evicted

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is evicted

**Description**

State for PDs/PTs and per VM BOs which are not at the location they should
be.

void amdgpu\_vm\_bo\_moved(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is moved

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is moved

**Description**

State for per VM BOs which are moved, but that change is not yet reflected
in the page tables.

void amdgpu\_vm\_bo\_idle(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is idle

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is now idle

**Description**

State for PDs/PTs and per VM BOs which have gone through the state machine
and are now idle.

void amdgpu\_vm\_bo\_invalidated(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is invalidated

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is now invalidated

**Description**

State for normal BOs which are invalidated and that change not yet reflected
in the PTs.

void amdgpu\_vm\_bo\_evicted\_user(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is evicted

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is evicted

**Description**

State for BOs used by user mode queues which are not at the location they
should be.

void amdgpu\_vm\_bo\_relocated(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is reloacted

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is relocated

**Description**

State for PDs/PTs which needs to update their parent PD.
For the root PD, just move to idle state.

void amdgpu\_vm\_bo\_done(struct amdgpu\_vm\_bo\_base \*vm\_bo)
:   vm\_bo is done

**Parameters**

`struct amdgpu_vm_bo_base *vm_bo`
:   vm\_bo which is now done

**Description**

State for normal BOs which are invalidated and that change has been updated
in the PTs.

void amdgpu\_vm\_bo\_reset\_state\_machine(struct amdgpu\_vm \*vm)
:   reset the vm\_bo state machine

**Parameters**

`struct amdgpu_vm *vm`
:   the VM which state machine to reset

**Description**

Move all vm\_bo object in the VM into a state where they will be updated
again during validation.

void amdgpu\_vm\_update\_shared(struct amdgpu\_vm\_bo\_base \*base)
:   helper to update shared memory stat

**Parameters**

`struct amdgpu_vm_bo_base *base`
:   base structure for tracking BO usage in a VM

**Description**

Takes the vm status\_lock and updates the shared memory stat. If the basic
stat changed (e.g. buffer was moved) amdgpu\_vm\_update\_stats need to be called
as well.

void amdgpu\_vm\_bo\_update\_shared(struct amdgpu\_bo \*bo)
:   callback when bo gets shared/unshared

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu buffer object

**Description**

Update the per VM stats for all the vm if needed from private to shared or
vice versa.

void amdgpu\_vm\_update\_stats\_locked(struct amdgpu\_vm\_bo\_base \*base, struct [ttm\_resource](../drm-mm.html#c.ttm_resource "ttm_resource") \*res, int sign)
:   helper to update normal memory stat

**Parameters**

`struct amdgpu_vm_bo_base *base`
:   base structure for tracking BO usage in a VM

`struct ttm_resource *res`
:   the ttm\_resource to use for the purpose of accounting, may or may not
    be bo->tbo.resource

`int sign`
:   if we should add (+1) or subtract (-1) from the stat

**Description**

Caller need to have the vm status\_lock held. Useful for when multiple update
need to happen at the same time.

void amdgpu\_vm\_update\_stats(struct amdgpu\_vm\_bo\_base \*base, struct [ttm\_resource](../drm-mm.html#c.ttm_resource "ttm_resource") \*res, int sign)
:   helper to update normal memory stat

**Parameters**

`struct amdgpu_vm_bo_base *base`
:   base structure for tracking BO usage in a VM

`struct ttm_resource *res`
:   the ttm\_resource to use for the purpose of accounting, may or may not
    be bo->tbo.resource

`int sign`
:   if we should add (+1) or subtract (-1) from the stat

**Description**

Updates the basic memory stat when bo is added/deleted/moved.

void amdgpu\_vm\_bo\_base\_init(struct amdgpu\_vm\_bo\_base \*base, struct amdgpu\_vm \*vm, struct amdgpu\_bo \*bo)
:   Adds bo to the list of bos associated with the vm

**Parameters**

`struct amdgpu_vm_bo_base *base`
:   base structure for tracking BO usage in a VM

`struct amdgpu_vm *vm`
:   vm to which bo is to be added

`struct amdgpu_bo *bo`
:   amdgpu buffer object

**Description**

Initialize a bo\_va\_base structure and add it to the appropriate lists

int amdgpu\_vm\_lock\_pd(struct amdgpu\_vm \*vm, struct [drm\_exec](../drm-mm.html#c.drm_exec "drm_exec") \*exec, unsigned int num\_fences)
:   lock PD in drm\_exec

**Parameters**

`struct amdgpu_vm *vm`
:   vm providing the BOs

`struct drm_exec *exec`
:   drm execution context

`unsigned int num_fences`
:   number of extra fences to reserve

**Description**

Lock the VM root PD in the DRM execution context.

int amdgpu\_vm\_lock\_done\_list(struct amdgpu\_vm \*vm, struct [drm\_exec](../drm-mm.html#c.drm_exec "drm_exec") \*exec, unsigned int num\_fences)
:   lock all BOs on the done list

**Parameters**

`struct amdgpu_vm *vm`
:   vm providing the BOs

`struct drm_exec *exec`
:   drm execution context

`unsigned int num_fences`
:   number of extra fences to reserve

**Description**

Lock the BOs on the done list in the DRM execution context.

void amdgpu\_vm\_move\_to\_lru\_tail(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm)
:   move all BOs to the end of LRU

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_vm *vm`
:   vm providing the BOs

**Description**

Move all BOs to the end of LRU and remember their positions to put them
together.

uint64\_t amdgpu\_vm\_generation(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm)
:   return the page table re-generation counter

**Parameters**

`struct amdgpu_device *adev`
:   the amdgpu\_device

`struct amdgpu_vm *vm`
:   optional VM to check, might be NULL

**Description**

Returns a page table re-generation token to allow checking if submissions
are still valid to use this VM. The VM parameter might be NULL in which case
just the VRAM lost counter will be used.

int amdgpu\_vm\_validate(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, struct ww\_acquire\_ctx \*ticket, int (\*validate)(void \*p, struct amdgpu\_bo \*bo), void \*param)
:   validate evicted BOs tracked in the VM

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_vm *vm`
:   vm providing the BOs

`struct ww_acquire_ctx *ticket`
:   optional reservation ticket used to reserve the VM

`int (*validate)(void *p, struct amdgpu_bo *bo)`
:   callback to do the validation

`void *param`
:   parameter for the validation callback

**Description**

Validate the page table BOs and per-VM BOs on command submission if
necessary. If a ticket is given, also try to validate evicted user queue
BOs. They must already be reserved with the given ticket.

**Return**

Validation result.

bool amdgpu\_vm\_ready(struct amdgpu\_vm \*vm)
:   check VM is ready for updates

**Parameters**

`struct amdgpu_vm *vm`
:   VM to check

**Description**

Check if all VM PDs/PTs are ready for updates

**Return**

True if VM is not evicting and all VM entities are not stopped

void amdgpu\_vm\_check\_compute\_bug(struct amdgpu\_device \*adev)
:   check whether asic has compute vm bug

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

bool amdgpu\_vm\_need\_pipeline\_sync(struct amdgpu\_ring \*ring, struct amdgpu\_job \*job)
:   Check if pipe sync is needed for job.

**Parameters**

`struct amdgpu_ring *ring`
:   ring on which the job will be submitted

`struct amdgpu_job *job`
:   job to submit

**Return**

True if sync is needed.

void amdgpu\_vm\_flush(struct amdgpu\_ring \*ring, struct amdgpu\_job \*job, bool need\_pipe\_sync)
:   hardware flush the vm

**Parameters**

`struct amdgpu_ring *ring`
:   ring to use for flush

`struct amdgpu_job *job`
:   related job

`bool need_pipe_sync`
:   is pipe sync needed

**Description**

Emit a VM flush when it is necessary.

struct amdgpu\_bo\_va \*amdgpu\_vm\_bo\_find(struct amdgpu\_vm \*vm, struct amdgpu\_bo \*bo)
:   find the bo\_va for a specific vm & bo

**Parameters**

`struct amdgpu_vm *vm`
:   requested vm

`struct amdgpu_bo *bo`
:   requested buffer object

**Description**

Find **bo** inside the requested vm.
Search inside the **bos** vm list for the requested vm
Returns the found bo\_va or NULL if none is found

Object has to be reserved!

**Return**

Found bo\_va or NULL.

uint64\_t amdgpu\_vm\_map\_gart(const dma\_addr\_t \*pages\_addr, uint64\_t addr)
:   Resolve gart mapping of addr

**Parameters**

`const dma_addr_t *pages_addr`
:   optional DMA address to use for lookup

`uint64_t addr`
:   the unmapped addr

**Description**

Look up the physical address of the page that the pte resolves
to.

**Return**

The pointer for the page table entry.

int amdgpu\_vm\_update\_pdes(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, bool immediate)
:   make sure that all directories are valid

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`bool immediate`
:   submit immediately to the paging queue

**Description**

Makes sure all directories are up to date.

**Return**

0 for success, error for failure.

void amdgpu\_vm\_tlb\_seq\_cb(struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, struct [dma\_fence\_cb](../../driver-api/dma-buf.html#c.dma_fence_cb "dma_fence_cb") \*cb)
:   make sure to increment tlb sequence

**Parameters**

`struct dma_fence *fence`
:   unused

`struct dma_fence_cb *cb`
:   the callback structure

**Description**

Increments the tlb sequence to make sure that future CS execute a VM flush.

void amdgpu\_vm\_tlb\_flush(struct amdgpu\_vm\_update\_params \*params, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*\*fence, struct [amdgpu\_vm\_tlb\_seq\_struct](#c.amdgpu_vm_tlb_seq_struct "amdgpu_vm_tlb_seq_struct") \*tlb\_cb)
:   prepare TLB flush

**Parameters**

`struct amdgpu_vm_update_params *params`
:   parameters for update

`struct dma_fence **fence`
:   input fence to sync TLB flush with

`struct amdgpu_vm_tlb_seq_struct *tlb_cb`
:   the callback structure

**Description**

Increments the tlb sequence to make sure that future CS execute a VM flush.

int amdgpu\_vm\_update\_range(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, bool immediate, bool unlocked, bool flush\_tlb, bool allow\_override, struct amdgpu\_sync \*sync, uint64\_t start, uint64\_t last, uint64\_t flags, uint64\_t offset, uint64\_t vram\_base, struct [ttm\_resource](../drm-mm.html#c.ttm_resource "ttm_resource") \*res, dma\_addr\_t \*pages\_addr, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*\*fence)
:   update a range in the vm page table

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer to use for commands

`struct amdgpu_vm *vm`
:   the VM to update the range

`bool immediate`
:   immediate submission in a page fault

`bool unlocked`
:   unlocked invalidation during MM callback

`bool flush_tlb`
:   trigger tlb invalidation after update completed

`bool allow_override`
:   change MTYPE for local NUMA nodes

`struct amdgpu_sync *sync`
:   fences we need to sync to

`uint64_t start`
:   start of mapped range

`uint64_t last`
:   last mapped entry

`uint64_t flags`
:   flags for the entries

`uint64_t offset`
:   offset into nodes and pages\_addr

`uint64_t vram_base`
:   base for vram mappings

`struct ttm_resource *res`
:   ttm\_resource to map

`dma_addr_t *pages_addr`
:   DMA addresses to use for mapping

`struct dma_fence **fence`
:   optional resulting fence

**Description**

Fill in the page table entries between **start** and **last**.

**Return**

0 for success, negative erro code for failure.

int amdgpu\_vm\_bo\_update(struct amdgpu\_device \*adev, struct amdgpu\_bo\_va \*bo\_va, bool clear)
:   update all BO mappings in the vm page table

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_bo_va *bo_va`
:   requested BO and VM object

`bool clear`
:   if true clear the entries

**Description**

Fill in the page table entries for **bo\_va**.

**Return**

0 for success, -EINVAL for failure.

void amdgpu\_vm\_update\_prt\_state(struct amdgpu\_device \*adev)
:   update the global PRT state

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

void amdgpu\_vm\_prt\_get(struct amdgpu\_device \*adev)
:   add a PRT user

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

void amdgpu\_vm\_prt\_put(struct amdgpu\_device \*adev)
:   drop a PRT user

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

void amdgpu\_vm\_prt\_cb(struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence, struct [dma\_fence\_cb](../../driver-api/dma-buf.html#c.dma_fence_cb "dma_fence_cb") \*\_cb)
:   callback for updating the PRT status

**Parameters**

`struct dma_fence *fence`
:   fence for the callback

`struct dma_fence_cb *_cb`
:   the callback function

void amdgpu\_vm\_add\_prt\_cb(struct amdgpu\_device \*adev, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   add callback for updating the PRT status

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct dma_fence *fence`
:   fence for the callback

void amdgpu\_vm\_free\_mapping(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, struct amdgpu\_bo\_va\_mapping \*mapping, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*fence)
:   free a mapping

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`struct amdgpu_bo_va_mapping *mapping`
:   mapping to be freed

`struct dma_fence *fence`
:   fence of the unmap operation

**Description**

Free a mapping and make sure we decrease the PRT usage count if applicable.

void amdgpu\_vm\_prt\_fini(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm)
:   finish all prt mappings

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

**Description**

Register a cleanup callback to disable PRT support after VM dies.

int amdgpu\_vm\_clear\_freed(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, struct [dma\_fence](../../driver-api/dma-buf.html#c.dma_fence "dma_fence") \*\*fence)
:   clear freed BOs in the PT

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`struct dma_fence **fence`
:   optional resulting fence (unchanged if no work needed to be done
    or if an error occurred)

**Description**

Make sure all freed BOs are cleared in the PT.
PTs have to be reserved and mutex must be locked!

**Return**

0 for success.

int amdgpu\_vm\_handle\_moved(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, struct ww\_acquire\_ctx \*ticket)
:   handle moved BOs in the PT

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`struct ww_acquire_ctx *ticket`
:   optional reservation ticket used to reserve the VM

**Description**

Make sure all BOs which are moved are updated in the PTs.

PTs have to be reserved!

**Return**

0 for success.

int amdgpu\_vm\_flush\_compute\_tlb(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, uint32\_t flush\_type, uint32\_t xcc\_mask)
:   Flush TLB on compute VM

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`uint32_t flush_type`
:   flush type

`uint32_t xcc_mask`
:   mask of XCCs that belong to the compute partition in need of a TLB flush.

**Description**

Flush TLB if needed for a compute VM.

**Return**

0 for success.

struct amdgpu\_bo\_va \*amdgpu\_vm\_bo\_add(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, struct amdgpu\_bo \*bo)
:   add a bo to a specific vm

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`struct amdgpu_bo *bo`
:   amdgpu buffer object

**Description**

Add **bo** into the requested vm.
Add **bo** to the list of bos associated with the vm

Object has to be reserved!

**Return**

Newly added bo\_va or NULL for failure

void amdgpu\_vm\_bo\_insert\_map(struct amdgpu\_device \*adev, struct amdgpu\_bo\_va \*bo\_va, struct amdgpu\_bo\_va\_mapping \*mapping)
:   insert a new mapping

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_bo_va *bo_va`
:   bo\_va to store the address

`struct amdgpu_bo_va_mapping *mapping`
:   the mapping to insert

**Description**

Insert a new mapping into all structures.

int amdgpu\_vm\_bo\_map(struct amdgpu\_device \*adev, struct amdgpu\_bo\_va \*bo\_va, uint64\_t saddr, uint64\_t offset, uint64\_t size, uint32\_t flags)
:   map bo inside a vm

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_bo_va *bo_va`
:   bo\_va to store the address

`uint64_t saddr`
:   where to map the BO

`uint64_t offset`
:   requested offset in the BO

`uint64_t size`
:   BO size in bytes

`uint32_t flags`
:   attributes of pages (read/write/valid/etc.)

**Description**

Add a mapping of the BO at the specefied addr into the VM.

Object has to be reserved and unreserved outside!

**Return**

0 for success, error for failure.

int amdgpu\_vm\_bo\_replace\_map(struct amdgpu\_device \*adev, struct amdgpu\_bo\_va \*bo\_va, uint64\_t saddr, uint64\_t offset, uint64\_t size, uint32\_t flags)
:   map bo inside a vm, replacing existing mappings

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_bo_va *bo_va`
:   bo\_va to store the address

`uint64_t saddr`
:   where to map the BO

`uint64_t offset`
:   requested offset in the BO

`uint64_t size`
:   BO size in bytes

`uint32_t flags`
:   attributes of pages (read/write/valid/etc.)

**Description**

Add a mapping of the BO at the specefied addr into the VM. Replace existing
mappings as we do so.

Object has to be reserved and unreserved outside!

**Return**

0 for success, error for failure.

int amdgpu\_vm\_bo\_unmap(struct amdgpu\_device \*adev, struct amdgpu\_bo\_va \*bo\_va, uint64\_t saddr)
:   remove bo mapping from vm

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_bo_va *bo_va`
:   bo\_va to remove the address from

`uint64_t saddr`
:   where to the BO is mapped

**Description**

Remove a mapping of the BO at the specefied addr from the VM.

Object has to be reserved and unreserved outside!

**Return**

0 for success, error for failure.

int amdgpu\_vm\_bo\_clear\_mappings(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, uint64\_t saddr, uint64\_t size)
:   remove all mappings in a specific range

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   VM structure to use

`uint64_t saddr`
:   start of the range

`uint64_t size`
:   size of the range

**Description**

Remove all mappings in a range, split them as appropriate.

**Return**

0 for success, error for failure.

struct amdgpu\_bo\_va\_mapping \*amdgpu\_vm\_bo\_lookup\_mapping(struct amdgpu\_vm \*vm, uint64\_t addr)
:   find mapping by address

**Parameters**

`struct amdgpu_vm *vm`
:   the requested VM

`uint64_t addr`
:   the address

**Description**

Find a mapping by it’s address.

**Return**

The amdgpu\_bo\_va\_mapping matching for addr or NULL

void amdgpu\_vm\_bo\_trace\_cs(struct amdgpu\_vm \*vm, struct ww\_acquire\_ctx \*ticket)
:   trace all reserved mappings

**Parameters**

`struct amdgpu_vm *vm`
:   the requested vm

`struct ww_acquire_ctx *ticket`
:   CS ticket

**Description**

Trace all mappings of BOs reserved during a command submission.

void amdgpu\_vm\_bo\_del(struct amdgpu\_device \*adev, struct amdgpu\_bo\_va \*bo\_va)
:   remove a bo from a specific vm

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_bo_va *bo_va`
:   requested bo\_va

**Description**

Remove **bo\_va->bo** from the requested vm.

Object have to be reserved!

bool amdgpu\_vm\_evictable(struct amdgpu\_bo \*bo)
:   check if we can evict a VM

**Parameters**

`struct amdgpu_bo *bo`
:   A page table of the VM.

**Description**

Check if it is possible to evict a VM.

void amdgpu\_vm\_bo\_invalidate(struct amdgpu\_bo \*bo, bool evicted)
:   mark the bo as invalid

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu buffer object

`bool evicted`
:   is the BO evicted

**Description**

Mark **bo** as invalid.

void amdgpu\_vm\_bo\_move(struct amdgpu\_bo \*bo, struct [ttm\_resource](../drm-mm.html#c.ttm_resource "ttm_resource") \*new\_mem, bool evicted)
:   handle BO move

**Parameters**

`struct amdgpu_bo *bo`
:   amdgpu buffer object

`struct ttm_resource *new_mem`
:   the new placement of the BO move

`bool evicted`
:   is the BO evicted

**Description**

Update the memory stats for the new placement and mark **bo** as invalid.

uint32\_t amdgpu\_vm\_get\_block\_size(uint64\_t vm\_size)
:   calculate VM page table size as power of two

**Parameters**

`uint64_t vm_size`
:   VM size

**Return**

VM page table as power of two

void amdgpu\_vm\_adjust\_size(struct amdgpu\_device \*adev, uint32\_t min\_vm\_size, uint32\_t fragment\_size\_default, unsigned max\_level, unsigned max\_bits)
:   adjust vm size, block size and fragment size

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`uint32_t min_vm_size`
:   the minimum vm size in GB if it’s set auto

`uint32_t fragment_size_default`
:   Default PTE fragment size

`unsigned max_level`
:   max VMPT level

`unsigned max_bits`
:   max address space size in bits

long amdgpu\_vm\_wait\_idle(struct amdgpu\_vm \*vm, long timeout)
:   wait for the VM to become idle

**Parameters**

`struct amdgpu_vm *vm`
:   VM object to wait for

`long timeout`
:   timeout to wait for VM to become idle

void amdgpu\_vm\_put\_task\_info(struct amdgpu\_task\_info \*task\_info)
:   reference down the vm task\_info ptr

**Parameters**

`struct amdgpu_task_info *task_info`
:   task\_info `struct under` discussion.

**Description**

frees the vm task\_info ptr at the last put

struct amdgpu\_task\_info \*amdgpu\_vm\_get\_task\_info\_vm(struct amdgpu\_vm \*vm)
:   Extracts task info for a vm.

**Parameters**

`struct amdgpu_vm *vm`
:   VM to get info from

**Description**

Returns the reference counted task\_info structure, which must be
referenced down with amdgpu\_vm\_put\_task\_info.

struct amdgpu\_task\_info \*amdgpu\_vm\_get\_task\_info\_pasid(struct amdgpu\_device \*adev, u32 pasid)
:   Extracts task info for a PASID.

**Parameters**

`struct amdgpu_device *adev`
:   drm device pointer

`u32 pasid`
:   PASID identifier for VM

**Description**

Returns the reference counted task\_info structure, which must be
referenced down with amdgpu\_vm\_put\_task\_info.

void amdgpu\_vm\_set\_task\_info(struct amdgpu\_vm \*vm)
:   Sets VMs task info.

**Parameters**

`struct amdgpu_vm *vm`
:   vm for which to set the info

int amdgpu\_vm\_init(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm, int32\_t xcp\_id, uint32\_t pasid)
:   initialize a vm instance

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

`int32_t xcp_id`
:   GPU partition selection id

`uint32_t pasid`
:   the pasid the VM is using on this GPU

**Description**

Init **vm** fields.

**Return**

0 for success, error for failure.

int amdgpu\_vm\_make\_compute(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm)
:   Turn a GFX VM into a compute VM

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

**Description**

This only works on GFX VMs that don’t have any BOs added and no
page tables allocated yet.

Changes the following VM parameters:
- use\_cpu\_for\_update
- pte\_supports\_ats

Reinitializes the page directory to reflect the changed ATS
setting.

**Return**

0 for success, -errno for errors.

void amdgpu\_vm\_fini(struct amdgpu\_device \*adev, struct amdgpu\_vm \*vm)
:   tear down a vm instance

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

`struct amdgpu_vm *vm`
:   requested vm

**Description**

Tear down **vm**.
Unbind the VM and remove all bos from the vm bo list

void amdgpu\_vm\_manager\_init(struct amdgpu\_device \*adev)
:   init the VM manager

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

**Description**

Initialize the VM manager structures

void amdgpu\_vm\_manager\_fini(struct amdgpu\_device \*adev)
:   cleanup VM manager

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

**Description**

Cleanup the VM manager and free resources.

int amdgpu\_vm\_ioctl(struct [drm\_device](../drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](../drm-internals.html#c.drm_file "drm_file") \*filp)
:   Manages VMID reservation for vm hubs.

**Parameters**

`struct drm_device *dev`
:   drm device pointer

`void *data`
:   drm\_amdgpu\_vm

`struct drm_file *filp`
:   drm file pointer

**Return**

0 for success, -errno for errors.

struct amdgpu\_vm \*amdgpu\_vm\_lock\_by\_pasid(struct amdgpu\_device \*adev, struct amdgpu\_bo \*\*root, u32 pasid)
:   return an amdgpu\_vm and its root bo from a pasid, if possible.

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_bo **root`
:   root BO of the VM

`u32 pasid`
:   PASID of the VM
    The caller needs to unreserve and unref the root bo on success.

bool amdgpu\_vm\_handle\_fault(struct amdgpu\_device \*adev, u32 pasid, u32 vmid, u32 node\_id, uint64\_t addr, uint64\_t ts, bool write\_fault)
:   graceful handling of VM faults.

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`u32 pasid`
:   PASID of the VM

`u32 vmid`
:   VMID, only used for GFX 9.4.3.

`u32 node_id`
:   Node\_id received in IH cookie. Only applicable for
    GFX 9.4.3.

`uint64_t addr`
:   Address of the fault

`uint64_t ts`
:   Timestamp of the fault

`bool write_fault`
:   true is write fault, false is read fault

**Description**

Try to gracefully handle a VM fault. Return true if the fault was handled and
shouldn’t be reported any more.

void amdgpu\_debugfs\_vm\_bo\_info(struct amdgpu\_vm \*vm, struct seq\_file \*m)
:   print BO info for the VM

**Parameters**

`struct amdgpu_vm *vm`
:   Requested VM for printing BO info

`struct seq_file *m`
:   debugfs file

**Description**

Print BO information in debugfs file for the VM

void amdgpu\_vm\_update\_fault\_cache(struct amdgpu\_device \*adev, unsigned int pasid, uint64\_t addr, uint32\_t status, unsigned int vmhub)
:   update cached fault into.

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`unsigned int pasid`
:   PASID of the VM

`uint64_t addr`
:   Address of the fault

`uint32_t status`
:   GPUVM fault status register

`unsigned int vmhub`
:   which vmhub got the fault

**Description**

Cache the fault info for later use by userspace in debugging.

## Interrupt Handling

Interrupts generated within GPU hardware raise interrupt requests that are
passed to amdgpu IRQ handler which is responsible for detecting source and
type of the interrupt and dispatching matching handlers. If handling an
interrupt requires calling kernel functions that may sleep processing is
dispatched to work handlers.

If MSI functionality is not disabled by module parameter then MSI
support will be enabled.

For GPU interrupt sources that may be driven by another driver, IRQ domain
support is used (with mapping between virtual and hardware IRQs).

void amdgpu\_irq\_disable\_all(struct amdgpu\_device \*adev)
:   disable *all* interrupts

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

**Description**

Disable all types of interrupts from all sources.

irqreturn\_t amdgpu\_irq\_handler(int irq, void \*arg)
:   IRQ handler

**Parameters**

`int irq`
:   IRQ number (unused)

`void *arg`
:   pointer to DRM device

**Description**

IRQ handler for amdgpu driver (all ASICs).

**Return**

result of handling the IRQ, as defined by `irqreturn_t`

void amdgpu\_irq\_handle\_ih1(struct work\_struct \*work)
:   kick of processing for IH1

**Parameters**

`struct work_struct *work`
:   work structure in `struct amdgpu_irq`

**Description**

Kick of processing IH ring 1.

void amdgpu\_irq\_handle\_ih2(struct work\_struct \*work)
:   kick of processing for IH2

**Parameters**

`struct work_struct *work`
:   work structure in `struct amdgpu_irq`

**Description**

Kick of processing IH ring 2.

void amdgpu\_irq\_handle\_ih\_soft(struct work\_struct \*work)
:   kick of processing for ih\_soft

**Parameters**

`struct work_struct *work`
:   work structure in `struct amdgpu_irq`

**Description**

Kick of processing IH soft ring.

bool amdgpu\_msi\_ok(struct amdgpu\_device \*adev)
:   check whether MSI functionality is enabled

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer (unused)

**Description**

Checks whether MSI functionality has been disabled via module parameter
(all ASICs).

**Return**

*true* if MSIs are allowed to be enabled or *false* otherwise

int amdgpu\_irq\_init(struct amdgpu\_device \*adev)
:   initialize interrupt handling

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

**Description**

Sets up work functions for hotplug and reset interrupts, enables MSI
functionality, initializes vblank, hotplug and reset interrupt handling.

**Return**

0 on success or error code on failure

void amdgpu\_irq\_fini\_sw(struct amdgpu\_device \*adev)
:   shut down interrupt handling

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

**Description**

Tears down work functions for hotplug and reset interrupts, disables MSI
functionality, shuts down vblank, hotplug and reset interrupt handling,
turns off interrupts from all sources (all ASICs).

int amdgpu\_irq\_add\_id(struct amdgpu\_device \*adev, unsigned int client\_id, unsigned int src\_id, struct amdgpu\_irq\_src \*source)
:   register IRQ source

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`unsigned int client_id`
:   client id

`unsigned int src_id`
:   source id

`struct amdgpu_irq_src *source`
:   IRQ source pointer

**Description**

Registers IRQ source on a client.

**Return**

0 on success or error code otherwise

void amdgpu\_irq\_dispatch(struct amdgpu\_device \*adev, struct amdgpu\_ih\_ring \*ih)
:   dispatch IRQ to IP blocks

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_ih_ring *ih`
:   interrupt ring instance

**Description**

Dispatches IRQ to IP blocks.

void amdgpu\_irq\_delegate(struct amdgpu\_device \*adev, struct amdgpu\_iv\_entry \*entry, unsigned int num\_dw)
:   delegate IV to soft IH ring

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_iv_entry *entry`
:   IV entry

`unsigned int num_dw`
:   size of IV

**Description**

Delegate the IV to the soft IH ring and schedule processing of it. Used
if the hardware delegation to IH1 or IH2 doesn’t work for some reason.

int amdgpu\_irq\_update(struct amdgpu\_device \*adev, struct amdgpu\_irq\_src \*src, unsigned int type)
:   update hardware interrupt state

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_irq_src *src`
:   interrupt source pointer

`unsigned int type`
:   type of interrupt

**Description**

Updates interrupt state for the specific source (all ASICs).

void amdgpu\_irq\_gpu\_reset\_resume\_helper(struct amdgpu\_device \*adev)
:   update interrupt states on all sources

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

**Description**

Updates state of all types of interrupts on all sources on resume after
reset.

int amdgpu\_irq\_get(struct amdgpu\_device \*adev, struct amdgpu\_irq\_src \*src, unsigned int type)
:   enable interrupt

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_irq_src *src`
:   interrupt source pointer

`unsigned int type`
:   type of interrupt

**Description**

Enables specified type of interrupt on the specified source (all ASICs).

**Return**

0 on success or error code otherwise

int amdgpu\_irq\_put(struct amdgpu\_device \*adev, struct amdgpu\_irq\_src \*src, unsigned int type)
:   disable interrupt

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_irq_src *src`
:   interrupt source pointer

`unsigned int type`
:   type of interrupt

**Description**

Enables specified type of interrupt on the specified source (all ASICs).

**Return**

0 on success or error code otherwise

bool amdgpu\_irq\_enabled(struct amdgpu\_device \*adev, struct amdgpu\_irq\_src \*src, unsigned int type)
:   check whether interrupt is enabled or not

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`struct amdgpu_irq_src *src`
:   interrupt source pointer

`unsigned int type`
:   type of interrupt

**Description**

Checks whether the given type of interrupt is enabled on the given source.

**Return**

*true* if interrupt is enabled, *false* if interrupt is disabled or on
invalid parameters

int amdgpu\_irqdomain\_map(struct [irq\_domain](../../core-api/irq/irq-domain.html#c.irq_domain "irq_domain") \*d, unsigned int irq, irq\_hw\_number\_t hwirq)
:   create mapping between virtual and hardware IRQ numbers

**Parameters**

`struct irq_domain *d`
:   amdgpu IRQ domain pointer (unused)

`unsigned int irq`
:   virtual IRQ number

`irq_hw_number_t hwirq`
:   hardware irq number

**Description**

Current implementation assigns simple interrupt handler to the given virtual
IRQ.

**Return**

0 on success or error code otherwise

int amdgpu\_irq\_add\_domain(struct amdgpu\_device \*adev)
:   create a linear IRQ domain

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

**Description**

Creates an IRQ domain for GPU interrupt sources
that may be driven by another driver (e.g., ACP).

**Return**

0 on success or error code otherwise

void amdgpu\_irq\_remove\_domain(struct amdgpu\_device \*adev)
:   remove the IRQ domain

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

**Description**

Removes the IRQ domain for GPU interrupt sources
that may be driven by another driver (e.g., ACP).

unsigned int amdgpu\_irq\_create\_mapping(struct amdgpu\_device \*adev, unsigned int src\_id)
:   create mapping between domain Linux IRQs

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device pointer

`unsigned int src_id`
:   IH source id

**Description**

Creates mapping between a domain IRQ (GPU IH src id) and a Linux IRQ
Use this for components that generate a GPU interrupt, but are driven
by a different driver (e.g., ACP).

**Return**

Linux IRQ

## IP Blocks

GPUs are composed of IP (intellectual property) blocks. These
IP blocks provide various functionalities: display, graphics,
video decode, etc. The IP blocks that comprise a particular GPU
are listed in the GPU’s respective SoC file. amdgpu\_device.c
acquires the list of IP blocks for the GPU in use on initialization.
It can then operate on this list to perform standard driver operations
such as: init, fini, suspend, resume, etc.

IP block implementations are named using the following convention:
<functionality>\_v<version> (E.g.: gfx\_v6\_0).

enum amd\_ip\_block\_type
:   Used to classify IP blocks by functionality.

**Constants**

`AMD_IP_BLOCK_TYPE_COMMON`
:   GPU Family

`AMD_IP_BLOCK_TYPE_GMC`
:   Graphics Memory Controller

`AMD_IP_BLOCK_TYPE_IH`
:   Interrupt Handler

`AMD_IP_BLOCK_TYPE_SMC`
:   System Management Controller

`AMD_IP_BLOCK_TYPE_PSP`
:   Platform Security Processor

`AMD_IP_BLOCK_TYPE_DCE`
:   Display and Compositing Engine

`AMD_IP_BLOCK_TYPE_GFX`
:   Graphics and Compute Engine

`AMD_IP_BLOCK_TYPE_SDMA`
:   System DMA Engine

`AMD_IP_BLOCK_TYPE_UVD`
:   Unified Video Decoder

`AMD_IP_BLOCK_TYPE_VCE`
:   Video Compression Engine

`AMD_IP_BLOCK_TYPE_ACP`
:   Audio Co-Processor

`AMD_IP_BLOCK_TYPE_VCN`
:   Video Core/Codec Next

`AMD_IP_BLOCK_TYPE_MES`
:   Micro-Engine Scheduler

`AMD_IP_BLOCK_TYPE_JPEG`
:   JPEG Engine

`AMD_IP_BLOCK_TYPE_VPE`
:   Video Processing Engine

`AMD_IP_BLOCK_TYPE_UMSCH_MM`
:   User Mode Scheduler for Multimedia

`AMD_IP_BLOCK_TYPE_ISP`
:   Image Signal Processor

`AMD_IP_BLOCK_TYPE_RAS`
:   Reliability, Availability, Serviceability

`AMD_IP_BLOCK_TYPE_NUM`
:   Total number of IP block types

enum DC\_FEATURE\_MASK
:   Bits that control DC feature defaults

**Constants**

`DC_FBC_MASK`
:   (0x1) disabled by default

`DC_MULTI_MON_PP_MCLK_SWITCH_MASK`
:   (0x2) enabled by default

`DC_DISABLE_FRACTIONAL_PWM_MASK`
:   (0x4) disabled by default

`DC_PSR_MASK`
:   (0x8) disabled by default for DCN < 3.1

`DC_EDP_NO_POWER_SEQUENCING`
:   (0x10) disabled by default

`DC_DISABLE_LTTPR_DP1_4A`
:   (0x20) disabled by default

`DC_DISABLE_LTTPR_DP2_0`
:   (0x40) disabled by default

`DC_PSR_ALLOW_SMU_OPT`
:   (0x80) disabled by default

`DC_PSR_ALLOW_MULTI_DISP_OPT`
:   (0x100) disabled by default

`DC_REPLAY_MASK`
:   (0x200) disabled by default for DCN < 3.1.4

enum DC\_DEBUG\_MASK
:   Bits that are useful for debugging the Display Core IP

**Constants**

`DC_DISABLE_PIPE_SPLIT`
:   (0x1) If set, disable pipe-splitting

`DC_DISABLE_STUTTER`
:   (0x2) If set, disable memory stutter mode

`DC_DISABLE_DSC`
:   (0x4) If set, disable display stream compression

`DC_DISABLE_CLOCK_GATING`
:   (0x8) If set, disable clock gating optimizations

`DC_DISABLE_PSR`
:   (0x10) If set, disable Panel self refresh v1 and PSR-SU

`DC_FORCE_SUBVP_MCLK_SWITCH`
:   (0x20) If set, force mclk switch in subvp, even
    if mclk switch in vblank is possible

`DC_DISABLE_MPO`
:   (0x40) If set, disable multi-plane offloading

`DC_ENABLE_DPIA_TRACE`
:   (0x80) If set, enable trace logging for DPIA

`DC_ENABLE_DML2`
:   (0x100) If set, force usage of DML2, even if the DCN version
    does not default to it.

`DC_DISABLE_PSR_SU`
:   (0x200) If set, disable PSR SU

`DC_DISABLE_REPLAY`
:   (0x400) If set, disable Panel Replay

`DC_DISABLE_IPS`
:   (0x800) If set, disable all Idle Power States, all the time.
    If more than one IPS debug bit is set, the lowest bit takes
    precedence. For example, if DC\_FORCE\_IPS\_ENABLE and
    DC\_DISABLE\_IPS\_DYNAMIC are set, then DC\_DISABLE\_IPS\_DYNAMIC takes
    precedence.

`DC_DISABLE_IPS_DYNAMIC`
:   (0x1000) If set, disable all IPS, all the time,
    *except* when driver goes into suspend.

`DC_DISABLE_IPS2_DYNAMIC`
:   (0x2000) If set, disable IPS2 (IPS1 allowed) if
    there is an enabled display. Otherwise, enable all IPS.

`DC_FORCE_IPS_ENABLE`
:   (0x4000) If set, force enable all IPS, all the time.

`DC_DISABLE_ACPI_EDID`
:   (0x8000) If set, don’t attempt to fetch EDID for
    eDP display from ACPI \_DDC method.

`DC_DISABLE_HDMI_CEC`
:   (0x10000) If set, disable HDMI-CEC feature in amdgpu driver.

`DC_DISABLE_SUBVP_FAMS`
:   (0x20000) If set, disable DCN Sub-Viewport & Firmware Assisted
    Memory Clock Switching (FAMS) feature in amdgpu driver.

`DC_DISABLE_CUSTOM_BRIGHTNESS_CURVE`
:   (0x40000) If set, disable support for custom
    brightness curves

`DC_HDCP_LC_FORCE_FW_ENABLE`
:   (0x80000) If set, use HDCP Locality Check FW
    path regardless of reported HW capabilities.

`DC_HDCP_LC_ENABLE_SW_FALLBACK`
:   (0x100000) If set, upon HDCP Locality Check FW
    path failure, retry using legacy SW path.

`DC_SKIP_DETECTION_LT`
:   (0x200000) If set, skip detection link training

struct amd\_ip\_funcs
:   general hooks for managing amdgpu IP Blocks

**Definition**:

```
struct amd_ip_funcs {
    char *name;
    int (*early_init)(struct amdgpu_ip_block *ip_block);
    int (*late_init)(struct amdgpu_ip_block *ip_block);
    int (*sw_init)(struct amdgpu_ip_block *ip_block);
    int (*sw_fini)(struct amdgpu_ip_block *ip_block);
    int (*early_fini)(struct amdgpu_ip_block *ip_block);
    int (*hw_init)(struct amdgpu_ip_block *ip_block);
    int (*hw_fini)(struct amdgpu_ip_block *ip_block);
    void (*late_fini)(struct amdgpu_ip_block *ip_block);
    int (*prepare_suspend)(struct amdgpu_ip_block *ip_block);
    int (*suspend)(struct amdgpu_ip_block *ip_block);
    int (*resume)(struct amdgpu_ip_block *ip_block);
    void (*complete)(struct amdgpu_ip_block *ip_block);
    bool (*is_idle)(struct amdgpu_ip_block *ip_block);
    int (*wait_for_idle)(struct amdgpu_ip_block *ip_block);
    bool (*check_soft_reset)(struct amdgpu_ip_block *ip_block);
    int (*pre_soft_reset)(struct amdgpu_ip_block *ip_block);
    int (*soft_reset)(struct amdgpu_ip_block *ip_block);
    int (*post_soft_reset)(struct amdgpu_ip_block *ip_block);
    int (*set_clockgating_state)(struct amdgpu_ip_block *ip_block, enum amd_clockgating_state state);
    int (*set_powergating_state)(struct amdgpu_ip_block *ip_block, enum amd_powergating_state state);
    void (*get_clockgating_state)(struct amdgpu_ip_block *ip_block, u64 *flags);
    void (*dump_ip_state)(struct amdgpu_ip_block *ip_block);
    void (*print_ip_state)(struct amdgpu_ip_block *ip_block, struct drm_printer *p);
};
```

**Members**

`name`
:   Name of IP block

`early_init`
:   sets up early driver state (pre sw\_init),
    does not configure hw - Optional

`late_init`
:   sets up late driver/hw state (post hw\_init) - Optional

`sw_init`
:   sets up driver state, does not configure hw

`sw_fini`
:   tears down driver state, does not configure hw

`early_fini`
:   tears down stuff before dev detached from driver

`hw_init`
:   sets up the hw state

`hw_fini`
:   tears down the hw state

`late_fini`
:   final cleanup

`prepare_suspend`
:   handle IP specific changes to prepare for suspend
    (such as allocating any required memory)

`suspend`
:   handles IP specific hw/sw changes for suspend

`resume`
:   handles IP specific hw/sw changes for resume

`complete`
:   handles IP specific changes after resume

`is_idle`
:   returns current IP block idle status

`wait_for_idle`
:   poll for idle

`check_soft_reset`
:   check soft reset the IP block

`pre_soft_reset`
:   pre soft reset the IP block

`soft_reset`
:   soft reset the IP block

`post_soft_reset`
:   post soft reset the IP block

`set_clockgating_state`
:   enable/disable cg for the IP block

`set_powergating_state`
:   enable/disable pg for the IP block

`get_clockgating_state`
:   get current clockgating status

`dump_ip_state`
:   dump the IP state of the ASIC during a gpu hang

`print_ip_state`
:   print the IP state in devcoredump for each IP of the ASIC

**Description**

These hooks provide an interface for controlling the operational state
of IP blocks. After acquiring a list of IP blocks for the GPU in use,
the driver can make chip-wide state changes by walking this list and
making calls to hooks from each IP block. This list is ordered to ensure
that the driver initializes the IP blocks in a safe sequence.
