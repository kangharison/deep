# 2.7.YUV Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/yuv-formats.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7. YUV Formats

YUV is the format native to TV broadcast and composite video signals. It
separates the brightness information (Y) from the color information (U
and V or Cb and Cr). The color information consists of red and blue
*color difference* signals, this way the green component can be
reconstructed by subtracting from the brightness component. See
[Colorspaces](colorspaces.html#colorspaces) for conversion examples. YUV was chosen because
early television would only transmit brightness information. To add
color in a way compatible with existing receivers a new signal carrier
was added to transmit the color difference signals.

## 2.7.1. Subsampling

YUV formats commonly encode images with a lower resolution for the chroma
components than for the luma component. This compression technique, taking
advantage of the human eye being more sensitive to luminance than color
differences, is called chroma subsampling.

While many combinations of subsampling factors in the horizontal and vertical
direction are possible, common factors are 1 (no subsampling), 2 and 4, with
horizontal subsampling always larger than or equal to vertical subsampling.
Common combinations are named as follows.

* 4:4:4: No subsampling
* 4:2:2: Horizontal subsampling by 2, no vertical subsampling
* 4:2:0: Horizontal subsampling by 2, vertical subsampling by 2
* 4:1:1: Horizontal subsampling by 4, no vertical subsampling
* 4:1:0: Horizontal subsampling by 4, vertical subsampling by 4

Subsampling the chroma component effectively creates chroma values that can be
located in different spatial locations:

* The subsampled chroma value may be calculated by simply averaging the chroma
  value of two consecutive pixels. It effectively models the chroma of a pixel
  sited between the two original pixels. This is referred to as centered or
  interstitially sited chroma.
* The other option is to subsample chroma values in a way that place them in
  the same spatial sites as the pixels. This may be performed by skipping every
  other chroma sample (creating aliasing artifacts), or with filters using an
  odd number of taps. This is referred to as co-sited chroma.

The following examples show different combination of chroma siting in a 4x4
image.

4:2:2 subsampling, interstitially sited

|  | 0 |  | 1 |  | 2 |  | 3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Y | C | Y |  | Y | C | Y |
| 1 | Y | C | Y |  | Y | C | Y |
| 2 | Y | C | Y |  | Y | C | Y |
| 3 | Y | C | Y |  | Y | C | Y |

4:2:2 subsampling, co-sited

|  | 0 |  | 1 |  | 2 |  | 3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Y/C |  | Y |  | Y/C |  | Y |
| 1 | Y/C |  | Y |  | Y/C |  | Y |
| 2 | Y/C |  | Y |  | Y/C |  | Y |
| 3 | Y/C |  | Y |  | Y/C |  | Y |

4:2:0 subsampling, horizontally interstitially sited, vertically co-sited

|  | 0 |  | 1 |  | 2 |  | 3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Y | C | Y |  | Y | C | Y |
| 1 | Y |  | Y |  | Y |  | Y |
| 2 | Y | C | Y |  | Y | C | Y |
| 3 | Y |  | Y |  | Y |  | Y |

4:1:0 subsampling, horizontally and vertically interstitially sited

|  | 0 |  | 1 |  | 2 |  | 3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | Y |  | Y |  | Y |  | Y |
|  |  |  |  |  |  |  |  |
| 1 | Y |  | Y |  | Y |  | Y |
|  |  |  |  | C |  |  |  |
| 2 | Y |  | Y |  | Y |  | Y |
|  |  |  |  |  |  |  |  |
| 3 | Y |  | Y |  | Y |  | Y |

* [2.7.1.1. Packed YUV formats](pixfmt-packed-yuv.html)
* [2.7.1.2. Planar YUV formats](pixfmt-yuv-planar.html)
* [2.7.1.3. Luma-Only Formats](pixfmt-yuv-luma.html)
* [2.7.1.4. V4L2\_PIX\_FMT\_Y8I (‘Y8I ‘)](pixfmt-y8i.html)
* [2.7.1.5. V4L2\_PIX\_FMT\_Y12I (‘Y12I’)](pixfmt-y12i.html)
* [2.7.1.6. V4L2\_PIX\_FMT\_Y16I (‘Y16I’)](pixfmt-y16i.html)
* [2.7.1.7. V4L2\_PIX\_FMT\_UV8 (‘UV8’)](pixfmt-uv8.html)
* [2.7.1.8. V4L2\_PIX\_FMT\_M420 (‘M420’)](pixfmt-m420.html)
