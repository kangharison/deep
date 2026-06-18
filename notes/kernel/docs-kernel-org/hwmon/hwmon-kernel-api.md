# The Linux Hardware Monitoring kernel API

> 출처(원문): https://docs.kernel.org/hwmon/hwmon-kernel-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux Hardware Monitoring kernel API

Guenter Roeck

## Introduction

This document describes the API that can be used by hardware monitoring
drivers that want to use the hardware monitoring framework.

This document does not describe what a hardware monitoring (hwmon) Driver or
Device is. It also does not describe the API which can be used by user space
to communicate with a hardware monitoring device. If you want to know this
then please read the following file: [Naming and data format standards for sysfs files](sysfs-interface.html).

For additional guidelines on how to write and improve hwmon drivers, please
also read [How to Get Your Patch Accepted Into the Hwmon Subsystem](submitting-patches.html).

## The API

Each hardware monitoring driver must #include <linux/hwmon.h> and, in some
cases, <linux/hwmon-sysfs.h>. linux/hwmon.h declares the following
register/unregister functions:

```
struct device *
hwmon_device_register_with_info(struct device *dev,
                                const char *name, void *drvdata,
                                const struct hwmon_chip_info *info,
                                const struct attribute_group **extra_groups);

struct device *
devm_hwmon_device_register_with_info(struct device *dev,
                                     const char *name,
                                     void *drvdata,
                                     const struct hwmon_chip_info *info,
                                     const struct attribute_group **extra_groups);

void hwmon_device_unregister(struct device *dev);

char *hwmon_sanitize_name(const char *name);

char *devm_hwmon_sanitize_name(struct device *dev, const char *name);

void hwmon_lock(struct device *dev);
void hwmon_unlock(struct device *dev);
```

hwmon\_device\_register\_with\_info registers a hardware monitoring device.
It creates the standard sysfs attributes in the hardware monitoring core,
letting the driver focus on reading from and writing to the chip instead
of having to bother with sysfs attributes. The parent device parameter
as well as the chip parameter must not be NULL. Its parameters are described
in more detail below.

devm\_hwmon\_device\_register\_with\_info is similar to
hwmon\_device\_register\_with\_info. However, it is device managed, meaning the
hwmon device does not have to be removed explicitly by the removal function.

All other hardware monitoring device registration functions are deprecated
and must not be used in new drivers.

hwmon\_device\_unregister deregisters a registered hardware monitoring device.
The parameter of this function is the pointer to the registered hardware
monitoring device structure. This function must be called from the driver
remove function if the hardware monitoring device was registered with
hwmon\_device\_register\_with\_info.

All supported hwmon device registration functions only accept valid device
names. Device names including invalid characters (whitespace, ‘\*’, or ‘-‘)
will be rejected. If NULL is passed as name parameter, the hardware monitoring
device name will be derived from the parent device name.

