# 2.6.1.10.V4L2_PIX_FMT_SRGGB12P (‘pRCC’), V4L2_PIX_FMT_SGRBG12P (‘pgCC’), V4L2_PIX_FMT_SGBRG12P (‘pGCC’), V4L2_PIX_FMT_SBGGR12P (‘pBCC’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb12p.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.10. V4L2\_PIX\_FMT\_SRGGB12P (‘pRCC’), V4L2\_PIX\_FMT\_SGRBG12P (‘pgCC’), V4L2\_PIX\_FMT\_SGBRG12P (‘pGCC’), V4L2\_PIX\_FMT\_SBGGR12P (‘pBCC’)

## 2.6.1.10.1. 12-bit packed Bayer formats

### 2.6.1.10.1.1. Description

These four pixel formats are packed raw sRGB / Bayer formats with 12
bits per colour. Every two consecutive samples are packed into three
bytes. Each of the first two bytes contain the 8 high order bits of
the pixels, and the third byte contains the four least significant
bits of each pixel, in the same order.

Each n-pixel row contains n/2 green samples and n/2 blue or red
samples, with alternating green-red and green-blue rows. They are
conventionally described as GRGR... BGBG..., RGRG... GBGB..., etc.
Below is an example of a small V4L2\_PIX\_FMT\_SBGGR12P image:

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- |
| start + 0: | B00high | G01high | G01low(bits 7--4)  B00low(bits 3--0) | B02high | G03high | G03low(bits 7--4)  B02low(bits 3--0) |
| start + 6: | G10high | R11high | R11low(bits 7--4)  G10low(bits 3--0) | G12high | R13high | R13low(bits 7--4)  G12low(bits 3--0) |
| start + 12: | B20high | G21high | G21low(bits 7--4)  B20low(bits 3--0) | B22high | G23high | G23low(bits 7--4)  B22low(bits 3--0) |
| start + 18: | G30high | R31high | R31low(bits 7--4)  G30low(bits 3--0) | G32high | R33high | R33low(bits 7--4)  G32low(bits 3--0) |
