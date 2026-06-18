# Kernel driver for lp5812

> 출처(원문): https://docs.kernel.org/leds/leds-lp5812.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver for lp5812

* TI/National Semiconductor LP5812 LED Driver
* Datasheet: <https://www.ti.com/product/LP5812#tech-docs>

Authors: Jared Zhou <[jared-zhou@ti.com](mailto:jared-zhou%40ti.com)>

## Description

The LP5812 is a 4x3 matrix LED driver with support for both manual and
autonomous animation control. This driver provides sysfs interfaces to
control and configure the LP5812 device and its LED channels.

## Sysfs Interface

This driver uses the standard multicolor LED class interfaces defined
in Documentation/ABI/testing/sysfs-class-led-multicolor.rst[ABI file testing/sysfs-class-led-multicolor](../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-class-led-multicolor).rst.

Each LP5812 LED output appears under `/sys/class/leds/` with its
assigned label (for example `LED_A`).

The following attributes are exposed:
:   * multi\_intensity: Per-channel RGB intensity control
    * brightness: Standard brightness control (0-255)

## Autonomous Control Modes

The driver also supports autonomous control through pattern configuration
(e.g., direct, tcmscan, or mixscan modes) defined in the device tree.
When configured, the LP5812 can generate transitions and color effects
without CPU intervention.

Refer to the device tree binding document for valid mode strings and
configuration examples.

## Example Usage

To control LED\_A::
:   # Set RGB intensity (R=50, G=50, B=50)
    echo 50 50 50 > /sys/class/leds/LED\_A/multi\_intensity
    # Set overall brightness to maximum
    echo 255 > /sys/class/leds/LED\_A/brightness
