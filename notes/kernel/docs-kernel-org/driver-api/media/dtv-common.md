# 4.1.Digital TV Common functions

> 출처(원문): https://docs.kernel.org/driver-api/media/dtv-common.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.1. Digital TV Common functions

## 4.1.1. DVB devices

Those functions are responsible for handling the DVB device nodes.

enum dvb\_device\_type
:   type of the Digital TV device

**Constants**

`DVB_DEVICE_SEC`
:   Digital TV standalone Common Interface (CI)

`DVB_DEVICE_FRONTEND`
:   Digital TV frontend.

`DVB_DEVICE_DEMUX`
:   Digital TV demux.

`DVB_DEVICE_DVR`
:   Digital TV digital video record (DVR).

`DVB_DEVICE_CA`
:   Digital TV Conditional Access (CA).

`DVB_DEVICE_NET`
:   Digital TV network.

`DVB_DEVICE_VIDEO`
:   Digital TV video decoder.
    Deprecated. Used only on av7110-av.

`DVB_DEVICE_AUDIO`
:   Digital TV audio decoder.
    Deprecated. Used only on av7110-av.

`DVB_DEVICE_OSD`
:   Digital TV On Screen Display (OSD).
    Deprecated. Used only on av7110.

struct dvb\_adapter
:   represents a Digital TV adapter using Linux DVB API

**Definition**:

```
struct dvb_adapter {
    int num;
    struct list_head list_head;
    struct list_head device_list;
    const char *name;
    u8 proposed_mac [6];
    void* priv;
    struct device *device;
    struct module *module;
    int mfe_shared;
    struct dvb_device *mfe_dvbdev;
    struct mutex mfe_lock;
#if defined(CONFIG_MEDIA_CONTROLLER_DVB);
    struct mutex mdev_lock;
    struct media_device *mdev;
    struct media_entity *conn;
    struct media_pad *conn_pads;
#endif;
};
```

**Members**

`num`
:   Number of the adapter

`list_head`
:   List with the DVB adapters

`device_list`
:   List with the DVB devices

`name`
:   Name of the adapter

`proposed_mac`
:   proposed MAC address for the adapter

`priv`
:   private data

