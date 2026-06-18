# Kernel driver sht3x

> 출처(원문): https://docs.kernel.org/hwmon/sht3x.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver sht3x

Supported chips:

> * Sensirion SHT3x-DIS
>
>   Prefix: ‘sht3x’
>
>   Addresses scanned: none
>
>   Datasheets:
>   :   + <https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf>
>       + <https://sensirion.com/media/documents/051DF50B/639C8101/Sensirion_Humidity_and_Temperature_Sensors_Datasheet_SHT33.pdf>
> * Sensirion STS3x-DIS
>
>   Prefix: ‘sts3x’
>
>   Addresses scanned: none
>
>   Datasheets:
>   :   + <https://sensirion.com/media/documents/1DA31AFD/61641F76/Sensirion_Temperature_Sensors_STS3x_Datasheet.pdf>
>       + <https://sensirion.com/media/documents/292A335C/65537BAF/Sensirion_Datasheet_STS32_STS33.pdf>
> * Sensirion SHT85
>
>   Prefix: ‘sht85’
>
>   Addresses scanned: none
>
>   Datasheet: <https://sensirion.com/media/documents/4B40CEF3/640B2346/Sensirion_Humidity_Sensors_SHT85_Datasheet.pdf>

Author:

> * David Frey <[david.frey@sensirion.com](mailto:david.frey%40sensirion.com)>
> * Pascal Sachs <[pascal.sachs@sensirion.com](mailto:pascal.sachs%40sensirion.com)>

## Description

This driver implements support for the Sensirion SHT3x-DIS, STS3x-DIS and SHT85
series of humidity and temperature sensors. Temperature is measured in degrees
celsius, relative humidity is expressed as a percentage. In the sysfs interface,
all values are scaled by 1000, i.e. the value for 31.5 degrees celsius is 31500.

The device communicates with the I2C protocol. SHT3x sensors can have the I2C
addresses 0x44 or 0x45 (0x4a or 0x4b for sts3x), depending on the wiring. SHT85
address is 0x44 and is fixed. See [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
methods to instantiate the device.

Even if sht3x sensor supports clock-stretch (blocking mode) and non-stretch
(non-blocking mode) in single-shot mode, this driver only supports the latter.

The sht3x sensor supports a single shot mode as well as 5 periodic measure
modes, which can be controlled with the update\_interval sysfs interface.
The allowed update\_interval in milliseconds are as follows:

> |  |  |  |
> | --- | --- | --- |
> | 0 |  | single shot mode |
> | 2000 | 0.5 Hz | periodic measurement |
> | 1000 | 1 Hz | periodic measurement |
> | 500 | 2 Hz | periodic measurement |
> | 250 | 4 Hz | periodic measurement |
> | 100 | 10 Hz | periodic measurement |

In the periodic measure mode, the sensor automatically triggers a measurement
with the configured update interval on the chip. When a temperature or humidity
reading exceeds the configured limits, the alert attribute is set to 1 and
the alert pin on the sensor is set to high.
When the temperature and humidity readings move back between the hysteresis
values, the alert bit is set to 0 and the alert pin on the sensor is set to
low.

The serial number exposed to debugfs allows for unique identification of the
sensors. For sts32, sts33 and sht33, the manufacturer provides calibration
certificates through an API.

## sysfs-Interface

|  |  |
| --- | --- |
| temp1\_input: | temperature input |
| humidity1\_input: | humidity input |
| temp1\_max: | temperature max value |
| temp1\_max\_hyst: | temperature hysteresis value for max limit |
| humidity1\_max: | humidity max value |
| humidity1\_max\_hyst: | humidity hysteresis value for max limit |
| temp1\_min: | temperature min value |
| temp1\_min\_hyst: | temperature hysteresis value for min limit |
| humidity1\_min: | humidity min value |
| humidity1\_min\_hyst: | humidity hysteresis value for min limit |
| temp1\_alarm: | alarm flag is set to 1 if the temperature is outside the configured limits. Alarm only works in periodic measure mode |
| humidity1\_alarm: | alarm flag is set to 1 if the humidity is outside the configured limits. Alarm only works in periodic measure mode |
| heater\_enable: | heater enable, heating element removes excess humidity from sensor:  * 0: turned off * 1: turned on |
| update\_interval: | update interval, 0 for single shot, interval in msec for periodic measurement. If the interval is not supported by the sensor, the next faster interval is chosen |
| repeatability: | write or read repeatability, higher repeatability means longer measurement duration, lower noise level and larger energy consumption:  * 0: low repeatability * 1: medium repeatability * 2: high repeatability |

## debugfs-Interface

|  |  |
| --- | --- |
| serial\_number: | unique serial number of the sensor in decimal |
