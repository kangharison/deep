# 3.2.6.DVB munmap()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-munmap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.6. DVB munmap()

## 3.2.6.1. Name

dmx-munmap - Unmap device memory

Warning

This API is still experimental.

## 3.2.6.2. Synopsis

```
#include <unistd.h>
#include <sys/mman.h>
```

int munmap(void \*start, size\_t length)

## 3.2.6.3. Arguments

`start`
:   Address of the mapped buffer as returned by the
    [`mmap()`](dmx-mmap.html#c.DTV.dmx.mmap "mmap") function.

`length`
:   Length of the mapped buffer. This must be the same value as given to
    [`mmap()`](dmx-mmap.html#c.DTV.dmx.mmap "mmap").

## 3.2.6.4. Description

Unmaps a previously with the [`mmap()`](dmx-mmap.html#c.DTV.dmx.mmap "mmap") function mapped
buffer and frees it, if possible.

## 3.2.6.5. Return Value

On success [`munmap()`](#c.DTV.dmx.munmap "munmap") returns 0, on failure -1 and the
`errno` variable is set appropriately:

EINVAL
:   The `start` or `length` is incorrect, or no buffers have been
    mapped yet.
