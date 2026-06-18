# 6.Cards List

> 출처(원문): https://docs.kernel.org/admin-guide/media/cardlist.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6. Cards List

The media subsystem provide support for lots of PCI and USB drivers, plus
platform-specific drivers. It also contains several ancillary I²C drivers.

The platform-specific drivers are usually present on embedded systems,
or are supported by the main board. Usually, setting them is done via
OpenFirmware or ACPI.

The PCI and USB drivers, however, are independent of the system’s board,
and may be added/removed by the user.

You may also take a look at
<https://linuxtv.org/wiki/index.php/Hardware_Device_Information>
for more details about supported cards.

* [6.1. USB drivers](usb-cardlist.html)
  + [6.1.1. AU0828 cards list](au0828-cardlist.html)
  + [6.1.2. cx231xx cards list](cx231xx-cardlist.html)
  + [6.1.3. EM28xx cards list](em28xx-cardlist.html)
  + [6.1.4. Siano cards list](siano-cardlist.html)
  + [6.1.5. The gspca cards list](gspca-cardlist.html)
  + [6.1.6. dvb-usb-dib0700 cards list](dvb-usb-dib0700-cardlist.html)
  + [6.1.7. dvb-usb-dibusb-mb cards list](dvb-usb-dibusb-mb-cardlist.html)
  + [6.1.8. dvb-usb-dibusb-mc cards list](dvb-usb-dibusb-mc-cardlist.html)
  + [6.1.9. dvb-usb-a800 cards list](dvb-usb-a800-cardlist.html)
  + [6.1.10. dvb-usb-af9005 cards list](dvb-usb-af9005-cardlist.html)
  + [6.1.11. dvb-usb-az6027 cards list](dvb-usb-az6027-cardlist.html)
  + [6.1.12. dvb-usb-cinergyT2 cards list](dvb-usb-cinergyT2-cardlist.html)
  + [6.1.13. dvb-usb-cxusb cards list](dvb-usb-cxusb-cardlist.html)
  + [6.1.14. dvb-usb-digitv cards list](dvb-usb-digitv-cardlist.html)
  + [6.1.15. dvb-usb-dtt200u cards list](dvb-usb-dtt200u-cardlist.html)
  + [6.1.16. dvb-usb-dtv5100 cards list](dvb-usb-dtv5100-cardlist.html)
  + [6.1.17. dvb-usb-dw2102 cards list](dvb-usb-dw2102-cardlist.html)
  + [6.1.18. dvb-usb-gp8psk cards list](dvb-usb-gp8psk-cardlist.html)
  + [6.1.19. dvb-usb-m920x cards list](dvb-usb-m920x-cardlist.html)
  + [6.1.20. dvb-usb-nova-t-usb2 cards list](dvb-usb-nova-t-usb2-cardlist.html)
  + [6.1.21. dvb-usb-opera1 cards list](dvb-usb-opera1-cardlist.html)
  + [6.1.22. dvb-usb-pctv452e cards list](dvb-usb-pctv452e-cardlist.html)
  + [6.1.23. dvb-usb-technisat-usb2 cards list](dvb-usb-technisat-usb2-cardlist.html)
  + [6.1.24. dvb-usb-ttusb2 cards list](dvb-usb-ttusb2-cardlist.html)
  + [6.1.25. dvb-usb-umt-010 cards list](dvb-usb-umt-010-cardlist.html)
  + [6.1.26. dvb-usb-vp702x cards list](dvb-usb-vp702x-cardlist.html)
  + [6.1.27. dvb-usb-vp7045 cards list](dvb-usb-vp7045-cardlist.html)
  + [6.1.28. dvb-usb-af9015 cards list](dvb-usb-af9015-cardlist.html)
  + [6.1.29. dvb-usb-af9035 cards list](dvb-usb-af9035-cardlist.html)
  + [6.1.30. dvb-usb-anysee cards list](dvb-usb-anysee-cardlist.html)
  + [6.1.31. dvb-usb-au6610 cards list](dvb-usb-au6610-cardlist.html)
  + [6.1.32. dvb-usb-az6007 cards list](dvb-usb-az6007-cardlist.html)
  + [6.1.33. dvb-usb-ce6230 cards list](dvb-usb-ce6230-cardlist.html)
  + [6.1.34. dvb-usb-dvbsky cards list](dvb-usb-dvbsky-cardlist.html)
  + [6.1.35. dvb-usb-ec168 cards list](dvb-usb-ec168-cardlist.html)
  + [6.1.36. dvb-usb-gl861 cards list](dvb-usb-gl861-cardlist.html)
  + [6.1.37. dvb-usb-lmedm04 cards list](dvb-usb-lmedm04-cardlist.html)
  + [6.1.38. dvb-usb-mxl111sf cards list](dvb-usb-mxl111sf-cardlist.html)
  + [6.1.39. dvb-usb-rtl28xxu cards list](dvb-usb-rtl28xxu-cardlist.html)
  + [6.1.40. dvb-usb-zd1301 cards list](dvb-usb-zd1301-cardlist.html)
  + [6.1.41. Other USB cards list](other-usb-cardlist.html)
* [6.2. PCI drivers](pci-cardlist.html)
  + [6.2.1. BTTV cards list](bttv-cardlist.html)
  + [6.2.2. CX18 cards list](cx18-cardlist.html)
  + [6.2.3. cx23885 cards list](cx23885-cardlist.html)
  + [6.2.4. CX88 cards list](cx88-cardlist.html)
  + [6.2.5. IVTV cards list](ivtv-cardlist.html)
  + [6.2.6. SAA7134 cards list](saa7134-cardlist.html)
  + [6.2.7. SAA7164 cards list](saa7164-cardlist.html)
  + [6.2.8. Zoran cards list](zoran-cardlist.html)
* [6.3. Platform drivers](platform-cardlist.html)
  + [6.3.1. MMC/SDIO DVB adapters](platform-cardlist.html#mmc-sdio-dvb-adapters)
* [6.4. Radio drivers](radio-cardlist.html)
* [6.5. I²C drivers](i2c-cardlist.html)
  + [6.5.1. Audio decoders, processors and mixers](i2c-cardlist.html#audio-decoders-processors-and-mixers)
  + [6.5.2. Audio/Video compression chips](i2c-cardlist.html#audio-video-compression-chips)
  + [6.5.3. Camera sensor devices](i2c-cardlist.html#camera-sensor-devices)
  + [6.5.4. Flash devices](i2c-cardlist.html#flash-devices)
  + [6.5.5. IR I2C driver](i2c-cardlist.html#ir-i2c-driver)
  + [6.5.6. Lens drivers](i2c-cardlist.html#lens-drivers)
  + [6.5.7. Miscellaneous helper chips](i2c-cardlist.html#miscellaneous-helper-chips)
  + [6.5.8. RDS decoders](i2c-cardlist.html#rds-decoders)
  + [6.5.9. SDR tuner chips](i2c-cardlist.html#sdr-tuner-chips)
  + [6.5.10. Video and audio decoders](i2c-cardlist.html#video-and-audio-decoders)
  + [6.5.11. Video decoders](i2c-cardlist.html#video-decoders)
  + [6.5.12. Video encoders](i2c-cardlist.html#video-encoders)
  + [6.5.13. Video improvement chips](i2c-cardlist.html#video-improvement-chips)
  + [6.5.14. Tuner drivers](i2c-cardlist.html#tuner-drivers)
* [6.6. Firewire driver](misc-cardlist.html)
* [6.7. Test drivers](misc-cardlist.html#test-drivers)
