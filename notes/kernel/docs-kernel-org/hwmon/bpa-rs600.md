# Kernel driver bpa-rs600

> 출처(원문): https://docs.kernel.org/hwmon/bpa-rs600.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver bpa-rs600

Supported chips:

> * BPA-RS600-120
>
>   Datasheet: Publicly available at the BluTek website
>   :   <http://blutekpower.com/wp-content/uploads/2019/01/BPA-RS600-120-07-19-2018.pdf>

Authors:
:   * Chris Packham <[chris.packham@alliedtelesis.co.nz](mailto:chris.packham%40alliedtelesis.co.nz)>

## Description

The BPA-RS600 is a compact 600W removable power supply module.

## Usage Notes

This driver does not probe for PMBus devices. You will have to instantiate
devices explicitly.

## Sysfs attributes

|  |  |
| --- | --- |
| curr1\_label | “iin” |
| curr1\_input | Measured input current |
| curr1\_max | Maximum input current |
| curr1\_max\_alarm | Input current high alarm |
| curr2\_label | “iout1” |
| curr2\_input | Measured output current |
| curr2\_max | Maximum output current |
| curr2\_max\_alarm | Output current high alarm |
| fan1\_input | Measured fan speed |
| fan1\_alarm | Fan warning |
| fan1\_fault | Fan fault |
| in1\_label | “vin” |
| in1\_input | Measured input voltage |
| in1\_max | Maximum input voltage |
| in1\_max\_alarm | Input voltage high alarm |
| in1\_min | Minimum input voltage |
| in1\_min\_alarm | Input voltage low alarm |
| in2\_label | “vout1” |
| in2\_input | Measured output voltage |
| in2\_max | Maximum output voltage |
| in2\_max\_alarm | Output voltage high alarm |
| in2\_min | Maximum output voltage |
| in2\_min\_alarm | Output voltage low alarm |
| power1\_label | “pin” |
| power1\_input | Measured input power |
| power1\_alarm | Input power alarm |
| power1\_max | Maximum input power |
| power2\_label | “pout1” |
| power2\_input | Measured output power |
| power2\_max | Maximum output power |
| power2\_max\_alarm | Output power high alarm |
| temp1\_input | Measured temperature around input connector |
| temp1\_alarm | Temperature alarm |
| temp2\_input | Measured temperature around output connector |
| temp2\_alarm | Temperature alarm |
