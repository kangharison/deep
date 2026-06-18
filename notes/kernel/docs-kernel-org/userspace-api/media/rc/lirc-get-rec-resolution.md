# 6.5.6.ioctl LIRC_GET_REC_RESOLUTION

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-get-rec-resolution.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.6. ioctl LIRC\_GET\_REC\_RESOLUTION

## 6.5.6.1. Name

LIRC\_GET\_REC\_RESOLUTION - Obtain the value of receive resolution, in microseconds.

## 6.5.6.2. Synopsis

LIRC\_GET\_REC\_RESOLUTION

`int ioctl(int fd, LIRC_GET_REC_RESOLUTION, __u32 *microseconds)`

## 6.5.6.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`microseconds`
:   Resolution, in microseconds.

## 6.5.6.4. Description

Some receivers have maximum resolution which is defined by internal
sample rate or data format limitations. E.g. it’s common that
signals can only be reported in 50 microsecond steps.

This ioctl returns the integer value with such resolution, with can be
used by userspace applications like lircd to automatically adjust the
tolerance value.

## 6.5.6.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
