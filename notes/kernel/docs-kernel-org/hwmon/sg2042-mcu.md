# Kernel driver sg2042-mcu

> 출처(원문): https://docs.kernel.org/hwmon/sg2042-mcu.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver sg2042-mcu

Supported chips:

> * Onboard MCU for sg2042
>
>   Addresses scanned: -
>
>   Prefix: ‘sg2042-mcu’

Authors:

> * Inochi Amaoto <[inochiama@outlook.com](mailto:inochiama%40outlook.com)>

## Description

This driver supprts hardware monitoring for onboard MCU with
i2c interface.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate
the devices explicitly.
Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for details.

## Sysfs Attributes

The following table shows the standard entries support by the driver:

| Name | Description |
| --- | --- |
| temp1\_input | Measured temperature of SoC |
| temp1\_crit | Critical high temperature |
| temp1\_crit\_hyst | hysteresis temperature restore from Critical |
| temp2\_input | Measured temperature of the base board |

The following table shows the extra entries support by the driver
(the MCU device is in i2c subsystem):

| Name | Perm | Description |
| --- | --- | --- |
| reset\_count | RO | Reset count of the SoC |
| uptime | RO | Seconds after the MCU is powered |
| reset\_reason | RO | Reset reason for the last reset |
| repower\_policy | RW | Execution policy when triggering repower |

`repower_policy`
:   The repower is triggered when the temperature of the SoC falls below
    the hysteresis temperature after triggering a shutdown due to
    reaching the critical temperature.
    The valid values for this entry are “repower” and “keep”. “keep” will
    leave the SoC down when the triggering repower, and “repower” will
    boot the SoC.

## Debugfs Interfaces

If debugfs is available, this driver exposes some hardware specific
data in `/sys/kernel/debug/sg2042-mcu/*/`.

| Name | Format | Description |
| --- | --- | --- |
| firmware\_version | 0x%02x | firmware version of the MCU |
| pcb\_version | 0x%02x | version number of the base board |
| board\_type | 0x%02x | identifiers for the base board |
| mcu\_type | %d | type of the MCU: 0 is STM32, 1 is GD32 |
