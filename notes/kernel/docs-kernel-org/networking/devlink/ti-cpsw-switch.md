# ti-cpsw-switch devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/ti-cpsw-switch.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ti-cpsw-switch devlink support

This document describes the devlink features implemented by the `ti-cpsw-switch`
device driver.

## Parameters

The `ti-cpsw-switch` driver implements the following driver-specific
parameters.

Driver-specific parameters implemented






|  |  |  |  |
| --- | --- | --- | --- |
| Name | Type | Mode | Description |
| `ale_bypass` | Boolean | runtime | Enables ALE\_CONTROL(4).BYPASS mode for debugging purposes. In this mode, all packets will be sent to the host port only. |
| `switch_mode` | Boolean | runtime | Enable switch mode |
