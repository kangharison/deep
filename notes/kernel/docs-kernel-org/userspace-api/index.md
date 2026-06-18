# The Linux kernel user-space API guide

> 출처(원문): https://docs.kernel.org/userspace-api/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux kernel user-space API guide

While much of the kernel’s user-space API is documented elsewhere
(particularly in the [man-pages](https://www.kernel.org/doc/man-pages/) project), some user-space information can
also be found in the kernel tree itself. This manual is intended to be the
place where this information is gathered.

## System calls

* [unshare system call](unshare.html)
* [futex2](futex2.html)
* [eBPF Userspace API](ebpf/index.html)
* [IOCTLs](ioctl/index.html)
* [Introduction of mseal](mseal.html)
* [Restartable Sequences](rseq.html)

## Security-related interfaces

* [No New Privileges Flag](no_new_privs.html)
* [Seccomp BPF (SECure COMPuting with filters)](seccomp_filter.html)
* [Landlock: unprivileged access control](landlock.html)
* [Linux Security Modules](lsm.html)
* [Introduction of non-executable mfd](mfd_noexec.html)
* [Speculation Control](spec_ctrl.html)
* [TEE (Trusted Execution Environment) Userspace API](tee.html)
* [Executability check](check_exec.html)

## Devices and I/O

* [OpenCAPI (Open Coherent Accelerator Processor Interface)](accelerators/ocxl.html)
* [Allocating dma-buf using heaps](dma-buf-heaps.html)
* [Exchanging pixel buffers](dma-buf-alloc-exchange.html)
* [Firmware Control (FWCTL) Userspace API](fwctl/index.html)
* [GPIO](gpio/index.html)
* [IOMMUFD](iommufd.html)
* [Linux Media Infrastructure userspace API](media/index.html)
* [Dell Systems Management Base Driver](dcdbas.html)
* [VDUSE - “vDPA Device in Userspace”](vduse.html)
* [ISA Plug & Play support](isapnp.html)

## Everything else

* [Linux-specific ELF idiosyncrasies](ELF.html)
* [Live Update uAPI](liveupdate.html)
* [Netlink Handbook](netlink/index.html)
* [Platform Profile Selection (e.g. /sys/firmware/acpi/platform\_profile)](sysfs-platform_profile.html)
* [VDUSE - “vDPA Device in Userspace”](vduse.html)
* [futex2](futex2.html)
* [Perf ring buffer](perf_ring_buffer.html)
* [NT synchronization primitive driver](ntsync.html)
