# The Linux kernel user’s and administrator’s guide

> 출처(원문): https://docs.kernel.org/admin-guide/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux kernel user’s and administrator’s guide

The following is a collection of user-oriented documents that have been
added to the kernel over time. There is, as yet, little overall order or
organization here — this material was not written to be a single, coherent
document! With luck things will improve quickly over time.

## General guides to kernel administration

This initial section contains overall information, including the README
file describing the kernel as a whole, documentation on kernel parameters,
etc.

* [Linux kernel release 6.x <http://kernel.org/>](README.html)
* [Linux allocated devices (4.x+ version)](devices.html)
* [Feature status on all architectures](features.html)

A big part of the kernel’s administrative interface is the /proc and sysfs
virtual filesystems; these documents describe how to interact with tem

* [Rules on how to access information in sysfs](sysfs-rules.html)
* [Documentation for /proc/sys](sysctl/index.html)
* [How CPU topology info is exported via sysfs](cputopology.html)
* [Linux ABI description](abi.html)

Security-related documentation:

* [Hardware vulnerabilities](hw-vuln/index.html)
* [Linux Security Module Usage](LSM/index.html)
* [Perf events and tool security](perf-security.html)

## Booting the kernel

* [Boot Configuration](bootconfig.html)
* [The kernel’s command-line parameters](kernel-parameters.html)
* [The EFI Boot Stub](efi-stub.html)
* [Using the initial RAM disk (initrd)](initrd.html)

## Tracking down and identifying problems

Here is a set of documents aimed at users who are trying to track down
problems and bugs in particular.

* [Reporting issues](reporting-issues.html)
* [Reporting regressions](reporting-regressions.html)
* [How to quickly build a trimmed Linux kernel](quickly-build-trimmed-linux.html)
* [How to verify bugs and bisect regressions](verify-bugs-and-bisect-regressions.html)
* [Bug hunting](bug-hunting.html)
* [Bisecting a regression](bug-bisect.html)
* [Tainted kernels](tainted-kernels.html)
* [Ramoops oops/panic logger](ramoops.html)
* [Dynamic debug](dynamic-debug-howto.html)
* [Explaining the “No working init found.” boot hang message](init.html)
* [Documentation for Kdump - The kexec-based Crash Dumping Solution](kdump/index.html)
* [Performance monitor support](perf/index.html)
* [pstore block oops/panic logger](pstore-blk.html)
* [Clearing WARN\_ONCE](clearing-warn-once.html)
* [Reducing OS jitter due to per-cpu kthreads](kernel-per-CPU-kthreads.html)
* [Softlockup detector and hardlockup detector (aka nmi\_watchdog)](lockup-watchdogs.html)
* [Reliability, Availability and Serviceability (RAS)](RAS/main.html)
* [Error decoding](RAS/error-decoding.html)
* [Address translation](RAS/address-translation.html)
* [Linux Magic System Request Key Hacks](sysrq.html)

## Core-kernel subsystems

These documents describe core-kernel administration interfaces that are
likely to be of interest on almost any system.

* [Control Group v2](cgroup-v2.html)
* [Control Groups version 1](cgroup-v1/index.html)
* [CPU Isolation](cpu-isolation.html)
* [CPU load](cpu-load.html)
* [Memory Management](mm/index.html)
* [Kernel module signing facility](module-signing.html)
* [Namespaces](namespaces/index.html)
* [Numa policy hit/miss statistics](numastat.html)
* [Power Management](pm/index.html)
* [Syscall User Dispatch](syscall-user-dispatch.html)

Support for non-native binary formats. Note that some of these
documents are ... old ...

* [Kernel Support for miscellaneous Binary Formats (binfmt\_misc)](binfmt-misc.html)
* [Java(tm) Binary Kernel Support for Linux v1.03](java.html)
* [Mono(tm) Binary Kernel Support for Linux](mono.html)

## Block-layer and filesystem administration

* [A block layer cache (bcache)](bcache.html)
* [The Android binderfs Filesystem](binderfs.html)
* [Block Devices](blockdev/index.html)
* [CIFS](cifs/index.html)
* [Device Mapper](device-mapper/index.html)
* [ext4 General Information](ext4.html)
* [File system Monitoring with fanotify](filesystem-monitoring.html)
* [NFS](nfs/index.html)
* [I/O statistics fields](iostats.html)
* [IBM’s Journaled File System (JFS) for Linux](jfs.html)
* [RAID arrays](md.html)
* [Using UFS](ufs.html)
* [The SGI XFS Filesystem](xfs.html)

## Device-specific guides

How to configure your hardware within your Linux system.

* [ACPI Support](acpi/index.html)
* [ATA over Ethernet (AoE)](aoe/index.html)
* [Auxiliary Display Support](auxdisplay/index.html)
* [Linux Braille Console](braille-console.html)
* [btmrvl driver](btmrvl.html)
* [Dell Remote BIOS Update driver (dell\_rbu)](dell_rbu.html)
* [EDID](edid.html)
* [GPIO](gpio/index.html)
* [Hardware random number generators](hw_random.html)
* [Laptop Drivers](laptops/index.html)
* [Parallel port LCD/Keypad Panel support](lcd-panel-cgram.html)
* [Media subsystem admin and user guide](media/index.html)
* [Linux NVMe multipath](nvme-multipath.html)
* [Parport](parport.html)
* [Linux Plug and Play Documentation](pnp.html)
* [RapidIO Subsystem Guide](rapidio.html)
* [Real Time Clock (RTC) Drivers for Linux](rtc.html)
* [Linux Serial Console](serial-console.html)
* [Video Mode Selection Support 2.13](svga.html)
* [Thermal Subsystem](thermal/index.html)
* [USB4 and Thunderbolt](thunderbolt.html)
* [Software cursor for VGA](vga-softcursor.html)
* [Video Output Switcher Control](video-output.html)

## Workload analysis

This is the beginning of a section with information of interest to
application developers and system integrators doing analysis of the
Linux kernel for safety critical applications. Documents supporting
analysis of kernel interactions with applications, and key kernel
subsystems expectations will be found here.

* [Discovering Linux kernel subsystems used by a workload](workload-tracing.html)

## Everything else

A few hard-to-categorize and generally obsolete documents.

* [LDM - Logical Disk Manager (Dynamic Disks)](ldm.html)
* [Unicode support](unicode.html)
