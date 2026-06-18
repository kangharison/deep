# Kernel driver sht15

> 출처(원문): https://docs.kernel.org/hwmon/sht15.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver sht15

Authors:

> * Wouter Horre
> * Jonathan Cameron
> * Vivien Didelot <[vivien.didelot@savoirfairelinux.com](mailto:vivien.didelot%40savoirfairelinux.com)>
> * Jerome Oufella <[jerome.oufella@savoirfairelinux.com](mailto:jerome.oufella%40savoirfairelinux.com)>

Supported chips:

> * Sensirion SHT10
>
>   Prefix: ‘sht10’
> * Sensirion SHT11
>
>   Prefix: ‘sht11’
> * Sensirion SHT15
>
>   Prefix: ‘sht15’
> * Sensirion SHT71
>
>   Prefix: ‘sht71’
> * Sensirion SHT75
>
>   Prefix: ‘sht75’

Datasheet: Publicly available at the Sensirion website

> <http://www.sensirion.ch/en/pdf/product_information/Datasheet-humidity-sensor-SHT1x.pdf>

## Description

The SHT10, SHT11, SHT15, SHT71, and SHT75 are humidity and temperature
sensors.

The devices communicate using two GPIO lines.

Supported resolutions for the measurements are 14 bits for temperature and 12
bits for humidity, or 12 bits for temperature and 8 bits for humidity.

The humidity calibration coefficients are programmed into an OTP memory on the
chip. These coefficients are used to internally calibrate the signals from the
sensors. Disabling the reload of those coefficients allows saving 10ms for each
measurement and decrease power consumption, while losing on precision.

Some options may be set via sysfs attributes.

Notes:
:   * The regulator supply name is set to “vcc”.
    * If a CRC validation fails, a soft reset command is sent, which resets
      status register to its hardware default value, but the driver will try to
      restore the previous device configuration.

## Platform data

* checksum:
  set it to true to enable CRC validation of the readings (default to false).
* no\_otp\_reload:
  flag to indicate not to reload from OTP (default to false).
* low\_resolution:
  flag to indicate the temp/humidity resolution to use (default to false).

## Sysfs interface

|  |  |
| --- | --- |
| temp1\_input | temperature input |
| humidity1\_input | humidity input |
| heater\_enable | write 1 in this attribute to enable the on-chip heater, 0 to disable it. Be careful not to enable the heater for too long. |
| temp1\_fault | if 1, this means that the voltage is low (below 2.47V) and measurement may be invalid. |
| humidity1\_fault | same as temp1\_fault. |
