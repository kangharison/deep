# 7.63.ioctl VIDIOC_SUBDEV_G_SELECTION, VIDIOC_SUBDEV_S_SELECTION

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-subdev-g-selection.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.63. ioctl VIDIOC\_SUBDEV\_G\_SELECTION, VIDIOC\_SUBDEV\_S\_SELECTION

## 7.63.1. Name

VIDIOC\_SUBDEV\_G\_SELECTION - VIDIOC\_SUBDEV\_S\_SELECTION - Get or set selection rectangles on a subdev pad

## 7.63.2. Synopsis

VIDIOC\_SUBDEV\_G\_SELECTION

`int ioctl(int fd, VIDIOC_SUBDEV_G_SELECTION, struct v4l2_subdev_selection *argp)`

VIDIOC\_SUBDEV\_S\_SELECTION

`int ioctl(int fd, VIDIOC_SUBDEV_S_SELECTION, struct v4l2_subdev_selection *argp)`

## 7.63.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_subdev_selection`](#c.V4L.v4l2_subdev_selection "v4l2_subdev_selection").

## 7.63.4. Description

The selections are used to configure various image processing
functionality performed by the subdevs which affect the image size. This
currently includes cropping, scaling and composition.

The selection API replaces
[the old subdev crop API](vidioc-subdev-g-crop.html#vidioc-subdev-g-crop). All the
function of the crop API, and more, are supported by the selections API.

See [Sub-device Interface](dev-subdev.html#subdev) for more information on how each selection target
affects the image processing pipeline inside the subdevice.

If the subdev device node has been registered in read-only mode, calls to
`VIDIOC_SUBDEV_S_SELECTION` are only valid if the `which` field is set to
`V4L2_SUBDEV_FORMAT_TRY`, otherwise an error is returned and the errno
variable is set to `-EPERM`.

### 7.63.4.1. Types of selection targets

There are two types of selection targets: actual and bounds. The actual
targets are the targets which configure the hardware. The BOUNDS target
will return a rectangle that contain all possible actual rectangles.

### 7.63.4.2. Discovering supported features

To discover which targets are supported, the user can perform
`VIDIOC_SUBDEV_G_SELECTION` on them. Any unsupported target will
return `EINVAL`.

Selection targets and flags are documented in
[Common selection definitions](selections-common.html#v4l2-selections-common).

type v4l2\_subdev\_selection

struct v4l2\_subdev\_selection

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `which` | Active or try selection, from enum [v4l2\_subdev\_format\_whence](vidioc-subdev-g-fmt.html#v4l2-subdev-format-whence). |
| \_\_u32 | `pad` | Pad number as reported by the media framework. |
| \_\_u32 | `target` | Target selection rectangle. See [Common selection definitions](selections-common.html#v4l2-selections-common). |
| \_\_u32 | `flags` | Flags. See [Selection flags](v4l2-selection-flags.html#v4l2-selection-flags). |
| struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") | `r` | Selection rectangle, in pixels. |
| \_\_u32 | `stream` | Stream identifier. |
| \_\_u32 | `reserved`[7] | Reserved for future extensions. Applications and drivers must set the array to zero. |

## 7.63.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EBUSY
:   The selection rectangle can’t be changed because the pad is
    currently busy. This can be caused, for instance, by an active video
    stream on the pad. The ioctl must not be retried without performing
    another action to fix the problem first. Only returned by
    `VIDIOC_SUBDEV_S_SELECTION`

EINVAL
:   The struct [`v4l2_subdev_selection`](#c.V4L.v4l2_subdev_selection "v4l2_subdev_selection") `pad` references a
    non-existing pad, the `which` field has an unsupported value, or the
    selection target is not supported on the given subdev pad.

EPERM
:   The `VIDIOC_SUBDEV_S_SELECTION` ioctl has been called on a read-only
    subdevice and the `which` field is set to `V4L2_SUBDEV_FORMAT_ACTIVE`.
