# 3.2.3.Digital TV demux read()

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx-fread.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.2.3. Digital TV demux read()

## 3.2.3.1. Name

Digital TV demux [`read()`](#c.DTV.dmx.read "DTV.dmx.read")

## 3.2.3.2. Synopsis

size\_t read(int fd, void \*buf, size\_t count)

## 3.2.3.3. Arguments

`fd`
:   > File descriptor returned by a previous call to [`open()`](dmx-fopen.html#c.DTV.dmx.open "open").

    `buf`
    :   Buffer to be filled

`count`
:   Max number of bytes to read

## 3.2.3.4. Description

This system call returns filtered data, which might be section or Packetized
Elementary Stream (PES) data. The filtered data is transferred from
the driver’s internal circular buffer to `buf`. The maximum amount of data
to be transferred is implied by count.

Note

if a section filter created with
[`DMX_CHECK_CRC`](dmx_types.html#c.DTV.dmx.dmx_sct_filter_params "dmx_sct_filter_params") flag set,
data that fails on CRC check will be silently ignored.

## 3.2.3.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

|  |  |
| --- | --- |
| `EWOULDBLOCK` | No data to return and `O_NONBLOCK` was specified. |
| `EOVERFLOW` | The filtered data was not read from the buffer in due time, resulting in non-read data being lost. The buffer is flushed. |
| `ETIMEDOUT` | The section was not loaded within the stated timeout period. See ioctl [DMX\_SET\_FILTER](dmx-set-filter.html#dmx-set-filter) for how to set a timeout. |
| `EFAULT` | The driver failed to write to the callers buffer due to an invalid \*buf pointer. |

The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
