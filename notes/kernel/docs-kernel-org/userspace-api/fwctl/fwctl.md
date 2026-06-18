# fwctl subsystem

> 출처(원문): https://docs.kernel.org/userspace-api/fwctl/fwctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# fwctl subsystem

Author:
:   Jason Gunthorpe

## Overview

Modern devices contain extensive amounts of FW, and in many cases, are largely
software-defined pieces of hardware. The evolution of this approach is largely a
reaction to Moore’s Law where a chip tape out is now highly expensive, and the
chip design is extremely large. Replacing fixed HW logic with a flexible and
tightly coupled FW/HW combination is an effective risk mitigation against chip
respin. Problems in the HW design can be counteracted in device FW. This is
especially true for devices which present a stable and backwards compatible
interface to the operating system driver (such as NVMe).

The FW layer in devices has grown to incredible size and devices frequently
integrate clusters of fast processors to run it. For example, mlx5 devices have
over 30MB of FW code, and big configurations operate with over 1GB of FW managed
runtime state.

The availability of such a flexible layer has created quite a variety in the
industry where single pieces of silicon are now configurable software-defined
devices and can operate in substantially different ways depending on the need.
Further, we often see cases where specific sites wish to operate devices in ways
that are highly specialized and require applications that have been tailored to
their unique configuration.

Further, devices have become multi-functional and integrated to the point they
no longer fit neatly into the kernel’s division of subsystems. Modern
multi-functional devices have drivers, such as bnxt/ice/mlx5/pds, that span many
subsystems while sharing the underlying hardware using the auxiliary device
system.

All together this creates a challenge for the operating system, where devices
have an expansive FW environment that needs robust device-specific debugging
support, and FW-driven functionality that is not well suited to “generic”
interfaces. fwctl seeks to allow access to the full device functionality from
user space in the areas of debuggability, management, and first-boot/nth-boot
provisioning.

fwctl is aimed at the common device design pattern where the OS and FW
communicate via an RPC message layer constructed with a queue or mailbox scheme.
In this case the driver will typically have some layer to deliver RPC messages
and collect RPC responses from device FW. The in-kernel subsystem drivers that
operate the device for its primary purposes will use these RPCs to build their
drivers, but devices also usually have a set of ancillary RPCs that don’t really
fit into any specific subsystem. For example, a HW RAID controller is primarily
operated by the block layer but also comes with a set of RPCs to administer the
construction of drives within the HW RAID.

In the past when devices were more single function, individual subsystems would
grow different approaches to solving some of these common problems. For instance,
monitoring device health, manipulating its FLASH, debugging the FW,
provisioning, all have various unique interfaces across the kernel.

fwctl’s purpose is to define a common set of limited rules, described below,
that allow user space to securely construct and execute RPCs inside device FW.
The rules serve as an agreement between the operating system and FW on how to
correctly design the RPC interface. As a uAPI the subsystem provides a thin
layer of discovery and a generic uAPI to deliver the RPCs and collect the
response. It supports a system of user space libraries and tools which will
use this interface to control the device using the device native protocols.

### Scope of Action

fwctl drivers are strictly restricted to being a way to operate the device FW.
It is not an avenue to access random kernel internals, or other operating system
SW states.

fwctl instances must operate on a well-defined device function, and the device
should have a well-defined security model for what scope within the physical
device the function is permitted to access. For instance, the most complex PCIe
device today may broadly have several function-level scopes:

> 1. A privileged function with full access to the on-device global state and
>    configuration
> 2. Multiple hypervisor functions with control over itself and child functions
>    used with VMs
> 3. Multiple VM functions tightly scoped within the VM

The device may create a logical parent/child relationship between these scopes.
For instance, a child VM’s FW may be within the scope of the hypervisor FW. It is
quite common in the VFIO world that the hypervisor environment has a complex
provisioning/profiling/configuration responsibility for the function VFIO
assigns to the VM.

