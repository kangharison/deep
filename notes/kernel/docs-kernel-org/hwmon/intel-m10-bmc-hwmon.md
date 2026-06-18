# Kernel driver intel-m10-bmc-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/intel-m10-bmc-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver intel-m10-bmc-hwmon

Supported chips:

> * Intel MAX 10 BMC for Intel PAC N3000
>
>   Prefix: ‘n3000bmc-hwmon’

Author: Xu Yilun <[yilun.xu@intel.com](mailto:yilun.xu%40intel.com)>

## Description

This driver adds the temperature, voltage, current and power reading
support for the Intel MAX 10 Board Management Controller (BMC) chip.
The BMC chip is integrated in some Intel Programmable Acceleration
Cards (PAC). It connects to a set of sensor chips to monitor the
sensor data of different components on the board. The BMC firmware is
responsible for sensor data sampling and recording in shared
registers. The host driver reads the sensor data from these shared
registers and exposes them to users as hwmon interfaces.

The BMC chip is implemented using the Intel MAX 10 CPLD. It could be
reprogramed to some variants in order to support different Intel
PACs. The driver is designed to be able to distinguish between the
variants, but now it only supports the BMC for Intel PAC N3000.

## Sysfs attributes

The following attributes are supported:

* Intel MAX 10 BMC for Intel PAC N3000:

|  |  |
| --- | --- |
| tempX\_input | Temperature of the component (specified by tempX\_label) |
| tempX\_max | Temperature maximum setpoint of the component |
| tempX\_crit | Temperature critical setpoint of the component |
| tempX\_max\_hyst | Hysteresis for temperature maximum of the component |
| tempX\_crit\_hyst | Hysteresis for temperature critical of the component |
| temp1\_label | “Board Temperature” |
| temp2\_label | “FPGA Die Temperature” |
| temp3\_label | “QSFP0 Temperature” |
| temp4\_label | “QSFP1 Temperature” |
| temp5\_label | “Retimer A Temperature” |
| temp6\_label | “Retimer A SerDes Temperature” |
| temp7\_label | “Retimer B Temperature” |
| temp8\_label | “Retimer B SerDes Temperature” |
| inX\_input | Measured voltage of the component (specified by inX\_label) |
| in0\_label | “QSFP0 Supply Voltage” |
| in1\_label | “QSFP1 Supply Voltage” |
| in2\_label | “FPGA Core Voltage” |
| in3\_label | “12V Backplane Voltage” |
| in4\_label | “1.2V Voltage” |
| in5\_label | “12V AUX Voltage” |
| in6\_label | “1.8V Voltage” |
| in7\_label | “3.3V Voltage” |
| currX\_input | Measured current of the component (specified by currX\_label) |
| curr1\_label | “FPGA Core Current” |
| curr2\_label | “12V Backplane Current” |
| curr3\_label | “12V AUX Current” |
| powerX\_input | Measured power of the component (specified by powerX\_label) |
| power1\_label | “Board Power” |

All the attributes are read-only.
