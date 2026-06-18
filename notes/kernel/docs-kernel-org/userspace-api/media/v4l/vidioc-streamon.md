# 7.55.ioctl VIDIOC_STREAMON, VIDIOC_STREAMOFF

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-streamon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.55. ioctl VIDIOC\_STREAMON, VIDIOC\_STREAMOFF

## 7.55.1. Name

VIDIOC\_STREAMON - VIDIOC\_STREAMOFF - Start or stop streaming I/O

## 7.55.2. Synopsis

VIDIOC\_STREAMON

`int ioctl(int fd, VIDIOC_STREAMON, const int *argp)`

VIDIOC\_STREAMOFF

`int ioctl(int fd, VIDIOC_STREAMOFF, const int *argp)`

## 7.55.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to an integer.

## 7.55.4. Description

The `VIDIOC_STREAMON` and `VIDIOC_STREAMOFF` ioctl start and stop
the capture or output process during streaming
([memory mapping](mmap.html#mmap), [user pointer](userp.html#userp) or
[DMABUF](dmabuf.html#dmabuf)) I/O.

Capture hardware is disabled and no input buffers are filled (if there
are any empty buffers in the incoming queue) until `VIDIOC_STREAMON`
has been called. Output hardware is disabled and no video signal is
produced until `VIDIOC_STREAMON` has been called.

Memory-to-memory devices will not start until `VIDIOC_STREAMON` has
been called for both the capture and output stream types.

If `VIDIOC_STREAMON` fails then any already queued buffers will remain
queued.

The `VIDIOC_STREAMOFF` ioctl, apart of aborting or finishing any DMA
in progress, unlocks any user pointer buffers locked in physical memory,
and it removes all buffers from the incoming and outgoing queues. That
means all images captured but not dequeued yet will be lost, likewise
all images enqueued for output but not transmitted yet. I/O returns to
the same state as after calling
[ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) and can be restarted
accordingly.

If buffers have been queued with [ioctl VIDIOC\_QBUF, VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) and
`VIDIOC_STREAMOFF` is called without ever having called
`VIDIOC_STREAMON`, then those queued buffers will also be removed from
the incoming queue and all are returned to the same state as after
calling [ioctl VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs) and can be restarted
accordingly.

Both ioctls take a pointer to an integer, the desired buffer or stream
type. This is the same as struct
[`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "v4l2_requestbuffers") `type`.

If `VIDIOC_STREAMON` is called when streaming is already in progress,
or if `VIDIOC_STREAMOFF` is called when streaming is already stopped,
then 0 is returned. Nothing happens in the case of `VIDIOC_STREAMON`,
but `VIDIOC_STREAMOFF` will return queued buffers to their starting
state as mentioned above.

Note

Applications can be preempted for unknown periods right before
or after the `VIDIOC_STREAMON` or `VIDIOC_STREAMOFF` calls, there is
no notion of starting or stopping “now”. Buffer timestamps can be used
to synchronize with other events.

## 7.55.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The buffer `type` is not supported, or no buffers have been
    allocated (memory mapping) or enqueued (output) yet.

EPIPE
:   The driver implements
    [pad-level format configuration](dev-subdev.html#pad-level-formats) and the
    pipeline configuration is invalid.

ENOLINK
:   The driver implements Media Controller interface and the pipeline
    link configuration is invalid.
