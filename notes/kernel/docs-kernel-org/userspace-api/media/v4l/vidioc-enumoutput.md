# 7.19.ioctl VIDIOC_ENUMOUTPUT

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-enumoutput.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.19. ioctl VIDIOC\_ENUMOUTPUT

## 7.19.1. Name

VIDIOC\_ENUMOUTPUT - Enumerate video outputs

## 7.19.2. Synopsis

VIDIOC\_ENUMOUTPUT

`int ioctl(int fd, VIDIOC_ENUMOUTPUT, struct v4l2_output *argp)`

## 7.19.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_output`](#c.V4L.v4l2_output "v4l2_output").

## 7.19.4. Description

To query the attributes of a video outputs applications initialize the
`index` field of struct [`v4l2_output`](#c.V4L.v4l2_output "v4l2_output") and call
the [ioctl VIDIOC\_ENUMOUTPUT](#vidioc-enumoutput) with a pointer to this structure.
Drivers fill the rest of the structure or return an `EINVAL` error code
when the index is out of bounds. To enumerate all outputs applications
shall begin at index zero, incrementing by one until the driver returns
`EINVAL`.

type v4l2\_output

struct v4l2\_output

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | Identifies the output, set by the application. |
| \_\_u8 | `name`[32] | Name of the video output, a NUL-terminated ASCII string, for example: “Vout”. This information is intended for the user, preferably the connector label on the device itself. |
| \_\_u32 | `type` | Type of the output, see [Output Type](#output-type). |
| \_\_u32 | `audioset` | Drivers can enumerate up to 32 video and audio outputs. This field shows which audio outputs were selectable as the current output if this was the currently selected video output. It is a bit mask. The LSB corresponds to audio output 0, the MSB to output 31. Any number of bits can be set, or none.  When the driver does not enumerate audio outputs no bits must be set. Applications shall not interpret this as lack of audio support. Drivers may automatically select audio outputs without enumerating them.  For details on audio outputs and how to select the current output see [Audio Inputs and Outputs](audio.html#audio). |
| \_\_u32 | `modulator` | Output devices can have zero or more RF modulators. When the `type` is `V4L2_OUTPUT_TYPE_MODULATOR` this is an RF connector and this field identifies the modulator. It corresponds to struct [`v4l2_modulator`](vidioc-g-modulator.html#c.V4L.v4l2_modulator "v4l2_modulator") field `index`. For details on modulators see [Tuners and Modulators](tuner.html#tuner). |
| [v4l2\_std\_id](vidioc-enumstd.html#v4l2-std-id) | `std` | Every video output supports one or more different video standards. This field is a set of all supported standards. For details on video standards and how to switch see [Video Standards](standard.html#standard). |
| \_\_u32 | `capabilities` | This field provides capabilities for the output. See [Output capabilities](#output-capabilities) for flags. |
| \_\_u32 | `reserved`[3] | Reserved for future extensions. Drivers must set the array to zero. |

Output Type

|  |  |  |
| --- | --- | --- |
| `V4L2_OUTPUT_TYPE_MODULATOR` | 1 | This output is an analog TV modulator. |
| `V4L2_OUTPUT_TYPE_ANALOG` | 2 | Any non-modulator video output, for example Composite Video, S-Video, HDMI. The naming as `_TYPE_ANALOG` is historical, today we would have called it `_TYPE_VIDEO`. |
| `V4L2_OUTPUT_TYPE_ANALOGVGAOVERLAY` | 3 | The video output will be copied to a [video overlay](dev-overlay.html#overlay). |

Output capabilities

|  |  |  |
| --- | --- | --- |
| `V4L2_OUT_CAP_DV_TIMINGS` | 0x00000002 | This output supports setting video timings by using `VIDIOC_S_DV_TIMINGS`. |
| `V4L2_OUT_CAP_STD` | 0x00000004 | This output supports setting the TV standard by using `VIDIOC_S_STD`. |
| `V4L2_OUT_CAP_NATIVE_SIZE` | 0x00000008 | This output supports setting the native size using the `V4L2_SEL_TGT_NATIVE_SIZE` selection target, see [Common selection definitions](selections-common.html#v4l2-selections-common). |

## 7.19.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`v4l2_output`](#c.V4L.v4l2_output "v4l2_output") `index` is out of
    bounds.
