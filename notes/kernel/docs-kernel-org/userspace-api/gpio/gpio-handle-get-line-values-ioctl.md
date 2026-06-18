# GPIOHANDLE_GET_LINE_VALUES_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-handle-get-line-values-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIOHANDLE\_GET\_LINE\_VALUES\_IOCTL

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_LINE\_GET\_VALUES\_IOCTL](gpio-v2-line-get-values-ioctl.html).

## Name

GPIOHANDLE\_GET\_LINE\_VALUES\_IOCTL - Get the values of all requested lines.

## Synopsis

GPIOHANDLE\_GET\_LINE\_VALUES\_IOCTL

`int ioctl(int handle_fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, struct gpiohandle_data *values)`

## Arguments

`handle_fd`
:   The file descriptor of the GPIO character device, as returned in the
    [`request.fd`](chardev_v1.html#c.gpiohandle_request "gpiohandle_request") by [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html).

`values`
:   The [`line_values`](chardev_v1.html#c.gpiohandle_data "gpiohandle_data") to be populated.

## Description

Get the values of all requested lines.

The values returned are logical, indicating if the line is active or inactive.
The `GPIOHANDLE_REQUEST_ACTIVE_LOW` flag controls the mapping between physical
values (high/low) and logical values (active/inactive).
If `GPIOHANDLE_REQUEST_ACTIVE_LOW` is not set then high is active and
low is inactive. If `GPIOHANDLE_REQUEST_ACTIVE_LOW` is set then low is active
and high is inactive.

The values of both input and output lines may be read.

For output lines, the value returned is driver and configuration dependent and
may be either the output buffer (the last requested value set) or the input
buffer (the actual level of the line), and depending on the hardware and
configuration these may differ.

This ioctl can also be used to read the line value for line events,
substituting the `event_fd` for the `handle_fd`. As there is only
one line requested in that case, only the one value is returned in `values`.

## Return Value

On success 0 and `values` populated with the values read.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
