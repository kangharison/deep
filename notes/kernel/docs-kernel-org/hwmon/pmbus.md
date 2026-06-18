# Kernel driver pmbus

> 출처(원문): https://docs.kernel.org/hwmon/pmbus.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver pmbus

Supported chips:

> * Flex BMR310, BMR453, BMR454, BMR456, BMR457, BMR458, BMR480,
>   BMR490, BMR491, BMR492
>
>   Prefixes: ‘bmr310’, ‘bmr453’, ‘bmr454’, ‘bmr456’, ‘bmr457’, ‘bmr458’, ‘bmr480’,
>   ‘bmr490’, ‘bmr491’, ‘bmr492’
>
>   Addresses scanned: -
>
>   Datasheets:
>
>   > <https://flexpowermodules.com/products>
> * ON Semiconductor ADP4000, NCP4200, NCP4208
>
>   Prefixes: ‘adp4000’, ‘ncp4200’, ‘ncp4208’
>
>   Addresses scanned: -
>
>   Datasheets:
>
>   > <https://www.onsemi.com/pub_link/Collateral/ADP4000-D.PDF>
>   >
>   > <https://www.onsemi.com/pub_link/Collateral/NCP4200-D.PDF>
>   >
>   > <https://www.onsemi.com/pub_link/Collateral/JUNE%202009-%20REV.%200.PDF>
> * Lineage Power
>
>   Prefixes: ‘mdt040’, ‘pdt003’, ‘pdt006’, ‘pdt012’, ‘udt020’
>
>   Addresses scanned: -
>
>   Datasheets:
>
>   > <http://www.lineagepower.com/oem/pdf/PDT003A0X.pdf>
>   >
>   > <http://www.lineagepower.com/oem/pdf/PDT006A0X.pdf>
>   >
>   > <http://www.lineagepower.com/oem/pdf/PDT012A0X.pdf>
>   >
>   > <http://www.lineagepower.com/oem/pdf/UDT020A0X.pdf>
>   >
>   > <http://www.lineagepower.com/oem/pdf/MDT040A0X.pdf>
> * Texas Instruments TPS40400, TPS544B20, TPS544B25, TPS544C20, TPS544C25
>
>   Prefixes: ‘tps40400’, ‘tps544b20’, ‘tps544b25’, ‘tps544c20’, ‘tps544c25’
>
>   Addresses scanned: -
>
>   Datasheets:
>
>   > <https://www.ti.com/lit/gpn/tps40400>
>   >
>   > <https://www.ti.com/lit/gpn/tps544b20>
>   >
>   > <https://www.ti.com/lit/gpn/tps544b25>
>   >
>   > <https://www.ti.com/lit/gpn/tps544c20>
>   >
>   > <https://www.ti.com/lit/gpn/tps544c25>
> * Maxim MAX20796
>
>   Prefix: ‘max20796’
>
>   Addresses scanned: -
>
>   Datasheet:
>
>   > <https://www.analog.com/media/en/technical-documentation/data-sheets/MAX20796.pdf>
> * Generic PMBus devices
>
>   Prefix: ‘pmbus’
>
>   Addresses scanned: -
>
>   Datasheet: n.a.

Author: Guenter Roeck <[linux@roeck-us.net](mailto:linux%40roeck-us.net)>

## Description

This driver supports hardware monitoring for various PMBus compliant devices.
It supports voltage, current, power, and temperature sensors as supported
by the device.

Each monitored channel has its own high and low limits, plus a critical
limit.

Fan support will be added in a later version of this driver.

## Usage Notes

This driver does not probe for PMBus devices, since there is no register
which can be safely used to identify the chip (The MFG\_ID register is not
supported by all chips), and since there is no well defined address range for
PMBus devices. You will have to instantiate the devices explicitly.

Example: the following will load the driver for an LTC2978 at address 0x60
on I2C bus #1:

```
$ modprobe pmbus
$ echo ltc2978 0x60 > /sys/bus/i2c/devices/i2c-1/new_device
```

