# Kernel driver stef48h28

> 출처(원문): https://docs.kernel.org/hwmon/stef48h28.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver stef48h28

Supported chips:

> * Analog Devices STEF48H28
>
>   Prefix: ‘stef48h28’
>
>   Addresses scanned: -
>
>   Datasheet: <https://www.st.com/resource/en/data_brief/stef48h28.pdf>

Author:

> * Charles Hsu <[hsu.yungteng@gmail.com](mailto:hsu.yungteng%40gmail.com)>

## Description

The STEF48H28 is a 30 A integrated e-fuse for 9-80 V DC power rails.
It provides inrush control, undervoltage/overvoltage lockout and
overcurrent protection using an adaptive (I x t) scheme that permits
short high-current pulses typical of CPU/GPU loads.

The device offers an analog current-monitor output and an on-chip
temperature-monitor signal for system supervision. Startup behavior is
programmable through insertion-delay and soft-start settings.

Additional features include power-good indication, self-diagnostics,
thermal shutdown and a PMBus interface for telemetry and status
reporting.

## Platform data support

The driver supports standard PMBus driver platform data.

## Sysfs entries

|  |  |
| --- | --- |
| in1\_label | “vin”. |
| in1\_input | Measured voltage. From READ\_VIN register. |
| in1\_min | Minimum Voltage. From VIN\_UV\_WARN\_LIMIT register. |
| in1\_max | Maximum voltage. From VIN\_OV\_WARN\_LIMIT register. |
| in2\_label | “vout1”. |
| in2\_input | Measured voltage. From READ\_VOUT register. |
| in2\_min | Minimum Voltage. From VOUT\_UV\_WARN\_LIMIT register. |
| in2\_max | Maximum voltage. From VOUT\_OV\_WARN\_LIMIT register. |
| curr1\_label “iin”. | curr1\_input Measured current. From READ\_IIN register. |
| curr2\_label “iout1”. | curr2\_input Measured current. From READ\_IOUT register. |
| power1\_label | “pin” |
| power1\_input | Measured input power. From READ\_PIN register. |
| power2\_label | “pout1” |
| power2\_input | Measured output power. From READ\_POUT register. |
| temp1\_input | Measured temperature. From READ\_TEMPERATURE\_1 register. |
| temp1\_max | Maximum temperature. From OT\_WARN\_LIMIT register. |
| temp1\_crit | Critical high temperature. From OT\_FAULT\_LIMIT register. |
| temp2\_input | Measured temperature. From READ\_TEMPERATURE\_2 register. |
