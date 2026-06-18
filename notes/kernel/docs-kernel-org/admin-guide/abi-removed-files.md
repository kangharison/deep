# Removed ABI Files

> 출처(원문): https://docs.kernel.org/admin-guide/abi-removed-files.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Removed ABI Files

## ABI file removed/devfs

Has the following ABI:

* [devfs](abi-removed.html#abi-devfs)

## ABI file removed/dv1394

Has the following ABI:

* [dv1394 (a.k.a. “OHCI-DV I/O support” for FireWire)](abi-removed.html#abi-dv1394-a-k-a-ohci-dv-i-o-support-for-firewire)

## ABI file removed/ip\_queue

Has the following ABI:

* [ip\_queue](abi-removed.html#abi-ip-queue)

## ABI file removed/net\_dma

Has the following ABI:

* [tcp\_dma\_copybreak sysctl](abi-removed.html#abi-tcp-dma-copybreak-sysctl)

## ABI file removed/o2cb

Has the following ABI:

* [/sys/o2cb symlink](abi-removed.html#abi-sys-o2cb-symlink)

## ABI file removed/raw1394

Has the following ABI:

* [raw1394 (a.k.a. “Raw IEEE1394 I/O support” for FireWire)](abi-removed.html#abi-raw1394-a-k-a-raw-ieee1394-i-o-support-for-firewire)

## ABI file removed/sysfs-bus-nfit

Has the following ABI:

* [/sys/bus/nd/devices/regionX/nfit/ecc\_unit\_size](abi-removed.html#abi-sys-bus-nd-devices-regionx-nfit-ecc-unit-size)

## ABI file removed/sysfs-class-cxl

The cxl driver was removed in 6.15.

Please note that attributes that are shared between devices are stored in
the directory pointed to by the symlink device/.
For example, the real path of the attribute /sys/class/cxl/afu0.0s/irqs\_max is
/sys/class/cxl/afu0.0s/device/irqs\_max, i.e. /sys/class/cxl/afu0.0/irqs\_max.

Slave contexts (eg. /sys/class/cxl/afu0.0s):

Has the following ABI:

* [/sys/class/cxl/<afu>/afu\_err\_buf](abi-removed.html#abi-sys-class-cxl-afu-afu-err-buf)
* [/sys/class/cxl/<afu>/irqs\_max](abi-removed.html#abi-sys-class-cxl-afu-irqs-max)
* [/sys/class/cxl/<afu>/irqs\_min](abi-removed.html#abi-sys-class-cxl-afu-irqs-min)
* [/sys/class/cxl/<afu>/mmio\_size](abi-removed.html#abi-sys-class-cxl-afu-mmio-size)
* [/sys/class/cxl/<afu>/modes\_supported](abi-removed.html#abi-sys-class-cxl-afu-modes-supported)
* [/sys/class/cxl/<afu>/mode](abi-removed.html#abi-sys-class-cxl-afu-mode)
* [/sys/class/cxl/<afu>/prefault\_mode](abi-removed.html#abi-sys-class-cxl-afu-prefault-mode)
* [/sys/class/cxl/<afu>/reset](abi-removed.html#abi-sys-class-cxl-afu-reset)
* [/sys/class/cxl/<afu>/api\_version](abi-removed.html#abi-sys-class-cxl-afu-api-version)
* [/sys/class/cxl/<afu>/api\_version\_compatible](abi-removed.html#abi-sys-class-cxl-afu-api-version-compatible)
* [/sys/class/cxl/<afu>/cr<config num>/vendor](abi-removed.html#abi-sys-class-cxl-afu-cr-config-num-vendor)
* [/sys/class/cxl/<afu>/cr<config num>/device](abi-removed.html#abi-sys-class-cxl-afu-cr-config-num-device)
* [/sys/class/cxl/<afu>/cr<config num>/class](abi-removed.html#abi-sys-class-cxl-afu-cr-config-num-class)
* [/sys/class/cxl/<afu>/cr<config num>/config](abi-removed.html#abi-sys-class-cxl-afu-cr-config-num-config)
* [/sys/class/cxl/<afu>m/mmio\_size](abi-removed.html#abi-sys-class-cxl-afu-m-mmio-size)
* [/sys/class/cxl/<afu>m/pp\_mmio\_len](abi-removed.html#abi-sys-class-cxl-afu-m-pp-mmio-len)
* [/sys/class/cxl/<afu>m/pp\_mmio\_off](abi-removed.html#abi-sys-class-cxl-afu-m-pp-mmio-off)
* [/sys/class/cxl/<card>/caia\_version](abi-removed.html#abi-sys-class-cxl-card-caia-version)
* [/sys/class/cxl/<card>/psl\_revision](abi-removed.html#abi-sys-class-cxl-card-psl-revision)
* [/sys/class/cxl/<card>/base\_image](abi-removed.html#abi-sys-class-cxl-card-base-image)
* [/sys/class/cxl/<card>/image\_loaded](abi-removed.html#abi-sys-class-cxl-card-image-loaded)
* [/sys/class/cxl/<card>/load\_image\_on\_perst](abi-removed.html#abi-sys-class-cxl-card-load-image-on-perst)
* [/sys/class/cxl/<card>/reset](abi-removed.html#abi-sys-class-cxl-card-reset)
* [/sys/class/cxl/<card>/perst\_reloads\_same\_image](abi-removed.html#abi-sys-class-cxl-card-perst-reloads-same-image)
* [/sys/class/cxl/<card>/psl\_timebase\_synced](abi-removed.html#abi-sys-class-cxl-card-psl-timebase-synced)
* [/sys/class/cxl/<card>/tunneled\_ops\_supported](abi-removed.html#abi-sys-class-cxl-card-tunneled-ops-supported)

## ABI file removed/sysfs-class-rfkill

rfkill - radio frequency (RF) connector kill switch support

For details to this subsystem look at [rfkill - RF kill switch support](../driver-api/rfkill.html).

Has the following ABI:

* [/sys/class/rfkill/rfkill[0-9]+/claim](abi-removed.html#abi-sys-class-rfkill-rfkill-0-9-claim)

## ABI file removed/sysfs-firmware-efi-vars

Has the following ABI:

* [/sys/firmware/efi/vars](abi-removed.html#abi-sys-firmware-efi-vars)

## ABI file removed/sysfs-kernel-fadump\_release\_opalcore

This ABI is moved to /sys/firmware/opal/mpipl/release\_core.

Has the following ABI:

* [/sys/kernel/fadump\_release\_opalcore](abi-removed.html#abi-sys-kernel-fadump-release-opalcore)

## ABI file removed/sysfs-kernel-uids

Has the following ABI:

* [/sys/kernel/uids/<uid>/cpu\_shares](abi-removed.html#abi-sys-kernel-uids-uid-cpu-shares)

## ABI file removed/sysfs-mce

Has the following ABI:

* [/sys/devices/system/machinecheck/machinecheckX/tolerant](abi-removed.html#abi-sys-devices-system-machinecheck-machinecheckx-tolerant)

## ABI file removed/sysfs-selinux-checkreqprot

Has the following ABI:

* [/sys/fs/selinux/checkreqprot](abi-removed.html#abi-sys-fs-selinux-checkreqprot)

## ABI file removed/sysfs-selinux-disable

Has the following ABI:

* [/sys/fs/selinux/disable](abi-removed.html#abi-sys-fs-selinux-disable)

## ABI file removed/sysfs-selinux-user

Has the following ABI:

* [/sys/fs/selinux/user](abi-removed.html#abi-sys-fs-selinux-user)

## ABI file removed/video1394

Has the following ABI:

* [video1394 (a.k.a. “OHCI-1394 Video support” for FireWire)](abi-removed.html#abi-video1394-a-k-a-ohci-1394-video-support-for-firewire)
