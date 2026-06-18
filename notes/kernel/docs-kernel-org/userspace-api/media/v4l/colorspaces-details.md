# 2.17.Detailed Colorspace Descriptions

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/colorspaces-details.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.17. Detailed Colorspace Descriptions

## 2.17.1. Colorspace SMPTE 170M (V4L2\_COLORSPACE\_SMPTE170M)

The [SMPTE 170M](biblio.html#smpte170m) standard defines the colorspace used by NTSC and
PAL and by SDTV in general. The default transfer function is
`V4L2_XFER_FUNC_709`. The default Y’CbCr encoding is
`V4L2_YCBCR_ENC_601`. The default Y’CbCr quantization is limited
range. The chromaticities of the primary colors and the white reference
are:

SMPTE 170M Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.630 | 0.340 |
| Green | 0.310 | 0.595 |
| Blue | 0.155 | 0.070 |
| White Reference (D65) | 0.3127 | 0.3290 |

The red, green and blue chromaticities are also often referred to as the
SMPTE C set, so this colorspace is sometimes called SMPTE C as well.

The transfer function defined for SMPTE 170M is the same as the one
defined in Rec. 709.

![L' = -1.099(-L)^{0.45} + 0.099 \text{, for } L \le-0.018

L' = 4.5L \text{, for } -0.018 < L < 0.018

L' = 1.099L^{0.45} - 0.099 \text{, for } L \ge 0.018](_images/math/20700f406bfc958460a8490b6965f4982cce7337.png)

Inverse Transfer function:

![L = -\left( \frac{L' - 0.099}{-1.099} \right) ^{\frac{1}{0.45}} \text{, for } L' \le -0.081

L = \frac{L'}{4.5} \text{, for } -0.081 < L' < 0.081

L = \left(\frac{L' + 0.099}{1.099}\right)^{\frac{1}{0.45} } \text{, for } L' \ge 0.081](_images/math/d749570f774001d563b5ef43a973351b923988c6.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_601` encoding:

![Y' = 0.2990R' + 0.5870G' + 0.1140B'

Cb = -0.1687R' - 0.3313G' + 0.5B'

Cr = 0.5R' - 0.4187G' - 0.0813B'](_images/math/3443fb32f8366e7cdca3cceb6ef313588bce1f8f.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5]. This conversion to Y’CbCr is identical to the one defined in
the [ITU BT.601](biblio.html#itu601) standard and this colorspace is sometimes called
BT.601 as well, even though BT.601 does not mention any color primaries.

The default quantization is limited range, but full range is possible
although rarely seen.

## 2.17.2. Colorspace Rec. 709 (V4L2\_COLORSPACE\_REC709)

The [ITU BT.709](biblio.html#itu709) standard defines the colorspace used by HDTV in
general. The default transfer function is `V4L2_XFER_FUNC_709`. The
default Y’CbCr encoding is `V4L2_YCBCR_ENC_709`. The default Y’CbCr
quantization is limited range. The chromaticities of the primary colors
and the white reference are:

Rec. 709 Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.640 | 0.330 |
| Green | 0.300 | 0.600 |
| Blue | 0.150 | 0.060 |
| White Reference (D65) | 0.3127 | 0.3290 |

The full name of this standard is Rec. ITU-R BT.709-5.

Transfer function. Normally L is in the range [0…1], but for the
extended gamut xvYCC encoding values outside that range are allowed.

![L' = -1.099(-L)^{0.45} + 0.099 \text{, for } L \le -0.018

L' = 4.5L \text{, for } -0.018 < L < 0.018

L' = 1.099L^{0.45} - 0.099 \text{, for } L \ge 0.018](_images/math/5a5e5e6c19b5b0c66918e8b378d8b94fb70f91c3.png)

Inverse Transfer function:

![L = -\left( \frac{L' - 0.099}{-1.099} \right)^\frac{1}{0.45} \text{, for } L' \le -0.081

L = \frac{L'}{4.5}\text{, for } -0.081 < L' < 0.081

L = \left(\frac{L' + 0.099}{1.099}\right)^{\frac{1}{0.45} } \text{, for } L' \ge 0.081](_images/math/06d2edaac965cda9ffe89dd633bab943d0c5f52f.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_709` encoding:

![Y' = 0.2126R' + 0.7152G' + 0.0722B'

Cb = -0.1146R' - 0.3854G' + 0.5B'

Cr = 0.5R' - 0.4542G' - 0.0458B'](_images/math/968187e4d5e190fec1a53cb068dba715c69ef331.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5].

The default quantization is limited range, but full range is possible
although rarely seen.

The `V4L2_YCBCR_ENC_709` encoding described above is the default for
this colorspace, but it can be overridden with `V4L2_YCBCR_ENC_601`,
in which case the BT.601 Y’CbCr encoding is used.

Two additional extended gamut Y’CbCr encodings are also possible with
this colorspace:

The xvYCC 709 encoding (`V4L2_YCBCR_ENC_XV709`, [xvYCC](biblio.html#xvycc)) is
similar to the Rec. 709 encoding, but it allows for R’, G’ and B’ values
that are outside the range [0…1]. The resulting Y’, Cb and Cr values are
scaled and offset according to the limited range formula:

![Y' = \frac{219}{256} * (0.2126R' + 0.7152G' + 0.0722B') + \frac{16}{256}

Cb = \frac{224}{256} * (-0.1146R' - 0.3854G' + 0.5B')

Cr = \frac{224}{256} * (0.5R' - 0.4542G' - 0.0458B')](_images/math/889706ac38f4ce905691f7c2f92179d6b7cd2d55.png)

The xvYCC 601 encoding (`V4L2_YCBCR_ENC_XV601`, [xvYCC](biblio.html#xvycc)) is
similar to the BT.601 encoding, but it allows for R’, G’ and B’ values
that are outside the range [0…1]. The resulting Y’, Cb and Cr values are
scaled and offset according to the limited range formula:

![Y' = \frac{219}{256} * (0.2990R' + 0.5870G' + 0.1140B') + \frac{16}{256}

Cb = \frac{224}{256} * (-0.1687R' - 0.3313G' + 0.5B')

Cr = \frac{224}{256} * (0.5R' - 0.4187G' - 0.0813B')](_images/math/ec83c99cbfc2190a021149b49147b9b68f5b871d.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5] and quantized without further scaling or offsets.
The non-standard xvYCC 709 or xvYCC 601 encodings can be
used by selecting `V4L2_YCBCR_ENC_XV709` or `V4L2_YCBCR_ENC_XV601`.
As seen by the xvYCC formulas these encodings always use limited range quantization,
there is no full range variant. The whole point of these extended gamut encodings
is that values outside the limited range are still valid, although they
map to R’, G’ and B’ values outside the [0…1] range and are therefore outside
the Rec. 709 colorspace gamut.

## 2.17.3. Colorspace sRGB (V4L2\_COLORSPACE\_SRGB)

The [sRGB](biblio.html#srgb) standard defines the colorspace used by most webcams
and computer graphics. The default transfer function is
`V4L2_XFER_FUNC_SRGB`. The default Y’CbCr encoding is
`V4L2_YCBCR_ENC_601`. The default Y’CbCr quantization is limited range.

Note that the [sYCC](biblio.html#sycc) standard specifies full range quantization,
however all current capture hardware supported by the kernel convert
R’G’B’ to limited range Y’CbCr. So choosing full range as the default
would break how applications interpret the quantization range.

The chromaticities of the primary colors and the white reference are:

sRGB Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.640 | 0.330 |
| Green | 0.300 | 0.600 |
| Blue | 0.150 | 0.060 |
| White Reference (D65) | 0.3127 | 0.3290 |

These chromaticities are identical to the Rec. 709 colorspace.

Transfer function. Note that negative values for L are only used by the
Y’CbCr conversion.

![L' = -1.055(-L)^{\frac{1}{2.4} } + 0.055\text{, for }L < -0.0031308

L' = 12.92L\text{, for }-0.0031308 \le L \le 0.0031308

L' = 1.055L ^{\frac{1}{2.4} } - 0.055\text{, for }0.0031308 < L \le 1](_images/math/d4f24f42229c5db8d0a2e22adb92eda900cad334.png)

Inverse Transfer function:

![L = -((-L' + 0.055) / 1.055) ^{2.4}\text{, for }L' < -0.04045

L = L' / 12.92\text{, for }-0.04045 \le L' \le 0.04045

L = ((L' + 0.055) / 1.055) ^{2.4}\text{, for }L' > 0.04045](_images/math/bde6c82cd265bc2befd9ad3867594b67ff57895e.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_601` encoding as defined by [sYCC](biblio.html#sycc):

![Y' = 0.2990R' + 0.5870G' + 0.1140B'

Cb = -0.1687R' - 0.3313G' + 0.5B'

Cr = 0.5R' - 0.4187G' - 0.0813B'](_images/math/3443fb32f8366e7cdca3cceb6ef313588bce1f8f.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5]. This transform is identical to one defined in SMPTE
170M/BT.601. The Y’CbCr quantization is limited range.

## 2.17.4. Colorspace opRGB (V4L2\_COLORSPACE\_OPRGB)

The [opRGB](biblio.html#oprgb) standard defines the colorspace used by computer
graphics that use the opRGB colorspace. The default transfer function is
`V4L2_XFER_FUNC_OPRGB`. The default Y’CbCr encoding is
`V4L2_YCBCR_ENC_601`. The default Y’CbCr quantization is limited
range.

Note that the [opRGB](biblio.html#oprgb) standard specifies full range quantization,
however all current capture hardware supported by the kernel convert
R’G’B’ to limited range Y’CbCr. So choosing full range as the default
would break how applications interpret the quantization range.

The chromaticities of the primary colors and the white reference are:

opRGB Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.6400 | 0.3300 |
| Green | 0.2100 | 0.7100 |
| Blue | 0.1500 | 0.0600 |
| White Reference (D65) | 0.3127 | 0.3290 |

Transfer function:

![L' = L ^{\frac{1}{2.19921875}}](_images/math/913fdb27cef6553d1abb31f1ebde3023542c1452.png)

Inverse Transfer function:

![L = L'^{(2.19921875)}](_images/math/879383eb7f4c7984a51808bbd73f7becbde30b24.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_601` encoding:

![Y' = 0.2990R' + 0.5870G' + 0.1140B'

Cb = -0.1687R' - 0.3313G' + 0.5B'

Cr = 0.5R' - 0.4187G' - 0.0813B'](_images/math/3443fb32f8366e7cdca3cceb6ef313588bce1f8f.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5]. This transform is identical to one defined in SMPTE
170M/BT.601. The Y’CbCr quantization is limited range.

## 2.17.5. Colorspace BT.2020 (V4L2\_COLORSPACE\_BT2020)

The [ITU BT.2020](biblio.html#itu2020) standard defines the colorspace used by Ultra-high
definition television (UHDTV). The default transfer function is
`V4L2_XFER_FUNC_709`. The default Y’CbCr encoding is
`V4L2_YCBCR_ENC_BT2020`. The default Y’CbCr quantization is limited range.
The chromaticities of the primary colors and the white reference are:

BT.2020 Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.708 | 0.292 |
| Green | 0.170 | 0.797 |
| Blue | 0.131 | 0.046 |
| White Reference (D65) | 0.3127 | 0.3290 |

Transfer function (same as Rec. 709):

![L' = 4.5L\text{, for }0 \le L < 0.018

L' = 1.099L ^{0.45} - 0.099\text{, for } 0.018 \le L \le 1](_images/math/1208cc94c20e1df2fcf369d4ddb909d855f4bc47.png)

Inverse Transfer function:

![L = L' / 4.5\text{, for } L' < 0.081

L = \left( \frac{L' + 0.099}{1.099}\right) ^{\frac{1}{0.45} }\text{, for } L' \ge 0.081](_images/math/967b2122a4171315cad26e53bb206da9ee8bc5b4.png)

Please note that while Rec. 709 is defined as the default transfer function
by the [ITU BT.2020](biblio.html#itu2020) standard, in practice this colorspace is often used
with the [Transfer Function SMPTE 2084 (V4L2\_XFER\_FUNC\_SMPTE2084)](#xf-smpte-2084). In particular Ultra HD Blu-ray discs use
this combination.

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_BT2020` encoding:

![Y' = 0.2627R' + 0.6780G' + 0.0593B'

Cb = -0.1396R' - 0.3604G' + 0.5B'

Cr = 0.5R' - 0.4598G' - 0.0402B'](_images/math/8407bba9ab333398b1f5469a5d213584615b016b.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5]. The Y’CbCr quantization is limited range.

There is also an alternate constant luminance R’G’B’ to Yc’CbcCrc
(`V4L2_YCBCR_ENC_BT2020_CONST_LUM`) encoding:

Luma:

![\begin{align*}
Yc' = (0.2627R + 0.6780G + 0.0593B)'& \\
B' - Yc' \le 0:& \\
    &Cbc = (B' - Yc') / 1.9404 \\
B' - Yc' > 0: & \\
    &Cbc = (B' - Yc') / 1.5816 \\
R' - Yc' \le 0:& \\
    &Crc = (R' - Y') / 1.7184 \\
R' - Yc' > 0:& \\
    &Crc = (R' - Y') / 0.9936
\end{align*}](_images/math/e41516f70e743569b02c20a0e70616e07f09a273.png)

Yc’ is clamped to the range [0…1] and Cbc and Crc are clamped to the
range [-0.5…0.5]. The Yc’CbcCrc quantization is limited range.

## 2.17.6. Colorspace DCI-P3 (V4L2\_COLORSPACE\_DCI\_P3)

The [SMPTE RP 431-2](biblio.html#smpte431) standard defines the colorspace used by cinema
projectors that use the DCI-P3 colorspace. The default transfer function
is `V4L2_XFER_FUNC_DCI_P3`. The default Y’CbCr encoding is
`V4L2_YCBCR_ENC_709`. The default Y’CbCr quantization is limited range.

Note

Note that this colorspace standard does not specify a
Y’CbCr encoding since it is not meant to be encoded to Y’CbCr. So this
default Y’CbCr encoding was picked because it is the HDTV encoding.

The chromaticities of the primary colors and the white reference are:

DCI-P3 Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.6800 | 0.3200 |
| Green | 0.2650 | 0.6900 |
| Blue | 0.1500 | 0.0600 |
| White Reference | 0.3140 | 0.3510 |

Transfer function:

![L' = L^{\frac{1}{2.6}}](_images/math/78c97f179aa052a24acf88ad64acdbccfcd90ba8.png)

Inverse Transfer function:

![L = L'^{(2.6)}](_images/math/d494c1c9416623f5035f5612f9b311d583bcbf5b.png)

Y’CbCr encoding is not specified. V4L2 defaults to Rec. 709.

## 2.17.7. Colorspace SMPTE 240M (V4L2\_COLORSPACE\_SMPTE240M)

The [SMPTE 240M](biblio.html#smpte240m) standard was an interim standard used during the
early days of HDTV (1988-1998). It has been superseded by Rec. 709. The
default transfer function is `V4L2_XFER_FUNC_SMPTE240M`. The default
Y’CbCr encoding is `V4L2_YCBCR_ENC_SMPTE240M`. The default Y’CbCr
quantization is limited range. The chromaticities of the primary colors
and the white reference are:

SMPTE 240M Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.630 | 0.340 |
| Green | 0.310 | 0.595 |
| Blue | 0.155 | 0.070 |
| White Reference (D65) | 0.3127 | 0.3290 |

These chromaticities are identical to the SMPTE 170M colorspace.

Transfer function:

![L' = 4L\text{, for } 0 \le L < 0.0228

L' = 1.1115L ^{0.45} - 0.1115\text{, for } 0.0228 \le L \le 1](_images/math/dfa503031e027ccdc45658848c15187db8acff69.png)

Inverse Transfer function:

![L = \frac{L'}{4}\text{, for } 0 \le L' < 0.0913

L = \left( \frac{L' + 0.1115}{1.1115}\right) ^{\frac{1}{0.45} }\text{, for } L' \ge 0.0913](_images/math/96bd0668efd142a81aca9695b45bfc617a7e7251.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_SMPTE240M` encoding:

![Y' = 0.2122R' + 0.7013G' + 0.0865B'

Cb = -0.1161R' - 0.3839G' + 0.5B'

Cr = 0.5R' - 0.4451G' - 0.0549B'](_images/math/453a6a0fa1f6be9aec4ec5eed18afc71b58087b1.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the
range [-0.5…0.5]. The Y’CbCr quantization is limited range.

## 2.17.8. Colorspace NTSC 1953 (V4L2\_COLORSPACE\_470\_SYSTEM\_M)

This standard defines the colorspace used by NTSC in 1953. In practice
this colorspace is obsolete and SMPTE 170M should be used instead. The
default transfer function is `V4L2_XFER_FUNC_709`. The default Y’CbCr
encoding is `V4L2_YCBCR_ENC_601`. The default Y’CbCr quantization is
limited range. The chromaticities of the primary colors and the white
reference are:

NTSC 1953 Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.67 | 0.33 |
| Green | 0.21 | 0.71 |
| Blue | 0.14 | 0.08 |
| White Reference (C) | 0.310 | 0.316 |

Note

This colorspace uses Illuminant C instead of D65 as the white
reference. To correctly convert an image in this colorspace to another
that uses D65 you need to apply a chromatic adaptation algorithm such as
the Bradford method.

The transfer function was never properly defined for NTSC 1953. The Rec.
709 transfer function is recommended in the literature:

![L' = 4.5L\text{, for } 0 \le L < 0.018

L' = 1.099L ^{0.45} - 0.099\text{, for } 0.018 \le L \le 1](_images/math/e933fdddd93febbe34ced91922f16fb19bbbc3eb.png)

Inverse Transfer function:

![L = \frac{L'}{4.5} \text{, for } L' < 0.081

L = \left( \frac{L' + 0.099}{1.099}\right) ^{\frac{1}{0.45} }\text{, for } L' \ge 0.081](_images/math/b3b39fa95b4a78001cf15ee45ef53ce8e4552368.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_601` encoding:

![Y' = 0.2990R' + 0.5870G' + 0.1140B'

Cb = -0.1687R' - 0.3313G' + 0.5B'

Cr = 0.5R' - 0.4187G' - 0.0813B'](_images/math/3443fb32f8366e7cdca3cceb6ef313588bce1f8f.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5]. The Y’CbCr quantization is limited range. This transform is
identical to one defined in SMPTE 170M/BT.601.

## 2.17.9. Colorspace EBU Tech. 3213 (V4L2\_COLORSPACE\_470\_SYSTEM\_BG)

The [EBU Tech 3213](biblio.html#tech3213) standard defines the colorspace used by PAL/SECAM
in 1975. Note that this colorspace is not supported by the HDMI interface.
Instead [EBU Tech 3321](biblio.html#tech3321) recommends that Rec. 709 is used instead for HDMI.
The default transfer function is
`V4L2_XFER_FUNC_709`. The default Y’CbCr encoding is
`V4L2_YCBCR_ENC_601`. The default Y’CbCr quantization is limited
range. The chromaticities of the primary colors and the white reference
are:

EBU Tech. 3213 Chromaticities

| Color | x | y |
| --- | --- | --- |
| Red | 0.64 | 0.33 |
| Green | 0.29 | 0.60 |
| Blue | 0.15 | 0.06 |
| White Reference (D65) | 0.3127 | 0.3290 |

The transfer function was never properly defined for this colorspace.
The Rec. 709 transfer function is recommended in the literature:

![L' = 4.5L\text{, for } 0 \le L < 0.018

L' = 1.099L ^{0.45} - 0.099\text{, for } 0.018 \le L \le 1](_images/math/e933fdddd93febbe34ced91922f16fb19bbbc3eb.png)

Inverse Transfer function:

![L = \frac{L'}{4.5} \text{, for } L' < 0.081

L = \left(\frac{L' + 0.099}{1.099} \right) ^{\frac{1}{0.45} }\text{, for } L' \ge 0.081](_images/math/c207d9c2e6e40564e78ec538a91098328a6be76e.png)

The luminance (Y’) and color difference (Cb and Cr) are obtained with
the following `V4L2_YCBCR_ENC_601` encoding:

![Y' = 0.2990R' + 0.5870G' + 0.1140B'

Cb = -0.1687R' - 0.3313G' + 0.5B'

Cr = 0.5R' - 0.4187G' - 0.0813B'](_images/math/3443fb32f8366e7cdca3cceb6ef313588bce1f8f.png)

Y’ is clamped to the range [0…1] and Cb and Cr are clamped to the range
[-0.5…0.5]. The Y’CbCr quantization is limited range. This transform is
identical to one defined in SMPTE 170M/BT.601.

## 2.17.10. Colorspace JPEG (V4L2\_COLORSPACE\_JPEG)

This colorspace defines the colorspace used by most (Motion-)JPEG
formats. The chromaticities of the primary colors and the white
reference are identical to sRGB. The transfer function use is
`V4L2_XFER_FUNC_SRGB`. The Y’CbCr encoding is `V4L2_YCBCR_ENC_601`
with full range quantization where Y’ is scaled to [0…255] and Cb/Cr are
scaled to [-128…128] and then clipped to [-128…127].

Note

The JPEG standard does not actually store colorspace
information. So if something other than sRGB is used, then the driver
will have to set that information explicitly. Effectively
`V4L2_COLORSPACE_JPEG` can be considered to be an abbreviation for
`V4L2_COLORSPACE_SRGB`, `V4L2_XFER_FUNC_SRGB`, `V4L2_YCBCR_ENC_601`
and `V4L2_QUANTIZATION_FULL_RANGE`.

# 2.18. Detailed Transfer Function Descriptions

## 2.18.1. Transfer Function SMPTE 2084 (V4L2\_XFER\_FUNC\_SMPTE2084)

The [SMPTE ST 2084](biblio.html#smpte2084) standard defines the transfer function used by
High Dynamic Range content.

Constants:
:   m1 = (2610 / 4096) / 4

    m2 = (2523 / 4096) \* 128

    c1 = 3424 / 4096

    c2 = (2413 / 4096) \* 32

    c3 = (2392 / 4096) \* 32

Transfer function:
:   L’ = ((c1 + c2 \* Lm1) / (1 + c3 \* Lm1))m2

Inverse Transfer function:
:   L = (max(L’1/m2 - c1, 0) / (c2 - c3 \*
    L’1/m2))1/m1

Take care when converting between this transfer function and non-HDR transfer
functions: the linear RGB values [0…1] of HDR content map to a luminance range
of 0 to 10000 cd/m2 whereas the linear RGB values of non-HDR (aka
Standard Dynamic Range or SDR) map to a luminance range of 0 to 100 cd/m2.

To go from SDR to HDR you will have to divide L by 100 first. To go in the other
direction you will have to multiply L by 100. Of course, this clamps all
luminance values over 100 cd/m2 to 100 cd/m2.

There are better methods, see e.g. [colimg](biblio.html#colimg) for more in-depth information
about this.
