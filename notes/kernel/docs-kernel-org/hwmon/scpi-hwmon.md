# Kernel driver scpi-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/scpi-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver scpi-hwmon

Supported chips:

> * Chips based on ARM System Control Processor Interface
>
>   Addresses scanned: -
>
>   Datasheet: <http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0922b/index.html>

Author: Punit Agrawal <[punit.agrawal@arm.com](mailto:punit.agrawal%40arm.com)>

## Description

This driver supports hardware monitoring for SoC’s based on the ARM
System Control Processor (SCP) implementing the System Control
Processor Interface (SCPI). The following sensor types are supported
by the SCP:

> * temperature
> * voltage
> * current
> * power

The SCP interface provides an API to query the available sensors and
their values which are then exported to userspace by this driver.

## Usage Notes

The driver relies on device tree node to indicate the presence of SCPI
support in the kernel. See
Documentation/devicetree/bindings/firmware/arm,scpi.yaml for details of the
devicetree node.
