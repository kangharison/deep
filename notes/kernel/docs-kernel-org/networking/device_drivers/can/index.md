# Controller Area Network (CAN) Device Drivers

> 출처(원문): https://docs.kernel.org/networking/device_drivers/can/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Controller Area Network (CAN) Device Drivers

Device drivers for CAN devices.

Contents:

* [can327: ELM327 driver for Linux SocketCAN](can327.html)
  + [Authors](can327.html#authors)
  + [Motivation](can327.html#motivation)
  + [Introduction](can327.html#introduction)
  + [Data sheet](can327.html#data-sheet)
  + [How to attach the line discipline](can327.html#how-to-attach-the-line-discipline)
  + [How to check the controller version](can327.html#how-to-check-the-controller-version)
  + [Communication example](can327.html#communication-example)
  + [Known limitations of the controller](can327.html#known-limitations-of-the-controller)
  + [Known limitations of the driver](can327.html#known-limitations-of-the-driver)
  + [Rationale behind the chosen configuration](can327.html#rationale-behind-the-chosen-configuration)
  + [A note on CAN bus termination](can327.html#a-note-on-can-bus-termination)
* [CTU CAN FD Driver](ctu/ctucanfd-driver.html)
  + [About CTU CAN FD IP Core](ctu/ctucanfd-driver.html#about-ctu-can-fd-ip-core)
  + [About SocketCAN](ctu/ctucanfd-driver.html#about-socketcan)
  + [Integrating the core to Xilinx Zynq](ctu/ctucanfd-driver.html#integrating-the-core-to-xilinx-zynq)
  + [CTU CAN FD Driver design](ctu/ctucanfd-driver.html#ctu-can-fd-driver-design)
  + [CTU CAN FD Driver Sources Reference](ctu/ctucanfd-driver.html#ctu-can-fd-driver-sources-reference)
  + [CTU CAN FD IP Core and Driver Development Acknowledgment](ctu/ctucanfd-driver.html#ctu-can-fd-ip-core-and-driver-development-acknowledgment)
  + [Notes](ctu/ctucanfd-driver.html#notes)
* [Flexcan CAN Controller driver](freescale/flexcan.html)
  + [On/off RTR frames reception](freescale/flexcan.html#on-off-rtr-frames-reception)
