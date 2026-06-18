# 3.2.7.DMX_START

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-start.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.7. DMX\_START

## 3.2.7.1. Name

DMX\_START

## 3.2.7.2. Synopsis

DMX\_START

`int ioctl(int fd, DMX_START)`

## 3.2.7.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

## 3.2.7.4. Description

This ioctl call is used to start the actual filtering operation defined
via the ioctl calls [DMX\_SET\_FILTER](dmx-set-filter.html#dmx-set-filter) or [DMX\_SET\_PES\_FILTER](dmx-set-pes-filter.html#dmx-set-pes-filter).

## 3.2.7.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `EINVAL` | Invalid argument, i.e. no filtering parameters provided via the [DMX\_SET\_FILTER](dmx-set-filter.html#dmx-set-filter) or [DMX\_SET\_PES\_FILTER](dmx-set-pes-filter.html#dmx-set-pes-filter) ioctls. |
| `EBUSY` | This error code indicates that there are conflicting requests. There are active filters filtering data from another input source. Make sure that these filters are stopped before starting this filter. |

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
