# 4.14.Metadata Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-meta.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.14. Metadata Interface

Metadata refers to any non-image data that supplements video frames with
additional information. This may include statistics computed over the image,
frame capture parameters supplied by the image source or device specific
parameters for specifying how the device processes images. This interface is
intended for transfer of metadata between the userspace and the hardware and
control of that operation.

The metadata interface is implemented on video device nodes. The device can be
dedicated to metadata or can support both video and metadata as specified in its
reported capabilities.

## 4.14.1. Querying Capabilities

Device nodes supporting the metadata capture interface set the
`V4L2_CAP_META_CAPTURE` flag in the `device_caps` field of the
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") structure returned by the [`VIDIOC_QUERYCAP()`](vidioc-querycap.html#c.V4L.VIDIOC_QUERYCAP "VIDIOC_QUERYCAP")
ioctl. That flag means the device can capture metadata to memory. Similarly,
device nodes supporting metadata output interface set the
`V4L2_CAP_META_OUTPUT` flag in the `device_caps` field of
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") structure. That flag means the device can read
metadata from memory.

At least one of the read/write or streaming I/O methods must be supported.

## 4.14.2. Data Format Negotiation

The metadata device uses the [Data Formats](format.html#format) ioctls to select the capture format.
The metadata buffer content format is bound to that selected format. In addition
to the basic [Data Formats](format.html#format) ioctls, the [`VIDIOC_ENUM_FMT()`](vidioc-enum-fmt.html#c.V4L.VIDIOC_ENUM_FMT "VIDIOC_ENUM_FMT") ioctl must be
supported as well.

To use the [Data Formats](format.html#format) ioctls applications set the `type` field of the
[`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") structure to `V4L2_BUF_TYPE_META_CAPTURE` or to
`V4L2_BUF_TYPE_META_OUTPUT` and use the [`v4l2_meta_format`](#c.V4L.v4l2_meta_format "v4l2_meta_format") `meta`
member of the `fmt` `union as` needed per the desired operation. Both drivers
and applications must set the remainder of the [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") structure
to 0.

Devices that capture metadata by line have the [`struct v4l2_fmtdesc`](vidioc-enum-fmt.html#c.V4L.v4l2_fmtdesc "V4L.v4l2_fmtdesc")
`V4L2_FMT_FLAG_META_LINE_BASED` flag set for [`VIDIOC_ENUM_FMT()`](vidioc-enum-fmt.html#c.V4L.VIDIOC_ENUM_FMT "VIDIOC_ENUM_FMT"). Such
devices can typically also [capture image data](dev-capture.html#capture). This primarily
involves devices that receive the data from a different devices such as a camera
sensor.

type v4l2\_meta\_format

struct v4l2\_meta\_format

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `dataformat` | The data format, set by the application. This is a little endian [four character code](vidioc-enum-fmt.html#v4l2-fourcc). V4L2 defines metadata formats in [Metadata Formats](meta-formats.html#meta-formats). |
| \_\_u32 | `buffersize` | Maximum buffer size in bytes required for data. The value is set by the driver. |
| \_\_u32 | `width` | Width of a line of metadata in Data Units. Valid when :c:type`v4l2\_fmtdesc` flag `V4L2_FMT_FLAG_META_LINE_BASED` is set, otherwise zero. See [`VIDIOC_ENUM_FMT()`](vidioc-enum-fmt.html#c.V4L.VIDIOC_ENUM_FMT "VIDIOC_ENUM_FMT"). |
| \_\_u32 | `height` | Number of rows of metadata. Valid when :c:type`v4l2\_fmtdesc` flag `V4L2_FMT_FLAG_META_LINE_BASED` is set, otherwise zero. See [`VIDIOC_ENUM_FMT()`](vidioc-enum-fmt.html#c.V4L.VIDIOC_ENUM_FMT "VIDIOC_ENUM_FMT"). |
| \_\_u32 | `bytesperline` | Offset in bytes between the beginning of two consecutive lines. Valid when :c:type`v4l2\_fmtdesc` flag `V4L2_FMT_FLAG_META_LINE_BASED` is set, otherwise zero. See [`VIDIOC_ENUM_FMT()`](vidioc-enum-fmt.html#c.V4L.VIDIOC_ENUM_FMT "VIDIOC_ENUM_FMT"). |
