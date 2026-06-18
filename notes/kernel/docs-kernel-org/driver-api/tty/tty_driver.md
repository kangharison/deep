# TTY Driver and TTY Operations

> 출처(원문): https://docs.kernel.org/driver-api/tty/tty_driver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TTY Driver and TTY Operations

## [Allocation](#id1)

The first thing a driver needs to do is to allocate a [`struct tty_driver`](#c.tty_driver "tty_driver"). This
is done by [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver") (or [`__tty_alloc_driver()`](#c.__tty_alloc_driver "__tty_alloc_driver")). Next, the newly
allocated structure is filled with information. See [TTY Driver Reference](#tty-driver-reference) at
the end of this document on what actually shall be filled in.

The allocation routines expect a number of devices the driver can handle at
most and flags. Flags are those starting `TTY_DRIVER_` listed and described
in [TTY Driver Flags](#tty-driver-flags) below.

When the driver is about to be freed, [`tty_driver_kref_put()`](#c.tty_driver_kref_put "tty_driver_kref_put") is called on that.
It will decrements the reference count and if it reaches zero, the driver is
freed.

For reference, both allocation and deallocation functions are explained here in
detail:

tty\_alloc\_driver

`tty_alloc_driver (lines, flags)`

> allocate tty driver

**Parameters**

`lines`
:   count of lines this driver can handle at most

`flags`
:   some of [`enum tty_driver_flag`](#c.tty_driver_flag "tty_driver_flag"), will be set in driver->flags

**Return**

[`struct tty_driver`](#c.tty_driver "tty_driver") or a PTR-encoded error (use [`IS_ERR()`](../../core-api/kernel-api.html#c.IS_ERR "IS_ERR") and friends).

struct [tty\_driver](#c.tty_driver "tty_driver") \*\_\_tty\_alloc\_driver(unsigned int lines, struct module \*owner, unsigned long flags)
:   allocate tty driver

**Parameters**

`unsigned int lines`
:   count of lines this driver can handle at most

`struct module *owner`
:   module which is responsible for this driver

`unsigned long flags`
:   some of [`enum tty_driver_flag`](#c.tty_driver_flag "tty_driver_flag"), will be set in driver->flags

**Description**

This should not be called directly, [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver") should be used
instead.

**Return**

[`struct tty_driver`](#c.tty_driver "tty_driver") or a PTR-encoded error (use [`IS_ERR()`](../../core-api/kernel-api.html#c.IS_ERR "IS_ERR") and friends).

void tty\_driver\_kref\_put(struct [tty\_driver](#c.tty_driver "tty_driver") \*driver)
:   drop a reference to a tty driver

**Parameters**

`struct tty_driver *driver`
:   driver of which to drop the reference

**Description**

The final put will destroy and free up the driver.

### [TTY Driver Flags](#id2)

Here comes the documentation of flags accepted by [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver") (or
[`__tty_alloc_driver()`](#c.__tty_alloc_driver "__tty_alloc_driver")):

enum tty\_driver\_flag
:   * TTY Driver Flags

**Constants**

`TTY_DRIVER_INSTALLED`
:   Whether this driver was succesfully installed. This is a tty internal
    flag. Do not touch.

`TTY_DRIVER_RESET_TERMIOS`
:   Requests the tty layer to reset the termios setting when the last
    process has closed the device. Used for PTYs, in particular.

`TTY_DRIVER_REAL_RAW`
:   Indicates that the driver will guarantee not to set any special
    character handling flags if this is set for the tty:

`TTY_DRIVER_DYNAMIC_DEV`
:   The individual tty devices need to be registered with a call to
    [`tty_register_device()`](#c.tty_register_device "tty_register_device") when the device is found in the system and
    unregistered with a call to [`tty_unregister_device()`](#c.tty_unregister_device "tty_unregister_device") so the devices will
    be show up properly in sysfs. If not set, all [`tty_driver.num`](#c.tty_driver "tty_driver") entries
    will be created by the tty core in sysfs when [`tty_register_driver()`](#c.tty_register_driver "tty_register_driver") is
    called. This is to be used by drivers that have tty devices that can
    appear and disappear while the main tty driver is registered with the
    tty core.

`TTY_DRIVER_DEVPTS_MEM`
:   Don’t use the standard arrays ([`tty_driver.ttys`](#c.tty_driver "tty_driver") and
    [`tty_driver.termios`](#c.tty_driver "tty_driver")), instead use dynamic memory keyed through the
    devpts filesystem. This is only applicable to the PTY driver.

`TTY_DRIVER_HARDWARE_BREAK`
:   Hardware handles break signals. Pass the requested timeout to the
    [`tty_operations.break_ctl`](#c.tty_operations "tty_operations") instead of using a simple on/off interface.

`TTY_DRIVER_DYNAMIC_ALLOC`
:   Do not allocate structures which are needed per line for this driver
    ([`tty_driver.ports`](#c.tty_driver "tty_driver")) as it would waste memory. The driver will take
    care. This is only applicable to the PTY driver.

`TTY_DRIVER_UNNUMBERED_NODE`
:   Do not create numbered `/dev` nodes. For example, create
    `/dev/ttyprintk` and not `/dev/ttyprintk0`. Applicable only when a
    driver for a single tty device is being allocated.

`TTY_DRIVER_NO_WORKQUEUE`
:   Do not create workqueue when [`tty_register_driver()`](#c.tty_register_driver "tty_register_driver"). Whenever set, flip
    buffer workqueue can be set by `tty_port_link_wq()` for every port.

**Description**

These are flags passed to [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver").

> `(IGNBRK || (!BRKINT && !PARMRK)) && (IGNPAR || !INPCK)`
>
> That is, if there is no reason for the driver to
> send notifications of parity and break characters up to the line
> driver, it won’t do so. This allows the line driver to optimize for
> this case if this flag is set. (Note that there is also a promise, if
> the above case is true, not to signal overruns, either.)

---

## [Registration](#id3)

When a [`struct tty_driver`](#c.tty_driver "tty_driver") is allocated and filled in, it can be registered using
[`tty_register_driver()`](#c.tty_register_driver "tty_register_driver"). It is recommended to pass `TTY_DRIVER_DYNAMIC_DEV` in
flags of [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver"). If it is not passed, *all* devices are also
registered during [`tty_register_driver()`](#c.tty_register_driver "tty_register_driver") and the following paragraph of
registering devices can be skipped for such drivers. However, the [`struct
tty_port`](tty_port.html#c.tty_port "tty_port") part in [Registering Devices](#registering-devices) is still relevant there.

int tty\_register\_driver(struct [tty\_driver](#c.tty_driver "tty_driver") \*driver)
:   register a tty driver

**Parameters**

`struct tty_driver *driver`
:   driver to register

**Description**

Called by a tty driver to register itself.

void tty\_unregister\_driver(struct [tty\_driver](#c.tty_driver "tty_driver") \*driver)
:   unregister a tty driver

**Parameters**

`struct tty_driver *driver`
:   driver to unregister

**Description**

Called by a tty driver to unregister itself.

### [Registering Devices](#id4)

Every TTY device shall be backed by a [`struct tty_port`](tty_port.html#c.tty_port "tty_port"). Usually, TTY drivers
embed tty\_port into device’s private structures. Further details about handling
tty\_port can be found in [TTY Port](tty_port.html). The driver is also recommended to use
tty\_port’s reference counting by `tty_port_get()` and [`tty_port_put()`](tty_port.html#c.tty_port_put "tty_port_put"). The final
put is supposed to free the tty\_port including the device’s private struct.

Unless `TTY_DRIVER_DYNAMIC_DEV` was passed as flags to [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver"),
TTY driver is supposed to register every device discovered in the system
(the latter is preferred). This is performed by [`tty_register_device()`](#c.tty_register_device "tty_register_device"). Or by
[`tty_register_device_attr()`](#c.tty_register_device_attr "tty_register_device_attr") if the driver wants to expose some information
through `struct attribute_group`. Both of them register `index`’th device and
upon return, the device can be opened. There are also preferred tty\_port
variants described in [Linking Devices to Ports](#linking-devices-to-ports) later. It is up to driver to
manage free indices and choosing the right one. The TTY layer only refuses to
register more devices than passed to [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver").

When the device is opened, the TTY layer allocates [`struct tty_struct`](tty_struct.html#c.tty_struct "tty_struct") and starts
calling operations from `tty_driver.ops`, see [TTY Operations
Reference](#tty-operations-reference).

The registration routines are documented as follows:

struct [device](#c.tty_register_device "device") \*tty\_register\_device(struct [tty\_driver](#c.tty_driver "tty_driver") \*driver, unsigned index, struct [device](#c.tty_register_device "device") \*device)
:   register a tty device

**Parameters**

`struct tty_driver *driver`
:   the tty driver that describes the tty device

`unsigned index`
:   the index in the tty driver for this tty device

`struct device *device`
:   a [`struct device`](../infrastructure.html#c.device "device") that is associated with this tty device.
    This field is optional, if there is no known [`struct device`](../infrastructure.html#c.device "device")
    for this tty device it can be set to NULL safely.

**Description**

This call is required to be made to register an individual tty device
if the tty driver’s flags have the `TTY_DRIVER_DYNAMIC_DEV` bit set. If
that bit is not set, this function should not be called by a tty
driver.

Locking: ??

**Return**

A pointer to the [`struct device`](../infrastructure.html#c.device "device") for this tty device (or
ERR\_PTR(-EFOO) on error).

struct [device](#c.tty_register_device_attr "device") \*tty\_register\_device\_attr(struct [tty\_driver](#c.tty_driver "tty_driver") \*driver, unsigned index, struct [device](#c.tty_register_device_attr "device") \*device, void \*drvdata, const struct attribute\_group \*\*attr\_grp)
:   register a tty device

**Parameters**

`struct tty_driver *driver`
:   the tty driver that describes the tty device

`unsigned index`
:   the index in the tty driver for this tty device

`struct device *device`
:   a [`struct device`](../infrastructure.html#c.device "device") that is associated with this tty device.
    This field is optional, if there is no known [`struct device`](../infrastructure.html#c.device "device")
    for this tty device it can be set to `NULL` safely.

`void *drvdata`
:   Driver data to be set to device.

`const struct attribute_group **attr_grp`
:   Attribute group to be set on device.

**Description**

This call is required to be made to register an individual tty device if the
tty driver’s flags have the `TTY_DRIVER_DYNAMIC_DEV` bit set. If that bit is
not set, this function should not be called by a tty driver.

Locking: ??

**Return**

A pointer to the [`struct device`](../infrastructure.html#c.device "device") for this tty device (or
ERR\_PTR(-EFOO) on error).

void tty\_unregister\_device(struct [tty\_driver](#c.tty_driver "tty_driver") \*driver, unsigned index)
:   unregister a tty device

**Parameters**

`struct tty_driver *driver`
:   the tty driver that describes the tty device

`unsigned index`
:   the index in the tty driver for this tty device

**Description**

If a tty device is registered with a call to [`tty_register_device()`](#c.tty_register_device "tty_register_device") then
this function must be called when the tty device is gone.

Locking: ??

---

### [Linking Devices to Ports](#id5)

As stated earlier, every TTY device shall have a [`struct tty_port`](tty_port.html#c.tty_port "tty_port") assigned to
it. It must be known to the TTY layer at `tty_driver.ops.install()`
at latest. There are few helpers to *link* the two. Ideally, the driver uses
[`tty_port_register_device()`](#c.tty_port_register_device "tty_port_register_device") or [`tty_port_register_device_attr()`](#c.tty_port_register_device_attr "tty_port_register_device_attr") instead of
[`tty_register_device()`](#c.tty_register_device "tty_register_device") and [`tty_register_device_attr()`](#c.tty_register_device_attr "tty_register_device_attr") at the registration time.
This way, the driver needs not care about linking later on.

If that is not possible, the driver still can link the tty\_port to a specific
index *before* the actual registration by [`tty_port_link_device()`](#c.tty_port_link_device "tty_port_link_device"). If it still
does not fit, [`tty_port_install()`](tty_port.html#c.tty_port_install "tty_port_install") can be used from the
`tty_driver.ops.install` hook as a last resort. The last one is
dedicated mostly for in-memory devices like PTY where tty\_ports are allocated
on demand.

The linking routines are documented here:

void tty\_port\_link\_device(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, struct [tty\_driver](#c.tty_driver "tty_driver") \*driver, unsigned index)
:   link tty and tty\_port

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_driver *driver`
:   tty\_driver for this device

`unsigned index`
:   index of the tty

**Description**

Provide the tty layer with a link from a tty (specified by **index**) to a
tty\_port (**port**). Use this only if neither [`tty_port_register_device()`](#c.tty_port_register_device "tty_port_register_device") nor
[`tty_port_install()`](tty_port.html#c.tty_port_install "tty_port_install") is used in the driver. If used, this has to be called
before [`tty_register_driver()`](#c.tty_register_driver "tty_register_driver").

struct [device](#c.tty_port_register_device "device") \*tty\_port\_register\_device(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, struct [tty\_driver](#c.tty_driver "tty_driver") \*driver, unsigned index, struct [device](#c.tty_port_register_device "device") \*device)
:   register tty device

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_driver *driver`
:   tty\_driver for this device

`unsigned index`
:   index of the tty

`struct device *device`
:   parent if exists, otherwise NULL

**Description**

It is the same as [`tty_register_device()`](#c.tty_register_device "tty_register_device") except the provided **port** is linked
to a concrete tty specified by **index**. Use this or [`tty_port_install()`](tty_port.html#c.tty_port_install "tty_port_install") (or
both). Call [`tty_port_link_device()`](#c.tty_port_link_device "tty_port_link_device") as a last resort.

struct [device](#c.tty_port_register_device_attr "device") \*tty\_port\_register\_device\_attr(struct [tty\_port](tty_port.html#c.tty_port "tty_port") \*port, struct [tty\_driver](#c.tty_driver "tty_driver") \*driver, unsigned index, struct [device](#c.tty_port_register_device_attr "device") \*device, void \*drvdata, const struct attribute\_group \*\*attr\_grp)
:   register tty device

**Parameters**

`struct tty_port *port`
:   tty\_port of the device

`struct tty_driver *driver`
:   tty\_driver for this device

`unsigned index`
:   index of the tty

`struct device *device`
:   parent if exists, otherwise NULL

`void *drvdata`
:   Driver data to be set to device.

`const struct attribute_group **attr_grp`
:   Attribute group to be set on device.

**Description**

It is the same as [`tty_register_device_attr()`](#c.tty_register_device_attr "tty_register_device_attr") except the provided **port** is
linked to a concrete tty specified by **index**. Use this or [`tty_port_install()`](tty_port.html#c.tty_port_install "tty_port_install")
(or both). Call [`tty_port_link_device()`](#c.tty_port_link_device "tty_port_link_device") as a last resort.

---

## [TTY Driver Reference](#id6)

All members of [`struct tty_driver`](#c.tty_driver "tty_driver") are documented here. The required members are
noted at the end. [`struct tty_operations`](#c.tty_operations "tty_operations") are documented next.

struct tty\_driver
:   * driver for TTY devices

**Definition**:

```
struct tty_driver {
    struct kref kref;
    struct cdev **cdevs;
    struct module   *owner;
    const char      *driver_name;
    const char      *name;
    int name_base;
    int major;
    int minor_start;
    unsigned int    num;
    enum tty_driver_type type;
    enum tty_driver_subtype subtype;
    struct ktermios init_termios;
    unsigned long   flags;
    struct proc_dir_entry *proc_entry;
    struct tty_driver *other;
    struct workqueue_struct *flip_wq;
    struct tty_struct **ttys;
    struct tty_port **ports;
    struct ktermios **termios;
    void *driver_state;
    const struct tty_operations *ops;
    struct list_head tty_drivers;
};
```

**Members**

`kref`
:   reference counting. Reaching zero frees all the internals and the
    driver.

`cdevs`
:   allocated/registered character /dev devices

`owner`
:   modules owning this driver. Used drivers cannot be rmmod’ed.
    Automatically set by [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver").

`driver_name`
:   name of the driver used in /proc/tty

`name`
:   used for constructing /dev node name

`name_base`
:   used as a number base for constructing /dev node name

`major`
:   major /dev device number (zero for autoassignment)

`minor_start`
:   the first minor /dev device number

`num`
:   number of devices allocated

`type`
:   type of tty driver (`enum tty_driver_type`)

`subtype`
:   subtype of tty driver (`enum tty_driver_subtype`)

`init_termios`
:   termios to set to each tty initially (e.g. `tty_std_termios`)

`flags`
:   tty driver flags (`TTY_DRIVER_`)

`proc_entry`
:   proc fs entry, used internally

`other`
:   driver of the linked tty; only used for the PTY driver

`flip_wq`
:   workqueue to queue flip buffer work on

`ttys`
:   array of active [`struct tty_struct`](tty_struct.html#c.tty_struct "tty_struct"), set by [`tty_standard_install()`](tty_struct.html#c.tty_standard_install "tty_standard_install")

`ports`
:   array of [`struct tty_port`](tty_port.html#c.tty_port "tty_port"); can be set during initialization by
    [`tty_port_link_device()`](#c.tty_port_link_device "tty_port_link_device") and similar

`termios`
:   storage for termios at each TTY close for the next open

`driver_state`
:   pointer to driver’s arbitrary data

`ops`
:   driver hooks for TTYs. Set them using `tty_set_operations()`. Use [`struct
    tty_port`](tty_port.html#c.tty_port "tty_port") helpers in them as much as possible.

`tty_drivers`
:   used internally to link tty\_drivers together

**Description**

The usual handling of [`struct tty_driver`](#c.tty_driver "tty_driver") is to allocate it by
[`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver"), set up all the necessary members, and register it by
[`tty_register_driver()`](#c.tty_register_driver "tty_register_driver"). At last, the driver is torn down by calling
[`tty_unregister_driver()`](#c.tty_unregister_driver "tty_unregister_driver") followed by [`tty_driver_kref_put()`](#c.tty_driver_kref_put "tty_driver_kref_put").

The fields required to be set before calling [`tty_register_driver()`](#c.tty_register_driver "tty_register_driver") include
**driver\_name**, **name**, **type**, **subtype**, **init\_termios**, and **ops**.

---

## [TTY Operations Reference](#id7)

When a TTY is registered, these driver hooks can be invoked by the TTY layer:

struct tty\_operations
:   * interface between driver and tty

**Definition**:

```
struct tty_operations {
    struct tty_struct * (*lookup)(struct tty_driver *driver, struct file *filp, int idx);
    int (*install)(struct tty_driver *driver, struct tty_struct *tty);
    void (*remove)(struct tty_driver *driver, struct tty_struct *tty);
    int (*open)(struct tty_struct * tty, struct file * filp);
    void (*close)(struct tty_struct * tty, struct file * filp);
    void (*shutdown)(struct tty_struct *tty);
    void (*cleanup)(struct tty_struct *tty);
    ssize_t (*write)(struct tty_struct *tty, const u8 *buf, size_t count);
    int (*put_char)(struct tty_struct *tty, u8 ch);
    void (*flush_chars)(struct tty_struct *tty);
    unsigned int (*write_room)(struct tty_struct *tty);
    unsigned int (*chars_in_buffer)(struct tty_struct *tty);
    int (*ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
    long (*compat_ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
    void (*set_termios)(struct tty_struct *tty, const struct ktermios *old);
    void (*throttle)(struct tty_struct * tty);
    void (*unthrottle)(struct tty_struct * tty);
    void (*stop)(struct tty_struct *tty);
    void (*start)(struct tty_struct *tty);
    void (*hangup)(struct tty_struct *tty);
    int (*break_ctl)(struct tty_struct *tty, int state);
    void (*flush_buffer)(struct tty_struct *tty);
    int (*ldisc_ok)(struct tty_struct *tty, int ldisc);
    void (*set_ldisc)(struct tty_struct *tty);
    void (*wait_until_sent)(struct tty_struct *tty, int timeout);
    void (*send_xchar)(struct tty_struct *tty, u8 ch);
    int (*tiocmget)(struct tty_struct *tty);
    int (*tiocmset)(struct tty_struct *tty, unsigned int set, unsigned int clear);
    int (*resize)(struct tty_struct *tty, struct winsize *ws);
    int (*get_icount)(struct tty_struct *tty, struct serial_icounter_struct *icount);
    int (*get_serial)(struct tty_struct *tty, struct serial_struct *p);
    int (*set_serial)(struct tty_struct *tty, struct serial_struct *p);
    void (*show_fdinfo)(struct tty_struct *tty, struct seq_file *m);
#ifdef CONFIG_CONSOLE_POLL;
    int (*poll_init)(struct tty_driver *driver, int line, char *options);
    int (*poll_get_char)(struct tty_driver *driver, int line);
    void (*poll_put_char)(struct tty_driver *driver, int line, char ch);
#endif;
    int (*proc_show)(struct seq_file *m, void *driver);
};
```

**Members**

`lookup`
:   `struct tty_struct *()(struct tty_driver *self, struct file *,
    int idx)`

`install`
:   `int ()(struct tty_driver *self, struct tty_struct *tty)`

`remove`
:   `void ()(struct tty_driver *self, struct tty_struct *tty)`

`open`
:   `int ()(struct tty_struct *tty, struct file *)`

`close`
:   `void ()(struct tty_struct *tty, struct file *)`

`shutdown`
:   `void ()(struct tty_struct *tty)`

`cleanup`
:   `void ()(struct tty_struct *tty)`

`write`
:   `ssize_t ()(struct tty_struct *tty, const u8 *buf, size_t count)`

`put_char`
:   `int ()(struct tty_struct *tty, u8 ch)`

`flush_chars`
:   `void ()(struct tty_struct *tty)`

`write_room`
:   `unsigned int ()(struct tty_struct *tty)`

`chars_in_buffer`
:   `unsigned int ()(struct tty_struct *tty)`

`ioctl`
:   `int ()(struct tty_struct *tty, unsigned int cmd,
    unsigned long arg)`

`compat_ioctl`
:   `long ()(struct tty_struct *tty, unsigned int cmd,
    unsigned long arg)`

`set_termios`
:   `void ()(struct tty_struct *tty, const struct ktermios *old)`

`throttle`
:   `void ()(struct tty_struct *tty)`

`unthrottle`
:   `void ()(struct tty_struct *tty)`

`stop`
:   `void ()(struct tty_struct *tty)`

`start`
:   `void ()(struct tty_struct *tty)`

`hangup`
:   `void ()(struct tty_struct *tty)`

`break_ctl`
:   `int ()(struct tty_struct *tty, int state)`

`flush_buffer`
:   `void ()(struct tty_struct *tty)`

`ldisc_ok`
:   `int ()(struct tty_struct *tty, int ldisc)`

`set_ldisc`
:   `void ()(struct tty_struct *tty)`

`wait_until_sent`
:   `void ()(struct tty_struct *tty, int timeout)`

`send_xchar`
:   `void ()(struct tty_struct *tty, u8 ch)`

`tiocmget`
:   `int ()(struct tty_struct *tty)`

`tiocmset`
:   `int ()(struct tty_struct *tty,
    unsigned int set, unsigned int clear)`

`resize`
:   `int ()(struct tty_struct *tty, struct winsize *ws)`

`get_icount`
:   `int ()(struct tty_struct *tty,
    struct serial_icounter *icount)`

`get_serial`
:   `int ()(struct tty_struct *tty, struct serial_struct *p)`

`set_serial`
:   `int ()(struct tty_struct *tty, struct serial_struct *p)`

`show_fdinfo`
:   `void ()(struct tty_struct *tty, struct seq_file *m)`

`poll_init`
:   `int ()(struct tty_driver *driver, int line, char *options)`

`poll_get_char`
:   `int ()(struct tty_driver *driver, int line)`

`poll_put_char`
:   `void ()(struct tty_driver *driver, int line, char ch)`

`proc_show`
:   `int ()(struct seq_file *m, void *driver)`

**Description**

> Return the tty device corresponding to **idx**, `NULL` if there is not
> one currently in use and an `ERR_PTR` value on error. Called under
> `tty_mutex` (for now!)
>
> Optional method. Default behaviour is to use the **self->ttys** array.
>
> Install a new **tty** into the **self**’s internal tables. Used in
> conjunction with **lookup** and **remove** methods.
>
> Optional method. Default behaviour is to use the **self->ttys** array.
>
> Remove a closed **tty** from the **self**’s internal tables. Used in
> conjunction with **lookup** and **remove** methods.
>
> Optional method. Default behaviour is to use the **self->ttys** array.
>
> This routine is called when a particular **tty** device is opened. This
> routine is mandatory; if this routine is not filled in, the attempted
> open will fail with `ENODEV`.
>
> Required method. Called with tty lock held. May sleep.
>
> This routine is called when a particular **tty** device is closed. At the
> point of return from this call the driver must make no further ldisc
> calls of any kind.
>
> Remark: called even if the corresponding **open()** failed.
>
> Required method. Called with tty lock held. May sleep.
>
> This routine is called under the tty lock when a particular **tty** device
> is closed for the last time. It executes before the **tty** resources
> are freed so may execute while another function holds a **tty** kref.
>
> This routine is called asynchronously when a particular **tty** device
> is closed for the last time freeing up the resources. This is
> actually the second part of shutdown for routines that might sleep.
>
> This routine is called by the kernel to write a series (**count**) of
> characters (**buf**) to the **tty** device. The characters may come from
> user space or kernel space. This routine will return the
> number of characters actually accepted for writing.
>
> May occur in parallel in special cases. Because this includes panic
> paths drivers generally shouldn’t try and do clever locking here.
>
> Optional: Required for writable devices. May not sleep.
>
> This routine is called by the kernel to write a single character **ch** to
> the **tty** device. If the kernel uses this routine, it must call the
> **`flush_chars()`** routine (if defined) when it is done stuffing characters
> into the driver. If there is no room in the queue, the character is
> ignored.
>
> Optional: Kernel will use the **write** method if not provided. Do not
> call this function directly, call [`tty_put_char()`](tty_struct.html#c.tty_put_char "tty_put_char").
>
> This routine is called by the kernel after it has written a
> series of characters to the tty device using **`put_char()`**.
>
> Optional. Do not call this function directly, call
> `tty_driver_flush_chars()`.
>
> This routine returns the numbers of characters the **tty** driver
> will accept for queuing to be written. This number is subject
> to change as output buffers get emptied, or if the output flow
> control is acted.
>
> The ldisc is responsible for being intelligent about multi-threading of
> write\_room/write calls
>
> Required if **write** method is provided else not needed. Do not call this
> function directly, call [`tty_write_room()`](tty_ioctl.html#c.tty_write_room "tty_write_room")
>
> This routine returns the number of characters in the device private
> output queue. Used in [`tty_wait_until_sent()`](tty_ioctl.html#c.tty_wait_until_sent "tty_wait_until_sent") and for poll()
> implementation.
>
> Optional: if not provided, it is assumed there is no queue on the
> device. Do not call this function directly, call [`tty_chars_in_buffer()`](tty_ioctl.html#c.tty_chars_in_buffer "tty_chars_in_buffer").
>
> This routine allows the **tty** driver to implement device-specific
> ioctls. If the ioctl number passed in **cmd** is not recognized by the
> driver, it should return `ENOIOCTLCMD`.
>
> Optional.
>
> Implement ioctl processing for 32 bit process on 64 bit system.
>
> Optional.
>
> This routine allows the **tty** driver to be notified when device’s
> termios settings have changed. New settings are in **tty->termios**.
> Previous settings are passed in the **old** argument.
>
> The API is defined such that the driver should return the actual modes
> selected. This means that the driver is responsible for modifying any
> bits in **tty->termios** it cannot fulfill to indicate the actual modes
> being used.
>
> Optional. Called under the **tty->termios\_rwsem**. May sleep.
>
> This routine allows the **tty** driver to decide if it can deal
> with a particular **ldisc**.
>
> Optional. Called under the **tty->ldisc\_sem** and **tty->termios\_rwsem**.
>
> This routine allows the **tty** driver to be notified when the device’s
> line discipline is being changed. At the point this is done the
> discipline is not yet usable.
>
> Optional. Called under the **tty->ldisc\_sem** and **tty->termios\_rwsem**.
>
> This routine notifies the **tty** driver that input buffers for the line
> discipline are close to full, and it should somehow signal that no more
> characters should be sent to the **tty**.
>
> Serialization including with **`unthrottle()`** is the job of the ldisc
> layer.
>
> Optional: Always invoke via [`tty_throttle_safe()`](tty_ioctl.html#c.tty_throttle_safe "tty_throttle_safe"). Called under the
> **tty->termios\_rwsem**.
>
> This routine notifies the **tty** driver that it should signal that
> characters can now be sent to the **tty** without fear of overrunning the
> input buffers of the line disciplines.
>
> Optional. Always invoke via [`tty_unthrottle()`](tty_ioctl.html#c.tty_unthrottle "tty_unthrottle"). Called under the
> **tty->termios\_rwsem**.
>
> This routine notifies the **tty** driver that it should stop outputting
> characters to the tty device.
>
> Called with **tty->flow.lock** held. Serialized with **[`start()`](../../networking/ieee802154.html#c.start "start")** method.
>
> Optional. Always invoke via [`stop_tty()`](tty_struct.html#c.stop_tty "stop_tty").
>
> This routine notifies the **tty** driver that it resumed sending
> characters to the **tty** device.
>
> Called with **tty->flow.lock** held. Serialized with [`stop()`](../../networking/ieee802154.html#c.stop "stop") method.
>
> Optional. Always invoke via [`start_tty()`](tty_struct.html#c.start_tty "start_tty").
>
> This routine notifies the **tty** driver that it should hang up the **tty**
> device.
>
> Optional. Called with tty lock held.
>
> This optional routine requests the **tty** driver to turn on or off BREAK
> status on the RS-232 port. If **state** is -1, then the BREAK status
> should be turned on; if **state** is 0, then BREAK should be turned off.
>
> If this routine is implemented, the high-level tty driver will handle
> the following ioctls: `TCSBRK`, `TCSBRKP`, `TIOCSBRK`, `TIOCCBRK`.
>
> If the driver sets `TTY_DRIVER_HARDWARE_BREAK` in [`tty_alloc_driver()`](#c.tty_alloc_driver "tty_alloc_driver"),
> then the interface will also be called with actual times and the
> hardware is expected to do the delay work itself. 0 and -1 are still
> used for on/off.
>
> Optional: Required for `TCSBRK`/`BRKP`/etc. handling. May sleep.
>
> This routine discards device private output buffer. Invoked on close,
> hangup, to implement `TCOFLUSH` ioctl and similar.
>
> Optional: if not provided, it is assumed there is no queue on the
> device. Do not call this function directly, call
> [`tty_driver_flush_buffer()`](tty_ioctl.html#c.tty_driver_flush_buffer "tty_driver_flush_buffer").
>
> This routine waits until the device has written out all of the
> characters in its transmitter FIFO. Or until **timeout** (in jiffies) is
> reached.
>
> Optional: If not provided, the device is assumed to have no FIFO.
> Usually correct to invoke via [`tty_wait_until_sent()`](tty_ioctl.html#c.tty_wait_until_sent "tty_wait_until_sent"). May sleep.
>
> This routine is used to send a high-priority XON/XOFF character (**ch**)
> to the **tty** device.
>
> Optional: If not provided, then the **write** method is called under
> the **tty->atomic\_write\_lock** to keep it serialized with the ldisc.
>
> This routine is used to obtain the modem status bits from the **tty**
> driver.
>
> Optional: If not provided, then `ENOTTY` is returned from the `TIOCMGET`
> ioctl. Do not call this function directly, call [`tty_tiocmget()`](tty_internals.html#c.tty_tiocmget "tty_tiocmget").
>
> This routine is used to set the modem status bits to the **tty** driver.
> First, **clear** bits should be cleared, then **set** bits set.
>
> Optional: If not provided, then `ENOTTY` is returned from the `TIOCMSET`
> ioctl. Do not call this function directly, call [`tty_tiocmset()`](tty_internals.html#c.tty_tiocmset "tty_tiocmset").
>
> Called when a termios request is issued which changes the requested
> terminal geometry to **ws**.
>
> Optional: the default action is to update the termios structure
> without error. This is usually the correct behaviour. Drivers should
> not force errors here if they are not resizable objects (e.g. a serial
> line). See [`tty_do_resize()`](tty_struct.html#c.tty_do_resize "tty_do_resize") if you need to wrap the standard method
> in your own logic -- the usual case.
>
> Called when the **tty** device receives a `TIOCGICOUNT` ioctl. Passed a
> kernel structure **icount** to complete.
>
> Optional: called only if provided, otherwise `ENOTTY` will be returned.
>
> Called when the **tty** device receives a `TIOCGSERIAL` ioctl. Passed a
> kernel structure **p** (`struct serial_struct`) to complete.
>
> Optional: called only if provided, otherwise `ENOTTY` will be returned.
> Do not call this function directly, call `tty_tiocgserial()`.
>
> Called when the **tty** device receives a `TIOCSSERIAL` ioctl. Passed a
> kernel structure **p** (`struct serial_struct`) to set the values from.
>
> Optional: called only if provided, otherwise `ENOTTY` will be returned.
> Do not call this function directly, call `tty_tiocsserial()`.
>
> Called when the **tty** device file descriptor receives a fdinfo request
> from VFS (to show in /proc/<pid>/fdinfo/). **m** should be filled with
> information.
>
> Optional: called only if provided, otherwise nothing is written to **m**.
> Do not call this function directly, call `tty_show_fdinfo()`.
>
> kgdboc support ([Using kgdb, kdb and the kernel debugger internals](../../process/debugging/kgdb.html)). This routine is
> called to initialize the HW for later use by calling **poll\_get\_char** or
> **poll\_put\_char**.
>
> Optional: called only if provided, otherwise skipped as a non-polling
> driver.
>
> kgdboc support (see **poll\_init**). **driver** should read a character from a
> tty identified by **line** and return it.
>
> Optional: called only if **poll\_init** provided.
>
> kgdboc support (see **poll\_init**). **driver** should write character **ch** to
> a tty identified by **line**.
>
> Optional: called only if **poll\_init** provided.
>
> Driver **driver** (cast to [`struct tty_driver`](#c.tty_driver "tty_driver")) can show additional info in
> /proc/tty/driver/<driver\_name>. It is enough to fill in the information
> into **m**.
>
> Optional: called only if provided, otherwise no /proc entry created.

This structure defines the interface between the low-level tty driver and
the tty routines. These routines can be defined. Unless noted otherwise,
they are optional, and can be filled in with a `NULL` pointer.
