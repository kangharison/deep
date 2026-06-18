# Reset controller API

> 출처(원문): https://docs.kernel.org/driver-api/reset.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Reset controller API

## Introduction

Reset controllers are central units that control the reset signals to multiple
peripherals.
The reset controller API is split into two parts:
the [consumer driver interface](#consumer-driver-interface) ([API reference](#reset-consumer-api)), which allows peripheral drivers to request control
over their reset input signals, and the [reset controller driver interface](#reset-controller-driver-interface) ([API reference](#reset-controller-driver-api)), which is used by drivers for reset
controller devices to register their reset controls to provide them to the
consumers.

While some reset controller hardware units also implement system restart
functionality, restart handlers are out of scope for the reset controller API.

### Glossary

The reset controller API uses these terms with a specific meaning:

Reset line

> Physical reset line carrying a reset signal from a reset controller
> hardware unit to a peripheral module.

Reset control

> Control method that determines the state of one or multiple reset lines.
> Most commonly this is a single bit in reset controller register space that
> either allows direct control over the physical state of the reset line, or
> is self-clearing and can be used to trigger a predetermined pulse on the
> reset line.
> In more complicated reset controls, a single trigger action can launch a
> carefully timed sequence of pulses on multiple reset lines.

Reset controller

> A hardware module that provides a number of reset controls to control a
> number of reset lines.

Reset consumer

> Peripheral module or external IC that is put into reset by the signal on a
> reset line.

## Consumer driver interface

This interface provides an API that is similar to the kernel clock framework.
Consumer drivers use get and put operations to acquire and release reset
controls.
Functions are provided to assert and deassert the controlled reset lines,
trigger reset pulses, or to query reset line status.

When requesting reset controls, consumers can use symbolic names for their
reset inputs, which are mapped to an actual reset control on an existing reset
controller device by the core.

A stub version of this API is provided when the reset controller framework is
not in use in order to minimize the need to use ifdefs.

### Shared and exclusive resets

The reset controller API provides either reference counted deassertion and
assertion or direct, exclusive control.
The distinction between shared and exclusive reset controls is made at the time
the reset control is requested, either via [`devm_reset_control_get_shared()`](#c.devm_reset_control_get_shared "devm_reset_control_get_shared") or
via [`devm_reset_control_get_exclusive()`](#c.devm_reset_control_get_exclusive "devm_reset_control_get_exclusive").
This choice determines the behavior of the API calls made with the reset
control.

Shared resets behave similarly to clocks in the kernel clock framework.
They provide reference counted deassertion, where only the first deassert,
which increments the deassertion reference count to one, and the last assert
which decrements the deassertion reference count back to zero, have a physical
effect on the reset line.

Exclusive resets on the other hand guarantee direct control.
That is, an assert causes the reset line to be asserted immediately, and a
deassert causes the reset line to be deasserted immediately.

### Assertion and deassertion

Consumer drivers use the [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") and [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert")
functions to assert and deassert reset lines.
For shared reset controls, calls to the two functions must be balanced.

Note that since multiple consumers may be using a shared reset control, there
is no guarantee that calling [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") on a shared reset control
will actually cause the reset line to be asserted.
Consumer drivers using shared reset controls should assume that the reset line
may be kept deasserted at all times.
The API only guarantees that the reset line can not be asserted as long as any
consumer has requested it to be deasserted.

### Triggering

Consumer drivers use [`reset_control_reset()`](#c.reset_control_reset "reset_control_reset") to trigger a reset pulse on a
self-deasserting reset control.
In general, these resets can not be shared between multiple consumers, since
requesting a pulse from any consumer driver will reset all connected
peripherals.

The reset controller API allows requesting self-deasserting reset controls as
shared, but for those only the first trigger request causes an actual pulse to
be issued on the reset line.
All further calls to this function have no effect until all consumers have
called [`reset_control_rearm()`](#c.reset_control_rearm "reset_control_rearm").
For shared reset controls, calls to the two functions must be balanced.
This allows devices that only require an initial reset at any point before the
driver is probed or resumed to share a pulsed reset line.

### Querying

Only some reset controllers support querying the current status of a reset
line, via [`reset_control_status()`](#c.reset_control_status "reset_control_status").
If supported, this function returns a positive non-zero value if the given
reset line is asserted.
The [`reset_control_status()`](#c.reset_control_status "reset_control_status") function does not accept a
[reset control array](#reset-control-arrays) handle as its input parameter.

### Optional resets

Often peripherals require a reset line on some platforms but not on others.
For this, reset controls can be requested as optional using
[`devm_reset_control_get_optional_exclusive()`](#c.devm_reset_control_get_optional_exclusive "devm_reset_control_get_optional_exclusive") or
[`devm_reset_control_get_optional_shared()`](#c.devm_reset_control_get_optional_shared "devm_reset_control_get_optional_shared").
These functions return a NULL pointer instead of an error when the requested
reset control is not specified in the device tree.
Passing a NULL pointer to the reset\_control functions causes them to return
quietly without an error.

### Reset control arrays

Some drivers need to assert a bunch of reset lines in no particular order.
[`devm_reset_control_array_get()`](#c.devm_reset_control_array_get "devm_reset_control_array_get") returns an opaque reset control handle that can
be used to assert, deassert, or trigger all specified reset controls at once.
The reset control API does not guarantee the order in which the individual
controls therein are handled.

## Reset controller driver interface

Drivers for reset controller modules provide the functionality necessary to
assert or deassert reset signals, to trigger a reset pulse on a reset line, or
to query its current state.
All functions are optional.

### Initialization

Drivers fill a struct [`reset_controller_dev`](#c.reset_controller_dev "reset_controller_dev") and register it with
[`reset_controller_register()`](#c.reset_controller_register "reset_controller_register") in their probe function.
The actual functionality is implemented in callback functions via a struct
[`reset_control_ops`](#c.reset_control_ops "reset_control_ops").

## API reference

The reset controller API is documented here in two parts:
the [reset consumer API](#reset-consumer-api) and the [reset controller
driver API](#reset-controller-driver-api).

### Reset consumer API

Reset consumers can control a reset line using an opaque reset control handle,
which can be obtained from [`devm_reset_control_get_exclusive()`](#c.devm_reset_control_get_exclusive "devm_reset_control_get_exclusive") or
[`devm_reset_control_get_shared()`](#c.devm_reset_control_get_shared "devm_reset_control_get_shared").
Given the reset control, consumers can call [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") and
[`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert"), trigger a reset pulse using [`reset_control_reset()`](#c.reset_control_reset "reset_control_reset"), or
query the reset line status using [`reset_control_status()`](#c.reset_control_status "reset_control_status").

struct reset\_control\_bulk\_data
:   Data used for bulk reset control operations.

**Definition**:

```
struct reset_control_bulk_data {
    const char                      *id;
    struct reset_control            *rstc;
};
```

**Members**

`id`
:   reset control consumer ID

`rstc`
:   `struct reset_control` \* to store the associated reset control

**Description**

The reset APIs provide a series of reset\_control\_bulk\_\*() API calls as
a convenience to consumers which require multiple reset controls.
This structure is used to manage data for these calls.

enum reset\_control\_flags
:   Flags that can be passed to the reset\_control\_get functions to determine the type of reset control. These values cannot be OR’d.

**Constants**

`RESET_CONTROL_EXCLUSIVE`
:   exclusive, acquired,

`RESET_CONTROL_EXCLUSIVE_DEASSERTED`
:   exclusive, acquired, deasserted

`RESET_CONTROL_EXCLUSIVE_RELEASED`
:   exclusive, released,

`RESET_CONTROL_SHARED`
:   shared

`RESET_CONTROL_SHARED_DEASSERTED`
:   shared, deasserted

`RESET_CONTROL_OPTIONAL_EXCLUSIVE`
:   optional, exclusive, acquired

`RESET_CONTROL_OPTIONAL_EXCLUSIVE_DEASSERTED`
:   optional, exclusive, acquired, deasserted

`RESET_CONTROL_OPTIONAL_EXCLUSIVE_RELEASED`
:   optional, exclusive, released

`RESET_CONTROL_OPTIONAL_SHARED`
:   optional, shared

`RESET_CONTROL_OPTIONAL_SHARED_DEASSERTED`
:   optional, shared, deasserted

struct reset\_control \*reset\_control\_get\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   Lookup and obtain an exclusive reference to a reset controller.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.
If this function is called more than once for the same reset\_control it will
return -EBUSY.

See [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared") for details on shared references to
reset-controls.

Use of id names is optional.

int reset\_control\_bulk\_get\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   Lookup and obtain exclusive references to multiple reset controllers.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Fills the rstcs array with pointers to exclusive reset controls and
returns 0, or an [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.

struct reset\_control \*reset\_control\_get\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   Lookup and obtain a temoprarily exclusive reference to a reset controller.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.
reset-controls returned by this function must be acquired via
[`reset_control_acquire()`](#c.reset_control_acquire "reset_control_acquire") before they can be used and should be released
via [`reset_control_release()`](#c.reset_control_release "reset_control_release") afterwards.

Use of id names is optional.

int reset\_control\_bulk\_get\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   Lookup and obtain temporarily exclusive references to multiple reset controllers.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Fills the rstcs array with pointers to exclusive reset controls and
returns 0, or an [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.
reset-controls returned by this function must be acquired via
`reset_control_bulk_acquire()` before they can be used and should be released
via `reset_control_bulk_release()` afterwards.

int reset\_control\_bulk\_get\_optional\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   Lookup and obtain optional temporarily exclusive references to multiple reset controllers.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Optional variant of [`reset_control_bulk_get_exclusive_released()`](#c.reset_control_bulk_get_exclusive_released "reset_control_bulk_get_exclusive_released"). If the
requested reset is not specified in the device tree, this function returns 0
instead of an error and missing rtsc is set to NULL.

See [`reset_control_bulk_get_exclusive_released()`](#c.reset_control_bulk_get_exclusive_released "reset_control_bulk_get_exclusive_released") for more information.

struct reset\_control \*reset\_control\_get\_shared(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   Lookup and obtain a shared reference to a reset controller.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.
This function is intended for use with reset-controls which are shared
between hardware blocks.

When a reset-control is shared, the behavior of reset\_control\_assert /
deassert is changed, the reset-core will keep track of a deassert\_count
and only (re-)assert the reset after reset\_control\_assert has been called
as many times as reset\_control\_deassert was called. Also see the remark
about shared reset-controls in the reset\_control\_assert docs.

Calling reset\_control\_assert without first calling reset\_control\_deassert
is not allowed on a shared reset control. Calling reset\_control\_reset is
also not allowed on a shared reset control.

Use of id names is optional.

int reset\_control\_bulk\_get\_shared(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   Lookup and obtain shared references to multiple reset controllers.

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Fills the rstcs array with pointers to shared reset controls and
returns 0, or an [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.

struct reset\_control \*reset\_control\_get\_optional\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   optional [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Optional variant of [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive"). If the requested reset
is not specified in the device tree, this function returns NULL instead of
an error.

See [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive") for more information.

int reset\_control\_bulk\_get\_optional\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   optional [`reset_control_bulk_get_exclusive()`](#c.reset_control_bulk_get_exclusive "reset_control_bulk_get_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Optional variant of [`reset_control_bulk_get_exclusive()`](#c.reset_control_bulk_get_exclusive "reset_control_bulk_get_exclusive"). If any of the
requested resets are not specified in the device tree, this function sets
them to NULL instead of returning an error.

See [`reset_control_bulk_get_exclusive()`](#c.reset_control_bulk_get_exclusive "reset_control_bulk_get_exclusive") for more information.

struct reset\_control \*reset\_control\_get\_optional\_shared(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   optional [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Optional variant of [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared"). If the requested reset
is not specified in the device tree, this function returns NULL instead of
an error.

See [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared") for more information.

int reset\_control\_bulk\_get\_optional\_shared(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   optional [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Optional variant of [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared"). If the requested resets
are not specified in the device tree, this function sets them to NULL
instead of returning an error.

See [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared") for more information.

struct reset\_control \*of\_reset\_control\_get\_exclusive(struct device\_node \*node, const char \*id)
:   Lookup and obtain an exclusive reference to a reset controller.

**Parameters**

`struct device_node *node`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.

Use of id names is optional.

struct reset\_control \*of\_reset\_control\_get\_optional\_exclusive(struct device\_node \*node, const char \*id)
:   Lookup and obtain an optional exclusive reference to a reset controller.

**Parameters**

`struct device_node *node`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Optional variant of [`of_reset_control_get_exclusive()`](#c.of_reset_control_get_exclusive "of_reset_control_get_exclusive"). If the requested reset
is not specified in the device tree, this function returns NULL instead of
an error.

Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.

Use of id names is optional.

struct reset\_control \*of\_reset\_control\_get\_shared(struct device\_node \*node, const char \*id)
:   Lookup and obtain a shared reference to a reset controller.

**Parameters**

`struct device_node *node`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

When a reset-control is shared, the behavior of reset\_control\_assert /
deassert is changed, the reset-core will keep track of a deassert\_count
and only (re-)assert the reset after reset\_control\_assert has been called
as many times as reset\_control\_deassert was called. Also see the remark
about shared reset-controls in the reset\_control\_assert docs.

Calling reset\_control\_assert without first calling reset\_control\_deassert
is not allowed on a shared reset control. Calling reset\_control\_reset is
also not allowed on a shared reset control.
Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.

Use of id names is optional.

struct reset\_control \*of\_reset\_control\_get\_exclusive\_by\_index(struct device\_node \*node, int index)
:   Lookup and obtain an exclusive reference to a reset controller by index.

**Parameters**

`struct device_node *node`
:   device to be reset by the controller

`int index`
:   index of the reset controller

**Description**

This is to be used to perform a list of resets for a device or power domain
in whatever order. Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition
containing errno.

struct reset\_control \*of\_reset\_control\_get\_shared\_by\_index(struct device\_node \*node, int index)
:   Lookup and obtain a shared reference to a reset controller by index.

**Parameters**

`struct device_node *node`
:   device to be reset by the controller

`int index`
:   index of the reset controller

**Description**

When a reset-control is shared, the behavior of reset\_control\_assert /
deassert is changed, the reset-core will keep track of a deassert\_count
and only (re-)assert the reset after reset\_control\_assert has been called
as many times as reset\_control\_deassert was called. Also see the remark
about shared reset-controls in the reset\_control\_assert docs.

Calling reset\_control\_assert without first calling reset\_control\_deassert
is not allowed on a shared reset control. Calling reset\_control\_reset is
also not allowed on a shared reset control.
Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing errno.

This is to be used to perform a list of resets for a device or power domain
in whatever order. Returns a `struct reset_control` or [`IS_ERR()`](../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition
containing errno.

struct reset\_control \*devm\_reset\_control\_get\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive"). For reset controllers returned
from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver
detach.

See [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive") for more information.

struct reset\_control \*devm\_reset\_control\_get\_exclusive\_deasserted(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert"). For reset
controllers returned from this function, [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") +
[`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver detach.

See [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive") for more information.

int devm\_reset\_control\_bulk\_get\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed [`reset_control_bulk_get_exclusive()`](#c.reset_control_bulk_get_exclusive "reset_control_bulk_get_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed [`reset_control_bulk_get_exclusive()`](#c.reset_control_bulk_get_exclusive "reset_control_bulk_get_exclusive"). For reset controllers returned
from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver
detach.

See [`reset_control_bulk_get_exclusive()`](#c.reset_control_bulk_get_exclusive "reset_control_bulk_get_exclusive") for more information.

struct reset\_control \*devm\_reset\_control\_get\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_exclusive_released()`](#c.reset_control_get_exclusive_released "reset_control_get_exclusive_released")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_exclusive_released()`](#c.reset_control_get_exclusive_released "reset_control_get_exclusive_released"). For reset controllers
returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on
driver detach.

See [`reset_control_get_exclusive_released()`](#c.reset_control_get_exclusive_released "reset_control_get_exclusive_released") for more information.

int devm\_reset\_control\_bulk\_get\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed [`reset_control_bulk_get_exclusive_released()`](#c.reset_control_bulk_get_exclusive_released "reset_control_bulk_get_exclusive_released")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed [`reset_control_bulk_get_exclusive_released()`](#c.reset_control_bulk_get_exclusive_released "reset_control_bulk_get_exclusive_released"). For reset controllers
returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on
driver detach.

See [`reset_control_bulk_get_exclusive_released()`](#c.reset_control_bulk_get_exclusive_released "reset_control_bulk_get_exclusive_released") for more information.

struct reset\_control \*devm\_reset\_control\_get\_optional\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed `reset_control_get_optional_exclusive_released()`

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed-and-optional variant of [`reset_control_get_exclusive_released()`](#c.reset_control_get_exclusive_released "reset_control_get_exclusive_released"). For
reset controllers returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called
automatically on driver detach.

See [`reset_control_get_exclusive_released()`](#c.reset_control_get_exclusive_released "reset_control_get_exclusive_released") for more information.

int devm\_reset\_control\_bulk\_get\_optional\_exclusive\_released(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed `reset_control_bulk_optional_get_exclusive_released()`

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed `reset_control_bulk_optional_get_exclusive_released()`. For reset
controllers returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called
automatically on driver detach.

See `reset_control_bulk_optional_get_exclusive_released()` for more information.

struct reset\_control \*devm\_reset\_control\_get\_shared(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared"). For reset controllers returned from
this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver detach.
See [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared") for more information.

struct reset\_control \*devm\_reset\_control\_get\_shared\_deasserted(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert"). For reset
controllers returned from this function, [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") +
[`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver detach.

See [`devm_reset_control_get_shared()`](#c.devm_reset_control_get_shared "devm_reset_control_get_shared") for more information.

int devm\_reset\_control\_bulk\_get\_shared(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared"). For reset controllers returned
from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver
detach.

See [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared") for more information.

int devm\_reset\_control\_bulk\_get\_shared\_deasserted(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared") + `reset_control_bulk_deassert()`

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed [`reset_control_bulk_get_shared()`](#c.reset_control_bulk_get_shared "reset_control_bulk_get_shared") + `reset_control_bulk_deassert()`. For
reset controllers returned from this function, `reset_control_bulk_assert()` +
`reset_control_bulk_put()` are called automatically on driver detach.

See [`devm_reset_control_bulk_get_shared()`](#c.devm_reset_control_bulk_get_shared "devm_reset_control_bulk_get_shared") for more information.

struct reset\_control \*devm\_reset\_control\_get\_optional\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_optional_exclusive()`](#c.reset_control_get_optional_exclusive "reset_control_get_optional_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_optional_exclusive()`](#c.reset_control_get_optional_exclusive "reset_control_get_optional_exclusive"). For reset controllers
returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on
driver detach.

See [`reset_control_get_optional_exclusive()`](#c.reset_control_get_optional_exclusive "reset_control_get_optional_exclusive") for more information.

struct reset\_control \*devm\_reset\_control\_get\_optional\_exclusive\_deasserted(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_optional_exclusive()`](#c.reset_control_get_optional_exclusive "reset_control_get_optional_exclusive") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_optional_exclusive()`](#c.reset_control_get_optional_exclusive "reset_control_get_optional_exclusive") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert").
For reset controllers returned from this function, [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") +
[`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver detach.

See [`devm_reset_control_get_optional_exclusive()`](#c.devm_reset_control_get_optional_exclusive "devm_reset_control_get_optional_exclusive") for more information.

int devm\_reset\_control\_bulk\_get\_optional\_exclusive(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed [`reset_control_bulk_get_optional_exclusive()`](#c.reset_control_bulk_get_optional_exclusive "reset_control_bulk_get_optional_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed [`reset_control_bulk_get_optional_exclusive()`](#c.reset_control_bulk_get_optional_exclusive "reset_control_bulk_get_optional_exclusive"). For reset controllers
returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on
driver detach.

See [`reset_control_bulk_get_optional_exclusive()`](#c.reset_control_bulk_get_optional_exclusive "reset_control_bulk_get_optional_exclusive") for more information.

struct reset\_control \*devm\_reset\_control\_get\_optional\_shared(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_optional_shared()`](#c.reset_control_get_optional_shared "reset_control_get_optional_shared")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_optional_shared()`](#c.reset_control_get_optional_shared "reset_control_get_optional_shared"). For reset controllers returned
from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver
detach.

See [`reset_control_get_optional_shared()`](#c.reset_control_get_optional_shared "reset_control_get_optional_shared") for more information.

struct reset\_control \*devm\_reset\_control\_get\_optional\_shared\_deasserted(struct [device](infrastructure.html#c.device "device") \*dev, const char \*id)
:   resource managed [`reset_control_get_optional_shared()`](#c.reset_control_get_optional_shared "reset_control_get_optional_shared") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`const char *id`
:   reset line name

**Description**

Managed [`reset_control_get_optional_shared()`](#c.reset_control_get_optional_shared "reset_control_get_optional_shared") + [`reset_control_deassert()`](#c.reset_control_deassert "reset_control_deassert"). For
reset controllers returned from this function, [`reset_control_assert()`](#c.reset_control_assert "reset_control_assert") +
[`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver detach.

See [`devm_reset_control_get_optional_shared()`](#c.devm_reset_control_get_optional_shared "devm_reset_control_get_optional_shared") for more information.

int devm\_reset\_control\_bulk\_get\_optional\_shared(struct [device](infrastructure.html#c.device "device") \*dev, int num\_rstcs, struct [reset\_control\_bulk\_data](#c.reset_control_bulk_data "reset_control_bulk_data") \*rstcs)
:   resource managed [`reset_control_bulk_get_optional_shared()`](#c.reset_control_bulk_get_optional_shared "reset_control_bulk_get_optional_shared")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int num_rstcs`
:   number of entries in rstcs array

`struct reset_control_bulk_data *rstcs`
:   array of [`struct reset_control_bulk_data`](#c.reset_control_bulk_data "reset_control_bulk_data") with reset line names set

**Description**

Managed [`reset_control_bulk_get_optional_shared()`](#c.reset_control_bulk_get_optional_shared "reset_control_bulk_get_optional_shared"). For reset controllers
returned from this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on
driver detach.

See [`reset_control_bulk_get_optional_shared()`](#c.reset_control_bulk_get_optional_shared "reset_control_bulk_get_optional_shared") for more information.

struct reset\_control \*devm\_reset\_control\_get\_exclusive\_by\_index(struct [device](infrastructure.html#c.device "device") \*dev, int index)
:   resource managed [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive")

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int index`
:   index of the reset controller

**Description**

Managed [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive"). For reset controllers returned from
this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver
detach.

See [`reset_control_get_exclusive()`](#c.reset_control_get_exclusive "reset_control_get_exclusive") for more information.

struct reset\_control \*devm\_reset\_control\_get\_shared\_by\_index(struct [device](infrastructure.html#c.device "device") \*dev, int index)
:   resource managed reset\_control\_get\_shared

**Parameters**

`struct device *dev`
:   device to be reset by the controller

`int index`
:   index of the reset controller

**Description**

Managed [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared"). For reset controllers returned from
this function, [`reset_control_put()`](#c.reset_control_put "reset_control_put") is called automatically on driver detach.
See [`reset_control_get_shared()`](#c.reset_control_get_shared "reset_control_get_shared") for more information.

int reset\_control\_reset(struct reset\_control \*rstc)
:   reset the controlled device

**Parameters**

`struct reset_control *rstc`
:   reset controller

**Description**

On a shared reset line the actual reset pulse is only triggered once for the
lifetime of the reset\_control instance: for all but the first caller this is
a no-op.
Consumers must not use reset\_control\_(de)assert on shared reset lines when
reset\_control\_reset has been used.

If rstc is NULL it is an optional reset and the function will just
return 0.

int reset\_control\_rearm(struct reset\_control \*rstc)
:   allow shared reset line to be re-triggered

**Parameters**

`struct reset_control *rstc`
:   reset controller

**Description**

On a shared reset line the actual reset pulse is only triggered once for the
lifetime of the reset\_control instance, except if this call is used.

Calls to this function must be balanced with calls to reset\_control\_reset,
a warning is thrown in case triggered\_count ever dips below 0.

Consumers must not use reset\_control\_(de)assert on shared reset lines when
reset\_control\_reset or reset\_control\_rearm have been used.

If rstc is NULL the function will just return 0.

int reset\_control\_assert(struct reset\_control \*rstc)
:   asserts the reset line

**Parameters**

`struct reset_control *rstc`
:   reset controller

**Description**

Calling this on an exclusive reset controller guarantees that the reset
will be asserted. When called on a shared reset controller the line may
still be deasserted, as long as other users keep it so.

For shared reset controls a driver cannot expect the hw’s registers and
internal state to be reset, but must be prepared for this to happen.
Consumers must not use reset\_control\_reset on shared reset lines when
reset\_control\_(de)assert has been used.

If rstc is NULL it is an optional reset and the function will just
return 0.

int reset\_control\_deassert(struct reset\_control \*rstc)
:   deasserts the reset line

**Parameters**

`struct reset_control *rstc`
:   reset controller

**Description**

After calling this function, the reset is guaranteed to be deasserted.
Consumers must not use reset\_control\_reset on shared reset lines when
reset\_control\_(de)assert has been used.

If rstc is NULL it is an optional reset and the function will just
return 0.

int reset\_control\_status(struct reset\_control \*rstc)
:   returns a negative errno if not supported, a positive value if the reset line is asserted, or zero if the reset line is not asserted or if the desc is NULL (optional reset).

**Parameters**

`struct reset_control *rstc`
:   reset controller

int reset\_control\_acquire(struct reset\_control \*rstc)
:   acquires a reset control for exclusive use

**Parameters**

`struct reset_control *rstc`
:   reset control

**Description**

This is used to explicitly acquire a reset control for exclusive use. Note
that exclusive resets are requested as acquired by default. In order for a
second consumer to be able to control the reset, the first consumer has to
release it first. Typically the easiest way to achieve this is to call the
[`reset_control_get_exclusive_released()`](#c.reset_control_get_exclusive_released "reset_control_get_exclusive_released") to obtain an instance of the reset
control. Such reset controls are not acquired by default.

Consumers implementing shared access to an exclusive reset need to follow
a specific protocol in order to work together. Before consumers can change
a reset they must acquire exclusive access using [`reset_control_acquire()`](#c.reset_control_acquire "reset_control_acquire").
After they are done operating the reset, they must release exclusive access
with a call to [`reset_control_release()`](#c.reset_control_release "reset_control_release"). Consumers are not granted exclusive
access to the reset as long as another consumer hasn’t released a reset.

See also: [`reset_control_release()`](#c.reset_control_release "reset_control_release")

void reset\_control\_release(struct reset\_control \*rstc)
:   releases exclusive access to a reset control

**Parameters**

`struct reset_control *rstc`
:   reset control

**Description**

Releases exclusive access right to a reset control previously obtained by a
call to [`reset_control_acquire()`](#c.reset_control_acquire "reset_control_acquire"). Until a consumer calls this function, no
other consumers will be granted exclusive access.

See also: [`reset_control_acquire()`](#c.reset_control_acquire "reset_control_acquire")

void reset\_control\_put(struct reset\_control \*rstc)
:   free the reset controller

**Parameters**

`struct reset_control *rstc`
:   reset controller

struct reset\_control \*devm\_reset\_control\_array\_get(struct [device](infrastructure.html#c.device "device") \*dev, enum [reset\_control\_flags](#c.reset_control_flags "reset_control_flags") flags)
:   Resource managed reset control array get

**Parameters**

`struct device *dev`
:   device that requests the list of reset controls

`enum reset_control_flags flags`
:   whether reset controls are shared, optional, acquired

**Description**

The reset control array APIs are intended for a list of resets
that just have to be asserted or deasserted, without any
requirements on the order.

Returns pointer to allocated reset\_control on success or error on failure

int reset\_control\_get\_count(struct [device](infrastructure.html#c.device "device") \*dev)
:   Count number of resets available with a device

**Parameters**

`struct device *dev`
:   device for which to return the number of resets

**Description**

Returns positive reset count on success, or error number on failure and
on count being zero.

### Reset controller driver API

Reset controller drivers are supposed to implement the necessary functions in
a static constant structure [`reset_control_ops`](#c.reset_control_ops "reset_control_ops"), allocate and fill out
a struct [`reset_controller_dev`](#c.reset_controller_dev "reset_controller_dev"), and register it using
[`devm_reset_controller_register()`](#c.devm_reset_controller_register "devm_reset_controller_register").

struct reset\_control\_ops
:   reset controller driver callbacks

**Definition**:

```
struct reset_control_ops {
    int (*reset)(struct reset_controller_dev *rcdev, unsigned long id);
    int (*assert)(struct reset_controller_dev *rcdev, unsigned long id);
    int (*deassert)(struct reset_controller_dev *rcdev, unsigned long id);
    int (*status)(struct reset_controller_dev *rcdev, unsigned long id);
};
```

**Members**

`reset`
:   for self-deasserting resets, does all necessary
    things to reset the device

`assert`
:   manually assert the reset line, if supported

`deassert`
:   manually deassert the reset line, if supported

`status`
:   return the status of the reset line, if supported

struct reset\_controller\_dev
:   reset controller entity that might provide multiple reset controls

**Definition**:

```
struct reset_controller_dev {
    const struct reset_control_ops *ops;
    struct module *owner;
    struct list_head list;
    struct list_head reset_control_head;
    struct device *dev;
    struct device_node *of_node;
    int of_reset_n_cells;
    int (*of_xlate)(struct reset_controller_dev *rcdev, const struct of_phandle_args *reset_spec);
    struct fwnode_handle *fwnode;
    int fwnode_reset_n_cells;
    int (*fwnode_xlate)(struct reset_controller_dev *rcdev, const struct fwnode_reference_args *reset_spec);
    unsigned int nr_resets;
    struct mutex lock;
};
```

**Members**

`ops`
:   a pointer to device specific [`struct reset_control_ops`](#c.reset_control_ops "reset_control_ops")

`owner`
:   kernel module of the reset controller driver

`list`
:   internal list of reset controller devices

`reset_control_head`
:   head of internal list of requested reset controls

`dev`
:   corresponding driver model device struct

`of_node`
:   corresponding device tree node as phandle target

`of_reset_n_cells`
:   number of cells in reset line specifiers

`of_xlate`
:   translation function to translate from specifier as found in the
    device tree to id as given to the reset control ops

`fwnode`
:   firmware node associated with this device

`fwnode_reset_n_cells`
:   number of cells in reset line specifiers

`fwnode_xlate`
:   translation function to translate from firmware specifier to
    id as given to the reset control ops, defaults to
    `fwnode_reset_simple_xlate()`

`nr_resets`
:   number of reset controls in this reset controller device

`lock`
:   protects the reset control list from concurrent access

int reset\_controller\_register(struct [reset\_controller\_dev](#c.reset_controller_dev "reset_controller_dev") \*rcdev)
:   register a reset controller device

**Parameters**

`struct reset_controller_dev *rcdev`
:   a pointer to the initialized reset controller device

void reset\_controller\_unregister(struct [reset\_controller\_dev](#c.reset_controller_dev "reset_controller_dev") \*rcdev)
:   unregister a reset controller device

**Parameters**

`struct reset_controller_dev *rcdev`
:   a pointer to the reset controller device

int devm\_reset\_controller\_register(struct [device](infrastructure.html#c.device "device") \*dev, struct [reset\_controller\_dev](#c.reset_controller_dev "reset_controller_dev") \*rcdev)
:   resource managed [`reset_controller_register()`](#c.reset_controller_register "reset_controller_register")

**Parameters**

`struct device *dev`
:   device that is registering this reset controller

`struct reset_controller_dev *rcdev`
:   a pointer to the initialized reset controller device

**Description**

Managed [`reset_controller_register()`](#c.reset_controller_register "reset_controller_register"). For reset controllers registered by
this function, [`reset_controller_unregister()`](#c.reset_controller_unregister "reset_controller_unregister") is automatically called on
driver detach. See [`reset_controller_register()`](#c.reset_controller_register "reset_controller_register") for more information.
