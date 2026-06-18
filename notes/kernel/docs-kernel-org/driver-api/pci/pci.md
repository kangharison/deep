# PCI Support Library

> 출처(원문): https://docs.kernel.org/driver-api/pci/pci.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# PCI Support Library

unsigned char pci\_bus\_max\_busnr(struct pci\_bus \*bus)
:   returns maximum PCI bus number of given bus’ children

**Parameters**

`struct pci_bus *bus`
:   pointer to PCI bus structure to search

**Description**

Given a PCI bus, returns the highest PCI bus number present in the set
including the given PCI bus and its list of child PCI buses.

int pci\_status\_get\_and\_clear\_errors(struct pci\_dev \*pdev)
:   return and clear error bits in PCI\_STATUS

**Parameters**

`struct pci_dev *pdev`
:   the PCI device

**Description**

Returns error bits set in PCI\_STATUS and clears them.

u8 pci\_find\_capability(struct pci\_dev \*dev, int cap)
:   query for devices’ capabilities

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`int cap`
:   capability code

**Description**

Tell if a device supports a given PCI capability.
Returns the address of the requested capability structure within the
device’s PCI configuration space or 0 in case the device does not
support it. Possible values for **cap** include:

> `PCI_CAP_ID_PM` Power Management
> `PCI_CAP_ID_AGP` Accelerated Graphics Port
> `PCI_CAP_ID_VPD` Vital Product Data
> `PCI_CAP_ID_SLOTID` Slot Identification
> `PCI_CAP_ID_MSI` Message Signalled Interrupts
> `PCI_CAP_ID_CHSWP` CompactPCI HotSwap
> `PCI_CAP_ID_PCIX` PCI-X
> `PCI_CAP_ID_EXP` PCI Express

u8 pci\_bus\_find\_capability(struct pci\_bus \*bus, unsigned int devfn, int cap)
:   query for devices’ capabilities

**Parameters**

`struct pci_bus *bus`
:   the PCI bus to query

`unsigned int devfn`
:   PCI device to query

`int cap`
:   capability code

**Description**

