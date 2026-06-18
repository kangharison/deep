# STM32MP151 Overview

> 출처(원문): https://docs.kernel.org/arch/arm/stm32/stm32mp151-overview.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# STM32MP151 Overview

## Introduction

The STM32MP151 is a Cortex-A MPU aimed at various applications.
It features:

* Single Cortex-A7 application core
* Standard memories interface support
* Standard connectivity, widely inherited from the STM32 MCU family
* Comprehensive security support

More details:

* Cortex-A7 core running up to @800MHz
* FMC controller to connect SDRAM, NOR and NAND memories
* QSPI
* SD/MMC/SDIO support
* Ethernet controller
* ADC/DAC
* USB EHCI/OHCI controllers
* USB OTG
* I2C, SPI buses support
* Several general purpose timers
* Serial Audio interface
* LCD-TFT controller
* DCMIPP
* SPDIFRX
* DFSDM

Authors:

* Roan van Dijk <[roan@protonic.nl](mailto:roan%40protonic.nl)>
