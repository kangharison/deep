# Writing s390 channel device drivers

> 출처(원문): https://docs.kernel.org/driver-api/s390-drivers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Writing s390 channel device drivers

Author:
:   Cornelia Huck

## Introduction

This document describes the interfaces available for device drivers that
drive s390 based channel attached I/O devices. This includes interfaces
for interaction with the hardware and interfaces for interacting with
the common driver core. Those interfaces are provided by the s390 common
I/O layer.

The document assumes a familarity with the technical terms associated
with the s390 channel I/O architecture. For a description of this
architecture, please refer to the “z/Architecture: Principles of
Operation”, IBM publication no. SA22-7832.

While most I/O devices on a s390 system are typically driven through the
channel I/O mechanism described here, there are various other methods
(like the diag interface). These are out of the scope of this document.

The s390 common I/O layer also provides access to some devices that are
not strictly considered I/O devices. They are considered here as well,
although they are not the focus of this document.

Some additional information can also be found in the kernel source under
[S/390 driver model interfaces](../arch/s390/driver-model.html).

## The css bus

The css bus contains the subchannels available on the system. They fall
into several categories:

* Standard I/O subchannels, for use by the system. They have a child
  device on the ccw bus and are described below.
* I/O subchannels bound to the vfio-ccw driver. See
  [vfio-ccw: the basic infrastructure](../arch/s390/vfio-ccw.html).
* Message subchannels. No Linux driver currently exists.
* CHSC subchannels (at most one). The chsc subchannel driver can be used
  to send asynchronous chsc commands.
* eADM subchannels. Used for talking to storage class memory.

## The ccw bus

The ccw bus typically contains the majority of devices available to a
s390 system. Named after the channel command word (ccw), the basic
command structure used to address its devices, the ccw bus contains
so-called channel attached devices. They are addressed via I/O
subchannels, visible on the css bus. A device driver for
channel-attached devices, however, will never interact with the
subchannel directly, but only via the I/O device on the ccw bus, the ccw
device.

### I/O functions for channel-attached devices

Some hardware structures have been translated into C structures for use
by the common I/O layer and device drivers. For more information on the
hardware structures represented here, please consult the Principles of
Operation.

struct ccw1
:   channel command word

**Definition**:

```
struct ccw1 {
    __u8 cmd_code;
    __u8 flags;
    __u16 count;
    dma32_t cda;
};
```

**Members**

`cmd_code`
:   command code

`flags`
:   flags, like IDA addressing, etc.

`count`
:   byte count

`cda`
:   data address

**Description**

The ccw is the basic structure to build channel programs that perform
operations with the device or the control unit. Only Format-1 channel
command words are supported.

struct ccw0
:   channel command word

**Definition**:

```
struct ccw0 {
    __u8 cmd_code;
    __u32 cda : 24;
    __u8 flags;
    __u8 reserved;
    __u16 count;
};
```

**Members**

`cmd_code`
:   command code

`cda`
:   data address

`flags`
:   flags, like IDA addressing, etc.

`reserved`
:   will be ignored

`count`
:   byte count

**Description**

The format-0 ccw structure.

struct erw
:   extended report word

**Definition**:

```
struct erw {
    __u32 res0  : 3;
    __u32 auth  : 1;
    __u32 pvrf  : 1;
    __u32 cpt   : 1;
    __u32 fsavf : 1;
    __u32 cons  : 1;
    __u32 scavf : 1;
    __u32 fsaf  : 1;
    __u32 scnt  : 6;
    __u32 res16 : 16;
};
```

**Members**

`res0`
:   reserved

`auth`
:   authorization check

`pvrf`
:   path-verification-required flag

`cpt`
:   channel-path timeout

`fsavf`
:   failing storage address validity flag

`cons`
:   concurrent sense

`scavf`
:   secondary ccw address validity flag

`fsaf`
:   failing storage address format

`scnt`
:   sense count, if **cons** == `1`

`res16`
:   reserved

struct erw\_eadm
:   EADM Subchannel extended report word

**Definition**:

```
struct erw_eadm {
    __u32 : 16;
    __u32 b : 1;
    __u32 r : 1;
    __u32 : 14;
};
```

**Members**

`b`
:   aob error

`r`
:   arsb error

struct sublog
:   subchannel logout area

**Definition**:

```
struct sublog {
    __u32 res0  : 1;
    __u32 esf   : 7;
    __u32 lpum  : 8;
    __u32 arep  : 1;
    __u32 fvf   : 5;
    __u32 sacc  : 2;
    __u32 termc : 2;
    __u32 devsc : 1;
    __u32 serr  : 1;
    __u32 ioerr : 1;
    __u32 seqc  : 3;
};
```

**Members**

`res0`
:   reserved

`esf`
:   extended status flags

`lpum`
:   last path used mask

`arep`
:   ancillary report

`fvf`
:   field-validity flags

`sacc`
:   storage access code

`termc`
:   termination code

`devsc`
:   device-status check

`serr`
:   secondary error

`ioerr`
:   i/o-error alert

`seqc`
:   sequence code

struct esw0
:   Format 0 Extended Status Word (ESW)

