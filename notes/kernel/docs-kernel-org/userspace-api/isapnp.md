# ISA Plug & Play support

> 출처(원문): https://docs.kernel.org/userspace-api/isapnp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ISA Plug & Play support

## Interface /proc/isapnp

The interface was removed in kernel 2.5.53. See pnp.rst for more details.

## Interface /proc/bus/isapnp

This directory allows access to ISA PnP cards and logical devices.
The regular files contain the contents of ISA PnP registers for
a logical device.
