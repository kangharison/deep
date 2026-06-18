# 3.2.18.ioctl DMX_EXPBUF

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-expbuf.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.18. ioctl DMX\_EXPBUF

## 3.2.18.1. Name

DMX\_EXPBUF - Export a buffer as a DMABUF file descriptor.

Warning

this API is still experimental

## 3.2.18.2. Synopsis

DMX\_EXPBUF

`int ioctl(int fd, DMX_EXPBUF, struct dmx_exportbuffer *argp)`

## 3.2.18.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`argp`
:   Pointer to struct [`dmx_exportbuffer`](dmx_types.html#c.DTV.dmx.dmx_exportbuffer "dmx_exportbuffer").

## 3.2.18.4. Description

This ioctl is an extension to the memory mapping I/O method.
It can be used to export a buffer as a DMABUF file at any time after
buffers have been allocated with the [ioctl DMX\_REQBUFS](dmx-reqbufs.html#dmx-reqbufs) ioctl.

To export a buffer, applications fill struct [`dmx_exportbuffer`](dmx_types.html#c.DTV.dmx.dmx_exportbuffer "dmx_exportbuffer").
Applications must set the `index` field. Valid index numbers
range from zero to the number of buffers allocated with [ioctl DMX\_REQBUFS](dmx-reqbufs.html#dmx-reqbufs)
(struct [`dmx_requestbuffers`](dmx_types.html#c.DTV.dmx.dmx_requestbuffers "dmx_requestbuffers") `count`) minus one.
Additional flags may be posted in the `flags` field. Refer to a manual
for [`open()`](dmx-fopen.html#c.DTV.dmx.open "DTV.dmx.open") for details. Currently only O\_CLOEXEC, O\_RDONLY, O\_WRONLY,
and O\_RDWR are supported.
All other fields must be set to zero. In the
case of multi-planar API, every plane is exported separately using
multiple [ioctl DMX\_EXPBUF](#dmx-expbuf) calls.

After calling [ioctl DMX\_EXPBUF](#dmx-expbuf) the `fd` field will be set by a
driver, on success. This is a DMABUF file descriptor. The application may
pass it to other DMABUF-aware devices. It is recommended to close a DMABUF
file when it is no longer used to allow the associated memory to be reclaimed.

## 3.2.18.5. Examples

```
int buffer_export(int v4lfd, enum dmx_buf_type bt, int index, int *dmafd)
{
    struct dmx_exportbuffer expbuf;

    memset(&expbuf, 0, sizeof(expbuf));
    expbuf.type = bt;
    expbuf.index = index;
    if (ioctl(v4lfd, DMX_EXPBUF, &expbuf) == -1) {
        perror("DMX_EXPBUF");
        return -1;
    }

    *dmafd = expbuf.fd;

    return 0;
}
```

## 3.2.18.6. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   A queue is not in MMAP mode or DMABUF exporting is not supported or
    `flags` or `index` fields are invalid.
