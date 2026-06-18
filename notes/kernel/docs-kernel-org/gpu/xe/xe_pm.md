# Runtime Power Management

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_pm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Runtime Power Management

Xe PM implements the main routines for both system level suspend states and
for the opportunistic runtime suspend states.

System Level Suspend (S-States) - In general this is OS initiated suspend
driven by ACPI for achieving S0ix (a.k.a. S2idle, freeze), S3 (suspend to ram),
S4 (disk). The main functions here are xe\_pm\_suspend and xe\_pm\_resume. They
are the main point for the suspend to and resume from these states.

PCI Device Suspend (D-States) - This is the opportunistic PCIe device low power
state D3, controlled by the PCI subsystem and ACPI with the help from the
runtime\_pm infrastructure.
PCI D3 is special and can mean D3hot, where Vcc power is on for keeping memory
alive and quicker low latency resume or D3Cold where Vcc power is off for
better power savings.
The Vcc control of PCI hierarchy can only be controlled at the PCI root port
level, while the device driver can be behind multiple bridges/switches and
paired with other devices. For this reason, the PCI subsystem cannot perform
the transition towards D3Cold. The lowest runtime PM possible from the PCI
subsystem is D3hot. Then, if all these paired devices in the same root port
are in D3hot, ACPI will assist here and run its own methods (\_PR3 and \_OFF)
to perform the transition from D3hot to D3cold. Xe may disallow this
transition by calling pci\_d3cold\_disable(root\_pdev) before going to runtime
suspend. It will be based on runtime conditions such as VRAM usage for a
quick and low latency resume for instance.

Runtime PM - This infrastructure provided by the Linux kernel allows the
device drivers to indicate when the can be runtime suspended, so the device
could be put at D3 (if supported), or allow deeper package sleep states
(PC-states), and/or other low level power states. Xe PM component provides
xe\_pm\_runtime\_suspend and xe\_pm\_runtime\_resume functions that PCI
subsystem will call before transition to/from runtime suspend.

Also, Xe PM provides get and put functions that Xe driver will use to
indicate activity. In order to avoid locking complications with the memory
management, whenever possible, these get and put functions needs to be called
from the higher/outer levels.
The main cases that need to be protected from the outer levels are: IOCTL,
sysfs, debugfs, dma-buf sharing, GPU execution.

This component is not responsible for GT idleness (RC6) nor GT frequency
management (RPS).

## Internal API

void xe\_pm\_might\_block\_on\_suspend(void)
:   Annotate that the code might block on suspend

**Parameters**

`void`
:   no arguments

**Description**

Annotation to use where the code might block or seize to make
progress pending resume completion.

int xe\_pm\_block\_on\_suspend(struct xe\_device \*xe)
:   Block pending suspend.

**Parameters**

`struct xe_device *xe`
:   The xe device about to be suspended.

**Description**

Block if the pm notifier has start evicting bos, to avoid
racing and validating those bos back. The function is
annotated to ensure no locks are held that are also grabbed
in the pm notifier or the device suspend / resume.
This is intended to be used by freezable tasks only.
(Not freezable workqueues), with the intention that the function
returns `-ERESTARTSYS` when tasks are frozen during suspend,
and allows the task to freeze. The caller must be able to
handle the `-ERESTARTSYS`.

**Return**

`0` on success, `-ERESTARTSYS` on signal pending or
if freezing requested.

bool xe\_rpm\_reclaim\_safe(const struct xe\_device \*xe)
:   Whether runtime resume can be done from reclaim context

**Parameters**

`const struct xe_device *xe`
:   The xe device.

**Return**

true if it is safe to runtime resume from reclaim context.
false otherwise.

int xe\_pm\_suspend(struct xe\_device \*xe)
:   Helper for System suspend, i.e. S0->S3 / S0->S2idle

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Return**

0 on success

int xe\_pm\_resume(struct xe\_device \*xe)
:   Helper for System resume S3->S0 / S2idle->S0

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Return**

0 on success

int xe\_pm\_init(struct xe\_device \*xe)
:   Initialize Xe Power Management

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

This component is responsible for System and Device sleep states.

Returns 0 for success, negative error code otherwise.

void xe\_pm\_fini(struct xe\_device \*xe)
:   Finalize PM

**Parameters**

`struct xe_device *xe`
:   xe device instance

bool xe\_pm\_runtime\_suspended(struct xe\_device \*xe)
:   Check if runtime\_pm state is suspended

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

