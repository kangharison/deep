# GPIOHANDLE_SET_CONFIG_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-handle-set-config-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIOHANDLE\_SET\_CONFIG\_IOCTL

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_LINE\_SET\_CONFIG\_IOCTL](gpio-v2-line-set-config-ioctl.html).

## Name

GPIOHANDLE\_SET\_CONFIG\_IOCTL - Update the configuration of previously requested lines.

## Synopsis

GPIOHANDLE\_SET\_CONFIG\_IOCTL

`int ioctl(int handle_fd, GPIOHANDLE_SET_CONFIG_IOCTL, struct gpiohandle_config *config)`

## Arguments

`handle_fd`
:   The file descriptor of the GPIO character device, as returned in the
    [`request.fd`](chardev_v1.html#c.gpiohandle_request "gpiohandle_request") by [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html).

`config`
:   The new [`configuration`](chardev_v1.html#c.gpiohandle_config "gpiohandle_config") to apply to the
    requested lines.

## Description

Update the configuration of previously requested lines, without releasing the
line or introducing potential glitches.

The configuration applies to all requested lines.

The same [Configuration Rules](gpio-get-linehandle-ioctl.html#gpio-get-linehandle-config-rules) and
[Configuration Support](gpio-get-linehandle-ioctl.html#gpio-get-linehandle-config-support) that apply when requesting the
lines also apply when updating the line configuration, with the additional
restriction that a direction flag must be set. Requesting an invalid
configuration, including without a direction flag set, is an error
(**EINVAL**).

The motivating use case for this command is changing direction of
bi-directional lines between input and output, but it may be used more
generally to move lines seamlessly from one configuration state to another.

To only change the value of output lines, use
[GPIO\_HANDLE\_SET\_LINE\_VALUES\_IOCTL](gpio-handle-set-line-values-ioctl.html).

First added in 5.5.

## Return Value

On success 0.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
