# I915 GuC Submission/DRM Scheduler Section

> 출처(원문): https://docs.kernel.org/gpu/rfc/i915_scheduler.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# I915 GuC Submission/DRM Scheduler Section

## Upstream plan

For upstream the overall plan for landing GuC submission and integrating the
i915 with the DRM scheduler is:

* Merge basic GuC submission
  :   + Basic submission support for all gen11+ platforms
      + Not enabled by default on any current platforms but can be enabled via
        modparam enable\_guc
      + Lots of rework will need to be done to integrate with DRM scheduler so
        no need to nit pick everything in the code, it just should be
        functional, no major coding style / layering errors, and not regress
        execlists
      + Update IGTs / selftests as needed to work with GuC submission
      + Enable CI on supported platforms for a baseline
      + Rework / get CI heathly for GuC submission in place as needed
* Merge new parallel submission uAPI
  :   + Bonding uAPI completely incompatible with GuC submission, plus it has
        severe design issues in general, which is why we want to retire it no
        matter what
      + New uAPI adds I915\_CONTEXT\_ENGINES\_EXT\_PARALLEL context setup step
        which configures a slot with N contexts
      + After I915\_CONTEXT\_ENGINES\_EXT\_PARALLEL a user can submit N batches to
        a slot in a single execbuf IOCTL and the batches run on the GPU in
        parallel
      + Initially only for GuC submission but execlists can be supported if
        needed
* Convert the i915 to use the DRM scheduler
  :   + GuC submission backend fully integrated with DRM scheduler
        :   - All request queues removed from backend (e.g. all backpressure
              handled in DRM scheduler)
            - Resets / cancels hook in DRM scheduler
            - Watchdog hooks into DRM scheduler
            - Lots of complexity of the GuC backend can be pulled out once
              integrated with DRM scheduler (e.g. state machine gets
              simpler, locking gets simpler, etc...)
      + Execlists backend will minimum required to hook in the DRM scheduler
        :   - Legacy interface
            - Features like timeslicing / preemption / virtual engines would
              be difficult to integrate with the DRM scheduler and these
              features are not required for GuC submission as the GuC does
              these things for us
            - ROI low on fully integrating into DRM scheduler
            - Fully integrating would add lots of complexity to DRM
              scheduler
      + Port i915 priority inheritance / boosting feature in DRM scheduler
        :   - Used for i915 page flip, may be useful to other DRM drivers as
              well
            - Will be an optional feature in the DRM scheduler
      + Remove in-order completion assumptions from DRM scheduler
        :   - Even when using the DRM scheduler the backends will handle
              preemption, timeslicing, etc... so it is possible for jobs to
              finish out of order
      + Pull out i915 priority levels and use DRM priority levels
      + Optimize DRM scheduler as needed

## TODOs for GuC submission upstream

* Need an update to GuC firmware / i915 to enable error state capture
* Open source tool to decode GuC logs
* Public GuC spec

## New uAPI for basic GuC submission

No major changes are required to the uAPI for basic GuC submission. The only
change is a new scheduler attribute: I915\_SCHEDULER\_CAP\_STATIC\_PRIORITY\_MAP.
This attribute indicates the 2k i915 user priority levels are statically mapped
into 3 levels as follows:

* -1k to -1 Low priority
* 0 Medium priority
* 1 to 1k High priority

This is needed because the GuC only has 4 priority bands. The highest priority
band is reserved with the kernel. This aligns with the DRM scheduler priority
levels too.

### Spec references:

* <https://www.khronos.org/registry/EGL/extensions/IMG/EGL_IMG_context_priority.txt>
* <https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap5.html#devsandqueues-priority>
* <https://spec.oneapi.com/level-zero/latest/core/api.html#ze-command-queue-priority-t>

## New parallel submission uAPI

The existing bonding uAPI is completely broken with GuC submission because
whether a submission is a single context submit or parallel submit isn’t known
until execbuf time activated via the I915\_SUBMIT\_FENCE. To submit multiple
contexts in parallel with the GuC the context must be explicitly registered with
N contexts and all N contexts must be submitted in a single command to the GuC.
The GuC interfaces do not support dynamically changing between N contexts as the
bonding uAPI does. Hence the need for a new parallel submission interface. Also
the legacy bonding uAPI is quite confusing and not intuitive at all. Furthermore
I915\_SUBMIT\_FENCE is by design a future fence, so not really something we should
continue to support.

The new parallel submission uAPI consists of 3 parts:

* Export engines logical mapping
* A ‘set\_parallel’ extension to configure contexts for parallel
  submission
* Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

### Export engines logical mapping

Certain use cases require BBs to be placed on engine instances in logical order
(e.g. split-frame on gen11+). The logical mapping of engine instances can change
based on fusing. Rather than making UMDs be aware of fusing, simply expose the
logical mapping with the existing query engine info IOCTL. Also the GuC
submission interface currently only supports submitting multiple contexts to
engines in logical order which is a new requirement compared to execlists.
Lastly, all current platforms have at most 2 engine instances and the logical
order is the same as uAPI order. This will change on platforms with more than 2
engine instances.

