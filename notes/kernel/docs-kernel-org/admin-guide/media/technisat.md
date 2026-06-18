# 8.5.How to set up the Technisat/B2C2 Flexcop devices

> 출처(원문): https://docs.kernel.org/admin-guide/media/technisat.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 8.5. How to set up the Technisat/B2C2 Flexcop devices

Note

This documentation is outdated.

Author: Uwe Bugla <[uwe.bugla@gmx.de](mailto:uwe.bugla%40gmx.de)> August 2009

## 8.5.1. Find out what device you have

Important Notice: The driver does NOT support Technisat USB 2 devices!

First start your linux box with a shipped kernel:

```
lspci -vvv for a PCI device (lsusb -vvv for an USB device) will show you for example:
02:0b.0 Network controller: Techsan Electronics Co Ltd B2C2 FlexCopII DVB chip /
Technisat SkyStar2 DVB card (rev 02)

dmesg | grep frontend may show you for example:
DVB: registering frontend 0 (Conexant CX24123/CX24109)...
```

## 8.5.2. Kernel compilation:

If the Flexcop / Technisat is the only DVB / TV / Radio device in your box
get rid of unnecessary modules and check this one:

`Multimedia support` => `Customise analog and hybrid tuner modules to build`

In this directory uncheck every driver which is activated there
(except `Simple tuner support` for ATSC 3rd generation only -> see case 9 please).

Then please activate:

* Main module part:

  `Multimedia support` => `DVB/ATSC adapters` => `Technisat/B2C2 FlexcopII(b) and FlexCopIII adapters`

  1. => `Technisat/B2C2 Air/Sky/Cable2PC PCI` (PCI card) or
  2. => `Technisat/B2C2 Air/Sky/Cable2PC USB` (USB 1.1 adapter)
     and for troubleshooting purposes:
  3. => `Enable debug for the B2C2 FlexCop drivers`
* Frontend / Tuner / Demodulator module part:

  `Multimedia support` => `DVB/ATSC adapters`
  :   => `Customise the frontend modules to build` `Customise DVB frontends` =>

  + SkyStar DVB-S Revision 2.3:

    1. => `Zarlink VP310/MT312/ZL10313 based`
    2. => `Generic I2C PLL based tuners`
  + SkyStar DVB-S Revision 2.6:

    1. => `ST STV0299 based`
    2. => `Generic I2C PLL based tuners`
  + SkyStar DVB-S Revision 2.7:

    1. => `Samsung S5H1420 based`
    2. => `Integrant ITD1000 Zero IF tuner for DVB-S/DSS`
    3. => `ISL6421 SEC controller`
  + SkyStar DVB-S Revision 2.8:

    1. => `Conexant CX24123 based`
    2. => `Conexant CX24113/CX24128 tuner for DVB-S/DSS`
    3. => `ISL6421 SEC controller`
  + AirStar DVB-T card:

    1. => `Zarlink MT352 based`
    2. => `Generic I2C PLL based tuners`
  + CableStar DVB-C card:

    1. => `ST STV0297 based`
    2. => `Generic I2C PLL based tuners`
  + AirStar ATSC card 1st generation:

    1. => `Broadcom BCM3510`
  + AirStar ATSC card 2nd generation:

    1. => `NxtWave Communications NXT2002/NXT2004 based`
    2. => `Generic I2C PLL based tuners`
  + AirStar ATSC card 3rd generation:

    1. => `LG Electronics LGDT3302/LGDT3303 based`
    2. `Multimedia support` => `Customise analog and hybrid tuner modules to build` => `Simple tuner support`
