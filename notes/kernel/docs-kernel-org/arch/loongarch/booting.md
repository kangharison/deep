# 2.Booting Linux/LoongArch

> 출처(원문): https://docs.kernel.org/arch/loongarch/booting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. Booting Linux/LoongArch

Author:
:   Yanteng Si <[siyanteng@loongson.cn](mailto:siyanteng%40loongson.cn)>

Date:
:   18 Nov 2022

## 2.1. Information passed from BootLoader to kernel

LoongArch supports ACPI and FDT. The information that needs to be passed
to the kernel includes the memmap, the initrd, the command line, optionally
the ACPI/FDT tables, and so on.

The kernel is passed the following arguments on kernel\_entry :

> * a0 = efi\_boot: efi\_boot is a flag indicating whether
>   this boot environment is fully UEFI-compliant.
> * a1 = cmdline: cmdline is a pointer to the kernel command line.
> * a2 = systemtable: systemtable points to the EFI system table.
>   All pointers involved at this stage are in physical addresses.

## 2.2. Header of Linux/LoongArch kernel images

Linux/LoongArch kernel images are EFI images. Being PE files, they have
a 64-byte header structured like:

```
u32     MZ_MAGIC                /* "MZ", MS-DOS header */
u32     res0 = 0                /* Reserved */
u64     kernel_entry            /* Kernel entry point */
u64     _end - _text            /* Kernel image effective size */
u64     load_offset             /* Kernel image load offset from start of RAM */
u64     res1 = 0                /* Reserved */
u64     res2 = 0                /* Reserved */
u64     res3 = 0                /* Reserved */
u32     LINUX_PE_MAGIC          /* Magic number */
u32     pe_header - _head       /* Offset to the PE header */
```
