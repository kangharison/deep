# 5.13.ioctl MEDIA_REQUEST_IOC_QUEUE

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-request-ioc-queue.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.13. ioctl MEDIA\_REQUEST\_IOC\_QUEUE

## 5.13.1. Name

MEDIA\_REQUEST\_IOC\_QUEUE - Queue a request

## 5.13.2. Synopsis

MEDIA\_REQUEST\_IOC\_QUEUE

`int ioctl(int request_fd, MEDIA_REQUEST_IOC_QUEUE)`

## 5.13.3. Arguments

`request_fd`
:   File descriptor returned by [ioctl MEDIA\_IOC\_REQUEST\_ALLOC](media-ioc-request-alloc.html#media-ioc-request-alloc).

## 5.13.4. Description

If the media device supports [requests](request-api.html#media-request-api), then
this request ioctl can be used to queue a previously allocated request.

If the request was successfully queued, then the file descriptor can be
[polled](request-func-poll.html#request-func-poll) to wait for the request to complete.

If the request was already queued before, then `EBUSY` is returned.
Other errors can be returned if the contents of the request contained
invalid or inconsistent data, see the next section for a list of
common error codes. On error both the request and driver state are unchanged.

Once a request is queued, then the driver is required to gracefully handle
errors that occur when the request is applied to the hardware. The
exception is the `EIO` error which signals a fatal error that requires
the application to stop streaming to reset the hardware state.

It is not allowed to mix queuing requests with queuing buffers directly
(without a request). `EBUSY` will be returned if the first buffer was
queued directly and you next try to queue a request, or vice versa.

A request must contain at least one buffer, otherwise this ioctl will
return an `ENOENT` error.

## 5.13.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EBUSY
:   The request was already queued or the application queued the first
    buffer directly, but later attempted to use a request. It is not permitted
    to mix the two APIs.

ENOENT
:   The request did not contain any buffers. All requests are required
    to have at least one buffer. This can also be returned if some required
    configuration is missing in the request.

ENOMEM
:   Out of memory when allocating internal data structures for this
    request.

EINVAL
:   The request has invalid data.

EIO
:   The hardware is in a bad state. To recover, the application needs to
    stop streaming to reset the hardware state and then try to restart
    streaming.
