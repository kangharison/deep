# 7.41.ioctl VIDIOC_G_STD, VIDIOC_S_STD, VIDIOC_SUBDEV_G_STD, VIDIOC_SUBDEV_S_STD

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-std.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.41. ioctl VIDIOC\_G\_STD, VIDIOC\_S\_STD, VIDIOC\_SUBDEV\_G\_STD, VIDIOC\_SUBDEV\_S\_STD

## 7.41.1. Name

VIDIOC\_G\_STD - VIDIOC\_S\_STD - VIDIOC\_SUBDEV\_G\_STD - VIDIOC\_SUBDEV\_S\_STD - Query or select the video standard of the current input

## 7.41.2. Synopsis

VIDIOC\_G\_STD

`int ioctl(int fd, VIDIOC_G_STD, v4l2_std_id *argp)`

VIDIOC\_S\_STD

`int ioctl(int fd, VIDIOC_S_STD, const v4l2_std_id *argp)`

VIDIOC\_SUBDEV\_G\_STD

`int ioctl(int fd, VIDIOC_SUBDEV_G_STD, v4l2_std_id *argp)`

VIDIOC\_SUBDEV\_S\_STD

`int ioctl(int fd, VIDIOC_SUBDEV_S_STD, const v4l2_std_id *argp)`

## 7.41.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to `v4l2_std_id`.

## 7.41.4. Description

To query and select the current video standard applications use the
[VIDIOC\_G\_STD](#vidioc-g-std) and [VIDIOC\_S\_STD](#vidioc-g-std) ioctls which take a pointer to a
[v4l2\_std\_id](vidioc-enumstd.html#v4l2-std-id) type as argument. [VIDIOC\_G\_STD](#vidioc-g-std)
can return a single flag or a set of flags as in struct
[`v4l2_standard`](vidioc-enumstd.html#c.V4L.v4l2_standard "v4l2_standard") field `id`. The flags must be
unambiguous such that they appear in only one enumerated
struct [`v4l2_standard`](vidioc-enumstd.html#c.V4L.v4l2_standard "v4l2_standard") structure.

[VIDIOC\_S\_STD](#vidioc-g-std) accepts one or more flags, being a write-only ioctl it
does not return the actual new standard as [VIDIOC\_G\_STD](#vidioc-g-std) does. When
no flags are given or the current input does not support the requested
standard the driver returns an `EINVAL` error code. When the standard set
is ambiguous drivers may return `EINVAL` or choose any of the requested
standards. If the current input or output does not support standard
video timings (e.g. if [ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput)
does not set the `V4L2_IN_CAP_STD` flag), then `ENODATA` error code is
returned.

Calling `VIDIOC_SUBDEV_S_STD` on a subdev device node that has been registered
in read-only mode is not allowed. An error is returned and the errno variable is
set to `-EPERM`.

## 7.41.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The [VIDIOC\_S\_STD](#vidioc-g-std) parameter was unsuitable.

ENODATA
:   Standard video timings are not supported for this input or output.

EPERM
:   `VIDIOC_SUBDEV_S_STD` has been called on a read-only subdevice.
