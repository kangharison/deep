# 7.58.ioctl VIDIOC_SUBDEV_ENUM_MBUS_CODE

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-enum-mbus-code.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.58. ioctl VIDIOC\_SUBDEV\_ENUM\_MBUS\_CODE

## 7.58.1. Name

VIDIOC\_SUBDEV\_ENUM\_MBUS\_CODE - Enumerate media bus formats

## 7.58.2. Synopsis

VIDIOC\_SUBDEV\_ENUM\_MBUS\_CODE

`int ioctl(int fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE, struct v4l2_subdev_mbus_code_enum * argp)`

## 7.58.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_subdev_mbus_code_enum`](#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum").

## 7.58.4. Description

This call is used by the application to access the enumeration
of media bus formats for the selected pad.

The enumerations are defined by the driver, and indexed using the `index` field
of struct [`v4l2_subdev_mbus_code_enum`](#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum").
Each enumeration starts with the `index` of 0, and
the lowest invalid index marks the end of enumeration.

Therefore, to enumerate media bus formats available at a given sub-device pad,
initialize the `pad`, and `which` fields to desired values,
and set `index` to 0.
Then call the [ioctl VIDIOC\_SUBDEV\_ENUM\_MBUS\_CODE](#vidioc-subdev-enum-mbus-code) ioctl
with a pointer to this structure.

A successful call will return with the `code` field filled in
with a mbus code value.
Repeat with increasing `index` until `EINVAL` is received.
`EINVAL` means that either `pad` is invalid,
or that there are no more codes available at this pad.

The driver must not return the same value of `code` for different indices
at the same pad.

Available media bus formats may depend on the current ‘try’ formats at
other pads of the sub-device, as well as on the current active links.
See [ioctl VIDIOC\_SUBDEV\_G\_FMT, VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) for more
information about the try formats.

type v4l2\_subdev\_mbus\_code\_enum

struct v4l2\_subdev\_mbus\_code\_enum

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `pad` | Pad number as reported by the media controller API. Filled in by the application. |
| \_\_u32 | `index` | Index of the mbus code in the enumeration belonging to the given pad. Filled in by the application. |
| \_\_u32 | `code` | The media bus format code, as defined in [Media Bus Formats](subdev-formats.html#v4l2-mbus-format). Filled in by the driver. |
| \_\_u32 | `which` | Media bus format codes to be enumerated, from enum [v4l2\_subdev\_format\_whence](vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence). |
| \_\_u32 | `flags` | See [Subdev Media Bus Code Enumerate Flags](#v4l2-subdev-mbus-code-flags) |
| \_\_u32 | `stream` | Stream identifier. |
| \_\_u32 | `reserved`[6] | Reserved for future extensions. Applications and drivers must set the array to zero. |

Subdev Media Bus Code Enumerate Flags

|  |  |  |
| --- | --- | --- |
| V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_COLORSPACE | 0x00000001 | The driver allows the application to try to change the default colorspace encoding. The application can ask to configure the colorspace of the subdevice when calling the [VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) ioctl with [V4L2\_MBUS\_FRAMEFMT\_SET\_CSC](subdev-formats.html#mbus-framefmt-set-csc) set. See [Media Bus Formats](subdev-formats.html#v4l2-mbus-format) on how to do this. |
| V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_XFER\_FUNC | 0x00000002 | The driver allows the application to try to change the default transform function. The application can ask to configure the transform function of the subdevice when calling the [VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) ioctl with [V4L2\_MBUS\_FRAMEFMT\_SET\_CSC](subdev-formats.html#mbus-framefmt-set-csc) set. See [Media Bus Formats](subdev-formats.html#v4l2-mbus-format) on how to do this. |
| V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_YCBCR\_ENC | 0x00000004 | The driver allows the application to try to change the default Y’CbCr encoding. The application can ask to configure the Y’CbCr encoding of the subdevice when calling the [VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) ioctl with [V4L2\_MBUS\_FRAMEFMT\_SET\_CSC](subdev-formats.html#mbus-framefmt-set-csc) set. See [Media Bus Formats](subdev-formats.html#v4l2-mbus-format) on how to do this. |
| V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_HSV\_ENC | 0x00000004 | The driver allows the application to try to change the default HSV encoding. The application can ask to configure the HSV encoding of the subdevice when calling the [VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) ioctl with [V4L2\_MBUS\_FRAMEFMT\_SET\_CSC](subdev-formats.html#mbus-framefmt-set-csc) set. See [Media Bus Formats](subdev-formats.html#v4l2-mbus-format) on how to do this. |
| V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_QUANTIZATION | 0x00000008 | The driver allows the application to try to change the default quantization. The application can ask to configure the quantization of the subdevice when calling the [VIDIOC\_SUBDEV\_S\_FMT](vidioc-subdev-g-fmt.html#vidioc-subdev-g-fmt) ioctl with [V4L2\_MBUS\_FRAMEFMT\_SET\_CSC](subdev-formats.html#mbus-framefmt-set-csc) set. See [Media Bus Formats](subdev-formats.html#v4l2-mbus-format) on how to do this. |

## 7.58.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`v4l2_subdev_mbus_code_enum`](#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum") `pad` references a
    non-existing pad, the `which` field has an unsupported value, or the
    `index` field is out of bounds.
