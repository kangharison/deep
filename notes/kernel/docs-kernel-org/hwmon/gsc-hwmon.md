# Kernel driver gsc-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/gsc-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver gsc-hwmon

Supported chips: Gateworks GSC
Datasheet: <http://trac.gateworks.com/wiki/gsc>
Author: Tim Harvey <[tharvey@gateworks.com](mailto:tharvey%40gateworks.com)>

## Description:

This driver supports hardware monitoring for the temperature sensor,
various ADC’s connected to the GSC, and optional FAN controller available
on some boards.

## Voltage Monitoring

The voltage inputs are scaled either internally or by the driver depending
on the GSC version and firmware. The values returned by the driver do not need
further scaling. The voltage input labels provide the voltage rail name:

inX\_input Measured voltage (mV).
inX\_label Name of voltage rail.

## Temperature Monitoring

Temperatures are measured with 12-bit or 10-bit resolution and are scaled
either internally or by the driver depending on the GSC version and firmware.
The values returned by the driver reflect millidegree Celsius:

tempX\_input Measured temperature.
tempX\_label Name of temperature input.

## PWM Output Control

The GSC features 1 PWM output that operates in automatic mode where the
PWM value will be scaled depending on 6 temperature boundaries.
The tempeature boundaries are read-write and in millidegree Celsius and the
read-only PWM values range from 0 (off) to 255 (full speed).
Fan speed will be set to minimum (off) when the temperature sensor reads
less than pwm1\_auto\_point1\_temp and maximum when the temperature sensor
equals or exceeds pwm1\_auto\_point6\_temp.

pwm1\_auto\_point[1-6]\_pwm PWM value.
pwm1\_auto\_point[1-6]\_temp Temperature boundary.
