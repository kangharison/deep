# 2.4.10.ioctl FE_SET_TONE

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-set-tone.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.10. ioctl FE\_SET\_TONE

## 2.4.10.1. Name

FE\_SET\_TONE - Sets/resets the generation of the continuous 22kHz tone.

## 2.4.10.2. Synopsis

FE\_SET\_TONE

`int ioctl(int fd, FE_SET_TONE, enum fe_sec_tone_mode tone)`

## 2.4.10.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`tone`
:   An integer enumerated value described at [`fe_sec_tone_mode`](frontend-header.html#c.fe_sec_tone_mode "fe_sec_tone_mode")

## 2.4.10.4. Description

This ioctl is used to set the generation of the continuous 22kHz tone.
This call requires read/write permissions.

Usually, satellite antenna subsystems require that the digital TV device
to send a 22kHz tone in order to select between high/low band on some
dual-band LNBf. It is also used to send signals to DiSEqC equipment, but
this is done using the DiSEqC ioctls.

Attention

If more than one device is connected to the same antenna,
setting a tone may interfere on other devices, as they may lose the
capability of selecting the band. So, it is recommended that applications
would change to SEC\_TONE\_OFF when the device is not used.

## 2.4.10.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
