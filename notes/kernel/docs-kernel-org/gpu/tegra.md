# drm/tegra NVIDIA Tegra GPU and display driver

> 출처(원문): https://docs.kernel.org/gpu/tegra.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/tegra NVIDIA Tegra GPU and display driver

NVIDIA Tegra SoCs support a set of display, graphics and video functions via
the host1x controller. host1x supplies command streams, gathered from a push
buffer provided directly by the CPU, to its clients via channels. Software,
or blocks amongst themselves, can use syncpoints for synchronization.

Up until, but not including, Tegra124 (aka Tegra K1) the drm/tegra driver
supports the built-in GPU, comprised of the gr2d and gr3d engines. Starting
with Tegra124 the GPU is based on the NVIDIA desktop GPU architecture and
supported by the drm/nouveau driver.

The drm/tegra driver supports NVIDIA Tegra SoC generations since Tegra20. It
has three parts:

> * A host1x driver that provides infrastructure and access to the host1x
>   services.
> * A KMS driver that supports the display controllers as well as a number of
>   outputs, such as RGB, HDMI, DSI, and DisplayPort.
> * A set of custom userspace IOCTLs that can be used to submit jobs to the
>   GPU and video engines via host1x.

## Driver Infrastructure

The various host1x clients need to be bound together into a logical device in
order to expose their functionality to users. The infrastructure that supports
this is implemented in the host1x driver. When a driver is registered with the
infrastructure it provides a list of compatible strings specifying the devices
that it needs. The infrastructure creates a logical device and scan the device
tree for matching device nodes, adding the required clients to a list. Drivers
for individual clients register with the infrastructure as well and are added
to the logical host1x device.

Once all clients are available, the infrastructure will initialize the logical
device using a driver-provided function which will set up the bits specific to
the subsystem and in turn initialize each of its clients.

Similarly, when one of the clients is unregistered, the infrastructure will
destroy the logical device by calling back into the driver, which ensures that
the subsystem specific bits are torn down and the clients destroyed in turn.

### Host1x Infrastructure Reference

struct host1x\_bo\_cache
:   host1x buffer object cache

**Definition**:

```
struct host1x_bo_cache {
    struct list_head mappings;
    struct mutex lock;
};
```

**Members**

`mappings`
:   list of mappings

`lock`
:   synchronizes accesses to the list of mappings

**Description**

Note that entries are not periodically evicted from this cache and instead need to be
explicitly released. This is used primarily for DRM/KMS where the cache’s reference is
released when the last reference to a buffer object represented by a mapping in this
cache is dropped.

struct host1x\_client\_ops
:   host1x client operations

**Definition**:

```
struct host1x_client_ops {
    int (*early_init)(struct host1x_client *client);
    int (*init)(struct host1x_client *client);
    int (*exit)(struct host1x_client *client);
    int (*late_exit)(struct host1x_client *client);
    int (*suspend)(struct host1x_client *client);
    int (*resume)(struct host1x_client *client);
};
```

**Members**

`early_init`
:   host1x client early initialization code

`init`
:   host1x client initialization code

`exit`
:   host1x client tear down code

`late_exit`
:   host1x client late tear down code

`suspend`
:   host1x client suspend code

`resume`
:   host1x client resume code

struct host1x\_client
:   host1x client structure

**Definition**:

```
struct host1x_client {
    struct list_head list;
    struct device *host;
    struct device *dev;
    struct iommu_group *group;
    const struct host1x_client_ops *ops;
    enum host1x_class class;
    struct host1x_channel *channel;
    struct host1x_syncpt **syncpts;
    unsigned int num_syncpts;
    struct host1x_client *parent;
    unsigned int usecount;
    struct mutex lock;
    struct host1x_bo_cache cache;
};
```

**Members**

`list`
:   list node for the host1x client

