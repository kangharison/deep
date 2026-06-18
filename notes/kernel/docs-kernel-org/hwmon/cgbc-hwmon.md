# Kernel driver cgbc-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/cgbc-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver cgbc-hwmon

Supported chips:

> * Congatec Board Controller.
>
>   Prefix: ‘cgbc-hwmon’

Author: Thomas Richard <[thomas.richard@bootlin.com](mailto:thomas.richard%40bootlin.com)>

## Description

This driver enables monitoring support for the Congatec Board Controller.
This controller is embedded on the x86 SoMs of Congatec.

## Sysfs entries

The following sysfs entries list contains all sensors defined in the Board
Controller. The available sensors in sysfs depend on the SoM and the
system.

| Name | Description |
| --- | --- |
| temp1\_input | CPU temperature |
| temp2\_input | Box temperature |
| temp3\_input | Ambient temperature |
| temp4\_input | Board temperature |
| temp5\_input | Carrier temperature |
| temp6\_input | Chipset temperature |
| temp7\_input | Video temperature |
| temp8\_input | Other temperature |
| temp9\_input | TOPDIM temperature |
| temp10\_input | BOTTOMDIM temperature |
| in0\_input | CPU voltage |
| in1\_input | DC Runtime voltage |
| in2\_input | DC Standby voltage |
| in3\_input | CMOS Battery voltage |
| in4\_input | Battery voltage |
| in5\_input | AC voltage |
| in6\_input | Other voltage |
| in7\_input | 5V voltage |
| in8\_input | 5V Standby voltage |
| in9\_input | 3V3 voltage |
| in10\_input | 3V3 Standby voltage |
| in11\_input | VCore A voltage |
| in12\_input | VCore B voltage |
| in13\_input | 12V voltage |
| curr1\_input | DC current |
| curr2\_input | 5V current |
| curr3\_input | 12V current |
| fan1\_input | CPU fan |
| fan2\_input | Box fan |
| fan3\_input | Ambient fan |
| fan4\_input | Chiptset fan |
| fan5\_input | Video fan |
| fan6\_input | Other fan |
