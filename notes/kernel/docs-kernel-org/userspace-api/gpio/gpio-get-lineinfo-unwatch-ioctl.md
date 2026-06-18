# GPIO_GET_LINEINFO_UNWATCH_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-get-lineinfo-unwatch-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_GET\_LINEINFO\_UNWATCH\_IOCTL

## Name

GPIO\_GET\_LINEINFO\_UNWATCH\_IOCTL - Disable watching a line for changes to its
requested state and configuration information.

## Synopsis

GPIO\_GET\_LINEINFO\_UNWATCH\_IOCTL

`int ioctl(int chip_fd, GPIO_GET_LINEINFO_UNWATCH_IOCTL, u32 *offset)`

## Arguments

`chip_fd`
:   The file descriptor of the GPIO character device returned by open().

`offset`
:   The offset of the line to no longer watch.

## Description

Remove the line from the list of lines being watched on this `chip_fd`.

This is the reverse of [GPIO\_V2\_GET\_LINEINFO\_WATCH\_IOCTL](gpio-v2-get-lineinfo-watch-ioctl.html) (v2) and
[GPIO\_GET\_LINEINFO\_WATCH\_IOCTL](gpio-get-lineinfo-watch-ioctl.html) (v1).

Unwatching a line that is not watched is an error (**EBUSY**).

First added in 5.7.

## Return Value

On success 0.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
