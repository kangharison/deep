# 4.1.Video Capture Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-capture.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.1. Video Capture Interface

Video capture devices sample an analog video signal and store the
digitized images in memory. Today nearly all devices can capture at full
25 or 30 frames/second. With this interface applications can control the
capture process and move images from the driver into user space.

Conventionally V4L2 video capture devices are accessed through character
device special files named `/dev/video` and `/dev/video0` to
`/dev/video63` with major number 81 and minor numbers 0 to 63.
`/dev/video` is typically a symbolic link to the preferred video
device.

Note

The same device file names are used for video output devices.

## 4.1.1. Querying Capabilities

Devices supporting the video capture interface set the
`V4L2_CAP_VIDEO_CAPTURE` or `V4L2_CAP_VIDEO_CAPTURE_MPLANE` flag in
the `capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. As secondary device
functions they may also support the [video overlay](dev-overlay.html#overlay)
(`V4L2_CAP_VIDEO_OVERLAY`) and the [raw VBI capture](dev-raw-vbi.html#raw-vbi)
(`V4L2_CAP_VBI_CAPTURE`) interface. At least one of the read/write or
streaming I/O methods must be supported. Tuners and audio inputs are
optional.

## 4.1.2. Supplemental Functions

Video capture devices shall support [audio input](audio.html#audio),
[Tuners and Modulators](tuner.html#tuner), [controls](control.html#control),
[cropping and scaling](crop.html#crop) and
[streaming parameter](streaming-par.html#streaming-par) ioctls as needed. The
[video input](video.html#video) ioctls must be supported by all video
capture devices.

## 4.1.3. Image Format Negotiation

The result of a capture operation is determined by cropping and image
format parameters. The former select an area of the video picture to
capture, the latter how images are stored in memory, i. e. in RGB or YUV
format, the number of bits per pixel or width and height. Together they
also define how images are scaled in the process.

As usual these parameters are *not* reset at [`open()`](func-open.html#c.V4L.open "open")
time to permit Unix tool chains, programming a device and then reading
from it as if it was a plain file. Well written V4L2 applications ensure
they really get what they want, including cropping and scaling.

Cropping initialization at minimum requires to reset the parameters to
defaults. An example is given in [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop).

To query the current image format applications set the `type` field of
a struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") to
`V4L2_BUF_TYPE_VIDEO_CAPTURE` or
`V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE` and call the
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl with a pointer to this
structure. Drivers fill the struct
[`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") `pix` or the struct
[`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") `pix_mp`
member of the `fmt` union.

To request different parameters applications set the `type` field of a
struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") as above and initialize all
fields of the struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format")
`vbi` member of the `fmt` union, or better just modify the results
of [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt), and call the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
ioctl with a pointer to this structure. Drivers may adjust the
parameters and finally return the actual parameters as [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
does.

Like [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) the [VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl
can be used to learn about hardware limitations without disabling I/O or
possibly time consuming hardware preparations.

The contents of struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format") and
struct [`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane") are
discussed in [Image Formats](pixfmt.html#pixfmt). See also the specification of the
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt), [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) and [VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctls for
details. Video capture devices must implement both the [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
and [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, even if [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ignores all
requests and always returns default parameters as [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) does.
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) is optional.

## 4.1.4. Reading Images

A video capture device may support the [read() function](func-read.html#func-read)
and/or streaming ([memory mapping](func-mmap.html#func-mmap) or
[user pointer](userp.html#userp)) I/O. See [Input/Output](io.html#io) for details.
