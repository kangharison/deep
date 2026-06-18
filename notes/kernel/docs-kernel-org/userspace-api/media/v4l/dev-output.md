# 4.3.Video Output Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-output.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.3. Video Output Interface

Video output devices encode stills or image sequences as analog video
signal. With this interface applications can control the encoding
process and move images from user space to the driver.

Conventionally V4L2 video output devices are accessed through character
device special files named `/dev/video` and `/dev/video0` to
`/dev/video63` with major number 81 and minor numbers 0 to 63.
`/dev/video` is typically a symbolic link to the preferred video
device.

Note

The same device file names are used also for video capture devices.

## 4.3.1. Querying Capabilities

Devices supporting the video output interface set the
`V4L2_CAP_VIDEO_OUTPUT` or `V4L2_CAP_VIDEO_OUTPUT_MPLANE` flag in
the `capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. As secondary device
functions they may also support the [raw VBI output](dev-raw-vbi.html#raw-vbi)
(`V4L2_CAP_VBI_OUTPUT`) interface. At least one of the read/write or
streaming I/O methods must be supported. Modulators and audio outputs
are optional.

## 4.3.2. Supplemental Functions

Video output devices shall support [audio output](audio.html#audio),
[modulator](tuner.html#tuner), [controls](control.html#control),
[cropping and scaling](crop.html#crop) and
[streaming parameter](streaming-par.html#streaming-par) ioctls as needed. The
[video output](video.html#video) ioctls must be supported by all video
output devices.

## 4.3.3. Image Format Negotiation

The output is determined by cropping and image format parameters. The
former select an area of the video picture where the image will appear,
the latter how images are stored in memory, i. e. in RGB or YUV format,
the number of bits per pixel or width and height. Together they also
define how images are scaled in the process.

As usual these parameters are *not* reset at [`open()`](func-open.html#c.V4L.open "open")
time to permit Unix tool chains, programming a device and then writing
to it as if it was a plain file. Well written V4L2 applications ensure
they really get what they want, including cropping and scaling.

Cropping initialization at minimum requires to reset the parameters to
defaults. An example is given in [Image Cropping, Insertion and Scaling -- the CROP API](crop.html#crop).

To query the current image format applications set the `type` field of
a struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") to
`V4L2_BUF_TYPE_VIDEO_OUTPUT` or `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE`
and call the [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl with a pointer
to this structure. Drivers fill the struct
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
details. Video output devices must implement both the [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
and [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl, even if [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ignores all
requests and always returns default parameters as [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) does.
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) is optional.

## 4.3.4. Writing Images

A video output device may support the [write() function](rw.html#rw)
and/or streaming ([memory mapping](mmap.html#mmap) or
[user pointer](userp.html#userp)) I/O. See [Input/Output](io.html#io) for details.
