# GPIO_GET_LINEINFO_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-get-lineinfo-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_GET\_LINEINFO\_IOCTL

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_GET\_LINEINFO\_IOCTL](gpio-v2-get-lineinfo-ioctl.html).

## Name

GPIO\_GET\_LINEINFO\_IOCTL - Get the publicly available information for a line.

## Synopsis

GPIO\_GET\_LINEINFO\_IOCTL

`int ioctl(int chip_fd, GPIO_GET_LINEINFO_IOCTL, struct gpioline_info *info)`

## Arguments

`chip_fd`
:   The file descriptor of the GPIO character device returned by open().

`info`
:   The [`line_info`](chardev_v1.html#c.gpioline_info "gpioline_info") to be populated, with the
    `offset` field set to indicate the line to be collected.

## Description

Get the publicly available information for a line.

This information is available independent of whether the line is in use.

Note

The line info does not include the line value.

The line must be requested using [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html) or
[GPIO\_GET\_LINEEVENT\_IOCTL](gpio-get-lineevent-ioctl.html) to access its value.

## Return Value

On success 0 and `info` is populated with the chip info.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
