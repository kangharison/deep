# Kernel driver surface_fan

> 출처(원문): https://docs.kernel.org/hwmon/surface_fan.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver surface\_fan

Supported Devices:

> * Microsoft Surface Pro 9

Author: Ivor Wanders <[ivor@iwanders.net](mailto:ivor%40iwanders.net)>

## Description

This provides monitoring of the fan found in some Microsoft Surface Pro devices,
like the Surface Pro 9. The fan is always controlled by the onboard controller.

## Sysfs interface

| Name | Perm | Description |
| --- | --- | --- |
| `fan1_input` | RO | Current fan speed in RPM. |
