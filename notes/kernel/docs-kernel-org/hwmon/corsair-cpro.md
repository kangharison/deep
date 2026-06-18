# Kernel driver corsair-cpro

> 출처(원문): https://docs.kernel.org/hwmon/corsair-cpro.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver corsair-cpro

Supported devices:

> * Corsair Commander Pro
> * Corsair Commander Pro (1000D)

Author: Marius Zachmann

## Description

This driver implements the sysfs interface for the Corsair Commander Pro.
The Corsair Commander Pro is a USB device with 6 fan connectors,
4 temperature sensor connectors and 2 Corsair LED connectors.
It can read the voltage levels on the SATA power connector.

## Usage Notes

Since it is a USB device, hotswapping is possible. The device is autodetected.

## Sysfs entries

|  |  |
| --- | --- |
| in0\_input | Voltage on SATA 12v |
| in1\_input | Voltage on SATA 5v |
| in2\_input | Voltage on SATA 3.3v |
| temp[1-4]\_input | Temperature on connected temperature sensors |
| fan[1-6]\_input | Connected fan rpm. |
| fan[1-6]\_label | Shows fan type as detected by the device. |
| fan[1-6]\_target | Sets fan speed target rpm. When reading, it reports the last value if it was set by the driver. Otherwise returns an error. |
| pwm[1-6] | Sets the fan speed. Values from 0-255. Can only be read if pwm was set directly. |

## Debugfs entries

|  |  |
| --- | --- |
| firmware\_version | Firmware version |
| bootloader\_version | Bootloader version |
