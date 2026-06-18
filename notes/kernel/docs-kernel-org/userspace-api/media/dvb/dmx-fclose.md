# 3.2.2.Digital TV demux close()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-fclose.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.2. Digital TV demux close()

## 3.2.2.1. Name

Digital TV demux [`close()`](#c.DTV.dmx.close "DTV.dmx.close")

## 3.2.2.2. Synopsis

int close(int fd)

## 3.2.2.3. Arguments

`fd`
:   File descriptor returned by a previous call to
    [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

## 3.2.2.4. Description

This system call deactivates and deallocates a filter that was
previously allocated via the [`open()`](dmx-fopen.html#c.DTV.dmx.open "open") call.

## 3.2.2.5. Return Value

On success 0 is returned.

On error, -1 is returned and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
