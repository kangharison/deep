# Kernel driver acbel-fsg032

> 출처(원문): https://docs.kernel.org/hwmon/acbel-fsg032.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver acbel-fsg032

Supported chips:

> * ACBEL FSG032-00xG power supply.

Author: Lakshmi Yadlapati <[lakshmiy@us.ibm.com](mailto:lakshmiy%40us.ibm.com)>

## Description

This driver supports ACBEL FSG032-00xG Power Supply. This driver
is a client to the core PMBus driver.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate the
devices explicitly. Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

## Sysfs entries

The following attributes are supported:

|  |  |
| --- | --- |
| curr1\_crit | Critical maximum current. |
| curr1\_crit\_alarm | Input current critical alarm. |
| curr1\_input | Measured output current. |
| curr1\_label | “iin” |
| curr1\_max | Maximum input current. |
| curr1\_max\_alarm | Maximum input current high alarm. |
| curr1\_rated\_max | Maximum rated input current. |
| curr2\_crit | Critical maximum current. |
| curr2\_crit\_alarm | Output current critical alarm. |
| curr2\_input | Measured output current. |
| curr2\_label | “iout1” |
| curr2\_max | Maximum output current. |
| curr2\_max\_alarm | Output current high alarm. |
| curr2\_rated\_max | Maximum rated output current. |
| fan1\_alarm | Fan 1 warning. |
| fan1\_fault | Fan 1 fault. |
| fan1\_input | Fan 1 speed in RPM. |
| fan1\_target | Set fan speed reference. |
| in1\_alarm | Input voltage under-voltage alarm. |
| in1\_input | Measured input voltage. |
| in1\_label | “vin” |
| in1\_rated\_max | Maximum rated input voltage. |
| in1\_rated\_min | Minimum rated input voltage. |
| in2\_crit | Critical maximum output voltage. |
| in2\_crit\_alarm | Output voltage critical high alarm. |
| in2\_input | Measured output voltage. |
| in2\_label | “vout1” |
| in2\_lcrit | Critical minimum output voltage. |
| in2\_lcrit\_alarm | Output voltage critical low alarm. |
| in2\_rated\_max | Maximum rated output voltage. |
| in2\_rated\_min | Minimum rated output voltage. |
| power1\_alarm | Input fault or alarm. |
| power1\_input | Measured input power. |
| power1\_label | “pin” |
| power1\_max | Input power limit. |
| power1\_rated\_max | Maximum rated input power. |
| power2\_crit | Critical output power limit. |
| power2\_crit\_alarm | Output power crit alarm limit exceeded. |
| power2\_input | Measured output power. |
| power2\_label | “pout” |
| power2\_max | Output power limit. |
| power2\_max\_alarm | Output power high alarm. |
| power2\_rated\_max | Maximum rated output power. |
| temp[1-3]\_input | Measured temperature. |
| temp[1-2]\_max | Maximum temperature. |
| temp[1-3]\_rated\_max | Temperature high alarm. |
