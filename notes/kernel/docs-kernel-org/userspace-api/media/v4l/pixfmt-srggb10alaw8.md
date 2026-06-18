# 2.6.1.6.V4L2_PIX_FMT_SBGGR10ALAW8 (‘aBA8’), V4L2_PIX_FMT_SGBRG10ALAW8 (‘aGA8’), V4L2_PIX_FMT_SGRBG10ALAW8 (‘agA8’), V4L2_PIX_FMT_SRGGB10ALAW8 (‘aRA8’),

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb10alaw8.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.6. V4L2\_PIX\_FMT\_SBGGR10ALAW8 (‘aBA8’), V4L2\_PIX\_FMT\_SGBRG10ALAW8 (‘aGA8’), V4L2\_PIX\_FMT\_SGRBG10ALAW8 (‘agA8’), V4L2\_PIX\_FMT\_SRGGB10ALAW8 (‘aRA8’),

V4L2\_PIX\_FMT\_SGBRG10ALAW8
V4L2\_PIX\_FMT\_SGRBG10ALAW8
V4L2\_PIX\_FMT\_SRGGB10ALAW8
10-bit Bayer formats compressed to 8 bits

## 2.6.1.6.1. Description

These four pixel formats are raw sRGB / Bayer formats with 10 bits per
color compressed to 8 bits each, using the A-LAW algorithm. Each color
component consumes 8 bits of memory. In other respects this format is
similar to [V4L2\_PIX\_FMT\_SRGGB8 (‘RGGB’), V4L2\_PIX\_FMT\_SGRBG8 (‘GRBG’), V4L2\_PIX\_FMT\_SGBRG8 (‘GBRG’), V4L2\_PIX\_FMT\_SBGGR8 (‘BA81’),](pixfmt-srggb8.html#v4l2-pix-fmt-srggb8).
