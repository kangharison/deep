# 7.64.ioctl VIDIOC_SUBDEV_G_CLIENT_CAP, VIDIOC_SUBDEV_S_CLIENT_CAP

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-g-client-cap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.64. ioctl VIDIOC\_SUBDEV\_G\_CLIENT\_CAP, VIDIOC\_SUBDEV\_S\_CLIENT\_CAP

## 7.64.1. Name

VIDIOC\_SUBDEV\_G\_CLIENT\_CAP - VIDIOC\_SUBDEV\_S\_CLIENT\_CAP - Get or set client
capabilities.

## 7.64.2. Synopsis

VIDIOC\_SUBDEV\_G\_CLIENT\_CAP

`int ioctl(int fd, VIDIOC_SUBDEV_G_CLIENT_CAP, struct v4l2_subdev_client_capability *argp)`

VIDIOC\_SUBDEV\_S\_CLIENT\_CAP

`int ioctl(int fd, VIDIOC_SUBDEV_S_CLIENT_CAP, struct v4l2_subdev_client_capability *argp)`

## 7.64.3. Arguments

`fd`
:   File descriptor returned by [open()](func-open.html#func-open).

`argp`
:   Pointer to struct [`v4l2_subdev_client_capability`](#c.V4L.v4l2_subdev_client_capability "v4l2_subdev_client_capability").

## 7.64.4. Description

These ioctls are used to get and set the client (the application using the
subdevice ioctls) capabilities. The client capabilities are stored in the file
handle of the opened subdev device node, and the client must set the
capabilities for each opened subdev separately.

By default no client capabilities are set when a subdev device node is opened.

The purpose of the client capabilities are to inform the kernel of the behavior
of the client, mainly related to maintaining compatibility with different
kernel and userspace versions.

The `VIDIOC_SUBDEV_G_CLIENT_CAP` ioctl returns the current client capabilities
associated with the file handle `fd`.

The `VIDIOC_SUBDEV_S_CLIENT_CAP` ioctl sets client capabilities for the file
handle `fd`. The new capabilities fully replace the current capabilities, the
ioctl can therefore also be used to remove capabilities that have previously
been set.

`VIDIOC_SUBDEV_S_CLIENT_CAP` modifies the struct
[`v4l2_subdev_client_capability`](#c.V4L.v4l2_subdev_client_capability "v4l2_subdev_client_capability") to reflect the capabilities that have
been accepted. A common case for the kernel not accepting a capability is that
the kernel is older than the headers the userspace uses, and thus the capability
is unknown to the kernel.

type v4l2\_subdev\_client\_capability

struct v4l2\_subdev\_client\_capability

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `capabilities` | Sub-device client capabilities of the opened device. |

Client Capabilities

| Capability | Description |
| --- | --- |
| `V4L2_SUBDEV_CLIENT_CAP_STREAMS` | The client is aware of streams. Setting this flag enables the use of ‘stream’ fields (referring to the stream number) with various ioctls. If this is not set (which is the default), the ‘stream’ fields will be forced to 0 by the kernel. |
| `V4L2_SUBDEV_CLIENT_CAP_INTERVAL_USES_WHICH` | The client is aware of the [`v4l2_subdev_frame_interval`](vidioc-subdev-g-frame-interval.html#c.V4L.v4l2_subdev_frame_interval "v4l2_subdev_frame_interval") `which` field. If this is not set (which is the default), the `which` field is forced to `V4L2_SUBDEV_FORMAT_ACTIVE` by the kernel. |

## 7.64.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

ENOIOCTLCMD
:   The kernel does not support this ioctl.
