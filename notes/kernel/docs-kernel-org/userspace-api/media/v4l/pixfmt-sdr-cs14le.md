# 2.11.4.V4L2_SDR_FMT_CS14LE (‘CS14’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-sdr-cs14le.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.11.4. V4L2\_SDR\_FMT\_CS14LE (‘CS14’)

Complex signed 14-bit little endian IQ sample

## 2.11.4.1. Description

This format contains sequence of complex number samples. Each complex
number consist two parts, called In-phase and Quadrature (IQ). Both I
and Q are represented as a 14 bit signed little endian number. I value
comes first and Q value after that. 14 bit value is stored in 16 bit
space with unused high bits padded with 0.

**Byte Order.**
Each cell is one byte.

|  |  |  |
| --- | --- | --- |
| start + 0: | I’0[7:0] | I’0[13:8] |
| start + 2: | Q’0[7:0] | Q’0[13:8] |
