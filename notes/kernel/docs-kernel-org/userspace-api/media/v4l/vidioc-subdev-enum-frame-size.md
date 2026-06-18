# 7.57.ioctl VIDIOC_SUBDEV_ENUM_FRAME_SIZE

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-enum-frame-size.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.57. ioctl VIDIOC\_SUBDEV\_ENUM\_FRAME\_SIZE

## 7.57.1. Name

VIDIOC\_SUBDEV\_ENUM\_FRAME\_SIZE - Enumerate media bus frame sizes

## 7.57.2. Synopsis

VIDIOC\_SUBDEV\_ENUM\_FRAME\_SIZE

`int ioctl(int fd, VIDIOC_SUBDEV_ENUM_FRAME_SIZE, struct v4l2_subdev_frame_size_enum * argp)`

## 7.57.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_subdev_frame_size_enum`](#c.V4L.v4l2_subdev_frame_size_enum "v4l2_subdev_frame_size_enum").

## 7.57.4. Description

This ioctl allows applications to access the enumeration of frame sizes
supported by a sub-device on the specified pad
for the specified media bus format.
Supported formats can be retrieved with the
[ioctl VIDIOC\_SUBDEV\_ENUM\_MBUS\_CODE](vidioc-subdev-enum-mbus-code.html#vidioc-subdev-enum-mbus-code)
ioctl.

The enumerations are defined by the driver, and indexed using the `index` field
of the struct [`v4l2_subdev_frame_size_enum`](#c.V4L.v4l2_subdev_frame_size_enum "v4l2_subdev_frame_size_enum").
Each pair of `pad` and `code` correspond to a separate enumeration.
Each enumeration starts with the `index` of 0, and
the lowest invalid index marks the end of the enumeration.

Therefore, to enumerate frame sizes allowed on the specified pad
and using the specified mbus format, initialize the
`pad`, `which`, and `code` fields to desired values,
and set `index` to 0.
Then call the [ioctl VIDIOC\_SUBDEV\_ENUM\_FRAME\_SIZE](#vidioc-subdev-enum-frame-size) ioctl with a pointer to the
structure.

A successful call will return with minimum and maximum frame sizes filled in.
Repeat with increasing `index` until `EINVAL` is received.
`EINVAL` means that either no more entries are available in the enumeration,
or that an input parameter was invalid.

Sub-devices that only support discrete frame sizes (such as most
sensors) will return one or more frame sizes with identical minimum and
maximum values.

Not all possible sizes in given [minimum, maximum] ranges need to be
supported. For instance, a scaler that uses a fixed-point scaling ratio
might not be able to produce every frame size between the minimum and
maximum values. Applications must use the
[VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) ioctl to try the
sub-device for an exact supported frame size.

Available frame sizes may depend on the current ‘try’ formats at other
pads of the sub-device, as well as on the current active links and the
current values of V4L2 controls. See
[ioctl VIDIOC\_SUBDEV\_G\_FMT, VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) for more
information about try formats.

type v4l2\_subdev\_frame\_size\_enum

struct v4l2\_subdev\_frame\_size\_enum

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | Index of the frame size in the enumeration belonging to the given pad and format. Filled in by the application. |
| \_\_u32 | `pad` | Pad number as reported by the media controller API. Filled in by the application. |
| \_\_u32 | `code` | The media bus format code, as defined in [Media Bus Formats](subdev-formats.html#v4l2-mbus-format). Filled in by the application. |
| \_\_u32 | `min_width` | Minimum frame width, in pixels. Filled in by the driver. |
| \_\_u32 | `max_width` | Maximum frame width, in pixels. Filled in by the driver. |
| \_\_u32 | `min_height` | Minimum frame height, in pixels. Filled in by the driver. |
| \_\_u32 | `max_height` | Maximum frame height, in pixels. Filled in by the driver. |
| \_\_u32 | `which` | Frame sizes to be enumerated, from enum [v4l2\_subdev\_format\_whence](vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence). |
| \_\_u32 | `stream` | Stream identifier. |
| \_\_u32 | `reserved`[7] | Reserved for future extensions. Applications and drivers must set the array to zero. |

## 7.57.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`v4l2_subdev_frame_size_enum`](#c.V4L.v4l2_subdev_frame_size_enum "v4l2_subdev_frame_size_enum") `pad` references a
    non-existing pad, the `which` field has an unsupported value, the `code`
    is invalid for the given pad, or the `index` field is out of bounds.
