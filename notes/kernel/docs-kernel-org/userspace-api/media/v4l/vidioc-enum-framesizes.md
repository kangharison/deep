# 7.15.ioctl VIDIOC_ENUM_FRAMESIZES

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-enum-framesizes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.15. ioctl VIDIOC\_ENUM\_FRAMESIZES

## 7.15.1. Name

VIDIOC\_ENUM\_FRAMESIZES - Enumerate frame sizes

## 7.15.2. Synopsis

VIDIOC\_ENUM\_FRAMESIZES

`int ioctl(int fd, VIDIOC_ENUM_FRAMESIZES, struct v4l2_frmsizeenum *argp)`

## 7.15.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_frmsizeenum`](#c.V4L.v4l2_frmsizeenum "v4l2_frmsizeenum")
    that contains an index and pixel format and receives a frame width
    and height.

## 7.15.4. Description

This ioctl allows applications to enumerate all frame sizes (i. e. width
and height in pixels) that the device supports for the given pixel
format.

The supported pixel formats can be obtained by using the
[ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt) function.

The return value and the content of the `v4l2_frmsizeenum.type` field
depend on the type of frame sizes the device supports. Here are the
semantics of the function for the different cases:

* **Discrete:** The function returns success if the given index value
  (zero-based) is valid. The application should increase the index by
  one for each call until `EINVAL` is returned. The
  `v4l2_frmsizeenum.type` field is set to
  `V4L2_FRMSIZE_TYPE_DISCRETE` by the driver. Of the `union only` the
  `discrete` member is valid.
* **Step-wise:** The function returns success if the given index value
  is zero and `EINVAL` for any other index value. The
  `v4l2_frmsizeenum.type` field is set to
  `V4L2_FRMSIZE_TYPE_STEPWISE` by the driver. Of the `union only` the
  `stepwise` member is valid.
* **Continuous:** This is a special case of the step-wise type above.
  The function returns success if the given index value is zero and
  `EINVAL` for any other index value. The `v4l2_frmsizeenum.type`
  field is set to `V4L2_FRMSIZE_TYPE_CONTINUOUS` by the driver. Of
  the `union only` the `stepwise` member is valid and the
  `step_width` and `step_height` values are set to 1.

When the application calls the function with index zero, it must check
the `type` field to determine the type of frame size enumeration the
device supports. Only for the `V4L2_FRMSIZE_TYPE_DISCRETE` type does
it make sense to increase the index value to receive more frame sizes.

Note

The order in which the frame sizes are returned has no special
meaning. In particular does it not say anything about potential default
format sizes.

Applications can assume that the enumeration data does not change
without any interaction from the application itself. This means that the
enumeration data is consistent if the application does not perform any
other ioctl calls while it runs the frame size enumeration.

## 7.15.5. Structs

In the structs below, *IN* denotes a value that has to be filled in by
the application, *OUT* denotes values that the driver fills in. The
application should zero out all members except for the *IN* fields.

type v4l2\_frmsize\_discrete

struct v4l2\_frmsize\_discrete

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `width` | Width of the frame [pixel]. |
| \_\_u32 | `height` | Height of the frame [pixel]. |

type v4l2\_frmsize\_stepwise

struct v4l2\_frmsize\_stepwise

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `min_width` | Minimum frame width [pixel]. |
| \_\_u32 | `max_width` | Maximum frame width [pixel]. |
| \_\_u32 | `step_width` | Frame width step size [pixel]. |
| \_\_u32 | `min_height` | Minimum frame height [pixel]. |
| \_\_u32 | `max_height` | Maximum frame height [pixel]. |
| \_\_u32 | `step_height` | Frame height step size [pixel]. |

type v4l2\_frmsizeenum

struct v4l2\_frmsizeenum

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | IN: Index of the given frame size in the enumeration. |
| \_\_u32 | `pixel_format` | IN: Pixel format for which the frame sizes are enumerated. |
| \_\_u32 | `type` | OUT: Frame size type the device supports. |
| union { | (anonymous) | OUT: Frame size with the given index. |
| struct [`v4l2_frmsize_discrete`](#c.V4L.v4l2_frmsize_discrete "v4l2_frmsize_discrete") | `discrete` |  |
| struct [`v4l2_frmsize_stepwise`](#c.V4L.v4l2_frmsize_stepwise "v4l2_frmsize_stepwise") | `stepwise` |  |
| } |  |  |
| \_\_u32 | `reserved[2]` | Reserved space for future use. Must be zeroed by drivers and applications. |

## 7.15.6. Enums

type v4l2\_frmsizetypes

enum v4l2\_frmsizetypes

|  |  |  |
| --- | --- | --- |
| `V4L2_FRMSIZE_TYPE_DISCRETE` | 1 | Discrete frame size. |
| `V4L2_FRMSIZE_TYPE_CONTINUOUS` | 2 | Continuous frame size. |
| `V4L2_FRMSIZE_TYPE_STEPWISE` | 3 | Step-wise defined frame size. |

## 7.15.7. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
