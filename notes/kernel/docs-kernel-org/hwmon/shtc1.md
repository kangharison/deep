# Kernel driver shtc1

> 출처(원문): https://docs.kernel.org/hwmon/shtc1.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver shtc1

Supported chips:

> * Sensirion SHTC1
>
>   Prefix: ‘shtc1’
>
>   Addresses scanned: none
>
>   Datasheet: <https://www.sensirion.com/file/datasheet_shtc1>
> * Sensirion SHTW1
>
>   Prefix: ‘shtw1’
>
>   Addresses scanned: none
>
>   Datasheet: <https://www.sensirion.com/file/datasheet_shtw1>
> * Sensirion SHTC3
>
>   Prefix: ‘shtc3’
>
>   Addresses scanned: none
>
>   Datasheet: <https://www.sensirion.com/file/datasheet_shtc3>

Author:

> Johannes Winkelmann <[johannes.winkelmann@sensirion.com](mailto:johannes.winkelmann%40sensirion.com)>

## Description

This driver implements support for the Sensirion SHTC1, SHTW1, and SHTC3
chips, a humidity and temperature sensor. Temperature is measured in degrees
celsius, relative humidity is expressed as a percentage.

The device communicates with the I2C protocol. All sensors are set to I2C
address 0x70. See [How to instantiate I2C devices](../i2c/instantiating-devices.html) for methods to
instantiate the device.

There are two options configurable by means of shtc1\_platform\_data:

1. blocking (pull the I2C clock line down while performing the measurement) or
   non-blocking mode. Blocking mode will guarantee the fastest result but
   the I2C bus will be busy during that time. By default, non-blocking mode
   is used. Make sure clock-stretching works properly on your device if you
   want to use blocking mode.
2. high or low accuracy. High accuracy is used by default and using it is
   strongly recommended.

## sysfs-Interface

temp1\_input
:   * temperature input

humidity1\_input
:   * humidity input
