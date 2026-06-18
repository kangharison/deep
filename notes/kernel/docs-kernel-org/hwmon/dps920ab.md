# Kernel driver dps920ab

> 출처(원문): https://docs.kernel.org/hwmon/dps920ab.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver dps920ab

Supported chips:

> * Delta DPS920AB
>
>   Prefix: ‘dps920ab’
>
>   Addresses scanned: -

Authors:
:   Robert Marko <[robert.marko@sartura.hr](mailto:robert.marko%40sartura.hr)>

## Description

This driver implements support for Delta DPS920AB 920W 54V DC single output
power supply with PMBus support.

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
| curr1\_alarm | Input current high alarm |
| curr2\_label | “iout1” |
| curr2\_input | Measured output current |
| curr2\_max | Maximum output current |
| curr2\_rated\_max | Maximum rated output current |
| in1\_label | “vin” |
| in1\_input | Measured input voltage |
| in1\_alarm | Input voltage alarm |
| in2\_label | “vout1” |
| in2\_input | Measured output voltage |
| in2\_rated\_min | Minimum rated output voltage |
| in2\_rated\_max | Maximum rated output voltage |
| in2\_alarm | Output voltage alarm |
| power1\_label | “pin” |
| power1\_input | Measured input power |
| power1\_alarm | Input power high alarm |
| power2\_label | “pout1” |
| power2\_input | Measured output power |
| power2\_rated\_max | Maximum rated output power |
| temp[1-3]\_input | Measured temperature |
| temp[1-3]\_alarm | Temperature alarm |
| fan1\_alarm | Fan 1 warning. |
| fan1\_fault | Fan 1 fault. |
| fan1\_input | Fan 1 speed in RPM. |
