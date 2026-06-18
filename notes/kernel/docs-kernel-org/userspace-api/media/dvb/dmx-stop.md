# 3.2.8.DMX_STOP

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-stop.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.8. DMX\_STOP

## 3.2.8.1. Name

DMX\_STOP

## 3.2.8.2. Synopsis

DMX\_STOP

`int ioctl(int fd, DMX_STOP)`

## 3.2.8.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

## 3.2.8.4. Description

This ioctl call is used to stop the actual filtering operation defined
via the ioctl calls [DMX\_SET\_FILTER](dmx-set-filter.html#dmx-set-filter) or [DMX\_SET\_PES\_FILTER](dmx-set-pes-filter.html#dmx-set-pes-filter) and
started via the [DMX\_START](dmx-start.html#dmx-start) command.

## 3.2.8.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
