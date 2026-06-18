# STM32 ARM Linux Overview

> 출처(원문): https://docs.kernel.org/arch/arm/stm32/overview.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# STM32 ARM Linux Overview

## Introduction

The STMicroelectronics STM32 family of Cortex-A microprocessors (MPUs) and
Cortex-M microcontrollers (MCUs) are supported by the ‘STM32’ platform of
ARM Linux.

## Configuration

For MCUs, use the provided default configuration:
:   make stm32\_defconfig

For MPUs, use multi\_v7 configuration:
:   make multi\_v7\_defconfig

## Layout

All the files for multiple machine families are located in the platform code
contained in arch/arm/mach-stm32

There is a generic board board-dt.c in the mach folder which support
Flattened Device Tree, which means, it works with any compatible board with
Device Trees.

Authors:

* Maxime Coquelin <[mcoquelin.stm32@gmail.com](mailto:mcoquelin.stm32%40gmail.com)>
* Ludovic Barre <[ludovic.barre@st.com](mailto:ludovic.barre%40st.com)>
* Gerald Baeza <[gerald.baeza@st.com](mailto:gerald.baeza%40st.com)>
