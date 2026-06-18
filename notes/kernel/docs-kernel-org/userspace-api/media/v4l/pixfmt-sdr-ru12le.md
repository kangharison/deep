# 2.11.5.V4L2_SDR_FMT_RU12LE (‘RU12’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-sdr-ru12le.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.11.5. V4L2\_SDR\_FMT\_RU12LE (‘RU12’)

Real unsigned 12-bit little endian sample

## 2.11.5.1. Description

This format contains sequence of real number samples. Each sample is
represented as a 12 bit unsigned little endian number. Sample is stored
in 16 bit space with unused high bits padded with 0.

**Byte Order.**
Each cell is one byte.

|  |  |  |
| --- | --- | --- |
| start + 0: | I’0[7:0] | I’0[11:8] |
