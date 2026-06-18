# 7.61.ioctl VIDIOC_SUBDEV_G_FRAME_INTERVAL, VIDIOC_SUBDEV_S_FRAME_INTERVAL

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-g-frame-interval.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.61. ioctl VIDIOC\_SUBDEV\_G\_FRAME\_INTERVAL, VIDIOC\_SUBDEV\_S\_FRAME\_INTERVAL

## 7.61.1. Name

VIDIOC\_SUBDEV\_G\_FRAME\_INTERVAL - VIDIOC\_SUBDEV\_S\_FRAME\_INTERVAL - Get or set the frame interval on a subdev pad

## 7.61.2. Synopsis

VIDIOC\_SUBDEV\_G\_FRAME\_INTERVAL

`int ioctl(int fd, VIDIOC_SUBDEV_G_FRAME_INTERVAL, struct v4l2_subdev_frame_interval *argp)`

VIDIOC\_SUBDEV\_S\_FRAME\_INTERVAL

`int ioctl(int fd, VIDIOC_SUBDEV_S_FRAME_INTERVAL, struct v4l2_subdev_frame_interval *argp)`

## 7.61.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_subdev_frame_interval`](#c.V4L.v4l2_subdev_frame_interval "v4l2_subdev_frame_interval").

## 7.61.4. Description

These ioctls are used to get and set the frame interval at specific
subdev pads in the image pipeline. The frame interval only makes sense
for sub-devices that can control the frame period on their own. This
includes, for instance, image sensors and TV tuners. Sub-devices that
don’t support frame intervals must not implement these ioctls.

To retrieve the current frame interval applications set the `pad`
field of a struct
[`v4l2_subdev_frame_interval`](#c.V4L.v4l2_subdev_frame_interval "v4l2_subdev_frame_interval") to
the desired pad number as reported by the media controller API. When
they call the `VIDIOC_SUBDEV_G_FRAME_INTERVAL` ioctl with a pointer to
this structure the driver fills the members of the `interval` field.

To change the current frame interval applications set both the `pad`
field and all members of the `interval` field. When they call the
`VIDIOC_SUBDEV_S_FRAME_INTERVAL` ioctl with a pointer to this
structure the driver verifies the requested interval, adjusts it based
on the hardware capabilities and configures the device. Upon return the
struct
[`v4l2_subdev_frame_interval`](#c.V4L.v4l2_subdev_frame_interval "v4l2_subdev_frame_interval")
contains the current frame interval as would be returned by a
`VIDIOC_SUBDEV_G_FRAME_INTERVAL` call.

If the subdev device node has been registered in read-only mode, calls to
`VIDIOC_SUBDEV_S_FRAME_INTERVAL` are only valid if the `which` field is set
to `V4L2_SUBDEV_FORMAT_TRY`, otherwise an error is returned and the errno
variable is set to `-EPERM`.

Drivers must not return an error solely because the requested interval
doesn’t match the device capabilities. They must instead modify the
interval to match what the hardware can provide. The modified interval
should be as close as possible to the original request.

Changing the frame interval shall never change the format. Changing the
format, on the other hand, may change the frame interval.

Sub-devices that support the frame interval ioctls should implement them
on a single pad only. Their behaviour when supported on multiple pads of
the same sub-device is not defined.

type v4l2\_subdev\_frame\_interval

struct v4l2\_subdev\_frame\_interval

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `pad` | Pad number as reported by the media controller API. |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `interval` | Period, in seconds, between consecutive video frames. |
| \_\_u32 | `stream` | Stream identifier. |
| \_\_u32 | `which` | Active or try frame interval, from enum [v4l2\_subdev\_format\_whence](vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence). |
| \_\_u32 | `reserved`[7] | Reserved for future extensions. Applications and drivers must set the array to zero. |

## 7.61.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EBUSY
:   The frame interval can’t be changed because the pad is currently
    busy. This can be caused, for instance, by an active video stream on
    the pad. The ioctl must not be retried without performing another
    action to fix the problem first. Only returned by
    `VIDIOC_SUBDEV_S_FRAME_INTERVAL`

EINVAL
:   The struct [`v4l2_subdev_frame_interval`](#c.V4L.v4l2_subdev_frame_interval "v4l2_subdev_frame_interval") `pad` references a
    non-existing pad, the `which` field has an unsupported value, or the pad
    doesn’t support frame intervals.

EPERM
:   The `VIDIOC_SUBDEV_S_FRAME_INTERVAL` ioctl has been called on a read-only
    subdevice and the `which` field is set to `V4L2_SUBDEV_FORMAT_ACTIVE`.
