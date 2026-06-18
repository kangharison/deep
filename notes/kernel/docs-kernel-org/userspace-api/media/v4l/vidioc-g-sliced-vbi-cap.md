# 7.40.ioctl VIDIOC_G_SLICED_VBI_CAP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-sliced-vbi-cap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.40. ioctl VIDIOC\_G\_SLICED\_VBI\_CAP

## 7.40.1. Name

VIDIOC\_G\_SLICED\_VBI\_CAP - Query sliced VBI capabilities

## 7.40.2. Synopsis

VIDIOC\_G\_SLICED\_VBI\_CAP

`int ioctl(int fd, VIDIOC_G_SLICED_VBI_CAP, struct v4l2_sliced_vbi_cap *argp)`

## 7.40.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_sliced_vbi_cap`](#c.V4L.v4l2_sliced_vbi_cap "v4l2_sliced_vbi_cap").

## 7.40.4. Description

To find out which data services are supported by a sliced VBI capture or
output device, applications initialize the `type` field of a struct
[`v4l2_sliced_vbi_cap`](#c.V4L.v4l2_sliced_vbi_cap "v4l2_sliced_vbi_cap"), clear the
`reserved` array and call the [VIDIOC\_G\_SLICED\_VBI\_CAP](#vidioc-g-sliced-vbi-cap) ioctl. The
driver fills in the remaining fields or returns an `EINVAL` error code if
the sliced VBI API is unsupported or `type` is invalid.

Note

The `type` field was added, and the ioctl changed from read-only
to write-read, in Linux 2.6.19.

type v4l2\_sliced\_vbi\_cap

struct v4l2\_sliced\_vbi\_cap

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| \_\_u16 | `service_set` | A set of all data services supported by the driver.  Equal to the `union of` all elements of the `service_lines` array. | | |
| \_\_u16 | `service_lines`[2][24] | Each element of this array contains a set of data services the hardware can look for or insert into a particular scan line. Data services are defined in [Sliced VBI services](#vbi-services). Array indices map to ITU-R line numbers[[1]](#f1) as follows: | | |
|  |  | Element | 525 line systems | 625 line systems |
|  |  | `service_lines`[0][1] | 1 | 1 |
|  |  | `service_lines`[0][23] | 23 | 23 |
|  |  | `service_lines`[1][1] | 264 | 314 |
|  |  | `service_lines`[1][23] | 286 | 336 |
|  | | | | |
|  |  | The number of VBI lines the hardware can capture or output per frame, or the number of services it can identify on a given line may be limited. For example on PAL line 16 the hardware may be able to look for a VPS or Teletext signal, but not both at the same time. Applications can learn about these limits using the [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt) ioctl as described in [Sliced VBI Data Interface](dev-sliced-vbi.html#sliced). | | |
|  | | | | |
|  |  | Drivers must set `service_lines` [0][0] and `service_lines`[1][0] to zero. | | |
| \_\_u32 | `type` | Type of the data stream, see [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type"). Should be `V4L2_BUF_TYPE_SLICED_VBI_CAPTURE` or `V4L2_BUF_TYPE_SLICED_VBI_OUTPUT`. | | |
| \_\_u32 | `reserved`[3] | This array is reserved for future extensions.  Applications and drivers must set it to zero. | | |

[[1](#id1)]

See also [ITU-R 525 line numbering (M/NTSC and M/PAL)](dev-raw-vbi.html#vbi-525) and [ITU-R 625 line numbering](dev-raw-vbi.html#vbi-625).



Sliced VBI services

| Symbol | Value | Reference | Lines, usually | Payload |
| --- | --- | --- | --- | --- |
| `V4L2_SLICED_TELETEXT_B` (Teletext System B) | 0x0001 | [ETS 300 706](biblio.html#ets300706),  [ITU BT.653](biblio.html#itu653) | PAL/SECAM line 7-22, 320-335 (second field 7-22) | Last 42 of the 45 byte Teletext packet, that is without clock run-in and framing code, lsb first transmitted. |
| `V4L2_SLICED_VPS` | 0x0400 | [ETS 300 231](biblio.html#ets300231) | PAL line 16 | Byte number 3 to 15 according to Figure 9 of ETS 300 231, lsb first transmitted. |
| `V4L2_SLICED_CAPTION_525` | 0x1000 | [CEA 608-E](biblio.html#cea608) | NTSC line 21, 284 (second field 21) | Two bytes in transmission order, including parity bit, lsb first transmitted. |
| `V4L2_SLICED_WSS_625` | 0x4000 | [EN 300 294](biblio.html#en300294),  [ITU BT.1119](biblio.html#itu1119) | PAL/SECAM line 23 | See [V4L2\_SLICED\_VBI\_CAP WSS\_625 payload](#v4l2-sliced-vbi-cap-wss-625-payload) below. |
| `V4L2_SLICED_VBI_525` | 0x1000 | Set of services applicable to 525 line systems. | | |
| `V4L2_SLICED_VBI_625` | 0x4401 | Set of services applicable to 625 line systems. | | |

### 7.40.4.1. V4L2\_SLICED\_VBI\_CAP WSS\_625 payload

The payload for `V4L2_SLICED_WSS_625` is:

> |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
> | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
> | Byte | 0 | | | | | | | | 1 | | | | | | | |
> | Bit | msb | | | | lsb | | | | msb | | | | lsb | | | |
> | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | x | x | 13 | 12 | 11 | 10 | 9 | 8 |

## 7.40.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The value in the `type` field is wrong.
