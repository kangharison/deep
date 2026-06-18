# 7.53.ioctl VIDIOC_REMOVE_BUFS

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-remove-bufs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.53. ioctl VIDIOC\_REMOVE\_BUFS

## 7.53.1. Name

VIDIOC\_REMOVE\_BUFS - Removes buffers from a queue

## 7.53.2. Synopsis

VIDIOC\_REMOVE\_BUFS

`int ioctl(int fd, VIDIOC_REMOVE_BUFS, struct v4l2_remove_buffers *argp)`

## 7.53.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_remove_buffers`](#c.V4L.v4l2_remove_buffers "v4l2_remove_buffers").

## 7.53.4. Description

Applications can optionally call the [ioctl VIDIOC\_REMOVE\_BUFS](#vidioc-remove-bufs) ioctl to
remove buffers from a queue.
[ioctl VIDIOC\_CREATE\_BUFS](vidioc-create-bufs.html#vidioc-create-bufs) ioctl support is mandatory to enable [ioctl VIDIOC\_REMOVE\_BUFS](#vidioc-remove-bufs).
This ioctl is available if the `V4L2_BUF_CAP_SUPPORTS_REMOVE_BUFS` capability
is set on the queue when [`VIDIOC_REQBUFS()`](vidioc-reqbufs.html#c.V4L.VIDIOC_REQBUFS "VIDIOC_REQBUFS") or [`VIDIOC_CREATE_BUFS()`](vidioc-create-bufs.html#c.V4L.VIDIOC_CREATE_BUFS "VIDIOC_CREATE_BUFS")
are invoked.

type v4l2\_remove\_buffers

struct v4l2\_remove\_buffers

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | The starting buffer index to remove. This field is ignored if count == 0. |
| \_\_u32 | `count` | The number of buffers to be removed with indices ‘index’ until ‘index + count - 1’. All buffers in this range must be valid and in DEQUEUED state. [ioctl VIDIOC\_REMOVE\_BUFS](#vidioc-remove-bufs) will always check the validity of `type`, if it is invalid it returns `EINVAL` error code. If count is set to 0 [ioctl VIDIOC\_REMOVE\_BUFS](#vidioc-remove-bufs) will do nothing and return 0. |
| \_\_u32 | `type` | Type of the stream or buffers, this is the same as the struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") `type` field. See [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type") for valid values. |
| \_\_u32 | `reserved`[13] | A place holder for future extensions. Drivers and applications must set the array to zero. |

## 7.53.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter. If an error occurs, no
buffers will be freed and one of the error codes below will be returned:

EBUSY
:   File I/O is in progress.
    One or more of the buffers in the range `index` to `index + count - 1` are not
    in DEQUEUED state.

EINVAL
:   One or more of the buffers in the range `index` to `index + count - 1` do not
    exist in the queue.
    The buffer type (`type` field) is not valid.
