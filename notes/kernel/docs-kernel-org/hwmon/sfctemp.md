# Kernel driver sfctemp

> 출처(원문): https://docs.kernel.org/hwmon/sfctemp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver sfctemp

Supported chips:
:   * StarFive JH7100
    * StarFive JH7110

Authors:
:   * Emil Renner Berthing <[kernel@esmil.dk](mailto:kernel%40esmil.dk)>

## Description

This driver adds support for reading the built-in temperature sensor on the
JH7100 and JH7110 RISC-V SoCs by StarFive Technology Co. Ltd.

## `sysfs` interface

The temperature sensor can be enabled, disabled and queried via the standard
hwmon interface in sysfs under `/sys/class/hwmon/hwmonX` for some value of
`X`:

| Name | Perm | Description |
| --- | --- | --- |
| temp1\_enable | RW | Enable or disable temperature sensor. Automatically enabled by the driver, but may be disabled to save power. |
| temp1\_input | RO | Temperature reading in milli-degrees Celsius. |
