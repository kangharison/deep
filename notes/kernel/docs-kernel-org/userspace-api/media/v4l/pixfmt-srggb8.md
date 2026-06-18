# 2.6.1.2.V4L2_PIX_FMT_SRGGB8 (‘RGGB’), V4L2_PIX_FMT_SGRBG8 (‘GRBG’), V4L2_PIX_FMT_SGBRG8 (‘GBRG’), V4L2_PIX_FMT_SBGGR8 (‘BA81’),

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb8.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.2. V4L2\_PIX\_FMT\_SRGGB8 (‘RGGB’), V4L2\_PIX\_FMT\_SGRBG8 (‘GRBG’), V4L2\_PIX\_FMT\_SGBRG8 (‘GBRG’), V4L2\_PIX\_FMT\_SBGGR8 (‘BA81’),

## 2.6.1.2.1. 8-bit Bayer formats

### 2.6.1.2.1.1. Description

These four pixel formats are raw sRGB / Bayer formats with 8 bits per
sample. Each sample is stored in a byte. Each n-pixel row contains n/2
green samples and n/2 blue or red samples, with alternating red and
blue rows. They are conventionally described as GRGR... BGBG...,
RGRG... GBGB..., etc. Below is an example of a small V4L2\_PIX\_FMT\_SBGGR8 image:

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | B00 | G01 | B02 | G03 |
| start + 4: | G10 | R11 | G12 | R13 |
| start + 8: | B20 | G21 | B22 | G23 |
| start + 12: | G30 | R31 | G32 | R33 |
