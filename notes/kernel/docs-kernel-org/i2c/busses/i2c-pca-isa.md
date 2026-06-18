# Kernel driver i2c-pca-isa

> 출처(원문): https://docs.kernel.org/i2c/busses/i2c-pca-isa.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver i2c-pca-isa

Supported adapters:

This driver supports ISA boards using the Philips PCA 9564
Parallel bus to I2C bus controller

Author: Ian Campbell <[icampbell@arcom.com](mailto:icampbell%40arcom.com)>, Arcom Control Systems

## Module Parameters

* base int
  :   I/O base address
* irq int
  :   IRQ interrupt
* clock int
  :   Clock rate as described in table 1 of PCA9564 datasheet

## Description

This driver supports ISA boards using the Philips PCA 9564
Parallel bus to I2C bus controller
