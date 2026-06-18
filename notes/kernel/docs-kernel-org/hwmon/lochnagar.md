# Kernel Driver Lochnagar

> 출처(원문): https://docs.kernel.org/hwmon/lochnagar.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel Driver Lochnagar

Supported systems:
:   * Cirrus Logic : Lochnagar 2

Author: Lucas A. Tanure Alves

## Description

Lochnagar 2 features built-in Current Monitor circuitry that allows for the
measurement of both voltage and current on up to eight of the supply voltage
rails provided to the minicards. The Current Monitor does not require any
hardware modifications or external circuitry to operate.

The current and voltage measurements are obtained through the standard register
map interface to the Lochnagar board controller, and can therefore be monitored
by software.

## Sysfs attributes

|  |  |
| --- | --- |
| temp1\_input | The Lochnagar board temperature (milliCelsius) |
| in0\_input | Measured voltage for DBVDD1 (milliVolts) |
| in0\_label | “DBVDD1” |
| curr1\_input | Measured current for DBVDD1 (milliAmps) |
| curr1\_label | “DBVDD1” |
| power1\_average | Measured average power for DBVDD1 (microWatts) |
| power1\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power1\_label | “DBVDD1” |
| in1\_input | Measured voltage for 1V8 DSP (milliVolts) |
| in1\_label | “1V8 DSP” |
| curr2\_input | Measured current for 1V8 DSP (milliAmps) |
| curr2\_label | “1V8 DSP” |
| power2\_average | Measured average power for 1V8 DSP (microWatts) |
| power2\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power2\_label | “1V8 DSP” |
| in2\_input | Measured voltage for 1V8 CDC (milliVolts) |
| in2\_label | “1V8 CDC” |
| curr3\_input | Measured current for 1V8 CDC (milliAmps) |
| curr3\_label | “1V8 CDC” |
| power3\_average | Measured average power for 1V8 CDC (microWatts) |
| power3\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power3\_label | “1V8 CDC” |
| in3\_input | Measured voltage for VDDCORE DSP (milliVolts) |
| in3\_label | “VDDCORE DSP” |
| curr4\_input | Measured current for VDDCORE DSP (milliAmps) |
| curr4\_label | “VDDCORE DSP” |
| power4\_average | Measured average power for VDDCORE DSP (microWatts) |
| power4\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power4\_label | “VDDCORE DSP” |
| in4\_input | Measured voltage for AVDD 1V8 (milliVolts) |
| in4\_label | “AVDD 1V8” |
| curr5\_input | Measured current for AVDD 1V8 (milliAmps) |
| curr5\_label | “AVDD 1V8” |
| power5\_average | Measured average power for AVDD 1V8 (microWatts) |
| power5\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power5\_label | “AVDD 1V8” |
| curr6\_input | Measured current for SYSVDD (milliAmps) |
| curr6\_label | “SYSVDD” |
| power6\_average | Measured average power for SYSVDD (microWatts) |
| power6\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power6\_label | “SYSVDD” |
| in6\_input | Measured voltage for VDDCORE CDC (milliVolts) |
| in6\_label | “VDDCORE CDC” |
| curr7\_input | Measured current for VDDCORE CDC (milliAmps) |
| curr7\_label | “VDDCORE CDC” |
| power7\_average | Measured average power for VDDCORE CDC (microWatts) |
| power7\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power7\_label | “VDDCORE CDC” |
| in7\_input | Measured voltage for MICVDD (milliVolts) |
| in7\_label | “MICVDD” |
| curr8\_input | Measured current for MICVDD (milliAmps) |
| curr8\_label | “MICVDD” |
| power8\_average | Measured average power for MICVDD (microWatts) |
| power8\_average\_interval | Power averaging time input valid from 1 to 1708mS |
| power8\_label | “MICVDD” |

Note:
:   It is not possible to measure voltage on the SYSVDD rail.
