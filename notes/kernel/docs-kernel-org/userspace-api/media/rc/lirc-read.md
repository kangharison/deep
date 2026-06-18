# 6.5.1.LIRC read()

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/lirc-read.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.5.1. LIRC read()

## 6.5.1.1. Name

lirc-read - Read from a LIRC device

## 6.5.1.2. Synopsis

```
#include <unistd.h>
```

ssize\_t read(int fd, void \*buf, size\_t count)

## 6.5.1.3. Arguments

`fd`
:   File descriptor returned by `open()`.

`buf`
:   Buffer to be filled

`count`
:   Max number of bytes to read

## 6.5.1.4. Description

[`read()`](#c.RC.read "read") attempts to read up to `count` bytes from file
descriptor `fd` into the buffer starting at `buf`. If `count` is zero,
[`read()`](#c.RC.read "read") returns zero and has no other results. If `count`
is greater than `SSIZE_MAX`, the result is unspecified.

The exact format of the data depends on what [LIRC modes](lirc-dev-intro.html#lirc-modes) a driver
uses. Use [ioctl LIRC\_GET\_FEATURES](lirc-get-features.html#lirc-get-features) to get the supported mode, and use
[ioctls LIRC\_GET\_REC\_MODE and LIRC\_SET\_REC\_MODE](lirc-get-rec-mode.html#lirc-set-rec-mode) set the current active mode.

The mode [LIRC\_MODE\_MODE2](lirc-dev-intro.html#lirc-mode-mode2) is for raw IR,
in which packets containing an unsigned int value describing an IR signal are
read from the chardev.

Alternatively, [LIRC\_MODE\_SCANCODE](lirc-dev-intro.html#lirc-mode-scancode) can be available,
in this mode scancodes which are either decoded by software decoders, or
by hardware decoders. The [`rc_proto`](lirc-dev-intro.html#c.rc_proto "rc_proto") member is set to the
[IR protocol](rc-protos.html#remote-controllers-protocols)
used for transmission, and `scancode` to the decoded scancode,
and the `keycode` set to the keycode or `KEY_RESERVED`.

## 6.5.1.5. Return Value

On success, the number of bytes read is returned. It is not an error if
this number is smaller than the number of bytes requested, or the amount
of data required for one frame. On error, -1 is returned, and the `errno`
variable is set appropriately.
