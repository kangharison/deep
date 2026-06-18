# 3.2.14.DMX_ADD_PID

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-add-pid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.14. DMX\_ADD\_PID

## 3.2.14.1. Name

DMX\_ADD\_PID

## 3.2.14.2. Synopsis

DMX\_ADD\_PID

`int ioctl(fd, DMX_ADD_PID, __u16 *pid)`

## 3.2.14.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`pid`
:   PID number to be filtered.

## 3.2.14.4. Description

This ioctl call allows to add multiple PIDs to a transport stream filter
previously set up with [DMX\_SET\_PES\_FILTER](dmx-set-pes-filter.html#dmx-set-pes-filter) and output equal to
[`DMX_OUT_TSDEMUX_TAP`](dmx_types.html#c.DTV.dmx.dmx_output "dmx_output").

## 3.2.14.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
