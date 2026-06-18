# Paravirt_ops

> 출처(원문): https://docs.kernel.org/virt/paravirt_ops.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Paravirt\_ops

Linux provides support for different hypervisor virtualization technologies.
Historically, different binary kernels would be required in order to support
different hypervisors; this restriction was removed with pv\_ops.
Linux pv\_ops is a virtualization API which enables support for different
hypervisors. It allows each hypervisor to override critical operations and
allows a single kernel binary to run on all supported execution environments
including native machine -- without any hypervisors.

pv\_ops provides a set of function pointers which represent operations
corresponding to low-level critical instructions and high-level
functionalities in various areas. pv\_ops allows for optimizations at run
time by enabling binary patching of the low-level critical operations
at boot time.

pv\_ops operations are classified into three categories:

* simple indirect call
  :   These operations correspond to high-level functionality where it is
      known that the overhead of indirect call isn’t very important.
* indirect call which allows optimization with binary patch
  :   Usually these operations correspond to low-level critical instructions. They
      are called frequently and are performance critical. The overhead is
      very important.
* a set of macros for hand written assembly code
  :   Hand written assembly codes (.S files) also need paravirtualization
      because they include sensitive instructions or some code paths in
      them are very performance critical.
