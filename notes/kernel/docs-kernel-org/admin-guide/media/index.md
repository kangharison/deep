# Media subsystem admin and user guide

> 출처(원문): https://docs.kernel.org/admin-guide/media/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Media subsystem admin and user guide

This section contains usage information about media subsystem and
its supported drivers.

Please see:

[Linux Media Infrastructure userspace API](../../userspace-api/media/index.html)

> * for the userspace APIs used on media devices.

[Media subsystem kernel internal API](../../driver-api/media/index.html)

> * for driver development information and Kernel APIs used by
>   media devices;

[Debugging and tracing in the media subsystem](../../process/debugging/media_specific_debugging_guide.html)

> * for advice about essential tools and techniques to debug drivers on this
>   subsystem

Table of Contents

* [1. Introduction](intro.html)
* [2. Building support for a media device](building.html)
  + [2.1. Configuring the Linux Kernel](building.html#configuring-the-linux-kernel)
  + [2.2. Building and installing a new Kernel](building.html#building-and-installing-a-new-kernel)
  + [2.3. Building just the new media drivers and core](building.html#building-just-the-new-media-drivers-and-core)
* [3. Infrared remote control support in video4linux drivers](remote-controller.html)
  + [3.1. Basics](remote-controller.html#basics)
  + [3.2. How it works](remote-controller.html#how-it-works)
* [4. HDMI CEC](cec.html)
  + [4.1. Supported hardware in mainline](cec.html#supported-hardware-in-mainline)
  + [4.2. Utilities](cec.html#utilities)
  + [4.3. DisplayPort to HDMI Adapters with working CEC](cec.html#displayport-to-hdmi-adapters-with-working-cec)
  + [4.4. USB CEC Dongles](cec.html#usb-cec-dongles)
  + [4.5. CEC Without HPD](cec.html#cec-without-hpd)
  + [4.6. Microcontrollers & CEC](cec.html#microcontrollers-cec)
  + [4.7. Making a CEC debugger](cec.html#making-a-cec-debugger)
  + [4.8. Extron DA HD 4K PLUS CEC Adapter driver](cec.html#extron-da-hd-4k-plus-cec-adapter-driver)
* [5. Digital TV](dvb.html)
  + [5.1. Using the Digital TV Framework](dvb_intro.html)
  + [5.2. Digital TV Conditional Access Interface](ci.html)
  + [5.3. FAQ](faq.html)
  + [5.4. References](dvb_references.html)
* [6. Cards List](cardlist.html)
  + [6.1. USB drivers](usb-cardlist.html)
  + [6.2. PCI drivers](pci-cardlist.html)
  + [6.3. Platform drivers](platform-cardlist.html)
  + [6.4. Radio drivers](radio-cardlist.html)
  + [6.5. I²C drivers](i2c-cardlist.html)
  + [6.6. Firewire driver](misc-cardlist.html)
  + [6.7. Test drivers](misc-cardlist.html#test-drivers)
* [7. Video4Linux (V4L) driver-specific documentation](v4l-drivers.html)
  + [7.1. The bttv driver](bttv.html)
  + [7.2. Amlogic C3 Image Signal Processing (C3ISP) driver](c3-isp.html)
  + [7.3. The cafe\_ccic driver](cafe_ccic.html)
  + [7.4. The cx88 driver](cx88.html)
  + [7.5. The Samsung S5P/Exynos4 FIMC driver](fimc.html)
  + [7.6. i.MX Video Capture Driver](imx.html)
  + [7.7. i.MX7 Video Capture Driver](imx7.html)
  + [7.8. Intel Image Processing Unit 3 (IPU3) Imaging Unit (ImgU) driver](ipu3.html)
  + [7.9. Intel Image Processing Unit 6 (IPU6) Input System driver](ipu6-isys.html)
  + [7.10. The ivtv driver](ivtv.html)
  + [7.11. ARM Mali-C55 Image Signal Processor driver](mali-c55.html)
  + [7.12. The mgb4 driver](mgb4.html)
  + [7.13. OMAP 3 Image Signal Processor (ISP) driver](omap3isp.html)
  + [7.14. Philips webcams (pwc driver)](philips.html)
  + [7.15. Qualcomm Camera Subsystem driver](qcom_camss.html)
  + [7.16. Raspberry Pi PiSP Back End Memory-to-Memory ISP (pisp-be)](raspberrypi-pisp-be.html)
  + [7.17. Renesas R-Car Fine Display Processor (FDP1) Driver](rcar-fdp1.html)
  + [7.18. Rockchip Camera Interface (CIF)](rkcif.html)
  + [7.19. Rockchip Image Signal Processor (rkisp1)](rkisp1.html)
  + [7.20. Raspberry Pi PiSP Camera Front End (rp1-cfe)](raspberrypi-rp1-cfe.html)
  + [7.21. The saa7134 driver](saa7134.html)
  + [7.22. The Silicon Labs Si470x FM Radio Receivers driver](si470x.html)
  + [7.23. The Silicon Labs Si4713 FM Radio Transmitter Driver](si4713.html)
  + [7.24. The SI476x Driver](si476x.html)
  + [7.25. The Virtual Media Controller Driver (vimc)](vimc.html)
  + [7.26. The Virtual Stateless Decoder Driver (visl)](visl.html)
  + [7.27. The Virtual Video Test Driver (vivid)](vivid.html)
* [8. Digital TV driver-specific documentation](dvb-drivers.html)
  + [8.1. Avermedia DVB-T on BT878 Release Notes](avermedia.html)
  + [8.2. How to get the bt8xx cards working](bt8xx.html)
  + [8.3. Firmware files for lmedm04 cards](lmedm04.html)
  + [8.4. Opera firmware](opera-firmware.html)
  + [8.5. How to set up the Technisat/B2C2 Flexcop devices](technisat.html)
  + [8.6. TechnoTrend/Hauppauge DEC USB Driver](ttusb-dec.html)

**Copyright** © 1999-2020 : LinuxTV Developers

```
This documentation is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

For more details see the file COPYING in the source distribution of Linux.
```
