# GPIO_HANDLE_SET_LINE_VALUES_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-handle-set-line-values-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_HANDLE\_SET\_LINE\_VALUES\_IOCTL

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_LINE\_SET\_VALUES\_IOCTL](gpio-v2-line-set-values-ioctl.html).

## Name

GPIO\_HANDLE\_SET\_LINE\_VALUES\_IOCTL - Set the values of all requested output lines.

## Synopsis

GPIO\_HANDLE\_SET\_LINE\_VALUES\_IOCTL

`int ioctl(int handle_fd, GPIO_HANDLE_SET_LINE_VALUES_IOCTL, struct gpiohandle_data *values)`

## Arguments

`handle_fd`
:   The file descriptor of the GPIO character device, as returned in the
    [`request.fd`](chardev_v1.html#c.gpiohandle_request "gpiohandle_request") by [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html).

`values`
:   The [`line_values`](chardev_v1.html#c.gpiohandle_data "gpiohandle_data") to set.

## Description

Set the values of all requested output lines.

The values set are logical, indicating if the line is to be active or inactive.
The `GPIOHANDLE_REQUEST_ACTIVE_LOW` flag controls the mapping between logical
values (active/inactive) and physical values (high/low).
If `GPIOHANDLE_REQUEST_ACTIVE_LOW` is not set then active is high and
inactive is low. If `GPIOHANDLE_REQUEST_ACTIVE_LOW` is set then active is low
and inactive is high.

Only the values of output lines may be set.
Attempting to set the value of input lines is an error (**EPERM**).

## Return Value

On success 0.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
