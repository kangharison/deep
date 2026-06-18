# 6.5.13.ioctl LIRC_SET_TRANSMITTER_MASK

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-transmitter-mask.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.13. ioctl LIRC\_SET\_TRANSMITTER\_MASK

## 6.5.13.1. Name

LIRC\_SET\_TRANSMITTER\_MASK - Enables send codes on a given set of transmitters

## 6.5.13.2. Synopsis

LIRC\_SET\_TRANSMITTER\_MASK

`int ioctl(int fd, LIRC_SET_TRANSMITTER_MASK, __u32 *mask)`

## 6.5.13.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`mask`
:   Mask with channels to enable tx. Channel 0 is the least significant bit.

## 6.5.13.4. Description

Some IR TX devices have multiple output channels, in such case,
[LIRC\_CAN\_SET\_TRANSMITTER\_MASK](lirc-get-features.html#lirc-can-set-transmitter-mask) is
returned via [ioctl LIRC\_GET\_FEATURES](lirc-get-features.html#lirc-get-features) and this ioctl sets what channels will
send IR codes.

This ioctl enables the given set of transmitters. The first transmitter is
encoded by the least significant bit and so on.

When an invalid bit mask is given, i.e. a bit is set, even though the device
does not have so many transitters, then this ioctl returns the number of
available transitters and does nothing otherwise.

## 6.5.13.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
