# 4.13.3.4.1.Media Bus Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/subdev-formats.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.13.3.4.1. Media Bus Formats

type v4l2\_mbus\_framefmt

struct v4l2\_mbus\_framefmt

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `width` | Image width in pixels. |
| \_\_u32 | `height` | Image height in pixels. If `field` is one of `V4L2_FIELD_TOP`, `V4L2_FIELD_BOTTOM` or `V4L2_FIELD_ALTERNATE` then height refers to the number of lines in the field, otherwise it refers to the number of lines in the frame (which is twice the field height for interlaced formats). |
| \_\_u32 | `code` | Format code, from enum [v4l2\_mbus\_pixelcode](#v4l2-mbus-pixelcode). |
| \_\_u32 | `field` | Field order, from enum [`v4l2_field`](field-order.html#c.V4L.v4l2_field "v4l2_field"). See [Field Order](field-order.html#field-order) for details. Zero for metadata mbus codes. |
| \_\_u32 | `colorspace` | Image colorspace, from enum [`v4l2_colorspace`](colorspaces-defs.html#c.V4L.v4l2_colorspace "v4l2_colorspace"). Must be set by the driver for subdevices. If the application sets the flag `V4L2_MBUS_FRAMEFMT_SET_CSC` then the application can set this field on the source pad to request a specific colorspace for the media bus data. If the driver cannot handle the requested conversion, it will return another supported colorspace. The driver indicates that colorspace conversion is supported by setting the flag V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_COLORSPACE in the corresponding struct [`v4l2_subdev_mbus_code_enum`](vidioc-subdev-enum-mbus-code.html#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum") during enumeration. See [Subdev Media Bus Code Enumerate Flags](vidioc-subdev-enum-mbus-code.html#v4l2-subdev-mbus-code-flags). Zero for metadata mbus codes. |
| union { | (anonymous) | |
| \_\_u16 | `ycbcr_enc` | Y’CbCr encoding, from enum [`v4l2_ycbcr_encoding`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "v4l2_ycbcr_encoding"). This information supplements the `colorspace` and must be set by the driver for subdevices, see [Colorspaces](colorspaces.html#colorspaces). If the application sets the flag `V4L2_MBUS_FRAMEFMT_SET_CSC` then the application can set this field on a source pad to request a specific Y’CbCr encoding for the media bus data. If the driver cannot handle the requested conversion, it will return another supported encoding. This field is ignored for HSV media bus formats. The driver indicates that ycbcr\_enc conversion is supported by setting the flag V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_YCBCR\_ENC in the corresponding struct [`v4l2_subdev_mbus_code_enum`](vidioc-subdev-enum-mbus-code.html#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum") during enumeration. See [Subdev Media Bus Code Enumerate Flags](vidioc-subdev-enum-mbus-code.html#v4l2-subdev-mbus-code-flags). Zero for metadata mbus codes. |
| \_\_u16 | `hsv_enc` | HSV encoding, from enum [`v4l2_hsv_encoding`](colorspaces-defs.html#c.V4L.v4l2_hsv_encoding "v4l2_hsv_encoding"). This information supplements the `colorspace` and must be set by the driver for subdevices, see [Colorspaces](colorspaces.html#colorspaces). If the application sets the flag `V4L2_MBUS_FRAMEFMT_SET_CSC` then the application can set this field on a source pad to request a specific HSV encoding for the media bus data. If the driver cannot handle the requested conversion, it will return another supported encoding. This field is ignored for Y’CbCr media bus formats. The driver indicates that hsv\_enc conversion is supported by setting the flag V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_HSV\_ENC in the corresponding struct [`v4l2_subdev_mbus_code_enum`](vidioc-subdev-enum-mbus-code.html#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum") during enumeration. See [Subdev Media Bus Code Enumerate Flags](vidioc-subdev-enum-mbus-code.html#v4l2-subdev-mbus-code-flags). Zero for metadata mbus codes. |
| } |  | |
| \_\_u16 | `quantization` | Quantization range, from enum [`v4l2_quantization`](colorspaces-defs.html#c.V4L.v4l2_quantization "v4l2_quantization"). This information supplements the `colorspace` and must be set by the driver for subdevices, see [Colorspaces](colorspaces.html#colorspaces). If the application sets the flag `V4L2_MBUS_FRAMEFMT_SET_CSC` then the application can set this field on a source pad to request a specific quantization for the media bus data. If the driver cannot handle the requested conversion, it will return another supported quantization. The driver indicates that quantization conversion is supported by setting the flag V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_QUANTIZATION in the corresponding struct [`v4l2_subdev_mbus_code_enum`](vidioc-subdev-enum-mbus-code.html#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum") during enumeration. See [Subdev Media Bus Code Enumerate Flags](vidioc-subdev-enum-mbus-code.html#v4l2-subdev-mbus-code-flags). Zero for metadata mbus codes. |
| \_\_u16 | `xfer_func` | Transfer function, from enum [`v4l2_xfer_func`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "v4l2_xfer_func"). This information supplements the `colorspace` and must be set by the driver for subdevices, see [Colorspaces](colorspaces.html#colorspaces). If the application sets the flag `V4L2_MBUS_FRAMEFMT_SET_CSC` then the application can set this field on a source pad to request a specific transfer function for the media bus data. If the driver cannot handle the requested conversion, it will return another supported transfer function. The driver indicates that the transfer function conversion is supported by setting the flag V4L2\_SUBDEV\_MBUS\_CODE\_CSC\_XFER\_FUNC in the corresponding struct [`v4l2_subdev_mbus_code_enum`](vidioc-subdev-enum-mbus-code.html#c.V4L.v4l2_subdev_mbus_code_enum "v4l2_subdev_mbus_code_enum") during enumeration. See [Subdev Media Bus Code Enumerate Flags](vidioc-subdev-enum-mbus-code.html#v4l2-subdev-mbus-code-flags). Zero for metadata mbus codes. |
| \_\_u16 | `flags` | flags See: :ref:v4l2-mbus-framefmt-flags |
| \_\_u16 | `reserved`[10] | Reserved for future extensions. Applications and drivers must set the array to zero. |

v4l2\_mbus\_framefmt Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_MBUS_FRAMEFMT_SET_CSC` | 0x0001 | Set by the application. It is only used for source pads and is ignored for sink pads. If set, then request the subdevice to do colorspace conversion from the received colorspace to the requested colorspace values. If the colorimetry field (`colorspace`, `xfer_func`, `ycbcr_enc`, `hsv_enc` or `quantization`) is set to `*_DEFAULT`, then that colorimetry setting will remain unchanged from what was received. So in order to change the quantization, only the `quantization` field shall be set to non default value (`V4L2_QUANTIZATION_FULL_RANGE` or `V4L2_QUANTIZATION_LIM_RANGE`) and all other colorimetry fields shall be set to `*_DEFAULT`.  To check which conversions are supported by the hardware for the current media bus frame format, see [Subdev Media Bus Code Enumerate Flags](vidioc-subdev-enum-mbus-code.html#v4l2-subdev-mbus-code-flags). |

## 4.13.3.4.1.1. Media Bus Pixel Codes

The media bus pixel codes describe image formats as flowing over
physical buses (both between separate physical components and inside
SoC devices). This should not be confused with the V4L2 pixel formats
that describe, using four character codes, image formats as stored in
memory.

While there is a relationship between image formats on buses and image
formats in memory (a raw Bayer image won’t be magically converted to
JPEG just by storing it to memory), there is no one-to-one
correspondence between them.

While the media bus pixel codes are named based on how pixels are
transmitted on parallel buses, serial buses do not define separate
codes. By convention, they use the codes that transfer a sample on a
single clock cycle, and whose bit orders from LSB to MSB correspond to
the order in which colour components are transmitted on the serial bus.
For instance, the MIPI CSI-2 24-bit RGB (RGB888) format uses the
MEDIA\_BUS\_FMT\_RGB888\_1X24 media bus code because CSI-2 transmits the
blue colour component first, followed by green and red, and
MEDIA\_BUS\_FMT\_RGB888\_1X24 defines the first bit of blue at bit 0.
While used for 24-bit RGB data on parallel buses, the
MEDIA\_BUS\_FMT\_RGB888\_3X8 or MEDIA\_BUS\_FMT\_BGR888\_1X24 codes must not be
used for CSI-2.

### 4.13.3.4.1.1.1. Packed RGB Formats

Those formats transfer pixel data as red, green and blue components. The
format code is made of the following information.

* The red, green and blue components order code, as encoded in a pixel
  sample. Possible values are RGB and BGR.
* The number of bits per component, for each component. The values can
  be different for all components. Common values are 555 and 565.
* The number of bus samples per pixel. Pixels that are wider than the
  bus width must be transferred in multiple samples. Common values are
  1 and 2.
* The bus width.
* For formats where the total number of bits per pixel is smaller than
  the number of bus samples per pixel times the bus width, a padding
  value stating if the bytes are padded in their most high order bits
  (PADHI) or low order bits (PADLO). A “C” prefix is used for
  component-wise padding in the most high order bits (CPADHI) or low
  order bits (CPADLO) of each separate component.
* For formats where the number of bus samples per pixel is larger than
  1, an endianness value stating if the pixel is transferred MSB first
  (BE) or LSB first (LE).

For instance, a format where pixels are encoded as 5-bits red, 5-bits
green and 5-bit blue values padded on the high bit, transferred as 2
8-bit samples per pixel with the most significant bits (padding, red and
half of the green value) transferred first will be named
`MEDIA_BUS_FMT_RGB555_2X8_PADHI_BE`.

The following tables list existing packed RGB formats.

RGB formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_RGB444\_1X12 | 0x1016 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r3 | r2 | r1 | r0 | g3 | g2 | g1 | g0 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB444\_2X8\_PADHI\_BE | 0x1001 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | 0 | 0 | 0 | r3 | r2 | r1 | r0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g3 | g2 | g1 | g0 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB444\_2X8\_PADHI\_LE | 0x1002 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g3 | g2 | g1 | g0 | b3 | b2 | b1 | b0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | 0 | 0 | 0 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_RGB555\_2X8\_PADHI\_BE | 0x1003 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | r4 | r3 | r2 | r1 | r0 | g4 | g3 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB555\_2X8\_PADHI\_LE | 0x1004 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | r4 | r3 | r2 | r1 | r0 | g4 | g3 |
| MEDIA\_BUS\_FMT\_RGB565\_1X16 | 0x1017 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_BGR565\_2X8\_BE | 0x1005 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b4 | b3 | b2 | b1 | b0 | g5 | g4 | g3 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_BGR565\_2X8\_LE | 0x1006 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | r4 | r3 | r2 | r1 | r0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b4 | b3 | b2 | b1 | b0 | g5 | g4 | g3 |
| MEDIA\_BUS\_FMT\_RGB565\_2X8\_BE | 0x1007 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB565\_2X8\_LE | 0x1008 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 |
| MEDIA\_BUS\_FMT\_RGB666\_1X18 | 0x1009 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r5 | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 | g2 | g1 | g0 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB666\_2X9\_BE | 0x1025 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r5 | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g2 | g1 | g0 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_BGR666\_1X18 | 0x1023 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b5 | b4 | b3 | b2 | b1 | b0 | g5 | g4 | g3 | g2 | g1 | g0 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_RBG888\_1X24 | 0x100e |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_RGB666\_1X24\_CPADHI | 0x1015 |  |  |  |  |  |  |  |  |  | 0 | 0 | r5 | r4 | r3 | r2 | r1 | r0 | 0 | 0 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_BGR666\_1X24\_CPADHI | 0x1024 |  |  |  |  |  |  |  |  |  | 0 | 0 | b5 | b4 | b3 | b2 | b1 | b0 | 0 | 0 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_RGB565\_1X24\_CPADHI | 0x1022 |  |  |  |  |  |  |  |  |  | 0 | 0 | 0 | r4 | r3 | r2 | r1 | r0 | 0 | 0 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | 0 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_BGR888\_1X24 | 0x1013 |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_BGR888\_3X8 | 0x101b |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_GBR888\_1X24 | 0x1014 |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_RGB888\_1X24 | 0x100a |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB888\_2X12\_BE | 0x100b |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g7 | g6 | g5 | g4 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB888\_2X12\_LE | 0x100c |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g7 | g6 | g5 | g4 |
| MEDIA\_BUS\_FMT\_RGB888\_3X8 | 0x101c |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB666\_1X30-CPADLO | 0x101e |  |  |  | r5 | r4 | r3 | r2 | r1 | r0 | 0 | 0 | 0 | 0 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | 0 | 0 | b5 | b4 | b3 | b2 | b1 | b0 | 0 | 0 | 0 | 0 |
| MEDIA\_BUS\_FMT\_RGB888\_1X30-CPADLO | 0x101f |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | 0 | 0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | 0 | 0 |
| MEDIA\_BUS\_FMT\_ARGB888\_1X32 | 0x100d |  | a7 | a6 | a5 | a4 | a3 | a2 | a1 | a0 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB888\_1X32\_PADHI | 0x100f |  | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_RGB101010\_1X30 | 0x1018 |  |  |  | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |

The following table list existing packed 36bit wide RGB formats.

36bit RGB formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 35 | 34 | 33 | 32 | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_RGB666\_1X36\_CPADLO | 0x1020 |  | r5 | r4 | r3 | r2 | r1 | r0 | 0 | 0 | 0 | 0 | 0 | 0 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | 0 | 0 | 0 | 0 | b5 | b4 | b3 | b2 | b1 | b0 | 0 | 0 | 0 | 0 | 0 | 0 |
| MEDIA\_BUS\_FMT\_RGB888\_1X36\_CPADLO | 0x1021 |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | 0 | 0 | 0 | 0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | 0 | 0 | 0 | 0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | 0 | 0 | 0 | 0 |
| MEDIA\_BUS\_FMT\_RGB121212\_1X36 | 0x1019 |  | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |

The following table list existing packed 48bit wide RGB formats.

48bit RGB formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
|  |  |  | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_RGB161616\_1X48 | 0x101a |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | r15 | r14 | r13 | r12 | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
|  |  |  | g15 | g14 | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b15 | b14 | b13 | b12 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |

The following table list existing packed 60bit wide RGB formats.

60bit RGB formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit |  |  |  |  | 59 | 58 | 57 | 56 | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
|  |  |  | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_RGB202020\_1X60 | 0x1026 |  |  |  |  |  | r19 | r18 | r17 | r16 | r15 | r14 | r13 | r12 | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g19 | g18 | g17 | g16 | g15 | g14 | g13 | g12 |
|  |  |  | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b19 | b18 | b17 | b16 | b15 | b14 | b13 | b12 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |

On LVDS buses, usually each sample is transferred serialized in seven
time slots per pixel clock, on three (18-bit) or four (24-bit) or five (30-bit)
differential data pairs at the same time. The remaining bits are used
for control signals as defined by SPWG/PSWG/VESA or JEIDA standards. The
24-bit RGB format serialized in seven time slots on four lanes using
JEIDA defined bit mapping will be named
`MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA`, for example.

LVDS RGB formats

| Identifier | Code |  |  | Data organization | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Timeslot | Lane | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_RGB666\_1X7X3\_SPWG | 0x1010 | 0 |  |  |  | d | b1 | g0 |
|  |  | 1 |  |  |  | d | b0 | r5 |
|  |  | 2 |  |  |  | d | g5 | r4 |
|  |  | 3 |  |  |  | b5 | g4 | r3 |
|  |  | 4 |  |  |  | b4 | g3 | r2 |
|  |  | 5 |  |  |  | b3 | g2 | r1 |
|  |  | 6 |  |  |  | b2 | g1 | r0 |
| MEDIA\_BUS\_FMT\_RGB888\_1X7X4\_SPWG | 0x1011 | 0 |  |  | d | d | b1 | g0 |
|  |  | 1 |  |  | b7 | d | b0 | r5 |
|  |  | 2 |  |  | b6 | d | g5 | r4 |
|  |  | 3 |  |  | g7 | b5 | g4 | r3 |
|  |  | 4 |  |  | g6 | b4 | g3 | r2 |
|  |  | 5 |  |  | r7 | b3 | g2 | r1 |
|  |  | 6 |  |  | r6 | b2 | g1 | r0 |
| MEDIA\_BUS\_FMT\_RGB888\_1X7X4\_JEIDA | 0x1012 | 0 |  |  | d | d | b3 | g2 |
|  |  | 1 |  |  | b1 | d | b2 | r7 |
|  |  | 2 |  |  | b0 | d | g7 | r6 |
|  |  | 3 |  |  | g1 | b7 | g6 | r5 |
|  |  | 4 |  |  | g0 | b6 | g5 | r4 |
|  |  | 5 |  |  | r1 | b5 | g4 | r3 |
|  |  | 6 |  |  | r0 | b4 | g3 | r2 |
| MEDIA\_BUS\_FMT\_RGB101010\_1X7X5\_SPWG | 0x1026 | 0 |  | d | d | d | b1 | g0 |
|  |  | 1 |  | b9 | b7 | d | b0 | r5 |
|  |  | 2 |  | b8 | b6 | d | g5 | r4 |
|  |  | 3 |  | g9 | g7 | b5 | g4 | r3 |
|  |  | 4 |  | g8 | g6 | b4 | g3 | r2 |
|  |  | 5 |  | r9 | r7 | b3 | g2 | r1 |
|  |  | 6 |  | r8 | r6 | b2 | g1 | r0 |
| MEDIA\_BUS\_FMT\_RGB101010\_1X7X5\_JEIDA | 0x1027 | 0 |  | d | d | d | b5 | g4 |
|  |  | 1 |  | b1 | b3 | d | b4 | r9 |
|  |  | 2 |  | b0 | b2 | d | g9 | r8 |
|  |  | 3 |  | g1 | g3 | b9 | g8 | r7 |
|  |  | 4 |  | g0 | g2 | b8 | g7 | r6 |
|  |  | 5 |  | r1 | r3 | b7 | g6 | r5 |
|  |  | 6 |  | r0 | r2 | b6 | g5 | r4 |

### 4.13.3.4.1.1.2. Bayer Formats

Those formats transfer pixel data as red, green and blue components. The
format code is made of the following information.

* The red, green and blue components order code, as encoded in a pixel
  sample. The possible values are shown in [Bayer Patterns](#bayer-patterns).
* The number of bits per pixel component. All components are
  transferred on the same number of bits. Common values are 8, 10 and
  12.
* The compression (optional). If the pixel components are ALAW- or
  DPCM-compressed, a mention of the compression scheme and the number
  of bits per compressed pixel component.
* The number of bus samples per pixel. Pixels that are wider than the
  bus width must be transferred in multiple samples. Common values are
  1 and 2.
* The bus width.
* For formats where the total number of bits per pixel is smaller than
  the number of bus samples per pixel times the bus width, a padding
  value stating if the bytes are padded in their most high order bits
  (PADHI) or low order bits (PADLO).
* For formats where the number of bus samples per pixel is larger than
  1, an endianness value stating if the pixel is transferred MSB first
  (BE) or LSB first (LE).

For instance, a format with uncompressed 10-bit Bayer components
arranged in a red, green, green, blue pattern transferred as 2 8-bit
samples per pixel with the least significant bits transferred first will
be named `MEDIA_BUS_FMT_SRGGB10_2X8_PADHI_LE`.

![bayer.svg](../../../_images/bayer.svg)

Bayer Patterns

The following table lists existing packed Bayer formats. The data
organization is given as an example for the first pixel only.

Bayer Formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_SBGGR8\_1X8 | 0x3001 |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG8\_1X8 | 0x3013 |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG8\_1X8 | 0x3002 |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB8\_1X8 | 0x3014 |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR10\_ALAW8\_1X8 | 0x3015 |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG10\_ALAW8\_1X8 | 0x3016 |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG10\_ALAW8\_1X8 | 0x3017 |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB10\_ALAW8\_1X8 | 0x3018 |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR10\_DPCM8\_1X8 | 0x300b |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG10\_DPCM8\_1X8 | 0x300c |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG10\_DPCM8\_1X8 | 0x3009 |  |  |  |  |  |  |  |  |  |  |  |  |  | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB10\_DPCM8\_1X8 | 0x300d |  |  |  |  |  |  |  |  |  |  |  |  |  | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR10\_2X8\_PADHI\_BE | 0x3003 |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | 0 | 0 | 0 | 0 | 0 | b9 | b8 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SBGGR10\_2X8\_PADHI\_LE | 0x3004 |  |  |  |  |  |  |  |  |  |  |  |  |  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | 0 | 0 | 0 | 0 | 0 | b9 | b8 |
| MEDIA\_BUS\_FMT\_SBGGR10\_2X8\_PADLO\_BE | 0x3005 |  |  |  |  |  |  |  |  |  |  |  |  |  | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b1 | b0 | 0 | 0 | 0 | 0 | 0 | 0 |
| MEDIA\_BUS\_FMT\_SBGGR10\_2X8\_PADLO\_LE | 0x3006 |  |  |  |  |  |  |  |  |  |  |  |  |  | b1 | b0 | 0 | 0 | 0 | 0 | 0 | 0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 |
| MEDIA\_BUS\_FMT\_SBGGR10\_1X10 | 0x3007 |  |  |  |  |  |  |  |  |  |  |  | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG10\_1X10 | 0x300e |  |  |  |  |  |  |  |  |  |  |  | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG10\_1X10 | 0x300a |  |  |  |  |  |  |  |  |  |  |  | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB10\_1X10 | 0x300f |  |  |  |  |  |  |  |  |  |  |  | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR12\_1X12 | 0x3008 |  |  |  |  |  |  |  |  |  | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG12\_1X12 | 0x3010 |  |  |  |  |  |  |  |  |  | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG12\_1X12 | 0x3011 |  |  |  |  |  |  |  |  |  | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB12\_1X12 | 0x3012 |  |  |  |  |  |  |  |  |  | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR14\_1X14 | 0x3019 |  |  |  |  |  |  |  | b13 | b12 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG14\_1X14 | 0x301a |  |  |  |  |  |  |  | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG14\_1X14 | 0x301b |  |  |  |  |  |  |  | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB14\_1X14 | 0x301c |  |  |  |  |  |  |  | r13 | r12 | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR16\_1X16 | 0x301d |  |  |  |  |  | b15 | b14 | b13 | b12 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG16\_1X16 | 0x301e |  |  |  |  |  | g15 | g14 | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG16\_1X16 | 0x301f |  |  |  |  |  | g15 | g14 | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB16\_1X16 | 0x3020 |  |  |  |  |  | r15 | r14 | r13 | r12 | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |
| MEDIA\_BUS\_FMT\_SBGGR20\_1X20 | 0x3021 |  | b19 | b18 | b17 | b16 | b15 | b14 | b13 | b12 | b11 | b10 | b9 | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| MEDIA\_BUS\_FMT\_SGBRG20\_1X20 | 0x3022 |  | g19 | g18 | g17 | g16 | g15 | g14 | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SGRBG20\_1X20 | 0x3023 |  | g19 | g18 | g17 | g16 | g15 | g14 | g13 | g12 | g11 | g10 | g9 | g8 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 |
| MEDIA\_BUS\_FMT\_SRGGB20\_1X20 | 0x3024 |  | r19 | r18 | r17 | r16 | r15 | r14 | r13 | r12 | r11 | r10 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 |

### 4.13.3.4.1.1.3. Packed YUV Formats

Those data formats transfer pixel data as (possibly downsampled) Y, U
and V components. Some formats include dummy bits in some of their
samples and are collectively referred to as “YDYC” (Y-Dummy-Y-Chroma)
formats. One cannot rely on the values of these dummy bits as those are
undefined.

The format code is made of the following information.

* The Y, U and V components order code, as transferred on the bus.
  Possible values are YUYV, UYVY, YVYU and VYUY for formats with no
  dummy bit, and YDYUYDYV, YDYVYDYU, YUYDYVYD and YVYDYUYD for YDYC
  formats.
* The number of bits per pixel component. All components are
  transferred on the same number of bits. Common values are 8, 10 and
  12.
* The number of bus samples per pixel. Pixels that are wider than the
  bus width must be transferred in multiple samples. Common values are
  0.5 (encoded as 0\_5; in this case two pixels are transferred per bus
  sample), 1, 1.5 (encoded as 1\_5) and 2.
* The bus width. When the bus width is larger than the number of bits
  per pixel component, several components are packed in a single bus
  sample. The components are ordered as specified by the order code,
  with components on the left of the code transferred in the high order
  bits. Common values are 8 and 16.

For instance, a format where pixels are encoded as 8-bit YUV values
downsampled to 4:2:2 and transferred as 2 8-bit bus samples per pixel in
the U, Y, V, Y order will be named `MEDIA_BUS_FMT_UYVY8_2X8`.

[YUV Formats](#v4l2-mbus-pixelcode-yuv8) lists existing packed YUV formats and
describes the organization of each pixel data in each sample. When a
format pattern is split across multiple samples each of the samples in
the pattern is described.

The role of each bit transferred over the bus is identified by one of
the following codes.

* yx for luma component bit number x
* ux for blue chroma component bit number x
* vx for red chroma component bit number x
* ax for alpha component bit number x
* for non-available bits (for positions higher than the bus width)
* d for dummy bits

YUV Formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 10 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_Y8\_1X8 | 0x2001 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_UV8\_1X8 | 0x2015 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_UYVY8\_1\_5X8 | 0x2002 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY8\_1\_5X8 | 0x2003 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV8\_1\_5X8 | 0x2004 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU8\_1\_5X8 | 0x2005 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_UYVY8\_2X8 | 0x2006 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY8\_2X8 | 0x2007 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV8\_2X8 | 0x2008 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU8\_2X8 | 0x2009 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_Y10\_1X10 | 0x200a |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_Y10\_2X8\_PADHI\_LE | 0x202c |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 0 | 0 | 0 | 0 | 0 | 0 | y9 | y8 |
| MEDIA\_BUS\_FMT\_UYVY10\_2X10 | 0x2018 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY10\_2X10 | 0x2019 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV10\_2X10 | 0x200b |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU10\_2X10 | 0x200c |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_Y12\_1X12 | 0x2013 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_UYVY12\_2X12 | 0x201c |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY12\_2X12 | 0x201d |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV12\_2X12 | 0x201e |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU12\_2X12 | 0x201f |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_Y14\_1X14 | 0x202d |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y13 | y12 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_Y16\_1X16 | 0x202e |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y15 | y14 | y13 | y12 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_UYVY8\_1X16 | 0x200f |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY8\_1X16 | 0x2010 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV8\_1X16 | 0x2011 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU8\_1X16 | 0x2012 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_YDYUYDYV8\_1X16 | 0x2014 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | d | d | d | d | d | d | d | d |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | d | d | d | d | d | d | d | d |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_UYVY10\_1X20 | 0x201a |  |  |  |  |  |  |  |  |  |  |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY10\_1X20 | 0x201b |  |  |  |  |  |  |  |  |  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV10\_1X20 | 0x200d |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU10\_1X20 | 0x200e |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_VUY8\_1X24 | 0x201a |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUV8\_1X24 | 0x2025 |  |  |  |  |  |  |  |  |  | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_UYYVYY8\_0\_5X24 | 0x2026 |  |  |  |  |  |  |  |  |  | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_UYVY12\_1X24 | 0x2020 |  |  |  |  |  |  |  |  |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_VYUY12\_1X24 | 0x2021 |  |  |  |  |  |  |  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUYV12\_1X24 | 0x2022 |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_YVYU12\_1X24 | 0x2023 |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  |  |  |  |  |  |  |  |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
| MEDIA\_BUS\_FMT\_YUV10\_1X30 | 0x2016 |  |  |  | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_UYYVYY10\_0\_5X30 | 0x2027 |  |  |  | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_AYUV8\_1X32 | 0x2017 |  | a7 | a6 | a5 | a4 | a3 | a2 | a1 | a0 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |

The following table list existing packed 36bit wide YUV formats.

36bit YUV Formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 35 | 34 | 33 | 32 | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 10 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_UYYVYY12\_0\_5X36 | 0x2028 |  | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
| MEDIA\_BUS\_FMT\_YUV12\_1X36 | 0x2029 |  | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |

The following table list existing packed 48bit wide YUV formats.

48bit YUV Formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 |
|  |  |  | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 10 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_YUV16\_1X48 | 0x202a |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | y15 | y14 | y13 | y12 | y11 | y10 | y8 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  | u15 | u14 | u13 | u12 | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 | v15 | v14 | v13 | v12 | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| MEDIA\_BUS\_FMT\_UYYVYY16\_0\_5X48 | 0x202b |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | u15 | u14 | u13 | u12 | u11 | u10 | u9 | u8 | u7 | u6 | u5 | u4 | u3 | u2 | u1 | u0 |
|  |  |  | y15 | y14 | y13 | y12 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y15 | y14 | y13 | y12 | y11 | y10 | y8 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | v15 | v14 | v13 | v12 | v11 | v10 | v9 | v8 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
|  |  |  | y15 | y14 | y13 | y12 | y11 | y10 | y9 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 | y15 | y14 | y13 | y12 | y11 | y10 | y8 | y8 | y7 | y6 | y5 | y4 | y3 | y2 | y1 | y0 |

### 4.13.3.4.1.1.4. HSV/HSL Formats

Those formats transfer pixel data as RGB values in a
cylindrical-coordinate system using Hue-Saturation-Value or
Hue-Saturation-Lightness components. The format code is made of the
following information.

* The hue, saturation, value or lightness and optional alpha components
  order code, as encoded in a pixel sample. The only currently
  supported value is AHSV.
* The number of bits per component, for each component. The values can
  be different for all components. The only currently supported value
  is 8888.
* The number of bus samples per pixel. Pixels that are wider than the
  bus width must be transferred in multiple samples. The only currently
  supported value is 1.
* The bus width.
* For formats where the total number of bits per pixel is smaller than
  the number of bus samples per pixel times the bus width, a padding
  value stating if the bytes are padded in their most high order bits
  (PADHI) or low order bits (PADLO).
* For formats where the number of bus samples per pixel is larger than
  1, an endianness value stating if the pixel is transferred MSB first
  (BE) or LSB first (LE).

The following table lists existing HSV/HSL formats.

HSV/HSL formats

| Identifier | Code |  | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_AHSV8888\_1X32 | 0x6001 |  | a7 | a6 | a5 | a4 | a3 | a2 | a1 | a0 | h7 | h6 | h5 | h4 | h3 | h2 | h1 | h0 | s7 | s6 | s5 | s4 | s3 | s2 | s1 | s0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |

### 4.13.3.4.1.1.5. JPEG Compressed Formats

Those data formats consist of an ordered sequence of 8-bit bytes
obtained from JPEG compression process. Additionally to the `_JPEG`
postfix the format code is made of the following information.

* The number of bus samples per entropy encoded byte.
* The bus width.

For instance, for a JPEG baseline process and an 8-bit bus width the
format will be named `MEDIA_BUS_FMT_JPEG_1X8`.

The following table lists existing JPEG compressed formats.

JPEG Formats

| Identifier | Code | Remarks |
| --- | --- | --- |
| MEDIA\_BUS\_FMT\_JPEG\_1X8 | 0x4001 | Besides of its usage for the parallel bus this format is recommended for transmission of JPEG data over MIPI CSI bus using the User Defined 8-bit Data types. |

### 4.13.3.4.1.1.6. Vendor and Device Specific Formats

This section lists complex data formats that are either vendor or device
specific.

The following table lists the existing vendor and device specific
formats.

Vendor and device specific formats

| Identifier | Code | Comments |
| --- | --- | --- |
| MEDIA\_BUS\_FMT\_S5C\_UYVY\_JPEG\_1X8 | 0x5001 | Interleaved raw UYVY and JPEG image format with embedded meta-data used by Samsung S3C73MX camera sensors. |

### 4.13.3.4.1.1.7. Metadata Formats

This section lists all metadata formats.

The following table lists the existing metadata formats.

Metadata formats

| Identifier | Code | Comments |
| --- | --- | --- |
| MEDIA\_BUS\_FMT\_METADATA\_FIXED | 0x7001 | This format should be used when the same driver handles both sides of the link and the bus format is a fixed metadata format that is not configurable from userspace. Width and height will be set to 0 for this format. |

### 4.13.3.4.1.1.8. Generic Serial Metadata Formats

Generic serial metadata formats are used on serial buses where the actual data
content is more or less device specific but the data is transmitted and received
by multiple devices that do not process the data in any way, simply writing
it to system memory for processing in software at the end of the pipeline.

“b” in an array cell signifies a byte of data, followed by the number of the bit
and finally the bit number in subscript. “x” indicates a padding bit.

Generic Serial Metadata Formats

| Identifier | Code |  | Data organization within bus [Data Unit](../glossary.html#term-Data-Unit) | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| MEDIA\_BUS\_FMT\_META\_8 | 0x8001 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 |
| MEDIA\_BUS\_FMT\_META\_10 | 0x8002 |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 | x | x |
| MEDIA\_BUS\_FMT\_META\_12 | 0x8003 |  |  |  |  |  |  |  |  |  |  |  |  |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 | x | x | x | x |
| MEDIA\_BUS\_FMT\_META\_14 | 0x8004 |  |  |  |  |  |  |  |  |  |  |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 | x | x | x | x | x | x |
| MEDIA\_BUS\_FMT\_META\_16 | 0x8005 |  |  |  |  |  |  |  |  |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 | x | x | x | x | x | x | x | x |
| MEDIA\_BUS\_FMT\_META\_20 | 0x8006 |  |  |  |  |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 | x | x | x | x | x | x | x | x | x | x | x | x |
| MEDIA\_BUS\_FMT\_META\_24 | 0x8007 |  | b07 | b06 | b05 | b04 | b03 | b02 | b01 | b00 | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x | x |
