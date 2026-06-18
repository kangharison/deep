# 2.8.1.Packed HSV formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-packed-hsv.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.8.1. Packed HSV formats

## 2.8.1.1. Description

The *hue* (h) is measured in degrees, the equivalence between degrees and LSBs
depends on the hsv-encoding used, see [Colorspaces](colorspaces.html#colorspaces).
The *saturation* (s) and the *value* (v) are measured in percentage of the
cylinder: 0 being the smallest value and 255 the maximum.

The values are packed in 24 or 32 bit formats.

Packed HSV Image Formats

| Identifier | Code |  | Byte 0 in memory | | | | | | | | Byte 1 | | | | | | | | Byte 2 | | | | | | | | Byte 3 | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| `V4L2_PIX_FMT_HSV32` | ‘HSV4’ |  |  |  |  |  |  |  |  |  | h7 | h6 | h5 | h4 | h3 | h2 | h1 | h0 | s7 | s6 | s5 | s4 | s3 | s2 | s1 | s0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |
| `V4L2_PIX_FMT_HSV24` | ‘HSV3’ |  | h7 | h6 | h5 | h4 | h3 | h2 | h1 | h0 | s7 | s6 | s5 | s4 | s3 | s2 | s1 | s0 | v7 | v6 | v5 | v4 | v3 | v2 | v1 | v0 |  | | | | | | | |

Bit 7 is the most significant bit.
