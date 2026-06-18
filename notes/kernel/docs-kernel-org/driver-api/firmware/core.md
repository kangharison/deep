# Firmware API core features

> 출처(원문): https://docs.kernel.org/driver-api/firmware/core.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Firmware API core features

The firmware API has a rich set of core features available. This section
documents these features.

* [Firmware search paths](fw_search_path.html)
* [Built-in firmware](built-in-fw.html)
* [Firmware cache](firmware_cache.html)
* [Direct filesystem lookup](direct-fs-lookup.html)
  + [Firmware and initramfs](direct-fs-lookup.html#firmware-and-initramfs)
* [Fallback mechanisms](fallback-mechanisms.html)
  + [Justifying the firmware fallback mechanism](fallback-mechanisms.html#justifying-the-firmware-fallback-mechanism)
  + [Types of fallback mechanisms](fallback-mechanisms.html#types-of-fallback-mechanisms)
  + [Firmware sysfs loading facility](fallback-mechanisms.html#firmware-sysfs-loading-facility)
    - [firmware\_fallback\_sysfs](fallback-mechanisms.html#firmware-fallback-sysfs)
  + [Firmware kobject uevent fallback mechanism](fallback-mechanisms.html#firmware-kobject-uevent-fallback-mechanism)
  + [Firmware custom fallback mechanism](fallback-mechanisms.html#firmware-custom-fallback-mechanism)
  + [Firmware fallback timeout](fallback-mechanisms.html#firmware-fallback-timeout)
  + [EFI embedded firmware fallback mechanism](fallback-mechanisms.html#efi-embedded-firmware-fallback-mechanism)
    - [Example how to check for and extract embedded firmware](fallback-mechanisms.html#example-how-to-check-for-and-extract-embedded-firmware)
* [Firmware lookup order](lookup-order.html)
* [Firmware Guidelines](firmware-usage-guidelines.html)
