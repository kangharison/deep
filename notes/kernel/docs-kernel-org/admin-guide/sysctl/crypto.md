# /proc/sys/crypto/

> 출처(원문): https://docs.kernel.org/admin-guide/sysctl/crypto.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# /proc/sys/crypto/

These files show up in `/proc/sys/crypto/`, depending on the
kernel configuration:

## [fips\_enabled](#id1)

Read-only flag that indicates whether FIPS mode is enabled.

* `0`: FIPS mode is disabled (default).
* `1`: FIPS mode is enabled.

This value is set at boot time via the `fips=1` kernel command line
parameter. When enabled, the cryptographic API will restrict the use
of certain algorithms and perform self-tests to ensure compliance with
FIPS (Federal Information Processing Standards) requirements, such as
FIPS 140-2 and the newer FIPS 140-3, depending on the kernel
configuration and the module in use.

## [fips\_name](#id2)

Read-only file that contains the name of the FIPS module currently in use.
The value is typically configured via the `CONFIG_CRYPTO_FIPS_NAME`
kernel configuration option.

## [fips\_version](#id3)

Read-only file that contains the version string of the FIPS module.
If `CONFIG_CRYPTO_FIPS_CUSTOM_VERSION` is set, it uses the value from
`CONFIG_CRYPTO_FIPS_VERSION`. Otherwise, it defaults to the kernel
release version (`UTS_RELEASE`).

Copyright (c) 2026, Shubham Chakraborty <[chakrabortyshubham66@gmail.com](mailto:chakrabortyshubham66%40gmail.com)>

For general info and legal blurb, please look in
[Documentation for /proc/sys](index.html).
