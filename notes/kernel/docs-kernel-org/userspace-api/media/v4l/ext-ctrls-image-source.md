# 1.13.Image Source Control Reference

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/ext-ctrls-image-source.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.13. Image Source Control Reference

The Image Source control class is intended for low-level control of
image source devices such as image sensors. The devices feature an
analogue to digital converter and a bus transmitter to transmit the
image data out of the device.

## 1.13.1. Image Source Control IDs

`V4L2_CID_IMAGE_SOURCE_CLASS (class)`
:   The IMAGE\_SOURCE class descriptor.

`V4L2_CID_VBLANK (integer)`
:   Vertical blanking. The idle period after every frame during which no
    image data is produced. The unit of vertical blanking is a line.
    Every line has length of the image width plus horizontal blanking at
    the pixel rate defined by `V4L2_CID_PIXEL_RATE` control in the
    same sub-device.

`V4L2_CID_HBLANK (integer)`
:   Horizontal blanking. The idle period after every line of image data
    during which no image data is produced. The unit of horizontal
    blanking is pixels.

`V4L2_CID_ANALOGUE_GAIN (integer)`
:   Analogue gain is gain affecting all colour components in the pixel
    matrix. The gain operation is performed in the analogue domain
    before A/D conversion.

`V4L2_CID_TEST_PATTERN_RED (integer)`
:   Test pattern red colour component.

`V4L2_CID_TEST_PATTERN_GREENR (integer)`
:   Test pattern green (next to red) colour component.

`V4L2_CID_TEST_PATTERN_BLUE (integer)`
:   Test pattern blue colour component.

`V4L2_CID_TEST_PATTERN_GREENB (integer)`
:   Test pattern green (next to blue) colour component.

`V4L2_CID_UNIT_CELL_SIZE (struct)`
:   This control returns the unit cell size in nanometers. The struct
    [`v4l2_area`](#c.V4L.v4l2_area "v4l2_area") provides the width and the height in separate
    fields to take into consideration asymmetric pixels.
    This control does not take into consideration any possible hardware
    binning.
    The unit cell consists of the whole area of the pixel, sensitive and
    non-sensitive.
    This control is required for automatic calibration of sensors/cameras.

type v4l2\_area

struct v4l2\_area

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `width` | Width of the area. |
| \_\_u32 | `height` | Height of the area. |

`V4L2_CID_NOTIFY_GAINS (integer array)`
:   The sensor is notified what gains will be applied to the different
    colour channels by subsequent processing (such as by an ISP). The
    sensor is merely informed of these values in case it performs
    processing that requires them, but it does not apply them itself to
    the output pixels.

    Currently it is defined only for Bayer sensors, and is an array
    control taking 4 gain values, being the gains for each of the
    Bayer channels. The gains are always in the order B, Gb, Gr and R,
    irrespective of the exact Bayer order of the sensor itself.

    The use of an array allows this control to be extended to sensors
    with, for example, non-Bayer CFAs (colour filter arrays).

    The units for the gain values are linear, with the default value
    representing a gain of exactly 1.0. For example, if this default value
    is reported as being (say) 128, then a value of 192 would represent
    a gain of exactly 1.5.
