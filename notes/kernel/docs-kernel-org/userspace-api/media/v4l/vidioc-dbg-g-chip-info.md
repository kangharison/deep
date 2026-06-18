# 7.5.ioctl VIDIOC_DBG_G_CHIP_INFO

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-dbg-g-chip-info.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.5. ioctl VIDIOC\_DBG\_G\_CHIP\_INFO

## 7.5.1. Name

VIDIOC\_DBG\_G\_CHIP\_INFO - Identify the chips on a TV card

## 7.5.2. Synopsis

VIDIOC\_DBG\_G\_CHIP\_INFO

`int ioctl(int fd, VIDIOC_DBG_G_CHIP_INFO, struct v4l2_dbg_chip_info *argp)`

## 7.5.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_dbg_chip_info`](#c.V4L.v4l2_dbg_chip_info "v4l2_dbg_chip_info").

## 7.5.4. Description

Note

This is an [Experimental API Elements](hist-v4l2.html#experimental) interface and may
change in the future.

For driver debugging purposes this ioctl allows test applications to
query the driver about the chips present on the TV card. Regular
applications must not use it. When you found a chip specific bug, please
contact the linux-media mailing list
(<https://linuxtv.org/lists.php>)
so it can be fixed.

Additionally the Linux kernel must be compiled with the
`CONFIG_VIDEO_ADV_DEBUG` option to enable this ioctl.

To query the driver applications must initialize the `match.type` and
`match.addr` or `match.name` fields of a struct
[`v4l2_dbg_chip_info`](#c.V4L.v4l2_dbg_chip_info "v4l2_dbg_chip_info") and call
[ioctl VIDIOC\_DBG\_G\_CHIP\_INFO](#vidioc-dbg-g-chip-info) with a pointer to this structure. On success
the driver stores information about the selected chip in the `name`
and `flags` fields.

When `match.type` is `V4L2_CHIP_MATCH_BRIDGE`, `match.addr`
selects the nth bridge ‘chip’ on the TV card. You can enumerate all
chips by starting at zero and incrementing `match.addr` by one until
[ioctl VIDIOC\_DBG\_G\_CHIP\_INFO](#vidioc-dbg-g-chip-info) fails with an `EINVAL` error code. The number
zero always selects the bridge chip itself, e. g. the chip connected to
the PCI or USB bus. Non-zero numbers identify specific parts of the
bridge chip such as an AC97 register block.

When `match.type` is `V4L2_CHIP_MATCH_SUBDEV`, `match.addr`
selects the nth sub-device. This allows you to enumerate over all
sub-devices.

On success, the `name` field will contain a chip name and the
`flags` field will contain `V4L2_CHIP_FL_READABLE` if the driver
supports reading registers from the device or `V4L2_CHIP_FL_WRITABLE`
if the driver supports writing registers to the device.

We recommended the v4l2-dbg utility over calling this ioctl directly. It
is available from the LinuxTV v4l-dvb repository; see
<https://linuxtv.org/repo/> for access
instructions.

struct v4l2\_dbg\_match

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | See [Chip Match Types](#name-chip-match-types) for a list of possible types. |
| union { | (anonymous) | |
| \_\_u32 | `addr` | Match a chip by this number, interpreted according to the `type` field. |
| char | `name[32]` | Match a chip by this name, interpreted according to the `type` field. Currently unused. |
| } |  | |

type v4l2\_dbg\_chip\_info

struct v4l2\_dbg\_chip\_info

|  |  |  |
| --- | --- | --- |
| [`struct v4l2_dbg_match`](vidioc-dbg-g-register.html#c.V4L.v4l2_dbg_match "V4L.v4l2_dbg_match") | `match` | How to match the chip, see [struct v4l2\_dbg\_match](#name-v4l2-dbg-match). |
| char | `name[32]` | The name of the chip. |
| \_\_u32 | `flags` | Set by the driver. If `V4L2_CHIP_FL_READABLE` is set, then the driver supports reading registers from the device. If `V4L2_CHIP_FL_WRITABLE` is set, then it supports writing registers. |
| \_\_u32 | `reserved[8]` | Reserved fields, both application and driver must set these to 0. |

Chip Match Types

|  |  |  |
| --- | --- | --- |
| `V4L2_CHIP_MATCH_BRIDGE` | 0 | Match the nth chip on the card, zero for the bridge chip. Does not match sub-devices. |
| `V4L2_CHIP_MATCH_SUBDEV` | 4 | Match the nth sub-device. |

## 7.5.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The `match_type` is invalid or no device could be matched.
