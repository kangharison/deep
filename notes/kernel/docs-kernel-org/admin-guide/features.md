# Feature status on all architectures

> 출처(원문): https://docs.kernel.org/admin-guide/features.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Feature status on all architectures

## Subsystem: core

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| cBPF-JIT | `HAVE_CBPF_JIT`  arch supports cBPF JIT optimizations | * **ok**: mips, powerpc   sparc * **TODO**: alpha, arc, arm   arm64, csky, hexagon   loongarch, m68k, microblaze   nios2, openrisc, parisc   riscv, s390, sh, um, x86   xtensa |
| eBPF-JIT | `HAVE_EBPF_JIT`  arch supports eBPF JIT optimizations | * **ok**: arc, arm, arm64   loongarch, mips, parisc   powerpc, riscv, s390, sparc   x86 * **TODO**: alpha, csky   hexagon, m68k, microblaze   nios2, openrisc, sh, um   xtensa |
| generic-idle-thread | `GENERIC_SMP_IDLE_THREAD`  arch makes use of the generic SMP idle thread facility | * **ok**: alpha, arc, arm   arm64, csky, hexagon   loongarch, mips, openrisc   parisc, powerpc, riscv   s390, sh, sparc, um, x86   xtensa * **TODO**: m68k, microblaze   nios2 |
| jump-labels | `HAVE_ARCH_JUMP_LABEL`  arch supports live patched, high efficiency branches | * **ok**: arc, arm, arm64   csky, loongarch, mips   openrisc, parisc, powerpc   riscv, s390, sparc, x86   xtensa * **TODO**: alpha, hexagon   m68k, microblaze, nios2, sh   um |
| mseal-system-mappings | `ARCH_SUPPORTS_MSEAL_SYSTEM_MAPPINGS`  arch supports mseal system mappings | * **ok**: arm64, loongarch   riscv, s390, x86 * **TODO**: alpha, mips   parisc, powerpc, sparc, um * **N/A**: arc, arm, csky   hexagon, m68k, microblaze   nios2, openrisc, sh, xtensa |
| thread-info-in-task | `THREAD_INFO_IN_TASK`  arch makes use of the core kernel facility to embed thread\_info in task\_struct | * **ok**: arm, arm64, parisc   powerpc, riscv, s390, um   x86 * **TODO**: alpha, arc, csky   hexagon, loongarch, m68k   microblaze, mips, nios2   openrisc, sh, sparc, xtensa |
| tracehook | `HAVE_ARCH_TRACEHOOK`  arch supports tracehook (ptrace) register handling APIs | * **ok**: arc, arm, arm64   csky, hexagon, loongarch   mips, nios2, openrisc   parisc, powerpc, riscv   s390, sh, sparc, um, x86   xtensa * **TODO**: alpha, m68k   microblaze |

