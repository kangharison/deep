# 2.7.1.2.Planar YUV formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-yuv-planar.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.2. Planar YUV formats

Planar formats split luma and chroma data in separate memory regions. They
exist in two variants:

* Semi-planar formats use two planes. The first plane is the luma plane and
  stores the Y components. The second plane is the chroma plane and stores the
  Cb and Cr components interleaved.
* Fully planar formats use three planes to store the Y, Cb and Cr components
  separately.

Within a plane, components are stored in pixel order, which may be linear or
tiled. Padding may be supported at the end of the lines, and the line stride of
the chroma planes may be constrained by the line stride of the luma plane.

Some planar formats allow planes to be placed in independent memory locations.
They are identified by an ‘M’ suffix in their name (such as in
`V4L2_PIX_FMT_NV12M`). Those formats are intended to be used only in drivers
and applications that support the multi-planar API, described in
[Single- and multi-planar APIs](planar-apis.html#planar-apis). Unless explicitly documented as supporting non-contiguous
planes, formats require the planes to follow each other immediately in memory.

## 2.7.1.2.1. Semi-Planar YUV Formats

These formats are commonly referred to as NV formats (NV12, NV16, ...). They
use two planes, and store the luma components in the first plane and the chroma
components in the second plane. The Cb and Cr components are interleaved in the
chroma plane, with Cb and Cr always stored in pairs. The chroma order is
exposed as different formats.

For memory contiguous formats, the number of padding pixels at the end of the
chroma lines is identical to the padding of the luma lines. Without horizontal
subsampling, the chroma line stride (in bytes) is thus equal to twice the luma
line stride. With horizontal subsampling by 2, the chroma line stride is equal
to the luma line stride. Vertical subsampling doesn’t affect the line stride.

For non-contiguous formats, no constraints are enforced by the format on the
relationship between the luma and chroma line padding and stride.

All components are stored with the same number of bits per component.

Overview of Semi-Planar YUV Formats

| Identifier | Code | Bits per component | Subsampling | Chroma order [[1]](#id4) | Contiguous [[2]](#id5) | Tiling [[3]](#id6) |
| --- | --- | --- | --- | --- | --- | --- |
| V4L2\_PIX\_FMT\_NV12 | ‘NV12’ | 8 | 4:2:0 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_NV21 | ‘NV21’ | 8 | 4:2:0 | Cr, Cb | Yes | Linear |
| V4L2\_PIX\_FMT\_NV12M | ‘NM12’ | 8 | 4:2:0 | Cb, Cr | No | Linear |
| V4L2\_PIX\_FMT\_NV21M | ‘NM21’ | 8 | 4:2:0 | Cr, Cb | No | Linear |
| V4L2\_PIX\_FMT\_NV12MT | ‘TM12’ | 8 | 4:2:0 | Cb, Cr | No | 64x32 tiles  Horizontal Z order |
| V4L2\_PIX\_FMT\_NV12MT\_16X16 | ‘VM12’ | 8 | 4:2:2 | Cb, Cr | No | 16x16 tiles |
| V4L2\_PIX\_FMT\_P010 | ‘P010’ | 10 | 4:2:0 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_P010\_4L4 | ‘T010’ | 10 | 4:2:0 | Cb, Cr | Yes | 4x4 tiles |
| V4L2\_PIX\_FMT\_P012 | ‘P012’ | 12 | 4:2:0 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_P012M | ‘PM12’ | 12 | 4:2:0 | Cb, Cr | No | Linear |
| V4L2\_PIX\_FMT\_NV15 | ‘NV15’ | 10 | 4:2:0 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_NV15\_4L4 | ‘VT15’ | 15 | 4:2:0 | Cb, Cr | Yes | 4x4 tiles |
| V4L2\_PIX\_FMT\_MT2110T | ‘MT2T’ | 15 | 4:2:0 | Cb, Cr | No | 16x32 / 16x16 tiles tiled low bits |
| V4L2\_PIX\_FMT\_MT2110R | ‘MT2R’ | 15 | 4:2:0 | Cb, Cr | No | 16x32 / 16x16 tiles raster low bits |
| V4L2\_PIX\_FMT\_NV16 | ‘NV16’ | 8 | 4:2:2 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_NV61 | ‘NV61’ | 8 | 4:2:2 | Cr, Cb | Yes | Linear |
| V4L2\_PIX\_FMT\_NV16M | ‘NM16’ | 8 | 4:2:2 | Cb, Cr | No | Linear |
| V4L2\_PIX\_FMT\_NV61M | ‘NM61’ | 8 | 4:2:2 | Cr, Cb | No | Linear |
| V4L2\_PIX\_FMT\_NV20 | ‘NV20’ | 10 | 4:2:2 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_NV24 | ‘NV24’ | 8 | 4:4:4 | Cb, Cr | Yes | Linear |
| V4L2\_PIX\_FMT\_NV42 | ‘NV42’ | 8 | 4:4:4 | Cr, Cb | Yes | Linear |

[[1](#id1)]

Order of chroma samples in the second plane


[[2](#id2)]

Indicates if planes have to be contiguous in memory or can be
disjoint


[[3](#id3)]

Macroblock size in pixels

**Color Sample Location:**
Chroma samples are [interstitially sited](yuv-formats.html#yuv-chroma-centered)
horizontally.

### 2.7.1.2.1.1. NV12, NV21, NV12M and NV21M

Semi-planar YUV 4:2:0 formats. The chroma plane is subsampled by 2 in each
direction. Chroma lines contain half the number of pixels and the same number
of bytes as luma lines, and the chroma plane contains half the number of lines
of the luma plane.

Sample 4x4 NV12 Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 16: | Cb00 | Cr00 | Cb01 | Cr01 |
| start + 20: | Cb10 | Cr10 | Cb11 | Cr11 |

Sample 4x4 NV12M Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start0 + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start0 + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start0 + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start0 + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
|  | | | | |
| start1 + 0: | Cb00 | Cr00 | Cb01 | Cr01 |
| start1 + 4: | Cb10 | Cr10 | Cb11 | Cr11 |

### 2.7.1.2.1.2. NV15

Semi-planar 10-bit YUV 4:2:0 format similar to NV12, using 10-bit components
with no padding between each component. A group of 4 components are stored over
5 bytes in little endian order.

Sample 4x4 NV15 Image (1 byte per cell)

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| start + 0: | Y’00[7:0] | Y’01[5:0]Y’00[9:8] | Y’02[3:0]Y’01[9:6] | Y’03[1:0]Y’02[9:4] | Y’03[9:2] |
| start + 5: | Y’10[7:0] | Y’11[5:0]Y’10[9:8] | Y’12[3:0]Y’11[9:6] | Y’13[1:0]Y’12[9:4] | Y’13[9:2] |
| start + 10: | Y’20[7:0] | Y’21[5:0]Y’20[9:8] | Y’22[3:0]Y’21[9:6] | Y’23[1:0]Y’22[9:4] | Y’23[9:2] |
| start + 15: | Y’30[7:0] | Y’31[5:0]Y’30[9:8] | Y’32[3:0]Y’31[9:6] | Y’33[1:0]Y’32[9:4] | Y’33[9:2] |
| start + 20: | Cb00[7:0] | Cr00[5:0]Cb00[9:8] | Cb01[3:0]Cr00[9:6] | Cr01[1:0]Cb01[9:4] | Cr01[9:2] |
| start + 25: | Cb10[7:0] | Cr10[5:0]Cb10[9:8] | Cb11[3:0]Cr10[9:6] | Cr11[1:0]Cb11[9:4] | Cr11[9:2] |

### 2.7.1.2.1.3. Tiled NV12

Semi-planar YUV 4:2:0 formats, using macroblock tiling. The chroma plane is
subsampled by 2 in each direction. Chroma lines contain half the number of
pixels and the same number of bytes as luma lines, and the chroma plane
contains half the number of lines of the luma plane. Each tile follows the
previous one linearly in memory (from left to right, top to bottom).

`V4L2_PIX_FMT_NV12MT_16X16` is similar to `V4L2_PIX_FMT_NV12M` but stores
pixels in 2D 16x16 tiles, and stores tiles linearly in memory.
The line stride and image height must be aligned to a multiple of 16.
The layouts of the luma and chroma planes are identical.

`V4L2_PIX_FMT_NV12MT` is similar to `V4L2_PIX_FMT_NV12M` but stores
pixels in 2D 64x32 tiles, and stores 2x2 groups of tiles in
Z-order in memory, alternating Z and mirrored Z shapes horizontally.
The line stride must be a multiple of 128 pixels to ensure an
integer number of Z shapes. The image height must be a multiple of 32 pixels.
If the vertical resolution is an odd number of tiles, the last row of
tiles is stored in linear order. The layouts of the luma and chroma
planes are identical.

![nv12mt.svg](../../../_images/nv12mt.svg)

V4L2\_PIX\_FMT\_NV12MT macroblock Z shape memory layout


![nv12mt_example.svg](../../../_images/nv12mt_example.svg)

Example V4L2\_PIX\_FMT\_NV12MT memory layout of tiles

`V4L2_PIX_FMT_NV12_4L4` stores pixels in 4x4 tiles, and stores
tiles linearly in memory. The line stride and image height must be
aligned to a multiple of 4. The layouts of the luma and chroma planes are
identical.

`V4L2_PIX_FMT_NV12_16L16` stores pixels in 16x16 tiles, and stores
tiles linearly in memory. The line stride and image height must be
aligned to a multiple of 16. The layouts of the luma and chroma planes are
identical.

`V4L2_PIX_FMT_NV12_32L32` stores pixels in 32x32 tiles, and stores
tiles linearly in memory. The line stride and image height must be
aligned to a multiple of 32. The layouts of the luma and chroma planes are
identical.

`V4L2_PIX_FMT_NV12M_8L128` is similar to `V4L2_PIX_FMT_NV12M` but stores
pixels in 2D 8x128 tiles, and stores tiles linearly in memory.
The image height must be aligned to a multiple of 128.
The layouts of the luma and chroma planes are identical.

`V4L2_PIX_FMT_NV12_8L128` is similar to `V4L2_PIX_FMT_NV12M_8L128` but stores
two planes in one memory.

`V4L2_PIX_FMT_MM21` store luma pixel in 16x32 tiles, and chroma pixels
in 16x16 tiles. The line stride must be aligned to a multiple of 16 and the
image height must be aligned to a multiple of 32. The number of luma and chroma
tiles are identical, even though the tile size differ. The image is formed of
two non-contiguous planes.

### 2.7.1.2.1.4. Tiled NV15

`V4L2_PIX_FMT_NV15_4L4` Semi-planar 10-bit YUV 4:2:0 formats, using 4x4 tiling.
All components are packed without any padding between each other.
As a side-effect, each group of 4 components are stored over 5 bytes
(YYYY or UVUV = 4 \* 10 bits = 40 bits = 5 bytes).

`V4L2_PIX_FMT_NV12M_10BE_8L128` is similar to `V4L2_PIX_FMT_NV12M` but stores
10 bits pixels in 2D 8x128 tiles, and stores tiles linearly in memory.
the data is arranged in big endian order.
The image height must be aligned to a multiple of 128.
The layouts of the luma and chroma planes are identical.
Note the tile size is 8bytes multiplied by 128 bytes,
it means that the low bits and high bits of one pixel may be in different tiles.
The 10 bit pixels are packed, so 5 bytes contain 4 10-bit pixels layout like
this (for luma):
byte 0: Y0(bits 9-2)
byte 1: Y0(bits 1-0) Y1(bits 9-4)
byte 2: Y1(bits 3-0) Y2(bits 9-6)
byte 3: Y2(bits 5-0) Y3(bits 9-8)
byte 4: Y3(bits 7-0)

`V4L2_PIX_FMT_NV12_10BE_8L128` is similar to `V4L2_PIX_FMT_NV12M_10BE_8L128` but stores
two planes in one memory.

`V4L2_PIX_FMT_MT2110T` is one of Mediatek packed 10bit YUV 4:2:0 formats.
It is fully packed 10bit 4:2:0 format like NV15 (15 bits per pixel), except
that the lower two bits data is stored in separate partitions. The format is
composed of 16x32 luma tiles, and 16x16 chroma tiles. Each tiles is 640 bytes
long, divided into 8 partitions of 80 bytes. The first 16 bytes of the
partition represent the 2 least significant bits of pixel data. The remaining
64 bytes represent the 8 most significant bits of pixel data.

![mt2110t.svg](../../../_images/mt2110t.svg)

Layout of MT2110T Chroma Tile

Filtering out the upper part of each partitions results in a valid
`V4L2_PIX_FMT_MM21` frame. A partition is a sub-tile of size 16 x 4. The
lower two bits is said to be tiled since each bytes contains the lower two
bits of the column of for pixel matching the same index. The chroma tiles
only have 4 partitions.

MT2110T LSB bits layout

|  | start + 0: | start + 1: | . . . | start+15: |
| --- | --- | --- | --- | --- |
| Bits 1:0 | Y’0:0 | Y’0:1 | . . . | Y’0:15 |
| Bit 3:2 | Y’1:0 | Y’1:1 | . . . | Y’1:15 |
| Bits 5:4 | Y’2:0 | Y’2:1 | . . . | Y’2:15 |
| Bits 7:6 | Y’3:0 | Y’3:1 | . . . | Y’3:15 |

`V4L2_PIX_FMT_MT2110R` is identical to `V4L2_PIX_FMT_MT2110T` except that
the least significant two bits layout is in raster order. This means the first byte
contains 4 pixels of the first row, with 4 bytes per line.

MT2110R LSB bits layout

|  | Byte 0 | | | | ... | Byte 3 | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|  | 7:6 | 5:4 | 3:2 | 1:0 | ... | 7:6 | 5:4 | 3:2 | 1:0 |
| start + 0: | Y’0:3 | Y’0:2 | Y’0:1 | Y’0:0 | ... | Y’0:15 | Y’0:14 | Y’0:13 | Y’0:12 |
| start + 4: | Y’1:3 | Y’1:2 | Y’1:1 | Y’1:0 | ... | Y’1:15 | Y’1:14 | Y’1:13 | Y’1:12 |
| start + 8: | Y’2:3 | Y’2:2 | Y’2:1 | Y’2:0 | ... | Y’2:15 | Y’2:14 | Y’2:13 | Y’2:12 |
| start+12: | Y’3:3 | Y’3:2 | Y’3:1 | Y’3:0 | ... | Y’3:15 | Y’3:14 | Y’3:13 | Y’3:12 |

### 2.7.1.2.1.5. NV16, NV61, NV16M and NV61M

Semi-planar YUV 4:2:2 formats. The chroma plane is subsampled by 2 in the
horizontal direction. Chroma lines contain half the number of pixels and the
same number of bytes as luma lines, and the chroma plane contains the same
number of lines as the luma plane.

Sample 4x4 NV16 Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 16: | Cb00 | Cr00 | Cb01 | Cr01 |
| start + 20: | Cb10 | Cr10 | Cb11 | Cr11 |
| start + 24: | Cb20 | Cr20 | Cb21 | Cr21 |
| start + 28: | Cb30 | Cr30 | Cb31 | Cr31 |

Sample 4x4 NV16M Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start0 + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start0 + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start0 + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start0 + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
|  | | | | |
| start1 + 0: | Cb00 | Cr00 | Cb02 | Cr02 |
| start1 + 4: | Cb10 | Cr10 | Cb12 | Cr12 |
| start1 + 8: | Cb20 | Cr20 | Cb22 | Cr22 |
| start1 + 12: | Cb30 | Cr30 | Cb32 | Cr32 |

### 2.7.1.2.1.6. NV20

Semi-planar 10-bit YUV 4:2:2 format similar to NV16, using 10-bit components
with no padding between each component. A group of 4 components are stored over
5 bytes in little endian order.

Sample 4x4 NV20 Image (1 byte per cell)

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| start + 0: | Y’00[7:0] | Y’01[5:0]Y’00[9:8] | Y’02[3:0]Y’01[9:6] | Y’03[1:0]Y’02[9:4] | Y’03[9:2] |
| start + 5: | Y’10[7:0] | Y’11[5:0]Y’10[9:8] | Y’12[3:0]Y’11[9:6] | Y’13[1:0]Y’12[9:4] | Y’13[9:2] |
| start + 10: | Y’20[7:0] | Y’21[5:0]Y’20[9:8] | Y’22[3:0]Y’21[9:6] | Y’23[1:0]Y’22[9:4] | Y’23[9:2] |
| start + 15: | Y’30[7:0] | Y’31[5:0]Y’30[9:8] | Y’32[3:0]Y’31[9:6] | Y’33[1:0]Y’32[9:4] | Y’33[9:2] |
| start + 20: | Cb00[7:0] | Cr00[5:0]Cb00[9:8] | Cb01[3:0]Cr00[9:6] | Cr01[1:0]Cb01[9:4] | Cr01[9:2] |
| start + 25: | Cb10[7:0] | Cr10[5:0]Cb10[9:8] | Cb11[3:0]Cr10[9:6] | Cr11[1:0]Cb11[9:4] | Cr11[9:2] |
| start + 30: | Cb20[7:0] | Cr20[5:0]Cb20[9:8] | Cb21[3:0]Cr20[9:6] | Cr21[1:0]Cb21[9:4] | Cr21[9:2] |
| start + 35: | Cb30[7:0] | Cr30[5:0]Cb30[9:8] | Cb31[3:0]Cr30[9:6] | Cr31[1:0]Cb31[9:4] | Cr31[9:2] |

### 2.7.1.2.1.7. NV24 and NV42

Semi-planar YUV 4:4:4 formats. The chroma plane is not subsampled.
Chroma lines contain the same number of pixels and twice the
number of bytes as luma lines, and the chroma plane contains the same
number of lines as the luma plane.

Sample 4x4 NV24 Image

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 | | | | |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 | | | | |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 | | | | |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 | | | | |
| start + 16: | Cb00 | Cr00 | Cb01 | Cr01 | Cb02 | Cr02 | Cb03 | Cr03 |
| start + 24: | Cb10 | Cr10 | Cb11 | Cr11 | Cb12 | Cr12 | Cb13 | Cr13 |
| start + 32: | Cb20 | Cr20 | Cb21 | Cr21 | Cb22 | Cr22 | Cb23 | Cr23 |
| start + 40: | Cb30 | Cr30 | Cb31 | Cr31 | Cb32 | Cr32 | Cb33 | Cr33 |

### 2.7.1.2.1.8. P010 and tiled P010

P010 is like NV12 with 10 bits per component, expanded to 16 bits.
Data in the 10 high bits, zeros in the 6 low bits, arranged in little endian order.

Sample 4x4 P010 Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 8: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 16: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 24: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 32: | Cb00 | Cr00 | Cb01 | Cr01 |
| start + 40: | Cb10 | Cr10 | Cb11 | Cr11 |

### 2.7.1.2.1.9. P012 and P012M

P012 is like NV12 with 12 bits per component, expanded to 16 bits.
Data in the 12 high bits, zeros in the 4 low bits, arranged in little endian order.

Sample 4x4 P012 Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 8: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 16: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 24: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 32: | Cb00 | Cr00 | Cb01 | Cr01 |
| start + 40: | Cb10 | Cr10 | Cb11 | Cr11 |

Sample 4x4 P012M Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start0 + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start0 + 8: | Y’10 | Y’11 | Y’12 | Y’13 |
| start0 + 16: | Y’20 | Y’21 | Y’22 | Y’23 |
| start0 + 24: | Y’30 | Y’31 | Y’32 | Y’33 |
|  | | | | |
| start1 + 0: | Cb00 | Cr00 | Cb01 | Cr01 |
| start1 + 8: | Cb10 | Cr10 | Cb11 | Cr11 |

## 2.7.1.2.2. Fully Planar YUV Formats

These formats store the Y, Cb and Cr components in three separate planes. The
luma plane comes first, and the order of the two chroma planes varies between
formats. The two chroma planes always use the same subsampling.

For memory contiguous formats, the number of padding pixels at the end of the
chroma lines is identical to the padding of the luma lines. The chroma line
stride (in bytes) is thus equal to the luma line stride divided by the
horizontal subsampling factor. Vertical subsampling doesn’t affect the line
stride.

For non-contiguous formats, no constraints are enforced by the format on the
relationship between the luma and chroma line padding and stride.

All components are stored with the same number of bits per component.

`V4L2_PIX_FMT_P010_4L4` stores pixels in 4x4 tiles, and stores tiles linearly
in memory. The line stride must be aligned to multiple of 8 and image height to
a multiple of 4. The layouts of the luma and chroma planes are identical.

Overview of Fully Planar YUV Formats

| Identifier | Code | Bits per component | Subsampling | Planes order [[4]](#id10) | Contiguous [[5]](#id11) |
| --- | --- | --- | --- | --- | --- |
| V4L2\_PIX\_FMT\_YUV410 | ‘YUV9’ | 8 | 4:1:0 | Y, Cb, Cr | Yes |
| V4L2\_PIX\_FMT\_YVU410 | ‘YVU9’ | 8 | 4:1:0 | Y, Cr, Cb | Yes |
| V4L2\_PIX\_FMT\_YUV411P | ‘411P’ | 8 | 4:1:1 | Y, Cb, Cr | Yes |
| V4L2\_PIX\_FMT\_YUV420M | ‘YM12’ | 8 | 4:2:0 | Y, Cb, Cr | No |
| V4L2\_PIX\_FMT\_YVU420M | ‘YM21’ | 8 | 4:2:0 | Y, Cr, Cb | No |
| V4L2\_PIX\_FMT\_YUV420 | ‘YU12’ | 8 | 4:2:0 | Y, Cb, Cr | Yes |
| V4L2\_PIX\_FMT\_YVU420 | ‘YV12’ | 8 | 4:2:0 | Y, Cr, Cb | Yes |
| V4L2\_PIX\_FMT\_YUV422P | ‘422P’ | 8 | 4:2:2 | Y, Cb, Cr | Yes |
| V4L2\_PIX\_FMT\_YUV422M | ‘YM16’ | 8 | 4:2:2 | Y, Cb, Cr | No |
| V4L2\_PIX\_FMT\_YVU422M | ‘YM61’ | 8 | 4:2:2 | Y, Cr, Cb | No |
| V4L2\_PIX\_FMT\_YUV444M | ‘YM24’ | 8 | 4:4:4 | Y, Cb, Cr | No |
| V4L2\_PIX\_FMT\_YVU444M | ‘YM42’ | 8 | 4:4:4 | Y, Cr, Cb | No |

[[4](#id8)]

Order of luma and chroma planes


[[5](#id9)]

Indicates if planes have to be contiguous in memory or can be
disjoint

**Color Sample Location:**
Chroma samples are [interstitially sited](yuv-formats.html#yuv-chroma-centered)
horizontally.

### 2.7.1.2.2.1. YUV410 and YVU410

Planar YUV 4:1:0 formats. The chroma planes are subsampled by 4 in each
direction. Chroma lines contain a quarter of the number of pixels and bytes of
the luma lines, and the chroma planes contain a quarter of the number of lines
of the luma plane.

Sample 4x4 YUV410 Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 16: | Cr00 | | | |
| start + 17: | Cb00 | | | |

### 2.7.1.2.2.2. YUV411P

Planar YUV 4:1:1 formats. The chroma planes are subsampled by 4 in the
horizontal direction. Chroma lines contain a quarter of the number of pixels
and bytes of the luma lines, and the chroma planes contain the same number of
lines as the luma plane.

Sample 4x4 YUV411P Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 16: | Cb00 | | | |
| start + 17: | Cb10 | | | |
| start + 18: | Cb20 | | | |
| start + 19: | Cb30 | | | |
| start + 20: | Cr00 | | | |
| start + 21: | Cr10 | | | |
| start + 22: | Cr20 | | | |
| start + 23: | Cr30 | | | |

### 2.7.1.2.2.3. YUV420, YVU420, YUV420M and YVU420M

Planar YUV 4:2:0 formats. The chroma planes are subsampled by 2 in each
direction. Chroma lines contain half of the number of pixels and bytes of the
luma lines, and the chroma planes contain half of the number of lines of the
luma plane.

Sample 4x4 YUV420 Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 16: | Cr00 | Cr01 | | |
| start + 18: | Cr10 | Cr11 | | |
| start + 20: | Cb00 | Cb01 | | |
| start + 22: | Cb10 | Cb11 | | |

Sample 4x4 YUV420M Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start0 + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start0 + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start0 + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start0 + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
|  | | | | |
| start1 + 0: | Cb00 | Cb01 | | |
| start1 + 2: | Cb10 | Cb11 | | |
|  | | | | |
| start2 + 0: | Cr00 | Cr01 | | |
| start2 + 2: | Cr10 | Cr11 | | |

### 2.7.1.2.2.4. YUV422P, YUV422M and YVU422M

Planar YUV 4:2:2 formats. The chroma planes are subsampled by 2 in the
horizontal direction. Chroma lines contain half of the number of pixels and
bytes of the luma lines, and the chroma planes contain the same number of lines
as the luma plane.

Sample 4x4 YUV422P Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
| start + 16: | Cb00 | Cb01 | | |
| start + 18: | Cb10 | Cb11 | | |
| start + 20: | Cb20 | Cb21 | | |
| start + 22: | Cb30 | Cb31 | | |
| start + 24: | Cr00 | Cr01 | | |
| start + 26: | Cr10 | Cr11 | | |
| start + 28: | Cr20 | Cr21 | | |
| start + 30: | Cr30 | Cr31 | | |

Sample 4x4 YUV422M Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start0 + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start0 + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start0 + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start0 + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
|  | | | | |
| start1 + 0: | Cb00 | Cb01 | | |
| start1 + 2: | Cb10 | Cb11 | | |
| start1 + 4: | Cb20 | Cb21 | | |
| start1 + 6: | Cb30 | Cb31 | | |
|  | | | | |
| start2 + 0: | Cr00 | Cr01 | | |
| start2 + 2: | Cr10 | Cr11 | | |
| start2 + 4: | Cr20 | Cr21 | | |
| start2 + 6: | Cr30 | Cr31 | | |

### 2.7.1.2.2.5. YUV444M and YVU444M

Planar YUV 4:4:4 formats. The chroma planes are no subsampled. Chroma lines
contain the same number of pixels and bytes of the luma lines, and the chroma
planes contain the same number of lines as the luma plane.

Sample 4x4 YUV444M Image

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start0 + 0: | Y’00 | Y’01 | Y’02 | Y’03 |
| start0 + 4: | Y’10 | Y’11 | Y’12 | Y’13 |
| start0 + 8: | Y’20 | Y’21 | Y’22 | Y’23 |
| start0 + 12: | Y’30 | Y’31 | Y’32 | Y’33 |
|  | | | | |
| start1 + 0: | Cb00 | Cb01 | Cb02 | Cb03 |
| start1 + 4: | Cb10 | Cb11 | Cb12 | Cb13 |
| start1 + 8: | Cb20 | Cb21 | Cb22 | Cb23 |
| start1 + 12: | Cb20 | Cb21 | Cb32 | Cb33 |
|  | | | | |
| start2 + 0: | Cr00 | Cr01 | Cr02 | Cr03 |
| start2 + 4: | Cr10 | Cr11 | Cr12 | Cr13 |
| start2 + 8: | Cr20 | Cr21 | Cr22 | Cr23 |
| start2 + 12: | Cr30 | Cr31 | Cr32 | Cr33 |
