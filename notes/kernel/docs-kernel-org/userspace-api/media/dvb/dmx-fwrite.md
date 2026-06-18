# 3.2.4.Digital TV demux write()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-fwrite.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.4. Digital TV demux write()

## 3.2.4.1. Name

Digital TV demux [`write()`](#c.DTV.dmx.write "DTV.dmx.write")

## 3.2.4.2. Synopsis

ssize\_t write(int fd, const void \*buf, size\_t count)

## 3.2.4.3. Arguments

`fd`
:   File descriptor returned by a previous call to [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

`buf`
:   Buffer with data to be written

`count`
:   Number of bytes at the buffer

## 3.2.4.4. Description

This system call is only provided by the logical device
`/dev/dvb/adapter?/dvr?`, associated with the physical demux device that
provides the actual DVR functionality. It is used for replay of a
digitally recorded Transport Stream. Matching filters have to be defined
in the corresponding physical demux device, `/dev/dvb/adapter?/demux?`.
The amount of data to be transferred is implied by count.

## 3.2.4.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `EWOULDBLOCK` | No data was written. This might happen if `O_NONBLOCK` was specified and there is no more buffer space available (if `O_NONBLOCK` is not specified the function will block until buffer space is available). |
| `EBUSY` | This error code indicates that there are conflicting requests. The corresponding demux device is setup to receive data from the front- end. Make sure that these filters are stopped and that the filters with input set to `DMX_IN_DVR` are started. |

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
