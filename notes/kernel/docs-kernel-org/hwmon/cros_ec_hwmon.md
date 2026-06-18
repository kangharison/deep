# Kernel driver cros_ec_hwmon

> 출처(원문): https://docs.kernel.org/hwmon/cros_ec_hwmon.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver cros\_ec\_hwmon

Supported chips:

> * ChromeOS embedded controllers.
>
>   Prefix: ‘cros\_ec’
>
>   Addresses scanned: -

Author:

> * Thomas Weißschuh <[linux@weissschuh.net](mailto:linux%40weissschuh.net)>

## Description

This driver implements support for hardware monitoring commands exposed by the
ChromeOS embedded controller used in Chromebooks and other devices.

The channel labels exposed via hwmon are retrieved from the EC itself.

## Supported features

Fan readings
:   Always supported.

Fan target speed
:   If supported by the EC.

Temperature readings
:   Always supported.

Temperature thresholds
:   If supported by the EC.

PWM fan control
:   If the EC also supports setting fan PWM values and fan mode.

    Note that EC will switch fan control mode back to auto when suspended.
    This driver will restore the fan state to what they were before suspended when resumed.

    If a fan is controllable, this driver will register that fan as a cooling device
    in the thermal framework as well.
