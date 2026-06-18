# Kernel driver xgene-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/xgene-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver xgene-hwmon

Supported chips:

> * APM X-Gene SoC

## Description

This driver adds hardware temperature and power reading support for
APM X-Gene SoC using the mailbox communication interface.
For device tree, it is the standard DT mailbox.
For ACPI, it is the PCC mailbox.

The following sensors are supported

> * Temperature
>   :   + SoC on-die temperature in milli-degree C
>       + Alarm when high/over temperature occurs
> * Power
>   :   + CPU power in uW
>       + IO power in uW

## sysfs-Interface

temp0\_input
:   * SoC on-die temperature (milli-degree C)

temp0\_critical\_alarm
:   * An 1 would indicates on-die temperature exceeded threshold

power0\_input
:   * CPU power in (uW)

power1\_input
:   * IO power in (uW)
