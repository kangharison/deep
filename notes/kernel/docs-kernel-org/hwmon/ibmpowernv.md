# Kernel Driver IBMPOWERNV

> 출처(원문): https://docs.kernel.org/hwmon/ibmpowernv.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel Driver IBMPOWERNV

Supported systems:

> * Any recent IBM P servers based on POWERNV platform

Author: Neelesh Gupta

## Description

This driver implements reading the platform sensors data like temperature/fan/
voltage/power for ‘POWERNV’ platform.

The driver uses the platform device infrastructure. It probes the device tree
for sensor devices during the \_\_init phase and registers them with the ‘hwmon’.
‘hwmon’ populates the ‘sysfs’ tree having attribute files, each for a given
sensor type and its attribute data.

All the nodes in the DT appear under “/ibm,opal/sensors” and each valid node in
the DT maps to an attribute file in ‘sysfs’. The node exports unique ‘sensor-id’
which the driver uses to make an OPAL call to the firmware.

## Usage notes

The driver is built statically with the kernel by enabling the config
CONFIG\_SENSORS\_IBMPOWERNV. It can also be built as module ‘ibmpowernv’.

## Sysfs attributes

|  |  |
| --- | --- |
| fanX\_input | Measured RPM value. |
| fanX\_min | Threshold RPM for alert generation. |
| fanX\_fault | * 0: No fail condition * 1: Failing fan |
| tempX\_input | Measured ambient temperature. |
| tempX\_max | Threshold ambient temperature for alert generation. |
| tempX\_highest | Historical maximum temperature |
| tempX\_lowest | Historical minimum temperature |
| tempX\_enable | Enable/disable all temperature sensors belonging to the sub-group. In POWER9, this attribute corresponds to each OCC. Using this attribute each OCC can be asked to disable/enable all of its temperature sensors.   * 1: Enable * 0: Disable |
| inX\_input | Measured power supply voltage (millivolt) |
| inX\_fault | * 0: No fail condition. * 1: Failing power supply. |
| inX\_highest | Historical maximum voltage |
| inX\_lowest | Historical minimum voltage |
| inX\_enable | Enable/disable all voltage sensors belonging to the sub-group. In POWER9, this attribute corresponds to each OCC. Using this attribute each OCC can be asked to disable/enable all of its voltage sensors.   * 1: Enable * 0: Disable |
| powerX\_input | Power consumption (microWatt) |
| powerX\_input\_highest | Historical maximum power |
| powerX\_input\_lowest | Historical minimum power |
| powerX\_enable | Enable/disable all power sensors belonging to the sub-group. In POWER9, this attribute corresponds to each OCC. Using this attribute each OCC can be asked to disable/enable all of its power sensors.   * 1: Enable * 0: Disable |
| currX\_input | Measured current (milliampere) |
| currX\_highest | Historical maximum current |
| currX\_lowest | Historical minimum current |
| currX\_enable | Enable/disable all current sensors belonging to the sub-group. In POWER9, this attribute corresponds to each OCC. Using this attribute each OCC can be asked to disable/enable all of its current sensors.   * 1: Enable * 0: Disable |
| energyX\_input | Cumulative energy (microJoule) |
