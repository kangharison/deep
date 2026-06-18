# 7.72.V4L2 select()

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/func-select.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.72. V4L2 select()

## 7.72.1. Name

v4l2-select - Synchronous I/O multiplexing

## 7.72.2. Synopsis

```
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
```

int select(int nfds, fd\_set \*readfds, fd\_set \*writefds, fd\_set \*exceptfds, struct timeval \*timeout)

## 7.72.3. Arguments

`nfds`
:   The highest-numbered file descriptor in any of the three sets, plus 1.

`readfds`
:   File descriptions to be watched if a [`read()`](func-read.html#c.V4L.read "V4L.read") call won’t block.

`writefds`
:   File descriptions to be watched if a [`write()`](func-write.html#c.V4L.write "V4L.write") won’t block.

`exceptfds`
:   File descriptions to be watched for V4L2 events.

`timeout`
:   Maximum time to wait.

## 7.72.4. Description

With the [`select()`](#c.V4L.select "select") function applications can suspend
execution until the driver has captured data or is ready to accept data
for output.

When streaming I/O has been negotiated this function waits until a
buffer has been filled or displayed and can be dequeued with the
[VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) ioctl. When buffers are already in
the outgoing queue of the driver the function returns immediately.

On success [`select()`](#c.V4L.select "select") returns the total number of bits set in
`fd_set`. When the function timed out it returns
a value of zero. On failure it returns -1 and the `errno` variable is
set appropriately. When the application did not call
[ioctl VIDIOC\_QBUF, VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf) or
[ioctl VIDIOC\_STREAMON, VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon) yet the [`select()`](#c.V4L.select "select")
function succeeds, setting the bit of the file descriptor in `readfds`
or `writefds`, but subsequent [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf)
calls will fail. [[1]](#f1)

When use of the [`read()`](func-read.html#c.V4L.read "read") function has been negotiated and the
driver does not capture yet, the [`select()`](#c.V4L.select "select") function starts
capturing. When that fails, [`select()`](#c.V4L.select "select") returns successful and
a subsequent [`read()`](func-read.html#c.V4L.read "read") call, which also attempts to start
capturing, will return an appropriate error code. When the driver
captures continuously (as opposed to, for example, still images) and
data is already available the [`select()`](#c.V4L.select "select") function returns
immediately.

When use of the [`write()`](func-write.html#c.V4L.write "write") function has been negotiated the
[`select()`](#c.V4L.select "select") function just waits until the driver is ready for a
non-blocking [`write()`](func-write.html#c.V4L.write "write") call.

All drivers implementing the [`read()`](func-read.html#c.V4L.read "read") or [`write()`](func-write.html#c.V4L.write "write")
function or streaming I/O must also support the [`select()`](#c.V4L.select "select")
function.

For more details see the [`select()`](#c.V4L.select "select") manual page.

## 7.72.5. Return Value

On success, [`select()`](#c.V4L.select "select") returns the number of descriptors
contained in the three returned descriptor sets, which will be zero if
the timeout expired. On error -1 is returned, and the `errno` variable
is set appropriately; the sets and `timeout` are undefined. Possible
error codes are:

EBADF
:   One or more of the file descriptor sets specified a file descriptor
    that is not open.

EBUSY
:   The driver does not support multiple read or write streams and the
    device is already in use.

EFAULT
:   The `readfds`, `writefds`, `exceptfds` or `timeout` pointer
    references an inaccessible memory area.

EINTR
:   The call was interrupted by a signal.

EINVAL
:   The `nfds` argument is less than zero or greater than
    `FD_SETSIZE`.

[[1](#id1)]

The Linux kernel implements [`select()`](#c.V4L.select "select") like the
[`poll()`](func-poll.html#c.V4L.poll "poll") function, but [`select()`](#c.V4L.select "select") cannot
return a `POLLERR`.
