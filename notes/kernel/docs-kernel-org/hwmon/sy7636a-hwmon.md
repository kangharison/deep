# Kernel driver sy7636a-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/sy7636a-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver sy7636a-hwmon

Supported chips:

> * Silergy SY7636A PMIC

## Description

This driver adds hardware temperature reading support for
the Silergy SY7636A PMIC.

The following sensors are supported

> * Temperature
>   :   + Temperature of external NTC in milli-degree C

## sysfs-Interface

temp1\_input
:   * Temperature of external NTC (milli-degree C)
