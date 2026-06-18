# Kernel driver sl28cpld

> 출처(원문): https://docs.kernel.org/hwmon/sl28cpld.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver sl28cpld

Supported chips:

> * Kontron sl28cpld
>
>   Prefix: ‘sl28cpld’
>
>   Datasheet: not available

Authors: Michael Walle <[michael@walle.cc](mailto:michael%40walle.cc)>

## Description

The sl28cpld is a board management controller which also exposes a hardware
monitoring controller. At the moment this controller supports a single fan
supervisor. In the future there might be other flavours and additional
hardware monitoring might be supported.

The fan supervisor has a 7 bit counter register and a counter period of 1
second. If the 7 bit counter overflows, the supervisor will automatically
switch to x8 mode to support a wider input range at the loss of
granularity.

## Sysfs entries

The following attributes are supported.

|  |  |
| --- | --- |
| fan1\_input | Fan RPM. Assuming 2 pulses per revolution. |
