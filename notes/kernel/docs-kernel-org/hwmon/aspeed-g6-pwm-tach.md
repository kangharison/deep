# Kernel driver aspeed-g6-pwm-tach

> 출처(원문): https://docs.kernel.org/hwmon/aspeed-g6-pwm-tach.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver aspeed-g6-pwm-tach

Supported chips:
:   ASPEED AST2600

Authors:
:   <[billy\_tsai@aspeedtech.com](mailto:billy_tsai%40aspeedtech.com)>

## Description:

This driver implements support for ASPEED AST2600 Fan Tacho controller.
The controller supports up to 16 tachometer inputs.

The driver provides the following sensor accesses in sysfs:

|  |  |  |
| --- | --- | --- |
| fanX\_input | ro | provide current fan rotation value in RPM as reported by the fan to the device. |
| fanX\_div | rw | Fan divisor: Supported value are power of 4 (1, 4, 16 64, ... 4194304) The larger divisor, the less rpm accuracy and the less affected by fan signal glitch. |
