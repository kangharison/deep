# 2.7.1.5.V4L2_PIX_FMT_Y12I (‘Y12I’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-y12i.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.7.1.5. V4L2\_PIX\_FMT\_Y12I (‘Y12I’)

Interleaved grey-scale image, e.g. from a stereo-pair

## 2.7.1.5.1. Description

This is a grey-scale image with a depth of 12 bits per pixel, but with
pixels from 2 sources interleaved and bit-packed. Each pixel is stored
in a 24-bit word in the little-endian order. On a little-endian machine
these pixels can be deinterlaced using

```
__u8 *buf;
left0 = 0xfff & *(__u16 *)buf;
right0 = *(__u16 *)(buf + 1) >> 4;
```

**Bit-packed representation.**
pixels cross the byte boundary and have a ratio of 3 bytes for each
interleaved pixel.

|  |  |  |
| --- | --- | --- |
| Y’0left[7:0] | Y’0right[3:0]Y’0left[11:8] | Y’0right[11:4] |
