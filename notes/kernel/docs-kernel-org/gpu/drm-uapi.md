# Userland interfaces

> 출처(원문): https://docs.kernel.org/gpu/drm-uapi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Userland interfaces

The DRM core exports several interfaces to applications, generally
intended to be used through corresponding libdrm wrapper functions. In
addition, drivers export device-specific interfaces for use by userspace
drivers & device-aware applications through ioctls and sysfs files.

External interfaces include: memory mapping, context management, DMA
operations, AGP management, vblank control, fence management, memory
management, and output management.

Cover generic ioctls and sysfs layout here. We only need high-level
info, since man pages should cover the rest.

## libdrm Device Lookup

BEWARE THE DRAGONS! MIND THE TRAPDOORS!

In an attempt to warn anyone else who’s trying to figure out what’s going
on here, I’ll try to summarize the story. First things first, let’s clear up
the names, because the kernel internals, libdrm and the ioctls are all named
differently:

> * GET\_UNIQUE ioctl, implemented by drm\_getunique is wrapped up in libdrm
>   through the drmGetBusid function.
> * The libdrm drmSetBusid function is backed by the SET\_UNIQUE ioctl. All
>   that code is nerved in the kernel with [`drm_invalid_op()`](#c.drm_invalid_op "drm_invalid_op").
> * The internal set\_busid kernel functions and driver callbacks are
>   exclusively use by the SET\_VERSION ioctl, because only drm 1.0 (which is
>   nerved) allowed userspace to set the busid through the above ioctl.
> * Other ioctls and functions involved are named consistently.

For anyone wondering what’s the difference between drm 1.1 and 1.4: Correctly
handling pci domains in the busid on ppc. Doing this correctly was only
implemented in libdrm in 2010, hence can’t be nerved yet. No one knows what’s
special with drm 1.2 and 1.3.

Now the actual horror story of how device lookup in drm works. At large,
there’s 2 different ways, either by busid, or by device driver name.

Opening by busid is fairly simple:

1. First call SET\_VERSION to make sure pci domains are handled properly. As a
   side-effect this fills out the unique name in the master structure.
2. Call GET\_UNIQUE to read out the unique name from the master structure,
   which matches the busid thanks to step 1. If it doesn’t, proceed to try
   the next device node.

Opening by name is slightly different:

1. Directly call VERSION to get the version and to match against the driver
   name returned by that ioctl. Note that SET\_VERSION is not called, which
   means the unique name for the master node just opening is \_not\_ filled
   out. This despite that with current drm device nodes are always bound to
   one device, and can’t be runtime assigned like with drm 1.0.
2. Match driver name. If it mismatches, proceed to the next device node.
3. Call GET\_UNIQUE, and check whether the unique name has length zero (by
   checking that the first byte in the string is 0). If that’s not the case
   libdrm skips and proceeds to the next device node. Probably this is just
   copypasta from drm 1.0 times where a set unique name meant that the driver
   was in use already, but that’s just conjecture.

Long story short: To keep the open by name logic working, GET\_UNIQUE must
\_not\_ return a unique string when SET\_VERSION hasn’t been called yet,
otherwise libdrm breaks. Even when that unique string can’t ever change, and
is totally irrelevant for actually opening the device because runtime
assignable device instances were only support in drm 1.0, which is long dead.
But the libdrm code in drmOpenByName somehow survived, hence this can’t be
broken.

## Primary Nodes, DRM Master and Authentication

[`struct drm_master`](#c.drm_master "drm_master") is used to track groups of clients with open
primary device nodes. For every [`struct drm_file`](drm-internals.html#c.drm_file "drm_file") which has had at
least once successfully became the device master (either through the
SET\_MASTER IOCTL, or implicitly through opening the primary device node when
no one else is the current master that time) there exists one [`drm_master`](#c.drm_master "drm_master").
This is noted in [`drm_file.is_master`](drm-internals.html#c.drm_file "drm_file"). All other clients have just a pointer
to the [`drm_master`](#c.drm_master "drm_master") they are associated with.

In addition only one [`drm_master`](#c.drm_master "drm_master") can be the current master for a [`drm_device`](drm-internals.html#c.drm_device "drm_device").
It can be switched through the DROP\_MASTER and SET\_MASTER IOCTL, or
implicitly through closing/opening the primary device node. See also
[`drm_is_current_master()`](#c.drm_is_current_master "drm_is_current_master").

Clients can authenticate against the current master (if it matches their own)
using the GETMAGIC and AUTHMAGIC IOCTLs. Together with exchanging masters,
this allows controlled access to the device for an entire group of mutually
trusted clients.

bool drm\_is\_current\_master(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*fpriv)
:   checks whether **priv** is the current master

**Parameters**

`struct drm_file *fpriv`
:   DRM file private

**Description**

Checks whether **fpriv** is current master on its device. This decides whether a
client is allowed to run DRM\_MASTER IOCTLs.

Most of the modern IOCTL which require DRM\_MASTER are for kernel modesetting
- the current master is assumed to own the non-shareable display hardware.

struct [drm\_master](#c.drm_master "drm_master") \*drm\_master\_get(struct [drm\_master](#c.drm_master "drm_master") \*master)
:   reference a master pointer

**Parameters**

`struct drm_master *master`
:   [`struct drm_master`](#c.drm_master "drm_master")

**Description**

Increments the reference count of **master** and returns a pointer to **master**.

struct [drm\_master](#c.drm_master "drm_master") \*drm\_file\_get\_master(struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv)
:   reference [`drm_file.master`](drm-internals.html#c.drm_file "drm_file") of **file\_priv**

**Parameters**

`struct drm_file *file_priv`
:   DRM file private

**Description**

Increments the reference count of **file\_priv**’s [`drm_file.master`](drm-internals.html#c.drm_file "drm_file") and returns
the [`drm_file.master`](drm-internals.html#c.drm_file "drm_file"). If **file\_priv** has no [`drm_file.master`](drm-internals.html#c.drm_file "drm_file"), returns NULL.

Master pointers returned from this function should be unreferenced using
[`drm_master_put()`](#c.drm_master_put "drm_master_put").

void drm\_master\_put(struct [drm\_master](#c.drm_master "drm_master") \*\*master)
:   unreference and clear a master pointer

**Parameters**

`struct drm_master **master`
:   pointer to a pointer of [`struct drm_master`](#c.drm_master "drm_master")

**Description**

This decrements the [`drm_master`](#c.drm_master "drm_master") behind **master** and sets it to NULL.

struct drm\_master
:   drm master structure

**Definition**:

```
struct drm_master {
    struct kref refcount;
    struct drm_device *dev;
    char *unique;
    int unique_len;
    struct idr magic_map;
    void *driver_priv;
    struct drm_master *lessor;
    int lessee_id;
    struct list_head lessee_list;
    struct list_head lessees;
    struct idr leases;
    struct idr lessee_idr;
};
```

**Members**

`refcount`
:   Refcount for this master object.

`dev`
:   Link back to the DRM device

`unique`
:   Unique identifier: e.g. busid. Protected by
    [`drm_device.master_mutex`](drm-internals.html#c.drm_device "drm_device").

`unique_len`
:   Length of unique field. Protected by
    [`drm_device.master_mutex`](drm-internals.html#c.drm_device "drm_device").

`magic_map`
:   Map of used authentication tokens. Protected by
    [`drm_device.master_mutex`](drm-internals.html#c.drm_device "drm_device").

`driver_priv`
:   Pointer to driver-private information.

`lessor`
:   Lease grantor, only set if this [`struct drm_master`](#c.drm_master "drm_master") represents a
    lessee holding a lease of objects from **lessor**. Full owners of the
    device have this set to NULL.

    The lessor does not change once it’s set in `drm_lease_create()`, and
    each lessee holds a reference to its lessor that it releases upon
    being destroyed in `drm_lease_destroy()`.

    See also the [section on display resource leasing](#drm-leasing).

`lessee_id`
:   ID for lessees. Owners (i.e. **lessor** is NULL) always have ID 0.
    Protected by [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device")’s [`drm_mode_config.idr_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config").

`lessee_list`
:   List entry of lessees of **lessor**, where they are linked to **lessees**.
    Not used for owners. Protected by [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device")’s
    [`drm_mode_config.idr_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config").

`lessees`
:   List of drm\_masters leasing from this one. Protected by
    [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device")’s [`drm_mode_config.idr_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config").

    This list is empty if no leases have been granted, or if all lessees
    have been destroyed. Since lessors are referenced by all their
    lessees, this master cannot be destroyed unless the list is empty.

`leases`
:   Objects leased to this drm\_master. Protected by
    [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device")’s [`drm_mode_config.idr_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config").

    Objects are leased all together in `drm_lease_create()`, and are
    removed all together when the lease is revoked.

`lessee_idr`
:   All lessees under this owner (only used where **lessor** is NULL).
    Protected by [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device")’s [`drm_mode_config.idr_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config").

**Description**

Note that master structures are only relevant for the legacy/primary device
nodes, hence there can only be one per device, not one per drm\_minor.

## DRM Display Resource Leasing

DRM leases provide information about whether a DRM master may control a DRM
mode setting object. This enables the creation of multiple DRM masters that
manage subsets of display resources.

The original DRM master of a device ‘owns’ the available drm resources. It
may create additional DRM masters and ‘lease’ resources which it controls
to the new DRM master. This gives the new DRM master control over the
leased resources until the owner revokes the lease, or the new DRM master
is closed. Some helpful terminology:

* An ‘owner’ is a [`struct drm_master`](#c.drm_master "drm_master") that is not leasing objects from
  another [`struct drm_master`](#c.drm_master "drm_master"), and hence ‘owns’ the objects. The owner can be
  identified as the [`struct drm_master`](#c.drm_master "drm_master") for which [`drm_master.lessor`](#c.drm_master "drm_master") is NULL.
* A ‘lessor’ is a [`struct drm_master`](#c.drm_master "drm_master") which is leasing objects to one or more
  other [`struct drm_master`](#c.drm_master "drm_master"). Currently, lessees are not allowed to
  create sub-leases, hence the lessor is the same as the owner.
* A ‘lessee’ is a [`struct drm_master`](#c.drm_master "drm_master") which is leasing objects from some
  other [`struct drm_master`](#c.drm_master "drm_master"). Each lessee only leases resources from a single
  lessor recorded in [`drm_master.lessor`](#c.drm_master "drm_master"), and holds the set of objects that
  it is leasing in [`drm_master.leases`](#c.drm_master "drm_master").
* A ‘lease’ is a contract between the lessor and lessee that identifies
  which resources may be controlled by the lessee. All of the resources
  that are leased must be owned by or leased to the lessor, and lessors are
  not permitted to lease the same object to multiple lessees.

The set of objects any [`struct drm_master`](#c.drm_master "drm_master") ‘controls’ is limited to the set
of objects it leases (for lessees) or all objects (for owners).

Objects not controlled by a [`struct drm_master`](#c.drm_master "drm_master") cannot be modified through
the various state manipulating ioctls, and any state reported back to user
space will be edited to make them appear idle and/or unusable. For
instance, connectors always report ‘disconnected’, while encoders
report no possible crtcs or clones.

Since each lessee may lease objects from a single lessor, display resource
leases form a tree of [`struct drm_master`](#c.drm_master "drm_master"). As lessees are currently not
allowed to create sub-leases, the tree depth is limited to 1. All of
these get activated simultaneously when the top level device owner changes
through the SETMASTER or DROPMASTER IOCTL, so [`drm_device.master`](drm-internals.html#c.drm_device "drm_device") points to
the owner at the top of the lease tree (i.e. the [`struct drm_master`](#c.drm_master "drm_master") for which
[`drm_master.lessor`](#c.drm_master "drm_master") is NULL). The full list of lessees that are leasing
objects from the owner can be searched via the owner’s
[`drm_master.lessee_idr`](#c.drm_master "drm_master").

## Open-Source Userspace Requirements

The DRM subsystem has stricter requirements than most other kernel subsystems on
what the userspace side for new uAPI needs to look like. This section here
explains what exactly those requirements are, and why they exist.

The short summary is that any addition of DRM uAPI requires corresponding
open-sourced userspace patches, and those patches must be reviewed and ready for
merging into a suitable and canonical upstream project.

GFX devices (both display and render/GPU side) are really complex bits of
hardware, with userspace and kernel by necessity having to work together really
closely. The interfaces, for rendering and modesetting, must be extremely wide
and flexible, and therefore it is almost always impossible to precisely define
them for every possible corner case. This in turn makes it really practically
infeasible to differentiate between behaviour that’s required by userspace, and
which must not be changed to avoid regressions, and behaviour which is only an
accidental artifact of the current implementation.

Without access to the full source code of all userspace users that means it
becomes impossible to change the implementation details, since userspace could
depend upon the accidental behaviour of the current implementation in minute
details. And debugging such regressions without access to source code is pretty
much impossible. As a consequence this means:

* The Linux kernel’s “no regression” policy holds in practice only for
  open-source userspace of the DRM subsystem. DRM developers are perfectly fine
  if closed-source blob drivers in userspace use the same uAPI as the open
  drivers, but they must do so in the exact same way as the open drivers.
  Creative (ab)use of the interfaces will, and in the past routinely has, lead
  to breakage.
* Any new userspace interface must have an open-source implementation as
  demonstration vehicle.

The other reason for requiring open-source userspace is uAPI review. Since the
kernel and userspace parts of a GFX stack must work together so closely, code
review can only assess whether a new interface achieves its goals by looking at
both sides. Making sure that the interface indeed covers the use-case fully
leads to a few additional requirements:

* The open-source userspace must not be a toy/test application, but the real
  thing. Specifically it needs to handle all the usual error and corner cases.
  These are often the places where new uAPI falls apart and hence essential to
  assess the fitness of a proposed interface.
* The userspace side must be fully reviewed and tested to the standards of that
  userspace project. For e.g. mesa this means piglit testcases and review on the
  mailing list. This is again to ensure that the new interface actually gets the
  job done. The userspace-side reviewer should also provide an Acked-by on the
  kernel uAPI patch indicating that they believe the proposed uAPI is sound and
  sufficiently documented and validated for userspace’s consumption.
* The userspace patches must be against the canonical upstream, not some vendor
  fork. This is to make sure that no one cheats on the review and testing
  requirements by doing a quick fork.
* The kernel patch can only be merged after all the above requirements are met,
  but it **must** be merged to either drm-next or drm-misc-next **before** the
  userspace patches land. uAPI always flows from the kernel, doing things the
  other way round risks divergence of the uAPI definitions and header files.

These are fairly steep requirements, but have grown out from years of shared
pain and experience with uAPI added hastily, and almost always regretted about
just as fast. GFX devices change really fast, requiring a paradigm shift and
entire new set of uAPI interfaces every few years at least. Together with the
Linux kernel’s guarantee to keep existing userspace running for 10+ years this
is already rather painful for the DRM subsystem, with multiple different uAPIs
for the same thing co-existing. If we add a few more complete mistakes into the
mix every year it would be entirely unmanageable.

## Render nodes

DRM core provides multiple character-devices for user-space to use.
Depending on which device is opened, user-space can perform a different
set of operations (mainly ioctls). The primary node is always created
and called card<num>. Additionally, a currently unused control node,
called controlD<num> is also created. The primary node provides all
legacy operations and historically was the only interface used by
userspace. With KMS, the control node was introduced. However, the
planned KMS control interface has never been written and so the control
node stays unused to date.

With the increased use of offscreen renderers and GPGPU applications,
clients no longer require running compositors or graphics servers to
make use of a GPU. But the DRM API required unprivileged clients to
authenticate to a DRM-Master prior to getting GPU access. To avoid this
step and to grant clients GPU access without authenticating, render
nodes were introduced. Render nodes solely serve render clients, that
is, no modesetting or privileged ioctls can be issued on render nodes.
Only non-global rendering commands are allowed. If a driver supports
render nodes, it must advertise it via the DRIVER\_RENDER DRM driver
capability. If not supported, the primary node must be used for render
clients together with the legacy drmAuth authentication procedure.

If a driver advertises render node support, DRM core will create a
separate render node called renderD<num>. There will be one render node
per device. No ioctls except PRIME-related ioctls will be allowed on
this node. Especially GEM\_OPEN will be explicitly prohibited. For a
complete list of driver-independent ioctls that can be used on render
nodes, see the ioctls marked DRM\_RENDER\_ALLOW in drm\_ioctl.c Render
nodes are designed to avoid the buffer-leaks, which occur if clients
guess the flink names or mmap offsets on the legacy interface.
Additionally to this basic interface, drivers must mark their
driver-dependent render-only ioctls as DRM\_RENDER\_ALLOW so render
clients can use them. Driver authors must be careful not to allow any
privileged ioctls on render nodes.

With render nodes, user-space can now control access to the render node
via basic file-system access-modes. A running graphics server which
authenticates clients on the privileged primary/legacy node is no longer
required. Instead, a client can open the render node and is immediately
granted GPU access. Communication between clients (or servers) is done
via PRIME. FLINK from render node to legacy node is not supported. New
clients must not use the insecure FLINK interface.

Besides dropping all modeset/global ioctls, render nodes also drop the
DRM-Master concept. There is no reason to associate render clients with
a DRM-Master as they are independent of any graphics server. Besides,
they must work without any running master, anyway. Drivers must be able
to run without a master object if they support render nodes. If, on the
other hand, a driver requires shared state between clients which is
visible to user-space and accessible beyond open-file boundaries, they
cannot support render nodes.

## Device Hot-Unplug

Note

The following is the plan. Implementation is not there yet
(2020 May).

Graphics devices (display and/or render) may be connected via USB (e.g.
display adapters or docking stations) or Thunderbolt (e.g. eGPU). An end
user is able to hot-unplug this kind of devices while they are being
used, and expects that the very least the machine does not crash. Any
damage from hot-unplugging a DRM device needs to be limited as much as
possible and userspace must be given the chance to handle it if it wants
to. Ideally, unplugging a DRM device still lets a desktop continue to
run, but that is going to need explicit support throughout the whole
graphics stack: from kernel and userspace drivers, through display
servers, via window system protocols, and in applications and libraries.

Other scenarios that should lead to the same are: unrecoverable GPU
crash, PCI device disappearing off the bus, or forced unbind of a driver
from the physical device.

In other words, from userspace perspective everything needs to keep on
working more or less, until userspace stops using the disappeared DRM
device and closes it completely. Userspace will learn of the device
disappearance from the device removed uevent, ioctls returning ENODEV
(or driver-specific ioctls returning driver-specific things), or open()
returning ENXIO.

Only after userspace has closed all relevant DRM device and dmabuf file
descriptors and removed all mmaps, the DRM driver can tear down its
instance for the device that no longer exists. If the same physical
device somehow comes back in the mean time, it shall be a new DRM
device.

Similar to PIDs, chardev minor numbers are not recycled immediately. A
new DRM device always picks the next free minor number compared to the
previous one allocated, and wraps around when minor numbers are
exhausted.

The goal raises at least the following requirements for the kernel and
drivers.

### Requirements for KMS UAPI

* KMS connectors must change their status to disconnected.
* Legacy modesets and pageflips, and atomic commits, both real and
  TEST\_ONLY, and any other ioctls either fail with ENODEV or fake
  success.
* Pending non-blocking KMS operations deliver the DRM events userspace
  is expecting. This applies also to ioctls that faked success.
* open() on a device node whose underlying device has disappeared will
  fail with ENXIO.
* Attempting to create a DRM lease on a disappeared DRM device will
  fail with ENODEV. Existing DRM leases remain and work as listed
  above.

### Requirements for Render and Cross-Device UAPI

* All GPU jobs that can no longer run must have their fences
  force-signalled to avoid inflicting hangs on userspace.
  The associated error code is ENODEV.
* Some userspace APIs already define what should happen when the device
  disappears (OpenGL, GL ES: [GL\_KHR\_robustness](https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_robustness.txt); [Vulkan](https://www.khronos.org/vulkan/):
  VK\_ERROR\_DEVICE\_LOST; etc.). DRM drivers are free to implement this
  behaviour the way they see best, e.g. returning failures in
  driver-specific ioctls and handling those in userspace drivers, or
  rely on uevents, and so on.
* dmabuf which point to memory that has disappeared will either fail to
  import with ENODEV or continue to be successfully imported if it would
  have succeeded before the disappearance. See also about memory maps
  below for already imported dmabufs.
* Attempting to import a dmabuf to a disappeared device will either fail
  with ENODEV or succeed if it would have succeeded without the
  disappearance.
* open() on a device node whose underlying device has disappeared will
  fail with ENXIO.

### Requirements for Memory Maps

Memory maps have further requirements that apply to both existing maps
and maps created after the device has disappeared. If the underlying
memory disappears, the map is created or modified such that reads and
writes will still complete successfully but the result is undefined.
This applies to both userspace mmap()’d memory and memory pointed to by
dmabuf which might be mapped to other devices (cross-device dmabuf
imports).

Raising SIGBUS is not an option, because userspace cannot realistically
handle it. Signal handlers are global, which makes them extremely
difficult to use correctly from libraries like those that Mesa produces.
Signal handlers are not composable, you can’t have different handlers
for GPU1 and GPU2 from different vendors, and a third handler for
mmapped regular files. Threads cause additional pain with signal
handling as well.

## Device reset

The GPU stack is really complex and is prone to errors, from hardware bugs,
faulty applications and everything in between the many layers. Some errors
require resetting the device in order to make the device usable again. This
section describes the expectations for DRM and usermode drivers when a
device resets and how to propagate the reset status.

Device resets can not be disabled without tainting the kernel, which can lead to
hanging the entire kernel through shrinkers/mmu\_notifiers. Userspace role in
device resets is to propagate the message to the application and apply any
special policy for blocking guilty applications, if any. Corollary is that
debugging a hung GPU context require hardware support to be able to preempt such
a GPU context while it’s stopped.

### Kernel Mode Driver

The KMD is responsible for checking if the device needs a reset, and to perform
it as needed. Usually a hang is detected when a job gets stuck executing.

Propagation of errors to userspace has proven to be tricky since it goes in
the opposite direction of the usual flow of commands. Because of this vendor
independent error handling was added to the &dma\_fence object, this way drivers
can add an error code to their fences before signaling them. See function
[`dma_fence_set_error()`](../driver-api/dma-buf.html#c.dma_fence_set_error "dma_fence_set_error") on how to do this and for examples of error codes to use.

The DRM scheduler also allows setting error codes on all pending fences when
hardware submissions are restarted after an reset. Error codes are also
forwarded from the hardware fence to the scheduler fence to bubble up errors
to the higher levels of the stack and eventually userspace.

Fence errors can be queried by userspace through the generic SYNC\_IOC\_FILE\_INFO
IOCTL as well as through driver specific interfaces.

Additional to setting fence errors drivers should also keep track of resets per
context, the DRM scheduler provides the [`drm_sched_entity_error()`](drm-mm.html#c.drm_sched_entity_error "drm_sched_entity_error") function as
helper for this use case. After a reset, KMD should reject new command
submissions for affected contexts.

### User Mode Driver

After command submission, UMD should check if the submission was accepted or
rejected. After a reset, KMD should reject submissions, and UMD can issue an
ioctl to the KMD to check the reset status, and this can be checked more often
if the UMD requires it. After detecting a reset, UMD will then proceed to report
it to the application using the appropriate API error code, as explained in the
section below about robustness.

### Robustness

The only way to try to keep a graphical API context working after a reset is if
it complies with the robustness aspects of the graphical API that it is using.

Graphical APIs provide ways to applications to deal with device resets. However,
there is no guarantee that the app will use such features correctly, and a
userspace that doesn’t support robust interfaces (like a non-robust
OpenGL context or API without any robustness support like libva) leave the
robustness handling entirely to the userspace driver. There is no strong
community consensus on what the userspace driver should do in that case,
since all reasonable approaches have some clear downsides.

#### OpenGL

Apps using OpenGL should use the available robust interfaces, like the
extension `GL_ARB_robustness` (or `GL_EXT_robustness` for OpenGL ES). This
interface tells if a reset has happened, and if so, all the context state is
considered lost and the app proceeds by creating new ones. There’s no consensus
on what to do to if robustness is not in use.

#### Vulkan

Apps using Vulkan should check for `VK_ERROR_DEVICE_LOST` for submissions.
This error code means, among other things, that a device reset has happened and
it needs to recreate the contexts to keep going.

### Reporting causes of resets

Apart from propagating the reset through the stack so apps can recover, it’s
really useful for driver developers to learn more about what caused the reset in
the first place. For this, drivers can make use of devcoredump to store relevant
information about the reset and send device wedged event with `none` recovery
method (as explained in “Device Wedging” chapter) to notify userspace, so this
information can be collected and added to user bug reports.

## Device Wedging

Drivers can optionally make use of device wedged event (implemented as
[`drm_dev_wedged_event()`](drm-internals.html#c.drm_dev_wedged_event "drm_dev_wedged_event") in DRM subsystem), which notifies userspace of ‘wedged’
(hanged/unusable) state of the DRM device through a uevent. This is useful
especially in cases where the device is no longer operating as expected and has
become unrecoverable from driver context. Purpose of this implementation is to
provide drivers a generic way to recover the device with the help of userspace
intervention, without taking any drastic measures (like resetting or
re-enumerating the full bus, on which the underlying physical device is sitting)
in the driver.

A ‘wedged’ device is basically a device that is declared dead by the driver
after exhausting all possible attempts to recover it from driver context. The
uevent is the notification that is sent to userspace along with a hint about
what could possibly be attempted to recover the device from userspace and bring
it back to usable state. Different drivers may have different ideas of a
‘wedged’ device depending on hardware implementation of the underlying physical
device, and hence the vendor agnostic nature of the event. It is up to the
drivers to decide when they see the need for device recovery and how they want
to recover from the available methods.

### Driver prerequisites

The driver, before opting for recovery, needs to make sure that the ‘wedged’
device doesn’t harm the system as a whole by taking care of the prerequisites.
Necessary actions must include disabling DMA to system memory as well as any
communication channels with other devices. Further, the driver must ensure
that all dma\_fences are signalled and any device state that the core kernel
might depend on is cleaned up. All existing mmaps should be invalidated and
page faults should be redirected to a dummy page. Once the event is sent, the
device must be kept in ‘wedged’ state until the recovery is performed. New
accesses to the device (IOCTLs) should be rejected, preferably with an error
code that resembles the type of failure the device has encountered. This will
signify the reason for wedging, which can be reported to the application if
needed.

### Recovery

Current implementation defines four recovery methods, out of which, drivers
can use any one, multiple or none. Method(s) of choice will be sent in the
uevent environment as `WEDGED=<method1>[,..,<methodN>]` in order of less to
more side-effects. See the section [Vendor Specific Recovery](#vendor-specific-recovery)
for `WEDGED=vendor-specific`. If driver is unsure about recovery or
method is unknown, `WEDGED=unknown` will be sent instead.

Userspace consumers can parse this event and attempt recovery as per the
following expectations.

> | Recovery method | Consumer expectations |
> | --- | --- |
> | none | optional telemetry collection |
> | rebind | unbind + bind driver |
> | bus-reset | unbind + bus reset/re-enumeration + bind |
> | vendor-specific | vendor specific recovery method |
> | unknown | consumer policy |

### No Recovery

Here `WEDGED=none` signifies that no recovery is expected from the consumer
but it can still try to gather telemetry information (devcoredump, syslog) for
debug purpose in order to root cause the hang. This is useful because the first
hang is usually the most critical one which can result in consequential hangs
or complete wedging.

### Vendor Specific Recovery

When `WEDGED=vendor-specific` is sent, it indicates that the device requires
a recovery procedure specific to the hardware vendor and is not one of the
standardized approaches.

`WEDGED=vendor-specific` may be used to indicate different cases within a
single vendor driver, each requiring a distinct recovery procedure.
In such scenarios, the vendor driver must provide comprehensive documentation
that describes each case, include additional hints to identify specific case and
outline the corresponding recovery procedure. The documentation includes:

Case - A list of all cases that sends the `WEDGED=vendor-specific` recovery method.

Hints - Additional Information to assist the userspace consumer in identifying and
differentiating between different cases. This can be exposed through sysfs, debugfs,
traces, dmesg etc.

Recovery Procedure - Clear instructions and guidance for recovering each case.
This may include userspace scripts, tools needed for the recovery procedure.

It is the responsibility of the admin/userspace consumer to identify the case and
verify additional identification hints before attempting a recovery procedure.

Example: If the device uses the Xe driver, then userspace consumer should refer to
[Xe Device Wedging](xe/xe_device.html#xe-device-wedging) for the detailed documentation.

### Task information

The information about which application (if any) was involved in the device
wedging is useful for userspace if they want to notify the user about what
happened (e.g. the compositor display a message to the user “The <task name>
caused a graphical error and the system recovered”) or to implement policies
(e.g. the daemon may “ban” an task that keeps resetting the device). If the task
information is available, the uevent will display as `PID=<pid>` and
`TASK=<task name>`. Otherwise, `PID` and `TASK` will not appear in the
event string.

The reliability of this information is driver and hardware specific, and should
be taken with a caution regarding it’s precision. To have a big picture of what
really happened, the devcoredump file provides much more detailed information
about the device state and about the event.

### Consumer prerequisites

It is the responsibility of the consumer to make sure that the device or its
resources are not in use by any process before attempting recovery. With IOCTLs
erroring out, all device memory should be unmapped and file descriptors should
be closed to prevent leaks or undefined behaviour. The idea here is to clear the
device of all user context beforehand and set the stage for a clean recovery.

For `WEDGED=vendor-specific` recovery method, it is the responsibility of the
consumer to check the driver documentation and the usecase before attempting
a recovery.

### Example - rebind

Udev rule:

```
SUBSYSTEM=="drm", ENV{WEDGED}=="rebind", DEVPATH=="*/drm/card[0-9]",
RUN+="/path/to/rebind.sh $env{DEVPATH}"
```

Recovery script:

```
#!/bin/sh

DEVPATH=$(readlink -f /sys/$1/device)
DEVICE=$(basename $DEVPATH)
DRIVER=$(readlink -f $DEVPATH/driver)

echo -n $DEVICE > $DRIVER/unbind
echo -n $DEVICE > $DRIVER/bind
```

### Customization

Although basic recovery is possible with a simple script, consumers can define
custom policies around recovery. For example, if the driver supports multiple
recovery methods, consumers can opt for the suitable one depending on scenarios
like repeat offences or vendor specific failures. Consumers can also choose to
have the device available for debugging or telemetry collection and base their
recovery decision on the findings. This is useful especially when the driver is
unsure about recovery or method is unknown.

## IOCTL Support on Device Nodes

First things first, driver private IOCTLs should only be needed for drivers
supporting rendering. Kernel modesetting is all standardized, and extended
through properties. There are a few exceptions in some existing drivers,
which define IOCTL for use by the display DRM master, but they all predate
properties.

Now if you do have a render driver you always have to support it through
driver private properties. There’s a few steps needed to wire all the things
up.

First you need to define the structure for your IOCTL in your driver private
UAPI header in `include/uapi/drm/my_driver_drm.h`:

```
struct my_driver_operation {
        u32 some_thing;
        u32 another_thing;
};
```

Please make sure that you follow all the best practices from
`Documentation/process/botching-up-ioctls.rst`. Note that [`drm_ioctl()`](#c.drm_ioctl "drm_ioctl")
automatically zero-extends structures, hence make sure you can add more stuff
at the end, i.e. don’t put a variable sized array there.

Then you need to define your IOCTL number, using one of `DRM_IO()`, `DRM_IOR()`,
`DRM_IOW()` or `DRM_IOWR()`. It must start with the DRM\_IOCTL\_ prefix:

```
##define DRM_IOCTL_MY_DRIVER_OPERATION \
    DRM_IOW(DRM_COMMAND_BASE, struct my_driver_operation)
```

DRM driver private IOCTL must be in the range from DRM\_COMMAND\_BASE to
DRM\_COMMAND\_END. Finally you need an array of [`struct drm_ioctl_desc`](#c.drm_ioctl_desc "drm_ioctl_desc") to wire
up the handlers and set the access rights:

```
static const struct drm_ioctl_desc my_driver_ioctls[] = {
    DRM_IOCTL_DEF_DRV(MY_DRIVER_OPERATION, my_driver_operation,
            DRM_AUTH|DRM_RENDER_ALLOW),
};
```

And then assign this to the [`drm_driver.ioctls`](drm-internals.html#c.drm_driver "drm_driver") field in your driver
structure.

See the separate chapter on [file operations](drm-internals.html#drm-driver-fops) for how
the driver-specific IOCTLs are wired up.

### Recommended IOCTL Return Values

In theory a driver’s IOCTL callback is only allowed to return very few error
codes. In practice it’s good to abuse a few more. This section documents common
practice within the DRM subsystem:

ENOENT:
:   Strictly this should only be used when a file doesn’t exist e.g. when
    calling the open() syscall. We reuse that to signal any kind of object
    lookup failure, e.g. for unknown GEM buffer object handles, unknown KMS
    object handles and similar cases.

ENOSPC:
:   Some drivers use this to differentiate “out of kernel memory” from “out
    of VRAM”. Sometimes also applies to other limited gpu resources used for
    rendering (e.g. when you have a special limited compression buffer).
    Sometimes resource allocation/reservation issues in command submission
    IOCTLs are also signalled through EDEADLK.

    Simply running out of kernel/system memory is signalled through ENOMEM.

EPERM/EACCES:
:   Returned for an operation that is valid, but needs more privileges.
    E.g. root-only or much more common, DRM master-only operations return
    this when called by unpriviledged clients. There’s no clear
    difference between EACCES and EPERM.

ENODEV:
:   The device is not present anymore or is not yet fully initialized.

EOPNOTSUPP:
:   Feature (like PRIME, modesetting, GEM) is not supported by the driver.

ENXIO:
:   Remote failure, either a hardware transaction (like i2c), but also used
    when the exporting driver of a shared dma-buf or fence doesn’t support a
    feature needed.

EINTR:
:   DRM drivers assume that userspace restarts all IOCTLs. Any DRM IOCTL can
    return EINTR and in such a case should be restarted with the IOCTL
    parameters left unchanged.

EIO:
:   The GPU died and couldn’t be resurrected through a reset. Modesetting
    hardware failures are signalled through the “link status” connector
    property.

EINVAL:
:   Catch-all for anything that is an invalid argument combination which
    cannot work.

IOCTL also use other error codes like ETIME, EFAULT, EBUSY, ENOTTY but their
usage is in line with the common meanings. The above list tries to just document
DRM specific patterns. Note that ENOTTY has the slightly unintuitive meaning of
“this IOCTL does not exist”, and is used exactly as such in DRM.

drm\_ioctl\_t
:   **Typedef**: DRM ioctl function type.

**Syntax**

> `typedef int drm_ioctl_t (struct drm_device *dev, void *data, struct drm_file *file_priv)`

**Parameters**

`struct drm_device *dev`
:   DRM device inode

`void *data`
:   private pointer of the ioctl call

`struct drm_file *file_priv`
:   DRM file this ioctl was made on

**Description**

This is the DRM ioctl typedef. Note that [`drm_ioctl()`](#c.drm_ioctl "drm_ioctl") has alrady copied **data**
into kernel-space, and will also copy it back, depending upon the read/write
settings in the ioctl command code.

drm\_ioctl\_compat\_t
:   **Typedef**: compatibility DRM ioctl function type.

**Syntax**

> `typedef int drm_ioctl_compat_t (struct file *filp, unsigned int cmd, unsigned long arg)`

**Parameters**

`struct file *filp`
:   file pointer

`unsigned int cmd`
:   ioctl command code

`unsigned long arg`
:   DRM file this ioctl was made on

**Description**

Just a `typedef to` make declaring an array of compatibility handlers easier.
New drivers shouldn’t screw up the structure layout for their ioctl
structures and hence never need this.

enum drm\_ioctl\_flags
:   DRM ioctl flags

**Constants**

`DRM_AUTH`
:   This is for ioctl which are used for rendering, and require that the
    file descriptor is either for a render node, or if it’s a
    legacy/primary node, then it must be authenticated.

`DRM_MASTER`
:   This must be set for any ioctl which can change the modeset or
    display state. Userspace must call the ioctl through a primary node,
    while it is the active master.

    Note that read-only modeset ioctl can also be called by
    unauthenticated clients, or when a master is not the currently active
    one.

`DRM_ROOT_ONLY`
:   Anything that could potentially wreak a master file descriptor needs
    to have this flag set. Current that’s only for the SETMASTER and
    DROPMASTER ioctl, which e.g. logind can call to force a non-behaving
    master (display compositor) into compliance.

    This is equivalent to callers with the SYSADMIN capability.

`DRM_RENDER_ALLOW`
:   This is used for all ioctl needed for rendering only, for drivers
    which support render nodes. This should be all new render drivers,
    and hence it should be always set for any ioctl with DRM\_AUTH set.
    Note though that read-only query ioctl might have this set, but have
    not set DRM\_AUTH because they do not require authentication.

**Description**

Various flags that can be set in [`drm_ioctl_desc.flags`](#c.drm_ioctl_desc "drm_ioctl_desc") to control how
userspace can use a given ioctl.

struct drm\_ioctl\_desc
:   DRM driver ioctl entry

**Definition**:

```
struct drm_ioctl_desc {
    unsigned int cmd;
    enum drm_ioctl_flags flags;
    drm_ioctl_t *func;
    const char *name;
};
```

**Members**

`cmd`
:   ioctl command number, without flags

`flags`
:   a bitmask of [`enum drm_ioctl_flags`](#c.drm_ioctl_flags "drm_ioctl_flags")

`func`
:   handler for this ioctl

`name`
:   user-readable name for debug output

**Description**

For convenience it’s easier to create these using the [`DRM_IOCTL_DEF_DRV()`](#c.DRM_IOCTL_DEF_DRV "DRM_IOCTL_DEF_DRV")
macro.

DRM\_IOCTL\_DEF\_DRV

`DRM_IOCTL_DEF_DRV (ioctl, _func, _flags)`

> helper macro to fill out a [`struct drm_ioctl_desc`](#c.drm_ioctl_desc "drm_ioctl_desc")

**Parameters**

`ioctl`
:   ioctl command suffix

`_func`
:   handler for the ioctl

`_flags`
:   a bitmask of [`enum drm_ioctl_flags`](#c.drm_ioctl_flags "drm_ioctl_flags")

**Description**

Small helper macro to create a [`struct drm_ioctl_desc`](#c.drm_ioctl_desc "drm_ioctl_desc") entry. The ioctl
command number is constructed by prepending `DRM_IOCTL\_` and passing that
to `DRM_IOCTL_NR()`.

int drm\_noop(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv)
:   DRM no-op ioctl implementation

**Parameters**

`struct drm_device *dev`
:   DRM device for the ioctl

`void *data`
:   data pointer for the ioctl

`struct drm_file *file_priv`
:   DRM file for the ioctl call

**Description**

This no-op implementation for drm ioctls is useful for deprecated
functionality where we can’t return a failure code because existing userspace
checks the result of the ioctl, but doesn’t care about the action.

Always returns successfully with 0.

int drm\_invalid\_op(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, void \*data, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv)
:   DRM invalid ioctl implementation

**Parameters**

`struct drm_device *dev`
:   DRM device for the ioctl

`void *data`
:   data pointer for the ioctl

`struct drm_file *file_priv`
:   DRM file for the ioctl call

**Description**

This no-op implementation for drm ioctls is useful for deprecated
functionality where we really don’t want to allow userspace to call the ioctl
any more. This is the case for old ums interfaces for drivers that
transitioned to kms gradually and so kept the old legacy tables around. This
only applies to radeon and i915 kms drivers, other drivers shouldn’t need to
use this function.

Always fails with a return value of -EINVAL.

long drm\_ioctl(struct [file](../filesystems/api-summary.html#c.file "file") \*filp, unsigned int cmd, unsigned long arg)
:   ioctl callback implementation for DRM drivers

**Parameters**

`struct file *filp`
:   file this ioctl is called on

`unsigned int cmd`
:   ioctl cmd number

`unsigned long arg`
:   user argument

**Description**

Looks up the ioctl function in the DRM core and the driver dispatch table,
stored in [`drm_driver.ioctls`](drm-internals.html#c.drm_driver "drm_driver"). It checks for necessary permission by calling
`drm_ioctl_permit()`, and dispatches to the respective function.

**Return**

Zero on success, negative error code on failure.

bool drm\_ioctl\_flags(unsigned int nr, unsigned int \*flags)
:   Check for core ioctl and return ioctl permission flags

**Parameters**

`unsigned int nr`
:   ioctl number

`unsigned int *flags`
:   where to return the ioctl permission flags

**Description**

This ioctl is only used by the vmwgfx driver to augment the access checks
done by the drm core and insofar a pretty decent layering violation. This
shouldn’t be used by any drivers.

**Return**

True if the **nr** corresponds to a DRM core ioctl number, false otherwise.

long drm\_compat\_ioctl(struct [file](../filesystems/api-summary.html#c.file "file") \*filp, unsigned int cmd, unsigned long arg)
:   32bit IOCTL compatibility handler for DRM drivers

**Parameters**

`struct file *filp`
:   file this ioctl is called on

`unsigned int cmd`
:   ioctl cmd number

`unsigned long arg`
:   user argument

**Description**

Compatibility handler for 32 bit userspace running on 64 kernels. All actual
IOCTL handling is forwarded to [`drm_ioctl()`](#c.drm_ioctl "drm_ioctl"), while marshalling structures as
appropriate. Note that this only handles DRM core IOCTLs, if the driver has
botched IOCTL itself, it must handle those by wrapping this function.

**Return**

Zero on success, negative error code on failure.

## Testing and validation

### Testing Requirements for userspace API

New cross-driver userspace interface extensions, like new IOCTL, new KMS
properties, new files in sysfs or anything else that constitutes an API change
should have driver-agnostic testcases in IGT for that feature, if such a test
can be reasonably made using IGT for the target hardware.

### Validating changes with IGT

There’s a collection of tests that aims to cover the whole functionality of
DRM drivers and that can be used to check that changes to DRM drivers or the
core don’t regress existing functionality. This test suite is called IGT and
its code and instructions to build and run can be found in
<https://gitlab.freedesktop.org/drm/igt-gpu-tools/>.

### Using VKMS to test DRM API

VKMS is a software-only model of a KMS driver that is useful for testing
and for running compositors. VKMS aims to enable a virtual display without
the need for a hardware display capability. These characteristics made VKMS
a perfect tool for validating the DRM core behavior and also support the
compositor developer. VKMS makes it possible to test DRM functions in a
virtual machine without display, simplifying the validation of some of the
core changes.

To Validate changes in DRM API with VKMS, start setting the kernel: make
sure to enable VKMS module; compile the kernel with the VKMS enabled and
install it in the target machine. VKMS can be run in a Virtual Machine
(QEMU, virtme or similar). It’s recommended the use of KVM with the minimum
of 1GB of RAM and four cores.

It’s possible to run the IGT-tests in a VM in two ways:

> 1. Use IGT inside a VM
> 2. Use IGT from the host machine and write the results in a shared directory.

Following is an example of using a VM with a shared directory with
the host machine to run igt-tests. This example uses virtme:

```
$ virtme-run --rwdir /path/for/shared_dir --kdir=path/for/kernel/directory --mods=auto
```

Run the igt-tests in the guest machine. This example runs the ‘kms\_flip’
tests:

```
$ /path/for/igt-gpu-tools/scripts/run-tests.sh -p -s -t "kms_flip.*" -v
```

In this example, instead of building the igt\_runner, Piglit is used
(-p option). It creates an HTML summary of the test results and saves
them in the folder “igt-gpu-tools/results”. It executes only the igt-tests
matching the -t option.

### Display CRC Support

DRM device drivers can provide to userspace CRC information of each frame as
it reached a given hardware component (a CRC sampling “source”).

Userspace can control generation of CRCs in a given CRTC by writing to the
file dri/0/crtc-N/crc/control in debugfs, with N being the [index of
the CRTC](#crtc-index). Accepted values are source names (which are
driver-specific) and the “auto” keyword, which will let the driver select a
default source of frame CRCs for this CRTC.

Once frame CRC generation is enabled, userspace can capture them by reading
the dri/0/crtc-N/crc/data file. Each line in that file contains the frame
number in the first field and then a number of unsigned integer fields
containing the CRC data. Fields are separated by a single space and the number
of CRC fields is source-specific.

Note that though in some cases the CRC is computed in a specified way and on
the frame contents as supplied by userspace (eDP 1.3), in general the CRC
computation is performed in an unspecified way and on frame contents that have
been already processed in also an unspecified way and thus userspace cannot
rely on being able to generate matching CRC values for the frame contents that
it submits. In this general case, the maximum userspace can do is to compare
the reported CRCs of frames that should have the same contents.

On the driver side the implementation effort is minimal, drivers only need to
implement [`drm_crtc_funcs.set_crc_source`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") and [`drm_crtc_funcs.verify_crc_source`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs").
The debugfs files are automatically set up if those vfuncs are set. CRC samples
need to be captured in the driver by calling [`drm_crtc_add_crc_entry()`](#c.drm_crtc_add_crc_entry "drm_crtc_add_crc_entry").
Depending on the driver and HW requirements, [`drm_crtc_funcs.set_crc_source`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs")
may result in a commit (even a full modeset).

CRC results must be reliable across non-full-modeset atomic commits, so if a
commit via DRM\_IOCTL\_MODE\_ATOMIC would disable or otherwise interfere with
CRC generation, then the driver must mark that commit as a full modeset
([`drm_atomic_crtc_needs_modeset()`](drm-kms.html#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset") should return true). As a result, to ensure
consistent results, generic userspace must re-setup CRC generation after a
legacy SETCRTC or an atomic commit with DRM\_MODE\_ATOMIC\_ALLOW\_MODESET.

int drm\_crtc\_add\_crc\_entry(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, bool has\_frame, uint32\_t frame, uint32\_t \*crcs)
:   Add entry with CRC information for a frame

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to which the frame belongs

`bool has_frame`
:   whether this entry has a frame number to go with

`uint32_t frame`
:   number of the frame these CRCs are about

`uint32_t *crcs`
:   array of CRC values, with length matching #drm\_crtc\_crc.values\_cnt

**Description**

For each frame, the driver polls the source of CRCs for new data and calls
this function to add them to the buffer from where userspace reads.

### Debugfs Support

DRM\_DEBUGFS\_GPUVA\_INFO

`DRM_DEBUGFS_GPUVA_INFO (show, data)`

> [`drm_info_list`](#c.drm_info_list "drm_info_list") entry to dump a GPU VA space

**Parameters**

`show`
:   the [`drm_info_list`](#c.drm_info_list "drm_info_list")’s show callback

`data`
:   driver private data

**Description**

Drivers should use this macro to define a [`drm_info_list`](#c.drm_info_list "drm_info_list") entry to provide a
debugfs file for dumping the GPU VA space regions and mappings.

For each DRM GPU VA space drivers should call [`drm_debugfs_gpuva_info()`](#c.drm_debugfs_gpuva_info "drm_debugfs_gpuva_info") from
their **show** callback.

struct drm\_info\_list
:   debugfs info list entry

**Definition**:

```
struct drm_info_list {
    const char *name;
    int (*show)(struct seq_file*, void*);
    u32 driver_features;
    void *data;
};
```

**Members**

`name`
:   file name

`show`
:   Show callback. `seq_file->private` will be set to the [`struct
    drm_info_node`](#c.drm_info_node "drm_info_node") corresponding to the instance of this info on a given
    [`struct drm_minor`](drm-internals.html#c.drm_minor "drm_minor").

`driver_features`
:   Required driver features for this entry

`data`
:   Driver-private data, should not be device-specific.

**Description**

This structure represents a debugfs file to be created by the drm
core.

struct drm\_info\_node
:   Per-minor debugfs node structure

**Definition**:

```
struct drm_info_node {
    struct drm_minor *minor;
    const struct drm_info_list *info_ent;
};
```

**Members**

`minor`
:   [`struct drm_minor`](drm-internals.html#c.drm_minor "drm_minor") for this node.

`info_ent`
:   template for this node.

**Description**

This structure represents a debugfs file, as an instantiation of a [`struct
drm_info_list`](#c.drm_info_list "drm_info_list") on a [`struct drm_minor`](drm-internals.html#c.drm_minor "drm_minor").

FIXME:

No it doesn’t make a hole lot of sense that we duplicate debugfs entries for
both the render and the primary nodes, but that’s how this has organically
grown. It should probably be fixed, with a compatibility link, if needed.

struct drm\_debugfs\_info
:   debugfs info list entry

**Definition**:

```
struct drm_debugfs_info {
    const char *name;
    int (*show)(struct seq_file*, void*);
    u32 driver_features;
    void *data;
};
```

**Members**

`name`
:   File name

`show`
:   Show callback. `seq_file->private` will be set to the [`struct
    drm_debugfs_entry`](#c.drm_debugfs_entry "drm_debugfs_entry") corresponding to the instance of this info
    on a given [`struct drm_device`](drm-internals.html#c.drm_device "drm_device").

`driver_features`
:   Required driver features for this entry.

`data`
:   Driver-private data, should not be device-specific.

**Description**

This structure represents a debugfs file to be created by the drm
core.

struct drm\_debugfs\_entry
:   Per-device debugfs node structure

**Definition**:

```
struct drm_debugfs_entry {
    struct drm_device *dev;
    struct drm_debugfs_info file;
    struct list_head list;
};
```

**Members**

`dev`
:   [`struct drm_device`](drm-internals.html#c.drm_device "drm_device") for this node.

`file`
:   Template for this node.

`list`
:   Linked list of all device nodes.

**Description**

This structure represents a debugfs file, as an instantiation of a [`struct
drm_debugfs_info`](#c.drm_debugfs_info "drm_debugfs_info") on a [`struct drm_device`](drm-internals.html#c.drm_device "drm_device").

int drm\_debugfs\_gpuva\_info(struct seq\_file \*m, struct [drm\_gpuvm](drm-mm.html#c.drm_gpuvm "drm_gpuvm") \*gpuvm)
:   dump the given DRM GPU VA space

**Parameters**

`struct seq_file *m`
:   pointer to the `seq_file` to write

`struct drm_gpuvm *gpuvm`
:   the [`drm_gpuvm`](drm-mm.html#c.drm_gpuvm "drm_gpuvm") representing the GPU VA space

**Description**

Dumps the GPU VA mappings of a given DRM GPU VA manager.

For each DRM GPU VA space drivers should call this function from their
[`drm_info_list`](#c.drm_info_list "drm_info_list")’s show callback.

**Return**

0 on success, -ENODEV if the `gpuvm` is not initialized

void drm\_debugfs\_create\_files(const struct [drm\_info\_list](#c.drm_info_list "drm_info_list") \*files, int count, struct dentry \*root, struct [drm\_minor](drm-internals.html#c.drm_minor "drm_minor") \*minor)
:   Initialize a given set of debugfs files for DRM minor

**Parameters**

`const struct drm_info_list *files`
:   The array of files to create

`int count`
:   The number of files given

`struct dentry *root`
:   DRI debugfs dir entry.

`struct drm_minor *minor`
:   device minor number

**Description**

Create a given set of debugfs files represented by an array of
[`struct drm_info_list`](#c.drm_info_list "drm_info_list") in the given root directory. These files will be removed
automatically on `drm_debugfs_dev_fini()`.

void drm\_debugfs\_add\_file(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const char \*name, int (\*show)(struct seq\_file\*, void\*), void \*data)
:   Add a given file to the DRM device debugfs file list

**Parameters**

`struct drm_device *dev`
:   drm device for the ioctl

`const char *name`
:   debugfs file name

`int (*show)(struct seq_file*, void*)`
:   show callback

`void *data`
:   driver-private data, should not be device-specific

**Description**

Add a given file entry to the DRM device debugfs file list to be created on
drm\_debugfs\_init.

void drm\_debugfs\_add\_files(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const struct [drm\_debugfs\_info](#c.drm_debugfs_info "drm_debugfs_info") \*files, int count)
:   Add an array of files to the DRM device debugfs file list

**Parameters**

`struct drm_device *dev`
:   drm device for the ioctl

`const struct drm_debugfs_info *files`
:   The array of files to create

`int count`
:   The number of files given

**Description**

Add a given set of debugfs files represented by an array of
[`struct drm_debugfs_info`](#c.drm_debugfs_info "drm_debugfs_info") in the DRM device debugfs file list.

## Sysfs Support

DRM provides very little additional support to drivers for sysfs
interactions, beyond just all the standard stuff. Drivers who want to expose
additional sysfs properties and property groups can attach them at either
[`drm_device.dev`](drm-internals.html#c.drm_device "drm_device") or [`drm_connector.kdev`](drm-kms.html#c.drm_connector "drm_connector").

Registration is automatically handled when calling [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register"), or
[`drm_connector_register()`](drm-kms.html#c.drm_connector_register "drm_connector_register") in case of hot-plugged connectors. Unregistration is
also automatically handled by [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister") and
[`drm_connector_unregister()`](drm-kms.html#c.drm_connector_unregister "drm_connector_unregister").

void drm\_sysfs\_hotplug\_event(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   generate a DRM uevent

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Send a uevent for the DRM device specified by **dev**. Currently we only
set HOTPLUG=1 in the uevent environment, but this could be expanded to
deal with other types of events.

Any new uapi should be using the `drm_sysfs_connector_status_event()`
for uevents on connector status change.

void drm\_sysfs\_connector\_hotplug\_event(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   generate a DRM uevent for any connector change

**Parameters**

`struct drm_connector *connector`
:   connector which has changed

**Description**

Send a uevent for the DRM connector specified by **connector**. This will send
a uevent with the properties HOTPLUG=1 and CONNECTOR.

void drm\_sysfs\_connector\_property\_event(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_property](drm-kms.html#c.drm_property "drm_property") \*property)
:   generate a DRM uevent for connector property change

**Parameters**

`struct drm_connector *connector`
:   connector on which property changed

`struct drm_property *property`
:   connector property which has changed.

**Description**

Send a uevent for the specified DRM connector and property. Currently we
set HOTPLUG=1 and connector id along with the attached property id
related to the change.

int drm\_class\_device\_register(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   register new device with the DRM sysfs class

**Parameters**

`struct device *dev`
:   device to register

**Description**

Registers a new [`struct device`](../driver-api/infrastructure.html#c.device "device") within the DRM sysfs class. Essentially only
used by ttm to have a place for its global settings. Drivers should never use
this.

void drm\_class\_device\_unregister(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   unregister device with the DRM sysfs class

**Parameters**

`struct device *dev`
:   device to unregister

**Description**

Unregisters a [`struct device`](../driver-api/infrastructure.html#c.device "device") from the DRM sysfs class. Essentially only used
by ttm to have a place for its global settings. Drivers should never use
this.

## VBlank event handling

The DRM core exposes two vertical blank related ioctls:

`DRM_IOCTL_WAIT_VBLANK`
:   This takes a `struct drm_wait_vblank` structure as its argument, and
    it is used to block or request a signal when a specified vblank
    event occurs.

`DRM_IOCTL_MODESET_CTL`
:   This was only used for user-mode-settind drivers around modesetting
    changes to allow the kernel to update the vblank interrupt after
    mode setting, since on many devices the vertical blank counter is
    reset to 0 at some point during modeset. Modern drivers should not
    call this any more since with kernel mode setting it is a no-op.

## Userspace API Structures

DRM exposes many UAPI and structure definitions to have a consistent
and standardized interface with users.
Userspace can refer to these structure definitions and UAPI formats
to communicate to drivers.

### CRTC index

CRTC’s have both an object ID and an index, and they are not the same thing.
The index is used in cases where a densely packed identifier for a CRTC is
needed, for instance a bitmask of CRTC’s. The member possible\_crtcs of [`struct
drm_mode_get_plane`](#c.drm_mode_get_plane "drm_mode_get_plane") is an example.

`DRM_IOCTL_MODE_GETRESOURCES` populates a structure with an array of
CRTC ID’s, and the CRTC index is its position in this array.

struct drm\_gem\_close
:   Argument for [`DRM_IOCTL_GEM_CLOSE`](#c.DRM_IOCTL_GEM_CLOSE "DRM_IOCTL_GEM_CLOSE") ioctl.

**Definition**:

```
struct drm_gem_close {
    __u32 handle;
    __u32 pad;
};
```

**Members**

`handle`
:   Handle of the object to be closed.

`pad`
:   Padding.

**Description**

Releases the handle to an mm object.

struct drm\_gem\_flink
:   Argument for `DRM_IOCTL_GEM_FLINK` ioctl.

**Definition**:

```
struct drm_gem_flink {
    __u32 handle;
    __u32 name;
};
```

**Members**

`handle`
:   Handle for the object being named.

`name`
:   Returned global name.

**Description**

Create a global name for an object, returning the name.

Note that the name does not hold a reference; when the object
is freed, the name goes away.

struct drm\_gem\_open
:   Argument for `DRM_IOCTL_GEM_OPEN` ioctl.

**Definition**:

```
struct drm_gem_open {
    __u32 name;
    __u32 handle;
    __u64 size;
};
```

**Members**

`name`
:   Name of object being opened.

`handle`
:   Returned handle for the object.

`size`
:   Returned size of the object

**Description**

Open an object using the global name, returning a handle and the size.

This handle (of course) holds a reference to the object, so the object
will not go away until the handle is deleted.

struct drm\_gem\_change\_handle
:   Argument for [`DRM_IOCTL_GEM_CHANGE_HANDLE`](#c.DRM_IOCTL_GEM_CHANGE_HANDLE "DRM_IOCTL_GEM_CHANGE_HANDLE") ioctl.

**Definition**:

```
struct drm_gem_change_handle {
    __u32 handle;
    __u32 new_handle;
};
```

**Members**

`handle`
:   The handle of a gem object.

`new_handle`
:   An available gem handle.

**Description**

This ioctl changes the handle of a GEM object to the specified one.
The new handle must be unused. On success the old handle is closed
and all further IOCTL should refer to the new handle only.
Calls to DRM\_IOCTL\_PRIME\_FD\_TO\_HANDLE will return the new handle.

DRM\_CAP\_DUMB\_BUFFER

`DRM_CAP_DUMB_BUFFER`

> **Description**
>
> If set to 1, the driver supports creating dumb buffers via the
> [`DRM_IOCTL_MODE_CREATE_DUMB`](#c.DRM_IOCTL_MODE_CREATE_DUMB "DRM_IOCTL_MODE_CREATE_DUMB") ioctl.

DRM\_CAP\_VBLANK\_HIGH\_CRTC

`DRM_CAP_VBLANK_HIGH_CRTC`

> **Description**
>
> If set to 1, the kernel supports specifying a [CRTC index](#crtc-index)
> in the high bits of `drm_wait_vblank_request.type`.
>
> Starting kernel version 2.6.39, this capability is always set to 1.

DRM\_CAP\_DUMB\_PREFERRED\_DEPTH

`DRM_CAP_DUMB_PREFERRED_DEPTH`

> **Description**
>
> The preferred bit depth for dumb buffers.
>
> The bit depth is the number of bits used to indicate the color of a single
> pixel excluding any padding. This is different from the number of bits per
> pixel. For instance, XRGB8888 has a bit depth of 24 but has 32 bits per
> pixel.
>
> Note that this preference only applies to dumb buffers, it’s irrelevant for
> other types of buffers.

DRM\_CAP\_DUMB\_PREFER\_SHADOW

`DRM_CAP_DUMB_PREFER_SHADOW`

> **Description**
>
> If set to 1, the driver prefers userspace to render to a shadow buffer
> instead of directly rendering to a dumb buffer. For best speed, userspace
> should do streaming ordered memory copies into the dumb buffer and never
> read from it.
>
> Note that this preference only applies to dumb buffers, it’s irrelevant for
> other types of buffers.

DRM\_CAP\_PRIME

`DRM_CAP_PRIME`

> **Description**
>
> Bitfield of supported PRIME sharing capabilities. See [`DRM_PRIME_CAP_IMPORT`](#c.DRM_PRIME_CAP_IMPORT "DRM_PRIME_CAP_IMPORT")
> and [`DRM_PRIME_CAP_EXPORT`](#c.DRM_PRIME_CAP_EXPORT "DRM_PRIME_CAP_EXPORT").
>
> Starting from kernel version 6.6, both [`DRM_PRIME_CAP_IMPORT`](#c.DRM_PRIME_CAP_IMPORT "DRM_PRIME_CAP_IMPORT") and
> [`DRM_PRIME_CAP_EXPORT`](#c.DRM_PRIME_CAP_EXPORT "DRM_PRIME_CAP_EXPORT") are always advertised.
>
> PRIME buffers are exposed as dma-buf file descriptors.
> See [PRIME Buffer Sharing](drm-mm.html#prime-buffer-sharing).

DRM\_PRIME\_CAP\_IMPORT

`DRM_PRIME_CAP_IMPORT`

> **Description**
>
> If this bit is set in [`DRM_CAP_PRIME`](#c.DRM_CAP_PRIME "DRM_CAP_PRIME"), the driver supports importing PRIME
> buffers via the [`DRM_IOCTL_PRIME_FD_TO_HANDLE`](#c.DRM_IOCTL_PRIME_FD_TO_HANDLE "DRM_IOCTL_PRIME_FD_TO_HANDLE") ioctl.
>
> Starting from kernel version 6.6, this bit is always set in [`DRM_CAP_PRIME`](#c.DRM_CAP_PRIME "DRM_CAP_PRIME").

DRM\_PRIME\_CAP\_EXPORT

`DRM_PRIME_CAP_EXPORT`

> **Description**
>
> If this bit is set in [`DRM_CAP_PRIME`](#c.DRM_CAP_PRIME "DRM_CAP_PRIME"), the driver supports exporting PRIME
> buffers via the [`DRM_IOCTL_PRIME_HANDLE_TO_FD`](#c.DRM_IOCTL_PRIME_HANDLE_TO_FD "DRM_IOCTL_PRIME_HANDLE_TO_FD") ioctl.
>
> Starting from kernel version 6.6, this bit is always set in [`DRM_CAP_PRIME`](#c.DRM_CAP_PRIME "DRM_CAP_PRIME").

DRM\_CAP\_TIMESTAMP\_MONOTONIC

`DRM_CAP_TIMESTAMP_MONOTONIC`

> **Description**
>
> If set to 0, the kernel will report timestamps with `CLOCK_REALTIME` in
> `struct drm_event_vblank`. If set to 1, the kernel will report timestamps with
> `CLOCK_MONOTONIC`. See `clock_gettime(2)` for the definition of these
> clocks.
>
> Starting from kernel version 2.6.39, the default value for this capability
> is 1. Starting kernel version 4.15, this capability is always set to 1.

DRM\_CAP\_ASYNC\_PAGE\_FLIP

`DRM_CAP_ASYNC_PAGE_FLIP`

> **Description**
>
> If set to 1, the driver supports [`DRM_MODE_PAGE_FLIP_ASYNC`](#c.DRM_MODE_PAGE_FLIP_ASYNC "DRM_MODE_PAGE_FLIP_ASYNC") for legacy
> page-flips.

DRM\_CAP\_CURSOR\_WIDTH

`DRM_CAP_CURSOR_WIDTH`

> **Description**
>
> The `CURSOR_WIDTH` and `CURSOR_HEIGHT` capabilities return a valid
> width x height combination for the hardware cursor. The intention is that a
> hardware agnostic userspace can query a cursor plane size to use.
>
> Note that the cross-driver contract is to merely return a valid size;
> drivers are free to attach another meaning on top, eg. i915 returns the
> maximum plane size.

DRM\_CAP\_CURSOR\_HEIGHT

`DRM_CAP_CURSOR_HEIGHT`

> **Description**
>
> See [`DRM_CAP_CURSOR_WIDTH`](#c.DRM_CAP_CURSOR_WIDTH "DRM_CAP_CURSOR_WIDTH").

DRM\_CAP\_ADDFB2\_MODIFIERS

`DRM_CAP_ADDFB2_MODIFIERS`

> **Description**
>
> If set to 1, the driver supports supplying modifiers in the
> `DRM_IOCTL_MODE_ADDFB2` ioctl.

DRM\_CAP\_PAGE\_FLIP\_TARGET

`DRM_CAP_PAGE_FLIP_TARGET`

> **Description**
>
> If set to 1, the driver supports the `DRM_MODE_PAGE_FLIP_TARGET_ABSOLUTE` and
> `DRM_MODE_PAGE_FLIP_TARGET_RELATIVE` flags in
> `drm_mode_crtc_page_flip_target.flags` for the `DRM_IOCTL_MODE_PAGE_FLIP`
> ioctl.

DRM\_CAP\_CRTC\_IN\_VBLANK\_EVENT

`DRM_CAP_CRTC_IN_VBLANK_EVENT`

> **Description**
>
> If set to 1, the kernel supports reporting the CRTC ID in
> `drm_event_vblank.crtc_id` for the [`DRM_EVENT_VBLANK`](#c.DRM_EVENT_VBLANK "DRM_EVENT_VBLANK") and
> [`DRM_EVENT_FLIP_COMPLETE`](#c.DRM_EVENT_FLIP_COMPLETE "DRM_EVENT_FLIP_COMPLETE") events.
>
> Starting kernel version 4.12, this capability is always set to 1.

DRM\_CAP\_SYNCOBJ

`DRM_CAP_SYNCOBJ`

> **Description**
>
> If set to 1, the driver supports sync objects. See [DRM Sync Objects](drm-mm.html#drm-sync-objects).

DRM\_CAP\_SYNCOBJ\_TIMELINE

`DRM_CAP_SYNCOBJ_TIMELINE`

> **Description**
>
> If set to 1, the driver supports timeline operations on sync objects. See
> [DRM Sync Objects](drm-mm.html#drm-sync-objects).

DRM\_CAP\_ATOMIC\_ASYNC\_PAGE\_FLIP

`DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP`

> **Description**
>
> If set to 1, the driver supports [`DRM_MODE_PAGE_FLIP_ASYNC`](#c.DRM_MODE_PAGE_FLIP_ASYNC "DRM_MODE_PAGE_FLIP_ASYNC") for atomic
> commits.

DRM\_CLIENT\_CAP\_STEREO\_3D

`DRM_CLIENT_CAP_STEREO_3D`

> **Description**
>
> If set to 1, the DRM core will expose the stereo 3D capabilities of the
> monitor by advertising the supported 3D layouts in the flags of [`struct
> drm_mode_modeinfo`](#c.drm_mode_modeinfo "drm_mode_modeinfo"). See `DRM_MODE_FLAG_3D_*`.
>
> This capability is always supported for all drivers starting from kernel
> version 3.13.

DRM\_CLIENT\_CAP\_UNIVERSAL\_PLANES

`DRM_CLIENT_CAP_UNIVERSAL_PLANES`

> **Description**
>
> If set to 1, the DRM core will expose all planes (overlay, primary, and
> cursor) to userspace.
>
> This capability has been introduced in kernel version 3.15. Starting from
> kernel version 3.17, this capability is always supported for all drivers.

DRM\_CLIENT\_CAP\_ATOMIC

`DRM_CLIENT_CAP_ATOMIC`

> **Description**
>
> If set to 1, the DRM core will expose atomic properties to userspace. This
> implicitly enables [`DRM_CLIENT_CAP_UNIVERSAL_PLANES`](#c.DRM_CLIENT_CAP_UNIVERSAL_PLANES "DRM_CLIENT_CAP_UNIVERSAL_PLANES") and
> [`DRM_CLIENT_CAP_ASPECT_RATIO`](#c.DRM_CLIENT_CAP_ASPECT_RATIO "DRM_CLIENT_CAP_ASPECT_RATIO").
>
> If the driver doesn’t support atomic mode-setting, enabling this capability
> will fail with -EOPNOTSUPP.
>
> This capability has been introduced in kernel version 4.0. Starting from
> kernel version 4.2, this capability is always supported for atomic-capable
> drivers.

DRM\_CLIENT\_CAP\_ASPECT\_RATIO

`DRM_CLIENT_CAP_ASPECT_RATIO`

> **Description**
>
> If set to 1, the DRM core will provide aspect ratio information in modes.
> See `DRM_MODE_FLAG_PIC_AR_*`.
>
> This capability is always supported for all drivers starting from kernel
> version 4.18.

DRM\_CLIENT\_CAP\_WRITEBACK\_CONNECTORS

`DRM_CLIENT_CAP_WRITEBACK_CONNECTORS`

> **Description**
>
> If set to 1, the DRM core will expose special connectors to be used for
> writing back to memory the scene setup in the commit. The client must enable
> [`DRM_CLIENT_CAP_ATOMIC`](#c.DRM_CLIENT_CAP_ATOMIC "DRM_CLIENT_CAP_ATOMIC") first.
>
> This capability is always supported for atomic-capable drivers starting from
> kernel version 4.19.

DRM\_CLIENT\_CAP\_CURSOR\_PLANE\_HOTSPOT

`DRM_CLIENT_CAP_CURSOR_PLANE_HOTSPOT`

> **Description**
>
> Drivers for para-virtualized hardware (e.g. vmwgfx, qxl, virtio and
> virtualbox) have additional restrictions for cursor planes (thus
> making cursor planes on those drivers not truly universal,) e.g.
> they need cursor planes to act like one would expect from a mouse
> cursor and have correctly set hotspot properties.
> If this client cap is not set the DRM core will hide cursor plane on
> those virtualized drivers because not setting it implies that the
> client is not capable of dealing with those extra restictions.
> Clients which do set cursor hotspot and treat the cursor plane
> like a mouse cursor should set this property.
> The client must enable [`DRM_CLIENT_CAP_ATOMIC`](#c.DRM_CLIENT_CAP_ATOMIC "DRM_CLIENT_CAP_ATOMIC") first.
>
> Setting this property on drivers which do not special case
> cursor planes (i.e. non-virtualized drivers) will return
> EOPNOTSUPP, which can be used by userspace to gauge
> requirements of the hardware/drivers they’re running on.
>
> This capability is always supported for atomic-capable virtualized
> drivers starting from kernel version 6.6.

DRM\_CLIENT\_CAP\_PLANE\_COLOR\_PIPELINE

`DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE`

> **Description**
>
> If set to 1 the DRM core will allow setting the COLOR\_PIPELINE
> property on a [`drm_plane`](drm-kms.html#c.drm_plane "drm_plane"), as well as drm\_colorop properties.
>
> Setting of these plane properties will be rejected when this client
> cap is set:
> - COLOR\_ENCODING
> - COLOR\_RANGE
>
> The client must enable [`DRM_CLIENT_CAP_ATOMIC`](#c.DRM_CLIENT_CAP_ATOMIC "DRM_CLIENT_CAP_ATOMIC") first.

struct drm\_syncobj\_eventfd

**Definition**:

```
struct drm_syncobj_eventfd {
    __u32 handle;
    __u32 flags;
    __u64 point;
    __s32 fd;
    __u32 pad;
};
```

**Members**

`handle`
:   syncobj handle.

`flags`
:   Zero to wait for the point to be signalled, or
    `DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE` to wait for a fence to be
    available for the point.

`point`
:   syncobj timeline point (set to zero for binary syncobjs).

`fd`
:   Existing eventfd to sent events to.

`pad`
:   Must be zero.

**Description**

Register an eventfd to be signalled by a syncobj. The eventfd counter will
be incremented by one.

DRM\_IOCTL\_GEM\_CLOSE

`DRM_IOCTL_GEM_CLOSE`

> > Close a GEM handle.
>
> **Description**
>
> GEM handles are not reference-counted by the kernel. User-space is
> responsible for managing their lifetime. For example, if user-space imports
> the same memory object twice on the same DRM file description, the same GEM
> handle is returned by both imports, and user-space needs to ensure
> [`DRM_IOCTL_GEM_CLOSE`](#c.DRM_IOCTL_GEM_CLOSE "DRM_IOCTL_GEM_CLOSE") is performed once only. The same situation can happen
> when a memory object is allocated, then exported and imported again on the
> same DRM file description. The [`DRM_IOCTL_MODE_GETFB2`](#c.DRM_IOCTL_MODE_GETFB2 "DRM_IOCTL_MODE_GETFB2") IOCTL is an exception
> and always returns fresh new GEM handles even if an existing GEM handle
> already refers to the same memory object before the IOCTL is performed.

DRM\_IOCTL\_PRIME\_HANDLE\_TO\_FD

`DRM_IOCTL_PRIME_HANDLE_TO_FD`

> > Convert a GEM handle to a DMA-BUF FD.
>
> **Description**
>
> User-space sets `drm_prime_handle.handle` with the GEM handle to export and
> `drm_prime_handle.flags`, and gets back a DMA-BUF file descriptor in
> `drm_prime_handle.fd`.
>
> The export can fail for any driver-specific reason, e.g. because export is
> not supported for this specific GEM handle (but might be for others).
>
> Support for exporting DMA-BUFs is advertised via [`DRM_PRIME_CAP_EXPORT`](#c.DRM_PRIME_CAP_EXPORT "DRM_PRIME_CAP_EXPORT").

DRM\_IOCTL\_PRIME\_FD\_TO\_HANDLE

`DRM_IOCTL_PRIME_FD_TO_HANDLE`

> > Convert a DMA-BUF FD to a GEM handle.
>
> **Description**
>
> User-space sets `drm_prime_handle.fd` with a DMA-BUF file descriptor to
> import, and gets back a GEM handle in `drm_prime_handle.handle`.
> `drm_prime_handle.flags` is unused.
>
> If an existing GEM handle refers to the memory object backing the DMA-BUF,
> that GEM handle is returned. Therefore user-space which needs to handle
> arbitrary DMA-BUFs must have a user-space lookup data structure to manually
> reference-count duplicated GEM handles. For more information see
> [`DRM_IOCTL_GEM_CLOSE`](#c.DRM_IOCTL_GEM_CLOSE "DRM_IOCTL_GEM_CLOSE").
>
> The import can fail for any driver-specific reason, e.g. because import is
> only supported for DMA-BUFs allocated on this DRM device.
>
> Support for importing DMA-BUFs is advertised via [`DRM_PRIME_CAP_IMPORT`](#c.DRM_PRIME_CAP_IMPORT "DRM_PRIME_CAP_IMPORT").

DRM\_IOCTL\_MODE\_RMFB

`DRM_IOCTL_MODE_RMFB`

> > Remove a framebuffer.
>
> **Description**
>
> This removes a framebuffer previously added via ADDFB/ADDFB2. The IOCTL
> argument is a framebuffer object ID.
>
> Warning: removing a framebuffer currently in-use on an enabled plane will
> disable that plane. The CRTC the plane is linked to may also be disabled
> (depending on driver capabilities).

DRM\_IOCTL\_MODE\_CREATE\_DUMB

`DRM_IOCTL_MODE_CREATE_DUMB`

> > Create a new dumb buffer object.
>
> **Description**
>
> KMS dumb buffers provide a very primitive way to allocate a buffer object
> suitable for scanout and map it for software rendering. KMS dumb buffers are
> not suitable for hardware-accelerated rendering nor video decoding. KMS dumb
> buffers are not suitable to be displayed on any other device than the KMS
> device where they were allocated from. Also see
> [Dumb Buffer Objects](drm-kms.html#kms-dumb-buffer-objects).
>
> The IOCTL argument is a [`struct drm_mode_create_dumb`](#c.drm_mode_create_dumb "drm_mode_create_dumb").
>
> User-space is expected to create a KMS dumb buffer via this IOCTL, then add
> it as a KMS framebuffer via `DRM_IOCTL_MODE_ADDFB` and map it via
> `DRM_IOCTL_MODE_MAP_DUMB`.
>
> [`DRM_CAP_DUMB_BUFFER`](#c.DRM_CAP_DUMB_BUFFER "DRM_CAP_DUMB_BUFFER") indicates whether this IOCTL is supported.
> [`DRM_CAP_DUMB_PREFERRED_DEPTH`](#c.DRM_CAP_DUMB_PREFERRED_DEPTH "DRM_CAP_DUMB_PREFERRED_DEPTH") and [`DRM_CAP_DUMB_PREFER_SHADOW`](#c.DRM_CAP_DUMB_PREFER_SHADOW "DRM_CAP_DUMB_PREFER_SHADOW") indicate
> driver preferences for dumb buffers.

DRM\_IOCTL\_MODE\_GETFB2

`DRM_IOCTL_MODE_GETFB2`

> > Get framebuffer metadata.
>
> **Description**
>
> This queries metadata about a framebuffer. User-space fills
> [`drm_mode_fb_cmd2.fb_id`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") as the input, and the kernels fills the rest of the
> `struct as` the output.
>
> If the client is DRM master or has `CAP_SYS_ADMIN`, [`drm_mode_fb_cmd2.handles`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2")
> will be filled with GEM buffer handles. Fresh new GEM handles are always
> returned, even if another GEM handle referring to the same memory object
> already exists on the DRM file description. The caller is responsible for
> removing the new handles, e.g. via the [`DRM_IOCTL_GEM_CLOSE`](#c.DRM_IOCTL_GEM_CLOSE "DRM_IOCTL_GEM_CLOSE") IOCTL. The same
> new handle will be returned for multiple planes in case they use the same
> memory object. Planes are valid until one has a zero handle -- this can be
> used to compute the number of planes.
>
> Otherwise, [`drm_mode_fb_cmd2.handles`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") will be zeroed and planes are valid
> until one has a zero [`drm_mode_fb_cmd2.pitches`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2").
>
> If the framebuffer has a format modifier, `DRM_MODE_FB_MODIFIERS` will be set
> in [`drm_mode_fb_cmd2.flags`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") and [`drm_mode_fb_cmd2.modifier`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") will contain the
> modifier. Otherwise, user-space must ignore [`drm_mode_fb_cmd2.modifier`](#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2").
>
> To obtain DMA-BUF FDs for each plane without leaking GEM handles, user-space
> can export each handle via [`DRM_IOCTL_PRIME_HANDLE_TO_FD`](#c.DRM_IOCTL_PRIME_HANDLE_TO_FD "DRM_IOCTL_PRIME_HANDLE_TO_FD"), then immediately
> close each unique handle via [`DRM_IOCTL_GEM_CLOSE`](#c.DRM_IOCTL_GEM_CLOSE "DRM_IOCTL_GEM_CLOSE"), making sure to not
> double-close handles which are specified multiple times in the array.

DRM\_IOCTL\_MODE\_CLOSEFB

`DRM_IOCTL_MODE_CLOSEFB`

> > Close a framebuffer.
>
> **Description**
>
> This closes a framebuffer previously added via ADDFB/ADDFB2. The IOCTL
> argument is a framebuffer object ID.
>
> This IOCTL is similar to [`DRM_IOCTL_MODE_RMFB`](#c.DRM_IOCTL_MODE_RMFB "DRM_IOCTL_MODE_RMFB"), except it doesn’t disable
> planes and CRTCs. As long as the framebuffer is used by a plane, it’s kept
> alive. When the plane no longer uses the framebuffer (because the
> framebuffer is replaced with another one, or the plane is disabled), the
> framebuffer is cleaned up.
>
> This is useful to implement flicker-free transitions between two processes.
>
> Depending on the threat model, user-space may want to ensure that the
> framebuffer doesn’t expose any sensitive user information: closed
> framebuffers attached to a plane can be read back by the next DRM master.

DRM\_IOCTL\_SET\_CLIENT\_NAME

`DRM_IOCTL_SET_CLIENT_NAME`

> > Attach a name to a drm\_file
>
> **Description**
>
> Having a name allows for easier tracking and debugging.
> The length of the name (without null ending char) must be
> <= DRM\_CLIENT\_NAME\_MAX\_LEN.
> The call will fail if the name contains whitespaces or non-printable chars.

DRM\_IOCTL\_GEM\_CHANGE\_HANDLE

`DRM_IOCTL_GEM_CHANGE_HANDLE`

> > Move an object to a different handle
>
> **Description**
>
> Some applications (notably CRIU) need objects to have specific gem handles.
> This ioctl changes the object at one gem handle to use a new gem handle.

struct drm\_event
:   Header for DRM events

**Definition**:

```
struct drm_event {
    __u32 type;
    __u32 length;
};
```

**Members**

`type`
:   event type.

`length`
:   total number of payload bytes (including header).

**Description**

This `struct is` a header for events written back to user-space on the DRM FD.
A read on the DRM FD will always only return complete events: e.g. if the
read buffer is 100 bytes large and there are two 64 byte events pending,
only one will be returned.

Event types 0 - 0x7fffffff are generic DRM events, 0x80000000 and
up are chipset specific. Generic DRM events include [`DRM_EVENT_VBLANK`](#c.DRM_EVENT_VBLANK "DRM_EVENT_VBLANK"),
[`DRM_EVENT_FLIP_COMPLETE`](#c.DRM_EVENT_FLIP_COMPLETE "DRM_EVENT_FLIP_COMPLETE") and [`DRM_EVENT_CRTC_SEQUENCE`](#c.DRM_EVENT_CRTC_SEQUENCE "DRM_EVENT_CRTC_SEQUENCE").

DRM\_EVENT\_VBLANK

`DRM_EVENT_VBLANK`

> > vertical blanking event
>
> **Description**
>
> This event is sent in response to `DRM_IOCTL_WAIT_VBLANK` with the
> `_DRM_VBLANK_EVENT` flag set.
>
> The event payload is a `struct drm_event_vblank`.

DRM\_EVENT\_FLIP\_COMPLETE

`DRM_EVENT_FLIP_COMPLETE`

> > page-flip completion event
>
> **Description**
>
> This event is sent in response to an atomic commit or legacy page-flip with
> the [`DRM_MODE_PAGE_FLIP_EVENT`](#c.DRM_MODE_PAGE_FLIP_EVENT "DRM_MODE_PAGE_FLIP_EVENT") flag set.
>
> The event payload is a `struct drm_event_vblank`.

DRM\_EVENT\_CRTC\_SEQUENCE

`DRM_EVENT_CRTC_SEQUENCE`

> > CRTC sequence event
>
> **Description**
>
> This event is sent in response to `DRM_IOCTL_CRTC_QUEUE_SEQUENCE`.
>
> The event payload is a `struct drm_event_crtc_sequence`.

struct drm\_mode\_modeinfo
:   Display mode information.

**Definition**:

```
struct drm_mode_modeinfo {
    __u32 clock;
    __u16 hdisplay;
    __u16 hsync_start;
    __u16 hsync_end;
    __u16 htotal;
    __u16 hskew;
    __u16 vdisplay;
    __u16 vsync_start;
    __u16 vsync_end;
    __u16 vtotal;
    __u16 vscan;
    __u32 vrefresh;
    __u32 flags;
    __u32 type;
    char name[DRM_DISPLAY_MODE_LEN];
};
```

**Members**

`clock`
:   pixel clock in kHz

`hdisplay`
:   horizontal display size

`hsync_start`
:   horizontal sync start

`hsync_end`
:   horizontal sync end

`htotal`
:   horizontal total size

`hskew`
:   horizontal skew

`vdisplay`
:   vertical display size

`vsync_start`
:   vertical sync start

`vsync_end`
:   vertical sync end

`vtotal`
:   vertical total size

`vscan`
:   vertical scan

`vrefresh`
:   approximate vertical refresh rate in Hz

`flags`
:   bitmask of misc. flags, see DRM\_MODE\_FLAG\_\* defines

`type`
:   bitmask of type flags, see DRM\_MODE\_TYPE\_\* defines

`name`
:   string describing the mode resolution

**Description**

This is the user-space API display mode information structure. For the
kernel version see [`struct drm_display_mode`](drm-kms.html#c.drm_display_mode "drm_display_mode").

struct drm\_mode\_get\_plane
:   Get plane metadata.

**Definition**:

```
struct drm_mode_get_plane {
    __u32 plane_id;
    __u32 crtc_id;
    __u32 fb_id;
    __u32 possible_crtcs;
    __u32 gamma_size;
    __u32 count_format_types;
    __u64 format_type_ptr;
};
```

**Members**

`plane_id`
:   Object ID of the plane whose information should be
    retrieved. Set by caller.

`crtc_id`
:   Object ID of the current CRTC.

`fb_id`
:   Object ID of the current fb.

`possible_crtcs`
:   Bitmask of CRTC’s compatible with the plane. CRTC’s
    are created and they receive an index, which corresponds to their
    position in the bitmask. Bit N corresponds to
    [CRTC index](#crtc-index) N.

`gamma_size`
:   Never used.

`count_format_types`
:   Number of formats.

`format_type_ptr`
:   Pointer to `__u32` array of formats that are
    supported by the plane. These formats do not require modifiers.

**Description**

Userspace can perform a GETPLANE ioctl to retrieve information about a
plane.

To retrieve the number of formats supported, set **count\_format\_types** to zero
and call the ioctl. **count\_format\_types** will be updated with the value.

To retrieve these formats, allocate an array with the memory needed to store
**count\_format\_types** formats. Point **format\_type\_ptr** to this array and call
the ioctl again (with **count\_format\_types** still set to the value returned in
the first ioctl call).

struct drm\_mode\_get\_connector
:   Get connector metadata.

**Definition**:

```
struct drm_mode_get_connector {
    __u64 encoders_ptr;
    __u64 modes_ptr;
    __u64 props_ptr;
    __u64 prop_values_ptr;
    __u32 count_modes;
    __u32 count_props;
    __u32 count_encoders;
    __u32 encoder_id;
    __u32 connector_id;
    __u32 connector_type;
    __u32 connector_type_id;
    __u32 connection;
    __u32 mm_width;
    __u32 mm_height;
    __u32 subpixel;
    __u32 pad;
};
```

**Members**

`encoders_ptr`
:   Pointer to `__u32` array of object IDs.

`modes_ptr`
:   Pointer to [`struct drm_mode_modeinfo`](#c.drm_mode_modeinfo "drm_mode_modeinfo") array.

`props_ptr`
:   Pointer to `__u32` array of property IDs.

`prop_values_ptr`
:   Pointer to `__u64` array of property values.

`count_modes`
:   Number of modes.

`count_props`
:   Number of properties.

`count_encoders`
:   Number of encoders.

`encoder_id`
:   Object ID of the current encoder.

`connector_id`
:   Object ID of the connector.

`connector_type`
:   Type of the connector.

    See DRM\_MODE\_CONNECTOR\_\* defines.

`connector_type_id`
:   Type-specific connector number.

    This is not an object ID. This is a per-type connector number. Each
    (type, type\_id) combination is unique across all connectors of a DRM
    device.

    The (type, type\_id) combination is not a stable identifier: the
    type\_id can change depending on the driver probe order.

`connection`
:   Status of the connector.

    See [`enum drm_connector_status`](drm-kms.html#c.drm_connector_status "drm_connector_status").

`mm_width`
:   Width of the connected sink in millimeters.

`mm_height`
:   Height of the connected sink in millimeters.

`subpixel`
:   Subpixel order of the connected sink.

    See `enum subpixel_order`.

`pad`
:   Padding, must be zero.

**Description**

User-space can perform a GETCONNECTOR ioctl to retrieve information about a
connector. User-space is expected to retrieve encoders, modes and properties
by performing this ioctl at least twice: the first time to retrieve the
number of elements, the second time to retrieve the elements themselves.

To retrieve the number of elements, set **count\_props** and **count\_encoders** to
zero, set **count\_modes** to 1, and set **modes\_ptr** to a temporary [`struct
drm_mode_modeinfo`](#c.drm_mode_modeinfo "drm_mode_modeinfo") element.

To retrieve the elements, allocate arrays for **encoders\_ptr**, **modes\_ptr**,
**props\_ptr** and **prop\_values\_ptr**, then set **count\_modes**, **count\_props** and
**count\_encoders** to their capacity.

Performing the ioctl only twice may be racy: the number of elements may have
changed with a hotplug event in-between the two ioctls. User-space is
expected to retry the last ioctl until the number of elements stabilizes.
The kernel won’t fill any array which doesn’t have the expected length.

**Force-probing a connector**

If the **count\_modes** field is set to zero and the DRM client is the current
DRM master, the kernel will perform a forced probe on the connector to
refresh the connector status, modes and EDID. A forced-probe can be slow,
might cause flickering and the ioctl will block.

User-space needs to force-probe connectors to ensure their metadata is
up-to-date at startup and after receiving a hot-plug event. User-space
may perform a forced-probe when the user explicitly requests it. User-space
shouldn’t perform a forced-probe in other situations.

struct drm\_mode\_property\_enum
:   Description for an enum/bitfield entry.

**Definition**:

```
struct drm_mode_property_enum {
    __u64 value;
    char name[DRM_PROP_NAME_LEN];
};
```

**Members**

`value`
:   numeric value for this `enum entry`.

`name`
:   symbolic name for this `enum entry`.

**Description**

See [`struct drm_property_enum`](drm-kms.html#c.drm_property_enum "drm_property_enum") for details.

struct drm\_mode\_get\_property
:   Get property metadata.

**Definition**:

```
struct drm_mode_get_property {
    __u64 values_ptr;
    __u64 enum_blob_ptr;
    __u32 prop_id;
    __u32 flags;
    char name[DRM_PROP_NAME_LEN];
    __u32 count_values;
    __u32 count_enum_blobs;
};
```

**Members**

`values_ptr`
:   Pointer to a `__u64` array.

`enum_blob_ptr`
:   Pointer to a [`struct drm_mode_property_enum`](#c.drm_mode_property_enum "drm_mode_property_enum") array.

`prop_id`
:   Object ID of the property which should be retrieved. Set
    by the caller.

`flags`
:   `DRM_MODE_PROP_*` bitfield. See [`drm_property.flags`](drm-kms.html#c.drm_property "drm_property") for
    a definition of the flags.

`name`
:   Symbolic property name. User-space should use this field to
    recognize properties.

`count_values`
:   Number of elements in **values\_ptr**.

`count_enum_blobs`
:   Number of elements in **enum\_blob\_ptr**.

**Description**

User-space can perform a GETPROPERTY ioctl to retrieve information about a
property. The same property may be attached to multiple objects, see
“Modeset Base Object Abstraction”.

The meaning of the **values\_ptr** field changes depending on the property type.
See [`drm_property.flags`](drm-kms.html#c.drm_property "drm_property") for more details.

The **enum\_blob\_ptr** and **count\_enum\_blobs** fields are only meaningful when the
property has the type `DRM_MODE_PROP_ENUM` or `DRM_MODE_PROP_BITMASK`. For
backwards compatibility, the kernel will always set **count\_enum\_blobs** to
zero when the property has the type `DRM_MODE_PROP_BLOB`. User-space must
ignore these two fields if the property has a different type.

User-space is expected to retrieve values and enums by performing this ioctl
at least twice: the first time to retrieve the number of elements, the
second time to retrieve the elements themselves.

To retrieve the number of elements, set **count\_values** and **count\_enum\_blobs**
to zero, then call the ioctl. **count\_values** will be updated with the number
of elements. If the property has the type `DRM_MODE_PROP_ENUM` or
`DRM_MODE_PROP_BITMASK`, **count\_enum\_blobs** will be updated as well.

To retrieve the elements themselves, allocate an array for **values\_ptr** and
set **count\_values** to its capacity. If the property has the type
`DRM_MODE_PROP_ENUM` or `DRM_MODE_PROP_BITMASK`, allocate an array for
**enum\_blob\_ptr** and set **count\_enum\_blobs** to its capacity. Calling the ioctl
again will fill the arrays.

struct drm\_mode\_fb\_cmd2
:   Frame-buffer metadata.

**Definition**:

```
struct drm_mode_fb_cmd2 {
    __u32 fb_id;
    __u32 width;
    __u32 height;
    __u32 pixel_format;
    __u32 flags;
    __u32 handles[4];
    __u32 pitches[4];
    __u32 offsets[4];
    __u64 modifier[4];
};
```

**Members**

`fb_id`
:   Object ID of the frame-buffer.

`width`
:   Width of the frame-buffer.

`height`
:   Height of the frame-buffer.

`pixel_format`
:   FourCC format code, see `DRM_FORMAT_*` constants in
    `drm_fourcc.h`.

`flags`
:   Frame-buffer flags (see `DRM_MODE_FB_INTERLACED` and
    `DRM_MODE_FB_MODIFIERS`).

`handles`
:   GEM buffer handle, one per plane. Set to 0 if the plane is
    unused. The same handle can be used for multiple planes.

`pitches`
:   Pitch (aka. stride) in bytes, one per plane.

`offsets`
:   Offset into the buffer in bytes, one per plane.

`modifier`
:   Format modifier, one per plane. See `DRM_FORMAT_MOD_*`
    constants in `drm_fourcc.h`. All planes must use the same
    modifier. Ignored unless `DRM_MODE_FB_MODIFIERS` is set in **flags**.

**Description**

This `struct holds` frame-buffer metadata. There are two ways to use it:

* User-space can fill this `struct and` perform a `DRM_IOCTL_MODE_ADDFB2`
  ioctl to register a new frame-buffer. The new frame-buffer object ID will
  be set by the kernel in **fb\_id**.
* User-space can set **fb\_id** and perform a [`DRM_IOCTL_MODE_GETFB2`](#c.DRM_IOCTL_MODE_GETFB2 "DRM_IOCTL_MODE_GETFB2") ioctl to
  fetch metadata about an existing frame-buffer.

In case of planar formats, this `struct allows` up to 4 buffer objects with
offsets and pitches per plane. The pitch and offset order are dictated by
the format FourCC as defined by `drm_fourcc.h`, e.g. NV12 is described as:

> YUV 4:2:0 image with a plane of 8-bit Y samples followed by an
> interleaved U/V plane containing 8-bit 2x2 subsampled colour difference
> samples.

So it would consist of a Y plane at `offsets[0]` and a UV plane at
`offsets[1]`.

To accommodate tiled, compressed, etc formats, a modifier can be specified.
For more information see the “Format Modifiers” section. Note that even
though it looks like we have a modifier per-plane, we in fact do not. The
modifier for each plane must be identical. Thus all combinations of
different data layouts for multi-plane formats must be enumerated as
separate modifiers.

All of the entries in **handles**, **pitches**, **offsets** and **modifier** must be
zero when unused. Warning, for **offsets** and **modifier** zero can’t be used to
figure out whether the entry is used or not since it’s a valid value (a zero
offset is common, and a zero modifier is `DRM_FORMAT_MOD_LINEAR`).

enum drm\_colorop\_type
:   Type of color operation

**Constants**

`DRM_COLOROP_1D_CURVE`
:   `enum string` “1D Curve”

    A 1D curve that is being applied to all color channels. The
    curve is specified via the CURVE\_1D\_TYPE colorop property.

`DRM_COLOROP_1D_LUT`
:   `enum string` “1D LUT”

    A simple 1D LUT of uniformly spaced `drm_color_lut32` entries,
    packed into a blob via the DATA property. The driver’s
    expected LUT size is advertised via the SIZE property.

    The DATA blob is an array of `struct drm_color_lut32` with size
    of “size”.

`DRM_COLOROP_CTM_3X4`
:   `enum string` “3x4 Matrix”

    A 3x4 matrix. Its values are specified via the
    `drm_color_ctm_3x4` `struct provided` via the DATA property.

    The DATA blob is a float[12]:
    out matrix in
    | R | | 0 1 2 3 | | R |
    | G | = | 4 5 6 7 | x | G |
    | B | | 8 9 10 12 | | B |

`DRM_COLOROP_MULTIPLIER`
:   `enum string` “Multiplier”

    A simple multiplier, applied to all color values. The
    multiplier is specified as a S31.32 via the MULTIPLIER
    property.

`DRM_COLOROP_3D_LUT`
:   `enum string` “3D LUT”

    A 3D LUT of `drm_color_lut32` entries,
    packed into a blob via the DATA property. The driver’s expected
    LUT size is advertised via the SIZE property, i.e., a 3D LUT with
    17x17x17 entries will have SIZE set to 17.

    The DATA blob is a 3D array of `struct drm_color_lut32` with dimension
    length of “size”.
    The LUT elements are traversed like so:

    > for B in range 0..n
    > :   for G in range 0..n
    >     :   for R in range 0..n
    >         :   index = R + n \* (G + n \* B)
    >             :   color = lut3d[index]

**Description**

drm\_colorops can be of many different types. Each type behaves differently
and defines a different set of properties. This `enum defines` all types and
gives a high-level description.

enum drm\_colorop\_lut3d\_interpolation\_type
:   type of 3DLUT interpolation

**Constants**

`DRM_COLOROP_LUT3D_INTERPOLATION_TETRAHEDRAL`
:   Tetrahedral 3DLUT interpolation

enum drm\_colorop\_lut1d\_interpolation\_type
:   type of interpolation for 1D LUTs

**Constants**

`DRM_COLOROP_LUT1D_INTERPOLATION_LINEAR`
:   Linear interpolation. Values between points of the LUT will be
    linearly interpolated.

struct drm\_plane\_size\_hint
:   Plane size hints

**Definition**:

```
struct drm_plane_size_hint {
    __u16 width;
    __u16 height;
};
```

**Members**

`width`
:   The width of the plane in pixel

`height`
:   The height of the plane in pixel

**Description**

The plane SIZE\_HINTS property blob contains an
array of [`struct drm_plane_size_hint`](#c.drm_plane_size_hint "drm_plane_size_hint").

struct hdr\_metadata\_infoframe
:   HDR Metadata Infoframe Data.

**Definition**:

```
struct hdr_metadata_infoframe {
    __u8 eotf;
    __u8 metadata_type;
    struct {
        __u16 x, y;
    } display_primaries[3];
    struct {
        __u16 x, y;
    } white_point;
    __u16 max_display_mastering_luminance;
    __u16 min_display_mastering_luminance;
    __u16 max_cll;
    __u16 max_fall;
};
```

**Members**

`eotf`
:   Electro-Optical Transfer Function (EOTF)
    used in the stream.

`metadata_type`
:   Static\_Metadata\_Descriptor\_ID.

`display_primaries`
:   Color Primaries of the Data.
    These are coded as unsigned 16-bit values in units of
    0.00002, where 0x0000 represents zero and 0xC350
    represents 1.0000.
    **display\_primaries.x**: X coordinate of color primary.
    **display\_primaries.y**: Y coordinate of color primary.

`white_point`
:   White Point of Colorspace Data.
    These are coded as unsigned 16-bit values in units of
    0.00002, where 0x0000 represents zero and 0xC350
    represents 1.0000.
    **white\_point.x**: X coordinate of whitepoint of color primary.
    **white\_point.y**: Y coordinate of whitepoint of color primary.

`max_display_mastering_luminance`
:   Max Mastering Display Luminance.
    This value is coded as an unsigned 16-bit value in units of 1 cd/m2,
    where 0x0001 represents 1 cd/m2 and 0xFFFF represents 65535 cd/m2.

`min_display_mastering_luminance`
:   Min Mastering Display Luminance.
    This value is coded as an unsigned 16-bit value in units of
    0.0001 cd/m2, where 0x0001 represents 0.0001 cd/m2 and 0xFFFF
    represents 6.5535 cd/m2.

`max_cll`
:   Max Content Light Level.
    This value is coded as an unsigned 16-bit value in units of 1 cd/m2,
    where 0x0001 represents 1 cd/m2 and 0xFFFF represents 65535 cd/m2.

`max_fall`
:   Max Frame Average Light Level.
    This value is coded as an unsigned 16-bit value in units of 1 cd/m2,
    where 0x0001 represents 1 cd/m2 and 0xFFFF represents 65535 cd/m2.

**Description**

HDR Metadata Infoframe as per CTA 861.G spec. This is expected
to match exactly with the spec.

Userspace is expected to pass the metadata information as per
the format described in this structure.

struct hdr\_output\_metadata
:   HDR output metadata

**Definition**:

```
struct hdr_output_metadata {
    __u32 metadata_type;
    union {
        struct hdr_metadata_infoframe hdmi_metadata_type1;
    };
};
```

**Members**

`metadata_type`
:   Static\_Metadata\_Descriptor\_ID.

`{unnamed_union}`
:   anonymous

`hdmi_metadata_type1`
:   HDR Metadata Infoframe.

**Description**

Metadata Information to be passed from userspace

DRM\_MODE\_PAGE\_FLIP\_EVENT

`DRM_MODE_PAGE_FLIP_EVENT`

> **Description**
>
> Request that the kernel sends back a vblank event (see
> `struct drm_event_vblank`) with the [`DRM_EVENT_FLIP_COMPLETE`](#c.DRM_EVENT_FLIP_COMPLETE "DRM_EVENT_FLIP_COMPLETE") type when the
> page-flip is done.
>
> When used with atomic uAPI, one event will be delivered per CRTC included in
> the atomic commit. A CRTC is included in an atomic commit if one of its
> properties is set, or if a property is set on a connector or plane linked
> via the CRTC\_ID property to the CRTC. At least one CRTC must be included,
> and all pulled in CRTCs must be either previously or newly powered on (in
> other words, a powered off CRTC which stays off cannot be included in the
> atomic commit).

DRM\_MODE\_PAGE\_FLIP\_ASYNC

`DRM_MODE_PAGE_FLIP_ASYNC`

> **Description**
>
> Request that the page-flip is performed as soon as possible, ie. with no
> delay due to waiting for vblank. This may cause tearing to be visible on
> the screen.
>
> When used with atomic uAPI, the driver will return an error if the hardware
> doesn’t support performing an asynchronous page-flip for this update.
> User-space should handle this, e.g. by falling back to a regular page-flip.
>
> Note, some hardware might need to perform one last synchronous page-flip
> before being able to switch to asynchronous page-flips. As an exception,
> the driver will return success even though that first page-flip is not
> asynchronous.

DRM\_MODE\_PAGE\_FLIP\_FLAGS

`DRM_MODE_PAGE_FLIP_FLAGS`

> **Description**
>
> Bitmask of flags suitable for `drm_mode_crtc_page_flip_target.flags`.

struct drm\_mode\_create\_dumb
:   Create a KMS dumb buffer for scanout.

**Definition**:

```
struct drm_mode_create_dumb {
    __u32 height;
    __u32 width;
    __u32 bpp;
    __u32 flags;
    __u32 handle;
    __u32 pitch;
    __u64 size;
};
```

**Members**

`height`
:   buffer height in pixels

`width`
:   buffer width in pixels

`bpp`
:   color mode

`flags`
:   must be zero

`handle`
:   buffer object handle

`pitch`
:   number of bytes between two consecutive lines

`size`
:   size of the whole buffer in bytes

**Description**

User-space fills **height**, **width**, **bpp** and **flags**. If the IOCTL succeeds,
the kernel fills **handle**, **pitch** and **size**.

The value of **bpp** is a color-mode number describing a specific format
or a variant thereof. The value often corresponds to the number of bits
per pixel for most modes, although there are exceptions. Each color mode
maps to a DRM format plus a number of modes with similar pixel layout.
Framebuffer layout is always linear.

Support for all modes and formats is optional. Even if dumb-buffer
creation with a certain color mode succeeds, it is not guaranteed that
the DRM driver supports any of the related formats. Most drivers support
a color mode of 32 with a format of DRM\_FORMAT\_XRGB8888 on their primary
plane.

| Color mode | Framebuffer format | Compatible formats |
| --- | --- | --- |
| 32 | * DRM\_FORMAT\_XRGB8888 | * DRM\_FORMAT\_BGRX8888 * DRM\_FORMAT\_RGBX8888 * DRM\_FORMAT\_XBGR8888 |
| 24 | * DRM\_FORMAT\_RGB888 | * DRM\_FORMAT\_BGR888 |
| 16 | * DRM\_FORMAT\_RGB565 | * DRM\_FORMAT\_BGR565 |
| 15 | * DRM\_FORMAT\_XRGB1555 | * DRM\_FORMAT\_BGRX1555 * DRM\_FORMAT\_RGBX1555 * DRM\_FORMAT\_XBGR1555 |
| 8 | * DRM\_FORMAT\_C8 | * DRM\_FORMAT\_D8 * DRM\_FORMAT\_R8 |
| 4 | * DRM\_FORMAT\_C4 | * DRM\_FORMAT\_D4 * DRM\_FORMAT\_R4 |
| 2 | * DRM\_FORMAT\_C2 | * DRM\_FORMAT\_D2 * DRM\_FORMAT\_R2 |
| 1 | * DRM\_FORMAT\_C1 | * DRM\_FORMAT\_D1 * DRM\_FORMAT\_R1 |

Color modes of 10, 12, 15, 30 and 64 are only supported for use by
legacy user space. Please don’t use them in new code. Other modes
are not support.

Do not attempt to allocate anything but linear framebuffer memory
with single-plane RGB data. Allocation of other framebuffer
layouts requires dedicated ioctls in the respective DRM driver.

DRM\_MODE\_ATOMIC\_TEST\_ONLY

`DRM_MODE_ATOMIC_TEST_ONLY`

> **Description**
>
> Do not apply the atomic commit, instead check whether the hardware supports
> this configuration.
>
> See [`drm_mode_config_funcs.atomic_check`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") for more details on test-only
> commits.

DRM\_MODE\_ATOMIC\_NONBLOCK

`DRM_MODE_ATOMIC_NONBLOCK`

> **Description**
>
> Do not block while applying the atomic commit. The `DRM_IOCTL_MODE_ATOMIC`
> IOCTL returns immediately instead of waiting for the changes to be applied
> in hardware. Note, the driver will still check that the update can be
> applied before retuning.

DRM\_MODE\_ATOMIC\_ALLOW\_MODESET

`DRM_MODE_ATOMIC_ALLOW_MODESET`

> **Description**
>
> Allow the update to result in temporary or transient visible artifacts while
> the update is being applied. Applying the update may also take significantly
> more time than a page flip. All visual artifacts will disappear by the time
> the update is completed, as signalled through the vblank event’s timestamp
> (see `struct drm_event_vblank`).
>
> This flag must be set when the KMS update might cause visible artifacts.
> Without this flag such KMS update will return a EINVAL error. What kind of
> update may cause visible artifacts depends on the driver and the hardware.
> User-space that needs to know beforehand if an update might cause visible
> artifacts can use [`DRM_MODE_ATOMIC_TEST_ONLY`](#c.DRM_MODE_ATOMIC_TEST_ONLY "DRM_MODE_ATOMIC_TEST_ONLY") without
> [`DRM_MODE_ATOMIC_ALLOW_MODESET`](#c.DRM_MODE_ATOMIC_ALLOW_MODESET "DRM_MODE_ATOMIC_ALLOW_MODESET") to see if it fails.
>
> To the best of the driver’s knowledge, visual artifacts are guaranteed to
> not appear when this flag is not set. Some sinks might display visual
> artifacts outside of the driver’s control.

DRM\_MODE\_ATOMIC\_FLAGS

`DRM_MODE_ATOMIC_FLAGS`

> **Description**
>
> Bitfield of flags accepted by the `DRM_IOCTL_MODE_ATOMIC` IOCTL in
> `drm_mode_atomic.flags`.

struct drm\_mode\_create\_blob
:   Create New blob property

**Definition**:

```
struct drm_mode_create_blob {
    __u64 data;
    __u32 length;
    __u32 blob_id;
};
```

**Members**

`data`
:   Pointer to data to copy.

`length`
:   Length of data to copy.

`blob_id`
:   Return: new property ID.

**Description**

Create a new ‘blob’ data property, copying length bytes from data pointer,
and returning new blob ID.

struct drm\_mode\_destroy\_blob
:   Destroy user blob

**Definition**:

```
struct drm_mode_destroy_blob {
    __u32 blob_id;
};
```

**Members**

`blob_id`
:   blob\_id to destroy

**Description**

Destroy a user-created blob property.

User-space can release blobs as soon as they do not need to refer to them by
their blob object ID. For instance, if you are using a MODE\_ID blob in an
atomic commit and you will not make another commit re-using the same ID, you
can destroy the blob as soon as the commit has been issued, without waiting
for it to complete.

struct drm\_mode\_create\_lease
:   Create lease

**Definition**:

```
struct drm_mode_create_lease {
    __u64 object_ids;
    __u32 object_count;
    __u32 flags;
    __u32 lessee_id;
    __u32 fd;
};
```

**Members**

`object_ids`
:   Pointer to array of object ids (\_\_u32)

`object_count`
:   Number of object ids

`flags`
:   flags for new FD (O\_CLOEXEC, etc)

`lessee_id`
:   Return: unique identifier for lessee.

`fd`
:   Return: file descriptor to new drm\_master file

**Description**

Lease mode resources, creating another drm\_master.

The **object\_ids** array must reference at least one CRTC, one connector and
one plane if [`DRM_CLIENT_CAP_UNIVERSAL_PLANES`](#c.DRM_CLIENT_CAP_UNIVERSAL_PLANES "DRM_CLIENT_CAP_UNIVERSAL_PLANES") is enabled. Alternatively,
the lease can be completely empty.

struct drm\_mode\_list\_lessees
:   List lessees

**Definition**:

```
struct drm_mode_list_lessees {
    __u32 count_lessees;
    __u32 pad;
    __u64 lessees_ptr;
};
```

**Members**

`count_lessees`
:   Number of lessees.

    On input, provides length of the array.
    On output, provides total number. No
    more than the input number will be written
    back, so two calls can be used to get
    the size and then the data.

`pad`
:   Padding.

`lessees_ptr`
:   Pointer to lessees.

    Pointer to \_\_u64 array of lessee ids

**Description**

List lesses from a drm\_master.

struct drm\_mode\_get\_lease
:   Get Lease

**Definition**:

```
struct drm_mode_get_lease {
    __u32 count_objects;
    __u32 pad;
    __u64 objects_ptr;
};
```

**Members**

`count_objects`
:   Number of leased objects.

    On input, provides length of the array.
    On output, provides total number. No
    more than the input number will be written
    back, so two calls can be used to get
    the size and then the data.

`pad`
:   Padding.

`objects_ptr`
:   Pointer to objects.

    Pointer to \_\_u32 array of object ids.

**Description**

Get leased objects.

struct drm\_mode\_revoke\_lease
:   Revoke lease

**Definition**:

```
struct drm_mode_revoke_lease {
    __u32 lessee_id;
};
```

**Members**

`lessee_id`
:   Unique ID of lessee

struct drm\_mode\_rect
:   Two dimensional rectangle.

**Definition**:

```
struct drm_mode_rect {
    __s32 x1;
    __s32 y1;
    __s32 x2;
    __s32 y2;
};
```

**Members**

`x1`
:   Horizontal starting coordinate (inclusive).

`y1`
:   Vertical starting coordinate (inclusive).

`x2`
:   Horizontal ending coordinate (exclusive).

`y2`
:   Vertical ending coordinate (exclusive).

**Description**

With drm subsystem using [`struct drm_rect`](drm-kms-helpers.html#c.drm_rect "drm_rect") to manage rectangular area this
export it to user-space.

Currently used by drm\_mode\_atomic blob property FB\_DAMAGE\_CLIPS.

struct drm\_mode\_closefb

**Definition**:

```
struct drm_mode_closefb {
    __u32 fb_id;
    __u32 pad;
};
```

**Members**

`fb_id`
:   Framebuffer ID.

`pad`
:   Must be zero.

## dma-buf interoperability

Please see [Exchanging pixel buffers](../userspace-api/dma-buf-alloc-exchange.html) for
information on how dma-buf is integrated and exposed within DRM.

## Trace events

See [Using the Linux Kernel Tracepoints](../trace/tracepoints.html) for information about using
Linux Kernel Tracepoints.
In the DRM subsystem, some events are considered stable uAPI to avoid
breaking tools (e.g.: GPUVis, umr) relying on them. Stable means that fields
cannot be removed, nor their formatting updated. Adding new fields is
possible, under the normal uAPI requirements.

### Stable uAPI events

From `drivers/gpu/drm/scheduler/gpu_scheduler_trace.h`

`drm_sched_job_queue`, `drm_sched_job_run`, `drm_sched_job_add_dep`,
`drm_sched_job_done` and `drm_sched_job_unschedulable` are considered
stable uAPI.

Common trace events attributes:

* `dev` - the [`dev_name()`](../driver-api/infrastructure.html#c.dev_name "dev_name") of the device running the job.
* `ring` - the hardware ring running the job. Together with `dev` it
  uniquely identifies where the job is going to be executed.
* `fence` - the [`struct dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence").context and the [`struct dma_fence`](../driver-api/dma-buf.html#c.dma_fence "dma_fence").seqno of
  [`struct drm_sched_fence`](drm-mm.html#c.drm_sched_fence "drm_sched_fence").finished

All the events depends on [`drm_sched_job_arm()`](drm-mm.html#c.drm_sched_job_arm "drm_sched_job_arm") having been called already for
the job because they use [`struct drm_sched_job`](drm-mm.html#c.drm_sched_job "drm_sched_job").sched or
[`struct drm_sched_job`](drm-mm.html#c.drm_sched_job "drm_sched_job").s\_fence.
