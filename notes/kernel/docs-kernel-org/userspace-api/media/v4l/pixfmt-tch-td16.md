# 2.12.1.V4L2_TCH_FMT_DELTA_TD16 (‘TD16’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-tch-td16.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.12.1. V4L2\_TCH\_FMT\_DELTA\_TD16 (‘TD16’)

*man V4L2\_TCH\_FMT\_DELTA\_TD16(2)*

16-bit signed little endian Touch Delta

## 2.12.1.1. Description

This format represents delta data from a touch controller.

Delta values may range from -32768 to 32767. Typically the values will vary
through a small range depending on whether the sensor is touched or not. The
full value may be seen if one of the touchscreen nodes has a fault or the line
is not connected.

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |  |  |  |  |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| start + 0: | D’00low | D’00high | D’01low | D’01high | D’02low | D’02high | D’03low | D’03high |
| start + 8: | D’10low | D’10high | D’11low | D’11high | D’12low | D’12high | D’13low | D’13high |
| start + 16: | D’20low | D’20high | D’21low | D’21high | D’22low | D’22high | D’23low | D’23high |
| start + 24: | D’30low | D’30high | D’31low | D’31high | D’32low | D’32high | D’33low | D’33high |
