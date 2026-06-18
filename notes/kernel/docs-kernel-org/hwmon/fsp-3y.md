# Kernel driver fsp3y

> 출처(원문): https://docs.kernel.org/hwmon/fsp-3y.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver fsp3y

Supported devices:
:   * 3Y POWER YH-5151E
    * 3Y POWER YM-2151E

Author: Václav Kubernát <[kubernat@cesnet.cz](mailto:kubernat%40cesnet.cz)>

## Description

This driver implements limited support for two 3Y POWER devices.

## Sysfs entries

> * in1\_input input voltage
> * in2\_input 12V output voltage
> * in3\_input 5V output voltage
> * curr1\_input input current
> * curr2\_input 12V output current
> * curr3\_input 5V output current
> * fan1\_input fan rpm
> * temp1\_input temperature 1
> * temp2\_input temperature 2
> * temp3\_input temperature 3
> * power1\_input input power
> * power2\_input output power
