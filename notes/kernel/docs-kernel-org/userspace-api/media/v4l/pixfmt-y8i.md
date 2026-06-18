# 2.7.1.4.V4L2_PIX_FMT_Y8I (‘Y8I ‘)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-y8i.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.4. V4L2\_PIX\_FMT\_Y8I (‘Y8I ‘)

Interleaved grey-scale image, e.g. from a stereo-pair

## 2.7.1.4.1. Description

This is a grey-scale image with a depth of 8 bits per pixel, but with
pixels from 2 sources interleaved. Each pixel is stored in a 16-bit
word. E.g. the R200 RealSense camera stores pixel from the left sensor
in lower and from the right sensor in the higher 8 bits.

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0: | Y’00left | Y’00right | Y’01left | Y’01right | Y’02left | Y’02right | Y’03left | Y’03right |
| start + 8: | Y’10left | Y’10right | Y’11left | Y’11right | Y’12left | Y’12right | Y’13left | Y’13right |
| start + 16: | Y’20left | Y’20right | Y’21left | Y’21right | Y’22left | Y’22right | Y’23left | Y’23right |
| start + 24: | Y’30left | Y’30right | Y’31left | Y’31right | Y’32left | Y’32right | Y’33left | Y’33right |
