# 4.10.Software Defined Radio Interface (SDR)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-sdr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.10. Software Defined Radio Interface (SDR)

SDR is an abbreviation of Software Defined Radio, the radio device which
uses application software for modulation or demodulation. This interface
is intended for controlling and data streaming of such devices.

SDR devices are accessed through character device special files named
`/dev/swradio0` to `/dev/swradio255` with major number 81 and
dynamically allocated minor numbers 0 to 255.

## 4.10.1. Querying Capabilities

Devices supporting the SDR receiver interface set the
`V4L2_CAP_SDR_CAPTURE` and `V4L2_CAP_TUNER` flag in the
`capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. That flag means the
device has an Analog to Digital Converter (ADC), which is a mandatory
element for the SDR receiver.

Devices supporting the SDR transmitter interface set the
`V4L2_CAP_SDR_OUTPUT` and `V4L2_CAP_MODULATOR` flag in the
`capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. That flag means the
device has an Digital to Analog Converter (DAC), which is a mandatory
element for the SDR transmitter.

At least one of the read/write or streaming I/O methods
must be supported.

## 4.10.2. Supplemental Functions

SDR devices can support [controls](control.html#control), and must support
the [Tuners and Modulators](tuner.html#tuner) ioctls. Tuner ioctls are used for setting the
ADC/DAC sampling rate (sampling frequency) and the possible radio
frequency (RF).

The `V4L2_TUNER_SDR` tuner type is used for setting SDR device ADC/DAC
frequency, and the `V4L2_TUNER_RF` tuner type is used for setting
radio frequency. The tuner index of the RF tuner (if any) must always
follow the SDR tuner index. Normally the SDR tuner is #0 and the RF
tuner is #1.

The [ioctl VIDIOC\_S\_HW\_FREQ\_SEEK](vidioc-s-hw-freq-seek.html#vidioc-s-hw-freq-seek) ioctl is
not supported.

## 4.10.3. Data Format Negotiation

The SDR device uses the [Data Formats](format.html#format) ioctls to select the
capture and output format. Both the sampling resolution and the data
streaming format are bound to that selectable format. In addition to the
basic [Data Formats](format.html#format) ioctls, the
[ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl must be supported as
well.

To use the [Data Formats](format.html#format) ioctls applications set the `type`
field of a struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") to
`V4L2_BUF_TYPE_SDR_CAPTURE` or `V4L2_BUF_TYPE_SDR_OUTPUT` and use
the struct [`v4l2_sdr_format`](#c.V4L.v4l2_sdr_format "v4l2_sdr_format") `sdr` member
of the `fmt` `union as` needed per the desired operation. Currently
there are two fields, `pixelformat` and `buffersize`, of
struct [`v4l2_sdr_format`](#c.V4L.v4l2_sdr_format "v4l2_sdr_format") which are used.
Content of the `pixelformat` is V4L2 FourCC code of the data format.
The `buffersize` field is maximum buffer size in bytes required for
data transfer, set by the driver in order to inform application.

type v4l2\_sdr\_format

struct v4l2\_sdr\_format

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `pixelformat` | The data format or type of compression, set by the application. This is a little endian [four character code](vidioc-enum-fmt.html#v4l2-fourcc). V4L2 defines SDR formats in [SDR Formats](sdr-formats.html#sdr-formats). |
| \_\_u32 | `buffersize` | Maximum size in bytes required for data. Value is set by the driver. |
| \_\_u8 | `reserved[24]` | This array is reserved for future extensions. Drivers and applications must set it to zero. |

An SDR device may support [read/write](rw.html#rw) and/or streaming
([memory mapping](mmap.html#mmap) or [user pointer](userp.html#userp)) I/O.
