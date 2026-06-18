# 7.12.ioctl VIDIOC_ENUMAUDOUT

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-enumaudioout.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.12. ioctl VIDIOC\_ENUMAUDOUT

## 7.12.1. Name

VIDIOC\_ENUMAUDOUT - Enumerate audio outputs

## 7.12.2. Synopsis

VIDIOC\_ENUMAUDOUT

`int ioctl(int fd, VIDIOC_ENUMAUDOUT, struct v4l2_audioout *argp)`

## 7.12.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_audioout`](vidioc-g-audioout.html#c.V4L.v4l2_audioout "v4l2_audioout").

## 7.12.4. Description

To query the attributes of an audio output applications initialize the
`index` field and zero out the `reserved` array of a struct
[`v4l2_audioout`](vidioc-g-audioout.html#c.V4L.v4l2_audioout "v4l2_audioout") and call the `VIDIOC_G_AUDOUT`
ioctl with a pointer to this structure. Drivers fill the rest of the
structure or return an `EINVAL` error code when the index is out of
bounds. To enumerate all audio outputs applications shall begin at index
zero, incrementing by one until the driver returns `EINVAL`.

Note

Connectors on a TV card to loop back the received audio signal
to a sound card are not audio outputs in this sense.

See [VIDIOC\_G\_AUDIOout](vidioc-g-audioout.html#vidioc-g-audout) for a description of struct
[`v4l2_audioout`](vidioc-g-audioout.html#c.V4L.v4l2_audioout "v4l2_audioout").

## 7.12.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The number of the audio output is out of bounds.
