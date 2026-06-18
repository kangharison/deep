# API for USB Type-C Alternate Mode drivers

> 출처(원문): https://docs.kernel.org/driver-api/usb/typec_bus.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# API for USB Type-C Alternate Mode drivers

## Introduction

Alternate modes require communication with the partner using Vendor Defined
Messages (VDM) as defined in USB Type-C and USB Power Delivery Specifications.
The communication is SVID (Standard or Vendor ID) specific, i.e. specific for
every alternate mode, so every alternate mode will need a custom driver.

USB Type-C bus allows binding a driver to the discovered partner alternate
modes by using the SVID and the mode number.

[USB Type-C Connector Class](typec.html#typec) provides a device for every alternate
mode a port supports, and separate device for every alternate mode the partner
supports. The drivers for the alternate modes are bound to the partner alternate
mode devices, and the port alternate mode devices must be handled by the port
drivers.

When a new partner alternate mode device is registered, it is linked to the
alternate mode device of the port that the partner is attached to, that has
matching SVID and mode. Communication between the port driver and alternate mode
driver will happen using the same API.

The port alternate mode devices are used as a proxy between the partner and the
alternate mode drivers, so the port drivers are only expected to pass the SVID
specific commands from the alternate mode drivers to the partner, and from the
partners to the alternate mode drivers. No direct SVID specific communication is
needed from the port drivers, but the port drivers need to provide the operation
callbacks for the port alternate mode devices, just like the alternate mode
drivers need to provide them for the partner alternate mode devices.

## Usage:

### General

By default, the alternate mode drivers are responsible for entering the mode.
It is also possible to leave the decision about entering the mode to the user
space (See [ABI file testing/sysfs-class-typec](../../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-class-typec)). Port drivers should not
enter any modes on their own.

`->vdm` is the most important callback in the operation callbacks vector. It
will be used to deliver all the SVID specific commands from the partner to the
alternate mode driver, and vice versa in case of port drivers. The drivers send
the SVID specific commands to each other using [`typec_altmode_vdm()`](#c.typec_altmode_vdm "typec_altmode_vdm").

If the communication with the partner using the SVID specific commands results
in need to reconfigure the pins on the connector, the alternate mode driver
needs to notify the bus using [`typec_altmode_notify()`](#c.typec_altmode_notify "typec_altmode_notify"). The driver
passes the negotiated SVID specific pin configuration value to the function as
parameter. The bus driver will then configure the mux behind the connector using
that value as the state value for the mux.

NOTE: The SVID specific pin configuration values must always start from
`TYPEC_STATE_MODAL`. USB Type-C specification defines two default states for
the connector: `TYPEC_STATE_USB` and `TYPEC_STATE_SAFE`. These values are
reserved by the bus as the first possible values for the state. When the
alternate mode is entered, the bus will put the connector into
`TYPEC_STATE_SAFE` before sending Enter or Exit Mode command as defined in USB
Type-C Specification, and also put the connector back to `TYPEC_STATE_USB`
after the mode has been exited.

An example of working definitions for SVID specific pin configurations would
look like this:

```
enum {
    ALTMODEX_CONF_A = TYPEC_STATE_MODAL,
    ALTMODEX_CONF_B,
    ...
};
```

Helper macro `TYPEC_MODAL_STATE()` can also be used:

```
#define ALTMODEX_CONF_A = TYPEC_MODAL_STATE(0);
#define ALTMODEX_CONF_B = TYPEC_MODAL_STATE(1);
```

### Cable plug alternate modes

The alternate mode drivers are not bound to cable plug alternate mode devices,
only to the partner alternate mode devices. If the alternate mode supports, or
requires, a cable that responds to SOP Prime, and optionally SOP Double Prime
messages, the driver for that alternate mode must request handle to the cable
plug alternate modes using [`typec_altmode_get_plug()`](#c.typec_altmode_get_plug "typec_altmode_get_plug"), and take over
their control.

## Driver API

### Alternate mode structs

struct typec\_altmode\_ops
:   Alternate mode specific operations vector

**Definition**:

```
struct typec_altmode_ops {
    int (*enter)(struct typec_altmode *altmode, u32 *vdo);
    int (*exit)(struct typec_altmode *altmode);
    void (*attention)(struct typec_altmode *altmode, u32 vdo);
    int (*vdm)(struct typec_altmode *altmode, const u32 hdr, const u32 *vdo, int cnt);
    int (*notify)(struct typec_altmode *altmode, unsigned long conf, void *data);
    int (*activate)(struct typec_altmode *altmode, int activate);
};
```

**Members**

`enter`
:   Operations to be executed with Enter Mode Command

`exit`
:   Operations to be executed with Exit Mode Command

`attention`
:   Callback for Attention Command

`vdm`
:   Callback for SVID specific commands

`notify`
:   Communication channel for platform and the alternate mode

`activate`
:   User callback for Enter/Exit Mode

struct typec\_altmode\_driver
:   USB Type-C alternate mode device driver

**Definition**:

```
struct typec_altmode_driver {
    const struct typec_device_id *id_table;
    int (*probe)(struct typec_altmode *altmode);
    void (*remove)(struct typec_altmode *altmode);
    struct device_driver driver;
};
```

**Members**

`id_table`
:   Null terminated array of SVIDs

`probe`
:   Callback for device binding

`remove`
:   Callback for device unbinding

`driver`
:   Device driver model driver

**Description**

These drivers will be bind to the partner alternate mode devices. They will
handle all SVID specific communication.

### Alternate mode driver registering/unregistering

typec\_altmode\_register\_driver

`typec_altmode_register_driver (drv)`

> registers a USB Type-C alternate mode device driver

**Parameters**

`drv`
:   pointer to [`struct typec_altmode_driver`](#c.typec_altmode_driver "typec_altmode_driver")

**Description**

These drivers will be bind to the partner alternate mode devices. They will
handle all SVID specific communication.

void typec\_altmode\_unregister\_driver(struct [typec\_altmode\_driver](#c.typec_altmode_driver "typec_altmode_driver") \*drv)
:   unregisters a USB Type-C alternate mode device driver

**Parameters**

`struct typec_altmode_driver *drv`
:   pointer to [`struct typec_altmode_driver`](#c.typec_altmode_driver "typec_altmode_driver")

**Description**

These drivers will be bind to the partner alternate mode devices. They will
handle all SVID specific communication.

### Alternate mode driver operations

int typec\_altmode\_notify(struct typec\_altmode \*adev, unsigned long conf, void \*data)
:   Communication between the OS and alternate mode driver

**Parameters**

`struct typec_altmode *adev`
:   Handle to the alternate mode

`unsigned long conf`
:   Alternate mode specific configuration value

`void *data`
:   Alternate mode specific data

**Description**

The primary purpose for this function is to allow the alternate mode drivers
to tell which pin configuration has been negotiated with the partner. That
information will then be used for example to configure the muxes.
Communication to the other direction is also possible, and low level device
drivers can also send notifications to the alternate mode drivers. The actual
communication will be specific for every SVID.

int typec\_altmode\_enter(struct typec\_altmode \*adev, u32 \*vdo)
:   Enter Mode

**Parameters**

`struct typec_altmode *adev`
:   The alternate mode

`u32 *vdo`
:   VDO for the Enter Mode command

**Description**

The alternate mode drivers use this function to enter mode. The port drivers
use this to inform the alternate mode drivers that the partner has initiated
Enter Mode command. If the alternate mode does not require VDO, **vdo** must be
NULL.

int typec\_altmode\_exit(struct typec\_altmode \*adev)
:   Exit Mode

**Parameters**

`struct typec_altmode *adev`
:   The alternate mode

**Description**

The partner of **adev** has initiated Exit Mode command.

int typec\_altmode\_attention(struct typec\_altmode \*adev, u32 vdo)
:   Attention command

**Parameters**

`struct typec_altmode *adev`
:   The alternate mode

`u32 vdo`
:   VDO for the Attention command

**Description**

Notifies the partner of **adev** about Attention command.

int typec\_altmode\_vdm(struct typec\_altmode \*adev, const u32 header, const u32 \*vdo, int count)
:   Send Vendor Defined Messages (VDM) to the partner

**Parameters**

`struct typec_altmode *adev`
:   Alternate mode handle

`const u32 header`
:   VDM Header

`const u32 *vdo`
:   Array of Vendor Defined Data Objects

`int count`
:   Number of Data Objects

**Description**

The alternate mode drivers use this function for SVID specific communication
with the partner. The port drivers use it to deliver the Structured VDMs
received from the partners to the alternate mode drivers.

### API for the port drivers

struct typec\_altmode \*typec\_match\_altmode(struct typec\_altmode \*\*altmodes, size\_t n, u16 svid, u8 mode)
:   Match SVID and mode to an array of alternate modes

**Parameters**

`struct typec_altmode **altmodes`
:   Array of alternate modes

`size_t n`
:   Number of elements in the array, or -1 for NULL terminated arrays

`u16 svid`
:   Standard or Vendor ID to match with

`u8 mode`
:   Mode to match with

**Description**

Return pointer to an alternate mode with SVID matching **svid**, or NULL when no
match is found.

### Cable Plug operations

struct typec\_altmode \*typec\_altmode\_get\_plug(struct typec\_altmode \*adev, enum typec\_plug\_index index)
:   Find cable plug alternate mode

**Parameters**

`struct typec_altmode *adev`
:   Handle to partner alternate mode

`enum typec_plug_index index`
:   Cable plug index

**Description**

Increment reference count for cable plug alternate mode device. Returns
handle to the cable plug alternate mode, or NULL if none is found.

void typec\_altmode\_put\_plug(struct typec\_altmode \*plug)
:   Decrement cable plug alternate mode reference count

**Parameters**

`struct typec_altmode *plug`
:   Handle to the cable plug alternate mode
