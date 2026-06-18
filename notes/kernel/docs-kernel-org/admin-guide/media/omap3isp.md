# 7.13.OMAP 3 Image Signal Processor (ISP) driver

> 출처(원문): https://docs.kernel.org/admin-guide/media/omap3isp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.13. OMAP 3 Image Signal Processor (ISP) driver

Copyright © 2010 Nokia Corporation

Copyright © 2009 Texas Instruments, Inc.

Contacts: Laurent Pinchart <[laurent.pinchart@ideasonboard.com](mailto:laurent.pinchart%40ideasonboard.com)>,
Sakari Ailus <[sakari.ailus@iki.fi](mailto:sakari.ailus%40iki.fi)>, David Cohen <[dacohen@gmail.com](mailto:dacohen%40gmail.com)>

## 7.13.1. Introduction

This file documents the Texas Instruments OMAP 3 Image Signal Processor (ISP)
driver located under drivers/media/platform/ti/omap3isp. The original driver was
written by Texas Instruments but since that it has been rewritten (twice) at
Nokia.

The driver has been successfully used on the following versions of OMAP 3:

* 3430
* 3530
* 3630

The driver implements V4L2, Media controller and v4l2\_subdev interfaces.
Sensor, lens and flash drivers using the v4l2\_subdev interface in the kernel
are supported.

## 7.13.2. Split to subdevs

The OMAP 3 ISP is split into V4L2 subdevs, each of the blocks inside the ISP
having one subdev to represent it. Each of the subdevs provide a V4L2 subdev
interface to userspace.

* OMAP3 ISP CCP2
* OMAP3 ISP CSI2a
* OMAP3 ISP CCDC
* OMAP3 ISP preview
* OMAP3 ISP resizer
* OMAP3 ISP AEWB
* OMAP3 ISP AF
* OMAP3 ISP histogram

Each possible link in the ISP is modelled by a link in the Media controller
interface. For an example program see [[1]](#id2).

## 7.13.3. Controlling the OMAP 3 ISP

In general, the settings given to the OMAP 3 ISP take effect at the beginning
of the following frame. This is done when the module becomes idle during the
vertical blanking period on the sensor. In memory-to-memory operation the pipe
is run one frame at a time. Applying the settings is done between the frames.

All the blocks in the ISP, excluding the CSI-2 and possibly the CCP2 receiver,
insist on receiving complete frames. Sensors must thus never send the ISP
partial frames.

Autoidle does have issues with some ISP blocks on the 3430, at least.
Autoidle is only enabled on 3630 when the omap3isp module parameter autoidle
is non-zero.

## 7.13.4. Technical reference manuals (TRMs) and other documentation

OMAP 3430 TRM:
<URL:http://focus.ti.com/pdfs/wtbu/OMAP34xx\_ES3.1.x\_PUBLIC\_TRM\_vZM.zip>
Referenced 2011-03-05.

OMAP 35xx TRM:
<URL:http://www.ti.com/litv/pdf/spruf98o> Referenced 2011-03-05.

OMAP 3630 TRM:
<URL:http://focus.ti.com/pdfs/wtbu/OMAP36xx\_ES1.x\_PUBLIC\_TRM\_vQ.zip>
Referenced 2011-03-05.

DM 3730 TRM:
<URL:http://www.ti.com/litv/pdf/sprugn4h> Referenced 2011-03-06.

## 7.13.5. References

[[1](#id1)]

<http://git.ideasonboard.org/?p=media-ctl.git;a=summary>
