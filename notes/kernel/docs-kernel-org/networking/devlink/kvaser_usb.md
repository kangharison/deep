# kvaser_usb devlink support

> 출처(원문): https://docs.kernel.org/networking/devlink/kvaser_usb.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# kvaser\_usb devlink support

This document describes the devlink features implemented by the
`kvaser_usb` device driver.

## Info versions

The `kvaser_usb` driver reports the following versions

devlink info versions implemented





|  |  |  |
| --- | --- | --- |
| Name | Type | Description |
| `fw` | running | Version of the firmware running on the device. Also available through `ethtool -i` as `firmware-version`. |
| `board.rev` | fixed | The device hardware revision. |
| `board.id` | fixed | The device EAN (product number). |
| `serial_number` | fixed | The device serial number. |
