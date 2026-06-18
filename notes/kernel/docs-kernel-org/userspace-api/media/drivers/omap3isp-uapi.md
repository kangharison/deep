# 10.OMAP 3 Image Signal Processor (ISP) driver

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/omap3isp-uapi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10. OMAP 3 Image Signal Processor (ISP) driver

Copyright © 2010 Nokia Corporation

Copyright © 2009 Texas Instruments, Inc.

Contacts: Laurent Pinchart <[laurent.pinchart@ideasonboard.com](mailto:laurent.pinchart%40ideasonboard.com)>,
Sakari Ailus <[sakari.ailus@iki.fi](mailto:sakari.ailus%40iki.fi)>, David Cohen <[dacohen@gmail.com](mailto:dacohen%40gmail.com)>

## 10.1. Events

The OMAP 3 ISP driver does support the V4L2 event interface on CCDC and
statistics (AEWB, AF and histogram) subdevs.

The CCDC subdev produces V4L2\_EVENT\_FRAME\_SYNC type event on HS\_VS
interrupt which is used to signal frame start. Earlier version of this
driver used V4L2\_EVENT\_OMAP3ISP\_HS\_VS for this purpose. The event is
triggered exactly when the reception of the first line of the frame starts
in the CCDC module. The event can be subscribed on the CCDC subdev.

(When using parallel interface one must pay account to correct configuration
of the VS signal polarity. This is automatically correct when using the serial
receivers.)

Each of the statistics subdevs is able to produce events. An event is
generated whenever a statistics buffer can be dequeued by a user space
application using the VIDIOC\_OMAP3ISP\_STAT\_REQ IOCTL. The events available
are:

* V4L2\_EVENT\_OMAP3ISP\_AEWB
* V4L2\_EVENT\_OMAP3ISP\_AF
* V4L2\_EVENT\_OMAP3ISP\_HIST

The type of the event data is `struct omap3isp_stat_event_status` for these
ioctls. If there is an error calculating the statistics, there will be an
event as usual, but no related statistics buffer. In this case
omap3isp\_stat\_event\_status.buf\_err is set to non-zero.

## 10.2. Private IOCTLs

The OMAP 3 ISP driver supports standard V4L2 IOCTLs and controls where
possible and practical. Much of the functions provided by the ISP, however,
does not fall under the standard IOCTLs --- gamma tables and configuration of
statistics collection are examples of such.

In general, there is a private ioctl for configuring each of the blocks
containing hardware-dependent functions.

The following private IOCTLs are supported:

* VIDIOC\_OMAP3ISP\_CCDC\_CFG
* VIDIOC\_OMAP3ISP\_PRV\_CFG
* VIDIOC\_OMAP3ISP\_AEWB\_CFG
* VIDIOC\_OMAP3ISP\_HIST\_CFG
* VIDIOC\_OMAP3ISP\_AF\_CFG
* VIDIOC\_OMAP3ISP\_STAT\_REQ
* VIDIOC\_OMAP3ISP\_STAT\_EN

The parameter structures used by these ioctls are described in
include/linux/omap3isp.h. The detailed functions of the ISP itself related to
a given ISP block is described in the Technical Reference Manuals (TRMs) ---
see the end of the document for those.

While it is possible to use the ISP driver without any use of these private
IOCTLs it is not possible to obtain optimal image quality this way. The AEWB,
AF and histogram modules cannot be used without configuring them using the
appropriate private IOCTLs.

## 10.3. CCDC and preview block IOCTLs

