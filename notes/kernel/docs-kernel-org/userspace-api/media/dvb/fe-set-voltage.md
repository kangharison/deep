# 2.4.11.ioctl FE_SET_VOLTAGE

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-set-voltage.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.11. ioctl FE\_SET\_VOLTAGE

## 2.4.11.1. Name

FE\_SET\_VOLTAGE - Allow setting the DC level sent to the antenna subsystem.

## 2.4.11.2. Synopsis

FE\_SET\_VOLTAGE

`int ioctl(int fd, FE_SET_VOLTAGE, enum fe_sec_voltage voltage)`

## 2.4.11.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`voltage`
:   An integer enumerated value described at [`fe_sec_voltage`](frontend-header.html#c.fe_sec_voltage "fe_sec_voltage")

## 2.4.11.4. Description

This ioctl allows to set the DC voltage level sent through the antenna
cable to 13V, 18V or off.

Usually, a satellite antenna subsystems require that the digital TV
device to send a DC voltage to feed power to the LNBf. Depending on the
LNBf type, the polarization or the intermediate frequency (IF) of the
LNBf can controlled by the voltage level. Other devices (for example,
the ones that implement DISEqC and multipoint LNBf’s don’t need to
control the voltage level, provided that either 13V or 18V is sent to
power up the LNBf.

Attention

if more than one device is connected to the same antenna,
setting a voltage level may interfere on other devices, as they may lose
the capability of setting polarization or IF. So, on those cases, setting
the voltage to SEC\_VOLTAGE\_OFF while the device is not is used is
recommended.

## 2.4.11.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
