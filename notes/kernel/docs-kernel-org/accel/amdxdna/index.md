# accel/amdxdna NPU driver

> 출처(원문): https://docs.kernel.org/accel/amdxdna/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# accel/amdxdna NPU driver

The accel/amdxdna driver supports the AMD NPU (Neural Processing Unit).

* [AMD NPU](amdnpu.html)
  + [Overview](amdnpu.html#overview)
  + [Hardware Description](amdnpu.html#hardware-description)
    - [AMD XDNA Array](amdnpu.html#amd-xdna-array)
    - [Shared L2 Memory](amdnpu.html#shared-l2-memory)
    - [Microcontroller](amdnpu.html#microcontroller)
    - [Mailboxes](amdnpu.html#mailboxes)
    - [PCIe EP](amdnpu.html#pcie-ep)
    - [Process Isolation Hardware](amdnpu.html#process-isolation-hardware)
  + [Mixed Spatial and Temporal Scheduling](amdnpu.html#mixed-spatial-and-temporal-scheduling)
    - [Resource Solver](amdnpu.html#resource-solver)
  + [Application Binaries](amdnpu.html#application-binaries)
  + [Special Host Buffers](amdnpu.html#special-host-buffers)
    - [Per-context Instruction Buffer](amdnpu.html#per-context-instruction-buffer)
    - [Global Privileged Buffer](amdnpu.html#global-privileged-buffer)
  + [High-level Use Flow](amdnpu.html#high-level-use-flow)
  + [Boot Flow](amdnpu.html#boot-flow)
  + [Userspace components](amdnpu.html#userspace-components)
    - [Compiler](amdnpu.html#compiler)
    - [Usermode Driver (UMD)](amdnpu.html#usermode-driver-umd)
  + [DMA Operation](amdnpu.html#dma-operation)
  + [Error Handling](amdnpu.html#error-handling)
  + [Telemetry](amdnpu.html#telemetry)
  + [References](amdnpu.html#references)