This does not provide any guarantee that the device is going to remain
suspended as it might be racing with the runtime state transitions.
It can be used only as a non-reliable assertion, to ensure that we are not in
the sleep state while trying to access some memory for instance.

Returns true if PCI device is suspended, false otherwise.

int xe\_pm\_runtime\_suspend(struct xe\_device \*xe)
:   Prepare our device for D3hot/D3Cold

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

Returns 0 for success, negative error code otherwise.

int xe\_pm\_runtime\_resume(struct xe\_device \*xe)
:   Waking up from D3hot/D3Cold

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

Returns 0 for success, negative error code otherwise.

void xe\_pm\_runtime\_get(struct xe\_device \*xe)
:   Get a runtime\_pm reference and resume synchronously

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

When possible, scope-based runtime PM (through guard(xe\_pm\_runtime)) is
be preferred over direct usage of this function. Manual get/put handling
should only be used when the function contains goto-based logic which
can break scope-based handling, or when the lifetime of the runtime PM
reference does not match a specific scope (e.g., runtime PM obtained in one
function and released in a different one).

void xe\_pm\_runtime\_put(struct xe\_device \*xe)
:   Put the runtime\_pm reference back and mark as idle

**Parameters**

`struct xe_device *xe`
:   xe device instance

int xe\_pm\_runtime\_get\_ioctl(struct xe\_device \*xe)
:   Get a runtime\_pm reference before ioctl

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

When possible, scope-based runtime PM (through
ACQUIRE(xe\_pm\_runtime\_ioctl, ...)) is be preferred over direct usage of this
function. Manual get/put handling should only be used when the function
contains goto-based logic which can break scope-based handling, or when the
lifetime of the runtime PM reference does not match a specific scope (e.g.,
runtime PM obtained in one function and released in a different one).

**Return**

Any number greater than or equal to 0 for success, negative error
code otherwise.

bool xe\_pm\_runtime\_get\_if\_active(struct xe\_device \*xe)
:   Get a runtime\_pm reference if device active

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Return**

True if device is awake (regardless the previous number of references)
and a new reference was taken, false otherwise.

bool xe\_pm\_runtime\_get\_if\_in\_use(struct xe\_device \*xe)
:   Get a new reference if device is active with previous ref taken

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Return**

True if device is awake, a previous reference had been already taken,
and a new reference was now taken, false otherwise.

void xe\_pm\_runtime\_get\_noresume(struct xe\_device \*xe)
:   Bump runtime PM usage counter without resuming

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

This function should be used in inner places where it is surely already
protected by outer-bound callers of xe\_pm\_runtime\_get.
It will warn if not protected.
The reference should be put back after this function regardless, since it
will always bump the usage counter, regardless.

When possible, scope-based runtime PM (through guard(xe\_pm\_runtime\_noresume))
is be preferred over direct usage of this function. Manual get/put handling
should only be used when the function contains goto-based logic which can
break scope-based handling, or when the lifetime of the runtime PM reference
does not match a specific scope (e.g., runtime PM obtained in one function
and released in a different one).

bool xe\_pm\_runtime\_resume\_and\_get(struct xe\_device \*xe)
:   Resume, then get a runtime\_pm ref if awake.

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Return**

True if device is awake and the reference was taken, false otherwise.

void xe\_pm\_assert\_unbounded\_bridge(struct xe\_device \*xe)
:   Disable PM on unbounded pcie parent bridge

**Parameters**

`struct xe_device *xe`
:   xe device instance

int xe\_pm\_set\_vram\_threshold(struct xe\_device \*xe, u32 threshold)
:   Set a VRAM threshold for allowing/blocking D3Cold

**Parameters**

`struct xe_device *xe`
:   xe device instance

`u32 threshold`
:   VRAM size in MiB for the D3cold threshold

**Return**

* 0 - success
* `-EINVAL`
  :   + invalid argument

void xe\_pm\_d3cold\_allowed\_toggle(struct xe\_device \*xe)
:   Check conditions to toggle d3cold.allowed

**Parameters**

`struct xe_device *xe`
:   xe device instance

**Description**

To be called during runtime\_pm idle callback.
Check for all the D3Cold conditions ahead of runtime suspend.

int xe\_pm\_module\_init(void)
:   Perform xe\_pm specific module initialization.

**Parameters**

`void`
:   no arguments

**Return**

0 on success. Currently doesn’t fail.
