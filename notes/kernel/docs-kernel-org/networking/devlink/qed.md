# qed devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/qed.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# qed devlink support

This document describes the devlink features implemented by the `qed` core
device driver.

## Parameters

The `qed` driver implements the following driver-specific parameters.

Driver-specific parameters implemented






|  |  |  |  |
| --- | --- | --- | --- |
| Name | Type | Mode | Description |
| `iwarp_cmt` | Boolean | runtime | Enable iWARP functionality for 100g devices. Note that this impacts L2 performance, and is therefore not enabled by default. |