Like [`pci_find_capability()`](#c.pci_find_capability "pci_find_capability") but works for PCI devices that do not have a
pci\_dev structure set up yet.

Returns the address of the requested capability structure within the
device’s PCI configuration space or 0 in case the device does not
support it.

u16 pci\_find\_next\_ext\_capability(struct pci\_dev \*dev, u16 start, int cap)
:   Find an extended capability

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`u16 start`
:   address at which to start looking (0 to start at beginning of list)

`int cap`
:   capability code

**Description**

Returns the address of the next matching extended capability structure
within the device’s PCI configuration space or 0 if the device does
not support it. Some capabilities can occur several times, e.g., the
vendor-specific capability, and this provides a way to find them all.

u16 pci\_find\_ext\_capability(struct pci\_dev \*dev, int cap)
:   Find an extended capability

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`int cap`
:   capability code

**Description**

Returns the address of the requested extended capability structure
within the device’s PCI configuration space or 0 if the device does
not support it. Possible values for **cap** include:

> `PCI_EXT_CAP_ID_ERR` Advanced Error Reporting
> `PCI_EXT_CAP_ID_VC` Virtual Channel
> `PCI_EXT_CAP_ID_DSN` Device Serial Number
> `PCI_EXT_CAP_ID_PWR` Power Budgeting

u64 pci\_get\_dsn(struct pci\_dev \*dev)
:   Read and return the 8-byte Device Serial Number

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Looks up the PCI\_EXT\_CAP\_ID\_DSN and reads the 8 bytes of the Device Serial
Number.

Returns the DSN, or zero if the capability does not exist.

u8 pci\_find\_next\_ht\_capability(struct pci\_dev \*dev, u8 pos, int ht\_cap)
:   query a device’s HyperTransport capabilities

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`u8 pos`
:   Position from which to continue searching

`int ht_cap`
:   HyperTransport capability code

**Description**

To be used in conjunction with [`pci_find_ht_capability()`](#c.pci_find_ht_capability "pci_find_ht_capability") to search for
all capabilities matching **ht\_cap**. **pos** should always be a value returned
from [`pci_find_ht_capability()`](#c.pci_find_ht_capability "pci_find_ht_capability").

NB. To be 100% safe against broken PCI devices, the caller should take
steps to avoid an infinite loop.

u8 pci\_find\_ht\_capability(struct pci\_dev \*dev, int ht\_cap)
:   query a device’s HyperTransport capabilities

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`int ht_cap`
:   HyperTransport capability code

**Description**

Tell if a device supports a given HyperTransport capability.
Returns an address within the device’s PCI configuration space
or 0 in case the device does not support the request capability.
The address points to the PCI capability, of type PCI\_CAP\_ID\_HT,
which has a HyperTransport capability matching **ht\_cap**.

u16 pci\_find\_vsec\_capability(struct pci\_dev \*dev, u16 vendor, int cap)
:   Find a vendor-specific extended capability

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`u16 vendor`
:   Vendor ID for which capability is defined

`int cap`
:   Vendor-specific capability ID

**Description**

If **dev** has Vendor ID **vendor**, search for a VSEC capability with
VSEC ID **cap**. If found, return the capability offset in
config space; otherwise return 0.

u16 pci\_find\_dvsec\_capability(struct pci\_dev \*dev, u16 vendor, u16 dvsec)
:   Find DVSEC for vendor

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`u16 vendor`
:   Vendor ID to match for the DVSEC

`u16 dvsec`
:   Designated Vendor-specific capability ID

**Description**

If DVSEC has Vendor ID **vendor** and DVSEC ID **dvsec** return the capability
offset in config space; otherwise return 0.

struct resource \*pci\_find\_parent\_resource(const struct pci\_dev \*dev, struct resource \*res)
:   return resource region of parent bus of given region

**Parameters**

`const struct pci_dev *dev`
:   PCI device structure contains resources to be searched

`struct resource *res`
:   child resource record for which parent is sought

**Description**

For given resource region of given device, return the resource region of
parent bus the given region is contained in.

struct resource \*pci\_find\_resource(struct pci\_dev \*dev, struct resource \*res)
:   Return matching PCI device resource

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`struct resource *res`
:   Resource to look for

**Description**

Goes over standard PCI resources (BARs) and checks if the given resource
is partially or fully contained in any of them. In that case the
matching resource is returned, `NULL` otherwise.

int pci\_platform\_power\_transition(struct pci\_dev \*dev, pci\_power\_t state)
:   Use platform to change device power state

**Parameters**

`struct pci_dev *dev`
:   PCI device to handle.

`pci_power_t state`
:   State to put the device into.

int pci\_set\_power\_state(struct pci\_dev \*dev, pci\_power\_t state)
:   Set the power state of a PCI device

**Parameters**

`struct pci_dev *dev`
:   PCI device to handle.

`pci_power_t state`
:   PCI power state (D0, D1, D2, D3hot) to put the device into.

**Description**

Transition a device to a new power state, using the platform firmware and/or
the device’s PCI PM registers.

RETURN VALUE:
-EINVAL if the requested state is invalid.
-EIO if device does not support PCI PM or its PM capabilities register has a
wrong version, or device doesn’t support the requested state.
0 if the transition is to D1 or D2 but D1 and D2 are not supported.
0 if device already is in the requested state.
0 if the transition is to D3 but D3 is not supported.
0 if device’s power state has been successfully changed.

int pci\_save\_state(struct pci\_dev \*dev)
:   save the PCI configuration space of a device before suspending

**Parameters**

`struct pci_dev *dev`
:   PCI device that we’re dealing with

void pci\_restore\_state(struct pci\_dev \*dev)
:   Restore the saved state of a PCI device

**Parameters**

`struct pci_dev *dev`
:   PCI device that we’re dealing with

struct pci\_saved\_state \*pci\_store\_saved\_state(struct pci\_dev \*dev)
:   Allocate and return an opaque `struct containing` the device saved state.

**Parameters**

`struct pci_dev *dev`
:   PCI device that we’re dealing with

**Description**

Return NULL if no state or error.

int pci\_load\_saved\_state(struct pci\_dev \*dev, struct pci\_saved\_state \*state)
:   Reload the provided save state into `struct pci_dev`.

**Parameters**

`struct pci_dev *dev`
:   PCI device that we’re dealing with

`struct pci_saved_state *state`
:   Saved state returned from [`pci_store_saved_state()`](#c.pci_store_saved_state "pci_store_saved_state")

int pci\_load\_and\_free\_saved\_state(struct pci\_dev \*dev, struct pci\_saved\_state \*\*state)
:   Reload the save state pointed to by state, and free the memory allocated for it.

**Parameters**

`struct pci_dev *dev`
:   PCI device that we’re dealing with

`struct pci_saved_state **state`
:   Pointer to saved state returned from [`pci_store_saved_state()`](#c.pci_store_saved_state "pci_store_saved_state")

int pci\_reenable\_device(struct pci\_dev \*dev)
:   Resume abandoned device

**Parameters**

`struct pci_dev *dev`
:   PCI device to be resumed

**NOTE**

This function is a backend of `pci_default_resume()` and is not supposed
to be called by normal code, write proper resume handler and use it instead.

int pci\_enable\_device\_mem(struct pci\_dev \*dev)
:   Initialize a device for use with Memory space

**Parameters**

`struct pci_dev *dev`
:   PCI device to be initialized

**Description**

Initialize device before it’s used by a driver. Ask low-level code
to enable Memory resources. Wake up the device if it was suspended.
Beware, this function can fail.

int pci\_enable\_device(struct pci\_dev \*dev)
:   Initialize device before it’s used by a driver.

**Parameters**

`struct pci_dev *dev`
:   PCI device to be initialized

**Description**

Initialize device before it’s used by a driver. Ask low-level code
to enable I/O and memory. Wake up the device if it was suspended.
Beware, this function can fail.

Note we don’t actually enable the device many times if we call
this function repeatedly (we just increment the count).

void pci\_disable\_device(struct pci\_dev \*dev)
:   Disable PCI device after use

**Parameters**

`struct pci_dev *dev`
:   PCI device to be disabled

**Description**

Signal to the system that the PCI device is not in use by the system
anymore. This only involves disabling PCI bus-mastering, if active.

Note we don’t actually disable the device until all callers of
[`pci_enable_device()`](#c.pci_enable_device "pci_enable_device") have called [`pci_disable_device()`](#c.pci_disable_device "pci_disable_device").

int pci\_set\_pcie\_reset\_state(struct pci\_dev \*dev, enum pcie\_reset\_state state)
:   set reset state for device dev

**Parameters**

`struct pci_dev *dev`
:   the PCIe device reset

`enum pcie_reset_state state`
:   Reset state to enter into

**Description**

Sets the PCI reset state for the device.

bool pci\_pme\_capable(struct pci\_dev \*dev, pci\_power\_t state)
:   check the capability of PCI device to generate PME#

**Parameters**

`struct pci_dev *dev`
:   PCI device to handle.

`pci_power_t state`
:   PCI state from which device will issue PME#.

void pci\_pme\_active(struct pci\_dev \*dev, bool enable)
:   enable or disable PCI device’s PME# function

**Parameters**

`struct pci_dev *dev`
:   PCI device to handle.

`bool enable`
:   ‘true’ to enable PME# generation; ‘false’ to disable it.

**Description**

The caller must verify that the device is capable of generating PME# before
calling this function with **enable** equal to ‘true’.

int pci\_enable\_wake(struct [pci\_dev](#c.pci_enable_wake "pci_dev") \*pci\_dev, pci\_power\_t state, bool enable)
:   change wakeup settings for a PCI device

**Parameters**

`struct pci_dev *pci_dev`
:   Target device

`pci_power_t state`
:   PCI state from which device will issue wakeup events

`bool enable`
:   Whether or not to enable event generation

**Description**

If **enable** is set, check `device_may_wakeup()` for the device before calling
`__pci_enable_wake()` for it.

int pci\_wake\_from\_d3(struct pci\_dev \*dev, bool enable)
:   enable/disable device to wake up from D3\_hot or D3\_cold

**Parameters**

`struct pci_dev *dev`
:   PCI device to prepare

`bool enable`
:   True to enable wake-up event generation; false to disable

**Description**

Many drivers want the device to wake up the system from D3\_hot or D3\_cold
and this function allows them to set that up cleanly - [`pci_enable_wake()`](#c.pci_enable_wake "pci_enable_wake")
should not be called twice in a row to enable wake-up due to PCI PM vs ACPI
ordering constraints.

This function only returns error code if the device is not allowed to wake
up the system from sleep or it is not capable of generating PME# from both
D3\_hot and D3\_cold and the platform is unable to enable wake-up power for it.

int pci\_prepare\_to\_sleep(struct pci\_dev \*dev)
:   prepare PCI device for system-wide transition into a sleep state

**Parameters**

`struct pci_dev *dev`
:   Device to handle.

**Description**

Choose the power state appropriate for the device depending on whether
it can wake up the system and/or is power manageable by the platform
(PCI\_D3hot is the default) and put the device into that state.

int pci\_back\_from\_sleep(struct pci\_dev \*dev)
:   turn PCI device on during system-wide transition into working state

**Parameters**

`struct pci_dev *dev`
:   Device to handle.

**Description**

Disable device’s system wake-up capability and put it into D0.

bool pci\_dev\_run\_wake(struct pci\_dev \*dev)
:   Check if device can generate run-time wake-up events.

**Parameters**

`struct pci_dev *dev`
:   Device to check.

**Description**

Return true if the device itself is capable of generating wake-up events
(through the platform or using the native PCIe PME) or if the device supports
PME and one of its upstream bridges can generate wake-up events.

pci\_power\_t pci\_choose\_state(struct pci\_dev \*dev, pm\_message\_t state)
:   Choose the power state of a PCI device.

**Parameters**

`struct pci_dev *dev`
:   Target PCI device.

`pm_message_t state`
:   Target state for the whole system.

**Description**

Returns PCI power state suitable for **dev** and **state**.

void pci\_d3cold\_enable(struct pci\_dev \*dev)
:   Enable D3cold for device

**Parameters**

`struct pci_dev *dev`
:   PCI device to handle

**Description**

This function can be used in drivers to enable D3cold from the device
they handle. It also updates upstream PCI bridge PM capabilities
accordingly.

void pci\_d3cold\_disable(struct pci\_dev \*dev)
:   Disable D3cold for device

**Parameters**

`struct pci_dev *dev`
:   PCI device to handle

**Description**

This function can be used in drivers to disable D3cold from the device
they handle. It also updates upstream PCI bridge PM capabilities
accordingly.

int pci\_enable\_atomic\_ops\_to\_root(struct pci\_dev \*dev, u32 cap\_mask)
:   enable AtomicOp requests to root port

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`u32 cap_mask`
:   mask of desired AtomicOp sizes, including one or more of:
    PCI\_EXP\_DEVCAP2\_ATOMIC\_COMP32
    PCI\_EXP\_DEVCAP2\_ATOMIC\_COMP64
    PCI\_EXP\_DEVCAP2\_ATOMIC\_COMP128

**Description**

Return 0 if all upstream bridges support AtomicOp routing, egress
blocking is disabled on all upstream ports, and the root port supports
the requested completion capabilities (32-bit, 64-bit and/or 128-bit
AtomicOp completion), or negative otherwise.

void pci\_release\_region(struct pci\_dev \*pdev, int bar)
:   Release a PCI bar

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources were previously reserved by
    [`pci_request_region()`](#c.pci_request_region "pci_request_region")

`int bar`
:   BAR to release

**Description**

Releases the PCI I/O and memory resources previously reserved by a
successful call to [`pci_request_region()`](#c.pci_request_region "pci_request_region"). Call this function only
after all use of the PCI regions has ceased.

int pci\_request\_region(struct pci\_dev \*pdev, int bar, const char \*name)
:   Reserve PCI I/O and memory resource

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources are to be reserved

`int bar`
:   BAR to be reserved

`const char *name`
:   name of the driver requesting the resource

**Return**

0 on success, negative error code on failure.

**Description**

Mark the PCI region associated with PCI device **pdev** BAR **bar** as being
reserved by owner **name**. Do not access any address inside the PCI regions
unless this call returns successfully.

Returns 0 on success, or `EBUSY` on error. A warning
message is also printed on failure.

void pci\_release\_selected\_regions(struct pci\_dev \*pdev, int bars)
:   Release selected PCI I/O and memory resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources were previously reserved

`int bars`
:   Bitmask of BARs to be released

**Description**

Release selected PCI I/O and memory resources previously reserved.
Call this function only after all use of the PCI regions has ceased.

int pci\_request\_selected\_regions(struct pci\_dev \*pdev, int bars, const char \*name)
:   Reserve selected PCI I/O and memory resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources are to be reserved

`int bars`
:   Bitmask of BARs to be requested

`const char *name`
:   Name of the driver requesting the resources

**Return**

0 on success, negative error code on failure.

int pci\_request\_selected\_regions\_exclusive(struct pci\_dev \*pdev, int bars, const char \*name)
:   Request regions exclusively

**Parameters**

`struct pci_dev *pdev`
:   PCI device to request regions from

`int bars`
:   bit mask of BARs to request

`const char *name`
:   name of the driver requesting the resources

**Return**

0 on success, negative error code on failure.

void pci\_release\_regions(struct pci\_dev \*pdev)
:   Release reserved PCI I/O and memory resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources were previously reserved by
    [`pci_request_regions()`](#c.pci_request_regions "pci_request_regions")

**Description**

Releases all PCI I/O and memory resources previously reserved by a
successful call to [`pci_request_regions()`](#c.pci_request_regions "pci_request_regions"). Call this function only
after all use of the PCI regions has ceased.

int pci\_request\_regions(struct pci\_dev \*pdev, const char \*name)
:   Reserve PCI I/O and memory resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources are to be reserved

`const char *name`
:   name of the driver requesting the resources

**Description**

Mark all PCI regions associated with PCI device **pdev** as being reserved by
owner **name**. Do not access any address inside the PCI regions unless this
call returns successfully.

Returns 0 on success, or `EBUSY` on error. A warning
message is also printed on failure.

int pci\_request\_regions\_exclusive(struct pci\_dev \*pdev, const char \*name)
:   Reserve PCI I/O and memory resources

**Parameters**

`struct pci_dev *pdev`
:   PCI device whose resources are to be reserved

`const char *name`
:   name of the driver requesting the resources

**Return**

0 on success, negative error code on failure.

**Description**

Mark all PCI regions associated with PCI device **pdev** as being reserved
by owner **name**. Do not access any address inside the PCI regions
unless this call returns successfully.

[`pci_request_regions_exclusive()`](#c.pci_request_regions_exclusive "pci_request_regions_exclusive") will mark the region so that /dev/mem
and the sysfs MMIO access will not be allowed.

Returns 0 on success, or `EBUSY` on error. A warning message is also
printed on failure.

int pci\_remap\_iospace(const struct resource \*res, phys\_addr\_t phys\_addr)
:   Remap the memory mapped I/O space

**Parameters**

`const struct resource *res`
:   Resource describing the I/O space

`phys_addr_t phys_addr`
:   physical address of range to be mapped

**Description**

Remap the memory mapped I/O space described by the **res** and the CPU
physical address **phys\_addr** into virtual address space. Only
architectures that have memory mapped IO functions defined (and the
PCI\_IOBASE value defined) should call this function.

void pci\_unmap\_iospace(struct resource \*res)
:   Unmap the memory mapped I/O space

**Parameters**

`struct resource *res`
:   resource to be unmapped

**Description**

Unmap the CPU virtual address **res** from virtual address space. Only
architectures that have memory mapped IO functions defined (and the
PCI\_IOBASE value defined) should call this function.

void pci\_set\_master(struct pci\_dev \*dev)
:   enables bus-mastering for device dev

**Parameters**

`struct pci_dev *dev`
:   the PCI device to enable

**Description**

Enables bus-mastering on the device and calls `pcibios_set_master()`
to do the needed arch specific settings.

void pci\_clear\_master(struct pci\_dev \*dev)
:   disables bus-mastering for device dev

**Parameters**

`struct pci_dev *dev`
:   the PCI device to disable

int pci\_set\_cacheline\_size(struct pci\_dev \*dev)
:   ensure the CACHE\_LINE\_SIZE register is programmed

**Parameters**

`struct pci_dev *dev`
:   the PCI device for which MWI is to be enabled

**Description**

Helper function for pci\_set\_mwi.
Originally copied from drivers/net/acenic.c.
Copyright 1998-2001 by Jes Sorensen, <jes\*\*trained\*\*-monkey.org>.

**Return**

An appropriate -ERRNO error value on error, or zero for success.

int pci\_set\_mwi(struct pci\_dev \*dev)
:   enables memory-write-invalidate PCI transaction

**Parameters**

`struct pci_dev *dev`
:   the PCI device for which MWI is enabled

**Description**

Enables the Memory-Write-Invalidate transaction in `PCI_COMMAND`.

**Return**

An appropriate -ERRNO error value on error, or zero for success.

int pci\_try\_set\_mwi(struct pci\_dev \*dev)
:   enables memory-write-invalidate PCI transaction

**Parameters**

`struct pci_dev *dev`
:   the PCI device for which MWI is enabled

**Description**

Enables the Memory-Write-Invalidate transaction in `PCI_COMMAND`.
Callers are not required to check the return value.

**Return**

An appropriate -ERRNO error value on error, or zero for success.

void pci\_clear\_mwi(struct pci\_dev \*dev)
:   disables Memory-Write-Invalidate for device dev

**Parameters**

`struct pci_dev *dev`
:   the PCI device to disable

**Description**

Disables PCI Memory-Write-Invalidate transaction on the device

void pci\_intx(struct pci\_dev \*pdev, int enable)
:   enables/disables PCI INTx for device dev

**Parameters**

`struct pci_dev *pdev`
:   the PCI device to operate on

`int enable`
:   boolean: whether to enable or disable PCI INTx

**Description**

Enables/disables PCI INTx for device **pdev**

int pci\_wait\_for\_pending\_transaction(struct pci\_dev \*dev)
:   wait for pending transaction

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Return 0 if transaction is pending 1 otherwise.

int pcie\_flr(struct pci\_dev \*dev)
:   initiate a PCIe function level reset

**Parameters**

`struct pci_dev *dev`
:   device to reset

**Description**

Initiate a function level reset unconditionally on **dev** without
checking any flags and DEVCAP

int pcie\_reset\_flr(struct pci\_dev \*dev, bool probe)
:   initiate a PCIe function level reset

**Parameters**

`struct pci_dev *dev`
:   device to reset

`bool probe`
:   if true, return 0 if device can be reset this way

**Description**

Initiate a function level reset on **dev**.

int pci\_bridge\_secondary\_bus\_reset(struct pci\_dev \*dev)
:   Reset the secondary bus on a PCI bridge.

**Parameters**

`struct pci_dev *dev`
:   Bridge device

**Description**

Use the bridge control register to assert reset on the secondary bus.
Devices on the secondary bus are left in power-on state.

int \_\_pci\_reset\_function\_locked(struct pci\_dev \*dev)
:   reset a PCI device function while holding the **dev** mutex lock.

**Parameters**

`struct pci_dev *dev`
:   PCI device to reset

**Description**

Some devices allow an individual function to be reset without affecting
other functions in the same device. The PCI device must be responsive
to PCI config space in order to use this function.

The device function is presumed to be unused and the caller is holding
the device mutex lock when this function is called.

Resetting the device will make the contents of PCI configuration space
random, so any caller of this must be prepared to reinitialise the
device including MSI, bus mastering, BARs, decoding IO and memory spaces,
etc.

**Context**

The caller must hold the device lock.

**Return**

0 if the device function was successfully reset or negative if the
device doesn’t support resetting a single function.

int pci\_reset\_function(struct pci\_dev \*dev)
:   quiesce and reset a PCI device function

**Parameters**

`struct pci_dev *dev`
:   PCI device to reset

**Description**

Some devices allow an individual function to be reset without affecting
other functions in the same device. The PCI device must be responsive
to PCI config space in order to use this function.

This function does not just reset the PCI portion of a device, but
clears all the state associated with the device. This function differs
from [`__pci_reset_function_locked()`](#c.__pci_reset_function_locked "__pci_reset_function_locked") in that it saves and restores device state
over the reset and takes the PCI device lock.

Returns 0 if the device function was successfully reset or negative if the
device doesn’t support resetting a single function.

int pci\_reset\_function\_locked(struct pci\_dev \*dev)
:   quiesce and reset a PCI device function

**Parameters**

`struct pci_dev *dev`
:   PCI device to reset

**Description**

Some devices allow an individual function to be reset without affecting
other functions in the same device. The PCI device must be responsive
to PCI config space in order to use this function.

This function does not just reset the PCI portion of a device, but
clears all the state associated with the device. This function differs
from [`__pci_reset_function_locked()`](#c.__pci_reset_function_locked "__pci_reset_function_locked") in that it saves and restores device state
over the reset. It also differs from [`pci_reset_function()`](#c.pci_reset_function "pci_reset_function") in that it
requires the PCI device lock to be held.

**Context**

The caller must hold the device lock.

**Return**

0 if the device function was successfully reset or negative if the
device doesn’t support resetting a single function.

int pci\_try\_reset\_function(struct pci\_dev \*dev)
:   quiesce and reset a PCI device function

**Parameters**

`struct pci_dev *dev`
:   PCI device to reset

**Description**

Same as above, except return -EAGAIN if unable to lock device.

int pci\_probe\_reset\_slot(struct pci\_slot \*slot)
:   probe whether a PCI slot can be reset

**Parameters**

`struct pci_slot *slot`
:   PCI slot to probe

**Description**

Return 0 if slot can be reset, negative if a slot reset is not supported.

int pci\_probe\_reset\_bus(struct pci\_bus \*bus)
:   probe whether a PCI bus can be reset

**Parameters**

`struct pci_bus *bus`
:   PCI bus to probe

**Description**

Return 0 if bus can be reset, negative if a bus reset is not supported.

int pci\_reset\_bus(struct pci\_dev \*pdev)
:   Try to reset a PCI bus

**Parameters**

`struct pci_dev *pdev`
:   top level PCI device to reset via slot/bus

**Description**

Same as above except return -EAGAIN if the bus cannot be locked

int pcix\_get\_max\_mmrbc(struct pci\_dev \*dev)
:   get PCI-X maximum designed memory read byte count

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Returns mmrbc: maximum designed memory read count in bytes or
appropriate error value.

int pcix\_get\_mmrbc(struct pci\_dev \*dev)
:   get PCI-X maximum memory read byte count

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Returns mmrbc: maximum memory read count in bytes or appropriate error
value.

int pcix\_set\_mmrbc(struct pci\_dev \*dev, int mmrbc)
:   set PCI-X maximum memory read byte count

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`int mmrbc`
:   maximum memory read count in bytes
    valid values are 512, 1024, 2048, 4096

**Description**

If possible sets maximum memory read byte count, some bridges have errata
that prevent this.

int pcie\_get\_readrq(struct pci\_dev \*dev)
:   get PCI Express read request size

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Returns maximum memory read request in bytes or appropriate error value.

int pcie\_set\_readrq(struct pci\_dev \*dev, int rq)
:   set PCI Express maximum memory read request

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`int rq`
:   maximum memory read count in bytes
    valid values are 128, 256, 512, 1024, 2048, 4096

**Description**

If possible sets maximum memory read request in bytes

int pcie\_get\_mps(struct pci\_dev \*dev)
:   get PCI Express maximum payload size

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Returns maximum payload size in bytes

int pcie\_set\_mps(struct pci\_dev \*dev, int mps)
:   set PCI Express maximum payload size

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`int mps`
:   maximum payload size in bytes
    valid values are 128, 256, 512, 1024, 2048, 4096

**Description**

If possible sets maximum payload size

u32 pcie\_bandwidth\_available(struct pci\_dev \*dev, struct pci\_dev \*\*limiting\_dev, enum pci\_bus\_speed \*speed, enum pcie\_link\_width \*width)
:   determine minimum link settings of a PCIe device and its bandwidth limitation

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

`struct pci_dev **limiting_dev`
:   storage for device causing the bandwidth limitation

`enum pci_bus_speed *speed`
:   storage for speed of limiting device

`enum pcie_link_width *width`
:   storage for width of limiting device

**Description**

Walk up the PCI device chain and find the point where the minimum
bandwidth is available. Return the bandwidth available there and (if
limiting\_dev, speed, and width pointers are supplied) information about
that point. The bandwidth returned is in Mb/s, i.e., megabits/second of
raw bandwidth.

enum pci\_bus\_speed pcie\_get\_speed\_cap(struct pci\_dev \*dev)
:   query for the PCI device’s link speed capability

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Query the PCI device speed capability.

**Return**

the maximum link speed supported by the device.

enum pcie\_link\_width pcie\_get\_width\_cap(struct pci\_dev \*dev)
:   query for the PCI device’s link width capability

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Query the PCI device width capability. Return the maximum link width
supported by the device.

void pcie\_print\_link\_status(struct pci\_dev \*dev)
:   Report the PCI device’s link speed and width

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Report the available bandwidth at the device.

int pci\_select\_bars(struct pci\_dev \*dev, unsigned long flags)
:   Make BAR mask from the type of resource

**Parameters**

`struct pci_dev *dev`
:   the PCI device for which BAR mask is made

`unsigned long flags`
:   resource type mask to be selected

**Description**

This helper routine makes bar mask from the type of resource.

int pci\_bus\_find\_emul\_domain\_nr(u32 hint, u32 min, u32 max)
:   allocate a PCI domain number per constraints

**Parameters**

`u32 hint`
:   desired domain, 0 if any ID in the range of **min** to **max** is acceptable

`u32 min`
:   minimum allowable domain

`u32 max`
:   maximum allowable domain, no IDs higher than INT\_MAX will be returned

void \_\_iomem \*pci\_iomap\_range(struct pci\_dev \*dev, int bar, unsigned long offset, unsigned long maxlen)
:   create a virtual mapping cookie for a PCI BAR

**Parameters**

`struct pci_dev *dev`
:   PCI device that owns the BAR

`int bar`
:   BAR number

`unsigned long offset`
:   map memory at the given offset in BAR

`unsigned long maxlen`
:   max length of the memory to map

**Description**

Using this function you will get a \_\_iomem address to your device BAR.
You can access it using ioread\*() and iowrite\*(). These functions hide
the details if this is a MMIO or PIO address space and will just do what
you expect from them in the correct way.

**maxlen** specifies the maximum length to map. If you want to get access to
the complete BAR from offset to the end, pass `0` here.

void \_\_iomem \*pci\_iomap\_wc\_range(struct pci\_dev \*dev, int bar, unsigned long offset, unsigned long maxlen)
:   create a virtual WC mapping cookie for a PCI BAR

**Parameters**

`struct pci_dev *dev`
:   PCI device that owns the BAR

`int bar`
:   BAR number

`unsigned long offset`
:   map memory at the given offset in BAR

`unsigned long maxlen`
:   max length of the memory to map

**Description**

Using this function you will get a \_\_iomem address to your device BAR.
You can access it using ioread\*() and iowrite\*(). These functions hide
the details if this is a MMIO or PIO address space and will just do what
you expect from them in the correct way. When possible write combining
is used.

**maxlen** specifies the maximum length to map. If you want to get access to
the complete BAR from offset to the end, pass `0` here.

void \_\_iomem \*pci\_iomap(struct pci\_dev \*dev, int bar, unsigned long maxlen)
:   create a virtual mapping cookie for a PCI BAR

**Parameters**

`struct pci_dev *dev`
:   PCI device that owns the BAR

`int bar`
:   BAR number

`unsigned long maxlen`
:   length of the memory to map

**Description**

Using this function you will get a \_\_iomem address to your device BAR.
You can access it using ioread\*() and iowrite\*(). These functions hide
the details if this is a MMIO or PIO address space and will just do what
you expect from them in the correct way.

**maxlen** specifies the maximum length to map. If you want to get access to
the complete BAR without checking for its length first, pass `0` here.

void \_\_iomem \*pci\_iomap\_wc(struct pci\_dev \*dev, int bar, unsigned long maxlen)
:   create a virtual WC mapping cookie for a PCI BAR

**Parameters**

`struct pci_dev *dev`
:   PCI device that owns the BAR

`int bar`
:   BAR number

`unsigned long maxlen`
:   length of the memory to map

**Description**

Using this function you will get a \_\_iomem address to your device BAR.
You can access it using ioread\*() and iowrite\*(). These functions hide
the details if this is a MMIO or PIO address space and will just do what
you expect from them in the correct way. When possible write combining
is used.

**maxlen** specifies the maximum length to map. If you want to get access to
the complete BAR without checking for its length first, pass `0` here.

int devm\_pci\_remap\_iospace(struct [device](../infrastructure.html#c.device "device") \*dev, const struct resource \*res, phys\_addr\_t phys\_addr)
:   Managed [`pci_remap_iospace()`](#c.pci_remap_iospace "pci_remap_iospace")

**Parameters**

`struct device *dev`
:   Generic device to remap IO address for

`const struct resource *res`
:   Resource describing the I/O space

`phys_addr_t phys_addr`
:   physical address of range to be mapped

**Description**

Managed [`pci_remap_iospace()`](#c.pci_remap_iospace "pci_remap_iospace"). Map is automatically unmapped on driver
detach.

void \_\_iomem \*devm\_pci\_remap\_cfgspace(struct [device](../infrastructure.html#c.device "device") \*dev, resource\_size\_t offset, resource\_size\_t size)
:   Managed `pci_remap_cfgspace()`

**Parameters**

`struct device *dev`
:   Generic device to remap IO address for

`resource_size_t offset`
:   Resource address to map

`resource_size_t size`
:   Size of map

**Description**

Managed `pci_remap_cfgspace()`. Map is automatically unmapped on driver
detach.

void \_\_iomem \*devm\_pci\_remap\_cfg\_resource(struct [device](../infrastructure.html#c.device "device") \*dev, struct resource \*res)
:   check, request region and ioremap cfg resource

**Parameters**

`struct device *dev`
:   generic device to handle the resource for

`struct resource *res`
:   configuration space resource to be handled

**Description**

Checks that a resource is a valid memory region, requests the memory
region and ioremaps with `pci_remap_cfgspace()` API that ensures the
proper PCI configuration space memory attributes are guaranteed.

All operations are managed and will be undone on driver detach.

Returns a pointer to the remapped memory or an `IOMEM_ERR_PTR()` encoded error
code on failure. Usage example:

```
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
base = devm_pci_remap_cfg_resource(&pdev->dev, res);
if (IS_ERR(base))
        return PTR_ERR(base);
```

int pcim\_set\_mwi(struct pci\_dev \*pdev)
:   a device-managed [`pci_set_mwi()`](#c.pci_set_mwi "pci_set_mwi")

**Parameters**

`struct pci_dev *pdev`
:   the PCI device for which MWI is enabled

**Description**

Managed [`pci_set_mwi()`](#c.pci_set_mwi "pci_set_mwi").

**Return**

An appropriate -ERRNO error value on error, or zero for success.

int pcim\_intx(struct pci\_dev \*pdev, int enable)
:   managed [`pci_intx()`](#c.pci_intx "pci_intx")

**Parameters**

`struct pci_dev *pdev`
:   the PCI device to operate on

`int enable`
:   boolean: whether to enable or disable PCI INTx

**Return**

0 on success, -ENOMEM on error.

**Description**

Enable/disable PCI INTx for device **pdev**.
Restore the original state on driver detach.

int pcim\_enable\_device(struct pci\_dev \*pdev)
:   Managed [`pci_enable_device()`](#c.pci_enable_device "pci_enable_device")

**Parameters**

`struct pci_dev *pdev`
:   PCI device to be initialized

**Return**

0 on success, negative error code on failure.

**Description**

Managed [`pci_enable_device()`](#c.pci_enable_device "pci_enable_device"). Device will automatically be disabled on
driver detach.

void pcim\_pin\_device(struct pci\_dev \*pdev)
:   Pin managed PCI device

**Parameters**

`struct pci_dev *pdev`
:   PCI device to pin

**Description**

Pin managed PCI device **pdev**. Pinned device won’t be disabled on driver
detach. **pdev** must have been enabled with [`pcim_enable_device()`](#c.pcim_enable_device "pcim_enable_device").

void \_\_iomem \*const \*pcim\_iomap\_table(struct pci\_dev \*pdev)
:   access iomap allocation table (DEPRECATED)

**Parameters**

`struct pci_dev *pdev`
:   PCI device to access iomap table for

**Return**

Const pointer to array of \_\_iomem pointers on success, NULL on failure.

**Description**

Access iomap allocation table for **dev**. If iomap table doesn’t
exist and **pdev** is managed, it will be allocated. All iomaps
recorded in the iomap table are automatically unmapped on driver
detach.

This function might sleep when the table is first allocated but can
be safely called without context and guaranteed to succeed once
allocated.

This function is DEPRECATED. Do not use it in new code. Instead, obtain a
mapping’s address directly from one of the pcim\_\* mapping functions. For

**example**

void \_\_iomem \*mappy = pcim\_iomap(pdev, bar, length);

void \_\_iomem \*pcim\_iomap(struct pci\_dev \*pdev, int bar, unsigned long maxlen)
:   Managed [`pcim_iomap()`](#c.pcim_iomap "pcim_iomap")

**Parameters**

`struct pci_dev *pdev`
:   PCI device to iomap for

`int bar`
:   BAR to iomap

`unsigned long maxlen`
:   Maximum length of iomap

**Return**

\_\_iomem pointer on success, NULL on failure.

**Description**

Managed [`pci_iomap()`](#c.pci_iomap "pci_iomap"). Map is automatically unmapped on driver detach. If
desired, unmap manually only with [`pcim_iounmap()`](#c.pcim_iounmap "pcim_iounmap").

This SHOULD only be used once per BAR.

**NOTE**

Contrary to the other pcim\_\* functions, this function does not return an
`IOMEM_ERR_PTR()` on failure, but a simple NULL. This is done for backwards
compatibility.

void pcim\_iounmap(struct pci\_dev \*pdev, void \_\_iomem \*addr)
:   Managed `pci_iounmap()`

**Parameters**

`struct pci_dev *pdev`
:   PCI device to iounmap for

`void __iomem *addr`
:   Address to unmap

**Description**

Managed `pci_iounmap()`. **addr** must have been mapped using a pcim\_\* mapping
function.

void \_\_iomem \*pcim\_iomap\_region(struct pci\_dev \*pdev, int bar, const char \*name)
:   Request and iomap a PCI BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device to map IO resources for

`int bar`
:   Index of a BAR to map

`const char *name`
:   Name of the driver requesting the resource

**Return**

\_\_iomem pointer on success, an IOMEM\_ERR\_PTR on failure.

**Description**

Mapping and region will get automatically released on driver detach. If
desired, release manually only with [`pcim_iounmap_region()`](#c.pcim_iounmap_region "pcim_iounmap_region").

void pcim\_iounmap\_region(struct pci\_dev \*pdev, int bar)
:   Unmap and release a PCI BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device to operate on

`int bar`
:   Index of BAR to unmap and release

**Description**

Unmap a BAR and release its region manually. Only pass BARs that were
previously mapped by [`pcim_iomap_region()`](#c.pcim_iomap_region "pcim_iomap_region").

int pcim\_iomap\_regions(struct pci\_dev \*pdev, int mask, const char \*name)
:   Request and iomap PCI BARs (DEPRECATED)

**Parameters**

`struct pci_dev *pdev`
:   PCI device to map IO resources for

`int mask`
:   Mask of BARs to request and iomap

`const char *name`
:   Name of the driver requesting the resources

**Return**

0 on success, negative error code on failure.

**Description**

Request and iomap regions specified by **mask**.

This function is DEPRECATED. Do not use it in new code.
Use [`pcim_iomap_region()`](#c.pcim_iomap_region "pcim_iomap_region") instead.

int pcim\_request\_region(struct pci\_dev \*pdev, int bar, const char \*name)
:   Request a PCI BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device to request region for

`int bar`
:   Index of BAR to request

`const char *name`
:   Name of the driver requesting the resource

**Return**

0 on success, a negative error code on failure.

**Description**

Request region specified by **bar**.

The region will automatically be released on driver detach. If desired,
release manually only with `pcim_release_region()`.

int pcim\_request\_all\_regions(struct pci\_dev \*pdev, const char \*name)
:   Request all regions

**Parameters**

`struct pci_dev *pdev`
:   PCI device to map IO resources for

`const char *name`
:   name of the driver requesting the resources

**Return**

0 on success, negative error code on failure.

**Description**

Requested regions will automatically be released at driver detach. If
desired, release individual regions with `pcim_release_region()` or all of
them at once with `pcim_release_all_regions()`.

void \_\_iomem \*pcim\_iomap\_range(struct pci\_dev \*pdev, int bar, unsigned long offset, unsigned long len)
:   Create a ranged \_\_iomap mapping within a PCI BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device to map IO resources for

`int bar`
:   Index of the BAR

`unsigned long offset`
:   Offset from the begin of the BAR

`unsigned long len`
:   Length in bytes for the mapping

**Return**

\_\_iomem pointer on success, an IOMEM\_ERR\_PTR on failure.

**Description**

Creates a new IO-Mapping within the specified **bar**, ranging from **offset** to
**offset** + **len**.

The mapping will automatically get unmapped on driver detach. If desired,
release manually only with [`pcim_iounmap()`](#c.pcim_iounmap "pcim_iounmap").

int pci\_add\_dynid(struct [pci\_driver](../../PCI/pci.html#c.pci_driver "pci_driver") \*drv, unsigned int vendor, unsigned int device, unsigned int subvendor, unsigned int subdevice, unsigned int class, unsigned int class\_mask, unsigned long driver\_data)
:   add a new PCI device ID to this driver and re-probe devices

**Parameters**

`struct pci_driver *drv`
:   target pci driver

`unsigned int vendor`
:   PCI vendor ID

`unsigned int device`
:   PCI device ID

`unsigned int subvendor`
:   PCI subvendor ID

`unsigned int subdevice`
:   PCI subdevice ID

`unsigned int class`
:   PCI class

`unsigned int class_mask`
:   PCI class mask

`unsigned long driver_data`
:   private driver data

**Description**

Adds a new dynamic pci device ID to this driver and causes the
driver to probe for all devices again. **drv** must have been
registered prior to calling this function.

**Context**

Does GFP\_KERNEL allocation.

**Return**

0 on success, -errno on failure.

const struct [pci\_device\_id](../../PCI/pci.html#c.pci_device_id "pci_device_id") \*pci\_match\_id(const struct [pci\_device\_id](../../PCI/pci.html#c.pci_device_id "pci_device_id") \*ids, struct pci\_dev \*dev)
:   See if a PCI device matches a given pci\_id table

**Parameters**

`const struct pci_device_id *ids`
:   array of PCI device ID structures to search in

`struct pci_dev *dev`
:   the PCI device structure to match against.

**Description**

Used by a driver to check whether a PCI device is in its list of
supported devices. Returns the matching pci\_device\_id structure or
`NULL` if there is no match.

Deprecated; don’t use this as it will not catch any dynamic IDs
that a driver might want to check for.

int \_\_pci\_register\_driver(struct [pci\_driver](../../PCI/pci.html#c.pci_driver "pci_driver") \*drv, struct module \*owner, const char \*mod\_name)
:   register a new pci driver

**Parameters**

`struct pci_driver *drv`
:   the driver structure to register

`struct module *owner`
:   owner module of drv

`const char *mod_name`
:   module name string

**Description**

Adds the driver structure to the list of registered drivers.
Returns a negative value on error, otherwise 0.
If no error occurred, the driver remains registered even if
no device was claimed during registration.

void pci\_unregister\_driver(struct [pci\_driver](../../PCI/pci.html#c.pci_driver "pci_driver") \*drv)
:   unregister a pci driver

**Parameters**

`struct pci_driver *drv`
:   the driver structure to unregister

**Description**

Deletes the driver structure from the list of registered PCI drivers,
gives it a chance to clean up by calling its `remove()` function for
each device it was responsible for, and marks those devices as
driverless.

struct [pci\_driver](../../PCI/pci.html#c.pci_driver "pci_driver") \*pci\_dev\_driver(const struct pci\_dev \*dev)
:   get the pci\_driver of a device

**Parameters**

`const struct pci_dev *dev`
:   the device to query

**Description**

Returns the appropriate pci\_driver structure or `NULL` if there is no
registered driver for the device.

struct pci\_dev \*pci\_dev\_get(struct pci\_dev \*dev)
:   increments the reference count of the pci device structure

**Parameters**

`struct pci_dev *dev`
:   the device being referenced

**Description**

Each live reference to a device should be refcounted.

Drivers for PCI devices should normally record such references in
their `probe()` methods, when they bind to a device, and release
them by calling [`pci_dev_put()`](#c.pci_dev_put "pci_dev_put"), in their `disconnect()` methods.

A pointer to the device with the incremented reference counter is returned.

void pci\_dev\_put(struct pci\_dev \*dev)
:   release a use of the pci device structure

**Parameters**

`struct pci_dev *dev`
:   device that’s been disconnected

**Description**

Must be called when a user of a device is finished with it. When the last
user of the device calls this function, the memory of the device is freed.

void pci\_stop\_and\_remove\_bus\_device(struct pci\_dev \*dev)
:   remove a PCI device and any children

**Parameters**

`struct pci_dev *dev`
:   the device to remove

**Description**

Remove a PCI device from the device lists, informing the drivers
that the device has been removed. We also remove any subordinate
buses and children in a depth-first manner.

For each device we remove, delete the device structure from the
device lists, remove the /proc entry, and notify userspace
(/sbin/hotplug).

struct pci\_bus \*pci\_find\_bus(int domain, int busnr)
:   locate PCI bus from a given domain and bus number

**Parameters**

`int domain`
:   number of PCI domain to search

`int busnr`
:   number of desired PCI bus

**Description**

Given a PCI bus number and domain number, the desired PCI bus is located
in the global list of PCI buses. If the bus is found, a pointer to its
data structure is returned. If no bus is found, `NULL` is returned.

struct pci\_bus \*pci\_find\_next\_bus(const struct pci\_bus \*from)
:   begin or continue searching for a PCI bus

**Parameters**

`const struct pci_bus *from`
:   Previous PCI bus found, or `NULL` for new search.

**Description**

Iterates through the list of known PCI buses. A new search is
initiated by passing `NULL` as the **from** argument. Otherwise if
**from** is not `NULL`, searches continue from next device on the
global list.

struct pci\_dev \*pci\_get\_slot(struct pci\_bus \*bus, unsigned int devfn)
:   locate PCI device for a given PCI slot

**Parameters**

`struct pci_bus *bus`
:   PCI bus on which desired PCI device resides

`unsigned int devfn`
:   encodes number of PCI slot in which the desired PCI
    device resides and the logical device number within that slot
    in case of multi-function devices.

**Description**

Given a PCI bus and slot/function number, the desired PCI device
is located in the list of PCI devices.
If the device is found, its reference count is increased and this
function returns a pointer to its data structure. The caller must
decrement the reference count by calling [`pci_dev_put()`](#c.pci_dev_put "pci_dev_put").
If no device is found, `NULL` is returned.

struct pci\_dev \*pci\_get\_domain\_bus\_and\_slot(int domain, unsigned int bus, unsigned int devfn)
:   locate PCI device for a given PCI domain (segment), bus, and slot

**Parameters**

`int domain`
:   PCI domain/segment on which the PCI device resides.

`unsigned int bus`
:   PCI bus on which desired PCI device resides

`unsigned int devfn`
:   encodes number of PCI slot in which the desired PCI device
    resides and the logical device number within that slot in case of
    multi-function devices.

**Description**

Given a PCI domain, bus, and slot/function number, the desired PCI
device is located in the list of PCI devices. If the device is
found, its reference count is increased and this function returns a
pointer to its data structure. The caller must decrement the
reference count by calling [`pci_dev_put()`](#c.pci_dev_put "pci_dev_put"). If no device is found,
`NULL` is returned.

struct pci\_dev \*pci\_get\_subsys(unsigned int vendor, unsigned int device, unsigned int ss\_vendor, unsigned int ss\_device, struct pci\_dev \*from)
:   begin or continue searching for a PCI device by vendor/subvendor/device/subdevice id

**Parameters**

`unsigned int vendor`
:   PCI vendor id to match, or `PCI_ANY_ID` to match all vendor ids

`unsigned int device`
:   PCI device id to match, or `PCI_ANY_ID` to match all device ids

`unsigned int ss_vendor`
:   PCI subsystem vendor id to match, or `PCI_ANY_ID` to match all vendor ids

`unsigned int ss_device`
:   PCI subsystem device id to match, or `PCI_ANY_ID` to match all device ids

`struct pci_dev *from`
:   Previous PCI device found in search, or `NULL` for new search.

**Description**

Iterates through the list of known PCI devices. If a PCI device is found
with a matching **vendor**, **device**, **ss\_vendor** and **ss\_device**, a pointer to its
device structure is returned, and the reference count to the device is
incremented. Otherwise, `NULL` is returned. A new search is initiated by
passing `NULL` as the **from** argument. Otherwise if **from** is not `NULL`,
searches continue from next device on the global list.
The reference count for **from** is always decremented if it is not `NULL`.

struct pci\_dev \*pci\_get\_device(unsigned int vendor, unsigned int device, struct pci\_dev \*from)
:   begin or continue searching for a PCI device by vendor/device id

**Parameters**

`unsigned int vendor`
:   PCI vendor id to match, or `PCI_ANY_ID` to match all vendor ids

`unsigned int device`
:   PCI device id to match, or `PCI_ANY_ID` to match all device ids

`struct pci_dev *from`
:   Previous PCI device found in search, or `NULL` for new search.

**Description**

Iterates through the list of known PCI devices. If a PCI device is
found with a matching **vendor** and **device**, the reference count to the
device is incremented and a pointer to its device structure is returned.
Otherwise, `NULL` is returned. A new search is initiated by passing `NULL`
as the **from** argument. Otherwise if **from** is not `NULL`, searches continue
from next device on the global list. The reference count for **from** is
always decremented if it is not `NULL`.

struct pci\_dev \*pci\_get\_class(unsigned int class, struct pci\_dev \*from)
:   begin or continue searching for a PCI device by class

**Parameters**

`unsigned int class`
:   search for a PCI device with this class designation

`struct pci_dev *from`
:   Previous PCI device found in search, or `NULL` for new search.

**Description**

Iterates through the list of known PCI devices. If a PCI device is
found with a matching **class**, the reference count to the device is
incremented and a pointer to its device structure is returned.
Otherwise, `NULL` is returned.
A new search is initiated by passing `NULL` as the **from** argument.
Otherwise if **from** is not `NULL`, searches continue from next device
on the global list. The reference count for **from** is always decremented
if it is not `NULL`.

struct pci\_dev \*pci\_get\_base\_class(unsigned int class, struct pci\_dev \*from)
:   searching for a PCI device by matching against the base class code only

**Parameters**

`unsigned int class`
:   search for a PCI device with this base class code

`struct pci_dev *from`
:   Previous PCI device found in search, or `NULL` for new search.

**Description**

Iterates through the list of known PCI devices. If a PCI device is found
with a matching base class code, the reference count to the device is
incremented. See `pci_match_one_device()` to figure out how does this works.
A new search is initiated by passing `NULL` as the **from** argument.
Otherwise if **from** is not `NULL`, searches continue from next device on the
global list. The reference count for **from** is always decremented if it is
not `NULL`.

**Return**

A pointer to a matched PCI device, `NULL` Otherwise.

int pci\_dev\_present(const struct [pci\_device\_id](../../PCI/pci.html#c.pci_device_id "pci_device_id") \*ids)
:   Returns 1 if device matching the device list is present, 0 if not.

**Parameters**

`const struct pci_device_id *ids`
:   A pointer to a null terminated list of [`struct pci_device_id`](../../PCI/pci.html#c.pci_device_id "pci_device_id") structures
    that describe the type of PCI device the caller is trying to find.

**Description**

Obvious fact: You do not have a reference to any device that might be found
by this function, so if that device is removed from the system right after
this function is finished, the value will be stale. Use this function to
find devices that are usually built into a system, or for a general hint as
to if another device happens to be present at this specific moment in time.

void pci\_msi\_mask\_irq(struct [irq\_data](../../core-api/genericirq.html#c.irq_data "irq_data") \*data)
:   Generic IRQ chip callback to mask PCI/MSI interrupts

**Parameters**

`struct irq_data *data`
:   pointer to irqdata associated to that interrupt

void pci\_msi\_unmask\_irq(struct [irq\_data](../../core-api/genericirq.html#c.irq_data "irq_data") \*data)
:   Generic IRQ chip callback to unmask PCI/MSI interrupts

**Parameters**

`struct irq_data *data`
:   pointer to irqdata associated to that interrupt

int pci\_msi\_vec\_count(struct pci\_dev \*dev)
:   Return the number of MSI vectors a device can send

**Parameters**

`struct pci_dev *dev`
:   device to report about

**Description**

This function returns the number of MSI vectors a device requested via
Multiple Message Capable register. It returns a negative errno if the
device is not capable sending MSI interrupts. Otherwise, the call succeeds
and returns a power of two, up to a maximum of 2^5 (32), according to the
MSI specification.

int pci\_bus\_alloc\_resource(struct pci\_bus \*bus, struct resource \*res, resource\_size\_t size, resource\_size\_t align, resource\_size\_t min, unsigned long type\_mask, resource\_alignf alignf, void \*alignf\_data)
:   allocate a resource from a parent bus

**Parameters**

`struct pci_bus *bus`
:   PCI bus

`struct resource *res`
:   resource to allocate

`resource_size_t size`
:   size of resource to allocate

`resource_size_t align`
:   alignment of resource to allocate

`resource_size_t min`
:   minimum /proc/iomem address to allocate

`unsigned long type_mask`
:   IORESOURCE\_\* type flags

`resource_alignf alignf`
:   resource alignment function

`void *alignf_data`
:   data argument for resource alignment function

**Description**

Given the PCI bus a device resides on, the size, minimum address,
alignment and type, try to find an acceptable resource allocation
for a specific device resource.

void pci\_bus\_add\_device(struct pci\_dev \*dev)
:   start driver for a single device

**Parameters**

`struct pci_dev *dev`
:   device to add

**Description**

This adds add sysfs entries and start device drivers

void pci\_bus\_add\_devices(const struct pci\_bus \*bus)
:   start driver for PCI devices

**Parameters**

`const struct pci_bus *bus`
:   bus to check for new devices

**Description**

Start driver for PCI devices and add some sysfs entries.

void pci\_walk\_bus(struct pci\_bus \*top, int (\*cb)(struct pci\_dev\*, void\*), void \*userdata)
:   walk devices on/under bus, calling callback.

**Parameters**

`struct pci_bus *top`
:   bus whose devices should be walked

`int (*cb)(struct pci_dev *, void *)`
:   callback to be called for each device found

`void *userdata`
:   arbitrary pointer to be passed to callback

**Description**

> Walk the given bus, including any bridged devices
> on buses under this bus. Call the provided callback
> on each device found.
>
> We check the return of **cb** each time. If it returns anything
> other than 0, we break out.

void pci\_walk\_bus\_reverse(struct pci\_bus \*top, int (\*cb)(struct pci\_dev\*, void\*), void \*userdata)
:   walk devices on/under bus, calling callback.

**Parameters**

`struct pci_bus *top`
:   bus whose devices should be walked

`int (*cb)(struct pci_dev *, void *)`
:   callback to be called for each device found

`void *userdata`
:   arbitrary pointer to be passed to callback

**Description**

Same semantics as [`pci_walk_bus()`](#c.pci_walk_bus "pci_walk_bus"), but walks the bus in reverse order.

struct pci\_ops \*pci\_bus\_set\_ops(struct pci\_bus \*bus, struct pci\_ops \*ops)
:   Set raw operations of pci bus

**Parameters**

`struct pci_bus *bus`
:   pci bus struct

`struct pci_ops *ops`
:   new raw operations

**Description**

Return previous raw operations

void pci\_cfg\_access\_lock(struct pci\_dev \*dev)
:   Lock PCI config reads/writes

**Parameters**

`struct pci_dev *dev`
:   pci device struct

**Description**

When access is locked, any userspace reads or writes to config
space and concurrent lock requests will sleep until access is
allowed via [`pci_cfg_access_unlock()`](#c.pci_cfg_access_unlock "pci_cfg_access_unlock") again.

bool pci\_cfg\_access\_trylock(struct pci\_dev \*dev)
:   try to lock PCI config reads/writes

**Parameters**

`struct pci_dev *dev`
:   pci device struct

**Description**

Same as pci\_cfg\_access\_lock, but will return 0 if access is
already locked, 1 otherwise. This function can be used from
atomic contexts.

void pci\_cfg\_access\_unlock(struct pci\_dev \*dev)
:   Unlock PCI config reads/writes

**Parameters**

`struct pci_dev *dev`
:   pci device struct

**Description**

This function allows PCI config accesses to resume.

int pci\_request\_irq(struct pci\_dev \*dev, unsigned int nr, irq\_handler\_t handler, irq\_handler\_t thread\_fn, void \*dev\_id, const char \*fmt, ...)
:   allocate an interrupt line for a PCI device

**Parameters**

`struct pci_dev *dev`
:   PCI device to operate on

`unsigned int nr`
:   device-relative interrupt vector index (0-based).

`irq_handler_t handler`
:   Function to be called when the IRQ occurs.
    Primary handler for threaded interrupts.
    If NULL and thread\_fn != NULL the default primary handler is
    installed.

`irq_handler_t thread_fn`
:   Function called from the IRQ handler thread
    If NULL, no IRQ thread is created

`void *dev_id`
:   Cookie passed back to the handler function

`const char *fmt`
:   Printf-like format string naming the handler

`...`
:   variable arguments

**Description**

This call allocates interrupt resources and enables the interrupt line and
IRQ handling. From the point this call is made **handler** and **thread\_fn** may
be invoked. All interrupts requested using this function might be shared.

**dev\_id** must not be NULL and must be globally unique.

void pci\_free\_irq(struct pci\_dev \*dev, unsigned int nr, void \*dev\_id)
:   free an interrupt allocated with pci\_request\_irq

**Parameters**

`struct pci_dev *dev`
:   PCI device to operate on

`unsigned int nr`
:   device-relative interrupt vector index (0-based).

`void *dev_id`
:   Device identity to free

**Description**

Remove an interrupt handler. The handler is removed and if the interrupt
line is no longer in use by any driver it is disabled. The caller must
ensure the interrupt is disabled on the device before calling this function.
The function does not return until any executing interrupts for this IRQ
have completed.

This function must not be called from interrupt context.

u8 pci\_common\_swizzle(struct pci\_dev \*dev, u8 \*pinp)
:   swizzle INTx all the way to root bridge

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`u8 *pinp`
:   pointer to the INTx pin value (1=INTA, 2=INTB, 3=INTD, 4=INTD)

**Description**

Perform INTx swizzling for a device. This traverses through all PCI-to-PCI
bridges all the way up to a PCI root bus.

bool pci\_check\_and\_mask\_intx(struct pci\_dev \*dev)
:   mask INTx on pending interrupt

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Check if the device dev has its INTx line asserted, mask it and return
true in that case. False is returned if no interrupt was pending.

bool pci\_check\_and\_unmask\_intx(struct pci\_dev \*dev)
:   unmask INTx if no interrupt is pending

**Parameters**

`struct pci_dev *dev`
:   the PCI device to operate on

**Description**

Check if the device dev has its INTx line asserted, unmask it if not and
return true. False is returned and the mask remains active if there was
still an interrupt pending.

unsigned char pcie\_get\_link\_speed(unsigned int speed)
:   Get speed value from PCIe generation number

**Parameters**

`unsigned int speed`
:   PCIe speed (1-based: 1 = 2.5GT, 2 = 5GT, ...)

**Description**

Returns the speed value (e.g., PCIE\_SPEED\_2\_5GT) if **speed** is valid,
otherwise returns PCI\_SPEED\_UNKNOWN.

bool pcie\_relaxed\_ordering\_enabled(struct pci\_dev \*dev)
:   Probe for PCIe relaxed ordering enable

**Parameters**

`struct pci_dev *dev`
:   PCI device to query

**Description**

Returns true if the device has enabled relaxed ordering attribute.

int pci\_scan\_slot(struct pci\_bus \*bus, int devfn)
:   Scan a PCI slot on a bus for devices

**Parameters**

`struct pci_bus *bus`
:   PCI bus to scan

`int devfn`
:   slot number to scan (must have zero function)

**Description**

Scan a PCI slot on the specified PCI bus for devices, adding
discovered devices to the **bus->devices** list. New devices
will not have is\_added set.

Returns the number of new devices found.

unsigned int pci\_scan\_child\_bus(struct pci\_bus \*bus)
:   Scan devices below a bus

**Parameters**

`struct pci_bus *bus`
:   Bus to scan for devices

**Description**

Scans devices below **bus** including subordinate buses. Returns new
subordinate number including all the found devices.

unsigned int pci\_rescan\_bus(struct pci\_bus \*bus)
:   Scan a PCI bus for devices

**Parameters**

`struct pci_bus *bus`
:   PCI bus to scan

**Description**

Scan a PCI bus and child buses for new devices, add them,
and enable them.

Returns the max number of subordinate bus discovered.

struct pci\_slot \*pci\_create\_slot(struct pci\_bus \*parent, int slot\_nr, const char \*name, struct hotplug\_slot \*hotplug)
:   create or increment refcount for physical PCI slot

**Parameters**

`struct pci_bus *parent`
:   `struct pci_bus` of parent bridge

`int slot_nr`
:   PCI\_SLOT(pci\_dev->devfn), -1 for placeholder, or
    PCI\_SLOT\_ALL\_DEVICES

`const char *name`
:   user visible string presented in /sys/bus/pci/slots/<name>

`struct hotplug_slot *hotplug`
:   set if caller is hotplug driver, NULL otherwise

**Description**

PCI slots have first class attributes such as address, speed, width,
and a `struct pci_slot` is used to manage them. This interface will
either return a new `struct pci_slot` to the caller, or if the pci\_slot
already exists, its refcount will be incremented.

Slots are uniquely identified by a **pci\_bus**, **slot\_nr** tuple.

There are known platforms with broken firmware that assign the same
name to multiple slots. Workaround these broken platforms by renaming
the slots on behalf of the caller. If firmware assigns name N to
multiple slots:

The first slot is assigned N
The second slot is assigned N-1
The third slot is assigned N-2
etc.

Placeholder slots:
In most cases, **pci\_bus**, **slot\_nr** will be sufficient to uniquely identify
a slot. There is one notable exception - pSeries (rpaphp), where the
**slot\_nr** cannot be determined until a device is actually inserted into
the slot. In this scenario, the caller may pass -1 for **slot\_nr**.

The following semantics are imposed when the caller passes **slot\_nr** ==
-1. First, we no longer check for an existing `struct` pci\_slot, as there
may be many slots with **slot\_nr** of -1. The other change in semantics is
user-visible, which is the ‘address’ parameter presented in sysfs will
consist solely of a dddd:bb tuple, where dddd is the PCI domain of the
`struct` pci\_bus and bb is the bus number. In other words, the devfn of
the ‘placeholder’ slot will not be displayed.

Bus-wide slots:
For PCIe hotplug, the physical slot encompasses the entire secondary
bus, not just a single device number. If the device supports ARI and ARI
Forwarding is enabled in the upstream bridge, a multi-function device
may include functions that appear to have several different device
numbers, i.e., `PCI_SLOT()` values. Pass **slot\_nr** == PCI\_SLOT\_ALL\_DEVICES
to create a slot that matches all devices on the bus. Unlike placeholder
slots, bus-wide slots go through normal slot lookup and reuse existing
slots if present.

void pci\_destroy\_slot(struct pci\_slot \*slot)
:   decrement refcount for physical PCI slot

**Parameters**

`struct pci_slot *slot`
:   `struct pci_slot` to decrement

**Description**

`struct` pci\_slot is refcounted, so destroying them is really easy; we
just call kobject\_put on its kobj and let our release methods do the
rest.

int pci\_rebar\_bytes\_to\_size(u64 bytes)
:   Convert size in bytes to PCI BAR Size

**Parameters**

`u64 bytes`
:   size in bytes

**Description**

Convert size in bytes to encoded BAR Size in Resizable BAR Capability
(PCIe r6.2, sec. 7.8.6.3).

**Return**

encoded BAR Size as defined in the PCIe spec (0=1MB, 31=128TB)

resource\_size\_t pci\_rebar\_size\_to\_bytes(int size)
:   Convert encoded BAR Size to size in bytes

**Parameters**

`int size`
:   encoded BAR Size as defined in the PCIe spec (0=1MB, 31=128TB)

**Return**

BAR size in bytes

u64 pci\_rebar\_get\_possible\_sizes(struct pci\_dev \*pdev, int bar)
:   get possible sizes for Resizable BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`int bar`
:   BAR to query

**Description**

Get the possible sizes of a resizable BAR as bitmask.

**Return**

A bitmask of possible sizes (bit 0=1MB, bit 31=128TB), or `0` if
BAR isn’t resizable.

bool pci\_rebar\_size\_supported(struct pci\_dev \*pdev, int bar, int size)
:   check if size is supported for BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`int bar`
:   BAR to check

`int size`
:   encoded size as defined in the PCIe spec (0=1MB, 31=128TB)

**Return**

`true` if **bar** is resizable and **size** is supported, otherwise
`false`.

int pci\_rebar\_get\_max\_size(struct pci\_dev \*pdev, int bar)
:   get the maximum supported size of a BAR

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`int bar`
:   BAR to query

**Description**

Get the largest supported size of a resizable BAR as a size.

**Return**

the encoded maximum BAR size as defined in the PCIe spec
(0=1MB, 31=128TB), or `-NOENT` on error.

int pci\_resize\_resource(struct pci\_dev \*dev, int resno, int size, int exclude\_bars)
:   reconfigure a Resizable BAR and resources

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`int resno`
:   index of the BAR to be resized

`int size`
:   new size as defined in the spec (0=1MB, 31=128TB)

`int exclude_bars`
:   a mask of BARs that should not be released

**Description**

Reconfigure **resno** to **size** and re-run resource assignment algorithm
with the new size.

Prior to resize, release **dev** resources that share a bridge window with
**resno**. This unpins the bridge window resource to allow changing it.

The caller may prevent releasing a particular BAR by providing
**exclude\_bars** mask, but this may result in the resize operation failing
due to insufficient space.

**Return**

0 on success, or negative on error. In case of an error, the
resources are restored to their original places.

int pci\_enable\_rom(struct pci\_dev \*pdev)
:   enable ROM decoding for a PCI device

**Parameters**

`struct pci_dev *pdev`
:   PCI device to enable

**Description**

Enable ROM decoding on **dev**. This involves simply turning on the last
bit of the PCI ROM BAR. Note that some cards may share address decoders
between the ROM and other resources, so enabling it may disable access
to MMIO registers or other card memory.

void pci\_disable\_rom(struct pci\_dev \*pdev)
:   disable ROM decoding for a PCI device

**Parameters**

`struct pci_dev *pdev`
:   PCI device to disable

**Description**

Disable ROM decoding on a PCI device by turning off the last bit in the
ROM BAR.

void \_\_iomem \*pci\_map\_rom(struct pci\_dev \*pdev, size\_t \*size)
:   map a PCI ROM to kernel space

**Parameters**

`struct pci_dev *pdev`
:   pointer to pci device struct

`size_t *size`
:   pointer to receive size of pci window over ROM

**Return**

kernel virtual pointer to image of ROM

**Description**

Map a PCI ROM into kernel space. If ROM is boot video ROM,
the shadow BIOS copy will be returned instead of the
actual ROM.

void pci\_unmap\_rom(struct pci\_dev \*pdev, void \_\_iomem \*rom)
:   unmap the ROM from kernel space

**Parameters**

`struct pci_dev *pdev`
:   pointer to pci device struct

`void __iomem *rom`
:   virtual address of the previous mapping

**Description**

Remove a mapping of a previously mapped ROM

void \*pci\_iov\_get\_pf\_drvdata(struct pci\_dev \*dev, struct [pci\_driver](../../PCI/pci.html#c.pci_driver "pci_driver") \*pf\_driver)
:   Return the drvdata of a PF

**Parameters**

`struct pci_dev *dev`
:   VF pci\_dev

`struct pci_driver *pf_driver`
:   Device driver required to own the PF

**Description**

This must be called from a context that ensures that a VF driver is attached.
The value returned is invalid once the VF driver completes its `remove()`
callback.

Locking is achieved by the driver core. A VF driver cannot be probed until
[`pci_enable_sriov()`](#c.pci_enable_sriov "pci_enable_sriov") is called and [`pci_disable_sriov()`](#c.pci_disable_sriov "pci_disable_sriov") does not return until
all VF drivers have completed their `remove()`.

The PF driver must call [`pci_disable_sriov()`](#c.pci_disable_sriov "pci_disable_sriov") before it begins to destroy the
drvdata.

int pci\_enable\_sriov(struct pci\_dev \*dev, int nr\_virtfn)
:   enable the SR-IOV capability

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`int nr_virtfn`
:   number of virtual functions to enable

**Description**

Returns 0 on success, or negative on failure.

void pci\_disable\_sriov(struct pci\_dev \*dev)
:   disable the SR-IOV capability

**Parameters**

`struct pci_dev *dev`
:   the PCI device

int pci\_num\_vf(struct pci\_dev \*dev)
:   return number of VFs associated with a PF device\_release\_driver

**Parameters**

`struct pci_dev *dev`
:   the PCI device

**Description**

Returns number of VFs, or 0 if SR-IOV is not enabled.

int pci\_vfs\_assigned(struct pci\_dev \*dev)
:   returns number of VFs are assigned to a guest

**Parameters**

`struct pci_dev *dev`
:   the PCI device

**Description**

Returns number of VFs belonging to this device that are assigned to a guest.
If device is not a physical function returns 0.

int pci\_sriov\_set\_totalvfs(struct pci\_dev \*dev, u16 numvfs)
:   * reduce the TotalVFs available

**Parameters**

`struct pci_dev *dev`
:   the PCI PF device

`u16 numvfs`
:   number that should be used for TotalVFs supported

**Description**

Should be called from PF driver’s probe routine with
device’s mutex held.

Returns 0 if PF is an SRIOV-capable device and
value of numvfs valid. If not a PF return -ENOSYS;
if numvfs is invalid return -EINVAL;
if VFs already enabled, return -EBUSY.

int pci\_sriov\_get\_totalvfs(struct pci\_dev \*dev)
:   * get total VFs supported on this device

**Parameters**

`struct pci_dev *dev`
:   the PCI PF device

**Description**

For a PCIe device with SRIOV support, return the PCIe
SRIOV capability value of TotalVFs or the value of driver\_max\_VFs
if the driver reduced it. Otherwise 0.

int pci\_sriov\_configure\_simple(struct pci\_dev \*dev, int nr\_virtfn)
:   helper to configure SR-IOV

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`int nr_virtfn`
:   number of virtual functions to enable, 0 to disable

**Description**

Enable or disable SR-IOV for devices that don’t require any PF setup
before enabling SR-IOV. Return value is negative on error, or number of
VFs allocated on success.

int pci\_iov\_vf\_bar\_set\_size(struct pci\_dev \*dev, int resno, int size)
:   set a new size for a VF BAR

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`int resno`
:   the resource number

`int size`
:   new size as defined in the spec (0=1MB, 31=128TB)

**Description**

Set the new size of a VF BAR that supports VF resizable BAR capability.
Unlike [`pci_resize_resource()`](#c.pci_resize_resource "pci_resize_resource"), this does not cause the resource that
reserves the MMIO space (originally up to total\_VFs) to be resized, which
means that following calls to [`pci_enable_sriov()`](#c.pci_enable_sriov "pci_enable_sriov") can fail if the resources
no longer fit.

**Return**

0 on success, or negative on failure.

u32 pci\_iov\_vf\_bar\_get\_sizes(struct pci\_dev \*dev, int resno, int num\_vfs)
:   get VF BAR sizes allowing to create up to num\_vfs

**Parameters**

`struct pci_dev *dev`
:   the PCI device

`int resno`
:   the resource number

`int num_vfs`
:   number of VFs

**Description**

Get the sizes of a VF resizable BAR that can accommodate **num\_vfs** within
the currently assigned size of the resource **resno**.

**Return**

A bitmask of sizes in format defined in the spec (bit 0=1MB,
bit 31=128TB).

ssize\_t pci\_read\_legacy\_io(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, struct kobject \*kobj, const struct bin\_attribute \*bin\_attr, char \*buf, loff\_t off, size\_t count)
:   read byte(s) from legacy I/O port space

**Parameters**

`struct file *filp`
:   open sysfs file

`struct kobject *kobj`
:   kobject corresponding to file to read from

`const struct bin_attribute *bin_attr`
:   `struct bin_attribute` for this file

`char *buf`
:   buffer to store results

`loff_t off`
:   offset into legacy I/O port space

`size_t count`
:   number of bytes to read

**Description**

Reads 1, 2, or 4 bytes from legacy I/O port space using an arch specific
callback routine (pci\_legacy\_read).

ssize\_t pci\_write\_legacy\_io(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, struct kobject \*kobj, const struct bin\_attribute \*bin\_attr, char \*buf, loff\_t off, size\_t count)
:   write byte(s) to legacy I/O port space

**Parameters**

`struct file *filp`
:   open sysfs file

`struct kobject *kobj`
:   kobject corresponding to file to read from

`const struct bin_attribute *bin_attr`
:   `struct bin_attribute` for this file

`char *buf`
:   buffer containing value to be written

`loff_t off`
:   offset into legacy I/O port space

`size_t count`
:   number of bytes to write

**Description**

Writes 1, 2, or 4 bytes from legacy I/O port space using an arch specific
callback routine (pci\_legacy\_write).

int pci\_mmap\_legacy\_mem(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, struct kobject \*kobj, const struct bin\_attribute \*attr, struct vm\_area\_struct \*vma)
:   map legacy PCI memory into user memory space

**Parameters**

`struct file *filp`
:   open sysfs file

`struct kobject *kobj`
:   kobject corresponding to device to be mapped

`const struct bin_attribute *attr`
:   `struct bin_attribute` for this file

`struct vm_area_struct *vma`
:   `struct vm_area_struct` passed to mmap

**Description**

Uses an arch specific callback, pci\_mmap\_legacy\_mem\_page\_range, to mmap
legacy memory space (first meg of bus space) into application virtual
memory space.

int pci\_mmap\_legacy\_io(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, struct kobject \*kobj, const struct bin\_attribute \*attr, struct vm\_area\_struct \*vma)
:   map legacy PCI IO into user memory space

**Parameters**

`struct file *filp`
:   open sysfs file

`struct kobject *kobj`
:   kobject corresponding to device to be mapped

`const struct bin_attribute *attr`
:   `struct bin_attribute` for this file

`struct vm_area_struct *vma`
:   `struct vm_area_struct` passed to mmap

**Description**

Uses an arch specific callback, pci\_mmap\_legacy\_io\_page\_range, to mmap
legacy IO space (first meg of bus space) into application virtual
memory space. Returns -ENOSYS if the operation isn’t supported

void pci\_adjust\_legacy\_attr(struct pci\_bus \*b, enum pci\_mmap\_state mmap\_type)
:   adjustment of legacy file attributes

**Parameters**

`struct pci_bus *b`
:   bus to create files under

`enum pci_mmap_state mmap_type`
:   I/O port or memory

**Description**

Stub implementation. Can be overridden by arch if necessary.

void pci\_create\_legacy\_files(struct pci\_bus \*b)
:   create legacy I/O port and memory files

**Parameters**

`struct pci_bus *b`
:   bus to create files under

**Description**

Some platforms allow access to legacy I/O port and ISA memory space on
a per-bus basis. This routine creates the files and ties them into
their associated read, write and mmap files from pci-sysfs.c

On error unwind, but don’t propagate the error to the caller
as it is ok to set up the PCI bus without these files.

int pci\_mmap\_resource(struct kobject \*kobj, const struct bin\_attribute \*attr, struct vm\_area\_struct \*vma, int write\_combine)
:   map a PCI resource into user memory space

**Parameters**

`struct kobject *kobj`
:   kobject for mapping

`const struct bin_attribute *attr`
:   `struct bin_attribute` for the file being mapped

`struct vm_area_struct *vma`
:   `struct vm_area_struct` passed into the mmap

`int write_combine`
:   1 for write\_combine mapping

**Description**

Use the regular PCI mapping routines to map a PCI resource into userspace.

void pci\_remove\_resource\_files(struct pci\_dev \*pdev)
:   cleanup resource files

**Parameters**

`struct pci_dev *pdev`
:   dev to cleanup

**Description**

If we created resource files for **pdev**, remove them from sysfs and
free their resources.

int pci\_create\_resource\_files(struct pci\_dev \*pdev)
:   create resource files in sysfs for **dev**

**Parameters**

`struct pci_dev *pdev`
:   dev in question

**Description**

Walk the resources in **pdev** creating files for each resource available.

ssize\_t pci\_write\_rom(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, struct kobject \*kobj, const struct bin\_attribute \*bin\_attr, char \*buf, loff\_t off, size\_t count)
:   used to enable access to the PCI ROM display

**Parameters**

`struct file *filp`
:   sysfs file

`struct kobject *kobj`
:   kernel object handle

`const struct bin_attribute *bin_attr`
:   `struct bin_attribute` for this file

`char *buf`
:   user input

`loff_t off`
:   file offset

`size_t count`
:   number of byte in input

**Description**

writing anything except 0 enables it

ssize\_t pci\_read\_rom(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp, struct kobject \*kobj, const struct bin\_attribute \*bin\_attr, char \*buf, loff\_t off, size\_t count)
:   read a PCI ROM

**Parameters**

`struct file *filp`
:   sysfs file

`struct kobject *kobj`
:   kernel object handle

`const struct bin_attribute *bin_attr`
:   `struct bin_attribute` for this file

`char *buf`
:   where to put the data we read from the ROM

`loff_t off`
:   file offset

`size_t count`
:   number of bytes to read

**Description**

Put **count** bytes starting at **off** into **buf** from the ROM in the PCI
device corresponding to **kobj**.

void pci\_remove\_sysfs\_dev\_files(struct pci\_dev \*pdev)
:   cleanup PCI specific sysfs files

**Parameters**

`struct pci_dev *pdev`
:   device whose entries we should free

**Description**

Cleanup when **pdev** is removed from sysfs.

u32 pcie\_tph\_get\_st\_table\_loc(struct pci\_dev \*pdev)
:   Return the device’s ST table location

**Parameters**

`struct pci_dev *pdev`
:   PCI device to query

**Return**

PCI\_TPH\_LOC\_NONE - Not present
PCI\_TPH\_LOC\_CAP - Located in the TPH Requester Extended Capability
PCI\_TPH\_LOC\_MSIX - Located in the MSI-X Table

int pcie\_tph\_get\_cpu\_st(struct pci\_dev \*pdev, enum tph\_mem\_type mem\_type, unsigned int cpu, u16 \*tag)
:   Retrieve Steering Tag for a target memory associated with a specific CPU

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`enum tph_mem_type mem_type`
:   target memory type (volatile or persistent RAM)

`unsigned int cpu`
:   associated CPU id

`u16 *tag`
:   Steering Tag to be returned

**Description**

Return the Steering Tag for a target memory that is associated with a
specific CPU as indicated by cpu.

**Return**

0 if success, otherwise negative value (-errno)

int pcie\_tph\_set\_st\_entry(struct pci\_dev \*pdev, unsigned int index, u16 tag)
:   Set Steering Tag in the ST table entry

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`unsigned int index`
:   ST table entry index

`u16 tag`
:   Steering Tag to be written

**Description**

Figure out the proper location of ST table, either in the MSI-X table or
in the TPH Extended Capability space, and write the Steering Tag into
the ST entry pointed by index.

**Return**

0 if success, otherwise negative value (-errno)

void pcie\_disable\_tph(struct pci\_dev \*pdev)
:   Turn off TPH support for device

**Parameters**

`struct pci_dev *pdev`
:   PCI device

**Return**

none

int pcie\_enable\_tph(struct pci\_dev \*pdev, int mode)
:   Enable TPH support for device using a specific ST mode

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`int mode`
:   ST mode to enable. Current supported modes include:

**Description**

> * PCI\_TPH\_ST\_NS\_MODE: NO ST Mode
> * PCI\_TPH\_ST\_IV\_MODE: Interrupt Vector Mode
> * PCI\_TPH\_ST\_DS\_MODE: Device Specific Mode

Check whether the mode is actually supported by the device before enabling
and return an error if not. Additionally determine what types of requests,
TPH or extended TPH, can be issued by the device based on its TPH requester
capability and the Root Port’s completer capability.

**Return**

0 on success, otherwise negative value (-errno)

# PCI Hotplug Support Library

int \_\_pci\_hp\_register(struct hotplug\_slot \*slot, struct pci\_bus \*bus, int devnr, const char \*name, struct module \*owner, const char \*mod\_name)
:   register a hotplug\_slot with the PCI hotplug subsystem

**Parameters**

`struct hotplug_slot *slot`
:   pointer to the `struct hotplug_slot` to register

`struct pci_bus *bus`
:   bus this slot is on

`int devnr`
:   device number

`const char *name`
:   name registered with kobject core

`struct module *owner`
:   caller module owner

`const char *mod_name`
:   caller module name

**Description**

Prepares a hotplug slot for in-kernel use and immediately publishes it to
user space in one go. Drivers may alternatively carry out the two steps
separately by invoking `pci_hp_initialize()` and [`pci_hp_add()`](#c.pci_hp_add "pci_hp_add").

Returns 0 if successful, anything else for an error.

int \_\_pci\_hp\_initialize(struct hotplug\_slot \*slot, struct pci\_bus \*bus, int devnr, const char \*name, struct module \*owner, const char \*mod\_name)
:   prepare hotplug slot for in-kernel use

**Parameters**

`struct hotplug_slot *slot`
:   pointer to the `struct hotplug_slot` to initialize

`struct pci_bus *bus`
:   bus this slot is on

`int devnr`
:   slot number

`const char *name`
:   name registered with kobject core

`struct module *owner`
:   caller module owner

`const char *mod_name`
:   caller module name

**Description**

Allocate and fill in a PCI slot for use by a hotplug driver. Once this has
been called, the driver may invoke `hotplug_slot_name()` to get the slot’s
unique name. The driver must be prepared to handle a ->reset\_slot callback
from this point on.

Returns 0 on success or a negative int on error.

int pci\_hp\_add(struct hotplug\_slot \*slot)
:   publish hotplug slot to user space

**Parameters**

`struct hotplug_slot *slot`
:   pointer to the `struct hotplug_slot` to publish

**Description**

Make a hotplug slot’s sysfs interface available and inform user space of its
addition by sending a uevent. The hotplug driver must be prepared to handle
all `struct hotplug_slot_ops` callbacks from this point on.

Returns 0 on success or a negative int on error.

void pci\_hp\_deregister(struct hotplug\_slot \*slot)
:   deregister a hotplug\_slot with the PCI hotplug subsystem

**Parameters**

`struct hotplug_slot *slot`
:   pointer to the `struct hotplug_slot` to deregister

**Description**

The **slot** must have been registered with the pci hotplug subsystem
previously with a call to `pci_hp_register()`.

void pci\_hp\_del(struct hotplug\_slot \*slot)
:   unpublish hotplug slot from user space

**Parameters**

`struct hotplug_slot *slot`
:   pointer to the `struct hotplug_slot` to unpublish

**Description**

Remove a hotplug slot’s sysfs interface.

void pci\_hp\_destroy(struct hotplug\_slot \*slot)
:   remove hotplug slot from in-kernel use

**Parameters**

`struct hotplug_slot *slot`
:   pointer to the `struct hotplug_slot` to destroy

**Description**

Destroy a PCI slot used by a hotplug driver. Once this has been called,
the driver may no longer invoke `hotplug_slot_name()` to get the slot’s
unique name. The driver no longer needs to handle a ->reset\_slot callback
from this point on.