The VIDIOC\_OMAP3ISP\_CCDC\_CFG and VIDIOC\_OMAP3ISP\_PRV\_CFG IOCTLs are used to
configure, enable and disable functions in the CCDC and preview blocks,
respectively. Both IOCTLs control several functions in the blocks they
control. VIDIOC\_OMAP3ISP\_CCDC\_CFG IOCTL accepts a pointer to `struct
omap3isp_ccdc_update_config` as its argument. Similarly VIDIOC\_OMAP3ISP\_PRV\_CFG
accepts a pointer to `struct omap3isp_prev_update_config`. The definition of
both structures is available in [[1]](#id2).

The update field in the structures tells whether to update the configuration
for the specific function and the flag tells whether to enable or disable the
function.

The update and flag bit masks accept the following values. Each separate
functions in the CCDC and preview blocks is associated with a flag (either
disable or enable; part of the flag field in the structure) and a pointer to
configuration data for the function.

Valid values for the update and flag fields are listed here for
VIDIOC\_OMAP3ISP\_CCDC\_CFG. Values may be or’ed to configure more than one
function in the same IOCTL call.

* OMAP3ISP\_CCDC\_ALAW
* OMAP3ISP\_CCDC\_LPF
* OMAP3ISP\_CCDC\_BLCLAMP
* OMAP3ISP\_CCDC\_BCOMP
* OMAP3ISP\_CCDC\_FPC
* OMAP3ISP\_CCDC\_CULL
* OMAP3ISP\_CCDC\_CONFIG\_LSC
* OMAP3ISP\_CCDC\_TBL\_LSC

The corresponding values for the VIDIOC\_OMAP3ISP\_PRV\_CFG are here:

* OMAP3ISP\_PREV\_LUMAENH
* OMAP3ISP\_PREV\_INVALAW
* OMAP3ISP\_PREV\_HRZ\_MED
* OMAP3ISP\_PREV\_CFA
* OMAP3ISP\_PREV\_CHROMA\_SUPP
* OMAP3ISP\_PREV\_WB
* OMAP3ISP\_PREV\_BLKADJ
* OMAP3ISP\_PREV\_RGB2RGB
* OMAP3ISP\_PREV\_COLOR\_CONV
* OMAP3ISP\_PREV\_YC\_LIMIT
* OMAP3ISP\_PREV\_DEFECT\_COR
* OMAP3ISP\_PREV\_GAMMABYPASS
* OMAP3ISP\_PREV\_DRK\_FRM\_CAPTURE
* OMAP3ISP\_PREV\_DRK\_FRM\_SUBTRACT
* OMAP3ISP\_PREV\_LENS\_SHADING
* OMAP3ISP\_PREV\_NF
* OMAP3ISP\_PREV\_GAMMA

The associated configuration pointer for the function may not be NULL when
enabling the function. When disabling a function the configuration pointer is
ignored.

## 10.4. Statistic blocks IOCTLs

The statistics subdevs do offer more dynamic configuration options than the
other subdevs. They can be enabled, disable and reconfigured when the pipeline
is in streaming state.

The statistics blocks always get the input image data from the CCDC (as the
histogram memory read isn’t implemented). The statistics are dequeueable by
the user from the statistics subdev nodes using private IOCTLs.

The private IOCTLs offered by the AEWB, AF and histogram subdevs are heavily
reflected by the register level interface offered by the ISP hardware. There
are aspects that are purely related to the driver implementation and these are
discussed next.

## 10.5. VIDIOC\_OMAP3ISP\_STAT\_EN

This private IOCTL enables/disables a statistic module. If this request is
done before streaming, it will take effect as soon as the pipeline starts to
stream. If the pipeline is already streaming, it will take effect as soon as
the CCDC becomes idle.

## 10.6. VIDIOC\_OMAP3ISP\_AEWB\_CFG, VIDIOC\_OMAP3ISP\_HIST\_CFG and VIDIOC\_OMAP3ISP\_AF\_CFG

Those IOCTLs are used to configure the modules. They require user applications
to have an in-depth knowledge of the hardware. Most of the fields explanation
can be found on OMAP’s TRMs. The two following fields common to all the above
configure private IOCTLs require explanation for better understanding as they
are not part of the TRM.

omap3isp\_[h3a\_af/h3a\_aewb/hist]\_config.buf\_size:

The modules handle their buffers internally. The necessary buffer size for the
module’s data output depends on the requested configuration. Although the
driver supports reconfiguration while streaming, it does not support a
reconfiguration which requires bigger buffer size than what is already
internally allocated if the module is enabled. It will return -EBUSY on this
case. In order to avoid such condition, either disable/reconfigure/enable the
module or request the necessary buffer size during the first configuration
while the module is disabled.

The internal buffer size allocation considers the requested configuration’s
minimum buffer size and the value set on buf\_size field. If buf\_size field is
out of [minimum, maximum] buffer size range, it’s clamped to fit in there.
The driver then selects the biggest value. The corrected buf\_size value is
written back to user application.

omap3isp\_[h3a\_af/h3a\_aewb/hist]\_config.config\_counter:

As the configuration doesn’t take effect synchronously to the request, the
driver must provide a way to track this information to provide more accurate
data. After a configuration is requested, the config\_counter returned to user
space application will be an unique value associated to that request. When
user application receives an event for buffer availability or when a new
buffer is requested, this config\_counter is used to match a buffer data and a
configuration.

## 10.7. VIDIOC\_OMAP3ISP\_STAT\_REQ

Send to user space the oldest data available in the internal buffer queue and
discards such buffer afterwards. The field omap3isp\_stat\_data.frame\_number
matches with the video buffer’s field\_count.

## 10.8. References

[[1](#id1)]

include/linux/omap3isp.h
