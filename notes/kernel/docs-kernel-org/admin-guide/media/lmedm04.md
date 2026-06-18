# 8.3.Firmware files for lmedm04 cards

> 출처(원문): https://docs.kernel.org/admin-guide/media/lmedm04.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 8.3. Firmware files for lmedm04 cards

To extract firmware for the DM04/QQBOX you need to copy the
following file(s) to this directory.

## 8.3.1. For DM04+/QQBOX LME2510C (Sharp 7395 Tuner)

The Sharp 7395 driver can be found in windows/system32/drivers

US2A0D.sys (dated 17 Mar 2009)

and run:

```
scripts/get_dvb_firmware lme2510c_s7395
```

will produce dvb-usb-lme2510c-s7395.fw

An alternative but older firmware can be found on the driver
disk DVB-S\_EN\_3.5A in BDADriver/driver

LMEBDA\_DVBS7395C.sys (dated 18 Jan 2008)

and run:

```
./get_dvb_firmware lme2510c_s7395_old
```

will produce dvb-usb-lme2510c-s7395.fw

The LG firmware can be found on the driver
disk DM04+\_5.1A[LG] in BDADriver/driver

## 8.3.2. For DM04 LME2510 (LG Tuner)

LMEBDA\_DVBS.sys (dated 13 Nov 2007)

and run:

```
./get_dvb_firmware lme2510_lg
```

will produce dvb-usb-lme2510-lg.fw

Other LG firmware can be extracted manually from US280D.sys
only found in windows/system32/drivers

dd if=US280D.sys ibs=1 skip=42360 count=3924 of=dvb-usb-lme2510-lg.fw

## 8.3.3. For DM04 LME2510C (LG Tuner)

```
dd if=US280D.sys ibs=1 skip=35200 count=3850 of=dvb-usb-lme2510c-lg.fw
```

The Sharp 0194 tuner driver can be found in windows/system32/drivers

US290D.sys (dated 09 Apr 2009)

## 8.3.4. For LME2510

```
dd if=US290D.sys ibs=1 skip=36856 count=3976 of=dvb-usb-lme2510-s0194.fw
```

## 8.3.5. For LME2510C

```
dd if=US290D.sys ibs=1 skip=33152 count=3697 of=dvb-usb-lme2510c-s0194.fw
```

The m88rs2000 tuner driver can be found in windows/system32/drivers

US2B0D.sys (dated 29 Jun 2010)

```
dd if=US2B0D.sys ibs=1 skip=34432 count=3871 of=dvb-usb-lme2510c-rs2000.fw
```

We need to modify id of rs2000 firmware or it will warm boot id 3344:1120.

```
echo -ne \\xF0\\x22 | dd conv=notrunc bs=1 count=2 seek=266 of=dvb-usb-lme2510c-rs2000.fw
```

Copy the firmware file(s) to /lib/firmware
