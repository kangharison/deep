# 7.62.ioctl VIDIOC_SUBDEV_G_ROUTING, VIDIOC_SUBDEV_S_ROUTING

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-g-routing.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.62. ioctl VIDIOC\_SUBDEV\_G\_ROUTING, VIDIOC\_SUBDEV\_S\_ROUTING

## 7.62.1. Name

VIDIOC\_SUBDEV\_G\_ROUTING - VIDIOC\_SUBDEV\_S\_ROUTING - Get or set routing between streams of media pads in a media entity.

## 7.62.2. Synopsis

VIDIOC\_SUBDEV\_G\_ROUTING

`int ioctl(int fd, VIDIOC_SUBDEV_G_ROUTING, struct v4l2_subdev_routing *argp)`

VIDIOC\_SUBDEV\_S\_ROUTING

`int ioctl(int fd, VIDIOC_SUBDEV_S_ROUTING, struct v4l2_subdev_routing *argp)`

## 7.62.3. Arguments

`fd`
:   File descriptor returned by [open()](func-open.html#func-open).

`argp`
:   Pointer to struct [`v4l2_subdev_routing`](#c.V4L.v4l2_subdev_routing "v4l2_subdev_routing").

## 7.62.4. Description

These ioctls are used to get and set the routing in a media entity.
The routing configuration determines the flows of data inside an entity.

Drivers report their current routing tables using the
`VIDIOC_SUBDEV_G_ROUTING` ioctl and application may enable or disable routes
with the `VIDIOC_SUBDEV_S_ROUTING` ioctl, by adding or removing routes and
setting or clearing flags of the `flags` field of a struct
[`v4l2_subdev_route`](#c.V4L.v4l2_subdev_route "v4l2_subdev_route"). Similarly to `VIDIOC_SUBDEV_G_ROUTING`, also
`VIDIOC_SUBDEV_S_ROUTING` returns the routes back to the user.

All stream configurations are reset when `VIDIOC_SUBDEV_S_ROUTING` is called.
This means that the userspace must reconfigure all stream formats and selections
after calling the ioctl with e.g. `VIDIOC_SUBDEV_S_FMT`.

Only subdevices which have both sink and source pads can support routing.

The `len_routes` field indicates the number of routes that can fit in the
`routes` array allocated by userspace. It is set by applications for both
ioctls to indicate how many routes the kernel can return, and is never modified
by the kernel.

The `num_routes` field indicates the number of routes in the routing
table. For `VIDIOC_SUBDEV_S_ROUTING`, it is set by userspace to the number of
routes that the application stored in the `routes` array. For both ioctls, it
is returned by the kernel and indicates how many routes are stored in the
subdevice routing table. This may be smaller or larger than the value of
`num_routes` set by the application for `VIDIOC_SUBDEV_S_ROUTING`, as
drivers may adjust the requested routing table.

The kernel can return a `num_routes` value larger than `len_routes` from
both ioctls. This indicates thare are more routes in the routing table than fits
the `routes` array. In this case, the `routes` array is filled by the kernel
with the first `len_routes` entries of the subdevice routing table. This is
not considered to be an error, and the ioctl call succeeds. If the applications
wants to retrieve the missing routes, it can issue a new
`VIDIOC_SUBDEV_G_ROUTING` call with a large enough `routes` array.

`VIDIOC_SUBDEV_S_ROUTING` may return more routes than the user provided in
`num_routes` field due to e.g. hardware properties.

type v4l2\_subdev\_routing

struct v4l2\_subdev\_routing

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `which` | Routing table to be accessed, from enum [v4l2\_subdev\_format\_whence](vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence). |
| \_\_u32 | `len_routes` | The length of the array (as in memory reserved for the array) |
| struct [`v4l2_subdev_route`](#c.V4L.v4l2_subdev_route "v4l2_subdev_route") | `routes[]` | Array of struct [`v4l2_subdev_route`](#c.V4L.v4l2_subdev_route "v4l2_subdev_route") entries |
| \_\_u32 | `num_routes` | Number of entries of the routes array |
| \_\_u32 | `reserved`[11] | Reserved for future extensions. Applications and drivers must set the array to zero. |

type v4l2\_subdev\_route

struct v4l2\_subdev\_route

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `sink_pad` | Sink pad number. |
| \_\_u32 | `sink_stream` | Sink pad stream number. |
| \_\_u32 | `source_pad` | Source pad number. |
| \_\_u32 | `source_stream` | Source pad stream number. |
| \_\_u32 | `flags` | Route enable/disable flags [v4l2\_subdev\_routing\_flags](#v4l2-subdev-routing-flags). |
| \_\_u32 | `reserved`[5] | Reserved for future extensions. Applications and drivers must set the array to zero. |

enum v4l2\_subdev\_routing\_flags

|  |  |  |
| --- | --- | --- |
| V4L2\_SUBDEV\_ROUTE\_FL\_ACTIVE | 0x0001 | The route is enabled. Set by applications. |

## 7.62.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The sink or source pad identifiers reference a non-existing pad or reference
    pads of different types (ie. the sink\_pad identifiers refers to a source
    pad), the `which` field has an unsupported value, or, for
    `VIDIOC_SUBDEV_S_ROUTING`, the num\_routes field set by the application is
    larger than the len\_routes field value.

ENXIO
:   The application requested routes cannot be created or the state of
    the specified routes cannot be modified. Only returned for
    `VIDIOC_SUBDEV_S_ROUTING`.

E2BIG
:   The application provided `num_routes` for `VIDIOC_SUBDEV_S_ROUTING` is
    larger than the number of routes the driver can handle.
