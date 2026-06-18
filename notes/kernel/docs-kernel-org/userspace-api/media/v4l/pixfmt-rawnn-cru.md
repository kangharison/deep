# 2.6.1.1.V4L2_PIX_FMT_RAW_CRU10 (‘CR10’), V4L2_PIX_FMT_RAW_CRU12 (‘CR12’), V4L2_PIX_FMT_RAW_CRU14 (‘CR14’), V4L2_PIX_FMT_RAW_CRU20 (‘CR20’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-rawnn-cru.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.1. V4L2\_PIX\_FMT\_RAW\_CRU10 (‘CR10’), V4L2\_PIX\_FMT\_RAW\_CRU12 (‘CR12’), V4L2\_PIX\_FMT\_RAW\_CRU14 (‘CR14’), V4L2\_PIX\_FMT\_RAW\_CRU20 (‘CR20’)

## 2.6.1.1.1. Renesas RZ/V2H Camera Receiver Unit 64-bit packed pixel formats

V4L2\_PIX\_FMT\_RAW\_CRU10 (CR10)

V4L2\_PIX\_FMT\_RAW\_CRU12 (CR12)

V4L2\_PIX\_FMT\_RAW\_CRU14 (CR14)

V4L2\_PIX\_FMT\_RAW\_CRU20 (CR20)

### 2.6.1.1.1.1. Description

These pixel formats are some of the RAW outputs for the Camera Receiver Unit in
the Renesas RZ/V2H SoC. They are raw formats which pack pixels contiguously into
64-bit units, with the 4 or 8 most significant bits padded.

**Byte Order**

RAW formats

| Pixel Format Code | Data organization | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 63 | 62 | 61 | 60 | 59 | 58 | 57 | 56 | 55 | 54 | 53 | 52 | 51 | 50 | 49 | 48 | 47 | 46 | 45 | 44 | 43 | 42 | 41 | 40 | 39 | 38 | 37 | 36 | 35 | 34 | 33 | 32 | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| V4L2\_PIX\_FMT\_RAW\_CRU10 | 0 | 0 | 0 | 0 | P5 | | | | | | | | | | P4 | | | | | | | | | | P3 | | | | | | | | | | P2 | | | | | | | | | | P1 | | | | | | | | | | P0 | | | | | | | | | |
| V4L2\_PIX\_FMT\_RAW\_CRU12 | 0 | 0 | 0 | 0 | P4 | | | | | | | | | | | | P3 | | | | | | | | | | | | P2 | | | | | | | | | | | | P1 | | | | | | | | | | | | P0 | | | | | | | | | | | |
| V4L2\_PIX\_FMT\_RAW\_CRU14 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | P3 | | | | | | | | | | | | | | P2 | | | | | | | | | | | | | | P1 | | | | | | | | | | | | | | P0 | | | | | | | | | | | | | |
| V4L2\_PIX\_FMT\_RAW\_CRU20 | 0 | 0 | 0 | 0 | P2 | | | | | | | | | | | | | | | | | | | | P1 | | | | | | | | | | | | | | | | | | | | P0 | | | | | | | | | | | | | | | | | | | |
