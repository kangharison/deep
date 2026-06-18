# 7.6.ioctl VIDIOC_DBG_G_REGISTER, VIDIOC_DBG_S_REGISTER

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-dbg-g-register.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.6. ioctl VIDIOC\_DBG\_G\_REGISTER, VIDIOC\_DBG\_S\_REGISTER

## 7.6.1. Name

VIDIOC\_DBG\_G\_REGISTER - VIDIOC\_DBG\_S\_REGISTER - Read or write hardware registers

## 7.6.2. Synopsis

VIDIOC\_DBG\_G\_REGISTER

`int ioctl(int fd, VIDIOC_DBG_G_REGISTER, struct v4l2_dbg_register *argp)`

VIDIOC\_DBG\_S\_REGISTER

`int ioctl(int fd, VIDIOC_DBG_S_REGISTER, const struct v4l2_dbg_register *argp)`

## 7.6.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_dbg_register`](#c.V4L.v4l2_dbg_register "v4l2_dbg_register").

## 7.6.4. Description

Note

This is an [Experimental API Elements](hist-v4l2.html#experimental) interface and may
change in the future.

For driver debugging purposes these ioctls allow test applications to
access hardware registers directly. Regular applications must not use
them.

Since writing or even reading registers can jeopardize the system
security, its stability and damage the hardware, both ioctls require
superuser privileges. Additionally the Linux kernel must be compiled
with the `CONFIG_VIDEO_ADV_DEBUG` option to enable these ioctls.

To write a register applications must initialize all fields of a struct
[`v4l2_dbg_register`](#c.V4L.v4l2_dbg_register "v4l2_dbg_register") except for `size` and
call `VIDIOC_DBG_S_REGISTER` with a pointer to this structure. The
`match.type` and `match.addr` or `match.name` fields select a chip
on the TV card, the `reg` field specifies a register number and the
`val` field the value to be written into the register.

To read a register applications must initialize the `match.type`,
`match.addr` or `match.name` and `reg` fields, and call
`VIDIOC_DBG_G_REGISTER` with a pointer to this structure. On success
the driver stores the register value in the `val` field and the size
(in bytes) of the value in `size`.

When `match.type` is `V4L2_CHIP_MATCH_BRIDGE`, `match.addr`
selects the nth non-sub-device chip on the TV card. The number zero
always selects the host chip, e. g. the chip connected to the PCI or USB
bus. You can find out which chips are present with the
[ioctl VIDIOC\_DBG\_G\_CHIP\_INFO](vidioc-dbg-g-chip-info.html#vidioc-dbg-g-chip-info) ioctl.

When `match.type` is `V4L2_CHIP_MATCH_SUBDEV`, `match.addr`
selects the nth sub-device.

These ioctls are optional, not all drivers may support them. However
when a driver supports these ioctls it must also support
[ioctl VIDIOC\_DBG\_G\_CHIP\_INFO](vidioc-dbg-g-chip-info.html#vidioc-dbg-g-chip-info). Conversely
it may support `VIDIOC_DBG_G_CHIP_INFO` but not these ioctls.

`VIDIOC_DBG_G_REGISTER` and `VIDIOC_DBG_S_REGISTER` were introduced
in Linux 2.6.21, but their API was changed to the one described here in
kernel 2.6.29.

We recommended the v4l2-dbg utility over calling these ioctls directly.
It is available from the LinuxTV v4l-dvb repository; see
<https://linuxtv.org/repo/> for access
instructions.

type v4l2\_dbg\_match

struct v4l2\_dbg\_match

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | See [Chip Match Types](#chip-match-types) for a list of possible types. |
| union { | (anonymous) | |
| \_\_u32 | `addr` | Match a chip by this number, interpreted according to the `type` field. |
| char | `name[32]` | Match a chip by this name, interpreted according to the `type` field. Currently unused. |
| } |  | |

type v4l2\_dbg\_register

struct v4l2\_dbg\_register

|  |  |  |
| --- | --- | --- |
| [`struct v4l2_dbg_match`](#c.V4L.v4l2_dbg_match "V4L.v4l2_dbg_match") | `match` | How to match the chip, see [`v4l2_dbg_match`](#c.V4L.v4l2_dbg_match "v4l2_dbg_match"). |
| \_\_u32 | `size` | The register size in bytes. |
| \_\_u64 | `reg` | A register number. |
| \_\_u64 | `val` | The value read from, or to be written into the register. |

Chip Match Types

|  |  |  |
| --- | --- | --- |
| `V4L2_CHIP_MATCH_BRIDGE` | 0 | Match the nth chip on the card, zero for the bridge chip. Does not match sub-devices. |
| `V4L2_CHIP_MATCH_SUBDEV` | 4 | Match the nth sub-device. |

## 7.6.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EPERM
:   Insufficient permissions. Root privileges are required to execute
    these ioctls.
