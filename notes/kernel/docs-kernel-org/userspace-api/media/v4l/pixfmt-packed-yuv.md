# 2.7.1.1.Packed YUV formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-packed-yuv.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.1. Packed YUV formats

Similarly to the packed RGB formats, the packed YUV formats store the Y, Cb and
Cr components consecutively in memory. They may apply subsampling to the chroma
components and thus differ in how they interlave the three components.

Note

* In all the tables that follow, bit 7 is the most significant bit in a byte.
* ‘Y’, ‘Cb’ and ‘Cr’ denote bits of the luma, blue chroma (also known as
  ‘U’) and red chroma (also known as ‘V’) components respectively. ‘A’
  denotes bits of the alpha component (if supported by the format), and ‘X’
  denotes padding bits.

## 2.7.1.1.1. 4:4:4 Subsampling

These formats do not subsample the chroma components and store each pixels as a
full triplet of Y, Cb and Cr values.

The next table lists the packed YUV 4:4:4 formats with less than 8 bits per
component. They are named based on the order of the Y, Cb and Cr components as
seen in a 16-bit word, which is then stored in memory in little endian byte
order, and on the number of bits for each component. For instance the YUV565
format stores a pixel in a 16-bit word [15:0] laid out at as [Y’4-0
Cb5-0 Cr4-0], and stored in memory in two bytes,
[Cb2-0 Cr4-0] followed by [Y’4-0 Cb5-3].

Packed YUV 4:4:4 Image Formats (less than 8bpc)

| Identifier | Code | Byte 0 in memory | | | | | | | | Byte 1 | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| `V4L2_PIX_FMT_YUV444` | ‘Y444’ | Cb3 | Cb2 | Cb1 | Cb0 | Cr3 | Cr2 | Cr1 | Cr0 | a3 | a2 | a1 | a0 | Y’3 | Y’2 | Y’1 | Y’0 |
| `V4L2_PIX_FMT_YUV555` | ‘YUVO’ | Cb2 | Cb1 | Cb0 | Cr4 | Cr3 | Cr2 | Cr1 | Cr0 | a | Y’4 | Y’3 | Y’2 | Y’1 | Y’0 | Cb4 | Cb3 |
| `V4L2_PIX_FMT_YUV565` | ‘YUVP’ | Cb2 | Cb1 | Cb0 | Cr4 | Cr3 | Cr2 | Cr1 | Cr0 | Y’4 | Y’3 | Y’2 | Y’1 | Y’0 | Cb5 | Cb4 | Cb3 |

Note

