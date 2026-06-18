# 6.5.5.ioctls LIRC_GET_REC_MODE and LIRC_SET_REC_MODE

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-get-rec-mode.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.5. ioctls LIRC\_GET\_REC\_MODE and LIRC\_SET\_REC\_MODE

## 6.5.5.1. Name

LIRC\_GET\_REC\_MODE/LIRC\_SET\_REC\_MODE - Get/set current receive mode.

## 6.5.5.2. Synopsis

LIRC\_GET\_REC\_MODE

`int ioctl(int fd, LIRC_GET_REC_MODE, __u32 *mode)`

LIRC\_SET\_REC\_MODE

`int ioctl(int fd, LIRC_SET_REC_MODE, __u32 *mode)`

## 6.5.5.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`mode`
:   Mode used for receive.

## 6.5.5.4. Description

Get and set the current receive mode. Only
[LIRC\_MODE\_MODE2](lirc-dev-intro.html#lirc-mode-mode2) and
[LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode) are supported.
Use [ioctl LIRC\_GET\_FEATURES](lirc-get-features.html#lirc-get-features) to find out which modes the driver supports.

## 6.5.5.5. Return Value

|  |  |
| --- | --- |
| `ENODEV` | Device not available. |
| `ENOTTY` | Device does not support receiving. |
| `EINVAL` | Invalid mode or invalid mode for this device. |
