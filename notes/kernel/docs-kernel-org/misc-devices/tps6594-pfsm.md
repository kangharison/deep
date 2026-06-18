# Texas Instruments TPS6594 PFSM driver

> 출처(원문): https://docs.kernel.org/misc-devices/tps6594-pfsm.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Texas Instruments TPS6594 PFSM driver

Author: Julien Panis ([jpanis@baylibre.com](mailto:jpanis%40baylibre.com))

## Overview

Strictly speaking, PFSM (Pre-configurable Finite State Machine) is not
hardware. It is a piece of code.

The TPS6594 PMIC (Power Management IC) integrates a state machine which
manages operational modes. Depending on the current operational mode,
some voltage domains remain energized while others can be off.

The PFSM driver can be used to trigger transitions between configured
states. It also provides R/W access to the device registers.

### Supported chips

* tps6594-q1
* tps6593-q1
* lp8764-q1

## Driver location

drivers/misc/tps6594-pfsm.c

## Driver type definitions

include/uapi/linux/tps6594\_pfsm.h

## Driver IOCTLs

`PMIC_GOTO_STANDBY`
All device resources are powered down. The processor is off, and
no voltage domains are energized.

`PMIC_GOTO_LP_STANDBY`
The digital and analog functions of the PMIC, which are not
required to be always-on, are turned off (low-power).

`PMIC_UPDATE_PGM`
Triggers a firmware update.

`PMIC_SET_ACTIVE_STATE`
One of the operational modes.
The PMICs are fully functional and supply power to all PDN loads.
All voltage domains are energized in both MCU and Main processor
sections.

`PMIC_SET_MCU_ONLY_STATE`
One of the operational modes.
Only the power resources assigned to the MCU Safety Island are on.

`PMIC_SET_RETENTION_STATE`
One of the operational modes.
Depending on the triggers set, some DDR/GPIO voltage domains can
remain energized, while all other domains are off to minimize
total system power.

## Driver usage

See available PFSMs:

```
# ls /dev/pfsm*
```

Dump the registers of pages 0 and 1:

```
# hexdump -C /dev/pfsm-0-0x48
```

See PFSM events:

```
# cat /proc/interrupts
```

### Userspace code example

samples/pfsm/pfsm-wakeup.c
