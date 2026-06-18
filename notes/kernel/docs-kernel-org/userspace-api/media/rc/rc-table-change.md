# 5.Changing default Remote Controller mappings

> 출처(원문): https://docs.kernel.org/userspace-api/media/rc/rc-table-change.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5. Changing default Remote Controller mappings

The event interface provides two ioctls to be used against the
/dev/input/event device, to allow changing the default keymapping.

This program demonstrates how to replace the keymap tables.

* [5.1. file: uapi/v4l/keytable.c](keytable.c.html)
