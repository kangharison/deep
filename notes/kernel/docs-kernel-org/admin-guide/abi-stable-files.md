# Stable ABI Files

> 출처(원문): https://docs.kernel.org/admin-guide/abi-stable-files.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Stable ABI Files

## ABI file stable/firewire-cdev

Has the following ABI:

* [/dev/fw[0-9]+](abi-stable.html#abi-dev-fw-0-9)

## ABI file stable/o2cb

Has the following ABI:

* [/sys/fs/o2cb/](abi-stable.html#abi-sys-fs-o2cb)

## ABI file stable/procfs-audit\_loginuid

Has the following ABI:

* [Audit Login UID](abi-stable.html#abi-audit-login-uid)
* [Audit Login Session ID](abi-stable.html#abi-audit-login-session-id)

## ABI file stable/syscalls

Has the following ABI:

* [The kernel syscall interface](abi-stable.html#abi-the-kernel-syscall-interface)

## ABI file stable/sysfs-acpi-pmprofile

Has the following ABI:

* [/sys/firmware/acpi/pm\_profile](abi-stable.html#abi-sys-firmware-acpi-pm-profile)

## ABI file stable/sysfs-block

Has the following ABI:

* [/sys/block/<disk>/alignment\_offset](abi-stable.html#abi-sys-block-disk-alignment-offset)
* [/sys/block/<disk>/discard\_alignment](abi-stable.html#abi-sys-block-disk-discard-alignment)
* [/sys/block/<disk>/atomic\_write\_max\_bytes](abi-stable.html#abi-sys-block-disk-atomic-write-max-bytes)
* [/sys/block/<disk>/atomic\_write\_unit\_min\_bytes](abi-stable.html#abi-sys-block-disk-atomic-write-unit-min-bytes)
* [/sys/block/<disk>/atomic\_write\_unit\_max\_bytes](abi-stable.html#abi-sys-block-disk-atomic-write-unit-max-bytes)
* [/sys/block/<disk>/atomic\_write\_boundary\_bytes](abi-stable.html#abi-sys-block-disk-atomic-write-boundary-bytes)
* [/sys/block/<disk>/diskseq](abi-stable.html#abi-sys-block-disk-diskseq)
* [/sys/block/<disk>/inflight](abi-stable.html#abi-sys-block-disk-inflight)
* [/sys/block/<disk>/integrity/device\_is\_integrity\_capable](abi-stable.html#abi-sys-block-disk-integrity-device-is-integrity-capable)
* [/sys/block/<disk>/integrity/format](abi-stable.html#abi-sys-block-disk-integrity-format)
* [/sys/block/<disk>/integrity/protection\_interval\_bytes](abi-stable.html#abi-sys-block-disk-integrity-protection-interval-bytes)
* [/sys/block/<disk>/integrity/read\_verify](abi-stable.html#abi-sys-block-disk-integrity-read-verify)
* [/sys/block/<disk>/integrity/tag\_size](abi-stable.html#abi-sys-block-disk-integrity-tag-size)
* [/sys/block/<disk>/integrity/write\_generate](abi-stable.html#abi-sys-block-disk-integrity-write-generate)
* [/sys/block/<disk>/partscan](abi-stable.html#abi-sys-block-disk-partscan)
* [/sys/block/<disk>/<partition>/alignment\_offset](abi-stable.html#abi-sys-block-disk-partition-alignment-offset)
* [/sys/block/<disk>/<partition>/discard\_alignment](abi-stable.html#abi-sys-block-disk-partition-discard-alignment)
* [/sys/block/<disk>/<partition>/stat](abi-stable.html#abi-sys-block-disk-partition-stat)
* [/sys/block/<disk>/queue/add\_random](abi-stable.html#abi-sys-block-disk-queue-add-random)
* [/sys/block/<disk>/queue/chunk\_sectors](abi-stable.html#abi-sys-block-disk-queue-chunk-sectors)
* [/sys/block/<disk>/queue/crypto/](abi-stable.html#abi-sys-block-disk-queue-crypto)
* [/sys/block/<disk>/queue/crypto/hw\_wrapped\_keys](abi-stable.html#abi-sys-block-disk-queue-crypto-hw-wrapped-keys)
* [/sys/block/<disk>/queue/crypto/max\_dun\_bits](abi-stable.html#abi-sys-block-disk-queue-crypto-max-dun-bits)
* [/sys/block/<disk>/queue/crypto/modes/<mode>](abi-stable.html#abi-sys-block-disk-queue-crypto-modes-mode)
* [/sys/block/<disk>/queue/crypto/num\_keyslots](abi-stable.html#abi-sys-block-disk-queue-crypto-num-keyslots)
* [/sys/block/<disk>/queue/crypto/raw\_keys](abi-stable.html#abi-sys-block-disk-queue-crypto-raw-keys)
* [/sys/block/<disk>/queue/dax](abi-stable.html#abi-sys-block-disk-queue-dax)
* [/sys/block/<disk>/queue/discard\_granularity](abi-stable.html#abi-sys-block-disk-queue-discard-granularity)
* [/sys/block/<disk>/queue/discard\_max\_bytes](abi-stable.html#abi-sys-block-disk-queue-discard-max-bytes)
* [/sys/block/<disk>/queue/discard\_max\_hw\_bytes](abi-stable.html#abi-sys-block-disk-queue-discard-max-hw-bytes)
* [/sys/block/<disk>/queue/discard\_zeroes\_data](abi-stable.html#abi-sys-block-disk-queue-discard-zeroes-data)
* [/sys/block/<disk>/queue/dma\_alignment](abi-stable.html#abi-sys-block-disk-queue-dma-alignment)
* [/sys/block/<disk>/queue/fua](abi-stable.html#abi-sys-block-disk-queue-fua)
* [/sys/block/<disk>/queue/hw\_sector\_size](abi-stable.html#abi-sys-block-disk-queue-hw-sector-size)
* [/sys/block/<disk>/queue/independent\_access\_ranges/](abi-stable.html#abi-sys-block-disk-queue-independent-access-ranges)
* [/sys/block/<disk>/queue/io\_poll](abi-stable.html#abi-sys-block-disk-queue-io-poll)
* [/sys/block/<disk>/queue/io\_poll\_delay](abi-stable.html#abi-sys-block-disk-queue-io-poll-delay)
* [/sys/block/<disk>/queue/io\_timeout](abi-stable.html#abi-sys-block-disk-queue-io-timeout)
* [/sys/block/<disk>/queue/iostats](abi-stable.html#abi-sys-block-disk-queue-iostats)
* [/sys/block/<disk>/queue/iostats\_passthrough](abi-stable.html#abi-sys-block-disk-queue-iostats-passthrough)
* [/sys/block/<disk>/queue/logical\_block\_size](abi-stable.html#abi-sys-block-disk-queue-logical-block-size)
* [/sys/block/<disk>/queue/max\_active\_zones](abi-stable.html#abi-sys-block-disk-queue-max-active-zones)
* [/sys/block/<disk>/queue/max\_discard\_segments](abi-stable.html#abi-sys-block-disk-queue-max-discard-segments)
* [/sys/block/<disk>/queue/max\_hw\_sectors\_kb](abi-stable.html#abi-sys-block-disk-queue-max-hw-sectors-kb)
* [/sys/block/<disk>/queue/max\_integrity\_segments](abi-stable.html#abi-sys-block-disk-queue-max-integrity-segments)
* [/sys/block/<disk>/queue/max\_open\_zones](abi-stable.html#abi-sys-block-disk-queue-max-open-zones)
* [/sys/block/<disk>/queue/max\_sectors\_kb](abi-stable.html#abi-sys-block-disk-queue-max-sectors-kb)
* [/sys/block/<disk>/queue/max\_segment\_size](abi-stable.html#abi-sys-block-disk-queue-max-segment-size)
* [/sys/block/<disk>/queue/max\_write\_streams](abi-stable.html#abi-sys-block-disk-queue-max-write-streams)
* [/sys/block/<disk>/queue/write\_stream\_granularity](abi-stable.html#abi-sys-block-disk-queue-write-stream-granularity)
* [/sys/block/<disk>/queue/max\_segments](abi-stable.html#abi-sys-block-disk-queue-max-segments)
* [/sys/block/<disk>/queue/minimum\_io\_size](abi-stable.html#abi-sys-block-disk-queue-minimum-io-size)
* [/sys/block/<disk>/queue/nomerges](abi-stable.html#abi-sys-block-disk-queue-nomerges)
* [/sys/block/<disk>/queue/nr\_requests](abi-stable.html#abi-sys-block-disk-queue-nr-requests)
* [/sys/block/<disk>/queue/async\_depth](abi-stable.html#abi-sys-block-disk-queue-async-depth)
* [/sys/block/<disk>/queue/nr\_zones](abi-stable.html#abi-sys-block-disk-queue-nr-zones)
* [/sys/block/<disk>/queue/optimal\_io\_size](abi-stable.html#abi-sys-block-disk-queue-optimal-io-size)
* [/sys/block/<disk>/queue/physical\_block\_size](abi-stable.html#abi-sys-block-disk-queue-physical-block-size)
* [/sys/block/<disk>/queue/read\_ahead\_kb](abi-stable.html#abi-sys-block-disk-queue-read-ahead-kb)
* [/sys/block/<disk>/queue/rotational](abi-stable.html#abi-sys-block-disk-queue-rotational)
* [/sys/block/<disk>/queue/rq\_affinity](abi-stable.html#abi-sys-block-disk-queue-rq-affinity)
* [/sys/block/<disk>/queue/scheduler](abi-stable.html#abi-sys-block-disk-queue-scheduler)
* [/sys/block/<disk>/queue/stable\_writes](abi-stable.html#abi-sys-block-disk-queue-stable-writes)
* [/sys/block/<disk>/queue/virt\_boundary\_mask](abi-stable.html#abi-sys-block-disk-queue-virt-boundary-mask)
* [/sys/block/<disk>/queue/wbt\_lat\_usec](abi-stable.html#abi-sys-block-disk-queue-wbt-lat-usec)
* [/sys/block/<disk>/queue/write\_cache](abi-stable.html#abi-sys-block-disk-queue-write-cache)
* [/sys/block/<disk>/queue/write\_same\_max\_bytes](abi-stable.html#abi-sys-block-disk-queue-write-same-max-bytes)
* [/sys/block/<disk>/queue/write\_zeroes\_max\_bytes](abi-stable.html#abi-sys-block-disk-queue-write-zeroes-max-bytes)
* [/sys/block/<disk>/queue/write\_zeroes\_unmap\_max\_hw\_bytes](abi-stable.html#abi-sys-block-disk-queue-write-zeroes-unmap-max-hw-bytes)
* [/sys/block/<disk>/queue/write\_zeroes\_unmap\_max\_bytes](abi-stable.html#abi-sys-block-disk-queue-write-zeroes-unmap-max-bytes)
* [/sys/block/<disk>/queue/zone\_append\_max\_bytes](abi-stable.html#abi-sys-block-disk-queue-zone-append-max-bytes)
* [/sys/block/<disk>/queue/zone\_write\_granularity](abi-stable.html#abi-sys-block-disk-queue-zone-write-granularity)
* [/sys/block/<disk>/queue/zoned](abi-stable.html#abi-sys-block-disk-queue-zoned)
* [/sys/block/<disk>/queue/zoned\_qd1\_writes](abi-stable.html#abi-sys-block-disk-queue-zoned-qd1-writes)
* [/sys/block/<disk>/hidden](abi-stable.html#abi-sys-block-disk-hidden)
* [/sys/block/<disk>/stat](abi-stable.html#abi-sys-block-disk-stat)

## ABI file stable/sysfs-bus-firewire

Has the following ABI:

* [/sys/bus/firewire/devices/fw[0-9]+/](abi-stable.html#abi-sys-bus-firewire-devices-fw-0-9)
* [/sys/bus/firewire/devices/fw[0-9]+/units](abi-stable.html#abi-sys-bus-firewire-devices-fw-0-9-units)
* [/sys/bus/firewire/devices/fw[0-9]+/is\_local](abi-stable.html#abi-sys-bus-firewire-devices-fw-0-9-is-local)
* [/sys/bus/firewire/devices/fw[0-9]+[.][0-9]+/](abi-stable.html#abi-sys-bus-firewire-devices-fw-0-9-0-9)
* [/sys/bus/firewire/devices/\*/](abi-stable.html#abi-sys-bus-firewire-devices)
* [/sys/bus/firewire/drivers/sbp2/fw\*/host\*/target\*/\*:\*:\*:\*/ieee1394\_id](abi-stable.html#abi-sys-bus-firewire-drivers-sbp2-fw-host-target-ieee1394-id)

## ABI file stable/sysfs-bus-fsl-mc

Has the following ABI:

* [/sys/bus/fsl-mc/rescan](abi-stable.html#abi-sys-bus-fsl-mc-rescan)
* [/sys/bus/fsl-mc/autorescan](abi-stable.html#abi-sys-bus-fsl-mc-autorescan)

## ABI file stable/sysfs-bus-mhi

Has the following ABI:

* [/sys/bus/mhi/devices/.../serialnumber](abi-stable.html#abi-sys-bus-mhi-devices-serialnumber)
* [/sys/bus/mhi/devices/.../oem\_pk\_hash](abi-stable.html#abi-sys-bus-mhi-devices-oem-pk-hash)
* [/sys/bus/mhi/devices/.../soc\_reset](abi-stable.html#abi-sys-bus-mhi-devices-soc-reset)
* [/sys/bus/mhi/devices/.../trigger\_edl](abi-stable.html#abi-sys-bus-mhi-devices-trigger-edl)

## ABI file stable/sysfs-bus-nvmem

Has the following ABI:

* [/sys/bus/nvmem/devices/.../force\_ro](abi-stable.html#abi-sys-bus-nvmem-devices-force-ro)
* [/sys/bus/nvmem/devices/.../nvmem](abi-stable.html#abi-sys-bus-nvmem-devices-nvmem)
* [/sys/bus/nvmem/devices/.../type](abi-stable.html#abi-sys-bus-nvmem-devices-type)

## ABI file stable/sysfs-bus-usb

Has the following ABI:

* [/sys/bus/usb/devices/.../power/persist](abi-stable.html#abi-sys-bus-usb-devices-power-persist)
* [/sys/bus/usb/devices/.../power/autosuspend](abi-stable.html#abi-sys-bus-usb-devices-power-autosuspend)
* [/sys/bus/usb/device/.../power/connected\_duration](abi-stable.html#abi-sys-bus-usb-device-power-connected-duration)
* [/sys/bus/usb/device/.../power/active\_duration](abi-stable.html#abi-sys-bus-usb-device-power-active-duration)
* [/sys/bus/usb/devices/<busnum>-<port[.port]>...:<config num>-<interface num>/supports\_autosuspend](abi-stable.html#abi-sys-bus-usb-devices-busnum-port-port-config-num-interface-num-supports-autosuspend)
* [/sys/bus/usb/device/.../avoid\_reset\_quirk](abi-stable.html#abi-sys-bus-usb-device-avoid-reset-quirk)
* [/sys/bus/usb/devices/.../devnum](abi-stable.html#abi-sys-bus-usb-devices-devnum)
* [/sys/bus/usb/devices/.../bConfigurationValue](abi-stable.html#abi-sys-bus-usb-devices-bconfigurationvalue)
* [/sys/bus/usb/devices/.../busnum](abi-stable.html#abi-sys-bus-usb-devices-busnum)
* [/sys/bus/usb/devices/.../descriptors](abi-stable.html#abi-sys-bus-usb-devices-descriptors)
* [/sys/bus/usb/devices/.../speed](abi-stable.html#abi-sys-bus-usb-devices-speed)

## ABI file stable/sysfs-bus-vmbus

Has the following ABI:

* [/sys/bus/vmbus/hibernation](abi-stable.html#abi-sys-bus-vmbus-hibernation)
* [/sys/bus/vmbus/devices/<UUID>/id](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-id)
* [/sys/bus/vmbus/devices/<UUID>/class\_id](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-class-id)
* [/sys/bus/vmbus/devices/<UUID>/device\_id](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-device-id)
* [/sys/bus/vmbus/devices/<UUID>/channel\_vp\_mapping](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channel-vp-mapping)
* [/sys/bus/vmbus/devices/<UUID>/device](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-device)
* [/sys/bus/vmbus/devices/<UUID>/vendor](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-vendor)
* [/sys/bus/vmbus/devices/<UUID>/numa\_node](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-numa-node)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/cpu](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-cpu)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/in\_mask](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-in-mask)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/latency](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-latency)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/out\_mask](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-out-mask)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/pending](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-pending)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/read\_avail](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-read-avail)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/write\_avail](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-write-avail)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/events](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-events)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/interrupts](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-interrupts)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/subchannel\_id](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-subchannel-id)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/monitor\_id](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-monitor-id)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/ring](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-ring)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/intr\_in\_full](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-intr-in-full)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/intr\_out\_empty](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-intr-out-empty)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/out\_full\_first](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-out-full-first)
* [/sys/bus/vmbus/devices/<UUID>/channels/<N>/out\_full\_total](abi-stable.html#abi-sys-bus-vmbus-devices-uuid-channels-n-out-full-total)

## ABI file stable/sysfs-bus-w1

Has the following ABI:

* [/sys/bus/w1/devices/.../w1\_master\_timeout\_us](abi-stable.html#abi-sys-bus-w1-devices-w1-master-timeout-us)

## ABI file stable/sysfs-bus-xen-backend

Has the following ABI:

* [/sys/bus/xen-backend/devices/\*/devtype](abi-stable.html#abi-sys-bus-xen-backend-devices-devtype)
* [/sys/bus/xen-backend/devices/\*/nodename](abi-stable.html#abi-sys-bus-xen-backend-devices-nodename)
* [/sys/bus/xen-backend/devices/vbd-\*/physical\_device](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-physical-device)
* [/sys/bus/xen-backend/devices/vbd-\*/mode](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-mode)
* [/sys/bus/xen-backend/devices/vbd-\*/statistics/f\_req](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-statistics-f-req)
* [/sys/bus/xen-backend/devices/vbd-\*/statistics/oo\_req](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-statistics-oo-req)
* [/sys/bus/xen-backend/devices/vbd-\*/statistics/rd\_req](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-statistics-rd-req)
* [/sys/bus/xen-backend/devices/vbd-\*/statistics/rd\_sect](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-statistics-rd-sect)
* [/sys/bus/xen-backend/devices/vbd-\*/statistics/wr\_req](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-statistics-wr-req)
* [/sys/bus/xen-backend/devices/vbd-\*/statistics/wr\_sect](abi-stable.html#abi-sys-bus-xen-backend-devices-vbd-statistics-wr-sect)
* [/sys/bus/xen-backend/devices/\*/state](abi-stable.html#abi-sys-bus-xen-backend-devices-state)

## ABI file stable/sysfs-class-backlight

Has the following ABI:

* [/sys/class/backlight/<backlight>/bl\_power](abi-stable.html#abi-sys-class-backlight-backlight-bl-power)
* [/sys/class/backlight/<backlight>/brightness](abi-stable.html#abi-sys-class-backlight-backlight-brightness)
* [/sys/class/backlight/<backlight>/actual\_brightness](abi-stable.html#abi-sys-class-backlight-backlight-actual-brightness)
* [/sys/class/backlight/<backlight>/max\_brightness](abi-stable.html#abi-sys-class-backlight-backlight-max-brightness)
* [/sys/class/backlight/<backlight>/type](abi-stable.html#abi-sys-class-backlight-backlight-type)

## ABI file stable/sysfs-class-bluetooth

Has the following ABI:

* [/sys/class/bluetooth/hci<index>/reset](abi-stable.html#abi-sys-class-bluetooth-hci-index-reset)

## ABI file stable/sysfs-class-infiniband

sysfs interface common for all infiniband devices

Has the following ABI:

* [/sys/class/infiniband/<device>/node\_type](abi-stable.html#abi-sys-class-infiniband-device-node-type)
* [/sys/class/infiniband/<device>/node\_guid](abi-stable.html#abi-sys-class-infiniband-device-node-type)
* [/sys/class/infiniband/<device>/sys\_image\_guid](abi-stable.html#abi-sys-class-infiniband-device-node-type)
* [/sys/class/infiniband/<device>/node\_desc](abi-stable.html#abi-sys-class-infiniband-device-node-desc)
* [/sys/class/infiniband/<device>/fw\_ver](abi-stable.html#abi-sys-class-infiniband-device-fw-ver)
* [/sys/class/infiniband/<device>/ports/<port-num>/lid](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/rate](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/lid\_mask\_count](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/sm\_sl](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/sm\_lid](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/state](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/phys\_state](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/cap\_mask](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-lid)
* [/sys/class/infiniband/<device>/ports/<port-num>/link\_layer](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-link-layer)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/symbol\_error](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_remote\_physical\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_switch\_relay\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/link\_error\_recovery](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_constraint\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_contraint\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/local\_link\_integrity\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/excessive\_buffer\_overrun\_errors](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_data](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_data](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_packets](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_rcv\_packets](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/unicast\_rcv\_packets](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/unicast\_xmit\_packets](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/multicast\_rcv\_packets](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/multicast\_xmit\_packets](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/link\_downed](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_discards](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/VL15\_dropped](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device>/ports/<port-num>/counters/port\_xmit\_wait](abi-stable.html#abi-sys-class-infiniband-device-ports-port-num-counters-symbol-error)
* [/sys/class/infiniband/<device-name>/hw\_counters/lifespan](abi-stable.html#abi-sys-class-infiniband-device-name-hw-counters-lifespan)
* [/sys/class/infiniband/<device-name>/ports/<port-num>/hw\_counters/lifespan](abi-stable.html#abi-sys-class-infiniband-device-name-hw-counters-lifespan)
* [/sys/class/infiniband/<hca>/ports/<port-number>/gid\_attrs/ndevs/<gid-index>](abi-stable.html#abi-sys-class-infiniband-hca-ports-port-number-gid-attrs-ndevs-gid-index)
* [/sys/class/infiniband/<hca>/ports/<port-number>/gid\_attrs/types/<gid-index>](abi-stable.html#abi-sys-class-infiniband-hca-ports-port-number-gid-attrs-types-gid-index)
* [/sys/class/infiniband\_mad/umad<N>/ibdev](abi-stable.html#abi-sys-class-infiniband-mad-umad-n-ibdev)
* [/sys/class/infiniband\_mad/umad<N>/port](abi-stable.html#abi-sys-class-infiniband-mad-umad-n-ibdev)
* [/sys/class/infiniband\_mad/issm<N>/ibdev](abi-stable.html#abi-sys-class-infiniband-mad-umad-n-ibdev)
* [/sys/class/infiniband\_mad/issm<N>/port](abi-stable.html#abi-sys-class-infiniband-mad-umad-n-ibdev)
* [/sys/class/infiniband\_mad/abi\_version](abi-stable.html#abi-sys-class-infiniband-mad-abi-version)
* [/sys/class/infiniband\_verbs/uverbs<N>/ibdev](abi-stable.html#abi-sys-class-infiniband-verbs-uverbs-n-ibdev)
* [/sys/class/infiniband\_verbs/uverbs<N>/abi\_version](abi-stable.html#abi-sys-class-infiniband-verbs-uverbs-n-ibdev)
* [/sys/class/infiniband\_verbs/abi\_version](abi-stable.html#abi-sys-class-infiniband-verbs-abi-version)
* [/sys/class/infiniband/mthcaX/hw\_rev](abi-stable.html#abi-sys-class-infiniband-mthcax-hw-rev)
* [/sys/class/infiniband/mthcaX/hca\_type](abi-stable.html#abi-sys-class-infiniband-mthcax-hw-rev)
* [/sys/class/infiniband/mthcaX/board\_id](abi-stable.html#abi-sys-class-infiniband-mthcax-hw-rev)
* [/sys/class/infiniband/mlx4\_X/hw\_rev](abi-stable.html#abi-sys-class-infiniband-mlx4-x-hw-rev)
* [/sys/class/infiniband/mlx4\_X/hca\_type](abi-stable.html#abi-sys-class-infiniband-mlx4-x-hw-rev)
* [/sys/class/infiniband/mlx4\_X/board\_id](abi-stable.html#abi-sys-class-infiniband-mlx4-x-hw-rev)
* [/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/gids/<n>](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-ports-port-num-gids-n)
* [/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/admin\_guids/<n>](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-ports-port-num-gids-n)
* [/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/pkeys/<n>](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-ports-port-num-gids-n)
* [/sys/class/infiniband/mlx4\_X/iov/ports/<port-num>/mcgs/](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-ports-port-num-gids-n)
* [/sys/class/infiniband/mlx4\_X/iov/ports/<pci-slot-num>/ports/<m>/gid\_idx/0](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-ports-port-num-gids-n)
* [/sys/class/infiniband/mlx4\_X/iov/ports/<pci-slot-num>/ports/<m>/pkey\_idx/<n>](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-ports-port-num-gids-n)
* [/sys/class/infiniband/mlx4\_X/iov/<pci-slot-num>/ports/<m>/smi\_enabled](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-pci-slot-num-ports-m-smi-enabled)
* [/sys/class/infiniband/mlx4\_X/iov/<pci-slot-num>/ports/<m>/enable\_smi\_admin](abi-stable.html#abi-sys-class-infiniband-mlx4-x-iov-pci-slot-num-ports-m-smi-enabled)
* [/sys/class/infiniband/cxgb4\_X/hw\_rev](abi-stable.html#abi-sys-class-infiniband-cxgb4-x-hw-rev)
* [/sys/class/infiniband/cxgb4\_X/hca\_type](abi-stable.html#abi-sys-class-infiniband-cxgb4-x-hw-rev)
* [/sys/class/infiniband/cxgb4\_X/board\_id](abi-stable.html#abi-sys-class-infiniband-cxgb4-x-hw-rev)
* [/sys/class/infiniband/qibX/version](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/hw\_rev](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/hca\_type](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/board\_id](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/boardversion](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/nctxts](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/localbus\_info](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/tempsense](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/serial](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/nfreectxts](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/chip\_reset](abi-stable.html#abi-sys-class-infiniband-qibx-version)
* [/sys/class/infiniband/qibX/ports/<N>/sl2vl/[0-15]](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-sl2vl-0-15)
* [/sys/class/infiniband/qibX/ports/<N>/CCMgtA/cc\_settings\_bin](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-ccmgta-cc-settings-bin)
* [/sys/class/infiniband/qibX/ports/<N>/CCMgtA/cc\_table\_bin](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-ccmgta-cc-settings-bin)
* [/sys/class/infiniband/qibX/ports/<N>/linkstate/loopback](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-linkstate-loopback)
* [/sys/class/infiniband/qibX/ports/<N>/linkstate/led\_override](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-linkstate-loopback)
* [/sys/class/infiniband/qibX/ports/<N>/linkstate/hrtbt\_enable](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-linkstate-loopback)
* [/sys/class/infiniband/qibX/ports/<N>/linkstate/status](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-linkstate-loopback)
* [/sys/class/infiniband/qibX/ports/<N>/linkstate/status\_str](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-linkstate-loopback)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rc\_resends](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/seq\_naks](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rdma\_seq](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rnr\_naks](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/other\_naks](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/rc\_timeouts](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/look\_pkts](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/pkt\_drops](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/dma\_wait](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/qibX/ports/<N>/diag\_counters/unaligned](abi-stable.html#abi-sys-class-infiniband-qibx-ports-n-diag-counters-rc-resends)
* [/sys/class/infiniband/mlx5\_X/hw\_rev](abi-stable.html#abi-sys-class-infiniband-mlx5-x-hw-rev)
* [/sys/class/infiniband/mlx5\_X/hca\_type](abi-stable.html#abi-sys-class-infiniband-mlx5-x-hw-rev)
* [/sys/class/infiniband/mlx5\_X/reg\_pages](abi-stable.html#abi-sys-class-infiniband-mlx5-x-hw-rev)
* [/sys/class/infiniband/mlx5\_X/fw\_pages](abi-stable.html#abi-sys-class-infiniband-mlx5-x-hw-rev)
* [/sys/class/infiniband/usnic\_X/board\_id](abi-stable.html#abi-sys-class-infiniband-usnic-x-board-id)
* [/sys/class/infiniband/usnic\_X/config](abi-stable.html#abi-sys-class-infiniband-usnic-x-board-id)
* [/sys/class/infiniband/usnic\_X/qp\_per\_vf](abi-stable.html#abi-sys-class-infiniband-usnic-x-board-id)
* [/sys/class/infiniband/usnic\_X/max\_vf](abi-stable.html#abi-sys-class-infiniband-usnic-x-board-id)
* [/sys/class/infiniband/usnic\_X/cq\_per\_vf](abi-stable.html#abi-sys-class-infiniband-usnic-x-board-id)
* [/sys/class/infiniband/usnic\_X/iface](abi-stable.html#abi-sys-class-infiniband-usnic-x-board-id)
* [/sys/class/infiniband/usnic\_X/qpn/summary](abi-stable.html#abi-sys-class-infiniband-usnic-x-qpn-summary)
* [/sys/class/infiniband/usnic\_X/qpn/context](abi-stable.html#abi-sys-class-infiniband-usnic-x-qpn-summary)
* [/sys/class/infiniband/ocrdmaX/hw\_rev](abi-stable.html#abi-sys-class-infiniband-ocrdmax-hw-rev)
* [/sys/class/infiniband/ocrdmaX/hca\_type](abi-stable.html#abi-sys-class-infiniband-ocrdmax-hca-type)
* [/sys/class/infiniband/hfi1\_X/hw\_rev](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/board\_id](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/nctxts](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/serial](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/chip\_reset](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/boardversion](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/nfreectxts](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/tempsense](abi-stable.html#abi-sys-class-infiniband-hfi1-x-hw-rev)
* [/sys/class/infiniband/hfi1\_X/ports/<N>/CCMgtA/cc\_settings\_bin](abi-stable.html#abi-sys-class-infiniband-hfi1-x-ports-n-ccmgta-cc-settings-bin)
* [/sys/class/infiniband/hfi1\_X/ports/<N>/CCMgtA/cc\_table\_bin](abi-stable.html#abi-sys-class-infiniband-hfi1-x-ports-n-ccmgta-cc-settings-bin)
* [/sys/class/infiniband/hfi1\_X/ports/<N>/CCMgtA/cc\_prescan](abi-stable.html#abi-sys-class-infiniband-hfi1-x-ports-n-ccmgta-cc-settings-bin)
* [/sys/class/infiniband/hfi1\_X/ports/<N>/sc2vl/[0-31]](abi-stable.html#abi-sys-class-infiniband-hfi1-x-ports-n-sc2vl-0-31)
* [/sys/class/infiniband/hfi1\_X/ports/<N>/sl2sc/[0-31]](abi-stable.html#abi-sys-class-infiniband-hfi1-x-ports-n-sc2vl-0-31)
* [/sys/class/infiniband/hfi1\_X/ports/<N>/vl2mtu/[0-15]](abi-stable.html#abi-sys-class-infiniband-hfi1-x-ports-n-sc2vl-0-31)
* [/sys/class/infiniband/hfi1\_X/sdma\_<N>/cpu\_list](abi-stable.html#abi-sys-class-infiniband-hfi1-x-sdma-n-cpu-list)
* [/sys/class/infiniband/hfi1\_X/sdma\_<N>/vl](abi-stable.html#abi-sys-class-infiniband-hfi1-x-sdma-n-cpu-list)
* [/sys/class/infiniband/qedrX/hw\_rev](abi-stable.html#abi-sys-class-infiniband-qedrx-hw-rev)
* [/sys/class/infiniband/qedrX/hca\_type](abi-stable.html#abi-sys-class-infiniband-qedrx-hw-rev)
* [/sys/class/infiniband/vmw\_pvrdmaX/hw\_rev](abi-stable.html#abi-sys-class-infiniband-vmw-pvrdmax-hw-rev)
* [/sys/class/infiniband/vmw\_pvrdmaX/hca\_type](abi-stable.html#abi-sys-class-infiniband-vmw-pvrdmax-hw-rev)
* [/sys/class/infiniband/vmw\_pvrdmaX/board\_id](abi-stable.html#abi-sys-class-infiniband-vmw-pvrdmax-hw-rev)
* [/sys/class/infiniband/bnxt\_reX/hw\_rev](abi-stable.html#abi-sys-class-infiniband-bnxt-rex-hw-rev)
* [/sys/class/infiniband/bnxt\_reX/hca\_type](abi-stable.html#abi-sys-class-infiniband-bnxt-rex-hw-rev)

## ABI file stable/sysfs-class-rfkill

rfkill - radio frequency (RF) connector kill switch support

For details to this subsystem look at [rfkill - RF kill switch support](../driver-api/rfkill.html).

For the deprecated `/sys/class/rfkill/*/claim` knobs of this interface look in
[removed/sysfs-class-rfkill](abi-removed-files.html#abi-file-removed-sysfs-class-rfkill).

Has the following ABI:

* [/sys/class/rfkill](abi-stable.html#abi-sys-class-rfkill)
* [/sys/class/rfkill/rfkill[0-9]+/name](abi-stable.html#abi-sys-class-rfkill-rfkill-0-9-name)
* [/sys/class/rfkill/rfkill[0-9]+/type](abi-stable.html#abi-sys-class-rfkill-rfkill-0-9-type)
* [/sys/class/rfkill/rfkill[0-9]+/persistent](abi-stable.html#abi-sys-class-rfkill-rfkill-0-9-persistent)
* [/sys/class/rfkill/rfkill[0-9]+/state](abi-stable.html#abi-sys-class-rfkill-rfkill-0-9-state)
* [/sys/class/rfkill/rfkill[0-9]+/hard](abi-stable.html#abi-sys-class-rfkill-rfkill-0-9-hard)
* [/sys/class/rfkill/rfkill[0-9]+/soft](abi-stable.html#abi-sys-class-rfkill-rfkill-0-9-soft)

## ABI file stable/sysfs-class-tpm

Has the following ABI:

* [/sys/class/tpm/tpmX/device/](abi-stable.html#abi-sys-class-tpm-tpmx-device)
* [/sys/class/tpm/tpmX/device/active](abi-stable.html#abi-sys-class-tpm-tpmx-device-active)
* [/sys/class/tpm/tpmX/device/cancel](abi-stable.html#abi-sys-class-tpm-tpmx-device-cancel)
* [/sys/class/tpm/tpmX/device/caps](abi-stable.html#abi-sys-class-tpm-tpmx-device-caps)
* [/sys/class/tpm/tpmX/device/durations](abi-stable.html#abi-sys-class-tpm-tpmx-device-durations)
* [/sys/class/tpm/tpmX/device/enabled](abi-stable.html#abi-sys-class-tpm-tpmx-device-enabled)
* [/sys/class/tpm/tpmX/device/owned](abi-stable.html#abi-sys-class-tpm-tpmx-device-owned)
* [/sys/class/tpm/tpmX/device/pcrs](abi-stable.html#abi-sys-class-tpm-tpmx-device-pcrs)
* [/sys/class/tpm/tpmX/device/pubek](abi-stable.html#abi-sys-class-tpm-tpmx-device-pubek)
* [/sys/class/tpm/tpmX/device/temp\_deactivated](abi-stable.html#abi-sys-class-tpm-tpmx-device-temp-deactivated)
* [/sys/class/tpm/tpmX/device/timeouts](abi-stable.html#abi-sys-class-tpm-tpmx-device-timeouts)
* [/sys/class/tpm/tpmX/tpm\_version\_major](abi-stable.html#abi-sys-class-tpm-tpmx-tpm-version-major)
* [/sys/class/tpm/tpmX/pcr-<H>/<N>](abi-stable.html#abi-sys-class-tpm-tpmx-pcr-h-n)

## ABI file stable/sysfs-class-ubi

Has the following ABI:

* [/sys/class/ubi/](abi-stable.html#abi-sys-class-ubi)
* [/sys/class/ubi/version](abi-stable.html#abi-sys-class-ubi-version)
* [/sys/class/ubiX/](abi-stable.html#abi-sys-class-ubix)
* [/sys/class/ubi/ubiX/avail\_eraseblocks](abi-stable.html#abi-sys-class-ubi-ubix-avail-eraseblocks)
* [/sys/class/ubi/ubiX/bad\_peb\_count](abi-stable.html#abi-sys-class-ubi-ubix-bad-peb-count)
* [/sys/class/ubi/ubiX/bgt\_enabled](abi-stable.html#abi-sys-class-ubi-ubix-bgt-enabled)
* [/sys/class/ubi/ubiX/dev](abi-stable.html#abi-sys-class-ubi-ubix-dev)
* [/sys/class/ubi/ubiX/eraseblock\_size](abi-stable.html#abi-sys-class-ubi-ubix-eraseblock-size)
* [/sys/class/ubi/ubiX/max\_ec](abi-stable.html#abi-sys-class-ubi-ubix-max-ec)
* [/sys/class/ubi/ubiX/max\_vol\_count](abi-stable.html#abi-sys-class-ubi-ubix-max-vol-count)
* [/sys/class/ubi/ubiX/min\_io\_size](abi-stable.html#abi-sys-class-ubi-ubix-min-io-size)
* [/sys/class/ubi/ubiX/mtd\_num](abi-stable.html#abi-sys-class-ubi-ubix-mtd-num)
* [/sys/class/ubi/ubiX/reserved\_for\_bad](abi-stable.html#abi-sys-class-ubi-ubix-reserved-for-bad)
* [/sys/class/ubi/ubiX/ro\_mode](abi-stable.html#abi-sys-class-ubi-ubix-ro-mode)
* [/sys/class/ubi/ubiX/total\_eraseblocks](abi-stable.html#abi-sys-class-ubi-ubix-total-eraseblocks)
* [/sys/class/ubi/ubiX/volumes\_count](abi-stable.html#abi-sys-class-ubi-ubix-volumes-count)
* [/sys/class/ubi/ubiX/ubiX\_Y/](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y)
* [/sys/class/ubi/ubiX/ubiX\_Y/alignment](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-alignment)
* [/sys/class/ubi/ubiX/ubiX\_Y/corrupted](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-corrupted)
* [/sys/class/ubi/ubiX/ubiX\_Y/data\_bytes](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-data-bytes)
* [/sys/class/ubi/ubiX/ubiX\_Y/dev](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-dev)
* [/sys/class/ubi/ubiX/ubiX\_Y/name](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-name)
* [/sys/class/ubi/ubiX/ubiX\_Y/reserved\_ebs](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-reserved-ebs)
* [/sys/class/ubi/ubiX/ubiX\_Y/type](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-type)
* [/sys/class/ubi/ubiX/ubiX\_Y/upd\_marker](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-upd-marker)
* [/sys/class/ubi/ubiX/ubiX\_Y/usable\_eb\_size](abi-stable.html#abi-sys-class-ubi-ubix-ubix-y-usable-eb-size)

## ABI file stable/sysfs-class-udc

Has the following ABI:

* [/sys/class/udc/<udc>/a\_alt\_hnp\_support](abi-stable.html#abi-sys-class-udc-udc-a-alt-hnp-support)
* [/sys/class/udc/<udc>/a\_hnp\_support](abi-stable.html#abi-sys-class-udc-udc-a-hnp-support)
* [/sys/class/udc/<udc>/b\_hnp\_enable](abi-stable.html#abi-sys-class-udc-udc-b-hnp-enable)
* [/sys/class/udc/<udc>/current\_speed](abi-stable.html#abi-sys-class-udc-udc-current-speed)
* [/sys/class/udc/<udc>/is\_a\_peripheral](abi-stable.html#abi-sys-class-udc-udc-is-a-peripheral)
* [/sys/class/udc/<udc>/is\_otg](abi-stable.html#abi-sys-class-udc-udc-is-otg)
* [/sys/class/udc/<udc>/maximum\_speed](abi-stable.html#abi-sys-class-udc-udc-maximum-speed)
* [/sys/class/udc/<udc>/soft\_connect](abi-stable.html#abi-sys-class-udc-udc-soft-connect)
* [/sys/class/udc/<udc>/srp](abi-stable.html#abi-sys-class-udc-udc-srp)
* [/sys/class/udc/<udc>/state](abi-stable.html#abi-sys-class-udc-udc-state)
* [/sys/class/udc/<udc>/function](abi-stable.html#abi-sys-class-udc-udc-function)

## ABI file stable/sysfs-devices

Note:
:   This documents additional properties of any device beyond what
    is documented in [Rules on how to access information in sysfs](sysfs-rules.html)

Has the following ABI:

* [/sys/devices/\*/of\_node](abi-stable.html#abi-sys-devices-of-node)
* [/sys/devices/\*/devspec](abi-stable.html#abi-sys-devices-devspec)
* [/sys/devices/\*/obppath](abi-stable.html#abi-sys-devices-obppath)
* [/sys/devices/\*/dev](abi-stable.html#abi-sys-devices-dev)

## ABI file stable/sysfs-devices-node

Has the following ABI:

* [/sys/devices/system/node/possible](abi-stable.html#abi-sys-devices-system-node-possible)
* [/sys/devices/system/node/online](abi-stable.html#abi-sys-devices-system-node-online)
* [/sys/devices/system/node/has\_normal\_memory](abi-stable.html#abi-sys-devices-system-node-has-normal-memory)
* [/sys/devices/system/node/has\_cpu](abi-stable.html#abi-sys-devices-system-node-has-cpu)
* [/sys/devices/system/node/has\_high\_memory](abi-stable.html#abi-sys-devices-system-node-has-high-memory)
* [/sys/devices/system/node/nodeX](abi-stable.html#abi-sys-devices-system-node-nodex)
* [/sys/devices/system/node/nodeX/cpumap](abi-stable.html#abi-sys-devices-system-node-nodex-cpumap)
* [/sys/devices/system/node/nodeX/cpulist](abi-stable.html#abi-sys-devices-system-node-nodex-cpulist)
* [/sys/devices/system/node/nodeX/meminfo](abi-stable.html#abi-sys-devices-system-node-nodex-meminfo)
* [/sys/devices/system/node/nodeX/numastat](abi-stable.html#abi-sys-devices-system-node-nodex-numastat)
* [/sys/devices/system/node/nodeX/distance](abi-stable.html#abi-sys-devices-system-node-nodex-distance)
* [/sys/devices/system/node/nodeX/vmstat](abi-stable.html#abi-sys-devices-system-node-nodex-vmstat)
* [/sys/devices/system/node/nodeX/compact](abi-stable.html#abi-sys-devices-system-node-nodex-compact)
* [/sys/devices/system/node/nodeX/hugepages/hugepages-<size>/](abi-stable.html#abi-sys-devices-system-node-nodex-hugepages-hugepages-size)
* [/sys/devices/system/node/nodeX/accessY/](abi-stable.html#abi-sys-devices-system-node-nodex-accessy)
* [/sys/devices/system/node/nodeX/accessY/initiators/](abi-stable.html#abi-sys-devices-system-node-nodex-accessy-initiators)
* [/sys/devices/system/node/nodeX/accessY/targets/](abi-stable.html#abi-sys-devices-system-node-nodex-accessy-targets)
* [/sys/devices/system/node/nodeX/accessY/initiators/read\_bandwidth](abi-stable.html#abi-sys-devices-system-node-nodex-accessy-initiators-read-bandwidth)
* [/sys/devices/system/node/nodeX/accessY/initiators/read\_latency](abi-stable.html#abi-sys-devices-system-node-nodex-accessy-initiators-read-latency)
* [/sys/devices/system/node/nodeX/accessY/initiators/write\_bandwidth](abi-stable.html#abi-sys-devices-system-node-nodex-accessy-initiators-write-bandwidth)
* [/sys/devices/system/node/nodeX/accessY/initiators/write\_latency](abi-stable.html#abi-sys-devices-system-node-nodex-accessy-initiators-write-latency)
* [/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/](abi-stable.html#abi-sys-devices-system-node-nodex-memory-side-cache-indexy)
* [/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/indexing](abi-stable.html#abi-sys-devices-system-node-nodex-memory-side-cache-indexy-indexing)
* [/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/line\_size](abi-stable.html#abi-sys-devices-system-node-nodex-memory-side-cache-indexy-line-size)
* [/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/size](abi-stable.html#abi-sys-devices-system-node-nodex-memory-side-cache-indexy-size)
* [/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/write\_policy](abi-stable.html#abi-sys-devices-system-node-nodex-memory-side-cache-indexy-write-policy)
* [/sys/devices/system/node/nodeX/memory\_side\_cache/indexY/address\_mode](abi-stable.html#abi-sys-devices-system-node-nodex-memory-side-cache-indexy-address-mode)
* [/sys/devices/system/node/nodeX/x86/sgx\_total\_bytes](abi-stable.html#abi-sys-devices-system-node-nodex-x86-sgx-total-bytes)
* [/sys/devices/system/node/nodeX/memory\_failure/total](abi-stable.html#abi-sys-devices-system-node-nodex-memory-failure-total)
* [/sys/devices/system/node/nodeX/memory\_failure/ignored](abi-stable.html#abi-sys-devices-system-node-nodex-memory-failure-ignored)
* [/sys/devices/system/node/nodeX/memory\_failure/failed](abi-stable.html#abi-sys-devices-system-node-nodex-memory-failure-failed)
* [/sys/devices/system/node/nodeX/memory\_failure/delayed](abi-stable.html#abi-sys-devices-system-node-nodex-memory-failure-delayed)
* [/sys/devices/system/node/nodeX/memory\_failure/recovered](abi-stable.html#abi-sys-devices-system-node-nodex-memory-failure-recovered)
* [/sys/devices/system/node/nodeX/reclaim](abi-stable.html#abi-sys-devices-system-node-nodex-reclaim)

## ABI file stable/sysfs-devices-system-cpu

Has the following ABI:

* [/sys/devices/system/cpu/dscr\_default](abi-stable.html#abi-sys-devices-system-cpu-dscr-default)
* [/sys/devices/system/cpu/cpu[0-9]+/dscr](abi-stable.html#abi-sys-devices-system-cpu-cpu-0-9-dscr)
* [/sys/devices/system/cpu/cpuX/topology/die\_id](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-die-id)
* [/sys/devices/system/cpu/cpuX/topology/core\_id](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-core-id)
* [/sys/devices/system/cpu/cpuX/topology/cluster\_id](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-cluster-id)
* [/sys/devices/system/cpu/cpuX/topology/book\_id](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-book-id)
* [/sys/devices/system/cpu/cpuX/topology/drawer\_id](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-drawer-id)
* [/sys/devices/system/cpu/cpuX/topology/core\_cpus](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-core-cpus)
* [/sys/devices/system/cpu/cpuX/topology/core\_cpus\_list](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-core-cpus-list)
* [/sys/devices/system/cpu/cpuX/topology/package\_cpus](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-package-cpus)
* [/sys/devices/system/cpu/cpuX/topology/package\_cpus\_list](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-package-cpus-list)
* [/sys/devices/system/cpu/cpuX/topology/die\_cpus](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-die-cpus)
* [/sys/devices/system/cpu/cpuX/topology/die\_cpus\_list](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-die-cpus-list)
* [/sys/devices/system/cpu/cpuX/topology/cluster\_cpus](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-cluster-cpus)
* [/sys/devices/system/cpu/cpuX/topology/cluster\_cpus\_list](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-cluster-cpus-list)
* [/sys/devices/system/cpu/cpuX/topology/book\_siblings](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-book-siblings)
* [/sys/devices/system/cpu/cpuX/topology/book\_siblings\_list](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-book-siblings-list)
* [/sys/devices/system/cpu/cpuX/topology/drawer\_siblings](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-drawer-siblings)
* [/sys/devices/system/cpu/cpuX/topology/drawer\_siblings\_list](abi-stable.html#abi-sys-devices-system-cpu-cpux-topology-drawer-siblings-list)

## ABI file stable/sysfs-devices-system-xen\_memory

Has the following ABI:

* [/sys/devices/system/xen\_memory/xen\_memory0/max\_retry\_count](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-max-retry-count)
* [/sys/devices/system/xen\_memory/xen\_memory0/max\_schedule\_delay](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-max-schedule-delay)
* [/sys/devices/system/xen\_memory/xen\_memory0/retry\_count](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-retry-count)
* [/sys/devices/system/xen\_memory/xen\_memory0/schedule\_delay](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-schedule-delay)
* [/sys/devices/system/xen\_memory/xen\_memory0/target](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-target)
* [/sys/devices/system/xen\_memory/xen\_memory0/target\_kb](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-target-kb)
* [/sys/devices/system/xen\_memory/xen\_memory0/info/current\_kb](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-info-current-kb)
* [/sys/devices/system/xen\_memory/xen\_memory0/info/high\_kb](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-info-high-kb)
* [/sys/devices/system/xen\_memory/xen\_memory0/info/low\_kb](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-info-low-kb)
* [/sys/devices/system/xen\_memory/xen\_memory0/scrub\_pages](abi-stable.html#abi-sys-devices-system-xen-memory-xen-memory0-scrub-pages)

## ABI file stable/sysfs-driver-aspeed-vuart

Has the following ABI:

* [/sys/bus/platform/drivers/aspeed-vuart/\*/lpc\_address](abi-stable.html#abi-sys-bus-platform-drivers-aspeed-vuart-lpc-address)
* [/sys/bus/platform/drivers/aspeed-vuart/\*/sirq](abi-stable.html#abi-sys-bus-platform-drivers-aspeed-vuart-sirq)
* [/sys/bus/platform/drivers/aspeed-vuart/\*/sirq\_polarity](abi-stable.html#abi-sys-bus-platform-drivers-aspeed-vuart-sirq-polarity)

## ABI file stable/sysfs-driver-dma-idxd

Has the following ABI:

* [/sys/bus/dsa/devices/dsa<m>/version](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-version)
* [/sys/bus/dsa/devices/dsa<m>/cdev\_major](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-cdev-major)
* [/sys/bus/dsa/devices/dsa<m>/errors](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-errors)
* [/sys/bus/dsa/devices/dsa<m>/max\_batch\_size](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-batch-size)
* [/sys/bus/dsa/devices/dsa<m>/max\_work\_queues\_size](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-work-queues-size)
* [/sys/bus/dsa/devices/dsa<m>/max\_engines](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-engines)
* [/sys/bus/dsa/devices/dsa<m>/max\_groups](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-groups)
* [/sys/bus/dsa/devices/dsa<m>/max\_read\_buffers](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-read-buffers)
* [/sys/bus/dsa/devices/dsa<m>/max\_transfer\_size](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-transfer-size)
* [/sys/bus/dsa/devices/dsa<m>/max\_work\_queues](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-max-work-queues)
* [/sys/bus/dsa/devices/dsa<m>/numa\_node](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-numa-node)
* [/sys/bus/dsa/devices/dsa<m>/op\_cap](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-op-cap)
* [/sys/bus/dsa/devices/dsa<m>/pasid\_enabled](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-pasid-enabled)
* [/sys/bus/dsa/devices/dsa<m>/state](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-state)
* [/sys/bus/dsa/devices/dsa<m>/group<m>.<n>](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-group-m-n)
* [/sys/bus/dsa/devices/dsa<m>/engine<m>.<n>](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-engine-m-n)
* [/sys/bus/dsa/devices/dsa<m>/wq<m>.<n>](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-wq-m-n)
* [/sys/bus/dsa/devices/dsa<m>/configurable](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-configurable)
* [/sys/bus/dsa/devices/dsa<m>/read\_buffer\_limit](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-read-buffer-limit)
* [/sys/bus/dsa/devices/dsa<m>/cmd\_status](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-cmd-status)
* [/sys/bus/dsa/devices/dsa<m>/dsacaps](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-dsacaps)
* [/sys/bus/dsa/devices/dsa<m>/iaa\_cap](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-iaa-cap)
* [/sys/bus/dsa/devices/dsa<m>/event\_log\_size](abi-stable.html#abi-sys-bus-dsa-devices-dsa-m-event-log-size)
* [/sys/bus/dsa/devices/wq<m>.<n>/block\_on\_fault](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-block-on-fault)
* [/sys/bus/dsa/devices/wq<m>.<n>/group\_id](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-group-id)
* [/sys/bus/dsa/devices/wq<m>.<n>/size](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-size)
* [/sys/bus/dsa/devices/wq<m>.<n>/type](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-type)
* [/sys/bus/dsa/devices/wq<m>.<n>/cdev\_minor](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-cdev-minor)
* [/sys/bus/dsa/devices/wq<m>.<n>/mode](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-mode)
* [/sys/bus/dsa/devices/wq<m>.<n>/priority](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-priority)
* [/sys/bus/dsa/devices/wq<m>.<n>/state](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-state)
* [/sys/bus/dsa/devices/wq<m>.<n>/threshold](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-threshold)
* [/sys/bus/dsa/devices/wq<m>.<n>/max\_transfer\_size](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-max-transfer-size)
* [/sys/bus/dsa/devices/wq<m>.<n>/max\_batch\_size](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-max-batch-size)
* [/sys/bus/dsa/devices/wq<m>.<n>/ats\_disable](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-ats-disable)
* [/sys/bus/dsa/devices/wq<m>.<n>/prs\_disable](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-prs-disable)
* [/sys/bus/dsa/devices/wq<m>.<n>/occupancy](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-occupancy)
* [/sys/bus/dsa/devices/wq<m>.<n>/enqcmds\_retries](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-enqcmds-retries)
* [/sys/bus/dsa/devices/wq<m>.<n>/op\_config](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-op-config)
* [/sys/bus/dsa/devices/wq<m>.<n>/driver\_name](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-driver-name)
* [/sys/bus/dsa/devices/engine<m>.<n>/group\_id](abi-stable.html#abi-sys-bus-dsa-devices-engine-m-n-group-id)
* [/sys/bus/dsa/devices/group<m>.<n>/use\_read\_buffer\_limit](abi-stable.html#abi-sys-bus-dsa-devices-group-m-n-use-read-buffer-limit)
* [/sys/bus/dsa/devices/group<m>.<n>/read\_buffers\_allowed](abi-stable.html#abi-sys-bus-dsa-devices-group-m-n-read-buffers-allowed)
* [/sys/bus/dsa/devices/group<m>.<n>/read\_buffers\_reserved](abi-stable.html#abi-sys-bus-dsa-devices-group-m-n-read-buffers-reserved)
* [/sys/bus/dsa/devices/group<m>.<n>/desc\_progress\_limit](abi-stable.html#abi-sys-bus-dsa-devices-group-m-n-desc-progress-limit)
* [/sys/bus/dsa/devices/group<m>.<n>/batch\_progress\_limit](abi-stable.html#abi-sys-bus-dsa-devices-group-m-n-batch-progress-limit)
* [/sys/bus/dsa/devices/wq<m>.<n>/dsa<x>\!wq<m>.<n>/file<y>/cr\_faults](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-dsa-x-wq-m-n-file-y-cr-faults)
* [/sys/bus/dsa/devices/wq<m>.<n>/dsa<x>\!wq<m>.<n>/file<y>/cr\_fault\_failures](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-dsa-x-wq-m-n-file-y-cr-fault-failures)
* [/sys/bus/dsa/devices/wq<m>.<n>/dsa<x>\!wq<m>.<n>/file<y>/pid](abi-stable.html#abi-sys-bus-dsa-devices-wq-m-n-dsa-x-wq-m-n-file-y-pid)

## ABI file stable/sysfs-driver-dma-ioatdma

Has the following ABI:

* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/cap](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-dma-dma-n-chan-n-quickdata-cap)
* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/ring\_active](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-dma-dma-n-chan-n-quickdata-ring-active)
* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/ring\_size](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-dma-dma-n-chan-n-quickdata-ring-size)
* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/version](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-dma-dma-n-chan-n-quickdata-version)
* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/dma/dma<n>chan<n>/quickdata/intr\_coalesce](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-dma-dma-n-chan-n-quickdata-intr-coalesce)

## ABI file stable/sysfs-driver-firmware-zynqmp

Has the following ABI:

* [/sys/devices/platform/firmware\:zynqmp-firmware/ggs\*](abi-stable.html#abi-sys-devices-platform-firmware-zynqmp-firmware-ggs)
* [/sys/devices/platform/firmware\:zynqmp-firmware/pggs\*](abi-stable.html#abi-sys-devices-platform-firmware-zynqmp-firmware-pggs)
* [/sys/devices/platform/firmware\:zynqmp-firmware/shutdown\_scope](abi-stable.html#abi-sys-devices-platform-firmware-zynqmp-firmware-shutdown-scope)
* [/sys/devices/platform/firmware\:zynqmp-firmware/health\_status](abi-stable.html#abi-sys-devices-platform-firmware-zynqmp-firmware-health-status)
* [/sys/devices/platform/firmware\:zynqmp-firmware/feature\_config\_id](abi-stable.html#abi-sys-devices-platform-firmware-zynqmp-firmware-feature-config-id)
* [/sys/devices/platform/firmware\:zynqmp-firmware/feature\_config\_value](abi-stable.html#abi-sys-devices-platform-firmware-zynqmp-firmware-feature-config-value)

## ABI file stable/sysfs-driver-ib\_srp

Has the following ABI:

* [/sys/class/infiniband\_srp/srp-<hca>-<port\_number>/add\_target](abi-stable.html#abi-sys-class-infiniband-srp-srp-hca-port-number-add-target)
* [/sys/class/infiniband\_srp/srp-<hca>-<port\_number>/ibdev](abi-stable.html#abi-sys-class-infiniband-srp-srp-hca-port-number-ibdev)
* [/sys/class/infiniband\_srp/srp-<hca>-<port\_number>/port](abi-stable.html#abi-sys-class-infiniband-srp-srp-hca-port-number-port)
* [/sys/class/scsi\_host/host<n>/allow\_ext\_sg](abi-stable.html#abi-sys-class-scsi-host-host-n-allow-ext-sg)
* [/sys/class/scsi\_host/host<n>/ch\_count](abi-stable.html#abi-sys-class-scsi-host-host-n-ch-count)
* [/sys/class/scsi\_host/host<n>/cmd\_sg\_entries](abi-stable.html#abi-sys-class-scsi-host-host-n-cmd-sg-entries)
* [/sys/class/scsi\_host/host<n>/comp\_vector](abi-stable.html#abi-sys-class-scsi-host-host-n-comp-vector)
* [/sys/class/scsi\_host/host<n>/dgid](abi-stable.html#abi-sys-class-scsi-host-host-n-dgid)
* [/sys/class/scsi\_host/host<n>/id\_ext](abi-stable.html#abi-sys-class-scsi-host-host-n-id-ext)
* [/sys/class/scsi\_host/host<n>/ioc\_guid](abi-stable.html#abi-sys-class-scsi-host-host-n-ioc-guid)
* [/sys/class/scsi\_host/host<n>/local\_ib\_device](abi-stable.html#abi-sys-class-scsi-host-host-n-local-ib-device)
* [/sys/class/scsi\_host/host<n>/local\_ib\_port](abi-stable.html#abi-sys-class-scsi-host-host-n-local-ib-port)
* [/sys/class/scsi\_host/host<n>/orig\_dgid](abi-stable.html#abi-sys-class-scsi-host-host-n-orig-dgid)
* [/sys/class/scsi\_host/host<n>/pkey](abi-stable.html#abi-sys-class-scsi-host-host-n-pkey)
* [/sys/class/scsi\_host/host<n>/req\_lim](abi-stable.html#abi-sys-class-scsi-host-host-n-req-lim)
* [/sys/class/scsi\_host/host<n>/service\_id](abi-stable.html#abi-sys-class-scsi-host-host-n-service-id)
* [/sys/class/scsi\_host/host<n>/sgid](abi-stable.html#abi-sys-class-scsi-host-host-n-sgid)
* [/sys/class/scsi\_host/host<n>/zero\_req\_lim](abi-stable.html#abi-sys-class-scsi-host-host-n-zero-req-lim)

## ABI file stable/sysfs-driver-misc-cp500

Has the following ABI:

* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/0000:XX:XX.X/version](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-0000-xx-xx-x-version)
* [/sys/devices/pciXXXX:XX/0000:XX:XX.X/0000:XX:XX.X/keep\_cfg](abi-stable.html#abi-sys-devices-pcixxxx-xx-0000-xx-xx-x-0000-xx-xx-x-keep-cfg)

## ABI file stable/sysfs-driver-mlxreg-io

Has the following ABI:

* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic\_health](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-asic-health)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld1\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-version)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld2\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-version)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/fan\_dir](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-fan-dir)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld3\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld3-version)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/jtag\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-jtag-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/select\_iio](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-select-iio)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu1\_on](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-psu1-on)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_aux\_pwr\_or\_ref](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_asic\_thermal](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_hotswap\_or\_halt](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_hotswap\_or\_wd](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_fw\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_long\_pb](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_main\_pwr\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_short\_pb](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_sw\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_comex\_pwr\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_from\_comex](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_system](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_voltmon\_upgrade\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld4\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld4-version)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_comex\_thermal](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-thermal)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_comex\_wd](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-thermal)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_from\_asic](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-thermal)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_reload\_bios](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-thermal)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_sff\_wd](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-thermal)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_swb\_wd](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-comex-thermal)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/config1](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-config1)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/config2](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-config1)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_ac\_pwr\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-ac-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_platform](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-ac-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_soc](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-ac-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_sw\_pwr\_off](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-ac-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pcie\_asic\_reset\_dis](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-pcie-asic-reset-dis)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/vpd\_wp](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-vpd-wp)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/voltreg\_update\_status](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-voltreg-update-status)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/ufm\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-ufm-version)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld1\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld2\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld3\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld4\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld1\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld2\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld3\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld4\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/bios\_active\_image](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-bios-active-image)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/bios\_auth\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-bios-active-image)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/bios\_upgrade\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-bios-active-image)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc1\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc2\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc3\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc4\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc5\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc6\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc7\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc8\_enable](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-enable)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc1\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc2\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc3\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc4\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc5\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc6\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc7\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc8\_pwr](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-pwr)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc1\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc2\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc3\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc4\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc5\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc6\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc7\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lc8\_rst\_mask](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lc1-rst-mask)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/os\_started](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-os-started)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pm\_mgmt\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-pm-mgmt-en)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu3\_on](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-psu3-on)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/psu4\_on](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-psu3-on)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/shutdown\_unlock](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-shutdown-unlock)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld1\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld1\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld1\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-cpld1-pn)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga1\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-fpga1-pn)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga1\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-fpga1-pn)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga1\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-fpga1-pn)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/vpd\_wp](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-vpd-wp)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_aux\_pwr\_or\_ref](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_dc\_dc\_pwr\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_fpga\_not\_done](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_from\_chassis](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_line\_card](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/reset\_pwr\_off\_from\_chassis](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-ref)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/cpld\_upgrade\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-cpld-upgrade-en)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga\_upgrade\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-cpld-upgrade-en)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/qsfp\_pwr\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-qsfp-pwr-en)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/pwr\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-qsfp-pwr-en)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/agb\_spi\_burn\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-agb-spi-burn-en)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/fpga\_spi\_burn\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-agb-spi-burn-en)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/max\_power](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-max-power)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/i2c-\*/\*-0032/mlxreg-io.\*/hwmon/hwmon\*/config](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-i2c-0032-mlxreg-io-hwmon-hwmon-max-power)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/phy\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-phy-reset)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/mac\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-mac-reset)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/qsfp\_pwr\_good](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-qsfp-pwr-good)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic2\_health](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-asic2-health)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-asic-reset)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic2\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-asic-reset)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/comm\_chnl\_ready](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-comm-chnl-ready)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/config3](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-config3)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_pwr\_converter\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-pwr-converter-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_ap\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-ap-reset)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_ap\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-ap-reset)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_recovery](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-recovery)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_recovery](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-recovery)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-recovery)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_reset](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-recovery)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot1\_wp](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-wp)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/erot2\_wp](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-erot1-wp)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/spi\_chnl\_select](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-spi-chnl-select)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/asic\_pg\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-asic-pg-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd1\_boot\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-clk-brd1-boot-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd2\_boot\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-clk-brd1-boot-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-clk-brd1-boot-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/clk\_brd\_prog\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-clk-brd-prog-en)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/pwr\_converter\_prog\_en](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-pwr-converter-prog-en)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_ac\_ok\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-ac-ok-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld5\_pn](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld5-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld5\_version](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld5-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/cpld5\_version\_min](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-cpld5-pn)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/jtag\_cap](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-jtag-cap)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/lid\_open](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-lid-open)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_long\_pwr\_pb](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-long-pwr-pb)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/reset\_swb\_dc\_dc\_pwr\_fail](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-reset-swb-dc-dc-pwr-fail)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/global\_wp\_request](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-global-wp-request)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/global\_wp\_response](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-global-wp-response)
* [/sys/devices/platform/mlxplat/mlxreg-io/hwmon/hwmon\*/shutdown\_unlock](abi-stable.html#abi-sys-devices-platform-mlxplat-mlxreg-io-hwmon-hwmon-shutdown-unlocko)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/boot\_progress](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-boot-progress)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/dpu\_id](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-dpu-id)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/reset\_aux\_pwr\_or\_reload](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-reload)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/reset\_dpu\_thermal](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-reload)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/reset\_from\_main\_board](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-reset-aux-pwr-or-reload)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/perst\_rst](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-perst-rst)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/phy\_rst](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-perst-rst)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/tpm\_rst](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-perst-rst)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/usbphy\_rst](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-perst-rst)
* [/sys/devices/platform/mlxplat/i2c\_mlxcpld.\*/i2c-\*/i2c-\*/\*-00\*\*/mlxreg-io.\*/hwmon/hwmon\*/ufm\_upgrade](abi-stable.html#abi-sys-devices-platform-mlxplat-i2c-mlxcpld-i2c-i2c-00-mlxreg-io-hwmon-hwmon-ufm-upgrade)

## ABI file stable/sysfs-driver-qaic

Has the following ABI:

* [/sys/bus/pci/drivers/qaic/XXXX:XX:XX.X/accel/accel<minor\_nr>/dbc<N>\_state](abi-stable.html#abi-sys-bus-pci-drivers-qaic-xxxx-xx-xx-x-accel-accel-minor-nr-dbc-n-state)

## ABI file stable/sysfs-driver-qla2xxx

Has the following ABI:

* [/sys/bus/pci/drivers/qla2xxx/.../devices/\*](abi-stable.html#abi-sys-bus-pci-drivers-qla2xxx-devices)

## ABI file stable/sysfs-driver-speakup

Has the following ABI:

* [/sys/accessibility/speakup/attrib\_bleep](abi-stable.html#abi-sys-accessibility-speakup-attrib-bleep)
* [/sys/accessibility/speakup/bell\_pos](abi-stable.html#abi-sys-accessibility-speakup-bell-pos)
* [/sys/accessibility/speakup/bleeps](abi-stable.html#abi-sys-accessibility-speakup-bleeps)
* [/sys/accessibility/speakup/bleep\_time](abi-stable.html#abi-sys-accessibility-speakup-bleep-time)
* [/sys/accessibility/speakup/cursor\_time](abi-stable.html#abi-sys-accessibility-speakup-cursor-time)
* [/sys/accessibility/speakup/cur\_phonetic](abi-stable.html#abi-sys-accessibility-speakup-cur-phonetic)
* [/sys/accessibility/speakup/delimiters](abi-stable.html#abi-sys-accessibility-speakup-delimiters)
* [/sys/accessibility/speakup/ex\_num](abi-stable.html#abi-sys-accessibility-speakup-ex-num)
* [/sys/accessibility/speakup/key\_echo](abi-stable.html#abi-sys-accessibility-speakup-key-echo)
* [/sys/accessibility/speakup/keymap](abi-stable.html#abi-sys-accessibility-speakup-keymap)
* [/sys/accessibility/speakup/no\_interrupt](abi-stable.html#abi-sys-accessibility-speakup-no-interrupt)
* [/sys/accessibility/speakup/punc\_all](abi-stable.html#abi-sys-accessibility-speakup-punc-all)
* [/sys/accessibility/speakup/punc\_level](abi-stable.html#abi-sys-accessibility-speakup-punc-level)
* [/sys/accessibility/speakup/punc\_most](abi-stable.html#abi-sys-accessibility-speakup-punc-most)
* [/sys/accessibility/speakup/punc\_some](abi-stable.html#abi-sys-accessibility-speakup-punc-some)
* [/sys/accessibility/speakup/reading\_punc](abi-stable.html#abi-sys-accessibility-speakup-reading-punc)
* [/sys/accessibility/speakup/repeats](abi-stable.html#abi-sys-accessibility-speakup-repeats)
* [/sys/accessibility/speakup/say\_control](abi-stable.html#abi-sys-accessibility-speakup-say-control)
* [/sys/accessibility/speakup/say\_word\_ctl](abi-stable.html#abi-sys-accessibility-speakup-say-word-ctl)
* [/sys/accessibility/speakup/silent](abi-stable.html#abi-sys-accessibility-speakup-silent)
* [/sys/accessibility/speakup/spell\_delay](abi-stable.html#abi-sys-accessibility-speakup-spell-delay)
* [/sys/accessibility/speakup/synth](abi-stable.html#abi-sys-accessibility-speakup-synth)
* [/sys/accessibility/speakup/synth\_direct](abi-stable.html#abi-sys-accessibility-speakup-synth-direct)
* [/sys/accessibility/speakup/version](abi-stable.html#abi-sys-accessibility-speakup-version)
* [/sys/accessibility/speakup/i18n/announcements](abi-stable.html#abi-sys-accessibility-speakup-i18n-announcements)
* [/sys/accessibility/speakup/i18n/chartab](abi-stable.html#abi-sys-accessibility-speakup-i18n-chartab)
* [/sys/accessibility/speakup/i18n/ctl\_keys](abi-stable.html#abi-sys-accessibility-speakup-i18n-ctl-keys)
* [/sys/accessibility/speakup/i18n/function\_names](abi-stable.html#abi-sys-accessibility-speakup-i18n-function-names)
* [/sys/accessibility/speakup/i18n/states](abi-stable.html#abi-sys-accessibility-speakup-i18n-states)
* [/sys/accessibility/speakup/i18n/characters](abi-stable.html#abi-sys-accessibility-speakup-i18n-characters)
* [/sys/accessibility/speakup/i18n/colors](abi-stable.html#abi-sys-accessibility-speakup-i18n-colors)
* [/sys/accessibility/speakup/i18n/formatted](abi-stable.html#abi-sys-accessibility-speakup-i18n-formatted)
* [/sys/accessibility/speakup/i18n/key\_names](abi-stable.html#abi-sys-accessibility-speakup-i18n-key-names)
* [/sys/accessibility/speakup/<synth-name>/](abi-stable.html#abi-sys-accessibility-speakup-synth-name)
* [/sys/accessibility/speakup/<synth-name>/caps\_start](abi-stable.html#abi-sys-accessibility-speakup-synth-name-caps-start)
* [/sys/accessibility/speakup/<synth-name>/caps\_stop](abi-stable.html#abi-sys-accessibility-speakup-synth-name-caps-stop)
* [/sys/accessibility/speakup/<synth-name>/delay\_time](abi-stable.html#abi-sys-accessibility-speakup-synth-name-delay-time)
* [/sys/accessibility/speakup/<synth-name>/direct](abi-stable.html#abi-sys-accessibility-speakup-synth-name-direct)
* [/sys/accessibility/speakup/<synth-name>/freq](abi-stable.html#abi-sys-accessibility-speakup-synth-name-freq)
* [/sys/accessibility/speakup/<synth-name>/flush\_time](abi-stable.html#abi-sys-accessibility-speakup-synth-name-flush-time)
* [/sys/accessibility/speakup/<synth-name>/full\_time](abi-stable.html#abi-sys-accessibility-speakup-synth-name-full-time)
* [/sys/accessibility/speakup/<synth-name>/jiffy\_delta](abi-stable.html#abi-sys-accessibility-speakup-synth-name-jiffy-delta)
* [/sys/accessibility/speakup/<synth-name>/pitch](abi-stable.html#abi-sys-accessibility-speakup-synth-name-pitch)
* [/sys/accessibility/speakup/<synth-name>/inflection](abi-stable.html#abi-sys-accessibility-speakup-synth-name-inflection)
* [/sys/accessibility/speakup/<synth-name>/punct](abi-stable.html#abi-sys-accessibility-speakup-synth-name-punct)
* [/sys/accessibility/speakup/<synth-name>/rate](abi-stable.html#abi-sys-accessibility-speakup-synth-name-rate)
* [/sys/accessibility/speakup/<synth-name>/tone](abi-stable.html#abi-sys-accessibility-speakup-synth-name-tone)
* [/sys/accessibility/speakup/<synth-name>/trigger\_time](abi-stable.html#abi-sys-accessibility-speakup-synth-name-trigger-time)
* [/sys/accessibility/speakup/<synth-name>/voice](abi-stable.html#abi-sys-accessibility-speakup-synth-name-voice)
* [/sys/accessibility/speakup/<synth-name>/vol](abi-stable.html#abi-sys-accessibility-speakup-synth-name-vol)

## ABI file stable/sysfs-driver-usb-usbtmc

Has the following ABI:

* [/sys/bus/usb/drivers/usbtmc/\*/interface\_capabilities](abi-stable.html#abi-sys-bus-usb-drivers-usbtmc-interface-capabilities)
* [/sys/bus/usb/drivers/usbtmc/\*/device\_capabilities](abi-stable.html#abi-sys-bus-usb-drivers-usbtmc-interface-capabilities)
* [/sys/bus/usb/drivers/usbtmc/\*/usb488\_interface\_capabilities](abi-stable.html#abi-sys-bus-usb-drivers-usbtmc-usb488-interface-capabilities)
* [/sys/bus/usb/drivers/usbtmc/\*/usb488\_device\_capabilities](abi-stable.html#abi-sys-bus-usb-drivers-usbtmc-usb488-interface-capabilities)

## ABI file stable/sysfs-driver-w1\_ds2438

Has the following ABI:

* [/sys/bus/w1/devices/.../page1](abi-stable.html#abi-sys-bus-w1-devices-page1)
* [/sys/bus/w1/devices/.../offset](abi-stable.html#abi-sys-bus-w1-devices-offset)

## ABI file stable/sysfs-driver-w1\_ds28e04

Has the following ABI:

* [/sys/bus/w1/devices/.../pio](abi-stable.html#abi-sys-bus-w1-devices-pio)
* [/sys/bus/w1/devices/.../eeprom](abi-stable.html#abi-sys-bus-w1-devices-eeprom)

## ABI file stable/sysfs-driver-w1\_ds28ea00

Has the following ABI:

* [/sys/bus/w1/devices/.../w1\_seq](abi-stable.html#abi-sys-bus-w1-devices-w1-seq)

## ABI file stable/sysfs-firmware-opal-dump

Has the following ABI:

* [/sys/firmware/opal/dump](abi-stable.html#abi-sys-firmware-opal-dump)

## ABI file stable/sysfs-firmware-opal-elog

Has the following ABI:

* [/sys/firmware/opal/elog](abi-stable.html#abi-sys-firmware-opal-elog)

## ABI file stable/sysfs-fs-orangefs

Has the following ABI:

* [/sys/fs/orangefs/perf\_counters/\*](abi-stable.html#abi-sys-fs-orangefs-perf-counters)
* [/sys/fs/orangefs/perf\_counter\_reset](abi-stable.html#abi-sys-fs-orangefs-perf-counter-reset)
* [/sys/fs/orangefs/perf\_time\_interval\_secs](abi-stable.html#abi-sys-fs-orangefs-perf-time-interval-secs)
* [/sys/fs/orangefs/perf\_history\_size](abi-stable.html#abi-sys-fs-orangefs-perf-history-size)
* [/sys/fs/orangefs/op\_timeout\_secs](abi-stable.html#abi-sys-fs-orangefs-op-timeout-secs)
* [/sys/fs/orangefs/slot\_timeout\_secs](abi-stable.html#abi-sys-fs-orangefs-slot-timeout-secs)
* [/sys/fs/orangefs/acache/\*](abi-stable.html#abi-sys-fs-orangefs-acache)
* [/sys/fs/orangefs/ncache/\*](abi-stable.html#abi-sys-fs-orangefs-ncache)
* [/sys/fs/orangefs/capcache/\*](abi-stable.html#abi-sys-fs-orangefs-capcache)
* [/sys/fs/orangefs/ccache/\*](abi-stable.html#abi-sys-fs-orangefs-ccache)

## ABI file stable/sysfs-hypervisor-xen

Has the following ABI:

* [/sys/hypervisor/compilation/compile\_date](abi-stable.html#abi-sys-hypervisor-compilation-compile-date)
* [/sys/hypervisor/compilation/compiled\_by](abi-stable.html#abi-sys-hypervisor-compilation-compiled-by)
* [/sys/hypervisor/compilation/compiler](abi-stable.html#abi-sys-hypervisor-compilation-compiler)
* [/sys/hypervisor/properties/capabilities](abi-stable.html#abi-sys-hypervisor-properties-capabilities)
* [/sys/hypervisor/properties/changeset](abi-stable.html#abi-sys-hypervisor-properties-changeset)
* [/sys/hypervisor/properties/features](abi-stable.html#abi-sys-hypervisor-properties-features)
* [/sys/hypervisor/properties/pagesize](abi-stable.html#abi-sys-hypervisor-properties-pagesize)
* [/sys/hypervisor/properties/virtual\_start](abi-stable.html#abi-sys-hypervisor-properties-virtual-start)
* [/sys/hypervisor/type](abi-stable.html#abi-sys-hypervisor-type)
* [/sys/hypervisor/uuid](abi-stable.html#abi-sys-hypervisor-uuid)
* [/sys/hypervisor/version/extra](abi-stable.html#abi-sys-hypervisor-version-extra)
* [/sys/hypervisor/version/major](abi-stable.html#abi-sys-hypervisor-version-major)
* [/sys/hypervisor/version/minor](abi-stable.html#abi-sys-hypervisor-version-minor)
* [/sys/hypervisor/start\_flags/\*](abi-stable.html#abi-sys-hypervisor-start-flags)

## ABI file stable/sysfs-kernel-notes

Has the following ABI:

* [/sys/kernel/notes](abi-stable.html#abi-sys-kernel-notes)

## ABI file stable/sysfs-kernel-time-aux-clocks

Has the following ABI:

* [/sys/kernel/time/aux\_clocks/<ID>/enable](abi-stable.html#abi-sys-kernel-time-aux-clocks-id-enable)

## ABI file stable/sysfs-module

The /sys/module tree consists of the following structure:

Has the following ABI:

* [/sys/module/<MODULENAME>](abi-stable.html#abi-sys-module-modulename)
* [/sys/module/<MODULENAME>/parameters](abi-stable.html#abi-sys-module-modulename-parameters)
* [/sys/module/<MODULENAME>/refcnt](abi-stable.html#abi-sys-module-modulename-refcnt)
* [/sys/module/<MODULENAME>/srcversion](abi-stable.html#abi-sys-module-modulename-srcversion)
* [/sys/module/<MODULENAME>/version](abi-stable.html#abi-sys-module-modulename-version)

## ABI file stable/sysfs-platform-wmi-bmof

Has the following ABI:

* [/sys/bus/wmi/devices/05901221-D566-11D1-B2F0-00A0C9062910[-X]/bmof](abi-stable.html#abi-sys-bus-wmi-devices-05901221-d566-11d1-b2f0-00a0c9062910-x-bmof)

## ABI file stable/sysfs-transport-srp

Has the following ABI:

* [/sys/class/srp\_remote\_ports/port-<h>:<n>/delete](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-delete)
* [/sys/class/srp\_remote\_ports/port-<h>:<n>/dev\_loss\_tmo](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-dev-loss-tmo)
* [/sys/class/srp\_remote\_ports/port-<h>:<n>/fast\_io\_fail\_tmo](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-fast-io-fail-tmo)
* [/sys/class/srp\_remote\_ports/port-<h>:<n>/port\_id](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-port-id)
* [/sys/class/srp\_remote\_ports/port-<h>:<n>/reconnect\_delay](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-reconnect-delay)
* [/sys/class/srp\_remote\_ports/port-<h>:<n>/roles](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-roles)
* [/sys/class/srp\_remote\_ports/port-<h>:<n>/state](abi-stable.html#abi-sys-class-srp-remote-ports-port-h-n-state)

## ABI file stable/thermal-notification

Has the following ABI:

* [A notification mechanism for thermal related events](abi-stable.html#abi-a-notification-mechanism-for-thermal-related-events)

## ABI file stable/vdso

Has the following ABI:

* [vDSO](abi-stable.html#abi-vdso)
