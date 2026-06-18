# Kernel driver pli1209bc

> 출처(원문): https://docs.kernel.org/hwmon/pli1209bc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver pli1209bc

Supported chips:

> * Digital Supervisor PLI1209BC
>
>   Prefix: ‘pli1209bc’
>
>   Addresses scanned: 0x50 - 0x5F
>
>   Datasheet: <https://www.vicorpower.com/documents/datasheets/ds-PLI1209BCxyzz-VICOR.pdf>

Authors:
:   * Marcello Sylvester Bauer <[sylv@sylv.io](mailto:sylv%40sylv.io)>

## Description

The Vicor PLI1209BC is an isolated digital power system supervisor that provides
a communication interface between a host processor and one Bus Converter Module
(BCM). The PLI communicates with a system controller via a PMBus compatible
interface over an isolated UART interface. Through the PLI, the host processor
can configure, set protection limits, and monitor the BCM.

## Sysfs entries

|  |  |
| --- | --- |
| in1\_label | “vin2” |
| in1\_input | Input voltage. |
| in1\_rated\_min | Minimum rated input voltage. |
| in1\_rated\_max | Maximum rated input voltage. |
| in1\_max | Maximum input voltage. |
| in1\_max\_alarm | Input voltage high alarm. |
| in1\_crit | Critical input voltage. |
| in1\_crit\_alarm | Input voltage critical alarm. |
| in2\_label | “vout2” |
| in2\_input | Output voltage. |
| in2\_rated\_min | Minimum rated output voltage. |
| in2\_rated\_max | Maximum rated output voltage. |
| in2\_alarm | Output voltage alarm |
| curr1\_label | “iin2” |
| curr1\_input | Input current. |
| curr1\_max | Maximum input current. |
| curr1\_max\_alarm | Maximum input current high alarm. |
| curr1\_crit | Critical input current. |
| curr1\_crit\_alarm | Input current critical alarm. |
| curr2\_label | “iout2” |
| curr2\_input | Output current. |
| curr2\_crit | Critical output current. |
| curr2\_crit\_alarm | Output current critical alarm. |
| curr2\_max | Maximum output current. |
| curr2\_max\_alarm | Output current high alarm. |
| power1\_label | “pin2” |
| power1\_input | Input power. |
| power1\_alarm | Input power alarm. |
| power2\_label | “pout2” |
| power2\_input | Output power. |
| power2\_rated\_max | Maximum rated output power. |
| temp1\_input | Die temperature. |
| temp1\_alarm | Die temperature alarm. |
| temp1\_max | Maximum die temperature. |
| temp1\_max\_alarm | Die temperature high alarm. |
| temp1\_crit | Critical die temperature. |
| temp1\_crit\_alarm | Die temperature critical alarm. |
