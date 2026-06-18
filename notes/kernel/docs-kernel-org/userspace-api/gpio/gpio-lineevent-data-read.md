# GPIO_LINEEVENT_DATA_READ

> 출처(원문): https://docs.kernel.org/userspace-api/gpio/gpio-lineevent-data-read.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPIO\_LINEEVENT\_DATA\_READ

Warning

This ioctl is part of [GPIO Character Device Userspace API (v1)](chardev_v1.html) and is obsoleted by
[GPIO\_V2\_LINE\_EVENT\_READ](gpio-v2-line-event-read.html).

## Name

GPIO\_LINEEVENT\_DATA\_READ - Read edge detection events from a line event.

## Synopsis

`int read(int event_fd, void *buf, size_t count)`

## Arguments

`event_fd`
:   The file descriptor of the GPIO character device, as returned in the
    [`request.fd`](chardev_v1.html#c.gpioevent_request "gpioevent_request") by [GPIO\_GET\_LINEEVENT\_IOCTL](gpio-get-lineevent-ioctl.html).

`buf`
:   The buffer to contain the [`events`](chardev_v1.html#c.gpioevent_data "gpioevent_data").

`count`
:   The number of bytes available in `buf`, which must be at
    least the size of a [`gpioevent_data`](chardev_v1.html#c.gpioevent_data "gpioevent_data").

## Description

Read edge detection events for a line from a line event.

Edge detection must be enabled for the input line using either
`GPIOEVENT_REQUEST_RISING_EDGE` or `GPIOEVENT_REQUEST_FALLING_EDGE`, or
both. Edge events are then generated whenever edge interrupts are detected on
the input line.

Edges are defined in terms of changes to the logical line value, so an inactive
to active transition is a rising edge. If `GPIOHANDLE_REQUEST_ACTIVE_LOW` is
set then logical polarity is the opposite of physical polarity, and
`GPIOEVENT_REQUEST_RISING_EDGE` then corresponds to a falling physical edge.

The kernel captures and timestamps edge events as close as possible to their
occurrence and stores them in a buffer from where they can be read by
userspace at its convenience using read().

The source of the clock for [`event.timestamp`](chardev_v1.html#c.gpioevent_data "gpioevent_data") is
`CLOCK_MONOTONIC`, except for kernels earlier than Linux 5.7 when it was
`CLOCK_REALTIME`. There is no indication in the [`gpioevent_data`](chardev_v1.html#c.gpioevent_data "gpioevent_data")
as to which clock source is used, it must be determined from either the kernel
version or sanity checks on the timestamp itself.

Events read from the buffer are always in the same order that they were
detected by the kernel.

The size of the kernel event buffer is fixed at 16 events.

The buffer may overflow if bursts of events occur quicker than they are read
by userspace. If an overflow occurs then the most recent event is discarded.
Overflow cannot be detected from userspace.

To minimize the number of calls required to copy events from the kernel to
userspace, read() supports copying multiple events. The number of events
copied is the lower of the number available in the kernel buffer and the
number that will fit in the userspace buffer (`buf`).

The read() will block if no event is available and the `event_fd` has not
been set **O\_NONBLOCK**.

The presence of an event can be tested for by checking that the `event_fd` is
readable using poll() or an equivalent.

## Return Value

On success the number of bytes read, which will be a multiple of the size of
a `gpio_lineevent_data` event.

On error -1 and the `errno` variable is set appropriately.
Common error codes are described in [GPIO Error Codes](error-codes.html).
