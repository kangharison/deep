# 7.44.ioctl VIDIOC_OVERLAY

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-overlay.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.44. ioctl VIDIOC\_OVERLAY

## 7.44.1. Name

VIDIOC\_OVERLAY - Start or stop video overlay

## 7.44.2. Synopsis

VIDIOC\_OVERLAY

`int ioctl(int fd, VIDIOC_OVERLAY, const int *argp)`

## 7.44.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to an integer.

## 7.44.4. Description

This ioctl is part of the [video overlay](dev-overlay.html#overlay) I/O method.
Applications call [ioctl VIDIOC\_OVERLAY](#vidioc-overlay) to start or stop the overlay. It
takes a pointer to an integer which must be set to zero by the
application to stop overlay, to one to start.

Drivers do not support [ioctl VIDIOC\_STREAMON, VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon) or
[VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon) with
`V4L2_BUF_TYPE_VIDEO_OVERLAY`.

## 7.44.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The overlay parameters have not been set up. See [Video Overlay Interface](dev-overlay.html#overlay)
    for the necessary steps.
