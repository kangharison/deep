# 1.29.Streaming Parameters

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/streaming-par.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.29. Streaming Parameters

Streaming parameters are intended to optimize the video capture process
as well as I/O. Presently applications can request a high quality
capture mode with the [VIDIOC\_S\_PARM](vidioc-g-parm.html#vidioc-g-parm) ioctl.

The current video standard determines a nominal number of frames per
second. If less than this number of frames is to be captured or output,
applications can request frame skipping or duplicating on the driver
side. This is especially useful when using the
[`read()`](func-read.html#c.V4L.read "read") or [`write()`](func-write.html#c.V4L.write "write"), which are
not augmented by timestamps or sequence counters, and to avoid
unnecessary data copying.

Finally these ioctls can be used to determine the number of buffers used
internally by a driver in read/write mode. For implications see the
section discussing the [`read()`](func-read.html#c.V4L.read "read") function.

To get and set the streaming parameters applications call the
[VIDIOC\_G\_PARM](vidioc-g-parm.html#vidioc-g-parm) and
[VIDIOC\_S\_PARM](vidioc-g-parm.html#vidioc-g-parm) ioctl, respectively. They take
a pointer to a struct [`v4l2_streamparm`](vidioc-g-parm.html#c.V4L.v4l2_streamparm "v4l2_streamparm"), which
contains a `union holding` separate parameters for input and output
devices.

These ioctls are optional, drivers need not implement them. If so, they
return the `EINVAL` error code.
