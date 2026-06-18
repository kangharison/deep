# 7.34.ioctl VIDIOC_G_JPEGCOMP, VIDIOC_S_JPEGCOMP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-jpegcomp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.34. ioctl VIDIOC\_G\_JPEGCOMP, VIDIOC\_S\_JPEGCOMP

## 7.34.1. Name

VIDIOC\_G\_JPEGCOMP - VIDIOC\_S\_JPEGCOMP

## 7.34.2. Synopsis

VIDIOC\_G\_JPEGCOMP

`int ioctl(int fd, VIDIOC_G_JPEGCOMP, v4l2_jpegcompression *argp)`

VIDIOC\_S\_JPEGCOMP

`int ioctl(int fd, VIDIOC_S_JPEGCOMP, const v4l2_jpegcompression *argp)`

## 7.34.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_jpegcompression`](#c.V4L.v4l2_jpegcompression "v4l2_jpegcompression").

## 7.34.4. Description

These ioctls are **deprecated**. New drivers and applications should use
[JPEG class controls](ext-ctrls-jpeg.html#jpeg-controls) for image quality and JPEG
markers control.

[to do]

Ronald Bultje elaborates:

APP is some application-specific information. The application can set it
itself, and it’ll be stored in the JPEG-encoded fields (eg; interlacing
information for in an AVI or so). COM is the same, but it’s comments,
like ‘encoded by me’ or so.

jpeg\_markers describes whether the huffman tables, quantization tables
and the restart interval information (all JPEG-specific stuff) should be
stored in the JPEG-encoded fields. These define how the JPEG field is
encoded. If you omit them, applications assume you’ve used standard
encoding. You usually do want to add them.

type v4l2\_jpegcompression

struct v4l2\_jpegcompression

|  |  |  |
| --- | --- | --- |
| int | `quality` | Deprecated. If [V4L2\_CID\_JPEG\_COMPRESSION\_QUALITY](ext-ctrls-jpeg.html#jpeg-quality-control) control is exposed by a driver applications should use it instead and ignore this field. |
| int | `APPn` |  |
| int | `APP_len` |  |
| char | `APP_data`[60] |  |
| int | `COM_len` |  |
| char | `COM_data`[60] |  |
| \_\_u32 | `jpeg_markers` | See [JPEG Markers Flags](#jpeg-markers). Deprecated. If [V4L2\_CID\_JPEG\_ACTIVE\_MARKER](ext-ctrls-jpeg.html#jpeg-active-marker-control) control is exposed by a driver applications should use it instead and ignore this field. |

JPEG Markers Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_JPEG_MARKER_DHT` | (1<<3) | Define Huffman Tables |
| `V4L2_JPEG_MARKER_DQT` | (1<<4) | Define Quantization Tables |
| `V4L2_JPEG_MARKER_DRI` | (1<<5) | Define Restart Interval |
| `V4L2_JPEG_MARKER_COM` | (1<<6) | Comment segment |
| `V4L2_JPEG_MARKER_APP` | (1<<7) | App segment, driver will always use APP0 |

## 7.34.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
