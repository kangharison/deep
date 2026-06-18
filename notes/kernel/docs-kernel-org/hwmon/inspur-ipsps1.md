# Kernel driver inspur-ipsps1

> 출처(원문): https://docs.kernel.org/hwmon/inspur-ipsps1.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver inspur-ipsps1

Supported chips:

> * Inspur Power System power supply unit

Author: John Wang <[wangzqbj@inspur.com](mailto:wangzqbj%40inspur.com)>

## Description

This driver supports Inspur Power System power supplies. This driver
is a client to the core PMBus driver.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate the
devices explicitly. Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

## Sysfs entries

The following attributes are supported:

|  |  |
| --- | --- |
| curr1\_input | Measured input current |
| curr1\_label | “iin” |
| curr1\_max | Maximum current |
| curr1\_max\_alarm | Current high alarm |
| curr2\_input | Measured output current in mA. |
| curr2\_label | “iout1” |
| curr2\_crit | Critical maximum current |
| curr2\_crit\_alarm | Current critical high alarm |
| curr2\_max | Maximum current |
| curr2\_max\_alarm | Current high alarm |
| fan1\_alarm | Fan 1 warning. |
| fan1\_fault | Fan 1 fault. |
| fan1\_input | Fan 1 speed in RPM. |
| in1\_alarm | Input voltage under-voltage alarm. |
| in1\_input | Measured input voltage in mV. |
| in1\_label | “vin” |
| in2\_input | Measured output voltage in mV. |
| in2\_label | “vout1” |
| in2\_lcrit | Critical minimum output voltage |
| in2\_lcrit\_alarm | Output voltage critical low alarm |
| in2\_max | Maximum output voltage |
| in2\_max\_alarm | Output voltage high alarm |
| in2\_min | Minimum output voltage |
| in2\_min\_alarm | Output voltage low alarm |
| power1\_alarm | Input fault or alarm. |
| power1\_input | Measured input power in uW. |
| power1\_label | “pin” |
| power1\_max | Input power limit |
| power2\_max\_alarm | Output power high alarm |
| power2\_max | Output power limit |
| power2\_input | Measured output power in uW. |
| power2\_label | “pout” |
| temp[1-3]\_input | Measured temperature |
| temp[1-2]\_max | Maximum temperature |
| temp[1-3]\_max\_alarm | Temperature high alarm |
| vendor | Manufacturer name |
| model | Product model |
| part\_number | Product part number |
| serial\_number | Product serial number |
| fw\_version | Firmware version |
| hw\_version | Hardware version |
| mode | Work mode. Can be set to active or standby, when set to standby, PSU will automatically switch between standby and redundancy mode. |
