# 1.BMIPS DeviceTree Booting

> 출처(원문): https://docs.kernel.org/arch/mips/booting.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1. BMIPS DeviceTree Booting

> Some bootloaders only support a single entry point, at the start of the
> kernel image. Other bootloaders will jump to the ELF start address.
> Both schemes are supported; CONFIG\_BOOT\_RAW=y and CONFIG\_NO\_EXCEPT\_FILL=y,
> so the first instruction immediately jumps to `kernel_entry()`.
>
> Similar to the arch/arm case (b), a DT-aware bootloader is expected to
> set up the following registers:
>
> > a0 : 0
> >
> > a1 : 0xffffffff
> >
> > a2 : Physical pointer to the device tree block (defined in chapter
> > II) in RAM. The device tree can be located anywhere in the first
> > 512MB of the physical address space (0x00000000 - 0x1fffffff),
> > aligned on a 64 bit boundary.
>
> Legacy bootloaders do not use this convention, and they do not pass in a
> DT block. In this case, Linux will look for a builtin DTB, selected via
> CONFIG\_DT\_\*.
>
> This convention is defined for 32-bit systems only, as there are not
> currently any 64-bit BMIPS implementations.
