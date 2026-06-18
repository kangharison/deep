# Kernel driver bel-pfe

> 출처(원문): https://docs.kernel.org/hwmon/bel-pfe.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver bel-pfe

Supported chips:

> * BEL PFE1100
>
>   Prefixes: ‘pfe1100’
>
>   Addresses scanned: -
>
>   Datasheet: <https://www.belfuse.com/resources/datasheets/powersolutions/ds-bps-pfe1100-12-054xa.pdf>
> * BEL PFE3000
>
>   Prefixes: ‘pfe3000’
>
>   Addresses scanned: -
>
>   Datasheet: <https://www.belfuse.com/resources/datasheets/powersolutions/ds-bps-pfe3000-series.pdf>

Author: Tao Ren <[rentao.bupt@gmail.com](mailto:rentao.bupt%40gmail.com)>

## Description

This driver supports hardware monitoring for below power supply devices
which support PMBus Protocol:

> * BEL PFE1100
>
>   1100 Watt AC to DC power-factor-corrected (PFC) power supply.
>   PMBus Communication Manual is not publicly available.
> * BEL PFE3000
>
>   3000 Watt AC/DC power-factor-corrected (PFC) and DC-DC power supply.
>   PMBus Communication Manual is not publicly available.

The driver is a client driver to the core PMBus driver. Please see
[Kernel driver pmbus](pmbus.html) for details on PMBus client drivers.

## Usage Notes

This driver does not auto-detect devices. You will have to instantiate the
devices explicitly. Please see [How to instantiate I2C devices](../i2c/instantiating-devices.html) for
details.

Example: the following will load the driver for an PFE3000 at address 0x20
on I2C bus #1:

```
$ modprobe bel-pfe
$ echo pfe3000 0x20 > /sys/bus/i2c/devices/i2c-1/new_device
```

## Platform data support

The driver supports standard PMBus driver platform data.

## Sysfs entries

|  |  |
| --- | --- |
| curr1\_label | “iin” |
| curr1\_input | Measured input current |
| curr1\_max | Input current max value |
| curr1\_max\_alarm | Input current max alarm |
| curr[2-3]\_label | “iout[1-2]” |
| curr[2-3]\_input | Measured output current |
| curr[2-3]\_max | Output current max value |
| curr[2-3]\_max\_alarm | Output current max alarm |
| fan[1-2]\_input | Fan 1 and 2 speed in RPM |
| fan1\_target | Set fan speed reference for both fans |
| in1\_label | “vin” |
| in1\_input | Measured input voltage |
| in1\_crit | Input voltage critical max value |
| in1\_crit\_alarm | Input voltage critical max alarm |
| in1\_lcrit | Input voltage critical min value |
| in1\_lcrit\_alarm | Input voltage critical min alarm |
| in1\_max | Input voltage max value |
| in1\_max\_alarm | Input voltage max alarm |
| in2\_label | “vcap” |
| in2\_input | Hold up capacitor voltage |
| in[3-8]\_label | “vout[1-3,5-7]” |
| in[3-8]\_input | Measured output voltage |
| in[3-4]\_alarm | vout[1-2] output voltage alarm |
| power[1-2]\_label | “pin[1-2]” |
| power[1-2]\_input | Measured input power |
| power[1-2]\_alarm | Input power high alarm |
| power[3-4]\_label | “pout[1-2]” |
| power[3-4]\_input | Measured output power |
| temp[1-3]\_input | Measured temperature |
| temp[1-3]\_alarm | Temperature alarm |

Note

* curr3, fan2, vout[2-7], vcap, pin2, pout2 and temp3 attributes only
  exist for PFE3000.