**Definition**:

```
struct esw0 {
    struct sublog sublog;
    struct erw erw;
    dma32_t faddr[2];
    dma32_t saddr;
};
```

**Members**

`sublog`
:   subchannel logout

`erw`
:   extended report word

`faddr`
:   failing storage address

`saddr`
:   secondary ccw address

struct esw1
:   Format 1 Extended Status Word (ESW)

**Definition**:

```
struct esw1 {
    __u8 zero0;
    __u8 lpum;
    __u16 zero16;
    struct erw erw;
    __u32 zeros[3];
};
```

**Members**

`zero0`
:   reserved zeros

`lpum`
:   last path used mask

`zero16`
:   reserved zeros

`erw`
:   extended report word

`zeros`
:   three fullwords of zeros

struct esw2
:   Format 2 Extended Status Word (ESW)

**Definition**:

```
struct esw2 {
    __u8 zero0;
    __u8 lpum;
    __u16 dcti;
    struct erw erw;
    __u32 zeros[3];
};
```

**Members**

`zero0`
:   reserved zeros

`lpum`
:   last path used mask

`dcti`
:   device-connect-time interval

`erw`
:   extended report word

`zeros`
:   three fullwords of zeros

struct esw3
:   Format 3 Extended Status Word (ESW)

**Definition**:

```
struct esw3 {
    __u8 zero0;
    __u8 lpum;
    __u16 res;
    struct erw erw;
    __u32 zeros[3];
};
```

**Members**

`zero0`
:   reserved zeros

`lpum`
:   last path used mask

`res`
:   reserved

`erw`
:   extended report word

`zeros`
:   three fullwords of zeros

struct esw\_eadm
:   EADM Subchannel Extended Status Word (ESW)

**Definition**:

```
struct esw_eadm {
    __u32 sublog;
    struct erw_eadm erw;
    __u32 : 32;
    __u32 : 32;
    __u32 : 32;
};
```

**Members**

`sublog`
:   subchannel logout

`erw`
:   extended report word

struct irb
:   interruption response block

**Definition**:

```
struct irb {
    union scsw scsw;
    union {
        struct esw0 esw0;
        struct esw1 esw1;
        struct esw2 esw2;
        struct esw3 esw3;
        struct esw_eadm eadm;
    } esw;
    __u8 ecw[32];
};
```

**Members**

`scsw`
:   subchannel status word

`esw`
:   extended status word

`ecw`
:   extended control word

**Description**

The irb that is handed to the device driver when an interrupt occurs. For
solicited interrupts, the common I/O layer already performs checks whether
a field is valid; a field not being valid is always passed as `0`.
If a unit check occurred, **ecw** may contain sense data; this is retrieved
by the common I/O layer itself if the device doesn’t support concurrent
sense (so that the device driver never needs to perform basic sense itself).
For unsolicited interrupts, the irb is passed as-is (expect for sense data,
if applicable).

struct ciw
:   command information word (CIW) layout

**Definition**:

```
struct ciw {
    __u32 et       :  2;
    __u32 reserved :  2;
    __u32 ct       :  4;
    __u32 cmd      :  8;
    __u32 count    : 16;
};
```

**Members**

`et`
:   entry type

`reserved`
:   reserved bits

`ct`
:   command type

`cmd`
:   command code

`count`
:   command count

struct ccw\_dev\_id
:   unique identifier for ccw devices

**Definition**:

```
struct ccw_dev_id {
    u8 ssid;
    u16 devno;
};
```

**Members**

`ssid`
:   subchannel set id

`devno`
:   device number

**Description**

This structure is not directly based on any hardware structure. The
hardware identifies a device by its device number and its subchannel,
which is in turn identified by its id. In order to get a unique identifier
for ccw devices across subchannel sets, **struct** ccw\_dev\_id has been
introduced.

