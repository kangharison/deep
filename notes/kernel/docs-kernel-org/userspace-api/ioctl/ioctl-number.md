# Ioctl Numbers

> 출처(원문): https://docs.kernel.org/userspace-api/ioctl/ioctl-number.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ioctl Numbers

19 October 1999

Michael Elizabeth Chastain
<[mec@shout.net](mailto:mec%40shout.net)>

If you are adding new ioctl’s to the kernel, you should use the \_IO
macros defined in <linux/ioctl.h>:

> | macro | parameters |
> | --- | --- |
> | \_IO | none |
> | \_IOW | write (read from userspace) |
> | \_IOR | read (write to userspace) |
> | \_IOWR | write and read |

‘Write’ and ‘read’ are from the user’s point of view, just like the
system calls ‘write’ and ‘read’. For example, a SET\_FOO ioctl would
be \_IOW, although the kernel would actually read data from user space;
a GET\_FOO ioctl would be \_IOR, although the kernel would actually write
data to user space.

The first argument to the macros is an identifying letter or number from
the table below. Because of the large number of drivers, many drivers
share a partial letter with other drivers.

If you are writing a driver for a new device and need a letter, pick an
unused block with enough room for expansion: 32 to 256 ioctl commands
should suffice. You can register the block by patching this file and
submitting the patch through [usual patch submission process](../../process/submitting-patches.html).

The second argument is a sequence number to distinguish ioctls from each
other. The third argument (not applicable to \_IO) is the type of the data
going into the kernel or coming out of the kernel (e.g. ‘int’ or
‘`struct foo`’).

Note

Do NOT use sizeof(arg) as the third argument as this results in your
ioctl thinking it passes an argument of type size\_t.

Some devices use their major number as the identifier; this is OK, as
long as it is unique. Some devices are irregular and don’t follow any
convention at all.

Following this convention is good because:

1. Keeping the ioctl’s globally unique helps error checking:
   if a program calls an ioctl on the wrong device, it will get an
   error rather than some unexpected behaviour.
2. The ‘strace’ build procedure automatically finds ioctl numbers
   defined with the macros.
3. ‘strace’ can decode numbers back into useful names when the
   numbers are unique.
4. People looking for ioctls can grep for them more easily when
   this convention is used to define the ioctl numbers.
5. When following the convention, the driver code can use generic
   code to copy the parameters between user and kernel space.

This table lists ioctls visible from userland, excluding ones from
drivers/staging/.

