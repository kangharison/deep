# Pcode

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_pcode.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Pcode

Xe PCODE is the component responsible for interfacing with the PCODE
firmware.
It shall provide a very simple ABI to other Xe components, but be the
single and consolidated place that will communicate with PCODE. All read
and write operations to PCODE will be internal and private to this component.

What’s next:
- PCODE hw metrics
- PCODE for display operations

## Internal API

int xe\_pcode\_request(struct xe\_tile \*tile, u32 mbox, u32 request, u32 reply\_mask, u32 reply, int timeout\_base\_ms)
:   send PCODE request until acknowledgment

**Parameters**

`struct xe_tile *tile`
:   tile

`u32 mbox`
:   PCODE mailbox ID the request is targeted for

`u32 request`
:   request ID

`u32 reply_mask`
:   mask used to check for request acknowledgment

`u32 reply`
:   value used to check for request acknowledgment

`int timeout_base_ms`
:   timeout for polling with preemption enabled

**Description**

Keep resending the **request** to **mbox** until PCODE acknowledges it, PCODE
reports an error or an overall timeout of **timeout\_base\_ms\*\*+50 ms expires.
The request is acknowledged once the PCODE reply dword equals \*\*reply** after
applying **reply\_mask**. Polling is first attempted with preemption enabled
for **timeout\_base\_ms** and if this times out for another 50 ms with
preemption disabled.

Returns 0 on success, `-ETIMEDOUT` in case of a timeout, <0 in case of some
other error as reported by PCODE.

int xe\_pcode\_init\_min\_freq\_table(struct xe\_tile \*tile, u32 min\_gt\_freq, u32 max\_gt\_freq)
:   Initialize PCODE’s QOS frequency table

**Parameters**

`struct xe_tile *tile`
:   tile instance

`u32 min_gt_freq`
:   Minimal (RPn) GT frequency in units of 50MHz.

`u32 max_gt_freq`
:   Maximal (RP0) GT frequency in units of 50MHz.

**Description**

This function initialize PCODE’s QOS frequency table for a proper minimal
frequency/power steering decision, depending on the current requested GT
frequency. For older platforms this was a more complete table including
the IA freq. However for the latest platforms this table become a simple
1-1 Ring vs GT frequency. Even though, without setting it, PCODE might
not take the right decisions for some memory frequencies and affect latency.

It returns 0 on success, and -ERROR number on failure, -EINVAL if max
frequency is higher then the minimal, and other errors directly translated
from the PCODE Error returns:
- -ENXIO: “Illegal Command”
- -ETIMEDOUT: “Timed out”
- -EINVAL: “Illegal Data”
- -ENXIO, “Illegal Subcommand”
- -EBUSY: “PCODE Locked”
- -EOVERFLOW, “GT ratio out of range”
- -EACCES, “PCODE Rejected”
- -EPROTO, “Unknown”

int xe\_pcode\_ready(struct xe\_device \*xe, bool locked)
:   Ensure PCODE is initialized

**Parameters**

`struct xe_device *xe`
:   xe instance

`bool locked`
:   true if lock held, false otherwise

**Description**

PCODE init mailbox is polled only on root gt of root tile
as the root tile provides the initialization is complete only
after all the tiles have completed the initialization.
Called only on early probe without locks and with locks in
resume path.

Returns 0 on success, and -error number on failure.

void xe\_pcode\_init(struct xe\_tile \*tile)
:   initialize components of PCODE

**Parameters**

`struct xe_tile *tile`
:   tile instance

**Description**

This function initializes the xe\_pcode component.
To be called once only during probe.

int xe\_pcode\_probe\_early(struct xe\_device \*xe)
:   initializes PCODE

**Parameters**

`struct xe_device *xe`
:   xe instance

**Description**

This function checks the initialization status of PCODE
To be called once only during early probe without locks.

Returns 0 on success, error code otherwise

# Survivability Mode

Survivability Mode is a software based workflow for recovering a system in a failed boot state
Here system recoverability is concerned with recovering the firmware responsible for boot.

## Boot Survivability

Boot Survivability is implemented by loading the driver with bare minimum (no drm card) to allow
the firmware to be flashed through mei driver and collect telemetry. The driver’s probe flow is
modified such that it enters survivability mode when pcode initialization is incomplete and boot
status denotes a failure.

Survivability mode can also be entered manually using the survivability mode attribute available
through configfs which is beneficial in several usecases. It can be used to address scenarios
where pcode does not detect failure or for validation purposes. It can also be used in
In-Field-Repair (IFR) to repair a single card without impacting the other cards in a node.

Use below command enable survivability mode manually:

```
# echo 1 > /sys/kernel/config/xe/0000:03:00.0/survivability_mode
```

It is the responsibility of the user to clear the mode once firmware flash is complete.

Refer [Xe Configfs](xe_configfs.html#xe-configfs) for more details on how to use configfs

Survivability mode is indicated by the below admin-only readable sysfs entry. It
provides information about the type of survivability mode (Boot/Runtime).

```
# cat /sys/bus/pci/devices/<device>/survivability_mode
  Boot
```

Any additional debug information if present will be visible under the directory
`survivability_info`:

```
/sys/bus/pci/devices/<device>/survivability_info/
├── aux_info0
├── aux_info1
├── aux_info2
├── aux_info3
├── aux_info4
├── capability_info
├── fdo_mode
├── postcode_trace
└── postcode_trace_overflow
```

This directory has the following attributes

* `capability_info` : Indicates Boot status and support for additional information
* `postcode_trace`, `postcode_trace_overflow` : Each postcode is a 8bit value and
  represents a boot failure event. When a new failure event is logged by PCODE the
  existing postcodes are shifted left. These entries provide a history of 8 postcodes.
* `aux_info<n>` : Some failures have additional debug information
* `fdo_mode` : To allow recovery in scenarios where MEI itself fails, a new SPI Flash
  Descriptor Override (FDO) mode is added in v2 survivability breadcrumbs. This mode is enabled
  by PCODE and provides the ability to directly update the firmware via SPI Driver without
  any dependency on MEI. Xe KMD initializes the nvm aux driver if FDO mode is enabled.

## Runtime Survivability

Certain runtime firmware errors can cause the device to enter a wedged state
([Xe Device Wedging](xe_device.html#xe-device-wedging)) requiring a firmware flash to restore normal operation.
Runtime Survivability Mode indicates that a firmware flash is necessary to recover the device and
is indicated by the presence of survivability mode sysfs.
Survivability mode sysfs provides information about the type of survivability mode.

```
# cat /sys/bus/pci/devices/<device>/survivability_mode
  Runtime
```

When such errors occur, userspace is notified with the drm device wedged uevent and runtime
survivability mode. User can then initiate a firmware flash using userspace tools like fwupd
to restore device to normal operation.
