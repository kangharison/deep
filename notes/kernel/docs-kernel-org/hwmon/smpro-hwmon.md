# Kernel driver Ampere(R)’s Altra(R) SMpro hwmon

> 출처(원문): https://docs.kernel.org/hwmon/smpro-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver Ampere(R)’s Altra(R) SMpro hwmon

Supported chips:

> * Ampere(R) Altra(R)
>
>   Prefix: `smpro`
>
>   Reference: Altra SoC BMC Interface Specification

Author: Thu Nguyen <[thu@os.amperecomputing.com](mailto:thu%40os.amperecomputing.com)>

## Description

The smpro-hwmon driver supports hardware monitoring for Ampere(R) Altra(R)
SoCs based on the SMpro co-processor (SMpro). The following sensor metrics
are supported by the driver:

> * temperature
> * voltage
> * current
> * power

The interface provides the registers to query the various sensors and
their values which are then exported to userspace by this driver.

## Usage Notes

The driver creates at least two sysfs files for each sensor.

* `<sensor_type><idx>_label` reports the sensor label.
* `<sensor_type><idx>_input` returns the sensor value.

The sysfs files are allocated in the SMpro rootfs folder, with one root
directory for each instance.

When the SoC is turned off, the driver will fail to read registers and
return `-ENXIO`.

## Sysfs entries

The following sysfs files are supported:

* Ampere(R) Altra(R):

  | Name | Unit | Perm | Description |
  | --- | --- | --- | --- |
  | temp1\_input | millicelsius | RO | SoC temperature |
  | temp2\_input | millicelsius | RO | Max temperature reported among SoC VRDs |
  | temp2\_crit | millicelsius | RO | SoC VRD HOT Threshold temperature |
  | temp3\_input | millicelsius | RO | Max temperature reported among DIMM VRDs |
  | temp4\_input | millicelsius | RO | Max temperature reported among Core VRDs |
  | temp5\_input | millicelsius | RO | Temperature of DIMM0 on CH0 |
  | temp5\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp6\_input | millicelsius | RO | Temperature of DIMM0 on CH1 |
  | temp6\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp7\_input | millicelsius | RO | Temperature of DIMM0 on CH2 |
  | temp7\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp8\_input | millicelsius | RO | Temperature of DIMM0 on CH3 |
  | temp8\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp9\_input | millicelsius | RO | Temperature of DIMM0 on CH4 |
  | temp9\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp10\_input | millicelsius | RO | Temperature of DIMM0 on CH5 |
  | temp10\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp11\_input | millicelsius | RO | Temperature of DIMM0 on CH6 |
  | temp11\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp12\_input | millicelsius | RO | Temperature of DIMM0 on CH7 |
  | temp12\_crit | millicelsius | RO | MEM HOT Threshold for all DIMMs |
  | temp13\_input | millicelsius | RO | Max temperature reported among RCA VRDs |
  | in0\_input | millivolts | RO | Core voltage |
  | in1\_input | millivolts | RO | SoC voltage |
  | in2\_input | millivolts | RO | DIMM VRD1 voltage |
  | in3\_input | millivolts | RO | DIMM VRD2 voltage |
  | in4\_input | millivolts | RO | RCA VRD voltage |
  | cur1\_input | milliamperes | RO | Core VRD current |
  | cur2\_input | milliamperes | RO | SoC VRD current |
  | cur3\_input | milliamperes | RO | DIMM VRD1 current |
  | cur4\_input | milliamperes | RO | DIMM VRD2 current |
  | cur5\_input | milliamperes | RO | RCA VRD current |
  | power1\_input | microwatts | RO | Core VRD power |
  | power2\_input | microwatts | RO | SoC VRD power |
  | power3\_input | microwatts | RO | DIMM VRD1 power |
  | power4\_input | microwatts | RO | DIMM VRD2 power |
  | power5\_input | microwatts | RO | RCA VRD power |

  Example:

  ```
  # cat in0_input
  830
  # cat temp1_input
  37000
  # cat curr1_input
  9000
  # cat power5_input
  19500000
  ```
