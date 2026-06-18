# 3.7.V4L2 sub-devices

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-subdev.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.7. V4L2 sub-devices

Many drivers need to communicate with sub-devices. These devices can do all
sort of tasks, but most commonly they handle audio and/or video muxing,
encoding or decoding. For webcams common sub-devices are sensors and camera
controllers.

Usually these are I2C devices, but not necessarily. In order to provide the
driver with a consistent interface to these sub-devices the
[`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") struct (v4l2-subdev.h) was created.

Each sub-device driver must have a [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") struct. This `struct
can` be stand-alone for simple sub-devices or it might be embedded in a larger
struct if more state information needs to be stored. Usually there is a
low-level device struct (e.g. `i2c_client`) that contains the device data as
setup by the kernel. It is recommended to store that pointer in the private
data of [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") using [`v4l2_set_subdevdata()`](#c.v4l2_set_subdevdata "v4l2_set_subdevdata"). That makes
it easy to go from a [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") to the actual low-level bus-specific
device data.

You also need a way to go from the low-level `struct to` [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev").
For the common i2c\_client `struct the` `i2c_set_clientdata()` call is used to store
a [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") pointer, for other buses you may have to use other
methods.

Bridges might also need to store per-subdev private data, such as a pointer to
bridge-specific per-subdev private data. The [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") structure
provides host private data for that purpose that can be accessed with
[`v4l2_get_subdev_hostdata()`](#c.v4l2_get_subdev_hostdata "v4l2_get_subdev_hostdata") and [`v4l2_set_subdev_hostdata()`](#c.v4l2_set_subdev_hostdata "v4l2_set_subdev_hostdata").

From the bridge driver perspective, you load the sub-device module and somehow
obtain the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") pointer. For i2c devices this is easy: you call
`i2c_get_clientdata()`. For other buses something similar needs to be done.
Helper functions exist for sub-devices on an I2C bus that do most of this
tricky work for you.

Each [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") contains function pointers that sub-device drivers
can implement (or leave `NULL` if it is not applicable). Since sub-devices can
do so many different things and you do not want to end up with a huge ops `struct
of` which only a handful of ops are commonly implemented, the function pointers
are sorted according to category and each category has its own ops struct.

The top-level ops `struct contains` pointers to the category ops structs, which
may be NULL if the subdev driver does not support anything from that category.

It looks like this:

```
struct v4l2_subdev_core_ops {
        int (*log_status)(struct v4l2_subdev *sd);
        int (*init)(struct v4l2_subdev *sd, u32 val);
        ...
};

struct v4l2_subdev_tuner_ops {
        ...
};

struct v4l2_subdev_audio_ops {
        ...
};

struct v4l2_subdev_video_ops {
        ...
};

struct v4l2_subdev_pad_ops {
        ...
};

struct v4l2_subdev_ops {
        const struct v4l2_subdev_core_ops  *core;
        const struct v4l2_subdev_tuner_ops *tuner;
        const struct v4l2_subdev_audio_ops *audio;
        const struct v4l2_subdev_video_ops *video;
        const struct v4l2_subdev_pad_ops *video;
};
```

The core ops are common to all subdevs, the other categories are implemented
depending on the sub-device. E.g. a video device is unlikely to support the
audio ops and vice versa.

This setup limits the number of function pointers while still making it easy
to add new ops and categories.

A sub-device driver initializes the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") `struct using`:

> [`v4l2_subdev_init`](#c.v4l2_subdev_init "v4l2_subdev_init")
> ([`sd`](#c.v4l2_subdev "v4l2_subdev"), &[`ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops")).

Afterwards you need to initialize [`sd`](#c.v4l2_subdev "v4l2_subdev")->name with a
unique name and set the module owner. This is done for you if you use the
i2c helper functions.

If integration with the media framework is needed, you must initialize the
[`media_entity`](mc-core.html#c.media_entity "media_entity") `struct embedded` in the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") struct
(entity field) by calling [`media_entity_pads_init()`](mc-core.html#c.media_entity_pads_init "media_entity_pads_init"), if the entity has
pads:

```
struct media_pad *pads = &my_sd->pads;
int err;

err = media_entity_pads_init(&sd->entity, npads, pads);
```

The pads array must have been previously initialized. There is no need to
manually set the [`struct media_entity`](mc-core.html#c.media_entity "media_entity") function and name fields, but the
revision field must be initialized if needed.

A reference to the entity will be automatically acquired/released when the
subdev device node (if any) is opened/closed.

Don’t forget to cleanup the media entity before the sub-device is destroyed:

```
media_entity_cleanup(&sd->entity);
```

If a sub-device driver implements sink pads, the subdev driver may set the
link\_validate field in [`v4l2_subdev_pad_ops`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops") to provide its own link
validation function. For every link in the pipeline, the link\_validate pad
operation of the sink end of the link is called. In both cases the driver is
still responsible for validating the correctness of the format configuration
between sub-devices and video nodes.

If link\_validate op is not set, the default function
[`v4l2_subdev_link_validate_default()`](#c.v4l2_subdev_link_validate_default "v4l2_subdev_link_validate_default") is used instead. This function
ensures that width, height and the media bus pixel code are equal on both source
and sink of the link. Subdev drivers are also free to use this function to
perform the checks mentioned above in addition to their own checks.

## 3.7.1. Subdev registration

There are currently two ways to register subdevices with the V4L2 core. The
first (traditional) possibility is to have subdevices registered by bridge
drivers. This can be done when the bridge driver has the complete information
about subdevices connected to it and knows exactly when to register them. This
is typically the case for internal subdevices, like video data processing units
within SoCs or complex PCI(e) boards, camera sensors in USB cameras or connected
to SoCs, which pass information about them to bridge drivers, usually in their
platform data.

There are however also situations where subdevices have to be registered
asynchronously to bridge devices. An example of such a configuration is a Device
Tree based system where information about subdevices is made available to the
system independently from the bridge devices, e.g. when subdevices are defined
in DT as I2C device nodes. The API used in this second case is described further
below.

Using one or the other registration method only affects the probing process, the
run-time bridge-subdevice interaction is in both cases the same.

### 3.7.1.1. Registering synchronous sub-devices

In the **synchronous** case a device (bridge) driver needs to register the
[`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") with the v4l2\_device:

> [`v4l2_device_register_subdev`](v4l2-device.html#c.v4l2_device_register_subdev "v4l2_device_register_subdev")
> ([`v4l2_dev`](v4l2-device.html#c.v4l2_device "v4l2_device"), [`sd`](#c.v4l2_subdev "v4l2_subdev")).

This can fail if the subdev module disappeared before it could be registered.
After this function was called successfully the subdev->dev field points to
the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device").

If the v4l2\_device parent device has a non-NULL mdev field, the sub-device
entity will be automatically registered with the media device.

You can unregister a sub-device using:

> [`v4l2_device_unregister_subdev`](v4l2-device.html#c.v4l2_device_unregister_subdev "v4l2_device_unregister_subdev")
> ([`sd`](#c.v4l2_subdev "v4l2_subdev")).

Afterwards the subdev module can be unloaded and
[`sd`](#c.v4l2_subdev "v4l2_subdev")->dev == `NULL`.

### 3.7.1.2. Registering asynchronous sub-devices

In the **asynchronous** case subdevice probing can be invoked independently of
the bridge driver availability. The subdevice driver then has to verify whether
all the requirements for a successful probing are satisfied. This can include a
check for a master clock availability. If any of the conditions aren’t satisfied
the driver might decide to return `-EPROBE_DEFER` to request further reprobing
attempts. Once all conditions are met the subdevice shall be registered using
the [`v4l2_async_register_subdev()`](v4l2-async.html#c.v4l2_async_register_subdev "v4l2_async_register_subdev") function. Unregistration is
performed using the [`v4l2_async_unregister_subdev()`](v4l2-async.html#c.v4l2_async_unregister_subdev "v4l2_async_unregister_subdev") call. Subdevices
registered this way are stored in a global list of subdevices, ready to be
picked up by bridge drivers.

Drivers must complete all initialization of the sub-device before
registering it using [`v4l2_async_register_subdev()`](v4l2-async.html#c.v4l2_async_register_subdev "v4l2_async_register_subdev"), including
enabling runtime PM. This is because the sub-device becomes accessible
as soon as it gets registered.

### 3.7.1.3. Asynchronous sub-device notifiers

Bridge drivers in turn have to register a notifier object. This is performed
using the [`v4l2_async_nf_register()`](v4l2-async.html#c.v4l2_async_nf_register "v4l2_async_nf_register") call. To unregister the notifier the
driver has to call [`v4l2_async_nf_unregister()`](v4l2-async.html#c.v4l2_async_nf_unregister "v4l2_async_nf_unregister"). Before releasing memory
of an unregister notifier, it must be cleaned up by calling
[`v4l2_async_nf_cleanup()`](v4l2-async.html#c.v4l2_async_nf_cleanup "v4l2_async_nf_cleanup").

Before registering the notifier, bridge drivers must do two things: first, the
notifier must be initialized using the [`v4l2_async_nf_init()`](v4l2-async.html#c.v4l2_async_nf_init "v4l2_async_nf_init"). Second,
bridge drivers can then begin to form a list of async connection descriptors
that the bridge device needs for its
operation. [`v4l2_async_nf_add_fwnode()`](v4l2-async.html#c.v4l2_async_nf_add_fwnode "v4l2_async_nf_add_fwnode"),
[`v4l2_async_nf_add_fwnode_remote()`](v4l2-async.html#c.v4l2_async_nf_add_fwnode_remote "v4l2_async_nf_add_fwnode_remote") and [`v4l2_async_nf_add_i2c()`](v4l2-async.html#c.v4l2_async_nf_add_i2c "v4l2_async_nf_add_i2c")

Async connection descriptors describe connections to external sub-devices the
drivers for which are not yet probed. Based on an async connection, a media data
or ancillary link may be created when the related sub-device becomes
available. There may be one or more async connections to a given sub-device but
this is not known at the time of adding the connections to the notifier. Async
connections are bound as matching async sub-devices are found, one by one.

### 3.7.1.4. Asynchronous sub-device notifier for sub-devices

A driver that registers an asynchronous sub-device may also register an
asynchronous notifier. This is called an asynchronous sub-device notifier and the
process is similar to that of a bridge driver apart from that the notifier is
initialised using [`v4l2_async_subdev_nf_init()`](v4l2-async.html#c.v4l2_async_subdev_nf_init "v4l2_async_subdev_nf_init") instead. A sub-device
notifier may complete only after the V4L2 device becomes available, i.e. there’s
a path via async sub-devices and notifiers to a notifier that is not an
asynchronous sub-device notifier.

### 3.7.1.5. Asynchronous sub-device registration helper for camera sensor drivers

[`v4l2_async_register_subdev_sensor()`](v4l2-async.html#c.v4l2_async_register_subdev_sensor "v4l2_async_register_subdev_sensor") is a helper function for sensor
drivers registering their own async connection, but it also registers a notifier
and further registers async connections for lens and flash devices found in
firmware. The notifier for the sub-device is unregistered and cleaned up with
the async sub-device, using [`v4l2_async_unregister_subdev()`](v4l2-async.html#c.v4l2_async_unregister_subdev "v4l2_async_unregister_subdev").

### 3.7.1.6. Asynchronous sub-device notifier example

These functions allocate an async connection descriptor which is of type struct
[`v4l2_async_connection`](v4l2-async.html#c.v4l2_async_connection "v4l2_async_connection") embedded in a driver-specific struct. The &struct
[`v4l2_async_connection`](v4l2-async.html#c.v4l2_async_connection "v4l2_async_connection") shall be the first member of this struct:

```
struct my_async_connection {
        struct v4l2_async_connection asc;
        ...
};

struct my_async_connection *my_asc;
struct fwnode_handle *ep;

...

my_asc = v4l2_async_nf_add_fwnode_remote(&notifier, ep,
                                         struct my_async_connection);
fwnode_handle_put(ep);

if (IS_ERR(my_asc))
        return PTR_ERR(my_asc);
```

### 3.7.1.7. Asynchronous sub-device notifier callbacks

The V4L2 core will then use these connection descriptors to match asynchronously
registered subdevices to them. If a match is detected the `.bound()` notifier
callback is called. After all connections have been bound the .`complete()`
callback is called. When a connection is removed from the system the
`.unbind()` method is called. All three callbacks are optional.

Drivers can store any type of custom data in their driver-specific
[`v4l2_async_connection`](v4l2-async.html#c.v4l2_async_connection "v4l2_async_connection") wrapper. If any of that data requires special
handling when the structure is freed, drivers must implement the `.destroy()`
notifier callback. The framework will call it right before freeing the
[`v4l2_async_connection`](v4l2-async.html#c.v4l2_async_connection "v4l2_async_connection").

## 3.7.2. Calling subdev operations

The advantage of using [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") is that it is a generic `struct and`
does not contain any knowledge about the underlying hardware. So a driver might
contain several subdevs that use an I2C bus, but also a subdev that is
controlled through GPIO pins. This distinction is only relevant when setting
up the device, but once the subdev is registered it is completely transparent.

Once the subdev has been registered you can call an ops function either
directly:

```
err = sd->ops->core->g_std(sd, &norm);
```

but it is better and easier to use this macro:

```
err = v4l2_subdev_call(sd, core, g_std, &norm);
```

The macro will do the right `NULL` pointer checks and returns `-ENODEV`
if [`sd`](#c.v4l2_subdev "v4l2_subdev") is `NULL`, `-ENOIOCTLCMD` if either
[`sd`](#c.v4l2_subdev "v4l2_subdev")->core or [`sd`](#c.v4l2_subdev "v4l2_subdev")->core->g\_std is `NULL`, or the actual result of the
[`sd`](#c.v4l2_subdev "v4l2_subdev")->ops->core->g\_std ops.

It is also possible to call all or a subset of the sub-devices:

```
v4l2_device_call_all(v4l2_dev, 0, core, g_std, &norm);
```

Any subdev that does not support this ops is skipped and error results are
ignored. If you want to check for errors use this:

```
err = v4l2_device_call_until_err(v4l2_dev, 0, core, g_std, &norm);
```

Any error except `-ENOIOCTLCMD` will exit the loop with that error. If no
errors (except `-ENOIOCTLCMD`) occurred, then 0 is returned.

The second argument to both calls is a group ID. If 0, then all subdevs are
called. If non-zero, then only those whose group ID match that value will
be called. Before a bridge driver registers a subdev it can set
[`sd`](#c.v4l2_subdev "v4l2_subdev")->grp\_id to whatever value it wants (it’s 0 by
default). This value is owned by the bridge driver and the sub-device driver
will never modify or use it.

The group ID gives the bridge driver more control how callbacks are called.
For example, there may be multiple audio chips on a board, each capable of
changing the volume. But usually only one will actually be used when the
user want to change the volume. You can set the group ID for that subdev to
e.g. AUDIO\_CONTROLLER and specify that as the group ID value when calling
`v4l2_device_call_all()`. That ensures that it will only go to the subdev
that needs it.

If the sub-device needs to notify its v4l2\_device parent of an event, then
it can call `v4l2_subdev_notify(sd, notification, arg)`. This macro checks
whether there is a `notify()` callback defined and returns `-ENODEV` if not.
Otherwise the result of the `notify()` call is returned.

# 3.8. V4L2 sub-device userspace API

Bridge drivers traditionally expose one or multiple video nodes to userspace,
and control subdevices through the [`v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops") operations in
response to video node operations. This hides the complexity of the underlying
hardware from applications. For complex devices, finer-grained control of the
device than what the video nodes offer may be required. In those cases, bridge
drivers that implement [the media controller API](../../userspace-api/media/mediactl/media-controller.html#media-controller) may
opt for making the subdevice operations directly accessible from userspace.

Device nodes named `v4l-subdev`*X* can be created in `/dev` to access
sub-devices directly. If a sub-device supports direct userspace configuration
it must set the `V4L2_SUBDEV_FL_HAS_DEVNODE` flag before being registered.

After registering sub-devices, the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") driver can create
device nodes for all registered sub-devices marked with
`V4L2_SUBDEV_FL_HAS_DEVNODE` by calling
[`v4l2_device_register_subdev_nodes()`](v4l2-device.html#c.v4l2_device_register_subdev_nodes "v4l2_device_register_subdev_nodes"). Those device nodes will be
automatically removed when sub-devices are unregistered.

The device node handles a subset of the V4L2 API.

`VIDIOC_QUERYCTRL`,
`VIDIOC_QUERYMENU`,
`VIDIOC_G_CTRL`,
`VIDIOC_S_CTRL`,
`VIDIOC_G_EXT_CTRLS`,
`VIDIOC_S_EXT_CTRLS` and
`VIDIOC_TRY_EXT_CTRLS`:

> The controls ioctls are identical to the ones defined in V4L2. They
> behave identically, with the only exception that they deal only with
> controls implemented in the sub-device. Depending on the driver, those
> controls can be also be accessed through one (or several) V4L2 device
> nodes.

`VIDIOC_DQEVENT`,
`VIDIOC_SUBSCRIBE_EVENT` and
`VIDIOC_UNSUBSCRIBE_EVENT`

> The events ioctls are identical to the ones defined in V4L2. They
> behave identically, with the only exception that they deal only with
> events generated by the sub-device. Depending on the driver, those
> events can also be reported by one (or several) V4L2 device nodes.
>
> Sub-device drivers that want to use events need to set the
> `V4L2_SUBDEV_FL_HAS_EVENTS` [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev").flags before registering
> the sub-device. After registration events can be queued as usual on the
> [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev").devnode device node.
>
> To properly support events, the `poll()` file operation is also
> implemented.

Private ioctls

> All ioctls not in the above list are passed directly to the sub-device
> driver through the core::ioctl operation.

# 3.9. Read-only sub-device userspace API

Bridge drivers that control their connected subdevices through direct calls to
the kernel API realized by [`v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops") structure do not usually
want userspace to be able to change the same parameters through the subdevice
device node and thus do not usually register any.

It is sometimes useful to report to userspace the current subdevice
configuration through a read-only API, that does not permit applications to
change to the device parameters but allows interfacing to the subdevice device
node to inspect them.

For instance, to implement cameras based on computational photography, userspace
needs to know the detailed camera sensor configuration (in terms of skipping,
binning, cropping and scaling) for each supported output resolution. To support
such use cases, bridge drivers may expose the subdevice operations to userspace
through a read-only API.

To create a read-only device node for all the subdevices registered with the
`V4L2_SUBDEV_FL_HAS_DEVNODE` set, the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") driver should call
[`v4l2_device_register_ro_subdev_nodes()`](v4l2-device.html#c.v4l2_device_register_ro_subdev_nodes "v4l2_device_register_ro_subdev_nodes").

Access to the following ioctls for userspace applications is restricted on
sub-device device nodes registered with
[`v4l2_device_register_ro_subdev_nodes()`](v4l2-device.html#c.v4l2_device_register_ro_subdev_nodes "v4l2_device_register_ro_subdev_nodes").

`VIDIOC_SUBDEV_S_FMT`,
`VIDIOC_SUBDEV_S_CROP`,
`VIDIOC_SUBDEV_S_SELECTION`:

> These ioctls are only allowed on a read-only subdevice device node
> for the [V4L2\_SUBDEV\_FORMAT\_TRY](../../userspace-api/media/v4l/vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence)
> formats and selection rectangles.

`VIDIOC_SUBDEV_S_FRAME_INTERVAL`,
`VIDIOC_SUBDEV_S_DV_TIMINGS`,
`VIDIOC_SUBDEV_S_STD`:

> These ioctls are not allowed on a read-only subdevice node.

In case the ioctl is not allowed, or the format to modify is set to
`V4L2_SUBDEV_FORMAT_ACTIVE`, the core returns a negative error code and
the errno variable is set to `-EPERM`.

# 3.10. I2C sub-device drivers

Since these drivers are so common, special helper functions are available to
ease the use of these drivers (`v4l2-common.h`).

The recommended method of adding [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") support to an I2C driver
is to embed the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") `struct into` the state struct that is
created for each I2C device instance. Very simple devices have no state
`struct and` in that case you can just create a [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") directly.

A typical state `struct would` look like this (where ‘chipname’ is replaced by
the name of the chip):

```
struct chipname_state {
        struct v4l2_subdev sd;
        ...  /* additional state fields */
};
```

Initialize the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") `struct as` follows:

```
v4l2_i2c_subdev_init(&state->sd, client, subdev_ops);
```

This function will fill in all the fields of [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") ensure that
the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") and i2c\_client both point to one another.

You should also add a helper inline function to go from a [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")
pointer to a chipname\_state struct:

```
static inline struct chipname_state *to_state(struct v4l2_subdev *sd)
{
        return container_of(sd, struct chipname_state, sd);
}
```

Use this to go from the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") `struct to` the `i2c_client`
struct:

```
struct i2c_client *client = v4l2_get_subdevdata(sd);
```

And this to go from an `i2c_client` to a [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") struct:

```
struct v4l2_subdev *sd = i2c_get_clientdata(client);
```

Make sure to call
[`v4l2_device_unregister_subdev()`](v4l2-device.html#c.v4l2_device_unregister_subdev "v4l2_device_unregister_subdev")([`sd`](#c.v4l2_subdev "v4l2_subdev"))
when the `remove()` callback is called. This will unregister the sub-device
from the bridge driver. It is safe to call this even if the sub-device was
never registered.

You need to do this because when the bridge driver destroys the i2c adapter
the `remove()` callbacks are called of the i2c devices on that adapter.
After that the corresponding v4l2\_subdev structures are invalid, so they
have to be unregistered first. Calling
[`v4l2_device_unregister_subdev()`](v4l2-device.html#c.v4l2_device_unregister_subdev "v4l2_device_unregister_subdev")([`sd`](#c.v4l2_subdev "v4l2_subdev"))
from the `remove()` callback ensures that this is always done correctly.

The bridge driver also has some helper functions it can use:

```
struct v4l2_subdev *sd = v4l2_i2c_new_subdev(v4l2_dev, adapter,
                                "module_foo", "chipid", 0x36, NULL);
```

This loads the given module (can be `NULL` if no module needs to be loaded)
and calls [`i2c_new_client_device()`](../i2c.html#c.i2c_new_client_device "i2c_new_client_device") with the given `i2c_adapter` and
chip/address arguments. If all goes well, then it registers the subdev with
the v4l2\_device.

You can also use the last argument of [`v4l2_i2c_new_subdev()`](v4l2-common.html#c.v4l2_i2c_new_subdev "v4l2_i2c_new_subdev") to pass
an array of possible I2C addresses that it should probe. These probe addresses
are only used if the previous argument is 0. A non-zero argument means that you
know the exact i2c address so in that case no probing will take place.

Both functions return `NULL` if something went wrong.

Note that the chipid you pass to [`v4l2_i2c_new_subdev()`](v4l2-common.html#c.v4l2_i2c_new_subdev "v4l2_i2c_new_subdev") is usually
the same as the module name. It allows you to specify a chip variant, e.g.
“saa7114” or “saa7115”. In general though the i2c driver autodetects this.
The use of chipid is something that needs to be looked at more closely at a
later date. It differs between i2c drivers and as such can be confusing.
To see which chip variants are supported you can look in the i2c driver code
for the i2c\_device\_id table. This lists all the possibilities.

There are one more helper function:

[`v4l2_i2c_new_subdev_board()`](v4l2-common.html#c.v4l2_i2c_new_subdev_board "v4l2_i2c_new_subdev_board") uses an [`i2c_board_info`](../i2c.html#c.i2c_board_info "i2c_board_info") struct
which is passed to the i2c driver and replaces the irq, platform\_data and addr
arguments.

If the subdev supports the s\_config core ops, then that op is called with
the irq and platform\_data arguments after the subdev was setup.

The [`v4l2_i2c_new_subdev()`](v4l2-common.html#c.v4l2_i2c_new_subdev "v4l2_i2c_new_subdev") function will call
[`v4l2_i2c_new_subdev_board()`](v4l2-common.html#c.v4l2_i2c_new_subdev_board "v4l2_i2c_new_subdev_board"), internally filling a
[`i2c_board_info`](../i2c.html#c.i2c_board_info "i2c_board_info") structure using the `client_type` and the
`addr` to fill it.

# 3.11. Centrally managed subdev active state

Traditionally V4L2 subdev drivers maintained internal state for the active
device configuration. This is often implemented as e.g. an array of `struct
v4l2_mbus_framefmt`, one entry for each pad, and similarly for crop and compose
rectangles.

In addition to the active configuration, each subdev file handle has a [`struct
v4l2_subdev_state`](#c.v4l2_subdev_state "v4l2_subdev_state"), managed by the V4L2 core, which contains the try
configuration.

To simplify the subdev drivers the V4L2 subdev API now optionally supports a
centrally managed active configuration represented by
[`v4l2_subdev_state`](#c.v4l2_subdev_state "v4l2_subdev_state"). One instance of state, which contains the active
device configuration, is stored in the sub-device itself as part of
the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") structure, while the core associates a try state to
each open file handle, to store the try configuration related to that file
handle.

Sub-device drivers can opt-in and use state to manage their active configuration
by initializing the subdevice state with a call to [`v4l2_subdev_init_finalize()`](#c.v4l2_subdev_init_finalize "v4l2_subdev_init_finalize")
before registering the sub-device. They must also call [`v4l2_subdev_cleanup()`](#c.v4l2_subdev_cleanup "v4l2_subdev_cleanup")
to release all the allocated resources before unregistering the sub-device.
The core automatically allocates and initializes a state for each open file
handle to store the try configurations and frees it when closing the file
handle.

V4L2 sub-device operations that use both the [ACTIVE and TRY formats](../../userspace-api/media/v4l/vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence) receive the correct state to operate on through
the ‘state’ parameter. The state must be locked and unlocked by the
caller by calling [`v4l2_subdev_lock_state()`](#c.v4l2_subdev_lock_state "v4l2_subdev_lock_state") and
[`v4l2_subdev_unlock_state()`](#c.v4l2_subdev_unlock_state "v4l2_subdev_unlock_state"). The caller can do so by calling the subdev
operation through the [`v4l2_subdev_call_state_active()`](#c.v4l2_subdev_call_state_active "v4l2_subdev_call_state_active") macro.

Operations that do not receive a state parameter implicitly operate on the
subdevice active state, which drivers can exclusively access by
calling [`v4l2_subdev_lock_and_get_active_state()`](#c.v4l2_subdev_lock_and_get_active_state "v4l2_subdev_lock_and_get_active_state"). The sub-device active
state must equally be released by calling [`v4l2_subdev_unlock_state()`](#c.v4l2_subdev_unlock_state "v4l2_subdev_unlock_state").

Drivers must never manually access the state stored in the [`v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")
or in the file handle without going through the designated helpers.

While the V4L2 core passes the correct try or active state to the subdevice
operations, many existing device drivers pass a NULL state when calling
operations with [`v4l2_subdev_call()`](#c.v4l2_subdev_call "v4l2_subdev_call"). This legacy construct causes
issues with subdevice drivers that let the V4L2 core manage the active state,
as they expect to receive the appropriate state as a parameter. To help the
conversion of subdevice drivers to a managed active state without having to
convert all callers at the same time, an additional wrapper layer has been
added to [`v4l2_subdev_call()`](#c.v4l2_subdev_call "v4l2_subdev_call"), which handles the NULL case by getting and locking
the callee’s active state with [`v4l2_subdev_lock_and_get_active_state()`](#c.v4l2_subdev_lock_and_get_active_state "v4l2_subdev_lock_and_get_active_state"),
and unlocking the state after the call.

The whole subdev state is in reality split into three parts: the
v4l2\_subdev\_state, subdev controls and subdev driver’s internal state. In the
future these parts should be combined into a single state. For the time being
we need a way to handle the locking for these parts. This can be accomplished
by sharing a lock. The v4l2\_ctrl\_handler already supports this via its ‘lock’
pointer and the same model is used with states. The driver can do the following
before calling [`v4l2_subdev_init_finalize()`](#c.v4l2_subdev_init_finalize "v4l2_subdev_init_finalize"):

```
sd->ctrl_handler->lock = &priv->mutex;
sd->state_lock = &priv->mutex;
```

This shares the driver’s private mutex between the controls and the states.

# 3.12. Streams, multiplexed media pads and internal routing

A subdevice driver can implement support for multiplexed streams by setting
the V4L2\_SUBDEV\_FL\_STREAMS subdev flag and implementing support for
centrally managed subdev active state, routing and stream based
configuration.

# 3.13. V4L2 sub-device functions and data structures

struct v4l2\_decode\_vbi\_line
:   used to decode\_vbi\_line

> **Definition**:
>
> ```
> struct v4l2_decode_vbi_line {
>       u32 is_second_field;
>       u8 *p;
>       u32 line;
>       u32 type;
> };
> ```

**Members**

`is_second_field`
:   Set to 0 for the first (odd) field;
    set to 1 for the second (even) field.

`p`
:   Pointer to the sliced VBI data from the decoder. On exit, points to
    the start of the payload.

`line`
:   Line number of the sliced VBI data (1-23)

`type`
:   VBI service type (V4L2\_SLICED\_\*). 0 if no service found

enum v4l2\_subdev\_io\_pin\_bits
:   Subdevice external IO pin configuration bits

**Constants**

`V4L2_SUBDEV_IO_PIN_DISABLE`
:   disables a pin config. ENABLE assumed.

`V4L2_SUBDEV_IO_PIN_OUTPUT`
:   set it if pin is an output.

`V4L2_SUBDEV_IO_PIN_INPUT`
:   set it if pin is an input.

`V4L2_SUBDEV_IO_PIN_SET_VALUE`
:   to set the output value via
    [`struct v4l2_subdev_io_pin_config`](#c.v4l2_subdev_io_pin_config "v4l2_subdev_io_pin_config")->value.

`V4L2_SUBDEV_IO_PIN_ACTIVE_LOW`
:   pin active is bit 0.
    Otherwise, ACTIVE HIGH is assumed.

struct v4l2\_subdev\_io\_pin\_config
:   Subdevice external IO pin configuration

> **Definition**:
>
> ```
> struct v4l2_subdev_io_pin_config {
>       u32 flags;
>       u8 pin;
>       u8 function;
>       u8 value;
>       u8 strength;
> };
> ```

**Members**

`flags`
:   bitmask with flags for this pin’s config, whose bits are defined by
    [`enum v4l2_subdev_io_pin_bits`](#c.v4l2_subdev_io_pin_bits "v4l2_subdev_io_pin_bits").

`pin`
:   Chip external IO pin to configure

`function`
:   Internal signal pad/function to route to IO pin

`value`
:   Initial value for pin - e.g. GPIO output value

`strength`
:   Pin drive strength

struct v4l2\_subdev\_core\_ops
:   Define core ops callbacks for subdevs

> **Definition**:
>
> ```
> struct v4l2_subdev_core_ops {
>       int (*log_status)(struct v4l2_subdev *sd);
>       int (*s_io_pin_config)(struct v4l2_subdev *sd, size_t n, struct v4l2_subdev_io_pin_config *pincfg);
>       int (*init)(struct v4l2_subdev *sd, u32 val);
>       int (*load_fw)(struct v4l2_subdev *sd);
>       int (*reset)(struct v4l2_subdev *sd, u32 val);
>       int (*s_gpio)(struct v4l2_subdev *sd, u32 val);
>       long (*command)(struct v4l2_subdev *sd, unsigned int cmd, void *arg);
>       long (*ioctl)(struct v4l2_subdev *sd, unsigned int cmd, void *arg);
> #ifdef CONFIG_COMPAT;
>       long (*compat_ioctl32)(struct v4l2_subdev *sd, unsigned int cmd, unsigned long arg);
> #endif;
> #ifdef CONFIG_VIDEO_ADV_DEBUG;
>       int (*g_register)(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg);
>       int (*s_register)(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg);
> #endif;
>       int (*s_power)(struct v4l2_subdev *sd, int on);
>       int (*interrupt_service_routine)(struct v4l2_subdev *sd, u32 status, bool *handled);
>       int (*subscribe_event)(struct v4l2_subdev *sd, struct v4l2_fh *fh, struct v4l2_event_subscription *sub);
>       int (*unsubscribe_event)(struct v4l2_subdev *sd, struct v4l2_fh *fh, struct v4l2_event_subscription *sub);
> };
> ```

**Members**

`log_status`
:   callback for `VIDIOC_LOG_STATUS()` ioctl handler code.

`s_io_pin_config`
:   configure one or more chip I/O pins for chips that
    multiplex different internal signal pads out to IO pins. This function
    takes a pointer to an array of ‘n’ pin configuration entries, one for
    each pin being configured. This function could be called at times
    other than just subdevice initialization.

`init`
:   initialize the sensor registers to some sort of reasonable default
    values. Do not use for new drivers and should be removed in existing
    drivers.

`load_fw`
:   load firmware.

`reset`
:   generic reset command. The argument selects which subsystems to
    reset. Passing 0 will always reset the whole chip. Do not use for new
    drivers without discussing this first on the linux-media mailinglist.
    There should be no reason normally to reset a device.

`s_gpio`
:   set GPIO pins. Very simple right now, might need to be extended with
    a direction argument if needed.

`command`
:   called by in-kernel drivers in order to call functions internal
    to subdev drivers driver that have a separate callback.

`ioctl`
:   called at the end of ioctl() syscall handler at the V4L2 core.
    used to provide support for private ioctls used on the driver.

`compat_ioctl32`
:   called when a 32 bits application uses a 64 bits Kernel,
    in order to fix data passed from/to userspace.

`g_register`
:   callback for `VIDIOC_DBG_G_REGISTER()` ioctl handler code.

`s_register`
:   callback for `VIDIOC_DBG_S_REGISTER()` ioctl handler code.

`s_power`
:   puts subdevice in power saving mode (on == 0) or normal operation
    mode (on == 1). DEPRECATED. See
    [Writing camera sensor drivers](camera-sensor.html) . pre\_streamon and
    post\_streamoff callbacks can be used for e.g. setting the bus to LP-11
    mode before s\_stream is called.

`interrupt_service_routine`
:   Called by the bridge chip’s interrupt service
    handler, when an interrupt status has be raised due to this subdev,
    so that this subdev can handle the details. It may schedule work to be
    performed later. It must not sleep. **Called from an IRQ context**.

`subscribe_event`
:   used by the drivers to request the control framework that
    for it to be warned when the value of a control changes.

`unsubscribe_event`
:   remove event subscription from the control framework.

struct v4l2\_subdev\_tuner\_ops
:   Callbacks used when v4l device was opened in radio mode.

> **Definition**:
>
> ```
> struct v4l2_subdev_tuner_ops {
>       int (*standby)(struct v4l2_subdev *sd);
>       int (*s_radio)(struct v4l2_subdev *sd);
>       int (*s_frequency)(struct v4l2_subdev *sd, const struct v4l2_frequency *freq);
>       int (*g_frequency)(struct v4l2_subdev *sd, struct v4l2_frequency *freq);
>       int (*enum_freq_bands)(struct v4l2_subdev *sd, struct v4l2_frequency_band *band);
>       int (*g_tuner)(struct v4l2_subdev *sd, struct v4l2_tuner *vt);
>       int (*s_tuner)(struct v4l2_subdev *sd, const struct v4l2_tuner *vt);
>       int (*g_modulator)(struct v4l2_subdev *sd, struct v4l2_modulator *vm);
>       int (*s_modulator)(struct v4l2_subdev *sd, const struct v4l2_modulator *vm);
>       int (*s_type_addr)(struct v4l2_subdev *sd, struct tuner_setup *type);
>       int (*s_config)(struct v4l2_subdev *sd, const struct v4l2_priv_tun_config *config);
> };
> ```

**Members**

`standby`
:   puts the tuner in standby mode. It will be woken up
    automatically the next time it is used.

`s_radio`
:   callback that switches the tuner to radio mode.
    drivers should explicitly call it when a tuner ops should
    operate on radio mode, before being able to handle it.
    Used on devices that have both AM/FM radio receiver and TV.

`s_frequency`
:   callback for `VIDIOC_S_FREQUENCY()` ioctl handler code.

`g_frequency`
:   callback for `VIDIOC_G_FREQUENCY()` ioctl handler code.
    freq->type must be filled in. Normally done by [`video_ioctl2()`](v4l2-common.html#c.video_ioctl2 "video_ioctl2")
    or the bridge driver.

`enum_freq_bands`
:   callback for `VIDIOC_ENUM_FREQ_BANDS()` ioctl handler code.

`g_tuner`
:   callback for `VIDIOC_G_TUNER()` ioctl handler code.

`s_tuner`
:   callback for `VIDIOC_S_TUNER()` ioctl handler code. **vt->type** must be
    filled in. Normally done by video\_ioctl2 or the
    bridge driver.

`g_modulator`
:   callback for `VIDIOC_G_MODULATOR()` ioctl handler code.

`s_modulator`
:   callback for `VIDIOC_S_MODULATOR()` ioctl handler code.

`s_type_addr`
:   sets tuner type and its I2C addr.

`s_config`
:   sets tda9887 specific stuff, like port1, port2 and qss

**Description**

Note

On devices that have both AM/FM and TV, it is up to the driver
to explicitly call s\_radio when the tuner should be switched to
radio mode, before handling other [`struct v4l2_subdev_tuner_ops`](#c.v4l2_subdev_tuner_ops "v4l2_subdev_tuner_ops")
that would require it. An example of such usage is:

```
static void s_frequency(void *priv, const struct v4l2_frequency *f)
{
      ...
      if (f.type == V4L2_TUNER_RADIO)
              v4l2_device_call_all(v4l2_dev, 0, tuner, s_radio);
      ...
      v4l2_device_call_all(v4l2_dev, 0, tuner, s_frequency);
}
```

struct v4l2\_subdev\_audio\_ops
:   Callbacks used for audio-related settings

> **Definition**:
>
> ```
> struct v4l2_subdev_audio_ops {
>       int (*s_clock_freq)(struct v4l2_subdev *sd, u32 freq);
>       int (*s_i2s_clock_freq)(struct v4l2_subdev *sd, u32 freq);
>       int (*s_routing)(struct v4l2_subdev *sd, u32 input, u32 output, u32 config);
>       int (*s_stream)(struct v4l2_subdev *sd, int enable);
> };
> ```

**Members**

`s_clock_freq`
:   set the frequency (in Hz) of the audio clock output.
    Used to slave an audio processor to the video decoder, ensuring that
    audio and video remain synchronized. Usual values for the frequency
    are 48000, 44100 or 32000 Hz. If the frequency is not supported, then
    -EINVAL is returned.

`s_i2s_clock_freq`
:   sets I2S speed in bps. This is used to provide a standard
    way to select I2S clock used by driving digital audio streams at some
    board designs. Usual values for the frequency are 1024000 and 2048000.
    If the frequency is not supported, then `-EINVAL` is returned.

`s_routing`
:   used to define the input and/or output pins of an audio chip,
    and any additional configuration data.
    Never attempt to use user-level input IDs (e.g. Composite, S-Video,
    Tuner) at this level. An i2c device shouldn’t know about whether an
    input pin is connected to a Composite connector, become on another
    board or platform it might be connected to something else entirely.
    The calling driver is responsible for mapping a user-level input to
    the right pins on the i2c device.

`s_stream`
:   used to notify the audio code that stream will start or has
    stopped.

struct v4l2\_mbus\_frame\_desc\_entry\_csi2

> **Definition**:
>
> ```
> struct v4l2_mbus_frame_desc_entry_csi2 {
>       u8 vc;
>       u8 dt;
> };
> ```

**Members**

`vc`
:   CSI-2 virtual channel

`dt`
:   CSI-2 data type ID

enum v4l2\_mbus\_frame\_desc\_flags
:   media bus frame description flags

**Constants**

`V4L2_MBUS_FRAME_DESC_FL_LEN_MAX`
:   Indicates that [`struct v4l2_mbus_frame_desc_entry`](#c.v4l2_mbus_frame_desc_entry "v4l2_mbus_frame_desc_entry")->length field
    specifies maximum data length.

`V4L2_MBUS_FRAME_DESC_FL_BLOB`
:   Indicates that the format does not have line offsets, i.e.
    the receiver should use 1D DMA.

struct v4l2\_mbus\_frame\_desc\_entry
:   media bus frame description structure

> **Definition**:
>
> ```
> struct v4l2_mbus_frame_desc_entry {
>       enum v4l2_mbus_frame_desc_flags flags;
>       u32 stream;
>       u32 pixelcode;
>       u32 length;
>       union {
>             struct v4l2_mbus_frame_desc_entry_csi2 csi2;
>       } bus;
> };
> ```

**Members**

`flags`
:   bitmask flags, as defined by [`enum v4l2_mbus_frame_desc_flags`](#c.v4l2_mbus_frame_desc_flags "v4l2_mbus_frame_desc_flags").

`stream`
:   stream in routing configuration

`pixelcode`
:   media bus pixel code, valid if **flags**
    `FRAME_DESC_FL_BLOB` is not set.

`length`
:   number of octets per frame, valid if **flags**
    `V4L2_MBUS_FRAME_DESC_FL_LEN_MAX` is set.

`bus`
:   Bus-specific frame descriptor parameters

`bus.csi2`
:   CSI-2-specific bus configuration

enum v4l2\_mbus\_frame\_desc\_type
:   media bus frame description type

**Constants**

`V4L2_MBUS_FRAME_DESC_TYPE_UNDEFINED`
:   Undefined frame desc type. Drivers should not use this, it is
    for backwards compatibility.

`V4L2_MBUS_FRAME_DESC_TYPE_PARALLEL`
:   Parallel media bus.

`V4L2_MBUS_FRAME_DESC_TYPE_CSI2`
:   CSI-2 media bus. Frame desc parameters must be set in
    [`struct v4l2_mbus_frame_desc_entry`](#c.v4l2_mbus_frame_desc_entry "v4l2_mbus_frame_desc_entry")->csi2.

struct v4l2\_mbus\_frame\_desc
:   media bus data frame description

> **Definition**:
>
> ```
> struct v4l2_mbus_frame_desc {
>       enum v4l2_mbus_frame_desc_type type;
>       struct v4l2_mbus_frame_desc_entry entry[V4L2_FRAME_DESC_ENTRY_MAX];
>       unsigned short num_entries;
> };
> ```

**Members**

`type`
:   type of the bus ([`enum v4l2_mbus_frame_desc_type`](#c.v4l2_mbus_frame_desc_type "v4l2_mbus_frame_desc_type"))

`entry`
:   frame descriptors array

`num_entries`
:   number of entries in **entry** array

enum v4l2\_subdev\_pre\_streamon\_flags
:   Flags for pre\_streamon subdev core op

**Constants**

`V4L2_SUBDEV_PRE_STREAMON_FL_MANUAL_LP`
:   Set the transmitter to either LP-11
    or LP-111 mode before call to `s_stream()`.

struct v4l2\_subdev\_video\_ops
:   Callbacks used when v4l device was opened in video mode.

> **Definition**:
>
> ```
> struct v4l2_subdev_video_ops {
>       int (*s_routing)(struct v4l2_subdev *sd, u32 input, u32 output, u32 config);
>       int (*s_crystal_freq)(struct v4l2_subdev *sd, u32 freq, u32 flags);
>       int (*g_std)(struct v4l2_subdev *sd, v4l2_std_id *norm);
>       int (*s_std)(struct v4l2_subdev *sd, v4l2_std_id norm);
>       int (*s_std_output)(struct v4l2_subdev *sd, v4l2_std_id std);
>       int (*g_std_output)(struct v4l2_subdev *sd, v4l2_std_id *std);
>       int (*querystd)(struct v4l2_subdev *sd, v4l2_std_id *std);
>       int (*g_tvnorms)(struct v4l2_subdev *sd, v4l2_std_id *std);
>       int (*g_tvnorms_output)(struct v4l2_subdev *sd, v4l2_std_id *std);
>       int (*g_input_status)(struct v4l2_subdev *sd, u32 *status);
>       int (*s_stream)(struct v4l2_subdev *sd, int enable);
>       int (*s_rx_buffer)(struct v4l2_subdev *sd, void *buf, unsigned int *size);
>       int (*pre_streamon)(struct v4l2_subdev *sd, u32 flags);
>       int (*post_streamoff)(struct v4l2_subdev *sd);
> };
> ```

**Members**

`s_routing`
:   see s\_routing in audio\_ops, except this version is for video
    devices.

`s_crystal_freq`
:   sets the frequency of the crystal used to generate the
    clocks in Hz. An extra flags field allows device specific configuration
    regarding clock frequency dividers, etc. If not used, then set flags
    to 0. If the frequency is not supported, then -EINVAL is returned.

`g_std`
:   callback for `VIDIOC_G_STD()` ioctl handler code.

`s_std`
:   callback for `VIDIOC_S_STD()` ioctl handler code.

`s_std_output`
:   set v4l2\_std\_id for video OUTPUT devices. This is ignored by
    video input devices.

`g_std_output`
:   get current standard for video OUTPUT devices. This is ignored
    by video input devices.

`querystd`
:   callback for `VIDIOC_QUERYSTD()` ioctl handler code.

`g_tvnorms`
:   get `v4l2_std_id` with all standards supported by the video
    CAPTURE device. This is ignored by video output devices.

`g_tvnorms_output`
:   get v4l2\_std\_id with all standards supported by the video
    OUTPUT device. This is ignored by video capture devices.

`g_input_status`
:   get input status. Same as the status field in the
    `struct v4l2_input`

`s_stream`
:   start (enabled == 1) or stop (enabled == 0) streaming on the
    sub-device. Failure on stop will remove any resources acquired in
    streaming start, while the error code is still returned by the driver.
    The caller shall track the subdev state, and shall not start or stop an
    already started or stopped subdev. Also see call\_s\_stream wrapper in
    v4l2-subdev.c.

`s_rx_buffer`
:   set a host allocated memory buffer for the subdev. The subdev
    can adjust **size** to a lower value and must not write more data to the
    buffer starting at **data** than the original value of **size**.

`pre_streamon`
:   May be called before streaming is actually started, to help
    initialising the bus. Current usage is to set a CSI-2 transmitter to
    LP-11 or LP-111 mode before streaming. See [`enum
    v4l2_subdev_pre_streamon_flags`](#c.v4l2_subdev_pre_streamon_flags "v4l2_subdev_pre_streamon_flags").

`post_streamoff`
:   Called after streaming is stopped, but if and only if
    pre\_streamon was called earlier.

**Description**

> This callback is DEPRECATED. New drivers should instead implement
> [`v4l2_subdev_pad_ops.enable_streams`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops") and
> [`v4l2_subdev_pad_ops.disable_streams`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops") operations, and use
> v4l2\_subdev\_s\_stream\_helper for the [`v4l2_subdev_video_ops.s_stream`](#c.v4l2_subdev_video_ops "v4l2_subdev_video_ops")
> operation to support legacy users.
>
> Drivers should also not call the .`s_stream()` subdev operation directly,
> but use the [`v4l2_subdev_enable_streams()`](#c.v4l2_subdev_enable_streams "v4l2_subdev_enable_streams") and
> [`v4l2_subdev_disable_streams()`](#c.v4l2_subdev_disable_streams "v4l2_subdev_disable_streams") helpers.
>
> pre\_streamon shall return error if it cannot perform the operation as
> indicated by the flags argument. In particular, -EACCES indicates lack
> of support for the operation. The caller shall call post\_streamoff for
> each successful call of pre\_streamon.

struct v4l2\_subdev\_vbi\_ops
:   Callbacks used when v4l device was opened in video mode via the vbi device node.

> **Definition**:
>
> ```
> struct v4l2_subdev_vbi_ops {
>       int (*decode_vbi_line)(struct v4l2_subdev *sd, struct v4l2_decode_vbi_line *vbi_line);
>       int (*s_vbi_data)(struct v4l2_subdev *sd, const struct v4l2_sliced_vbi_data *vbi_data);
>       int (*g_vbi_data)(struct v4l2_subdev *sd, struct v4l2_sliced_vbi_data *vbi_data);
>       int (*g_sliced_vbi_cap)(struct v4l2_subdev *sd, struct v4l2_sliced_vbi_cap *cap);
>       int (*s_raw_fmt)(struct v4l2_subdev *sd, struct v4l2_vbi_format *fmt);
>       int (*g_sliced_fmt)(struct v4l2_subdev *sd, struct v4l2_sliced_vbi_format *fmt);
>       int (*s_sliced_fmt)(struct v4l2_subdev *sd, struct v4l2_sliced_vbi_format *fmt);
> };
> ```

**Members**

`decode_vbi_line`
:   video decoders that support sliced VBI need to implement
    this ioctl. Field p of the [`struct v4l2_decode_vbi_line`](#c.v4l2_decode_vbi_line "v4l2_decode_vbi_line") is set to the
    start of the VBI data that was generated by the decoder. The driver
    then parses the sliced VBI data and sets the other fields in the
    `struct accordingly`. The pointer p is updated to point to the start of
    the payload which can be copied verbatim into the data field of the
    `struct v4l2_sliced_vbi_data`. If no valid VBI data was found, then the
    type field is set to 0 on return.

`s_vbi_data`
:   used to generate VBI signals on a video signal.
    `struct v4l2_sliced_vbi_data` is filled with the data packets that
    should be output. Note that if you set the line field to 0, then that
    VBI signal is disabled. If no valid VBI data was found, then the type
    field is set to 0 on return.

`g_vbi_data`
:   used to obtain the sliced VBI packet from a readback register.
    Not all video decoders support this. If no data is available because
    the readback register contains invalid or erroneous data `-EIO` is
    returned. Note that you must fill in the ‘id’ member and the ‘field’
    member (to determine whether CC data from the first or second field
    should be obtained).

`g_sliced_vbi_cap`
:   callback for `VIDIOC_G_SLICED_VBI_CAP()` ioctl handler
    code.

`s_raw_fmt`
:   setup the video encoder/decoder for raw VBI.

`g_sliced_fmt`
:   retrieve the current sliced VBI settings.

`s_sliced_fmt`
:   setup the sliced VBI settings.

struct v4l2\_subdev\_sensor\_ops
:   v4l2-subdev sensor operations

> **Definition**:
>
> ```
> struct v4l2_subdev_sensor_ops {
>       int (*g_skip_top_lines)(struct v4l2_subdev *sd, u32 *lines);
>       int (*g_skip_frames)(struct v4l2_subdev *sd, u32 *frames);
> };
> ```

**Members**

`g_skip_top_lines`
:   number of lines at the top of the image to be skipped.
    This is needed for some sensors, which always corrupt
    several top lines of the output image, or which send their
    metadata in them.

`g_skip_frames`
:   number of frames to skip at stream start. This is needed for
    buggy sensors that generate faulty frames when they are
    turned on.

enum v4l2\_subdev\_ir\_mode
:   describes the type of IR supported

**Constants**

`V4L2_SUBDEV_IR_MODE_PULSE_WIDTH`
:   IR uses `struct ir_raw_event` records

struct v4l2\_subdev\_ir\_parameters
:   Parameters for IR TX or TX

> **Definition**:
>
> ```
> struct v4l2_subdev_ir_parameters {
>       unsigned int bytes_per_data_element;
>       enum v4l2_subdev_ir_mode mode;
>       bool enable;
>       bool interrupt_enable;
>       bool shutdown;
>       bool modulation;
>       u32 max_pulse_width;
>       unsigned int carrier_freq;
>       unsigned int duty_cycle;
>       bool invert_level;
>       bool invert_carrier_sense;
>       u32 noise_filter_min_width;
>       unsigned int carrier_range_lower;
>       unsigned int carrier_range_upper;
>       u32 resolution;
> };
> ```

**Members**

`bytes_per_data_element`
:   bytes per data element of data in read or
    write call.

`mode`
:   IR mode as defined by [`enum v4l2_subdev_ir_mode`](#c.v4l2_subdev_ir_mode "v4l2_subdev_ir_mode").

`enable`
:   device is active if true

`interrupt_enable`
:   IR interrupts are enabled if true

`shutdown`
:   if true: set hardware to low/no power, false: normal mode

`modulation`
:   if true, it uses carrier, if false: baseband

`max_pulse_width`
:   maximum pulse width in ns, valid only for baseband signal

`carrier_freq`
:   carrier frequency in Hz, valid only for modulated signal

`duty_cycle`
:   duty cycle percentage, valid only for modulated signal

`invert_level`
:   invert signal level

`invert_carrier_sense`
:   Send 0/space as a carrier burst. used only in TX.

`noise_filter_min_width`
:   min time of a valid pulse, in ns. Used only for RX.

`carrier_range_lower`
:   Lower carrier range, in Hz, valid only for modulated
    signal. Used only for RX.

`carrier_range_upper`
:   Upper carrier range, in Hz, valid only for modulated
    signal. Used only for RX.

`resolution`
:   The receive resolution, in ns . Used only for RX.

struct v4l2\_subdev\_ir\_ops
:   operations for IR subdevices

> **Definition**:
>
> ```
> struct v4l2_subdev_ir_ops {
>       int (*rx_read)(struct v4l2_subdev *sd, u8 *buf, size_t count, ssize_t *num);
>       int (*rx_g_parameters)(struct v4l2_subdev *sd, struct v4l2_subdev_ir_parameters *params);
>       int (*rx_s_parameters)(struct v4l2_subdev *sd, struct v4l2_subdev_ir_parameters *params);
>       int (*tx_write)(struct v4l2_subdev *sd, u8 *buf, size_t count, ssize_t *num);
>       int (*tx_g_parameters)(struct v4l2_subdev *sd, struct v4l2_subdev_ir_parameters *params);
>       int (*tx_s_parameters)(struct v4l2_subdev *sd, struct v4l2_subdev_ir_parameters *params);
> };
> ```

**Members**

`rx_read`
:   Reads received codes or pulse width data.
    The semantics are similar to a non-blocking read() call.

`rx_g_parameters`
:   Get the current operating parameters and state of
    the IR receiver.

`rx_s_parameters`
:   Set the current operating parameters and state of
    the IR receiver. It is recommended to call
    [rt]x\_g\_parameters first to fill out the current state, and only change
    the fields that need to be changed. Upon return, the actual device
    operating parameters and state will be returned. Note that hardware
    limitations may prevent the actual settings from matching the requested
    settings - e.g. an actual carrier setting of 35,904 Hz when 36,000 Hz
    was requested. An exception is when the shutdown parameter is true.
    The last used operational parameters will be returned, but the actual
    state of the hardware be different to minimize power consumption and
    processing when shutdown is true.

`tx_write`
:   Writes codes or pulse width data for transmission.
    The semantics are similar to a non-blocking write() call.

`tx_g_parameters`
:   Get the current operating parameters and state of
    the IR transmitter.

`tx_s_parameters`
:   Set the current operating parameters and state of
    the IR transmitter. It is recommended to call
    [rt]x\_g\_parameters first to fill out the current state, and only change
    the fields that need to be changed. Upon return, the actual device
    operating parameters and state will be returned. Note that hardware
    limitations may prevent the actual settings from matching the requested
    settings - e.g. an actual carrier setting of 35,904 Hz when 36,000 Hz
    was requested. An exception is when the shutdown parameter is true.
    The last used operational parameters will be returned, but the actual
    state of the hardware be different to minimize power consumption and
    processing when shutdown is true.

struct v4l2\_subdev\_pad\_config
:   Used for storing subdev pad information.

> **Definition**:
>
> ```
> struct v4l2_subdev_pad_config {
>       struct v4l2_mbus_framefmt format;
>       struct v4l2_rect crop;
>       struct v4l2_rect compose;
>       struct v4l2_fract interval;
> };
> ```

**Members**

`format`
:   `struct v4l2_mbus_framefmt`

`crop`
:   `struct v4l2_rect` to be used for crop

`compose`
:   `struct v4l2_rect` to be used for compose

`interval`
:   frame interval

struct v4l2\_subdev\_stream\_configs
:   A collection of stream configs.

> **Definition**:
>
> ```
> struct v4l2_subdev_stream_configs {
>       u32 num_configs;
>       struct v4l2_subdev_stream_config *configs;
> };
> ```

**Members**

`num_configs`
:   number of entries in **config**.

`configs`
:   an array of [`struct v4l2_subdev_stream_configs`](#c.v4l2_subdev_stream_configs "v4l2_subdev_stream_configs").

struct v4l2\_subdev\_krouting
:   subdev routing table

> **Definition**:
>
> ```
> struct v4l2_subdev_krouting {
>       unsigned int len_routes;
>       unsigned int num_routes;
>       struct v4l2_subdev_route *routes;
> };
> ```

**Members**

`len_routes`
:   length of routes array, in routes

`num_routes`
:   number of routes

`routes`
:   `struct v4l2_subdev_route`

**Description**

This structure contains the routing table for a subdev.

struct v4l2\_subdev\_state
:   Used for storing subdev state information.

> **Definition**:
>
> ```
> struct v4l2_subdev_state {
>       struct mutex _lock;
>       struct mutex *lock;
>       struct v4l2_subdev *sd;
>       struct v4l2_subdev_pad_config *pads;
>       struct v4l2_subdev_krouting routing;
>       struct v4l2_subdev_stream_configs stream_configs;
> };
> ```

**Members**

`_lock`
:   default for ‘lock’

`lock`
:   mutex for the state. May be replaced by the user.

`sd`
:   the sub-device which the state is related to

`pads`
:   [`struct v4l2_subdev_pad_config`](#c.v4l2_subdev_pad_config "v4l2_subdev_pad_config") array

`routing`
:   routing table for the subdev

`stream_configs`
:   stream configurations (only for V4L2\_SUBDEV\_FL\_STREAMS)

**Description**

This structure only needs to be passed to the pad op if the ‘which’ field
of the main argument is set to `V4L2_SUBDEV_FORMAT_TRY`. For
`V4L2_SUBDEV_FORMAT_ACTIVE` it is safe to pass `NULL`.

struct v4l2\_subdev\_pad\_ops
:   v4l2-subdev pad level operations

> **Definition**:
>
> ```
> struct v4l2_subdev_pad_ops {
>       int (*enum_mbus_code)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_mbus_code_enum *code);
>       int (*enum_frame_size)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_size_enum *fse);
>       int (*enum_frame_interval)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_interval_enum *fie);
>       int (*get_fmt)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *format);
>       int (*set_fmt)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *format);
>       int (*get_selection)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_selection *sel);
>       int (*set_selection)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_selection *sel);
>       int (*get_frame_interval)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_interval *interval);
>       int (*set_frame_interval)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_interval *interval);
>       int (*get_edid)(struct v4l2_subdev *sd, struct v4l2_edid *edid);
>       int (*set_edid)(struct v4l2_subdev *sd, struct v4l2_edid *edid);
>       int (*s_dv_timings)(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_dv_timings *timings);
>       int (*g_dv_timings)(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_dv_timings *timings);
>       int (*query_dv_timings)(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_dv_timings *timings);
>       int (*dv_timings_cap)(struct v4l2_subdev *sd, struct v4l2_dv_timings_cap *cap);
>       int (*enum_dv_timings)(struct v4l2_subdev *sd, struct v4l2_enum_dv_timings *timings);
> #ifdef CONFIG_MEDIA_CONTROLLER;
>       int (*link_validate)(struct v4l2_subdev *sd, struct media_link *link, struct v4l2_subdev_format *source_fmt, struct v4l2_subdev_format *sink_fmt);
> #endif ;
>       int (*get_frame_desc)(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_mbus_frame_desc *fd);
>       int (*set_frame_desc)(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_mbus_frame_desc *fd);
>       int (*get_mbus_config)(struct v4l2_subdev *sd, unsigned int pad, struct v4l2_mbus_config *config);
>       int (*set_routing)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, enum v4l2_subdev_format_whence which, struct v4l2_subdev_krouting *route);
>       int (*enable_streams)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, u32 pad, u64 streams_mask);
>       int (*disable_streams)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, u32 pad, u64 streams_mask);
> };
> ```

**Members**

`enum_mbus_code`
:   callback for `VIDIOC_SUBDEV_ENUM_MBUS_CODE()` ioctl handler
    code.

`enum_frame_size`
:   callback for `VIDIOC_SUBDEV_ENUM_FRAME_SIZE()` ioctl handler
    code.

`enum_frame_interval`
:   callback for `VIDIOC_SUBDEV_ENUM_FRAME_INTERVAL()` ioctl
    handler code.

`get_fmt`
:   callback for `VIDIOC_SUBDEV_G_FMT()` ioctl handler code.

`set_fmt`
:   callback for `VIDIOC_SUBDEV_S_FMT()` ioctl handler code.

`get_selection`
:   callback for `VIDIOC_SUBDEV_G_SELECTION()` ioctl handler code.

`set_selection`
:   callback for `VIDIOC_SUBDEV_S_SELECTION()` ioctl handler code.

`get_frame_interval`
:   callback for `VIDIOC_SUBDEV_G_FRAME_INTERVAL()`
    ioctl handler code.

`set_frame_interval`
:   callback for `VIDIOC_SUBDEV_S_FRAME_INTERVAL()`
    ioctl handler code.

`get_edid`
:   callback for `VIDIOC_SUBDEV_G_EDID()` ioctl handler code.

`set_edid`
:   callback for `VIDIOC_SUBDEV_S_EDID()` ioctl handler code.

`s_dv_timings`
:   Set custom dv timings in the sub device. This is used
    when sub device is capable of setting detailed timing information
    in the hardware to generate/detect the video signal.

`g_dv_timings`
:   Get custom dv timings in the sub device.

`query_dv_timings`
:   callback for `VIDIOC_QUERY_DV_TIMINGS()` ioctl handler code.

`dv_timings_cap`
:   callback for `VIDIOC_SUBDEV_DV_TIMINGS_CAP()` ioctl handler
    code.

`enum_dv_timings`
:   callback for `VIDIOC_SUBDEV_ENUM_DV_TIMINGS()` ioctl handler
    code.

`link_validate`
:   used by the media controller code to check if the links
    that belongs to a pipeline can be used for stream.

`get_frame_desc`
:   get the current low level media bus frame parameters.

`set_frame_desc`
:   set the low level media bus frame parameters, **fd** array
    may be adjusted by the subdev driver to device capabilities.

`get_mbus_config`
:   get the media bus configuration of a remote sub-device.
    The media bus configuration is usually retrieved from the
    firmware interface at sub-device probe time, immediately
    applied to the hardware and eventually adjusted by the
    driver. Remote sub-devices (usually video receivers) shall
    use this operation to query the transmitting end bus
    configuration in order to adjust their own one accordingly.
    Callers should make sure they get the most up-to-date as
    possible configuration from the remote end, likely calling
    this operation as close as possible to stream on time. The
    operation shall fail if the pad index it has been called on
    is not valid or in case of unrecoverable failures. The
    config argument has been memset to 0 just before calling
    the op.

`set_routing`
:   Enable or disable data connection routes described in the
    subdevice routing table. Subdevs that implement this operation
    must set the V4L2\_SUBDEV\_FL\_STREAMS flag.

`enable_streams`
:   Enable the streams defined in streams\_mask on the given
    source pad. Subdevs that implement this operation must use the active
    state management provided by the subdev core (enabled through a call to
    [`v4l2_subdev_init_finalize()`](#c.v4l2_subdev_init_finalize "v4l2_subdev_init_finalize") at initialization time). Do not call
    directly, use [`v4l2_subdev_enable_streams()`](#c.v4l2_subdev_enable_streams "v4l2_subdev_enable_streams") instead.

`disable_streams`
:   Disable the streams defined in streams\_mask on the given
    source pad. Subdevs that implement this operation must use the active
    state management provided by the subdev core (enabled through a call to
    [`v4l2_subdev_init_finalize()`](#c.v4l2_subdev_init_finalize "v4l2_subdev_init_finalize") at initialization time). Do not call
    directly, use [`v4l2_subdev_disable_streams()`](#c.v4l2_subdev_disable_streams "v4l2_subdev_disable_streams") instead.

**Description**

> Drivers that support only a single stream without setting the
> V4L2\_SUBDEV\_CAP\_STREAMS sub-device capability flag can ignore the mask
> argument.
>
> Drivers that support only a single stream without setting the
> V4L2\_SUBDEV\_CAP\_STREAMS sub-device capability flag can ignore the mask
> argument.

struct v4l2\_subdev\_ops
:   Subdev operations

> **Definition**:
>
> ```
> struct v4l2_subdev_ops {
>       const struct v4l2_subdev_core_ops       *core;
>       const struct v4l2_subdev_tuner_ops      *tuner;
>       const struct v4l2_subdev_audio_ops      *audio;
>       const struct v4l2_subdev_video_ops      *video;
>       const struct v4l2_subdev_vbi_ops        *vbi;
>       const struct v4l2_subdev_ir_ops         *ir;
>       const struct v4l2_subdev_sensor_ops     *sensor;
>       const struct v4l2_subdev_pad_ops        *pad;
> };
> ```

**Members**

`core`
:   pointer to [`struct v4l2_subdev_core_ops`](#c.v4l2_subdev_core_ops "v4l2_subdev_core_ops"). Can be `NULL`

`tuner`
:   pointer to [`struct v4l2_subdev_tuner_ops`](#c.v4l2_subdev_tuner_ops "v4l2_subdev_tuner_ops"). Can be `NULL`

`audio`
:   pointer to [`struct v4l2_subdev_audio_ops`](#c.v4l2_subdev_audio_ops "v4l2_subdev_audio_ops"). Can be `NULL`

`video`
:   pointer to [`struct v4l2_subdev_video_ops`](#c.v4l2_subdev_video_ops "v4l2_subdev_video_ops"). Can be `NULL`

`vbi`
:   pointer to [`struct v4l2_subdev_vbi_ops`](#c.v4l2_subdev_vbi_ops "v4l2_subdev_vbi_ops"). Can be `NULL`

`ir`
:   pointer to [`struct v4l2_subdev_ir_ops`](#c.v4l2_subdev_ir_ops "v4l2_subdev_ir_ops"). Can be `NULL`

`sensor`
:   pointer to [`struct v4l2_subdev_sensor_ops`](#c.v4l2_subdev_sensor_ops "v4l2_subdev_sensor_ops"). Can be `NULL`

`pad`
:   pointer to [`struct v4l2_subdev_pad_ops`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops"). Can be `NULL`

struct v4l2\_subdev\_internal\_ops
:   V4L2 subdev internal ops

> **Definition**:
>
> ```
> struct v4l2_subdev_internal_ops {
>       int (*init_state)(struct v4l2_subdev *sd, struct v4l2_subdev_state *state);
>       int (*registered)(struct v4l2_subdev *sd);
>       void (*unregistered)(struct v4l2_subdev *sd);
>       int (*open)(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh);
>       int (*close)(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh);
>       void (*release)(struct v4l2_subdev *sd);
> };
> ```

**Members**

`init_state`
:   initialize the subdev state to default values

`registered`
:   called when this subdev is registered. When called the v4l2\_dev
    field is set to the correct v4l2\_device.

`unregistered`
:   called when this subdev is unregistered. When called the
    v4l2\_dev field is still set to the correct v4l2\_device.

`open`
:   called when the subdev device node is opened by an application.

`close`
:   called when the subdev device node is closed. Please note that
    it is possible for **close** to be called after **unregistered**!

`release`
:   called when the last user of the subdev device is gone. This
    happens after the **unregistered** callback and when the last open
    filehandle to the v4l-subdevX device node was closed. If no device
    node was created for this sub-device, then the **release** callback
    is called right after the **unregistered** callback.
    The **release** callback is typically used to free the memory containing
    the v4l2\_subdev structure. It is almost certainly required for any
    sub-device that sets the V4L2\_SUBDEV\_FL\_HAS\_DEVNODE flag.

**Description**

Note

Never call this from drivers, only the v4l2 framework can call
these ops.

struct v4l2\_subdev\_platform\_data
:   regulators config struct

> **Definition**:
>
> ```
> struct v4l2_subdev_platform_data {
>       struct regulator_bulk_data *regulators;
>       int num_regulators;
>       void *host_priv;
> };
> ```

**Members**

`regulators`
:   Optional regulators used to power on/off the subdevice

`num_regulators`
:   Number of regululators

`host_priv`
:   Per-subdevice data, specific for a certain video host device

struct v4l2\_subdev
:   describes a V4L2 sub-device

> **Definition**:
>
> ```
> struct v4l2_subdev {
> #if defined(CONFIG_MEDIA_CONTROLLER);
>       struct media_entity entity;
> #endif;
>       struct list_head list;
>       struct module *owner;
>       bool owner_v4l2_dev;
>       u32 flags;
>       struct v4l2_device *v4l2_dev;
>       const struct v4l2_subdev_ops *ops;
>       const struct v4l2_subdev_internal_ops *internal_ops;
>       struct v4l2_ctrl_handler *ctrl_handler;
>       char name[52];
>       u32 grp_id;
>       void *dev_priv;
>       void *host_priv;
>       struct video_device *devnode;
>       struct device *dev;
>       struct fwnode_handle *fwnode;
>       struct list_head async_list;
>       struct list_head async_subdev_endpoint_list;
>       struct v4l2_async_notifier *subdev_notifier;
>       struct list_head asc_list;
>       struct v4l2_subdev_platform_data *pdata;
>       struct mutex *state_lock;
>       struct led_classdev *privacy_led;
>       struct v4l2_subdev_state *active_state;
>       u64 enabled_pads;
>       bool s_stream_enabled;
> };
> ```

**Members**

`entity`
:   pointer to [`struct media_entity`](mc-core.html#c.media_entity "media_entity")

`list`
:   List of sub-devices

`owner`
:   The owner is the same as the driver’s [`struct device`](../infrastructure.html#c.device "device") owner.

`owner_v4l2_dev`
:   true if the `sd->owner` matches the owner of **v4l2\_dev->dev**
    owner. Initialized by [`v4l2_device_register_subdev()`](v4l2-device.html#c.v4l2_device_register_subdev "v4l2_device_register_subdev").

`flags`
:   subdev flags. Can be:
    `V4L2_SUBDEV_FL_IS_I2C` - Set this flag if this subdev is a i2c device;
    `V4L2_SUBDEV_FL_IS_SPI` - Set this flag if this subdev is a spi device;
    `V4L2_SUBDEV_FL_HAS_DEVNODE` - Set this flag if this subdev needs a
    device node;
    `V4L2_SUBDEV_FL_HAS_EVENTS` - Set this flag if this subdev generates
    events.

`v4l2_dev`
:   pointer to struct [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device")

`ops`
:   pointer to struct [`v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops")

`internal_ops`
:   pointer to struct [`v4l2_subdev_internal_ops`](#c.v4l2_subdev_internal_ops "v4l2_subdev_internal_ops").
    Never call these internal ops from within a driver!

`ctrl_handler`
:   The control handler of this subdev. May be NULL.

`name`
:   Name of the sub-device. Please notice that the name must be unique.

`grp_id`
:   can be used to group similar subdevs. Value is driver-specific

`dev_priv`
:   pointer to private data

`host_priv`
:   pointer to private data used by the device where the subdev
    is attached.

`devnode`
:   subdev device node

`dev`
:   pointer to the physical device, if any

`fwnode`
:   The fwnode\_handle of the subdev, usually the same as
    either dev->of\_node->fwnode or dev->fwnode (whichever is non-NULL).

`async_list`
:   Links this subdev to a global subdev\_list or
    **notifier->done\_list** list.

`async_subdev_endpoint_list`
:   List entry in async\_subdev\_endpoint\_entry of
    [`struct v4l2_async_subdev_endpoint`](v4l2-async.html#c.v4l2_async_subdev_endpoint "v4l2_async_subdev_endpoint").

`subdev_notifier`
:   A sub-device notifier implicitly registered for the sub-
    device using [`v4l2_async_register_subdev_sensor()`](v4l2-async.html#c.v4l2_async_register_subdev_sensor "v4l2_async_register_subdev_sensor").

`asc_list`
:   Async connection list, of [`struct
    v4l2_async_connection`](v4l2-async.html#c.v4l2_async_connection "v4l2_async_connection").subdev\_entry.

`pdata`
:   common part of subdevice platform data

`state_lock`
:   A pointer to a lock used for all the subdev’s states, set by the
    driver. This is optional. If NULL, each state instance will get
    a lock of its own.

`privacy_led`
:   Optional pointer to a LED classdev for the privacy LED for sensors.

`active_state`
:   Active state for the subdev (NULL for subdevs tracking the
    state internally). Initialized by calling
    [`v4l2_subdev_init_finalize()`](#c.v4l2_subdev_init_finalize "v4l2_subdev_init_finalize").

`enabled_pads`
:   Bitmask of enabled pads used by [`v4l2_subdev_enable_streams()`](#c.v4l2_subdev_enable_streams "v4l2_subdev_enable_streams")
    and [`v4l2_subdev_disable_streams()`](#c.v4l2_subdev_disable_streams "v4l2_subdev_disable_streams") helper functions for
    fallback cases.

`s_stream_enabled`
:   Tracks whether streaming has been enabled with s\_stream.
    This is only for `call_s_stream()` internal use.

**Description**

Each instance of a subdev driver should create this struct, either
stand-alone or embedded in a larger struct.

This structure should be initialized by [`v4l2_subdev_init()`](#c.v4l2_subdev_init "v4l2_subdev_init") or one of
its variants: [`v4l2_spi_subdev_init()`](v4l2-common.html#c.v4l2_spi_subdev_init "v4l2_spi_subdev_init"), [`v4l2_i2c_subdev_init()`](v4l2-common.html#c.v4l2_i2c_subdev_init "v4l2_i2c_subdev_init").

media\_entity\_to\_v4l2\_subdev

`media_entity_to_v4l2_subdev (ent)`

> Returns a [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") from the [`struct media_entity`](mc-core.html#c.media_entity "media_entity") embedded in it.

**Parameters**

`ent`
:   pointer to [`struct media_entity`](mc-core.html#c.media_entity "media_entity").

vdev\_to\_v4l2\_subdev

`vdev_to_v4l2_subdev (vdev)`

> Returns a [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") from the [`struct video_device`](v4l2-dev.html#c.video_device "video_device") embedded on it.

**Parameters**

`vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

struct v4l2\_subdev\_fh
:   Used for storing subdev information per file handle

> **Definition**:
>
> ```
> struct v4l2_subdev_fh {
>       struct v4l2_fh vfh;
>       struct module *owner;
> #if defined(CONFIG_VIDEO_V4L2_SUBDEV_API);
>       struct v4l2_subdev_state *state;
>       u64 client_caps;
> #endif;
> };
> ```

**Members**

`vfh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`owner`
:   module pointer to the owner of this file handle

`state`
:   pointer to [`struct v4l2_subdev_state`](#c.v4l2_subdev_state "v4l2_subdev_state")

`client_caps`
:   bitmask of `V4L2_SUBDEV_CLIENT_CAP_*`

to\_v4l2\_subdev\_fh

`to_v4l2_subdev_fh (fh)`

> Returns a [`struct v4l2_subdev_fh`](#c.v4l2_subdev_fh "v4l2_subdev_fh") from the [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") embedded on it.

**Parameters**

`fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

void v4l2\_set\_subdevdata(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, void \*p)
:   Sets V4L2 dev private device data

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`void *p`
:   pointer to the private device data to be stored.

void \*v4l2\_get\_subdevdata(const struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Gets V4L2 dev private device data

**Parameters**

`const struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

**Description**

Returns the pointer to the private device data to be stored.

void v4l2\_set\_subdev\_hostdata(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, void \*p)
:   Sets V4L2 dev private host data

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`void *p`
:   pointer to the private data to be stored.

void \*v4l2\_get\_subdev\_hostdata(const struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Gets V4L2 dev private data

**Parameters**

`const struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

**Description**

Returns the pointer to the private host data to be stored.

int v4l2\_subdev\_get\_fwnode\_pad\_1\_to\_1(struct [media\_entity](mc-core.html#c.media_entity "media_entity") \*entity, struct fwnode\_endpoint \*endpoint)
:   Get pad number from a subdev fwnode endpoint, assuming 1:1 port:pad

**Parameters**

`struct media_entity *entity`
:   Pointer to the subdev entity

`struct fwnode_endpoint *endpoint`
:   Pointer to a parsed fwnode endpoint

**Description**

This function can be used as the .get\_fwnode\_pad operation for
subdevices that map port numbers and pad indexes 1:1. If the endpoint
is owned by the subdevice, the function returns the endpoint port
number.

Returns the endpoint port number on success or a negative error code.

int v4l2\_subdev\_link\_validate\_default(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, struct [media\_link](mc-core.html#c.media_link "media_link") \*link, struct v4l2\_subdev\_format \*source\_fmt, struct v4l2\_subdev\_format \*sink\_fmt)
:   validates a media link

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`struct media_link *link`
:   pointer to [`struct media_link`](mc-core.html#c.media_link "media_link")

`struct v4l2_subdev_format *source_fmt`
:   pointer to `struct v4l2_subdev_format`

`struct v4l2_subdev_format *sink_fmt`
:   pointer to `struct v4l2_subdev_format`

**Description**

This function ensures that width, height and the media bus pixel
code are equal on both source and sink of the link.

int v4l2\_subdev\_link\_validate(struct [media\_link](mc-core.html#c.media_link "media_link") \*link)
:   validates a media link

**Parameters**

`struct media_link *link`
:   pointer to [`struct media_link`](mc-core.html#c.media_link "media_link")

**Description**

This function calls the subdev’s link\_validate ops to validate
if a media link is valid for streaming. It also internally
calls [`v4l2_subdev_link_validate_default()`](#c.v4l2_subdev_link_validate_default "v4l2_subdev_link_validate_default") to ensure that
width, height and the media bus pixel code are equal on both
source and sink of the link.

The function can be used as a drop-in `media_entity_ops.link_validate`
implementation for v4l2\_subdev instances. It supports all links between
subdevs, as well as links between subdevs and video devices, provided that
the video devices also implement their `media_entity_ops.link_validate`
operation.

bool v4l2\_subdev\_has\_pad\_interdep(struct [media\_entity](mc-core.html#c.media_entity "media_entity") \*entity, unsigned int pad0, unsigned int pad1)
:   MC has\_pad\_interdep implementation for subdevs

**Parameters**

`struct media_entity *entity`
:   pointer to [`struct media_entity`](mc-core.html#c.media_entity "media_entity")

`unsigned int pad0`
:   pad number for the first pad

`unsigned int pad1`
:   pad number for the second pad

**Description**

This function is an implementation of the
media\_entity\_operations.has\_pad\_interdep operation for subdevs that
implement the multiplexed streams API (as indicated by the
V4L2\_SUBDEV\_FL\_STREAMS subdev flag).

It considers two pads interdependent if there is an active route between pad0
and pad1.

struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*\_\_v4l2\_subdev\_state\_alloc(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, const char \*lock\_name, struct lock\_class\_key \*key)
:   allocate v4l2\_subdev\_state

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") for which the state is being allocated.

`const char *lock_name`
:   name of the state lock

`struct lock_class_key *key`
:   lock\_class\_key for the lock

**Description**

Must call [`__v4l2_subdev_state_free()`](#c.__v4l2_subdev_state_free "__v4l2_subdev_state_free") when state is no longer needed.

Not to be called directly by the drivers.

void \_\_v4l2\_subdev\_state\_free(struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state)
:   free a v4l2\_subdev\_state

**Parameters**

`struct v4l2_subdev_state *state`
:   v4l2\_subdev\_state to be freed.

**Description**

Not to be called directly by the drivers.

v4l2\_subdev\_init\_finalize

`v4l2_subdev_init_finalize (sd)`

> Finalizes the initialization of the subdevice

**Parameters**

`sd`
:   The subdev

**Description**

This function finalizes the initialization of the subdev, including
allocation of the active state for the subdev.

This function must be called by the subdev drivers that use the centralized
active state, after the subdev `struct has` been initialized and
[`media_entity_pads_init()`](mc-core.html#c.media_entity_pads_init "media_entity_pads_init") has been called, but before registering the
subdev.

The user must call [`v4l2_subdev_cleanup()`](#c.v4l2_subdev_cleanup "v4l2_subdev_cleanup") when the subdev is being removed.

void v4l2\_subdev\_cleanup(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Releases the resources allocated by the subdevice

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

**Description**

Clean up a V4L2 async sub-device. Must be called for a sub-device as part of
its release if resources have been associated with it using
[`v4l2_async_subdev_endpoint_add()`](v4l2-async.html#c.v4l2_async_subdev_endpoint_add "v4l2_async_subdev_endpoint_add") or [`v4l2_subdev_init_finalize()`](#c.v4l2_subdev_init_finalize "v4l2_subdev_init_finalize").

v4l2\_subdev\_state\_get\_format

`v4l2_subdev_state_get_format (state, pad, ...)`

> Get pointer to a stream format

**Parameters**

`state`
:   subdevice state

`pad`
:   pad id

`...`
:   stream id (optional argument)

**Description**

This returns a pointer to `struct v4l2_mbus_framefmt` for the given pad +
stream in the subdev state.

For stream-unaware drivers the format for the corresponding pad is returned.
If the pad does not exist, NULL is returned.

v4l2\_subdev\_state\_get\_crop

`v4l2_subdev_state_get_crop (state, pad, ...)`

> Get pointer to a stream crop rectangle

**Parameters**

`state`
:   subdevice state

`pad`
:   pad id

`...`
:   stream id (optional argument)

**Description**

This returns a pointer to crop rectangle for the given pad + stream in the
subdev state.

For stream-unaware drivers the crop rectangle for the corresponding pad is
returned. If the pad does not exist, NULL is returned.

v4l2\_subdev\_state\_get\_compose

`v4l2_subdev_state_get_compose (state, pad, ...)`

> Get pointer to a stream compose rectangle

**Parameters**

`state`
:   subdevice state

`pad`
:   pad id

`...`
:   stream id (optional argument)

**Description**

This returns a pointer to compose rectangle for the given pad + stream in the
subdev state.

For stream-unaware drivers the compose rectangle for the corresponding pad is
returned. If the pad does not exist, NULL is returned.

v4l2\_subdev\_state\_get\_interval

`v4l2_subdev_state_get_interval (state, pad, ...)`

> Get pointer to a stream frame interval

**Parameters**

`state`
:   subdevice state

`pad`
:   pad id

`...`
:   stream id (optional argument)

**Description**

This returns a pointer to the frame interval for the given pad + stream in
the subdev state.

For stream-unaware drivers the frame interval for the corresponding pad is
returned. If the pad does not exist, NULL is returned.

int v4l2\_subdev\_get\_fmt(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, struct v4l2\_subdev\_format \*format)
:   Fill format based on state

**Parameters**

`struct v4l2_subdev *sd`
:   subdevice

`struct v4l2_subdev_state *state`
:   subdevice state

`struct v4l2_subdev_format *format`
:   pointer to `struct v4l2_subdev_format`

**Description**

Fill **format->format** field based on the information in the **format** struct.

This function can be used by the subdev drivers which support active state to
implement v4l2\_subdev\_pad\_ops.get\_fmt if the subdev driver does not need to
do anything special in their get\_fmt op.

Returns 0 on success, error value otherwise.

int v4l2\_subdev\_get\_frame\_interval(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, struct v4l2\_subdev\_frame\_interval \*fi)
:   Fill frame interval based on state

**Parameters**

`struct v4l2_subdev *sd`
:   subdevice

`struct v4l2_subdev_state *state`
:   subdevice state

`struct v4l2_subdev_frame_interval *fi`
:   pointer to `struct v4l2_subdev_frame_interval`

**Description**

Fill **fi->interval** field based on the information in the **fi** struct.

This function can be used by the subdev drivers which support active state to
implement v4l2\_subdev\_pad\_ops.get\_frame\_interval if the subdev driver does
not need to do anything special in their get\_frame\_interval op.

Returns 0 on success, error value otherwise.

int v4l2\_subdev\_set\_routing(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, const struct [v4l2\_subdev\_krouting](#c.v4l2_subdev_krouting "v4l2_subdev_krouting") \*routing)
:   Set given routing to subdev state

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`struct v4l2_subdev_state *state`
:   The subdevice state

`const struct v4l2_subdev_krouting *routing`
:   Routing that will be copied to subdev state

**Description**

This will release old routing table (if any) from the state, allocate
enough space for the given routing, and copy the routing.

This can be used from the subdev driver’s set\_routing op, after validating
the routing.

for\_each\_active\_route

`for_each_active_route (routing, route)`

> iterate on all active routes of a routing table

**Parameters**

`routing`
:   The routing table

`route`
:   The route iterator

int v4l2\_subdev\_set\_routing\_with\_fmt(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, const struct [v4l2\_subdev\_krouting](#c.v4l2_subdev_krouting "v4l2_subdev_krouting") \*routing, const struct v4l2\_mbus\_framefmt \*fmt)
:   Set given routing and format to subdev state

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`struct v4l2_subdev_state *state`
:   The subdevice state

`const struct v4l2_subdev_krouting *routing`
:   Routing that will be copied to subdev state

`const struct v4l2_mbus_framefmt *fmt`
:   Format used to initialize all the streams

**Description**

This is the same as v4l2\_subdev\_set\_routing, but additionally initializes
all the streams using the given format.

int v4l2\_subdev\_routing\_find\_opposite\_end(const struct [v4l2\_subdev\_krouting](#c.v4l2_subdev_krouting "v4l2_subdev_krouting") \*routing, u32 pad, u32 stream, u32 \*other\_pad, u32 \*other\_stream)
:   Find the opposite stream

**Parameters**

`const struct v4l2_subdev_krouting *routing`
:   routing used to find the opposite side

`u32 pad`
:   pad id

`u32 stream`
:   stream id

`u32 *other_pad`
:   pointer used to return the opposite pad

`u32 *other_stream`
:   pointer used to return the opposite stream

**Description**

This function uses the routing table to find the pad + stream which is
opposite the given pad + stream.

**other\_pad** and/or **other\_stream** can be NULL if the caller does not need the
value.

Returns 0 on success, or -EINVAL if no matching route is found.

struct v4l2\_mbus\_framefmt \*v4l2\_subdev\_state\_get\_opposite\_stream\_format(struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, u32 pad, u32 stream)
:   Get pointer to opposite stream format

**Parameters**

`struct v4l2_subdev_state *state`
:   subdevice state

`u32 pad`
:   pad id

`u32 stream`
:   stream id

**Description**

This returns a pointer to `struct v4l2_mbus_framefmt` for the pad + stream
that is opposite the given pad + stream in the subdev state.

If the state does not contain the given pad + stream, NULL is returned.

u64 v4l2\_subdev\_state\_xlate\_streams(const struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, u32 pad0, u32 pad1, u64 \*streams)
:   Translate streams from one pad to another

**Parameters**

`const struct v4l2_subdev_state *state`
:   Subdevice state

`u32 pad0`
:   The first pad

`u32 pad1`
:   The second pad

`u64 *streams`
:   Streams bitmask on the first pad

**Description**

Streams on sink pads of a subdev are routed to source pads as expressed in
the subdev state routing table. Stream numbers don’t necessarily match on
the sink and source side of a route. This function translates stream numbers
on **pad0**, expressed as a bitmask in **streams**, to the corresponding streams
on **pad1** using the routing table from the **state**. It returns the stream mask
on **pad1**, and updates **streams** with the streams that have been found in the
routing table.

**pad0** and **pad1** must be a sink and a source, in any order.

**Return**

The bitmask of streams of **pad1** that are routed to **streams** on **pad0**.

enum v4l2\_subdev\_routing\_restriction
:   Subdevice internal routing restrictions

**Constants**

`V4L2_SUBDEV_ROUTING_NO_1_TO_N`
:   an input stream shall not be routed to multiple output streams (stream
    duplication)

`V4L2_SUBDEV_ROUTING_NO_N_TO_1`
:   multiple input streams shall not be routed to the same output stream
    (stream merging)

`V4L2_SUBDEV_ROUTING_NO_SINK_STREAM_MIX`
:   all streams from a sink pad must be routed to a single source pad

`V4L2_SUBDEV_ROUTING_NO_SOURCE_STREAM_MIX`
:   all streams on a source pad must originate from a single sink pad

`V4L2_SUBDEV_ROUTING_NO_SINK_MULTIPLEXING`
:   sink pads shall not contain multiplexed streams

`V4L2_SUBDEV_ROUTING_NO_SOURCE_MULTIPLEXING`
:   source pads shall not contain multiplexed streams

`V4L2_SUBDEV_ROUTING_ONLY_1_TO_1`
:   only non-overlapping 1-to-1 stream routing is allowed (a combination of
    **V4L2\_SUBDEV\_ROUTING\_NO\_1\_TO\_N** and **V4L2\_SUBDEV\_ROUTING\_NO\_N\_TO\_1**)

`V4L2_SUBDEV_ROUTING_NO_STREAM_MIX`
:   all streams from a sink pad must be routed to a single source pad, and
    that source pad shall not get routes from any other sink pad
    (a combination of **V4L2\_SUBDEV\_ROUTING\_NO\_SINK\_STREAM\_MIX** and
    **V4L2\_SUBDEV\_ROUTING\_NO\_SOURCE\_STREAM\_MIX**)

`V4L2_SUBDEV_ROUTING_NO_MULTIPLEXING`
:   no multiplexed streams allowed on either source or sink sides.

int v4l2\_subdev\_routing\_validate(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, const struct [v4l2\_subdev\_krouting](#c.v4l2_subdev_krouting "v4l2_subdev_krouting") \*routing, enum [v4l2\_subdev\_routing\_restriction](#c.v4l2_subdev_routing_restriction "v4l2_subdev_routing_restriction") disallow)
:   Verify that routes comply with driver constraints

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`const struct v4l2_subdev_krouting *routing`
:   Routing to verify

`enum v4l2_subdev_routing_restriction disallow`
:   Restrictions on routes

**Description**

This verifies that the given routing complies with the **disallow** constraints.

Returns 0 on success, error value otherwise.

int v4l2\_subdev\_enable\_streams(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, u32 pad, u64 streams\_mask)
:   Enable streams on a pad

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`u32 pad`
:   The pad

`u64 streams_mask`
:   Bitmask of streams to enable

**Description**

This function enables streams on a source **pad** of a subdevice. The pad is
identified by its index, while the streams are identified by the
**streams\_mask** bitmask. This allows enabling multiple streams on a pad at
once.

Enabling a stream that is already enabled isn’t allowed. If **streams\_mask**
contains an already enabled stream, this function returns -EALREADY without
performing any operation.

Per-stream enable is only available for subdevs that implement the
.`enable_streams()` and .`disable_streams()` operations. For other subdevs, this
function implements a best-effort compatibility by calling the .`s_stream()`
operation, limited to subdevs that have a single source pad.

Drivers that are not stream-aware shall set **streams\_mask** to BIT\_ULL(0).

**Return**

* 0: Success
* -EALREADY: One of the streams in streams\_mask is already enabled
* -EINVAL: The pad index is invalid, or doesn’t correspond to a source pad
* -EOPNOTSUPP: Falling back to the legacy .`s_stream()` operation is
  impossible because the subdev has multiple source pads

int v4l2\_subdev\_disable\_streams(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, u32 pad, u64 streams\_mask)
:   Disable streams on a pad

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`u32 pad`
:   The pad

`u64 streams_mask`
:   Bitmask of streams to disable

**Description**

This function disables streams on a source **pad** of a subdevice. The pad is
identified by its index, while the streams are identified by the
**streams\_mask** bitmask. This allows disabling multiple streams on a pad at
once.

Disabling a streams that is not enabled isn’t allowed. If **streams\_mask**
contains a disabled stream, this function returns -EALREADY without
performing any operation.

Per-stream disable is only available for subdevs that implement the
.`enable_streams()` and .`disable_streams()` operations. For other subdevs, this
function implements a best-effort compatibility by calling the .`s_stream()`
operation, limited to subdevs that have a single source pad.

Drivers that are not stream-aware shall set **streams\_mask** to BIT\_ULL(0).

**Return**

* 0: Success
* -EALREADY: One of the streams in streams\_mask is not enabled
* -EINVAL: The pad index is invalid, or doesn’t correspond to a source pad
* -EOPNOTSUPP: Falling back to the legacy .`s_stream()` operation is
  impossible because the subdev has multiple source pads

int v4l2\_subdev\_s\_stream\_helper(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, int enable)
:   Helper to implement the subdev s\_stream operation using enable\_streams and disable\_streams

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`int enable`
:   Enable or disable streaming

**Description**

Subdevice drivers that implement the streams-aware
[`v4l2_subdev_pad_ops.enable_streams`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops") and [`v4l2_subdev_pad_ops.disable_streams`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops")
operations can use this helper to implement the legacy
[`v4l2_subdev_video_ops.s_stream`](#c.v4l2_subdev_video_ops "v4l2_subdev_video_ops") operation.

This helper can only be used by subdevs that have a single source pad.

**Return**

0 on success, or a negative error code otherwise.

int \_\_v4l2\_subdev\_get\_frame\_desc\_passthrough(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state, unsigned int pad, struct [v4l2\_mbus\_frame\_desc](#c.v4l2_mbus_frame_desc "v4l2_mbus_frame_desc") \*fd)
:   Helper to implement the subdev get\_frame\_desc operation in simple passthrough cases

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`struct v4l2_subdev_state *state`
:   The locked subdevice active state

`unsigned int pad`
:   The source pad index

`struct v4l2_mbus_frame_desc *fd`
:   The mbus frame desc

**Description**

This helper implements the get\_frame\_desc operation for subdevices that pass
streams through without modification.

The helper iterates over the subdevice’s sink pads, calls get\_frame\_desc on
the remote subdevice connected to each sink pad, and collects the frame desc
entries for streams that are routed to the given source pad according to the
subdevice’s routing table. Each entry is copied as-is from the upstream
source, with the exception of the ‘stream’ field which is remapped to the
source stream ID from the routing table.

The frame desc type is taken from the first upstream source. If multiple
sink pads are involved and the upstream sources report different frame desc
types, -EPIPE is returned.

The caller must hold the subdevice’s active state lock. This variant is
intended for drivers that need to perform additional work around the
passthrough frame descriptor collection. Drivers that do not need any
customization should use [`v4l2_subdev_get_frame_desc_passthrough()`](#c.v4l2_subdev_get_frame_desc_passthrough "v4l2_subdev_get_frame_desc_passthrough") instead.

**Return**

0 on success, or a negative error code otherwise.

int v4l2\_subdev\_get\_frame\_desc\_passthrough(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, unsigned int pad, struct [v4l2\_mbus\_frame\_desc](#c.v4l2_mbus_frame_desc "v4l2_mbus_frame_desc") \*fd)
:   Helper to implement the subdev get\_frame\_desc operation in simple passthrough cases

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

`unsigned int pad`
:   The source pad index

`struct v4l2_mbus_frame_desc *fd`
:   The mbus frame desc

**Description**

This function locks the subdevice’s active state, calls
[`__v4l2_subdev_get_frame_desc_passthrough()`](#c.__v4l2_subdev_get_frame_desc_passthrough "__v4l2_subdev_get_frame_desc_passthrough"), and unlocks the state.

This function can be assigned directly as the .get\_frame\_desc callback in
[`v4l2_subdev_pad_ops`](#c.v4l2_subdev_pad_ops "v4l2_subdev_pad_ops") for subdevices that pass streams through without
modification. Drivers that need to perform additional work should use
[`__v4l2_subdev_get_frame_desc_passthrough()`](#c.__v4l2_subdev_get_frame_desc_passthrough "__v4l2_subdev_get_frame_desc_passthrough") in their custom
.get\_frame\_desc implementation instead.

**Return**

0 on success, or a negative error code otherwise.

void v4l2\_subdev\_lock\_state(struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state)
:   Locks the subdev state

**Parameters**

`struct v4l2_subdev_state *state`
:   The subdevice state

**Description**

Locks the given subdev state.

The state must be unlocked with [`v4l2_subdev_unlock_state()`](#c.v4l2_subdev_unlock_state "v4l2_subdev_unlock_state") after use.

void v4l2\_subdev\_unlock\_state(struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state)
:   Unlocks the subdev state

**Parameters**

`struct v4l2_subdev_state *state`
:   The subdevice state

**Description**

Unlocks the given subdev state.

void v4l2\_subdev\_lock\_states(struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state1, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state2)
:   Lock two sub-device states

**Parameters**

`struct v4l2_subdev_state *state1`
:   One subdevice state

`struct v4l2_subdev_state *state2`
:   The other subdevice state

**Description**

Locks the state of two sub-devices.

The states must be unlocked with [`v4l2_subdev_unlock_states()`](#c.v4l2_subdev_unlock_states "v4l2_subdev_unlock_states") after use.

This differs from calling [`v4l2_subdev_lock_state()`](#c.v4l2_subdev_lock_state "v4l2_subdev_lock_state") on both states so that if
the states share the same lock, the lock is acquired only once (so no
deadlock occurs). The caller is responsible for ensuring the locks will
always be acquired in the same order.

void v4l2\_subdev\_unlock\_states(struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state1, struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*state2)
:   Unlock two sub-device states

**Parameters**

`struct v4l2_subdev_state *state1`
:   One subdevice state

`struct v4l2_subdev_state *state2`
:   The other subdevice state

**Description**

Unlocks the state of two sub-devices.

This differs from calling [`v4l2_subdev_unlock_state()`](#c.v4l2_subdev_unlock_state "v4l2_subdev_unlock_state") on both states so that
if the states share the same lock, the lock is released only once.

struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*v4l2\_subdev\_get\_unlocked\_active\_state(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Checks that the active subdev state is unlocked and returns it

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

**Description**

Returns the active state for the subdevice, or NULL if the subdev does not
support active state. If the state is not NULL, calls
`lockdep_assert_not_held()` to issue a warning if the state is locked.

This function is to be used e.g. when getting the active state for the sole
purpose of passing it forward, without accessing the state fields.

struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*v4l2\_subdev\_get\_locked\_active\_state(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Checks that the active subdev state is locked and returns it

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

**Description**

Returns the active state for the subdevice, or NULL if the subdev does not
support active state. If the state is not NULL, calls `lockdep_assert_held()`
to issue a warning if the state is not locked.

This function is to be used when the caller knows that the active state is
already locked.

struct [v4l2\_subdev\_state](#c.v4l2_subdev_state "v4l2_subdev_state") \*v4l2\_subdev\_lock\_and\_get\_active\_state(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Locks and returns the active subdev state for the subdevice

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

**Description**

Returns the locked active state for the subdevice, or NULL if the subdev
does not support active state.

The state must be unlocked with [`v4l2_subdev_unlock_state()`](#c.v4l2_subdev_unlock_state "v4l2_subdev_unlock_state") after use.

void v4l2\_subdev\_init(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, const struct [v4l2\_subdev\_ops](#c.v4l2_subdev_ops "v4l2_subdev_ops") \*ops)
:   initializes the sub-device struct

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to the [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev") to be initialized

`const struct v4l2_subdev_ops *ops`
:   pointer to [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops").

v4l2\_subdev\_call

`v4l2_subdev_call (sd, o, f, args...)`

> call an operation of a v4l2\_subdev.

**Parameters**

`sd`
:   pointer to the [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`o`
:   name of the element at [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of callbacks functions.

`f`
:   callback function to be called.
    The callback functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Example**

err = v4l2\_subdev\_call(sd, video, s\_std, norm);

v4l2\_subdev\_call\_state\_active

`v4l2_subdev_call_state_active (sd, o, f, args...)`

> call an operation of a v4l2\_subdev which takes state as a parameter, passing the subdev its active state.

**Parameters**

`sd`
:   pointer to the [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`o`
:   name of the element at [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of callbacks functions.

`f`
:   callback function to be called.
    The callback functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

This is similar to [`v4l2_subdev_call()`](#c.v4l2_subdev_call "v4l2_subdev_call"), except that this version can only be
used for ops that take a subdev state as a parameter. The macro will get the
active state, lock it before calling the op and unlock it after the call.

v4l2\_subdev\_call\_state\_try

`v4l2_subdev_call_state_try (sd, o, f, args...)`

> call an operation of a v4l2\_subdev which takes state as a parameter, passing the subdev a newly allocated try state.

**Parameters**

`sd`
:   pointer to the [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`o`
:   name of the element at [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops") that contains **f**.
    Each element there groups a set of callbacks functions.

`f`
:   callback function to be called.
    The callback functions are defined in groups, according to
    each element at [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops").

`args...`
:   arguments for **f**.

**Description**

This is similar to [`v4l2_subdev_call_state_active()`](#c.v4l2_subdev_call_state_active "v4l2_subdev_call_state_active"), except that as this
version allocates a new state, this is only usable for
V4L2\_SUBDEV\_FORMAT\_TRY use cases.

**Note**

only legacy non-MC drivers may need this macro.

v4l2\_subdev\_has\_op

`v4l2_subdev_has_op (sd, o, f)`

> Checks if a subdev defines a certain operation.

**Parameters**

`sd`
:   pointer to the [`struct v4l2_subdev`](#c.v4l2_subdev "v4l2_subdev")

`o`
:   The group of callback functions in [`struct v4l2_subdev_ops`](#c.v4l2_subdev_ops "v4l2_subdev_ops")
    which **f** is a part of.

`f`
:   callback function to be checked for its existence.

void v4l2\_subdev\_notify\_event(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd, const struct v4l2\_event \*ev)
:   Delivers event notification for subdevice

**Parameters**

`struct v4l2_subdev *sd`
:   The subdev for which to deliver the event

`const struct v4l2_event *ev`
:   The event to deliver

**Description**

Will deliver the specified event to all userspace event listeners which are
subscribed to the v42l subdev event queue as well as to the bridge driver
using the notify callback. The notification type for the notify callback
will be `V4L2_DEVICE_NOTIFY_EVENT`.

bool v4l2\_subdev\_is\_streaming(struct [v4l2\_subdev](#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Returns if the subdevice is streaming

**Parameters**

`struct v4l2_subdev *sd`
:   The subdevice

**Description**

[`v4l2_subdev_is_streaming()`](#c.v4l2_subdev_is_streaming "v4l2_subdev_is_streaming") tells if the subdevice is currently streaming.
“Streaming” here means whether .`s_stream()` or .`enable_streams()` has been
successfully called, and the streaming has not yet been disabled.

If the subdevice implements .`enable_streams()` this function must be called
while holding the active state lock.
