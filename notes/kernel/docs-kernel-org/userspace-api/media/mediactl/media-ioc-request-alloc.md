# 5.9.ioctl MEDIA_IOC_REQUEST_ALLOC

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-ioc-request-alloc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.9. ioctl MEDIA\_IOC\_REQUEST\_ALLOC

## 5.9.1. Name

MEDIA\_IOC\_REQUEST\_ALLOC - Allocate a request

## 5.9.2. Synopsis

MEDIA\_IOC\_REQUEST\_ALLOC

`int ioctl(int fd, MEDIA_IOC_REQUEST_ALLOC, int *argp)`

## 5.9.3. Arguments

`fd`
:   File descriptor returned by [`open()`](media-func-open.html#c.MC.open "open").

`argp`
:   Pointer to an integer.

## 5.9.4. Description

If the media device supports [requests](request-api.html#media-request-api), then
this ioctl can be used to allocate a request. If it is not supported, then
`errno` is set to `ENOTTY`. A request is accessed through a file descriptor
that is returned in `*argp`.

If the request was successfully allocated, then the request file descriptor
can be passed to the [VIDIOC\_QBUF](../v4l/vidioc-qbuf.html#vidioc-qbuf),
[VIDIOC\_G\_EXT\_CTRLS](../v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls),
[VIDIOC\_S\_EXT\_CTRLS](../v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) and
[VIDIOC\_TRY\_EXT\_CTRLS](../v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctls.

In addition, the request can be queued by calling
[ioctl MEDIA\_REQUEST\_IOC\_QUEUE](media-request-ioc-queue.html#media-request-ioc-queue) and re-initialized by calling
[ioctl MEDIA\_REQUEST\_IOC\_REINIT](media-request-ioc-reinit.html#media-request-ioc-reinit).

Finally, the file descriptor can be [polled](request-func-poll.html#request-func-poll) to wait
for the request to complete.

The request will remain allocated until all the file descriptors associated
with it are closed by [`close()`](media-func-close.html#c.MC.close "close") and the driver no
longer uses the request internally. See also
[here](request-api.html#media-request-life-time) for more information.

## 5.9.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

ENOTTY
:   The driver has no support for requests.