For the YUV444 and YUV555 formats, the value of alpha bits is undefined
when reading from the driver, ignored when writing to the driver, except
when alpha blending has been negotiated for a [Video Overlay](dev-overlay.html#overlay) or [Video Output Overlay](dev-osd.html#osd).

The next table lists the packed YUV 4:4:4 formats with 8 bits per component.
They are named based on the order of the Y, Cb and Cr components as stored in
memory, and on the total number of bits per pixel. For instance, the VUYX32
format stores a pixel with Cr7-0 in the first byte, Cb7-0 in
the second byte and Y’7-0 in the third byte.

Packed YUV Image Formats (8bpc)

| Identifier | Code | Byte 0 | Byte 1 | Byte 2 | Byte 3 |
| --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_YUV32` | ‘YUV4’ | A7-0 | Y’7-0 | Cb7-0 | Cr7-0 |
| `V4L2_PIX_FMT_AYUV32` | ‘AYUV’ | A7-0 | Y’7-0 | Cb7-0 | Cr7-0 |
| `V4L2_PIX_FMT_XYUV32` | ‘XYUV’ | X7-0 | Y’7-0 | Cb7-0 | Cr7-0 |
| `V4L2_PIX_FMT_VUYA32` | ‘VUYA’ | Cr7-0 | Cb7-0 | Y’7-0 | A7-0 |
| `V4L2_PIX_FMT_VUYX32` | ‘VUYX’ | Cr7-0 | Cb7-0 | Y’7-0 | X7-0 |
| `V4L2_PIX_FMT_YUVA32` | ‘YUVA’ | Y’7-0 | Cb7-0 | Cr7-0 | A7-0 |
| `V4L2_PIX_FMT_YUVX32` | ‘YUVX’ | Y’7-0 | Cb7-0 | Cr7-0 | X7-0 |
| `V4L2_PIX_FMT_YUV24` | ‘YUV3’ | Y’7-0 | Cb7-0 | Cr7-0 | - |

Note

* The alpha component is expected to contain a meaningful value that can be
  used by drivers and applications.
* The padding bits contain undefined values that must be ignored by all
  applications and drivers.

The next table lists the packed YUV 4:4:4 formats with 12 bits per component.
Expand the bits per component to 16 bits, data in the high bits, zeros in the low bits,
arranged in little endian order, storing 1 pixel in 6 bytes.

Packed YUV 4:4:4 Image Formats (12bpc)

| Identifier | Code | Byte 1-0 | Byte 3-2 | Byte 5-4 | Byte 7-6 | Byte 9-8 | Byte 11-10 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_YUV48_12` | ‘Y312’ | Y’0 | Cb0 | Cr0 | Y’1 | Cb1 | Cr1 |

## 2.7.1.1.2. 4:2:2 Subsampling

These formats, commonly referred to as YUYV or YUY2, subsample the chroma
components horizontally by 2, storing 2 pixels in a container. The container
is 32-bits for 8-bit formats, and 64-bits for 10+-bit formats.

The packed YUYV formats with more than 8 bits per component are stored as four
16-bit little-endian words. Each word’s most significant bits contain one
component, and the least significant bits are zero padding.

Packed YUV 4:2:2 Formats in 32-bit container

| Identifier | Code | Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Byte 6 | Byte 7 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_UYVY` | ‘UYVY’ | Cb0 | Y’0 | Cr0 | Y’1 | Cb2 | Y’2 | Cr2 | Y’3 |
| `V4L2_PIX_FMT_VYUY` | ‘VYUY’ | Cr0 | Y’0 | Cb0 | Y’1 | Cr2 | Y’2 | Cb2 | Y’3 |
| `V4L2_PIX_FMT_YUYV` | ‘YUYV’ | Y’0 | Cb0 | Y’1 | Cr0 | Y’2 | Cb2 | Y’3 | Cr2 |
| `V4L2_PIX_FMT_YVYU` | ‘YVYU’ | Y’0 | Cr0 | Y’1 | Cb0 | Y’2 | Cr2 | Y’3 | Cb2 |

Packed YUV 4:2:2 Formats in 64-bit container

| Identifier | Code | Word 0 | Word 1 | Word 2 | Word 3 |
| --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_Y210` | ‘Y210’ | Y’0 (bits 15-6) | Cb0 (bits 15-6) | Y’1 (bits 15-6) | Cr0 (bits 15-6) |
| `V4L2_PIX_FMT_Y212` | ‘Y212’ | Y’0 (bits 15-4) | Cb0 (bits 15-4) | Y’1 (bits 15-4) | Cr0 (bits 15-4) |
| `V4L2_PIX_FMT_Y216` | ‘Y216’ | Y’0 (bits 15-0) | Cb0 (bits 15-0) | Y’1 (bits 15-0) | Cr0 (bits 15-0) |

**Color Sample Location:**
Chroma samples are [interstitially sited](yuv-formats.html#yuv-chroma-centered)
horizontally.

## 2.7.1.1.3. 4:1:1 Subsampling

This format subsamples the chroma components horizontally by 4, storing 8
pixels in 12 bytes.

Packed YUV 4:1:1 Formats

| Identifier | Code | Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Byte 6 | Byte 7 | Byte 8 | Byte 9 | Byte 10 | Byte 11 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_Y41P` | ‘Y41P’ | Cb0 | Y’0 | Cr0 | Y’1 | Cb4 | Y’2 | Cr4 | Y’3 | Y’4 | Y’5 | Y’6 | Y’7 |

Note

Do not confuse `V4L2_PIX_FMT_Y41P` with
[V4L2\_PIX\_FMT\_YUV411P](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv411p). Y41P is derived from
“YUV 4:1:1 **packed**”, while YUV411P stands for “YUV 4:1:1 **planar**”.

**Color Sample Location:**
Chroma samples are [interstitially sited](yuv-formats.html#yuv-chroma-centered)
horizontally.
