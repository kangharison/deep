# 7.27.ioctl VIDIOC_G_EDID, VIDIOC_S_EDID, VIDIOC_SUBDEV_G_EDID, VIDIOC_SUBDEV_S_EDID

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-edid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.27. ioctl VIDIOC\_G\_EDID, VIDIOC\_S\_EDID, VIDIOC\_SUBDEV\_G\_EDID, VIDIOC\_SUBDEV\_S\_EDID

## 7.27.1. Name

VIDIOC\_G\_EDID - VIDIOC\_S\_EDID - VIDIOC\_SUBDEV\_G\_EDID - VIDIOC\_SUBDEV\_S\_EDID - Get or set the EDID of a video receiver/transmitter

## 7.27.2. Synopsis

VIDIOC\_G\_EDID

`int ioctl(int fd, VIDIOC_G_EDID, struct v4l2_edid *argp)`

VIDIOC\_S\_EDID

`int ioctl(int fd, VIDIOC_S_EDID, struct v4l2_edid *argp)`

VIDIOC\_SUBDEV\_G\_EDID

`int ioctl(int fd, VIDIOC_SUBDEV_G_EDID, struct v4l2_edid *argp)`

VIDIOC\_SUBDEV\_S\_EDID

`int ioctl(int fd, VIDIOC_SUBDEV_S_EDID, struct v4l2_edid *argp)`

## 7.27.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_edid`](#c.V4L.v4l2_edid "v4l2_edid").

## 7.27.4. Description

These ioctls can be used to get or set an EDID associated with an input
from a receiver or an output of a transmitter device. They can be used
with subdevice nodes (/dev/v4l-subdevX) or with video nodes
(/dev/videoX).

When used with video nodes the `pad` field represents the input (for
video capture devices) or output (for video output devices) index as is
returned by [ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) and
[ioctl VIDIOC\_ENUMOUTPUT](vidioc-enumoutput.html#vidioc-enumoutput) respectively. When used
with subdevice nodes the `pad` field represents the input or output
pad of the subdevice. If there is no EDID support for the given `pad`
value, then the `EINVAL` error code will be returned.

To get the EDID data the application has to fill in the `pad`,
`start_block`, `blocks` and `edid` fields, zero the `reserved`
array and call [VIDIOC\_G\_EDID](#vidioc-g-edid). The current EDID from block
`start_block` and of size `blocks` will be placed in the memory
`edid` points to. The `edid` pointer must point to memory at least
`blocks` \* 128 bytes large (the size of one block is 128 bytes).

If there are fewer blocks than specified, then the driver will set
`blocks` to the actual number of blocks. If there are no EDID blocks
available at all, then the error code `ENODATA` is set.

If blocks have to be retrieved from the sink, then this call will block
until they have been read.

If `start_block` and `blocks` are both set to 0 when
[VIDIOC\_G\_EDID](#vidioc-g-edid) is called, then the driver will set `blocks` to the
total number of available EDID blocks and it will return 0 without
copying any data. This is an easy way to discover how many EDID blocks
there are.

Note

If there are no EDID blocks available at all, then
the driver will set `blocks` to 0 and it returns 0.

To set the EDID blocks of a receiver the application has to fill in the
`pad`, `blocks` and `edid` fields, set `start_block` to 0 and
zero the `reserved` array. It is not possible to set part of an EDID,
it is always all or nothing. Setting the EDID data is only valid for
receivers as it makes no sense for a transmitter.

The driver assumes that the full EDID is passed in. If there are more
EDID blocks than the hardware can handle then the EDID is not written,
but instead the error code `E2BIG` is set and `blocks` is set to the
maximum that the hardware supports. If `start_block` is any value
other than 0 then the error code `EINVAL` is set.

To disable an EDID you set `blocks` to 0. Depending on the hardware
this will drive the hotplug pin low and/or block the source from reading
the EDID data in some way. In any case, the end result is the same: the
EDID is no longer available.

type v4l2\_edid

struct v4l2\_edid

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `pad` | Pad for which to get/set the EDID blocks. When used with a video device node the pad represents the input or output index as returned by [ioctl VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput) and [ioctl VIDIOC\_ENUMOUTPUT](vidioc-enumoutput.html#vidioc-enumoutput) respectively. |
| \_\_u32 | `start_block` | Read the EDID from starting with this block. Must be 0 when setting the EDID. |
| \_\_u32 | `blocks` | The number of blocks to get or set. Must be less or equal to 256 (the maximum number of blocks as defined by the standard). When you set the EDID and `blocks` is 0, then the EDID is disabled or erased. |
| \_\_u32 | `reserved`[5] | Reserved for future extensions. Applications and drivers must set the array to zero. |
| \_\_u8 \* | `edid` | Pointer to memory that contains the EDID. The minimum size is `blocks` \* 128. |

## 7.27.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

`ENODATA`
:   The EDID data is not available.

`E2BIG`
:   The EDID data you provided is more than the hardware can handle.
