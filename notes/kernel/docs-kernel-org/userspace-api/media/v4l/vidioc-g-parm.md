# 7.37.ioctl VIDIOC_G_PARM, VIDIOC_S_PARM

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-parm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.37. ioctl VIDIOC\_G\_PARM, VIDIOC\_S\_PARM

## 7.37.1. Name

VIDIOC\_G\_PARM - VIDIOC\_S\_PARM - Get or set streaming parameters

## 7.37.2. Synopsis

VIDIOC\_G\_PARM

`int ioctl(int fd, VIDIOC_G_PARM, v4l2_streamparm *argp)`

VIDIOC\_S\_PARM

`int ioctl(int fd, VIDIOC_S_PARM, v4l2_streamparm *argp)`

## 7.37.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_streamparm`](#c.V4L.v4l2_streamparm "v4l2_streamparm").

## 7.37.4. Description

Applications can request a different frame interval. The capture or
output device will be reconfigured to support the requested frame
interval if possible. Optionally drivers may choose to skip or
repeat frames to achieve the requested frame interval.

For stateful encoders (see [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder)) this represents the
frame interval that is typically embedded in the encoded video stream.

Changing the frame interval shall never change the format. Changing the
format, on the other hand, may change the frame interval.

Further these ioctls can be used to determine the number of buffers used
internally by a driver in read/write mode. For implications see the
section discussing the [`read()`](func-read.html#c.V4L.read "read") function.

To get and set the streaming parameters applications call the
[VIDIOC\_G\_PARM](#vidioc-g-parm) and
[VIDIOC\_S\_PARM](#vidioc-g-parm) ioctl, respectively. They take a
pointer to a struct [`v4l2_streamparm`](#c.V4L.v4l2_streamparm "v4l2_streamparm") which contains a
`union holding` separate parameters for input and output devices.

type v4l2\_streamparm

struct v4l2\_streamparm

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | The buffer (stream) type, same as struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") `type`, set by the application. See [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type"). |
| union { | `parm` | |
| struct [`v4l2_captureparm`](#c.V4L.v4l2_captureparm "v4l2_captureparm") | `capture` | Parameters for capture devices, used when `type` is `V4L2_BUF_TYPE_VIDEO_CAPTURE` or `V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`. |
| struct [`v4l2_outputparm`](#c.V4L.v4l2_outputparm "v4l2_outputparm") | `output` | Parameters for output devices, used when `type` is `V4L2_BUF_TYPE_VIDEO_OUTPUT` or `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE`. |
| \_\_u8 | `raw_data`[200] | A place holder for future extensions. |
| } | | |

type v4l2\_captureparm

struct v4l2\_captureparm

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `capability` | See [Streaming Parameters Capabilities](#parm-caps). |
| \_\_u32 | `capturemode` | Set by drivers and applications, see [Capture Parameters Flags](#parm-flags). |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `timeperframe` | This is the desired period between successive frames captured by the driver, in seconds. |
| This will configure the speed at which the video source (e.g. a sensor) generates video frames. If the speed is fixed, then the driver may choose to skip or repeat frames in order to achieve the requested frame rate.  For stateful encoders (see [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder)) this represents the frame interval that is typically embedded in the encoded video stream.  Applications store here the desired frame period, drivers return the actual frame period.  Changing the video standard (also implicitly by switching the video input) may reset this parameter to the nominal frame period. To reset manually applications can just set this field to zero.  Drivers support this function only when they set the `V4L2_CAP_TIMEPERFRAME` flag in the `capability` field. | | |
| \_\_u32 | `extendedmode` | Custom (driver specific) streaming parameters. When unused, applications and drivers must set this field to zero. Applications using this field should check the driver name and version, see [Querying Capabilities](querycap.html#querycap). |
| \_\_u32 | `readbuffers` | Applications set this field to the desired number of buffers used internally by the driver in [`read()`](func-read.html#c.V4L.read "read") mode. Drivers return the actual number of buffers. When an application requests zero buffers, drivers should just return the current setting rather than the minimum or an error code. For details see [Read/Write](rw.html#rw). |
| \_\_u32 | `reserved`[4] | Reserved for future extensions. Drivers and applications must set the array to zero. |

type v4l2\_outputparm

struct v4l2\_outputparm

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `capability` | See [Streaming Parameters Capabilities](#parm-caps). |
| \_\_u32 | `outputmode` | Set by drivers and applications, see [Capture Parameters Flags](#parm-flags). |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `timeperframe` | This is the desired period between successive frames output by the driver, in seconds. |
| The field is intended to repeat frames on the driver side in [`write()`](func-write.html#c.V4L.write "write") mode (in streaming mode timestamps can be used to throttle the output), saving I/O bandwidth.  For stateful encoders (see [Memory-to-Memory Stateful Video Encoder Interface](dev-encoder.html#encoder)) this represents the frame interval that is typically embedded in the encoded video stream and it provides a hint to the encoder of the speed at which raw frames are queued up to the encoder.  Applications store here the desired frame period, drivers return the actual frame period.  Changing the video standard (also implicitly by switching the video output) may reset this parameter to the nominal frame period. To reset manually applications can just set this field to zero.  Drivers support this function only when they set the `V4L2_CAP_TIMEPERFRAME` flag in the `capability` field. | | |
| \_\_u32 | `extendedmode` | Custom (driver specific) streaming parameters. When unused, applications and drivers must set this field to zero. Applications using this field should check the driver name and version, see [Querying Capabilities](querycap.html#querycap). |
| \_\_u32 | `writebuffers` | Applications set this field to the desired number of buffers used internally by the driver in [`write()`](func-write.html#c.V4L.write "write") mode. Drivers return the actual number of buffers. When an application requests zero buffers, drivers should just return the current setting rather than the minimum or an error code. For details see [Read/Write](rw.html#rw). |
| \_\_u32 | `reserved`[4] | Reserved for future extensions. Drivers and applications must set the array to zero. |

Streaming Parameters Capabilities

|  |  |  |
| --- | --- | --- |
| `V4L2_CAP_TIMEPERFRAME` | 0x1000 | The frame period can be modified by setting the `timeperframe` field. |

Capture Parameters Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_MODE_HIGHQUALITY` | 0x0001 | High quality imaging mode. High quality mode is intended for still imaging applications. The idea is to get the best possible image quality that the hardware can deliver. It is not defined how the driver writer may achieve that; it will depend on the hardware and the ingenuity of the driver writer. High quality mode is a different mode from the regular motion video capture modes. In high quality mode:   * The driver may be able to capture higher resolutions than for   motion capture. * The driver may support fewer pixel formats than motion capture   (eg; true color). * The driver may capture and arithmetically combine multiple   successive fields or frames to remove color edge artifacts and   reduce the noise in the video data. * The driver may capture images in slices like a scanner in order   to handle larger format images than would otherwise be   possible. * An image capture operation may be significantly slower than   motion capture. * Moving objects in the image might have excessive motion blur. * Capture might only work through the [`read()`](func-read.html#c.V4L.read "read") call. |

## 7.37.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
