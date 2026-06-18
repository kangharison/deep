# Kernel driver nouveau

> 출처(원문): https://docs.kernel.org/driver-api/thermal/nouveau_thermal.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver nouveau

Supported chips:

* NV43+

Authors: Martin Peres (mupuf) <[martin.peres@free.fr](mailto:martin.peres%40free.fr)>

## Description

This driver allows to read the GPU core temperature, drive the GPU fan and
set temperature alarms.

Currently, due to the absence of in-kernel API to access HWMON drivers, Nouveau
cannot access any of the i2c external monitoring chips it may find. If you
have one of those, temperature and/or fan management through Nouveau’s HWMON
interface is likely not to work. This document may then not cover your situation
entirely.

## Temperature management

Temperature is exposed under as a read-only HWMON attribute temp1\_input.

In order to protect the GPU from overheating, Nouveau supports 4 configurable
temperature thresholds:

> * Fan\_boost:
>   :   Fan speed is set to 100% when reaching this temperature;
> * Downclock:
>   :   The GPU will be downclocked to reduce its power dissipation;
> * Critical:
>   :   The GPU is put on hold to further lower power dissipation;
> * Shutdown:
>   :   Shut the computer down to protect your GPU.

WARNING:
:   Some of these thresholds may not be used by Nouveau depending
    on your chipset.

The default value for these thresholds comes from the GPU’s vbios. These
thresholds can be configured thanks to the following HWMON attributes:

> * Fan\_boost: temp1\_auto\_point1\_temp and temp1\_auto\_point1\_temp\_hyst;
> * Downclock: temp1\_max and temp1\_max\_hyst;
> * Critical: temp1\_crit and temp1\_crit\_hyst;
> * Shutdown: temp1\_emergency and temp1\_emergency\_hyst.

NOTE: Remember that the values are stored as milli degrees Celsius. Don’t forget
to multiply!

## Fan management

Not all cards have a drivable fan. If you do, then the following HWMON
attributes should be available:

> * pwm1\_enable:
>   :   Current fan management mode (NONE, MANUAL or AUTO);
> * pwm1:
>   :   Current PWM value (power percentage);
> * pwm1\_min:
>   :   The minimum PWM speed allowed;
> * pwm1\_max:
>   :   The maximum PWM speed allowed (bypassed when hitting Fan\_boost);

You may also have the following attribute:

> * fan1\_input:
>   :   Speed in RPM of your fan.

Your fan can be driven in different modes:

> * 0: The fan is left untouched;
> * 1: The fan can be driven in manual (use pwm1 to change the speed);
> * 2; The fan is driven automatically depending on the temperature.

NOTE:
:   Be sure to use the manual mode if you want to drive the fan speed manually

NOTE2:
:   When operating in manual mode outside the vbios-defined
    [PWM\_min, PWM\_max] range, the reported fan speed (RPM) may not be accurate
    depending on your hardware.

## Bug reports

Thermal management on Nouveau is new and may not work on all cards. If you have
inquiries, please ping mupuf on IRC (#nouveau, OFTC).

Bug reports should be filled on Freedesktop’s bug tracker. Please follow
<https://nouveau.freedesktop.org/wiki/Bugs>
