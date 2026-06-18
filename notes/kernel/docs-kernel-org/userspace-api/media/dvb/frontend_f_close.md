# 2.4.2.Digital TV frontend close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/frontend_f_close.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.2. Digital TV frontend close()

## 2.4.2.1. Name

fe-close - Close a frontend device

## 2.4.2.2. Synopsis

```
#include <unistd.h>
```

int close(int fd)

## 2.4.2.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

## 2.4.2.4. Description

This system call closes a previously opened front-end device. After
closing a front-end device, its corresponding hardware might be powered
down automatically.

## 2.4.2.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
