# 6.1.2.4.FE_READ_UNCORRECTED_BLOCKS

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/fe-read-uncorrected-blocks.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.1.2.4. FE\_READ\_UNCORRECTED\_BLOCKS

## 6.1.2.4.1. Name

FE\_READ\_UNCORRECTED\_BLOCKS

Attention

This ioctl is deprecated.

## 6.1.2.4.2. Synopsis

FE\_READ\_UNCORRECTED\_BLOCKS

`int ioctl(int fd, FE_READ_UNCORRECTED_BLOCKS, uint32_t *ublocks)`

## 6.1.2.4.3. Arguments

`fd`
:   File descriptor returned by [`open()`](frontend_f_open.html#c.DTV.fe.open "open").

`ublocks`
:   The total number of uncorrected blocks seen by the driver so far.

## 6.1.2.4.4. Description

This ioctl call returns the number of uncorrected blocks detected by the
device driver during its lifetime. For meaningful measurements, the
increment in block count during a specific time interval should be
calculated. For this command, read-only access to the device is
sufficient.

## 6.1.2.4.5. Return Value

On success 0 is returned.

On error -1 is returned, and the `errno` variable is set
appropriately.

Generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
