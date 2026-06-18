# 3.SoC Platforms with DTS Compliance Requirements

> 출처(원문): https://docs.kernel.org/process/maintainer-soc-clean-dts.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. SoC Platforms with DTS Compliance Requirements

## 3.1. Overview

SoC platforms or subarchitectures should follow all the rules from
[SoC Subsystem](maintainer-soc.html). This document referenced in
MAINTAINERS impose additional requirements listed below.

## 3.2. Strict DTS DT Schema and dtc Compliance

No changes to the SoC platform Devicetree sources (DTS files) should introduce
new `make dtbs_check W=1` warnings. Warnings in a new board DTS, which are
results of issues in an included DTSI file, are considered existing, not new
warnings. For series split between different trees (DT bindings go via driver
subsystem tree), warnings on linux-next are decisive. The platform maintainers
have automation in place which should point out any new warnings.

If a commit introducing new warnings gets accepted somehow, the resulting
issues shall be fixed in reasonable time (e.g. within one release) or the
commit reverted.
