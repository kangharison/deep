# Map Layer

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_map.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Map Layer

All access to any memory shared with a device (both sysmem and vram) in the
Xe driver should go through this layer (xe\_map). This layer is built on top
of [Generalizing Access to System and I/O Memory](../../driver-api/device-io.html#generalizing-access-to-system-and-i-o-memory)
and with extra hooks into the Xe driver that allows adding asserts to memory
accesses (e.g. for blocking runtime\_pm D3Cold on Discrete Graphics).
