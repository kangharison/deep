# 2.11.8.V4L2_SDR_FMT_PCU20BE (‘PC20’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-sdr-pcu20be.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.11.8. V4L2\_SDR\_FMT\_PCU20BE (‘PC20’)

Planar complex unsigned 20-bit big endian IQ sample

## 2.11.8.1. Description

This format contains a sequence of complex number samples. Each complex
number consist of two parts called In-phase and Quadrature (IQ). Both I
and Q are represented as a 20 bit unsigned big endian number stored in
32 bit space. The remaining unused bits within the 32 bit space will be
padded with 0. I value starts first and Q value starts at an offset
equalling half of the buffer size (i.e.) offset = buffersize/2. Out of
the 20 bits, bit 19:2 (18 bit) is data and bit 1:0 (2 bit) can be any
value.

**Byte Order.**
Each cell is one byte.

| Offset: | Byte B0 | Byte B1 | Byte B2 | Byte B3 |
| --- | --- | --- | --- | --- |
| start + 0: | I’0[19:12] | I’0[11:4] | I’0[3:0]; B2[3:0]=pad | pad |
| start + 4: | I’1[19:12] | I’1[11:4] | I’1[3:0]; B2[3:0]=pad | pad |
| ... | | | | |
| start + offset: | Q’0[19:12] | Q’0[11:4] | Q’0[3:0]; B2[3:0]=pad | pad |
| start + offset + 4: | Q’1[19:12] | Q’1[11:4] | Q’1[3:0]; B2[3:0]=pad | pad |
