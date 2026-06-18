# 2.6.1.12.V4L2_PIX_FMT_SRGGB14P (‘pREE’), V4L2_PIX_FMT_SGRBG14P (‘pgEE’), V4L2_PIX_FMT_SGBRG14P (‘pGEE’), V4L2_PIX_FMT_SBGGR14P (‘pBEE’),

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb14p.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.12. V4L2\_PIX\_FMT\_SRGGB14P (‘pREE’), V4L2\_PIX\_FMT\_SGRBG14P (‘pgEE’), V4L2\_PIX\_FMT\_SGBRG14P (‘pGEE’), V4L2\_PIX\_FMT\_SBGGR14P (‘pBEE’),

*man V4L2\_PIX\_FMT\_SRGGB14P(2)*

V4L2\_PIX\_FMT\_SGRBG14P
V4L2\_PIX\_FMT\_SGBRG14P
V4L2\_PIX\_FMT\_SBGGR14P
14-bit packed Bayer formats

## 2.6.1.12.1. Description

These four pixel formats are packed raw sRGB / Bayer formats with 14
bits per colour. Every four consecutive samples are packed into seven
bytes. Each of the first four bytes contain the eight high order bits
of the pixels, and the three following bytes contains the six least
significant bits of each pixel, in the same order.

Each n-pixel row contains n/2 green samples and n/2 blue or red samples,
with alternating green-red and green-blue rows. They are conventionally
described as GRGR... BGBG..., RGRG... GBGB..., etc. Below is an example
of one of these formats:

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0 | B00high | G01high | B02high | G03high | G01low bits 1--0(bits 7--6)  B00low bits 5--0(bits 5--0) | B02low bits 3--0(bits 7--4)  G01low bits 5--2(bits 3--0) | G03low bits 5--0(bits 7--2)  B02low bits 5--4(bits 1--0) |
| start + 7 | G10high | R11high | G12high | R13high | R11low bits 1--0(bits 7--6)  G10low bits 5--0(bits 5--0) | G12low bits 3--0(bits 7--4)  R11low bits 5--2(bits 3--0) | R13low bits 5--0(bits 7--2)  G12low bits 5--4(bits 1--0) |
| start + 14 | B20high | G21high | B22high | G23high | G21low bits 1--0(bits 7--6)  B20low bits 5--0(bits 5--0) | B22low bits 3--0(bits 7--4)  G21low bits 5--2(bits 3--0) | G23low bits 5--0(bits 7--2)  B22low bits 5--4(bits 1--0) |
| start + 21 | G30high | R31high | G32high | R33high | R31low bits 1--0(bits 7--6) G30low bits 5--0(bits 5--0) | G32low bits 3--0(bits 7--4) R31low bits 5--2(bits 3--0) | R33low bits 5--0(bits 7--2) G32low bits 5--4(bits 1--0) |
