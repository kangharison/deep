# 2.6.1.7.V4L2_PIX_FMT_SBGGR10DPCM8 (‘bBA8’), V4L2_PIX_FMT_SGBRG10DPCM8 (‘bGA8’), V4L2_PIX_FMT_SGRBG10DPCM8 (‘BD10’), V4L2_PIX_FMT_SRGGB10DPCM8 (‘bRA8’),

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb10dpcm8.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.7. V4L2\_PIX\_FMT\_SBGGR10DPCM8 (‘bBA8’), V4L2\_PIX\_FMT\_SGBRG10DPCM8 (‘bGA8’), V4L2\_PIX\_FMT\_SGRBG10DPCM8 (‘BD10’), V4L2\_PIX\_FMT\_SRGGB10DPCM8 (‘bRA8’),

*man V4L2\_PIX\_FMT\_SBGGR10DPCM8(2)*

V4L2\_PIX\_FMT\_SGBRG10DPCM8
V4L2\_PIX\_FMT\_SGRBG10DPCM8
V4L2\_PIX\_FMT\_SRGGB10DPCM8
10-bit Bayer formats compressed to 8 bits

## 2.6.1.7.1. Description

These four pixel formats are raw sRGB / Bayer formats with 10 bits per
colour compressed to 8 bits each, using DPCM compression. DPCM,
differential pulse-code modulation, is lossy. Each colour component
consumes 8 bits of memory. In other respects this format is similar to
[V4L2\_PIX\_FMT\_SRGGB10 (‘RG10’), V4L2\_PIX\_FMT\_SGRBG10 (‘BA10’), V4L2\_PIX\_FMT\_SGBRG10 (‘GB10’), V4L2\_PIX\_FMT\_SBGGR10 (‘BG10’),](pixfmt-srggb10.html#v4l2-pix-fmt-srggb10).
