# Xe Configfs

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_configfs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Xe Configfs

## Overview

Configfs is a filesystem-based manager of kernel objects. Xe KMD registers a
configfs subsystem called `xe` that creates a directory in the mounted
configfs directory. The user can create devices under this directory and
configure them as necessary. See [Configfs - Userspace-driven Kernel Object Configuration](../../filesystems/configfs.html) for
more information about how configfs works.

## Create devices

To create a device, the `xe` module should already be loaded, but some
attributes can only be set before binding the device. It can be accomplished
by blocking the driver autoprobe:

```
# echo 0 > /sys/bus/pci/drivers_autoprobe
# modprobe xe
```

In order to create a device, the user has to create a directory inside `xe`:

```
# mkdir /sys/kernel/config/xe/0000:03:00.0/
```

Every device created is populated by the driver with entries that can be
used to configure it:

```
/sys/kernel/config/xe/
├── 0000:00:02.0
│   └── ...
├── 0000:00:02.1
│   └── ...
:
└── 0000:03:00.0
    ├── survivability_mode
    ├── gt_types_allowed
    ├── engines_allowed
    └── enable_psmi
```

After configuring the attributes as per next section, the device can be
probed with:

```
# echo 0000:03:00.0 > /sys/bus/pci/drivers/xe/bind
# # or
# echo 0000:03:00.0 > /sys/bus/pci/drivers_probe
```

## Configure Attributes

### Survivability mode:

Enable survivability mode on supported cards. This setting only takes
effect when probing the device. Example to enable it:

```
# echo 1 > /sys/kernel/config/xe/0000:03:00.0/survivability_mode
```

This attribute can only be set before binding to the device.

### Allowed GT types:

Allow only specific types of GTs to be detected and initialized by the
driver. Any combination of GT types can be enabled/disabled, although
some settings will cause the device to fail to probe.

Writes support both comma- and newline-separated input format. Reads
will always return one GT type per line. “primary” and “media” are the
GT type names supported by this interface.

This attribute can only be set before binding to the device.

Examples:

Allow both primary and media GTs to be initialized and used. This matches
the driver’s default behavior:

```
# echo 'primary,media' > /sys/kernel/config/xe/0000:03:00.0/gt_types_allowed
```

Allow only the primary GT of each tile to be initialized and used,
effectively disabling the media GT if it exists on the platform:

```
# echo 'primary' > /sys/kernel/config/xe/0000:03:00.0/gt_types_allowed
```

Allow only the media GT of each tile to be initialized and used,
effectively disabling the primary GT. **This configuration will cause
device probe failure on all current platforms, but may be allowed on
igpu platforms in the future**:

```
# echo 'media' > /sys/kernel/config/xe/0000:03:00.0/gt_types_allowed
```

Disable all GTs. Only other GPU IP (such as display) is potentially usable.
**This configuration will cause device probe failure on all current
platforms, but may be allowed on igpu platforms in the future**:

```
# echo '' > /sys/kernel/config/xe/0000:03:00.0/gt_types_allowed
```

### Allowed engines:

Allow only a set of engine(s) to be available, disabling the other engines
even if they are available in hardware. This is applied after HW fuses are
considered on each tile. Examples:

Allow only one render and one copy engines, nothing else:

```
# echo 'rcs0,bcs0' > /sys/kernel/config/xe/0000:03:00.0/engines_allowed
```

Allow only compute engines and first copy engine:

```
# echo 'ccs*,bcs0' > /sys/kernel/config/xe/0000:03:00.0/engines_allowed
```

Note that the engine names are the per-GT hardware names. On multi-tile
platforms, writing `rcs0,bcs0` to this file would allow the first render
and copy engines on each tile.

The requested configuration may not be supported by the platform and driver
may fail to probe. For example: if at least one copy engine is expected to be
available for migrations, but it’s disabled. This is intended for debugging
purposes only.

This attribute can only be set before binding to the device.

### PSMI

Enable extra debugging capabilities to trace engine execution. Only useful
during early platform enabling and requires additional hardware connected.
Once it’s enabled, additionals WAs are added and runtime configuration is
done via debugfs. Example to enable it:

```
# echo 1 > /sys/kernel/config/xe/0000:03:00.0/enable_psmi
```

This attribute can only be set before binding to the device.

### Context restore BB

Allow to execute a batch buffer during any context switches. When the
GPU is restoring the context, it executes additional commands. It’s useful
for testing additional workarounds and validating certain HW behaviors: it’s
not intended for normal execution and will taint the kernel with TAINT\_TEST
when used.

The syntax allows to pass straight instructions to be executed by the engine
in a batch buffer or set specific registers.

1. Generic instruction:

   ```
   <engine-class> cmd <instr> [[dword0] [dword1] [...]]
   ```
2. Simple register setting:

   ```
   <engine-class> reg <address> <value>
   ```

Commands are saved per engine class: all instances of that class will execute
those commands during context switch. The instruction, dword arguments,
addresses and values are in hex format like in the examples below.

1. Execute a LRI command to write 0xDEADBEEF to register 0x4f10 after the
   normal context restore:

   ```
   # echo 'rcs cmd 11000001 4F100 DEADBEEF' \
           > /sys/kernel/config/xe/0000:03:00.0/ctx_restore_post_bb
   ```
