# 7.1.V4L2 close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/func-close.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.1. V4L2 close()

## 7.1.1. Name

v4l2-close - Close a V4L2 device

## 7.1.2. Synopsis

```
#include <unistd.h>
```

int close(int fd)

## 7.1.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

## 7.1.4. Description

Closes the device. Any I/O in progress is terminated and resources
associated with the file descriptor are freed. However data format
parameters, current input or output, control values or other properties
remain unchanged.

## 7.1.5. Return Value

The function returns 0 on success, -1 on failure and the `errno` is
set appropriately. Possible error codes:

EBADF
:   `fd` is not a valid open file descriptor.
