# Kernel Memory Layout on ARM Linux

> 출처(원문): https://docs.kernel.org/arch/arm/memory.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel Memory Layout on ARM Linux

> Russell King <[rmk@arm.linux.org.uk](mailto:rmk%40arm.linux.org.uk)>
>
> > November 17, 2005 (2.6.15)

This document describes the virtual memory layout which the Linux
kernel uses for ARM processors. It indicates which regions are
free for platforms to use, and which are used by generic code.

The ARM CPU is capable of addressing a maximum of 4GB virtual memory
space, and this must be shared between user space processes, the
kernel, and hardware devices.

As the ARM architecture matures, it becomes necessary to reserve
certain regions of VM space for use for new facilities; therefore
this document may reserve more VM space over time.

| Start | End | Use |
| --- | --- | --- |
| ffff8000 | ffffffff | copy\_user\_page / clear\_user\_page use. For SA11xx and Xscale, this is used to setup a minicache mapping. |
| ffff4000 | ffffffff | cache aliasing on ARMv6 and later CPUs. |
| ffff1000 | ffff7fff | Reserved. Platforms must not use this address range. |
| ffff0000 | ffff0fff | CPU vector page. The CPU vectors are mapped here if the CPU supports vector relocation (control register V bit.) |
| fffe0000 | fffeffff | XScale cache flush area. This is used in proc-xscale.S to flush the whole data cache. (XScale does not have TCM.) |
| fffe8000 | fffeffff | DTCM mapping area for platforms with DTCM mounted inside the CPU. |
| fffe0000 | fffe7fff | ITCM mapping area for platforms with ITCM mounted inside the CPU. |
| ffc80000 | ffefffff | Fixmap mapping region. Addresses provided by `fix_to_virt()` will be located here. |
| ffc00000 | ffc7ffff | Guard region |
| ff800000 | ffbfffff | Permanent, fixed read-only mapping of the firmware provided DT blob |
| fee00000 | feffffff | Mapping of PCI I/O space. This is a static mapping within the vmalloc space. |
| VMALLOC\_START | VMALLOC\_END-1 | [`vmalloc()`](../../core-api/mm-api.html#c.vmalloc "vmalloc") / [`ioremap()`](../../driver-api/device-io.html#c.ioremap "ioremap") space. Memory returned by vmalloc/ioremap will be dynamically placed in this region. Machine specific static mappings are also located here through `iotable_init()`. VMALLOC\_START is based upon the value of the high\_memory variable, and VMALLOC\_END is equal to 0xff800000. |
| PAGE\_OFFSET | high\_memory-1 | Kernel direct-mapped RAM region. This maps the platforms RAM, and typically maps all platform RAM in a 1:1 relationship. |
| PKMAP\_BASE | PAGE\_OFFSET-1 | Permanent kernel mappings One way of mapping HIGHMEM pages into kernel space. |
| MODULES\_VADDR | MODULES\_END-1 | Kernel module space Kernel modules inserted via insmod are placed here using dynamic mappings. |
| TASK\_SIZE | MODULES\_VADDR-1 | KASAn shadow memory when KASan is in use. The range from MODULES\_VADDR to the top of the memory is shadowed here with 1 bit per byte of memory. |
| 00001000 | TASK\_SIZE-1 | User space mappings Per-thread mappings are placed here via the mmap() system call. |
| 00000000 | 00000fff | CPU vector page / null pointer trap CPUs which do not support vector remapping place their vector page here. NULL pointer dereferences by both the kernel and user space are also caught via this mapping. |

Please note that mappings which collide with the above areas may result
in a non-bootable kernel, or may cause the kernel to (eventually) panic
at run time.

Since future CPUs may impact the kernel mapping layout, user programs
must not access any memory which is not mapped inside their 0x0001000
to TASK\_SIZE address range. If they wish to access these areas, they
must set up their own mappings using open() and mmap().
