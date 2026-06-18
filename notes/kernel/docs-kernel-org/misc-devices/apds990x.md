# Kernel driver apds990x

> 출처(원문): https://docs.kernel.org/misc-devices/apds990x.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver apds990x

Supported chips:
Avago APDS990X

Data sheet:
Not freely available

Author:
Samu Onkalo <[samu.p.onkalo@nokia.com](mailto:samu.p.onkalo%40nokia.com)>

## Description

APDS990x is a combined ambient light and proximity sensor. ALS and proximity
functionality are highly connected. ALS measurement path must be running
while the proximity functionality is enabled.

ALS produces raw measurement values for two channels: Clear channel
(infrared + visible light) and IR only. However, threshold comparisons happen
using clear channel only. Lux value and the threshold level on the HW
might vary quite much depending the spectrum of the light source.

Driver makes necessary conversions to both directions so that user handles
only lux values. Lux value is calculated using information from the both
channels. HW threshold level is calculated from the given lux value to match
with current type of the lightning. Sometimes inaccuracy of the estimations
lead to false interrupt, but that doesn’t harm.

ALS contains 4 different gain steps. Driver automatically
selects suitable gain step. After each measurement, reliability of the results
is estimated and new measurement is triggered if necessary.

Platform data can provide tuned values to the conversion formulas if
values are known. Otherwise plain sensor default values are used.

Proximity side is little bit simpler. There is no need for complex conversions.
It produces directly usable values.

Driver controls chip operational state using pm\_runtime framework.
Voltage regulators are controlled based on chip operational state.

## SYSFS

chip\_id
:   RO - shows detected chip type and version

power\_state
:   RW - enable / disable chip. Uses counting logic

    > 1 enables the chip
    > 0 disables the chip

lux0\_input
:   RO - measured lux value

    > sysfs\_notify called when threshold interrupt occurs

lux0\_sensor\_range
:   RO - lux0\_input max value.

    > Actually never reaches since sensor tends
    > to saturate much before that. Real max value varies depending
    > on the light spectrum etc.

lux0\_rate
:   RW - measurement rate in Hz

lux0\_rate\_avail
:   RO - supported measurement rates

lux0\_calibscale
:   RW - calibration value.

    > Set to neutral value by default.
    > Output results are multiplied with calibscale / calibscale\_default
    > value.

lux0\_calibscale\_default
:   RO - neutral calibration value

lux0\_thresh\_above\_value
:   RW - HI level threshold value.

    > All results above the value
    > trigs an interrupt. 65535 (i.e. sensor\_range) disables the above
    > interrupt.

lux0\_thresh\_below\_value
:   RW - LO level threshold value.

    > All results below the value
    > trigs an interrupt. 0 disables the below interrupt.

prox0\_raw
:   RO - measured proximity value

    > sysfs\_notify called when threshold interrupt occurs

prox0\_sensor\_range
:   RO - prox0\_raw max value (1023)

prox0\_raw\_en
:   RW - enable / disable proximity - uses counting logic

    > * 1 enables the proximity
    > * 0 disables the proximity

prox0\_reporting\_mode
:   RW - trigger / periodic.

    > In “trigger” mode the driver tells two possible
    > values: 0 or prox0\_sensor\_range value. 0 means no proximity,
    > 1023 means proximity. This causes minimal number of interrupts.
    > In “periodic” mode the driver reports all values above
    > prox0\_thresh\_above. This causes more interrupts, but it can give
    > \_rough\_ estimate about the distance.

prox0\_reporting\_mode\_avail
:   RO - accepted values to prox0\_reporting\_mode (trigger, periodic)

prox0\_thresh\_above\_value
:   RW - threshold level which trigs proximity events.
