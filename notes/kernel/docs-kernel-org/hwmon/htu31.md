# Kernel driver HTU31

> 출처(원문): https://docs.kernel.org/hwmon/htu31.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver HTU31

Supported chips:

> * Measurement Specialties HTU31
>
>   Prefix: ‘htu31’
>
>   Addresses scanned: -
>
>   Datasheet: Publicly available from <https://www.te.com/en/product-CAT-HSC0007.html>

Author:

> * Andrei Lalaev <[andrey.lalaev@gmail.com](mailto:andrey.lalaev%40gmail.com)>

## Description

HTU31 is a humidity and temperature sensor.

Supported temperature range is from -40 to 125 degrees Celsius.

Communication with the device is performed via I2C protocol. Sensor’s default address
is 0x40.

## sysfs-Interface

|  |  |
| --- | --- |
| temp1\_input: | temperature input |
| humidity1\_input: | humidity input |
| heater\_enable: | heater control |
