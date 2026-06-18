# 5.10.request close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/request-func-close.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.10. request close()

## 5.10.1. Name

request-close - Close a request file descriptor

## 5.10.2. Synopsis

```
#include <unistd.h>
```

int close(int fd)

## 5.10.3. Arguments

`fd`
:   File descriptor returned by [ioctl MEDIA\_IOC\_REQUEST\_ALLOC](media-ioc-request-alloc.html#media-ioc-request-alloc).

## 5.10.4. Description

Closes the request file descriptor. Resources associated with the request
are freed once all file descriptors associated with the request are closed
and the driver has completed the request.
See [here](request-api.html#media-request-life-time) for more information.

## 5.10.5. Return Value

[`close()`](#c.MC.request.close "close") returns 0 on success. On error, -1 is
returned, and `errno` is set appropriately. Possible error codes are:

EBADF
:   `fd` is not a valid open file descriptor.
