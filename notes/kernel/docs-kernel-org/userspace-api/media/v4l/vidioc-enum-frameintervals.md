# 7.16.ioctl VIDIOC_ENUM_FRAMEINTERVALS

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-enum-frameintervals.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.16. ioctl VIDIOC\_ENUM\_FRAMEINTERVALS

## 7.16.1. Name

VIDIOC\_ENUM\_FRAMEINTERVALS - Enumerate frame intervals

## 7.16.2. Synopsis

VIDIOC\_ENUM\_FRAMEINTERVALS

`int ioctl(int fd, VIDIOC_ENUM_FRAMEINTERVALS, struct v4l2_frmivalenum *argp)`

## 7.16.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_frmivalenum`](#c.V4L.v4l2_frmivalenum "v4l2_frmivalenum")
    that contains a pixel format and size and receives a frame interval.

## 7.16.4. Description

This ioctl allows applications to enumerate all frame intervals that the
device supports for the given pixel format and frame size.

The supported pixel formats and frame sizes can be obtained by using the
[ioctl VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt) and
[ioctl VIDIOC\_ENUM\_FRAMESIZES](vidioc-enum-framesizes.html#vidioc-enum-framesizes) functions.

The return value and the content of the `v4l2_frmivalenum.type` field
depend on the type of frame intervals the device supports. Here are the
semantics of the function for the different cases:

* **Discrete:** The function returns success if the given index value
  (zero-based) is valid. The application should increase the index by
  one for each call until `EINVAL` is returned. The
  v4l2\_frmivalenum.type field is set to
  V4L2\_FRMIVAL\_TYPE\_DISCRETE by the driver. Of the `union only`
  the discrete member is valid.
* **Step-wise:** The function returns success if the given index value
  is zero and `EINVAL` for any other index value. The
  `v4l2_frmivalenum.type` field is set to
  `V4L2_FRMIVAL_TYPE_STEPWISE` by the driver. Of the `union only` the
  `stepwise` member is valid.
* **Continuous:** This is a special case of the step-wise type above.
  The function returns success if the given index value is zero and
  `EINVAL` for any other index value. The `v4l2_frmivalenum.type`
  field is set to `V4L2_FRMIVAL_TYPE_CONTINUOUS` by the driver. Of
  the `union only` the `stepwise` member is valid and the `step`
  value is set to 1.

When the application calls the function with index zero, it must check
the `type` field to determine the type of frame interval enumeration
the device supports. Only for the `V4L2_FRMIVAL_TYPE_DISCRETE` type
does it make sense to increase the index value to receive more frame
intervals.

Note

The order in which the frame intervals are returned has no
special meaning. In particular does it not say anything about potential
default frame intervals.

Applications can assume that the enumeration data does not change
without any interaction from the application itself. This means that the
enumeration data is consistent if the application does not perform any
other ioctl calls while it runs the frame interval enumeration.

Note

**Frame intervals and frame rates:** The V4L2 API uses frame
intervals instead of frame rates. Given the frame interval the frame
rate can be computed as follows:

```
frame_rate = 1 / frame_interval
```

## 7.16.5. Structs

In the structs below, *IN* denotes a value that has to be filled in by
the application, *OUT* denotes values that the driver fills in. The
application should zero out all members except for the *IN* fields.

type v4l2\_frmival\_stepwise

struct v4l2\_frmival\_stepwise

|  |  |  |
| --- | --- | --- |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `min` | Minimum frame interval [s]. |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `max` | Maximum frame interval [s]. |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `step` | Frame interval step size [s]. |

type v4l2\_frmivalenum

struct v4l2\_frmivalenum

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | IN: Index of the given frame interval in the enumeration. |
| \_\_u32 | `pixel_format` | IN: Pixel format for which the frame intervals are enumerated. |
| \_\_u32 | `width` | IN: Frame width for which the frame intervals are enumerated. |
| \_\_u32 | `height` | IN: Frame height for which the frame intervals are enumerated. |
| \_\_u32 | `type` | OUT: Frame interval type the device supports. |
| union { | (anonymous) | OUT: Frame interval with the given index. |
| struct [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "v4l2_fract") | `discrete` | Frame interval [s]. |
| struct [`v4l2_frmival_stepwise`](#c.V4L.v4l2_frmival_stepwise "v4l2_frmival_stepwise") | `stepwise` |  |
| } |  |  |
| \_\_u32 | `reserved[2]` | Reserved space for future use. Must be zeroed by drivers and applications. |

## 7.16.6. Enums

type v4l2\_frmivaltypes

enum v4l2\_frmivaltypes

|  |  |  |
| --- | --- | --- |
| `V4L2_FRMIVAL_TYPE_DISCRETE` | 1 | Discrete frame interval. |
| `V4L2_FRMIVAL_TYPE_CONTINUOUS` | 2 | Continuous frame interval. |
| `V4L2_FRMIVAL_TYPE_STEPWISE` | 3 | Step-wise defined frame interval. |

## 7.16.7. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
