# drm/mcde ST-Ericsson MCDE Multi-channel display engine

> 출처(원문): https://docs.kernel.org/gpu/mcde.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/mcde ST-Ericsson MCDE Multi-channel display engine

The MCDE (short for multi-channel display engine) is a graphics
controller found in the Ux500 chipsets, such as NovaThor U8500.
It was initially conceptualized by ST Microelectronics for the
successor of the Nomadik line, STn8500 but productified in the
ST-Ericsson U8500 where is was used for mass-market deployments
in Android phones from Samsung and Sony Ericsson.

It can do 1080p30 on SDTV CCIR656, DPI-2, DBI-2 or DSI for
panels with or without frame buffering and can convert most
input formats including most variants of RGB and YUV.

The hardware has four display pipes, and the layout is a little
bit like this:

```
Memory     -> Overlay -> Channel -> FIFO -> 8 formatters -> DSI/DPI
External      0..5       0..3       A,B,    6 x DSI         bridge
source 0..9                         C0,C1   2 x DPI
```

FIFOs A and B are for LCD and HDMI while FIFO CO/C1 are for
panels with embedded buffer.
6 of the formatters are for DSI, 3 pairs for VID/CMD respectively.
2 of the formatters are for DPI.

Behind the formatters are the DSI or DPI ports that route to
the external pins of the chip. As there are 3 DSI ports and one
DPI port, it is possible to configure up to 4 display pipelines
(effectively using channels 0..3) for concurrent use.

In the current DRM/KMS setup, we use one external source, one overlay,
one FIFO and one formatter which we connect to the simple DMA framebuffer
helpers. We then provide a bridge to the DSI port, and on the DSI port
bridge we connect hang a panel bridge or other bridge. This may be subject
to change as we exploit more of the hardware capabilities.

TODO:

* Enabled damaged rectangles using [`drm_plane_enable_fb_damage_clips()`](drm-kms.html#c.drm_plane_enable_fb_damage_clips "drm_plane_enable_fb_damage_clips")
  so we can selectively just transmit the damaged area to a
  command-only display.
* Enable mixing of more planes, possibly at the cost of moving away
  from using the simple framebuffer pipeline.
* Enable output to bridges such as the AV8100 HDMI encoder from
  the DSI bridge.
