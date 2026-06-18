# 3.4.Video device’ s internal representation

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-dev.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.4. Video device’ s internal representation

The actual device nodes in the `/dev` directory are created using the
[`video_device`](#c.video_device "video_device") struct (`v4l2-dev.h`). This `struct can` either be
allocated dynamically or embedded in a larger struct.

To allocate it dynamically use [`video_device_alloc()`](#c.video_device_alloc "video_device_alloc"):

```
struct video_device *vdev = video_device_alloc();

if (vdev == NULL)
        return -ENOMEM;

vdev->release = video_device_release;
```

If you embed it in a larger struct, then you must set the `release()`
callback to your own function:

```
struct video_device *vdev = &my_vdev->vdev;

vdev->release = my_vdev_release;
```

The `release()` callback must be set and it is called when the last user
of the video device exits.

The default [`video_device_release()`](#c.video_device_release "video_device_release") callback currently
just calls `kfree` to free the allocated memory.

There is also a [`video_device_release_empty()`](#c.video_device_release_empty "video_device_release_empty") function that does
nothing (is empty) and should be used if the `struct is` embedded and there
is nothing to do when it is released.

You should also set these fields of [`video_device`](#c.video_device "video_device"):

* [`video_device`](#c.video_device "video_device")->v4l2\_dev: must be set to the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device")
  parent device.
* [`video_device`](#c.video_device "video_device")->name: set to something descriptive and unique.
* [`video_device`](#c.video_device "video_device")->vfl\_dir: set this to `VFL_DIR_RX` for capture
  devices (`VFL_DIR_RX` has value 0, so this is normally already the
  default), set to `VFL_DIR_TX` for output devices and `VFL_DIR_M2M` for mem2mem (codec) devices.
* [`video_device`](#c.video_device "video_device")->fops: set to the [`v4l2_file_operations`](#c.v4l2_file_operations "v4l2_file_operations")
  struct.
* [`video_device`](#c.video_device "video_device")->ioctl\_ops: if you use the [`v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops")
  to simplify ioctl maintenance (highly recommended to use this and it might
  become compulsory in the future!), then set this to your
  [`v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops") struct. The [`video_device`](#c.video_device "video_device")->vfl\_type and
  [`video_device`](#c.video_device "video_device")->vfl\_dir fields are used to disable ops that do not
  match the type/dir combination. E.g. VBI ops are disabled for non-VBI nodes,
  and output ops are disabled for a capture device. This makes it possible to
  provide just one [`v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops") struct for both vbi and
  video nodes.
* [`video_device`](#c.video_device "video_device")->lock: leave to `NULL` if you want to do all the
  locking in the driver. Otherwise you give it a pointer to a struct
  `mutex_lock` and before the [`video_device`](#c.video_device "video_device")->unlocked\_ioctl
  file operation is called this lock will be taken by the core and released
  afterwards. See the next section for more details.
* [`video_device`](#c.video_device "video_device")->queue: a pointer to the [`struct vb2_queue`](v4l2-videobuf2.html#c.vb2_queue "vb2_queue")
  associated with this device node.
  If queue is not `NULL`, and queue->lock is not `NULL`, then queue->lock
  is used for the queuing ioctls (`VIDIOC_REQBUFS`, `CREATE_BUFS`,
  `QBUF`, `DQBUF`, `QUERYBUF`, `PREPARE_BUF`, `STREAMON` and
  `STREAMOFF`) instead of the lock above.
  That way the [vb2](v4l2-videobuf2.html#vb2-framework) queuing framework does not have
  to wait for other ioctls. This queue pointer is also used by the
  [vb2](v4l2-videobuf2.html#vb2-framework) helper functions to check for
  queuing ownership (i.e. is the filehandle calling it allowed to do the
  operation).
* [`video_device`](#c.video_device "video_device")->prio: keeps track of the priorities. Used to
  implement `VIDIOC_G_PRIORITY` and `VIDIOC_S_PRIORITY`.
  If left to `NULL`, then it will use the [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state")
  in [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device"). If you want to have a separate priority state per
  (group of) device node(s), then you can point it to your own struct
  [`v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state").
* [`video_device`](#c.video_device "video_device")->dev\_parent: you only set this if v4l2\_device was
  registered with `NULL` as the parent `device` struct. This only happens
  in cases where one hardware device has multiple PCI devices that all share
  the same [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") core.

  The cx88 driver is an example of this: one core [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") struct,
  but it is used by both a raw video PCI device (cx8800) and a MPEG PCI device
  (cx8802). Since the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") cannot be associated with two PCI
  devices at the same time it is setup without a parent device. But when the
  [`struct video_device`](#c.video_device "video_device") is initialized you **do** know which parent
  PCI device to use and so you set `dev_device` to the correct PCI device.

If you use [`v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops"), then you should set
[`video_device`](#c.video_device "video_device")->unlocked\_ioctl to [`video_ioctl2()`](v4l2-common.html#c.video_ioctl2 "video_ioctl2") in your
[`v4l2_file_operations`](#c.v4l2_file_operations "v4l2_file_operations") struct.

In some cases you want to tell the core that a function you had specified in
your [`v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops") should be ignored. You can mark such ioctls by
calling this function before [`video_register_device()`](#c.video_register_device "video_register_device") is called:

> [`v4l2_disable_ioctl`](#c.v4l2_disable_ioctl "v4l2_disable_ioctl")
> ([`vdev`](#c.video_device "video_device"), cmd).

This tends to be needed if based on external factors (e.g. which card is
being used) you want to turns off certain features in [`v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops")
without having to make a new struct.

The [`v4l2_file_operations`](#c.v4l2_file_operations "v4l2_file_operations") `struct is` a subset of file\_operations.
The main difference is that the inode argument is omitted since it is never
used.

If integration with the media framework is needed, you must initialize the
[`media_entity`](mc-core.html#c.media_entity "media_entity") `struct embedded` in the [`video_device`](#c.video_device "video_device") struct
(entity field) by calling [`media_entity_pads_init()`](mc-core.html#c.media_entity_pads_init "media_entity_pads_init"):

```
struct media_pad *pad = &my_vdev->pad;
int err;

err = media_entity_pads_init(&vdev->entity, 1, pad);
```

The pads array must have been previously initialized. There is no need to
manually set the [`struct media_entity`](mc-core.html#c.media_entity "media_entity") type and name fields.

A reference to the entity will be automatically acquired/released when the
video device is opened/closed.

## 3.4.1. ioctls and locking

The V4L core provides optional locking services. The main service is the
lock field in [`struct video_device`](#c.video_device "video_device"), which is a pointer to a mutex.
If you set this pointer, then that will be used by unlocked\_ioctl to
serialize all ioctls.

If you are using the [videobuf2 framework](v4l2-videobuf2.html#vb2-framework), then there
is a second lock that you can set: [`video_device`](#c.video_device "video_device")->queue->lock. If
set, then this lock will be used instead of [`video_device`](#c.video_device "video_device")->lock
to serialize all queuing ioctls (see the previous section
for the full list of those ioctls).

The advantage of using a different lock for the queuing ioctls is that for some
drivers (particularly USB drivers) certain commands such as setting controls
can take a long time, so you want to use a separate lock for the buffer queuing
ioctls. That way your `VIDIOC_DQBUF` doesn’t stall because the driver is busy
changing the e.g. exposure of the webcam.

Of course, you can always do all the locking yourself by leaving both lock
pointers at `NULL`.

In the case of [videobuf2](v4l2-videobuf2.html#vb2-framework) you must set the `queue->lock`
pointer to the lock you use to serialize the queuing ioctls. This ensures that
that lock is released while waiting in `VIDIOC_DQBUF` for a buffer to arrive,
and it is retaken afterwards.

The implementation of a hotplug disconnect should also take the lock from
[`video_device`](#c.video_device "video_device") before calling v4l2\_device\_disconnect. If you are also
using [`video_device`](#c.video_device "video_device")->queue->lock, then you have to first lock
[`video_device`](#c.video_device "video_device")->queue->lock followed by [`video_device`](#c.video_device "video_device")->lock.
That way you can be sure no ioctl is running when you call
[`v4l2_device_disconnect()`](v4l2-device.html#c.v4l2_device_disconnect "v4l2_device_disconnect").

## 3.4.2. Video device registration

Next you register the video device with [`video_register_device()`](#c.video_register_device "video_register_device").
This will create the character device for you.

```
err = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
if (err) {
        video_device_release(vdev); /* or kfree(my_vdev); */
        return err;
}
```

If the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") parent device has a not `NULL` mdev field,
the video device entity will be automatically registered with the media
device.

Which device is registered depends on the type argument. The following
types exist:

| [`vfl_devnode_type`](#c.vfl_devnode_type "vfl_devnode_type") | Device name | Usage |
| --- | --- | --- |
| `VFL_TYPE_VIDEO` | `/dev/videoX` | for video input/output devices |
| `VFL_TYPE_VBI` | `/dev/vbiX` | for vertical blank data (i.e. closed captions, teletext) |
| `VFL_TYPE_RADIO` | `/dev/radioX` | for radio tuners |
| `VFL_TYPE_SUBDEV` | `/dev/v4l-subdevX` | for V4L2 subdevices |
| `VFL_TYPE_SDR` | `/dev/swradioX` | for Software Defined Radio (SDR) tuners |
| `VFL_TYPE_TOUCH` | `/dev/v4l-touchX` | for touch sensors |

The last argument gives you a certain amount of control over the device
node number used (i.e. the X in `videoX`). Normally you will pass -1
to let the v4l2 framework pick the first free number. But sometimes users
want to select a specific node number. It is common that drivers allow
the user to select a specific device node number through a driver module
option. That number is then passed to this function and video\_register\_device
will attempt to select that device node number. If that number was already
in use, then the next free device node number will be selected and it
will send a warning to the kernel log.

Another use-case is if a driver creates many devices. In that case it can
be useful to place different video devices in separate ranges. For example,
video capture devices start at 0, video output devices start at 16.
So you can use the last argument to specify a minimum device node number
and the v4l2 framework will try to pick the first free number that is equal
or higher to what you passed. If that fails, then it will just pick the
first free number.

Since in this case you do not care about a warning about not being able
to select the specified device node number, you can call the function
[`video_register_device_no_warn()`](#c.video_register_device_no_warn "video_register_device_no_warn") instead.

Whenever a device node is created some attributes are also created for you.
If you look in `/sys/class/video4linux` you see the devices. Go into e.g.
`video0` and you will see ‘name’, ‘dev\_debug’ and ‘index’ attributes. The
‘name’ attribute is the ‘name’ field of the video\_device struct. The
‘dev\_debug’ attribute can be used to enable core debugging. See the next
section for more detailed information on this.

The ‘index’ attribute is the index of the device node: for each call to
[`video_register_device()`](#c.video_register_device "video_register_device") the index is just increased by 1. The
first video device node you register always starts with index 0.

Users can setup udev rules that utilize the index attribute to make fancy
device names (e.g. ‘`mpegX`’ for MPEG video capture device nodes).

After the device was successfully registered, then you can use these fields:

* [`video_device`](#c.video_device "video_device")->vfl\_type: the device type passed to
  [`video_register_device()`](#c.video_register_device "video_register_device").
* [`video_device`](#c.video_device "video_device")->minor: the assigned device minor number.
* [`video_device`](#c.video_device "video_device")->num: the device node number (i.e. the X in
  `videoX`).
* [`video_device`](#c.video_device "video_device")->index: the device index number.

If the registration failed, then you need to call
[`video_device_release()`](#c.video_device_release "video_device_release") to free the allocated [`video_device`](#c.video_device "video_device")
struct, or free your own struct if the [`video_device`](#c.video_device "video_device") was embedded in
it. The `vdev->release()` callback will never be called if the registration
failed, nor should you ever attempt to unregister the device if the
registration failed.

## 3.4.3. video device debugging

The ‘dev\_debug’ attribute that is created for each video, vbi, radio or swradio
device in `/sys/class/video4linux/<devX>/` allows you to enable logging of
file operations.

It is a bitmask and the following bits can be set:

| Mask | Description |
| --- | --- |
| 0x01 | Log the ioctl name and error code. VIDIOC\_(D)QBUF ioctls are only logged if bit 0x08 is also set. |
| 0x02 | Log the ioctl name arguments and error code. VIDIOC\_(D)QBUF ioctls are only logged if bit 0x08 is also set. |
| 0x04 | Log the file operations open, release, read, write, mmap and get\_unmapped\_area. The read and write operations are only logged if bit 0x08 is also set. |
| 0x08 | Log the read and write file operations and the VIDIOC\_QBUF and VIDIOC\_DQBUF ioctls. |
| 0x10 | Log the poll file operation. |
| 0x20 | Log error and messages in the control operations. |

## 3.4.4. Video device cleanup

When the video device nodes have to be removed, either during the unload
of the driver or because the USB device was disconnected, then you should
unregister them with:

> [`video_unregister_device()`](#c.video_unregister_device "video_unregister_device")
> ([`vdev`](#c.video_device "video_device"));

This will remove the device nodes from sysfs (causing udev to remove them
from `/dev`).

After [`video_unregister_device()`](#c.video_unregister_device "video_unregister_device") returns no new opens can be done.
However, in the case of USB devices some application might still have one of
these device nodes open. So after the unregister all file operations (except
release, of course) will return an error as well.

When the last user of the video device node exits, then the `vdev->release()`
callback is called and you can do the final cleanup there.

Don’t forget to cleanup the media entity associated with the video device if
it has been initialized:

> [`media_entity_cleanup`](mc-core.html#c.media_entity_cleanup "media_entity_cleanup")
> (&vdev->entity);

This can be done from the release callback.

## 3.4.5. helper functions

There are a few useful helper functions:

* file and [`video_device`](#c.video_device "video_device") private data

You can set/get driver private data in the video\_device `struct using`:

> [`video_get_drvdata`](#c.video_get_drvdata "video_get_drvdata")
> ([`vdev`](#c.video_device "video_device"));
>
> [`video_set_drvdata`](#c.video_set_drvdata "video_set_drvdata")
> ([`vdev`](#c.video_device "video_device"));

Note that you can safely call [`video_set_drvdata()`](#c.video_set_drvdata "video_set_drvdata") before calling
[`video_register_device()`](#c.video_register_device "video_register_device").

And this function:

> [`video_devdata`](#c.video_devdata "video_devdata")
> ([`struct file`](../../filesystems/api-summary.html#c.file "file") \*file);

returns the video\_device belonging to the file struct.

The [`video_devdata()`](#c.video_devdata "video_devdata") function combines [`video_get_drvdata()`](#c.video_get_drvdata "video_get_drvdata")
with [`video_devdata()`](#c.video_devdata "video_devdata"):

> [`video_drvdata`](#c.video_drvdata "video_drvdata")
> ([`struct file`](../../filesystems/api-summary.html#c.file "file") \*file);

You can go from a [`video_device`](#c.video_device "video_device") `struct to` the v4l2\_device `struct using`:

```
struct v4l2_device *v4l2_dev = vdev->v4l2_dev;
```

* Device node name

The [`video_device`](#c.video_device "video_device") node kernel name can be retrieved using:

> [`video_device_node_name`](#c.video_device_node_name "video_device_node_name")
> ([`vdev`](#c.video_device "video_device"));

The name is used as a hint by userspace tools such as udev. The function
should be used where possible instead of accessing the video\_device::num and
video\_device::minor fields.

## 3.4.6. video\_device functions and data structures

enum vfl\_devnode\_type
:   type of V4L2 device node

**Constants**

`VFL_TYPE_VIDEO`
:   for video input/output devices

`VFL_TYPE_VBI`
:   for vertical blank data (i.e. closed captions, teletext)

`VFL_TYPE_RADIO`
:   for radio tuners

`VFL_TYPE_SUBDEV`
:   for V4L2 subdevices

`VFL_TYPE_SDR`
:   for Software Defined Radio tuners

`VFL_TYPE_TOUCH`
:   for touch sensors

`VFL_TYPE_MAX`
:   number of VFL types, must always be last in the enum

enum vfl\_devnode\_direction
:   Identifies if a [`struct video_device`](#c.video_device "video_device") corresponds to a receiver, a transmitter or a mem-to-mem device.

> **Constants**
>
> `VFL_DIR_RX`
> :   device is a receiver.
>
> `VFL_DIR_TX`
> :   device is a transmitter.
>
> `VFL_DIR_M2M`
> :   device is a memory to memory device.

**Note**

Ignored if [`enum vfl_devnode_type`](#c.vfl_devnode_type "vfl_devnode_type") is `VFL_TYPE_SUBDEV`.

enum v4l2\_video\_device\_flags
:   Flags used by [`struct video_device`](#c.video_device "video_device")

**Constants**

`V4L2_FL_REGISTERED`
:   indicates that a [`struct video_device`](#c.video_device "video_device") is registered.
    Drivers can clear this flag if they want to block all future
    device access. It is cleared by video\_unregister\_device.

`V4L2_FL_USES_V4L2_FH`
:   indicates that file->private\_data points to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh").
    This flag is set by the core when [`v4l2_fh_init()`](v4l2-fh.html#c.v4l2_fh_init "v4l2_fh_init") is called.
    All drivers must use it.

`V4L2_FL_QUIRK_INVERTED_CROP`
:   some old M2M drivers use g/s\_crop/cropcap incorrectly: crop and
    compose are swapped. If this flag is set, then the selection
    targets are swapped in the g/s\_crop/cropcap functions in v4l2-ioctl.c.
    This allows those drivers to correctly implement the selection API,
    but the old crop API will still work as expected in order to preserve
    backwards compatibility.
    Never set this flag for new drivers.

`V4L2_FL_SUBDEV_RO_DEVNODE`
:   indicates that the video device node is registered in read-only mode.
    The flag only applies to device nodes registered for sub-devices, it is
    set by the core when the sub-devices device nodes are registered with
    [`v4l2_device_register_ro_subdev_nodes()`](v4l2-device.html#c.v4l2_device_register_ro_subdev_nodes "v4l2_device_register_ro_subdev_nodes") and used by the sub-device ioctl
    handler to restrict access to some ioctl calls.

struct v4l2\_prio\_state
:   stores the priority states

> **Definition**:
>
> ```
> struct v4l2_prio_state {
>       atomic_t prios[4];
> };
> ```

**Members**

`prios`
:   array with elements to store the array priorities

**Description**

Note

The size of **prios** array matches the number of priority types defined
by enum `v4l2_priority`.

void v4l2\_prio\_init(struct [v4l2\_prio\_state](#c.v4l2_prio_state "v4l2_prio_state") \*global)
:   initializes a [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state")

**Parameters**

`struct v4l2_prio_state *global`
:   pointer to [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state")

int v4l2\_prio\_change(struct [v4l2\_prio\_state](#c.v4l2_prio_state "v4l2_prio_state") \*global, enum v4l2\_priority \*local, enum v4l2\_priority new)
:   changes the v4l2 file handler priority

**Parameters**

`struct v4l2_prio_state *global`
:   pointer to the [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state") of the device node.

`enum v4l2_priority *local`
:   pointer to the desired priority, as defined by enum `v4l2_priority`

`enum v4l2_priority new`
:   Priority type requested, as defined by enum `v4l2_priority`.

**Description**

Note

This function should be used only by the V4L2 core.

void v4l2\_prio\_open(struct [v4l2\_prio\_state](#c.v4l2_prio_state "v4l2_prio_state") \*global, enum v4l2\_priority \*local)
:   Implements the priority logic for a file handler open

**Parameters**

`struct v4l2_prio_state *global`
:   pointer to the [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state") of the device node.

`enum v4l2_priority *local`
:   pointer to the desired priority, as defined by enum `v4l2_priority`

**Description**

Note

This function should be used only by the V4L2 core.

void v4l2\_prio\_close(struct [v4l2\_prio\_state](#c.v4l2_prio_state "v4l2_prio_state") \*global, enum v4l2\_priority local)
:   Implements the priority logic for a file handler close

**Parameters**

`struct v4l2_prio_state *global`
:   pointer to the [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state") of the device node.

`enum v4l2_priority local`
:   priority to be released, as defined by enum `v4l2_priority`

**Description**

Note

This function should be used only by the V4L2 core.

enum v4l2\_priority v4l2\_prio\_max(struct [v4l2\_prio\_state](#c.v4l2_prio_state "v4l2_prio_state") \*global)
:   Return the maximum priority, as stored at the **global** array.

**Parameters**

`struct v4l2_prio_state *global`
:   pointer to the [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state") of the device node.

**Description**

Note

This function should be used only by the V4L2 core.

int v4l2\_prio\_check(struct [v4l2\_prio\_state](#c.v4l2_prio_state "v4l2_prio_state") \*global, enum v4l2\_priority local)
:   Implements the priority logic for a file handler close

**Parameters**

`struct v4l2_prio_state *global`
:   pointer to the [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state") of the device node.

`enum v4l2_priority local`
:   desired priority, as defined by enum `v4l2_priority` local

**Description**

Note

This function should be used only by the V4L2 core.

struct v4l2\_file\_operations
:   fs operations used by a V4L2 device

> **Definition**:
>
> ```
> struct v4l2_file_operations {
>       struct module *owner;
>       ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
>       ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
>       __poll_t (*poll) (struct file *, struct poll_table_struct *);
>       long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
> #ifdef CONFIG_COMPAT;
>       long (*compat_ioctl32) (struct file *, unsigned int, unsigned long);
> #endif;
>       unsigned long (*get_unmapped_area) (struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
>       int (*mmap) (struct file *, struct vm_area_struct *);
>       int (*open) (struct file *);
>       int (*release) (struct file *);
> };
> ```

**Members**

`owner`
:   pointer to `struct module`

`read`
:   operations needed to implement the read() syscall

`write`
:   operations needed to implement the write() syscall

`poll`
:   operations needed to implement the poll() syscall

`unlocked_ioctl`
:   operations needed to implement the ioctl() syscall

`compat_ioctl32`
:   operations needed to implement the ioctl() syscall for
    the special case where the Kernel uses 64 bits instructions, but
    the userspace uses 32 bits.

`get_unmapped_area`
:   called by the mmap() syscall, used when %!CONFIG\_MMU

`mmap`
:   operations needed to implement the mmap() syscall

`open`
:   operations needed to implement the open() syscall

`release`
:   operations needed to implement the `release()` syscall

**Description**

Note

Those operations are used to implemente the fs `struct file_operations`
at the V4L2 drivers. The V4L2 core overrides the fs ops with some
extra logic needed by the subsystem.

struct video\_device
:   Structure used to create and manage the V4L2 device nodes.

> **Definition**:
>
> ```
> struct video_device {
> #if defined(CONFIG_MEDIA_CONTROLLER);
>       struct media_entity entity;
>       struct media_intf_devnode *intf_devnode;
>       struct media_pipeline pipe;
> #endif;
>       const struct v4l2_file_operations *fops;
>       u32 device_caps;
>       struct device dev;
>       struct cdev *cdev;
>       struct v4l2_device *v4l2_dev;
>       struct device *dev_parent;
>       struct v4l2_ctrl_handler *ctrl_handler;
>       struct vb2_queue *queue;
>       struct v4l2_prio_state *prio;
>       char name[64];
>       enum vfl_devnode_type vfl_type;
>       enum vfl_devnode_direction vfl_dir;
>       int minor;
>       u16 num;
>       unsigned long flags;
>       int index;
>       spinlock_t fh_lock;
>       struct list_head        fh_list;
>       int dev_debug;
>       v4l2_std_id tvnorms;
>       void (*release)(struct video_device *vdev);
>       const struct v4l2_ioctl_ops *ioctl_ops;
>       unsigned long valid_ioctls[BITS_TO_LONGS( BASE_VIDIOC_PRIVATE)];
>       struct mutex *lock;
> };
> ```

**Members**

`entity`
:   [`struct media_entity`](mc-core.html#c.media_entity "media_entity")

`intf_devnode`
:   pointer to [`struct media_intf_devnode`](mc-core.html#c.media_intf_devnode "media_intf_devnode")

`pipe`
:   [`struct media_pipeline`](mc-core.html#c.media_pipeline "media_pipeline")

`fops`
:   pointer to [`struct v4l2_file_operations`](#c.v4l2_file_operations "v4l2_file_operations") for the video device

`device_caps`
:   device capabilities as used in v4l2\_capabilities

`dev`
:   [`struct device`](../infrastructure.html#c.device "device") for the video device

`cdev`
:   character device

`v4l2_dev`
:   pointer to [`struct v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") parent

`dev_parent`
:   pointer to [`struct device`](../infrastructure.html#c.device "device") parent

`ctrl_handler`
:   Control handler associated with this device node.
    May be NULL.

`queue`
:   [`struct vb2_queue`](v4l2-videobuf2.html#c.vb2_queue "vb2_queue") associated with this device node. May be NULL.

`prio`
:   pointer to [`struct v4l2_prio_state`](#c.v4l2_prio_state "v4l2_prio_state") with device’s Priority state.
    If NULL, then v4l2\_dev->prio will be used.

`name`
:   video device name

`vfl_type`
:   V4L device type, as defined by [`enum vfl_devnode_type`](#c.vfl_devnode_type "vfl_devnode_type")

`vfl_dir`
:   V4L receiver, transmitter or m2m

`minor`
:   device node ‘minor’. It is set to -1 if the registration failed

`num`
:   number of the video device node

`flags`
:   video device flags. Use bitops to set/clear/test flags.
    Contains a set of [`enum v4l2_video_device_flags`](#c.v4l2_video_device_flags "v4l2_video_device_flags").

`index`
:   attribute to differentiate multiple indices on one physical device

`fh_lock`
:   Lock for all v4l2\_fhs

`fh_list`
:   List of [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`dev_debug`
:   Internal device debug flags, not for use by drivers

`tvnorms`
:   Supported tv norms

`release`
:   video device `release()` callback

`ioctl_ops`
:   pointer to [`struct v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops") with ioctl callbacks

`valid_ioctls`
:   bitmap with the valid ioctls for this device

`lock`
:   pointer to `struct mutex` serialization lock

**Description**

Note

Only set **dev\_parent** if that can’t be deduced from **v4l2\_dev**.

media\_entity\_to\_video\_device

`media_entity_to_video_device (__entity)`

> Returns a [`struct video_device`](#c.video_device "video_device") from the [`struct media_entity`](mc-core.html#c.media_entity "media_entity") embedded on it.

**Parameters**

`__entity`
:   pointer to [`struct media_entity`](mc-core.html#c.media_entity "media_entity"), may be NULL

to\_video\_device

`to_video_device (cd)`

> Returns a [`struct video_device`](#c.video_device "video_device") from the [`struct device`](../infrastructure.html#c.device "device") embedded on it.

**Parameters**

`cd`
:   pointer to [`struct device`](../infrastructure.html#c.device "device")

int \_\_video\_register\_device(struct [video\_device](#c.video_device "video_device") \*vdev, enum [vfl\_devnode\_type](#c.vfl_devnode_type "vfl_devnode_type") type, int nr, int warn\_if\_nr\_in\_use, struct module \*owner)
:   register video4linux devices

**Parameters**

`struct video_device *vdev`
:   [`struct video_device`](#c.video_device "video_device") to register

`enum vfl_devnode_type type`
:   type of device to register, as defined by [`enum vfl_devnode_type`](#c.vfl_devnode_type "vfl_devnode_type")

`int nr`
:   which device node number is desired:
    (0 == /dev/video0, 1 == /dev/video1, ..., -1 == first free)

`int warn_if_nr_in_use`
:   warn if the desired device node number
    was already in use and another number was chosen instead.

`struct module *owner`
:   module that owns the video device node

**Description**

The registration code assigns minor numbers and device node numbers
based on the requested type and registers the new device node with
the kernel.

This function assumes that [`struct video_device`](#c.video_device "video_device") was zeroed when it
was allocated and does not contain any stale date.

An error is returned if no free minor or device node number could be
found, or if the registration of the device node failed.

Returns 0 on success.

Note

This function is meant to be used only inside the V4L2 core.
Drivers should use [`video_register_device()`](#c.video_register_device "video_register_device") or
[`video_register_device_no_warn()`](#c.video_register_device_no_warn "video_register_device_no_warn").

int video\_register\_device(struct [video\_device](#c.video_device "video_device") \*vdev, enum [vfl\_devnode\_type](#c.vfl_devnode_type "vfl_devnode_type") type, int nr)
:   register video4linux devices

**Parameters**

`struct video_device *vdev`
:   [`struct video_device`](#c.video_device "video_device") to register

`enum vfl_devnode_type type`
:   type of device to register, as defined by [`enum vfl_devnode_type`](#c.vfl_devnode_type "vfl_devnode_type")

`int nr`
:   which device node number is desired:
    (0 == /dev/video0, 1 == /dev/video1, ..., -1 == first free)

**Description**

Internally, it calls [`__video_register_device()`](#c.__video_register_device "__video_register_device"). Please see its
documentation for more details.

Note

if video\_register\_device fails, the `release()` callback of
[`struct video_device`](#c.video_device "video_device") structure is *not* called, so the caller
is responsible for freeing any data. Usually that means that
you [`video_device_release()`](#c.video_device_release "video_device_release") should be called on failure.

int video\_register\_device\_no\_warn(struct [video\_device](#c.video_device "video_device") \*vdev, enum [vfl\_devnode\_type](#c.vfl_devnode_type "vfl_devnode_type") type, int nr)
:   register video4linux devices

**Parameters**

`struct video_device *vdev`
:   [`struct video_device`](#c.video_device "video_device") to register

`enum vfl_devnode_type type`
:   type of device to register, as defined by [`enum vfl_devnode_type`](#c.vfl_devnode_type "vfl_devnode_type")

`int nr`
:   which device node number is desired:
    (0 == /dev/video0, 1 == /dev/video1, ..., -1 == first free)

**Description**

This function is identical to [`video_register_device()`](#c.video_register_device "video_register_device") except that no
warning is issued if the desired device node number was already in use.

Internally, it calls [`__video_register_device()`](#c.__video_register_device "__video_register_device"). Please see its
documentation for more details.

Note

if video\_register\_device fails, the `release()` callback of
[`struct video_device`](#c.video_device "video_device") structure is *not* called, so the caller
is responsible for freeing any data. Usually that means that
you [`video_device_release()`](#c.video_device_release "video_device_release") should be called on failure.

void video\_unregister\_device(struct [video\_device](#c.video_device "video_device") \*vdev)
:   Unregister video devices.

**Parameters**

`struct video_device *vdev`
:   [`struct video_device`](#c.video_device "video_device") to register

**Description**

Does nothing if vdev == NULL or if [`video_is_registered()`](#c.video_is_registered "video_is_registered") returns false.

struct [video\_device](#c.video_device "video_device") \*video\_device\_alloc(void)
:   helper function to alloc [`struct video_device`](#c.video_device "video_device")

**Parameters**

`void`
:   no arguments

**Description**

Returns NULL if `-ENOMEM` or a [`struct video_device`](#c.video_device "video_device") on success.

void video\_device\_release(struct [video\_device](#c.video_device "video_device") \*vdev)
:   helper function to release [`struct video_device`](#c.video_device "video_device")

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

**Description**

Can also be used for video\_device->`release()`.

void video\_device\_release\_empty(struct [video\_device](#c.video_device "video_device") \*vdev)
:   helper function to implement the video\_device->`release()` callback.

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

**Description**

This release function does nothing.

It should be used when the video\_device is a static global struct.

Note

Having a static video\_device is a dubious construction at best.

void v4l2\_disable\_ioctl(struct [video\_device](#c.video_device "video_device") \*vdev, unsigned int cmd)
:   mark that a given command isn’t implemented. shouldn’t use core locking

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

`unsigned int cmd`
:   ioctl command

**Description**

This function allows drivers to provide just one v4l2\_ioctl\_ops struct, but
disable ioctls based on the specific card that is actually found.

Note

This must be called before video\_register\_device.
See also the comments for `determine_valid_ioctls()`.

void \*video\_get\_drvdata(struct [video\_device](#c.video_device "video_device") \*vdev)
:   gets private data from [`struct video_device`](#c.video_device "video_device").

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

**Description**

returns a pointer to the private data

void video\_set\_drvdata(struct [video\_device](#c.video_device "video_device") \*vdev, void \*data)
:   sets private data from [`struct video_device`](#c.video_device "video_device").

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

`void *data`
:   private data pointer

struct [video\_device](#c.video_device "video_device") \*video\_devdata(struct [file](#c.video_devdata "file") \*file)
:   gets [`struct video_device`](#c.video_device "video_device") from [`struct file`](../../filesystems/api-summary.html#c.file "file").

**Parameters**

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

void \*video\_drvdata(struct [file](#c.video_drvdata "file") \*file)
:   gets private data from [`struct video_device`](#c.video_device "video_device") using the [`struct file`](../../filesystems/api-summary.html#c.file "file").

**Parameters**

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

**Description**

This is function combines both [`video_get_drvdata()`](#c.video_get_drvdata "video_get_drvdata") and [`video_devdata()`](#c.video_devdata "video_devdata")
as this is used very often.

const char \*video\_device\_node\_name(struct [video\_device](#c.video_device "video_device") \*vdev)
:   returns the video device name

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

**Description**

Returns the device name string

int video\_is\_registered(struct [video\_device](#c.video_device "video_device") \*vdev)
:   returns true if the [`struct video_device`](#c.video_device "video_device") is registered.

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](#c.video_device "video_device")

struct dentry \*v4l2\_debugfs\_root(void)
:   returns the dentry of the top-level “v4l2” debugfs dir

**Parameters**

`void`
:   no arguments

**Description**

If this directory does not yet exist, then it will be created.

int video\_device\_pipeline\_start(struct [video\_device](#c.video_device "video_device") \*vdev, struct [media\_pipeline](mc-core.html#c.media_pipeline "media_pipeline") \*pipe)
:   Mark a pipeline as streaming

**Parameters**

`struct video_device *vdev`
:   Starting video device

`struct media_pipeline *pipe`
:   Media pipeline to be assigned to all entities in the pipeline.

**Description**

Mark all entities connected to a given video device through enabled links,
either directly or indirectly, as streaming. The given pipeline object is
assigned to every pad in the pipeline and stored in the media\_pad pipe
field.

Calls to this function can be nested, in which case the same number of
[`video_device_pipeline_stop()`](#c.video_device_pipeline_stop "video_device_pipeline_stop") calls will be required to stop streaming. The
pipeline pointer must be identical for all nested calls to
[`video_device_pipeline_start()`](#c.video_device_pipeline_start "video_device_pipeline_start").

The video device must contain a single pad.

This is a convenience wrapper around [`media_pipeline_start()`](mc-core.html#c.media_pipeline_start "media_pipeline_start").

int \_\_video\_device\_pipeline\_start(struct [video\_device](#c.video_device "video_device") \*vdev, struct [media\_pipeline](mc-core.html#c.media_pipeline "media_pipeline") \*pipe)
:   Mark a pipeline as streaming

**Parameters**

`struct video_device *vdev`
:   Starting video device

`struct media_pipeline *pipe`
:   Media pipeline to be assigned to all entities in the pipeline.

**Description**

..note:: This is the non-locking version of [`video_device_pipeline_start()`](#c.video_device_pipeline_start "video_device_pipeline_start")

The video device must contain a single pad.

This is a convenience wrapper around [`__media_pipeline_start()`](mc-core.html#c.__media_pipeline_start "__media_pipeline_start").

void video\_device\_pipeline\_stop(struct [video\_device](#c.video_device "video_device") \*vdev)
:   Mark a pipeline as not streaming

**Parameters**

`struct video_device *vdev`
:   Starting video device

**Description**

Mark all entities connected to a given video device through enabled links,
either directly or indirectly, as not streaming. The media\_pad pipe field
is reset to `NULL`.

If multiple calls to [`media_pipeline_start()`](mc-core.html#c.media_pipeline_start "media_pipeline_start") have been made, the same
number of calls to this function are required to mark the pipeline as not
streaming.

The video device must contain a single pad.

This is a convenience wrapper around [`media_pipeline_stop()`](mc-core.html#c.media_pipeline_stop "media_pipeline_stop").

void \_\_video\_device\_pipeline\_stop(struct [video\_device](#c.video_device "video_device") \*vdev)
:   Mark a pipeline as not streaming

**Parameters**

`struct video_device *vdev`
:   Starting video device

**Description**

Note

This is the non-locking version of [`media_pipeline_stop()`](mc-core.html#c.media_pipeline_stop "media_pipeline_stop")

The video device must contain a single pad.

This is a convenience wrapper around [`__media_pipeline_stop()`](mc-core.html#c.__media_pipeline_stop "__media_pipeline_stop").

int video\_device\_pipeline\_alloc\_start(struct [video\_device](#c.video_device "video_device") \*vdev)
:   Mark a pipeline as streaming

**Parameters**

`struct video_device *vdev`
:   Starting video device

**Description**

[`video_device_pipeline_alloc_start()`](#c.video_device_pipeline_alloc_start "video_device_pipeline_alloc_start") is similar to [`video_device_pipeline_start()`](#c.video_device_pipeline_start "video_device_pipeline_start")
but instead of working on a given pipeline the function will use an
existing pipeline if the video device is already part of a pipeline, or
allocate a new pipeline.

Calls to [`video_device_pipeline_alloc_start()`](#c.video_device_pipeline_alloc_start "video_device_pipeline_alloc_start") must be matched with
[`video_device_pipeline_stop()`](#c.video_device_pipeline_stop "video_device_pipeline_stop").

struct [media\_pipeline](mc-core.html#c.media_pipeline "media_pipeline") \*video\_device\_pipeline(struct [video\_device](#c.video_device "video_device") \*vdev)
:   Get the media pipeline a video device is part of

**Parameters**

`struct video_device *vdev`
:   The video device

**Description**

This function returns the media pipeline that a video device has been
associated with when constructing the pipeline with
[`video_device_pipeline_start()`](#c.video_device_pipeline_start "video_device_pipeline_start"). The pointer remains valid until
[`video_device_pipeline_stop()`](#c.video_device_pipeline_stop "video_device_pipeline_stop") is called.

The video device must contain a single pad.

This is a convenience wrapper around [`media_entity_pipeline()`](mc-core.html#c.media_entity_pipeline "media_entity_pipeline").

**Return**

The media\_pipeline the video device is part of, or NULL if the video
device is not part of any pipeline.
