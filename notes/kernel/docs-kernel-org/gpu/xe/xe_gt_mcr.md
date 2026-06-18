# GT Multicast/Replicated (MCR) Register Support

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_gt_mcr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GT Multicast/Replicated (MCR) Register Support

Some GT registers are designed as “multicast” or “replicated” registers:
multiple instances of the same register share a single MMIO offset. MCR
registers are generally used when the hardware needs to potentially track
independent values of a register per hardware unit (e.g., per-subslice,
per-L3bank, etc.). The specific types of replication that exist vary
per-platform.

MMIO accesses to MCR registers are controlled according to the settings
programmed in the platform’s MCR\_SELECTOR register(s). MMIO writes to MCR
registers can be done in either multicast (a single write updates all
instances of the register to the same value) or unicast (a write updates only
one specific instance) form. Reads of MCR registers always operate in a
unicast manner regardless of how the multicast/unicast bit is set in
MCR\_SELECTOR. Selection of a specific MCR instance for unicast operations is
referred to as “steering.”

If MCR register operations are steered toward a hardware unit that is
fused off or currently powered down due to power gating, the MMIO operation
is “terminated” by the hardware. Terminated read operations will return a
value of zero and terminated unicast write operations will be silently
ignored. During device initialization, the goal of the various
`init_steering_*()` functions is to apply the platform-specific rules for
each MCR register type to identify a steering target that will select a
non-terminated instance.

MCR registers are not available on Virtual Function (VF).

## Internal API

TODO
