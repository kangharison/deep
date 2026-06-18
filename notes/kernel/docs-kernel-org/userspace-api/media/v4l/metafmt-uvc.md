# 2.13.10.V4L2_META_FMT_UVC (‘UVCH’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-uvc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.10. V4L2\_META\_FMT\_UVC (‘UVCH’)

UVC Payload Header Data

## 2.13.10.1. Description

This format describes standard UVC metadata, extracted from UVC packet headers
and provided by the UVC driver through metadata video nodes. That data includes
exact copies of the standard part of UVC Payload Header contents and auxiliary
timing information, required for precise interpretation of timestamps, contained
in those headers. See section “2.4.3.3 Video and Still Image Payload Headers” of
the “UVC 1.5 Class specification” for details.

Each UVC payload header can be between 2 and 12 bytes large. Buffers can
contain multiple headers, if multiple such headers have been transmitted by the
camera for the respective frame. However, the driver may drop headers when the
buffer is full, when they contain no useful information (e.g. those without the
SCR field or with that field identical to the previous header), or generally to
perform rate limiting when the device sends a large number of headers.

Each individual block contains the following fields:

UVC Metadata Block

| Field | Description |
| --- | --- |
| \_\_u64 ts; | system timestamp in host byte order, measured by the driver upon reception of the payload |
| \_\_u16 sof; | USB Frame Number in host byte order, also obtained by the driver as close as possible to the above timestamp to enable correlation between them |
| *The rest is an exact copy of the UVC payload header:* | |
| \_\_u8 length; | length of the rest of the block, including this field. Please note that regardless of this value, for V4L2\_META\_FMT\_UVC the kernel will never copy more than 2-12 bytes. |
| \_\_u8 flags; | Flags, indicating presence of other standard UVC fields |
| \_\_u8 buf[]; | The rest of the header, possibly including UVC PTS and SCR fields |
