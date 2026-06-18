# 3.5.V4L2 device instance

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-device.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.5. V4L2 device instance

Each device instance is represented by a [`struct v4l2_device`](#c.v4l2_device "v4l2_device").
Very simple devices can just allocate this struct, but most of the time you
would embed this `struct inside` a larger struct.

You must register the device instance by calling:

> [`v4l2_device_register`](#c.v4l2_device_register "v4l2_device_register")
> (dev, [`v4l2_dev`](#c.v4l2_device "v4l2_device")).

Registration will initialize the [`v4l2_device`](#c.v4l2_device "v4l2_device") struct. If the
dev->driver\_data field is `NULL`, it will be linked to
[`v4l2_dev`](#c.v4l2_device "v4l2_device") argument.

Drivers that want integration with the media device framework need to set
dev->driver\_data manually to point to the driver-specific device structure
that embed the [`struct v4l2_device`](#c.v4l2_device "v4l2_device") instance. This is achieved by a
`dev_set_drvdata()` call before registering the V4L2 device instance.
They must also set the [`struct v4l2_device`](#c.v4l2_device "v4l2_device") mdev field to point to a
properly initialized and registered [`media_device`](mc-core.html#c.media_device "media_device") instance.

If [`v4l2_dev`](#c.v4l2_device "v4l2_device")->name is empty then it will be set to a
value derived from dev (driver name followed by the bus\_id, to be precise).
If you set it up before calling [`v4l2_device_register()`](#c.v4l2_device_register "v4l2_device_register") then it will
be untouched. If dev is `NULL`, then you **must** setup
[`v4l2_dev`](#c.v4l2_device "v4l2_device")->name before calling
[`v4l2_device_register()`](#c.v4l2_device_register "v4l2_device_register").

You can use [`v4l2_device_set_name()`](#c.v4l2_device_set_name "v4l2_device_set_name") to set the name based on a driver
name and a driver-global atomic\_t instance. This will generate names like
`ivtv0`, `ivtv1`, etc. If the name ends with a digit, then it will insert
a dash: `cx18-0`, `cx18-1`, etc. This function returns the instance number.

The first `dev` argument is normally the `struct device` pointer of a
`pci_dev`, `usb_interface` or `platform_device`. It is rare for dev to
be `NULL`, but it happens with ISA devices or when one device creates
multiple PCI devices, thus making it impossible to associate
[`v4l2_dev`](#c.v4l2_device "v4l2_device") with a particular parent.

You can also supply a `notify()` callback that can be called by sub-devices
to notify you of events. Whether you need to set this depends on the
sub-device. Any notifications a sub-device supports must be defined in a header
in `include/media/subdevice.h`.

V4L2 devices are unregistered by calling:

> [`v4l2_device_unregister()`](#c.v4l2_device_unregister "v4l2_device_unregister")
> ([`v4l2_dev`](#c.v4l2_device "v4l2_device")).

If the dev->driver\_data field points to [`v4l2_dev`](#c.v4l2_device "v4l2_device"),
it will be reset to `NULL`. Unregistering will also automatically unregister
all subdevs from the device.

If you have a hotpluggable device (e.g. a USB device), then when a disconnect
happens the parent device becomes invalid. Since [`v4l2_device`](#c.v4l2_device "v4l2_device") has a
pointer to that parent device it has to be cleared as well to mark that the
parent is gone. To do this call:

> [`v4l2_device_disconnect()`](#c.v4l2_device_disconnect "v4l2_device_disconnect")
> ([`v4l2_dev`](#c.v4l2_device "v4l2_device")).

This does *not* unregister the subdevs, so you still need to call the
[`v4l2_device_unregister()`](#c.v4l2_device_unregister "v4l2_device_unregister") function for that. If your driver is not
hotpluggable, then there is no need to call [`v4l2_device_disconnect()`](#c.v4l2_device_disconnect "v4l2_device_disconnect").

Sometimes you need to iterate over all devices registered by a specific
driver. This is usually the case if multiple device drivers use the same
hardware. E.g. the ivtvfb driver is a framebuffer driver that uses the ivtv
hardware. The same is true for alsa drivers for example.

You can iterate over all registered devices as follows:

```
static int callback(struct device *dev, void *p)
{
        struct v4l2_device *v4l2_dev = dev_get_drvdata(dev);

        /* test if this device was inited */
        if (v4l2_dev == NULL)
                return 0;
        ...
        return 0;
}

int iterate(void *p)
{
        struct device_driver *drv;
        int err;

        /* Find driver 'ivtv' on the PCI bus.
        pci_bus_type is a global. For USB buses use usb_bus_type. */
        drv = driver_find("ivtv", &pci_bus_type);
        /* iterate over all ivtv device instances */
        err = driver_for_each_device(drv, NULL, p, callback);
        put_driver(drv);
        return err;
}
```

Sometimes you need to keep a running counter of the device instance. This is
commonly used to map a device instance to an index of a module option array.

The recommended approach is as follows:

```
static atomic_t drv_instance = ATOMIC_INIT(0);

static int drv_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
        ...
        state->instance = atomic_inc_return(&drv_instance) - 1;
}
```

If you have multiple device nodes then it can be difficult to know when it is
safe to unregister [`v4l2_device`](#c.v4l2_device "v4l2_device") for hotpluggable devices. For this
purpose [`v4l2_device`](#c.v4l2_device "v4l2_device") has refcounting support. The refcount is
increased whenever [`video_register_device()`](v4l2-dev.html#c.video_register_device "video_register_device") is called and it is
decreased whenever that device node is released. When the refcount reaches
zero, then the [`v4l2_device`](#c.v4l2_device "v4l2_device") `release()` callback is called. You can
do your final cleanup there.

If other device nodes (e.g. ALSA) are created, then you can increase and
decrease the refcount manually as well by calling:

> [`v4l2_device_get()`](#c.v4l2_device_get "v4l2_device_get")
> ([`v4l2_dev`](#c.v4l2_device "v4l2_device")).

or:

> [`v4l2_device_put()`](#c.v4l2_device_put "v4l2_device_put")
> ([`v4l2_dev`](#c.v4l2_device "v4l2_device")).

Since the initial refcount is 1 you also need to call
[`v4l2_device_put()`](#c.v4l2_device_put "v4l2_device_put") in the `disconnect()` callback (for USB devices)
or in the `remove()` callback (for e.g. PCI devices), otherwise the refcount
will never reach 0.

## 3.5.1. v4l2\_device functions and data structures

struct v4l2\_device
:   main `struct to` for V4L2 device drivers

> **Definition**:
>
> ```
> struct v4l2_device {
>       struct device *dev;
>       struct media_device *mdev;
>       struct list_head subdevs;
>       spinlock_t lock;
>       char name[36];
>       void (*notify)(struct v4l2_subdev *sd, unsigned int notification, void *arg);
>       struct v4l2_ctrl_handler *ctrl_handler;
>       struct v4l2_prio_state prio;
>       struct kref ref;
>       void (*release)(struct v4l2_device *v4l2_dev);
> };
> ```

**Members**

`dev`
:   pointer to [`struct device`](../infrastructure.html#c.device "device").

`mdev`
:   pointer to [`struct media_device`](mc-core.html#c.media_device "media_device"), may be NULL.

`subdevs`
:   used to keep track of the registered subdevs

`lock`
:   lock this struct; can be used by the driver as well
    if this `struct is` embedded into a larger struct.

`name`
:   unique device name, by default the driver name + bus ID

`notify`
:   notify operation called by some sub-devices.

`ctrl_handler`
:   The control handler. May be `NULL`.

`prio`
:   Device’s priority state

`ref`
:   Keep track of the references to this struct.

`release`
:   Release function that is called when the ref count
    goes to 0.

**Description**

Each instance of a V4L2 device should create the v4l2\_device struct,
either stand-alone or embedded in a larger struct.

It allows easy access to sub-devices (see v4l2-subdev.h) and provides
basic V4L2 device-level support.

Note

1. **dev->driver\_data** points to this struct.
2. **dev** might be `NULL` if there is no parent device

void v4l2\_device\_get(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   gets a V4L2 device reference

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to struct [`v4l2_device`](#c.v4l2_device "v4l2_device")

**Description**

This is an ancillary routine meant to increment the usage for the
struct [`v4l2_device`](#c.v4l2_device "v4l2_device") pointed by **v4l2\_dev**.

int v4l2\_device\_put(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   puts a V4L2 device reference

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to struct [`v4l2_device`](#c.v4l2_device "v4l2_device")

**Description**

This is an ancillary routine meant to decrement the usage for the
struct [`v4l2_device`](#c.v4l2_device "v4l2_device") pointed by **v4l2\_dev**.

int v4l2\_device\_register(struct [device](../infrastructure.html#c.device "device") \*dev, struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Initialize v4l2\_dev and make **dev->driver\_data** point to **v4l2\_dev**.

**Parameters**

`struct device *dev`
:   pointer to struct [`device`](../infrastructure.html#c.device "device")

`struct v4l2_device *v4l2_dev`
:   pointer to struct [`v4l2_device`](#c.v4l2_device "v4l2_device")

**Description**

Note

**dev** may be `NULL` in rare cases (ISA devices).
In such case the caller must fill in the **v4l2\_dev->name** field
before calling this function.

int v4l2\_device\_set\_name(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev, const char \*basename, atomic\_t \*instance)
:   Optional function to initialize the name field of struct [`v4l2_device`](#c.v4l2_device "v4l2_device")

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to struct [`v4l2_device`](#c.v4l2_device "v4l2_device")

`const char *basename`
:   base name for the device name

`atomic_t *instance`
:   pointer to a static atomic\_t var with the instance usage for
    the device driver.

**Description**

[`v4l2_device_set_name()`](#c.v4l2_device_set_name "v4l2_device_set_name") initializes the name field of struct [`v4l2_device`](#c.v4l2_device "v4l2_device")
using the driver name and a driver-global atomic\_t instance.

This function will increment the instance counter and returns the
instance value used in the name.

**Example**

> static atomic\_t drv\_instance = ATOMIC\_INIT(0);
>
> ...
>
> instance = v4l2\_device\_set\_name(&v4l2\_dev, “foo”, &drv\_instance);

The first time this is called the name field will be set to foo0 and
this function returns 0. If the name ends with a digit (e.g. cx18),
then the name will be set to cx18-0 since cx180 would look really odd.

void v4l2\_device\_disconnect(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Change V4L2 device state to disconnected.

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](#c.v4l2_device "v4l2_device")

**Description**

Should be called when the USB parent disconnects.
Since the parent disappears, this ensures that **v4l2\_dev** doesn’t have
an invalid parent pointer.

Note

This function sets **v4l2\_dev->dev** to NULL.

void v4l2\_device\_unregister(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Unregister all sub-devices and any other resources related to **v4l2\_dev**.

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](#c.v4l2_device "v4l2_device")

v4l2\_device\_register\_subdev

`v4l2_device_register_subdev (v4l2_dev, sd)`

> Registers a subdev with a v4l2 device.

**Parameters**

`v4l2_dev`
:   pointer to struct [`v4l2_device`](#c.v4l2_device "v4l2_device")

`sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

**Description**

While registered, the subdev module is marked as in-use.

An error is returned if the module is no longer loaded on any attempts
to register it.

void v4l2\_device\_unregister\_subdev(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Unregisters a subdev with a v4l2 device.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

**Description**

Note

Can also be called if the subdev wasn’t registered. In such
case, it will do nothing.

int \_\_v4l2\_device\_register\_subdev\_nodes(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev, bool read\_only)
:   Registers device nodes for all subdevs of the v4l2 device that are marked with the `V4L2_SUBDEV_FL_HAS_DEVNODE` flag.

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](#c.v4l2_device "v4l2_device")

`bool read_only`
:   subdevices read-only flag. True to register the subdevices
    device nodes in read-only mode, false to allow full access to the
    subdevice userspace API.

int v4l2\_device\_register\_subdev\_nodes(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Registers subdevices device nodes with unrestricted access to the subdevice userspace operations

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](#c.v4l2_device "v4l2_device")

**Description**

Internally calls [`__v4l2_device_register_subdev_nodes()`](#c.__v4l2_device_register_subdev_nodes "__v4l2_device_register_subdev_nodes"). See its documentation
for more details.

int v4l2\_device\_register\_ro\_subdev\_nodes(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Registers subdevices device nodes in read-only mode

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](#c.v4l2_device "v4l2_device")

**Description**

Internally calls [`__v4l2_device_register_subdev_nodes()`](#c.__v4l2_device_register_subdev_nodes "__v4l2_device_register_subdev_nodes"). See its documentation
for more details.

void v4l2\_subdev\_notify(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, unsigned int notification, void \*arg)
:   Sends a notification to v4l2\_device.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`unsigned int notification`
:   type of notification. Please notice that the notification
    type is driver-specific.

`void *arg`
:   arguments for the notification. Those are specific to each
    notification type.

bool v4l2\_device\_supports\_requests(struct [v4l2\_device](#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Test if requests are supported.

**Parameters**

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](#c.v4l2_device "v4l2_device")

v4l2\_device\_for\_each\_subdev

`v4l2_device_for_each_subdev (sd, v4l2_dev)`

> Helper macro that interates over all sub-devices of a given [`v4l2_device`](#c.v4l2_device "v4l2_device").

**Parameters**

`sd`
:   pointer that will be filled by the macro with all
    [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") pointer used as an iterator by the loop.

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

**Description**

This macro iterates over all sub-devices owned by the **v4l2\_dev** device.
It acts as a for loop iterator and executes the next statement with
the **sd** variable pointing to each sub-device in turn.

\_\_v4l2\_device\_call\_subdevs\_p

`__v4l2_device_call_subdevs_p (v4l2_dev, sd, cond, o, f, args...)`

> Calls the specified operation for all subdevs matching the condition.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`sd`
:   pointer that will be filled by the macro with all
    [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") pointer used as an iterator by the loop.

`cond`
:   condition to be match

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

Ignore any errors.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

\_\_v4l2\_device\_call\_subdevs

`__v4l2_device_call_subdevs (v4l2_dev, cond, o, f, args...)`

> Calls the specified operation for all subdevs matching the condition.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`cond`
:   condition to be match

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

Ignore any errors.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

\_\_v4l2\_device\_call\_subdevs\_until\_err\_p

`__v4l2_device_call_subdevs_until_err_p (v4l2_dev, sd, cond, o, f, args...)`

> Calls the specified operation for all subdevs matching the condition.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`sd`
:   pointer that will be filled by the macro with all
    [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") sub-devices associated with **v4l2\_dev**.

`cond`
:   condition to be match

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

If the operation returns an error other than 0 or `-ENOIOCTLCMD`
for any subdevice, then abort and return with that error code, zero
otherwise.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

\_\_v4l2\_device\_call\_subdevs\_until\_err

`__v4l2_device_call_subdevs_until_err (v4l2_dev, cond, o, f, args...)`

> Calls the specified operation for all subdevs matching the condition.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`cond`
:   condition to be match

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

If the operation returns an error other than 0 or `-ENOIOCTLCMD`
for any subdevice, then abort and return with that error code,
zero otherwise.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

v4l2\_device\_call\_all

`v4l2_device_call_all (v4l2_dev, grpid, o, f, args...)`

> Calls the specified operation for all subdevs matching the [`v4l2_subdev.grp_id`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev"), as assigned by the bridge driver.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`grpid`
:   [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")->grp\_id group ID to match.
    Use 0 to match them all.

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

Ignore any errors.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

v4l2\_device\_call\_until\_err

`v4l2_device_call_until_err (v4l2_dev, grpid, o, f, args...)`

> Calls the specified operation for all subdevs matching the [`v4l2_subdev.grp_id`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev"), as assigned by the bridge driver, until an error occurs.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`grpid`
:   [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")->grp\_id group ID to match.
    Use 0 to match them all.

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

If the operation returns an error other than 0 or `-ENOIOCTLCMD`
for any subdevice, then abort and return with that error code,
zero otherwise.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

v4l2\_device\_mask\_call\_all

`v4l2_device_mask_call_all (v4l2_dev, grpmsk, o, f, args...)`

> Calls the specified operation for all subdevices where a group ID matches a specified bitmask.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`grpmsk`
:   bitmask to be checked against [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")->grp\_id
    group ID to be matched. Use 0 to match them all.

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

Ignore any errors.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

v4l2\_device\_mask\_call\_until\_err

`v4l2_device_mask_call_until_err (v4l2_dev, grpmsk, o, f, args...)`

> Calls the specified operation for all subdevices where a group ID matches a specified bitmask.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`grpmsk`
:   bitmask to be checked against [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")->grp\_id
    group ID to be matched. Use 0 to match them all.

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

If the operation returns an error other than 0 or `-ENOIOCTLCMD`
for any subdevice, then abort and return with that error code,
zero otherwise.

**Note**

subdevs cannot be added or deleted while walking
the subdevs list.

v4l2\_device\_has\_op

`v4l2_device_has_op (v4l2_dev, grpid, o, f)`

> checks if any subdev with matching grpid has a given ops.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`grpid`
:   [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")->grp\_id group ID to match.
    Use 0 to match them all.

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").

v4l2\_device\_mask\_has\_op

`v4l2_device_mask_has_op (v4l2_dev, grpmsk, o, f)`

> checks if any subdev with matching group mask has a given ops.

**Parameters**

`v4l2_dev`
:   [`struct v4l2_device`](#c.v4l2_device "v4l2_device") owning the sub-devices to iterate over.

`grpmsk`
:   bitmask to be checked against [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")->grp\_id
    group ID to be matched. Use 0 to match them all.

`o`
:   name of the element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of operations functions.

`f`
:   operation function that will be called if **cond** matches.
    The operation functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](v4l2-subdev.html#c.v4l2_subdev_ops "v4l2_subdev_ops").
