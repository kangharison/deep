# 2.13.11.V4L2_META_FMT_UVC_MSXU_1_5 (‘UVCM’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-uvc-msxu-1-5.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.11. V4L2\_META\_FMT\_UVC\_MSXU\_1\_5 (‘UVCM’)

Microsoft(R)’s UVC Payload Metadata.

## 2.13.11.1. Description

V4L2\_META\_FMT\_UVC\_MSXU\_1\_5 buffers follow the metadata buffer layout of
V4L2\_META\_FMT\_UVC with the only difference that it includes all the UVC
metadata in the buffer[] field, not just the first 2-12 bytes.

The metadata format follows the specification from Microsoft(R) [1].

[1] <https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/uvc-extensions-1-5>
