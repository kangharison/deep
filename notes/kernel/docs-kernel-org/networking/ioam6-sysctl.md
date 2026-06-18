# IOAM6 Sysfs variables

> 출처(원문): https://docs.kernel.org/networking/ioam6-sysctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# IOAM6 Sysfs variables

## /proc/sys/net/conf/<iface>/ioam6\_\* variables:

ioam6\_enabled - BOOL
:   Accept (= enabled) or ignore (= disabled) IPv6 IOAM options on ingress
    for this interface.

    * 0 - disabled (default)
    * 1 - enabled

ioam6\_id - SHORT INTEGER
:   Define the IOAM id of this interface.

    Default is ~0.

ioam6\_id\_wide - INTEGER
:   Define the wide IOAM id of this interface.

    Default is ~0.
