# 5.2.media close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-func-close.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.2. media close()

## 5.2.1. Name

media-close - Close a media device

## 5.2.2. Synopsis

```
#include <unistd.h>
```

int close(int fd)

## 5.2.3. Arguments

`fd`
:   File descriptor returned by [`open()`](media-func-open.html#c.MC.open "open").

## 5.2.4. Description

Closes the media device. Resources associated with the file descriptor
are freed. The device configuration remain unchanged.

## 5.2.5. Return Value

[`close()`](#c.MC.close "close") returns 0 on success. On error, -1 is returned, and
`errno` is set appropriately. Possible error codes are:

EBADF
:   `fd` is not a valid open file descriptor.
