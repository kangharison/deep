# 7.65.ioctl VIDIOC_SUBDEV_QUERYCAP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-querycap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.65. ioctl VIDIOC\_SUBDEV\_QUERYCAP

## 7.65.1. Name

VIDIOC\_SUBDEV\_QUERYCAP - Query sub-device capabilities

## 7.65.2. Synopsis

VIDIOC\_SUBDEV\_QUERYCAP

`int ioctl(int fd, VIDIOC_SUBDEV_QUERYCAP, struct v4l2_subdev_capability *argp)`

## 7.65.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_subdev_capability`](#c.V4L.v4l2_subdev_capability "v4l2_subdev_capability").

## 7.65.4. Description

All V4L2 sub-devices support the `VIDIOC_SUBDEV_QUERYCAP` ioctl. It is used to
identify kernel devices compatible with this specification and to obtain
information about driver and hardware capabilities. The ioctl takes a pointer to
a struct [`v4l2_subdev_capability`](#c.V4L.v4l2_subdev_capability "v4l2_subdev_capability") which is filled by the driver. When
the driver is not compatible with this specification the ioctl returns
`ENOTTY` error code.

type v4l2\_subdev\_capability

struct v4l2\_subdev\_capability

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `version` | Version number of the driver.  The version reported is provided by the V4L2 subsystem following the kernel numbering scheme. However, it may not always return the same version as the kernel if, for example, a stable or distribution-modified kernel uses the V4L2 stack from a newer kernel.  The version number is formatted using the `KERNEL_VERSION()` macro: |
| `#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))`  `__u32 version = KERNEL_VERSION(0, 8, 1);`  `printf ("Version: %u.%u.%u\\n",`  `(version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);` | | |
| \_\_u32 | `capabilities` | Sub-device capabilities of the opened device, see [Sub-Device Capabilities Flags](#subdevice-capabilities). |
| \_\_u32 | `reserved`[14] | Reserved for future extensions. Set to 0 by the V4L2 core. |

Sub-Device Capabilities Flags

|  |  |  |
| --- | --- | --- |
| V4L2\_SUBDEV\_CAP\_RO\_SUBDEV | 0x00000001 | The sub-device device node is registered in read-only mode. Access to the sub-device ioctls that modify the device state is restricted. Refer to each individual subdevice ioctl documentation for a description of which restrictions apply to a read-only sub-device. |

## 7.65.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

ENOTTY
:   The device node is not a V4L2 sub-device.
