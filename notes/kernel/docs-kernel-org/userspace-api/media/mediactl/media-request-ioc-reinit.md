# 5.14.ioctl MEDIA_REQUEST_IOC_REINIT

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-request-ioc-reinit.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.14. ioctl MEDIA\_REQUEST\_IOC\_REINIT

## 5.14.1. Name

MEDIA\_REQUEST\_IOC\_REINIT - Re-initialize a request

## 5.14.2. Synopsis

MEDIA\_REQUEST\_IOC\_REINIT

`int ioctl(int request_fd, MEDIA_REQUEST_IOC_REINIT)`

## 5.14.3. Arguments

`request_fd`
:   File descriptor returned by [ioctl MEDIA\_IOC\_REQUEST\_ALLOC](media-ioc-request-alloc.html#media-ioc-request-alloc).

## 5.14.4. Description

If the media device supports [requests](request-api.html#media-request-api), then
this request ioctl can be used to re-initialize a previously allocated
request.

Re-initializing a request will clear any existing data from the request.
This avoids having to [`close()`](media-func-close.html#c.MC.close "close") a completed
request and allocate a new request. Instead the completed request can just
be re-initialized and it is ready to be used again.

A request can only be re-initialized if it either has not been queued
yet, or if it was queued and completed. Otherwise it will set `errno`
to `EBUSY`. No other error codes can be returned.

## 5.14.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately.

EBUSY
:   The request is queued but not yet completed.
