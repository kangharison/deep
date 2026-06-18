# 2.11.3.V4L2_SDR_FMT_CS8 (‘CS08’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-sdr-cs08.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.11.3. V4L2\_SDR\_FMT\_CS8 (‘CS08’)

Complex signed 8-bit IQ sample

## 2.11.3.1. Description

This format contains sequence of complex number samples. Each complex
number consist two parts, called In-phase and Quadrature (IQ). Both I
and Q are represented as a 8 bit signed number. I value comes first and
Q value after that.

**Byte Order.**
Each cell is one byte.

|  |  |
| --- | --- |
| start + 0: | I’0 |
| start + 1: | Q’0 |
