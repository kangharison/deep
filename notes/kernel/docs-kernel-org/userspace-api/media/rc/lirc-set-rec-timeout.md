# 6.5.9.ioctl LIRC_GET_REC_TIMEOUT and LIRC_SET_REC_TIMEOUT

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-rec-timeout.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.9. ioctl LIRC\_GET\_REC\_TIMEOUT and LIRC\_SET\_REC\_TIMEOUT

## 6.5.9.1. Name

LIRC\_GET\_REC\_TIMEOUT/LIRC\_SET\_REC\_TIMEOUT - Get/set the integer value for IR inactivity timeout.

## 6.5.9.2. Synopsis

LIRC\_GET\_REC\_TIMEOUT

`int ioctl(int fd, LIRC_GET_REC_TIMEOUT, __u32 *timeout)`

LIRC\_SET\_REC\_TIMEOUT

`int ioctl(int fd, LIRC_SET_REC_TIMEOUT, __u32 *timeout)`

## 6.5.9.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`timeout`
:   Timeout, in microseconds.

## 6.5.9.4. Description

Get and set the integer value for IR inactivity timeout.

If supported by the hardware, setting it to 0 disables all hardware timeouts
and data should be reported as soon as possible. If the exact value
cannot be set, then the next possible value \_greater\_ than the
given value should be set.

Note

The range of supported timeout is given by [ioctls LIRC\_GET\_MIN\_TIMEOUT and LIRC\_GET\_MAX\_TIMEOUT](lirc-get-timeout.html#lirc-get-min-timeout).

## 6.5.9.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
