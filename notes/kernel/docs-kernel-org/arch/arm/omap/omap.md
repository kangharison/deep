# OMAP history

> 출처(원문): https://docs.kernel.org/arch/arm/omap/omap.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# OMAP history

This file contains documentation for running mainline
kernel on omaps.

| KERNEL | NEW DEPENDENCIES |
| --- | --- |
| v4.3+ | Update is needed for custom .config files to make sure CONFIG\_REGULATOR\_PBIAS is enabled for MMC1 to work properly. |
| v4.18+ | Update is needed for custom .config files to make sure CONFIG\_MMC\_SDHCI\_OMAP is enabled for all MMC instances to work in DRA7 and K2G based boards. |
