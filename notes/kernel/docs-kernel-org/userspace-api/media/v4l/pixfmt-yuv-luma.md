# 2.7.1.3.Luma-Only Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-yuv-luma.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.3. Luma-Only Formats

This family of formats only store the luma component of a Y’CbCr image. They
are often referred to as greyscale formats.

Note

* In all the tables that follow, bit 7 is the most significant bit in a byte.
* Formats are described with the minimum number of pixels needed to create a
  byte-aligned repeating pattern. ... indicates repetition of the pattern.
* Y’x[9:2] denotes bits 9 to 2 of the Y’ value for pixel at column
  x.
* 0 denotes padding bits set to 0.

Luma-Only Image Formats

| Identifier | Code | Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Byte 6 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_GREY` | ‘GREY’ | Y’0[7:0] | ... | ... | ... | ... | ... | ... |
| `V4L2_PIX_FMT_IPU3_Y10` | ‘ip3y’ | Y’0[7:0] | Y’1[5:0] Y’0[9:8] | Y’2[3:0] Y’1[9:6] | Y’3[1:0] Y’2[9:4] | Y’3[9:2] | ... | ... |
| `V4L2_PIX_FMT_Y10` | ‘Y10 ‘ | Y’0[7:0] | 000000 Y’0[9:8] | ... | ... | ... | ... | ... |
| `V4L2_PIX_FMT_Y10BPACK` | ‘Y10B’ | Y’0[9:2] | Y’0[1:0] Y’1[9:4] | Y’1[3:0] Y’2[9:6] | Y’2[5:0] Y’3[9:8] | Y’3[7:0] | ... | ... |
| `V4L2_PIX_FMT_Y10P` | ‘Y10P’ | Y’0[9:2] | Y’1[9:2] | Y’2[9:2] | Y’3[9:2] | Y’3[1:0] Y’2[1:0] Y’1[1:0] Y’0[1:0] | ... | ... |
| `V4L2_PIX_FMT_Y12` | ‘Y12 ‘ | Y’0[7:0] | 0000 Y’0[11:8] | ... | ... | ... | ... | ... |
| `V4L2_PIX_FMT_Y012` | ‘Y012’ | Y’0[3:0] 0000 | Y’0[11:4] | ... | ... | ... | ... | ... |
| `V4L2_PIX_FMT_Y12P` | ‘Y12P’ | Y’0[11:4] | Y’1[11:4] | Y’1[3:0] Y’0[3:0] | ... | ... | ... | ... |
| `V4L2_PIX_FMT_Y14` | ‘Y14 ‘ | Y’0[7:0] | 00 Y’0[13:8] | ... | ... | ... | ... | ... |
| `V4L2_PIX_FMT_Y14P` | ‘Y14P’ | Y’0[13:6] | Y’1[13:6] | Y’2[13:6] | Y’3[13:6] | Y’1[1:0] Y’0[5:0] | Y’2[3:0] Y’1[5:2] | Y’3[5:0] Y’2[5:4] |
| `V4L2_PIX_FMT_Y16` | ‘Y16 ‘ | Y’0[7:0] | Y’0[15:8] | ... | ... | ... | ... | ... |
| `V4L2_PIX_FMT_Y16_BE` | ‘Y16 ‘ | (1U << 31) | Y’0[15:8] | Y’0[7:0] | ... | ... | ... | ... | ... |

Note

For the Y16 and Y16\_BE formats, the actual sampling precision may be lower
than 16 bits. For example, 10 bits per pixel uses values in the range 0 to
1023. For the IPU3\_Y10 format 25 pixels are packed into 32 bytes, which
leaves the 6 most significant bits of the last byte padded with 0.

For Y012 and Y12 formats, Y012 places its data in the 12 high bits, with
padding zeros in the 4 low bits, in contrast to the Y12 format, which has
its padding located in the most significant bits of the 16 bit word.

The ‘P’ variations of the Y10, Y12 and Y14 formats are packed according to
the RAW10, RAW12 and RAW14 packing scheme as defined by the MIPI CSI-2
specification.
