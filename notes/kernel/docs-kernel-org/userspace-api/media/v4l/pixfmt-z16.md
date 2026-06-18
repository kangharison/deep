# 2.9.2.V4L2_PIX_FMT_Z16 (‘Z16 ‘)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-z16.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.9.2. V4L2\_PIX\_FMT\_Z16 (‘Z16 ‘)

16-bit depth data with distance values at each pixel

## 2.9.2.1. Description

This is a 16-bit format, representing depth data. Each pixel is a
distance to the respective point in the image coordinates. Distance unit
can vary and has to be negotiated with the device separately. Each pixel
is stored in a 16-bit word in the little endian byte order.

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0: | Z00low | Z00high | Z01low | Z01high | Z02low | Z02high | Z03low | Z03high |
| start + 8: | Z10low | Z10high | Z11low | Z11high | Z12low | Z12high | Z13low | Z13high |
| start + 16: | Z20low | Z20high | Z21low | Z21high | Z22low | Z22high | Z23low | Z23high |
| start + 24: | Z30low | Z30high | Z31low | Z31high | Z32low | Z32high | Z33low | Z33high |
