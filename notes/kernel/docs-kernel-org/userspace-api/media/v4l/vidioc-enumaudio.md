# 7.11.ioctl VIDIOC_ENUMAUDIO

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-enumaudio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.11. ioctl VIDIOC\_ENUMAUDIO

## 7.11.1. Name

VIDIOC\_ENUMAUDIO - Enumerate audio inputs

## 7.11.2. Synopsis

VIDIOC\_ENUMAUDIO

`int ioctl(int fd, VIDIOC_ENUMAUDIO, struct v4l2_audio *argp)`

## 7.11.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio").

## 7.11.4. Description

To query the attributes of an audio input applications initialize the
`index` field and zero out the `reserved` array of a struct
[`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio") and call the [ioctl VIDIOC\_ENUMAUDIO](#vidioc-enumaudio)
ioctl with a pointer to this structure. Drivers fill the rest of the
structure or return an `EINVAL` error code when the index is out of
bounds. To enumerate all audio inputs applications shall begin at index
zero, incrementing by one until the driver returns `EINVAL`.

See [VIDIOC\_G\_AUDIO](vidioc-g-audio.html#vidioc-g-audio) for a description of struct
[`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "v4l2_audio").

## 7.11.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The number of the audio input is out of bounds.
