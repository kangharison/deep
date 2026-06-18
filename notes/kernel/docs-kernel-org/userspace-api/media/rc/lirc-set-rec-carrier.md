# 6.5.10.ioctl LIRC_SET_REC_CARRIER

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-rec-carrier.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.10. ioctl LIRC\_SET\_REC\_CARRIER

## 6.5.10.1. Name

LIRC\_SET\_REC\_CARRIER - Set carrier used to modulate IR receive.

## 6.5.10.2. Synopsis

LIRC\_SET\_REC\_CARRIER

`int ioctl(int fd, LIRC_SET_REC_CARRIER, __u32 *frequency)`

## 6.5.10.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`frequency`
:   Frequency of the carrier that modulates PWM data, in Hz.

## 6.5.10.4. Description

Set receive carrier used to modulate IR PWM pulses and spaces.

Note

If called together with [ioctl LIRC\_SET\_REC\_CARRIER\_RANGE](lirc-set-rec-carrier-range.html#lirc-set-rec-carrier-range), this ioctl
sets the upper bound frequency that will be recognized by the device.

## 6.5.10.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
