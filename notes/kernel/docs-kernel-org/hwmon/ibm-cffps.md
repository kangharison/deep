# Kernel driver ibm-cffps

> 출처(원문): https://docs.kernel.org/hwmon/ibm-cffps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver ibm-cffps

Supported chips:

> * IBM Common Form Factor power supply

Author: Eddie James <[eajames@us.ibm.com](mailto:eajames%40us.ibm.com)>

## Description

This driver supports IBM Common Form Factor (CFF) power supplies. This driver
is a client to the core PMBus driver.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate the
devices explicitly. Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

## Sysfs entries

The following attributes are supported:

|  |  |
| --- | --- |
| curr1\_alarm | Output current over-current alarm. |
| curr1\_input | Measured output current in mA. |
| curr1\_label | “iout1” |
| fan1\_alarm | Fan 1 warning. |
| fan1\_fault | Fan 1 fault. |
| fan1\_input | Fan 1 speed in RPM. |
| fan2\_alarm | Fan 2 warning. |
| fan2\_fault | Fan 2 fault. |
| fan2\_input | Fan 2 speed in RPM. |
| in1\_alarm | Input voltage under-voltage alarm. |
| in1\_input | Measured input voltage in mV. |
| in1\_label | “vin” |
| in2\_alarm | Output voltage over-voltage alarm. |
| in2\_input | Measured output voltage in mV. |
| in2\_label | “vout1” |
| power1\_alarm | Input fault or alarm. |
| power1\_input | Measured input power in uW. |
| power1\_label | “pin” |
| temp1\_alarm | PSU inlet ambient temperature over-temperature alarm. |
| temp1\_input | Measured PSU inlet ambient temp in millidegrees C. |
| temp2\_alarm | Secondary rectifier temp over-temperature alarm. |
| temp2\_input | Measured secondary rectifier temp in millidegrees C. |
| temp3\_alarm | ORing FET temperature over-temperature alarm. |
| temp3\_input | Measured ORing FET temperature in millidegrees C. |
