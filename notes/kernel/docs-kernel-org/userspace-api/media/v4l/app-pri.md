# 1.3.Application Priority

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/app-pri.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.3. Application Priority

When multiple applications share a device it may be desirable to assign
them different priorities. Contrary to the traditional “rm -rf /” school
of thought, a video recording application could for example block other
applications from changing video controls or switching the current TV
channel. Another objective is to permit low priority applications
working in background, which can be preempted by user controlled
applications and automatically regain control of the device at a later
time.

Since these features cannot be implemented entirely in user space V4L2
defines the [VIDIOC\_G\_PRIORITY](vidioc-g-priority.html#vidioc-g-priority) and
[VIDIOC\_S\_PRIORITY](vidioc-g-priority.html#vidioc-g-priority) ioctls to request and
query the access priority associate with a file descriptor. Opening a
device assigns a medium priority, compatible with earlier versions of
V4L2 and drivers not supporting these ioctls. Applications requiring a
different priority will usually call [VIDIOC\_S\_PRIORITY](vidioc-g-priority.html#vidioc-g-priority) after verifying the device with the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl.

Ioctls changing driver properties, such as
[VIDIOC\_S\_INPUT](vidioc-g-input.html#vidioc-g-input), return an `EBUSY` error code
after another application obtained higher priority.
