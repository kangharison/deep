# Kernel driver npcm750-pwm-fan

> 출처(원문): https://docs.kernel.org/hwmon/npcm750-pwm-fan.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver npcm750-pwm-fan

Supported chips:

> NUVOTON NPCM750/730/715/705

Authors:

> <[tomer.maimon@nuvoton.com](mailto:tomer.maimon%40nuvoton.com)>

## Description:

This driver implements support for NUVOTON NPCM7XX PWM and Fan Tacho
controller. The PWM controller supports up to 8 PWM outputs. The Fan tacho
controller supports up to 16 tachometer inputs.

The driver provides the following sensor accesses in sysfs:

|  |  |  |
| --- | --- | --- |
| fanX\_input | ro | provide current fan rotation value in RPM as reported by the fan to the device. |
| pwmX | rw | get or set PWM fan control value. This is an integer value between 0(off) and 255(full speed). |
