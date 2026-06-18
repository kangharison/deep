# Kernel driver stpddc60

> 출처(원문): https://docs.kernel.org/hwmon/stpddc60.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver stpddc60

Supported chips:

> * ST STPDDC60
>
>   Prefix: ‘stpddc60’, ‘bmr481’
>
>   Addresses scanned: -
>
>   Datasheet: <https://flexpowermodules.com/documents/fpm-techspec-bmr481>

Author: Erik Rosen <[erik.rosen@metormote.com](mailto:erik.rosen%40metormote.com)>

## Description

This driver supports hardware monitoring for ST STPDDC60 controller chip and
compatible modules.

The driver is a client driver to the core PMBus driver. Please see
[Kernel driver pmbus](pmbus.html) and Documentation.hwmon/pmbus-core for details
on PMBus client drivers.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate the
devices explicitly. Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

The vout under- and over-voltage limits are set in relation to the commanded
output voltage as a positive or negative offset in the interval 50mV to 400mV
in 50mV steps. This means that the absolute values of the limits will change
when the commanded output voltage changes. Also, care should be taken when
writing to those limits since in the worst case the commanded output voltage
could change at the same time as the limit is written to, which will lead to
unpredictable results.

## Platform data support

The driver supports standard PMBus driver platform data.

## Sysfs entries

The following attributes are supported. Vin, iout, pout and temp limits
are read-write; all other attributes are read-only.

|  |  |
| --- | --- |
| in1\_label | “vin” |
| in1\_input | Measured input voltage. |
| in1\_lcrit | Critical minimum input voltage. |
| in1\_crit | Critical maximum input voltage. |
| in1\_lcrit\_alarm | Input voltage critical low alarm. |
| in1\_crit\_alarm | Input voltage critical high alarm. |
| in2\_label | “vout1” |
| in2\_input | Measured output voltage. |
| in2\_lcrit | Critical minimum output voltage. |
| in2\_crit | Critical maximum output voltage. |
| in2\_lcrit\_alarm | Critical output voltage critical low alarm. |
| in2\_crit\_alarm | Critical output voltage critical high alarm. |
| curr1\_label | “iout1” |
| curr1\_input | Measured output current. |
| curr1\_max | Maximum output current. |
| curr1\_max\_alarm | Output current high alarm. |
| curr1\_crit | Critical maximum output current. |
| curr1\_crit\_alarm | Output current critical high alarm. |
| power1\_label | “pout1” |
| power1\_input | Measured output power. |
| power1\_crit | Critical maximum output power. |
| power1\_crit\_alarm | Output power critical high alarm. |
| temp1\_input | Measured maximum temperature of all phases. |
| temp1\_max | Maximum temperature limit. |
| temp1\_max\_alarm | High temperature alarm. |
| temp1\_crit | Critical maximum temperature limit. |
| temp1\_crit\_alarm | Critical maximum temperature alarm. |
