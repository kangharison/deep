# 2.9.3.V4L2_PIX_FMT_CNF4 (‘CNF4’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-cnf4.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.9.3. V4L2\_PIX\_FMT\_CNF4 (‘CNF4’)

Depth sensor confidence information as a 4 bits per pixel packed array

## 2.9.3.1. Description

Proprietary format used by Intel RealSense Depth cameras containing depth
confidence information in range 0-15 with 0 indicating that the sensor was
unable to resolve any signal and 15 indicating maximum level of confidence for
the specific sensor (actual error margins might change from sensor to sensor).

Every two consecutive pixels are packed into a single byte.
Bits 0-3 of byte n refer to confidence value of depth pixel 2\*n,
bits 4-7 to confidence value of depth pixel 2\*n+1.

**Bit-packed representation.**

|  |  |
| --- | --- |
| Y’01[3:0](bits 7--4) Y’00[3:0](bits 3--0) | Y’03[3:0](bits 7--4) Y’02[3:0](bits 3--0) |
