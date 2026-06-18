# Cirrus Logic EP93xx ADC driver

> 출처(원문): https://docs.kernel.org/iio/ep93xx_adc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Cirrus Logic EP93xx ADC driver

## 1. Overview

The driver is intended to work on both low-end (EP9301, EP9302) devices with
5-channel ADC and high-end (EP9307, EP9312, EP9315) devices with 10-channel
touchscreen/ADC module.

## 2. Channel numbering

Numbering scheme for channels 0..4 is defined in EP9301 and EP9302 datasheets.
EP9307, EP9312 and EP9315 have 3 channels more (total 8), but the numbering is
not defined. So the last three are numbered randomly, let’s say.

Assuming ep93xx\_adc is IIO device0, you’d find the following entries under
/sys/bus/iio/devices/iio:device0/:

> | sysfs entry | ball/pin name |
> | --- | --- |
> | in\_voltage0\_raw | YM |
> | in\_voltage1\_raw | SXP |
> | in\_voltage2\_raw | SXM |
> | in\_voltage3\_raw | SYP |
> | in\_voltage4\_raw | SYM |
> | in\_voltage5\_raw | XP |
> | in\_voltage6\_raw | XM |
> | in\_voltage7\_raw | YP |
