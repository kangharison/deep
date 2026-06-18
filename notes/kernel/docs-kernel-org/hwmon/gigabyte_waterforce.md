# Kernel driver gigabyte_waterforce

> 출처(원문): https://docs.kernel.org/hwmon/gigabyte_waterforce.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver gigabyte\_waterforce

Supported devices:

* Gigabyte AORUS WATERFORCE X240
* Gigabyte AORUS WATERFORCE X280
* Gigabyte AORUS WATERFORCE X360

Author: Aleksa Savic

## Description

This driver enables hardware monitoring support for the listed Gigabyte Waterforce
all-in-one CPU liquid coolers. Available sensors are pump and fan speed in RPM, as
well as coolant temperature. Also available through debugfs is the firmware version.

Attaching a fan is optional and allows it to be controlled from the device. If
it’s not connected, the fan-related sensors will report zeroes.

The addressable RGB LEDs and LCD screen are not supported in this driver and should
be controlled through userspace tools.

## Usage notes

As these are USB HIDs, the driver can be loaded automatically by the kernel and
supports hot swapping.

## Sysfs entries

|  |  |
| --- | --- |
| fan1\_input | Fan speed (in rpm) |
| fan2\_input | Pump speed (in rpm) |
| temp1\_input | Coolant temperature (in millidegrees Celsius) |

## Debugfs entries

|  |  |
| --- | --- |
| firmware\_version | Device firmware version |
