# Kernel driver aspeed-pwm-tacho

> 출처(원문): https://docs.kernel.org/hwmon/aspeed-pwm-tacho.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver aspeed-pwm-tacho

Supported chips:
:   ASPEED AST2400/2500

Authors:
:   <[jaghu@google.com](mailto:jaghu%40google.com)>

## Description:

This driver implements support for ASPEED AST2400/2500 PWM and Fan Tacho
controller. The PWM controller supports up to 8 PWM outputs. The Fan tacho
controller supports up to 16 tachometer inputs.

The driver provides the following sensor accesses in sysfs:

|  |  |  |
| --- | --- | --- |
| fanX\_input | ro | provide current fan rotation value in RPM as reported by the fan to the device. |
| pwmX | rw | get or set PWM fan control value. This is an integer value between 0(off) and 255(full speed). |
