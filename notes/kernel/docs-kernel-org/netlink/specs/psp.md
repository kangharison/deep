# Familypspnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/psp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `psp` netlink specification](#id2)

## [Summary](#id3)

PSP Security Protocol Generic Netlink family.

## [Operations](#id4)

### [dev-get](#id5)

Get / dump information about PSP capable devices on the system.

attribute-set:
:   [dev](#psp-attribute-set-dev)

do:
:   **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`, `ifindex`, `psp-versions-cap`, `psp-versions-ena`]

    **pre**
    :   psp-device-get-locked

    **post**
    :   psp-device-unlock

dump:
:   **reply**
    :   attributes:
        :   [`id`, `ifindex`, `psp-versions-cap`, `psp-versions-ena`]

### [dev-add-ntf](#id6)

Notification about device appearing.

notify:
:   dev-get

mcgrp:
:   mgmt

### [dev-del-ntf](#id7)

Notification about device disappearing.

notify:
:   dev-get

mcgrp:
:   mgmt

### [dev-set](#id8)

Set the configuration of a PSP device.

attribute-set:
:   [dev](#psp-attribute-set-dev)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`id`, `psp-versions-ena`]

    **reply**
    :   attributes:
        :   []

    **pre**
    :   psp-device-get-locked

    **post**
    :   psp-device-unlock

### [dev-change-ntf](#id9)

Notification about device configuration being changed.

notify:
:   dev-get

mcgrp:
:   mgmt

### [key-rotate](#id10)

Rotate the device key.

attribute-set:
:   [dev](#psp-attribute-set-dev)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`]

    **pre**
    :   psp-device-get-locked

    **post**
    :   psp-device-unlock

### [key-rotate-ntf](#id11)

Notification about device key getting rotated.

notify:
:   key-rotate

mcgrp:
:   use

### [rx-assoc](#id12)

Allocate a new Rx key + SPI pair, associate it with a socket.

attribute-set:
:   [assoc](#psp-attribute-set-assoc)

do:
:   **request**
    :   attributes:
        :   [`dev-id`, `version`, `sock-fd`]

    **reply**
    :   attributes:
        :   [`dev-id`, `rx-key`]

    **pre**
    :   psp-assoc-device-get-locked

    **post**
    :   psp-device-unlock

### [tx-assoc](#id13)

Add a PSP Tx association.

attribute-set:
:   [assoc](#psp-attribute-set-assoc)

do:
:   **request**
    :   attributes:
        :   [`dev-id`, `version`, `tx-key`, `sock-fd`]

    **reply**
    :   attributes:
        :   []

    **pre**
    :   psp-assoc-device-get-locked

    **post**
    :   psp-device-unlock

### [get-stats](#id14)

Get device statistics.

attribute-set:
:   [stats](#psp-attribute-set-stats)

do:
:   **request**
    :   attributes:
        :   [`dev-id`]

    **reply**
    :   attributes:
        :   [`dev-id`, `key-rotations`, `stale-events`, `rx-packets`, `rx-bytes`, `rx-auth-fail`, `rx-error`, `rx-bad`, `tx-packets`, `tx-bytes`, `tx-error`]

    **pre**
    :   psp-device-get-locked

    **post**
    :   psp-device-unlock

dump:
:   **reply**
    :   attributes:
        :   [`dev-id`, `key-rotations`, `stale-events`, `rx-packets`, `rx-bytes`, `rx-auth-fail`, `rx-error`, `rx-bad`, `tx-packets`, `tx-bytes`, `tx-error`]

## [Multicast groups](#id15)

* mgmt
* use

## [Definitions](#id16)

### [version](#id17)

type:
:   enum

entries:
:   * `hdr0-aes-gcm-128`
    * `hdr0-aes-gcm-256`
    * `hdr0-aes-gmac-128`
    * `hdr0-aes-gmac-256`

## [Attribute sets](#id18)

### [dev](#id19)

#### id (`u32`)

doc:
:   PSP device ID.

#### ifindex (`u32`)

doc:
:   ifindex of the main netdevice linked to the PSP device.

#### psp-versions-cap (`u32`)

doc:
:   Bitmask of PSP versions supported by the device.

enum:
:   [version](#psp-definition-version)

enum-as-flags:
:   True

#### psp-versions-ena (`u32`)

doc:
:   Bitmask of currently enabled (accepted on Rx) PSP versions.

enum:
:   [version](#psp-definition-version)

enum-as-flags:
:   True

### [assoc](#id20)

#### dev-id (`u32`)

doc:
:   PSP device ID.

#### version (`u32`)

doc:
:   PSP versions (AEAD and protocol version) used by this association, dictates the size of the key.

enum:
:   [version](#psp-definition-version)

#### rx-key (`nest`)

nested-attributes:
:   [keys](#psp-attribute-set-keys)

#### tx-key (`nest`)

nested-attributes:
:   [keys](#psp-attribute-set-keys)

#### sock-fd (`u32`)

doc:
:   Sockets which should be bound to the association immediately.

### [keys](#id21)

#### key (`binary`)

#### spi (`u32`)

doc:
:   Security Parameters Index (SPI) of the association.

### [stats](#id22)

#### dev-id (`u32`)

doc:
:   PSP device ID.

#### key-rotations (`uint`)

doc:
:   Number of key rotations during the lifetime of the device. Kernel statistic.

#### stale-events (`uint`)

doc:
:   Number of times a socket’s Rx got shut down due to using a key which went stale (fully rotated out). Kernel statistic.

#### rx-packets (`uint`)

doc:
:   Number of successfully processed and authenticated PSP packets. Device statistic (from the PSP spec).

#### rx-bytes (`uint`)

doc:
:   Number of successfully authenticated PSP bytes received, counting from the first byte after the IV through the last byte of payload. The fixed initial portion of the PSP header (16 bytes) and the PSP trailer/ICV (16 bytes) are not included in this count. Device statistic (from the PSP spec).

#### rx-auth-fail (`uint`)

doc:
:   Number of received PSP packets with unsuccessful authentication. Device statistic (from the PSP spec).

#### rx-error (`uint`)

doc:
:   Number of received PSP packets with length/framing errors. Device statistic (from the PSP spec).

#### rx-bad (`uint`)

doc:
:   Number of received PSP packets with miscellaneous errors (invalid master key indicated by SPI, unsupported version, etc.) Device statistic (from the PSP spec).

#### tx-packets (`uint`)

doc:
:   Number of successfully processed PSP packets for transmission. Device statistic (from the PSP spec).

#### tx-bytes (`uint`)

doc:
:   Number of successfully processed PSP bytes for transmit, counting from the first byte after the IV through the last byte of payload. The fixed initial portion of the PSP header (16 bytes) and the PSP trailer/ICV (16 bytes) are not included in this count. Device statistic (from the PSP spec).

#### tx-error (`uint`)

doc:
:   Number of PSP packets for transmission with errors. Device statistic (from the PSP spec).
