# 4.The MSI Driver Guide HOWTO

> 출처(원문): https://docs.kernel.org/PCI/msi-howto.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4. The MSI Driver Guide HOWTO

Authors:
:   Tom L Nguyen; Martine Silbermann; Matthew Wilcox

Copyright:
:   2003, 2008 Intel Corporation

## 4.1. About this guide

This guide describes the basics of Message Signaled Interrupts (MSIs),
the advantages of using MSI over traditional interrupt mechanisms, how
to change your driver to use MSI or MSI-X and some basic diagnostics to
try if a device doesn’t support MSIs.

## 4.2. What are MSIs?

A Message Signaled Interrupt is a write from the device to a special
address which causes an interrupt to be received by the CPU.

The MSI capability was first specified in PCI 2.2 and was later enhanced
in PCI 3.0 to allow each interrupt to be masked individually. The MSI-X
capability was also introduced with PCI 3.0. It supports more interrupts
per device than MSI and allows interrupts to be independently configured.

Devices may support both MSI and MSI-X, but only one can be enabled at
a time.

## 4.3. Why use MSIs?

There are three reasons why using MSIs can give an advantage over
traditional pin-based interrupts.

Pin-based PCI interrupts are often shared amongst several devices.
To support this, the kernel must call each interrupt handler associated
with an interrupt, which leads to reduced performance for the system as
a whole. MSIs are never shared, so this problem cannot arise.

When a device writes data to memory, then raises a pin-based interrupt,
it is possible that the interrupt may arrive before all the data has
arrived in memory (this becomes more likely with devices behind PCI-PCI
bridges). In order to ensure that all the data has arrived in memory,
the interrupt handler must read a register on the device which raised
the interrupt. PCI transaction ordering rules require that all the data
arrive in memory before the value may be returned from the register.
Using MSIs avoids this problem as the interrupt-generating write cannot
pass the data writes, so by the time the interrupt is raised, the driver
knows that all the data has arrived in memory.

PCI devices can only support a single pin-based interrupt per function.
Often drivers have to query the device to find out what event has
occurred, slowing down interrupt handling for the common case. With
MSIs, a device can support more interrupts, allowing each interrupt
to be specialised to a different purpose. One possible design gives
infrequent conditions (such as errors) their own interrupt which allows
the driver to handle the normal interrupt handling path more efficiently.
Other possible designs include giving one interrupt to each packet queue
in a network card or each port in a storage controller.

## 4.4. How to use MSIs

PCI devices are initialised to use pin-based interrupts. The device
driver has to set up the device to use MSI or MSI-X. Not all machines
support MSIs correctly, and for those machines, the APIs described below
will simply fail and the device will continue to use pin-based interrupts.

### 4.4.1. Include kernel support for MSIs

To support MSI or MSI-X, the kernel must be built with the CONFIG\_PCI\_MSI
option enabled. This option is only available on some architectures,
and it may depend on some other options also being set. For example,
on x86, you must also enable X86\_UP\_APIC or SMP in order to see the
CONFIG\_PCI\_MSI option.

### 4.4.2. Using MSI

Most of the hard work is done for the driver in the PCI layer. The driver
simply has to request that the PCI layer set up the MSI capability for this
device.

To automatically use MSI or MSI-X interrupt vectors, use the following
function:

```
int pci_alloc_irq_vectors(struct pci_dev *dev, unsigned int min_vecs,
              unsigned int max_vecs, unsigned int flags);
```

which allocates up to max\_vecs interrupt vectors for a PCI device. It
returns the number of vectors allocated or a negative error. If the device
has a requirement for a minimum number of vectors the driver can pass a
min\_vecs argument set to this limit, and the PCI core will return -ENOSPC
if it can’t meet the minimum number of vectors.

