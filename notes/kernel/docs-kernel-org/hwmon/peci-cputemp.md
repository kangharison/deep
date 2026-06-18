# Kernel driver peci-cputemp

> 출처(원문): https://docs.kernel.org/hwmon/peci-cputemp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver peci-cputemp

Supported chips:
:   One of Intel server CPUs listed below which is connected to a PECI bus.
    :   * Intel Xeon E5/E7 v3 server processors
          :   Intel Xeon E5-14xx v3 family
              Intel Xeon E5-24xx v3 family
              Intel Xeon E5-16xx v3 family
              Intel Xeon E5-26xx v3 family
              Intel Xeon E5-46xx v3 family
              Intel Xeon E7-48xx v3 family
              Intel Xeon E7-88xx v3 family
        * Intel Xeon E5/E7 v4 server processors
          :   Intel Xeon E5-16xx v4 family
              Intel Xeon E5-26xx v4 family
              Intel Xeon E5-46xx v4 family
              Intel Xeon E7-48xx v4 family
              Intel Xeon E7-88xx v4 family
        * Intel Xeon Scalable server processors
          :   Intel Xeon D family
              Intel Xeon Bronze family
              Intel Xeon Silver family
              Intel Xeon Gold family
              Intel Xeon Platinum family

    Datasheet: Available from <http://www.intel.com/design/literature.htm>

Author: Jae Hyun Yoo <[jae.hyun.yoo@linux.intel.com](mailto:jae.hyun.yoo%40linux.intel.com)>

## Description

This driver implements a generic PECI hwmon feature which provides Digital
Thermal Sensor (DTS) thermal readings of the CPU package and CPU cores that are
accessible via the processor PECI interface.

All temperature values are given in millidegree Celsius and will be measurable
only when the target CPU is powered on.

## Sysfs interface

|  |  |
| --- | --- |
| temp1\_label | “Die” |
| temp1\_input | Provides current die temperature of the CPU package. |
| temp1\_max | Provides thermal control temperature of the CPU package which is also known as Tcontrol. |
| temp1\_crit | Provides shutdown temperature of the CPU package which is also known as the maximum processor junction temperature, Tjmax or Tprochot. |
| temp1\_crit\_hyst | Provides the hysteresis temperature of the CPU package. Returns Tcontrol, the temperature at which the critical condition clears. |
| temp2\_label | “DTS” |
| temp2\_input | Provides current temperature of the CPU package scaled to match DTS thermal profile. |
| temp2\_max | Provides thermal control temperature of the CPU package which is also known as Tcontrol. |
| temp2\_crit | Provides shutdown temperature of the CPU package which is also known as the maximum processor junction temperature, Tjmax or Tprochot. |
| temp2\_crit\_hyst | Provides the hysteresis temperature of the CPU package. Returns Tcontrol, the temperature at which the critical condition clears. |
| temp3\_label | “Tcontrol” |
| temp3\_input | Provides current Tcontrol temperature of the CPU package which is also known as Fan Temperature target. Indicates the relative value from thermal monitor trip temperature at which fans should be engaged. |
| temp3\_crit | Provides Tcontrol critical value of the CPU package which is same to Tjmax. |
| temp4\_label | “Tthrottle” |
| temp4\_input | Provides current Tthrottle temperature of the CPU package. Used for throttling temperature. If this value is allowed and lower than Tjmax - the throttle will occur and reported at lower than Tjmax. |
| temp5\_label | “Tjmax” |
| temp5\_input | Provides the maximum junction temperature, Tjmax of the CPU package. |
| temp[6-N]\_label | Provides string “Core X”, where X is resolved core number. |
| temp[6-N]\_input | Provides current temperature of each core. |
