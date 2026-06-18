# 13.ST VGXY61 camera sensor driver

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/vgxy61.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 13. ST VGXY61 camera sensor driver

The ST VGXY61 driver implements the following controls:

## 13.1. `V4L2_CID_HDR_SENSOR_MODE`

> Change the sensor HDR mode. A HDR picture is obtained by merging two
> captures of the same scene using two different exposure periods.

|  |  |
| --- | --- |
| HDR linearize | The merger outputs a long exposure capture as long as it is not saturated. |
| HDR subtraction | This involves subtracting the short exposure frame from the long exposure frame. |
| No HDR | This mode is used for standard dynamic range (SDR) exposures. |