int ccw\_dev\_id\_is\_equal(struct [ccw\_dev\_id](#c.ccw_dev_id "ccw_dev_id") \*dev\_id1, struct [ccw\_dev\_id](#c.ccw_dev_id "ccw_dev_id") \*dev\_id2)
:   compare two ccw\_dev\_ids

**Parameters**

`struct ccw_dev_id *dev_id1`
:   a ccw\_dev\_id

`struct ccw_dev_id *dev_id2`
:   another ccw\_dev\_id

**Return**

`1` if the two structures are equal field-by-field,
`0` if not.

**Context**

any

u8 pathmask\_to\_pos(u8 mask)
:   find the position of the left-most bit in a pathmask

**Parameters**

`u8 mask`
:   pathmask with at least one bit set

### ccw devices

Devices that want to initiate channel I/O need to attach to the ccw bus.
Interaction with the driver core is done via the common I/O layer, which
provides the abstractions of ccw devices and ccw device drivers.

The functions that initiate or terminate channel I/O all act upon a ccw
device structure. Device drivers must not bypass those functions or
strange side effects may happen.

struct ccw\_device
:   channel attached device

**Definition**:

```
struct ccw_device {
    spinlock_t *ccwlock;
    struct ccw_device_id id;
    struct ccw_driver *drv;
    struct device dev;
    int online;
    void (*handler) (struct ccw_device *, unsigned long, struct irb *);
};
```

**Members**

`ccwlock`
:   pointer to device lock

`id`
:   id of this device

`drv`
:   ccw driver for this device

`dev`
:   embedded device structure

`online`
:   online status of device

`handler`
:   interrupt handler

**Description**

**handler** is a member of the device rather than the driver since a driver
can have different interrupt handlers for different ccw devices
(multi-subchannel drivers).

struct ccw\_driver
:   device driver for channel attached devices

**Definition**:

```
struct ccw_driver {
    struct ccw_device_id *ids;
    int (*probe) (struct ccw_device *);
    void (*remove) (struct ccw_device *);
    int (*set_online) (struct ccw_device *);
    int (*set_offline) (struct ccw_device *);
    int (*notify) (struct ccw_device *, int);
    void (*path_event) (struct ccw_device *, int *);
    void (*shutdown) (struct ccw_device *);
    enum uc_todo (*uc_handler) (struct ccw_device *, struct irb *);
    struct device_driver driver;
    enum interruption_class int_class;
};
```

**Members**

`ids`
:   ids supported by this driver

`probe`
:   function called on probe

`remove`
:   function called on remove

`set_online`
:   called when setting device online

`set_offline`
:   called when setting device offline

`notify`
:   notify driver of device state changes

`path_event`
:   notify driver of channel path events

`shutdown`
:   called at device shutdown

`uc_handler`
:   callback for unit check handler

`driver`
:   embedded device driver structure

`int_class`
:   interruption class to use for accounting interrupts

int ccw\_device\_set\_offline(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   disable a ccw device for I/O

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

**Description**

This function calls the driver’s `set_offline()` function for **cdev**, if
given, and then disables **cdev**.

**Return**

`0` on success and a negative error value on failure.

**Context**

enabled, ccw device lock not held

int ccw\_device\_set\_online(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   enable a ccw device for I/O

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

**Description**

This function first enables **cdev** and then calls the driver’s `set_online()`
function for **cdev**, if given. If `set_online()` returns an error, **cdev** is
disabled again.

**Return**

`0` on success and a negative error value on failure.

**Context**

enabled, ccw device lock not held

struct [ccw\_device](#c.ccw_device "ccw_device") \*get\_ccwdev\_by\_dev\_id(struct [ccw\_dev\_id](#c.ccw_dev_id "ccw_dev_id") \*dev\_id)
:   obtain device from a ccw device id

**Parameters**

`struct ccw_dev_id *dev_id`
:   id of the device to be searched

**Description**

This function searches all devices attached to the ccw bus for a device
matching **dev\_id**.

**Return**

If a device is found its reference count is increased and returned;
else `NULL` is returned.

struct [ccw\_device](#c.ccw_device "ccw_device") \*get\_ccwdev\_by\_busid(struct [ccw\_driver](#c.ccw_driver "ccw_driver") \*cdrv, const char \*bus\_id)
:   obtain device from a bus id

**Parameters**

`struct ccw_driver *cdrv`
:   driver the device is owned by

`const char *bus_id`
:   bus id of the device to be searched

**Description**

This function searches all devices owned by **cdrv** for a device with a bus
id matching **bus\_id**.

**Return**

If a match is found, its reference count of the found device is increased
and it is returned; else `NULL` is returned.

int ccw\_driver\_register(struct [ccw\_driver](#c.ccw_driver "ccw_driver") \*cdriver)
:   register a ccw driver

**Parameters**

`struct ccw_driver *cdriver`
:   driver to be registered

**Description**

This function is mainly a wrapper around [`driver_register()`](infrastructure.html#c.driver_register "driver_register").

**Return**

`0` on success and a negative error value on failure.

void ccw\_driver\_unregister(struct [ccw\_driver](#c.ccw_driver "ccw_driver") \*cdriver)
:   deregister a ccw driver

**Parameters**

`struct ccw_driver *cdriver`
:   driver to be deregistered

**Description**

This function is mainly a wrapper around [`driver_unregister()`](infrastructure.html#c.driver_unregister "driver_unregister").

int ccw\_device\_siosl(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   initiate logging

**Parameters**

`struct ccw_device *cdev`
:   ccw device

**Description**

This function is used to invoke model-dependent logging within the channel
subsystem.

int ccw\_device\_set\_options\_mask(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, unsigned long flags)
:   set some options and unset the rest

**Parameters**

`struct ccw_device *cdev`
:   device for which the options are to be set

`unsigned long flags`
:   options to be set

**Description**

All flags specified in **flags** are set, all flags not specified in **flags**
are cleared.

**Return**

`0` on success, -`EINVAL` on an invalid flag combination.

int ccw\_device\_set\_options(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, unsigned long flags)
:   set some options

**Parameters**

`struct ccw_device *cdev`
:   device for which the options are to be set

`unsigned long flags`
:   options to be set

**Description**

All flags specified in **flags** are set, the remainder is left untouched.

**Return**

`0` on success, -`EINVAL` if an invalid flag combination would ensue.

void ccw\_device\_clear\_options(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, unsigned long flags)
:   clear some options

**Parameters**

`struct ccw_device *cdev`
:   device for which the options are to be cleared

`unsigned long flags`
:   options to be cleared

**Description**

All flags specified in **flags** are cleared, the remainder is left untouched.

int ccw\_device\_is\_pathgroup(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   determine if paths to this device are grouped

**Parameters**

`struct ccw_device *cdev`
:   ccw device

**Description**

Return non-zero if there is a path group, zero otherwise.

int ccw\_device\_is\_multipath(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   determine if device is operating in multipath mode

**Parameters**

`struct ccw_device *cdev`
:   ccw device

**Description**

Return non-zero if device is operating in multipath mode, zero otherwise.

int ccw\_device\_clear(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, unsigned long intparm)
:   terminate I/O request processing

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

`unsigned long intparm`
:   interruption parameter to be returned upon conclusion of csch

**Description**

[`ccw_device_clear()`](#c.ccw_device_clear "ccw_device_clear") calls csch on **cdev**’s subchannel.

**Return**

`0` on success,
-`ENODEV` on device not operational,
-`EINVAL` on invalid device state.

**Context**

Interrupts disabled, ccw device lock held

int ccw\_device\_start\_timeout\_key(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [ccw1](#c.ccw1 "ccw1") \*cpa, unsigned long intparm, \_\_u8 lpm, \_\_u8 key, unsigned long flags, int expires)
:   start a s390 channel program with timeout and key

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

`struct ccw1 *cpa`
:   logical start address of channel program

`unsigned long intparm`
:   user specific interruption parameter; will be presented back to
    **cdev**’s interrupt handler. Allows a device driver to associate
    the interrupt with a particular I/O request.

`__u8 lpm`
:   defines the channel path to be used for a specific I/O request. A
    value of 0 will make cio use the opm.

`__u8 key`
:   storage key to be used for the I/O

`unsigned long flags`
:   additional flags; defines the action to be performed for I/O
    processing.

`int expires`
:   timeout value in jiffies

**Description**

Start a S/390 channel program. When the interrupt arrives, the
IRQ handler is called, either immediately, delayed (dev-end missing,
or sense required) or never (no IRQ handler registered).
This function notifies the device driver if the channel program has not
completed during the time specified by **expires**. If a timeout occurs, the
channel program is terminated via xsch, hsch or csch, and the device’s
interrupt handler will be called with an irb containing ERR\_PTR(-`ETIMEDOUT`).
The interruption handler will echo back the **intparm** specified here, unless
another interruption parameter is specified by a subsequent invocation of
[`ccw_device_halt()`](#c.ccw_device_halt "ccw_device_halt") or [`ccw_device_clear()`](#c.ccw_device_clear "ccw_device_clear").

**Return**

`0`, if the operation was successful;
-`EBUSY`, if the device is busy, or status pending;
-`EACCES`, if no path specified in **lpm** is operational;
-`ENODEV`, if the device is not operational.

**Context**

Interrupts disabled, ccw device lock held

int ccw\_device\_start\_key(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [ccw1](#c.ccw1 "ccw1") \*cpa, unsigned long intparm, \_\_u8 lpm, \_\_u8 key, unsigned long flags)
:   start a s390 channel program with key

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

`struct ccw1 *cpa`
:   logical start address of channel program

`unsigned long intparm`
:   user specific interruption parameter; will be presented back to
    **cdev**’s interrupt handler. Allows a device driver to associate
    the interrupt with a particular I/O request.

`__u8 lpm`
:   defines the channel path to be used for a specific I/O request. A
    value of 0 will make cio use the opm.

`__u8 key`
:   storage key to be used for the I/O

`unsigned long flags`
:   additional flags; defines the action to be performed for I/O
    processing.

**Description**

Start a S/390 channel program. When the interrupt arrives, the
IRQ handler is called, either immediately, delayed (dev-end missing,
or sense required) or never (no IRQ handler registered).
The interruption handler will echo back the **intparm** specified here, unless
another interruption parameter is specified by a subsequent invocation of
[`ccw_device_halt()`](#c.ccw_device_halt "ccw_device_halt") or [`ccw_device_clear()`](#c.ccw_device_clear "ccw_device_clear").

**Return**

`0`, if the operation was successful;
-`EBUSY`, if the device is busy, or status pending;
-`EACCES`, if no path specified in **lpm** is operational;
-`ENODEV`, if the device is not operational.

**Context**

Interrupts disabled, ccw device lock held

int ccw\_device\_start(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [ccw1](#c.ccw1 "ccw1") \*cpa, unsigned long intparm, \_\_u8 lpm, unsigned long flags)
:   start a s390 channel program

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

`struct ccw1 *cpa`
:   logical start address of channel program

`unsigned long intparm`
:   user specific interruption parameter; will be presented back to
    **cdev**’s interrupt handler. Allows a device driver to associate
    the interrupt with a particular I/O request.

`__u8 lpm`
:   defines the channel path to be used for a specific I/O request. A
    value of 0 will make cio use the opm.

`unsigned long flags`
:   additional flags; defines the action to be performed for I/O
    processing.

**Description**

Start a S/390 channel program. When the interrupt arrives, the
IRQ handler is called, either immediately, delayed (dev-end missing,
or sense required) or never (no IRQ handler registered).
The interruption handler will echo back the **intparm** specified here, unless
another interruption parameter is specified by a subsequent invocation of
[`ccw_device_halt()`](#c.ccw_device_halt "ccw_device_halt") or [`ccw_device_clear()`](#c.ccw_device_clear "ccw_device_clear").

**Return**

`0`, if the operation was successful;
-`EBUSY`, if the device is busy, or status pending;
-`EACCES`, if no path specified in **lpm** is operational;
-`ENODEV`, if the device is not operational.

**Context**

Interrupts disabled, ccw device lock held

int ccw\_device\_start\_timeout(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [ccw1](#c.ccw1 "ccw1") \*cpa, unsigned long intparm, \_\_u8 lpm, unsigned long flags, int expires)
:   start a s390 channel program with timeout

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

`struct ccw1 *cpa`
:   logical start address of channel program

`unsigned long intparm`
:   user specific interruption parameter; will be presented back to
    **cdev**’s interrupt handler. Allows a device driver to associate
    the interrupt with a particular I/O request.

`__u8 lpm`
:   defines the channel path to be used for a specific I/O request. A
    value of 0 will make cio use the opm.

`unsigned long flags`
:   additional flags; defines the action to be performed for I/O
    processing.

`int expires`
:   timeout value in jiffies

**Description**

Start a S/390 channel program. When the interrupt arrives, the
IRQ handler is called, either immediately, delayed (dev-end missing,
or sense required) or never (no IRQ handler registered).
This function notifies the device driver if the channel program has not
completed during the time specified by **expires**. If a timeout occurs, the
channel program is terminated via xsch, hsch or csch, and the device’s
interrupt handler will be called with an irb containing ERR\_PTR(-`ETIMEDOUT`).
The interruption handler will echo back the **intparm** specified here, unless
another interruption parameter is specified by a subsequent invocation of
[`ccw_device_halt()`](#c.ccw_device_halt "ccw_device_halt") or [`ccw_device_clear()`](#c.ccw_device_clear "ccw_device_clear").

**Return**

`0`, if the operation was successful;
-`EBUSY`, if the device is busy, or status pending;
-`EACCES`, if no path specified in **lpm** is operational;
-`ENODEV`, if the device is not operational.

**Context**

Interrupts disabled, ccw device lock held

int ccw\_device\_halt(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, unsigned long intparm)
:   halt I/O request processing

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

`unsigned long intparm`
:   interruption parameter to be returned upon conclusion of hsch

**Description**

[`ccw_device_halt()`](#c.ccw_device_halt "ccw_device_halt") calls hsch on **cdev**’s subchannel.
The interruption handler will echo back the **intparm** specified here, unless
another interruption parameter is specified by a subsequent invocation of
[`ccw_device_clear()`](#c.ccw_device_clear "ccw_device_clear").

**Return**

`0` on success,
-`ENODEV` on device not operational,
-`EINVAL` on invalid device state,
-`EBUSY` on device busy or interrupt pending.

**Context**

Interrupts disabled, ccw device lock held

int ccw\_device\_resume(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   resume channel program execution

**Parameters**

`struct ccw_device *cdev`
:   target ccw device

**Description**

[`ccw_device_resume()`](#c.ccw_device_resume "ccw_device_resume") calls rsch on **cdev**’s subchannel.

**Return**

`0` on success,
-`ENODEV` on device not operational,
-`EINVAL` on invalid device state,
-`EBUSY` on device busy or interrupt pending.

**Context**

Interrupts disabled, ccw device lock held

struct [ciw](#c.ciw "ciw") \*ccw\_device\_get\_ciw(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, \_\_u32 ct)
:   Search for CIW command in extended sense data.

**Parameters**

`struct ccw_device *cdev`
:   ccw device to inspect

`__u32 ct`
:   command type to look for

**Description**

During SenseID, command information words (CIWs) describing special
commands available to the device may have been stored in the extended
sense data. This function searches for CIWs of a specified command
type in the extended sense data.

**Return**

`NULL` if no extended sense data has been stored or if no CIW of the
specified command type could be found,
else a pointer to the CIW of the specified command type.

\_\_u8 ccw\_device\_get\_path\_mask(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   get currently available paths

**Parameters**

`struct ccw_device *cdev`
:   ccw device to be queried

**Return**

`0` if no subchannel for the device is available,
else the mask of currently available paths for the ccw device’s subchannel.

struct channel\_path\_desc\_fmt0 \*ccw\_device\_get\_chp\_desc(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, int chp\_idx)
:   return newly allocated channel-path descriptor

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the descriptor for

`int chp_idx`
:   index of the channel path

**Description**

On success return a newly allocated copy of the channel-path description
data associated with the given channel path. Return `NULL` on error.

u8 \*ccw\_device\_get\_util\_str(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, int chp\_idx)
:   return newly allocated utility strings

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the utility strings for

`int chp_idx`
:   index of the channel path

**Description**

On success return a newly allocated copy of the utility strings
associated with the given channel path. Return `NULL` on error.

void ccw\_device\_get\_id(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [ccw\_dev\_id](#c.ccw_dev_id "ccw_dev_id") \*dev\_id)
:   obtain a ccw device id

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the id for

`struct ccw_dev_id *dev_id`
:   where to fill in the values

int ccw\_device\_tm\_start\_timeout\_key(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [tcw](#c.ccw_device_tm_start_timeout_key "tcw") \*tcw, unsigned long intparm, u8 lpm, u8 key, int expires)
:   perform start function

**Parameters**

`struct ccw_device *cdev`
:   ccw device on which to perform the start function

`struct tcw *tcw`
:   transport-command word to be started

`unsigned long intparm`
:   user defined parameter to be passed to the interrupt handler

`u8 lpm`
:   mask of paths to use

`u8 key`
:   storage key to use for storage access

`int expires`
:   time span in jiffies after which to abort request

**Description**

Start the tcw on the given ccw device. Return zero on success, non-zero
otherwise.

int ccw\_device\_tm\_start\_key(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [tcw](#c.ccw_device_tm_start_key "tcw") \*tcw, unsigned long intparm, u8 lpm, u8 key)
:   perform start function

**Parameters**

`struct ccw_device *cdev`
:   ccw device on which to perform the start function

`struct tcw *tcw`
:   transport-command word to be started

`unsigned long intparm`
:   user defined parameter to be passed to the interrupt handler

`u8 lpm`
:   mask of paths to use

`u8 key`
:   storage key to use for storage access

**Description**

Start the tcw on the given ccw device. Return zero on success, non-zero
otherwise.

int ccw\_device\_tm\_start(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [tcw](#c.ccw_device_tm_start "tcw") \*tcw, unsigned long intparm, u8 lpm)
:   perform start function

**Parameters**

`struct ccw_device *cdev`
:   ccw device on which to perform the start function

`struct tcw *tcw`
:   transport-command word to be started

`unsigned long intparm`
:   user defined parameter to be passed to the interrupt handler

`u8 lpm`
:   mask of paths to use

**Description**

Start the tcw on the given ccw device. Return zero on success, non-zero
otherwise.

int ccw\_device\_tm\_start\_timeout(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [tcw](#c.ccw_device_tm_start_timeout "tcw") \*tcw, unsigned long intparm, u8 lpm, int expires)
:   perform start function

**Parameters**

`struct ccw_device *cdev`
:   ccw device on which to perform the start function

`struct tcw *tcw`
:   transport-command word to be started

`unsigned long intparm`
:   user defined parameter to be passed to the interrupt handler

`u8 lpm`
:   mask of paths to use

`int expires`
:   time span in jiffies after which to abort request

**Description**

Start the tcw on the given ccw device. Return zero on success, non-zero
otherwise.

int ccw\_device\_get\_mdc(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, u8 mask)
:   accumulate max data count

**Parameters**

`struct ccw_device *cdev`
:   ccw device for which the max data count is accumulated

`u8 mask`
:   mask of paths to use

**Description**

Return the number of 64K-bytes blocks all paths at least support
for a transport command. Return value 0 indicates failure.

int ccw\_device\_tm\_intrg(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   perform interrogate function

**Parameters**

`struct ccw_device *cdev`
:   ccw device on which to perform the interrogate function

**Description**

Perform an interrogate function on the given ccw device. Return zero on
success, non-zero otherwise.

void ccw\_device\_get\_schid(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct subchannel\_id \*schid)
:   obtain a subchannel id

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the id for

`struct subchannel_id *schid`
:   where to fill in the values

int ccw\_device\_pnso(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct chsc\_pnso\_area \*pnso\_area, u8 oc, struct chsc\_pnso\_resume\_token resume\_token, int cnc)
:   Perform Network-Subchannel Operation

**Parameters**

`struct ccw_device *cdev`
:   device on which PNSO is performed

`struct chsc_pnso_area *pnso_area`
:   request and response block for the operation

`u8 oc`
:   Operation Code

`struct chsc_pnso_resume_token resume_token`
:   resume token for multiblock response

`int cnc`
:   Boolean change-notification control

**Description**

pnso\_area must be allocated by the caller with get\_zeroed\_page(GFP\_KERNEL)

Returns 0 on success.

int ccw\_device\_get\_cssid(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, u8 \*cssid)
:   obtain Channel Subsystem ID

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the CSSID for

`u8 *cssid`
:   The resulting Channel Subsystem ID

int ccw\_device\_get\_iid(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, u8 \*iid)
:   obtain MIF-image ID

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the MIF-image ID for

`u8 *iid`
:   The resulting MIF-image ID

int ccw\_device\_get\_chpid(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, int chp\_idx, u8 \*chpid)
:   obtain Channel Path ID

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the Channel Path ID for

`int chp_idx`
:   Index of the channel path

`u8 *chpid`
:   The resulting Channel Path ID

int ccw\_device\_get\_chid(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, int chp\_idx, u16 \*chid)
:   obtain Channel ID associated with specified CHPID

**Parameters**

`struct ccw_device *cdev`
:   device to obtain the Channel ID for

`int chp_idx`
:   Index of the channel path

`u16 *chid`
:   The resulting Channel ID

### The channel-measurement facility

The channel-measurement facility provides a means to collect measurement
data which is made available by the channel subsystem for each channel
attached device.

struct cmbdata
:   channel measurement block data for user space

**Definition**:

```
struct cmbdata {
    __u64 size;
    __u64 elapsed_time;
    __u64 ssch_rsch_count;
    __u64 sample_count;
    __u64 device_connect_time;
    __u64 function_pending_time;
    __u64 device_disconnect_time;
    __u64 control_unit_queuing_time;
    __u64 device_active_only_time;
    __u64 device_busy_time;
    __u64 initial_command_response_time;
};
```

**Members**

`size`
:   size of the stored data

`elapsed_time`
:   time since last sampling

`ssch_rsch_count`
:   number of ssch and rsch

`sample_count`
:   number of samples

`device_connect_time`
:   time of device connect

`function_pending_time`
:   time of function pending

`device_disconnect_time`
:   time of device disconnect

`control_unit_queuing_time`
:   time of control unit queuing

`device_active_only_time`
:   time of device active only

`device_busy_time`
:   time of device busy (ext. format)

`initial_command_response_time`
:   initial command response time (ext. format)

**Description**

All values are stored as 64 bit for simplicity, especially
in 32 bit emulation mode. All time values are normalized to
nanoseconds.
Currently, two formats are known, which differ by the size of
this structure, i.e. the last two members are only set when
the extended channel measurement facility (first shipped in
z990 machines) is activated.
Potentially, more fields could be added, which would result in a
new ioctl number.

int enable\_cmf(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   switch on the channel measurement for a specific device

**Parameters**

`struct ccw_device *cdev`
:   The ccw device to be enabled

**Description**

> Enable channel measurements for **cdev**. If this is called on a device
> for which channel measurement is already enabled a reset of the
> measurement data is triggered.

**Return**

`0` for success or a negative error value.

**Context**

non-atomic

int disable\_cmf(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   switch off the channel measurement for a specific device

**Parameters**

`struct ccw_device *cdev`
:   The ccw device to be disabled

**Return**

`0` for success or a negative error value.

**Context**

non-atomic

u64 cmf\_read(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, int index)
:   read one value from the current channel measurement block

**Parameters**

`struct ccw_device *cdev`
:   the channel to be read

`int index`
:   the index of the value to be read

**Return**

The value read or `0` if the value cannot be read.

**Context**

any

int cmf\_readall(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev, struct [cmbdata](#c.cmbdata "cmbdata") \*data)
:   read the current channel measurement block

**Parameters**

`struct ccw_device *cdev`
:   the channel to be read

`struct cmbdata *data`
:   a pointer to a data block that will be filled

**Return**

`0` on success, a negative error value otherwise.

**Context**

any

## The ccwgroup bus

The ccwgroup bus only contains artificial devices, created by the user.
Many networking devices (e.g. qeth) are in fact composed of several ccw
devices (like read, write and data channel for qeth). The ccwgroup bus
provides a mechanism to create a meta-device which contains those ccw
devices as slave devices and can be associated with the netdevice.

### ccw group devices

struct ccwgroup\_device
:   ccw group device

**Definition**:

```
struct ccwgroup_device {
    enum {
        CCWGROUP_OFFLINE,
        CCWGROUP_ONLINE,
    } state;
    unsigned int count;
    struct device   dev;
    struct work_struct ungroup_work;
    struct ccw_device *cdev[];
};
```

**Members**

`state`
:   online/offline state

`count`
:   number of attached slave devices

`dev`
:   embedded device structure

`ungroup_work`
:   used to ungroup the ccwgroup device

`cdev`
:   variable number of slave devices, allocated as needed

struct ccwgroup\_driver
:   driver for ccw group devices

**Definition**:

```
struct ccwgroup_driver {
    int (*setup) (struct ccwgroup_device *);
    void (*remove) (struct ccwgroup_device *);
    int (*set_online) (struct ccwgroup_device *);
    int (*set_offline) (struct ccwgroup_device *);
    void (*shutdown)(struct ccwgroup_device *);
    struct device_driver driver;
    struct ccw_driver *ccw_driver;
};
```

**Members**

`setup`
:   function called during device creation to setup the device

`remove`
:   function called on remove

`set_online`
:   function called when device is set online

`set_offline`
:   function called when device is set offline

`shutdown`
:   function called when device is shut down

`driver`
:   embedded driver structure

`ccw_driver`
:   supported ccw\_driver (optional)

int ccwgroup\_set\_online(struct [ccwgroup\_device](#c.ccwgroup_device "ccwgroup_device") \*gdev)
:   enable a ccwgroup device

**Parameters**

`struct ccwgroup_device *gdev`
:   target ccwgroup device

**Description**

This function attempts to put the ccwgroup device into the online state.

**Return**

`0` on success and a negative error value on failure.

int ccwgroup\_set\_offline(struct [ccwgroup\_device](#c.ccwgroup_device "ccwgroup_device") \*gdev, bool call\_gdrv)
:   disable a ccwgroup device

**Parameters**

`struct ccwgroup_device *gdev`
:   target ccwgroup device

`bool call_gdrv`
:   Call the registered gdrv set\_offline function

**Description**

This function attempts to put the ccwgroup device into the offline state.

**Return**

`0` on success and a negative error value on failure.

int ccwgroup\_create\_dev(struct [device](infrastructure.html#c.device "device") \*parent, struct [ccwgroup\_driver](#c.ccwgroup_driver "ccwgroup_driver") \*gdrv, int num\_devices, const char \*buf)
:   create and register a ccw group device

**Parameters**

`struct device *parent`
:   parent device for the new device

`struct ccwgroup_driver *gdrv`
:   driver for the new group device

`int num_devices`
:   number of slave devices

`const char *buf`
:   buffer containing comma separated bus ids of slave devices

**Description**

Create and register a new ccw group device as a child of **parent**. Slave
devices are obtained from the list of bus ids given in **buf**.

**Return**

`0` on success and an error code on failure.

**Context**

non-atomic

int ccwgroup\_driver\_register(struct [ccwgroup\_driver](#c.ccwgroup_driver "ccwgroup_driver") \*cdriver)
:   register a ccw group driver

**Parameters**

`struct ccwgroup_driver *cdriver`
:   driver to be registered

**Description**

This function is mainly a wrapper around [`driver_register()`](infrastructure.html#c.driver_register "driver_register").

void ccwgroup\_driver\_unregister(struct [ccwgroup\_driver](#c.ccwgroup_driver "ccwgroup_driver") \*cdriver)
:   deregister a ccw group driver

**Parameters**

`struct ccwgroup_driver *cdriver`
:   driver to be deregistered

**Description**

This function is mainly a wrapper around [`driver_unregister()`](infrastructure.html#c.driver_unregister "driver_unregister").

int ccwgroup\_probe\_ccwdev(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   probe function for slave devices

**Parameters**

`struct ccw_device *cdev`
:   ccw device to be probed

**Description**

This is a dummy probe function for ccw devices that are slave devices in
a ccw group device.

**Return**

always `0`

void ccwgroup\_remove\_ccwdev(struct [ccw\_device](#c.ccw_device "ccw_device") \*cdev)
:   remove function for slave devices

**Parameters**

`struct ccw_device *cdev`
:   ccw device to be removed

**Description**

This is a remove function for ccw devices that are slave devices in a ccw
group device. It sets the ccw device offline and also deregisters the
embedding ccw group device.

## Generic interfaces

The following section contains interfaces in use not only by drivers
dealing with ccw devices, but drivers for various other s390 hardware
as well.

### Adapter interrupts

The common I/O layer provides helper functions for dealing with adapter
interrupts and interrupt vectors.

int register\_adapter\_interrupt(struct airq\_struct \*airq)
:   register adapter interrupt handler

**Parameters**

`struct airq_struct *airq`
:   pointer to adapter interrupt descriptor

**Description**

Returns 0 on success, or -EINVAL.

void unregister\_adapter\_interrupt(struct airq\_struct \*airq)
:   unregister adapter interrupt handler

**Parameters**

`struct airq_struct *airq`
:   pointer to adapter interrupt descriptor

struct airq\_iv \*airq\_iv\_create(unsigned long bits, unsigned long flags, unsigned long \*vec)
:   create an interrupt vector

**Parameters**

`unsigned long bits`
:   number of bits in the interrupt vector

`unsigned long flags`
:   allocation flags

`unsigned long *vec`
:   pointer to pinned guest memory if AIRQ\_IV\_GUESTVEC

**Description**

Returns a pointer to an interrupt vector structure

void airq\_iv\_release(struct airq\_iv \*iv)
:   release an interrupt vector

**Parameters**

`struct airq_iv *iv`
:   pointer to interrupt vector structure

unsigned long airq\_iv\_alloc(struct airq\_iv \*iv, unsigned long num)
:   allocate irq bits from an interrupt vector

**Parameters**

`struct airq_iv *iv`
:   pointer to an interrupt vector structure

`unsigned long num`
:   number of consecutive irq bits to allocate

**Description**

Returns the bit number of the first irq in the allocated block of irqs,
or -1UL if no bit is available or the AIRQ\_IV\_ALLOC flag has not been
specified

void airq\_iv\_free(struct airq\_iv \*iv, unsigned long bit, unsigned long num)
:   free irq bits of an interrupt vector

**Parameters**

`struct airq_iv *iv`
:   pointer to interrupt vector structure

`unsigned long bit`
:   number of the first irq bit to free

`unsigned long num`
:   number of consecutive irq bits to free

unsigned long airq\_iv\_scan(struct airq\_iv \*iv, unsigned long start, unsigned long end)
:   scan interrupt vector for non-zero bits

**Parameters**

`struct airq_iv *iv`
:   pointer to interrupt vector structure

`unsigned long start`
:   bit number to start the search

`unsigned long end`
:   bit number to end the search

**Description**

Returns the bit number of the next non-zero interrupt bit, or
-1UL if the scan completed without finding any more any non-zero bits.
