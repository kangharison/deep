# TEE (Trusted Execution Environment)

> 출처(원문): https://docs.kernel.org/tee/tee.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TEE (Trusted Execution Environment)

This document describes the TEE subsystem in Linux.

## Overview

A TEE is a trusted OS running in some secure environment, for example,
TrustZone on ARM CPUs, or a separate secure co-processor etc. A TEE driver
handles the details needed to communicate with the TEE.

This subsystem deals with:

* Registration of TEE drivers
* Managing shared memory between Linux and the TEE
* Providing a generic API to the TEE
