# 2.16.Defining Colorspaces in V4L2

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/colorspaces-defs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.16. Defining Colorspaces in V4L2

In V4L2 colorspaces are defined by four values. The first is the
colorspace identifier (enum [`v4l2_colorspace`](#c.V4L.v4l2_colorspace "v4l2_colorspace"))
which defines the chromaticities, the default transfer function, the
default Y’CbCr encoding and the default quantization method. The second
is the transfer function identifier (enum
[`v4l2_xfer_func`](#c.V4L.v4l2_xfer_func "v4l2_xfer_func")) to specify non-standard
transfer functions. The third is the Y’CbCr encoding identifier (enum
[`v4l2_ycbcr_encoding`](#c.V4L.v4l2_ycbcr_encoding "v4l2_ycbcr_encoding")) to specify
non-standard Y’CbCr encodings and the fourth is the quantization
identifier (enum [`v4l2_quantization`](#c.V4L.v4l2_quantization "v4l2_quantization")) to
specify non-standard quantization methods. Most of the time only the
colorspace field of struct [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "v4l2_pix_format")
or struct [`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "v4l2_pix_format_mplane")
needs to be filled in.

On [HSV formats](hsv-formats.html#hsv-formats) the *Hue* is defined as the angle on
the cylindrical color representation. Usually this angle is measured in
degrees, i.e. 0-360. When we map this angle value into 8 bits, there are
two basic ways to do it: Divide the angular value by 2 (0-179), or use the
whole range, 0-255, dividing the angular value by 1.41. The enum
[`v4l2_hsv_encoding`](#c.V4L.v4l2_hsv_encoding "v4l2_hsv_encoding") specifies which encoding is used.

Note

The default R’G’B’ quantization is full range for all
colorspaces. HSV formats are always full range.

type v4l2\_colorspace

V4L2 Colorspaces

| Identifier | Details |
| --- | --- |
| `V4L2_COLORSPACE_DEFAULT` | The default colorspace. This can be used by applications to let the driver fill in the colorspace. |
| `V4L2_COLORSPACE_SMPTE170M` | See [Colorspace SMPTE 170M (V4L2\_COLORSPACE\_SMPTE170M)](colorspaces-details.html#col-smpte-170m). |
| `V4L2_COLORSPACE_REC709` | See [Colorspace Rec. 709 (V4L2\_COLORSPACE\_REC709)](colorspaces-details.html#col-rec709). |
| `V4L2_COLORSPACE_SRGB` | See [Colorspace sRGB (V4L2\_COLORSPACE\_SRGB)](colorspaces-details.html#col-srgb). |
| `V4L2_COLORSPACE_OPRGB` | See [Colorspace opRGB (V4L2\_COLORSPACE\_OPRGB)](colorspaces-details.html#col-oprgb). |
| `V4L2_COLORSPACE_BT2020` | See [Colorspace BT.2020 (V4L2\_COLORSPACE\_BT2020)](colorspaces-details.html#col-bt2020). |
| `V4L2_COLORSPACE_DCI_P3` | See [Colorspace DCI-P3 (V4L2\_COLORSPACE\_DCI\_P3)](colorspaces-details.html#col-dcip3). |
| `V4L2_COLORSPACE_SMPTE240M` | See [Colorspace SMPTE 240M (V4L2\_COLORSPACE\_SMPTE240M)](colorspaces-details.html#col-smpte-240m). |
| `V4L2_COLORSPACE_470_SYSTEM_M` | See [Colorspace NTSC 1953 (V4L2\_COLORSPACE\_470\_SYSTEM\_M)](colorspaces-details.html#col-sysm). |
| `V4L2_COLORSPACE_470_SYSTEM_BG` | See [Colorspace EBU Tech. 3213 (V4L2\_COLORSPACE\_470\_SYSTEM\_BG)](colorspaces-details.html#col-sysbg). |
| `V4L2_COLORSPACE_JPEG` | See [Colorspace JPEG (V4L2\_COLORSPACE\_JPEG)](colorspaces-details.html#col-jpeg). |
| `V4L2_COLORSPACE_RAW` | The raw colorspace. This is used for raw image capture where the image is minimally processed and is using the internal colorspace of the device. The software that processes an image using this ‘colorspace’ will have to know the internals of the capture device. |

type v4l2\_xfer\_func

V4L2 Transfer Function

| Identifier | Details |
| --- | --- |
| `V4L2_XFER_FUNC_DEFAULT` | Use the default transfer function as defined by the colorspace. |
| `V4L2_XFER_FUNC_709` | Use the Rec. 709 transfer function. |
| `V4L2_XFER_FUNC_SRGB` | Use the sRGB transfer function. |
| `V4L2_XFER_FUNC_OPRGB` | Use the opRGB transfer function. |
| `V4L2_XFER_FUNC_SMPTE240M` | Use the SMPTE 240M transfer function. |
| `V4L2_XFER_FUNC_NONE` | Do not use a transfer function (i.e. use linear RGB values). |
| `V4L2_XFER_FUNC_DCI_P3` | Use the DCI-P3 transfer function. |
| `V4L2_XFER_FUNC_SMPTE2084` | Use the SMPTE 2084 transfer function. See [Transfer Function SMPTE 2084 (V4L2\_XFER\_FUNC\_SMPTE2084)](colorspaces-details.html#xf-smpte-2084). |

type v4l2\_ycbcr\_encoding

V4L2 Y’CbCr Encodings

| Identifier | Details |
| --- | --- |
| `V4L2_YCBCR_ENC_DEFAULT` | Use the default Y’CbCr encoding as defined by the colorspace. |
| `V4L2_YCBCR_ENC_601` | Use the BT.601 Y’CbCr encoding. |
| `V4L2_YCBCR_ENC_709` | Use the Rec. 709 Y’CbCr encoding. |
| `V4L2_YCBCR_ENC_XV601` | Use the extended gamut xvYCC BT.601 encoding. |
| `V4L2_YCBCR_ENC_XV709` | Use the extended gamut xvYCC Rec. 709 encoding. |
| `V4L2_YCBCR_ENC_BT2020` | Use the default non-constant luminance BT.2020 Y’CbCr encoding. |
| `V4L2_YCBCR_ENC_BT2020_CONST_LUM` | Use the constant luminance BT.2020 Yc’CbcCrc encoding. |
| `V4L2_YCBCR_ENC_SMPTE_240M` | Use the SMPTE 240M Y’CbCr encoding. |

type v4l2\_hsv\_encoding

V4L2 HSV Encodings

| Identifier | Details |
| --- | --- |
| `V4L2_HSV_ENC_180` | For the Hue, each LSB is two degrees. |
| `V4L2_HSV_ENC_256` | For the Hue, the 360 degrees are mapped into 8 bits, i.e. each LSB is roughly 1.41 degrees. |

type v4l2\_quantization

V4L2 Quantization Methods

| Identifier | Details |
| --- | --- |
| `V4L2_QUANTIZATION_DEFAULT` | Use the default quantization encoding as defined by the colorspace. This is always full range for R’G’B’ and HSV. It is usually limited range for Y’CbCr. |
| `V4L2_QUANTIZATION_FULL_RANGE` | Use the full range quantization encoding. I.e. the range [0…1] is mapped to [0…255] (with possible clipping to [1…254] to avoid the 0x00 and 0xff values). Cb and Cr are mapped from [-0.5…0.5] to [0…255] (with possible clipping to [1…254] to avoid the 0x00 and 0xff values). |
| `V4L2_QUANTIZATION_LIM_RANGE` | Use the limited range quantization encoding. I.e. the range [0…1] is mapped to [16…235]. Cb and Cr are mapped from [-0.5…0.5] to [16…240]. Limited Range cannot be used with HSV. |
