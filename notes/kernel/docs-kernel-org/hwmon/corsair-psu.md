# Kernel driver corsair-psu

> 출처(원문): https://docs.kernel.org/hwmon/corsair-psu.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver corsair-psu

Supported devices:

* Corsair Power Supplies

  Corsair HX550i

  Corsair HX650i

  Corsair HX750i

  Corsair HX850i

  Corsair HX1000i (Legacy and Series 2023)

  Corsair HX1200i (Legacy, Series 2023 and Series 2025)

  Corsair HX1500i (Legacy and Series 2023)

  Corsair RM550i

  Corsair RM650i

  Corsair RM750i

  Corsair RM850i

  Corsair RM1000i

Author: Wilken Gottwalt

## Description

This driver implements the sysfs interface for the Corsair PSUs with a HID protocol
interface of the HXi and RMi series.
These power supplies provide access to a micro-controller with 2 attached
temperature sensors, 1 fan rpm sensor, 4 sensors for volt levels, 4 sensors for
power usage and 4 sensors for current levels and additional non-sensor information
like uptimes.

## Sysfs entries

|  |  |
| --- | --- |
| curr1\_input | Total current usage |
| curr2\_input | Current on the 12v psu rail |
| curr2\_crit | Current max critical value on the 12v psu rail |
| curr3\_input | Current on the 5v psu rail |
| curr3\_crit | Current max critical value on the 5v psu rail |
| curr4\_input | Current on the 3.3v psu rail |
| curr4\_crit | Current max critical value on the 3.3v psu rail |
| fan1\_input | RPM of psu fan |
| in0\_input | Voltage of the psu ac input |
| in1\_input | Voltage of the 12v psu rail |
| in1\_crit | Voltage max critical value on the 12v psu rail |
| in1\_lcrit | Voltage min critical value on the 12v psu rail |
| in2\_input | Voltage of the 5v psu rail |
| in2\_crit | Voltage max critical value on the 5v psu rail |
| in2\_lcrit | Voltage min critical value on the 5v psu rail |
| in3\_input | Voltage of the 3.3v psu rail |
| in3\_crit | Voltage max critical value on the 3.3v psu rail |
| in3\_lcrit | Voltage min critical value on the 3.3v psu rail |
| power1\_input | Total power usage |
| power2\_input | Power usage of the 12v psu rail |
| power3\_input | Power usage of the 5v psu rail |
| power4\_input | Power usage of the 3.3v psu rail |
| pwm1 | PWM value, read only |
| pwm1\_enable | PWM mode, read only |
| temp1\_input | Temperature of the psu vrm component |
| temp1\_crit | Temperature max cirtical value of the psu vrm component |
| temp2\_input | Temperature of the psu case |
| temp2\_crit | Temperature max critical value of psu case |

## Usage Notes

It is an USB HID device, so it is auto-detected, supports hot-swapping and
several devices at once.

Flickering values in the rail voltage levels can be an indicator for a failing
PSU. Accordingly to the default automatic fan speed plan the fan starts at about
30% of the wattage rating. If this does not happen, a fan failure is likely. The
driver also provides some additional useful values via debugfs, which do not fit
into the hwmon class.

## Debugfs entries

|  |  |
| --- | --- |
| ocpmode | Single or multi rail mode of the PCIe power connectors |
| product | Product name of the psu |
| uptime | Session uptime of the psu |
| uptime\_total | Total uptime of the psu |
| vendor | Vendor name of the psu |
