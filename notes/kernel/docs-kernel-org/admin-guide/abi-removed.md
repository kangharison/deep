# ABI removed symbols

> 출처(원문): https://docs.kernel.org/admin-guide/abi-removed.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ABI removed symbols

## Symbols under /sys/bus

|  |
| --- |
| **/sys/bus/nd/devices/regionX/nfit/ecc\_unit\_size** |

Defined on file [sysfs-bus-nfit](abi-removed-files.html#abi-file-removed-sysfs-bus-nfit)

(RO) Size of a write request to a DIMM that will not incur a
read-modify-write cycle at the memory controller.

When the nfit driver initializes it runs an ARS (Address Range
Scrub) operation across every pmem range. Part of that process
involves determining the ARS capabilities of a given address
range. One of the capabilities that is reported is the ‘Clear
Uncorrectable Error Range Length Unit Size’ (see: ACPI 6.2
section 9.20.7.4 Function Index 1 - Query ARS Capabilities).
This property indicates the boundary at which the NVDIMM may
need to perform read-modify-write cycles to maintain ECC (Error
Correcting Code) blocks.

## Symbols under /sys/class

|  |
| --- |
| **/sys/class/cxl/<afu>/afu\_err\_buf** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
AFU Error Buffer contents. The contents of this file are
application specific and depends on the AFU being used.
Applications interacting with the AFU can use this attribute
to know about the current error condition and take appropriate
action like logging the event etc.

|  |
| --- |
| **/sys/class/cxl/<afu>/api\_version** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Decimal value of the current version of the kernel/user API.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/api\_version\_compatible** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Decimal value of the lowest version of the userspace API
this kernel supports.

Users:
:   <https://github.com/ibm-capi/libcxl>

    AFU configuration records (eg. /sys/class/cxl/afu0.0/cr0):

    An AFU may optionally export one or more PCIe like configuration records, known
    as AFU configuration records, which will show up here (if present).

|  |
| --- |
| **/sys/class/cxl/<afu>/cr<config num>/class** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Hexadecimal value of the class code found in this AFU
configuration record.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/cr<config num>/config** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
This binary file provides raw access to the AFU configuration
record. The format is expected to match the either the standard
or extended configuration space defined by the PCIe
specification.

Users:
:   <https://github.com/ibm-capi/libcxl>

    Master contexts (eg. /sys/class/cxl/afu0.0m)

|  |
| --- |
| **/sys/class/cxl/<afu>/cr<config num>/device** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Hexadecimal value of the device ID found in this AFU
configuration record.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/cr<config num>/vendor** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Hexadecimal value of the vendor ID found in this AFU
configuration record.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/irqs\_max** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read/write
Decimal value of maximum number of interrupts that can be
requested by userspace. The default on probe is the maximum
that hardware can support (eg. 2037). Write values will limit
userspace applications to that many userspace interrupts. Must
be >= irqs\_min.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/irqs\_min** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Decimal value of the minimum number of interrupts that
userspace must request on a CXL\_START\_WORK ioctl. Userspace may
omit the num\_interrupts field in the START\_WORK IOCTL to get
this minimum automatically.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/mmio\_size** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Decimal value of the size of the MMIO space that may be mmapped
by userspace.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/mode** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read/write
The current mode the AFU is using. Will be one of the modes
given in modes\_supported. Writing will change the mode
provided that no user contexts are attached.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/modes\_supported** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
List of the modes this AFU supports. One per line.
Valid entries are: “dedicated\_process” and “afu\_directed”

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/prefault\_mode** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read/write
Set the mode for prefaulting in segments into the segment table
when performing the START\_WORK ioctl. Only applicable when
running under hashed page table mmu.
Possible values:

|  |  |
| --- | --- |
| none | No prefaulting (default) |
| work\_element\_descriptor | Treat the work element descriptor as an effective address and prefault what it points to. |
| all | all segments process calling START\_WORK maps. |

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>/reset** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

write only
Writing 1 here will reset the AFU provided there are not
contexts active on the AFU.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>m/mmio\_size** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Decimal value of the size of the MMIO space that may be mmapped
by userspace. This includes all slave contexts space also.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>m/pp\_mmio\_len** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Decimal value of the Per Process MMIO space length.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<afu>m/pp\_mmio\_off** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
(not in a guest)
Decimal value of the Per Process MMIO space offset.

Users:
:   <https://github.com/ibm-capi/libcxl>

    Card info (eg. /sys/class/cxl/card0)

|  |
| --- |
| **/sys/class/cxl/<card>/base\_image** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
(not in a guest)
Identifies the revision level of the base image for devices
that support loadable PSLs. For FPGAs this field identifies
the image contained in the on-adapter flash which is loaded
during the initial program load.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/caia\_version** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Identifies the CAIA Version the card implements.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/image\_loaded** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
(not in a guest)
Will return “user” or “factory” depending on the image loaded
onto the card.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/load\_image\_on\_perst** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read/write
(not in a guest)
Valid entries are “none”, “user”, and “factory”.
“none” means PERST will not cause image to be loaded to the
card. A power cycle is required to load the image.
“none” could be useful for debugging because the trace arrays
are preserved.

“user” and “factory” means PERST will cause either the user or
user or factory image to be loaded.
Default is to reload on PERST whichever image the card has
loaded.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/perst\_reloads\_same\_image** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read/write
(not in a guest)
Trust that when an image is reloaded via PERST, it will not
have changed.

|  |  |
| --- | --- |
| 0 | don’t trust, the image may be different (default) |
| 1 | trust that the image will not change. |

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/psl\_revision** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Identifies the revision level of the PSL.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/psl\_timebase\_synced** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Returns 1 if the psl timebase register is synchronized
with the core timebase register, 0 otherwise.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/reset** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

write only
Writing 1 will issue a PERST to card provided there are no
contexts active on any one of the card AFUs. This may cause
the card to reload the FPGA depending on load\_image\_on\_perst.
Writing -1 will do a force PERST irrespective of any active
contexts on the card AFUs.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/cxl/<card>/tunneled\_ops\_supported** |

Defined on file [sysfs-class-cxl](abi-removed-files.html#abi-file-removed-sysfs-class-cxl)

read only
Returns 1 if tunneled operations are supported in capi mode,
0 otherwise.

Users:
:   <https://github.com/ibm-capi/libcxl>

|  |
| --- |
| **/sys/class/rfkill/rfkill[0-9]+/claim** |

Defined on file [sysfs-class-rfkill](abi-removed-files.html#abi-file-removed-sysfs-class-rfkill)

This file was deprecated because there no longer was a way to
claim just control over a single rfkill instance.
This file was scheduled to be removed in 2012, and was removed
in 2016.
Values: 0: Kernel handles events

## Symbols under /sys/devices

|  |
| --- |
| **/sys/devices/system/machinecheck/machinecheckX/tolerant** |

Defined on file [sysfs-mce](abi-removed-files.html#abi-file-removed-sysfs-mce)

Unused and obsolete after the advent of recoverable machine
checks (see last sentence below) and those are present since
2010 (Nehalem).

Original description:

The entries appear for each CPU, but they are truly shared
between all CPUs.

Tolerance level. When a machine check exception occurs for a
non corrected machine check the kernel can take different
actions.

Since machine check exceptions can happen any time it is
sometimes risky for the kernel to kill a process because it
defies normal kernel locking rules. The tolerance level
configures how hard the kernel tries to recover even at some
risk of deadlock. Higher tolerant values trade potentially
better uptime with the risk of a crash or even corruption
(for tolerant >= 3).

|  |  |
| --- | --- |
| 0 | always panic on uncorrected errors, log corrected errors |
| 1 | panic or SIGBUS on uncorrected errors, log corrected errors |
| 2 | SIGBUS or log uncorrected errors, log corrected errors |
| 3 | never panic or SIGBUS, log all errors (for testing only) |

Default: 1

Note this only makes a difference if the CPU allows recovery
from a machine check exception. Current x86 CPUs generally
do not.

## Symbols under /sys/firmware

|  |
| --- |
| **/sys/firmware/efi/vars** |

Defined on file [sysfs-firmware-efi-vars](abi-removed-files.html#abi-file-removed-sysfs-firmware-efi-vars)

This directory exposed interfaces for interacting with
EFI variables. For more information on EFI variables,
see ‘Variable Services’ in the UEFI specification
(section 7.2 in specification version 2.3 Errata D).

The ‘efivars’ sysfs interface was removed in March of 2023,
after being considered deprecated no later than September
of 2020. Its functionality has been replaced by the
‘efivarfs’ filesystem.

## Symbols under /sys/fs

|  |
| --- |
| **/sys/fs/selinux/checkreqprot** |

Defined on file [sysfs-selinux-checkreqprot](abi-removed-files.html#abi-file-removed-sysfs-selinux-checkreqprot)

REMOVAL UPDATE: The SELinux checkreqprot functionality was removed in
March 2023, the original deprecation notice is shown below.

The selinuxfs “checkreqprot” node allows SELinux to be configured
to check the protection requested by userspace for mmap/mprotect
calls instead of the actual protection applied by the kernel.
This was a compatibility mechanism for legacy userspace and
for the READ\_IMPLIES\_EXEC personality flag. However, if set to
1, it weakens security by allowing mappings to be made executable
without authorization by policy. The default value of checkreqprot
at boot was changed starting in Linux v4.4 to 0 (i.e. check the
actual protection), and Android and Linux distributions have been
explicitly writing a “0” to /sys/fs/selinux/checkreqprot during
initialization for some time. Support for setting checkreqprot to 1
will be removed no sooner than June 2021, at which point the kernel
will always cease using checkreqprot internally and will always
check the actual protections being applied upon mmap/mprotect calls.
The checkreqprot selinuxfs node will remain for backward compatibility
but will discard writes of the “0” value and will reject writes of the
“1” value when this mechanism is removed.

|  |
| --- |
| **/sys/fs/selinux/disable** |

Defined on file [sysfs-selinux-disable](abi-removed-files.html#abi-file-removed-sysfs-selinux-disable)

REMOVAL UPDATE: The SELinux runtime disable functionality was removed
in March 2023, the original deprecation notice is shown below.

The selinuxfs “disable” node allows SELinux to be disabled at runtime
prior to a policy being loaded into the kernel. If disabled via this
mechanism, SELinux will remain disabled until the system is rebooted.

The preferred method of disabling SELinux is via the “selinux=0” boot
parameter, but the selinuxfs “disable” node was created to make it
easier for systems with primitive bootloaders that did not allow for
easy modification of the kernel command line. Unfortunately, allowing
for SELinux to be disabled at runtime makes it difficult to secure the
kernel’s LSM hooks using the “\_\_ro\_after\_init” feature.

Thankfully, the need for the SELinux runtime disable appears to be
gone, the default Kconfig configuration disables this selinuxfs node,
and only one of the major distributions, Fedora, supports disabling
SELinux at runtime. Fedora is in the process of removing the
selinuxfs “disable” node and once that is complete we will start the
slow process of removing this code from the kernel.

More information on /sys/fs/selinux/disable can be found under the
CONFIG\_SECURITY\_SELINUX\_DISABLE Kconfig option.

|  |
| --- |
| **/sys/fs/selinux/user** |

Defined on file [sysfs-selinux-user](abi-removed-files.html#abi-file-removed-sysfs-selinux-user)

The selinuxfs “user” node allows userspace to request a list
of security contexts that can be reached for a given SELinux
user from a given starting context. This was used by libselinux
when various login-style programs requested contexts for
users, but libselinux stopped using it in 2020.
Kernel support will be removed no sooner than Dec 2025.

## Symbols under /sys/kernel

|  |
| --- |
| **/sys/kernel/fadump\_release\_opalcore** |

Defined on file [sysfs-kernel-fadump\_release\_opalcore](abi-removed-files.html#abi-file-removed-sysfs-kernel-fadump-release-opalcore)

write only
The sysfs file is available when the system is booted to
collect the dump on OPAL based machine. It used to release
the memory used to collect the opalcore.

|  |
| --- |
| **/sys/kernel/uids/<uid>/cpu\_shares** |

Defined on file [sysfs-kernel-uids](abi-removed-files.html#abi-file-removed-sysfs-kernel-uids)

The /sys/kernel/uids/<uid>/cpu\_shares tunable is used
to set the cpu bandwidth a user is allowed. This is a
proportional value. What that means is that if there
are two users logged in, each with an equal number of
shares, then they will get equal CPU bandwidth. Another
example would be, if User A has shares = 1024 and user
B has shares = 2048, User B will get twice the CPU
bandwidth user A will. For more details refer
[CFS Scheduler](../scheduler/sched-design-CFS.html)

## Symbols under /sys/o2cb

|  |
| --- |
| **/sys/o2cb symlink** |

Defined on file [o2cb](abi-removed-files.html#abi-file-removed-o2cb)

This is a symlink: /sys/o2cb to /sys/fs/o2cb. The symlink is
removed when new versions of ocfs2-tools which know to look
in /sys/fs/o2cb are sufficiently prevalent. Don’t code new
software to look here, it should try /sys/fs/o2cb instead.

Users:
:   ocfs2-tools. It’s sufficient to mail proposed changes to
    [ocfs2-devel@lists.linux.dev](mailto:ocfs2-devel%40lists.linux.dev).

## devfs

|  |
| --- |
| **devfs** |

Defined on file [devfs](abi-removed-files.html#abi-file-removed-devfs)

devfs has been unmaintained for a number of years, has unfixable
races, contains a naming policy within the kernel that is
against the LSB, and can be replaced by using udev.

The files fs/devfs/*, include/linux/devfs\_fs*.h were removed,
along with the assorted devfs function calls throughout the
kernel tree.

## Symbols under dv1394

|  |
| --- |
| **dv1394 (a.k.a. "OHCI-DV I/O support" for FireWire)** |

Defined on file [dv1394](abi-removed-files.html#abi-file-removed-dv1394)

/dev/dv1394/\* were character device files, one for each FireWire
controller and for NTSC and PAL respectively, from which DV data
could be received by read() or transmitted by write(). A few
ioctl()s allowed limited control.
This special-purpose interface has been superseded by libraw1394 +
libiec61883 which are functionally equivalent, support HDV, and
transparently work on top of the newer firewire kernel drivers.

Users:
:   ffmpeg/libavformat (if configured for DV1394)

## ip\_queue

|  |
| --- |
| **ip\_queue** |

Defined on file [ip\_queue](abi-removed-files.html#abi-file-removed-ip-queue)

ip\_queue has been replaced by nfnetlink\_queue which provides
more advanced queueing mechanism to user-space. The ip\_queue
module was already announced to become obsolete years ago.

## Symbols under raw1394

|  |
| --- |
| **raw1394 (a.k.a. "Raw IEEE1394 I/O support" for FireWire)** |

Defined on file [raw1394](abi-removed-files.html#abi-file-removed-raw1394)

/dev/raw1394 was a character device file that allowed low-level
access to FireWire buses. Its major drawbacks were its inability
to implement sensible device security policies, and its low level
of abstraction that required userspace clients to duplicate much
of the kernel’s ieee1394 core functionality.

Replaced by /dev/fw\*, i.e. the <linux/firewire-cdev.h> ABI of
firewire-core.

Users:
:   libraw1394 (works with firewire-cdev too, transparent to library ABI
    users)

## tcp\_dma\_copybreak sysctl

|  |
| --- |
| **tcp\_dma\_copybreak sysctl** |

Defined on file [net\_dma](abi-removed-files.html#abi-file-removed-net-dma)

Formerly the lower limit, in bytes, of the size of socket reads
that will be offloaded to a DMA copy engine. Removed due to
coherency issues of the cpu potentially touching the buffers
while dma is in flight.

## video1394 (a.k.a. “OHCI-1394 Video support” for FireWire)

|  |
| --- |
| **video1394 (a.k.a. "OHCI-1394 Video support" for FireWire)** |

Defined on file [video1394](abi-removed-files.html#abi-file-removed-video1394)

/dev/video1394/\* were character device files, one for each FireWire
controller, which were used for isochronous I/O. It was added as an
alternative to raw1394’s isochronous I/O functionality which had
performance issues in its first generation. Any video1394 user had
to use raw1394 + libraw1394 too because video1394 did not provide
asynchronous I/O for device discovery and configuration.

Replaced by /dev/fw\*, i.e. the <linux/firewire-cdev.h> ABI of
firewire-core.

Users:
:   libdc1394 (works with firewire-cdev too, transparent to library ABI
    users)
