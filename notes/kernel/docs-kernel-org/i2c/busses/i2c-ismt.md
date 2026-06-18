# Kernel driver i2c-ismt

> 출처(원문): https://docs.kernel.org/i2c/busses/i2c-ismt.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver i2c-ismt

Supported adapters:
:   * Intel S12xx series SOCs

Authors:
:   Bill Brown <[bill.e.brown@intel.com](mailto:bill.e.brown%40intel.com)>

## Module Parameters

* bus\_speed (unsigned int)

Allows changing of the bus speed. Normally, the bus speed is set by the BIOS
and never needs to be changed. However, some SMBus analyzers are too slow for
monitoring the bus during debug, thus the need for this module parameter.
Specify the bus speed in kHz.

Available bus frequency settings:

> |  |  |
> | --- | --- |
> | 0 | no change |
> | 80 | kHz |
> | 100 | kHz |
> | 400 | kHz |
> | 1000 | kHz |

## Description

The S12xx series of SOCs have a pair of integrated SMBus 2.0 controllers
targeted primarily at the microserver and storage markets.

The S12xx series contain a pair of PCI functions. An output of lspci will show
something similar to the following:

```
00:13.0 System peripheral: Intel Corporation Centerton SMBus 2.0 Controller 0
00:13.1 System peripheral: Intel Corporation Centerton SMBus 2.0 Controller 1
```
