# Huge Pages

> 출처(원문): https://docs.kernel.org/driver-api/cxl/allocation/hugepages.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Huge Pages

## Contiguous Memory Allocator

CXL Memory onlined as SystemRAM during early boot is eligible for use by CMA,
as the NUMA node hosting that capacity will be Online at the time CMA
carves out contiguous capacity.

CXL Memory deferred to the CXL Driver for configuration cannot have its
capacity allocated by CMA - as the NUMA node hosting the capacity is Offline
at `__init` time - when CMA carves out contiguous capacity.

## HugeTLB

Different huge page sizes allow different memory configurations.

### 2MB Huge Pages

All CXL capacity regardless of configuration time or memory zone is eligible
for use as 2MB huge pages.

### 1GB Huge Pages

CXL capacity onlined in `ZONE_NORMAL` is eligible for 1GB Gigantic Page
allocation.

CXL capacity onlined in `ZONE_MOVABLE` is not eligible for 1GB Gigantic
Page allocation.
