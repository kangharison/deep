# 7.24.ioctl VIDIOC_G_CROP, VIDIOC_S_CROP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-crop.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.24. ioctl VIDIOC\_G\_CROP, VIDIOC\_S\_CROP

## 7.24.1. Name

VIDIOC\_G\_CROP - VIDIOC\_S\_CROP - Get or set the current cropping rectangle

## 7.24.2. Synopsis

VIDIOC\_G\_CROP

`int ioctl(int fd, VIDIOC_G_CROP, struct v4l2_crop *argp)`

VIDIOC\_S\_CROP

`int ioctl(int fd, VIDIOC_S_CROP, const struct v4l2_crop *argp)`

## 7.24.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_crop`](#c.V4L.v4l2_crop "v4l2_crop").

## 7.24.4. Description

To query the cropping rectangle size and position applications set the
`type` field of a struct [`v4l2_crop`](#c.V4L.v4l2_crop "v4l2_crop") structure to the
respective buffer (stream) type and call the [VIDIOC\_G\_CROP](#vidioc-g-crop) ioctl
with a pointer to this structure. The driver fills the rest of the
structure or returns the `EINVAL` error code if cropping is not supported.

To change the cropping rectangle applications initialize the `type`
and struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") substructure named `c` of a
v4l2\_crop structure and call the [VIDIOC\_S\_CROP](#vidioc-g-crop) ioctl with a pointer
to this structure.

The driver first adjusts the requested dimensions against hardware
limits, i. e. the bounds given by the capture/output window, and it
rounds to the closest possible values of horizontal and vertical offset,
width and height. In particular the driver must round the vertical
offset of the cropping rectangle to frame lines modulo two, such that
the field order cannot be confused.

Second the driver adjusts the image size (the opposite rectangle of the
scaling process, source or target depending on the data direction) to
the closest size possible while maintaining the current horizontal and
vertical scaling factor.

Finally the driver programs the hardware with the actual cropping and
image parameters. [VIDIOC\_S\_CROP](#vidioc-g-crop) is a write-only ioctl, it does not
return the actual parameters. To query them applications must call
[VIDIOC\_G\_CROP](#vidioc-g-crop) and [ioctl VIDIOC\_G\_FMT, VIDIOC\_S\_FMT, VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt). When the
parameters are unsuitable the application may modify the cropping or
image parameters and repeat the cycle until satisfactory parameters have
been negotiated.

When cropping is not supported then no parameters are changed and
[VIDIOC\_S\_CROP](#vidioc-g-crop) returns the `EINVAL` error code.

type v4l2\_crop

struct v4l2\_crop

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | Type of the data stream, set by the application. Only these types are valid here: `V4L2_BUF_TYPE_VIDEO_CAPTURE`, `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`, `V4L2_BUF_TYPE_VIDEO_OUTPUT`, `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE` and `V4L2_BUF_TYPE_VIDEO_OVERLAY`. See [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type") and the note below. |
| struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") | `c` | Cropping rectangle. The same co-ordinate system as for struct [`v4l2_cropcap`](vidioc-cropcap.html#c.V4L.v4l2_cropcap "v4l2_cropcap") `bounds` is used. |

Note

Unfortunately in the case of multiplanar buffer types
(`V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE` and `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE`)
this API was messed up with regards to how the [`v4l2_crop`](#c.V4L.v4l2_crop "v4l2_crop") `type` field
should be filled in. Some drivers only accepted the `_MPLANE` buffer type while
other drivers only accepted a non-multiplanar buffer type (i.e. without the
`_MPLANE` at the end).

Starting with kernel 4.13 both variations are allowed.

## 7.24.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

ENODATA
:   Cropping is not supported for this input or output.
