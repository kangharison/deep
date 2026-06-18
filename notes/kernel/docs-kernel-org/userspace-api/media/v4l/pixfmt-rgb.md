# 2.5.RGB Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-rgb.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.5. RGB Formats

These formats encode each pixel as a triplet of RGB values. They are packed
formats, meaning that the RGB values for one pixel are stored consecutively in
memory and each pixel consumes an integer number of bytes. When the number of
bits required to store a pixel is not aligned to a byte boundary, the data is
padded with additional bits to fill the remaining byte.

The formats differ by the number of bits per RGB component (typically but not
always the same for all components), the order of components in memory, and the
presence of an alpha component or additional padding bits.

The usage and value of the alpha bits in formats that support them (named ARGB
or a permutation thereof, collectively referred to as alpha formats) depend on
the device type and hardware operation. [Capture](dev-capture.html#capture) devices
(including capture queues of mem-to-mem devices) fill the alpha component in
memory. When the device captures an alpha channel the alpha component will have
a meaningful value. Otherwise, when the device doesn’t capture an alpha channel
but can set the alpha bit to a user-configurable value, the
[V4L2\_CID\_ALPHA\_COMPONENT](control.html#v4l2-alpha-component) control is used to
specify that alpha value, and the alpha component of all pixels will be set to
the value specified by that control. Otherwise a corresponding format without
an alpha component (XRGB or XBGR) must be used instead of an alpha format.

[Output](dev-output.html#output) devices (including output queues of mem-to-mem devices
and [video output overlay](dev-osd.html#osd) devices) read the alpha component from
memory. When the device processes the alpha channel the alpha component must be
filled with meaningful values by applications. Otherwise a corresponding format
without an alpha component (XRGB or XBGR) must be used instead of an alpha
format.

Formats that contain padding bits are named XRGB (or a permutation thereof).
The padding bits contain undefined values and must be ignored by applications,
devices and drivers, for both [Video Capture Interface](dev-capture.html#capture) and [Video Output Interface](dev-output.html#output) devices.

Note

* In all the tables that follow, bit 7 is the most significant bit in a byte.
* ‘r’, ‘g’ and ‘b’ denote bits of the red, green and blue components
  respectively. ‘a’ denotes bits of the alpha component (if supported by the
  format), and ‘x’ denotes padding bits.

## 2.5.1. Less Than 8 Bits Per Component

These formats store an RGB triplet in one, two or four bytes. They are named
based on the order of the RGB components as seen in a 8-, 16- or 32-bit word,
which is then stored in memory in little endian byte order (unless otherwise
noted by the presence of bit 31 in the 4CC value), and on the number of bits
for each component. For instance, the RGB565 format stores a pixel in a 16-bit
word [15:0] laid out at as [R4 R3 R2 R1
R0 G5 G4 G3 G2 G1
G0 B4 B3 B2 B1 B0], and
stored in memory in two bytes, [R4 R3 R2 R1
R0 G5 G4 G3] followed by [G2
G1 G0 B4 B3 B2 B1
B0].

RGB Formats With Less Than 8 Bits Per Component

| Identifier | Code | Byte 0 in memory | | | | | | | | Byte 1 | | | | | | | | Byte 2 | | | | | | | | Byte 3 | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| `V4L2_PIX_FMT_RGB332` | ‘RGB1’ | r2 | r1 | r0 | g2 | g1 | g0 | b1 | b0 |  | | | | | | | | | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_ARGB444` | ‘AR12’ | g3 | g2 | g1 | g0 | b3 | b2 | b1 | b0 | a3 | a2 | a1 | a0 | r3 | r2 | r1 | r0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_XRGB444` | ‘XR12’ | g3 | g2 | g1 | g0 | b3 | b2 | b1 | b0 | x | x | x | x | r3 | r2 | r1 | r0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGBA444` | ‘RA12’ | b3 | b2 | b1 | b0 | a3 | a2 | a1 | a0 | r3 | r2 | r1 | r0 | g3 | g2 | g1 | g0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGBX444` | ‘RX12’ | b3 | b2 | b1 | b0 | x | x | x | x | r3 | r2 | r1 | r0 | g3 | g2 | g1 | g0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_ABGR444` | ‘AB12’ | g3 | g2 | g1 | g0 | r3 | r2 | r1 | r0 | a3 | a2 | a1 | a0 | b3 | b2 | b1 | b0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_XBGR444` | ‘XB12’ | g3 | g2 | g1 | g0 | r3 | r2 | r1 | r0 | x | x | x | x | b3 | b2 | b1 | b0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_BGRA444` | ‘BA12’ | r3 | r2 | r1 | r0 | a3 | a2 | a1 | a0 | b3 | b2 | b1 | b0 | g3 | g2 | g1 | g0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_BGRX444` | ‘BX12’ | r3 | r2 | r1 | r0 | x | x | x | x | b3 | b2 | b1 | b0 | g3 | g2 | g1 | g0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_ARGB555` | ‘AR15’ | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 | a | r4 | r3 | r2 | r1 | r0 | g4 | g3 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_XRGB555` | ‘XR15’ | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 | x | r4 | r3 | r2 | r1 | r0 | g4 | g3 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGBA555` | ‘RA15’ | g1 | g0 | b4 | b3 | b2 | b1 | b0 | a | r4 | r3 | r2 | r1 | r0 | g4 | g3 | g2 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGBX555` | ‘RX15’ | g1 | g0 | b4 | b3 | b2 | b1 | b0 | x | r4 | r3 | r2 | r1 | r0 | g4 | g3 | g2 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_ABGR555` | ‘AB15’ | g2 | g1 | g0 | r4 | r3 | r2 | r1 | r0 | a | b4 | b3 | b2 | b1 | b0 | g4 | g3 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_XBGR555` | ‘XB15’ | g2 | g1 | g0 | r4 | r3 | r2 | r1 | r0 | x | b4 | b3 | b2 | b1 | b0 | g4 | g3 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_BGRA555` | ‘BA15’ | g1 | g0 | r4 | r3 | r2 | r1 | r0 | a | b4 | b3 | b2 | b1 | b0 | g4 | g3 | g2 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_BGRX555` | ‘BX15’ | g1 | g0 | r4 | r3 | r2 | r1 | r0 | x | b4 | b3 | b2 | b1 | b0 | g4 | g3 | g2 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGB565` | ‘RGBP’ | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_ARGB555X` | ‘AR15’ | (1 << 31) | a | r4 | r3 | r2 | r1 | r0 | g4 | g3 | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_XRGB555X` | ‘XR15’ | (1 << 31) | x | r4 | r3 | r2 | r1 | r0 | g4 | g3 | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGB565X` | ‘RGBR’ | r4 | r3 | r2 | r1 | r0 | g5 | g4 | g3 | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_BGR666` | ‘BGRH’ | b5 | b4 | b3 | b2 | b1 | b0 | g5 | g4 | g3 | g2 | g1 | g0 | r5 | r4 | r3 | r2 | r1 | r0 | x | x | x | x | x | x | x | x | x | x | x | x | x | x |

## 2.5.2. 8 Bits Per Component

These formats store an RGB triplet in three or four bytes. They are named based
on the order of the RGB components as stored in memory, and on the total number
of bits per pixel. For instance, RGB24 format stores a pixel with [R7
R6 R5 R4 R3 R2 R1
R0] in the first byte, [G7 G6 G5 G4
G3 G2 G1 G0] in the second byte and
[B7 B6 B5 B4 B3 B2
B1 B0] in the third byte. This differs from the DRM format
nomenclature that instead use the order of components as seen in a 24- or
32-bit little endian word.

RGB Formats With 8 Bits Per Component

| Identifier | Code | Byte 0 in memory | Byte 1 | Byte 2 | Byte 3 |
| --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_BGR24` | ‘BGR3’ | B7-0 | G7-0 | R7-0 |  |
| `V4L2_PIX_FMT_RGB24` | ‘RGB3’ | R7-0 | G7-0 | B7-0 |  |
| `V4L2_PIX_FMT_ABGR32` | ‘AR24’ | B7-0 | G7-0 | R7-0 | A7-0 |
| `V4L2_PIX_FMT_XBGR32` | ‘XR24’ | B7-0 | G7-0 | R7-0 | X7-0 |
| `V4L2_PIX_FMT_BGRA32` | ‘RA24’ | A7-0 | B7-0 | G7-0 | R7-0 |
| `V4L2_PIX_FMT_BGRX32` | ‘RX24’ | X7-0 | B7-0 | G7-0 | R7-0 |
| `V4L2_PIX_FMT_RGBA32` | ‘AB24’ | R7-0 | G7-0 | B7-0 | A7-0 |
| `V4L2_PIX_FMT_RGBX32` | ‘XB24’ | R7-0 | G7-0 | B7-0 | X7-0 |
| `V4L2_PIX_FMT_ARGB32` | ‘BA24’ | A7-0 | R7-0 | G7-0 | B7-0 |
| `V4L2_PIX_FMT_XRGB32` | ‘BX24’ | X7-0 | R7-0 | G7-0 | B7-0 |

## 2.5.3. 10 Bits Per Component

These formats store a 30-bit RGB triplet with an optional 2 bit alpha in four
bytes. They are named based on the order of the RGB components as seen in a
32-bit word, which is then stored in memory in little endian byte order
(unless otherwise noted by the presence of bit 31 in the 4CC value), and on the
number of bits for each component.

RGB Formats 10 Bits Per Color Component

| Identifier | Code | Byte 0 in memory | | | | | | | | Byte 1 | | | | | | | | Byte 2 | | | | | | | | Byte 3 | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| `V4L2_PIX_FMT_RGBX1010102` | ‘RX30’ | b5 | b4 | b3 | b2 | b1 | b0 | x | x | g3 | g2 | g1 | g0 | b9 | b8 | b7 | b6 | r1 | r0 | g9 | g8 | g7 | g6 | g5 | g4 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 |
| `V4L2_PIX_FMT_RGBA1010102` | ‘RA30’ | b5 | b4 | b3 | b2 | b1 | b0 | a1 | a0 | g3 | g2 | g1 | g0 | b9 | b8 | b7 | b6 | r1 | r0 | g9 | g8 | g7 | g6 | g5 | g4 | r9 | r8 | r7 | r6 | r5 | r4 | r3 | r2 |
| `V4L2_PIX_FMT_ARGB2101010` | ‘AR30’ | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | g5 | g4 | g3 | g2 | g1 | g0 | b9 | b8 | r3 | r2 | r1 | r0 | g9 | g8 | g7 | g6 | a1 | a0 | r9 | r8 | r7 | r6 | r5 | r4 |

## 2.5.4. 12 Bits Per Component

These formats store an RGB triplet in six or eight bytes, with 12 bits per component.
Expand the bits per component to 16 bits, data in the high bits, zeros in the low bits,
arranged in little endian order.

RGB Formats With 12 Bits Per Component

| Identifier | Code | Byte 1-0 | Byte 3-2 | Byte 5-4 | Byte 7-6 |
| --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_BGR48_12` | ‘B312’ | B15-4 | G15-4 | R15-4 |  |
| `V4L2_PIX_FMT_ABGR64_12` | ‘B412’ | B15-4 | G15-4 | R15-4 | A15-4 |

## 2.5.5. 16 Bits Per Component

These formats store an RGB triplet in six bytes, with 16 bits per component
stored in memory in little endian byte order. They are named based on the order
of the RGB components as stored in memory. For instance, RGB48 stores R7:0 and R15:8 in bytes 0 and 1 respectively. This differs from
the DRM format nomenclature that instead uses the order of components as seen in
the 48-bits little endian word.

RGB Formats With 16 Bits Per Component

| Identifier | Code | Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `V4L2_PIX_FMT_BGR48` | ‘BGR6’ | B7-0 | B15-8 | G7-0 | G15-8 | R7-0 | R15-8 |
| `V4L2_PIX_FMT_RGB48` | ‘RGB6’ | R7-0 | R15-8 | G7-0 | G15-8 | B7-0 | B15-8 |

## 2.5.6. Deprecated RGB Formats

Formats defined in [Deprecated Packed RGB Image Formats](#pixfmt-rgb-deprecated) are deprecated and must not be
used by new drivers. They are documented here for reference. The meaning of
their alpha bits `(a)` is ill-defined and they are interpreted as in either
the corresponding ARGB or XRGB format, depending on the driver.

Deprecated Packed RGB Image Formats

| Identifier | Code | Byte 0 in memory | | | | | | | | Byte 1 | | | | | | | | Byte 2 | | | | | | | | Byte 3 | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  |  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| `V4L2_PIX_FMT_RGB444` | ‘R444’ | g3 | g2 | g1 | g0 | b3 | b2 | b1 | b0 | a3 | a2 | a1 | a0 | r3 | r2 | r1 | r0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGB555` | ‘RGBO’ | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 | a | r4 | r3 | r2 | r1 | r0 | g4 | g3 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_RGB555X` | ‘RGBQ’ | a | r4 | r3 | r2 | r1 | r0 | g4 | g3 | g2 | g1 | g0 | b4 | b3 | b2 | b1 | b0 |  | | | | | | | | | | | | | | | |
| `V4L2_PIX_FMT_BGR32` | ‘BGR4’ | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | a7 | a6 | a5 | a4 | a3 | a2 | a1 | a0 |
| `V4L2_PIX_FMT_RGB32` | ‘RGB4’ | a7 | a6 | a5 | a4 | a3 | a2 | a1 | a0 | r7 | r6 | r5 | r4 | r3 | r2 | r1 | r0 | g7 | g6 | g5 | g4 | g3 | g2 | g1 | g0 | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |

A test utility to determine which RGB formats a driver actually supports
is available from the LinuxTV v4l-dvb repository. See
<https://linuxtv.org/repo/> for access
instructions.
