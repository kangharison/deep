# 6.1.2.3.FE_READ_SIGNAL_STRENGTH

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-read-signal-strength.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.3. FE\_READ\_SIGNAL\_STRENGTH

## 6.1.2.3.1. Name

FE\_READ\_SIGNAL\_STRENGTH

Attention

This ioctl is deprecated.

## 6.1.2.3.2. Synopsis

FE\_READ\_SIGNAL\_STRENGTH

`int ioctl(int fd, FE_READ_SIGNAL_STRENGTH, uint16_t *strength)`

## 6.1.2.3.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`strength`
:   The signal strength value is stored into \*strength.

## 6.1.2.3.4. Description

This ioctl call returns the signal strength value for the signal
currently received by the front-end. For this command, read-only access
to the device is sufficient.

## 6.1.2.3.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
