# GPIO_GET_LINEINFO_WATCH_IOCTL

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-get-lineinfo-watch-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_GET\_LINEINFO\_WATCH\_IOCTL

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_GET\_LINEINFO\_WATCH\_IOCTL](gpio-v2-get-lineinfo-watch-ioctl.html).

## Name

GPIO\_GET\_LINEINFO\_WATCH\_IOCTL - Enable watching a line for changes to its
request state and configuration information.

## Synopsis

GPIO\_GET\_LINEINFO\_WATCH\_IOCTL

`int ioctl(int chip_fd, GPIO_GET_LINEINFO_WATCH_IOCTL, struct gpioline_info *info)`

## Arguments

`chip_fd`
:   The file descriptor of the GPIO character device returned by open().

`info`
:   The [`line_info`](chardev_v1.html#c.gpioline_info "gpioline_info") `struct to` be populated, with
    the `offset` set to indicate the line to watch

## Description

Enable watching a line for changes to its request state and configuration
information. Changes to line info include a line being requested, released
or reconfigured.

Note

Watching line info is not generally required, and would typically only be
used by a system monitoring component.

The line info does NOT include the line value.

The line must be requested using [GPIO\_GET\_LINEHANDLE\_IOCTL](gpio-get-linehandle-ioctl.html) or
[GPIO\_GET\_LINEEVENT\_IOCTL](gpio-get-lineevent-ioctl.html) to access its value, and the line event can
monitor a line for events using [GPIO\_LINEEVENT\_DATA\_READ](gpio-lineevent-data-read.html).

By default all lines are unwatched when the GPIO chip is opened.

Multiple lines may be watched simultaneously by adding a watch for each.

Once a watch is set, any changes to line info will generate events which can be
read from the `chip_fd` as described in
[GPIO\_LINEINFO\_CHANGED\_READ](gpio-lineinfo-changed-read.html).

Adding a watch to a line that is already watched is an error (**EBUSY**).

Watches are specific to the `chip_fd` and are independent of watches
on the same GPIO chip opened with a separate call to open().

First added in 5.7.

## Return Value

On success 0 and `info` is populated with the current line info.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
