# 6.5.15.ioctl LIRC_SET_WIDEBAND_RECEIVER

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-set-wideband-receiver.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.15. ioctl LIRC\_SET\_WIDEBAND\_RECEIVER

## 6.5.15.1. Name

LIRC\_SET\_WIDEBAND\_RECEIVER - enable wide band receiver.

## 6.5.15.2. Synopsis

LIRC\_SET\_WIDEBAND\_RECEIVER

`int ioctl(int fd, LIRC_SET_WIDEBAND_RECEIVER, __u32 *enable)`

## 6.5.15.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`enable`
:   enable = 1 means enable wideband receiver, enable = 0 means disable
    wideband receiver.

## 6.5.15.4. Description

Some receivers are equipped with special wide band receiver which is
intended to be used to learn output of existing remote. This ioctl
allows enabling or disabling it.

This might be useful of receivers that have otherwise narrow band receiver
that prevents them to be used with some remotes. Wide band receiver might
also be more precise. On the other hand its disadvantage it usually
reduced range of reception.

Note

Wide band receiver might be implicitly enabled if you enable
carrier reports. In that case it will be disabled as soon as you disable
carrier reports. Trying to disable wide band receiver while carrier
reports are active will do nothing.

## 6.5.15.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
