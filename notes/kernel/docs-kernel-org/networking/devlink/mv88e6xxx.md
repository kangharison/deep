# mv88e6xxx devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/mv88e6xxx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# mv88e6xxx devlink support

This document describes the devlink features implemented by the `mv88e6xxx`
device driver.

## Parameters

The `mv88e6xxx` driver implements the following driver-specific parameters.

Driver-specific parameters implemented






|  |  |  |  |
| --- | --- | --- | --- |
| Name | Type | Mode | Description |
| `ATU_hash` | u8 | runtime | Select one of four possible hashing algorithms for MAC addresses in the Address Translation Unit. A value of 3 may work better than the default of 1 when many MAC addresses have the same OUI. Only the values 0 to 3 are valid for this parameter. |
