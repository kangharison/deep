# Kernel driver POWERZ

> 출처(원문): https://docs.kernel.org/hwmon/powerz.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver POWERZ

Supported chips:

> * ChargerLAB POWER-Z KM003C
>
>   Prefix: ‘powerz’
>
>   Addresses scanned: -

Author:

> * Thomas Weißschuh <[linux@weissschuh.net](mailto:linux%40weissschuh.net)>

## Description

This driver implements support for the ChargerLAB POWER-Z USB-C power testing
family.

The device communicates with the custom protocol over USB.

The channel labels exposed via hwmon match the labels used by the on-device
display and the official POWER-Z PC software.

As current can flow in both directions through the tester the sign of the
channel “curr1\_input” (label “IBUS”) indicates the direction.
