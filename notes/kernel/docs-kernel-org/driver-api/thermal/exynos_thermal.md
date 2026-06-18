# Kernel driver exynos_tmu

> 출처(원문): https://docs.kernel.org/driver-api/thermal/exynos_thermal.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver exynos\_tmu

Supported chips:

* ARM Samsung Exynos4, Exynos5 series of SoC

  Datasheet: Not publicly available

Authors: Donggeun Kim <[dg77.kim@samsung.com](mailto:dg77.kim%40samsung.com)>
Authors: Amit Daniel <[amit.daniel@samsung.com](mailto:amit.daniel%40samsung.com)>

## TMU controller Description:

This driver allows to read temperature inside Samsung Exynos4/5 series of SoC.

The chip only exposes the measured 8-bit temperature code value
through a register.
Temperature can be taken from the temperature code.
There are three equations converting from temperature to temperature code.

The three equations are:
:   1. Two point trimming:

       ```
       Tc = (T - 25) * (TI2 - TI1) / (85 - 25) + TI1
       ```
    2. One point trimming:

       ```
       Tc = T + TI1 - 25
       ```
    3. No trimming:

       ```
       Tc = T + 50
       ```

    Tc:
    :   Temperature code, T: Temperature,

    TI1:
    :   Trimming info for 25 degree Celsius (stored at TRIMINFO register)
        Temperature code measured at 25 degree Celsius which is unchanged

    TI2:
    :   Trimming info for 85 degree Celsius (stored at TRIMINFO register)
        Temperature code measured at 85 degree Celsius which is unchanged

TMU(Thermal Management Unit) in Exynos4/5 generates interrupt
when temperature exceeds pre-defined levels.
The maximum number of configurable threshold is five.
The threshold levels are defined as follows:

```
Level_0: current temperature > trigger_level_0 + threshold
Level_1: current temperature > trigger_level_1 + threshold
Level_2: current temperature > trigger_level_2 + threshold
Level_3: current temperature > trigger_level_3 + threshold
```

The threshold and each trigger\_level are set
through the corresponding registers.

When an interrupt occurs, this driver notify kernel thermal framework
with the function exynos\_report\_trigger.
Although an interrupt condition for level\_0 can be set,
it can be used to synchronize the cooling action.

## TMU driver description:

The exynos thermal driver is structured as:

```
                                      Kernel Core thermal framework
                              (thermal_core.c, step_wise.c, cpufreq_cooling.c)
                                                              ^
                                                              |
                                                              |
TMU configuration data -----> TMU Driver  <----> Exynos Core thermal wrapper
(exynos_tmu_data.c)         (exynos_tmu.c)       (exynos_thermal_common.c)
(exynos_tmu_data.h)         (exynos_tmu.h)       (exynos_thermal_common.h)
```

1. TMU configuration data:
   :   This consist of TMU register offsets/bitfields
       described through structure exynos\_tmu\_registers. Also several
       other platform data (`struct exynos_tmu_platform_data`) members
       are used to configure the TMU.
2. TMU driver:
   :   This component initialises the TMU controller and sets different
       thresholds. It invokes core thermal implementation with the call
       exynos\_report\_trigger.
3. Exynos Core thermal wrapper:
   :   This provides 3 wrapper function to use the
       Kernel core thermal framework. They are exynos\_unregister\_thermal,
       exynos\_register\_thermal and exynos\_report\_trigger.
