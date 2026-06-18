# GPU RFC Section

> 출처(원문): https://docs.kernel.org/gpu/rfc/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# GPU RFC Section

For complex work, especially new uapi, it is often good to nail the high level
design issues before getting lost in the code details. This section is meant to
host such documentation:

* Each RFC should be a section in this file, explaining the goal and main design
  considerations. Especially for uapi make sure you Cc: all relevant project
  mailing lists and involved people outside of dri-devel.
* For uapi structures add a file to this directory with and then pull the
  kerneldoc in like with real uapi headers.
* Once the code has landed move all the documentation to the right places in
  the main core, helper or driver sections.

* [GPU SVM Section](gpusvm.html)
  + [Agreed upon design principles](gpusvm.html#agreed-upon-design-principles)
  + [Overview of baseline design](gpusvm.html#overview-of-baseline-design)
  + [Overview of drm\_pagemap design](gpusvm.html#overview-of-drm-pagemap-design)
  + [Possible future design features](gpusvm.html#possible-future-design-features)

* [I915 DG1/LMEM RFC Section](i915_gem_lmem.html)
  + [Upstream plan](i915_gem_lmem.html#upstream-plan)

* [I915 GuC Submission/DRM Scheduler Section](i915_scheduler.html)
  + [Upstream plan](i915_scheduler.html#upstream-plan)
  + [TODOs for GuC submission upstream](i915_scheduler.html#todos-for-guc-submission-upstream)
  + [New uAPI for basic GuC submission](i915_scheduler.html#new-uapi-for-basic-guc-submission)
    - [Spec references:](i915_scheduler.html#spec-references)
  + [New parallel submission uAPI](i915_scheduler.html#new-parallel-submission-uapi)
    - [Export engines logical mapping](i915_scheduler.html#export-engines-logical-mapping)
    - [A ‘set\_parallel’ extension to configure contexts for parallel submission](i915_scheduler.html#a-set-parallel-extension-to-configure-contexts-for-parallel-submission)
    - [Extend execbuf2 IOCTL to support submitting N BBs in a single IOCTL](i915_scheduler.html#extend-execbuf2-ioctl-to-support-submitting-n-bbs-in-a-single-ioctl)

* [I915 Small BAR RFC Section](i915_small_bar.html)
  + [I915\_GEM\_CREATE\_EXT\_FLAG\_NEEDS\_CPU\_ACCESS flag](i915_small_bar.html#i915-gem-create-ext-flag-needs-cpu-access-flag)
  + [probed\_cpu\_visible\_size attribute](i915_small_bar.html#probed-cpu-visible-size-attribute)
  + [Error Capture restrictions](i915_small_bar.html#error-capture-restrictions)

* [I915 VM\_BIND feature design and use cases](i915_vm_bind.html)
  + [VM\_BIND feature](i915_vm_bind.html#vm-bind-feature)
    - [TLB flush consideration](i915_vm_bind.html#tlb-flush-consideration)
    - [Execbuf ioctl in VM\_BIND mode](i915_vm_bind.html#execbuf-ioctl-in-vm-bind-mode)
    - [VM\_PRIVATE objects](i915_vm_bind.html#vm-private-objects)
    - [VM\_BIND locking hierarchy](i915_vm_bind.html#vm-bind-locking-hierarchy)
    - [VM\_BIND LRU handling](i915_vm_bind.html#vm-bind-lru-handling)
    - [VM\_BIND dma\_resv usage](i915_vm_bind.html#vm-bind-dma-resv-usage)
    - [Mesa use case](i915_vm_bind.html#mesa-use-case)
  + [Other VM\_BIND use cases](i915_vm_bind.html#other-vm-bind-use-cases)
    - [Long running Compute contexts](i915_vm_bind.html#long-running-compute-contexts)
      * [User/Memory Fence](i915_vm_bind.html#user-memory-fence)
      * [Low Latency Submission](i915_vm_bind.html#low-latency-submission)
    - [Debugger](i915_vm_bind.html#debugger)
    - [GPU page faults](i915_vm_bind.html#gpu-page-faults)
    - [Page level hints settings](i915_vm_bind.html#page-level-hints-settings)
    - [Page level Cache/CLOS settings](i915_vm_bind.html#page-level-cache-clos-settings)
    - [Evictable page table allocations](i915_vm_bind.html#evictable-page-table-allocations)
    - [Shared Virtual Memory (SVM) support](i915_vm_bind.html#shared-virtual-memory-svm-support)
  + [VM\_BIND UAPI](i915_vm_bind.html#vm-bind-uapi)

* [Linux Color Pipeline API](color_pipeline.html)
  + [What problem are we solving?](color_pipeline.html#what-problem-are-we-solving)
  + [How are other OSes solving this problem?](color_pipeline.html#how-are-other-oses-solving-this-problem)
  + [Why is Linux different?](color_pipeline.html#why-is-linux-different)
  + [Descriptive API](color_pipeline.html#descriptive-api)
  + [Prescriptive API](color_pipeline.html#prescriptive-api)
  + [The Color Pipeline API](color_pipeline.html#the-color-pipeline-api)
  + [drm\_colorop Object](color_pipeline.html#drm-colorop-object)
    - [drm\_colorop extensibility](color_pipeline.html#drm-colorop-extensibility)
  + [COLOR\_PIPELINE Plane Property](color_pipeline.html#color-pipeline-plane-property)
  + [Color Pipeline Discovery](color_pipeline.html#color-pipeline-discovery)
  + [Color Pipeline Programming](color_pipeline.html#color-pipeline-programming)
  + [Driver Implementer’s Guide](color_pipeline.html#driver-implementer-s-guide)
  + [Driver Forward/Backward Compatibility](color_pipeline.html#driver-forward-backward-compatibility)
  + [References](color_pipeline.html#references)
