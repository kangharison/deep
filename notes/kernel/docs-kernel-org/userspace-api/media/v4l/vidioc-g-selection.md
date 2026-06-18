# 7.39.ioctl VIDIOC_G_SELECTION, VIDIOC_S_SELECTION

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-g-selection.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.39. ioctl VIDIOC\_G\_SELECTION, VIDIOC\_S\_SELECTION

## 7.39.1. Name

VIDIOC\_G\_SELECTION - VIDIOC\_S\_SELECTION - Get or set one of the selection rectangles

## 7.39.2. Synopsis

VIDIOC\_G\_SELECTION

`int ioctl(int fd, VIDIOC_G_SELECTION, struct v4l2_selection *argp)`

VIDIOC\_S\_SELECTION

`int ioctl(int fd, VIDIOC_S_SELECTION, struct v4l2_selection *argp)`

## 7.39.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection").

## 7.39.4. Description

The ioctls are used to query and configure selection rectangles.

To query the cropping (composing) rectangle set struct
[`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `type` field to the
respective buffer type. The next step is setting the
value of struct [`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `target`
field to `V4L2_SEL_TGT_CROP` (`V4L2_SEL_TGT_COMPOSE`). Please refer
to table [Common selection definitions](selections-common.html#v4l2-selections-common) or [Cropping, composing and scaling -- the SELECTION API](selection-api.html#selection-api) for
additional targets. The `flags` and `reserved` fields of struct
[`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") are ignored and they must be
filled with zeros. The driver fills the rest of the structure or returns
EINVAL error code if incorrect buffer type or target was used. If
cropping (composing) is not supported then the active rectangle is not
mutable and it is always equal to the bounds rectangle. Finally, the
struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") `r` rectangle is filled with
the current cropping (composing) coordinates. The coordinates are
expressed in driver-dependent units. The only exception are rectangles
for images in raw formats, whose coordinates are always expressed in
pixels.

To change the cropping (composing) rectangle set the struct
[`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `type` field to the
respective buffer type. The next step is setting the
value of struct [`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `target` to
`V4L2_SEL_TGT_CROP` (`V4L2_SEL_TGT_COMPOSE`). Please refer to table
[Common selection definitions](selections-common.html#v4l2-selections-common) or [Cropping, composing and scaling -- the SELECTION API](selection-api.html#selection-api) for additional
targets. The struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") `r` rectangle need
to be set to the desired active area. Field struct
[`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `reserved` is ignored and
must be filled with zeros. The driver may adjust coordinates of the
requested rectangle. An application may introduce constraints to control
rounding behaviour. The struct [`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection")
`flags` field must be set to one of the following:

* `0` - The driver can adjust the rectangle size freely and shall
  choose a crop/compose rectangle as close as possible to the requested
  one.
* `V4L2_SEL_FLAG_GE` - The driver is not allowed to shrink the
  rectangle. The original rectangle must lay inside the adjusted one.
* `V4L2_SEL_FLAG_LE` - The driver is not allowed to enlarge the
  rectangle. The adjusted rectangle must lay inside the original one.
* `V4L2_SEL_FLAG_GE | V4L2_SEL_FLAG_LE` - The driver must choose the
  size exactly the same as in the requested rectangle.

Please refer to [Size adjustments with constraint flags.](#sel-const-adjust).

The driver may have to adjusts the requested dimensions against hardware
limits and other parts as the pipeline, i.e. the bounds given by the
capture/output window or TV display. The closest possible values of
horizontal and vertical offset and sizes are chosen according to
following priority:

1. Satisfy constraints from struct
   [`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `flags`.
2. Adjust width, height, left, and top to hardware limits and
   alignments.
3. Keep center of adjusted rectangle as close as possible to the
   original one.
4. Keep width and height as close as possible to original ones.
5. Keep horizontal and vertical offset as close as possible to original
   ones.

On success the struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") `r` field
contains the adjusted rectangle. When the parameters are unsuitable the
application may modify the cropping (composing) or image parameters and
repeat the cycle until satisfactory parameters have been negotiated. If
constraints flags have to be violated at then `ERANGE` is returned. The
error indicates that *there exist no rectangle* that satisfies the
constraints.

Selection targets and flags are documented in
[Common selection definitions](selections-common.html#v4l2-selections-common).

![constraints.svg](../../../_images/constraints.svg)

Size adjustments with constraint flags.

Behaviour of rectangle adjustment for different constraint flags.

type v4l2\_selection

struct v4l2\_selection

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `type` | Type of the buffer (from enum [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "v4l2_buf_type")). |
| \_\_u32 | `target` | Used to select between [cropping and composing rectangles](selections-common.html#v4l2-selections-common). |
| \_\_u32 | `flags` | Flags controlling the selection rectangle adjustments, refer to [selection flags](v4l2-selection-flags.html#v4l2-selection-flags). |
| struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect") | `r` | The selection rectangle. |
| \_\_u32 | `reserved[9]` | Reserved fields for future use. Drivers and applications must zero this array. |

Note

Unfortunately in the case of multiplanar buffer types
(`V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE` and `V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE`)
this API was messed up with regards to how the [`v4l2_selection`](#c.V4L.v4l2_selection "v4l2_selection") `type` field
should be filled in. Some drivers only accepted the `_MPLANE` buffer type while
other drivers only accepted a non-multiplanar buffer type (i.e. without the
`_MPLANE` at the end).

Starting with kernel 4.13 both variations are allowed.

## 7.39.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   Given buffer type `type` or the selection target `target` is not
    supported, or the `flags` argument is not valid.

ERANGE
:   It is not possible to adjust struct [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "v4l2_rect")
    `r` rectangle to satisfy all constraints given in the `flags`
    argument.

ENODATA
:   Selection is not supported for this input or output.

EBUSY
:   It is not possible to apply change of the selection rectangle at the
    moment. Usually because streaming is in progress.
