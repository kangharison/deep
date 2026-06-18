# Kernel driver menf21bmc_hwmon

> 출처(원문): https://docs.kernel.org/hwmon/menf21bmc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver menf21bmc\_hwmon

Supported chips:

> * MEN 14F021P00
>
>   Prefix: ‘menf21bmc\_hwmon’
>
>   Addresses scanned: -

Author: Andreas Werner <[andreas.werner@men.de](mailto:andreas.werner%40men.de)>

## Description

The menf21bmc is a Board Management Controller (BMC) which provides an I2C
interface to the host to access the features implemented in the BMC.

This driver gives access to the voltage monitoring feature of the main
voltages of the board.
The voltage sensors are connected to the ADC inputs of the BMC which is
a PIC16F917 Mikrocontroller.

## Usage Notes

This driver is part of the MFD driver named “menf21bmc” and does
not auto-detect devices.
You will have to instantiate the MFD driver explicitly.
Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

## Sysfs entries

The following attributes are supported. All attributes are read only
The Limits are read once by the driver.

|  |  |
| --- | --- |
| in0\_input | +3.3V input voltage |
| in1\_input | +5.0V input voltage |
| in2\_input | +12.0V input voltage |
| in3\_input | +5V Standby input voltage |
| in4\_input | VBAT (on board battery) |
| in[0-4]\_min | Minimum voltage limit |
| in[0-4]\_max | Maximum voltage limit |
| in0\_label | “MON\_3\_3V” |
| in1\_label | “MON\_5V” |
| in2\_label | “MON\_12V” |
| in3\_label | “5V\_STANDBY” |
| in4\_label | “VBAT” |
