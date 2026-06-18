# Kernel driver i2c-amd-mp2

> 출처(원문): https://docs.kernel.org/i2c/busses/i2c-amd-mp2.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver i2c-amd-mp2

Supported adapters:
:   * AMD MP2 PCIe interface

Datasheet: not publicly available.

Authors:
:   * Shyam Sundar S K <[Shyam-sundar.S-k@amd.com](mailto:Shyam-sundar.S-k%40amd.com)>
    * Nehal Shah <[nehal-bakulchandra.shah@amd.com](mailto:nehal-bakulchandra.shah%40amd.com)>
    * Elie Morisse <[syniurge@gmail.com](mailto:syniurge%40gmail.com)>

## Description

The MP2 is an ARM processor programmed as an I2C controller and communicating
with the x86 host through PCI.

If you see something like this:

```
03:00.7 MP2 I2C controller: Advanced Micro Devices, Inc. [AMD] Device 15e6
```

in your `lspci -v`, then this driver is for your device.
