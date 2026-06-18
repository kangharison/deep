# 3.2.11.DMX_SET_BUFFER_SIZE

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-set-buffer-size.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.11. DMX\_SET\_BUFFER\_SIZE

## 3.2.11.1. Name

DMX\_SET\_BUFFER\_SIZE

## 3.2.11.2. Synopsis

DMX\_SET\_BUFFER\_SIZE

`int ioctl(int fd, DMX_SET_BUFFER_SIZE, unsigned long size)`

## 3.2.11.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`size`
:   Unsigned long size

## 3.2.11.4. Description

This ioctl call is used to set the size of the circular buffer used for
filtered data. The default size is two maximum sized sections, i.e. if
this function is not called a buffer size of `2 * 4096` bytes will be
used.

## 3.2.11.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
