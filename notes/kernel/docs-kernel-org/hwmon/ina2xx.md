# Kernel driver ina2xx

> 출처(원문): https://docs.kernel.org/hwmon/ina2xx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver ina2xx

Supported chips:

> * Texas Instruments INA219
>
>   Prefix: ‘ina219’
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>
> * Texas Instruments INA220
>
>   Prefix: ‘ina220’
>
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>
> * Texas Instruments INA226
>
>   Prefix: ‘ina226’
>
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>
> * Texas Instruments INA230
>
>   Prefix: ‘ina230’
>
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>
> * Texas Instruments INA231
>
>   Prefix: ‘ina231’
>
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>
> * Texas Instruments INA260
>
>   Prefix: ‘ina260’
>
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>
> * Silergy SY24655
>
>   Prefix: ‘sy24655’
>
>   Addresses: I2C 0x40 - 0x4f
>
>   Datasheet: Publicly available at the Silergy website
>
>   > <https://us1.silergy.com/>
> * Texas Instruments INA234
>
>   Prefix: ‘ina234’
>
>   Addresses: I2C 0x40 - 0x43
>
>   Datasheet: Publicly available at the Texas Instruments website
>
>   > <https://www.ti.com/>

Author: Lothar Felten <[lothar.felten@gmail.com](mailto:lothar.felten%40gmail.com)>

## Description

The INA219 is a high-side current shunt and power monitor with an I2C
interface. The INA219 monitors both shunt drop and supply voltage, with
programmable conversion times and filtering.

The INA220 is a high or low side current shunt and power monitor with an I2C
interface. The INA220 monitors both shunt drop and supply voltage.

The INA226 is a current shunt and power monitor with an I2C interface.
The INA226 monitors both a shunt voltage drop and bus supply voltage.

INA230, INA231, and INA234 are high or low side current shunt and power monitors
with an I2C interface. The chips monitor both a shunt voltage drop and
bus supply voltage.

INA260 is a high or low side current and power monitor with integrated shunt
resistor.

The SY24655 is a high- and low-side current shunt and power monitor with an I2C
interface. The SY24655 supports both shunt drop and supply voltage, with
programmable calibration value and conversion times. The SY24655 can also
calculate average power for use in energy conversion.

The shunt value in micro-ohms can be set via platform data or device tree at
compile-time or via the shunt\_resistor attribute in sysfs at run-time. Please
refer to the Documentation/devicetree/bindings/hwmon/ti,ina2xx.yaml for bindings
if the device tree is used.

Additionally ina226 supports update\_interval attribute as described in
[Naming and data format standards for sysfs files](sysfs-interface.html). Internally the interval is the sum of
bus and shunt voltage conversion times multiplied by the averaging rate. We
don’t touch the conversion times and only modify the number of averages. The
lower limit of the update\_interval is 2 ms, the upper limit is 2253 ms.
The actual programmed interval may vary from the desired value.

## General sysfs entries

|  |  |
| --- | --- |
| in0\_input | Shunt voltage(mV) channel |
| in1\_input | Bus voltage(mV) channel |
| curr1\_input | Current(mA) measurement channel |
| power1\_input | Power(uW) measurement channel |
| shunt\_resistor | Shunt resistance(uOhm) channel (not for ina260) |

## Additional sysfs entries

Additional entries are available for the following chips:

> * ina226
> * ina230
> * ina231
> * ina234
> * ina260
> * sy24655

|  |  |
| --- | --- |
| curr1\_lcrit | Critical low current |
| curr1\_crit | Critical high current |
| curr1\_lcrit\_alarm | Current critical low alarm |
| curr1\_crit\_alarm | Current critical high alarm |
| in0\_lcrit | Critical low shunt voltage |
| in0\_crit | Critical high shunt voltage |
| in0\_lcrit\_alarm | Shunt voltage critical low alarm |
| in0\_crit\_alarm | Shunt voltage critical high alarm |
| in1\_lcrit | Critical low bus voltage |
| in1\_crit | Critical high bus voltage |
| in1\_lcrit\_alarm | Bus voltage critical low alarm |
| in1\_crit\_alarm | Bus voltage critical high alarm |
| power1\_crit | Critical high power |
| power1\_crit\_alarm | Power critical high alarm |
| update\_interval | data conversion time; affects number of samples used to average results for shunt and bus voltages. |

## Sysfs entries for sy24655 only

|  |  |
| --- | --- |
| power1\_average | average power from last reading to the present. |

Note

* Configure shunt\_resistor before configure power1\_crit, because power
  value is calculated based on shunt\_resistor set.
* Because of the underlying register implementation, only one \*crit setting
  and its alarm can be active. Writing to one \*crit setting clears other
  \*crit settings and alarms. Writing 0 to any \*crit setting clears all
  \*crit settings and alarms.
