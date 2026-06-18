# Kernel initialisation parameters on ARM Linux

> 출처(원문): https://docs.kernel.org/arch/arm/setup.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel initialisation parameters on ARM Linux

The following document describes the kernel initialisation parameter
structure, otherwise known as ‘`struct param_struct`’ which is used
for most ARM Linux architectures.

This structure is used to pass initialisation parameters from the
kernel loader to the Linux kernel proper, and may be short lived
through the kernel initialisation process. As a general rule, it
should not be referenced outside of arch/arm/kernel/setup.c:`setup_arch()`.

There are a lot of parameters listed in there, and they are described
below:

> page\_size
> :   This parameter must be set to the page size of the machine, and
>     will be checked by the kernel.
>
> nr\_pages
> :   This is the total number of pages of memory in the system. If
>     the memory is banked, then this should contain the total number
>     of pages in the system.
>
>     If the system contains separate VRAM, this value should not
>     include this information.
>
> ramdisk\_size
> :   This is now obsolete, and should not be used.
>
> flags
> :   Various kernel flags, including:
>
>     > |  |  |
>     > | --- | --- |
>     > | bit 0 | 1 = mount root read only |
>     > | bit 1 | unused |
>     > | bit 2 | 0 = load ramdisk |
>     > | bit 3 | 0 = prompt for ramdisk |
>
> rootdev
> :   major/minor number pair of device to mount as the root filesystem.
>
> video\_num\_cols / video\_num\_rows
> :   These two together describe the character size of the dummy console,
>     or VGA console character size. They should not be used for any other
>     purpose.
>
>     It’s generally a good idea to set these to be either standard VGA, or
>     the equivalent character size of your fbcon display. This then allows
>     all the bootup messages to be displayed correctly.
>
> video\_x / video\_y
> :   This describes the character position of cursor on VGA console, and
>     is otherwise unused. (should not be used for other console types, and
>     should not be used for other purposes).
>
> memc\_control\_reg
> :   MEMC chip control register for Acorn Archimedes and Acorn A5000
>     based machines. May be used differently by different architectures.
>
> sounddefault
> :   Default sound setting on Acorn machines. May be used differently by
>     different architectures.
>
> adfsdrives
> :   Number of ADFS/MFM disks. May be used differently by different
>     architectures.
>
> bytes\_per\_char\_h / bytes\_per\_char\_v
> :   These are now obsolete, and should not be used.
>
> pages\_in\_bank[4]
> :   Number of pages in each bank of the systems memory (used for RiscPC).
>     This is intended to be used on systems where the physical memory
>     is non-contiguous from the processors point of view.
>
> pages\_in\_vram
> :   Number of pages in VRAM (used on Acorn RiscPC). This value may also
>     be used by loaders if the size of the video RAM can’t be obtained
>     from the hardware.
>
> initrd\_start / initrd\_size
> :   This describes the kernel virtual start address and size of the
>     initial ramdisk.
>
> rd\_start
> :   Start address in sectors of the ramdisk image on a floppy disk.
>
> system\_rev
> :   system revision number.
>
> system\_serial\_low / system\_serial\_high
> :   system 64-bit serial number
>
> mem\_fclk\_21285
> :   The speed of the external oscillator to the 21285 (footbridge),
>     which control’s the speed of the memory bus, timer & serial port.
>     Depending upon the speed of the cpu its value can be between
>     0-66 MHz. If no params are passed or a value of zero is passed,
>     then a value of 50 Mhz is the default on 21285 architectures.
>
> paths[8][128]
> :   These are now obsolete, and should not be used.
>
> commandline
> :   Kernel command line parameters. Details can be found elsewhere.
