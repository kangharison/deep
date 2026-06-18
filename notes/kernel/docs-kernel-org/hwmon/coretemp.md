# Kernel driver coretemp

> 출처(원문): https://docs.kernel.org/hwmon/coretemp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver coretemp

Supported chips:
:   * All Intel Core family and Atom processors with Digital Thermal Sensor (DTS)

      Prefix: ‘coretemp’

      CPUID: family 0x6, models with X86\_FEATURE\_DTHERM, including:

      > + 0xe (Pentium M DC), 0xf (Core 2 DC 65nm),
      > + 0x16 (Core 2 SC 65nm), 0x17 (Penryn 45nm),
      > + 0x1a (Nehalem), 0x1c (Atom), 0x1e (Lynnfield),
      > + 0x26 (Tunnel Creek Atom), 0x27 (Medfield Atom),
      > + 0x36 (Cedar Trail Atom), 0x37 (Bay Trail Atom),
      > + 0x4a (Merrifield Atom), 0x4c (Cherry Trail Atom),
      > + 0x5a (Moorefield Atom), 0x5c (Apollo Lake Atom),
      > + 0x7a (Gemini Lake Atom),
      > + 0x96 (Elkhart Lake Atom), 0x9c (Jasper Lake Atom)

      Datasheet:

      > Intel 64 and IA-32 Architectures Software Developer’s Manual
      > Volume 3A: System Programming Guide
      >
      > <http://softwarecommunity.intel.com/Wiki/Mobility/720.htm>

Author: Rudolf Marek

## Description

This driver permits reading the DTS (Digital Temperature Sensor) embedded
inside Intel CPUs. This driver can read both the per-core and per-package
temperature using the appropriate sensors. The per-package sensor is
available on Sandy Bridge and all newer processors. The driver will show
the temperature of all cores inside a package under a single device
directory inside hwmon.

Temperature is measured in degrees Celsius and measurement resolution is
1 degree C. Valid temperatures are from 0 to TjMax degrees C, because
the actual value of temperature register is in fact a delta from TjMax.

Temperature known as TjMax is the maximum junction temperature of processor,
which depends on the CPU model. See table below. At this temperature, protection
mechanism will perform actions to forcibly cool down the processor. Alarm
may be raised, if the temperature grows enough (more than TjMax) to trigger
the Out-Of-Spec bit. Following table summarizes the exported sysfs files:

All Sysfs entries are named with their core\_id (represented here by ‘X’).

|  |  |
| --- | --- |
| tempX\_input | Core temperature (in millidegrees Celsius). |
| tempX\_max | All cooling devices should be turned on (on Core2). |
| tempX\_crit | Maximum junction temperature (in millidegrees Celsius). |
| tempX\_crit\_alarm | Set when Out-of-spec bit is set, never clears. Correct CPU operation is no longer guaranteed. |
| tempX\_label | Contains string “Core X”, where X is processor number. For Package temp, this will be “Physical id Y”, where Y is the package number. |

On modern CPUs (Nehalem and newer), TjMax is read from the
MSR\_IA32\_TEMPERATURE\_TARGET register. On older models without this MSR,
TjMax is determined using lookup tables or heuristics. If these don’t work
for your CPU, you can pass the correct TjMax value as a module parameter
(tjmax).

Appendix A. Known TjMax lists (TBD):
Some information comes from ark.intel.com

