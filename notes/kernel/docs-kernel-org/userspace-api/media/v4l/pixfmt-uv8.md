# 2.7.1.7.V4L2_PIX_FMT_UV8 (‘UV8’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-uv8.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.7. V4L2\_PIX\_FMT\_UV8 (‘UV8’)

UV plane interleaved

## 2.7.1.7.1. Description

In this format there is no Y plane, Only CbCr plane. ie (UV interleaved)

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | Cb00 | Cr00 | Cb01 | Cr01 |
| start + 4: | Cb10 | Cr10 | Cb11 | Cr11 |
| start + 8: | Cb20 | Cr20 | Cb21 | Cr21 |
| start + 12: | Cb30 | Cr30 | Cb31 | Cr31 |
