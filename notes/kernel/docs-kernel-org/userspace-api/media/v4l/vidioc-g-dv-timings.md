# 7.26.ioctl VIDIOC_G_DV_TIMINGS, VIDIOC_S_DV_TIMINGS

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-dv-timings.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.26. ioctl VIDIOC\_G\_DV\_TIMINGS, VIDIOC\_S\_DV\_TIMINGS

## 7.26.1. Name

VIDIOC\_G\_DV\_TIMINGS - VIDIOC\_S\_DV\_TIMINGS - VIDIOC\_SUBDEV\_G\_DV\_TIMINGS - VIDIOC\_SUBDEV\_S\_DV\_TIMINGS - Get or set DV timings for input or output

## 7.26.2. Synopsis

VIDIOC\_G\_DV\_TIMINGS

`int ioctl(int fd, VIDIOC_G_DV_TIMINGS, struct v4l2_dv_timings *argp)`

VIDIOC\_S\_DV\_TIMINGS

`int ioctl(int fd, VIDIOC_S_DV_TIMINGS, struct v4l2_dv_timings *argp)`

VIDIOC\_SUBDEV\_G\_DV\_TIMINGS

`int ioctl(int fd, VIDIOC_SUBDEV_G_DV_TIMINGS, struct v4l2_dv_timings *argp)`

VIDIOC\_SUBDEV\_S\_DV\_TIMINGS

`int ioctl(int fd, VIDIOC_SUBDEV_S_DV_TIMINGS, struct v4l2_dv_timings *argp)`

## 7.26.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_dv_timings`](#c.V4L.v4l2_dv_timings "v4l2_dv_timings").

## 7.26.4. Description