`device`
:   pointer to [`struct device`](../infrastructure.html#c.device "device")

`module`
:   pointer to `struct module`

`mfe_shared`
:   indicates mutually exclusive frontends.
    1 = legacy exclusion behavior: blocking any open() call
    2 = enhanced exclusion behavior, emulating the standard
    behavior of busy frontends: allowing read-only sharing
    and otherwise returning immediately with -EBUSY when any
    of the frontends is already opened with write access.

`mfe_dvbdev`
:   Frontend device in use, in the case of MFE

`mfe_lock`
:   Lock to prevent using the other frontends when MFE is
    used.

`mdev_lock`
:   Protect access to the mdev pointer.

`mdev`
:   pointer to [`struct media_device`](mc-core.html#c.media_device "media_device"), used when the media
    controller is used.

`conn`
:   RF connector. Used only if the device has no separate
    tuner.

`conn_pads`
:   pointer to [`struct media_pad`](mc-core.html#c.media_pad "media_pad") associated with **conn**;

struct dvb\_device
:   represents a DVB device node

**Definition**:

```
struct dvb_device {
    struct list_head list_head;
    struct kref ref;
    const struct file_operations *fops;
    struct dvb_adapter *adapter;
    enum dvb_device_type type;
    int minor;
    u32 id;
    int readers;
    int writers;
    int users;
    wait_queue_head_t wait_queue;
    int (*kernel_ioctl)(struct file *file, unsigned int cmd, void *arg);
#if defined(CONFIG_MEDIA_CONTROLLER_DVB);
    const char *name;
    struct media_intf_devnode *intf_devnode;
    unsigned tsout_num_entities;
    struct media_entity *entity, *tsout_entity;
    struct media_pad *pads, *tsout_pads;
#endif;
    void *priv;
};
```

**Members**

`list_head`
:   List head with all DVB devices

`ref`
:   reference count for this device

`fops`
:   pointer to `struct file_operations`

`adapter`
:   pointer to the adapter that holds this device node

`type`
:   type of the device, as defined by [`enum dvb_device_type`](#c.dvb_device_type "dvb_device_type").

`minor`
:   devnode minor number. Major number is always DVB\_MAJOR.

`id`
:   device ID number, inside the adapter

`readers`
:   Initialized by the caller. Each call to open() in Read Only mode
    decreases this counter by one.

`writers`
:   Initialized by the caller. Each call to open() in Read/Write
    mode decreases this counter by one.

`users`
:   Initialized by the caller. Each call to open() in any mode
    decreases this counter by one.

`wait_queue`
:   wait queue, used to wait for certain events inside one of
    the DVB API callers

`kernel_ioctl`
:   callback function used to handle ioctl calls from userspace.

`name`
:   Name to be used for the device at the Media Controller

`intf_devnode`
:   Pointer to media\_intf\_devnode. Used by the dvbdev core to
    store the MC device node interface

`tsout_num_entities`
:   Number of Transport Stream output entities

`entity`
:   pointer to [`struct media_entity`](mc-core.html#c.media_entity "media_entity") associated with the device node

`tsout_entity`
:   array with MC entities associated to each TS output node

`pads`
:   pointer to [`struct media_pad`](mc-core.html#c.media_pad "media_pad") associated with **entity**;

`tsout_pads`
:   array with the source pads for each **tsout\_entity**

`priv`
:   private data

**Description**

This structure is used by the DVB core (frontend, CA, net, demux) in
order to create the device nodes. Usually, driver should not initialize
this `struct diretly`.

struct dvbdevfops\_node
:   fops nodes registered in dvbdevfops\_list

**Definition**:

```
struct dvbdevfops_node {
    struct file_operations *fops;
    enum dvb_device_type type;
    const struct dvb_device *template;
    struct list_head list_head;
};
```

**Members**

`fops`
:   Dynamically allocated fops for ->owner registration

`type`
:   type of dvb\_device

`template`
:   dvb\_device used for registration

`list_head`
:   list\_head for dvbdevfops\_list

struct [dvb\_device](#c.dvb_device "dvb_device") \*dvb\_device\_get(struct [dvb\_device](#c.dvb_device "dvb_device") \*dvbdev)
:   Increase dvb\_device reference

**Parameters**

`struct dvb_device *dvbdev`
:   pointer to [`struct dvb_device`](#c.dvb_device "dvb_device")

void dvb\_device\_put(struct [dvb\_device](#c.dvb_device "dvb_device") \*dvbdev)
:   Decrease dvb\_device reference

**Parameters**

`struct dvb_device *dvbdev`
:   pointer to [`struct dvb_device`](#c.dvb_device "dvb_device")

int dvb\_register\_adapter(struct [dvb\_adapter](#c.dvb_adapter "dvb_adapter") \*adap, const char \*name, struct [module](#c.dvb_register_adapter "module") \*module, struct [device](#c.dvb_register_adapter "device") \*device, short \*adapter\_nums)
:   Registers a new DVB adapter

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](#c.dvb_adapter "dvb_adapter")

`const char *name`
:   Adapter’s name

`struct module *module`
:   initialized with THIS\_MODULE at the caller

`struct device *device`
:   pointer to [`struct device`](../infrastructure.html#c.device "device") that corresponds to the device driver

`short *adapter_nums`
:   Array with a list of the numbers for **dvb\_register\_adapter**;
    to select among them. Typically, initialized with:
    DVB\_DEFINE\_MOD\_OPT\_ADAPTER\_NR(adapter\_nums)

int dvb\_unregister\_adapter(struct [dvb\_adapter](#c.dvb_adapter "dvb_adapter") \*adap)
:   Unregisters a DVB adapter

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](#c.dvb_adapter "dvb_adapter")

int dvb\_register\_device(struct [dvb\_adapter](#c.dvb_adapter "dvb_adapter") \*adap, struct [dvb\_device](#c.dvb_device "dvb_device") \*\*pdvbdev, const struct [dvb\_device](#c.dvb_device "dvb_device") \*template, void \*priv, enum [dvb\_device\_type](#c.dvb_device_type "dvb_device_type") type, int demux\_sink\_pads)
:   Registers a new DVB device

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](#c.dvb_adapter "dvb_adapter")

`struct dvb_device **pdvbdev`
:   pointer to the place where the new [`struct dvb_device`](#c.dvb_device "dvb_device") will be
    stored

`const struct dvb_device *template`
:   Template used to create `pdvbdev`;

`void *priv`
:   private data

`enum dvb_device_type type`
:   type of the device, as defined by [`enum dvb_device_type`](#c.dvb_device_type "dvb_device_type").

`int demux_sink_pads`
:   Number of demux outputs, to be used to create the TS
    outputs via the Media Controller.

void dvb\_remove\_device(struct [dvb\_device](#c.dvb_device "dvb_device") \*dvbdev)
:   Remove a registered DVB device

**Parameters**

`struct dvb_device *dvbdev`
:   pointer to [`struct dvb_device`](#c.dvb_device "dvb_device")

**Description**

This does not free memory. `dvb_free_device()` will do that when
reference counter is empty

void dvb\_unregister\_device(struct [dvb\_device](#c.dvb_device "dvb_device") \*dvbdev)
:   Unregisters a DVB device

**Parameters**

`struct dvb_device *dvbdev`
:   pointer to [`struct dvb_device`](#c.dvb_device "dvb_device")

int dvb\_create\_media\_graph(struct [dvb\_adapter](#c.dvb_adapter "dvb_adapter") \*adap, bool create\_rf\_connector)
:   Creates media graph for the Digital TV part of the device.

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](#c.dvb_adapter "dvb_adapter")

`bool create_rf_connector`
:   if true, it creates the RF connector too

**Description**

This function checks all DVB-related functions at the media controller
entities and creates the needed links for the media graph. It is
capable of working with multiple tuners or multiple frontends, but it
won’t create links if the device has multiple tuners and multiple frontends
or if the device has multiple muxes. In such case, the caller driver should
manually create the remaining links.

void dvb\_register\_media\_controller(struct [dvb\_adapter](#c.dvb_adapter "dvb_adapter") \*adap, struct [media\_device](mc-core.html#c.media_device "media_device") \*mdev)
:   registers a media controller at DVB adapter

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](#c.dvb_adapter "dvb_adapter")

`struct media_device *mdev`
:   pointer to [`struct media_device`](mc-core.html#c.media_device "media_device")

struct [media\_device](mc-core.html#c.media_device "media_device") \*dvb\_get\_media\_controller(struct [dvb\_adapter](#c.dvb_adapter "dvb_adapter") \*adap)
:   gets the associated media controller

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](#c.dvb_adapter "dvb_adapter")

int dvb\_generic\_open(struct [inode](#c.dvb_generic_open "inode") \*inode, struct [file](#c.dvb_generic_open "file") \*file)
:   Digital TV open function, used by DVB devices

**Parameters**

`struct inode *inode`
:   pointer to `struct inode`.

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file").

**Description**

Checks if a DVB devnode is still valid, and if the permissions are
OK and increment negative use count.

int dvb\_generic\_release(struct [inode](#c.dvb_generic_release "inode") \*inode, struct [file](#c.dvb_generic_release "file") \*file)
:   Digital TV close function, used by DVB devices

**Parameters**

`struct inode *inode`
:   pointer to `struct inode`.

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file").

**Description**

Checks if a DVB devnode is still valid, and if the permissions are
OK and decrement negative use count.

long dvb\_generic\_ioctl(struct [file](#c.dvb_generic_ioctl "file") \*file, unsigned int cmd, unsigned long arg)
:   Digital TV close function, used by DVB devices

**Parameters**

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file").

`unsigned int cmd`
:   Ioctl name.

`unsigned long arg`
:   Ioctl argument.

**Description**

Checks if a DVB devnode and `struct dvbdev`.kernel\_ioctl is still valid.
If so, calls [`dvb_usercopy()`](#c.dvb_usercopy "dvb_usercopy").

int dvb\_usercopy(struct [file](#c.dvb_usercopy "file") \*file, unsigned int cmd, unsigned long arg, int (\*func)(struct [file](#c.dvb_usercopy "file") \*file, unsigned int cmd, void \*arg))
:   copies data from/to userspace memory when an ioctl is issued.

**Parameters**

`struct file *file`
:   Pointer to struct [`file`](../../filesystems/api-summary.html#c.file "file").

`unsigned int cmd`
:   Ioctl name.

`unsigned long arg`
:   Ioctl argument.

`int (*func)(struct file *file, unsigned int cmd, void *arg)`
:   function that will actually handle the ioctl

**Description**

Ancillary function that uses ioctl direction and size to copy from
userspace. Then, it calls **func**, and, if needed, data is copied back
to userspace.

struct [i2c\_client](../i2c.html#c.i2c_client "i2c_client") \*dvb\_module\_probe(const char \*module\_name, const char \*name, struct i2c\_adapter \*adap, unsigned char addr, void \*platform\_data)
:   helper routine to probe an I2C module

**Parameters**

`const char *module_name`
:   Name of the I2C module to be probed

`const char *name`
:   Optional name for the I2C module. Used for debug purposes.
    If `NULL`, defaults to **module\_name**.

`struct i2c_adapter *adap`
:   pointer to `struct i2c_adapter` that describes the I2C adapter where
    the module will be bound.

`unsigned char addr`
:   I2C address of the adapter, in 7-bit notation.

`void *platform_data`
:   Platform data to be passed to the I2C module probed.

**Description**

This function binds an I2C device into the DVB core. Should be used by
all drivers that use I2C bus to control the hardware. A module bound
with [`dvb_module_probe()`](#c.dvb_module_probe "dvb_module_probe") should use [`dvb_module_release()`](#c.dvb_module_release "dvb_module_release") to unbind.

Note

In the past, DVB modules (mainly, frontends) were bound via [`dvb_attach()`](#c.dvb_attach "dvb_attach")
macro, with does an ugly hack, using I2C low level functions. Such
usage is deprecated and will be removed soon. Instead, use this routine.

**Return**

On success, return an [`struct i2c_client`](../i2c.html#c.i2c_client "i2c_client"), pointing to the bound
I2C device. `NULL` otherwise.

void dvb\_module\_release(struct [i2c\_client](../i2c.html#c.i2c_client "i2c_client") \*client)
:   releases an I2C device allocated with [`dvb_module_probe()`](#c.dvb_module_probe "dvb_module_probe").

**Parameters**

`struct i2c_client *client`
:   pointer to [`struct i2c_client`](../i2c.html#c.i2c_client "i2c_client") with the I2C client to be released.
    can be `NULL`.

**Description**

This function should be used to free all resources reserved by
[`dvb_module_probe()`](#c.dvb_module_probe "dvb_module_probe") and unbinding the I2C hardware.

dvb\_attach

`dvb_attach (FUNCTION, ARGS...)`

> attaches a DVB frontend into the DVB core.

**Parameters**

`FUNCTION`
:   function on a frontend module to be called.

`ARGS...`
:   **FUNCTION** arguments.

**Description**

This ancillary function loads a frontend module in runtime and runs
the **FUNCTION** function there, with **ARGS**.
As it increments symbol usage cont, at unregister, [`dvb_detach()`](#c.dvb_detach "dvb_detach")
should be called.

Note

In the past, DVB modules (mainly, frontends) were bound via [`dvb_attach()`](#c.dvb_attach "dvb_attach")
macro, with does an ugly hack, using I2C low level functions. Such
usage is deprecated and will be removed soon. Instead, you should use
[`dvb_module_probe()`](#c.dvb_module_probe "dvb_module_probe").

dvb\_detach

`dvb_detach (FUNC)`

> detaches a DVB frontend loaded via [`dvb_attach()`](#c.dvb_attach "dvb_attach")

**Parameters**

`FUNC`
:   attach function

**Description**

Decrements usage count for a function previously called via [`dvb_attach()`](#c.dvb_attach "dvb_attach").

## 4.1.2. Digital TV Ring buffer

Those routines implement ring buffers used to handle digital TV data and
copy it from/to userspace.

Note

1. For performance reasons read and write routines don’t check buffer sizes
   and/or number of bytes free/available. This has to be done before these
   routines are called. For example:

> ```
> /* write @buflen: bytes */
> free = dvb_ringbuffer_free(rbuf);
> if (free >= buflen)
>         count = dvb_ringbuffer_write(rbuf, buffer, buflen);
> else
>         /* do something */
>
> /* read min. 1000, max. @bufsize: bytes */
> avail = dvb_ringbuffer_avail(rbuf);
> if (avail >= 1000)
>         count = dvb_ringbuffer_read(rbuf, buffer, min(avail, bufsize));
> else
>         /* do something */
> ```

2. If there is exactly one reader and one writer, there is no need
   to lock read or write operations.
   Two or more readers must be locked against each other.
   Flushing the buffer counts as a read operation.
   Resetting the buffer counts as a read and write operation.
   Two or more writers must be locked against each other.

struct dvb\_ringbuffer
:   Describes a ring buffer used at DVB framework

**Definition**:

```
struct dvb_ringbuffer {
    u8 *data;
    ssize_t size;
    ssize_t pread;
    ssize_t pwrite;
    int error;
    wait_queue_head_t queue;
    spinlock_t lock;
};
```

**Members**

`data`
:   Area were the ringbuffer data is written

`size`
:   size of the ringbuffer

`pread`
:   next position to read

`pwrite`
:   next position to write

`error`
:   used by ringbuffer clients to indicate that an error happened.

`queue`
:   Wait queue used by ringbuffer clients to indicate when buffer
    was filled

`lock`
:   Spinlock used to protect the ringbuffer

void dvb\_ringbuffer\_init(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, void \*data, size\_t len)
:   initialize ring buffer, lock and queue

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`void *data`
:   pointer to the buffer where the data will be stored

`size_t len`
:   bytes from ring buffer into **buf**

int dvb\_ringbuffer\_empty(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf)
:   test whether buffer is empty

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

ssize\_t dvb\_ringbuffer\_free(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf)
:   returns the number of free bytes in the buffer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

**Return**

number of free bytes in the buffer

ssize\_t dvb\_ringbuffer\_avail(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf)
:   returns the number of bytes waiting in the buffer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

**Return**

number of bytes waiting in the buffer

void dvb\_ringbuffer\_reset(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf)
:   resets the ringbuffer to initial state

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

**Description**

Resets the read and write pointers to zero and flush the buffer.

This counts as a read and write operation

void dvb\_ringbuffer\_flush(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf)
:   flush buffer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

void dvb\_ringbuffer\_flush\_spinlock\_wakeup(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf)
:   flush buffer protected by spinlock and wake-up waiting task(s)

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

DVB\_RINGBUFFER\_PEEK

`DVB_RINGBUFFER_PEEK (rbuf, offs)`

> peek at byte **offs** in the buffer

**Parameters**

`rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`offs`
:   offset inside the ringbuffer

DVB\_RINGBUFFER\_SKIP

`DVB_RINGBUFFER_SKIP (rbuf, num)`

> advance read ptr by **num** bytes

**Parameters**

`rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`num`
:   number of bytes to advance

ssize\_t dvb\_ringbuffer\_read\_user(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, u8 \_\_user \*buf, size\_t len)
:   Reads a buffer into a user pointer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`u8 __user *buf`
:   pointer to the buffer where the data will be stored

`size_t len`
:   bytes from ring buffer into **buf**

**Description**

This variant assumes that the buffer is a memory at the userspace. So,
it will internally call `copy_to_user()`.

**Return**

number of bytes transferred or -EFAULT

void dvb\_ringbuffer\_read(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, u8 \*buf, size\_t len)
:   Reads a buffer into a pointer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`u8 *buf`
:   pointer to the buffer where the data will be stored

`size_t len`
:   bytes from ring buffer into **buf**

**Description**

This variant assumes that the buffer is a memory at the Kernel space

**Return**

number of bytes transferred or -EFAULT

DVB\_RINGBUFFER\_WRITE\_BYTE

`DVB_RINGBUFFER_WRITE_BYTE (rbuf, byte)`

> write single byte to ring buffer

**Parameters**

`rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`byte`
:   byte to write

ssize\_t dvb\_ringbuffer\_write(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, const u8 \*buf, size\_t len)
:   Writes a buffer into the ringbuffer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`const u8 *buf`
:   pointer to the buffer where the data will be read

`size_t len`
:   bytes from ring buffer into **buf**

**Description**

This variant assumes that the buffer is a memory at the Kernel space

**Return**

number of bytes transferred or -EFAULT

ssize\_t dvb\_ringbuffer\_write\_user(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, const u8 \_\_user \*buf, size\_t len)
:   Writes a buffer received via a user pointer

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   pointer to [`struct dvb_ringbuffer`](#c.dvb_ringbuffer "dvb_ringbuffer")

`const u8 __user *buf`
:   pointer to the buffer where the data will be read

`size_t len`
:   bytes from ring buffer into **buf**

**Description**

This variant assumes that the buffer is a memory at the userspace. So,
it will internally call `copy_from_user()`.

**Return**

number of bytes transferred or -EFAULT

ssize\_t dvb\_ringbuffer\_pkt\_write(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, u8 \*buf, size\_t len)
:   Write a packet into the ringbuffer.

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   Ringbuffer to write to.

`u8 *buf`
:   Buffer to write.

`size_t len`
:   Length of buffer (currently limited to 65535 bytes max).

**Return**

Number of bytes written, or -EFAULT, -ENOMEM, -EINVAL.

ssize\_t dvb\_ringbuffer\_pkt\_read\_user(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, size\_t idx, int offset, u8 \_\_user \*buf, size\_t len)
:   Read from a packet in the ringbuffer.

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   Ringbuffer concerned.

`size_t idx`
:   Packet index as returned by [`dvb_ringbuffer_pkt_next()`](#c.dvb_ringbuffer_pkt_next "dvb_ringbuffer_pkt_next").

`int offset`
:   Offset into packet to read from.

`u8 __user *buf`
:   Destination buffer for data.

`size_t len`
:   Size of destination buffer.

**Return**

Number of bytes read, or -EFAULT.

**Description**

Note

unlike [`dvb_ringbuffer_read()`](#c.dvb_ringbuffer_read "dvb_ringbuffer_read"), this does **NOT** update the read pointer
in the ringbuffer. You must use [`dvb_ringbuffer_pkt_dispose()`](#c.dvb_ringbuffer_pkt_dispose "dvb_ringbuffer_pkt_dispose") to mark a
packet as no longer required.

ssize\_t dvb\_ringbuffer\_pkt\_read(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, size\_t idx, int offset, u8 \*buf, size\_t len)
:   Read from a packet in the ringbuffer.

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   Ringbuffer concerned.

`size_t idx`
:   Packet index as returned by [`dvb_ringbuffer_pkt_next()`](#c.dvb_ringbuffer_pkt_next "dvb_ringbuffer_pkt_next").

`int offset`
:   Offset into packet to read from.

`u8 *buf`
:   Destination buffer for data.

`size_t len`
:   Size of destination buffer.

**Note**

unlike [`dvb_ringbuffer_read_user()`](#c.dvb_ringbuffer_read_user "dvb_ringbuffer_read_user"), this DOES update the read pointer
in the ringbuffer.

**Return**

Number of bytes read, or -EFAULT.

void dvb\_ringbuffer\_pkt\_dispose(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, size\_t idx)
:   Dispose of a packet in the ring buffer.

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   Ring buffer concerned.

`size_t idx`
:   Packet index as returned by [`dvb_ringbuffer_pkt_next()`](#c.dvb_ringbuffer_pkt_next "dvb_ringbuffer_pkt_next").

ssize\_t dvb\_ringbuffer\_pkt\_next(struct [dvb\_ringbuffer](#c.dvb_ringbuffer "dvb_ringbuffer") \*rbuf, size\_t idx, size\_t \*pktlen)
:   Get the index of the next packet in a ringbuffer.

**Parameters**

`struct dvb_ringbuffer *rbuf`
:   Ringbuffer concerned.

`size_t idx`
:   Previous packet index, or -1 to return the first packet index.

`size_t *pktlen`
:   On success, will be updated to contain the length of the packet
    in bytes.
    returns Packet index (if >=0), or -1 if no packets available.

## 4.1.3. Digital TV VB2 handler

enum dvb\_buf\_type
:   types of Digital TV memory-mapped buffers

**Constants**

`DVB_BUF_TYPE_CAPTURE`
:   buffer is filled by the Kernel,
    with a received Digital TV stream

enum dvb\_vb2\_states
:   states to control VB2 state machine

**Constants**

`DVB_VB2_STATE_NONE`
:   VB2 engine not initialized yet, init failed or VB2 was released.

`DVB_VB2_STATE_INIT`
:   VB2 engine initialized.

`DVB_VB2_STATE_REQBUFS`
:   Buffers were requested

`DVB_VB2_STATE_STREAMON`
:   VB2 is streaming. Callers should not check it directly. Instead,
    they should use [`dvb_vb2_is_streaming()`](#c.dvb_vb2_is_streaming "dvb_vb2_is_streaming").

**Note**

Callers should not touch at the state machine directly. This
is handled inside dvb\_vb2.c.

struct dvb\_buffer
:   video buffer information for v4l2.

**Definition**:

```
struct dvb_buffer {
    struct vb2_buffer       vb;
    struct list_head        list;
};
```

**Members**

`vb`
:   embedded struct [`vb2_buffer`](v4l2-videobuf2.html#c.vb2_buffer "vb2_buffer").

`list`
:   list of [`struct dvb_buffer`](#c.dvb_buffer "dvb_buffer").

struct dvb\_vb2\_ctx
:   control struct for VB2 handler

**Definition**:

```
struct dvb_vb2_ctx {
    struct vb2_queue        vb_q;
    spinlock_t slock;
    struct list_head        dvb_q;
    struct dvb_buffer       *buf;
    int offset;
    int remain;
    int state;
    int buf_siz;
    int buf_cnt;
    int nonblocking;
    enum dmx_buffer_flags flags;
    u32 count;
    char name[DVB_VB2_NAME_MAX + 1];
};
```

**Members**

`vb_q`
:   pointer to [`struct vb2_queue`](v4l2-videobuf2.html#c.vb2_queue "vb2_queue") with videobuf2 queue.

`slock`
:   spin lock used to protect buffer filling at dvb\_vb2.c.

`dvb_q`
:   List of buffers that are not filled yet.

`buf`
:   Pointer to the buffer that are currently being filled.

`offset`
:   index to the next position at the **buf** to be filled.

`remain`
:   How many bytes are left to be filled at **buf**.

`state`
:   bitmask of buffer states as defined by [`enum dvb_vb2_states`](#c.dvb_vb2_states "dvb_vb2_states").

`buf_siz`
:   size of each VB2 buffer.

`buf_cnt`
:   number of VB2 buffers.

`nonblocking`
:   If different than zero, device is operating on non-blocking
    mode.

`flags`
:   buffer flags as defined by `enum dmx_buffer_flags`.
    Filled only at `DMX_DQBUF`. `DMX_QBUF` should zero this field.

`count`
:   monotonic counter for filled buffers. Helps to identify
    data stream loses. Filled only at `DMX_DQBUF`. `DMX_QBUF` should
    zero this field.

`name`
:   name of the device type. Currently, it can either be
    “dvr” or “demux\_filter”.

int dvb\_vb2\_init(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, const char \*name, struct [mutex](#c.dvb_vb2_init "mutex") \*mutex, int non\_blocking)
:   initializes VB2 handler

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`const char *name`
:   name for the VB2 handler

`struct mutex *mutex`
:   pointer to the mutex that serializes vb2 ioctls

`int non_blocking`
:   if not zero, it means that the device is at non-blocking mode

int dvb\_vb2\_release(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx)
:   Releases the VB2 handler allocated resources and put **ctx** at DVB\_VB2\_STATE\_NONE state.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

int dvb\_vb2\_is\_streaming(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx)
:   checks if the VB2 handler is streaming

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

**Return**

0 if not streaming, 1 otherwise.

int dvb\_vb2\_fill\_buffer(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, const unsigned char \*src, int len, enum dmx\_buffer\_flags \*buffer\_flags, bool flush)
:   fills a VB2 buffer

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`const unsigned char *src`
:   place where the data is stored

`int len`
:   number of bytes to be copied from **src**

`enum dmx_buffer_flags *buffer_flags`
:   pointer to buffer flags as defined by `enum dmx_buffer_flags`.
    can be NULL.

`bool flush`
:   flush the buffer, even if it isn’t full.

\_\_poll\_t dvb\_vb2\_poll(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct [file](#c.dvb_vb2_poll "file") \*file, poll\_table \*wait)
:   Wrapper to [`vb2_core_streamon()`](v4l2-videobuf2.html#c.vb2_core_streamon "vb2_core_streamon") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct file *file`
:   [`struct file`](../../filesystems/api-summary.html#c.file "file") argument passed to the poll
    file operation handler.

`poll_table *wait`
:   `poll_table` wait argument passed to the poll
    file operation handler.

**Description**

Implements poll `syscall()` logic.

int dvb\_vb2\_stream\_on(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx)
:   Wrapper to [`vb2_core_streamon()`](v4l2-videobuf2.html#c.vb2_core_streamon "vb2_core_streamon") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

**Description**

Starts dvb streaming

int dvb\_vb2\_stream\_off(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx)
:   Wrapper to [`vb2_core_streamoff()`](v4l2-videobuf2.html#c.vb2_core_streamoff "vb2_core_streamoff") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

**Description**

Stops dvb streaming

int dvb\_vb2\_reqbufs(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct dmx\_requestbuffers \*req)
:   Wrapper to [`vb2_core_reqbufs()`](v4l2-videobuf2.html#c.vb2_core_reqbufs "vb2_core_reqbufs") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct dmx_requestbuffers *req`
:   `struct dmx_requestbuffers` passed from userspace in
    order to handle `DMX_REQBUFS`.

**Description**

Initiate streaming by requesting a number of buffers. Also used to
free previously requested buffers, is `req->count` is zero.

int dvb\_vb2\_querybuf(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct dmx\_buffer \*b)
:   Wrapper to [`vb2_core_querybuf()`](v4l2-videobuf2.html#c.vb2_core_querybuf "vb2_core_querybuf") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct dmx_buffer *b`
:   `struct dmx_buffer` passed from userspace in
    order to handle `DMX_QUERYBUF`.

int dvb\_vb2\_expbuf(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct dmx\_exportbuffer \*exp)
:   Wrapper to [`vb2_core_expbuf()`](v4l2-videobuf2.html#c.vb2_core_expbuf "vb2_core_expbuf") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct dmx_exportbuffer *exp`
:   `struct dmx_exportbuffer` passed from userspace in
    order to handle `DMX_EXPBUF`.

**Description**

Export a buffer as a file descriptor.

int dvb\_vb2\_qbuf(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct dmx\_buffer \*b)
:   Wrapper to [`vb2_core_qbuf()`](v4l2-videobuf2.html#c.vb2_core_qbuf "vb2_core_qbuf") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct dmx_buffer *b`
:   `struct dmx_buffer` passed from userspace in
    order to handle `DMX_QBUF`.

**Description**

Queue a Digital TV buffer as requested by userspace

int dvb\_vb2\_dqbuf(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct dmx\_buffer \*b)
:   Wrapper to [`vb2_core_dqbuf()`](v4l2-videobuf2.html#c.vb2_core_dqbuf "vb2_core_dqbuf") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct dmx_buffer *b`
:   `struct dmx_buffer` passed from userspace in
    order to handle `DMX_DQBUF`.

**Description**

Dequeue a Digital TV buffer to the userspace

int dvb\_vb2\_mmap(struct [dvb\_vb2\_ctx](#c.dvb_vb2_ctx "dvb_vb2_ctx") \*ctx, struct vm\_area\_struct \*vma)
:   Wrapper to [`vb2_mmap()`](v4l2-videobuf2.html#c.vb2_mmap "vb2_mmap") for Digital TV buffer handling.

**Parameters**

`struct dvb_vb2_ctx *ctx`
:   control struct for VB2 handler

`struct vm_area_struct *vma`
:   pointer to `struct vm_area_struct` with the vma passed
    to the mmap file operation handler in the driver.

**Description**

map Digital TV video buffers into application address space.
