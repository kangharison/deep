# 6.5.3.ioctl LIRC_GET_FEATURES

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-get-features.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.3. ioctl LIRC\_GET\_FEATURES

## 6.5.3.1. Name

LIRC\_GET\_FEATURES - Get the underlying hardware device’s features

## 6.5.3.2. Synopsis

LIRC\_GET\_FEATURES

`int ioctl(int fd, LIRC_GET_FEATURES, __u32 *features)`

## 6.5.3.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`features`
:   Bitmask with the LIRC features.

## 6.5.3.4. Description

Get the underlying hardware device’s features. If a driver does not
announce support of certain features, calling of the corresponding ioctls
is undefined.

## 6.5.3.5. LIRC features

`LIRC_CAN_REC_RAW`

> Unused. Kept just to avoid breaking uAPI.

`LIRC_CAN_REC_PULSE`

> Unused. Kept just to avoid breaking uAPI.
> [LIRC\_MODE\_PULSE](lirc-dev-intro.html#lirc-mode-pulse) can only be used for transmitting.

`LIRC_CAN_REC_MODE2`

> This is raw IR driver for receiving. This means that
> [LIRC\_MODE\_MODE2](lirc-dev-intro.html#lirc-mode-mode2) is used. This also implies
> that [LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode) is also supported,
> as long as the kernel is recent enough. Use the
> [ioctls LIRC\_GET\_REC\_MODE and LIRC\_SET\_REC\_MODE](lirc-get-rec-mode.html#lirc-set-rec-mode) to switch modes.

`LIRC_CAN_REC_LIRCCODE`

> Unused. Kept just to avoid breaking uAPI.

`LIRC_CAN_REC_SCANCODE`

> This is a scancode driver for receiving. This means that
> [LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode) is used.

`LIRC_CAN_SET_SEND_CARRIER`

> The driver supports changing the modulation frequency via
> [ioctl LIRC\_SET\_SEND\_CARRIER](lirc-set-send-carrier.html#lirc-set-send-carrier).

`LIRC_CAN_SET_SEND_DUTY_CYCLE`

> The driver supports changing the duty cycle using
> [ioctl LIRC\_SET\_SEND\_DUTY\_CYCLE](lirc-set-send-duty-cycle.html#lirc-set-send-duty-cycle).

`LIRC_CAN_SET_TRANSMITTER_MASK`

> The driver supports changing the active transmitter(s) using
> [ioctl LIRC\_SET\_TRANSMITTER\_MASK](lirc-set-transmitter-mask.html#lirc-set-transmitter-mask).

`LIRC_CAN_SET_REC_CARRIER`

> The driver supports setting the receive carrier frequency using
> [ioctl LIRC\_SET\_REC\_CARRIER](lirc-set-rec-carrier.html#lirc-set-rec-carrier).

`LIRC_CAN_SET_REC_CARRIER_RANGE`

> The driver supports
> [ioctl LIRC\_SET\_REC\_CARRIER\_RANGE](lirc-set-rec-carrier-range.html#lirc-set-rec-carrier-range).

`LIRC_CAN_GET_REC_RESOLUTION`

> The driver supports
> [ioctl LIRC\_GET\_REC\_RESOLUTION](lirc-get-rec-resolution.html#lirc-get-rec-resolution).

`LIRC_CAN_SET_REC_TIMEOUT`

> The driver supports
> [ioctl LIRC\_SET\_REC\_TIMEOUT](lirc-set-rec-timeout.html#lirc-set-rec-timeout).

`LIRC_CAN_MEASURE_CARRIER`

> The driver supports measuring of the modulation frequency using
> [ioctl LIRC\_SET\_MEASURE\_CARRIER\_MODE](lirc-set-measure-carrier-mode.html#lirc-set-measure-carrier-mode).

`LIRC_CAN_USE_WIDEBAND_RECEIVER`

> The driver supports learning mode using
> [ioctl LIRC\_SET\_WIDEBAND\_RECEIVER](lirc-set-wideband-receiver.html#lirc-set-wideband-receiver).

`LIRC_CAN_SEND_RAW`

> Unused. Kept just to avoid breaking uAPI.

`LIRC_CAN_SEND_PULSE`

> The driver supports sending (also called as IR blasting or IR TX) using
> [LIRC\_MODE\_PULSE](lirc-dev-intro.html#lirc-mode-pulse). This implies that
> [LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode) is also supported for
> transmit, as long as the kernel is recent enough. Use the
> [ioctls LIRC\_GET\_SEND\_MODE and LIRC\_SET\_SEND\_MODE](lirc-get-send-mode.html#lirc-set-send-mode) to switch modes.

`LIRC_CAN_SEND_MODE2`

> Unused. Kept just to avoid breaking uAPI.
> [LIRC\_MODE\_MODE2](lirc-dev-intro.html#lirc-mode-mode2) can only be used for receiving.

`LIRC_CAN_SEND_LIRCCODE`

> Unused. Kept just to avoid breaking uAPI.

## 6.5.3.6. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
