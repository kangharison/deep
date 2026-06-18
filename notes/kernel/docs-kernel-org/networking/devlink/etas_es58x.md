# etas_es58x devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/etas_es58x.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# etas\_es58x devlink support

This document describes the devlink features implemented by the
`etas_es58x` device driver.

## Info versions

The `etas_es58x` driver reports the following versions

devlink info versions implemented





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `fw` | running | Version of the firmware running on the device. Also available through `ethtool -i` as the first member of the `firmware-version`. |
| `fw.bootloader` | running | Version of the bootloader running on the device. Also available through `ethtool -i` as the second member of the `firmware-version`. |
| `board.rev` | fixed | The hardware revision of the device. |
| `serial_number` | fixed | The USB serial number. Also available through `lsusb -v`. |
