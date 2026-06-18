# 6.1.USB drivers

> 출처(원문): https://docs.kernel.org/admin-guide/media/usb-cardlist.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1. USB drivers

The USB boards are identified by an identification called USB ID.

The `lsusb` command allows identifying the USB IDs:

```
$ lsusb
...
Bus 001 Device 015: ID 046d:082d Logitech, Inc. HD Pro Webcam C920
Bus 001 Device 074: ID 2040:b131 Hauppauge
Bus 001 Device 075: ID 2013:024f PCTV Systems nanoStick T2 290e
...
```

Newer camera devices use a standard way to expose themselves as such,
via USB Video Class. Those cameras are automatically supported by the
`uvc-driver`.

Older cameras and TV USB devices uses USB Vendor Classes: each vendor
defines its own way to access the device. This section contains
card lists for such vendor-class devices.

While this is not as common as on PCI, sometimes the same USB ID is used
by different products. So, several media drivers allow passing a `card=`
parameter, in order to setup a card number that would match the correct
settings for an specific product type.

The current supported USB cards (not including staging drivers) are
listed below[[1]](#id2).

[[1](#id1)]

some of the drivers have sub-drivers, not shown at this table.
In particular, gspca driver has lots of sub-drivers,
for cameras not supported by the USB Video Class (UVC) driver,
as shown at [gspca card list](gspca-cardlist.html).

| Driver | Name |
| --- | --- |
| airspy | AirSpy |
| au0828 | Auvitek AU0828 |
| b2c2-flexcop-usb | Technisat/B2C2 Air/Sky/Cable2PC USB |
| cx231xx | Conexant cx231xx USB video capture |
| dvb-as102 | Abilis AS102 DVB receiver |
| dvb-ttusb-budget | Technotrend/Hauppauge Nova - USB devices |
| dvb-usb-a800 | AVerMedia AverTV DVB-T USB 2.0 (A800) |
| dvb-usb-af9005 | Afatech AF9005 DVB-T USB1.1 |
| dvb-usb-af9015 | Afatech AF9015 DVB-T USB2.0 |
| dvb-usb-af9035 | Afatech AF9035 DVB-T USB2.0 |
| dvb-usb-anysee | Anysee DVB-T/C USB2.0 |
| dvb-usb-au6610 | Alcor Micro AU6610 USB2.0 |
| dvb-usb-az6007 | AzureWave 6007 and clones DVB-T/C USB2.0 |
| dvb-usb-az6027 | Azurewave DVB-S/S2 USB2.0 AZ6027 |
| dvb-usb-ce6230 | Intel CE6230 DVB-T USB2.0 |
| dvb-usb-cinergyT2 | Terratec CinergyT2/qanu USB 2.0 DVB-T |
| dvb-usb-cxusb | Conexant USB2.0 hybrid |
| dvb-usb-dib0700 | DiBcom DiB0700 |
| dvb-usb-dibusb-common | DiBcom DiB3000M-B |
| dvb-usb-dibusb-mc | DiBcom DiB3000M-C/P |
| dvb-usb-digitv | Nebula Electronics uDigiTV DVB-T USB2.0 |
| dvb-usb-dtt200u | WideView WT-200U and WT-220U (pen) DVB-T |
| dvb-usb-dtv5100 | AME DTV-5100 USB2.0 DVB-T |
| dvb-usb-dvbsky | DVBSky USB |
| dvb-usb-dw2102 | DvbWorld & TeVii DVB-S/S2 USB2.0 |
| dvb-usb-ec168 | E3C EC168 DVB-T USB2.0 |
| dvb-usb-gl861 | Genesys Logic GL861 USB2.0 |
| dvb-usb-gp8psk | GENPIX 8PSK->USB module |
| dvb-usb-lmedm04 | LME DM04/QQBOX DVB-S USB2.0 |
| dvb-usb-m920x | Uli m920x DVB-T USB2.0 |
| dvb-usb-nova-t-usb2 | Hauppauge WinTV-NOVA-T usb2 DVB-T USB2.0 |
| dvb-usb-opera | Opera1 DVB-S USB2.0 receiver |
| dvb-usb-pctv452e | Pinnacle PCTV HDTV Pro USB device/TT Connect S2-3600 |
| dvb-usb-rtl28xxu | Realtek RTL28xxU DVB USB |
| dvb-usb-technisat-usb2 | Technisat DVB-S/S2 USB2.0 |
| dvb-usb-ttusb2 | Pinnacle 400e DVB-S USB2.0 |
| dvb-usb-umt-010 | HanfTek UMT-010 DVB-T USB2.0 |
| dvb\_usb\_v2 | Support for various USB DVB devices v2 |
| dvb-usb-vp702x | TwinhanDTV StarBox and clones DVB-S USB2.0 |
| dvb-usb-vp7045 | TwinhanDTV Alpha/MagicBoxII, DNTV tinyUSB2, Beetle USB2.0 |
| em28xx | Empia EM28xx USB devices |
| go7007 | WIS GO7007 MPEG encoder |
| gspca | Drivers for several USB Cameras |
| hackrf | HackRF |
| hdpvr | Hauppauge HD PVR |
| msi2500 | Mirics MSi2500 |
| mxl111sf-tuner | MxL111SF DTV USB2.0 |
| pvrusb2 | Hauppauge WinTV-PVR USB2 |
| pwc | USB Philips Cameras |
| s2250 | Sensoray 2250/2251 |
| s2255drv | USB Sensoray 2255 video capture device |
| smsusb | Siano SMS1xxx based MDTV receiver |
| ttusb\_dec | Technotrend/Hauppauge USB DEC devices |
| usbtv | USBTV007 video capture |
| uvcvideo | USB Video Class (UVC) |
| zd1301 | ZyDAS ZD1301 |

* [6.1.1. AU0828 cards list](au0828-cardlist.html)
* [6.1.2. cx231xx cards list](cx231xx-cardlist.html)
* [6.1.3. EM28xx cards list](em28xx-cardlist.html)
* [6.1.4. Siano cards list](siano-cardlist.html)
* [6.1.5. The gspca cards list](gspca-cardlist.html)
* [6.1.6. dvb-usb-dib0700 cards list](dvb-usb-dib0700-cardlist.html)
* [6.1.7. dvb-usb-dibusb-mb cards list](dvb-usb-dibusb-mb-cardlist.html)
* [6.1.8. dvb-usb-dibusb-mc cards list](dvb-usb-dibusb-mc-cardlist.html)
* [6.1.9. dvb-usb-a800 cards list](dvb-usb-a800-cardlist.html)
* [6.1.10. dvb-usb-af9005 cards list](dvb-usb-af9005-cardlist.html)
* [6.1.11. dvb-usb-az6027 cards list](dvb-usb-az6027-cardlist.html)
* [6.1.12. dvb-usb-cinergyT2 cards list](dvb-usb-cinergyT2-cardlist.html)
* [6.1.13. dvb-usb-cxusb cards list](dvb-usb-cxusb-cardlist.html)
* [6.1.14. dvb-usb-digitv cards list](dvb-usb-digitv-cardlist.html)
* [6.1.15. dvb-usb-dtt200u cards list](dvb-usb-dtt200u-cardlist.html)
* [6.1.16. dvb-usb-dtv5100 cards list](dvb-usb-dtv5100-cardlist.html)
* [6.1.17. dvb-usb-dw2102 cards list](dvb-usb-dw2102-cardlist.html)
* [6.1.18. dvb-usb-gp8psk cards list](dvb-usb-gp8psk-cardlist.html)
* [6.1.19. dvb-usb-m920x cards list](dvb-usb-m920x-cardlist.html)
* [6.1.20. dvb-usb-nova-t-usb2 cards list](dvb-usb-nova-t-usb2-cardlist.html)
* [6.1.21. dvb-usb-opera1 cards list](dvb-usb-opera1-cardlist.html)
* [6.1.22. dvb-usb-pctv452e cards list](dvb-usb-pctv452e-cardlist.html)
* [6.1.23. dvb-usb-technisat-usb2 cards list](dvb-usb-technisat-usb2-cardlist.html)
* [6.1.24. dvb-usb-ttusb2 cards list](dvb-usb-ttusb2-cardlist.html)
* [6.1.25. dvb-usb-umt-010 cards list](dvb-usb-umt-010-cardlist.html)
* [6.1.26. dvb-usb-vp702x cards list](dvb-usb-vp702x-cardlist.html)
* [6.1.27. dvb-usb-vp7045 cards list](dvb-usb-vp7045-cardlist.html)
* [6.1.28. dvb-usb-af9015 cards list](dvb-usb-af9015-cardlist.html)
* [6.1.29. dvb-usb-af9035 cards list](dvb-usb-af9035-cardlist.html)
* [6.1.30. dvb-usb-anysee cards list](dvb-usb-anysee-cardlist.html)
* [6.1.31. dvb-usb-au6610 cards list](dvb-usb-au6610-cardlist.html)
* [6.1.32. dvb-usb-az6007 cards list](dvb-usb-az6007-cardlist.html)
* [6.1.33. dvb-usb-ce6230 cards list](dvb-usb-ce6230-cardlist.html)
* [6.1.34. dvb-usb-dvbsky cards list](dvb-usb-dvbsky-cardlist.html)
* [6.1.35. dvb-usb-ec168 cards list](dvb-usb-ec168-cardlist.html)
* [6.1.36. dvb-usb-gl861 cards list](dvb-usb-gl861-cardlist.html)
* [6.1.37. dvb-usb-lmedm04 cards list](dvb-usb-lmedm04-cardlist.html)
* [6.1.38. dvb-usb-mxl111sf cards list](dvb-usb-mxl111sf-cardlist.html)
* [6.1.39. dvb-usb-rtl28xxu cards list](dvb-usb-rtl28xxu-cardlist.html)
* [6.1.40. dvb-usb-zd1301 cards list](dvb-usb-zd1301-cardlist.html)
* [6.1.41. Other USB cards list](other-usb-cardlist.html)