To set DV timings for the input or output, applications use the
[VIDIOC\_S\_DV\_TIMINGS](#vidioc-g-dv-timings) ioctl and to get the current timings,
applications use the [VIDIOC\_G\_DV\_TIMINGS](#vidioc-g-dv-timings) ioctl. The detailed timing
information is filled in using the structure struct
[`v4l2_dv_timings`](#c.V4L.v4l2_dv_timings "v4l2_dv_timings"). These ioctls take a
pointer to the struct [`v4l2_dv_timings`](#c.V4L.v4l2_dv_timings "v4l2_dv_timings")
structure as argument. If the ioctl is not supported or the timing
values are not correct, the driver returns `EINVAL` error code.

Calling `VIDIOC_SUBDEV_S_DV_TIMINGS` on a subdev device node that has been
registered in read-only mode is not allowed. An error is returned and the errno
variable is set to `-EPERM`.

The `linux/v4l2-dv-timings.h` header can be used to get the timings of
the formats in the [CEA-861-E](biblio.html#cea861) and [VESA DMT](biblio.html#vesadmt) standards. If
the current input or output does not support DV timings (e.g. if
[ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) does not set the
`V4L2_IN_CAP_DV_TIMINGS` flag), then `ENODATA` error code is returned.

## 7.26.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   This ioctl is not supported, or the [VIDIOC\_S\_DV\_TIMINGS](#vidioc-g-dv-timings)
    parameter was unsuitable.

ENODATA
:   Digital video timings are not supported for this input or output.

EBUSY
:   The device is busy and therefore can not change the timings.

EPERM
:   `VIDIOC_SUBDEV_S_DV_TIMINGS` has been called on a read-only subdevice.

type v4l2\_bt\_timings

struct v4l2\_bt\_timings

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `width` | Width of the active video in pixels. |
| \_\_u32 | `height` | Height of the active video frame in lines. So for interlaced formats the height of the active video in each field is `height`/2. |
| \_\_u32 | `interlaced` | Progressive (`V4L2_DV_PROGRESSIVE`) or interlaced (`V4L2_DV_INTERLACED`). |
| \_\_u32 | `polarities` | This is a bit mask that defines polarities of sync signals. bit 0 (`V4L2_DV_VSYNC_POS_POL`) is for vertical sync polarity and bit 1 (`V4L2_DV_HSYNC_POS_POL`) is for horizontal sync polarity. If the bit is set (1) it is positive polarity and if is cleared (0), it is negative polarity. |
| \_\_u64 | `pixelclock` | Pixel clock in Hz. Ex. 74.25MHz->74250000 |
| \_\_u32 | `hfrontporch` | Horizontal front porch in pixels |
| \_\_u32 | `hsync` | Horizontal sync length in pixels |
| \_\_u32 | `hbackporch` | Horizontal back porch in pixels |
| \_\_u32 | `vfrontporch` | Vertical front porch in lines. For interlaced formats this refers to the odd field (aka field 1). |
| \_\_u32 | `vsync` | Vertical sync length in lines. For interlaced formats this refers to the odd field (aka field 1). |
| \_\_u32 | `vbackporch` | Vertical back porch in lines. For interlaced formats this refers to the odd field (aka field 1). |
| \_\_u32 | `il_vfrontporch` | Vertical front porch in lines for the even field (aka field 2) of interlaced field formats. Must be 0 for progressive formats. |
| \_\_u32 | `il_vsync` | Vertical sync length in lines for the even field (aka field 2) of interlaced field formats. Must be 0 for progressive formats. |
| \_\_u32 | `il_vbackporch` | Vertical back porch in lines for the even field (aka field 2) of interlaced field formats. Must be 0 for progressive formats. |
| \_\_u32 | `standards` | The video standard(s) this format belongs to. This will be filled in by the driver. Applications must set this to 0. See [DV BT Timing standards](#dv-bt-standards) for a list of standards. |
| \_\_u32 | `flags` | Several flags giving more information about the format. See [DV BT Timing flags](#dv-bt-flags) for a description of the flags. |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `picture_aspect` | The picture aspect if the pixels are not square. Only valid if the `V4L2_DV_FL_HAS_PICTURE_ASPECT` flag is set. |
| \_\_u8 | `cea861_vic` | The Video Identification Code according to the CEA-861 standard. Only valid if the `V4L2_DV_FL_HAS_CEA861_VIC` flag is set. |
| \_\_u8 | `hdmi_vic` | The Video Identification Code according to the HDMI standard. Only valid if the `V4L2_DV_FL_HAS_HDMI_VIC` flag is set. |
| \_\_u8 | `reserved[46]` | Reserved for future extensions. Drivers and applications must set the array to zero. |

type v4l2\_dv\_timings

struct v4l2\_dv\_timings

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | Type of DV timings as listed in [DV Timing types](#dv-timing-types). |
| union { | (anonymous) | |
| struct [`v4l2_bt_timings`](#c.V4L.v4l2_bt_timings "v4l2_bt_timings") | `bt` | Timings defined by BT.656/1120 specifications |
| \_\_u32 | `reserved`[32] |  |
| } |  | |

DV Timing types

|  |  |  |
| --- | --- | --- |
| Timing type | value | Description |
|  |  |  |
| `V4L2_DV_BT_656_1120` | 0 | BT.656/1120 timings |

DV BT Timing standards

|  |  |
| --- | --- |
| Timing standard | Description |
| `V4L2_DV_BT_STD_CEA861` | The timings follow the CEA-861 Digital TV Profile standard |
| `V4L2_DV_BT_STD_DMT` | The timings follow the VESA Discrete Monitor Timings standard |
| `V4L2_DV_BT_STD_CVT` | The timings follow the VESA Coordinated Video Timings standard |
| `V4L2_DV_BT_STD_GTF` | The timings follow the VESA Generalized Timings Formula standard |
| `V4L2_DV_BT_STD_SDI` | The timings follow the SDI Timings standard. There are no horizontal syncs/porches at all in this format. Total blanking timings must be set in hsync or vsync fields only. |

DV BT Timing flags

|  |  |
| --- | --- |
| Flag | Description |
| `V4L2_DV_FL_REDUCED_BLANKING` | CVT/GTF specific: the timings use reduced blanking (CVT) or the ‘Secondary GTF’ curve (GTF). In both cases the horizontal and/or vertical blanking intervals are reduced, allowing a higher resolution over the same bandwidth. This is a read-only flag, applications must not set this. |
| `V4L2_DV_FL_CAN_REDUCE_FPS` | CEA-861 specific: set for CEA-861 formats with a framerate that is a multiple of six. These formats can be optionally played at 1 / 1.001 speed to be compatible with 60 Hz based standards such as NTSC and PAL-M that use a framerate of 29.97 frames per second. If the transmitter can’t generate such frequencies, then the flag will also be cleared. This is a read-only flag, applications must not set this. |
| `V4L2_DV_FL_REDUCED_FPS` | CEA-861 specific: only valid for video transmitters or video receivers that have the `V4L2_DV_FL_CAN_DETECT_REDUCED_FPS` set. This flag is cleared otherwise. It is also only valid for formats with the `V4L2_DV_FL_CAN_REDUCE_FPS` flag set, for other formats the flag will be cleared by the driver.  If the application sets this flag for a transmitter, then the pixelclock used to set up the transmitter is divided by 1.001 to make it compatible with NTSC framerates. If the transmitter can’t generate such frequencies, then the flag will be cleared.  If a video receiver detects that the format uses a reduced framerate, then it will set this flag to signal this to the application. |
| `V4L2_DV_FL_HALF_LINE` | Specific to interlaced formats: if set, then the vertical frontporch of field 1 (aka the odd field) is really one half-line longer and the vertical backporch of field 2 (aka the even field) is really one half-line shorter, so each field has exactly the same number of half-lines. Whether half-lines can be detected or used depends on the hardware. |
| `V4L2_DV_FL_IS_CE_VIDEO` | If set, then this is a Consumer Electronics (CE) video format. Such formats differ from other formats (commonly called IT formats) in that if R’G’B’ encoding is used then by default the R’G’B’ values use limited range (i.e. 16-235) as opposed to full range (i.e. 0-255). All formats defined in CEA-861 except for the 640x480p59.94 format are CE formats. |
| `V4L2_DV_FL_FIRST_FIELD_EXTRA_LINE` | Some formats like SMPTE-125M have an interlaced signal with a odd total height. For these formats, if this flag is set, the first field has the extra line. Else, it is the second field. |
| `V4L2_DV_FL_HAS_PICTURE_ASPECT` | If set, then the picture\_aspect field is valid. Otherwise assume that the pixels are square, so the picture aspect ratio is the same as the width to height ratio. |
| `V4L2_DV_FL_HAS_CEA861_VIC` | If set, then the cea861\_vic field is valid and contains the Video Identification Code as per the CEA-861 standard. |
| `V4L2_DV_FL_HAS_HDMI_VIC` | If set, then the hdmi\_vic field is valid and contains the Video Identification Code as per the HDMI standard (HDMI Vendor Specific InfoFrame). |
| `V4L2_DV_FL_CAN_DETECT_REDUCED_FPS` | CEA-861 specific: only valid for video receivers, the flag is cleared by transmitters. If set, then the hardware can detect the difference between regular framerates and framerates reduced by 1000/1001. E.g.: 60 vs 59.94 Hz, 30 vs 29.97 Hz or 24 vs 23.976 Hz. |