If the driver doesn’t use a static device name (for example it uses
[`dev_name()`](../driver-api/infrastructure.html#c.dev_name "dev_name")), and therefore cannot make sure the name only contains valid
characters, hwmon\_sanitize\_name can be used. This convenience function
will duplicate the string and replace any invalid characters with an
underscore. It will allocate memory for the new string and it is the
responsibility of the caller to release the memory when the device is
removed.

devm\_hwmon\_sanitize\_name is the resource managed version of
hwmon\_sanitize\_name; the memory will be freed automatically on device
removal.

When using `[devm_]hwmon_device_register_with_info()` to register the
hardware monitoring device, accesses using the associated access functions
are serialised by the hardware monitoring core. If a driver needs locking
for other functions such as interrupt handlers or for attributes which are
fully implemented in the driver, `hwmon_lock()` and `hwmon_unlock()` can be used
to ensure that calls to those functions are serialized.

## Using devm\_hwmon\_device\_register\_with\_info()

`hwmon_device_register_with_info()` registers a hardware monitoring device.
The parameters to this function are

|  |  |
| --- | --- |
| [`struct device`](../driver-api/infrastructure.html#c.device "device") \*dev | Pointer to parent device |
| const char \*name | Device name |
| void \*drvdata | Driver private data |
| const `struct hwmon_chip_info` \*info | Pointer to chip description. |
| const `struct attribute_group` \*\*extra\_groups | Null-terminated list of additional non-standard sysfs attribute groups. |

This function returns a pointer to the created hardware monitoring device
on success and a negative error code for failure.

The hwmon\_chip\_info structure looks as follows:

```
struct hwmon_chip_info {
        const struct hwmon_ops *ops;
        const struct hwmon_channel_info * const *info;
};
```

It contains the following fields:

* ops:
  :   Pointer to device operations.
* info:
  :   NULL-terminated list of device channel descriptors.

The list of hwmon operations is defined as:

```
struct hwmon_ops {
      umode_t (*is_visible)(const void *, enum hwmon_sensor_types type,
                            u32 attr, int);
      int (*read)(struct device *, enum hwmon_sensor_types type,
                  u32 attr, int, long *);
      int (*write)(struct device *, enum hwmon_sensor_types type,
                   u32 attr, int, long);
};
```

It defines the following operations.

* is\_visible:
  :   Pointer to a function to return the file mode for each supported
      attribute. This function is mandatory.
* read:
  :   Pointer to a function for reading a value from the chip. This function
      is optional, but must be provided if any readable attributes exist.
* write:
  :   Pointer to a function for writing a value to the chip. This function is
      optional, but must be provided if any writeable attributes exist.

Each sensor channel is described with `struct hwmon_channel_info`, which is
defined as follows:

```
struct hwmon_channel_info {
        enum hwmon_sensor_types type;
        u32 *config;
};
```

It contains following fields:

* type:
  :   The hardware monitoring sensor type.

      Supported sensor types are

      > |  |  |
      > | --- | --- |
      > | hwmon\_chip | A virtual sensor type, used to describe attributes which are not bound to a specific input or output |
      > | hwmon\_temp | Temperature sensor |
      > | hwmon\_in | Voltage sensor |
      > | hwmon\_curr | Current sensor |
      > | hwmon\_power | Power sensor |
      > | hwmon\_energy | Energy sensor |
      > | hwmon\_energy64 | Energy sensor, reported as 64-bit signed value |
      > | hwmon\_humidity | Humidity sensor |
      > | hwmon\_fan | Fan speed sensor |
      > | hwmon\_pwm | PWM control |
* config:
  :   Pointer to a 0-terminated list of configuration values for each
      sensor of the given type. Each value is a combination of bit values
      describing the attributes supposed by a single sensor.

As an example, here is the complete description file for a LM75 compatible
sensor chip. The chip has a single temperature sensor. The driver wants to
register with the thermal subsystem (HWMON\_C\_REGISTER\_TZ), and it supports
the update\_interval attribute (HWMON\_C\_UPDATE\_INTERVAL). The chip supports
reading the temperature (HWMON\_T\_INPUT), it has a maximum temperature
register (HWMON\_T\_MAX) as well as a maximum temperature hysteresis register
(HWMON\_T\_MAX\_HYST):

```
static const u32 lm75_chip_config[] = {
        HWMON_C_REGISTER_TZ | HWMON_C_UPDATE_INTERVAL,
        0
};

static const struct hwmon_channel_info lm75_chip = {
        .type = hwmon_chip,
        .config = lm75_chip_config,
};

static const u32 lm75_temp_config[] = {
        HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_MAX_HYST,
        0
};

static const struct hwmon_channel_info lm75_temp = {
        .type = hwmon_temp,
        .config = lm75_temp_config,
};

static const struct hwmon_channel_info * const lm75_info[] = {
        &lm75_chip,
        &lm75_temp,
        NULL
};

The HWMON_CHANNEL_INFO() macro can and should be used when possible.
With this macro, the above example can be simplified to

static const struct hwmon_channel_info * const lm75_info[] = {
        HWMON_CHANNEL_INFO(chip,
                        HWMON_C_REGISTER_TZ | HWMON_C_UPDATE_INTERVAL),
        HWMON_CHANNEL_INFO(temp,
                        HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_MAX_HYST),
        NULL
};

The remaining declarations are as follows.

static const struct hwmon_ops lm75_hwmon_ops = {
        .is_visible = lm75_is_visible,
        .read = lm75_read,
        .write = lm75_write,
};

static const struct hwmon_chip_info lm75_chip_info = {
        .ops = &lm75_hwmon_ops,
        .info = lm75_info,
};
```

A complete list of bit values indicating individual attribute support
is defined in include/linux/hwmon.h. Definition prefixes are as follows.

|  |  |
| --- | --- |
| HWMON\_C\_xxxx | Chip attributes, for use with hwmon\_chip. |
| HWMON\_T\_xxxx | Temperature attributes, for use with hwmon\_temp. |
| HWMON\_I\_xxxx | Voltage attributes, for use with hwmon\_in. |
| HWMON\_C\_xxxx | Current attributes, for use with hwmon\_curr. Notice the prefix overlap with chip attributes. |
| HWMON\_P\_xxxx | Power attributes, for use with hwmon\_power. |
| HWMON\_E\_xxxx | Energy attributes, for use with hwmon\_energy. |
| HWMON\_H\_xxxx | Humidity attributes, for use with hwmon\_humidity. |
| HWMON\_F\_xxxx | Fan speed attributes, for use with hwmon\_fan. |
| HWMON\_PWM\_xxxx | PWM control attributes, for use with hwmon\_pwm. |

## Driver callback functions

Each driver provides is\_visible, read, and write functions. Parameters
and return values for those functions are as follows:

```
umode_t is_visible_func(const void *data, enum hwmon_sensor_types type,
                        u32 attr, int channel)
```

Parameters:
:   data:
    :   Pointer to device private data structure.

    type:
    :   The sensor type.

    attr:
    :   Attribute identifier associated with a specific attribute.
        For example, the attribute value for HWMON\_T\_INPUT would be
        hwmon\_temp\_input. For complete mappings of bit fields to
        attribute values please see include/linux/hwmon.h.

    channel:
    :   The sensor channel number.

Return value:
:   The file mode for this attribute. Typically, this will be 0 (the
    attribute will not be created), 0444, or 0644.

```
int read_func(struct device *dev, enum hwmon_sensor_types type,
              u32 attr, int channel, long *val)
```

Parameters:
:   dev:
    :   Pointer to the hardware monitoring device.

    type:
    :   The sensor type.

    attr:
    :   Attribute identifier associated with a specific attribute.
        For example, the attribute value for HWMON\_T\_INPUT would be
        hwmon\_temp\_input. For complete mappings please see
        include/linux/hwmon.h.

    channel:
    :   The sensor channel number.

    val:
    :   Pointer to attribute value.
        For hwmon\_energy64, ‘val’ is passed as long \* but needs
        a typecast to s64 \*.

Return value:
:   0 on success, a negative error number otherwise.

```
int write_func(struct device *dev, enum hwmon_sensor_types type,
               u32 attr, int channel, long val)
```

Parameters:
:   dev:
    :   Pointer to the hardware monitoring device.

    type:
    :   The sensor type.

    attr:
    :   Attribute identifier associated with a specific attribute.
        For example, the attribute value for HWMON\_T\_INPUT would be
        hwmon\_temp\_input. For complete mappings please see
        include/linux/hwmon.h.

    channel:
    :   The sensor channel number.

    val:
    :   The value to write to the chip.

Return value:
:   0 on success, a negative error number otherwise.

## Driver-provided sysfs attributes

In most situations it should not be necessary for a driver to provide sysfs
attributes since the hardware monitoring core creates those internally.
Only additional non-standard sysfs attributes need to be provided.

The header file linux/hwmon-sysfs.h provides a number of useful macros to
declare and use hardware monitoring sysfs attributes.

In many cases, you can use the existing define DEVICE\_ATTR or its variants
DEVICE\_ATTR\_{RW,RO,WO} to declare such attributes. This is feasible if an
attribute has no additional context. However, in many cases there will be
additional information such as a sensor index which will need to be passed
to the sysfs attribute handling function.

SENSOR\_DEVICE\_ATTR and SENSOR\_DEVICE\_ATTR\_2 can be used to define attributes
which need such additional context information. SENSOR\_DEVICE\_ATTR requires
one additional argument, SENSOR\_DEVICE\_ATTR\_2 requires two.

Simplified variants of SENSOR\_DEVICE\_ATTR and SENSOR\_DEVICE\_ATTR\_2 are available
and should be used if standard attribute permissions and function names are
feasible. Standard permissions are 0644 for SENSOR\_DEVICE\_ATTR[\_2]\_RW,
0444 for SENSOR\_DEVICE\_ATTR[\_2]\_RO, and 0200 for SENSOR\_DEVICE\_ATTR[\_2]\_WO.
Standard functions, similar to DEVICE\_ATTR\_{RW,RO,WO}, have \_show and \_store
appended to the provided function name.

SENSOR\_DEVICE\_ATTR and its variants define a `struct sensor_device_attribute`
variable. This structure has the following fields:

```
struct sensor_device_attribute {
        struct device_attribute dev_attr;
        int index;
};
```

You can use to\_sensor\_dev\_attr to get the pointer to this structure from the
attribute read or write function. Its parameter is the device to which the
attribute is attached.

SENSOR\_DEVICE\_ATTR\_2 and its variants define a `struct sensor_device_attribute_2`
variable, which is defined as follows:

```
struct sensor_device_attribute_2 {
        struct device_attribute dev_attr;
        u8 index;
        u8 nr;
};
```

Use to\_sensor\_dev\_attr\_2 to get the pointer to this structure. Its parameter
is the device to which the attribute is attached.