## Platform data support

Support for additional PMBus chips can be added by defining chip parameters in
a new chip specific driver file. For example, (untested) code to add support for
Emerson DS1200 power modules might look as follows:

```
static struct pmbus_driver_info ds1200_info = {
      .pages = 1,
      /* Note: All other sensors are in linear mode */
      .direct[PSC_VOLTAGE_OUT] = true,
      .direct[PSC_TEMPERATURE] = true,
      .direct[PSC_CURRENT_OUT] = true,
      .m[PSC_VOLTAGE_IN] = 1,
      .b[PSC_VOLTAGE_IN] = 0,
      .R[PSC_VOLTAGE_IN] = 3,
      .m[PSC_VOLTAGE_OUT] = 1,
      .b[PSC_VOLTAGE_OUT] = 0,
      .R[PSC_VOLTAGE_OUT] = 3,
      .m[PSC_TEMPERATURE] = 1,
      .b[PSC_TEMPERATURE] = 0,
      .R[PSC_TEMPERATURE] = 3,
      .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_STATUS_INPUT
                 | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
                 | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
                 | PMBUS_HAVE_PIN | PMBUS_HAVE_POUT
                 | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP
                 | PMBUS_HAVE_FAN12 | PMBUS_HAVE_STATUS_FAN12,
};

static int ds1200_probe(struct i2c_client *client)
{
      return pmbus_do_probe(client, &ds1200_info);
}

static const struct i2c_device_id ds1200_id[] = {
      {"ds1200"},
      {}
};

MODULE_DEVICE_TABLE(i2c, ds1200_id);

/* This is the driver that will be inserted */
static struct i2c_driver ds1200_driver = {
      .driver = {
                 .name = "ds1200",
                 },
      .probe = ds1200_probe,
      .id_table = ds1200_id,
};

static int __init ds1200_init(void)
{
      return i2c_add_driver(&ds1200_driver);
}

static void __exit ds1200_exit(void)
{
      i2c_del_driver(&ds1200_driver);
}
```

## Sysfs entries

When probing the chip, the driver identifies which PMBus registers are
supported, and determines available sensors from this information.
Attribute files only exist if respective sensors are supported by the chip.
Labels are provided to inform the user about the sensor associated with
a given sysfs entry.

The following attributes are supported. Limits are read-write; all other
attributes are read-only.

