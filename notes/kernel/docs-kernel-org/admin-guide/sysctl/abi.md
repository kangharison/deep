# Documentation for /proc/sys/abi/

> 출처(원문): https://docs.kernel.org/admin-guide/sysctl/abi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Documentation for /proc/sys/abi/

Copyright (c) 2020, Stephen Kitt

For general info, see [Documentation for /proc/sys](index.html).

---

The files in `/proc/sys/abi` can be used to see and modify
ABI-related settings.

Currently, these files might (depending on your configuration)
show up in `/proc/sys/kernel`:

## [vsyscall32 (x86)](#id1)

Determines whether the kernels maps a vDSO page into 32-bit processes;
can be set to 1 to enable, or 0 to disable. Defaults to enabled if
`CONFIG_COMPAT_VDSO` is set, disabled otherwise.

This controls the same setting as the `vdso32` kernel boot
parameter.
