# 2.2.Multi-planar format structures

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-v4l2-mplane.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.2. Multi-planar format structures

The struct [`v4l2_plane_pix_format`](#c.V4L.v4l2_plane_pix_format "v4l2_plane_pix_format") structures define size
and layout for each of the planes in a multi-planar format. The
struct [`v4l2_pix_format_mplane`](#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") structure contains
information common to all planes (such as image width and height) and an
array of struct [`v4l2_plane_pix_format`](#c.V4L.v4l2_plane_pix_format "v4l2_plane_pix_format") structures,
describing all planes of that format.

type v4l2\_plane\_pix\_format

struct v4l2\_plane\_pix\_format

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `sizeimage` | Maximum size in bytes required for image data in this plane, set by the driver. When the image consists of variable length compressed data this is the number of bytes required by the codec to support the worst-case compression scenario.  The driver will set the value for uncompressed images.  Clients are allowed to set the sizeimage field for variable length compressed data flagged with `V4L2_FMT_FLAG_COMPRESSED` at [ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt), but the driver may ignore it and set the value itself, or it may modify the provided value based on alignment requirements or minimum/maximum size requirements. If the client wants to leave this to the driver, then it should set sizeimage to 0. |
| \_\_u32 | `bytesperline` | Distance in bytes between the leftmost pixels in two adjacent lines. See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u16 | `reserved[6]` | Reserved for future extensions. Should be zeroed by drivers and applications. |

type v4l2\_pix\_format\_mplane

struct v4l2\_pix\_format\_mplane

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `width` | Image width in pixels. See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u32 | `height` | Image height in pixels. See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u32 | `pixelformat` | The pixel format. Both single- and multi-planar four character codes can be used. |
| \_\_u32 | `field` | Field order, from enum [`v4l2_field`](field-order.html#c.V4L.v4l2_field "v4l2_field"). See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u32 | `colorspace` | Colorspace encoding, from enum [`v4l2_colorspace`](colorspaces-defs.html#c.V4L.v4l2_colorspace "v4l2_colorspace"). See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| struct [`v4l2_plane_pix_format`](#c.V4L.v4l2_plane_pix_format "v4l2_plane_pix_format") | `plane_fmt[VIDEO_MAX_PLANES]` | An array of structures describing format of each plane this pixel format consists of. The number of valid entries in this array has to be put in the `num_planes` field. |
| \_\_u8 | `num_planes` | Number of planes (i.e. separate memory buffers) for this format and the number of valid entries in the `plane_fmt` array. |
| \_\_u8 | `flags` | Flags set by the application or driver, see [Format Flags](pixfmt-v4l2.html#format-flags). |
| union { | (anonymous) | |
| \_\_u8 | `ycbcr_enc` | Y’CbCr encoding, from enum [`v4l2_ycbcr_encoding`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "v4l2_ycbcr_encoding"). See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u8 | `hsv_enc` | HSV encoding, from enum [`v4l2_hsv_encoding`](colorspaces-defs.html#c.V4L.v4l2_hsv_encoding "v4l2_hsv_encoding"). See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| } |  | |
| \_\_u8 | `quantization` | Quantization range, from enum [`v4l2_quantization`](colorspaces-defs.html#c.V4L.v4l2_quantization "v4l2_quantization"). See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u8 | `xfer_func` | Transfer function, from enum [`v4l2_xfer_func`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "v4l2_xfer_func"). See struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format"). |
| \_\_u8 | `reserved[7]` | Reserved for future extensions. Should be zeroed by drivers and applications. |