Further, within the function, devices often have RPC commands that fall within
some general scopes of action (see [`enum fwctl_rpc_scope`](#c.fwctl_rpc_scope "fwctl_rpc_scope")):

> 1. Access to function & child configuration, FLASH, etc. that becomes live at a
>    function reset. Access to function & child runtime configuration that is
>    transparent or non-disruptive to any driver or VM.
> 2. Read-only access to function debug information that may report on FW objects
>    in the function & child, including FW objects owned by other kernel
>    subsystems.
> 3. Write access to function & child debug information strictly compatible with
>    the principles of kernel lockdown and kernel integrity protection. Triggers
>    a kernel taint.
> 4. Full debug device access. Triggers a kernel taint, requires CAP\_SYS\_RAWIO.

User space will provide a scope label on each RPC and the kernel must enforce the
above CAPs and taints based on that scope. A combination of kernel and FW can
enforce that RPCs are placed in the correct scope by user space.

### Disallowed behavior

There are many things this interface must not allow user space to do (without a
taint or CAP), broadly derived from the principles of kernel lockdown. Some
examples:

> 1. DMA to/from arbitrary memory, hang the system, compromise FW integrity with
>    untrusted code, or otherwise compromise device or system security and
>    integrity.
> 2. Provide an abnormal “back door” to kernel drivers. No manipulation of kernel
>    objects owned by kernel drivers.
> 3. Directly configure or otherwise control kernel drivers. A subsystem kernel
>    driver can react to the device configuration at function reset/driver load
>    time, but otherwise must not be coupled to fwctl.
> 4. Operate the HW in a way that overlaps with the core purpose of another
>    primary kernel subsystem, such as read/write to LBAs, send/receive of
>    network packets, or operate an accelerator’s data plane.

fwctl is not a replacement for device direct access subsystems like uacce or
VFIO.

Operations exposed through fwctl’s non-tainting interfaces should be fully
sharable with other users of the device. For instance, exposing a RPC through
fwctl should never prevent a kernel subsystem from also concurrently using that
same RPC or hardware unit down the road. In such cases fwctl will be less
important than proper kernel subsystems that eventually emerge. Mistakes in this
area resulting in clashes will be resolved in favour of a kernel implementation.

## fwctl User API

struct fwctl\_info\_bnxt
:   ioctl(FWCTL\_INFO) out\_device\_data

**Definition**:

```
struct fwctl_info_bnxt {
    __u32 uctx_caps;
};
```

**Members**

`uctx_caps`
:   The command capabilities driver accepts.

**Description**

Return basic information about the FW interface available.

**General ioctl format**

The ioctl interface follows a general format to allow for extensibility. Each
ioctl is passed a structure pointer as the argument providing the size of
the structure in the first u32. The kernel checks that any structure space
beyond what it understands is 0. This allows userspace to use the backward
compatible portion while consistently using the newer, larger, structures.

ioctls use a standard meaning for common errnos:

> * ENOTTY: The IOCTL number itself is not supported at all
> * E2BIG: The IOCTL number is supported, but the provided structure has
>   non-zero in a part the kernel does not understand.
> * EOPNOTSUPP: The IOCTL number is supported, and the structure is
>   understood, however a known field has a value the kernel does not
>   understand or support.
> * EINVAL: Everything about the IOCTL was understood, but a field is not
>   correct.
> * ENOMEM: Out of memory.
> * ENODEV: The underlying device has been hot-unplugged and the FD is
>   :   orphaned.

As well as additional errnos, within specific ioctls.

struct fwctl\_info
:   ioctl(FWCTL\_INFO)

**Definition**:

```
struct fwctl_info {
    __u32 size;
    __u32 flags;
    __u32 out_device_type;
    __u32 device_data_len;
    __aligned_u64 out_device_data;
};
```

**Members**

`size`
:   sizeof([`struct fwctl_info`](#c.fwctl_info "fwctl_info"))

`flags`
:   Must be 0

`out_device_type`
:   Returns the type of the device from `enum fwctl_device_type`

`device_data_len`
:   On input the length of the out\_device\_data memory. On
    output the size of the kernel’s device\_data which may be larger or
    smaller than the input. Maybe 0 on input.

`out_device_data`
:   Pointer to a memory of device\_data\_len bytes. Kernel will
    fill the entire memory, zeroing as required.

**Description**

Returns basic information about this fwctl instance, particularly what driver
is being used to define the device\_data format.

enum fwctl\_rpc\_scope
:   Scope of access for the RPC

**Constants**

`FWCTL_RPC_CONFIGURATION`
:   Device configuration access scope

    Read/write access to device configuration. When configuration
    is written to the device it remains in a fully supported state.

`FWCTL_RPC_DEBUG_READ_ONLY`
:   Read only access to debug information

    Readable debug information. Debug information is compatible with
    kernel lockdown, and does not disclose any sensitive information. For
    instance exposing any encryption secrets from this information is
    forbidden.

`FWCTL_RPC_DEBUG_WRITE`
:   Writable access to lockdown compatible debug information

    Allows write access to data in the device which may leave a fully
    supported state. This is intended to permit intensive and possibly
    invasive debugging. This scope will taint the kernel.

`FWCTL_RPC_DEBUG_WRITE_FULL`
:   Write access to all debug information

    Allows read/write access to everything. Requires CAP\_SYS\_RAW\_IO, so
    it is not required to follow lockdown principals. If in doubt
    debugging should be placed in this scope. This scope will taint the
    kernel.

**Description**

Refer to [fwctl subsystem](#) for a more detailed discussion of these scopes.

struct fwctl\_rpc
:   ioctl(FWCTL\_RPC)

**Definition**:

```
struct fwctl_rpc {
    __u32 size;
    __u32 scope;
    __u32 in_len;
    __u32 out_len;
    __aligned_u64 in;
    __aligned_u64 out;
};
```

**Members**

`size`
:   sizeof([`struct fwctl_rpc`](#c.fwctl_rpc "fwctl_rpc"))

`scope`
:   One of [`enum fwctl_rpc_scope`](#c.fwctl_rpc_scope "fwctl_rpc_scope"), required scope for the RPC

`in_len`
:   Length of the in memory

`out_len`
:   Length of the out memory

`in`
:   Request message in device specific format

`out`
:   Response message in device specific format

**Description**

Deliver a Remote Procedure Call to the device FW and return the response. The
call’s parameters and return are marshaled into linear buffers of memory. Any
errno indicates that delivery of the RPC to the device failed. Return status
originating in the device during a successful delivery must be encoded into
out.

The format of the buffers matches the out\_device\_type from FWCTL\_INFO.

struct fwctl\_info\_mlx5
:   ioctl(FWCTL\_INFO) out\_device\_data

**Definition**:

```
struct fwctl_info_mlx5 {
    __u32 uid;
    __u32 uctx_caps;
};
```

**Members**

`uid`
:   The FW UID this FD is bound to. Each command header will force
    this value.

`uctx_caps`
:   The FW capabilities that are enabled for the uid.

**Description**

Return basic information about the FW interface available.

struct fwctl\_info\_pds

**Definition**:

```
struct fwctl_info_pds {
    __u32 uctx_caps;
};
```

**Members**

`uctx_caps`
:   bitmap of firmware capabilities

**Description**

Return basic information about the FW interface available.

enum pds\_fwctl\_capabilities

**Constants**

`PDS_FWCTL_QUERY_CAP`
:   firmware can be queried for information

`PDS_FWCTL_SEND_CAP`
:   firmware can be sent commands

struct fwctl\_rpc\_pds

**Definition**:

```
struct fwctl_rpc_pds {
    struct {
        __u32 op;
        __u32 ep;
        __u32 rsvd;
        __u32 len;
        __aligned_u64 payload;
    } in;
    struct {
        __u32 retval;
        __u32 rsvd[2];
        __u32 len;
        __aligned_u64 payload;
    } out;
};
```

**Members**

`in`
:   rpc in parameters

`in.op`
:   requested operation code

`in.ep`
:   firmware endpoint to operate on

`in.rsvd`
:   reserved

`in.len`
:   length of payload data

`in.payload`
:   address of payload buffer

`out`
:   rpc out parameters

`out.retval`
:   operation result value

`out.rsvd`
:   reserved

`out.len`
:   length of result data buffer

`out.payload`
:   address of payload data buffer

### sysfs Class

fwctl has a sysfs class (/sys/class/fwctl/fwctlNN/) and character devices
(/dev/fwctl/fwctlNN) with a simple numbered scheme. The character device
operates the iotcl uAPI described above.

fwctl devices can be related to driver components in other subsystems through
sysfs:

```
$ ls /sys/class/fwctl/fwctl0/device/infiniband/
ibp0s10f0

$ ls /sys/class/infiniband/ibp0s10f0/device/fwctl/
fwctl0/

$ ls /sys/devices/pci0000:00/0000:00:0a.0/fwctl/fwctl0
dev  device  power  subsystem  uevent
```

### User space Community

Drawing inspiration from nvme-cli, participating in the kernel side must come
with a user space in a common TBD git tree, at a minimum to usefully operate the
kernel driver. Providing such an implementation is a pre-condition to merging a
kernel driver.

The goal is to build user space community around some of the shared problems
we all have, and ideally develop some common user space programs with some
starting themes of:

> * Device in-field debugging
> * HW provisioning
> * VFIO child device profiling before VM boot
> * Confidential Compute topics (attestation, secure provisioning)

that stretch across all subsystems in the kernel. fwupd is a great example of
how an excellent user space experience can emerge out of kernel-side diversity.

## fwctl Kernel API

int fwctl\_register(struct [fwctl\_device](#c.fwctl_device "fwctl_device") \*fwctl)
:   Register a new device to the subsystem

**Parameters**

`struct fwctl_device *fwctl`
:   Previously allocated fwctl\_device

**Description**

On return the device is visible through sysfs and /dev, driver ops may be
called.

void fwctl\_unregister(struct [fwctl\_device](#c.fwctl_device "fwctl_device") \*fwctl)
:   Unregister a device from the subsystem

**Parameters**

`struct fwctl_device *fwctl`
:   Previously allocated and registered fwctl\_device

**Description**

Undoes [`fwctl_register()`](#c.fwctl_register "fwctl_register"). On return no driver ops will be called. The
caller must still call `fwctl_put()` to free the fwctl.

Unregister will return even if userspace still has file descriptors open.
This will call ops->`close_uctx()` on any open FDs and after return no driver
op will be called. The FDs remain open but all fops will return -ENODEV.

The design of fwctl allows this sort of disassociation of the driver from the
subsystem primarily by keeping memory allocations owned by the core subsytem.
The fwctl\_device and fwctl\_uctx can both be freed without requiring a driver
callback. This allows the module to remain unlocked while FDs are open.

struct fwctl\_ops
:   Driver provided operations

**Definition**:

```
struct fwctl_ops {
    enum fwctl_device_type device_type;
    size_t uctx_size;
    int (*open_uctx)(struct fwctl_uctx *uctx);
    void (*close_uctx)(struct fwctl_uctx *uctx);
    void *(*info)(struct fwctl_uctx *uctx, size_t *length);
    void *(*fw_rpc)(struct fwctl_uctx *uctx, enum fwctl_rpc_scope scope, void *rpc_in, size_t in_len, size_t *out_len);
};
```

**Members**

`device_type`
:   The drivers assigned device\_type number. This is uABI.

`uctx_size`
:   The size of the fwctl\_uctx `struct to` allocate. The first
    bytes of this memory will be a fwctl\_uctx. The driver can use the
    remaining bytes as its private memory.

`open_uctx`
:   Called when a file descriptor is opened before the uctx
    is ever used.

`close_uctx`
:   Called when the uctx is destroyed, usually when the FD
    is closed.

`info`
:   Implement FWCTL\_INFO. Return a [`kmalloc()`](../../core-api/mm-api.html#c.kmalloc "kmalloc") memory that is copied
    to out\_device\_data. On input length indicates the size of the user
    buffer on output it indicates the size of the memory. The driver can
    ignore length on input, the core code will handle everything.

`fw_rpc`
:   Implement FWCTL\_RPC. Deliver rpc\_in/in\_len to the FW and
    return the response and set out\_len. rpc\_in can be returned as the
    response pointer. Otherwise the returned pointer is freed with
    [`kvfree()`](../../core-api/mm-api.html#c.kvfree "kvfree").

**Description**

[`fwctl_unregister()`](#c.fwctl_unregister "fwctl_unregister") will wait until all excuting ops are completed before it
returns. Drivers should be mindful to not let their ops run for too long as
it will block device hot unplug and module unloading.

struct fwctl\_device
:   Per-driver registration struct

**Definition**:

```
struct fwctl_device {
    struct device dev;
};
```

**Members**

`dev`
:   The sysfs (class/fwctl/fwctlXX) device

**Description**

Each driver instance will have one of these structs with the driver private
data following immediately after. This `struct is` refcounted, it is freed by
calling `fwctl_put()`.

fwctl\_alloc\_device

`fwctl_alloc_device (parent, ops, drv_struct, member)`

> Allocate a fwctl

**Parameters**

`parent`
:   Physical device that provides the FW interface

`ops`
:   Driver ops to register

`drv_struct`
:   ‘`struct driver_fwctl`’ that holds the [`struct fwctl_device`](#c.fwctl_device "fwctl_device")

`member`
:   Name of the [`struct fwctl_device`](#c.fwctl_device "fwctl_device") in **drv\_struct**

**Description**

This allocates and initializes the fwctl\_device embedded in the drv\_struct.
Upon success the pointer must be freed via `fwctl_put()`. Returns a ‘drv\_struct
\*’ on success, NULL on error.

struct fwctl\_uctx
:   Per user FD context

**Definition**:

```
struct fwctl_uctx {
    struct fwctl_device *fwctl;
};
```

**Members**

`fwctl`
:   fwctl instance that owns the context

**Description**

Every FD opened by userspace will get a unique context allocation. Any driver
private data will follow immediately after.

### fwctl Driver design

In many cases a fwctl driver is going to be part of a larger cross-subsystem
device possibly using the auxiliary\_device mechanism. In that case several
subsystems are going to be sharing the same device and FW interface layer so the
device design must already provide for isolation and cooperation between kernel
subsystems. fwctl should fit into that same model.

Part of the driver should include a description of how its scope restrictions
and security model work. The driver and FW together must ensure that RPCs
provided by user space are mapped to the appropriate scope. If the validation is
done in the driver then the validation can read a ‘command effects’ report from
the device, or hardwire the enforcement. If the validation is done in the FW,
then the driver should pass the fwctl\_rpc\_scope to the FW along with the command.

The driver and FW must cooperate to ensure that either fwctl cannot allocate
any FW resources, or any resources it does allocate are freed on FD closure. A
driver primarily constructed around FW RPCs may find that its core PCI function
and RPC layer belongs under fwctl with auxiliary devices connecting to other
subsystems.

Each device type must be mindful of Linux’s philosophy for stable ABI. The FW
RPC interface does not have to meet a strictly stable ABI, but it does need to
meet an expectation that user space tools that are deployed and in significant
use don’t needlessly break. FW upgrade and kernel upgrade should keep widely
deployed tooling working.

Development and debugging focused RPCs under more permissive scopes can have
less stability if the tools using them are only run under exceptional
circumstances and not for every day use of the device. Debugging tools may even
require exact version matching as they may require something similar to DWARF
debug information from the FW binary.

## Security Response

The kernel remains the gatekeeper for this interface. If violations of the
scopes, security or isolation principles are found, we have options to let
devices fix them with a FW update, push a kernel patch to parse and block RPC
commands or push a kernel patch to block entire firmware versions/devices.

While the kernel can always directly parse and restrict RPCs, it is expected
that the existing kernel pattern of allowing drivers to delegate validation to
FW to be a useful design.

## Existing Similar Examples

The approach described in this document is not a new idea. Direct, or near
direct device access has been offered by the kernel in different areas for
decades. With more devices wanting to follow this design pattern it is becoming
clear that it is not entirely well understood and, more importantly, the
security considerations are not well defined or agreed upon.

Some examples:

> * HW RAID controllers. This includes RPCs to do things like compose drives into
>   a RAID volume, configure RAID parameters, monitor the HW and more.
> * Baseboard managers. RPCs for configuring settings in the device and more.
> * NVMe vendor command capsules. nvme-cli provides access to some monitoring
>   functions that different products have defined, but more exist.
> * CXL also has a NVMe-like vendor command system.
> * DRM allows user space drivers to send commands to the device via kernel
>   mediation.
> * RDMA allows user space drivers to directly push commands to the device
>   without kernel involvement.
> * Various “raw” APIs, raw HID (SDL2), raw USB, NVMe Generic Interface, etc.

The first 4 are examples of areas that fwctl intends to cover. The latter three
are examples of disallowed behavior as they fully overlap with the primary purpose
of a kernel subsystem.

Some key lessons learned from these past efforts are the importance of having a
common user space project to use as a pre-condition for obtaining a kernel
driver. Developing good community around useful software in user space is key to
getting companies to fund participation to enable their products.
