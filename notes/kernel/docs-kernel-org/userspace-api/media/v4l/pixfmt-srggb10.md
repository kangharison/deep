# 2.6.1.4.V4L2_PIX_FMT_SRGGB10 (‘RG10’), V4L2_PIX_FMT_SGRBG10 (‘BA10’), V4L2_PIX_FMT_SGBRG10 (‘GB10’), V4L2_PIX_FMT_SBGGR10 (‘BG10’),

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb10.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.4. V4L2\_PIX\_FMT\_SRGGB10 (‘RG10’), V4L2\_PIX\_FMT\_SGRBG10 (‘BA10’), V4L2\_PIX\_FMT\_SGBRG10 (‘GB10’), V4L2\_PIX\_FMT\_SBGGR10 (‘BG10’),

V4L2\_PIX\_FMT\_SGRBG10
V4L2\_PIX\_FMT\_SGBRG10
V4L2\_PIX\_FMT\_SBGGR10
10-bit Bayer formats expanded to 16 bits

## 2.6.1.4.1. Description

These four pixel formats are raw sRGB / Bayer formats with 10 bits per
sample. Each sample is stored in a 16-bit word, with 6 unused
high bits filled with zeros. Each n-pixel row contains n/2 green samples and
n/2 blue or red samples, with alternating red and blue rows. Bytes are
stored in memory in little endian order. They are conventionally described
as GRGR... BGBG..., RGRG... GBGB..., etc. Below is an example of one of
these formats:

**Byte Order.**
Each cell is one byte, the 6 most significant bits in the high bytes
are 0.

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0: | B00low | B00high | G01low | G01high | B02low | B02high | G03low | G03high |
| start + 8: | G10low | G10high | R11low | R11high | G12low | G12high | R13low | R13high |
| start + 16: | B20low | B20high | G21low | G21high | B22low | B22high | G23low | G23high |
| start + 24: | G30low | G30high | R31low | R31high | G32low | G32high | R33low | R33high |
