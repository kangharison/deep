# 2.12.4.V4L2_TCH_FMT_TU08 (‘TU08’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-tch-tu08.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.12.4. V4L2\_TCH\_FMT\_TU08 (‘TU08’)

*man V4L2\_TCH\_FMT\_TU08(2)*

8-bit unsigned raw touch data

## 2.12.4.1. Description

This format represents unsigned 8-bit data from a touch controller.

This may be used for output for raw and reference data. Values may range from
0 to 255.

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | R’00 | R’01 | R’02 | R’03 |
| start + 4: | R’10 | R’11 | R’12 | R’13 |
| start + 8: | R’20 | R’21 | R’22 | R’23 |
| start + 12: | R’30 | R’31 | R’32 | R’33 |
