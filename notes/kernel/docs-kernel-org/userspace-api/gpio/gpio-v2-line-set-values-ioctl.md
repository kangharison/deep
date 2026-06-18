# GPIO_V2_LINE_SET_VALUES_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-v2-line-set-values-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_V2\_LINE\_SET\_VALUES\_IOCTL

## Name

GPIO\_V2\_LINE\_SET\_VALUES\_IOCTL - Set the values of requested output lines.

## Synopsis

GPIO\_V2\_LINE\_SET\_VALUES\_IOCTL

`int ioctl(int req_fd, GPIO_V2_LINE_SET_VALUES_IOCTL, struct gpio_v2_line_values *values)`

## Arguments

`req_fd`
:   The file descriptor of the GPIO character device, as returned in the
    [`request.fd`](chardev.html#c.gpio_v2_line_request "gpio_v2_line_request") by [GPIO\_V2\_GET\_LINE\_IOCTL](gpio-v2-get-line-ioctl.html).

`values`
:   The [`line_values`](chardev.html#c.gpio_v2_line_values "gpio_v2_line_values") to set with the `mask` set
    to indicate the subset of requested lines to set and `bits` set to
    indicate the new value.

## Description

Set the values of requested output lines.

The values set are logical, indicating if the line is to be active or inactive.
The `GPIO_V2_LINE_FLAG_ACTIVE_LOW` flag controls the mapping between logical
values (active/inactive) and physical values (high/low).
If `GPIO_V2_LINE_FLAG_ACTIVE_LOW` is not set then active is high and inactive
is low. If `GPIO_V2_LINE_FLAG_ACTIVE_LOW` is set then active is low and
inactive is high.

Only the values of output lines may be set.
Attempting to set the value of an input line is an error (**EPERM**).

## Return Value

On success 0.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
