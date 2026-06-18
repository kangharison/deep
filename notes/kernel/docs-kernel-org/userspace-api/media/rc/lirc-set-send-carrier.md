# 6.5.12.ioctl LIRC_SET_SEND_CARRIER

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-send-carrier.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.12. ioctl LIRC\_SET\_SEND\_CARRIER

## 6.5.12.1. Name

LIRC\_SET\_SEND\_CARRIER - Set send carrier used to modulate IR TX.

## 6.5.12.2. Synopsis

LIRC\_SET\_SEND\_CARRIER

`int ioctl(int fd, LIRC_SET_SEND_CARRIER, __u32 *frequency)`

## 6.5.12.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`frequency`
:   Frequency of the carrier to be modulated, in Hz.

## 6.5.12.4. Description

Set send carrier used to modulate IR PWM pulses and spaces.

## 6.5.12.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