The flags argument is used to specify which type of interrupt can be used
by the device and the driver (PCI\_IRQ\_INTX, PCI\_IRQ\_MSI, PCI\_IRQ\_MSIX).
A convenient short-hand (PCI\_IRQ\_ALL\_TYPES) is also available to ask for
any possible kind of interrupt. If the PCI\_IRQ\_AFFINITY flag is set,
[`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") will spread the interrupts around the available CPUs.

To get the Linux IRQ numbers passed to [`request_irq()`](../core-api/genericirq.html#c.request_irq "request_irq") and [`free_irq()`](../core-api/genericirq.html#c.free_irq "free_irq") and the
vectors, use the following function:

```
int pci_irq_vector(struct pci_dev *dev, unsigned int nr);
```

If the driver enables the device using [`pcim_enable_device()`](../driver-api/pci/pci.html#c.pcim_enable_device "pcim_enable_device"), the driver
shouldn’t call [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") because [`pcim_enable_device()`](../driver-api/pci/pci.html#c.pcim_enable_device "pcim_enable_device")
activates automatic management for IRQ vectors. Otherwise, the driver should
free any allocated IRQ vectors before removing the device using the following
function:

```
void pci_free_irq_vectors(struct pci_dev *dev);
```

If a device supports both MSI-X and MSI capabilities, this API will use the
MSI-X facilities in preference to the MSI facilities. MSI-X supports any
number of interrupts between 1 and 2048. In contrast, MSI is restricted to
a maximum of 32 interrupts (and must be a power of two). In addition, the
MSI interrupt vectors must be allocated consecutively, so the system might
not be able to allocate as many vectors for MSI as it could for MSI-X. On
some platforms, MSI interrupts must all be targeted at the same set of CPUs
whereas MSI-X interrupts can all be targeted at different CPUs.

If a device supports neither MSI-X nor MSI it will fall back to a single
legacy IRQ vector.

The typical usage of MSI or MSI-X interrupts is to allocate as many vectors
as possible, likely up to the limit supported by the device. If nvec is
larger than the number supported by the device it will automatically be
capped to the supported limit, so there is no need to query the number of
vectors supported beforehand:

```
nvec = pci_alloc_irq_vectors(pdev, 1, nvec, PCI_IRQ_ALL_TYPES)
if (nvec < 0)
        goto out_err;
```

If a driver is unable or unwilling to deal with a variable number of MSI
interrupts it can request a particular number of interrupts by passing that
number to [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") function as both ‘min\_vecs’ and
‘max\_vecs’ parameters:

```
ret = pci_alloc_irq_vectors(pdev, nvec, nvec, PCI_IRQ_ALL_TYPES);
if (ret < 0)
        goto out_err;
```

The most notorious example of the request type described above is enabling
the single MSI mode for a device. It could be done by passing two 1s as
‘min\_vecs’ and ‘max\_vecs’:

```
ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
if (ret < 0)
        goto out_err;
```

Some devices might not support using legacy line interrupts, in which case
the driver can specify that only MSI or MSI-X is acceptable:

```
nvec = pci_alloc_irq_vectors(pdev, 1, nvec, PCI_IRQ_MSI | PCI_IRQ_MSIX);
if (nvec < 0)
        goto out_err;
```

### 4.4.3. Legacy APIs

The following old APIs to enable and disable MSI or MSI-X interrupts should
not be used in new code:

```
pci_enable_msi()              /* deprecated */
pci_disable_msi()             /* deprecated */
pci_enable_msix_range()       /* deprecated */
pci_enable_msix_exact()       /* deprecated */
pci_disable_msix()            /* deprecated */
```

Additionally there are APIs to provide the number of supported MSI or MSI-X
vectors: [`pci_msi_vec_count()`](../driver-api/pci/pci.html#c.pci_msi_vec_count "pci_msi_vec_count") and [`pci_msix_vec_count()`](#c.pci_msix_vec_count "pci_msix_vec_count"). In general these
should be avoided in favor of letting [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") cap the
number of vectors. If you have a legitimate special use case for the count
of vectors we might have to revisit that decision and add a
`pci_nr_irq_vectors()` helper that handles MSI and MSI-X transparently.

### 4.4.4. Considerations when using MSIs

#### 4.4.4.1. Spinlocks

Most device drivers have a per-device spinlock which is taken in the
interrupt handler. With pin-based interrupts or a single MSI, it is not
necessary to disable interrupts (Linux guarantees the same interrupt will
not be re-entered). If a device uses multiple interrupts, the driver
must disable interrupts while the lock is held. If the device sends
a different interrupt, the driver will deadlock trying to recursively
acquire the spinlock. Such deadlocks can be avoided by using
`spin_lock_irqsave()` or `spin_lock_irq()` which disable local interrupts
and acquire the lock (see [Unreliable Guide To Locking](../kernel-hacking/locking.html)).

### 4.4.5. How to tell whether MSI/MSI-X is enabled on a device

Using ‘lspci -v’ (as root) may show some devices with “MSI”, “Message
Signaled Interrupts” or “MSI-X” capabilities. Each of these capabilities
has an ‘Enable’ flag which is followed with either “+” (enabled)
or “-” (disabled).

## 4.5. MSI quirks

Several PCI chipsets or devices are known not to support MSIs.
The PCI stack provides three ways to disable MSIs:

1. globally
2. on all devices behind a specific bridge
3. on a single device

### 4.5.1. Disabling MSIs globally

Some host chipsets simply don’t support MSIs properly. If we’re
lucky, the manufacturer knows this and has indicated it in the ACPI
FADT table. In this case, Linux automatically disables MSIs.
Some boards don’t include this information in the table and so we have
to detect them ourselves. The complete list of these is found near the
`quirk_disable_all_msi()` function in drivers/pci/quirks.c.

If you have a board which has problems with MSIs, you can pass pci=nomsi
on the kernel command line to disable MSIs on all devices. It would be
in your best interests to report the problem to [linux-pci@vger.kernel.org](mailto:linux-pci%40vger.kernel.org)
including a full ‘lspci -v’ so we can add the quirks to the kernel.

### 4.5.2. Disabling MSIs below a bridge

Some PCI bridges are not able to route MSIs between buses properly.
In this case, MSIs must be disabled on all devices behind the bridge.

Some bridges allow you to enable MSIs by changing some bits in their
PCI configuration space (especially the Hypertransport chipsets such
as the nVidia nForce and Serverworks HT2000). As with host chipsets,
Linux mostly knows about them and automatically enables MSIs if it can.
If you have a bridge unknown to Linux, you can enable
MSIs in configuration space using whatever method you know works, then
enable MSIs on that bridge by doing:

```
echo 1 > /sys/bus/pci/devices/$bridge/msi_bus
```

where $bridge is the PCI address of the bridge you’ve enabled (eg
0000:00:0e.0).

To disable MSIs, echo 0 instead of 1. Changing this value should be
done with caution as it could break interrupt handling for all devices
below this bridge.

Again, please notify [linux-pci@vger.kernel.org](mailto:linux-pci%40vger.kernel.org) of any bridges that need
special handling.

### 4.5.3. Disabling MSIs on a single device

Some devices are known to have faulty MSI implementations. Usually this
is handled in the individual device driver, but occasionally it’s necessary
to handle this with a quirk. Some drivers have an option to disable use
of MSI. While this is a convenient workaround for the driver author,
it is not good practice, and should not be emulated.

### 4.5.4. Finding why MSIs are disabled on a device

From the above three sections, you can see that there are many reasons
why MSIs may not be enabled for a given device. Your first step should
be to examine your dmesg carefully to determine whether MSIs are enabled
for your machine. You should also check your .config to be sure you
have enabled CONFIG\_PCI\_MSI.

Then, ‘lspci -t’ gives the list of bridges above a device. Reading
/sys/bus/pci/devices/\*/msi\_bus will tell you whether MSIs are enabled (1)
or disabled (0). If 0 is found in any of the msi\_bus files belonging
to bridges between the PCI root and the device, MSIs are disabled.

It is also worth checking the device driver to see whether it supports MSIs.
For example, it may contain calls to [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") with the
PCI\_IRQ\_MSI or PCI\_IRQ\_MSIX flags.

## 4.6. List of device drivers MSI(-X) APIs

The PCI/MSI subsystem has a dedicated C file for its exported device driver
APIs — drivers/pci/msi/api.c. The following functions are exported:

int pci\_enable\_msi(struct pci\_dev \*dev)
:   Enable MSI interrupt mode on device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Legacy device driver API to enable MSI interrupts mode on device and
allocate a single interrupt vector. On success, the allocated vector
Linux IRQ will be saved at **dev->irq**. The driver must invoke
[`pci_disable_msi()`](#c.pci_disable_msi "pci_disable_msi") on cleanup.

**NOTE**

The newer [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") / [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") API
pair should, in general, be used instead.

**Return**

0 on success, errno otherwise

void pci\_disable\_msi(struct pci\_dev \*dev)
:   Disable MSI interrupt mode on device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Legacy device driver API to disable MSI interrupt mode on device,
free earlier allocated interrupt vectors, and restore INTx emulation.
The PCI device Linux IRQ (**dev->irq**) is restored to its default
pin-assertion IRQ. This is the cleanup pair of [`pci_enable_msi()`](#c.pci_enable_msi "pci_enable_msi").

**NOTE**

The newer [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") / [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") API
pair should, in general, be used instead.

int pci\_msix\_vec\_count(struct pci\_dev \*dev)
:   Get number of MSI-X interrupt vectors on device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Return**

number of MSI-X interrupt vectors available on this device
(i.e., the device’s MSI-X capability structure “table size”), -EINVAL
if the device is not MSI-X capable, other errnos otherwise.

int pci\_enable\_msix\_range(struct pci\_dev \*dev, struct msix\_entry \*entries, int minvec, int maxvec)
:   Enable MSI-X interrupt mode on device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

`struct msix_entry *entries`
:   input/output parameter, array of MSI-X configuration entries

`int minvec`
:   minimum required number of MSI-X vectors

`int maxvec`
:   maximum desired number of MSI-X vectors

**Description**

Legacy device driver API to enable MSI-X interrupt mode on device and
configure its MSI-X capability structure as appropriate. The passed
**entries** array must have each of its members “entry” field set to a
desired (valid) MSI-X vector number, where the range of valid MSI-X
vector numbers can be queried through [`pci_msix_vec_count()`](#c.pci_msix_vec_count "pci_msix_vec_count"). If
successful, the driver must invoke [`pci_disable_msix()`](#c.pci_disable_msix "pci_disable_msix") on cleanup.

**NOTE**

The newer [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") / [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") API
pair should, in general, be used instead.

**Return**

number of MSI-X vectors allocated (which might be smaller
than **maxvecs**), where Linux IRQ numbers for such allocated vectors
are saved back in the **entries** array elements’ “vector” field. Return
-ENOSPC if less than **minvecs** interrupt vectors are available.
Return -EINVAL if one of the passed **entries** members “entry” field
was invalid or a duplicate, or if plain MSI interrupts mode was
earlier enabled on device. Return other errnos otherwise.

bool pci\_msix\_can\_alloc\_dyn(struct pci\_dev \*dev)
:   Query whether dynamic allocation after enabling MSI-X is supported

**Parameters**

`struct pci_dev *dev`
:   PCI device to operate on

**Return**

True if supported, false otherwise

struct msi\_map pci\_msix\_alloc\_irq\_at(struct pci\_dev \*dev, unsigned int index, const struct [irq\_affinity\_desc](../core-api/genericirq.html#c.irq_affinity_desc "irq_affinity_desc") \*affdesc)
:   Allocate an MSI-X interrupt after enabling MSI-X at a given MSI-X vector index or any free vector index

**Parameters**

`struct pci_dev *dev`
:   PCI device to operate on

`unsigned int index`
:   Index to allocate. If **index** == MSI\_ANY\_INDEX this allocates
    the next free index in the MSI-X table

`const struct irq_affinity_desc *affdesc`
:   Optional pointer to an affinity descriptor structure. NULL otherwise

**Return**

A `struct msi_map`

**Description**

> On success msi\_map::index contains the allocated index (>= 0) and
> msi\_map::virq contains the allocated Linux interrupt number (> 0).
>
> On fail msi\_map::index contains the error code and msi\_map::virq
> is set to 0.

void pci\_msix\_free\_irq(struct pci\_dev \*dev, struct msi\_map map)
:   Free an interrupt on a PCI/MSI-X interrupt domain

**Parameters**

`struct pci_dev *dev`
:   The PCI device to operate on

`struct msi_map map`
:   A `struct msi_map` describing the interrupt to free

**Description**

Undo an interrupt vector allocation. Does not disable MSI-X.

void pci\_disable\_msix(struct pci\_dev \*dev)
:   Disable MSI-X interrupt mode on device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Legacy device driver API to disable MSI-X interrupt mode on device,
free earlier-allocated interrupt vectors, and restore INTx.
The PCI device Linux IRQ (**dev->irq**) is restored to its default pin
assertion IRQ. This is the cleanup pair of [`pci_enable_msix_range()`](#c.pci_enable_msix_range "pci_enable_msix_range").

**NOTE**

The newer [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") / [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") API
pair should, in general, be used instead.

int pci\_alloc\_irq\_vectors(struct pci\_dev \*dev, unsigned int min\_vecs, unsigned int max\_vecs, unsigned int flags)
:   Allocate multiple device interrupt vectors

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

`unsigned int min_vecs`
:   minimum required number of vectors (must be >= 1)

`unsigned int max_vecs`
:   maximum desired number of vectors

`unsigned int flags`
:   One or more of:

**Description**

> * `PCI_IRQ_MSIX` Allow trying MSI-X vector allocations
> * `PCI_IRQ_MSI` Allow trying MSI vector allocations
> * `PCI_IRQ_INTX` Allow trying INTx interrupts, if and
>   only if **min\_vecs** == 1
> * `PCI_IRQ_AFFINITY` Auto-manage IRQs affinity by spreading
>   the vectors around available CPUs

Allocate up to **max\_vecs** interrupt vectors on device. MSI-X irq
vector allocation has a higher precedence over plain MSI, which has a
higher precedence over legacy INTx emulation.

Upon a successful allocation, the caller should use [`pci_irq_vector()`](#c.pci_irq_vector "pci_irq_vector")
to get the Linux IRQ number to be passed to [`request_threaded_irq()`](../core-api/genericirq.html#c.request_threaded_irq "request_threaded_irq").
The driver must call [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") on cleanup.

**Return**

number of allocated vectors (which might be smaller than
**max\_vecs**), -ENOSPC if less than **min\_vecs** interrupt vectors are
available, other errnos otherwise.

int pci\_alloc\_irq\_vectors\_affinity(struct pci\_dev \*dev, unsigned int min\_vecs, unsigned int max\_vecs, unsigned int flags, struct [irq\_affinity](../core-api/genericirq.html#c.irq_affinity "irq_affinity") \*affd)
:   Allocate multiple device interrupt vectors with affinity requirements

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

`unsigned int min_vecs`
:   minimum required number of vectors (must be >= 1)

`unsigned int max_vecs`
:   maximum desired number of vectors

`unsigned int flags`
:   allocation flags, as in [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors")

`struct irq_affinity *affd`
:   affinity requirements (can be `NULL`).

**Description**

Same as [`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors"), but with the extra **affd** parameter.
Check that function docs, and [`struct irq_affinity`](../core-api/genericirq.html#c.irq_affinity "irq_affinity"), for more details.

int pci\_irq\_vector(struct pci\_dev \*dev, unsigned int nr)
:   Get Linux IRQ number of a device interrupt vector

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

`unsigned int nr`
:   device-relative interrupt vector index (0-based); has different
    meanings, depending on interrupt mode:

**Description**

> * MSI-X the index in the MSI-X vector table
> * MSI the index of the enabled MSI vectors
> * INTx must be 0

**Return**

the Linux IRQ number, or -EINVAL if **nr** is out of range

const struct cpumask \*pci\_irq\_get\_affinity(struct pci\_dev \*dev, int nr)
:   Get a device interrupt vector affinity

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

`int nr`
:   device-relative interrupt vector index (0-based); has different
    meanings, depending on interrupt mode:

**Description**

> * MSI-X the index in the MSI-X vector table
> * MSI the index of the enabled MSI vectors
> * INTx must be 0

**Return**

MSI/MSI-X vector affinity, NULL if **nr** is out of range or if
the MSI(-X) vector was allocated without explicit affinity
requirements (e.g., by [`pci_enable_msi()`](#c.pci_enable_msi "pci_enable_msi"), [`pci_enable_msix_range()`](#c.pci_enable_msix_range "pci_enable_msix_range"), or
[`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors") without the `PCI_IRQ_AFFINITY` flag). Return a
generic set of CPU IDs representing all possible CPUs available
during system boot if the device is in legacy INTx mode.

void pci\_free\_irq\_vectors(struct pci\_dev \*dev)
:   Free previously allocated IRQs for a device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Undo the interrupt vector allocations and possible device MSI/MSI-X
enablement earlier done through [`pci_alloc_irq_vectors_affinity()`](#c.pci_alloc_irq_vectors_affinity "pci_alloc_irq_vectors_affinity") or
[`pci_alloc_irq_vectors()`](#c.pci_alloc_irq_vectors "pci_alloc_irq_vectors").

WARNING: Do not call this function if the device has been enabled
with [`pcim_enable_device()`](../driver-api/pci/pci.html#c.pcim_enable_device "pcim_enable_device"). In that case, IRQ vectors are automatically
managed via `pcim_msi_release()` and calling [`pci_free_irq_vectors()`](#c.pci_free_irq_vectors "pci_free_irq_vectors") can
lead to double-free issues.

void pci\_restore\_msi\_state(struct pci\_dev \*dev)
:   Restore cached MSI(-X) state on device

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Write the Linux-cached MSI(-X) state back on device. This is
typically useful upon system resume, or after an error-recovery PCI
adapter reset.

bool pci\_msi\_enabled(void)
:   Are MSI(-X) interrupts enabled system-wide?

**Parameters**

`void`
:   no arguments

**Return**

true if MSI has not been globally disabled through ACPI FADT,
PCI bridge quirks, or the “pci=nomsi” kernel command-line option.
