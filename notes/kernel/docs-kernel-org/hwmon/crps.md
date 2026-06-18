# Kernel driver crps

> 출처(원문): https://docs.kernel.org/hwmon/crps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver crps

Supported chips:

> * Intel CRPS185
>
>   Prefix: ‘crps185’
>
>   Addresses scanned: -
>
>   Datasheet: Only available under NDA.

Authors:
:   Ninad Palsule <[ninad@linux.ibm.com](mailto:ninad%40linux.ibm.com)>

## Description

This driver implements support for Intel Common Redundant Power supply with
PMBus support.

The driver is a client driver to the core PMBus driver.
Please see [Kernel driver pmbus](pmbus.html) for details on PMBus client drivers.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate the
devices explicitly. Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

## Sysfs entries

|  |  |
| --- | --- |
| curr1\_label | “iin” |
| curr1\_input | Measured input current |
| curr1\_max | Maximum input current |
| curr1\_max\_alarm | Input maximum current high alarm |
| curr1\_crit | Critical high input current |
| curr1\_crit\_alarm | Input critical current high alarm |
| curr1\_rated\_max | Maximum rated input current |
| curr2\_label | “iout1” |
| curr2\_input | Measured output current |
| curr2\_max | Maximum output current |
| curr2\_max\_alarm | Output maximum current high alarm |
| curr2\_crit | Critical high output current |
| curr2\_crit\_alarm | Output critical current high alarm |
| curr2\_rated\_max | Maximum rated output current |
| in1\_label | “vin” |
| in1\_input | Measured input voltage |
| in1\_crit | Critical input over voltage |
| in1\_crit\_alarm | Critical input over voltage alarm |
| in1\_max | Maximum input over voltage |
| in1\_max\_alarm | Maximum input over voltage alarm |
| in1\_rated\_min | Minimum rated input voltage |
| in1\_rated\_max | Maximum rated input voltage |
| in2\_label | “vout1” |
| in2\_input | Measured input voltage |
| in2\_crit | Critical input over voltage |
| in2\_crit\_alarm | Critical input over voltage alarm |
| in2\_lcrit | Critical input under voltage fault |
| in2\_lcrit\_alarm | Critical input under voltage fault alarm |
| in2\_max | Maximum input over voltage |
| in2\_max\_alarm | Maximum input over voltage alarm |
| in2\_min | Minimum input under voltage warning |
| in2\_min\_alarm | Minimum input under voltage warning alarm |
| in2\_rated\_min | Minimum rated input voltage |
| in2\_rated\_max | Maximum rated input voltage |
| power1\_label | “pin” |
| power1\_input | Measured input power |
| power1\_alarm | Input power high alarm |
| power1\_max | Maximum input power |
| power1\_rated\_max | Maximum rated input power |
| temp[1-2]\_input | Measured temperature |
| temp[1-2]\_crit | Critical temperature |
| temp[1-2]\_crit\_alarm | Critical temperature alarm |
| temp[1-2]\_max | Maximum temperature |
| temp[1-2]\_max\_alarm | Maximum temperature alarm |
| temp[1-2]\_rated\_max | Maximum rated temperature |
| fan1\_alarm | Fan 1 warning. |
| fan1\_fault | Fan 1 fault. |
| fan1\_input | Fan 1 speed in RPM. |
| fan1\_target | Fan 1 target. |
