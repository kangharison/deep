# drm/xen-front Xen para-virtualized frontend driver

> 출처(원문): https://docs.kernel.org/gpu/xen-front.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/xen-front Xen para-virtualized frontend driver

This frontend driver implements Xen para-virtualized display
according to the display protocol described at
include/xen/interface/io/displif.h

## Driver modes of operation in terms of display buffers used

Depending on the requirements for the para-virtualized environment, namely
requirements dictated by the accompanying DRM/(v)GPU drivers running in both
host and guest environments, display buffers can be allocated by either
frontend driver or backend.

### Buffers allocated by the frontend driver

In this mode of operation driver allocates buffers from system memory.

Note! If used with accompanying DRM/(v)GPU drivers this mode of operation
may require IOMMU support on the platform, so accompanying DRM/vGPU
hardware can still reach display buffer memory while importing PRIME
buffers from the frontend driver.

### Buffers allocated by the backend

This mode of operation is run-time configured via guest domain configuration
through XenStore entries.

For systems which do not provide IOMMU support, but having specific
requirements for display buffers it is possible to allocate such buffers
at backend side and share those with the frontend.
For example, if host domain is 1:1 mapped and has DRM/GPU hardware expecting
physically contiguous memory, this allows implementing zero-copying
use-cases.

Note, while using this scenario the following should be considered:

1. If guest domain dies then pages/grants received from the backend
   cannot be claimed back
2. Misbehaving guest may send too many requests to the
   backend exhausting its grant references and memory
   (consider this from security POV)

## Driver limitations

1. Only primary plane without additional properties is supported.
2. Only one video mode per connector supported which is configured
   via XenStore.
3. All CRTCs operate at fixed frequency of 60Hz.
