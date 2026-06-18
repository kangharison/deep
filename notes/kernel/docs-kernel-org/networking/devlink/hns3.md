# hns3 devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/hns3.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# hns3 devlink support

This document describes the devlink features implemented by the `hns3`
device driver.

The `hns3` driver supports reloading via `DEVLINK_CMD_RELOAD`.

## Info versions

The `hns3` driver reports the following versions

devlink info versions implemented





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `fw` | running | Used to represent the firmware version. |
| `fw.scc` | running | Used to represent the Soft Congestion Control (SSC) firmware version. SCC is a firmware component which provides multiple RDMA congestion control algorithms, including DCQCN. |
