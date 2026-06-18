# 7.31.ioctl VIDIOC_G_FMT, VIDIOC_S_FMT, VIDIOC_TRY_FMT

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-fmt.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.31. ioctl VIDIOC\_G\_FMT, VIDIOC\_S\_FMT, VIDIOC\_TRY\_FMT

## 7.31.1. Name

VIDIOC\_G\_FMT - VIDIOC\_S\_FMT - VIDIOC\_TRY\_FMT - Get or set the data format, try a format

## 7.31.2. Synopsis

VIDIOC\_G\_FMT

`int ioctl(int fd, VIDIOC_G_FMT, struct v4l2_format *argp)`

VIDIOC\_S\_FMT

`int ioctl(int fd, VIDIOC_S_FMT, struct v4l2_format *argp)`

VIDIOC\_TRY\_FMT

`int ioctl(int fd, VIDIOC_TRY_FMT, struct v4l2_format *argp)`

## 7.31.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_format`](#c.V4L.v4l2_format "v4l2_format").

## 7.31.4. Description

These ioctls are used to negotiate the format of data (typically image
format) exchanged between driver and application.

To query the current parameters applications set the `type` field of a
struct [`v4l2_format`](#c.V4L.v4l2_format "v4l2_format") to the respective buffer (stream)
type. For example video capture devices use
`V4L2_BUF_TYPE_VIDEO_CAPTURE` or
`V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`. When the application calls the
[VIDIOC\_G\_FMT](#vidioc-g-fmt) ioctl with a pointer to this structure the driver fills
the respective member of the `fmt` union. In case of video capture
devices that is either the struct
[`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") `pix` or the struct
[`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") `pix_mp`
member. When the requested buffer type is not supported drivers return
an `EINVAL` error code.

To change the current format parameters applications initialize the
`type` field and all fields of the respective `fmt` `union member`.
For details see the documentation of the various devices types in
[Interfaces](devices.html#devices). Good practice is to query the current parameters
first, and to modify only those parameters not suitable for the
application. When the application calls the [VIDIOC\_S\_FMT](#vidioc-g-fmt) ioctl with
a pointer to a struct [`v4l2_format`](#c.V4L.v4l2_format "v4l2_format") structure the driver
checks and adjusts the parameters against hardware abilities. Drivers
should not return an error code unless the `type` field is invalid,
this is a mechanism to fathom device capabilities and to approach
parameters acceptable for both the application and driver. On success
the driver may program the hardware, allocate resources and generally
prepare for data exchange. Finally the [VIDIOC\_S\_FMT](#vidioc-g-fmt) ioctl returns
the current format parameters as [VIDIOC\_G\_FMT](#vidioc-g-fmt) does. Very simple,
inflexible devices may even ignore all input and always return the
default parameters. However all V4L2 devices exchanging data with the
application must implement the [VIDIOC\_G\_FMT](#vidioc-g-fmt) and [VIDIOC\_S\_FMT](#vidioc-g-fmt)
ioctl. When the requested buffer type is not supported drivers return an
EINVAL error code on a [VIDIOC\_S\_FMT](#vidioc-g-fmt) attempt. When I/O is already in
progress or the resource is not available for other reasons drivers
return the `EBUSY` error code.

The [VIDIOC\_TRY\_FMT](#vidioc-g-fmt) ioctl is equivalent to [VIDIOC\_S\_FMT](#vidioc-g-fmt) with one
exception: it does not change driver state. It can also be called at any
time, never returning `EBUSY`. This function is provided to negotiate
parameters, to learn about hardware limitations, without disabling I/O
or possibly time consuming hardware preparations. Although strongly
recommended drivers are not required to implement this ioctl.

The format as returned by [VIDIOC\_TRY\_FMT](#vidioc-g-fmt) must be identical to what
[VIDIOC\_S\_FMT](#vidioc-g-fmt) returns for the same input or output.

type v4l2\_format

struct v4l2\_format

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | Type of the data stream, see [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type"). |
| union { | `fmt` | |
| struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") | `pix` | Definition of an image format, see [Image Formats](pixfmt.html#pixfmt), used by video capture and output devices. |
| struct [`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") | `pix_mp` | Definition of an image format, see [Image Formats](pixfmt.html#pixfmt), used by video capture and output devices that support the [multi-planar version of the API](planar-apis.html#planar-apis). |
| struct [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "v4l2_window") | `win` | Definition of an overlaid image, see [Video Overlay Interface](dev-overlay.html#overlay), used by video overlay devices. |
| struct [`v4l2_vbi_format`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "v4l2_vbi_format") | `vbi` | Raw VBI capture or output parameters. This is discussed in more detail in [Raw VBI Data Interface](dev-raw-vbi.html#raw-vbi). Used by raw VBI capture and output devices. |
| struct [`v4l2_sliced_vbi_format`](dev-sliced-vbi.html#c.V4L.v4l2_sliced_vbi_format "v4l2_sliced_vbi_format") | `sliced` | Sliced VBI capture or output parameters. See [Sliced VBI Data Interface](dev-sliced-vbi.html#sliced) for details. Used by sliced VBI capture and output devices. |
| struct [`v4l2_sdr_format`](dev-sdr.html#c.V4L.v4l2_sdr_format "v4l2_sdr_format") | `sdr` | Definition of a data format, see [Image Formats](pixfmt.html#pixfmt), used by SDR capture and output devices. |
| struct [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") | `meta` | Definition of a metadata format, see [Metadata Formats](meta-formats.html#meta-formats), used by metadata capture devices. |
| \_\_u8 | `raw_data`[200] | Place holder for future extensions. |
| } |  | |

## 7.31.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`v4l2_format`](#c.V4L.v4l2_format "v4l2_format") `type` field is
    invalid or the requested buffer type not supported.

EBUSY
:   The device is busy and cannot change the format. This could be
    because or the device is streaming or buffers are allocated or
    queued to the driver. Relevant for [VIDIOC\_S\_FMT](#vidioc-g-fmt) only.
