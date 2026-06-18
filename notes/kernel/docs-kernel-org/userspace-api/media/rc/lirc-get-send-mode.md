# 6.5.4.ioctls LIRC_GET_SEND_MODE and LIRC_SET_SEND_MODE

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-get-send-mode.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.4. ioctls LIRC\_GET\_SEND\_MODE and LIRC\_SET\_SEND\_MODE

## 6.5.4.1. Name

LIRC\_GET\_SEND\_MODE/LIRC\_SET\_SEND\_MODE - Get/set current transmit mode.

## 6.5.4.2. Synopsis

LIRC\_GET\_SEND\_MODE

`int ioctl(int fd, LIRC_GET_SEND_MODE, __u32 *mode)`

LIRC\_SET\_SEND\_MODE

`int ioctl(int fd, LIRC_SET_SEND_MODE, __u32 *mode)`

## 6.5.4.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`mode`
:   The mode used for transmitting.

## 6.5.4.4. Description

Get/set current transmit mode.

Only [LIRC\_MODE\_PULSE](lirc-dev-intro.html#lirc-mode-pulse) and
[LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode) are supported by for IR send,
depending on the driver. Use [ioctl LIRC\_GET\_FEATURES](lirc-get-features.html#lirc-get-features) to find out which
modes the driver supports.

## 6.5.4.5. Return Value

|  |  |
| --- | --- |
| `ENODEV` | Device not available. |
| `ENOTTY` | Device does not support transmitting. |
| `EINVAL` | Invalid mode or invalid mode for this device. |
