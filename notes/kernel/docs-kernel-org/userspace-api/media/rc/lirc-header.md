# 6.6.LIRC uAPI symbols

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-header.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.6. LIRC uAPI symbols

## [6.6.1. Enumerations](#id1)

* [`rc_proto`](lirc-dev-intro.html#c.rc_proto "rc_proto"): include/uapi/linux/lirc.h#205

## [6.6.2. IOCTL Commands](#id2)

* [LIRC\_GET\_FEATURES](lirc-get-features.html#lirc-get-features): include/uapi/linux/lirc.h#96
* [LIRC\_GET\_MAX\_TIMEOUT](lirc-get-timeout.html#lirc-get-max-timeout): include/uapi/linux/lirc.h#103
* [LIRC\_GET\_MIN\_TIMEOUT](lirc-get-timeout.html#lirc-get-min-timeout): include/uapi/linux/lirc.h#102
* [LIRC\_GET\_REC\_MODE](lirc-get-rec-mode.html#lirc-get-rec-mode): include/uapi/linux/lirc.h#99
* [LIRC\_GET\_REC\_RESOLUTION](lirc-get-rec-resolution.html#lirc-get-rec-resolution): include/uapi/linux/lirc.h#100
* [LIRC\_GET\_REC\_TIMEOUT](lirc-set-rec-timeout.html#lirc-get-rec-timeout): include/uapi/linux/lirc.h#144
* [LIRC\_GET\_SEND\_MODE](lirc-get-send-mode.html#lirc-get-send-mode): include/uapi/linux/lirc.h#98
* [LIRC\_SET\_MEASURE\_CARRIER\_MODE](lirc-set-measure-carrier-mode.html#lirc-set-measure-carrier-mode): include/uapi/linux/lirc.h#130
* [LIRC\_SET\_REC\_CARRIER](lirc-set-rec-carrier.html#lirc-set-rec-carrier): include/uapi/linux/lirc.h#112
* [LIRC\_SET\_REC\_CARRIER\_RANGE](lirc-set-rec-carrier-range.html#lirc-set-rec-carrier-range): include/uapi/linux/lirc.h#136
* [LIRC\_SET\_REC\_MODE](lirc-get-rec-mode.html#lirc-set-rec-mode): include/uapi/linux/lirc.h#109
* [LIRC\_SET\_REC\_TIMEOUT](lirc-set-rec-timeout.html#lirc-set-rec-timeout): include/uapi/linux/lirc.h#121
* [LIRC\_SET\_SEND\_CARRIER](lirc-set-send-carrier.html#lirc-set-send-carrier): include/uapi/linux/lirc.h#111
* [LIRC\_SET\_SEND\_DUTY\_CYCLE](lirc-set-send-duty-cycle.html#lirc-set-send-duty-cycle): include/uapi/linux/lirc.h#113
* [LIRC\_SET\_SEND\_MODE](lirc-get-send-mode.html#lirc-set-send-mode): include/uapi/linux/lirc.h#108
* [LIRC\_SET\_TRANSMITTER\_MASK](lirc-set-transmitter-mask.html#lirc-set-transmitter-mask): include/uapi/linux/lirc.h#114
* [LIRC\_SET\_WIDEBAND\_RECEIVER](lirc-set-wideband-receiver.html#lirc-set-wideband-receiver): include/uapi/linux/lirc.h#138

## [6.6.3. Macros and Definitions](#id3)

* [LIRC\_CAN\_GET\_REC\_RESOLUTION](lirc-get-features.html#lirc-can-get-rec-resolution): include/uapi/linux/lirc.h#78
* [LIRC\_CAN\_MEASURE\_CARRIER](lirc-get-features.html#lirc-can-measure-carrier): include/uapi/linux/lirc.h#81
* [LIRC\_CAN\_REC\_LIRCCODE](lirc-get-features.html#lirc-can-rec-lirccode): include/uapi/linux/lirc.h#71
* [LIRC\_CAN\_REC\_MODE2](lirc-get-features.html#lirc-can-rec-mode2): include/uapi/linux/lirc.h#69
* [LIRC\_CAN\_REC\_PULSE](lirc-get-features.html#lirc-can-rec-pulse): include/uapi/linux/lirc.h#68
* [LIRC\_CAN\_REC\_RAW](lirc-get-features.html#lirc-can-rec-raw): include/uapi/linux/lirc.h#67
* [LIRC\_CAN\_REC\_SCANCODE](lirc-get-features.html#lirc-can-rec-scancode): include/uapi/linux/lirc.h#70
* [LIRC\_CAN\_SEND\_LIRCCODE](lirc-get-features.html#lirc-can-send-lirccode): include/uapi/linux/lirc.h#59
* [LIRC\_CAN\_SEND\_MODE2](lirc-get-features.html#lirc-can-send-mode2): include/uapi/linux/lirc.h#58
* [LIRC\_CAN\_SEND\_PULSE](lirc-get-features.html#lirc-can-send-pulse): include/uapi/linux/lirc.h#57
* [LIRC\_CAN\_SEND\_RAW](lirc-get-features.html#lirc-can-send-raw): include/uapi/linux/lirc.h#56
* [LIRC\_CAN\_SET\_REC\_CARRIER](lirc-get-features.html#lirc-can-set-rec-carrier): include/uapi/linux/lirc.h#75
* [LIRC\_CAN\_SET\_REC\_CARRIER\_RANGE](lirc-get-features.html#lirc-can-set-rec-carrier-range): include/uapi/linux/lirc.h#77
* [LIRC\_CAN\_SET\_REC\_TIMEOUT](lirc-get-features.html#lirc-can-set-rec-timeout): include/uapi/linux/lirc.h#79
* [LIRC\_CAN\_SET\_SEND\_CARRIER](lirc-get-features.html#lirc-can-set-send-carrier): include/uapi/linux/lirc.h#63
* [LIRC\_CAN\_SET\_SEND\_DUTY\_CYCLE](lirc-get-features.html#lirc-can-set-send-duty-cycle): include/uapi/linux/lirc.h#64
* [LIRC\_CAN\_SET\_TRANSMITTER\_MASK](lirc-get-features.html#lirc-can-set-transmitter-mask): include/uapi/linux/lirc.h#65
* [LIRC\_CAN\_USE\_WIDEBAND\_RECEIVER](lirc-get-features.html#lirc-can-use-wideband-receiver): include/uapi/linux/lirc.h#82
* [LIRC\_MODE2\_FREQUENCY](lirc-set-measure-carrier-mode.html#lirc-mode2-frequency): include/uapi/linux/lirc.h#17
* [LIRC\_MODE\_MODE2](lirc-dev-intro.html#lirc-mode-mode2): include/uapi/linux/lirc.h#51
* [LIRC\_MODE\_PULSE](lirc-dev-intro.html#lirc-mode-pulse): include/uapi/linux/lirc.h#50
* [LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode): include/uapi/linux/lirc.h#52
* [LIRC\_SCANCODE\_FLAG\_REPEAT](lirc-dev-intro.html#lirc-scancode-flag-repeat): include/uapi/linux/lirc.h#170
* [LIRC\_SCANCODE\_FLAG\_TOGGLE](lirc-dev-intro.html#lirc-scancode-flag-toggle): include/uapi/linux/lirc.h#168

## [6.6.4. Structures](#id4)

* [`lirc_scancode`](lirc-dev-intro.html#c.lirc_scancode "lirc_scancode"): include/uapi/linux/lirc.h#159