| Code | Seq# (hex) | Include File | Comments |
| --- | --- | --- | --- |
| 0x00 | 00-1F | linux/fs.h | conflict! |
| 0x00 | 00-1F | scsi/scsi\_ioctl.h | conflict! |
| 0x00 | 00-1F | linux/fb.h | conflict! |
| 0x00 | 00-1F | linux/wavefront.h | conflict! |
| 0x02 | all | linux/fd.h |  |
| 0x03 | all | linux/hdreg.h |  |
| 0x04 | D2-DC | linux/umsdos\_fs.h | Dead since 2.6.11, but don’t reuse these. |
| 0x06 | all | linux/lp.h |  |
| 0x07 | 9F-D0 | linux/vmw\_vmci\_defs.h, uapi/linux/vm\_sockets.h |  |
| 0x09 | all | linux/raid/md\_u.h |  |
| 0x10 | 00-0F | drivers/char/s390/vmcp.h |  |
| 0x10 | 10-1F | arch/s390/include/uapi/sclp\_ctl.h |  |
| 0x10 | 20-2F | arch/s390/include/uapi/asm/hypfs.h |  |
| 0x12 | all | linux/fs.h linux/blkpg.h linux/blkzoned.h linux/blk-crypto.h | BLK\* ioctls |
| 0x15 | all | linux/fs.h | FS\_IOC\_\* ioctls |
| 0x1b | all |  | InfiniBand Subsystem <<http://infiniband.sourceforge.net/>> |
| 0x20 | all | drivers/cdrom/cm206.h |  |
| 0x22 | all | scsi/sg.h |  |
| 0x3E | 00-0F | linux/counter.h | <[mailto:linux-iio@vger.kernel.org](mailto:linux-iio%40vger.kernel.org)> |
| ‘!’ | 00-1F | uapi/linux/seccomp.h |  |
| ‘#’ | 00-3F |  | IEEE 1394 Subsystem Block for the entire subsystem |
| ‘$’ | 00-0F | linux/perf\_counter.h, linux/perf\_event.h |  |
| ‘%’ | 00-0F | include/uapi/linux/stm.h | System Trace Module subsystem <[mailto:alexander.shishkin@linux.intel.com](mailto:alexander.shishkin%40linux.intel.com)> |
| ‘&’ | 00-07 | drivers/firewire/nosy-user.h |  |
| ‘\*’ | 00-1F | uapi/linux/user\_events.h | User Events Subsystem <[mailto:linux-trace-kernel@vger.kernel.org](mailto:linux-trace-kernel%40vger.kernel.org)> |
| ‘1’ | 00-1F | linux/timepps.h | PPS kit from Ulrich Windl <<ftp://ftp.de.kernel.org/pub/linux/daemons/ntp/PPS/>> |
| ‘2’ | 01-04 | linux/i2o.h |  |
| ‘3’ | 00-0F | drivers/s390/char/raw3270.h | conflict! |
| ‘3’ | 00-1F | linux/suspend\_ioctls.h, kernel/power/user.c | conflict! |
| ‘8’ | all |  | SNP8023 advanced NIC card <[mailto:mcr@solidum.com](mailto:mcr%40solidum.com)> |
| ‘;’ | 64-7F | linux/vfio.h |  |
| ‘;’ | 80-FF | linux/iommufd.h |  |
| ‘=’ | 00-3f | uapi/linux/ptp\_clock.h | <[mailto:richardcochran@gmail.com](mailto:richardcochran%40gmail.com)> |
| ‘@’ | 00-0F | linux/radeonfb.h | conflict! |
| ‘@’ | 00-0F | drivers/video/aty/aty128fb.c | conflict! |
| ‘A’ | 00-1F | linux/apm\_bios.h | conflict! |
| ‘A’ | 00-0F | linux/agpgart.h, drivers/char/agp/compat\_ioctl.h | conflict! |
| ‘A’ | 00-7F | sound/asound.h | conflict! |
| ‘B’ | 00-1F | linux/cciss\_ioctl.h | conflict! |
| ‘B’ | 00-0F | include/linux/pmu.h | conflict! |
| ‘B’ | C0-FF | advanced bbus | <[mailto:maassen@uni-freiburg.de](mailto:maassen%40uni-freiburg.de)> |
| ‘B’ | 00-0F | xen/xenbus\_dev.h | conflict! |
| ‘C’ | all | linux/soundcard.h | conflict! |
| ‘C’ | 01-2F | linux/capi.h | conflict! |
| ‘C’ | F0-FF | drivers/net/wan/cosa.h | conflict! |
| ‘D’ | all | arch/s390/include/asm/dasd.h |  |
| ‘D’ | 40-5F | drivers/scsi/dpt/dtpi\_ioctl.h | Dead since 2022 |
| ‘D’ | 05 | drivers/scsi/pmcraid.h |  |
| ‘E’ | all | linux/input.h | conflict! |
| ‘E’ | 00-0F | xen/evtchn.h | conflict! |
| ‘F’ | all | linux/fb.h | conflict! |
| ‘F’ | 01-02 | drivers/scsi/pmcraid.h | conflict! |
| ‘F’ | 20 | drivers/video/fsl-diu-fb.h | conflict! |
| ‘F’ | 20 | linux/ivtvfb.h | conflict! |
| ‘F’ | 20 | linux/matroxfb.h | conflict! |
| ‘F’ | 20 | drivers/video/aty/atyfb\_base.c | conflict! |
| ‘F’ | 00-0F | video/da8xx-fb.h | conflict! |
| ‘F’ | 80-8F | linux/arcfb.h | conflict! |
| ‘F’ | DD | video/sstfb.h | conflict! |
| ‘G’ | 00-3F | drivers/misc/sgi-gru/grulib.h | conflict! |
| ‘G’ | 00-0F | xen/gntalloc.h, xen/gntdev.h | conflict! |
| ‘H’ | 00-7F | linux/hiddev.h | conflict! |
| ‘H’ | 00-0F | linux/hidraw.h | conflict! |
| ‘H’ | 01 | linux/mei.h | conflict! |
| ‘H’ | 02 | linux/mei.h | conflict! |
| ‘H’ | 03 | linux/mei.h | conflict! |
| ‘H’ | 00-0F | sound/asound.h | conflict! |
| ‘H’ | 20-40 | sound/asound\_fm.h | conflict! |
| ‘H’ | 80-8F | sound/sfnt\_info.h | conflict! |
| ‘H’ | 10-8F | sound/emu10k1.h | conflict! |
| ‘H’ | 10-1F | sound/sb16\_csp.h | conflict! |
| ‘H’ | 10-1F | sound/hda\_hwdep.h | conflict! |
| ‘H’ | 40-4F | sound/hdspm.h | conflict! |
| ‘H’ | 40-4F | sound/hdsp.h | conflict! |
| ‘H’ | 90 | sound/usb/usx2y/usb\_stream.h |  |
| ‘H’ | 00-0F | uapi/misc/habanalabs.h | conflict! |
| ‘H’ | A0 | uapi/linux/usb/cdc-wdm.h |  |
| ‘H’ | C0-F0 | net/bluetooth/hci.h | conflict! |
| ‘H’ | C0-DF | net/bluetooth/hidp/hidp.h | conflict! |
| ‘H’ | C0-DF | net/bluetooth/cmtp/cmtp.h | conflict! |
| ‘H’ | C0-DF | net/bluetooth/bnep/bnep.h | conflict! |
| ‘H’ | F1 | linux/hid-roccat.h | <[mailto:erazor\_de@users.sourceforge.net](mailto:erazor_de%40users.sourceforge.net)> |
| ‘H’ | F8-FA | sound/firewire.h |  |
| ‘I’ | all | linux/isdn.h | conflict! |
| ‘I’ | 00-0F | drivers/isdn/divert/isdn\_divert.h | conflict! |
| ‘I’ | 40-4F | linux/mISDNif.h | conflict! |
| ‘K’ | all | linux/kd.h |  |
| ‘L’ | 00-1F | linux/loop.h | conflict! |
| ‘L’ | 10-1F | drivers/scsi/mpt3sas/mpt3sas\_ctl.h | conflict! |
| ‘L’ | E0-FF | linux/ppdd.h | encrypted disk device driver <<http://linux01.gwdg.de/~alatham/ppdd.html>> |
| ‘M’ | all | linux/soundcard.h | conflict! |
| ‘M’ | 01-16 and | mtd/mtd-abi.h drivers/mtd/mtdchar.c | conflict! |
| ‘M’ | 01-03 | drivers/scsi/megaraid/megaraid\_sas.h |  |
| ‘M’ | 00-0F | drivers/video/fsl-diu-fb.h | conflict! |
| ‘N’ | 00-1F | drivers/usb/scanner.h |  |
| ‘N’ | 40-7F | drivers/block/nvme.c |  |
| ‘N’ | 80-8F | uapi/linux/ntsync.h | NT synchronization primitives <[mailto:wine-devel@winehq.org](mailto:wine-devel%40winehq.org)> |
| ‘O’ | 00-06 | mtd/ubi-user.h | UBI |
| ‘P’ | all | linux/soundcard.h | conflict! |
| ‘P’ | 60-6F | sound/sscape\_ioctl.h | conflict! |
| ‘P’ | 00-0F | drivers/usb/class/usblp.c | conflict! |
| ‘P’ | 01-09 | drivers/misc/pci\_endpoint\_test.c | conflict! |
| ‘P’ | 00-0F | xen/privcmd.h | conflict! |
| ‘P’ | 00-05 | linux/tps6594\_pfsm.h | conflict! |
| ‘Q’ | all | linux/soundcard.h |  |
| ‘R’ | 00-1F | linux/random.h | conflict! |
| ‘R’ | 01 | linux/rfkill.h | conflict! |
| ‘R’ | 20-2F | linux/trace\_mmap.h |  |
| ‘R’ | C0-DF | net/bluetooth/rfcomm.h |  |
| ‘R’ | E0 | uapi/linux/fsl\_mc.h |  |
| ‘S’ | all | linux/cdrom.h | conflict! |
| ‘S’ | 80-81 | scsi/scsi\_ioctl.h | conflict! |
| ‘S’ | 82-FF | scsi/scsi.h | conflict! |
| ‘S’ | 00-7F | sound/asequencer.h | conflict! |
| ‘T’ | all | linux/soundcard.h | conflict! |
| ‘T’ | 00-AF | sound/asound.h | conflict! |
| ‘T’ | all | arch/x86/include/asm/ioctls.h | conflict! |
| ‘T’ | C0-DF | linux/if\_tun.h | conflict! |
| ‘U’ | all | sound/asound.h | conflict! |
| ‘U’ | 00-CF | linux/uinput.h | conflict! |
| ‘U’ | 00-EF | linux/usbdevice\_fs.h |  |
| ‘U’ | C0-CF | drivers/bluetooth/hci\_uart.h |  |
| ‘V’ | all | linux/vt.h | conflict! |
| ‘V’ | all | linux/videodev2.h | conflict! |
| ‘V’ | C0 | linux/ivtvfb.h | conflict! |
| ‘V’ | C0 | linux/ivtv.h | conflict! |
| ‘V’ | C0 | media/si4713.h | conflict! |
| ‘W’ | 00-1F | linux/watchdog.h | conflict! |
| ‘W’ | 00-1F | linux/wanrouter.h | conflict! (pre 3.9) |
| ‘W’ | 00-3F | sound/asound.h | conflict! |
| ‘W’ | 40-5F | drivers/pci/switch/switchtec.c |  |
| ‘W’ | 60-61 | linux/watch\_queue.h |  |
| ‘X’ | all | fs/xfs/xfs\_fs.h, fs/xfs/linux-2.6/xfs\_ioctl32.h, include/linux/falloc.h, linux/fs.h, | conflict! |
| ‘X’ | all | fs/ocfs2/ocfs\_fs.h | conflict! |
| ‘Z’ | 14-15 | drivers/message/fusion/mptctl.h |  |
| ‘[’ | 00-3F | linux/usb/tmc.h | USB Test and Measurement Devices <[mailto:gregkh@linuxfoundation.org](mailto:gregkh%40linuxfoundation.org)> |
| ‘a’ | all | linux/atm\*.h, linux/sonet.h | ATM on linux <<http://lrcwww.epfl.ch/>> |
| ‘a’ | 00-0F | drivers/crypto/qat/qat\_common/adf\_cfg\_common.h | conflict! qat driver |
| ‘b’ | 00-FF |  | conflict! bit3 vme host bridge <[mailto:natalia@nikhefk.nikhef.nl](mailto:natalia%40nikhefk.nikhef.nl)> |
| ‘b’ | 00-0F | linux/dma-buf.h | conflict! |
| ‘c’ | 00-7F | linux/comstats.h | conflict! |
| ‘c’ | 00-7F | linux/coda.h | conflict! |
| ‘c’ | 00-1F | linux/chio.h | conflict! |
| ‘c’ | 80-9F | arch/s390/include/asm/chsc.h | conflict! |
| ‘c’ | A0-AF | arch/x86/include/asm/msr.h conflict! |  |
| ‘d’ | 00-FF | linux/char/drm/drm.h | conflict! |
| ‘d’ | 02-40 | pcmcia/ds.h | conflict! |
| ‘d’ | F0-FF | linux/digi1.h |  |
| ‘e’ | all | linux/digi1.h | conflict! |
| ‘f’ | 00-1F | linux/ext2\_fs.h | conflict! |
| ‘f’ | 00-1F | linux/ext3\_fs.h | conflict! |
| ‘f’ | 00-0F | fs/jfs/jfs\_dinode.h | conflict! |
| ‘f’ | 00-0F | fs/ext4/ext4.h | conflict! |
| ‘f’ | 00-0F | linux/fs.h | conflict! |
| ‘f’ | 00-0F | fs/ocfs2/ocfs2\_fs.h | conflict! |
| ‘f’ | 13-27 | linux/fscrypt.h |  |
| ‘f’ | 81-8F | linux/fsverity.h |  |
| ‘g’ | 00-0F | linux/usb/gadgetfs.h |  |
| ‘g’ | 20-2F | linux/usb/g\_printer.h |  |
| ‘h’ | 00-7F |  | conflict! Charon filesystem <[mailto:zapman@interlan.net](mailto:zapman%40interlan.net)> |
| ‘h’ | 00-1F | linux/hpet.h | conflict! |
| ‘h’ | 80-8F | fs/hfsplus/ioctl.c |  |
| ‘i’ | 00-3F | linux/i2o-dev.h | conflict! |
| ‘i’ | 0B-1F | linux/ipmi.h | conflict! |
| ‘i’ | 80-8F | linux/i8k.h |  |
| ‘i’ | 90-9F | linux/iio/\*.h | IIO |
| ‘j’ | 00-3F | linux/joystick.h |  |
| ‘k’ | 00-0F | linux/spi/spidev.h | conflict! |
| ‘k’ | 00-05 | video/kyro.h | conflict! |
| ‘k’ | 10-17 | linux/hsi/hsi\_char.h | HSI character device |
| ‘l’ | 00-3F | linux/tcfs\_fs.h | transparent cryptographic file system <<http://web.archive.org/web/%2A/http://mikonos.dia.unisa.it/tcfs>> |
| ‘l’ | 40-7F | linux/udf\_fs\_i.h | in development: <<https://github.com/pali/udftools>> |
| ‘m’ | 00-09 | linux/mmtimer.h | conflict! |
| ‘m’ | all | linux/mtio.h | conflict! |
| ‘m’ | all | linux/soundcard.h | conflict! |
| ‘m’ | all | linux/synclink.h | conflict! |
| ‘m’ | 00-19 | drivers/message/fusion/mptctl.h | conflict! |
| ‘m’ | 00 | drivers/scsi/megaraid/megaraid\_ioctl.h | conflict! |
| ‘n’ | 00-7F | linux/ncp\_fs.h and fs/ncpfs/ioctl.c |  |
| ‘n’ | 80-8F | uapi/linux/nilfs2\_api.h | NILFS2 |
| ‘n’ | E0-FF | linux/matroxfb.h | matroxfb |
| ‘o’ | 00-1F | fs/ocfs2/ocfs2\_fs.h | OCFS2 |
| ‘o’ | 00-03 | mtd/ubi-user.h | conflict! (OCFS2 and UBI overlaps) |
| ‘o’ | 40-41 | mtd/ubi-user.h | UBI |
| ‘o’ | 01-A1 | linux/dvb/\*.h | DVB |
| ‘p’ | 00-0F | linux/phantom.h | conflict! (OpenHaptics needs this) |
| ‘p’ | 00-1F | linux/rtc.h | conflict! |
| ‘p’ | 40-7F | linux/nvram.h |  |
| ‘p’ | 80-9F | linux/ppdev.h | user-space parport <[mailto:tim@cyberelk.net](mailto:tim%40cyberelk.net)> |
| ‘p’ | A1-A5 | linux/pps.h | LinuxPPS |
| ‘p’ | B1-B3 | linux/pps\_gen.h | LinuxPPS <[mailto:giometti@linux.it](mailto:giometti%40linux.it)> |
| ‘q’ | 00-1F | linux/serio.h |  |
| ‘q’ | 80-FF | linux/telephony.h linux/ixjuser.h | Internet PhoneJACK, Internet LineJACK <<http://web.archive.org/web/%2A/http://www.quicknet.net>> |
| ‘r’ | 00-1F | linux/msdos\_fs.h and fs/fat/dir.c |  |
| ‘s’ | all | linux/cdk.h |  |
| ‘t’ | 00-7F | linux/ppp-ioctl.h |  |
| ‘t’ | 80-8F | linux/isdn\_ppp.h |  |
| ‘t’ | 90-91 | linux/toshiba.h | toshiba and toshiba\_acpi SMM |
| ‘u’ | 00-1F | linux/smb\_fs.h | gone |
| ‘u’ | 00-2F | linux/ublk\_cmd.h | conflict! |
| ‘u’ | 20-3F | linux/uvcvideo.h | USB video class host driver |
| ‘u’ | 40-4f | linux/udmabuf.h | userspace dma-buf misc device |
| ‘v’ | 00-1F | linux/ext2\_fs.h | conflict! |
| ‘v’ | 00-1F | linux/fs.h | conflict! |
| ‘v’ | 00-0F | linux/sonypi.h | conflict! |
| ‘v’ | 00-0F | media/v4l2-subdev.h | conflict! |
| ‘v’ | 20-27 | arch/powerpc/include/uapi/asm/vas-api.h | VAS API |
| ‘v’ | C0-FF | linux/meye.h | conflict! |
| ‘w’ | all |  | CERN SCI driver |
| ‘y’ | 00-1F |  | packet based user level communications <[mailto:zapman@interlan.net](mailto:zapman%40interlan.net)> |
| ‘z’ | 00-3F |  | CAN bus card conflict! <[mailto:hdstich@connectu.ulm.circular.de](mailto:hdstich%40connectu.ulm.circular.de)> |
| ‘z’ | 40-7F |  | CAN bus card conflict! <[mailto:oe@port.de](mailto:oe%40port.de)> |
| ‘z’ | 10-4F | drivers/s390/crypto/zcrypt\_api.h | conflict! |
| ‘|’ | 00-7F | linux/media.h |  |
| ‘|’ | 80-9F | samples/ | Any sample and example drivers |
| 0x80 | 00-1F | linux/fb.h |  |
| 0x81 | 00-1F | linux/vduse.h |  |
| 0x89 | 00-06 | arch/x86/include/asm/sockios.h |  |
| 0x89 | 0B-DF | linux/sockios.h |  |
| 0x89 | E0-EF | linux/sockios.h | SIOCPROTOPRIVATE range |
| 0x89 | F0-FF | linux/sockios.h | SIOCDEVPRIVATE range |
| 0x8A | 00-1F | linux/eventpoll.h |  |
| 0x8B | all | linux/wireless.h |  |
| 0x8C | 00-3F |  | WiNRADiO driver <<http://www.winradio.com.au/>> |
| 0x90 | 00 | drivers/cdrom/sbpcd.h |  |
| 0x92 | 00-0F | drivers/usb/mon/mon\_bin.c |  |
| 0x93 | 60-7F | linux/auto\_fs.h |  |
| 0x94 | all | fs/btrfs/ioctl.h and linux/fs.h | Btrfs filesystem some lifted to vfs/generic |
| 0x97 | 00-7F | fs/ceph/ioctl.h | Ceph file system |
| 0x99 | 00-0F |  | 537-Addinboard driver <[mailto:buk@buks.ipn.de](mailto:buk%40buks.ipn.de)> |
| 0x9A | 00-0F | include/uapi/fwctl/fwctl.h |  |
| 0xA0 | all | linux/sdp/sdp.h | Industrial Device Project <[mailto:kenji@bitgate.com](mailto:kenji%40bitgate.com)> |
| 0xA1 | 0 | linux/vtpm\_proxy.h | TPM Emulator Proxy Driver |
| 0xA2 | all | uapi/linux/acrn.h | ACRN hypervisor |
| 0xA3 | 80-8F |  | Port ACL in development: <[mailto:tlewis@mindspring.com](mailto:tlewis%40mindspring.com)> |
| 0xA3 | 90-9F | linux/dtlk.h |  |
| 0xA4 | 00-1F | uapi/linux/tee.h | Generic TEE subsystem |
| 0xA4 | 00-1F | uapi/asm/sgx.h | <[mailto:linux-sgx@vger.kernel.org](mailto:linux-sgx%40vger.kernel.org)> |
| 0xA5 | 01-05 | linux/surface\_aggregator/cdev.h | Microsoft Surface Platform System Aggregator <[mailto:luzmaximilian@gmail.com](mailto:luzmaximilian%40gmail.com)> |
| 0xA5 | 20-2F | linux/surface\_aggregator/dtx.h | Microsoft Surface DTX driver <[mailto:luzmaximilian@gmail.com](mailto:luzmaximilian%40gmail.com)> |
| 0xAA | 00-3F | linux/uapi/linux/userfaultfd.h |  |
| 0xAB | 00-1F | linux/nbd.h |  |
| 0xAC | 00-1F | linux/raw.h |  |
| 0xAD | 00 |  | Netfilter device in development: <[mailto:rusty@rustcorp.com.au](mailto:rusty%40rustcorp.com.au)> |
| 0xAE | 00-1F | linux/kvm.h | Kernel-based Virtual Machine <[mailto:kvm@vger.kernel.org](mailto:kvm%40vger.kernel.org)> |
| 0xAE | 40-FF | linux/kvm.h | Kernel-based Virtual Machine <[mailto:kvm@vger.kernel.org](mailto:kvm%40vger.kernel.org)> |
| 0xAE | 20-3F | linux/nitro\_enclaves.h | Nitro Enclaves |
| 0xAF | 00-1F | linux/fsl\_hypervisor.h | Freescale hypervisor |
| 0xB0 | all |  | RATIO devices in development: <[mailto:vgo@ratio.de](mailto:vgo%40ratio.de)> |
| 0xB1 | 00-1F |  | PPPoX <[mailto:mostrows@styx.uwaterloo.ca](mailto:mostrows%40styx.uwaterloo.ca)> |
| 0xB2 | 00 | arch/powerpc/include/uapi/asm/papr-vpd.h | powerpc/pseries VPD API <[mailto:linuxppc-dev@lists.ozlabs.org](mailto:linuxppc-dev%40lists.ozlabs.org)> |
| 0xB2 | 01-02 | arch/powerpc/include/uapi/asm/papr-sysparm.h | powerpc/pseries system parameter API <[mailto:linuxppc-dev@lists.ozlabs.org](mailto:linuxppc-dev%40lists.ozlabs.org)> |
| 0xB2 | 03-05 | arch/powerpc/include/uapi/asm/papr-indices.h | powerpc/pseries indices API <[mailto:linuxppc-dev@lists.ozlabs.org](mailto:linuxppc-dev%40lists.ozlabs.org)> |
| 0xB2 | 06-07 | arch/powerpc/include/uapi/asm/papr-platform-dump.h | powerpc/pseries Platform Dump API <[mailto:linuxppc-dev@lists.ozlabs.org](mailto:linuxppc-dev%40lists.ozlabs.org)> |
| 0xB2 | 08 | arch/powerpc/include/uapi/asm/papr-physical-attestation.h | powerpc/pseries Physical Attestation API <[mailto:linuxppc-dev@lists.ozlabs.org](mailto:linuxppc-dev%40lists.ozlabs.org)> |
| 0xB2 | 09 | arch/powerpc/include/uapi/asm/papr-hvpipe.h | powerpc/pseries HVPIPE API <[mailto:linuxppc-dev@lists.ozlabs.org](mailto:linuxppc-dev%40lists.ozlabs.org)> |
| 0xB3 | 00 | linux/mmc/ioctl.h |  |
| 0xB4 | 00-0F | linux/gpio.h | <[mailto:linux-gpio@vger.kernel.org](mailto:linux-gpio%40vger.kernel.org)> |
| 0xB5 | 00-0F | uapi/linux/rpmsg.h | <[mailto:linux-remoteproc@vger.kernel.org](mailto:linux-remoteproc%40vger.kernel.org)> |
| 0xB6 | all | linux/fpga-dfl.h |  |
| 0xB7 | all | uapi/linux/remoteproc\_cdev.h | <[mailto:linux-remoteproc@vger.kernel.org](mailto:linux-remoteproc%40vger.kernel.org)> |
| 0xB7 | all | uapi/linux/nsfs.h | <<mailto:Andrei> Vagin <[avagin@openvz.org](mailto:avagin%40openvz.org)>> |
| 0xB8 | 01-02 | uapi/misc/mrvl\_cn10k\_dpi.h | Marvell CN10K DPI driver |
| 0xB8 | all | uapi/linux/mshv.h | Microsoft Hyper-V /dev/mshv driver <[mailto:linux-hyperv@vger.kernel.org](mailto:linux-hyperv%40vger.kernel.org)> |
| 0xBA | 00-0F | uapi/linux/liveupdate.h | Pasha Tatashin <[mailto:pasha.tatashin@soleen.com](mailto:pasha.tatashin%40soleen.com)> |
| 0xC0 | 00-0F | linux/usb/iowarrior.h |  |
| 0xCA | 00-0F | uapi/misc/cxl.h | Dead since 6.15 |
| 0xCA | 10-2F | uapi/misc/ocxl.h |  |
| 0xCA | 80-BF | uapi/scsi/cxlflash\_ioctl.h | Dead since 6.15 |
| 0xCB | 00-1F |  | CBM serial IEC bus in development: <[mailto:michael.klein@puffin.lb.shuttle.de](mailto:michael.klein%40puffin.lb.shuttle.de)> |
| 0xCC | 00-0F | drivers/misc/ibmvmc.h | pseries VMC driver |
| 0xCD | 01 | linux/reiserfs\_fs.h | Dead since 6.13 |
| 0xCE | 01-02 | uapi/linux/cxl\_mem.h | Compute Express Link Memory Devices |
| 0xCF | 02 | fs/smb/client/cifs\_ioctl.h |  |
| 0xDD | 00-3F |  | ZFCP device driver see drivers/s390/scsi/ <[mailto:aherrman@de.ibm.com](mailto:aherrman%40de.ibm.com)> |
| 0xE5 | 00-3F | linux/fuse.h |  |
| 0xEC | 00-01 | drivers/platform/chrome/cros\_ec\_dev.h | ChromeOS EC driver |
| 0xEE | 00-09 | uapi/linux/pfrut.h | Platform Firmware Runtime Update and Telemetry |
| 0xF3 | 00-3F | drivers/usb/misc/sisusbvga/sisusb.h | sisfb (in development) <[mailto:thomas@winischhofer.net](mailto:thomas%40winischhofer.net)> |
| 0xF6 | all |  | LTTng Linux Trace Toolkit Next Generation <[mailto:mathieu.desnoyers@efficios.com](mailto:mathieu.desnoyers%40efficios.com)> |
| 0xF8 | all | arch/x86/include/uapi/asm/amd\_hsmp.h | AMD HSMP EPYC system management interface driver <[mailto:nchatrad@amd.com](mailto:nchatrad%40amd.com)> |
| 0xF9 | 00-0F | uapi/misc/amd-apml.h | AMD side band system management interface driver <[mailto:naveenkrishna.chatradhi@amd.com](mailto:naveenkrishna.chatradhi%40amd.com)> |
| 0xFD | all | linux/dm-ioctl.h |  |
| 0xFE | all | linux/isst\_if.h |  |
