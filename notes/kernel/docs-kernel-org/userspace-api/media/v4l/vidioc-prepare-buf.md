# 7.45.ioctl VIDIOC_PREPARE_BUF

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-prepare-buf.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.45. ioctl VIDIOC\_PREPARE\_BUF

## 7.45.1. Name

VIDIOC\_PREPARE\_BUF - Prepare a buffer for I/O

## 7.45.2. Synopsis

VIDIOC\_PREPARE\_BUF

`int ioctl(int fd, VIDIOC_PREPARE_BUF, struct v4l2_buffer *argp)`

## 7.45.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer").

## 7.45.4. Description

Applications can optionally call the [ioctl VIDIOC\_PREPARE\_BUF](#vidioc-prepare-buf) ioctl to
pass ownership of the buffer to the driver before actually enqueuing it,
using the [VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl, and to prepare it for future I/O. Such
preparations may include cache invalidation or cleaning. Performing them
in advance saves time during the actual I/O.

The struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") structure is specified in
[Buffers](buffer.html#buffer).

## 7.45.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EBUSY
:   File I/O is in progress.

EINVAL
:   The buffer `type` is not supported, or the `index` is out of
    bounds, or no buffers have been allocated yet, or the `userptr` or
    `length` are invalid.
