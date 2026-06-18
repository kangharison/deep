# GPIO_V2_LINE_GET_VALUES_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-v2-line-get-values-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_V2\_LINE\_GET\_VALUES\_IOCTL

## Name

GPIO\_V2\_LINE\_GET\_VALUES\_IOCTL - Get the values of requested lines.

## Synopsis

GPIO\_V2\_LINE\_GET\_VALUES\_IOCTL

`int ioctl(int req_fd, GPIO_V2_LINE_GET_VALUES_IOCTL, struct gpio_v2_line_values *values)`

## Arguments

`req_fd`
:   The file descriptor of the GPIO character device, as returned in the
    [`request.fd`](chardev.html#c.gpio_v2_line_request "gpio_v2_line_request") by [GPIO\_V2\_GET\_LINE\_IOCTL](gpio-v2-get-line-ioctl.html).

`values`
:   The [`line_values`](chardev.html#c.gpio_v2_line_values "gpio_v2_line_values") to get with the `mask` set
    to indicate the subset of requested lines to get.

## Description

Get the values of requested lines.

The values returned are logical, indicating if the line is active or inactive.
The `GPIO_V2_LINE_FLAG_ACTIVE_LOW` flag controls the mapping between physical
values (high/low) and logical values (active/inactive).
If `GPIO_V2_LINE_FLAG_ACTIVE_LOW` is not set then high is active and low is
inactive. If `GPIO_V2_LINE_FLAG_ACTIVE_LOW` is set then low is active and
high is inactive.

The values of both input and output lines may be read.

For output lines, the value returned is driver and configuration dependent and
may be either the output buffer (the last requested value set) or the input
buffer (the actual level of the line), and depending on the hardware and
configuration these may differ.

## Return Value

On success 0 and the corresponding [`values.bits`](chardev.html#c.gpio_v2_line_values "gpio_v2_line_values")
contain the value read.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
