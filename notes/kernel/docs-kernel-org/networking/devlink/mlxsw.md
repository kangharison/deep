# mlxsw devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/mlxsw.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# mlxsw devlink support

This document describes the devlink features implemented by the `mlxsw`
device driver.

## Parameters

Generic parameters implemented

|  |  |
| --- | --- |
| Name | Mode |
| `fw_load_policy` | driverinit |

The `mlxsw` driver also implements the following driver-specific
parameters.

Driver-specific parameters implemented






|  |  |  |  |
| --- | --- | --- | --- |
| Name | Type | Mode | Description |
| `acl_region_rehash_interval` | u32 | runtime | Sets an interval for periodic ACL region rehashes. The value is specified in milliseconds, with a minimum of `3000`. The value of `0` disables periodic work entirely. The first rehash will be run immediately after the value is set. |

The `mlxsw` driver supports reloading via `DEVLINK_CMD_RELOAD`

## Info versions

The `mlxsw` driver reports the following versions

devlink info versions implemented





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `hw.revision` | fixed | The hardware revision for this board |
| `fw.psid` | fixed | Firmware PSID |
| `fw.version` | running | Three digit firmware version |

## Line card auxiliary device info versions

The `mlxsw` driver reports the following versions for line card auxiliary device

devlink info versions implemented





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `hw.revision` | fixed | The hardware revision for this line card |
| `ini.version` | running | Version of line card INI loaded |
| `fw.psid` | fixed | Line card device PSID |
| `fw.version` | running | Three digit firmware version of line card device |

## Driver-specific Traps

List of Driver-specific Traps Registered by `mlxsw`





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `irif_disabled` | `drop` | Traps packets that the device decided to drop because they need to be routed from a disabled router interface (RIF). This can happen during RIF dismantle, when the RIF is first disabled before being removed completely |
| `erif_disabled` | `drop` | Traps packets that the device decided to drop because they need to be routed through a disabled router interface (RIF). This can happen during RIF dismantle, when the RIF is first disabled before being removed completely |