## Subsystem: debug

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| debug-vm-pgtable | `ARCH_HAS_DEBUG_VM_PGTABLE`  arch supports pgtable tests for semantics compliance | * **ok**: arc, arm64   loongarch, parisc, powerpc   riscv, s390, x86, xtensa * **TODO**: alpha, arm, csky   hexagon, m68k, microblaze   mips, nios2, openrisc, sh   sparc, um |
| gcov-profile-all | `ARCH_HAS_GCOV_PROFILE_ALL`  arch supports whole-kernel GCOV code coverage profiling | * **ok**: arm, arm64, csky   microblaze, mips, powerpc   riscv, s390, sh, um, x86   xtensa * **TODO**: alpha, arc   hexagon, loongarch, m68k   nios2, openrisc, parisc   sparc |
| KASAN | `HAVE_ARCH_KASAN`  arch supports the KASAN runtime memory checker | * **ok**: arm, arm64   loongarch, powerpc, riscv   s390, um, x86, xtensa * **TODO**: alpha, arc, csky   hexagon, m68k, microblaze   mips, nios2, openrisc   parisc, sh, sparc |
| kcov | `ARCH_HAS_KCOV`  arch supports kcov for coverage-guided fuzzing | * **ok**: arm, arm64   loongarch, mips, powerpc   riscv, s390, um, x86   xtensa * **TODO**: alpha, arc, csky   hexagon, m68k, microblaze   nios2, openrisc, parisc, sh   sparc |
| kgdb | `HAVE_ARCH_KGDB`  arch supports the kGDB kernel debugger | * **ok**: arc, arm, arm64   hexagon, loongarch   microblaze, mips, nios2   parisc, powerpc, riscv, sh   sparc, x86 * **TODO**: alpha, csky, m68k   openrisc, s390, um, xtensa |
| kmemleak | `HAVE_DEBUG_KMEMLEAK`  arch supports the kernel memory leak detector | * **ok**: arc, arm, arm64   csky, loongarch, microblaze   mips, powerpc, riscv, s390   sh, sparc, um, x86, xtensa * **TODO**: alpha, hexagon   m68k, nios2, openrisc   parisc |
| kprobes | `HAVE_KPROBES`  arch supports live patched kernel probe | * **ok**: arc, arm, arm64   csky, loongarch, mips   parisc, powerpc, riscv   s390, sh, sparc, x86 * **TODO**: alpha, hexagon   m68k, microblaze, nios2   openrisc, um, xtensa |
| kprobes-on-ftrace | `HAVE_KPROBES_ON_FTRACE`  arch supports combined kprobes and ftrace live patching | * **ok**: csky, loongarch   parisc, powerpc, s390, x86 * **TODO**: alpha, arc, arm   arm64, hexagon, m68k   microblaze, mips, nios2   openrisc, riscv, sh, sparc   um, xtensa |
| kretprobes | `HAVE_KRETPROBES`  arch supports kernel function-return probes | * **ok**: arc, arm, arm64   csky, loongarch, mips   parisc, powerpc, riscv   s390, sh, sparc, x86 * **TODO**: alpha, hexagon   m68k, microblaze, nios2   openrisc, um, xtensa |
| optprobes | `HAVE_OPTPROBES`  arch supports live patched optprobes | * **ok**: arm, powerpc, x86 * **TODO**: alpha, arc, arm64   csky, hexagon, loongarch   m68k, microblaze, mips   nios2, openrisc, parisc   riscv, s390, sh, sparc, um   xtensa |
| stackprotector | `HAVE_STACKPROTECTOR`  arch supports compiler driven stack overflow protection | * **ok**: arm, arm64, csky   loongarch, mips, powerpc   riscv, s390, sh, x86   xtensa * **TODO**: alpha, arc   hexagon, m68k, microblaze   nios2, openrisc, parisc   sparc, um |
| uprobes | `ARCH_SUPPORTS_UPROBES`  arch supports live patched user probes | * **ok**: arm, arm64, csky   loongarch, mips, powerpc   riscv, s390, sparc, x86 * **TODO**: alpha, arc   hexagon, m68k, microblaze   nios2, openrisc, parisc, sh   um, xtensa |
| user-ret-profiler | `HAVE_USER_RETURN_NOTIFIER`  arch supports user-space return from system call profiler | * **ok**: x86 * **TODO**: alpha, arc, arm   arm64, csky, hexagon   loongarch, m68k, microblaze   mips, nios2, openrisc   parisc, powerpc, riscv   s390, sh, sparc, um, xtensa |

## Subsystem: io

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| dma-contiguous | `HAVE_DMA_CONTIGUOUS`  arch supports the DMA CMA (continuous memory allocator) | * **ok**: arm, arm64, csky   loongarch, microblaze, mips   riscv, s390, x86, xtensa * **TODO**: alpha, arc   hexagon, m68k, nios2   openrisc, parisc, powerpc   sh, sparc, um |

