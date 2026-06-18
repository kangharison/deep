# 1.Introduction

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/rc-intro.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1. Introduction

Currently, most analog and digital devices have a Infrared input for
remote controllers. Each manufacturer has their own type of control. It
is not rare for the same manufacturer to ship different types of
controls, depending on the device.

A Remote Controller interface is mapped as a normal evdev/input
interface, just like a keyboard or a mouse. So, it uses all ioctls
already defined for any other input devices.

However, remove controllers are more flexible than a normal input
device, as the IR receiver (and/or transmitter) can be used in
conjunction with a wide variety of different IR remotes.

In order to allow flexibility, the Remote Controller subsystem allows
controlling the RC-specific attributes via
[the sysfs class nodes](rc-sysfs-nodes.html#remote-controllers-sysfs-nodes).