|  |  |  |
| --- | --- | --- |
| Process | Processor | TjMax(C) |
| 22nm | Core i5/i7 Processors i7 3920XM, 3820QM, 3720QM, 3667U, 3520M i5 3427U, 3360M/3320M i7 3770/3770K i5 3570/3570K, 3550, 3470/3450 i7 3770S i5 3570S/3550S, 3475S/3470S/3450S i7 3770T i5 3570T i5 3470T | 105 105 105 105 103 103 94 94 91 |
| 32nm | Core i3/i5/i7 Processors i7 2600 i7 660UM/640/620, 640LM/620, 620M, 610E i5 540UM/520/430, 540M/520/450/430 i3 330E, 370M/350/330 i3 330UM | 98 105 105 90 rPGA, 105 BGA 105 |
| 32nm | Core i7 Extreme Processors 980X | 100 |
| 32nm | Celeron Processors U3400 P4505/P4500 | 105 90 |
| 32nm | Atom Processors S1260/1220 S1240 Z2460 Z2760 D2700/2550/2500 N2850/2800/2650/2600 | 95 102 90 90 100 100 |
| 22nm | Atom Processors (Silvermont/Bay Trail) E3845/3827/3826/3825/3815/3805 Z3795/3775/3770/3740/3736/3735/3680 | 110 90 |
| 22nm | Atom Processors (Silvermont/Moorefield) Z3580/3570/3560/3530 | 90 |
| 14nm | Atom Processors (Airmont/Cherry Trail) x5-Z8550/Z8500/Z8350/Z8330/Z8300 x7-Z8750/Z8700 | 90 90 |
| 14nm | Atom Processors (Goldmont/Apollo Lake) x5-E3940/E3930 x7-E3950 | 105 105 |
| 14nm | Celeron/Pentium Processors (Goldmont/Apollo Lake) J3455/J3355 N3450/N3350 N4200 | 105 105 105 |
| 14nm | Celeron/Pentium Processors (Goldmont Plus/Gemini Lake) J4105/J4005 N4100/N4000 N5000 | 105 105 105 |
| 10nm | Atom Processors (Tremont/Elkhart Lake) x6000E | 105 |
| 10nm | Celeron/Pentium Processors (Tremont/Jasper Lake) N4500/N5100/N6000 series | 105 |
| 45nm | Xeon Processors 5400 Quad-Core X5492, X5482, X5472, X5470, X5460, X5450 E5472, E5462, E5450/40/30/20/10/05 L5408 L5430, L5420, L5410 | 85 85 95 70 |
| 45nm | Xeon Processors 5200 Dual-Core X5282, X5272, X5270, X5260 E5240 E5205, E5220 L5240 L5238, L5215 | 90 90 70, 90 70 95 |
| 45nm | Atom Processors D525/510/425/410 K525/510/425/410 Z670/650 Z560/550/540/530P/530/520PT/520/515/510PT/510P Z510/500 N570/550 N475/470/455/450 N280/270 330/230 E680/660/640/620 E680T/660T/640T/620T E665C/645C E665CT/645CT CE4170/4150/4110 CE4200 series CE5300 series | 100 100 90 90 90 100 100 90 125 90 110 90 110 110 unknown unknown |
| 45nm | Core2 Processors Solo ULV SU3500/3300 T9900/9800/9600/9550/9500/9400/9300/8300/8100 T6670/6500/6400 T6600 SU9600/9400/9300 SP9600/9400 SL9600/9400/9380/9300 P9700/9600/9500/8800/8700/8600/8400/7570 P7550/7450 | 100 105 105 90 105 105 105 105 90 |
| 45nm | Core2 Quad Processors Q9100/9000 | 100 |
| 45nm | Core2 Extreme Processors X9100/9000 QX9300 | 105 100 |
| 45nm | Core i3/i5/i7 Processors i7 940XM/920 i7 840QM/820/740/720 | 100 100 |
| 45nm | Celeron Processors SU2300 900 | 100 105 |
| 65nm | Core2 Duo Processors Solo U2200, U2100 U7700/7600/7500 T7800/7700/7600/7500/7400/7300/7250/7200/7100 T5870/5670/5600/5550/5500/5470/5450/5300/5270 T5250 T5800/5750/5200 L7700/7500/7400/7300/7200 | 100 100 100 100 100 85 100 |
| 65nm | Core2 Extreme Processors X7900/7800 | 100 |
| 65nm | Core Duo Processors U2500/2400 T2700/2600/2450/2400/2350/2300E/2300/2250/2050 L2500/2400/2300 | 100 100 100 |
| 65nm | Core Solo Processors U1500/1400/1300 T1400/1350/1300/1250 | 100 100 |
| 65nm | Xeon Processors 5000 Quad-Core X5000 E5000 L5000 L5318 | 90-95 80 70 95 |
| 65nm | Xeon Processors 5000 Dual-Core 5080, 5063, 5060, 5050, 5030 5160, 5150, 5148, 5140, 5130, 5120, 5110 L5138 | 80-90 80 100 |
| 65nm | Celeron Processors T1700/1600 560/550/540/530 | 100 100 |
