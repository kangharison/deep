# 2.2.cec close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-func-close.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.2. cec close()

## 2.2.1. Name

cec-close - Close a cec device

## 2.2.2. Synopsis

```
#include <unistd.h>
```

int close(int fd)

## 2.2.3. Arguments

`fd`
:   File descriptor returned by [`open()`](cec-func-open.html#c.CEC.open "open").

## 2.2.4. Description

Closes the cec device. Resources associated with the file descriptor are
freed. The device configuration remain unchanged.

## 2.2.5. Return Value

[`close()`](#c.CEC.close "close") returns 0 on success. On error, -1 is returned, and
`errno` is set appropriately. Possible error codes are:

`EBADF`
:   `fd` is not a valid open file descriptor.
