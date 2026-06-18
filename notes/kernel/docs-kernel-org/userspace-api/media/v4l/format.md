# 1.25.Data Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/format.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.25. Data Formats

## 1.25.1. Data Format Negotiation

Different devices exchange different kinds of data with applications,
for example video images, raw or sliced VBI data, RDS datagrams. Even
within one kind many different formats are possible, in particular there is an
abundance of image formats. Although drivers must provide a default and
the selection persists across closing and reopening a device,
applications should always negotiate a data format before engaging in
data exchange. Negotiation means the application asks for a particular
format and the driver selects and reports the best the hardware can do
to satisfy the request. Of course applications can also just query the
current selection.

A single mechanism exists to negotiate all data formats using the
aggregate struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") and the
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) and
[VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctls. Additionally the
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl can be used to examine
what the hardware *could* do, without actually selecting a new data
format. The data formats supported by the V4L2 API are covered in the
respective device section in [Interfaces](devices.html#devices). For a closer look at
image formats see [Image Formats](pixfmt.html#pixfmt).

The [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl is a major turning-point in the
initialization sequence. Prior to this point multiple panel applications
can access the same device concurrently to select the current input,
change controls or modify other properties. The first [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
assigns a logical stream (video data, VBI data etc.) exclusively to one
file descriptor.

Exclusive means no other application, more precisely no other file
descriptor, can grab this stream or change device properties
inconsistent with the negotiated parameters. A video standard change for
example, when the new standard uses a different number of scan lines,
can invalidate the selected image format. Therefore only the file
descriptor owning the stream can make invalidating changes. Accordingly
multiple file descriptors which grabbed different logical streams
prevent each other from interfering with their settings. When for
example video overlay is about to start or already in progress,
simultaneous video capturing may be restricted to the same cropping and
image size.

When applications omit the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl its locking side
effects are implied by the next step, the selection of an I/O method
with the [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) ioctl or implicit
with the first [`read()`](func-read.html#c.V4L.read "read") or
[`write()`](func-write.html#c.V4L.write "write") call.

Generally only one logical stream can be assigned to a file descriptor,
the exception being drivers permitting simultaneous video capturing and
overlay using the same file descriptor for compatibility with V4L and
earlier versions of V4L2. Switching the logical stream or returning into
“panel mode” is possible by closing and reopening the device. Drivers
*may* support a switch using [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt).

All drivers exchanging data with applications must support the
[VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) and [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl. Implementation of the
[VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) is highly recommended but optional.

## 1.25.2. Image Format Enumeration

Apart of the generic format negotiation functions a special ioctl to
enumerate all image formats supported by video capture, overlay or
output devices is available. [[1]](#f1)

The [ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt) ioctl must be supported
by all drivers exchanging image data with applications.

Important

Drivers are not supposed to convert image formats in kernel space.
They must enumerate only formats directly supported by the hardware.
If necessary driver writers should publish an example conversion
routine or library for integration into applications.

[[1](#id1)]

Enumerating formats an application has no a-priori knowledge of
(otherwise it could explicitly ask for them and need not enumerate)
seems useless, but there are applications serving as proxy between
drivers and the actual video applications for which this is useful.
