# 2.13.12.V4L2_META_FMT_VIVID (‘VIVD’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-vivid.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.12. V4L2\_META\_FMT\_VIVID (‘VIVD’)

VIVID Metadata Format

## 2.13.12.1. Description

This describes metadata format used by the vivid driver.

It sets Brightness, Saturation, Contrast and Hue, each of which maps to
corresponding controls of the vivid driver with respect to the range and default values.

It contains the following fields:

VIVID Metadata

| Field | Description |
| --- | --- |
| u16 brightness; | Image brightness, the value is in the range 0 to 255, with the default value as 128. |
| u16 contrast; | Image contrast, the value is in the range 0 to 255, with the default value as 128. |
| u16 saturation; | Image color saturation, the value is in the range 0 to 255, with the default value as 128. |
| s16 hue; | Image color balance, the value is in the range -128 to 128, with the default value as 0. |
