# GPIO_GET_LINEEVENT_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-get-lineevent-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_GET\_LINEEVENT\_IOCTL

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_GET\_LINE\_IOCTL](gpio-v2-get-line-ioctl.html).

## Name

GPIO\_GET\_LINEEVENT\_IOCTL - Request a line with edge detection from the kernel.

## Synopsis

GPIO\_GET\_LINEEVENT\_IOCTL

`int ioctl(int chip_fd, GPIO_GET_LINEEVENT_IOCTL, struct gpioevent_request *request)`

## Arguments

`chip_fd`
:   The file descriptor of the GPIO character device returned by open().

`request`
:   The [`event_request`](chardev_v1.html#c.gpioevent_request "gpioevent_request") specifying the line
    to request and its configuration.

## Description

Request a line with edge detection from the kernel.

On success, the requesting process is granted exclusive access to the line
value and may receive events when edges are detected on the line, as
described in [GPIO\_LINEEVENT\_DATA\_READ](gpio-lineevent-data-read.html).

The state of a line is guaranteed to remain as requested until the returned
file descriptor is closed. Once the file descriptor is closed, the state of
the line becomes uncontrolled from the userspace perspective, and may revert
to its default state.

Requesting a line already in use is an error (**EBUSY**).

Requesting edge detection on a line that does not support interrupts is an
error (**ENXIO**).

As with the [line handle](gpio-get-linehandle-ioctl.html#gpio-get-linehandle-config-support), the
bias configuration is best effort.

Closing the `chip_fd` has no effect on existing line events.

### Configuration Rules

The following configuration rules apply:

The line event is requested as an input, so no flags specific to output lines,
`GPIOHANDLE_REQUEST_OUTPUT`, `GPIOHANDLE_REQUEST_OPEN_DRAIN`, or
`GPIOHANDLE_REQUEST_OPEN_SOURCE`, may be set.

Only one bias flag, `GPIOHANDLE_REQUEST_BIAS_xxx`, may be set.
If no bias flags are set then the bias configuration is not changed.

The edge flags, `GPIOEVENT_REQUEST_RISING_EDGE` and
`GPIOEVENT_REQUEST_FALLING_EDGE`, may be combined to detect both rising
and falling edges.

Requesting an invalid configuration is an error (**EINVAL**).

## Return Value

On success 0 and the [`request.fd`](chardev_v1.html#c.gpioevent_request "gpioevent_request") contains the file
descriptor for the request.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
