# Kernel driver mc33xs2410_hwmon

> 출처(원문): https://docs.kernel.org/hwmon/mc33xs2410_hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver mc33xs2410\_hwmon

Supported devices:

> * NXPs MC33XS2410
>
>   Datasheet: <https://www.nxp.com/docs/en/data-sheet/MC33XS2410.pdf>

Authors:

> Dimitri Fedrau <[dimitri.fedrau@liebherr.com](mailto:dimitri.fedrau%40liebherr.com)>

## Description

The MC33XS2410 is a four channel self-protected high-side switch featuring
hardware monitoring functions such as temperature, current and voltages for each
of the four channels.

## Sysfs entries

|  |  |
| --- | --- |
| temp1\_label | “Central die temperature” |
| temp1\_input | Measured temperature of central die |
| temp[2-5]\_label | “Channel [1-4] temperature” |
| temp[2-5]\_input | Measured temperature of a single channel |
| temp[2-5]\_alarm | Temperature alarm |
| temp[2-5]\_max | Maximal temperature |
