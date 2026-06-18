# Core elements

> 출처(원문): https://docs.kernel.org/driver-api/iio/core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Core elements

The Industrial I/O core offers both a unified framework for writing drivers for
many different types of embedded sensors and a standard interface to user space
applications manipulating sensors. The implementation can be found under
`drivers/iio/industrialio-*`

## Industrial I/O Devices

* [`struct iio_dev`](#c.iio_dev "iio_dev") - industrial I/O device
* [`iio_device_alloc()`](#c.iio_device_alloc "iio_device_alloc") - allocate an [`iio_dev`](#c.iio_dev "iio_dev") from a driver
* [`iio_device_free()`](#c.iio_device_free "iio_device_free") - free an [`iio_dev`](#c.iio_dev "iio_dev") from a driver
* [`iio_device_register()`](#c.iio_device_register "iio_device_register") - register a device with the IIO subsystem
* [`iio_device_unregister()`](#c.iio_device_unregister "iio_device_unregister") - unregister a device from the IIO
  subsystem

An IIO device usually corresponds to a single hardware sensor and it
provides all the information needed by a driver handling a device.
Let’s first have a look at the functionality embedded in an IIO device
then we will show how a device driver makes use of an IIO device.

There are two ways for a user space application to interact with an IIO driver.

1. `/sys/bus/iio/devices/iio:deviceX/`, this represents a hardware sensor
   and groups together the data channels of the same chip.
2. `/dev/iio:deviceX`, character device node interface used for
   buffered data transfer and for events information retrieval.

A typical IIO driver will register itself as an [I2C](../i2c.html) or
[SPI](../spi.html) driver and will create two routines, probe and remove.

At probe:

1. Call [`iio_device_alloc()`](#c.iio_device_alloc "iio_device_alloc"), which allocates memory for an IIO device.
2. Initialize IIO device fields with driver specific information (e.g.
   device name, device channels).
3. Call [`iio_device_register()`](#c.iio_device_register "iio_device_register"), this registers the device with the
   IIO core. After this call the device is ready to accept requests from user
   space applications.

At remove, we free the resources allocated in probe in reverse order:

1. [`iio_device_unregister()`](#c.iio_device_unregister "iio_device_unregister"), unregister the device from the IIO core.
2. [`iio_device_free()`](#c.iio_device_free "iio_device_free"), free the memory allocated for the IIO device.

### IIO device sysfs interface

Attributes are sysfs files used to expose chip info and also allowing
applications to set various configuration parameters. For device with
index X, attributes can be found under /sys/bus/iio/devices/iio:deviceX/
directory. Common attributes are:

* `name`, description of the physical chip.
* `dev`, shows the major:minor pair associated with
  `/dev/iio:deviceX` node.
* `sampling_frequency_available`, available discrete set of sampling
  frequency values for device.
* Available standard attributes for IIO devices are described in the
  :<file:Documentation/ABI/testing/sysfs-bus-iio> file in the Linux kernel
  sources.

### IIO device channels

[`struct iio_chan_spec`](#c.iio_chan_spec "iio_chan_spec") - specification of a single channel

An IIO device channel is a representation of a data channel. An IIO device can
have one or multiple channels. For example:

* a thermometer sensor has one channel representing the temperature measurement.
* a light sensor with two channels indicating the measurements in the visible
  and infrared spectrum.
* an accelerometer can have up to 3 channels representing acceleration on X, Y
  and Z axes.

An IIO channel is described by the [`struct iio_chan_spec`](#c.iio_chan_spec "iio_chan_spec").
A thermometer driver for the temperature sensor in the example above would
have to describe its channel as follows:

```
static const struct iio_chan_spec temp_channel[] = {
     {
         .type = IIO_TEMP,
         .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
     },
};
```

Channel sysfs attributes exposed to userspace are specified in the form of
bitmasks. Depending on their shared info, attributes can be set in one of the
following masks:

* **info\_mask\_separate**, attributes will be specific to
  this channel
* **info\_mask\_shared\_by\_type**, attributes are shared by all channels of the
  same type
* **info\_mask\_shared\_by\_dir**, attributes are shared by all channels of the same
  direction
* **info\_mask\_shared\_by\_all**, attributes are shared by all channels

When there are multiple data channels per channel type we have two ways to
distinguish between them:

* set **.modified** field of [`iio_chan_spec`](#c.iio_chan_spec "iio_chan_spec") to 1. Modifiers are
  specified using **.channel2** field of the same [`iio_chan_spec`](#c.iio_chan_spec "iio_chan_spec")
  structure and are used to indicate a physically unique characteristic of the
  channel such as its direction or spectral response. For example, a light
  sensor can have two channels, one for infrared light and one for both
  infrared and visible light.
* set **.indexed** field of [`iio_chan_spec`](#c.iio_chan_spec "iio_chan_spec") to 1. In this case the
  channel is simply another instance with an index specified by the **.channel**
  field.

Here is how we can make use of the channel’s modifiers:

```
static const struct iio_chan_spec light_channels[] = {
        {
                .type = IIO_INTENSITY,
                .modified = 1,
                .channel2 = IIO_MOD_LIGHT_IR,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
                .info_mask_shared = BIT(IIO_CHAN_INFO_SAMP_FREQ),
        },
        {
                .type = IIO_INTENSITY,
                .modified = 1,
                .channel2 = IIO_MOD_LIGHT_BOTH,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
                .info_mask_shared = BIT(IIO_CHAN_INFO_SAMP_FREQ),
        },
        {
                .type = IIO_LIGHT,
                .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
                .info_mask_shared = BIT(IIO_CHAN_INFO_SAMP_FREQ),
        },
   }
```

This channel’s definition will generate two separate sysfs files for raw data
retrieval:

* `/sys/bus/iio/devices/iio:deviceX/in_intensity_ir_raw`
* `/sys/bus/iio/devices/iio:deviceX/in_intensity_both_raw`

one file for processed data:

* `/sys/bus/iio/devices/iio:deviceX/in_illuminance_input`

and one shared sysfs file for sampling frequency:

* `/sys/bus/iio/devices/iio:deviceX/sampling_frequency`.

Here is how we can make use of the channel’s indexing:

```
static const struct iio_chan_spec light_channels[] = {
        {
                .type = IIO_VOLTAGE,
                .indexed = 1,
                .channel = 0,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        },
        {
                .type = IIO_VOLTAGE,
                .indexed = 1,
                .channel = 1,
                .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        },
}
```

This will generate two separate attributes files for raw data retrieval:

* `/sys/bus/iio/devices/iio:deviceX/in_voltage0_raw`, representing
  voltage measurement for channel 0.
* `/sys/bus/iio/devices/iio:deviceX/in_voltage1_raw`, representing
  voltage measurement for channel 1.

### More details

struct iio\_chan\_spec\_ext\_info
:   Extended channel info attribute

**Definition**:

```
struct iio_chan_spec_ext_info {
    const char *name;
    enum iio_shared_by shared;
    ssize_t (*read)(struct iio_dev *, uintptr_t private, struct iio_chan_spec const *, char *buf);
    ssize_t (*write)(struct iio_dev *, uintptr_t private, struct iio_chan_spec const *, const char *buf, size_t len);
    uintptr_t private;
};
```

**Members**

`name`
:   Info attribute name

`shared`
:   Whether this attribute is shared between all channels.

`read`
:   Read callback for this info attribute, may be NULL.

`write`
:   Write callback for this info attribute, may be NULL.

`private`
:   Data private to the driver.

struct iio\_enum
:   Enum channel info attribute

**Definition**:

```
struct iio_enum {
    const char * const *items;
    unsigned int num_items;
    int (*set)(struct iio_dev *, const struct iio_chan_spec *, unsigned int);
    int (*get)(struct iio_dev *, const struct iio_chan_spec *);
};
```

**Members**

`items`
:   An array of strings.

`num_items`
:   Length of the item array.

`set`
:   Set callback function, may be NULL.

`get`
:   Get callback function, may be NULL.

**Description**

The iio\_enum `struct can` be used to implement `enum style` channel attributes.
Enum style attributes are those which have a set of strings which map to
unsigned integer values. The IIO `enum helper` code takes care of mapping
between value and string as well as generating a “\_available” file which
contains a list of all available items. The set callback will be called when
the attribute is updated. The last parameter is the index to the newly
activated item. The get callback will be used to query the currently active
item and is supposed to return the index for it.

IIO\_ENUM

`IIO_ENUM (_name, _shared, _e)`

> Initialize `enum extended` channel attribute

**Parameters**

`_name`
:   Attribute name

`_shared`
:   Whether the attribute is shared between all channels

`_e`
:   Pointer to an iio\_enum struct

**Description**

This should usually be used together with [`IIO_ENUM_AVAILABLE()`](#c.IIO_ENUM_AVAILABLE "IIO_ENUM_AVAILABLE")

IIO\_ENUM\_AVAILABLE

`IIO_ENUM_AVAILABLE (_name, _shared, _e)`

> Initialize `enum available` extended channel attribute

**Parameters**

`_name`
:   Attribute name (“\_available” will be appended to the name)

`_shared`
:   Whether the attribute is shared between all channels

`_e`
:   Pointer to an iio\_enum struct

**Description**

Creates a read only attribute which lists all the available `enum items` in a
space separated list. This should usually be used together with [`IIO_ENUM()`](#c.IIO_ENUM "IIO_ENUM")

struct iio\_mount\_matrix
:   iio mounting matrix

**Definition**:

```
struct iio_mount_matrix {
    const char *rotation[9];
};
```

**Members**

`rotation`
:   3 dimensional space rotation matrix defining sensor alignment with
    main hardware

IIO\_MOUNT\_MATRIX

`IIO_MOUNT_MATRIX (_shared, _get)`

> Initialize mount matrix extended channel attribute

**Parameters**

`_shared`
:   Whether the attribute is shared between all channels

`_get`
:   Pointer to an iio\_get\_mount\_matrix\_t accessor

struct iio\_event\_spec
:   specification for a channel event

**Definition**:

```
struct iio_event_spec {
    enum iio_event_type type;
    enum iio_event_direction dir;
    unsigned long mask_separate;
    unsigned long mask_shared_by_type;
    unsigned long mask_shared_by_dir;
    unsigned long mask_shared_by_all;
};
```

**Members**

`type`
:   Type of the event

`dir`
:   Direction of the event

`mask_separate`
:   Bit mask of `enum iio_event_info` values. Attributes
    set in this mask will be registered per channel.

`mask_shared_by_type`
:   Bit mask of `enum iio_event_info` values. Attributes
    set in this mask will be shared by channel type.

`mask_shared_by_dir`
:   Bit mask of `enum iio_event_info` values. Attributes
    set in this mask will be shared by channel type and
    direction.

`mask_shared_by_all`
:   Bit mask of `enum iio_event_info` values. Attributes
    set in this mask will be shared by all channels.

struct iio\_scan\_type
:   specification for channel data format in buffer

**Definition**:

```
struct iio_scan_type {
    char sign;
    u8 realbits;
    u8 storagebits;
    u8 shift;
    u8 repeat;
    enum iio_endian endianness;
};
```

**Members**

`sign`
:   ‘s’ or ‘u’ to specify signed or unsigned

`realbits`
:   Number of valid bits of data

`storagebits`
:   Realbits + padding

`shift`
:   Shift right by this before masking out realbits.

`repeat`
:   Number of times real/storage bits repeats. When the
    repeat element is more than 1, then the type element in
    sysfs will show a repeat value. Otherwise, the number
    of repetitions is omitted.

`endianness`
:   little or big endian

struct iio\_chan\_spec
:   specification of a single channel

**Definition**:

```
struct iio_chan_spec {
    enum iio_chan_type      type;
    int channel;
    int channel2;
    unsigned long           address;
    int scan_index;
    union {
        struct iio_scan_type scan_type;
        struct {
            const struct iio_scan_type *ext_scan_type;
            unsigned int num_ext_scan_type;
        };
    };
    unsigned long                   info_mask_separate;
    unsigned long                   info_mask_separate_available;
    unsigned long                   info_mask_shared_by_type;
    unsigned long                   info_mask_shared_by_type_available;
    unsigned long                   info_mask_shared_by_dir;
    unsigned long                   info_mask_shared_by_dir_available;
    unsigned long                   info_mask_shared_by_all;
    unsigned long                   info_mask_shared_by_all_available;
    const struct iio_event_spec *event_spec;
    unsigned int            num_event_specs;
    const struct iio_chan_spec_ext_info *ext_info;
    const char              *extend_name;
    const char              *datasheet_name;
    unsigned int            modified:1;
    unsigned int            indexed:1;
    unsigned int            output:1;
    unsigned int            differential:1;
    unsigned int            has_ext_scan_type:1;
};
```

**Members**

`type`
:   What type of measurement is the channel making.

`channel`
:   What number do we wish to assign the channel.

`channel2`
:   If there is a second number for a differential
    channel then this is it. If modified is set then the
    value here specifies the modifier.

`address`
:   Driver specific identifier.

`scan_index`
:   Monotonic index to give ordering in scans when read
    from a buffer.

`{unnamed_union}`
:   anonymous

`scan_type`
:   `struct describing` the scan type - mutually exclusive
    with ext\_scan\_type.

`{unnamed_struct}`
:   anonymous

`ext_scan_type`
:   Used in rare cases where there is more than one scan
    format for a channel. When this is used, the flag
    has\_ext\_scan\_type must be set and the driver must
    implement get\_current\_scan\_type in [`struct iio_info`](#c.iio_info "iio_info").

`num_ext_scan_type`
:   Number of elements in ext\_scan\_type.

`info_mask_separate`
:   What information is to be exported that is specific to
    this channel.

`info_mask_separate_available`
:   What availability information is to be
    exported that is specific to this channel.

`info_mask_shared_by_type`
:   What information is to be exported that is shared
    by all channels of the same type.

`info_mask_shared_by_type_available`
:   What availability information is to be
    exported that is shared by all channels of the same
    type.

`info_mask_shared_by_dir`
:   What information is to be exported that is shared
    by all channels of the same direction.

`info_mask_shared_by_dir_available`
:   What availability information is to be
    exported that is shared by all channels of the same
    direction.

`info_mask_shared_by_all`
:   What information is to be exported that is shared
    by all channels.

`info_mask_shared_by_all_available`
:   What availability information is to be
    exported that is shared by all channels.

`event_spec`
:   Array of events which should be registered for this
    channel.

`num_event_specs`
:   Size of the event\_spec array.

`ext_info`
:   Array of extended info attributes for this channel.
    The array is NULL terminated, the last element should
    have its name field set to NULL.

`extend_name`
:   Allows labeling of channel attributes with an
    informative name. Note this has no effect codes etc,
    unlike modifiers.
    This field is deprecated in favour of providing
    iio\_info->`read_label()` to override the label, which
    unlike **extend\_name** does not affect sysfs filenames.

`datasheet_name`
:   A name used in in-kernel mapping of channels. It should
    correspond to the first name that the channel is referred
    to by in the datasheet (e.g. IND), or the nearest
    possible compound name (e.g. IND-INC).

`modified`
:   Does a modifier apply to this channel. What these are
    depends on the channel type. Modifier is set in
    channel2. Examples are IIO\_MOD\_X for axial sensors about
    the ‘x’ axis.

`indexed`
:   Specify the channel has a numerical index. If not,
    the channel index number will be suppressed for sysfs
    attributes but not for event codes.

`output`
:   Channel is output.

`differential`
:   Channel is differential.

`has_ext_scan_type`
:   True if ext\_scan\_type is used instead of scan\_type.

bool iio\_channel\_has\_info(const struct [iio\_chan\_spec](#c.iio_chan_spec "iio_chan_spec") \*chan, enum iio\_chan\_info\_enum type)
:   Checks whether a channel supports a info attribute

**Parameters**

`const struct iio_chan_spec *chan`
:   The channel to be queried

`enum iio_chan_info_enum type`
:   Type of the info attribute to be checked

**Description**

Returns true if the channels supports reporting values for the given info
attribute type, false otherwise.

bool iio\_channel\_has\_available(const struct [iio\_chan\_spec](#c.iio_chan_spec "iio_chan_spec") \*chan, enum iio\_chan\_info\_enum type)
:   Checks if a channel has an available attribute

**Parameters**

`const struct iio_chan_spec *chan`
:   The channel to be queried

`enum iio_chan_info_enum type`
:   Type of the available attribute to be checked

**Description**

Returns true if the channel supports reporting available values for the
given attribute type, false otherwise.

struct iio\_info
:   constant information about device

**Definition**:

```
struct iio_info {
    const struct attribute_group    *event_attrs;
    const struct attribute_group    *attrs;
    int (*read_raw)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask);
    int (*read_raw_multi)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int max_len, int *vals, int *val_len, long mask);
    int (*read_avail)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, const int **vals, int *type, int *length, long mask);
    int (*write_raw)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int val, int val2, long mask);
    int (*read_label)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, char *label);
    int (*write_raw_get_fmt)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, long mask);
    int (*read_event_config)(struct iio_dev *indio_dev, const struct iio_chan_spec *chan, enum iio_event_type type, enum iio_event_direction dir);
    int (*write_event_config)(struct iio_dev *indio_dev, const struct iio_chan_spec *chan, enum iio_event_type type, enum iio_event_direction dir, bool state);
    int (*read_event_value)(struct iio_dev *indio_dev, const struct iio_chan_spec *chan, enum iio_event_type type, enum iio_event_direction dir, enum iio_event_info info, int *val, int *val2);
    int (*write_event_value)(struct iio_dev *indio_dev, const struct iio_chan_spec *chan, enum iio_event_type type, enum iio_event_direction dir, enum iio_event_info info, int val, int val2);
    int (*read_event_label)(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, enum iio_event_type type, enum iio_event_direction dir, char *label);
    int (*validate_trigger)(struct iio_dev *indio_dev, struct iio_trigger *trig);
    int (*get_current_scan_type)(const struct iio_dev *indio_dev, const struct iio_chan_spec *chan);
    int (*update_scan_mode)(struct iio_dev *indio_dev, const unsigned long *scan_mask);
    int (*debugfs_reg_access)(struct iio_dev *indio_dev, unsigned int reg, unsigned int writeval, unsigned int *readval);
    int (*fwnode_xlate)(struct iio_dev *indio_dev, const struct fwnode_reference_args *iiospec);
    int (*hwfifo_set_watermark)(struct iio_dev *indio_dev, unsigned int val);
    int (*hwfifo_flush_to_buffer)(struct iio_dev *indio_dev, unsigned int count);
};
```

**Members**

`event_attrs`
:   event control attributes

`attrs`
:   general purpose device attributes

`read_raw`
:   function to request a value from the device.
    mask specifies which value. Note 0 means a reading of
    the channel in question. Return value will specify the
    type of value returned by the device. val and val2 will
    contain the elements making up the returned value.

`read_raw_multi`
:   function to return values from the device.
    mask specifies which value. Note 0 means a reading of
    the channel in question. Return value will specify the
    type of value returned by the device. vals pointer
    contain the elements making up the returned value.
    max\_len specifies maximum number of elements
    vals pointer can contain. val\_len is used to return
    length of valid elements in vals.

`read_avail`
:   function to return the available values from the device.
    mask specifies which value. Note 0 means the available
    values for the channel in question. Return value
    specifies if a IIO\_AVAIL\_LIST or a IIO\_AVAIL\_RANGE is
    returned in vals. The type of the vals are returned in
    type and the number of vals is returned in length. For
    ranges, there are always three vals returned; min, step
    and max. For lists, all possible values are enumerated.

`write_raw`
:   function to write a value to the device.
    Parameters are the same as for read\_raw.

`read_label`
:   function to request label name for a specified label,
    for better channel identification.

`write_raw_get_fmt`
:   callback function to query the expected
    format/precision. If not set by the driver, write\_raw
    returns IIO\_VAL\_INT\_PLUS\_MICRO.

`read_event_config`
:   find out if the event is enabled.

`write_event_config`
:   set if the event is enabled.

`read_event_value`
:   read a configuration value associated with the event.

`write_event_value`
:   write a configuration value for the event.

`read_event_label`
:   function to request label name for a specified label,
    for better event identification.

`validate_trigger`
:   function to validate the trigger when the
    current trigger gets changed.

`get_current_scan_type`
:   must be implemented by drivers that use ext\_scan\_type
    in the channel spec to return the index of the currently
    active ext\_scan type for a channel.

`update_scan_mode`
:   function to configure device and scan buffer when
    channels have changed

`debugfs_reg_access`
:   function to read or write register value of device

`fwnode_xlate`
:   fwnode based function pointer to obtain channel specifier index.

`hwfifo_set_watermark`
:   function pointer to set the current hardware
    fifo watermark level; see hwfifo\_\* entries in
    [ABI file testing/sysfs-bus-iio](../../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-bus-iio) for details on
    how the hardware fifo operates

`hwfifo_flush_to_buffer`
:   function pointer to flush the samples stored
    in the hardware fifo to the device buffer. The driver
    should not flush more than count samples. The function
    must return the number of samples flushed, 0 if no
    samples were flushed or a negative integer if no samples
    were flushed and there was an error.

struct iio\_buffer\_setup\_ops
:   buffer setup related callbacks

**Definition**:

```
struct iio_buffer_setup_ops {
    int (*preenable)(struct iio_dev *);
    int (*postenable)(struct iio_dev *);
    int (*predisable)(struct iio_dev *);
    int (*postdisable)(struct iio_dev *);
    bool (*validate_scan_mask)(struct iio_dev *indio_dev, const unsigned long *scan_mask);
};
```

**Members**

`preenable`
:   [DRIVER] function to run prior to marking buffer enabled

`postenable`
:   [DRIVER] function to run after marking buffer enabled

`predisable`
:   [DRIVER] function to run prior to marking buffer
    disabled

`postdisable`
:   [DRIVER] function to run after marking buffer disabled

`validate_scan_mask`
:   [DRIVER] function callback to check whether a given
    scan mask is valid for the device.

struct iio\_dev
:   industrial I/O device

**Definition**:

```
struct iio_dev {
    int modes;
    struct device                   dev;
    struct iio_buffer               *buffer;
    int scan_bytes;
    const unsigned long             *available_scan_masks;
    unsigned int                    masklength;
    const unsigned long             *active_scan_mask;
    bool scan_timestamp;
    struct iio_trigger              *trig;
    struct iio_poll_func            *pollfunc;
    struct iio_poll_func            *pollfunc_event;
    struct iio_chan_spec const      *channels;
    int num_channels;
    const char                      *name;
    const char                      *label;
    const struct iio_info           *info;
    const struct iio_buffer_setup_ops       *setup_ops;
    void *priv;
};
```

**Members**

`modes`
:   [DRIVER] bitmask listing all the operating modes
    supported by the IIO device. This list should be
    initialized before registering the IIO device. It can
    also be filed up by the IIO core, as a result of
    enabling particular features in the driver
    (see `iio_triggered_event_setup()`).

`dev`
:   [DRIVER] device structure, should be assigned a parent
    and owner

`buffer`
:   [DRIVER] any buffer present

`scan_bytes`
:   [INTERN] num bytes captured to be fed to buffer demux

`available_scan_masks`
:   [DRIVER] optional array of allowed bitmasks. Sort the
    array in order of preference, the most preferred
    masks first.

`masklength`
:   [INTERN] the length of the mask established from
    channels

`active_scan_mask`
:   [INTERN] `union of` all scan masks requested by buffers

`scan_timestamp`
:   [INTERN] set if any buffers have requested timestamp

`trig`
:   [INTERN] current device trigger (buffer modes)

`pollfunc`
:   [DRIVER] function run on trigger being received

`pollfunc_event`
:   [DRIVER] function run on events trigger being received

`channels`
:   [DRIVER] channel specification structure table

`num_channels`
:   [DRIVER] number of channels specified in **channels**.

`name`
:   [DRIVER] name of the device.

`label`
:   [DRIVER] unique name to identify which device this is

`info`
:   [DRIVER] callbacks and constant info from driver

`setup_ops`
:   [DRIVER] callbacks to call before and after buffer
    enable/disable

`priv`
:   [DRIVER] reference to driver’s private information
    **MUST** be accessed **ONLY** via `iio_priv()` helper

iio\_device\_register

`iio_device_register (indio_dev)`

> register a device with the IIO subsystem

**Parameters**

`indio_dev`
:   Device structure filled by the device driver

devm\_iio\_device\_register

`devm_iio_device_register (dev, indio_dev)`

> Resource-managed [`iio_device_register()`](#c.iio_device_register "iio_device_register")

**Parameters**

`dev`
:   Device to allocate iio\_dev for

`indio_dev`
:   Device structure filled by the device driver

**Description**

Managed iio\_device\_register. The IIO device registered with this
function is automatically unregistered on driver detach. This function
calls [`iio_device_register()`](#c.iio_device_register "iio_device_register") internally. Refer to that function for more
information.

**Return**

0 on success, negative error number on failure.

bool iio\_device\_claim\_direct(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Keep device in direct mode

**Parameters**

`struct iio_dev *indio_dev`
:   the iio\_dev associated with the device

**Description**

If the device is in direct mode it is guaranteed to stay
that way until [`iio_device_release_direct()`](#c.iio_device_release_direct "iio_device_release_direct") is called.

Use with [`iio_device_release_direct()`](#c.iio_device_release_direct "iio_device_release_direct").

**Return**

true on success, false on failure.

iio\_device\_release\_direct

`iio_device_release_direct (indio_dev)`

> Releases claim on direct mode

**Parameters**

`indio_dev`
:   the iio\_dev associated with the device

**Description**

Release the claim. Device is no longer guaranteed to stay
in direct mode.

Use with [`iio_device_claim_direct()`](#c.iio_device_claim_direct "iio_device_claim_direct").

bool iio\_device\_try\_claim\_buffer\_mode(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Keep device in buffer mode

**Parameters**

`struct iio_dev *indio_dev`
:   the iio\_dev associated with the device

**Description**

If the device is in buffer mode it is guaranteed to stay
that way until [`iio_device_release_buffer_mode()`](#c.iio_device_release_buffer_mode "iio_device_release_buffer_mode") is called.

Use with [`iio_device_release_buffer_mode()`](#c.iio_device_release_buffer_mode "iio_device_release_buffer_mode").

**Return**

true on success, false on failure.

iio\_device\_release\_buffer\_mode

`iio_device_release_buffer_mode (indio_dev)`

> releases claim on buffer mode

**Parameters**

`indio_dev`
:   the iio\_dev associated with the device

**Description**

Release the claim. Device is no longer guaranteed to stay
in buffer mode.

Use with [`iio_device_try_claim_buffer_mode()`](#c.iio_device_try_claim_buffer_mode "iio_device_try_claim_buffer_mode").

IIO\_DEV\_ACQUIRE\_DIRECT\_MODE

`IIO_DEV_ACQUIRE_DIRECT_MODE (dev, claim)`

> Tries to acquire the direct mode lock with automatic release

**Parameters**

`dev`
:   IIO device instance

`claim`
:   Variable identifier to store acquire result

**Description**

Tries to acquire the direct mode lock with cleanup `ACQUIRE()` semantics and
automatically releases it at the end of the scope. It most be always paired
with `IIO_DEV_ACQUIRE_ERR()`, for example (notice the scope braces):

```
switch() {
case IIO_CHAN_INFO_RAW: {
        IIO_DEV_ACQUIRE_DIRECT_MODE(indio_dev, claim);
        if (IIO_DEV_ACQUIRE_FAILED(claim))
                return -EBUSY;

        ...
}
case IIO_CHAN_INFO_SCALE:
        ...
...
}
```

**Context**

Can sleep

IIO\_DEV\_ACQUIRE\_FAILED

`IIO_DEV_ACQUIRE_FAILED (claim)`

> `ACQUIRE_ERR()` wrapper

**Parameters**

`claim`
:   The claim variable passed to IIO\_DEV\_ACQUIRE\_\*`_MODE()`

**Return**

true if failed to acquire the mode, otherwise false.

IIO\_DEV\_GUARD\_CURRENT\_MODE

`IIO_DEV_GUARD_CURRENT_MODE (dev)`

> Acquires the mode lock with automatic release

**Parameters**

`dev`
:   IIO device instance

**Description**

Acquires the mode lock with cleanup `guard()` semantics. It is usually paired
with [`iio_buffer_enabled()`](#c.iio_buffer_enabled "iio_buffer_enabled").

This should *not* be used to protect internal driver state and it’s use in
general is *strongly* discouraged. Use any of the IIO\_DEV\_ACQUIRE\_\*`_MODE()`
variants.

**Context**

Can sleep

void iio\_device\_put(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   reference counted deallocation of [`struct device`](../infrastructure.html#c.device "device")

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure containing the device

struct [iio\_dev](#c.iio_dev "iio_dev") \*dev\_to\_iio\_dev(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Get IIO device `struct from` a device struct

**Parameters**

`struct device *dev`
:   The device embedded in the IIO device

**Note**

The device must be a IIO device, otherwise the result is undefined.

struct [iio\_dev](#c.iio_dev "iio_dev") \*iio\_device\_get(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   increment reference count for the device

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure

**Return**

The passed IIO device

void iio\_device\_set\_parent(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev, struct [device](../infrastructure.html#c.device "device") \*parent)
:   assign parent device to the IIO device object

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure

`struct device *parent`
:   reference to parent device object

**Description**

This utility must be called between IIO device allocation
(via [`devm_iio_device_alloc()`](#c.devm_iio_device_alloc "devm_iio_device_alloc")) & IIO device registration
(via [`iio_device_register()`](#c.iio_device_register "iio_device_register") and [`devm_iio_device_register()`](#c.devm_iio_device_register "devm_iio_device_register"))).
By default, the device allocation will also assign a parent device to
the IIO device object. In cases where [`devm_iio_device_alloc()`](#c.devm_iio_device_alloc "devm_iio_device_alloc") is used,
sometimes the parent device must be different than the device used to
manage the allocation.
In that case, this helper should be used to change the parent, hence the
requirement to call this between allocation & registration.

void iio\_device\_set\_drvdata(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev, void \*data)
:   Set device driver data

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure

`void *data`
:   Driver specific data

**Description**

Allows to attach an arbitrary pointer to an IIO device, which can later be
retrieved by [`iio_device_get_drvdata()`](#c.iio_device_get_drvdata "iio_device_get_drvdata").

void \*iio\_device\_get\_drvdata(const struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Get device driver data

**Parameters**

`const struct iio_dev *indio_dev`
:   IIO device structure

**Description**

Returns the data previously set with [`iio_device_set_drvdata()`](#c.iio_device_set_drvdata "iio_device_set_drvdata")

IIO\_DECLARE\_BUFFER\_WITH\_TS

`IIO_DECLARE_BUFFER_WITH_TS (type, name, count)`

> Declare a buffer with timestamp

**Parameters**

`type`
:   element type of the buffer

`name`
:   identifier name of the buffer

`count`
:   number of elements in the buffer

**Description**

Declares a buffer that is safe to use with [`iio_push_to_buffers_with_ts()`](buffers.html#c.iio_push_to_buffers_with_ts "iio_push_to_buffers_with_ts"). In
addition to allocating enough space for **count** elements of **type**, it also
allocates space for a s64 timestamp at the end of the buffer and ensures
proper alignment of the timestamp.

IIO\_DECLARE\_DMA\_BUFFER\_WITH\_TS

`IIO_DECLARE_DMA_BUFFER_WITH_TS (type, name, count)`

> Declare a DMA-aligned buffer with timestamp

**Parameters**

`type`
:   element type of the buffer

`name`
:   identifier name of the buffer

`count`
:   number of elements in the buffer

**Description**

Same as [`IIO_DECLARE_BUFFER_WITH_TS()`](#c.IIO_DECLARE_BUFFER_WITH_TS "IIO_DECLARE_BUFFER_WITH_TS"), but is uses \_\_aligned(IIO\_DMA\_MINALIGN)
to ensure that the buffer doesn’t share cachelines with anything that comes
before it in a struct. This should not be used for stack-allocated buffers
as stack memory cannot generally be used for DMA.

IIO\_DECLARE\_QUATERNION

`IIO_DECLARE_QUATERNION (type, name)`

> Declare a quaternion element

**Parameters**

`type`
:   element type of the individual vectors

`name`
:   identifier name

**Description**

Quaternions are a vector composed of 4 elements (W, X, Y, Z). Use this macro
to declare a quaternion element in a `struct to` ensure proper alignment in
an IIO buffer.

struct dentry \*iio\_get\_debugfs\_dentry(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   helper function to get the debugfs\_dentry

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure for device

int iio\_device\_suspend\_triggering(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   suspend trigger attached to an iio\_dev

**Parameters**

`struct iio_dev *indio_dev`
:   iio\_dev associated with the device that will have triggers suspended

**Description**

Return 0 if successful, negative otherwise

int iio\_device\_resume\_triggering(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   resume trigger attached to an iio\_dev that was previously suspended with [`iio_device_suspend_triggering()`](#c.iio_device_suspend_triggering "iio_device_suspend_triggering")

**Parameters**

`struct iio_dev *indio_dev`
:   iio\_dev associated with the device that will have triggers resumed

**Description**

Return 0 if successful, negative otherwise

const struct [iio\_scan\_type](#c.iio_scan_type "iio_scan_type") \*iio\_get\_current\_scan\_type(const struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev, const struct [iio\_chan\_spec](#c.iio_chan_spec "iio_chan_spec") \*chan)
:   Get the current scan type for a channel

**Parameters**

`const struct iio_dev *indio_dev`
:   the IIO device to get the scan type for

`const struct iio_chan_spec *chan`
:   the channel to get the scan type for

**Description**

Most devices only have one scan type per channel and can just access it
directly without calling this function. Core IIO code and drivers that
implement ext\_scan\_type in the channel spec should use this function to
get the current scan type for a channel.

**Return**

the current scan type for the channel or error.

unsigned int iio\_get\_masklength(const struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Get length of the channels mask

**Parameters**

`const struct iio_dev *indio_dev`
:   the IIO device to get the masklength for

iio\_for\_each\_active\_channel

`iio_for_each_active_channel (indio_dev, chan)`

> Iterated over active channels

**Parameters**

`indio_dev`
:   the IIO device

`chan`
:   Holds the index of the enabled channel

IIO\_DEGREE\_TO\_RAD

`IIO_DEGREE_TO_RAD (deg)`

> Convert degree to rad

**Parameters**

`deg`
:   A value in degree

**Description**

Returns the given value converted from degree to rad

IIO\_RAD\_TO\_DEGREE

`IIO_RAD_TO_DEGREE (rad)`

> Convert rad to degree

**Parameters**

`rad`
:   A value in rad

**Description**

Returns the given value converted from rad to degree

IIO\_G\_TO\_M\_S\_2

`IIO_G_TO_M_S_2 (g)`

> Convert g to meter / second\*\*2

**Parameters**

`g`
:   A value in g

**Description**

Returns the given value converted from g to meter / second\*\*2

IIO\_M\_S\_2\_TO\_G

`IIO_M_S_2_TO_G (ms2)`

> Convert meter / second\*\*2 to g

**Parameters**

`ms2`
:   A value in meter / second\*\*2

**Description**

Returns the given value converted from meter / second\*\*2 to g

int iio\_device\_id(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   query the unique ID for the device

**Parameters**

`struct iio_dev *indio_dev`
:   Device structure whose ID is being queried

**Description**

The IIO device ID is a unique index used for example for the naming
of the character device /dev/iio:device[ID].

**Return**

Unique ID for the device.

bool iio\_buffer\_enabled(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   helper function to test if the buffer is enabled

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure for device

**Return**

True, if the buffer is enabled.

int iio\_device\_set\_clock(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev, clockid\_t clock\_id)
:   Set current timestamping clock for the device

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure containing the device

`clockid_t clock_id`
:   timestamping clock POSIX identifier to set.

**Return**

0 on success, or a negative error code.

clockid\_t iio\_device\_get\_clock(const struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Retrieve current timestamping clock for the device

**Parameters**

`const struct iio_dev *indio_dev`
:   IIO device structure containing the device

**Return**

Clock ID of the current timestamping clock for the device.

s64 iio\_get\_time\_ns(const struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   utility function to get a time stamp for events etc

**Parameters**

`const struct iio_dev *indio_dev`
:   device

**Return**

Timestamp of the event in nanoseconds.

int iio\_read\_mount\_matrix(struct [device](../infrastructure.html#c.device "device") \*dev, struct [iio\_mount\_matrix](#c.iio_mount_matrix "iio_mount_matrix") \*matrix)
:   retrieve iio device mounting matrix from device “mount-matrix” property

**Parameters**

`struct device *dev`
:   device the mounting matrix property is assigned to

`struct iio_mount_matrix *matrix`
:   where to store retrieved matrix

**Description**

If device is assigned no mounting matrix property, a default 3x3 identity
matrix will be filled in.

**Return**

0 if success, or a negative error code on failure.

ssize\_t iio\_format\_value(char \*buf, unsigned int type, int size, int \*vals)
:   Formats a IIO value into its string representation

**Parameters**

`char *buf`
:   The buffer to which the formatted value gets written
    which is assumed to be big enough (i.e. PAGE\_SIZE).

`unsigned int type`
:   One of the IIO\_VAL\_\* constants. This decides how the val
    and val2 parameters are formatted.

`int size`
:   Number of IIO value entries contained in vals

`int *vals`
:   Pointer to the values, exact meaning depends on the
    type parameter.

**Return**

0 by default, a negative number on failure or the total number of characters
written for a type that belongs to the IIO\_VAL\_\* constant.

int iio\_str\_to\_fixpoint(const char \*str, int fract\_mult, int \*integer, int \*fract)
:   Parse a fixed-point number from a string

**Parameters**

`const char *str`
:   The string to parse

`int fract_mult`
:   Multiplier for the first decimal place, should be a power of 10

`int *integer`
:   The integer part of the number

`int *fract`
:   The fractional part of the number

**Return**

0 on success, or a negative error code if the string could not be parsed.

struct [iio\_dev](#c.iio_dev "iio_dev") \*iio\_device\_alloc(struct [device](../infrastructure.html#c.device "device") \*parent, int sizeof\_priv)
:   allocate an iio\_dev from a driver

**Parameters**

`struct device *parent`
:   Parent device.

`int sizeof_priv`
:   Space to allocate for private structure.

**Return**

Pointer to allocated iio\_dev on success, NULL on failure.

void iio\_device\_free(struct [iio\_dev](#c.iio_dev "iio_dev") \*dev)
:   free an iio\_dev from a driver

**Parameters**

`struct iio_dev *dev`
:   the iio\_dev associated with the device

struct [iio\_dev](#c.iio_dev "iio_dev") \*devm\_iio\_device\_alloc(struct [device](../infrastructure.html#c.device "device") \*parent, int sizeof\_priv)
:   Resource-managed [`iio_device_alloc()`](#c.iio_device_alloc "iio_device_alloc")

**Parameters**

`struct device *parent`
:   Device to allocate iio\_dev for, and parent for this IIO device

`int sizeof_priv`
:   Space to allocate for private structure.

**Description**

Managed iio\_device\_alloc. iio\_dev allocated with this function is
automatically freed on driver detach.

**Return**

Pointer to allocated iio\_dev on success, NULL on failure.

int iio\_active\_scan\_mask\_index(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Get index of the active scan mask inside the available scan masks array

**Parameters**

`struct iio_dev *indio_dev`
:   the IIO device containing the active and available scan masks

**Return**

the index or -EINVAL if active\_scan\_mask is not set

void iio\_device\_unregister(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   unregister a device from the IIO subsystem

**Parameters**

`struct iio_dev *indio_dev`
:   Device structure representing the device.

void \_\_iio\_dev\_mode\_lock(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Locks the current IIO device mode

**Parameters**

`struct iio_dev *indio_dev`
:   the iio\_dev associated with the device

**Description**

If the device is either in direct or buffer mode, it’s guaranteed to stay
that way until [`__iio_dev_mode_unlock()`](#c.__iio_dev_mode_unlock "__iio_dev_mode_unlock") is called.

This function is not meant to be used directly by drivers to protect internal
state; a driver should have it’s own mechanisms for that matter.

There are very few cases where a driver actually needs to lock the current
mode unconditionally. It’s recommended to use [`iio_device_claim_direct()`](#c.iio_device_claim_direct "iio_device_claim_direct") or
[`iio_device_try_claim_buffer_mode()`](#c.iio_device_try_claim_buffer_mode "iio_device_try_claim_buffer_mode") pairs or related helpers instead.

void \_\_iio\_dev\_mode\_unlock(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   Unlocks the current IIO device mode

**Parameters**

`struct iio_dev *indio_dev`
:   the iio\_dev associated with the device

int iio\_device\_get\_current\_mode(struct [iio\_dev](#c.iio_dev "iio_dev") \*indio\_dev)
:   helper function providing read-only access to the opaque **currentmode** variable

**Parameters**

`struct iio_dev *indio_dev`
:   IIO device structure for device
