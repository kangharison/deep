# 7.47.ioctl VIDIOC_QUERYBUF

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-querybuf.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.47. ioctl VIDIOC\_QUERYBUF

## 7.47.1. Name

VIDIOC\_QUERYBUF - Query the status of a buffer

## 7.47.2. Synopsis

VIDIOC\_QUERYBUF

`int ioctl(int fd, VIDIOC_QUERYBUF, struct v4l2_buffer *argp)`

## 7.47.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer").

## 7.47.4. Description

This ioctl is part of the [streaming](mmap.html#mmap) I/O method. It can
be used to query the status of a buffer at any time after buffers have
been allocated with the [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) ioctl.

Applications set the `type` field of a struct
[`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to the same buffer type as was
previously used with struct [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "v4l2_format") `type`
and struct [`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") `type`,
and the `index` field. Valid index numbers range from zero to the
number of buffers allocated with
[ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) (struct
[`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") `count`) minus
one. The `reserved` and `reserved2` fields must be set to 0. When
using the [multi-planar API](planar-apis.html#planar-apis), the `m.planes`
field must contain a userspace pointer to an array of struct
[`v4l2_plane`](buffer.html#c.V4L.v4l2_plane "v4l2_plane") and the `length` field has to be set
to the number of elements in that array. After calling
[ioctl VIDIOC\_QUERYBUF](#vidioc-querybuf) with a pointer to this structure drivers return an
error code or fill the rest of the structure.

In the `flags` field the `V4L2_BUF_FLAG_MAPPED`,
`V4L2_BUF_FLAG_PREPARED`, `V4L2_BUF_FLAG_QUEUED` and
`V4L2_BUF_FLAG_DONE` flags will be valid. The `memory` field will be
set to the current I/O method. For the single-planar API, the
`m.offset` contains the offset of the buffer from the start of the
device memory, the `length` field its size. For the multi-planar API,
fields `m.mem_offset` and `length` in the `m.planes` array
elements will be used instead and the `length` field of struct
[`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") is set to the number of filled-in
array elements. The driver may or may not set the remaining fields and
flags, they are meaningless in this context.

The struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") structure is specified in
[Buffers](buffer.html#buffer).

## 7.47.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The buffer `type` is not supported, or the `index` is out of
    bounds.