|  |  |
| --- | --- |
| inX\_input | Measured voltage. From READ\_VIN or READ\_VOUT register. |
| inX\_min | Minimum Voltage. From VIN\_UV\_WARN\_LIMIT or VOUT\_UV\_WARN\_LIMIT register. |
| inX\_max | Maximum voltage. From VIN\_OV\_WARN\_LIMIT or VOUT\_OV\_WARN\_LIMIT register. |
| inX\_lcrit | Critical minimum Voltage. From VIN\_UV\_FAULT\_LIMIT or VOUT\_UV\_FAULT\_LIMIT register. |
| inX\_crit | Critical maximum voltage. From VIN\_OV\_FAULT\_LIMIT or VOUT\_OV\_FAULT\_LIMIT register. |
| inX\_min\_alarm | Voltage low alarm. From VOLTAGE\_UV\_WARNING status. |
| inX\_max\_alarm | Voltage high alarm. From VOLTAGE\_OV\_WARNING status. |
| inX\_lcrit\_alarm | Voltage critical low alarm. From VOLTAGE\_UV\_FAULT status. |
| inX\_crit\_alarm | Voltage critical high alarm. From VOLTAGE\_OV\_FAULT status. |
| inX\_label | “vin”, “vcap”, or “voutY” |
| inX\_rated\_min | Minimum rated voltage. From MFR\_VIN\_MIN or MFR\_VOUT\_MIN register. |
| inX\_rated\_max | Maximum rated voltage. From MFR\_VIN\_MAX or MFR\_VOUT\_MAX register. |
| currX\_input | Measured current. From READ\_IIN or READ\_IOUT register. |
| currX\_max | Maximum current. From IIN\_OC\_WARN\_LIMIT or IOUT\_OC\_WARN\_LIMIT register. |
| currX\_lcrit | Critical minimum output current. From IOUT\_UC\_FAULT\_LIMIT register. |
| currX\_crit | Critical maximum current. From IIN\_OC\_FAULT\_LIMIT or IOUT\_OC\_FAULT\_LIMIT register. |
| currX\_alarm | Current high alarm. From IIN\_OC\_WARNING or IOUT\_OC\_WARNING status. |
| currX\_max\_alarm | Current high alarm. From IIN\_OC\_WARN\_LIMIT or IOUT\_OC\_WARN\_LIMIT status. |
| currX\_lcrit\_alarm | Output current critical low alarm. From IOUT\_UC\_FAULT status. |
| currX\_crit\_alarm | Current critical high alarm. From IIN\_OC\_FAULT or IOUT\_OC\_FAULT status. |
| currX\_label | “iin”, “iinY”, “iinY.Z”, “ioutY”, or “ioutY.Z”, where Y reflects the page number and Z reflects the phase. |
| currX\_rated\_max | Maximum rated current. From MFR\_IIN\_MAX or MFR\_IOUT\_MAX register. |
| powerX\_input | Measured power. From READ\_PIN or READ\_POUT register. |
| powerX\_cap | Output power cap. From POUT\_MAX register. |
| powerX\_max | Power limit. From PIN\_OP\_WARN\_LIMIT or POUT\_OP\_WARN\_LIMIT register. |
| powerX\_crit | Critical output power limit. From POUT\_OP\_FAULT\_LIMIT register. |
| powerX\_alarm | Power high alarm. From PIN\_OP\_WARNING or POUT\_OP\_WARNING status. |
| powerX\_crit\_alarm | Output power critical high alarm. From POUT\_OP\_FAULT status. |
| powerX\_label | “pin”, “pinY”, “pinY.Z”, “poutY”, or “poutY.Z”, where Y reflects the page number and Z reflects the phase. |
| powerX\_rated\_max | Maximum rated power. From MFR\_PIN\_MAX or MFR\_POUT\_MAX register. |
| tempX\_input | Measured temperature. From READ\_TEMPERATURE\_X register. |
| tempX\_min | Minimum temperature. From UT\_WARN\_LIMIT register. |
| tempX\_max | Maximum temperature. From OT\_WARN\_LIMIT register. |
| tempX\_lcrit | Critical low temperature. From UT\_FAULT\_LIMIT register. |
| tempX\_crit | Critical high temperature. From OT\_FAULT\_LIMIT register. |
| tempX\_min\_alarm | Chip temperature low alarm. Set by comparing READ\_TEMPERATURE\_X with UT\_WARN\_LIMIT if TEMP\_UT\_WARNING status is set. |
| tempX\_max\_alarm | Chip temperature high alarm. Set by comparing READ\_TEMPERATURE\_X with OT\_WARN\_LIMIT if TEMP\_OT\_WARNING status is set. |
| tempX\_lcrit\_alarm | Chip temperature critical low alarm. Set by comparing READ\_TEMPERATURE\_X with UT\_FAULT\_LIMIT if TEMP\_UT\_FAULT status is set. |
| tempX\_crit\_alarm | Chip temperature critical high alarm. Set by comparing READ\_TEMPERATURE\_X with OT\_FAULT\_LIMIT if TEMP\_OT\_FAULT status is set. |
| tempX\_rated\_min | Minimum rated temperature. From MFR\_TAMBIENT\_MIN register. |
| tempX\_rated\_max | Maximum rated temperature. From MFR\_TAMBIENT\_MAX, MFR\_MAX\_TEMP\_1, MFR\_MAX\_TEMP\_2 or MFR\_MAX\_TEMP\_3 register. |
