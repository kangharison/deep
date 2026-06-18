# I915 DG1/LMEM RFC Section

> 출처(원문): https://docs.kernel.org/gpu/rfc/i915_gem_lmem.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# I915 DG1/LMEM RFC Section

## Upstream plan

For upstream the overall plan for landing all the DG1 stuff and turning it for
real, with all the uAPI bits is:

* Merge basic HW enabling of DG1(still without pciid)
* Merge the uAPI bits behind special CONFIG\_BROKEN(or so) flag
  :   + At this point we can still make changes, but importantly this lets us
        start running IGTs which can utilize local-memory in CI
* Convert over to TTM, make sure it all keeps working. Some of the work items:
  :   + TTM shrinker for discrete
      + dma\_resv\_lockitem for full dma\_resv\_lock, i.e not just trylock
      + Use TTM CPU pagefault handler
      + Route shmem backend over to TTM SYSTEM for discrete
      + TTM purgeable object support
      + Move i915 buddy allocator over to TTM
* Send RFC(with mesa-dev on cc) for final sign off on the uAPI
* Add pciid for DG1 and turn on uAPI for real
