# 1.ASPEED video driver

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/aspeed-video.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1. ASPEED video driver

ASPEED Video Engine found on AST2400/2500/2600 SoC supports high performance
video compressions with a wide range of video quality and compression ratio
options. The adopted compressing algorithm is a modified JPEG algorithm.

There are 2 types of compressions in this IP.

* JPEG JFIF standard mode: for single frame and management compression
* ASPEED proprietary mode: for multi-frame and differential compression.
  Support 2-pass (high quality) video compression scheme (Patent pending by
  ASPEED). Provide visually lossless video compression quality or to reduce
  the network average loading under intranet KVM applications.

VIDIOC\_S\_FMT can be used to choose which format you want. V4L2\_PIX\_FMT\_JPEG
stands for JPEG JFIF standard mode; V4L2\_PIX\_FMT\_AJPG stands for ASPEED
proprietary mode.

More details on the ASPEED video hardware operations can be found in
*chapter 6.2.16 KVM Video Driver* of SDK\_User\_Guide which available on
[github](https://github.com/AspeedTech-BMC/openbmc/releases/).

The ASPEED video driver implements the following driver-specific control:

## 1.1. `V4L2_CID_ASPEED_HQ_MODE`

> Enable/Disable ASPEED’s High quality mode. This is a private control
> that can be used to enable high quality for aspeed proprietary mode.

|  |  |
| --- | --- |
| `(0)` | ASPEED HQ mode is disabled. |
| `(1)` | ASPEED HQ mode is enabled. |

## 1.2. `V4L2_CID_ASPEED_HQ_JPEG_QUALITY`

> Define the quality of ASPEED’s High quality mode. This is a private control
> that can be used to decide compression quality if High quality mode enabled
> . Higher the value, better the quality and bigger the size.

|  |  |
| --- | --- |
| `(1)` | minimum |
| `(12)` | maximum |
| `(1)` | step |
| `(1)` | default |

**Copyright** © 2022 ASPEED Technology Inc.
