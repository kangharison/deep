# VGA Arbiter

> 출처(원문): https://docs.kernel.org/gpu/vgaarbiter.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VGA Arbiter

Graphic devices are accessed through ranges in I/O or memory space. While most
modern devices allow relocation of such ranges, some “Legacy” VGA devices
implemented on PCI will typically have the same “hard-decoded” addresses as
they did on ISA. For more details see “PCI Bus Binding to IEEE Std 1275-1994
Standard for Boot (Initialization Configuration) Firmware Revision 2.1”
Section 7, Legacy Devices.

The Resource Access Control (RAC) module inside the X server [0] existed for
the legacy VGA arbitration task (besides other bus management tasks) when more
than one legacy device co-exist on the same machine. But the problem happens
when these devices are trying to be accessed by different userspace clients
(e.g. two servers in parallel). Their address assignments conflict. Moreover,
ideally, being a userspace application, it is not the role of the X server to
control bus resources. Therefore an arbitration scheme outside of the X server
is needed to control the sharing of these resources. This document introduces
the operation of the VGA arbiter implemented for the Linux kernel.

## vgaarb kernel/userspace ABI

The vgaarb is a module of the Linux Kernel. When it is initially loaded, it
scans all PCI devices and adds the VGA ones inside the arbitration. The
arbiter then enables/disables the decoding on different devices of the VGA
legacy instructions. Devices which do not want/need to use the arbiter may
explicitly tell it by calling [`vga_set_legacy_decoding()`](#c.vga_set_legacy_decoding "vga_set_legacy_decoding").

The kernel exports a char device interface (/dev/vga\_arbiter) to the clients,
which has the following semantics:

open
:   Opens a user instance of the arbiter. By default, it’s attached to the
    default VGA device of the system.

close
:   Close a user instance. Release locks made by the user

read
:   Return a string indicating the status of the target like:

    “<card\_ID>,decodes=<io\_state>,owns=<io\_state>,locks=<io\_state> (ic,mc)”

    An IO state string is of the form {io,mem,io+mem,none}, mc and
    ic are respectively mem and io lock counts (for debugging/
    diagnostic only). “decodes” indicate what the card currently
    decodes, “owns” indicates what is currently enabled on it, and
    “locks” indicates what is locked by this card. If the card is
    unplugged, we get “invalid” then for card\_ID and an -ENODEV
    error is returned for any command until a new card is targeted.

write
:   Write a command to the arbiter. List of commands:

    target <card\_ID>
    :   switch target to card <card\_ID> (see below)

    lock <io\_state>
    :   acquires locks on target (“none” is an invalid io\_state)

    trylock <io\_state>
    :   non-blocking acquire locks on target (returns EBUSY if
        unsuccessful)

    unlock <io\_state>
    :   release locks on target

    unlock all
    :   release all locks on target held by this user (not implemented
        yet)

    decodes <io\_state>
    :   set the legacy decoding attributes for the card

    poll
    :   event if something changes on any card (not just the target)

    card\_ID is of the form “PCI:domain:bus:dev.fn”. It can be set to “default”
    to go back to the system default card (TODO: not implemented yet). Currently,
    only PCI is supported as a prefix, but the userland API may support other bus
    types in the future, even if the current kernel implementation doesn’t.

Note about locks:

The driver keeps track of which user has which locks on which card. It
supports stacking, like the kernel one. This complexifies the implementation
a bit, but makes the arbiter more tolerant to user space problems and able
to properly cleanup in all cases when a process dies.
Currently, a max of 16 cards can have locks simultaneously issued from
user space for a given user (file descriptor instance) of the arbiter.

In the case of devices hot-{un,}plugged, there is a hook - `pci_notify()` - to
notify them being added/removed in the system and automatically added/removed
in the arbiter.

There is also an in-kernel API of the arbiter in case DRM, vgacon, or other
drivers want to use it.

## In-kernel interface

int vga\_get\_interruptible(struct pci\_dev \*pdev, unsigned int rsrc)

**Parameters**

`struct pci_dev *pdev`
:   pci device of the VGA card or NULL for the system default

`unsigned int rsrc`
:   bit mask of resources to acquire and lock

**Description**

Shortcut to vga\_get with interruptible set to true.

On success, release the VGA resource again with [`vga_put()`](#c.vga_put "vga_put").

int vga\_get\_uninterruptible(struct pci\_dev \*pdev, unsigned int rsrc)
:   shortcut to [`vga_get()`](#c.vga_get "vga_get")

**Parameters**

`struct pci_dev *pdev`
:   pci device of the VGA card or NULL for the system default

`unsigned int rsrc`
:   bit mask of resources to acquire and lock

**Description**

Shortcut to vga\_get with interruptible set to false.

On success, release the VGA resource again with [`vga_put()`](#c.vga_put "vga_put").

struct pci\_dev \*vga\_default\_device(void)
:   return the default VGA device, for vgacon

**Parameters**

`void`
:   no arguments

**Description**

This can be defined by the platform. The default implementation is
rather dumb and will probably only work properly on single VGA card
setups and/or x86 platforms.

If your VGA default device is not PCI, you’ll have to return NULL here.
In this case, I assume it will not conflict with any PCI card. If this
is not true, I’ll have to define two arch hooks for enabling/disabling
the VGA default device if that is possible. This may be a problem with
real \_ISA\_ VGA cards, in addition to a PCI one. I don’t know at this
point how to deal with that card. Can their IOs be disabled at all? If
not, then I suppose it’s a matter of having the proper arch hook telling
us about it, so we basically never allow anybody to succeed a [`vga_get()`](#c.vga_get "vga_get").

int vga\_remove\_vgacon(struct pci\_dev \*pdev)
:   deactivate VGA console

**Parameters**

`struct pci_dev *pdev`
:   PCI device.

**Description**

Unbind and unregister vgacon in case pdev is the default VGA device.
Can be called by GPU drivers on initialization to make sure VGA register
access done by vgacon will not disturb the device.

int vga\_get(struct pci\_dev \*pdev, unsigned int rsrc, int interruptible)
:   acquire & lock VGA resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device of the VGA card or NULL for the system default

`unsigned int rsrc`
:   bit mask of resources to acquire and lock

`int interruptible`
:   blocking should be interruptible by signals ?

**Description**

Acquire VGA resources for the given card and mark those resources
locked. If the resources requested are “normal” (and not legacy)
resources, the arbiter will first check whether the card is doing legacy
decoding for that type of resource. If yes, the lock is “converted” into
a legacy resource lock.

The arbiter will first look for all VGA cards that might conflict and disable
their IOs and/or Memory access, including VGA forwarding on P2P bridges if
necessary, so that the requested resources can be used. Then, the card is
marked as locking these resources and the IO and/or Memory accesses are
enabled on the card (including VGA forwarding on parent P2P bridges if any).

This function will block if some conflicting card is already locking one of
the required resources (or any resource on a different bus segment, since P2P
bridges don’t differentiate VGA memory and IO afaik). You can indicate
whether this blocking should be interruptible by a signal (for userland
interface) or not.

Must not be called at interrupt time or in atomic context. If the card
already owns the resources, the function succeeds. Nested calls are
supported (a per-resource counter is maintained)

On success, release the VGA resource again with [`vga_put()`](#c.vga_put "vga_put").

0 on success, negative error code on failure.

void vga\_put(struct pci\_dev \*pdev, unsigned int rsrc)
:   release lock on legacy VGA resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device of VGA card or NULL for system default

`unsigned int rsrc`
:   bit mask of resource to release

**Description**

Release resources previously locked by [`vga_get()`](#c.vga_get "vga_get") or `vga_tryget()`. The
resources aren’t disabled right away, so that a subsequent [`vga_get()`](#c.vga_get "vga_get") on
the same card will succeed immediately. Resources have a counter, so
locks are only released if the counter reaches 0.

void vga\_set\_legacy\_decoding(struct pci\_dev \*pdev, unsigned int decodes)

**Parameters**

`struct pci_dev *pdev`
:   PCI device of the VGA card

`unsigned int decodes`
:   bit mask of what legacy regions the card decodes

**Description**

Indicate to the arbiter if the card decodes legacy VGA IOs, legacy VGA
Memory, both, or none. All cards default to both, the card driver (fbdev for
example) should tell the arbiter if it has disabled legacy decoding, so the
card can be left out of the arbitration process (and can be safe to take
interrupts at any time.

int vga\_client\_register(struct pci\_dev \*pdev, unsigned int (\*set\_decode)(struct pci\_dev \*pdev, bool decode))
:   register or unregister a VGA arbitration client

**Parameters**

`struct pci_dev *pdev`
:   PCI device of the VGA client

`unsigned int (*set_decode)(struct pci_dev *pdev, bool decode)`
:   VGA decode change callback

**Description**

Clients have two callback mechanisms they can use.

**set\_decode** callback: If a client can disable its GPU VGA resource, it
will get a callback from this to set the encode/decode state.

Rationale: we cannot disable VGA decode resources unconditionally
because some single GPU laptops seem to require ACPI or BIOS access to
the VGA registers to control things like backlights etc. Hopefully newer
multi-GPU laptops do something saner, and desktops won’t have any
special ACPI for this. The driver will get a callback when VGA
arbitration is first used by userspace since some older X servers have
issues.

Does not check whether a client for **pdev** has been registered already.

To unregister, call `vga_client_unregister()`.

**Return**

0 on success, -ENODEV on failure

## libpciaccess

To use the vga arbiter char device, an API was implemented inside the
libpciaccess library. One field was added to `struct pci_device` (each device
on the system):

```
/* the type of resource decoded by the device */
int vgaarb_rsrc;
```

Besides it, in pci\_system were added:

```
int vgaarb_fd;
int vga_count;
struct pci_device *vga_target;
struct pci_device *vga_default_dev;
```

The vga\_count is used to track how many cards are being arbitrated, so for
instance, if there is only one card, then it can completely escape arbitration.

These functions below acquire VGA resources for the given card and mark those
resources as locked. If the resources requested are “normal” (and not legacy)
resources, the arbiter will first check whether the card is doing legacy
decoding for that type of resource. If yes, the lock is “converted” into a
legacy resource lock. The arbiter will first look for all VGA cards that
might conflict and disable their IOs and/or Memory access, including VGA
forwarding on P2P bridges if necessary, so that the requested resources can
be used. Then, the card is marked as locking these resources and the IO and/or
Memory access is enabled on the card (including VGA forwarding on parent
P2P bridges if any). In the case of `vga_arb_lock()`, the function will block
if some conflicting card is already locking one of the required resources (or
any resource on a different bus segment, since P2P bridges don’t differentiate
VGA memory and IO afaik). If the card already owns the resources, the function
succeeds. `vga_arb_trylock()` will return (-EBUSY) instead of blocking. Nested
calls are supported (a per-resource counter is maintained).

Set the target device of this client.

```
int  pci_device_vgaarb_set_target   (struct pci_device *dev);
```

For instance, in x86 if two devices on the same bus want to lock different
resources, both will succeed (lock). If devices are in different buses and
trying to lock different resources, only the first who tried succeeds.

```
int  pci_device_vgaarb_lock         (void);
int  pci_device_vgaarb_trylock      (void);
```

Unlock resources of device.

```
int  pci_device_vgaarb_unlock       (void);
```

Indicates to the arbiter if the card decodes legacy VGA IOs, legacy VGA
Memory, both, or none. All cards default to both, the card driver (fbdev for
example) should tell the arbiter if it has disabled legacy decoding, so the
card can be left out of the arbitration process (and can be safe to take
interrupts at any time.

```
int  pci_device_vgaarb_decodes      (int new_vgaarb_rsrc);
```

Connects to the arbiter device, allocates the struct

```
int  pci_device_vgaarb_init         (void);
```

Close the connection

```
void pci_device_vgaarb_fini         (void);
```

## xf86VGAArbiter (X server implementation)

X server basically wraps all the functions that touch VGA registers somehow.

## References

Benjamin Herrenschmidt (IBM?) started this work when he discussed such design
with the Xorg community in 2005 [1, 2]. In the end of 2007, Paulo Zanoni and
Tiago Vignatti (both of C3SL/Federal University of Paraná) proceeded his work
enhancing the kernel code to adapt as a kernel module and also did the
implementation of the user space side [3]. Now (2009) Tiago Vignatti and Dave
Airlie finally put this work in shape and queued to Jesse Barnes’ PCI tree.

0. <https://cgit.freedesktop.org/xorg/xserver/commit/?id=4b42448a2388d40f257774fbffdccaea87bd0347>
1. <https://lists.freedesktop.org/archives/xorg/2005-March/006663.html>
2. <https://lists.freedesktop.org/archives/xorg/2005-March/006745.html>
3. <https://lists.freedesktop.org/archives/xorg/2007-October/029507.html>