2. Execute a LRI command to write 0xDEADBEEF to register 0x4f10 at the
   beginning of the context restore:

   ```
   # echo 'rcs cmd 11000001 4F100 DEADBEEF' \
           > /sys/kernel/config/xe/0000:03:00.0/ctx_restore_mid_bb
   ```
3. Load certain values in a couple of registers (it can be used as a simpler
   alternative to the cmd) action:

   ```
   # cat > /sys/kernel/config/xe/0000:03:00.0/ctx_restore_post_bb <<EOF
   rcs reg 4F100 DEADBEEF
   rcs reg 4F104 FFFFFFFF
   EOF
   ```

   Note

   When using multiple lines, make sure to use a command that is
   implemented with a single write syscall, like HEREDOC.

Currently this is implemented only for post and mid context restore and
these attributes can only be set before binding to the device.

### Max SR-IOV Virtual Functions

This config allows to limit number of the Virtual Functions (VFs) that can
be managed by the Physical Function (PF) driver, where value 0 disables the
PF mode (no VFs).

The default max\_vfs config value is taken from the max\_vfs modparam.

How to enable PF with support with unlimited (up to HW limit) number of VFs:

```
# echo unlimited > /sys/kernel/config/xe/0000:00:02.0/sriov/max_vfs
# echo 0000:00:02.0 > /sys/bus/pci/drivers/xe/bind
```

How to enable PF with support up to 3 VFs:

```
# echo 3 > /sys/kernel/config/xe/0000:00:02.0/sriov/max_vfs
# echo 0000:00:02.0 > /sys/bus/pci/drivers/xe/bind
```

How to disable PF mode and always run as native:

```
# echo 0 > /sys/kernel/config/xe/0000:00:02.0/sriov/max_vfs
# echo 0000:00:02.0 > /sys/bus/pci/drivers/xe/bind
```

This setting only takes effect when probing the device.

## Remove devices

The created device directories can be removed using `rmdir`:

```
# rmdir /sys/kernel/config/xe/0000:03:00.0/
```

## Internal API

void xe\_configfs\_check\_device(struct pci\_dev \*pdev)
:   Test if device was configured by configfs

**Parameters**

`struct pci_dev *pdev`
:   the `pci_dev` device to test

**Description**

Try to find the configfs group that belongs to the specified pci device
and print a diagnostic message if different than the default value.

bool xe\_configfs\_get\_survivability\_mode(struct pci\_dev \*pdev)
:   get configfs survivability mode attribute

**Parameters**

`struct pci_dev *pdev`
:   pci device

**Return**

survivability\_mode attribute in configfs

bool xe\_configfs\_primary\_gt\_allowed(struct pci\_dev \*pdev)
:   determine whether primary GTs are supported

**Parameters**

`struct pci_dev *pdev`
:   pci device

**Return**

True if primary GTs are enabled, false if they have been disabled via
configfs.

bool xe\_configfs\_media\_gt\_allowed(struct pci\_dev \*pdev)
:   determine whether media GTs are supported

**Parameters**

`struct pci_dev *pdev`
:   pci device

**Return**

True if the media GTs are enabled, false if they have been disabled
via configfs.

u64 xe\_configfs\_get\_engines\_allowed(struct pci\_dev \*pdev)
:   get engine allowed mask from configfs

**Parameters**

`struct pci_dev *pdev`
:   pci device

**Return**

engine mask with allowed engines set in configfs

bool xe\_configfs\_get\_psmi\_enabled(struct pci\_dev \*pdev)
:   get configfs enable\_psmi setting

**Parameters**

`struct pci_dev *pdev`
:   pci device

**Return**

enable\_psmi setting in configfs

u32 xe\_configfs\_get\_ctx\_restore\_mid\_bb(struct pci\_dev \*pdev, enum xe\_engine\_class class, const u32 \*\*cs)
:   get configfs ctx\_restore\_mid\_bb setting

**Parameters**

`struct pci_dev *pdev`
:   pci device

`enum xe_engine_class class`
:   hw engine class

`const u32 **cs`
:   pointer to the bb to use - only valid during probe

**Return**

Number of dwords used in the mid\_ctx\_restore setting in configfs

u32 xe\_configfs\_get\_ctx\_restore\_post\_bb(struct pci\_dev \*pdev, enum xe\_engine\_class class, const u32 \*\*cs)
:   get configfs ctx\_restore\_post\_bb setting

**Parameters**

`struct pci_dev *pdev`
:   pci device

`enum xe_engine_class class`
:   hw engine class

`const u32 **cs`
:   pointer to the bb to use - only valid during probe

**Return**

Number of dwords used in the post\_ctx\_restore setting in configfs

bool xe\_configfs\_admin\_only\_pf(struct pci\_dev \*pdev)
:   Get PF’s operational mode.

**Parameters**

`struct pci_dev *pdev`
:   the `pci_dev` device

**Description**

Find the configfs group that belongs to the PCI device and return a flag
whether the PF driver should be dedicated for VFs management only.

If configfs group is not present, use driver’s default value.

**Return**

true if PF driver is dedicated for VFs administration only.

unsigned int xe\_configfs\_get\_max\_vfs(struct pci\_dev \*pdev)
:   Get number of VFs that could be managed

**Parameters**

`struct pci_dev *pdev`
:   the `pci_dev` device

**Description**

Find the configfs group that belongs to the PCI device and return maximum
number of Virtual Functions (VFs) that could be managed by this device.
If configfs group is not present, use value of max\_vfs module parameter.

**Return**

maximum number of VFs that could be managed.
