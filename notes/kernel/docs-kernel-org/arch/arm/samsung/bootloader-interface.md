# Interface between kernel and boot loaders on Exynos boards

> 출처(원문): https://docs.kernel.org/arch/arm/samsung/bootloader-interface.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Interface between kernel and boot loaders on Exynos boards

Author: Krzysztof Kozlowski

Date : 6 June 2015

The document tries to describe currently used interface between Linux kernel
and boot loaders on Samsung Exynos based boards. This is not a definition
of interface but rather a description of existing state, a reference
for information purpose only.

In the document “boot loader” means any of following: U-boot, proprietary
SBOOT or any other firmware for ARMv7 and ARMv8 initializing the board before
executing kernel.

1. Non-Secure mode

Address: sysram\_ns\_base\_addr

| Offset | Value | Purpose |
| --- | --- | --- |
| 0x08 | exynos\_cpu\_resume\_ns, mcpm\_entry\_point | System suspend |
| 0x0c | 0x00000bad (Magic cookie) | System suspend |
| 0x1c | exynos4\_secondary\_startup | Secondary CPU boot |
| 0x1c + 4\*cpu | exynos4\_secondary\_startup (Exynos4412) | Secondary CPU boot |
| 0x20 | 0xfcba0d10 (Magic cookie) | AFTR |
| 0x24 | exynos\_cpu\_resume\_ns | AFTR |
| 0x28 + 4\*cpu | 0x8 (Magic cookie, Exynos3250) | AFTR |
| 0x28 | 0x0 or last value during resume (Exynos542x) | System suspend |

2. Secure mode

Address: sysram\_base\_addr

| Offset | Value | Purpose |
| --- | --- | --- |
| 0x00 | exynos4\_secondary\_startup | Secondary CPU boot |
| 0x04 | exynos4\_secondary\_startup (Exynos542x) | Secondary CPU boot |
| 4\*cpu | exynos4\_secondary\_startup (Exynos4412) | Secondary CPU boot |
| 0x20 | exynos\_cpu\_resume (Exynos4210 r1.0) | AFTR |
| 0x24 | 0xfcba0d10 (Magic cookie, Exynos4210 r1.0) | AFTR |

Address: pmu\_base\_addr

| Offset | Value | Purpose |
| --- | --- | --- |
| 0x0800 | exynos\_cpu\_resume | AFTR, suspend |
| 0x0800 | mcpm\_entry\_point (Exynos542x with MCPM) | AFTR, suspend |
| 0x0804 | 0xfcba0d10 (Magic cookie) | AFTR |
| 0x0804 | 0x00000bad (Magic cookie) | System suspend |
| 0x0814 | exynos4\_secondary\_startup (Exynos4210 r1.1) | Secondary CPU boot |
| 0x0818 | 0xfcba0d10 (Magic cookie, Exynos4210 r1.1) | AFTR |
| 0x081C | exynos\_cpu\_resume (Exynos4210 r1.1) | AFTR |

3. Other (regardless of secure/non-secure mode)

Address: pmu\_base\_addr

| Offset | Value | Purpose |
| --- | --- | --- |
| 0x0908 | Non-zero | Secondary CPU boot up indicator on Exynos3250 and Exynos542x |

4. Glossary

AFTR - ARM Off Top Running, a low power mode, Cortex cores and many other
modules are power gated, except the TOP modules
MCPM - Multi-Cluster Power Management
