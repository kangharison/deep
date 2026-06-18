# 3.Feature status on mips architecture

> 출처(원문): https://docs.kernel.org/translations/zh_TW/arch/mips/features.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

Warning

此文件的目的是爲讓中文讀者更容易閱讀和理解，而不是作爲一個分支。因此，
如果您對此文件有任何意見或改動，請先嘗試更新原始英文文件。如果要更改或
修正某處翻譯文件，請將意見或補丁發送給維護者（聯繫方式見下）。

Note

如果您發現本文檔與原始文件有任何不同或者有翻譯問題，請聯繫該文件的譯者，
或者發送電子郵件給胡皓文以獲取幫助：<[2023002089@link.tyut.edu.cn](mailto:2023002089%40link.tyut.edu.cn)>。

Original:
:   [Feature status on mips architecture](../../../../arch/mips/features.html)

翻譯:
:   司延騰 Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

# 3. Feature status on mips architecture

| Subsystem | Feature | Kconfig | Status | Description |
| --- | --- | --- | --- | --- |
| core | cBPF-JIT | HAVE\_CBPF\_JIT | ok | arch supports cBPF JIT optimizations |
| core | eBPF-JIT | HAVE\_EBPF\_JIT | ok | arch supports eBPF JIT optimizations |
| core | generic-idle-thread | GENERIC\_SMP\_IDLE\_THREAD | ok | arch makes use of the generic SMP idle thread facility |
| core | jump-labels | HAVE\_ARCH\_JUMP\_LABEL | ok | arch supports live patched, high efficiency branches |
| core | mseal-system-mappings | ARCH\_SUPPORTS\_MSEAL\_SYSTEM\_MAPPINGS | TODO | arch supports mseal system mappings |
| core | thread-info-in-task | THREAD\_INFO\_IN\_TASK | TODO | arch makes use of the core kernel facility to embed thread\_info in task\_struct |
| core | tracehook | HAVE\_ARCH\_TRACEHOOK | ok | arch supports tracehook (ptrace) register handling APIs |
| debug | debug-vm-pgtable | ARCH\_HAS\_DEBUG\_VM\_PGTABLE | TODO | arch supports pgtable tests for semantics compliance |
| debug | gcov-profile-all | ARCH\_HAS\_GCOV\_PROFILE\_ALL | ok | arch supports whole-kernel GCOV code coverage profiling |
| debug | KASAN | HAVE\_ARCH\_KASAN | TODO | arch supports the KASAN runtime memory checker |
| debug | kcov | ARCH\_HAS\_KCOV | ok | arch supports kcov for coverage-guided fuzzing |
| debug | kgdb | HAVE\_ARCH\_KGDB | ok | arch supports the kGDB kernel debugger |
| debug | kmemleak | HAVE\_DEBUG\_KMEMLEAK | ok | arch supports the kernel memory leak detector |
| debug | kprobes | HAVE\_KPROBES | ok | arch supports live patched kernel probe |
| debug | kprobes-on-ftrace | HAVE\_KPROBES\_ON\_FTRACE | TODO | arch supports combined kprobes and ftrace live patching |
| debug | kretprobes | HAVE\_KRETPROBES | ok | arch supports kernel function-return probes |
| debug | optprobes | HAVE\_OPTPROBES | TODO | arch supports live patched optprobes |
| debug | stackprotector | HAVE\_STACKPROTECTOR | ok | arch supports compiler driven stack overflow protection |
| debug | uprobes | ARCH\_SUPPORTS\_UPROBES | ok | arch supports live patched user probes |
| debug | user-ret-profiler | HAVE\_USER\_RETURN\_NOTIFIER | TODO | arch supports user-space return from system call profiler |
| io | dma-contiguous | HAVE\_DMA\_CONTIGUOUS | ok | arch supports the DMA CMA (continuous memory allocator) |
| locking | cmpxchg-local | HAVE\_CMPXCHG\_LOCAL | TODO | arch supports the `this_cpu_cmpxchg()` API |
| locking | lockdep | LOCKDEP\_SUPPORT | ok | arch supports the runtime locking correctness debug facility |
| locking | queued-rwlocks | ARCH\_USE\_QUEUED\_RWLOCKS | ok | arch supports queued rwlocks |
| locking | queued-spinlocks | ARCH\_USE\_QUEUED\_SPINLOCKS | ok | arch supports queued spinlocks |
| perf | kprobes-event | HAVE\_REGS\_AND\_STACK\_ACCESS\_API | ok | arch supports kprobes with perf events |
| perf | perf-regs | HAVE\_PERF\_REGS | ok | arch supports perf events register access |
| perf | perf-stackdump | HAVE\_PERF\_USER\_STACK\_DUMP | ok | arch supports perf events stack dumps |
| sched | membarrier-sync-core | ARCH\_HAS\_MEMBARRIER\_SYNC\_CORE | TODO | arch supports core serializing membarrier |
| sched | numa-balancing | ARCH\_SUPPORTS\_NUMA\_BALANCING | TODO | arch supports NUMA balancing |
| seccomp | seccomp-filter | HAVE\_ARCH\_SECCOMP\_FILTER | ok | arch supports seccomp filters |
| time | arch-tick-broadcast | ARCH\_HAS\_TICK\_BROADCAST | ok | arch provides `tick_broadcast()` |
| time | clockevents | !LEGACY\_TIMER\_TICK | ok | arch support generic clock events |
| time | irq-time-acct | HAVE\_IRQ\_TIME\_ACCOUNTING | ok | arch supports precise IRQ time accounting |
| time | user-context-tracking | HAVE\_CONTEXT\_TRACKING\_USER | ok | arch supports user context tracking for NO\_HZ\_FULL |
| time | virt-cpuacct | HAVE\_VIRT\_CPU\_ACCOUNTING | ok | arch supports precise virtual CPU time accounting |
| vm | batch-unmap-tlb-flush | ARCH\_WANT\_BATCHED\_UNMAP\_TLB\_FLUSH | TODO | arch supports deferral of TLB flush until multiple pages are unmapped |
| vm | ELF-ASLR | ARCH\_WANT\_DEFAULT\_TOPDOWN\_MMAP\_LAYOUT | ok | arch randomizes the stack, heap and binary images of ELF binaries |
| vm | huge-vmap | HAVE\_ARCH\_HUGE\_VMAP | TODO | arch supports the `arch_vmap_pud_supported()` and `arch_vmap_pmd_supported()` VM APIs |
| vm | ioremap\_prot | HAVE\_IOREMAP\_PROT | ok | arch has `ioremap_prot()` |
| vm | pte\_special | ARCH\_HAS\_PTE\_SPECIAL | ok | arch supports the `pte_special()`/`pte_mkspecial()` VM APIs |
| vm | THP | HAVE\_ARCH\_TRANSPARENT\_HUGEPAGE | ok | arch supports transparent hugepages |
