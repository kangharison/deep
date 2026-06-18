# 2.3.cec ioctl()

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-func-ioctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.3. cec ioctl()

## 2.3.1. Name

cec-ioctl - Control a cec device

## 2.3.2. Synopsis

```
#include <sys/ioctl.h>
```

`int ioctl(int fd, int request, void *argp)`

## 2.3.3. Arguments

`fd`
:   File descriptor returned by [`open()`](cec-func-open.html#c.CEC.open "open").

`request`
:   CEC ioctl request code as defined in the cec.h header file, for
    example [CEC\_ADAP\_G\_CAPS](cec-ioc-adap-g-caps.html#cec-adap-g-caps).

`argp`
:   Pointer to a request-specific structure.

## 2.3.4. Description

The `ioctl()` function manipulates cec device parameters. The
argument `fd` must be an open file descriptor.

The ioctl `request` code specifies the cec function to be called. It
has encoded in it whether the argument is an input, output or read/write
parameter, and the size of the argument `argp` in bytes.

Macros and structures definitions specifying cec ioctl requests and
their parameters are located in the cec.h header file. All cec ioctl
requests, their respective function and parameters are specified in
[Function Reference](cec-funcs.html#cec-user-func).

## 2.3.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

Request-specific error codes are listed in the individual requests
descriptions.

When an ioctl that takes an output or read/write parameter fails, the
parameter remains unmodified.
