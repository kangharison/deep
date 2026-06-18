# 6.5.11.ioctl LIRC_SET_REC_CARRIER_RANGE

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-rec-carrier-range.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.11. ioctl LIRC\_SET\_REC\_CARRIER\_RANGE

## 6.5.11.1. Name

LIRC\_SET\_REC\_CARRIER\_RANGE - Set lower bound of the carrier used to modulate
IR receive.

## 6.5.11.2. Synopsis

LIRC\_SET\_REC\_CARRIER\_RANGE

`int ioctl(int fd, LIRC_SET_REC_CARRIER_RANGE, __u32 *frequency)`

## 6.5.11.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`frequency`
:   Frequency of the carrier that modulates PWM data, in Hz.

## 6.5.11.4. Description

This ioctl sets the upper range of carrier frequency that will be recognized
by the IR receiver.

Note

To set a range use [LIRC\_SET\_REC\_CARRIER\_RANGE](#lirc-set-rec-carrier-range) with the lower bound first and later call
[LIRC\_SET\_REC\_CARRIER](lirc-set-rec-carrier.html#lirc-set-rec-carrier) with the upper bound.

## 6.5.11.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
