# 6.1.2.2.FE_READ_SNR

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-read-snr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.2. FE\_READ\_SNR

## 6.1.2.2.1. Name

FE\_READ\_SNR

Attention

This ioctl is deprecated.

## 6.1.2.2.2. Synopsis

FE\_READ\_SNR

`int ioctl(int fd, FE_READ_SNR, int16_t *snr)`

## 6.1.2.2.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`snr`
:   The signal-to-noise ratio is stored into \*snr.

## 6.1.2.2.4. Description

This ioctl call returns the signal-to-noise ratio for the signal
currently received by the front-end. For this command, read-only access
to the device is sufficient.

## 6.1.2.2.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
