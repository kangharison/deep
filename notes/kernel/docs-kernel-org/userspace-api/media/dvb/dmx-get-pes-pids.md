# 3.2.13.DMX_GET_PES_PIDS

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-get-pes-pids.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.13. DMX\_GET\_PES\_PIDS

## 3.2.13.1. Name

DMX\_GET\_PES\_PIDS

## 3.2.13.2. Synopsis

DMX\_GET\_PES\_PIDS

`int ioctl(fd, DMX_GET_PES_PIDS, __u16 pids[5])`

## 3.2.13.3. Arguments

`fd`
:   File descriptor returned by [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`pids`
:   Array used to store 5 Program IDs.

## 3.2.13.4. Description

This ioctl allows to query a DVB device to return the first PID used
by audio, video, textext, subtitle and PCR programs on a given service.
They’re stored as:

| PID element | position | content |
| --- | --- | --- |
| pids[DMX\_PES\_AUDIO] | 0 | first audio PID |
| pids[DMX\_PES\_VIDEO] | 1 | first video PID |
| pids[DMX\_PES\_TELETEXT] | 2 | first teletext PID |
| pids[DMX\_PES\_SUBTITLE] | 3 | first subtitle PID |
| pids[DMX\_PES\_PCR] | 4 | first Program Clock Reference PID |

Note

A value equal to 0xffff means that the PID was not filled by the
Kernel.

## 3.2.13.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
