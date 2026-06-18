# Kernel driver nzxt-kraken2

> 출처(원문): https://docs.kernel.org/hwmon/nzxt-kraken2.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver nzxt-kraken2

Supported devices:

* NZXT Kraken X42
* NZXT Kraken X52
* NZXT Kraken X62
* NZXT Kraken X72

Author: Jonas Malaco

## Description

This driver enables hardware monitoring support for NZXT Kraken X42/X52/X62/X72
all-in-one CPU liquid coolers. Three sensors are available: fan speed, pump
speed and coolant temperature.

Fan and pump control, while supported by the firmware, are not currently
exposed. The addressable RGB LEDs, present in the integrated CPU water block
and pump head, are not supported either. But both features can be found in
existing user-space tools (e.g. [liquidctl](https://github.com/liquidctl/liquidctl)).

## Usage Notes

As these are USB HIDs, the driver can be loaded automatically by the kernel and
supports hot swapping.

## Sysfs entries

|  |  |
| --- | --- |
| fan1\_input | Fan speed (in rpm) |
| fan2\_input | Pump speed (in rpm) |
| temp1\_input | Coolant temperature (in millidegrees Celsius) |
