# GPIO Character Device Userspace API (v1)

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/chardev_v1.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO Character Device Userspace API (v1)

Warning

This API is obsoleted by [GPIO Character Device Userspace API](chardev.html) (v2).

New developments should use the v2 API, and existing developments are
encouraged to migrate as soon as possible, as this API will be removed
in the future. The v2 API is a functional superset of the v1 API so any
v1 call can be directly translated to a v2 equivalent.

This interface will continue to be maintained for the migration period,
but new features will only be added to the new API.

First added in 4.8.

The API is based around three major objects, the [Chip](#gpio-v1-chip), the
[Line Handle](#gpio-v1-line-handle), and the [Line Event](#gpio-v1-line-event).

Where “line event” is used in this document it refers to the request that can
monitor a line for edge events, not the edge events themselves.

## Chip

The Chip represents a single GPIO chip and is exposed to userspace using device
files of the form `/dev/gpiochipX`.

Each chip supports a number of GPIO lines,
[`chip.lines`](chardev.html#c.gpiochip_info "gpiochip_info"). Lines on the chip are identified by an
`offset` in the range from 0 to `chip.lines - 1`, i.e. [0,chip.lines).

Lines are requested from the chip using either [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html)
and the resulting line handle is used to access the GPIO chip’s lines, or
[GPIO\_GET\_LINEEVENT\_IOCTL](gpio-get-lineevent-ioctl.html) and the resulting line event is used to monitor
a GPIO line for edge events.

Within this documentation, the file descriptor returned by calling open()
on the GPIO device file is referred to as `chip_fd`.

### Operations

The following operations may be performed on the chip:

* [Get Line Handle](gpio-get-linehandle-ioctl.html)
* [Get Line Event](gpio-get-lineevent-ioctl.html)
* [Get Chip Info](gpio-get-chipinfo-ioctl.html)
* [Get Line Info](gpio-get-lineinfo-ioctl.html)
* [Watch Line Info](gpio-get-lineinfo-watch-ioctl.html)
* [Unwatch Line Info](gpio-get-lineinfo-unwatch-ioctl.html)
* [Read Line Info Changed Events](gpio-lineinfo-changed-read.html)

## Line Handle

Line handles are created by [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html) and provide
access to a set of requested lines. The line handle is exposed to userspace
via the anonymous file descriptor returned in
[`request.fd`](#c.gpiohandle_request "gpiohandle_request") by [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html).

Within this documentation, the line handle file descriptor is referred to
as `handle_fd`.

### Operations

The following operations may be performed on the line handle:

* [Get Line Values](gpio-handle-get-line-values-ioctl.html)
* [Set Line Values](gpio-handle-set-line-values-ioctl.html)
* [Reconfigure Lines](gpio-handle-set-config-ioctl.html)

## Line Event

Line events are created by [GPIO\_GET\_LINEEVENT\_IOCTL](gpio-get-lineevent-ioctl.html) and provide
access to a requested line. The line event is exposed to userspace
via the anonymous file descriptor returned in
[`request.fd`](#c.gpioevent_request "gpioevent_request") by [GPIO\_GET\_LINEEVENT\_IOCTL](gpio-get-lineevent-ioctl.html).

Within this documentation, the line event file descriptor is referred to
as `event_fd`.

### Operations

The following operations may be performed on the line event:

* [Get Line Value](gpio-handle-get-line-values-ioctl.html)
* [Read Line Edge Events](gpio-lineevent-data-read.html)

## Types

This section contains the structs that are referenced by the ABI v1.

The [`struct gpiochip_info`](chardev.html#c.gpiochip_info "gpiochip_info") is common to ABI v1 and v2.

struct gpioline\_info
:   Information about a certain GPIO line

**Definition**:

```
struct gpioline_info {
    __u32 line_offset;
    __u32 flags;
    char name[GPIO_MAX_NAME_SIZE];
    char consumer[GPIO_MAX_NAME_SIZE];
};
```

**Members**

`line_offset`
:   the local offset on this GPIO device, fill this in when
    requesting the line information from the kernel

`flags`
:   various flags for this line

`name`
:   the name of this GPIO line, such as the output pin of the line on the
    chip, a rail or a pin header name on a board, as specified by the gpio
    chip, may be empty (i.e. name[0] == ‘0’)

`consumer`
:   a functional name for the consumer of this GPIO line as set by
    whatever is using it, will be empty if there is no current user but may
    also be empty if the consumer doesn’t set this up

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_info`](chardev.html#c.gpio_v2_line_info "gpio_v2_line_info") instead.

struct gpioline\_info\_changed
:   Information about a change in status of a GPIO line

**Definition**:

```
struct gpioline_info_changed {
    struct gpioline_info info;
    __u64 timestamp;
    __u32 event_type;
    __u32 padding[5];
};
```

**Members**

`info`
:   updated line information

`timestamp`
:   estimate of time of status change occurrence, in nanoseconds

`event_type`
:   one of `GPIOLINE_CHANGED_REQUESTED`,
    `GPIOLINE_CHANGED_RELEASED` and `GPIOLINE_CHANGED_CONFIG`

`padding`
:   reserved for future use

**Description**

The [`struct gpioline_info`](#c.gpioline_info "gpioline_info") embedded here has 32-bit alignment on its own,
but it works fine with 64-bit alignment too. With its 72 byte size, we can
guarantee there are no implicit holes between it and subsequent members.
The 20-byte padding at the end makes sure we don’t add any implicit padding
at the end of the structure on 64-bit architectures.

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_info_changed`](chardev.html#c.gpio_v2_line_info_changed "gpio_v2_line_info_changed") instead.

struct gpiohandle\_request
:   Information about a GPIO handle request

**Definition**:

```
struct gpiohandle_request {
    __u32 lineoffsets[GPIOHANDLES_MAX];
    __u32 flags;
    __u8 default_values[GPIOHANDLES_MAX];
    char consumer_label[GPIO_MAX_NAME_SIZE];
    __u32 lines;
    int fd;
};
```

**Members**

`lineoffsets`
:   an array of desired lines, specified by offset index for the
    associated GPIO device

`flags`
:   desired flags for the desired GPIO lines, such as
    `GPIOHANDLE_REQUEST_OUTPUT`, `GPIOHANDLE_REQUEST_ACTIVE_LOW` etc, added
    together. Note that even if multiple lines are requested, the same flags
    must be applicable to all of them, if you want lines with individual
    flags set, request them one by one. It is possible to select
    a batch of input or output lines, but they must all have the same
    characteristics, i.e. all inputs or all outputs, all active low etc

`default_values`
:   if the `GPIOHANDLE_REQUEST_OUTPUT` is set for a requested
    line, this specifies the default output value, should be 0 (inactive) or
    1 (active). Anything other than 0 or 1 will be interpreted as active.

`consumer_label`
:   a desired consumer label for the selected GPIO line(s)
    such as “my-bitbanged-relay”

`lines`
:   number of lines requested in this request, i.e. the number of
    valid fields in the above arrays, set to 1 to request a single line

`fd`
:   after a successful `GPIO_GET_LINEHANDLE_IOCTL` operation, contains
    a valid anonymous file descriptor representing the request

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_request`](chardev.html#c.gpio_v2_line_request "gpio_v2_line_request") instead.

struct gpiohandle\_config
:   Configuration for a GPIO handle request

**Definition**:

```
struct gpiohandle_config {
    __u32 flags;
    __u8 default_values[GPIOHANDLES_MAX];
    __u32 padding[4];
};
```

**Members**

`flags`
:   updated flags for the requested GPIO lines, such as
    `GPIOHANDLE_REQUEST_OUTPUT`, `GPIOHANDLE_REQUEST_ACTIVE_LOW` etc, added
    together

`default_values`
:   if the `GPIOHANDLE_REQUEST_OUTPUT` is set in flags,
    this specifies the default output value, should be 0 (inactive) or
    1 (active). Anything other than 0 or 1 will be interpreted as active.

`padding`
:   reserved for future use and should be zero filled

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_config`](chardev.html#c.gpio_v2_line_config "gpio_v2_line_config") instead.

struct gpiohandle\_data
:   Information of values on a GPIO handle

**Definition**:

```
struct gpiohandle_data {
    __u8 values[GPIOHANDLES_MAX];
};
```

**Members**

`values`
:   when getting the state of lines this contains the current
    state of a line, when setting the state of lines these should contain
    the desired target state. States are 0 (inactive) or 1 (active).
    When setting, anything other than 0 or 1 will be interpreted as active.

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_values`](chardev.html#c.gpio_v2_line_values "gpio_v2_line_values") instead.

struct gpioevent\_request
:   Information about a GPIO event request

**Definition**:

```
struct gpioevent_request {
    __u32 lineoffset;
    __u32 handleflags;
    __u32 eventflags;
    char consumer_label[GPIO_MAX_NAME_SIZE];
    int fd;
};
```

**Members**

`lineoffset`
:   the desired line to subscribe to events from, specified by
    offset index for the associated GPIO device

`handleflags`
:   desired handle flags for the desired GPIO line, such as
    `GPIOHANDLE_REQUEST_ACTIVE_LOW` or `GPIOHANDLE_REQUEST_OPEN_DRAIN`

`eventflags`
:   desired flags for the desired GPIO event line, such as
    `GPIOEVENT_REQUEST_RISING_EDGE` or `GPIOEVENT_REQUEST_FALLING_EDGE`

`consumer_label`
:   a desired consumer label for the selected GPIO line(s)
    such as “my-listener”

`fd`
:   after a successful `GPIO_GET_LINEEVENT_IOCTL` operation, contains a
    valid anonymous file descriptor representing the request

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_request`](chardev.html#c.gpio_v2_line_request "gpio_v2_line_request") instead.

struct gpioevent\_data
:   The actual event being pushed to userspace

**Definition**:

```
struct gpioevent_data {
    __u64 timestamp;
    __u32 id;
};
```

**Members**

`timestamp`
:   best estimate of time of event occurrence, in nanoseconds

`id`
:   event identifier, one of `GPIOEVENT_EVENT_RISING_EDGE` or
    `GPIOEVENT_EVENT_FALLING_EDGE`

**Note**

This `struct is` part of ABI v1 and is deprecated.
Use ABI v2 and [`struct gpio_v2_line_event`](chardev.html#c.gpio_v2_line_event "gpio_v2_line_event") instead.
