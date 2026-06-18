# Familydpllnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/dpll.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `dpll` netlink specification](#id9)

## [Summary](#id10)

DPLL subsystem.

## [Operations](#id11)

### [device-id-get](#id12)

Get id of dpll device that matches given attributes

attribute-set:
:   [dpll](#dpll-attribute-set-dpll)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   dpll-lock-doit

    **post**
    :   dpll-unlock-doit

    **request**
    :   attributes:
        :   [`module-name`, `clock-id`, `type`]

    **reply**
    :   attributes:
        :   [`id`]

### [device-get](#id13)

Get list of DPLL devices (dump) or attributes of a single dpll device

attribute-set:
:   [dpll](#dpll-attribute-set-dpll)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   dpll-pre-doit

    **post**
    :   dpll-post-doit

    **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`, `module-name`, `mode`, `mode-supported`, `lock-status`, `lock-status-error`, `temp`, `clock-id`, `type`, `phase-offset-monitor`, `phase-offset-avg-factor`, `frequency-monitor`]

dump:
:   **reply**
    :   attributes:
        :   [`id`, `module-name`, `mode`, `mode-supported`, `lock-status`, `lock-status-error`, `temp`, `clock-id`, `type`, `phase-offset-monitor`, `phase-offset-avg-factor`, `frequency-monitor`]

### [device-set](#id14)

Set attributes for a DPLL device

attribute-set:
:   [dpll](#dpll-attribute-set-dpll)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   dpll-pre-doit

    **post**
    :   dpll-post-doit

    **request**
    :   attributes:
        :   [`id`, `mode`, `phase-offset-monitor`, `phase-offset-avg-factor`, `frequency-monitor`]

### [device-create-ntf](#id15)

Notification about device appearing

notify:
:   device-get

mcgrp:
:   monitor

### [device-delete-ntf](#id16)

Notification about device disappearing

notify:
:   device-get

mcgrp:
:   monitor

### [device-change-ntf](#id17)

Notification about device configuration being changed

notify:
:   device-get

mcgrp:
:   monitor

### [pin-id-get](#id18)

Get id of a pin that matches given attributes

attribute-set:
:   [pin](#dpll-attribute-set-pin)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   dpll-lock-doit

    **post**
    :   dpll-unlock-doit

    **request**
    :   attributes:
        :   [`module-name`, `clock-id`, `board-label`, `panel-label`, `package-label`, `type`]

    **reply**
    :   attributes:
        :   [`id`]

### [pin-get](#id19)

Get list of pins and its attributes.

* dump request without any attributes given - list all the pins in the
  system
* dump request with target dpll - list all the pins registered with
  a given dpll device
* do request with target dpll and target pin - single pin attributes

attribute-set:
:   [pin](#dpll-attribute-set-pin)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   dpll-pin-pre-doit

    **post**
    :   dpll-pin-post-doit

    **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`, `module-name`, `clock-id`, `board-label`, `panel-label`, `package-label`, `type`, `frequency`, `frequency-supported`, `capabilities`, `parent-device`, `parent-pin`, `phase-adjust-gran`, `phase-adjust-min`, `phase-adjust-max`, `phase-adjust`, `fractional-frequency-offset`, `fractional-frequency-offset-ppt`, `esync-frequency`, `esync-frequency-supported`, `esync-pulse`, `reference-sync`, `measured-frequency`]

dump:
:   **request**
    :   attributes:
        :   [`id`]

    **reply**
    :   attributes:
        :   [`id`, `module-name`, `clock-id`, `board-label`, `panel-label`, `package-label`, `type`, `frequency`, `frequency-supported`, `capabilities`, `parent-device`, `parent-pin`, `phase-adjust-gran`, `phase-adjust-min`, `phase-adjust-max`, `phase-adjust`, `fractional-frequency-offset`, `fractional-frequency-offset-ppt`, `esync-frequency`, `esync-frequency-supported`, `esync-pulse`, `reference-sync`, `measured-frequency`]

### [pin-set](#id20)

Set attributes of a target pin

attribute-set:
:   [pin](#dpll-attribute-set-pin)

flags:
:   [`admin-perm`]

do:
:   **pre**
    :   dpll-pin-pre-doit

    **post**
    :   dpll-pin-post-doit

    **request**
    :   attributes:
        :   [`id`, `frequency`, `direction`, `prio`, `state`, `parent-device`, `parent-pin`, `phase-adjust`, `esync-frequency`, `reference-sync`]

### [pin-create-ntf](#id21)

Notification about pin appearing

notify:
:   pin-get

mcgrp:
:   monitor

### [pin-delete-ntf](#id22)

Notification about pin disappearing

notify:
:   pin-get

mcgrp:
:   monitor

### [pin-change-ntf](#id23)

Notification about pin configuration being changed

notify:
:   pin-get

mcgrp:
:   monitor

## [Multicast groups](#id24)

* monitor

## [Definitions](#id25)

### [mode](#id26)

type:
:   enum

doc:
:   working modes a dpll can support, differentiates if and how dpll selects one of its inputs to syntonize with it, valid values for DPLL\_A\_MODE attribute

entries:
:   manual:
    :   input can be only selected by sending a request to dpll

    automatic:
    :   highest prio input pin auto selected by dpll

### [lock-status](#id27)

type:
:   enum

doc:
:   provides information of dpll device lock status, valid values for DPLL\_A\_LOCK\_STATUS attribute

entries:
:   unlocked:
    :   dpll was not yet locked to any valid input (or forced by setting DPLL\_A\_MODE to DPLL\_MODE\_DETACHED)

    locked:
    :   dpll is locked to a valid signal, but no holdover available

    locked-ho-acq:
    :   dpll is locked and holdover acquired

    holdover:
    :   dpll is in holdover state - lost a valid lock or was forced by disconnecting all the pins (latter possible only when dpll lock-state was already DPLL\_LOCK\_STATUS\_LOCKED\_HO\_ACQ, if dpll lock-state was not DPLL\_LOCK\_STATUS\_LOCKED\_HO\_ACQ, the dpll’s lock-state shall remain DPLL\_LOCK\_STATUS\_UNLOCKED)

### [lock-status-error](#id28)

type:
:   enum

doc:
:   if previous status change was done due to a failure, this provides information of dpll device lock status error. Valid values for DPLL\_A\_LOCK\_STATUS\_ERROR attribute

entries:
:   none:
    :   dpll device lock status was changed without any error

    undefined:
    :   dpll device lock status was changed due to undefined error. Driver fills this value up in case it is not able to obtain suitable exact error type.

    media-down:
    :   dpll device lock status was changed because of associated media got down. This may happen for example if dpll device was previously locked on an input pin of type PIN\_TYPE\_SYNCE\_ETH\_PORT.

    fractional-frequency-offset-too-high:
    :   the FFO (Fractional Frequency Offset) between the RX and TX symbol rate on the media got too high. This may happen for example if dpll device was previously locked on an input pin of type PIN\_TYPE\_SYNCE\_ETH\_PORT.

### [clock-quality-level](#id29)

type:
:   enum

doc:
:   level of quality of a clock device. This mainly applies when the dpll lock-status is DPLL\_LOCK\_STATUS\_HOLDOVER. The current list is defined according to the table 11-7 contained in ITU-T G.8264/Y.1364 document. One may extend this list freely by other ITU-T defined clock qualities, or different ones defined by another standardization body (for those, please use different prefix).

entries:
:   itu-opt1-prc:


    itu-opt1-ssu-a:


    itu-opt1-ssu-b:


    itu-opt1-eec1:


    itu-opt1-prtc:


    itu-opt1-eprtc:


    itu-opt1-eeec:


    itu-opt1-eprc:

### [temp-divider](#id30)

type:
:   const

value:
:   1000

doc:
:   temperature divider allowing userspace to calculate the temperature as float with three digit decimal precision. Value of (DPLL\_A\_TEMP / DPLL\_TEMP\_DIVIDER) is integer part of temperature value. Value of (DPLL\_A\_TEMP % DPLL\_TEMP\_DIVIDER) is fractional part of temperature value.

### [type](#id31)

type:
:   enum

doc:
:   type of dpll, valid values for DPLL\_A\_TYPE attribute

entries:
:   pps:
    :   dpll produces Pulse-Per-Second signal

    eec:
    :   dpll drives the Ethernet Equipment Clock

### [pin-type](#id32)

type:
:   enum

doc:
:   defines possible types of a pin, valid values for DPLL\_A\_PIN\_TYPE attribute

entries:
:   mux:
    :   aggregates another layer of selectable pins

    ext:
    :   external input

    synce-eth-port:
    :   ethernet port PHY’s recovered clock

    int-oscillator:
    :   device internal oscillator

    gnss:
    :   GNSS recovered clock

### [pin-direction](#id33)

type:
:   enum

doc:
:   defines possible direction of a pin, valid values for DPLL\_A\_PIN\_DIRECTION attribute

entries:
:   input:
    :   pin used as a input of a signal

    output:
    :   pin used to output the signal

### [pin-frequency-1-hz](#id34)

type:
:   const

value:
:   1

### [pin-frequency-10-khz](#id35)

type:
:   const

value:
:   10000

### [pin-frequency-77-5-khz](#id36)

type:
:   const

value:
:   77500

### [pin-frequency-10-mhz](#id37)

type:
:   const

value:
:   10000000

### [pin-state](#id38)

type:
:   enum

doc:
:   defines possible states of a pin, valid values for DPLL\_A\_PIN\_STATE attribute

entries:
:   connected:
    :   pin connected, active input of phase locked loop

    disconnected:
    :   pin disconnected, not considered as a valid input

    selectable:
    :   pin enabled for automatic input selection

### [pin-capabilities](#id39)

type:
:   flags

doc:
:   defines possible capabilities of a pin, valid flags on DPLL\_A\_PIN\_CAPABILITIES attribute

entries:
:   direction-can-change:
    :   pin direction can be changed

    priority-can-change:
    :   pin priority can be changed

    state-can-change:
    :   pin state can be changed

### [phase-offset-divider](#id40)

type:
:   const

value:
:   1000

doc:
:   phase offset divider allows userspace to calculate a value of measured signal phase difference between a pin and dpll device as a fractional value with three digit decimal precision. Value of (DPLL\_A\_PHASE\_OFFSET / DPLL\_PHASE\_OFFSET\_DIVIDER) is an integer part of a measured phase offset value. Value of (DPLL\_A\_PHASE\_OFFSET % DPLL\_PHASE\_OFFSET\_DIVIDER) is a fractional part of a measured phase offset value.

### [pin-measured-frequency-divider](#id41)

type:
:   const

value:
:   1000

doc:
:   pin measured frequency divider allows userspace to calculate a value of measured input frequency as a fractional value with three digit decimal precision (millihertz). Value of (DPLL\_A\_PIN\_MEASURED\_FREQUENCY / DPLL\_PIN\_MEASURED\_FREQUENCY\_DIVIDER) is an integer part of a measured frequency value. Value of (DPLL\_A\_PIN\_MEASURED\_FREQUENCY % DPLL\_PIN\_MEASURED\_FREQUENCY\_DIVIDER) is a fractional part of a measured frequency value.

### [feature-state](#id42)

type:
:   enum

doc:
:   Allow control (enable/disable) and status checking over features.

entries:
:   disable:
    :   feature shall be disabled

    enable:
    :   feature shall be enabled

## [Attribute sets](#id43)

### [dpll](#id44)

#### id (`u32`)

#### module-name (`string`)

#### pad (`pad`)

#### clock-id (`u64`)

#### mode (`u32`)

enum:
:   [mode](#dpll-definition-mode)

#### mode-supported (`u32`)

enum:
:   [mode](#dpll-definition-mode)

multi-attr:
:   True

#### lock-status (`u32`)

enum:
:   [lock-status](#dpll-definition-lock-status)

#### temp (`s32`)

#### type (`u32`)

enum:
:   [type](#dpll-definition-type)

#### lock-status-error (`u32`)

enum:
:   [lock-status-error](#dpll-definition-lock-status-error)

#### clock-quality-level (`u32`)

enum:
:   [clock-quality-level](#dpll-definition-clock-quality-level)

multi-attr:
:   True

doc:
:   Level of quality of a clock device. This mainly applies when the dpll lock-status is DPLL\_LOCK\_STATUS\_HOLDOVER. This could be put to message multiple times to indicate possible parallel quality levels (e.g. one specified by ITU option 1 and another one specified by option 2).

#### phase-offset-monitor (`u32`)

enum:
:   [feature-state](#dpll-definition-feature-state)

doc:
:   Receive or request state of phase offset monitor feature. If enabled, dpll device shall monitor and notify all currently available inputs for changes of their phase offset against the dpll device.

#### phase-offset-avg-factor (`u32`)

doc:
:   Averaging factor applied to calculation of reported phase offset.

#### frequency-monitor (`u32`)

enum:
:   [feature-state](#dpll-definition-feature-state)

doc:
:   Current or desired state of the frequency monitor feature. If enabled, dpll device shall measure all currently available inputs for their actual input frequency.

### [pin](#id45)

#### id (`u32`)

#### parent-id (`u32`)

#### module-name (`string`)

#### pad (`pad`)

#### clock-id (`u64`)

#### board-label (`string`)

#### panel-label (`string`)

#### package-label (`string`)

#### type (`u32`)

enum:
:   [pin-type](#dpll-definition-pin-type)

#### direction (`u32`)

enum:
:   [pin-direction](#dpll-definition-pin-direction)

#### frequency (`u64`)

#### frequency-supported (`nest`)

multi-attr:
:   True

nested-attributes:
:   [frequency-range](#dpll-attribute-set-frequency-range)

#### frequency-min (`u64`)

#### frequency-max (`u64`)

#### prio (`u32`)

#### state (`u32`)

enum:
:   [pin-state](#dpll-definition-pin-state)

#### capabilities (`u32`)

enum:
:   [pin-capabilities](#dpll-definition-pin-capabilities)

#### parent-device (`nest`)

multi-attr:
:   True

nested-attributes:
:   [pin-parent-device](#dpll-attribute-set-pin-parent-device)

#### parent-pin (`nest`)

multi-attr:
:   True

nested-attributes:
:   [pin-parent-pin](#dpll-attribute-set-pin-parent-pin)

#### phase-adjust-min (`s32`)

#### phase-adjust-max (`s32`)

#### phase-adjust (`s32`)

#### phase-offset (`s64`)

#### fractional-frequency-offset (`sint`)

doc:
:   The FFO (Fractional Frequency Offset) between the RX and TX symbol rate on the media associated with the pin: (rx\_frequency-tx\_frequency)/rx\_frequency Value is in PPM (parts per million). This may be implemented for example for pin of type PIN\_TYPE\_SYNCE\_ETH\_PORT.

#### esync-frequency (`u64`)

doc:
:   Frequency of Embedded SYNC signal. If provided, the pin is configured with a SYNC signal embedded into its base clock frequency.

#### esync-frequency-supported (`nest`)

multi-attr:
:   True

nested-attributes:
:   [frequency-range](#dpll-attribute-set-frequency-range)

doc:
:   If provided a pin is capable of embedding a SYNC signal (within given range) into its base frequency signal.

#### esync-pulse (`u32`)

doc:
:   A ratio of high to low state of a SYNC signal pulse embedded into base clock frequency. Value is in percents.

#### reference-sync (`nest`)

multi-attr:
:   True

nested-attributes:
:   [reference-sync](#dpll-attribute-set-reference-sync)

doc:
:   Capable pin provides list of pins that can be bound to create a reference-sync pin pair.

#### phase-adjust-gran (`u32`)

doc:
:   Granularity of phase adjustment, in picoseconds. The value of phase adjustment must be a multiple of this granularity.

#### fractional-frequency-offset-ppt (`sint`)

doc:
:   The FFO (Fractional Frequency Offset) of the pin with respect to the nominal frequency. Value = (frequency\_measured - frequency\_nominal) / frequency\_nominal Value is in PPT (parts per trillion, 10^-12). Note: This attribute provides higher resolution than the standard fractional-frequency-offset (which is in PPM).

#### measured-frequency (`u64`)

doc:
:   The measured frequency of the input pin in millihertz (mHz). Value of (DPLL\_A\_PIN\_MEASURED\_FREQUENCY / DPLL\_PIN\_MEASURED\_FREQUENCY\_DIVIDER) is an integer part (Hz) of a measured frequency value. Value of (DPLL\_A\_PIN\_MEASURED\_FREQUENCY % DPLL\_PIN\_MEASURED\_FREQUENCY\_DIVIDER) is a fractional part of a measured frequency value.

### [pin-parent-device](#id46)

#### parent-id

#### direction

#### prio

#### state

#### phase-offset

### [pin-parent-pin](#id47)

#### parent-id

#### state

### [frequency-range](#id48)

#### frequency-min

#### frequency-max

### [reference-sync](#id49)

#### id

#### state
