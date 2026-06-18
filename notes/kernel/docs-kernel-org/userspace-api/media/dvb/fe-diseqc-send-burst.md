# 2.4.9.ioctl FE_DISEQC_SEND_BURST

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-diseqc-send-burst.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.9. ioctl FE\_DISEQC\_SEND\_BURST

## 2.4.9.1. Name

FE\_DISEQC\_SEND\_BURST - Sends a 22KHz tone burst for 2x1 mini DiSEqC satellite selection.

## 2.4.9.2. Synopsis

FE\_DISEQC\_SEND\_BURST

`int ioctl(int fd, FE_DISEQC_SEND_BURST, enum fe_sec_mini_cmd tone)`

## 2.4.9.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`tone`
:   An integer enumerated value described at [`fe_sec_mini_cmd`](frontend-header.html#c.fe_sec_mini_cmd "fe_sec_mini_cmd").

## 2.4.9.4. Description

This ioctl is used to set the generation of a 22kHz tone burst for mini
DiSEqC satellite selection for 2x1 switches. This call requires
read/write permissions.

It provides support for what’s specified at
[Digital Satellite Equipment Control (DiSEqC) - Simple “ToneBurst” Detection Circuit specification.](http://www.eutelsat.com/files/contributed/satellites/pdf/Diseqc/associated%20docs/simple_tone_burst_detec.pdf)

## 2.4.9.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
