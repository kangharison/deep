# 6.5.8.ioctls LIRC_GET_MIN_TIMEOUT and LIRC_GET_MAX_TIMEOUT

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-get-timeout.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.8. ioctls LIRC\_GET\_MIN\_TIMEOUT and LIRC\_GET\_MAX\_TIMEOUT

## 6.5.8.1. Name

LIRC\_GET\_MIN\_TIMEOUT / LIRC\_GET\_MAX\_TIMEOUT - Obtain the possible timeout
range for IR receive.

## 6.5.8.2. Synopsis

LIRC\_GET\_MIN\_TIMEOUT

`int ioctl(int fd, LIRC_GET_MIN_TIMEOUT, __u32 *timeout)`

LIRC\_GET\_MAX\_TIMEOUT

`int ioctl(int fd, LIRC_GET_MAX_TIMEOUT, __u32 *timeout)`

## 6.5.8.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`timeout`
:   Timeout, in microseconds.

## 6.5.8.4. Description

Some devices have internal timers that can be used to detect when
there’s no IR activity for a long time. This can help lircd in
detecting that a IR signal is finished and can speed up the decoding
process. Returns an integer value with the minimum/maximum timeout
that can be set.

Note

Some devices have a fixed timeout, in that case
both ioctls will return the same value even though the timeout
cannot be changed via [ioctl LIRC\_GET\_REC\_TIMEOUT and LIRC\_SET\_REC\_TIMEOUT](lirc-set-rec-timeout.html#lirc-set-rec-timeout).

## 6.5.8.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
