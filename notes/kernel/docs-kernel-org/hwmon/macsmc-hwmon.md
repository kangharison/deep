# Kernel driver macsmc-hwmon

> 출처(원문): https://docs.kernel.org/hwmon/macsmc-hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver macsmc-hwmon

Supported hardware

> * Apple Silicon Macs (M1 and up)

Author: James Calligeros <[jcalligeros99@gmail.com](mailto:jcalligeros99%40gmail.com)>

## Description

macsmc-hwmon exposes the Apple System Management controller’s
temperature, voltage, current and power sensors, as well as
fan speed and control capabilities, via hwmon.

Because each Apple Silicon Mac exposes a different set of sensors
(e.g. the MacBooks expose battery telemetry that is not present on
the desktop Macs), sensors present on any given machine are described
via Devicetree. The driver picks these up and registers them with
hwmon when probed.

Manual fan speed is supported via the fan\_control module parameter. This
is disabled by default and marked as unsafe, as it cannot be proven that
the system will fail safe if overheating due to manual fan control being
used.

## sysfs interface

currX\_input
:   Ammeter value

currX\_label
:   Ammeter label

fanX\_input
:   Current fan speed

fanX\_label
:   Fan label

fanX\_min
:   Minimum possible fan speed

fanX\_max
:   Maximum possible fan speed

fanX\_target
:   Current fan setpoint

inX\_input
:   Voltmeter value

inX\_label
:   Voltmeter label

powerX\_input
:   Power meter value

powerX\_label
:   Power meter label

tempX\_input
:   Temperature sensor value

tempX\_label
:   Temperature sensor label
