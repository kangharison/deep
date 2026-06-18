# 30.x86_64 Support

> 출처(원문): https://docs.kernel.org/arch/x86/x86_64/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 30. x86\_64 Support

* [30.1. General note on [U]EFI x86\_64 support](uefi.html)
  + [30.1.1. Mechanics](uefi.html#mechanics)
* [30.2. Memory Management](mm.html)
  + [30.2.1. Complete virtual memory map with 4-level page tables](mm.html#complete-virtual-memory-map-with-4-level-page-tables)
  + [30.2.2. Complete virtual memory map with 5-level page tables](mm.html#complete-virtual-memory-map-with-5-level-page-tables)
* [30.3. 5-level paging](5level-paging.html)
  + [30.3.1. Overview](5level-paging.html#overview)
  + [30.3.2. User-space and large virtual address space](5level-paging.html#user-space-and-large-virtual-address-space)
* [30.4. Fake NUMA For CPUSets](fake-numa-for-cpusets.html)
* [30.5. Firmware support for CPU hotplug under Linux/x86-64](cpu-hotplug-spec.html)
* [30.6. Configurable sysfs parameters for the x86-64 machine check code](machinecheck.html)
* [30.7. Using FS and GS segments in user space applications](fsgs.html)
  + [30.7.1. Common FS and GS usage](fsgs.html#common-fs-and-gs-usage)
  + [30.7.2. Reading and writing the FS/GS base address](fsgs.html#reading-and-writing-the-fs-gs-base-address)
  + [30.7.3. Accessing FS/GS base with arch\_prctl()](fsgs.html#accessing-fs-gs-base-with-arch-prctl)
  + [30.7.4. Accessing FS/GS base with the FSGSBASE instructions](fsgs.html#accessing-fs-gs-base-with-the-fsgsbase-instructions)
  + [30.7.5. Compiler support for FS/GS based addressing](fsgs.html#compiler-support-for-fs-gs-based-addressing)
  + [30.7.6. FS/GS based addressing with inline assembly](fsgs.html#fs-gs-based-addressing-with-inline-assembly)
* [30.8. Flexible Return and Event Delivery (FRED)](fred.html)
  + [30.8.1. Overview](fred.html#overview)
  + [30.8.2. Software based event dispatching](fred.html#software-based-event-dispatching)
  + [30.8.3. Full supervisor/user context](fred.html#full-supervisor-user-context)
  + [30.8.4. LKGS](fred.html#lkgs)
  + [30.8.5. Stack levels](fred.html#stack-levels)
