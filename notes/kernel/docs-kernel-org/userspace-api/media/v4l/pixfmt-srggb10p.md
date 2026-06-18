# 2.6.1.5.V4L2_PIX_FMT_SRGGB10P (‘pRAA’), V4L2_PIX_FMT_SGRBG10P (‘pgAA’), V4L2_PIX_FMT_SGBRG10P (‘pGAA’), V4L2_PIX_FMT_SBGGR10P (‘pBAA’),

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb10p.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.5. V4L2\_PIX\_FMT\_SRGGB10P (‘pRAA’), V4L2\_PIX\_FMT\_SGRBG10P (‘pgAA’), V4L2\_PIX\_FMT\_SGBRG10P (‘pGAA’), V4L2\_PIX\_FMT\_SBGGR10P (‘pBAA’),

V4L2\_PIX\_FMT\_SGRBG10P
V4L2\_PIX\_FMT\_SGBRG10P
V4L2\_PIX\_FMT\_SBGGR10P
10-bit packed Bayer formats

## 2.6.1.5.1. Description

These four pixel formats are packed raw sRGB / Bayer formats with 10
bits per sample. Every four consecutive samples are packed into 5
bytes. Each of the first 4 bytes contain the 8 high order bits
of the pixels, and the 5th byte contains the 2 least significants
bits of each pixel, in the same order.

Each n-pixel row contains n/2 green samples and n/2 blue or red samples,
with alternating green-red and green-blue rows. They are conventionally
described as GRGR... BGBG..., RGRG... GBGB..., etc. Below is an example
of a small V4L2\_PIX\_FMT\_SBGGR10P image:

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| start + 0: | B00high | G01high | B02high | G03high | G03low(bits 7--6) B02low(bits 5--4)  G01low(bits 3--2) B00low(bits 1--0) |
| start + 5: | G10high | R11high | G12high | R13high | R13low(bits 7--6) G12low(bits 5--4)  R11low(bits 3--2) G10low(bits 1--0) |
| start + 10: | B20high | G21high | B22high | G23high | G23low(bits 7--6) B22low(bits 5--4)  G21low(bits 3--2) B20low(bits 1--0) |
| start + 15: | G30high | R31high | G32high | R33high | R33low(bits 7--6) G32low(bits 5--4)  R31low(bits 3--2) G30low(bits 1--0) |
