# 3.2.12.DMX_GET_STC

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-get-stc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.12. DMX\_GET\_STC

## 3.2.12.1. Name

DMX\_GET\_STC

## 3.2.12.2. Synopsis

DMX\_GET\_STC

`int ioctl(int fd, DMX_GET_STC, struct dmx_stc *stc)`

## 3.2.12.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`stc`
:   Pointer to [`dmx_stc`](dmx_types.html#c.DTV.dmx.dmx_stc "dmx_stc") where the stc data is to be stored.

## 3.2.12.4. Description

This ioctl call returns the current value of the system time counter
(which is driven by a PES filter of type [`DMX_PES_PCR`](dmx_types.html#c.DTV.dmx.dmx_ts_pes "dmx_ts_pes")).
Some hardware supports more than one STC, so you must specify which one by
setting the [`num`](dmx_types.html#c.DTV.dmx.dmx_stc "dmx_stc") field of stc before the ioctl (range 0...n).
The result is returned in form of a ratio with a 64 bit numerator
and a 32 bit denominator, so the real 90kHz STC value is
`stc->stc / stc->base`.

## 3.2.12.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `EINVAL` | Invalid stc number. |

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
