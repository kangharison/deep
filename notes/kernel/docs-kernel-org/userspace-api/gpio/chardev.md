# GPIO Character Device Userspace API

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/chardev.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO Character Device Userspace API

This is latest version (v2) of the character device API, as defined in
`include/uapi/linux/gpio.h.`

First added in 5.10.

Note

Do NOT abuse userspace APIs to control hardware that has proper kernel
drivers. There may already be a driver for your use case, and an existing
kernel driver is sure to provide a superior solution to bitbashing
from userspace.

Read [Subsystem drivers using GPIO](../../driver-api/gpio/drivers-on-gpio.html) to avoid reinventing
kernel wheels in userspace.

Similarly, for multi-function lines there may be other subsystems, such as
[Serial Peripheral Interface (SPI)](../../spi/index.html), [I2C/SMBus Subsystem](../../i2c/index.html),
[Pulse Width Modulation (PWM) interface](../../driver-api/pwm.html), [1-Wire Subsystem](../../w1/index.html) etc, that
provide suitable drivers and APIs for your hardware.

Basic examples using the character device API can be found in `tools/gpio/*`.

The API is based around two major objects, the [Chip](#gpio-v2-chip) and the
[Line Request](#gpio-v2-line-request).

## Chip

The Chip represents a single GPIO chip and is exposed to userspace using device
files of the form `/dev/gpiochipX`.

Each chip supports a number of GPIO lines,
[`chip.lines`](#c.gpiochip_info "gpiochip_info"). Lines on the chip are identified by an
`offset` in the range from 0 to `chip.lines - 1`, i.e. [0,chip.lines).

Lines are requested from the chip using [GPIO\_V2\_GET\_LINE\_IOCTL](gpio-v2-get-line-ioctl.html)
and the resulting line request is used to access the GPIO chip’s lines or
monitor the lines for edge events.

Within this documentation, the file descriptor returned by calling open()
on the GPIO device file is referred to as `chip_fd`.

### Operations

The following operations may be performed on the chip:

* [Get Line](gpio-v2-get-line-ioctl.html)
* [Get Chip Info](gpio-get-chipinfo-ioctl.html)
* [Get Line Info](gpio-v2-get-lineinfo-ioctl.html)
* [Watch Line Info](gpio-v2-get-lineinfo-watch-ioctl.html)
* [Unwatch Line Info](gpio-get-lineinfo-unwatch-ioctl.html)
* [Read Line Info Changed Events](gpio-v2-lineinfo-changed-read.html)

## Line Request

Line requests are created by [GPIO\_V2\_GET\_LINE\_IOCTL](gpio-v2-get-line-ioctl.html) and provide
access to a set of requested lines. The line request is exposed to userspace
via the anonymous file descriptor returned in
[`request.fd`](#c.gpio_v2_line_request "gpio_v2_line_request") by [GPIO\_V2\_GET\_LINE\_IOCTL](gpio-v2-get-line-ioctl.html).

Within this documentation, the line request file descriptor is referred to
as `req_fd`.

### Operations

The following operations may be performed on the line request:

* [Get Line Values](gpio-v2-line-get-values-ioctl.html)
* [Set Line Values](gpio-v2-line-set-values-ioctl.html)
* [Read Line Edge Events](gpio-v2-line-event-read.html)
* [Reconfigure Lines](gpio-v2-line-set-config-ioctl.html)

## Types

This section contains the structs and enums that are referenced by the API v2,
as defined in `include/uapi/linux/gpio.h`.

struct gpiochip\_info
:   Information about a certain GPIO chip

**Definition**:

```
struct gpiochip_info {
    char name[GPIO_MAX_NAME_SIZE];
    char label[GPIO_MAX_NAME_SIZE];
    __u32 lines;
};
```

**Members**

`name`
:   the Linux kernel name of this GPIO chip

`label`
:   a functional name for this GPIO chip, such as a product
    number, may be empty (i.e. label[0] == ‘0’)

`lines`
:   number of GPIO lines on this chip

enum gpio\_v2\_line\_flag
:   [`struct gpio_v2_line_attribute`](#c.gpio_v2_line_attribute "gpio_v2_line_attribute").flags values

**Constants**

`GPIO_V2_LINE_FLAG_USED`
:   line is not available for request

`GPIO_V2_LINE_FLAG_ACTIVE_LOW`
:   line active state is physical low

`GPIO_V2_LINE_FLAG_INPUT`
:   line is an input

`GPIO_V2_LINE_FLAG_OUTPUT`
:   line is an output

`GPIO_V2_LINE_FLAG_EDGE_RISING`
:   line detects rising (inactive to active)
    edges

`GPIO_V2_LINE_FLAG_EDGE_FALLING`
:   line detects falling (active to
    inactive) edges

`GPIO_V2_LINE_FLAG_OPEN_DRAIN`
:   line is an open drain output

`GPIO_V2_LINE_FLAG_OPEN_SOURCE`
:   line is an open source output

`GPIO_V2_LINE_FLAG_BIAS_PULL_UP`
:   line has pull-up bias enabled

`GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN`
:   line has pull-down bias enabled

`GPIO_V2_LINE_FLAG_BIAS_DISABLED`
:   line has bias disabled

`GPIO_V2_LINE_FLAG_EVENT_CLOCK_REALTIME`
:   line events contain REALTIME timestamps

`GPIO_V2_LINE_FLAG_EVENT_CLOCK_HTE`
:   line events contain timestamps from
    the hardware timestamping engine (HTE) subsystem

struct gpio\_v2\_line\_values
:   Values of GPIO lines

**Definition**:

```
struct gpio_v2_line_values {
    __aligned_u64 bits;
    __aligned_u64 mask;
};
```

**Members**

`bits`
:   a bitmap containing the value of the lines, set to 1 for active
    and 0 for inactive

`mask`
:   a bitmap identifying the lines to get or set, with each bit
    number corresponding to the index into [`struct
    gpio_v2_line_request`](#c.gpio_v2_line_request "gpio_v2_line_request").offsets

enum gpio\_v2\_line\_attr\_id
:   [`struct gpio_v2_line_attribute`](#c.gpio_v2_line_attribute "gpio_v2_line_attribute").id values identifying which field of the attribute `union is` in use.

**Constants**

`GPIO_V2_LINE_ATTR_ID_FLAGS`
:   flags field is in use

`GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES`
:   values field is in use

`GPIO_V2_LINE_ATTR_ID_DEBOUNCE`
:   debounce\_period\_us field is in use

struct gpio\_v2\_line\_attribute
:   a configurable attribute of a line

**Definition**:

```
struct gpio_v2_line_attribute {
    __u32 id;
    __u32 padding;
    union {
        __aligned_u64 flags;
        __aligned_u64 values;
        __u32 debounce_period_us;
    };
};
```

**Members**

`id`
:   attribute identifier with value from [`enum gpio_v2_line_attr_id`](#c.gpio_v2_line_attr_id "gpio_v2_line_attr_id")

`padding`
:   reserved for future use and must be zero filled

`{unnamed_union}`
:   anonymous

`flags`
:   if id is `GPIO_V2_LINE_ATTR_ID_FLAGS`, the flags for the GPIO
    line, with values from [`enum gpio_v2_line_flag`](#c.gpio_v2_line_flag "gpio_v2_line_flag"), such as
    `GPIO_V2_LINE_FLAG_ACTIVE_LOW`, `GPIO_V2_LINE_FLAG_OUTPUT` etc, added
    together. This overrides the default flags contained in the [`struct
    gpio_v2_line_config`](#c.gpio_v2_line_config "gpio_v2_line_config") for the associated line.

`values`
:   if id is `GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES`, a bitmap
    containing the values to which the lines will be set, with each bit
    number corresponding to the index into [`struct
    gpio_v2_line_request`](#c.gpio_v2_line_request "gpio_v2_line_request").offsets

`debounce_period_us`
:   if id is `GPIO_V2_LINE_ATTR_ID_DEBOUNCE`, the
    desired debounce period, in microseconds

struct gpio\_v2\_line\_config\_attribute
:   a configuration attribute associated with one or more of the requested lines.

**Definition**:

```
struct gpio_v2_line_config_attribute {
    struct gpio_v2_line_attribute attr;
    __aligned_u64 mask;
};
```

**Members**

`attr`
:   the configurable attribute

`mask`
:   a bitmap identifying the lines to which the attribute applies,
    with each bit number corresponding to the index into [`struct
    gpio_v2_line_request`](#c.gpio_v2_line_request "gpio_v2_line_request").offsets

struct gpio\_v2\_line\_config
:   Configuration for GPIO lines

**Definition**:

```
struct gpio_v2_line_config {
    __aligned_u64 flags;
    __u32 num_attrs;
    __u32 padding[5];
    struct gpio_v2_line_config_attribute attrs[GPIO_V2_LINE_NUM_ATTRS_MAX];
};
```

**Members**

`flags`
:   flags for the GPIO lines, with values from [`enum
    gpio_v2_line_flag`](#c.gpio_v2_line_flag "gpio_v2_line_flag"), such as `GPIO_V2_LINE_FLAG_ACTIVE_LOW`,
    `GPIO_V2_LINE_FLAG_OUTPUT` etc, added together. This is the default for
    all requested lines but may be overridden for particular lines using
    **attrs**.

`num_attrs`
:   the number of attributes in **attrs**

`padding`
:   reserved for future use and must be zero filled

`attrs`
:   the configuration attributes associated with the requested
    lines. Any attribute should only be associated with a particular line
    once. If an attribute is associated with a line multiple times then the
    first occurrence (i.e. lowest index) has precedence.

struct gpio\_v2\_line\_request
:   Information about a request for GPIO lines

**Definition**:

```
struct gpio_v2_line_request {
    __u32 offsets[GPIO_V2_LINES_MAX];
    char consumer[GPIO_MAX_NAME_SIZE];
    struct gpio_v2_line_config config;
    __u32 num_lines;
    __u32 event_buffer_size;
    __u32 padding[5];
    __s32 fd;
};
```

**Members**

`offsets`
:   an array of desired lines, specified by offset index for the
    associated GPIO chip

`consumer`
:   a desired consumer label for the selected GPIO lines such as
    “my-bitbanged-relay”

`config`
:   requested configuration for the lines

`num_lines`
:   number of lines requested in this request, i.e. the number
    of valid fields in the `GPIO_V2_LINES_MAX` sized arrays, set to 1 to
    request a single line

`event_buffer_size`
:   a suggested minimum number of line events that the
    kernel should buffer. This is only relevant if edge detection is
    enabled in the configuration. Note that this is only a suggested value
    and the kernel may allocate a larger buffer or cap the size of the
    buffer. If this field is zero then the buffer size defaults to a minimum
    of **num\_lines** \* 16.

`padding`
:   reserved for future use and must be zero filled

`fd`
:   after a successful `GPIO_V2_GET_LINE_IOCTL` operation, contains
    a valid anonymous file descriptor representing the request

struct gpio\_v2\_line\_info
:   Information about a certain GPIO line

**Definition**:

```
struct gpio_v2_line_info {
    char name[GPIO_MAX_NAME_SIZE];
    char consumer[GPIO_MAX_NAME_SIZE];
    __u32 offset;
    __u32 num_attrs;
    __aligned_u64 flags;
    struct gpio_v2_line_attribute attrs[GPIO_V2_LINE_NUM_ATTRS_MAX];
    __u32 padding[4];
};
```

**Members**

`name`
:   the name of this GPIO line, such as the output pin of the line on
    the chip, a rail or a pin header name on a board, as specified by the
    GPIO chip, may be empty (i.e. name[0] == ‘0’)

`consumer`
:   a functional name for the consumer of this GPIO line as set
    by whatever is using it, will be empty if there is no current user but
    may also be empty if the consumer doesn’t set this up

`offset`
:   the local offset on this GPIO chip, fill this in when
    requesting the line information from the kernel

`num_attrs`
:   the number of attributes in **attrs**

`flags`
:   flags for this GPIO line, with values from [`enum
    gpio_v2_line_flag`](#c.gpio_v2_line_flag "gpio_v2_line_flag"), such as `GPIO_V2_LINE_FLAG_ACTIVE_LOW`,
    `GPIO_V2_LINE_FLAG_OUTPUT` etc, added together

`attrs`
:   the configuration attributes associated with the line

`padding`
:   reserved for future use

enum gpio\_v2\_line\_changed\_type
:   `struct gpio_v2_line_changed`.event\_type values

**Constants**

`GPIO_V2_LINE_CHANGED_REQUESTED`
:   line has been requested

`GPIO_V2_LINE_CHANGED_RELEASED`
:   line has been released

`GPIO_V2_LINE_CHANGED_CONFIG`
:   line has been reconfigured

struct gpio\_v2\_line\_info\_changed
:   Information about a change in status of a GPIO line

**Definition**:

```
struct gpio_v2_line_info_changed {
    struct gpio_v2_line_info info;
    __aligned_u64 timestamp_ns;
    __u32 event_type;
    __u32 padding[5];
};
```

**Members**

`info`
:   updated line information

`timestamp_ns`
:   estimate of time of status change occurrence, in nanoseconds

`event_type`
:   the type of change with a value from [`enum
    gpio_v2_line_changed_type`](#c.gpio_v2_line_changed_type "gpio_v2_line_changed_type")

`padding`
:   reserved for future use

enum gpio\_v2\_line\_event\_id
:   [`struct gpio_v2_line_event`](#c.gpio_v2_line_event "gpio_v2_line_event").id values

**Constants**

`GPIO_V2_LINE_EVENT_RISING_EDGE`
:   event triggered by a rising edge

`GPIO_V2_LINE_EVENT_FALLING_EDGE`
:   event triggered by a falling edge

struct gpio\_v2\_line\_event
:   The actual event being pushed to userspace

**Definition**:

```
struct gpio_v2_line_event {
    __aligned_u64 timestamp_ns;
    __u32 id;
    __u32 offset;
    __u32 seqno;
    __u32 line_seqno;
    __u32 padding[6];
};
```

**Members**

`timestamp_ns`
:   best estimate of time of event occurrence, in nanoseconds

`id`
:   event identifier with value from [`enum gpio_v2_line_event_id`](#c.gpio_v2_line_event_id "gpio_v2_line_event_id")

`offset`
:   the offset of the line that triggered the event

`seqno`
:   the sequence number for this event in the sequence of events for
    all the lines in this line request

`line_seqno`
:   the sequence number for this event in the sequence of
    events on this particular line

`padding`
:   reserved for future use

**Description**

By default the **timestamp\_ns** is read from `CLOCK_MONOTONIC` and is
intended to allow the accurate measurement of the time between events.
It does not provide the wall-clock time.

If the `GPIO_V2_LINE_FLAG_EVENT_CLOCK_REALTIME` flag is set then the
**timestamp\_ns** is read from `CLOCK_REALTIME`.

If the `GPIO_V2_LINE_FLAG_EVENT_CLOCK_HTE` flag is set then the
**timestamp\_ns** is provided by the hardware timestamping engine (HTE)
subsystem.
