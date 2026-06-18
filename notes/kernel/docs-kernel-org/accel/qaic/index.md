# accel/qaic Qualcomm Cloud AI driver

> 출처(원문): https://docs.kernel.org/accel/qaic/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# accel/qaic Qualcomm Cloud AI driver

The accel/qaic driver supports the Qualcomm Cloud AI machine learning
accelerator cards.

* [QAIC driver](qaic.html)
  + [Interrupts](qaic.html#interrupts)
    - [IRQ Storm Mitigation](qaic.html#irq-storm-mitigation)
    - [Single MSI Mode](qaic.html#single-msi-mode)
  + [Neural Network Control (NNC) Protocol](qaic.html#neural-network-control-nnc-protocol)
  + [uAPI](qaic.html#uapi)
  + [Userspace Client Isolation](qaic.html#userspace-client-isolation)
  + [Module parameters](qaic.html#module-parameters)
* [Qualcomm Cloud AI 80 (AIC080)](aic080.html)
  + [Overview](aic080.html#overview)
* [Qualcomm Cloud AI 100 (AIC100)](aic100.html)
  + [Overview](aic100.html#overview)
  + [Hardware Description](aic100.html#hardware-description)
    - [MHI](aic100.html#mhi)
    - [QSM](aic100.html#qsm)
    - [NSP](aic100.html#nsp)
    - [DMA Bridge](aic100.html#dma-bridge)
    - [DDR](aic100.html#ddr)
  + [High-level Use Flow](aic100.html#high-level-use-flow)
  + [Boot Flow](aic100.html#boot-flow)
  + [Userspace components](aic100.html#userspace-components)
    - [Compiler](aic100.html#compiler)
    - [Usermode Driver (UMD)](aic100.html#usermode-driver-umd)
    - [Sahara loader](aic100.html#sahara-loader)
  + [MHI Channels](aic100.html#mhi-channels)
  + [DMA Bridge](aic100.html#id1)
    - [Overview](aic100.html#id2)
    - [Request FIFO](aic100.html#request-fifo)
    - [Response FIFO](aic100.html#response-fifo)
  + [Neural Network Control (NNC) Protocol](aic100.html#neural-network-control-nnc-protocol)
    - [Transaction descriptions](aic100.html#transaction-descriptions)
  + [Subsystem Restart (SSR)](aic100.html#subsystem-restart-ssr)
  + [Reliability, Accessibility, Serviceability (RAS)](aic100.html#reliability-accessibility-serviceability-ras)
  + [Telemetry](aic100.html#telemetry)