A single bit will be added to drm\_i915\_engine\_info.flags indicating that the
logical instance has been returned and a new field,
drm\_i915\_engine\_info.logical\_instance, returns the logical instance.

### A ‘set\_parallel’ extension to configure contexts for parallel submission

The ‘set\_parallel’ extension configures a slot for parallel submission of N BBs.
It is a setup step that must be called before using any of the contexts. See
I915\_CONTEXT\_ENGINES\_EXT\_LOAD\_BALANCE or I915\_CONTEXT\_ENGINES\_EXT\_BOND for
similar existing examples. Once a slot is configured for parallel submission the
execbuf2 IOCTL can be called submitting N BBs in a single IOCTL. Initially only
supports GuC submission. Execlists supports can be added later if needed.

Add I915\_CONTEXT\_ENGINES\_EXT\_PARALLEL\_SUBMIT and
drm\_i915\_context\_engines\_parallel\_submit to the uAPI to implement this
extension.

struct i915\_context\_engines\_parallel\_submit
:   Configure engine for parallel submission.

**Definition**:

```
struct i915_context_engines_parallel_submit {
    struct i915_user_extension base;
    __u16 engine_index;
    __u16 width;
    __u16 num_siblings;
    __u16 mbz16;
    __u64 flags;
    __u64 mbz64[3];
    struct i915_engine_class_instance engines[];
};
```

**Members**

`base`
:   base user extension.

`engine_index`
:   slot for parallel engine

`width`
:   number of contexts per parallel engine or in other words the
    number of batches in each submission

`num_siblings`
:   number of siblings per context or in other words the
    number of possible placements for each submission

`mbz16`
:   reserved for future use; must be zero

`flags`
:   all undefined flags must be zero, currently not defined flags

`mbz64`
:   reserved for future use; must be zero

`engines`
:   2-d array of engine instances to configure parallel engine

    length = width (i) \* num\_siblings (j)
    index = j + i \* num\_siblings

**Description**

Setup a slot in the context engine map to allow multiple BBs to be submitted
in a single execbuf IOCTL. Those BBs will then be scheduled to run on the GPU
in parallel. Multiple hardware contexts are created internally in the i915 to
run these BBs. Once a slot is configured for N BBs only N BBs can be
submitted in each execbuf IOCTL and this is implicit behavior e.g. The user
doesn’t tell the execbuf IOCTL there are N BBs, the execbuf IOCTL knows how
many BBs there are based on the slot’s configuration. The N BBs are the last
N buffer objects or first N if I915\_EXEC\_BATCH\_FIRST is set.

The default placement behavior is to create implicit bonds between each
context if each context maps to more than 1 physical engine (e.g. context is
a virtual engine). Also we only allow contexts of same engine class and these
contexts must be in logically contiguous order. Examples of the placement
behavior are described below. Lastly, the default is to not allow BBs to be
preempted mid-batch. Rather insert coordinated preemption points on all
hardware contexts between each set of BBs. Flags could be added in the future
to change both of these default behaviors.

Returns -EINVAL if hardware context placement configuration is invalid or if
the placement configuration isn’t supported on the platform / submission
interface.
Returns -ENODEV if extension isn’t supported on the platform / submission
interface.

```
Examples syntax:
CS[X] = generic engine of same class, logical instance X
INVALID = I915_ENGINE_CLASS_INVALID, I915_ENGINE_CLASS_INVALID_NONE

Example 1 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=1,
             engines=CS[0],CS[1])

Results in the following valid placement:
CS[0], CS[1]

Example 2 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[2],CS[1],CS[3])

Results in the following valid placements:
CS[0], CS[1]
CS[2], CS[3]

This can be thought of as two virtual engines, each containing two
engines thereby making a 2D array. However, there are bonds tying the
entries together and placing restrictions on how they can be scheduled.
Specifically, the scheduler can choose only vertical columns from the 2D
array. That is, CS[0] is bonded to CS[1] and CS[2] to CS[3]. So if the
scheduler wants to submit to CS[0], it must also choose CS[1] and vice
versa. Same for CS[2] requires also using CS[3].
VE[0] = CS[0], CS[2]
VE[1] = CS[1], CS[3]

Example 3 pseudo code:
set_engines(INVALID)
set_parallel(engine_index=0, width=2, num_siblings=2,
             engines=CS[0],CS[1],CS[1],CS[3])

Results in the following valid and invalid placements:
CS[0], CS[1]
CS[1], CS[3] - Not logically contiguous, return -EINVAL
```

### Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL

Contexts that have been configured with the ‘set\_parallel’ extension can only
submit N BBs in a single execbuf2 IOCTL. The BBs are either the last N objects
in the drm\_i915\_gem\_exec\_object2 list or the first N if I915\_EXEC\_BATCH\_FIRST is
set. The number of BBs is implicit based on the slot submitted and how it has
been configured by ‘set\_parallel’ or other extensions. No uAPI changes are
required to the execbuf2 IOCTL.
