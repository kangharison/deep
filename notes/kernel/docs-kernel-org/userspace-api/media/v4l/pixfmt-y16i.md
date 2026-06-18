# 2.7.1.6.V4L2_PIX_FMT_Y16I (‘Y16I’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-y16i.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.6. V4L2\_PIX\_FMT\_Y16I (‘Y16I’)

Interleaved grey-scale image, e.g. from a stereo-pair

## 2.7.1.6.1. Description

This is a grey-scale image with a depth of 16 bits per pixel, but with pixels
from 2 sources interleaved and unpacked. Each pixel is stored in a 16-bit word
in the little-endian order. The first pixel is from the left source.

**Pixel unpacked representation.**
Left/Right pixels 16-bit unpacked - 16-bit for each interleaved pixel.

|  |  |  |  |
| --- | --- | --- | --- |
| Y’0L[7:0] | Y’0L[15:8] | Y’0R[7:0] | Y’0R[15:8] |

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0: | Y’00Llow | Y’00Lhigh | Y’00Rlow | Y’00Rhigh | Y’01Llow | Y’01Lhigh | Y’01Rlow | Y’01Rhigh |
| start + 8: | Y’10Llow | Y’10Lhigh | Y’10Rlow | Y’10Rhigh | Y’11Llow | Y’11Lhigh | Y’11Rlow | Y’11Rhigh |
| start + 16: | Y’20Llow | Y’20Lhigh | Y’20Rlow | Y’20Rhigh | Y’21Llow | Y’21Lhigh | Y’21Rlow | Y’21Rhigh |
| start + 24: | Y’30Llow | Y’30Lhigh | Y’30Rlow | Y’30Rhigh | Y’31Llow | Y’31Lhigh | Y’31Rlow | Y’31Rhigh |