## Subsystem: locking

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| cmpxchg-local | `HAVE_CMPXCHG_LOCAL`  arch supports the `this_cpu_cmpxchg()` API | * **ok**: arm64, s390, x86 * **TODO**: alpha, arc, arm   csky, hexagon, loongarch   m68k, microblaze, mips   nios2, openrisc, parisc   powerpc, riscv, sh, sparc   um, xtensa |
| lockdep | `LOCKDEP_SUPPORT`  arch supports the runtime locking correctness debug facility | * **ok**: arc, arm, arm64   csky, hexagon, loongarch   microblaze, mips, openrisc   parisc, powerpc, riscv   s390, sh, sparc, um, x86   xtensa * **TODO**: alpha, m68k   nios2 |
| queued-rwlocks | `ARCH_USE_QUEUED_RWLOCKS`  arch supports queued rwlocks | * **ok**: arm64, csky   loongarch, mips, openrisc   powerpc, riscv, sparc, x86   xtensa * **TODO**: alpha, arc, arm   hexagon, m68k, microblaze   nios2, parisc, s390, sh, um |
| queued-spinlocks | `ARCH_USE_QUEUED_SPINLOCKS`  arch supports queued spinlocks | * **ok**: arm64, csky   loongarch, mips, openrisc   powerpc, riscv, sparc, x86   xtensa * **TODO**: alpha, arc, arm   hexagon, m68k, microblaze   nios2, parisc, s390, sh, um |

## Subsystem: perf

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| kprobes-event | `HAVE_REGS_AND_STACK_ACCESS_API`  arch supports kprobes with perf events | * **ok**: arc, arm, arm64   csky, hexagon, loongarch   mips, openrisc, parisc   powerpc, riscv, s390, sh   sparc, x86 * **TODO**: alpha, m68k   microblaze, nios2, um   xtensa |
| perf-regs | `HAVE_PERF_REGS`  arch supports perf events register access | * **ok**: arm, arm64, csky   loongarch, mips, parisc   powerpc, riscv, s390, x86 * **TODO**: alpha, arc   hexagon, m68k, microblaze   nios2, openrisc, sh, sparc   um, xtensa |
| perf-stackdump | `HAVE_PERF_USER_STACK_DUMP`  arch supports perf events stack dumps | * **ok**: arm, arm64, csky   loongarch, mips, parisc   powerpc, riscv, s390, x86 * **TODO**: alpha, arc   hexagon, m68k, microblaze   nios2, openrisc, sh, sparc   um, xtensa |

## Subsystem: sched

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| membarrier-sync-core | `ARCH_HAS_MEMBARRIER_SYNC_CORE`  arch supports core serializing membarrier | * **ok**: arm, arm64, powerpc   riscv, s390, x86 * **TODO**: alpha, arc, csky   hexagon, loongarch, m68k   microblaze, mips, nios2   openrisc, parisc, sh, sparc   um, xtensa |
| numa-balancing | `ARCH_SUPPORTS_NUMA_BALANCING`  arch supports NUMA balancing | * **ok**: arm64, loongarch   powerpc, riscv, s390, x86 * **TODO**: alpha, mips   sparc * **Not compatible**: arc   arm, csky, hexagon, m68k   microblaze, nios2, openrisc   parisc, sh, um, xtensa |

## Subsystem: seccomp

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| seccomp-filter | `HAVE_ARCH_SECCOMP_FILTER`  arch supports seccomp filters | * **ok**: alpha, arm, arm64   csky, loongarch, m68k, mips   parisc, powerpc, riscv   s390, sh, um, x86, xtensa * **TODO**: arc, hexagon   microblaze, nios2, openrisc   sparc |

