# Kernel driver raspberrypi-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/raspberrypi-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver raspberrypi-hwmon

Supported boards:

> * Raspberry Pi A+ (via GPIO on SoC)
> * Raspberry Pi B+ (via GPIO on SoC)
> * Raspberry Pi 2 B (via GPIO on SoC)
> * Raspberry Pi 3 B (via GPIO on port expander)
> * Raspberry Pi 3 B+ (via PMIC)

Author: Stefan Wahren <[stefan.wahren@i2se.com](mailto:stefan.wahren%40i2se.com)>

## Description

This driver periodically polls a mailbox property of the VC4 firmware to detect
undervoltage conditions.

## Sysfs entries

|  |  |
| --- | --- |
| in0\_lcrit\_alarm | Undervoltage alarm |
