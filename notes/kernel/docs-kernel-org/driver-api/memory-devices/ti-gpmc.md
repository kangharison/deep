# GPMC (General Purpose Memory Controller)

> 출처(원문): https://docs.kernel.org/driver-api/memory-devices/ti-gpmc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPMC (General Purpose Memory Controller)

GPMC is an unified memory controller dedicated to interfacing external
memory devices like

> * Asynchronous SRAM like memories and application specific integrated
>   circuit devices.
> * Asynchronous, synchronous, and page mode burst NOR flash devices
>   NAND flash
> * Pseudo-SRAM devices

GPMC is found on Texas Instruments SoC’s (OMAP based)
IP details: <https://www.ti.com/lit/pdf/spruh73> section 7.1

## GPMC generic timing calculation:

GPMC has certain timings that has to be programmed for proper
functioning of the peripheral, while peripheral has another set of
timings. To have peripheral work with gpmc, peripheral timings has to
be translated to the form gpmc can understand. The way it has to be
translated depends on the connected peripheral. Also there is a
dependency for certain gpmc timings on gpmc clock frequency. Hence a
generic timing routine was developed to achieve above requirements.

Generic routine provides a generic method to calculate gpmc timings
from gpmc peripheral timings. `struct gpmc_device_timings` fields has to
be updated with timings from the datasheet of the peripheral that is
connected to gpmc. A few of the peripheral timings can be fed either
in time or in cycles, provision to handle this scenario has been
provided (refer `struct gpmc_device_timings` definition). It may so
happen that timing as specified by peripheral datasheet is not present
in timing structure, in this scenario, try to correlate peripheral
timing to the one available. If that doesn’t work, try to add a new
field as required by peripheral, educate generic timing routine to
handle it, make sure that it does not break any of the existing.
Then there may be cases where peripheral datasheet doesn’t mention
certain fields of `struct gpmc_device_timings`, zero those entries.

Generic timing routine has been verified to work properly on
multiple onenand’s and tusb6010 peripherals.

A word of caution: generic timing routine has been developed based
on understanding of gpmc timings, peripheral timings, available
custom timing routines, a kind of reverse engineering without
most of the datasheets & hardware (to be exact none of those supported
in mainline having custom timing routine) and by simulation.

gpmc timing dependency on peripheral timings:

[<gpmc\_timing>: <peripheral timing1>, <peripheral timing2> ...]

1. common

cs\_on:
:   t\_ceasu

adv\_on:
:   t\_avdasu, t\_ceavd

2. sync common

sync\_clk:
:   clk

page\_burst\_access:
:   t\_bacc

clk\_activation:
:   t\_ces, t\_avds

3. read async muxed

adv\_rd\_off:
:   t\_avdp\_r

oe\_on:
:   t\_oeasu, t\_aavdh

access:
:   t\_iaa, t\_oe, t\_ce, t\_aa

rd\_cycle:
:   t\_rd\_cycle, t\_cez\_r, t\_oez

4. read async non-muxed

adv\_rd\_off:
:   t\_avdp\_r

oe\_on:
:   t\_oeasu

access:
:   t\_iaa, t\_oe, t\_ce, t\_aa

rd\_cycle:
:   t\_rd\_cycle, t\_cez\_r, t\_oez

5. read sync muxed

adv\_rd\_off:
:   t\_avdp\_r, t\_avdh

oe\_on:
:   t\_oeasu, t\_ach, cyc\_aavdh\_oe

access:
:   t\_iaa, cyc\_iaa, cyc\_oe

rd\_cycle:
:   t\_cez\_r, t\_oez, t\_ce\_rdyz

6. read sync non-muxed

adv\_rd\_off:
:   t\_avdp\_r

oe\_on:
:   t\_oeasu

access:
:   t\_iaa, cyc\_iaa, cyc\_oe

rd\_cycle:
:   t\_cez\_r, t\_oez, t\_ce\_rdyz

7. write async muxed

adv\_wr\_off:
:   t\_avdp\_w

we\_on, wr\_data\_mux\_bus:
:   t\_weasu, t\_aavdh, cyc\_aavhd\_we

we\_off:
:   t\_wpl

cs\_wr\_off:
:   t\_wph

wr\_cycle:
:   t\_cez\_w, t\_wr\_cycle

8. write async non-muxed

adv\_wr\_off:
:   t\_avdp\_w

we\_on, wr\_data\_mux\_bus:
:   t\_weasu

we\_off:
:   t\_wpl

cs\_wr\_off:
:   t\_wph

wr\_cycle:
:   t\_cez\_w, t\_wr\_cycle

9. write sync muxed

adv\_wr\_off:
:   t\_avdp\_w, t\_avdh

we\_on, wr\_data\_mux\_bus:
:   t\_weasu, t\_rdyo, t\_aavdh, cyc\_aavhd\_we

we\_off:
:   t\_wpl, cyc\_wpl

cs\_wr\_off:
:   t\_wph

wr\_cycle:
:   t\_cez\_w, t\_ce\_rdyz

10. write sync non-muxed

adv\_wr\_off:
:   t\_avdp\_w

we\_on, wr\_data\_mux\_bus:
:   t\_weasu, t\_rdyo

we\_off:
:   t\_wpl, cyc\_wpl

cs\_wr\_off:
:   t\_wph

wr\_cycle:
:   t\_cez\_w, t\_ce\_rdyz

Note:
:   Many of gpmc timings are dependent on other gpmc timings (a few
    gpmc timings purely dependent on other gpmc timings, a reason that
    some of the gpmc timings are missing above), and it will result in
    indirect dependency of peripheral timings to gpmc timings other than
    mentioned above, refer timing routine for more details. To know what
    these peripheral timings correspond to, please see explanations in
    `struct gpmc_device_timings` definition. And for gpmc timings refer
    IP details (link above).
