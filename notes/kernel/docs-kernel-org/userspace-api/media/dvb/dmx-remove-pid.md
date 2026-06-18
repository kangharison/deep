# 3.2.15.DMX_REMOVE_PID

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-remove-pid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.15. DMX\_REMOVE\_PID

## 3.2.15.1. Name

DMX\_REMOVE\_PID

## 3.2.15.2. Synopsis

DMX\_REMOVE\_PID

`int ioctl(fd, DMX_REMOVE_PID, __u16 *pid)`

## 3.2.15.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`pid`
:   PID of the PES filter to be removed.

## 3.2.15.4. Description

This ioctl call allows to remove a PID when multiple PIDs are set on a
transport stream filter, e. g. a filter previously set up with output
equal to [`DMX_OUT_TSDEMUX_TAP`](dmx_types.html#c.DTV.dmx.dmx_output "dmx_output"), created via either
[DMX\_SET\_PES\_FILTER](dmx-set-pes-filter.html#dmx-set-pes-filter) or [DMX\_ADD\_PID](dmx-add-pid.html#dmx-add-pid).

## 3.2.15.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
