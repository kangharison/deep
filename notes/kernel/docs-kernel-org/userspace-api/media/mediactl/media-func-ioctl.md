# 5.3.media ioctl()

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-func-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.3. media ioctl()

## 5.3.1. Name

media-ioctl - Control a media device

## 5.3.2. Synopsis

```
#include <sys/ioctl.h>
```

`int ioctl(int fd, int request, void *argp)`

## 5.3.3. Arguments

`fd`
:   File descriptor returned by [`open()`](media-func-open.html#c.MC.open "open").

`request`
:   Media ioctl request code as defined in the media.h header file, for
    example MEDIA\_IOC\_SETUP\_LINK.

`argp`
:   Pointer to a request-specific structure.

## 5.3.4. Description

The [ioctl()](#media-func-ioctl) function manipulates media device
parameters. The argument `fd` must be an open file descriptor.

The ioctl `request` code specifies the media function to be called. It
has encoded in it whether the argument is an input, output or read/write
parameter, and the size of the argument `argp` in bytes.

Macros and structures definitions specifying media ioctl requests and
their parameters are located in the media.h header file. All media ioctl
requests, their respective function and parameters are specified in
[Function Reference](media-funcs.html#media-user-func).

## 5.3.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

Request-specific error codes are listed in the individual requests
descriptions.

When an ioctl that takes an output or read/write parameter fails, the
parameter remains unmodified.
