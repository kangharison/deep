# octeontx2 devlink support

> 출처(원문): https://docs.kernel.org/crypto/device_drivers/octeontx2.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# octeontx2 devlink support

This document describes the devlink features implemented by the `octeontx2 CPT`
device drivers.

## Parameters

The `octeontx2` driver implements the following driver-specific parameters.

Driver-specific parameters implemented






|  |  |  |  |
| --- | --- | --- | --- |
| Name | Type | Mode | Description |
| `t106_mode` | u8 | runtime | Used to configure CN10KA B0/CN10KB CPT to work as CN10KA A0/A1. |
