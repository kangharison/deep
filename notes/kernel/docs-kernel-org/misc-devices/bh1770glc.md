# Kernel driver bh1770glc

> 출처(원문): https://docs.kernel.org/misc-devices/bh1770glc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver bh1770glc

Supported chips:

* ROHM BH1770GLC
* OSRAM SFH7770

Data sheet:
Not freely available

Author:
Samu Onkalo <[samu.p.onkalo@nokia.com](mailto:samu.p.onkalo%40nokia.com)>

## Description

BH1770GLC and SFH7770 are combined ambient light and proximity sensors.
ALS and proximity parts operates on their own, but they shares common I2C
interface and interrupt logic. In principle they can run on their own,
but ALS side results are used to estimate reliability of the proximity sensor.

ALS produces 16 bit lux values. The chip contains interrupt logic to produce
low and high threshold interrupts.

Proximity part contains IR-led driver up to 3 IR leds. The chip measures
amount of reflected IR light and produces proximity result. Resolution is
8 bit. Driver supports only one channel. Driver uses ALS results to estimate
reliability of the proximity results. Thus ALS is always running while
proximity detection is needed.

Driver uses threshold interrupts to avoid need for polling the values.
Proximity low interrupt doesn’t exists in the chip. This is simulated
by using a delayed work. As long as there is proximity threshold above
interrupts the delayed work is pushed forward. So, when proximity level goes
below the threshold value, there is no interrupt and the delayed work will
finally run. This is handled as no proximity indication.

Chip state is controlled via runtime pm framework when enabled in config.

Calibscale factor is used to hide differences between the chips. By default
value set to neutral state meaning factor of 1.00. To get proper values,
calibrated source of light is needed as a reference. Calibscale factor is set
so that measurement produces about the expected lux value.

## SYSFS

chip\_id
:   RO - shows detected chip type and version

power\_state
:   RW - enable / disable chip

    Uses counting logic

    > * 1 enables the chip
    > * 0 disables the chip

lux0\_input
:   RO - measured lux value

    > sysfs\_notify called when threshold interrupt occurs

lux0\_sensor\_range
:   RO - lux0\_input max value

lux0\_rate
:   RW - measurement rate in Hz

lux0\_rate\_avail
:   RO - supported measurement rates

lux0\_thresh\_above\_value
:   RW - HI level threshold value

    > All results above the value
    > trigs an interrupt. 65535 (i.e. sensor\_range) disables the above
    > interrupt.

lux0\_thresh\_below\_value
:   RW - LO level threshold value

    > All results below the value
    > trigs an interrupt. 0 disables the below interrupt.

lux0\_calibscale
:   RW - calibration value

    > Set to neutral value by default.
    > Output results are multiplied with calibscale / calibscale\_default
    > value.

lux0\_calibscale\_default
:   RO - neutral calibration value

prox0\_raw
:   RO - measured proximity value

    > sysfs\_notify called when threshold interrupt occurs

prox0\_sensor\_range
:   RO - prox0\_raw max value

prox0\_raw\_en
:   RW - enable / disable proximity

    > Uses counting logic
    >
    > * 1 enables the proximity
    > * 0 disables the proximity

prox0\_thresh\_above\_count
:   RW - number of proximity interrupts needed before triggering the event

prox0\_rate\_above
:   RW - Measurement rate (in Hz) when the level is above threshold
    i.e. when proximity on has been reported.

prox0\_rate\_below
:   RW - Measurement rate (in Hz) when the level is below threshold
    i.e. when proximity off has been reported.

prox0\_rate\_avail
:   RO - Supported proximity measurement rates in Hz

prox0\_thresh\_above0\_value
:   RW - threshold level which trigs proximity events.

    > Filtered by persistence filter (prox0\_thresh\_above\_count)

prox0\_thresh\_above1\_value
:   RW - threshold level which trigs event immediately
