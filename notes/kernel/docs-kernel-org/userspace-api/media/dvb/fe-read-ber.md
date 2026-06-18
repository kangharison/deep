# 6.1.2.1.FE_READ_BER

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-read-ber.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.1. FE\_READ\_BER

## 6.1.2.1.1. Name

FE\_READ\_BER

Attention

This ioctl is deprecated.

## 6.1.2.1.2. Synopsis

FE\_READ\_BER

`int ioctl(int fd, FE_READ_BER, uint32_t *ber)`

## 6.1.2.1.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`ber`
:   The bit error rate is stored into \*ber.

## 6.1.2.1.4. Description

This ioctl call returns the bit error rate for the signal currently
received/demodulated by the front-end. For this command, read-only
access to the device is sufficient.

## 6.1.2.1.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
