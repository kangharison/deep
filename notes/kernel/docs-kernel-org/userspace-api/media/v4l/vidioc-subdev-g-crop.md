# 7.59.ioctl VIDIOC_SUBDEV_G_CROP, VIDIOC_SUBDEV_S_CROP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-g-crop.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.59. ioctl VIDIOC\_SUBDEV\_G\_CROP, VIDIOC\_SUBDEV\_S\_CROP

## 7.59.1. Name

VIDIOC\_SUBDEV\_G\_CROP - VIDIOC\_SUBDEV\_S\_CROP - Get or set the crop rectangle on a subdev pad

## 7.59.2. Synopsis

VIDIOC\_SUBDEV\_G\_CROP

`int ioctl(int fd, VIDIOC_SUBDEV_G_CROP, struct v4l2_subdev_crop *argp)`

VIDIOC\_SUBDEV\_S\_CROP

`int ioctl(int fd, VIDIOC_SUBDEV_S_CROP, const struct v4l2_subdev_crop *argp)`

## 7.59.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_subdev_crop`](#c.V4L.v4l2_subdev_crop "v4l2_subdev_crop").

## 7.59.4. Description

Note

This is an [Obsolete API Elements](hist-v4l2.html#obsolete) interface and may be removed in the future. It is
superseded by [the selection API](vidioc-subdev-g-selection.html#vidioc-subdev-g-selection). No new
extensions to the [`v4l2_subdev_crop`](#c.V4L.v4l2_subdev_crop "v4l2_subdev_crop") structure will be accepted.

To retrieve the current crop rectangle applications set the `pad`
field of a struct [`v4l2_subdev_crop`](#c.V4L.v4l2_subdev_crop "v4l2_subdev_crop") to the
desired pad number as reported by the media API and the `which` field
to `V4L2_SUBDEV_FORMAT_ACTIVE`. They then call the
`VIDIOC_SUBDEV_G_CROP` ioctl with a pointer to this structure. The
driver fills the members of the `rect` field or returns `EINVAL` error
code if the input arguments are invalid, or if cropping is not supported
on the given pad.

To change the current crop rectangle applications set both the `pad`
and `which` fields and all members of the `rect` field. They then
call the `VIDIOC_SUBDEV_S_CROP` ioctl with a pointer to this
structure. The driver verifies the requested crop rectangle, adjusts it
based on the hardware capabilities and configures the device. Upon
return the struct [`v4l2_subdev_crop`](#c.V4L.v4l2_subdev_crop "v4l2_subdev_crop")
contains the current format as would be returned by a
`VIDIOC_SUBDEV_G_CROP` call.

Applications can query the device capabilities by setting the `which`
to `V4L2_SUBDEV_FORMAT_TRY`. When set, ‘try’ crop rectangles are not
applied to the device by the driver, but are mangled exactly as active
crop rectangles and stored in the sub-device file handle. Two
applications querying the same sub-device would thus not interact with
each other.

If the subdev device node has been registered in read-only mode, calls to
`VIDIOC_SUBDEV_S_CROP` are only valid if the `which` field is set to
`V4L2_SUBDEV_FORMAT_TRY`, otherwise an error is returned and the errno
variable is set to `-EPERM`.

Drivers must not return an error solely because the requested crop
rectangle doesn’t match the device capabilities. They must instead
modify the rectangle to match what the hardware can provide. The
modified format should be as close as possible to the original request.

type v4l2\_subdev\_crop

struct v4l2\_subdev\_crop

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `pad` | Pad number as reported by the media framework. |
| \_\_u32 | `which` | Crop rectangle to get or set, from enum [v4l2\_subdev\_format\_whence](vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence). |
| struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") | `rect` | Crop rectangle boundaries, in pixels. |
| \_\_u32 | `stream` | Stream identifier. |
| \_\_u32 | `reserved`[7] | Reserved for future extensions. Applications and drivers must set the array to zero. |

## 7.59.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EBUSY
:   The crop rectangle can’t be changed because the pad is currently
    busy. This can be caused, for instance, by an active video stream on
    the pad. The ioctl must not be retried without performing another
    action to fix the problem first. Only returned by
    `VIDIOC_SUBDEV_S_CROP`

EINVAL
:   The struct [`v4l2_subdev_crop`](#c.V4L.v4l2_subdev_crop "v4l2_subdev_crop") `pad` references a non-existing pad,
    the `which` field has an unsupported value, or cropping is not supported
    on the given subdev pad.

EPERM
:   The `VIDIOC_SUBDEV_S_CROP` ioctl has been called on a read-only subdevice
    and the `which` field is set to `V4L2_SUBDEV_FORMAT_ACTIVE`.
