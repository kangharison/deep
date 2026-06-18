# Kernel driver lineage-pem

> 출처(원문): https://docs.kernel.org/hwmon/lineage-pem.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver lineage-pem

Supported devices:

> * Lineage Compact Power Line Power Entry Modules
>
>   Prefix: ‘lineage-pem’
>
>   Addresses scanned: -
>
>   Documentation:
>
>   > <http://www.lineagepower.com/oem/pdf/CPLI2C.pdf>

Author: Guenter Roeck <[linux@roeck-us.net](mailto:linux%40roeck-us.net)>

## Description

This driver supports various Lineage Compact Power Line DC/DC and AC/DC
converters such as CP1800, CP2000AC, CP2000DC, CP2100DC, and others.

Lineage CPL power entry modules are nominally PMBus compliant. However, most
standard PMBus commands are not supported. Specifically, all hardware monitoring
and status reporting commands are non-standard. For this reason, a standard
PMBus driver can not be used.

## Usage Notes

This driver does not probe for Lineage CPL devices, since there is no register
which can be safely used to identify the chip. You will have to instantiate
the devices explicitly.

Example: the following will load the driver for a Lineage PEM at address 0x40
on I2C bus #1:

```
$ modprobe lineage-pem
$ echo lineage-pem 0x40 > /sys/bus/i2c/devices/i2c-1/new_device
```

All Lineage CPL power entry modules have a built-in I2C bus master selector
(PCA9541). To ensure device access, this driver should only be used as client
driver to the pca9541 I2C master selector driver.

## Sysfs entries

All Lineage CPL devices report output voltage and device temperature as well as
alarms for output voltage, temperature, input voltage, input current, input power,
and fan status.

Input voltage, input current, input power, and fan speed measurement is only
supported on newer devices. The driver detects if those attributes are supported,
and only creates respective sysfs entries if they are.

|  |  |
| --- | --- |
| in1\_input | Output voltage (mV) |
| in1\_min\_alarm | Output undervoltage alarm |
| in1\_max\_alarm | Output overvoltage alarm |
| in1\_crit | Output voltage critical alarm |
| in2\_input | Input voltage (mV, optional) |
| in2\_alarm | Input voltage alarm |
| curr1\_input | Input current (mA, optional) |
| curr1\_alarm | Input overcurrent alarm |
| power1\_input | Input power (uW, optional) |
| power1\_alarm | Input power alarm |
| fan1\_input | Fan 1 speed (rpm, optional) |
| fan2\_input | Fan 2 speed (rpm, optional) |
| fan3\_input | Fan 3 speed (rpm, optional) |
| temp1\_input |  |
| temp1\_max |  |
| temp1\_crit |  |
| temp1\_alarm |  |
| temp1\_crit\_alarm |  |
| temp1\_fault |  |
