# 7.68.V4L2 munmap()

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/func-munmap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.68. V4L2 munmap()

## 7.68.1. Name

v4l2-munmap - Unmap device memory

## 7.68.2. Synopsis

```
#include <unistd.h>
#include <sys/mman.h>
```

int munmap(void \*start, size\_t length)

## 7.68.3. Arguments

`start`
:   Address of the mapped buffer as returned by the
    [`mmap()`](func-mmap.html#c.V4L.mmap "mmap") function.

`length`
:   Length of the mapped buffer. This must be the same value as given to
    [`mmap()`](func-mmap.html#c.V4L.mmap "mmap") and returned by the driver in the struct
    [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") `length` field for the
    single-planar API and in the struct
    [`v4l2_plane`](buffer.html#c.V4L.v4l2_plane "v4l2_plane") `length` field for the
    multi-planar API.

## 7.68.4. Description

Unmaps a previously with the [`mmap()`](func-mmap.html#c.V4L.mmap "mmap") function mapped
buffer and frees it, if possible.

## 7.68.5. Return Value

On success [`munmap()`](#c.V4L.munmap "munmap") returns 0, on failure -1 and the
`errno` variable is set appropriately:

EINVAL
:   The `start` or `length` is incorrect, or no buffers have been
    mapped yet.
