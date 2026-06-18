# 2.4.6.ioctl FE_DISEQC_RESET_OVERLOAD

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-diseqc-reset-overload.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.4.6. ioctl FE\_DISEQC\_RESET\_OVERLOAD

## 2.4.6.1. Name

FE\_DISEQC\_RESET\_OVERLOAD - Restores the power to the antenna subsystem, if it was powered off due - to power overload.

## 2.4.6.2. Synopsis

FE\_DISEQC\_RESET\_OVERLOAD

`int ioctl(int fd, FE_DISEQC_RESET_OVERLOAD, NULL)`

## 2.4.6.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

## 2.4.6.4. Description

If the bus has been automatically powered off due to power overload,
this ioctl call restores the power to the bus. The call requires
read/write access to the device. This call has no effect if the device
is manually powered off. Not all Digital TV adapters support this ioctl.

## 2.4.6.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
