# 3.Input/Output

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/io.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. Input/Output

The V4L2 API defines several different methods to read from or write to
a device. All drivers exchanging data with applications must support at
least one of them.

The classic I/O method using the [`read()`](func-read.html#c.V4L.read "read") and
[`write()`](func-write.html#c.V4L.write "write") function is automatically selected after opening a
V4L2 device. When the driver does not support this method attempts to
read or write will fail at any time.

Other methods must be negotiated. To select the streaming I/O method
with memory mapped or user buffers applications call the
[ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) ioctl.

Video overlay can be considered another I/O method, although the
application does not directly receive the image data. It is selected by
initiating video overlay with the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt)
ioctl. For more information see [Video Overlay Interface](dev-overlay.html#overlay).

Generally exactly one I/O method, including overlay, is associated with
each file descriptor. The only exceptions are applications not
exchanging data with a driver (“panel applications”, see [Opening and Closing Devices](open.html#open))
and drivers permitting simultaneous video capturing and overlay using
the same file descriptor, for compatibility with V4L and earlier
versions of V4L2.

[VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) and [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) would permit this to some
degree, but for simplicity drivers need not support switching the I/O
method (after first switching away from read/write) other than by
closing and reopening the device.

The following sections describe the various I/O methods in more detail.

* [3.1. Read/Write](rw.html)
* [3.2. Streaming I/O (Memory Mapping)](mmap.html)
* [3.3. Streaming I/O (User Pointers)](userp.html)
* [3.4. Streaming I/O (DMA buffer importing)](dmabuf.html)
* [3.5. Buffers](buffer.html)
* [3.6. Field Order](field-order.html)
