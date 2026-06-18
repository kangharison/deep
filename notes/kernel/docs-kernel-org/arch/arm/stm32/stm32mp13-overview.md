# STM32MP13 Overview

> 출처(원문): https://docs.kernel.org/arch/arm/stm32/stm32mp13-overview.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# STM32MP13 Overview

## Introduction

The STM32MP131/STM32MP133/STM32MP135 are Cortex-A MPU aimed at various applications.
They feature:

* One Cortex-A7 application core
* Standard memories interface support
* Standard connectivity, widely inherited from the STM32 MCU family
* Comprehensive security support

More details:

* Cortex-A7 core running up to @900MHz
* FMC controller to connect SDRAM, NOR and NAND memories
* QSPI
* SD/MMC/SDIO support
* 2\*Ethernet controller
* CAN
* ADC/DAC
* USB EHCI/OHCI controllers
* USB OTG
* I2C, SPI, CAN buses support
* Several general purpose timers
* Serial Audio interface
* LCD controller
* DCMIPP
* SPDIFRX
* DFSDM

Authors:

* Alexandre Torgue <[alexandre.torgue@foss.st.com](mailto:alexandre.torgue%40foss.st.com)>
