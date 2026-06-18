# Kernel driver aht10

> 출처(원문): https://docs.kernel.org/hwmon/aht10.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver aht10

Supported chips:

> * Aosong AHT10/AHT20
>
>   Prefix: ‘aht10’
>
>   Addresses scanned: None
>
>   Datasheet(AHT10):
>
>   > Chinese: <http://www.aosong.com/userfiles/files/media/AHT10%E4%BA%A7%E5%93%81%E6%89%8B%E5%86%8C%20A3%2020201210.pdf>
>   > English: <https://server4.eca.ir/eshop/AHT10/Aosong_AHT10_en_draft_0c.pdf>
>
>   Datasheet(AHT20):
>
>   > English: <http://www.aosong.com/userfiles/files/media/Data%20Sheet%20AHT20.pdf>
> * Aosong DHT20
>
>   Prefix: ‘dht20’
>
>   Addresses scanned: None
>
>   Datasheet: <https://www.digikey.co.nz/en/htmldatasheets/production/9184855/0/0/1/101020932>

Author: Johannes Cornelis Draaijer <[jcdra1@gmail.com](mailto:jcdra1%40gmail.com)>

## Description

The AHT10/AHT20 is a Temperature and Humidity sensor

The address of this i2c device may only be 0x38

## Special Features

AHT20, DHT20 has additional CRC8 support which is sent as the last byte of the sensor
values.

## Usage Notes

This driver does not probe for AHT10/ATH20 devices, as there is no reliable
way to determine if an i2c chip is or isn’t an AHT10/AHT20. The device has
to be instantiated explicitly with the address 0x38. See
[How to instantiate I2C devices](../i2c/instantiating-devices.html) for details.

## Sysfs entries

|  |  |
| --- | --- |
| temp1\_input | Measured temperature in millidegrees Celsius |
| humidity1\_input | Measured humidity in %H |
| update\_interval | The minimum interval for polling the sensor, in milliseconds. Writable. Must be at least 2000. |
