# 1.2.Querying Capabilities

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/querycap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.2. Querying Capabilities

Because V4L2 covers a wide variety of devices not all aspects of the API
are equally applicable to all types of devices. Furthermore devices of
the same type have different capabilities and this specification permits
the omission of a few complicated and less important parts of the API.

The [ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl is available to
check if the kernel device is compatible with this specification, and to
query the [functions](devices.html#devices) and [I/O methods](io.html#io)
supported by the device.

Starting with kernel version 3.1, [ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap)
will return the V4L2 API version used by the driver, with generally
matches the Kernel version. There’s no need of using
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) to check if a specific ioctl
is supported, the V4L2 core now returns `ENOTTY` if a driver doesn’t
provide support for an ioctl.

Other features can be queried by calling the respective ioctl, for
example [ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) to learn about the
number, types and names of video connectors on the device. Although
abstraction is a major objective of this API, the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl also allows driver
specific applications to reliably identify the driver.

All V4L2 drivers must support [ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap).
Applications should always call this ioctl after opening the device.
