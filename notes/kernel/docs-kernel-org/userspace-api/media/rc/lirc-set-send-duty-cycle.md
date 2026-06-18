# 6.5.7.ioctl LIRC_SET_SEND_DUTY_CYCLE

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-send-duty-cycle.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.7. ioctl LIRC\_SET\_SEND\_DUTY\_CYCLE

## 6.5.7.1. Name

LIRC\_SET\_SEND\_DUTY\_CYCLE - Set the duty cycle of the carrier signal for
IR transmit.

## 6.5.7.2. Synopsis

LIRC\_SET\_SEND\_DUTY\_CYCLE

`int ioctl(int fd, LIRC_SET_SEND_DUTY_CYCLE, __u32 *duty_cycle)`

## 6.5.7.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`duty_cycle`
:   Duty cycle, describing the pulse width in percent (from 1 to 99) of
    the total cycle. Values 0 and 100 are reserved.

## 6.5.7.4. Description

Get/set the duty cycle of the carrier signal for IR transmit.

Currently, no special meaning is defined for 0 or 100, but this
could be used to switch off carrier generation in the future, so
these values should be reserved.

## 6.5.7.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
