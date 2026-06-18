# Miscellaneous Devices

> 출처(원문): https://docs.kernel.org/driver-api/misc_devices.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Miscellaneous Devices

int misc\_register(struct miscdevice \*misc)
:   register a miscellaneous device

**Parameters**

`struct miscdevice *misc`
:   device structure

**Description**

> Register a miscellaneous device with the kernel. If the minor
> number is set to `MISC_DYNAMIC_MINOR` a minor number is assigned
> and placed in the minor field of the structure. For other cases
> the minor number requested is used.
>
> The structure passed is linked into the kernel and may not be
> destroyed until it has been unregistered. By default, an open()
> syscall to the device sets file->private\_data to point to the
> structure. Drivers don’t need open in fops for this.
>
> A zero is returned on success and a negative errno code for
> failure.

void misc\_deregister(struct miscdevice \*misc)
:   unregister a miscellaneous device

**Parameters**

`struct miscdevice *misc`
:   device to unregister

**Description**

> Unregister a miscellaneous device that was previously
> successfully registered with [`misc_register()`](#c.misc_register "misc_register").
