# 7.43.ioctl VIDIOC_LOG_STATUS

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-log-status.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.43. ioctl VIDIOC\_LOG\_STATUS

## 7.43.1. Name

VIDIOC\_LOG\_STATUS - Log driver status information

## 7.43.2. Synopsis

VIDIOC\_LOG\_STATUS

`int ioctl(int fd, VIDIOC_LOG_STATUS)`

## 7.43.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

## 7.43.4. Description

As the video/audio devices become more complicated it becomes harder to
debug problems. When this ioctl is called the driver will output the
current device status to the kernel log. This is particular useful when
dealing with problems like no sound, no video and incorrectly tuned
channels. Also many modern devices autodetect video and audio standards
and this ioctl will report what the device thinks what the standard is.
Mismatches may give an indication where the problem is.

This ioctl is optional and not all drivers support it. It was introduced
in Linux 2.6.15.

## 7.43.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
