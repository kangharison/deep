# 2.Remote Controller’s sysfs nodes

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/rc-sysfs-nodes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. Remote Controller’s sysfs nodes

As defined at [ABI file testing/sysfs-class-rc](../../../admin-guide/abi-testing-files.html#abi-file-testing-sysfs-class-rc), those are
the sysfs nodes that control the Remote Controllers:

## 2.1. /sys/class/rc/

The `/sys/class/rc/` class sub-directory belongs to the Remote
Controller core and provides a sysfs interface for configuring infrared
remote controller receivers.

## 2.2. /sys/class/rc/rcN/

A `/sys/class/rc/rcN` directory is created for each remote control
receiver device where N is the number of the receiver.

## 2.3. /sys/class/rc/rcN/protocols

Reading this file returns a list of available protocols, something like:

```
rc5 [rc6] nec jvc [sony]
```

Enabled protocols are shown in [] brackets.

Writing “+proto” will add a protocol to the list of enabled protocols.

Writing “-proto” will remove a protocol from the list of enabled
protocols.

Writing “proto” will enable only “proto”.

Writing “none” will disable all protocols.

Write fails with `EINVAL` if an invalid protocol combination or unknown
protocol name is used.

## 2.4. /sys/class/rc/rcN/filter

Sets the scancode filter expected value.

Use in combination with `/sys/class/rc/rcN/filter_mask` to set the
expected value of the bits set in the filter mask. If the hardware
supports it then scancodes which do not match the filter will be
ignored. Otherwise the write will fail with an error.

This value may be reset to 0 if the current protocol is altered.

## 2.5. /sys/class/rc/rcN/filter\_mask

Sets the scancode filter mask of bits to compare. Use in combination
with `/sys/class/rc/rcN/filter` to set the bits of the scancode which
should be compared against the expected value. A value of 0 disables the
filter to allow all valid scancodes to be processed.

If the hardware supports it then scancodes which do not match the filter
will be ignored. Otherwise the write will fail with an error.

This value may be reset to 0 if the current protocol is altered.

## 2.6. /sys/class/rc/rcN/wakeup\_protocols

Reading this file returns a list of available protocols to use for the
wakeup filter, something like:

```
rc-5 nec nec-x rc-6-0 rc-6-6a-24 [rc-6-6a-32] rc-6-mce
```

Note that protocol variants are listed, so `nec`, `sony`, `rc-5`, `rc-6`
have their different bit length encodings listed if available.

Note that all protocol variants are listed.

The enabled wakeup protocol is shown in [] brackets.

Only one protocol can be selected at a time.

Writing “proto” will use “proto” for wakeup events.

Writing “none” will disable wakeup.

Write fails with `EINVAL` if an invalid protocol combination or unknown
protocol name is used, or if wakeup is not supported by the hardware.

## 2.7. /sys/class/rc/rcN/wakeup\_filter

Sets the scancode wakeup filter expected value. Use in combination with
`/sys/class/rc/rcN/wakeup_filter_mask` to set the expected value of
the bits set in the wakeup filter mask to trigger a system wake event.

If the hardware supports it and wakeup\_filter\_mask is not 0 then
scancodes which match the filter will wake the system from e.g. suspend
to RAM or power off. Otherwise the write will fail with an error.

This value may be reset to 0 if the wakeup protocol is altered.

## 2.8. /sys/class/rc/rcN/wakeup\_filter\_mask

Sets the scancode wakeup filter mask of bits to compare. Use in
combination with `/sys/class/rc/rcN/wakeup_filter` to set the bits of
the scancode which should be compared against the expected value to
trigger a system wake event.

If the hardware supports it and wakeup\_filter\_mask is not 0 then
scancodes which match the filter will wake the system from e.g. suspend
to RAM or power off. Otherwise the write will fail with an error.

This value may be reset to 0 if the wakeup protocol is altered.