## Subsystem: time

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| arch-tick-broadcast | `ARCH_HAS_TICK_BROADCAST`  arch provides `tick_broadcast()` | * **ok**: arm, arm64   loongarch, mips, powerpc   riscv, sh * **TODO**: alpha, arc, csky   hexagon, m68k, microblaze   nios2, openrisc, parisc   s390, sparc, um, x86   xtensa |
| clockevents | `!LEGACY_TIMER_TICK`  arch support generic clock events | * **ok**: alpha, arc, arm64   csky, hexagon, loongarch   microblaze, mips, nios2   openrisc, parisc, powerpc   riscv, s390, sh, sparc, um   x86, xtensa * **TODO**: arm, m68k |
| irq-time-acct | `HAVE_IRQ_TIME_ACCOUNTING`  arch supports precise IRQ time accounting | * **ok**: arm, arm64   loongarch, mips, powerpc   riscv, x86, xtensa * **TODO**: arc, csky   hexagon, m68k, microblaze   nios2, openrisc, sh, um * **Not compatible**: alpha   parisc, s390, sparc |
| user-context-tracking | `HAVE_CONTEXT_TRACKING_USER`  arch supports user context tracking for NO\_HZ\_FULL | * **ok**: arm, arm64, csky   loongarch, mips, powerpc   riscv, sparc, x86, xtensa * **TODO**: alpha, arc   hexagon, m68k, microblaze   nios2, openrisc, parisc   s390, sh, um |
| virt-cpuacct | `HAVE_VIRT_CPU_ACCOUNTING`  arch supports precise virtual CPU time accounting | * **ok**: alpha, arm, arm64   csky, loongarch, mips   parisc, powerpc, s390   sparc, x86, xtensa * **TODO**: arc, hexagon   m68k, microblaze, nios2   openrisc, riscv, sh, um |

## Subsystem: vm

| Feature | Kconfig / Description | Status per architecture |
| --- | --- | --- |
| batch-unmap-tlb-flush | `ARCH_WANT_BATCHED_UNMAP_TLB_FLUSH`  arch supports deferral of TLB flush until multiple pages are unmapped | * **ok**: arm64, riscv, x86 * **TODO**: alpha, arc, arm   csky, hexagon, loongarch   mips, parisc, powerpc, s390   sh, sparc, xtensa * **Not compatible**: m68k   microblaze, nios2, openrisc   um |
| ELF-ASLR | `ARCH_WANT_DEFAULT_TOPDOWN_MMAP_LAYOUT`  arch randomizes the stack, heap and binary images of ELF binaries | * **ok**: arm, arm64, csky   loongarch, mips, parisc   powerpc, riscv, s390, x86 * **TODO**: alpha, arc   hexagon, m68k, microblaze   nios2, openrisc, sh, sparc   um, xtensa |
| huge-vmap | `HAVE_ARCH_HUGE_VMAP`  arch supports the `arch_vmap_pud_supported()` and `arch_vmap_pmd_supported()` VM APIs | * **ok**: arm64, powerpc   riscv, x86 * **TODO**: alpha, arc, arm   csky, hexagon, loongarch   m68k, microblaze, mips   nios2, openrisc, parisc   s390, sh, sparc, um, xtensa |
| ioremap\_prot | `HAVE_IOREMAP_PROT`  arch has `ioremap_prot()` | * **ok**: arc, arm64   loongarch, mips, powerpc   riscv, s390, sh, x86 * **TODO**: alpha, arm, csky   hexagon, m68k, microblaze   nios2, openrisc, parisc   sparc, um, xtensa |
| pte\_special | `ARCH_HAS_PTE_SPECIAL`  arch supports the `pte_special()`/`pte_mkspecial()` VM APIs | * **ok**: arc, arm, arm64   loongarch, mips, parisc   powerpc, riscv, s390, sh   sparc, x86 * **TODO**: alpha, csky   hexagon, m68k, microblaze   nios2, openrisc, um, xtensa |
| THP | `HAVE_ARCH_TRANSPARENT_HUGEPAGE`  arch supports transparent hugepages | * **ok**: arc, arm, arm64   loongarch, mips, powerpc   riscv, s390, sparc, x86 * **TODO**: alpha, parisc * **Not compatible**: csky   hexagon, m68k, microblaze   nios2, openrisc, sh, um   xtensa |
