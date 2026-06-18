# Xe Device Wedging

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_device.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Xe Device Wedging

Xe driver uses drm device wedged uevent as documented in [Userland interfaces](../drm-uapi.html).
When device is in wedged state, every IOCTL will be blocked and GT cannot
be used. The conditions under which the driver declares the device wedged
depend on the wedged mode configuration (see `enum xe_wedged_mode`). The
default recovery method for a wedged state is rebind/bus-reset.

Another recovery method is vendor-specific. Below are the cases that send
`WEDGED=vendor-specific` recovery method in drm device wedged uevent.

## Case: Firmware Flash

### Identification Hint

`WEDGED=vendor-specific` drm device wedged uevent with
[Runtime Survivability mode](xe_pcode.html#xe-survivability-mode) is used to notify
admin/userspace consumer about the need for a firmware flash.

### Recovery Procedure

Once `WEDGED=vendor-specific` drm device wedged uevent is received, follow
the below steps

* Check Runtime Survivability mode sysfs.
  If enabled, firmware flash is required to recover the device.

  /sys/bus/pci/devices/<device>/survivability\_mode
* Admin/userspace consumer can use firmware flashing tools like fwupd to flash
  firmware and restore device to normal operation.
