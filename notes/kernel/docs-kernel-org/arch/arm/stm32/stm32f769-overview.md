# STM32F769 Overview

> 출처(원문): https://docs.kernel.org/arch/arm/stm32/stm32f769-overview.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# STM32F769 Overview

## Introduction

The STM32F769 is a Cortex-M7 MCU aimed at various applications.
It features:

* Cortex-M7 core running up to @216MHz
* 2MB internal flash, 512KBytes internal RAM (+4KB of backup SRAM)
* FMC controller to connect SDRAM, NOR and NAND memories
* Dual mode QSPI
* SD/MMC/SDIO support\*2
* Ethernet controller
* USB OTFG FS & HS controllers
* I2C\*4, SPI\*6, CAN\*3 buses support
* Several 16 & 32 bits general purpose timers
* Serial Audio interface\*2
* LCD controller
* HDMI-CEC
* DSI
* SPDIFRX
* MDIO salave interface

## Resources

Datasheet and reference manual are publicly available on ST website ([STM32F769](http://www.st.com/content/st_com/en/products/microcontrollers/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x9/stm32f769ni.html)).

Authors:
:   Alexandre Torgue <[alexandre.torgue@st.com](mailto:alexandre.torgue%40st.com)>
