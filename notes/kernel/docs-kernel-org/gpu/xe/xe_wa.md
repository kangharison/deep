# Hardware workarounds

> 출처(원문): https://docs.kernel.org/gpu/xe/xe_wa.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Hardware workarounds

Hardware workarounds are register programming documented to be executed in
the driver that fall outside of the normal programming sequences for a
platform. There are some basic categories of workarounds, depending on
how/when they are applied:

* LRC workarounds: workarounds that touch registers that are
  saved/restored to/from the HW context image. The list is emitted (via Load
  Register Immediate commands) once when initializing the device and saved in
  the default context. That default context is then used on every context
  creation to have a “primed golden context”, i.e. a context image that
  already contains the changes needed to all the registers. See
  drivers/gpu/drm/xe/xe\_lrc.c for default context handling.
* Engine workarounds: the list of these WAs is applied whenever the specific
  engine is reset. It’s also possible that a set of engine classes share a
  common power domain and they are reset together. This happens on some
  platforms with render and compute engines. In this case (at least) one of
  them need to keeep the workaround programming: the approach taken in the
  driver is to tie those workarounds to the first compute/render engine that
  is registered. When executing with GuC submission, engine resets are
  outside of kernel driver control, hence the list of registers involved is
  written once, on engine initialization, and then passed to GuC, that
  saves/restores their values before/after the reset takes place. See
  drivers/gpu/drm/xe/xe\_guc\_ads.c for reference.
* GT workarounds: the list of these WAs is applied whenever these registers
  revert to their default values: on GPU reset, suspend/resume [[1]](#id2), etc.
* Register whitelist: some workarounds need to be implemented in userspace,
  but need to touch privileged registers. The whitelist in the kernel
  instructs the hardware to allow the access to happen. From the kernel side,
  this is just a special case of a MMIO workaround (as we write the list of
  these to/be-whitelisted registers to some special HW registers).
* Workaround batchbuffers: buffers that get executed automatically by the
  hardware on every HW context restore. These buffers are created and
  programmed in the default context so the hardware always go through those
  programming sequences when switching contexts. The support for workaround
  batchbuffers is enabled via these hardware mechanisms:

  1. INDIRECT\_CTX (also known as **mid context restore bb**): A batchbuffer
     and an offset are provided in the default context, pointing the hardware
     to jump to that location when that offset is reached in the context
     restore. When a context is being restored, this is executed after the
     ring context, in the middle (or beginning) of the engine context image.
  2. BB\_PER\_CTX\_PTR (also known as **post context restore bb**): A
     batchbuffer is provided in the default context, pointing the hardware to
     a buffer to continue executing after the engine registers are restored
     in a context restore sequence.

  Below is the timeline for a context restore sequence:

  ```
                    INDIRECT_CTX_OFFSET
               |----------->|
  .------------.------------.-------------.------------.--------------.-----------.
  |Ring        | Engine     | Mid-context | Engine     | Post-context | Ring      |
  |Restore     | Restore (1)| BB Restore  | Restore (2)| BB Restore   | Execution |
  `------------'------------'-------------'------------'--------------'-----------'
  ```
* Other/OOB: There are WAs that, due to their nature, cannot be applied from
  a central place. Those are peppered around the rest of the code, as needed.
  There’s a central place to control which workarounds are enabled:
  drivers/gpu/drm/xe/xe\_wa\_oob.rules for GT workarounds and
  drivers/gpu/drm/xe/xe\_device\_wa\_oob.rules for device/SoC workarounds.
  These files only record which workarounds are enabled: during early device
  initialization those rules are evaluated and recorded by the driver. Then
  later the driver checks with `XE_GT_WA()` and `XE_DEVICE_WA()` to
  implement them.

[[1](#id1)]

Technically, some registers are powercontext saved & restored, so they
survive a suspend/resume. In practice, writing them again is not too
costly and simplifies things, so it’s the approach taken in the driver.

Note

Hardware workarounds in xe work the same way as in i915, with the
difference of how they are maintained in the code. In xe it uses the
xe\_rtp infrastructure so the workarounds can be kept in tables, following
a more declarative approach rather than procedural.

Note

When a workaround applies to every single known IP version in a range,
the preferred handling is to use a single range-based RTP entry rather
than individual entries for each version, even if some of the intermediate
version numbers are currently unused. If a new intermediate IP version
appears in the future and is enabled in the driver, any existing
range-based entries that contain the new version number will need to be
analyzed to determine whether their workarounds should apply to the new
version, or whether any existing range based entries needs to be split
into two entries that do not include the new intermediate version.

## Internal API

void xe\_wa\_process\_device\_oob(struct xe\_device \*xe)
:   process OOB workaround table

**Parameters**

`struct xe_device *xe`
:   device instance to process workarounds for

**Description**

process OOB workaround table for this device, marking in **xe** the
workarounds that are active.

void xe\_wa\_process\_gt\_oob(struct xe\_gt \*gt)
:   process GT OOB workaround table

**Parameters**

`struct xe_gt *gt`
:   GT instance to process workarounds for

**Description**

Process OOB workaround table for this platform, marking in **gt** the
workarounds that are active.

void xe\_wa\_process\_gt(struct xe\_gt \*gt)
:   process GT workaround table

**Parameters**

`struct xe_gt *gt`
:   GT instance to process workarounds for

**Description**

Process GT workaround table for this platform, saving in **gt** all the
workarounds that need to be applied at the GT level.

void xe\_wa\_process\_engine(struct xe\_hw\_engine \*hwe)
:   process engine workaround table

**Parameters**

`struct xe_hw_engine *hwe`
:   engine instance to process workarounds for

**Description**

Process engine workaround table for this platform, saving in **hwe** all the
workarounds that need to be applied at the engine level that match this
engine.

void xe\_wa\_process\_lrc(struct xe\_hw\_engine \*hwe)
:   process context workaround table

**Parameters**

`struct xe_hw_engine *hwe`
:   engine instance to process workarounds for

**Description**

Process context workaround table for this platform, saving in **hwe** all the
workarounds that need to be applied on context restore. These are workarounds
touching registers that are part of the HW context image.

int xe\_wa\_device\_init(struct xe\_device \*xe)
:   initialize device with workaround oob bookkeeping

**Parameters**

`struct xe_device *xe`
:   Xe device instance to initialize

**Description**

Returns 0 for success, negative with error code otherwise

int xe\_wa\_gt\_init(struct xe\_gt \*gt)
:   initialize gt with workaround bookkeeping

**Parameters**

`struct xe_gt *gt`
:   GT instance to initialize

**Description**

Returns 0 for success, negative error code otherwise.

int xe\_wa\_gt\_dump(struct xe\_gt \*gt, struct [drm\_printer](../drm-internals.html#c.drm_printer "drm_printer") \*p)
:   Dump GT workarounds into a drm printer.

**Parameters**

`struct xe_gt *gt`
:   the `xe_gt`

`struct drm_printer *p`
:   the [`drm_printer`](../drm-internals.html#c.drm_printer "drm_printer")

**Return**

always 0.