`host`
:   pointer to [`struct device`](../driver-api/infrastructure.html#c.device "device") representing the host1x controller

`dev`
:   pointer to [`struct device`](../driver-api/infrastructure.html#c.device "device") backing this host1x client

`group`
:   IOMMU group that this client is a member of

`ops`
:   host1x client operations

`class`
:   host1x class represented by this client

`channel`
:   host1x channel associated with this client

`syncpts`
:   array of syncpoints requested for this client

`num_syncpts`
:   number of syncpoints requested for this client

`parent`
:   pointer to parent structure

`usecount`
:   reference count for this structure

`lock`
:   mutex for mutually exclusive concurrency

`cache`
:   host1x buffer object cache

struct host1x\_driver
:   host1x logical device driver

**Definition**:

```
struct host1x_driver {
    struct device_driver driver;
    const struct of_device_id *subdevs;
    struct list_head list;
    int (*probe)(struct host1x_device *device);
    void (*remove)(struct host1x_device *device);
    void (*shutdown)(struct host1x_device *device);
};
```

**Members**

`driver`
:   core driver

`subdevs`
:   table of OF device IDs matching subdevices for this driver

`list`
:   list node for the driver

`probe`
:   called when the host1x logical device is probed

`remove`
:   called when the host1x logical device is removed

`shutdown`
:   called when the host1x logical device is shut down

int host1x\_device\_init(struct host1x\_device \*device)
:   initialize a host1x logical device

**Parameters**

`struct host1x_device *device`
:   host1x logical device

**Description**

The driver for the host1x logical device can call this during execution of
its [`host1x_driver.probe`](#c.host1x_driver "host1x_driver") implementation to initialize each of its clients.
The client drivers access the subsystem specific driver data using the
[`host1x_client.parent`](#c.host1x_client "host1x_client") field and driver data associated with it (usually by
calling `dev_get_drvdata()`).

int host1x\_device\_exit(struct host1x\_device \*device)
:   uninitialize host1x logical device

**Parameters**

`struct host1x_device *device`
:   host1x logical device

**Description**

When the driver for a host1x logical device is unloaded, it can call this
function to tear down each of its clients. Typically this is done after a
subsystem-specific data structure is removed and the functionality can no
longer be used.

int host1x\_driver\_register\_full(struct [host1x\_driver](#c.host1x_driver "host1x_driver") \*driver, struct module \*owner)
:   register a host1x driver

**Parameters**

`struct host1x_driver *driver`
:   host1x driver

`struct module *owner`
:   owner module

**Description**

Drivers for host1x logical devices call this function to register a driver
with the infrastructure. Note that since these drive logical devices, the
registration of the driver actually triggers tho logical device creation.
A logical device will be created for each host1x instance.

void host1x\_driver\_unregister(struct [host1x\_driver](#c.host1x_driver "host1x_driver") \*driver)
:   unregister a host1x driver

**Parameters**

`struct host1x_driver *driver`
:   host1x driver

**Description**

Unbinds the driver from each of the host1x logical devices that it is
bound to, effectively removing the subsystem devices that they represent.

void \_\_host1x\_client\_init(struct [host1x\_client](#c.host1x_client "host1x_client") \*client, struct lock\_class\_key \*key)
:   initialize a host1x client

**Parameters**

`struct host1x_client *client`
:   host1x client

`struct lock_class_key *key`
:   lock class key for the client-specific mutex

void host1x\_client\_exit(struct [host1x\_client](#c.host1x_client "host1x_client") \*client)
:   uninitialize a host1x client

**Parameters**

`struct host1x_client *client`
:   host1x client

int \_\_host1x\_client\_register(struct [host1x\_client](#c.host1x_client "host1x_client") \*client)
:   register a host1x client

**Parameters**

`struct host1x_client *client`
:   host1x client

**Description**

Registers a host1x client with each host1x controller instance. Note that
each client will only match their parent host1x controller and will only be
associated with that instance. Once all clients have been registered with
their parent host1x controller, the infrastructure will set up the logical
device and call [`host1x_device_init()`](#c.host1x_device_init "host1x_device_init"), which will in turn call each client’s
[`host1x_client_ops.init`](#c.host1x_client_ops "host1x_client_ops") implementation.

void host1x\_client\_unregister(struct [host1x\_client](#c.host1x_client "host1x_client") \*client)
:   unregister a host1x client

**Parameters**

`struct host1x_client *client`
:   host1x client

**Description**

Removes a host1x client from its host1x controller instance. If a logical
device has already been initialized, it will be torn down.

### Host1x Syncpoint Reference

struct host1x\_syncpt \*host1x\_syncpt\_alloc(struct host1x \*host, unsigned long flags, const char \*name)
:   allocate a syncpoint

**Parameters**

`struct host1x *host`
:   host1x device data

`unsigned long flags`
:   bitfield of HOST1X\_SYNCPT\_\* flags

`const char *name`
:   name for the syncpoint for use in debug prints

**Description**

Allocates a hardware syncpoint for the caller’s use. The caller then has
the sole authority to mutate the syncpoint’s value until it is freed again.

If no free syncpoints are available, or a NULL name was specified, returns
NULL.

u32 host1x\_syncpt\_id(struct host1x\_syncpt \*sp)
:   retrieve syncpoint ID

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

**Description**

Given a pointer to a `struct host1x_syncpt`, retrieves its ID. This ID is
often used as a value to program into registers that control how hardware
blocks interact with syncpoints.

u32 host1x\_syncpt\_incr\_max(struct host1x\_syncpt \*sp, u32 incrs)
:   update the value sent to hardware

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

`u32 incrs`
:   number of increments

int host1x\_syncpt\_incr(struct host1x\_syncpt \*sp)
:   increment syncpoint value from CPU, updating cache

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

int host1x\_syncpt\_wait(struct host1x\_syncpt \*sp, u32 thresh, long timeout, u32 \*value)
:   wait for a syncpoint to reach a given value

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

`u32 thresh`
:   threshold

`long timeout`
:   maximum time to wait for the syncpoint to reach the given value

`u32 *value`
:   return location for the syncpoint value

struct host1x\_syncpt \*host1x\_syncpt\_request(struct [host1x\_client](#c.host1x_client "host1x_client") \*client, unsigned long flags)
:   request a syncpoint

**Parameters**

`struct host1x_client *client`
:   client requesting the syncpoint

`unsigned long flags`
:   flags

**Description**

host1x client drivers can use this function to allocate a syncpoint for
subsequent use. A syncpoint returned by this function will be reserved for
use by the client exclusively. When no longer using a syncpoint, a host1x
client driver needs to release it using [`host1x_syncpt_put()`](#c.host1x_syncpt_put "host1x_syncpt_put").

void host1x\_syncpt\_put(struct host1x\_syncpt \*sp)
:   free a requested syncpoint

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

**Description**

Release a syncpoint previously allocated using [`host1x_syncpt_request()`](#c.host1x_syncpt_request "host1x_syncpt_request"). A
host1x client driver should call this when the syncpoint is no longer in
use.

u32 host1x\_syncpt\_read\_max(struct host1x\_syncpt \*sp)
:   read maximum syncpoint value

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

**Description**

The maximum syncpoint value indicates how many operations there are in
queue, either in channel or in a software thread.

u32 host1x\_syncpt\_read\_min(struct host1x\_syncpt \*sp)
:   read minimum syncpoint value

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

**Description**

The minimum syncpoint value is a shadow of the current sync point value in
hardware.

u32 host1x\_syncpt\_read(struct host1x\_syncpt \*sp)
:   read the current syncpoint value

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

struct host1x\_syncpt \*host1x\_syncpt\_get\_by\_id(struct host1x \*host, unsigned int id)
:   obtain a syncpoint by ID

**Parameters**

`struct host1x *host`
:   host1x controller

`unsigned int id`
:   syncpoint ID

struct host1x\_syncpt \*host1x\_syncpt\_get\_by\_id\_noref(struct host1x \*host, unsigned int id)
:   obtain a syncpoint by ID but don’t increase the refcount.

**Parameters**

`struct host1x *host`
:   host1x controller

`unsigned int id`
:   syncpoint ID

struct host1x\_syncpt \*host1x\_syncpt\_get(struct host1x\_syncpt \*sp)
:   increment syncpoint refcount

**Parameters**

`struct host1x_syncpt *sp`
:   syncpoint

struct host1x\_syncpt\_base \*host1x\_syncpt\_get\_base(struct host1x\_syncpt \*sp)
:   obtain the wait base associated with a syncpoint

**Parameters**

`struct host1x_syncpt *sp`
:   host1x syncpoint

u32 host1x\_syncpt\_base\_id(struct host1x\_syncpt\_base \*base)
:   retrieve the ID of a syncpoint wait base

**Parameters**

`struct host1x_syncpt_base *base`
:   host1x syncpoint wait base

void host1x\_syncpt\_release\_vblank\_reservation(struct [host1x\_client](#c.host1x_client "host1x_client") \*client, u32 syncpt\_id)
:   Make VBLANK syncpoint available for allocation

**Parameters**

`struct host1x_client *client`
:   host1x bus client

`u32 syncpt_id`
:   syncpoint ID to make available

**Description**

Makes VBLANK<i> syncpoint available for allocatation if it was
reserved at initialization time. This should be called by the display
driver after it has ensured that any VBLANK increment programming configured
by the boot chain has been disabled.

## KMS driver

The display hardware has remained mostly backwards compatible over the various
Tegra SoC generations, up until Tegra186 which introduces several changes that
make it difficult to support with a parameterized driver.

### Display Controllers

Tegra SoCs have two display controllers, each of which can be associated with
zero or more outputs. Outputs can also share a single display controller, but
only if they run with compatible display timings. Two display controllers can
also share a single framebuffer, allowing cloned configurations even if modes
on two outputs don’t match. A display controller is modelled as a CRTC in KMS
terms.

On Tegra186, the number of display controllers has been increased to three. A
display controller can no longer drive all of the outputs. While two of these
controllers can drive both DSI outputs and both SOR outputs, the third cannot
drive any DSI.

#### Windows

A display controller controls a set of windows that can be used to composite
multiple buffers onto the screen. While it is possible to assign arbitrary Z
ordering to individual windows (by programming the corresponding blending
registers), this is currently not supported by the driver. Instead, it will
assume a fixed Z ordering of the windows (window A is the root window, that
is, the lowest, while windows B and C are overlaid on top of window A). The
overlay windows support multiple pixel formats and can automatically convert
from YUV to RGB at scanout time. This makes them useful for displaying video
content. In KMS, each window is modelled as a plane. Each display controller
has a hardware cursor that is exposed as a cursor plane.

### Outputs

The type and number of supported outputs varies between Tegra SoC generations.
All generations support at least HDMI. While earlier generations supported the
very simple RGB interfaces (one per display controller), recent generations no
longer do and instead provide standard interfaces such as DSI and eDP/DP.

Outputs are modelled as a composite encoder/connector pair.

#### RGB/LVDS

This interface is no longer available since Tegra124. It has been replaced by
the more standard DSI and eDP interfaces.

#### HDMI

HDMI is supported on all Tegra SoCs. Starting with Tegra210, HDMI is provided
by the versatile SOR output, which supports eDP, DP and HDMI. The SOR is able
to support HDMI 2.0, though support for this is currently not merged.

#### DSI

Although Tegra has supported DSI since Tegra30, the controller has changed in
several ways in Tegra114. Since none of the publicly available development
boards prior to Dalmore (Tegra114) have made use of DSI, only Tegra114 and
later are supported by the drm/tegra driver.

#### eDP/DP

eDP was first introduced in Tegra124 where it was used to drive the display
panel for notebook form factors. Tegra210 added support for full DisplayPort
support, though this is currently not implemented in the drm/tegra driver.

## Userspace Interface

The userspace interface provided by drm/tegra allows applications to create
GEM buffers, access and control syncpoints as well as submit command streams
to host1x.

### GEM Buffers

The `DRM_IOCTL_TEGRA_GEM_CREATE` IOCTL is used to create a GEM buffer object
with Tegra-specific flags. This is useful for buffers that should be tiled, or
that are to be scanned out upside down (useful for 3D content).

After a GEM buffer object has been created, its memory can be mapped by an
application using the mmap offset returned by the `DRM_IOCTL_TEGRA_GEM_MMAP`
IOCTL.

### Syncpoints

The current value of a syncpoint can be obtained by executing the
`DRM_IOCTL_TEGRA_SYNCPT_READ` IOCTL. Incrementing the syncpoint is achieved
using the `DRM_IOCTL_TEGRA_SYNCPT_INCR` IOCTL.

Userspace can also request blocking on a syncpoint. To do so, it needs to
execute the `DRM_IOCTL_TEGRA_SYNCPT_WAIT` IOCTL, specifying the value of
the syncpoint to wait for. The kernel will release the application when the
syncpoint reaches that value or after a specified timeout.

### Command Stream Submission

Before an application can submit command streams to host1x it needs to open a
channel to an engine using the `DRM_IOCTL_TEGRA_OPEN_CHANNEL` IOCTL. Client
IDs are used to identify the target of the channel. When a channel is no
longer needed, it can be closed using the `DRM_IOCTL_TEGRA_CLOSE_CHANNEL`
IOCTL. To retrieve the syncpoint associated with a channel, an application
can use the `DRM_IOCTL_TEGRA_GET_SYNCPT`.

After opening a channel, submitting command streams is easy. The application
writes commands into the memory backing a GEM buffer object and passes these
to the `DRM_IOCTL_TEGRA_SUBMIT` IOCTL along with various other parameters,
such as the syncpoints or relocations used in the job submission.
