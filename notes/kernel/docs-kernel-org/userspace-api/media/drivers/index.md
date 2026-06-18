# Video4Linux (V4L)  driver-specific documentation

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Video4Linux (V4L) driver-specific documentation

**Copyright** © 1999-2016 : LinuxTV Developers

This documentation is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

For more details see the file COPYING in the source distribution of Linux.

Table of Contents

* [1. ASPEED video driver](aspeed-video.html)
  + [1.1. `V4L2_CID_ASPEED_HQ_MODE`](aspeed-video.html#v4l2-cid-aspeed-hq-mode)
  + [1.2. `V4L2_CID_ASPEED_HQ_JPEG_QUALITY`](aspeed-video.html#v4l2-cid-aspeed-hq-jpeg-quality)
* [2. Using camera sensor drivers](camera-sensor.html)
  + [2.1. Sensor internal pipeline configuration](camera-sensor.html#sensor-internal-pipeline-configuration)
    - [2.1.1. Freely configurable camera sensor drivers](camera-sensor.html#freely-configurable-camera-sensor-drivers)
    - [2.1.2. Register list-based drivers](camera-sensor.html#register-list-based-drivers)
  + [2.2. Frame interval configuration](camera-sensor.html#frame-interval-configuration)
    - [2.2.1. Raw camera sensors](camera-sensor.html#raw-camera-sensors)
    - [2.2.2. USB cameras etc. devices](camera-sensor.html#usb-cameras-etc-devices)
  + [2.3. Rotation, orientation and flipping](camera-sensor.html#rotation-orientation-and-flipping)
* [3. MIPI CCS camera sensor driver](ccs.html)
  + [3.1. Pixel Array sub-device](ccs.html#pixel-array-sub-device)
  + [3.2. Binner](ccs.html#binner)
  + [3.3. Scaler](ccs.html#scaler)
  + [3.4. Digital and analogue crop](ccs.html#digital-and-analogue-crop)
  + [3.5. Private controls](ccs.html#private-controls)
    - [3.5.1. Analogue gain model](ccs.html#analogue-gain-model)
    - [3.5.2. Alternate analogue gain model](ccs.html#alternate-analogue-gain-model)
    - [3.5.3. Shading correction](ccs.html#shading-correction)
* [4. The cx2341x driver](cx2341x-uapi.html)
  + [4.1. Non-compressed file format](cx2341x-uapi.html#non-compressed-file-format)
    - [4.1.1. Raw format c example](cx2341x-uapi.html#raw-format-c-example)
  + [4.2. Format of embedded V4L2\_MPEG\_STREAM\_VBI\_FMT\_IVTV VBI data](cx2341x-uapi.html#format-of-embedded-v4l2-mpeg-stream-vbi-fmt-ivtv-vbi-data)
* [5. DW100 dewarp driver](dw100.html)
* [6. i.MX Video Capture Driver](imx-uapi.html)
  + [6.1. Events](imx-uapi.html#events)
    - [6.1.1. ipuX\_csiY](imx-uapi.html#ipux-csiy)
  + [6.2. Controls](imx-uapi.html#controls)
    - [6.2.1. Frame Interval Monitor in ipuX\_csiY](imx-uapi.html#frame-interval-monitor-in-ipux-csiy)
    - [6.2.2. File list](imx-uapi.html#file-list)
    - [6.2.3. Authors](imx-uapi.html#authors)
* [7. Arm Mali-C55 ISP driver](mali-c55.html)
* [8. Maxim Integrated MAX2175 RF to bits tuner driver](max2175.html)
  + [8.1. `V4L2_CID_MAX2175_I2S_ENABLE`](max2175.html#v4l2-cid-max2175-i2s-enable)
  + [8.2. `V4L2_CID_MAX2175_HSLS`](max2175.html#v4l2-cid-max2175-hsls)
  + [8.3. `V4L2_CID_MAX2175_RX_MODE (menu)`](max2175.html#v4l2-cid-max2175-rx-mode-menu)
* [9. NPCM video driver](npcm-video.html)
  + [9.1. Driver-specific Controls](npcm-video.html#driver-specific-controls)
    - [9.1.1. V4L2\_CID\_NPCM\_CAPTURE\_MODE](npcm-video.html#v4l2-cid-npcm-capture-mode)
    - [9.1.2. V4L2\_CID\_NPCM\_RECT\_COUNT](npcm-video.html#v4l2-cid-npcm-rect-count)
  + [9.2. References](npcm-video.html#references)
* [10. OMAP 3 Image Signal Processor (ISP) driver](omap3isp-uapi.html)
  + [10.1. Events](omap3isp-uapi.html#events)
  + [10.2. Private IOCTLs](omap3isp-uapi.html#private-ioctls)
  + [10.3. CCDC and preview block IOCTLs](omap3isp-uapi.html#ccdc-and-preview-block-ioctls)
  + [10.4. Statistic blocks IOCTLs](omap3isp-uapi.html#statistic-blocks-ioctls)
  + [10.5. VIDIOC\_OMAP3ISP\_STAT\_EN](omap3isp-uapi.html#vidioc-omap3isp-stat-en)
  + [10.6. VIDIOC\_OMAP3ISP\_AEWB\_CFG, VIDIOC\_OMAP3ISP\_HIST\_CFG and VIDIOC\_OMAP3ISP\_AF\_CFG](omap3isp-uapi.html#vidioc-omap3isp-aewb-cfg-vidioc-omap3isp-hist-cfg-and-vidioc-omap3isp-af-cfg)
  + [10.7. VIDIOC\_OMAP3ISP\_STAT\_REQ](omap3isp-uapi.html#vidioc-omap3isp-stat-req)
  + [10.8. References](omap3isp-uapi.html#references)
* [11. THine THP7312 ISP driver](thp7312.html)
* [12. The Linux USB Video Class (UVC) driver](uvcvideo.html)
  + [12.1. Extension Unit (XU) support](uvcvideo.html#extension-unit-xu-support)
    - [12.1.1. Introduction](uvcvideo.html#introduction)
    - [12.1.2. Control mappings](uvcvideo.html#control-mappings)
    - [12.1.3. Security](uvcvideo.html#security)
    - [12.1.4. Debugging](uvcvideo.html#debugging)
    - [12.1.5. IOCTL reference](uvcvideo.html#ioctl-reference)
      * [12.1.5.1. UVCIOC\_CTRL\_MAP - Map a UVC control to a V4L2 control](uvcvideo.html#uvcioc-ctrl-map-map-a-uvc-control-to-a-v4l2-control)
      * [12.1.5.2. UVCIOC\_CTRL\_QUERY - Query a UVC XU control](uvcvideo.html#uvcioc-ctrl-query-query-a-uvc-xu-control)
  + [12.2. Driver-specific V4L2 controls](uvcvideo.html#driver-specific-v4l2-controls)
* [13. ST VGXY61 camera sensor driver](vgxy61.html)
  + [13.1. `V4L2_CID_HDR_SENSOR_MODE`](vgxy61.html#v4l2-cid-hdr-sensor-mode)
