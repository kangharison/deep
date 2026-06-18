# 5.11.request ioctl()

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/request-func-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.11. request ioctl()

## 5.11.1. Name

request-ioctl - Control a request file descriptor

## 5.11.2. Synopsis

```
#include <sys/ioctl.h>
```

`int ioctl(int fd, int cmd, void *argp)`

## 5.11.3. Arguments

`fd`
:   File descriptor returned by [ioctl MEDIA\_IOC\_REQUEST\_ALLOC](media-ioc-request-alloc.html#media-ioc-request-alloc).

`cmd`
:   The request ioctl command code as defined in the media.h header file, for
    example [ioctl MEDIA\_REQUEST\_IOC\_QUEUE](media-request-ioc-queue.html#media-request-ioc-queue).

`argp`
:   Pointer to a request-specific structure.

## 5.11.4. Description

The [ioctl()](#request-func-ioctl) function manipulates request
parameters. The argument `fd` must be an open file descriptor.

The ioctl `cmd` code specifies the request function to be called. It
has encoded in it whether the argument is an input, output or read/write
parameter, and the size of the argument `argp` in bytes.

Macros and structures definitions specifying request ioctl commands and
their parameters are located in the media.h header file. All request ioctl
commands, their respective function and parameters are specified in
[Function Reference](media-funcs.html#media-user-func).

## 5.11.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

Command-specific error codes are listed in the individual command
descriptions.

When an ioctl that takes an output or read/write parameter fails, the
parameter remains unmodified.
